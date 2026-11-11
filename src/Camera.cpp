#include "Camera.h"
#include "Game.h"
#include "Settings.h"

namespace Camera
{
	namespace
	{
		bool g_active = true;

		// The compass heading the camera is kept at while nothing but True Directional Movement turns it.
		std::atomic<bool> g_headingValid{ false };
		float             g_heading = 0.0f;  // radians, camera update only

		float DegToRad(float a_deg) { return a_deg * (std::numbers::pi_v<float> / 180.0f); }

		float NormalizeAngle(float a_angle)
		{
			constexpr float pi = std::numbers::pi_v<float>;
			constexpr float twoPi = 2.0f * pi;
			a_angle = std::fmod(a_angle + pi, twoPi);
			if (a_angle < 0.0f) {
				a_angle += twoPi;
			}
			return a_angle - pi;
		}

		// True Directional Movement sets this graph variable on the player while target lock is on.
		bool IsTargetLocked(RE::PlayerCharacter* a_player)
		{
			static const RE::BSFixedString var{ "TDM_TargetLock" };
			bool                           locked = false;
			return a_player && a_player->GetGraphVariableBool(var, locked) && locked;
		}

		void ApplyLock(RE::ThirdPersonState* a_state)
		{
			if (!g_active || !a_state) {
				return;
			}
			auto* player = RE::PlayerCharacter::GetSingleton();
			if (!player) {
				return;
			}
			// While locked on, TDM keeps the camera still (TISS settings). Writing pitch here fights it.
			if (Settings::PauseOnTargetLock && IsTargetLocked(player)) {
				return;
			}

			if (Settings::LockYaw) {
				// Camera world yaw = player heading + freeRotation.x when free rotation is on.
				// Solve for the offset that keeps the camera at a fixed world heading.
				a_state->freeRotationEnabled = true;
				const float desired = DegToRad(Settings::Yaw);
				a_state->freeRotation.x = NormalizeAngle(desired - player->data.angle.z);
				g_headingValid.store(false);
			} else if (Settings::KeepHeading && a_state->freeRotationEnabled) {
				// True Directional Movement turns the camera by itself: toward the target while its lock is
				// on, behind the character when the lock key is pressed with nothing to lock (its camera
				// reset), and slowly behind the character while moving (its auto-adjust). All of them write
				// freeRotation.x from TDM's own update, which runs before this one, so the last word is had
				// here. While the mouse can turn the camera its heading is followed; otherwise it is put back.
				const float yaw = NormalizeAngle(player->data.angle.z + a_state->freeRotation.x);
				const bool  mouseTurns = State::rotateHeld.load() || (!Game::BlockLook() && !IsTargetLocked(player));
				if (mouseTurns || !g_headingValid.load()) {
					g_heading = yaw;
					g_headingValid.store(true);
				} else {
					a_state->freeRotation.x = NormalizeAngle(g_heading - player->data.angle.z);
				}
			} else {
				g_headingValid.store(false);
			}

			if (Settings::LockPitch) {
				float pitch = DegToRad(Settings::Pitch);
				if (Settings::InvertPitch) {
					pitch = -pitch;
				}
				// Casting or drawing a bow: the player aims at the cursor while the camera keeps its tilt.
				// Camera tilt = player pitch - freeRotation.y (only while free rotation is on).
				static bool s_aiming = false;
				static bool s_freeBefore = false;
				const bool  aiming = State::aiming.load() && Settings::PitchMode != 1;
				if (aiming && !s_aiming) {
					s_freeBefore = a_state->freeRotationEnabled;
				} else if (!aiming && s_aiming && !Settings::LockYaw) {
					a_state->freeRotationEnabled = s_freeBefore;
				}
				s_aiming = aiming;

				if (aiming) {
					const float aim = State::aimPitch.load();
					a_state->freeRotationEnabled = true;
					player->data.angle.x = aim;
					a_state->freeRotation.y = aim - pitch;
				} else if (Settings::PitchMode == 1) {
					a_state->freeRotation.y = pitch;
				} else {
					player->data.angle.x = pitch;
					// TDM's target lock adds its own camera tilt here (seen hitting -60 deg, which points the
					// camera at the ground). Camera tilt = player pitch - freeRotation.y, so keep it at zero.
					a_state->freeRotation.y = 0.0f;
				}
			}
		}

