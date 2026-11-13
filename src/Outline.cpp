// Feeds the point-and-click targets (enemies, NPCs, the player, items, whatever the cursor
// is on) to the outline renderer in src/render.
//
// The renderer, and the structure of the per-frame code below (render-target lookup, frame
// gate, geometry cache handling), come from Highlight Lootable Corpses by AmazingBuff, GPL-3.0:
// https://github.com/AmazingBuff/HighlightLootableCorpses

#include "Outline.h"
#include "Game.h"
#include "Settings.h"

#include "config/config.h"
#include "render/dx11/common_states.h"
#include "render/geometry/render_geometry_cache.h"
#include "render/mask/mask_overlay.h"
#include "render/shader_manager.h"
#include "render/ui_render_hook.h"

namespace Outline
{
	namespace
	{
		using namespace PLUGIN_NAMESPACE;

		// Each outlined thing costs a few draw calls, so there's a ceiling. Nearest and most
		// important first (see Priority).
		constexpr std::size_t kMaxTargets = 64;
		constexpr float       kDepthMargin = 40.0f;  // Skyrim units
		// Most model parts drawn in one frame, from the settings (the renderer's own default is Max_Render_Geometries_Per_Frame).
		std::size_t MaxParts()
		{
			return static_cast<std::size_t>(std::clamp(Settings::OutlineMaxParts, 64, 1024));
		}

		std::atomic<bool> g_hooked{ false };
		std::atomic<bool> g_failed{ false };

		// A character is outlined whole or not at all. The renderer leaves out any part of a model it
		// can't read (buffers not made yet, a mesh that fails its checks), and what's left is fragments:
		// a helmet, a pair of boots. So what was read is weighed against the model (skinned triangles),
		// and under kFullCoverage the model is read again every kRetryFrames, kMaxTries times, before
		// that answer is kept.
		constexpr float         kFullCoverage = 0.9f;
		constexpr std::uint32_t kRetryFrames = 20;
		constexpr std::uint8_t  kMaxTries = 15;
		constexpr std::uint32_t kCountFrames = 15;  // how often a living model's part count is checked
		// After kMaxTries short reads a model's answer used to be kept for good, so a player whose model was read
		// short once (going through a door, while its parts are rebuilt) kept no outline until something was
		// equipped. The player is now read again this often for as long as it stays short.
		constexpr std::uint32_t kPlayerSlowRetryFrames = 300;

		struct Entry
		{
			RE::FormID        formID;
			bool              hot;     // the cursor is on it
			bool              sticky;  // was drawn last frame
			RE::NiPoint3      centre;
			float             radius;
			float             distance;
			int               priority;
			bool              living;
			DirectX::XMFLOAT4 color;
		};

		bool Wanted(const Game::Target& a_target, bool a_hovered)
		{
			// Loot above you (shelves, ledges, the floor upstairs) gets no outline, hovered or not:
			// outlines ignore what's in front of them, so those would be drawn over the scene.
			const bool loot = a_target.kind == Game::Kind::kItem || a_target.kind == Game::Kind::kContainer || a_target.kind == Game::Kind::kCorpse;
			if (loot && a_target.above > Settings::OutlineMaxAbove) {
				return false;
			}
			// Actors on another level (a storey up or down, under the ground) get none either, indoors
			// and out: the same height band the labels use.
			if (Game::IsLiving(a_target.kind) && a_target.kind != Game::Kind::kPlayer && std::fabs(a_target.above) > Settings::LabelMaxHeightDiff) {
				return false;
			}
			if (a_hovered) {
				return true;  // otherwise, whatever the cursor is on is always outlined
			}
			return Settings::Outlines && Game::WantsMarker(a_target);
		}

		int Priority(const Game::Target& a_target, bool a_hovered)
		{
			if (a_hovered) {
				return 0;
			}
			// The player comes before the crowd: when there are more model parts than one frame may
			// draw (see MaxParts), the last in line are the ones left out.
			switch (a_target.kind) {
			case Game::Kind::kPlayer:
				return 1;
			case Game::Kind::kEnemy:
				return 2;
			case Game::Kind::kFollower:
				return 3;
			case Game::Kind::kNpc:
				return 4;
			default:
				return 5;
			}
		}

