#include "Game.h"
#include "Hud.h"
#include "Settings.h"

namespace Game
{
	namespace
	{
		enum class Primary
		{
			kNone,
			kMove,     // click on the ground: the attack is swallowed
			kAttack,   // click on an enemy / attack in place: the game gets the click
			kSwallow   // click used something (item, door, NPC...)
		};

		Primary             g_primary = Primary::kNone;
		bool                g_primaryHeld = false;
		bool                g_moving = false;
		bool                g_stickSent = false;  // our last stick input was non-zero
		RE::NiPoint3        g_target;
		RE::ObjectRefHandle g_pending;            // walk to this, then activate it
		float               g_faceTime = 0.0f;
		float               g_faceX = 0.0f;
		float               g_faceY = 1.0f;
		float               g_scanTimer = 0.0f;
		float               g_stuckTimer = 0.0f;
		RE::NiPoint3        g_stuckPos;

		constexpr std::uint32_t Rgb(std::uint32_t a_r, std::uint32_t a_g, std::uint32_t a_b)
		{
			return a_r | (a_g << 8) | (a_b << 16) | (0xFFu << 24);
		}

		// -----------------------------------------------------------------------
		// Cursor -> ground
		// -----------------------------------------------------------------------
		// The cutaway hides roofs but their collision is still there, so a physics ray from an
		// overhead camera would hit an invisible ceiling. Instead the cursor is mapped onto the
		// flat plane at the player's feet. Four points on that plane are projected to the screen
		// with the game's own function, which pins down the exact screen -> plane mapping
		// (a homography) with no assumptions about the camera's axes or field of view.
		struct GroundMap
		{
			double       h[8]{};
			double       u0{ 0.0 };
			double       v0{ 0.0 };
			RE::NiPoint3 origin;
			bool         valid{ false };
		};

		bool Solve8(double a_m[8][9])
		{
			for (int c = 0; c < 8; ++c) {
				int p = c;
				for (int r = c + 1; r < 8; ++r) {
					if (std::fabs(a_m[r][c]) > std::fabs(a_m[p][c])) {
						p = r;
					}
				}
				if (std::fabs(a_m[p][c]) < 1e-12) {
					return false;
				}
				if (p != c) {
					for (int k = 0; k < 9; ++k) {
						std::swap(a_m[p][k], a_m[c][k]);
					}
				}
				for (int r = 0; r < 8; ++r) {
					if (r == c) {
						continue;
					}
					const double f = a_m[r][c] / a_m[c][c];
					if (f == 0.0) {
						continue;
					}
					for (int k = c; k < 9; ++k) {
						a_m[r][k] -= f * a_m[c][k];
					}
				}
			}
			for (int i = 0; i < 8; ++i) {
				a_m[i][8] /= a_m[i][i];
			}
			return true;
		}

		GroundMap BuildGroundMap(const RE::NiPoint3& a_origin)
		{
			GroundMap map;
			map.origin = a_origin;
			float u0 = 0.0f;
			float v0 = 0.0f;
			if (!WorldToScreen(a_origin, u0, v0)) {
				return map;
			}
			map.u0 = u0;
			map.v0 = v0;

			// Smaller squares are the fallback when a corner falls behind a very close camera.
			for (const float s : { 300.0f, 120.0f, 40.0f }) {
				const float offsets[4][2] = { { -s, -s }, { s, -s }, { s, s }, { -s, s } };
				double      m[8][9]{};
				bool        ok = true;
				for (int i = 0; i < 4; ++i) {
					const RE::NiPoint3 p{ a_origin.x + offsets[i][0], a_origin.y + offsets[i][1], a_origin.z };
					float              u = 0.0f;
					float              v = 0.0f;
					if (!WorldToScreen(p, u, v)) {
						ok = false;
						break;
					}
					const double du = u - map.u0;
					const double dv = v - map.v0;
					const double X = offsets[i][0];
					const double Y = offsets[i][1];
					double*      r0 = m[2 * i];
					double*      r1 = m[2 * i + 1];
					r0[0] = du;
					r0[1] = dv;
					r0[2] = 1.0;
					r0[6] = -X * du;
					r0[7] = -X * dv;
					r0[8] = X;
					r1[3] = du;
					r1[4] = dv;
					r1[5] = 1.0;
					r1[6] = -Y * du;
					r1[7] = -Y * dv;
					r1[8] = Y;
				}
				if (ok && Solve8(m)) {
					for (int i = 0; i < 8; ++i) {
						map.h[i] = m[i][8];
					}
					map.valid = true;
					return map;
				}
			}
			return map;
		}