		void DebugLog(RE::ThirdPersonState* a_state)
		{
			if (!Settings::CameraDebugLog || !a_state) {
				return;
			}
			static auto last = std::chrono::steady_clock::now();
			const auto  now = std::chrono::steady_clock::now();
			if (now - last < std::chrono::seconds(1)) {
				return;
			}
			last = now;

			auto* player = RE::PlayerCharacter::GetSingleton();
			if (!player) {
				return;
			}
			logs::info("[camera] targetLock={} freeRotEnabled={} freeRot=({:.3f}, {:.3f}) playerAngle=(x {:.3f}, z {:.3f}) curYaw={:.3f} tgtYaw={:.3f}",
				IsTargetLocked(player), a_state->freeRotationEnabled, a_state->freeRotation.x, a_state->freeRotation.y,
				player->data.angle.x, player->data.angle.z, a_state->currentYaw, a_state->targetYaw);
		}

		// -----------------------------------------------------------------------
		// Interior room culling
		// -----------------------------------------------------------------------
		// Interiors are split into rooms joined by portals, and the engine only draws the rooms it can
		// reach from the room the CAMERA is in. An overhead camera is often above the ceiling or in a
		// neighbouring room, so the player's own room vanishes. When the engine asks a room "does it
		// contain this point?" and the answer is no, ask again at the player's eye and at points walking
		// from there back toward the camera. Approach from Skyrim Camera Disocclusion System.
		std::atomic<float> g_eyeX{ 0.0f };
		std::atomic<float> g_eyeY{ 0.0f };
		std::atomic<float> g_eyeZ{ 0.0f };
		std::atomic<bool>  g_eyeValid{ false };

		void UpdateEye()
		{
			if (auto* player = RE::PlayerCharacter::GetSingleton()) {
				const auto pos = player->GetPosition();
				g_eyeX.store(pos.x, std::memory_order_relaxed);
				g_eyeY.store(pos.y, std::memory_order_relaxed);
				g_eyeZ.store(pos.z + 120.0f, std::memory_order_relaxed);
				g_eyeValid.store(true, std::memory_order_release);
			}
		}

		struct RoomContainsPointHook
		{
			static bool thunk(RE::BSMultiBoundRoom* a_this, const RE::NiPoint3& a_point)
			{
				if (func(a_this, a_point)) {
					return true;
				}
				if (!Settings::FixInteriorRooms || !g_eyeValid.load(std::memory_order_acquire)) {
					return false;
				}
				const RE::NiPoint3 eye{ g_eyeX.load(std::memory_order_relaxed), g_eyeY.load(std::memory_order_relaxed),
					g_eyeZ.load(std::memory_order_relaxed) };
				for (const float t : { 1.0f, 0.75f, 0.5f, 0.25f }) {
					const RE::NiPoint3 p{ a_point.x + (eye.x - a_point.x) * t, a_point.y + (eye.y - a_point.y) * t,
						a_point.z + (eye.z - a_point.z) * t };
					if (func(a_this, p)) {
						return true;
					}
				}
				return false;
			}
			static inline REL::Relocation<decltype(thunk)> func;

			static void Install()
			{
				REL::Relocation<std::uintptr_t> vtbl{ RE::VTABLE_BSMultiBoundRoom[0] };
				func = vtbl.write_vfunc(0x3F, thunk);
				logs::info("[camera] hooked BSMultiBoundRoom::QPointWithin");
			}
		};

		// ThirdPersonState::Update (vtable slot 0x3).
		struct ThirdPersonUpdateHook
		{
			static void thunk(RE::ThirdPersonState* a_this, RE::BSTSmartPointer<RE::TESCameraState>& a_nextState)
			{
				UpdateEye();
				ApplyLock(a_this);  // set before the game builds this frame's camera
				func(a_this, a_nextState);
				ApplyLock(a_this);  // undo anything mouse input or other mods changed this frame
				DebugLog(a_this);
			}
			static inline REL::Relocation<decltype(thunk)> func;