		// The size of the image the interface is drawn into (from Highlight Lootable Corpses).
		bool TargetDimensions(REX::W32::ID3D11RenderTargetView* a_target, std::uint32_t& a_width, std::uint32_t& a_height)
		{
			if (!a_target) {
				return false;
			}
			REX::W32::D3D11_RENDER_TARGET_VIEW_DESC viewDesc{};
			a_target->GetDesc(&viewDesc);

			std::uint32_t mip = 0;
			switch (viewDesc.viewDimension) {
			case REX::W32::D3D11_RTV_DIMENSION_TEXTURE2D:
				mip = viewDesc.texture2D.mipSlice;
				break;
			case REX::W32::D3D11_RTV_DIMENSION_TEXTURE2DARRAY:
				mip = viewDesc.texture2DArray.mipSlice;
				break;
			case REX::W32::D3D11_RTV_DIMENSION_TEXTURE2DMS:
			case REX::W32::D3D11_RTV_DIMENSION_TEXTURE2DMSARRAY:
				break;
			default:
				return false;
			}

			REX::W32::ID3D11Resource* resource = nullptr;
			a_target->GetResource(&resource);
			if (!resource) {
				return false;
			}
			REX::W32::ID3D11Texture2D* texture = nullptr;
			const REX::W32::HRESULT    hr = resource->QueryInterface(REX::W32::IID_ID3D11Texture2D, reinterpret_cast<void**>(&texture));
			resource->Release();
			if (!REX::W32::SUCCESS(hr) || !texture) {
				return false;
			}
			REX::W32::D3D11_TEXTURE2D_DESC textureDesc{};
			texture->GetDesc(&textureDesc);
			texture->Release();
			if (mip >= textureDesc.mipLevels || mip >= 32) {
				return false;
			}
			a_width = std::max(1u, textureDesc.width >> mip);
			a_height = std::max(1u, textureDesc.height >> mip);
			return true;
		}

		struct ModelSize
		{
			std::size_t   count{ 0 };    // model parts
			std::uint64_t skinned{ 0 };  // triangles in its skinned parts, the ones the renderer draws
		};

		ModelSize MeasureModel(RE::NiAVObject* a_root)
		{
			ModelSize size;
			RE::BSVisit::TraverseScenegraphGeometries(a_root, [&](RE::BSGeometry* a_geometry) {
				++size.count;
				const auto& runtime = a_geometry->GetGeometryRuntimeData();
				// Effect-shader parts (glows, fx) are never drawn by the renderer, so they don't count.
				if (runtime.shaderProperty && runtime.shaderProperty->GetRTTI() &&
					std::strcmp(runtime.shaderProperty->GetRTTI()->GetName(), "BSEffectShaderProperty") == 0) {
					return RE::BSVisit::BSVisitControl::kContinue;
				}
				const auto* skin = runtime.skinInstance.get();
				const auto* partition = skin ? skin->skinPartition.get() : nullptr;
				if (partition) {
					const auto parts = std::min<std::size_t>(partition->numPartitions, partition->partitions.size());
					for (std::size_t i = 0; i < parts; ++i) {
						size.skinned += partition->partitions[static_cast<std::uint32_t>(i)].triangles;
					}
				}
				return RE::BSVisit::BSVisitControl::kContinue;
			});
			return size;
		}

		// What is known about a target's model since it was last read.
		struct Parts
		{
			std::size_t   count{ 0 };        // model parts at that time
			float         coverage{ 1.0f };  // share of its skinned triangles the renderer could read
			std::uint32_t readAt{ 0 };       // frame
			std::uint8_t  tries{ 0 };        // reads in a row that came back short
			bool          logged{ false };
		};

		class Director
		{
		public:
			static Director& Get()
			{
				static Director s_instance;
				return s_instance;
			}