		// Screen position (0..1) -> point on the ground plane, at most a_maxDist from the origin.
		// At and above the horizon there is no ground under the cursor, so it gives the point
		// a_maxDist away in that direction.
		bool GroundAt(const GroundMap& a_map, float a_u, float a_v, float a_maxDist, RE::NiPoint3& a_out)
		{
			if (!a_map.valid) {
				return false;
			}
			const double du = a_u - a_map.u0;
			const double dv = a_v - a_map.v0;
			const double X = a_map.h[0] * du + a_map.h[1] * dv + a_map.h[2];
			const double Y = a_map.h[3] * du + a_map.h[4] * dv + a_map.h[5];
			const double w = a_map.h[6] * du + a_map.h[7] * dv + 1.0;
			const double len = std::hypot(X, Y);
			a_out = a_map.origin;
			if (len < 1e-9) {
				return true;
			}
			double dist = w > 1e-4 ? len / w : static_cast<double>(a_maxDist);
			dist = std::min(dist, static_cast<double>(a_maxDist));
			a_out.x += static_cast<float>(X / len * dist);
			a_out.y += static_cast<float>(Y / len * dist);
			return true;
		}

		bool CursorGround(RE::PlayerCharacter* a_player, RE::NiPoint3& a_out)
		{
			const float w = State::screenW.load();
			const float h = State::screenH.load();
			const float cx = State::cursorX.load();
			const float cy = State::cursorY.load();
			if (w < 1.0f || h < 1.0f || cx < 0.0f || cy < 0.0f) {
				return false;
			}
			const auto map = BuildGroundMap(a_player->GetPosition());
			return GroundAt(map, cx / w, cy / h, Settings::MaxDistance, a_out);
		}

		// The ground direction that points "up the screen" (what a gamepad's stick-up means).
		bool CameraForward(RE::PlayerCharacter* a_player, float& a_x, float& a_y)
		{
			const auto   map = BuildGroundMap(a_player->GetPosition());
			RE::NiPoint3 upper;
			RE::NiPoint3 lower;
			if (!GroundAt(map, 0.5f, 0.65f, 100000.0f, upper) || !GroundAt(map, 0.5f, 0.75f, 100000.0f, lower)) {
				return false;
			}
			const float dx = upper.x - lower.x;
			const float dy = upper.y - lower.y;
			const float len = std::hypot(dx, dy);
			if (len < 1e-3f) {
				return false;
			}
			a_x = dx / len;
			a_y = dy / len;
			return true;
		}

		// -----------------------------------------------------------------------
		// Movement
		// -----------------------------------------------------------------------
		// Movement goes in as left-thumbstick input through the game's own movement handler. That
		// is the same path a gamepad uses, so True Directional Movement turns and moves the
		// character exactly as it does for a controller.
		RE::ThumbstickEvent* MakeStickEvent()
		{
			auto* userEvents = RE::UserEvents::GetSingleton();
			auto* evt = RE::malloc<RE::ThumbstickEvent>();
			if (!evt || !userEvents) {
				return nullptr;
			}
			std::memset(static_cast<void*>(evt), 0, sizeof(RE::ThumbstickEvent));
			reinterpret_cast<std::uintptr_t*>(evt)[0] = RE::VTABLE_ThumbstickEvent[0].address();
			evt->Init(RE::ThumbstickEvent::InputType::kLeftThumbstick, RE::INPUT_DEVICE::kGamepad, 0.0f, 0.0f, userEvents->move);
			evt->eventType = RE::INPUT_EVENT_TYPE::kThumbstick;
			evt->next = nullptr;
			return evt;
		}

		void SendStick(float a_x, float a_y)
		{
			const bool zero = a_x == 0.0f && a_y == 0.0f;
			if (zero && !g_stickSent) {
				return;  // nothing of ours to cancel; don't stomp on keyboard movement
			}
			auto* controls = RE::PlayerControls::GetSingleton();
			auto* handler = controls ? controls->movementHandler : nullptr;
			if (!handler) {
				return;
			}
			static RE::ThumbstickEvent* evt = MakeStickEvent();
			if (!evt) {
				return;
			}
			evt->xValue = a_x;
			evt->yValue = a_y;
			handler->ProcessThumbstick(evt, &controls->data);
			g_stickSent = !zero;
		}

		bool UseTdm()
		{
			if (Settings::MoveMode == 1) {
				return true;
			}
			if (Settings::MoveMode == 2) {
				return false;
			}
			static const bool installed = REX::W32::GetModuleHandleW(L"TrueDirectionalMovement.dll") != nullptr;
			return installed;
		}