			static void Install()
			{
				REL::Relocation<std::uintptr_t> vtbl{ RE::VTABLE_ThirdPersonState[0] };
				func = vtbl.write_vfunc(0x3, thunk);
				logs::info("[camera] hooked ThirdPersonState::Update");
			}
		};
	}

	bool IsTargetLocked()
	{
		return IsTargetLocked(RE::PlayerCharacter::GetSingleton());
	}

	// The engine's own camera settings (Skyrim.ini [Camera] / [Display]). 3PCO set these; now this does.
	void ApplyCameraSettings()
	{
		auto* ini = RE::INISettingCollection::GetSingleton();
		auto* prefs = RE::INIPrefSettingCollection::GetSingleton();
		static std::unordered_map<std::string, float> s_original;  // the game's values, as they were before this changed them
		static bool                                   s_applied = false;

		const auto set = [&](const char* a_name, float a_value) {
			RE::Setting* setting = ini ? ini->GetSetting(a_name) : nullptr;
			if (!setting && prefs) {
				setting = prefs->GetSetting(a_name);
			}
			if (!setting || setting->GetType() != RE::Setting::Type::kFloat) {
				logs::warn("[camera] engine setting {} not found", a_name);
				return;
			}
			s_original.try_emplace(a_name, setting->data.f);
			setting->data.f = a_value;
		};

		if (!Settings::ManageCamera) {
			// Switched off: hand back everything that was changed, so the game or another camera
			// mod has its own values again and not this mod's last ones.
			if (s_applied) {
				for (const auto& [name, value] : s_original) {
					set(name.c_str(), value);
				}
				if (const auto it = s_original.find("fDefaultWorldFOV:Display"); it != s_original.end()) {
					if (auto* camera = RE::PlayerCamera::GetSingleton()) {
						camera->GetRuntimeData2().worldFOV = it->second;
					}
				}
				s_applied = false;
				logs::info("[camera] no longer managed: the game's distance, offsets, zoom and field of view are back");
			}
			return;
		}
		s_applied = true;
		const auto restore = [&](const char* a_name) {
			if (const auto it = s_original.find(a_name); it != s_original.end()) {
				set(a_name, it->second);
			}
		};

		// Same nearest and furthest distance = one fixed distance.
		set("fVanityModeMinDist:Camera", Settings::CameraDistance);
		set("fVanityModeMaxDist:Camera", Settings::CameraDistance);
		// The same offsets sheathed and in combat, so drawing a weapon doesn't shift the view.
		set("fOverShoulderPosX:Camera", Settings::CameraOffsetX);
		set("fOverShoulderPosZ:Camera", Settings::CameraOffsetZ);
		set("fOverShoulderCombatPosX:Camera", Settings::CameraOffsetX);
		set("fOverShoulderCombatPosZ:Camera", Settings::CameraOffsetZ);
		set("fOverShoulderCombatAddY:Camera", 0.0f);
		if (Settings::WheelZoom) {
			restore("fMouseWheelZoomSpeed:Camera");
			restore("fMouseWheelZoomIncrement:Camera");
		} else {
			set("fMouseWheelZoomSpeed:Camera", 0.0f);
			set("fMouseWheelZoomIncrement:Camera", 0.0f);
		}
		set("fDefaultWorldFOV:Display", Settings::CameraFov);
		logs::info("[camera] managed: distance={} fov={} offset=({}, {}) wheelZoom={}", Settings::CameraDistance, Settings::CameraFov,
			Settings::CameraOffsetX, Settings::CameraOffsetZ, Settings::WheelZoom);
	}

	void OnFrame()
	{
		auto* camera = RE::PlayerCamera::GetSingleton();
		auto* player = RE::PlayerCharacter::GetSingleton();
		if (!camera || !player) {
			return;
		}
		if (Settings::ManageCamera) {
			camera->GetRuntimeData2().worldFOV = Settings::CameraFov;
		}
		// The lock only runs in third person, but it leaves the view tilted down. Entering first
		// person, level it once; after that the mouse looks around freely.
		static bool s_first = false;
		const bool  first = camera->IsInFirstPerson();
		if (first && !s_first && g_active && Settings::LockPitch) {
			player->data.angle.x = 0.0f;
		}
		s_first = first;
		if (first) {
			g_headingValid.store(false);  // back in third person, start from wherever the camera is then
		}
	}