			void OnPreUiDraw()
			{
				// Once per rendered frame, and never on frame 0.
				auto*               state = RE::BSGraphics::State::GetSingleton();
				const std::uint32_t frame = state ? state->GetFrameCount() : 0;
				if (frame == _lastFrame || frame == 0) {
					return;
				}
				_lastFrame = frame;

				if (g_failed.load() || !State::gameplay.load()) {
					return;
				}

				auto* renderer = RE::BSGraphics::Renderer::GetSingleton();
				if (!renderer) {
					return;
				}
				auto&                          data = renderer->GetRuntimeData();
				REX::W32::ID3D11Device*        device = data.forwarder;
				REX::W32::ID3D11DeviceContext* context = data.context;
				if (!device || !context) {
					return;
				}
				REX::W32::ID3D11RenderTargetView* const output = data.renderTargets[RE::RENDER_TARGETS::kFRAMEBUFFER].RTV;
				std::uint32_t                           width = 0;
				std::uint32_t                           height = 0;
				if (!TargetDimensions(output, width, height)) {
					return;
				}
				Draw(device, context, output, width, height, frame);
			}

		private:
			bool Init(REX::W32::ID3D11Device* a_device)
			{
				if (_ready) {
					return true;
				}
				if (!ShaderManager::instance().compile()) {
					logs::error("[outline] the outline shaders didn't compile; outlines are off");
					g_failed.store(true);
					return false;
				}
				_states = std::make_unique<CommonStates>(a_device);
				if (!_states || !_states->valid() || !_mask.init(a_device)) {
					logs::error("[outline] couldn't set up the outline renderer; outlines are off");
					g_failed.store(true);
					return false;
				}
				_ready = true;
				logs::info("[outline] renderer ready");
				return true;
			}