		// Move (and turn) toward a world direction. a_x/a_y must be normalised.
		void Drive(RE::PlayerCharacter* a_player, float a_x, float a_y)
		{
			if (UseTdm()) {
				float fx = 0.0f;
				float fy = 1.0f;
				if (!CameraForward(a_player, fx, fy)) {
					return;
				}
				// Screen-right on the ground is forward turned 90 degrees clockwise (seen from above).
				const float rx = fy;
				const float ry = -fx;
				SendStick(a_x * rx + a_y * ry, a_x * fx + a_y * fy);
			} else {
				a_player->SetHeading(std::atan2(a_x, a_y));  // heading: 0 = north, clockwise
				SendStick(0.0f, 1.0f);
			}
		}

		void Halt()
		{
			g_moving = false;
			g_pending = {};
			SendStick(0.0f, 0.0f);
		}

		void StartMove(const RE::NiPoint3& a_target, RE::ObjectRefHandle a_pending, RE::PlayerCharacter* a_player)
		{
			g_target = a_target;
			g_pending = a_pending;
			g_moving = true;
			g_faceTime = 0.0f;
			g_stuckTimer = 0.0f;
			g_stuckPos = a_player->GetPosition();
		}

		void FaceToward(const RE::NiPoint3& a_point, RE::PlayerCharacter* a_player)
		{
			const auto  pos = a_player->GetPosition();
			const float dx = a_point.x - pos.x;
			const float dy = a_point.y - pos.y;
			const float len = std::hypot(dx, dy);
			g_moving = false;
			g_pending = {};
			if (len < 1.0f) {
				return;
			}
			g_faceX = dx / len;
			g_faceY = dy / len;
			g_faceTime = std::max(Settings::FaceTime, 0.01f);
		}

		// -----------------------------------------------------------------------
		// Using things
		// -----------------------------------------------------------------------
		// Where to walk to use something: its middle (a door's own position can be at its hinge).
		RE::NiPoint3 UsePoint(RE::TESObjectREFR* a_ref)
		{
			if (!a_ref->IsActor()) {
				if (auto* node = a_ref->Get3D(); node && node->worldBound.radius > 0.0f) {
					return node->worldBound.center;
				}
			}
			return a_ref->GetPosition();
		}

		void Activate(RE::TESObjectREFR* a_ref, RE::PlayerCharacter* a_player)
		{
			if (!a_ref || a_ref->IsDisabled() || a_ref->IsMarkedForDeletion()) {
				return;
			}
			auto* controlMap = RE::ControlMap::GetSingleton();
			if (controlMap && !controlMap->IsActivateControlsEnabled()) {
				return;
			}
			// The same activation the E key does: picks up items (and flags theft), opens
			// containers and doors, loots corpses, starts dialogue.
			a_ref->ActivateRef(a_player, 0, nullptr, 1, false);
			g_scanTimer = 0.0f;  // refresh the labels straight away
			if (Settings::PointClickDebugLog) {
				logs::info("[use] activated {:08X}", a_ref->GetFormID());
			}
		}

		void Interact(RE::ObjectRefHandle a_handle, RE::PlayerCharacter* a_player)
		{
			const auto ref = a_handle.get();
			if (!ref) {
				return;
			}
			const auto  pos = a_player->GetPosition();
			const auto  at = UsePoint(ref.get());
			const float dist = std::hypot(at.x - pos.x, at.y - pos.y);
			if (dist <= Settings::PickupRange) {
				Halt();
				Activate(ref.get(), a_player);
			} else {
				StartMove(at, a_handle, a_player);  // activated on arrival
			}
		}

		// What the cursor is on, as worked out by the HUD.
		bool Hovered(RE::ObjectRefHandle& a_handle, Kind& a_kind)
		{
			std::scoped_lock guard(State::lock);
			a_handle = State::hovered;
			a_kind = State::hoveredKind;
			return static_cast<bool>(a_handle);
		}

		// -----------------------------------------------------------------------
		// Targets: everything nearby that gets an outline, a label or a click
		// -----------------------------------------------------------------------
		std::vector<Target> g_targets;

		constexpr std::uint32_t kWhite = Rgb(235, 235, 235);
		constexpr std::uint32_t kGold = Rgb(230, 195, 100);
		constexpr std::uint32_t kRed = Rgb(255, 60, 60);
		constexpr std::uint32_t kGreen = Rgb(90, 225, 110);  // same green as the talk cursor

		bool HasKeyword(RE::TESBoundObject* a_base, std::string_view a_keyword)
		{
			const auto* keywords = a_base->As<RE::BGSKeywordForm>();
			return keywords && keywords->HasKeywordString(a_keyword);
		}