	void Install()
	{
		g_active = Settings::CameraEnabled;
		ThirdPersonUpdateHook::Install();
		RoomContainsPointHook::Install();
	}

	bool IsActive()
	{
		return g_active;
	}

	void SetActive(bool a_active)
	{
		g_active = a_active;
		g_headingValid.store(false);
	}

	void ForgetHeading()
	{
		g_headingValid.store(false);
	}

	void Toggle()
	{
		g_active = !g_active;
		g_headingValid.store(false);
		RE::SendHUDMessage::ShowHUDMessage(g_active ? "Camera lock: ON" : "Camera lock: OFF");
		logs::info("[camera] toggled {}", g_active ? "on" : "off");
	}

	void AdjustPitch(float a_delta)
	{
		Settings::Pitch = std::clamp(Settings::Pitch + a_delta, 5.0f, 85.0f);
		Settings::SetPresetName("Custom");
		Settings::Save();
		static std::string msg;  // keep the text alive in case the HUD queues the pointer
		msg = std::format("Camera pitch: {:.0f} deg", Settings::Pitch);
		RE::SendHUDMessage::ShowHUDMessage(msg.c_str());
		logs::info("[camera] pitch set to {:.0f}", Settings::Pitch);
	}

	void StepYaw(float a_degrees)
	{
		if (!Settings::LockYaw) {
			RE::SendHUDMessage::ShowHUDMessage("Camera yaw key: turn on Lock yaw first");
			return;
		}
		float yaw = std::fmod(Settings::Yaw + a_degrees, 360.0f);
		if (yaw < 0.0f) {
			yaw += 360.0f;
		}
		Settings::Yaw = yaw;
		Settings::Save();
		static std::string msg;  // keep the text alive in case the HUD queues the pointer
		msg = std::format("Camera yaw: {:.0f} deg", Settings::Yaw);
		RE::SendHUDMessage::ShowHUDMessage(msg.c_str());
		logs::info("[camera] yaw set to {:.0f}", Settings::Yaw);
	}

	// [GameSettings] entries are engine settings (the ones `setgs` changes). The engine resets them
	// on every launch, so they are re-applied once the game data has loaded.
	void ApplyGameSettings()
	{
		auto* collection = RE::GameSettingCollection::GetSingleton();
		if (!collection) {
			return;
		}
		for (const auto& [name, value] : Settings::GameSettings) {
			auto* setting = collection->GetSetting(name.c_str());
			if (!setting) {
				logs::warn("[game] setting {} not found", name);
				continue;
			}
			try {
				switch (setting->GetType()) {
				case RE::Setting::Type::kFloat:
					setting->data.f = std::stof(value);
					break;
				case RE::Setting::Type::kInteger:
					setting->data.i = std::stoi(value);
					break;
				case RE::Setting::Type::kUnsignedInteger:
					setting->data.u = static_cast<std::uint32_t>(std::stoul(value));
					break;
				case RE::Setting::Type::kBool:
					setting->data.b = std::stoi(value) != 0;
					break;
				default:
					logs::warn("[game] setting {} has a type this plugin can't set", name);
					continue;
				}
				logs::info("[game] {} = {}", name, value);
			} catch (...) {
				logs::warn("[game] bad value for {}: {}", name, value);
			}
		}
	}

	// [Player] values are character stats (the ones `player.setav` changes). They live in the save,
	// so they are applied whenever a save loads or a new game starts.
	void ApplyPlayerValues()
	{
		auto* player = RE::PlayerCharacter::GetSingleton();
		if (!player) {
			return;
		}
		auto* av = player->AsActorValueOwner();
		if (!av) {
			return;
		}
		if (Settings::SpeedMult > 0.0f) {
			av->SetBaseActorValue(RE::ActorValue::kSpeedMult, Settings::SpeedMult);
		}
		if (Settings::CarryWeight > 0.0f) {
			// Also refreshes movement speed, which otherwise only updates on the next weight change.
			av->SetBaseActorValue(RE::ActorValue::kCarryWeight, Settings::CarryWeight);
		}
		logs::info("[player] values applied: speedMult={} carryWeight={}", Settings::SpeedMult, Settings::CarryWeight);
	}
}