			void Draw(REX::W32::ID3D11Device* a_device, REX::W32::ID3D11DeviceContext* a_context, REX::W32::ID3D11RenderTargetView* a_output,
				std::uint32_t a_width, std::uint32_t a_height, std::uint32_t a_frame)
			{
				if (!Init(a_device)) {
					return;
				}
				auto* camera = RE::Main::WorldRootCamera();
				if (!camera) {
					return;
				}

				std::shared_ptr<const std::vector<Game::Target>> targets;
				RE::ObjectRefHandle                              hovered;
				{
					std::scoped_lock guard(State::lock);
					targets = State::targets;
					hovered = State::hovered;
				}
				if (!targets || targets->empty()) {
					return;
				}

				// What to outline this frame.
				std::vector<Entry> entries;
				entries.reserve(targets->size());
				const char* playerState = "not in the target list";  // for the interior debug log
				for (const auto& target : *targets) {
					if (target.kind == Game::Kind::kPlayer) {
						playerState = target.points.empty() ? "no outline points (no 3D)" : "listed";
					}
					if (target.formID == 0 || target.points.empty()) {
						continue;
					}
					const bool hot = hovered && target.handle == hovered;
					if (!Wanted(target, hot)) {
						if (target.kind == Game::Kind::kPlayer) {
							playerState = "not wanted (Outlines/Player switch)";
						}
						continue;
					}
					Entry entry{};
					for (const auto& point : target.points) {
						entry.centre += point;
					}
					entry.centre /= static_cast<float>(target.points.size());
					for (const auto& point : target.points) {
						entry.radius = std::max(entry.radius, entry.centre.GetDistance(point));
					}
					entry.radius += target.pad + 20.0f;
					if (!camera->PointInFrustum(entry.centre, entry.radius)) {
						if (target.kind == Game::Kind::kPlayer) {
							playerState = "outside the camera's view";
						}
						continue;
					}
					entry.formID = target.formID;
					entry.hot = hot;
					entry.distance = target.distance;
					entry.priority = Priority(target, hot);
					entry.living = Game::IsLiving(target.kind);
					entry.sticky = _drawn.contains(target.formID);
					// A soft glow: colours darkened, drawn part-transparent, fainter with distance.
					// The thing under the cursor stays at full strength so it stands out.
					const float bright = hot ? 1.0f : std::clamp(Settings::OutlineBrightness, 0.1f, 1.0f);
					float       alpha = hot ? 1.0f : std::clamp(Settings::OutlineOpacity, 0.05f, 1.0f);
					if (!hot && Settings::OutlineFade) {
						const float reach = std::max(entry.living ? Settings::OutlineRadius : Settings::OutlineItemRadius, 1.0f);
						const float t = std::clamp(target.distance / reach, 0.0f, 1.0f);
						alpha *= 1.0f - 0.6f * t * t;  // 40% of its strength at the edge of the outline distance
					}
					entry.color = { bright * static_cast<float>(target.color & 0xFF) / 255.0f,
						bright * static_cast<float>((target.color >> 8) & 0xFF) / 255.0f,
						bright * static_cast<float>((target.color >> 16) & 0xFF) / 255.0f, alpha };
					entries.push_back(entry);
				}
				if (entries.empty()) {
					LogPlayer(a_frame, playerState, RE::PlayerCharacter::GetSingleton(), 0);
					return;
				}
				// Most important first, then whatever was drawn last frame, then nearest. The order only
				// matters when there is more than one frame may draw (kMaxTargets, MaxParts); keeping
				// last frame's picks ahead stops the outlines hopping between actors in a crowd as their
				// distances change.
				std::sort(entries.begin(), entries.end(), [](const Entry& a_lhs, const Entry& a_rhs) {
					if (a_lhs.priority != a_rhs.priority) {
						return a_lhs.priority < a_rhs.priority;
					}
					if (a_lhs.sticky != a_rhs.sticky) {
						return a_lhs.sticky;
					}
					return a_lhs.distance < a_rhs.distance;
				});
				if (entries.size() > kMaxTargets) {
					entries.resize(kMaxTargets);
				}

				auto& config = Setting::instance().get_config();
				config.display_mode = Config::DisplayMode::e_outline;
				config.outline_thickness = std::clamp(Settings::OutlineThickness, Setting::Min_Outline_Thickness, Setting::Max_Outline_Thickness);

				if (!_mask.begin_frame(a_device, a_width, a_height)) {
					_mask.end_frame();
					return;
				}

				auto& cache = RenderGeometryCache::instance();
				cache.drain_invalidations();
				// A door or a load (Game bumps the epoch): every model gets its full set of reads again. Without this a
				// model read short during the transition kept its short answer in the new cell.
				if (const auto epoch = State::cellEpoch.load(); epoch != _epoch) {
					_epoch = epoch;
					for (const auto& [formID, info] : _parts) {
						cache.erase(formID);
					}
					_parts.clear();
					_behind.clear();
					if (Settings::InteriorDebugLog) {
						logs::info("[interior] cell changed (interior={}): outline models will be read again", State::interior.load());
					}
				}
				if (_behind.size() > 1024) {
					_behind.clear();
				}
				if (_parts.size() > 1024) {
					_parts.clear();
				}

				std::vector<DirectX::XMFLOAT4>           colors;
				std::vector<std::vector<RenderGeometry>> geometries;
				colors.reserve(entries.size());
				geometries.reserve(entries.size());

				// A model's parts, from the cache when it's still good. True = the whole model was read.
				auto fetch = [&](RE::FormID a_formID, bool a_living, std::vector<RenderGeometry>& a_group) {
					auto*                    form = RE::TESForm::LookupByID(a_formID);
					const RE::TESObjectREFR* ref = form ? form->AsReference() : nullptr;
					RE::NiAVObject* const    root = ref ? ref->GetCurrent3D() : nullptr;
					if (!ref || !root) {
						return true;
					}
					auto&      info = _parts[a_formID];
					const auto cached = cache.lookup(a_formID);
					if (cached.geometries) {
						bool stale = cached.root3d != root;
						if (!stale && a_living) {
							// Living things change: a weapon drawn or sheathed swaps parts of the model
							// without rebuilding it. A cached part that has been taken off the model
							// (no parent) would be drawn frozen in mid-air, so check every frame; added
							// parts are caught by a part count, checked about once a second.
							for (const auto& part : *cached.geometries) {
								if (part.node && !part.node->parent) {
									stale = true;
									break;
								}
							}
							if (!stale && (a_frame + a_formID) % kCountFrames == 0) {
								stale = info.count != MeasureModel(root).count;
							}
						}
						if (stale) {
							info.tries = 0;  // a new or changed model: its parts get their time to load again
						} else if (info.coverage < kFullCoverage && info.tries < kMaxTries && a_frame - info.readAt >= kRetryFrames) {
							stale = true;  // it came back short last time: read it again
						} else if (info.coverage < kFullCoverage && a_formID == 0x14 && a_frame - info.readAt >= kPlayerSlowRetryFrames) {
							stale = true;  // the player never stays without an outline for good
						}
						if (stale) {
							cache.erase(a_formID);
						} else {
							a_group = *cached.geometries;
							return info.coverage >= kFullCoverage;
						}
					}
					collect_render_geometries(ref, a_device, a_group);
					const auto    size = MeasureModel(root);
					std::uint64_t read = 0;
					for (const auto& part : a_group) {
						if (part.skin) {
							read += part.triangle_count;
						}
					}
					info.count = size.count;
					info.coverage = size.skinned > 0 ? static_cast<float>(read) / static_cast<float>(size.skinned) : 1.0f;
					info.readAt = a_frame;
					if (info.coverage >= kFullCoverage) {
						info.tries = 0;
					} else if (info.tries < kMaxTries && ++info.tries == kMaxTries && !info.logged && _partialLogs < 24) {
						// Says which model stays short, so the cause can be found in the renderer's own
						// "skip skinned draw" lines near this one in the log.
						info.logged = true;
						++_partialLogs;
						logs::info("[outline] {:08X} ({}): only {:.0f}% of the model could be read ({} of {} skinned triangles, {} parts); "
								   "no outline unless hovered or [Outlines] Partial = 1",
							a_formID, ref->GetName(), info.coverage * 100.0f, read, size.skinned, size.count);
					}
					cache.insert(a_formID, root, std::vector<RenderGeometry>(a_group));
					// The cache owns the position buffers; this frame's copy only borrows them.
					return info.coverage >= kFullCoverage;
				};

				// Things further from the camera than the player are behind them on screen.
				auto*              player = RE::PlayerCharacter::GetSingleton();
				const RE::NiPoint3 eye = camera->world.translate;
				float              playerDepth = -1.0f;
				if (player && Settings::OutlineBehindPlayer) {
					const auto  feet = player->GetPosition();
					const float height = std::max(player->GetHeight(), 60.0f);
					playerDepth = eye.GetDistance(RE::NiPoint3{ feet.x, feet.y, feet.z + height * 0.5f });
				}

				std::vector<std::uint8_t> behind;
				behind.reserve(entries.size());
				std::uint32_t index = 0;
				std::size_t   total = 0;
				std::size_t   leftOut = 0;
				std::size_t   partial = 0;
				const auto    maxParts = MaxParts();
				_drawn.clear();
				for (const auto& entry : entries) {
					std::vector<RenderGeometry> group;
					const bool                  whole = fetch(entry.formID, entry.living, group);
					const bool                  isPlayerEntry = player && entry.formID == player->GetFormID();
					if (group.empty()) {
						if (isPlayerEntry) {
							playerState = "no model parts could be read";
						}
						continue;
					}
					// All or nothing: fragments of a character are worse than no outline. The one under
					// the cursor still shows what there is.
					if (!whole && !entry.hot && !Settings::OutlinePartial) {
						++partial;
						if (isPlayerEntry) {
							playerState = "model only read in part (left out: Partial = 0)";
						}
						continue;
					}
					// Out of room for this one. Keep going: a smaller model further down may still fit.
					if (total + group.size() > maxParts) {
						++leftOut;
						if (isPlayerEntry) {
							playerState = "left out by MaxParts";
						}
						continue;
					}
					if (isPlayerEntry) {
						playerState = "drawn";
					}
					total += group.size();
					_drawn.insert(entry.formID);
					for (auto& part : group) {
						part.target_index = index;
					}
					colors.push_back(entry.color);
					geometries.push_back(std::move(group));
					// In front or behind is decided with a margin and then kept, so something at about
					// the player's depth doesn't flip between the two from frame to frame.
					const bool isPlayer = player && entry.formID == player->GetFormID();
					bool       isBehind = false;
					if (!isPlayer && playerDepth > 0.0f) {
						const float depth = eye.GetDistance(entry.centre);
						const auto  it = _behind.find(entry.formID);
						isBehind = it != _behind.end() ? it->second : depth > playerDepth;
						if (depth > playerDepth + kDepthMargin) {
							isBehind = true;
						} else if (depth < playerDepth - kDepthMargin) {
							isBehind = false;
						}
						_behind[entry.formID] = isBehind;
					}
					behind.push_back(isBehind ? 1 : 0);
					++index;
				}

				if (playerDepth > 0.0f && std::find(behind.begin(), behind.end(), std::uint8_t{ 1 }) != behind.end()) {
					std::vector<RenderGeometry> occluder;
					fetch(player->GetFormID(), true, occluder);  // whole or not: it only hides what is behind you
					if (!occluder.empty() && total + occluder.size() <= maxParts * 2) {
						_mask.set_occluder(std::move(occluder), std::move(behind));
					}
				}

				// For tracking down outlines that come and go in crowds: says when the part limit was hit.
				if ((leftOut > 0 || partial > 0) && Settings::PointClickDebugLog && a_frame - _lastBudgetLog > 300) {
					_lastBudgetLog = a_frame;
					logs::info("[outline] {} parts drawn for {} targets; {} left out by the part limit, {} because their model was only read in part",
						total, geometries.size(), leftOut, partial);
				}

				LogPlayer(a_frame, playerState, player, total);

				if (!geometries.empty()) {
					_mask.draw(a_device, a_context, camera, a_output, a_width, a_height, colors, geometries, *_states);
				}
				_mask.end_frame();
			}