		// The loot filter. Skyrim lets you pick up almost anything (cabbages, kettles, buckets),
		// so "can be picked up" isn't enough: this decides what counts as loot.
		bool ClassifyItem(RE::TESObjectREFR* a_ref, RE::TESBoundObject* a_base, std::uint32_t& a_color)
		{
			switch (a_base->GetFormType()) {
			case RE::FormType::Weapon:
			case RE::FormType::Armor:
			case RE::FormType::Ammo:
				if (!a_base->GetPlayable()) {
					return false;
				}
				a_color = kWhite;
				break;
			case RE::FormType::AlchemyItem:
				{
					const auto* potion = a_base->As<RE::AlchemyItem>();
					if (potion && potion->IsFood() && !Settings::ShowFood) {
						return false;
					}
					a_color = Rgb(130, 220, 150);
				}
				break;
			case RE::FormType::Ingredient:
				if (!Settings::ShowIngredients) {
					return false;
				}
				a_color = Rgb(130, 220, 150);
				break;
			case RE::FormType::Book:
				{
					const auto* book = a_base->As<RE::TESObjectBOOK>();
					if (!(book && book->TeachesSpell()) && !Settings::ShowBooks) {
						return false;  // spell tomes always show
					}
					a_color = Rgb(140, 210, 235);
				}
				break;
			case RE::FormType::Scroll:
				a_color = Rgb(140, 210, 235);
				break;
			case RE::FormType::KeyMaster:
			case RE::FormType::SoulGem:
				a_color = kGold;
				break;
			case RE::FormType::Misc:
				if (a_base->IsGold() || a_base->IsLockpick() || HasKeyword(a_base, "VendorItemGem") || HasKeyword(a_base, "VendorItemOreIngot")) {
					a_color = kGold;
				} else if (Settings::ShowClutter) {
					a_color = Rgb(200, 200, 200);
				} else {
					return false;  // kettles, buckets, plates...
				}
				break;
			case RE::FormType::Light:
				{
					const auto* light = a_base->As<RE::TESObjectLIGH>();
					if (!Settings::ShowClutter || !light || !light->CanBeCarried()) {
						return false;
					}
					a_color = Rgb(200, 200, 200);
				}
				break;
			default:
				return false;  // anything that can't go in the inventory
			}
			if (Settings::LabelMinValue > 0 && !a_base->IsGold()) {
				const auto value = a_base->GetGoldValue();
				if (value >= 0 && value < Settings::LabelMinValue) {
					return false;
				}
			}
			if (a_ref->IsEnchanted()) {
				a_color = Rgb(136, 136, 255);  // magic item
			}
			return true;
		}

		// Keeps only the outermost points in 26 directions. The outline of those on screen is
		// (near enough) the outline of all the points, from any camera angle.
		struct Extremes
		{
			static constexpr int kCount = 26;

			Extremes()
			{
				std::fill(std::begin(best), std::end(best), -1e30f);
			}

			void Add(const RE::NiPoint3& a_point)
			{
				int i = 0;
				for (int x = -1; x <= 1; ++x) {
					for (int y = -1; y <= 1; ++y) {
						for (int z = -1; z <= 1; ++z) {
							if (x == 0 && y == 0 && z == 0) {
								continue;
							}
							const float d = a_point.x * static_cast<float>(x) + a_point.y * static_cast<float>(y) + a_point.z * static_cast<float>(z);
							if (d > best[i]) {
								best[i] = d;
								point[i] = a_point;
							}
							++i;
						}
					}
				}
				++count;
			}

			RE::NiPoint3 point[kCount];
			float        best[kCount];
			int          count{ 0 };
		};

		// Walks a skeleton and collects where its bones are.
		void CollectBones(RE::NiAVObject* a_object, const RE::NiPoint3& a_centre, float a_limit, Extremes& a_out, int a_depth)
		{
			auto* node = a_object ? a_object->AsNode() : nullptr;
			if (!node || a_depth > 32 || a_out.count > 600) {
				return;
			}
			// Helper nodes that aren't part of the body (the camera target, look-at point, bumper).
			if (const char* name = node->name.c_str(); name && (std::strstr(name, "Cam") || std::strstr(name, "Look") || std::strstr(name, "Bumper") || std::strstr(name, "x_NPC"))) {
				return;
			}
			const auto& at = node->world.translate;
			const float dx = at.x - a_centre.x;
			const float dy = at.y - a_centre.y;
			const float dz = at.z - a_centre.z;
			if (dx * dx + dy * dy + dz * dz <= a_limit * a_limit) {
				a_out.Add(at);
			}
			for (auto& child : node->GetChildren()) {
				CollectBones(child.get(), a_centre, a_limit, a_out, a_depth + 1);
			}
		}

