#include "Settings.h"

namespace
{
	constexpr const char* kIniPath = "Data\\SKSE\\Plugins\\ARPGPOV.ini";

	using IniMap = std::unordered_map<std::string, std::string>;

	// Load, Save and the preset name can be reached from the render thread (the menu) and the
	// main thread (hotkeys, reload) at once. Recursive: Load applies a preset while holding it.
	std::recursive_mutex g_lock;

	std::string Trim(const std::string& a_s)
	{
		const auto first = a_s.find_first_not_of(" \t\r\n");
		if (first == std::string::npos) {
			return {};
		}
		const auto last = a_s.find_last_not_of(" \t\r\n");
		return a_s.substr(first, last - first + 1);
	}

	std::string Lower(std::string a_s)
	{
		std::transform(a_s.begin(), a_s.end(), a_s.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
		return a_s;
	}

	// Flattens the INI into "Section.Key" -> value. [GameSettings] is kept in order separately.
	IniMap Parse(std::vector<std::pair<std::string, std::string>>& a_gameSettings)
	{
		IniMap        out;
		std::ifstream in(kIniPath);
		if (!in.is_open()) {
			logs::warn("settings: {} not found, using defaults", kIniPath);
			return out;
		}
		std::string section;
		std::string line;
		while (std::getline(in, line)) {
			line = Trim(line);
			if (line.empty() || line[0] == ';' || line[0] == '#') {
				continue;
			}
			if (line.front() == '[' && line.back() == ']') {
				section = Trim(line.substr(1, line.size() - 2));
				continue;
			}
			const auto eq = line.find('=');
			if (eq == std::string::npos) {
				continue;
			}
			const auto key = Trim(line.substr(0, eq));
			auto       value = Trim(line.substr(eq + 1));
			if (const auto semi = value.find(';'); semi != std::string::npos) {
				value = Trim(value.substr(0, semi));
			}
			if (section == "GameSettings") {
				a_gameSettings.emplace_back(key, value);
			} else {
				out[section + "." + key] = value;
			}
		}
		return out;
	}

	float GetFloat(const IniMap& a_ini, const char* a_key, float a_default, float a_min, float a_max)
	{
		const auto it = a_ini.find(a_key);
		if (it == a_ini.end()) {
			return a_default;
		}
		char*       end = nullptr;
		const float v = std::strtof(it->second.c_str(), &end);
		if (end == it->second.c_str()) {
			logs::warn("settings: {} = '{}' is not a number, using {}", a_key, it->second, a_default);
			return a_default;
		}
		return std::clamp(v, a_min, a_max);
	}

	bool GetBool(const IniMap& a_ini, const char* a_key, bool a_default)
	{
		return GetFloat(a_ini, a_key, a_default ? 1.0f : 0.0f, 0.0f, 1.0f) > 0.5f;
	}

	int GetInt(const IniMap& a_ini, const char* a_key, int a_default, int a_min, int a_max)
	{
		return static_cast<int>(std::lround(GetFloat(a_ini, a_key, static_cast<float>(a_default), static_cast<float>(a_min), static_cast<float>(a_max))));
	}

	// Scan codes may be written in hex (0x44) or decimal (68).
	std::uint32_t GetKey(const IniMap& a_ini, const char* a_key, std::uint32_t a_default)
	{
		const auto it = a_ini.find(a_key);
		if (it == a_ini.end()) {
			return a_default;
		}
		char*               end = nullptr;
		const unsigned long v = std::strtoul(it->second.c_str(), &end, 0);
		if (end == it->second.c_str() || v > 0xFF) {
			logs::warn("settings: {} = '{}' is not a key code, using {:#x}", a_key, it->second, a_default);
			return a_default;
		}
		return static_cast<std::uint32_t>(v);
	}

	// Key and button codes accept decimal or 0x hex.
	std::uint32_t GetCode(const IniMap& a_ini, const char* a_key, std::uint32_t a_default)
	{
		const auto it = a_ini.find(a_key);
		if (it == a_ini.end()) {
			return a_default;
		}
		char*               end = nullptr;
		const unsigned long v = std::strtoul(it->second.c_str(), &end, 0);
		if (end == it->second.c_str()) {
			logs::warn("settings: {} = '{}' is not a key code, using {:#x}", a_key, it->second, a_default);
			return a_default;
		}
		return static_cast<std::uint32_t>(v);
	}
}

namespace Settings
{
	std::string PresetName()
	{
		std::scoped_lock guard(g_lock);
		return CameraPreset;
	}

	void SetPresetName(std::string_view a_name)
	{
		std::scoped_lock guard(g_lock);
		CameraPreset = a_name;
	}

	bool ApplyCameraPreset(std::string_view a_name)
	{
		std::scoped_lock guard(g_lock);
		for (const auto& preset : CameraPresets) {
			if (Lower(std::string(a_name)) == Lower(preset.name)) {
				CameraPreset = preset.name;
				LockPitch = true;
				Pitch = preset.pitch;
				CameraFov = preset.fov;
				CameraDistance = preset.distance;
				CameraOffsetX = 0.0f;
				CameraOffsetZ = 0.0f;
				ManageCamera = true;
				return true;
			}
		}
		return false;
	}