			// [Debug] Interior: once a second, why your own outline is or isn't drawn, so a missing outline indoors or
			// after a door can be told apart: not listed, off screen, model read short, out of parts, or drawn.
			void LogPlayer(std::uint32_t a_frame, const char* a_state, RE::PlayerCharacter* a_player, std::size_t a_total)
			{
				if (!Settings::InteriorDebugLog || a_frame - _lastPlayerLog < 60) {
					return;
				}
				_lastPlayerLog = a_frame;
				const auto it = a_player ? _parts.find(a_player->GetFormID()) : _parts.end();
				if (it == _parts.end()) {
					logs::info("[interior] player outline: {} (interior={}, epoch={}, model not read yet)", a_state, State::interior.load(), _epoch);
					return;
				}
				const auto& info = it->second;
				logs::info("[interior] player outline: {} (interior={}, epoch={}, read {:.0f}% of the model, {} short reads in a row, {} parts, "
						   "read {} frames ago, {} parts drawn this frame)",
					a_state, State::interior.load(), _epoch, info.coverage * 100.0f, static_cast<int>(info.tries), info.count, a_frame - info.readAt, a_total);
			}

			std::uint32_t                               _lastFrame{ std::numeric_limits<std::uint32_t>::max() };
			std::unique_ptr<CommonStates>               _states;
			Mask::MaskOverlay                           _mask;
			std::unordered_map<RE::FormID, Parts>       _parts;   // per target: its model when it was last read
			std::uint32_t                               _partialLogs{ 0 };
			std::unordered_map<RE::FormID, bool>        _behind;  // per target: currently behind the player
			std::unordered_set<RE::FormID>              _drawn;   // targets outlined on the last frame
			std::uint32_t                               _lastBudgetLog{ 0 };
			std::uint32_t                               _lastPlayerLog{ 0 };
			std::uint32_t                               _epoch{ 0 };      // State::cellEpoch the models were read in
			bool                                        _ready{ false };
		};

		void PreUiCallback(std::int64_t)
		{
			Director::Get().OnPreUiDraw();
		}
	}

	void Install()
	{
		RenderGeometryCache::instance().install();
		g_hooked.store(UiRenderHook::instance().install(&PreUiCallback));
		if (!g_hooked.load()) {
			logs::error("[outline] couldn't hook the interface draw; falling back to simple shapes");
		}
	}

	bool Active()
	{
		return g_hooked.load() && !g_failed.load();
	}
}