		// An actor's outline points: its bones, so the outline follows the pose.
		void ActorPoints(RE::Actor* a_actor, Target& a_target)
		{
			const auto  pos = a_actor->GetPosition();
			const float height = std::max(a_actor->GetHeight(), 60.0f);
			a_target.pos = pos;
			a_target.points.clear();
			auto* root = a_actor->Get3D();
			if (!root) {
				return;
			}
			const RE::NiPoint3 centre{ pos.x, pos.y, pos.z + height * 0.5f };
			const float        limit = std::max(height, root->worldBound.radius) * 1.25f;
			Extremes           extremes;
			CollectBones(root, centre, limit, extremes, 0);
			if (extremes.count >= 4) {
				a_target.points.assign(std::begin(extremes.point), std::end(extremes.point));
				return;
			}
			// No usable skeleton: a cylinder around where it stands.
			for (int i = 0; i < 8; ++i) {
				const float angle = static_cast<float>(i) * 0.785398f;
				const float x = pos.x + std::cos(angle) * 25.0f;
				const float y = pos.y + std::sin(angle) * 25.0f;
				a_target.points.push_back({ x, y, pos.z });
				a_target.points.push_back({ x, y, pos.z + height });
			}
		}

		// An object's outline points: the corners of its bounding box, wherever it's lying.
		bool BoxPoints(RE::TESObjectREFR* a_ref, Target& a_target)
		{
			auto* node = a_ref->Get3D();
			if (!node) {
				return false;
			}
			a_target.points.clear();
			const auto min = a_ref->GetBoundMin();
			const auto max = a_ref->GetBoundMax();
			if (max.x - min.x > 1.0f || max.y - min.y > 1.0f || max.z - min.z > 1.0f) {
				const auto& world = node->world;
				for (int i = 0; i < 8; ++i) {
					const RE::NiPoint3 corner{ (i & 1) ? max.x : min.x, (i & 2) ? max.y : min.y, (i & 4) ? max.z : min.z };
					a_target.points.push_back(world.translate + world.rotate * (corner * world.scale));
				}
				return true;
			}
			// No bounds on the base object: a box inside the model's bounding sphere.
			const float r = node->worldBound.radius * 0.6f;
			if (r <= 0.0f) {
				return false;
			}
			const auto& c = node->worldBound.center;
			for (int i = 0; i < 8; ++i) {
				a_target.points.push_back({ c.x + ((i & 1) ? r : -r), c.y + ((i & 2) ? r : -r), c.z + ((i & 4) ? r : -r) });
			}
			return true;
		}

		constexpr std::size_t kMaxItems = 400;

		void Scan(RE::PlayerCharacter* a_player)
		{
			auto* tes = RE::TES::GetSingleton();
			if (!tes) {
				return;
			}
			const auto  origin = a_player->GetPosition();
			const auto* cell = a_player->GetParentCell();
			const bool  interior = cell && cell->IsInteriorCell();
			const float radius = std::max(Settings::LabelRadius, Settings::OutlineRadius);  // actors, doors and containers; items stop at LabelRadius

			struct Item
			{
				Target target;
				float  dist;
			};
			std::vector<Item>   items;
			std::vector<Target> others;

			tes->ForEachReferenceInRange(a_player, radius, [&](RE::TESObjectREFR* a_ref) {
				constexpr auto next = RE::BSContainer::ForEachResult::kContinue;
				if (!a_ref || a_ref == a_player || a_ref->IsDisabled() || a_ref->IsMarkedForDeletion() || !a_ref->Is3DLoaded()) {
					return next;
				}
				auto* base = a_ref->GetBaseObject();
				if (!base) {
					return next;
				}
				const auto  pos = a_ref->GetPosition();
				const float heightDiff = std::fabs(pos.z - origin.z);

				Target target;
				target.handle = a_ref->GetHandle();
				target.formID = a_ref->GetFormID();
				target.distance = std::hypot(pos.x - origin.x, pos.y - origin.y);
				target.above = pos.z - origin.z;

				if (a_ref->IsActor()) {
					auto* actor = a_ref->As<RE::Actor>();
					// Indoors, other floors are out (the cutaway hides them).
					if (!actor || (interior && heightDiff > Settings::LabelMaxHeightDiff)) {
						return next;
					}
					if (actor->IsDead()) {
						target.kind = Kind::kCorpse;
						target.color = kGold;
					} else if (actor->IsHostileToActor(a_player)) {
						target.kind = Kind::kEnemy;
						target.color = kRed;
					} else {
						target.kind = Kind::kNpc;
						target.color = kGreen;
					}
					target.pad = 10.0f;
					ActorPoints(actor, target);
					if (!target.points.empty()) {
						others.push_back(std::move(target));
					}
					return next;
				}

				if (heightDiff > Settings::LabelMaxHeightDiff) {
					return next;
				}
				target.pos = pos;
				const auto type = base->GetFormType();
				if (type == RE::FormType::Container) {
					if (!Settings::Containers) {
						return next;
					}
					target.kind = Kind::kContainer;
					target.color = kGold;
				} else if (type == RE::FormType::Door) {
					target.kind = Kind::kDoor;
					target.color = kWhite;
				} else {
					const float dist = target.distance;
					// A cap for very crowded rooms (piles of coins, dozens of boxes).
					if (items.size() >= kMaxItems || dist > Settings::LabelRadius || !ClassifyItem(a_ref, base, target.color)) {
						return next;
					}
					const char* name = a_ref->GetDisplayFullName();
					if (!name || !*name) {
						return next;
					}
					target.kind = Kind::kItem;
					if (const auto count = a_ref->extraList.GetCount(); count >= Settings::StackCount) {
						target.text = std::format("{}x {}", count, name);
					} else {
						target.text = name;
					}
					if (a_ref->IsCrimeToActivate()) {
						target.color = Rgb(255, 90, 90);  // taking it is stealing
					}
					if (BoxPoints(a_ref, target)) {
						items.push_back({ std::move(target), dist });
					}
					return next;
				}
				if (BoxPoints(a_ref, target)) {
					others.push_back(std::move(target));
				}
				return next;
			});

			Target self;
			self.handle = a_player->GetHandle();
			self.formID = a_player->GetFormID();
			self.kind = Kind::kPlayer;
			self.color = kWhite;
			self.pad = 10.0f;
			ActorPoints(a_player, self);
			if (!self.points.empty()) {
				others.push_back(std::move(self));
			}

			// Items nearest first: the HUD gives the closest their true label position and stacks the rest.
			std::sort(items.begin(), items.end(), [](const Item& a_lhs, const Item& a_rhs) { return a_lhs.dist < a_rhs.dist; });
			g_targets.clear();
			g_targets.reserve(items.size() + others.size());
			for (auto& item : items) {
				g_targets.push_back(std::move(item.target));
			}
			for (auto& other : others) {
				g_targets.push_back(std::move(other));
			}
		}