	void Load()
	{
		std::scoped_lock guard(g_lock);
		std::vector<std::pair<std::string, std::string>> gameSettings;
		const auto                                       ini = Parse(gameSettings);
		GameSettings = std::move(gameSettings);

		CameraEnabled = GetBool(ini, "Camera.Enabled", CameraEnabled);
		LockYaw = GetBool(ini, "Camera.LockYaw", LockYaw);
		LockPitch = GetBool(ini, "Camera.LockPitch", LockPitch);
		Yaw = GetFloat(ini, "Camera.Yaw", Yaw, -360.0f, 360.0f);
		Pitch = GetFloat(ini, "Camera.Pitch", Pitch, 5.0f, 85.0f);
		PitchMode = GetInt(ini, "Camera.PitchMode", PitchMode, 0, 1);
		InvertPitch = GetBool(ini, "Camera.InvertPitch", InvertPitch);
		PauseOnTargetLock = GetBool(ini, "Camera.PauseOnTargetLock", PauseOnTargetLock);
		FixInteriorRooms = GetBool(ini, "Camera.FixInteriorRooms", FixInteriorRooms);
		KeepHeading = GetBool(ini, "Camera.KeepHeading", KeepHeading);
		ManageCamera = GetBool(ini, "Camera.Manage", ManageCamera);
		CameraDistance = GetFloat(ini, "Camera.Distance", CameraDistance, 100.0f, 3000.0f);
		CameraFov = GetFloat(ini, "Camera.Fov", CameraFov, 30.0f, 120.0f);
		CameraOffsetX = GetFloat(ini, "Camera.OffsetX", CameraOffsetX, -300.0f, 300.0f);
		CameraOffsetZ = GetFloat(ini, "Camera.OffsetZ", CameraOffsetZ, -300.0f, 300.0f);
		WheelZoom = GetBool(ini, "Camera.WheelZoom", WheelZoom);
		// A named preset overrides the pitch, distance and field of view lines above. An INI with
		// no Preset line but its own camera numbers (from before presets) keeps them as Custom.
		// With neither, the built-in default (Path of Exile 2) stands.
		if (const auto it = ini.find("Camera.Preset"); it != ini.end()) {
			CameraPreset = "Custom";
			if (!ApplyCameraPreset(it->second) && Lower(it->second) != "custom") {
				logs::warn("settings: Camera.Preset = '{}' is not a known preset, using the values in the INI", it->second);
			}
		} else if (ini.contains("Camera.Pitch") || ini.contains("Camera.Distance") || ini.contains("Camera.Fov")) {
			CameraPreset = "Custom";
		}
		PitchStep = GetFloat(ini, "Camera.PitchStep", PitchStep, 0.5f, 45.0f);
		CameraToggleKey = GetKey(ini, "Camera.ToggleKey", CameraToggleKey);
		PitchUpKey = GetKey(ini, "Camera.PitchUpKey", PitchUpKey);
		PitchDownKey = GetKey(ini, "Camera.PitchDownKey", PitchDownKey);
		YawStepKey = GetKey(ini, "Camera.YawStepKey", YawStepKey);
		ReloadKey = GetKey(ini, "Camera.ReloadKey", ReloadKey);
		CameraDebugLog = GetBool(ini, "Camera.DebugLog", CameraDebugLog);

		CarryWeight = GetFloat(ini, "Player.CarryWeight", CarryWeight, 0.0f, 1000000.0f);
		SpeedMult = GetFloat(ini, "Player.SpeedMult", SpeedMult, 0.0f, 1000.0f);

		CutawayEnabled = GetBool(ini, "Cutaway.Enabled", CutawayEnabled);
		InteriorOnly = GetBool(ini, "Cutaway.InteriorOnly", InteriorOnly);
		CutawayMode = GetInt(ini, "Cutaway.Mode", CutawayMode, 0, 1);

		CutHeight = GetFloat(ini, "Cut.Height", CutHeight, 0.0f, 2000.0f);
		CutRadius = GetFloat(ini, "Cut.Radius", CutRadius, 0.0f, 100000.0f);
		CutAllGeometry = GetBool(ini, "Cut.AllGeometry", CutAllGeometry);
		CutSlope = GetFloat(ini, "Cut.Slope", CutSlope, 0.0f, 20.0f);
		ReanchorDistance = GetFloat(ini, "Cut.ReanchorDistance", ReanchorDistance, 0.0f, 100000.0f);
		ReanchorDelay = GetFloat(ini, "Cut.ReanchorDelay", ReanchorDelay, 0.0f, 30.0f);
		HeightSmoothing = GetFloat(ini, "Cut.HeightSmoothing", HeightSmoothing, 0.0f, 30.0f);
		MaxHeightSpeed = GetFloat(ini, "Cut.MaxHeightSpeed", MaxHeightSpeed, 1.0f, 100000.0f);
		AutoLift = GetBool(ini, "Cut.AutoLift", AutoLift);
		StyleKey = GetKey(ini, "Cut.StyleKey", StyleKey);
		ClearDelay = GetFloat(ini, "Cut.ClearDelay", ClearDelay, 0.0f, 30.0f);
		LiftMax = GetFloat(ini, "Cut.LiftMax", LiftMax, 0.0f, 100000.0f);

		CellOverrides.clear();
		for (const auto& [key, value] : ini) {
			if (key.rfind("Cells.", 0) != 0 || key.size() <= 6) {
				continue;
			}
			CellOverride entry;
			if (Lower(value) == "off") {
				entry.off = true;
			} else {
				char* end = nullptr;
				entry.height = std::strtof(value.c_str(), &end);
				if (end == value.c_str()) {
					logs::warn("settings: [Cells] {} = '{}' is not a number or 'off', ignored", key.substr(6), value);
					continue;
				}
			}
			CellOverrides[Lower(key.substr(6))] = entry;
		}

		HeadHeight = GetFloat(ini, "Sweep.HeadHeight", HeadHeight, 0.0f, 500.0f);
		CorridorRadius = GetFloat(ini, "Sweep.CorridorRadius", CorridorRadius, 0.0f, 5000.0f);
		SweepInterval = GetFloat(ini, "Sweep.Interval", SweepInterval, 0.0f, 1.0f);
		MinCameraDistance = GetFloat(ini, "Sweep.MinCameraDistance", MinCameraDistance, 0.0f, 1000.0f);
		MinHeight = GetFloat(ini, "Hide.MinHeight", MinHeight, -500.0f, 2000.0f);
		CeilingHeight = GetFloat(ini, "Hide.CeilingHeight", CeilingHeight, -500.0f, 5000.0f);
		BehindMargin = GetFloat(ini, "Hide.BehindMargin", BehindMargin, -2000.0f, 5000.0f);
		MaxShapeRadius = GetFloat(ini, "Hide.MaxShapeRadius", MaxShapeRadius, 0.0f, 100000.0f);
		Linger = GetFloat(ini, "Hide.Linger", Linger, 0.0f, 10.0f);
		Hysteresis = GetFloat(ini, "Hide.Hysteresis", Hysteresis, 0.0f, 1000.0f);

		BlackFog = GetBool(ini, "Background.BlackFog", BlackFog);
		HideSky = GetBool(ini, "Background.HideSky", HideSky);
		CutawayDebugLog = GetBool(ini, "Debug.Cutaway", CutawayDebugLog);

		logs::info("settings: camera enabled={} pitch={:.0f} lockPitch={} lockYaw={} yaw={:.0f} fixRooms={} keys toggle={:#x} pitch+={:#x} pitch-={:#x} reload={:#x}",
			CameraEnabled, Pitch, LockPitch, LockYaw, Yaw, FixInteriorRooms, CameraToggleKey, PitchUpKey, PitchDownKey, ReloadKey);
		logs::info("settings: player carry={:.0f} speed={:.0f} gameSettings={}", CarryWeight, SpeedMult, GameSettings.size());
		logs::info("settings: cutaway enabled={} interiorOnly={} mode={} height={:.0f} slope={:.2f} allGeometry={} autoLift={} styleKey={:#x}",
			CutawayEnabled, InteriorOnly, CutawayMode, CutHeight, CutSlope, CutAllGeometry, AutoLift, StyleKey);
		logs::info("settings: reanchor={:.0f}/{:.2f}s smoothing={:.2f}s maxSpeed={:.0f} clearDelay={:.2f} liftMax={:.0f} cells={} blackFog={} hideSky={} log={}",
			ReanchorDistance, ReanchorDelay, HeightSmoothing, MaxHeightSpeed, ClearDelay, LiftMax, CellOverrides.size(), BlackFog, HideSky, CutawayDebugLog);

		PointClickEnabled = GetBool(ini, "PointClick.Enabled", PointClickEnabled);
		ToggleKey = GetCode(ini, "PointClick.ToggleKey", ToggleKey);
		ClickMarker = GetBool(ini, "PointClick.ClickMarker", ClickMarker);

		Sensitivity = GetFloat(ini, "Cursor.Sensitivity", Sensitivity, 0.1f, 10.0f);
		CursorSize = GetFloat(ini, "Cursor.Size", CursorSize, 0.5f, 4.0f);
		CursorRing = GetBool(ini, "Cursor.Ring", CursorRing);
		MenuCursor = GetBool(ini, "Cursor.InMenus", MenuCursor);
		BlockMouseLook = GetBool(ini, "Cursor.BlockMouseLook", BlockMouseLook);
		RotateKey = GetCode(ini, "Cursor.RotateKey", RotateKey);

		MoveButton = GetCode(ini, "Move.Button", MoveButton);
		HoldToMove = GetBool(ini, "Move.HoldToMove", HoldToMove);
		ArriveRadius = GetFloat(ini, "Move.ArriveRadius", ArriveRadius, 5.0f, 500.0f);
		MaxDistance = GetFloat(ini, "Move.MaxDistance", MaxDistance, 200.0f, 20000.0f);
		MoveMode = GetInt(ini, "Move.Mode", MoveMode, 0, 2);
		ClickEnemies = GetBool(ini, "Move.ClickEnemies", ClickEnemies);
		ClickInteract = GetBool(ini, "Move.ClickInteract", ClickInteract);
		AttackInPlaceKey = GetCode(ini, "Move.AttackInPlaceKey", AttackInPlaceKey);
		FaceTime = GetFloat(ini, "Move.FaceTime", FaceTime, 0.0f, 1.0f);

		PickupButton = GetCode(ini, "Pickup.Button", PickupButton);
		PickupRange = GetFloat(ini, "Pickup.Range", PickupRange, 50.0f, 1000.0f);
		Containers = GetBool(ini, "Pickup.Containers", Containers);
		Actors = GetBool(ini, "Pickup.Actors", Actors);
		Activators = GetBool(ini, "Pickup.Activators", Activators);

		LabelMode = GetInt(ini, "Labels.Mode", LabelMode, 0, 2);
		LabelKey = GetCode(ini, "Labels.Key", LabelKey);
		LabelRadius = GetFloat(ini, "Labels.Radius", LabelRadius, 200.0f, 10000.0f);
		LabelMaxHeightDiff = GetFloat(ini, "Labels.MaxHeightDiff", LabelMaxHeightDiff, 50.0f, 10000.0f);
		LabelMaxBelow = GetFloat(ini, "Labels.MaxBelow", LabelMaxBelow, 0.0f, 10000.0f);
		MaxLabels = GetInt(ini, "Labels.Max", MaxLabels, 1, 300);
		LabelScale = GetFloat(ini, "Labels.Scale", LabelScale, 0.5f, 3.0f);
		LabelMinValue = GetInt(ini, "Labels.MinValue", LabelMinValue, 0, 1000000);
		StackCount = GetInt(ini, "Labels.StackCount", StackCount, 2, 1000000);
		ShowFood = GetBool(ini, "Labels.ShowFood", ShowFood);
		ShowIngredients = GetBool(ini, "Labels.ShowIngredients", ShowIngredients);
		ShowBooks = GetBool(ini, "Labels.ShowBooks", ShowBooks);
		ShowClutter = GetBool(ini, "Labels.ShowClutter", ShowClutter);

		HoldToAttack = GetBool(ini, "Combat.HoldToAttack", HoldToAttack);
		PowerAttackKey = GetCode(ini, "Combat.PowerAttackKey", PowerAttackKey);
		AutoAttack = GetBool(ini, "Combat.AutoAttack", AutoAttack);
		AimAtCursor = GetBool(ini, "Combat.AimAtCursor", AimAtCursor);
		AimProjectiles = GetBool(ini, "Combat.AimProjectiles", AimProjectiles);
		AimTracker = GetBool(ini, "Combat.AimTracker", AimTracker);
		AimMarker = GetBool(ini, "Combat.AimMarker", AimMarker);
		QuestActivators = GetBool(ini, "Outlines.QuestActivators", QuestActivators);
		HideGlobesInMenus = GetBool(ini, "Globes.HideInMenus", HideGlobesInMenus);
		Globes = GetBool(ini, "Globes.Enabled", Globes);
		HideVanillaBars = GetBool(ini, "Globes.HideVanillaBars", HideVanillaBars);
		GlobeSize = GetFloat(ini, "Globes.Size", GlobeSize, 0.5f, 2.0f);
		GlobeNumbers = GetBool(ini, "Globes.Numbers", GlobeNumbers);
		GlobeAnimate = GetBool(ini, "Globes.Animate", GlobeAnimate);
		Outlines = GetBool(ini, "Outlines.Enabled", Outlines);
		OutlineEnemies = GetBool(ini, "Outlines.Enemies", OutlineEnemies);
		OutlineNpcs = GetBool(ini, "Outlines.NPCs", OutlineNpcs);
		OutlinePlayer = GetBool(ini, "Outlines.Player", OutlinePlayer);
		OutlineFollowers = GetBool(ini, "Outlines.Followers", OutlineFollowers);
		OutlineFollowerColor = GetCode(ini, "Outlines.FollowerColor", OutlineFollowerColor) & 0xFFFFFF;
		// "Loot" was called "Items" before 1.6 (and only covered dropped items); an old INI's line still counts.
		OutlineLoot = GetBool(ini, "Outlines.Loot", GetBool(ini, "Outlines.Items", OutlineLoot));
		OutlineQuestItems = GetBool(ini, "Outlines.QuestItems", OutlineQuestItems);
		OutlineQuestItemColor = GetCode(ini, "Outlines.QuestItemColor", OutlineQuestItemColor) & 0xFFFFFF;
		OutlineThickness = GetInt(ini, "Outlines.Thickness", OutlineThickness, 1, 8);
		OutlineOpacity = GetFloat(ini, "Outlines.Opacity", OutlineOpacity, 0.05f, 1.0f);
		OutlineBrightness = GetFloat(ini, "Outlines.Brightness", OutlineBrightness, 0.1f, 1.0f);
		OutlineFade = GetBool(ini, "Outlines.Fade", OutlineFade);
		OutlineMaxAbove = GetFloat(ini, "Outlines.MaxAbove", OutlineMaxAbove, 0.0f, 10000.0f);
		OutlineMaxBelow = GetFloat(ini, "Outlines.MaxBelow", OutlineMaxBelow, 0.0f, 10000.0f);
		GroundCircles = GetBool(ini, "Outlines.Circles", GroundCircles);
		CircleRadius = GetFloat(ini, "Outlines.CircleRadius", CircleRadius, 10.0f, 300.0f);
		OutlineBehindPlayer = GetBool(ini, "Outlines.BehindPlayer", OutlineBehindPlayer);
		OutlineRadius = GetFloat(ini, "Outlines.Radius", OutlineRadius, 500.0f, 20000.0f);
		OutlineItemRadius = GetFloat(ini, "Outlines.ItemRadius", OutlineItemRadius, 100.0f, 10000.0f);
		OutlineMaxParts = GetInt(ini, "Outlines.MaxParts", OutlineMaxParts, 64, 1024);
		OutlinePartial = GetBool(ini, "Outlines.Partial", OutlinePartial);

		PointClickDebugLog = GetBool(ini, "Debug.PointClick", PointClickDebugLog);

		logs::info("settings: point and click enabled={} moveButton={} pickupButton={} labelMode={}", PointClickEnabled, MoveButton, PickupButton, LabelMode);
	}