		// Every frame: living actors move, so their outlines are refreshed, then the list is
		// handed to the HUD.
		void PublishTargets()
		{
			for (auto& target : g_targets) {
				if (target.kind != Kind::kEnemy && target.kind != Kind::kNpc && target.kind != Kind::kPlayer) {
					continue;
				}
				const auto ref = target.handle.get();
				auto*      actor = ref ? ref->As<RE::Actor>() : nullptr;
				if (actor && actor->Is3DLoaded()) {
					ActorPoints(actor, target);
				} else {
					target.points.clear();
				}
			}
			auto             snapshot = std::make_shared<const std::vector<Target>>(g_targets);
			std::scoped_lock guard(State::lock);
			State::targets = std::move(snapshot);
		}

		// -----------------------------------------------------------------------
		// Per frame
		// -----------------------------------------------------------------------
		void Tick(RE::PlayerCharacter* a_player, float a_delta)
		{
			RefreshGameplay();
			if (!State::gameplay.load()) {
				Halt();
				g_primary = Primary::kNone;
				g_primaryHeld = false;
				g_faceTime = 0.0f;
				return;
			}

			auto* controls = RE::PlayerControls::GetSingleton();
			if (controls && BlockLook()) {
				controls->data.lookInputVec = RE::NiPoint2(0.0f, 0.0f);
			}

			g_scanTimer -= a_delta;
			if (g_scanTimer <= 0.0f) {
				g_scanTimer = 0.15f;
				Scan(a_player);
			}
			PublishTargets();

			auto* controlMap = RE::ControlMap::GetSingleton();
			if (controlMap && !controlMap->IsMovementControlsEnabled()) {
				Halt();
				g_faceTime = 0.0f;
				return;
			}

			// WASD takes over: the keyboard's own input is already in place, so send nothing.
			if (State::moveKeysHeld.load() > 0) {
				g_moving = false;
				g_pending = {};
				g_faceTime = 0.0f;
				g_stickSent = false;
				return;
			}

			if (g_faceTime > 0.0f) {
				g_faceTime -= a_delta;
				Drive(a_player, g_faceX, g_faceY);
				if (g_faceTime <= 0.0f) {
					SendStick(0.0f, 0.0f);
				}
				return;
			}

			const bool following = g_primaryHeld && g_primary == Primary::kMove && Settings::HoldToMove;
			if (following) {
				RE::NiPoint3 point;
				if (CursorGround(a_player, point)) {
					g_target = point;
					g_pending = {};
					g_moving = true;
				}
			}
			if (!g_moving) {
				return;
			}

			float range = Settings::ArriveRadius;
			if (g_pending) {
				const auto ref = g_pending.get();
				if (!ref || ref->IsDisabled() || ref->IsMarkedForDeletion()) {
					Halt();
					return;
				}
				g_target = UsePoint(ref.get());
				range = Settings::PickupRange * 0.8f;
			}

			const auto  pos = a_player->GetPosition();
			const float dx = g_target.x - pos.x;
			const float dy = g_target.y - pos.y;
			const float dist = std::hypot(dx, dy);
			if (dist <= range) {
				if (g_pending) {
					const auto ref = g_pending.get();
					Halt();
					Activate(ref.get(), a_player);
				} else if (following) {
					SendStick(0.0f, 0.0f);  // standing under the cursor; keep following it
				} else {
					Halt();
				}
				return;
			}

			// Walking into a wall: give up rather than run in place (movement is a straight line).
			g_stuckTimer += a_delta;
			if (g_stuckTimer >= 0.6f) {
				const float moved = std::hypot(pos.x - g_stuckPos.x, pos.y - g_stuckPos.y);
				g_stuckTimer = 0.0f;
				g_stuckPos = pos;
				if (moved < 8.0f && !following) {
					// Blocked just short of the thing being used (a door frame, a counter): close enough counts.
					const auto ref = g_pending.get();
					Halt();
					if (ref && dist <= Settings::PickupRange * 2.5f) {
						Activate(ref.get(), a_player);
					}
					return;
				}
			}

			Drive(a_player, dx / dist, dy / dist);
		}

		struct PlayerUpdate
		{
			static void thunk(RE::Actor* a_this, float a_delta)
			{
				if (auto* player = RE::PlayerCharacter::GetSingleton()) {
					Tick(player, a_delta);
				}
				func(a_this, a_delta);
			}
			static inline REL::Relocation<decltype(thunk)> func;
		};

		class MenuSink : public RE::BSTEventSink<RE::MenuOpenCloseEvent>
		{
		public:
			static MenuSink* Get()
			{
				static MenuSink s_sink;
				return &s_sink;
			}

			RE::BSEventNotifyControl ProcessEvent(const RE::MenuOpenCloseEvent*, RE::BSTEventSource<RE::MenuOpenCloseEvent>*) override
			{
				// The per-frame hook stops while the game is paused, so menus update the cursor
				// state themselves: now, and again next frame once the menu has settled.
				RefreshGameplay();
				if (auto* tasks = SKSE::GetTaskInterface()) {
					tasks->AddTask([] { RefreshGameplay(); });
				}
				return RE::BSEventNotifyControl::kContinue;
			}
		};
	}

	bool WorldToScreen(const RE::NiPoint3& a_point, float& a_x, float& a_y)
	{
		auto* camera = RE::Main::WorldRootCamera();
		if (!camera) {
			return false;
		}
		float x = 0.0f;
		float y = 0.0f;
		float z = 0.0f;
		if (!RE::NiCamera::WorldPtToScreenPt3(camera->GetRuntimeData().worldToCam, camera->GetRuntimeData2().port, a_point, x, y, z, 1e-5f)) {
			return false;  // behind the camera
		}
		a_x = x;
		a_y = 1.0f - y;  // the game's y runs bottom-up
		return true;
	}

	void Install()
	{
		// The same slot the camera mod hooks; vtable hooks chain, so both run.
		REL::Relocation<std::uintptr_t> vtbl{ RE::VTABLE_PlayerCharacter[0] };
		PlayerUpdate::func = vtbl.write_vfunc(0xAD, PlayerUpdate::thunk);
		logs::info("[game] hooked PlayerCharacter::Update");
	}

	void RegisterMenuSink()
	{
		if (auto* ui = RE::UI::GetSingleton()) {
			ui->AddEventSink<RE::MenuOpenCloseEvent>(MenuSink::Get());
		}
	}

	// Menus with the game's cursor (inventory, containers, barter, Grid Inventory): the game's
	// cursor starts where ours was, ours picks up where the game's was left, and ours is drawn
	// in its place. Main thread.
	void SyncMenuCursor()
	{
		static bool s_open = false;
		auto*       ui = RE::UI::GetSingleton();
		auto*       cursor = RE::MenuCursor::GetSingleton();
		if (!ui || !cursor) {
			return;
		}
		const bool open = ui->IsMenuOpen(RE::CursorMenu::MENU_NAME);
		auto&      data = cursor->GetRuntimeData();
		const float screenW = State::screenW.load();
		const float screenH = State::screenH.load();
		// The game's cursor is in its own screen units; ours is in pixels.
		const float sx = data.screenWidthX > 1.0f ? data.screenWidthX / screenW : 1.0f;
		const float sy = data.screenWidthY > 1.0f ? data.screenWidthY / screenH : 1.0f;

		if (open && !s_open) {
			if (const float x = State::cursorX.load(), y = State::cursorY.load(); x >= 0.0f && y >= 0.0f) {
				data.cursorPosX = x * sx;
				data.cursorPosY = y * sy;
			}
		} else if (!open && s_open) {
			const float x = data.cursorPosX / sx;
			const float y = data.cursorPosY / sy;
			if (x >= 0.0f && y >= 0.0f && x <= screenW && y <= screenH) {
				State::cursorX.store(x);
				State::cursorY.store(y);
			}
		}
		s_open = open;

		const bool ours = open && Settings::PointClickEnabled && Settings::MenuCursor && Hud::Available();
		State::menuCursor.store(ours);
		if (open) {
			if (const auto menu = ui->GetMenu(RE::CursorMenu::MENU_NAME); menu && menu->uiMovie) {
				menu->uiMovie->SetVisible(!ours);
			}
		}
	}