	bool Save()
	{
		std::scoped_lock guard(g_lock);
		std::ofstream out(kIniPath, std::ios::trunc);
		if (!out.is_open()) {
			logs::error("settings: can't write {}", kIniPath);
			return false;
		}
		const auto b = [](bool a_v) { return a_v ? 1 : 0; };

		out << "; ARPG POV - settings. Editable here or in game (SKSE Menu Framework > ARPG POV).\n"
			<< "; Distances are Skyrim units (a human is roughly 128 tall). Key codes are DirectX scan codes.\n"
			<< "; Mouse buttons: 0 = left, 1 = right, 2 = middle (mouse 3), 3+ = side buttons.\n\n";

		out << "[Camera]\n"
			<< "; Camera lock on at game start (ToggleKey flips it in game).\n"
			<< std::format("Enabled = {}\n", b(CameraEnabled))
			<< "; Preset = Custom uses the Pitch, Distance and Fov lines below. Default is this mod's look (same as Path of Exile 2).\n"
			<< "; A game's name replaces them with that game's look: Skyrim ARPG, Diablo II, Path of Exile, Path of Exile 2, Diablo III, Diablo IV, Last Epoch, Grim Dawn, Torchlight II.\n"
			<< std::format("Preset = {}\n", CameraPreset)
			<< "; Pitch = degrees looking down (30 = Path of Exile 2). Yaw only applies with LockYaw.\n"
			<< std::format("LockPitch = {}\nPitch = {}\nLockYaw = {}\nYaw = {}\n", b(LockPitch), Pitch, b(LockYaw), Yaw)
			<< "; Leave at 0 unless the camera looks up instead of down.\n"
			<< std::format("PitchMode = {}\nInvertPitch = {}\n", PitchMode, b(InvertPitch))
			<< "; Let True Directional Movement hold the camera while target lock is on.\n"
			<< std::format("PauseOnTargetLock = {}\n", b(PauseOnTargetLock))
			<< "; Indoors, draw the room you're in even when the camera is above the ceiling.\n"
			<< std::format("FixInteriorRooms = {}\n", b(FixInteriorRooms))
			<< "; Keep the camera's compass heading when True Directional Movement tries to turn it (target lock, its camera\n"
			<< "; reset, its auto-adjust). Only acts while the mouse is not turning the camera. 0 = leave the heading to TDM.\n"
			<< std::format("KeepHeading = {}\n", b(KeepHeading))
			<< "; Replaces 3PCO. Manage = 1 applies: Distance from you, Fov in degrees, shoulder offsets (0 = centred),\n"
			<< "; and WheelZoom (0 = the mouse wheel does not zoom; use the change-view key for first person).\n"
			<< std::format("Manage = {}\nDistance = {}\nFov = {}\nOffsetX = {}\nOffsetZ = {}\nWheelZoom = {}\n", b(ManageCamera), CameraDistance, CameraFov,
				   CameraOffsetX, CameraOffsetZ, b(WheelZoom))
			<< "; Keys: 0x44 = F10, 0xc9 = Page Up, 0xd1 = Page Down, 0x42 = F8. 0 = no key. (0x49 / 0x51 are Numpad 9 / 3.)\n"
			<< std::format("ToggleKey = {:#x}\nPitchUpKey = {:#x}\nPitchDownKey = {:#x}\nPitchStep = {}\nReloadKey = {:#x}\n",
				   CameraToggleKey, PitchUpKey, PitchDownKey, PitchStep, ReloadKey)
			<< "; YawStepKey turns the camera 90 degrees per press (0x3c = F2; it was Insert, 0xd2, before 1.6.1). It only works with LockYaw = 1.\n"
			<< std::format("YawStepKey = {:#x}\n", YawStepKey)
			<< std::format("DebugLog = {}\n\n", b(CameraDebugLog));

		out << "[Player]\n"
			<< "; Applied whenever a save loads. 0 = leave the game's value alone. SpeedMult 100 = vanilla.\n"
			<< std::format("CarryWeight = {}\nSpeedMult = {}\n\n", CarryWeight, SpeedMult);

		out << "[GameSettings]\n"
			<< "; Engine settings (the names `setgs` uses), re-applied every launch.\n";
		for (const auto& [name, value] : GameSettings) {
			out << name << " = " << value << '\n';
		}
		out << '\n';

		out << "[Cutaway]\n"
			<< "; Cutaway master switch, interiors only, and method (1 = cut, 0 = legacy per-mesh hiding).\n"
			<< std::format("Enabled = {}\nInteriorOnly = {}\nMode = {}\n\n", b(CutawayEnabled), b(InteriorOnly), CutawayMode);

		out << "[Cut]\n"
			<< "; Cut height above the floor. Slope applies to the Automatic style only: the cut is at Height at the bottom of the\n"
			<< "; screen and rises this fast away from the camera (0 = flat), so walls are never cut lower than in the Default style.\n"
			<< std::format("Height = {}\nSlope = {}\n", CutHeight, CutSlope)
			<< "; 1 = every mesh uses the cut projection so depth stays consistent.\n"
			<< std::format("AllGeometry = {}\nRadius = {}\n", b(CutAllGeometry), CutRadius)
			<< "; Height is anchored per area; it re-anchors only after standing this far\n"
			<< "; above/below it for ReanchorDelay seconds on the ground (jumps never count).\n"
			<< std::format("ReanchorDistance = {}\nReanchorDelay = {}\n", ReanchorDistance, ReanchorDelay)
			<< "; Every height change eases in over ~HeightSmoothing seconds, capped at MaxHeightSpeed.\n"
			<< std::format("HeightSmoothing = {}\nMaxHeightSpeed = {}\n", HeightSmoothing, MaxHeightSpeed)
			<< "; Style: 0 = Default, always a flat cut at Height. 1 = Automatic (experimental): never lower than that, sloping up\n"
			<< "; away from the camera, and rising (up to LiftMax) so walls and ceilings show when nothing is in the way.\n"
			<< "; StyleKey switches in game (0x43 = F9).\n"
			<< std::format("AutoLift = {}\nStyleKey = {:#x}\nClearDelay = {}\nLiftMax = {}\n\n", b(AutoLift), StyleKey, ClearDelay, LiftMax);

		out << "[Cells]\n"
			<< "; Per-interior overrides by editor ID (see the log's [cell] lines): a height, or off.\n";
		for (const auto& [name, entry] : CellOverrides) {
			if (entry.off) {
				out << name << " = off\n";
			} else {
				out << std::format("{} = {}\n", name, entry.height);
			}
		}
		out << '\n';

		out << "[Background]\n"
			<< "; Black interior fog and no sky dome while inside (what shows through the cut).\n"
			<< std::format("BlackFog = {}\nHideSky = {}\n\n", b(BlackFog), b(HideSky));

		out << "[Sweep]\n; Mode 0 (legacy) only.\n"
			<< std::format("HeadHeight = {}\nCorridorRadius = {}\nInterval = {}\nMinCameraDistance = {}\n\n",
				   HeadHeight, CorridorRadius, SweepInterval, MinCameraDistance)
			<< "[Hide]\n; Mode 0 (legacy) only.\n"
			<< std::format("MinHeight = {}\nCeilingHeight = {}\nBehindMargin = {}\nMaxShapeRadius = {}\nLinger = {}\nHysteresis = {}\n\n",
				   MinHeight, CeilingHeight, BehindMargin, MaxShapeRadius, Linger, Hysteresis);

		out << "[PointClick]\n";
		out << "; Point-and-click on at game start. ToggleKey (0x41 = F7) switches back to WASD + mouse look.\n";
		out << std::format("Enabled = {}\nToggleKey = {:#x}\n", b(PointClickEnabled), ToggleKey);
		out << "; ClickMarker = a small ring on the ground where a click is sending you. It fades when you arrive or cancel.\n";
		out << std::format("ClickMarker = {}\n\n", b(ClickMarker));

		out << "[Cursor]\n";
		out << "; Cursor speed and size. The cursor can never leave the screen.\n";
		out << std::format("Sensitivity = {}\nSize = {}\n", Sensitivity, CursorSize);
		out << "; 1 = a ring behind the sword, loot bag and exclamation mark, in the symbol's colour.\n";
		out << std::format("Ring = {}\n", b(CursorRing));
		out << "; 1 = this cursor is also used in the inventory, containers, barter and other menus, in place of the game's.\n";
		out << std::format("InMenus = {}\n", b(MenuCursor));
		out << "; 1 = the mouse moves the cursor instead of the camera. Hold RotateKey to rotate the camera (0 = no key).\n";
		out << std::format("BlockMouseLook = {}\nRotateKey = {:#x}\n\n", b(BlockMouseLook), RotateKey);

		out << "[Move]\n";
		out << "; Button that moves you to the cursor. HoldToMove = keep following the cursor while it's held.\n";
		out << std::format("Button = {}\nHoldToMove = {}\n", MoveButton, b(HoldToMove));
		out << "; Stop this close to the clicked point, and the furthest one click can send you (Skyrim units).\n";
		out << std::format("ArriveRadius = {}\nMaxDistance = {}\n", ArriveRadius, MaxDistance);
		out << "; 0 = auto, 1 = True Directional Movement, 2 = turn the player directly (no TDM).\n";
		out << std::format("Mode = {}\n", MoveMode);
		out << "; Clicking a hostile attacks it. AttackInPlaceKey + click attacks toward the cursor (0x2A = Left Shift).\n";
		out << "; ClickInteract = clicking an item, container, corpse, door or NPC walks over and uses it.\n";
		out << std::format("ClickEnemies = {}\nClickInteract = {}\nAttackInPlaceKey = {:#x}\nFaceTime = {}\n\n", b(ClickEnemies), b(ClickInteract),
			AttackInPlaceKey, FaceTime);

		out << "[Pickup]\n";
		out << "; Extra button that uses what's under the cursor (2 = middle mouse / mouse 3). Out of Range, you walk there first.\n";
		out << std::format("Button = {}\nRange = {}\n", PickupButton, PickupRange);
		out << "; Containers = chests and barrels can be clicked. Actors = corpses (loot) and NPCs (talk) can be clicked.\n";
		out << "; Activators = furniture and activators that have a name (seats, levers, the ferry seat) can be clicked. No label.\n";
		out << std::format("Containers = {}\nActors = {}\nActivators = {}\n\n", b(Containers), b(Actors), b(Activators));

		out << "[Labels]\n";
		out << "; 0 = only the item under the cursor, 1 = always, 2 = while Key is held (0x38 = Left Alt).\n";
		out << std::format("Mode = {}\nKey = {:#x}\n", LabelMode, LabelKey);
		out << "; How far away items get a label, and how far above/below you (keeps other floors out).\n";
		out << std::format("Radius = {}\nMaxHeightDiff = {}\n", LabelRadius, LabelMaxHeightDiff);
		out << "; Items lower than MaxBelow under your feet get no label or click (floors below, hidden merchant chest rooms).\n";
		out << std::format("MaxBelow = {}\n", LabelMaxBelow);
		out << "; Most labels on screen, text size, and a loot filter: hide items worth less than MinValue.\n";
		out << std::format("Max = {}\nScale = {}\nMinValue = {}\n", MaxLabels, LabelScale, LabelMinValue);
		out << "; Stacks of at least StackCount read \"200x Gold\"; smaller stacks show the name only (2 = always show the count).\n";
		out << std::format("StackCount = {}\n", StackCount);
		out << "; Loot filter. Always shown: weapons, armor, ammo, potions, scrolls, spell tomes, keys, soul gems, gold, lockpicks,\n";
		out << "; gems, ore and ingots. These add the rest: food, ingredients, other books, and clutter (kettles, buckets...).\n";
		out << std::format("ShowFood = {}\nShowIngredients = {}\nShowBooks = {}\nShowClutter = {}\n\n", b(ShowFood), b(ShowIngredients), b(ShowBooks),
			b(ShowClutter));

		out << "[Combat]\n";
		out << "; HoldToAttack = holding your attack button swings again and again (melee). Hold PowerAttackKey with it for a\n";
		out << "; power attack, as in the base game (0x38 = Left Alt, 0 = no key).\n";
		out << std::format("HoldToAttack = {}\nPowerAttackKey = {:#x}\n", b(HoldToAttack), PowerAttackKey);
		out << "; AutoAttack = while locked on (True Directional Movement), one press of attack keeps attacking until you press again.\n";
		out << std::format("AutoAttack = {}\n", b(AutoAttack));
		out << "; AimAtCursor = spells and bows aim at the cursor (or the enemy under it) and not along the tilted camera.\n";
		out << std::format("AimAtCursor = {}\n", b(AimAtCursor));
		out << "; AimProjectiles = your spells and arrows are steered at the aim point as they leave (fixes shots going sideways).\n";
		out << "; AimTracker = a dashed line to the aim point while casting or drawing. AimMarker = a red ring where the shot goes.\n";
		out << std::format("AimProjectiles = {}\nAimTracker = {}\nAimMarker = {}\n\n", b(AimProjectiles), b(AimTracker), b(AimMarker));

		out << "[Globes]\n";
		out << "; Life and mana globes (Path of Exile style), stamina as a bar on the life globe. HideVanillaBars hides the game's bars.\n";
		out << std::format("Enabled = {}\nHideVanillaBars = {}\nSize = {}\nNumbers = {}\nAnimate = {}\n", b(Globes), b(HideVanillaBars), GlobeSize,
			b(GlobeNumbers), b(GlobeAnimate));
		out << "; HideInMenus = the globes hide while the inventory, a container, a shop or the magic menu is open.\n";
		out << std::format("HideInMenus = {}\n\n", b(HideGlobesInMenus));

		out << "[Outlines]\n";
		out << "; True outlines around the model: red = enemies, green = NPCs, FollowerColor = followers, white = you.\n";
		out << "; Whatever the cursor is on is always outlined.\n";
		out << std::format("Enabled = {}\nEnemies = {}\nNPCs = {}\nPlayer = {}\n", b(Outlines), b(OutlineEnemies), b(OutlineNpcs), b(OutlinePlayer));
		out << "; Followers = your followers / teammates get their own colour (0xRRGGBB) and their own switch.\n";
		out << std::format("Followers = {}\nFollowerColor = {:#08x}\n", b(OutlineFollowers), OutlineFollowerColor);
		out << "; Loot = dropped items (their label colour), containers and corpses (gold). Called Items before 1.6.\n";
		out << "; Weapons and shields an NPC is holding are part of that NPC and can't be outlined separately.\n";
		out << std::format("Loot = {}\n", b(OutlineLoot));
		out << "; QuestItems = quest items lying in the world always get a label and an outline in QuestItemColor (0xRRGGBB), even with\n";
		out << "; Loot = 0 and whatever the loot filter says. The distance and height limits still apply.\n";
		out << std::format("QuestItems = {}\nQuestItemColor = {:#08x}\n", b(OutlineQuestItems), OutlineQuestItemColor);
		out << "; QuestActivators = levers, pull chains and quest activators get the quest colour and an outline (needs QuestItems).\n";
		out << std::format("QuestActivators = {}\n", b(QuestActivators));
		out << "; Thickness = glow size, 1 (a thin line) to 8. Opacity = how strong the glow is. Brightness = how bright\n";
		out << "; its colours are (lower = darker). Fade = outlines get fainter with distance.\n";
		out << std::format("Thickness = {}\nOpacity = {}\nBrightness = {}\nFade = {}\n", OutlineThickness, OutlineOpacity, OutlineBrightness, b(OutlineFade));
		out << "; Radius / ItemRadius: how far from you actors / items get an outline. Each outlined thing costs a little,\n";
		out << "; so lower the two radii if frames drop.\n";
		out << std::format("Radius = {}\nItemRadius = {}\n", OutlineRadius, OutlineItemRadius);
		out << "; MaxParts = most model parts outlined in one frame (64-1024). An armoured character is many parts, so a crowd\n";
		out << "; can reach it; past it the furthest characters get no outline. Higher outlines more of a crowd and costs frames.\n";
		out << std::format("MaxParts = {}\n", OutlineMaxParts);
		out << "; Partial = 0: a character whose model could only be read in part gets no outline (unless the cursor is on it).\n";
		out << "; 1 = outline whatever parts were read, even if that is only a helmet or a pair of boots.\n";
		out << std::format("Partial = {}\n", b(OutlinePartial));
		out << "; Items, containers and corpses higher than this above your feet get no outline or label (shelves, upper floors).\n";
		out << std::format("MaxAbove = {}\n", OutlineMaxAbove);
		out << "; Enemies, NPCs and followers lower than MaxBelow under your feet (or further than [Labels] MaxHeightDiff above\n";
		out << "; or below) get no outline or circle. Between the two they still glow while the cursor is on them.\n";
		out << std::format("MaxBelow = {}\n", OutlineMaxBelow);
		out << "; Circles = a circle on the ground under enemies, NPCs and followers, in the outline's colour. Uses the Enemies,\n";
		out << "; NPCs and Followers switches above and works with Enabled = 0. Drawn over the scene, so it shows through walls.\n";
		out << std::format("Circles = {}\nCircleRadius = {}\n", b(GroundCircles), CircleRadius);
		out << "; 1 = things behind you don't glow over your character.\n";
		out << std::format("BehindPlayer = {}\n\n", b(OutlineBehindPlayer));

		out << "[Debug]\n"
			<< std::format("Cutaway = {}\nPointClick = {}\n", b(CutawayDebugLog), b(PointClickDebugLog));

		out.flush();
		if (!out.good()) {
			logs::error("settings: writing {} failed", kIniPath);
			return false;
		}
		logs::info("settings: saved to {}", kIniPath);
		return true;
	}

	const CellOverride* FindCellOverride(const char* a_editorID)
	{
		if (!a_editorID || !a_editorID[0]) {
			return nullptr;
		}
		const auto it = CellOverrides.find(Lower(a_editorID));
		return it != CellOverrides.end() ? &it->second : nullptr;
	}
}