	void RefreshGameplay()
	{
		bool on = false;
		if (Settings::PointClickEnabled && Hud::Available()) {
			auto* ui = RE::UI::GetSingleton();
			auto* player = RE::PlayerCharacter::GetSingleton();
			auto* camera = RE::PlayerCamera::GetSingleton();
			on = ui && player && camera &&
			     !ui->GameIsPaused() &&
			     !ui->IsMenuOpen(RE::CursorMenu::MENU_NAME) &&
			     !ui->IsMenuOpen(RE::DialogueMenu::MENU_NAME) &&
			     !ui->IsMenuOpen(RE::LoadingMenu::MENU_NAME) &&
			     !ui->IsMenuOpen(RE::MainMenu::MENU_NAME) &&
			     !ui->IsMenuOpen(RE::MapMenu::MENU_NAME) &&
			     player->Is3DLoaded() &&
			     camera->IsInThirdPerson() &&
			     !Hud::FrameworkMenuOpen();
		}
		State::gameplay.store(on);
		SyncMenuCursor();
	}

	void Reset()
	{
		g_moving = false;
		g_pending = {};
		g_stickSent = false;
		g_primary = Primary::kNone;
		g_primaryHeld = false;
		g_faceTime = 0.0f;
		g_scanTimer = 0.0f;
		State::moveKeysHeld.store(0);
		g_targets.clear();
		std::scoped_lock guard(State::lock);
		State::targets.reset();
		State::hovered = {};
	}

	bool OnPrimaryButton(RE::ButtonEvent* a_event)
	{
		auto* player = RE::PlayerCharacter::GetSingleton();
		if (!a_event || !player) {
			return false;
		}

		if (a_event->IsDown()) {
			g_primary = Primary::kNone;
			g_primaryHeld = false;
			if (State::gameplay.load() && State::moveKeysHeld.load() == 0) {
				RE::NiPoint3        point;
				RE::ObjectRefHandle hovered;
				Kind                kind = Kind::kItem;
				const bool          onSomething = Hovered(hovered, kind);
				const bool          onEnemy = onSomething && kind == Kind::kEnemy;
				if (State::attackInPlaceHeld.load()) {
					if (CursorGround(player, point)) {
						FaceToward(point, player);
					}
					g_primary = Primary::kAttack;
				} else if (onEnemy && Settings::ClickEnemies) {
					if (const auto enemy = hovered.get()) {
						FaceToward(enemy->GetPosition(), player);
					}
					g_primary = Primary::kAttack;
				} else if (onSomething && !onEnemy && Settings::ClickInteract) {
					Interact(hovered, player);  // pick up, loot, open, enter, talk
					g_primary = Primary::kSwallow;
				} else if (CursorGround(player, point)) {
					StartMove(point, {}, player);
					g_primary = Primary::kMove;
				}
			}
		}

		const auto mode = g_primary;
		if (a_event->IsUp()) {
			g_primary = Primary::kNone;
			g_primaryHeld = false;
		} else if (mode == Primary::kMove) {
			g_primaryHeld = true;
		}
		return mode == Primary::kMove || mode == Primary::kSwallow;
	}

	void OnPickupButton()
	{
		auto* player = RE::PlayerCharacter::GetSingleton();
		if (!player || !State::gameplay.load()) {
			return;
		}
		RE::ObjectRefHandle hovered;
		Kind                kind = Kind::kItem;
		if (Hovered(hovered, kind) && kind != Kind::kEnemy) {
			Interact(hovered, player);
		}
	}

	bool BlockLook()
	{
		return Settings::BlockMouseLook && State::gameplay.load() && !State::rotateHeld.load();
	}

	void Toggle()
	{
		Settings::PointClickEnabled = !Settings::PointClickEnabled;
		RE::SendHUDMessage::ShowHUDMessage(Settings::PointClickEnabled ? "Point and click: ON" : "Point and click: OFF");
	}
}
