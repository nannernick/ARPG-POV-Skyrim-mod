#include "Settings.h"

namespace
{
	constexpr const char* kIniPath = "Data\\SKSE\\Plugins\\ARPGPOV.ini";

	using IniMap = std::unordered_map<std::string, std::string>;

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
	void Load()
	{
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
		PitchStep = GetFloat(ini, "Camera.PitchStep", PitchStep, 0.5f, 45.0f);
		CameraToggleKey = GetKey(ini, "Camera.ToggleKey", CameraToggleKey);
		PitchUpKey = GetKey(ini, "Camera.PitchUpKey", PitchUpKey);
		PitchDownKey = GetKey(ini, "Camera.PitchDownKey", PitchDownKey);
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

		LabelMode = GetInt(ini, "Labels.Mode", LabelMode, 0, 2);
		LabelKey = GetCode(ini, "Labels.Key", LabelKey);
		LabelRadius = GetFloat(ini, "Labels.Radius", LabelRadius, 200.0f, 10000.0f);
		LabelMaxHeightDiff = GetFloat(ini, "Labels.MaxHeightDiff", LabelMaxHeightDiff, 50.0f, 10000.0f);
		MaxLabels = GetInt(ini, "Labels.Max", MaxLabels, 1, 300);
		LabelScale = GetFloat(ini, "Labels.Scale", LabelScale, 0.5f, 3.0f);
		LabelMinValue = GetInt(ini, "Labels.MinValue", LabelMinValue, 0, 1000000);
		StackCount = GetInt(ini, "Labels.StackCount", StackCount, 2, 1000000);
		ShowFood = GetBool(ini, "Labels.ShowFood", ShowFood);
		ShowIngredients = GetBool(ini, "Labels.ShowIngredients", ShowIngredients);
		ShowBooks = GetBool(ini, "Labels.ShowBooks", ShowBooks);
		ShowClutter = GetBool(ini, "Labels.ShowClutter", ShowClutter);

		Outlines = GetBool(ini, "Outlines.Enabled", Outlines);
		OutlineEnemies = GetBool(ini, "Outlines.Enemies", OutlineEnemies);
		OutlineNpcs = GetBool(ini, "Outlines.NPCs", OutlineNpcs);
		OutlineItems = GetBool(ini, "Outlines.Items", OutlineItems);
		OutlineThickness = GetInt(ini, "Outlines.Thickness", OutlineThickness, 1, 8);
		OutlineOpacity = GetFloat(ini, "Outlines.Opacity", OutlineOpacity, 0.05f, 1.0f);
		OutlineBrightness = GetFloat(ini, "Outlines.Brightness", OutlineBrightness, 0.1f, 1.0f);
		OutlineFade = GetBool(ini, "Outlines.Fade", OutlineFade);
		OutlineMaxAbove = GetFloat(ini, "Outlines.MaxAbove", OutlineMaxAbove, 0.0f, 10000.0f);
		OutlineBehindPlayer = GetBool(ini, "Outlines.BehindPlayer", OutlineBehindPlayer);
		OutlineRadius = GetFloat(ini, "Outlines.Radius", OutlineRadius, 500.0f, 20000.0f);
		OutlineItemRadius = GetFloat(ini, "Outlines.ItemRadius", OutlineItemRadius, 100.0f, 10000.0f);

		PointClickDebugLog = GetBool(ini, "Debug.PointClick", PointClickDebugLog);

		logs::info("settings: point and click enabled={} moveButton={} pickupButton={} labelMode={}", PointClickEnabled, MoveButton, PickupButton, LabelMode);
	}

	bool Save()
	{
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
			<< "; Pitch = degrees looking down (30 = Path of Exile 2). Yaw only applies with LockYaw.\n"
			<< std::format("LockPitch = {}\nPitch = {}\nLockYaw = {}\nYaw = {}\n", b(LockPitch), Pitch, b(LockYaw), Yaw)
			<< "; Leave at 0 unless the camera looks up instead of down.\n"
			<< std::format("PitchMode = {}\nInvertPitch = {}\n", PitchMode, b(InvertPitch))
			<< "; Let True Directional Movement hold the camera while target lock is on.\n"
			<< std::format("PauseOnTargetLock = {}\n", b(PauseOnTargetLock))
			<< "; Indoors, draw the room you're in even when the camera is above the ceiling.\n"
			<< std::format("FixInteriorRooms = {}\n", b(FixInteriorRooms))
			<< "; Keys: 0x44 = F10, 0x49 = Page Up, 0x51 = Page Down, 0x42 = F8. 0 = no key.\n"
			<< std::format("ToggleKey = {:#x}\nPitchUpKey = {:#x}\nPitchDownKey = {:#x}\nPitchStep = {}\nReloadKey = {:#x}\n",
				   CameraToggleKey, PitchUpKey, PitchDownKey, PitchStep, ReloadKey)
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
			<< "; Cut height above the floor at your position, and how fast it rises behind you\n"
			<< "; (1 = 45 deg, 0 = flat; clamped automatically to keep the floor on screen).\n"
			<< std::format("Height = {}\nSlope = {}\n", CutHeight, CutSlope)
			<< "; 1 = every mesh uses the cut projection so depth stays consistent.\n"
			<< std::format("AllGeometry = {}\nRadius = {}\n", b(CutAllGeometry), CutRadius)
			<< "; Height is anchored per area; it re-anchors only after standing this far\n"
			<< "; above/below it for ReanchorDelay seconds on the ground (jumps never count).\n"
			<< std::format("ReanchorDistance = {}\nReanchorDelay = {}\n", ReanchorDistance, ReanchorDelay)
			<< "; Every height change eases in over ~HeightSmoothing seconds, capped at MaxHeightSpeed.\n"
			<< std::format("HeightSmoothing = {}\nMaxHeightSpeed = {}\n", HeightSmoothing, MaxHeightSpeed)
			<< "; Style: 0 = always cut (Diablo look), 1 = automatic, cut only when something blocks the view (experimental).\n"
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
		out << std::format("Enabled = {}\nToggleKey = {:#x}\n\n", b(PointClickEnabled), ToggleKey);

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
		out << std::format("Containers = {}\nActors = {}\n\n", b(Containers), b(Actors));

		out << "[Labels]\n";
		out << "; 0 = only the item under the cursor, 1 = always, 2 = while Key is held (0x38 = Left Alt).\n";
		out << std::format("Mode = {}\nKey = {:#x}\n", LabelMode, LabelKey);
		out << "; How far away items get a label, and how far above/below you (keeps other floors out).\n";
		out << std::format("Radius = {}\nMaxHeightDiff = {}\n", LabelRadius, LabelMaxHeightDiff);
		out << "; Most labels on screen, text size, and a loot filter: hide items worth less than MinValue.\n";
		out << std::format("Max = {}\nScale = {}\nMinValue = {}\n", MaxLabels, LabelScale, LabelMinValue);
		out << "; Stacks of at least StackCount read \"200x Gold\"; smaller stacks show the name only (2 = always show the count).\n";
		out << std::format("StackCount = {}\n", StackCount);
		out << "; Loot filter. Always shown: weapons, armor, ammo, potions, scrolls, spell tomes, keys, soul gems, gold, lockpicks,\n";
		out << "; gems, ore and ingots. These add the rest: food, ingredients, other books, and clutter (kettles, buckets...).\n";
		out << std::format("ShowFood = {}\nShowIngredients = {}\nShowBooks = {}\nShowClutter = {}\n\n", b(ShowFood), b(ShowIngredients), b(ShowBooks),
			b(ShowClutter));

		out << "[Outlines]\n";
		out << "; True outlines around the model: red = enemies, white = you and NPCs, items = their label colour.\n";
		out << "; Whatever the cursor is on is always outlined.\n";
		out << std::format("Enabled = {}\nEnemies = {}\nNPCs = {}\nItems = {}\n", b(Outlines), b(OutlineEnemies), b(OutlineNpcs), b(OutlineItems));
		out << "; Thickness = glow size, 1 (a thin line) to 8. Opacity = how strong the glow is. Brightness = how bright\n";
		out << "; its colours are (lower = darker). Fade = outlines get fainter with distance.\n";
		out << std::format("Thickness = {}\nOpacity = {}\nBrightness = {}\nFade = {}\n", OutlineThickness, OutlineOpacity, OutlineBrightness, b(OutlineFade));
		out << "; Radius / ItemRadius: how far from you actors / items get an outline. Each outlined thing costs a little,\n";
		out << "; so lower the two radii if frames drop.\n";
		out << std::format("Radius = {}\nItemRadius = {}\n", OutlineRadius, OutlineItemRadius);
		out << "; Items, containers and corpses higher than this above your feet get no outline or label (shelves, upper floors).\n";
		out << std::format("MaxAbove = {}\n", OutlineMaxAbove);
		out << "; 1 = things behind you don't glow over your character.\n";
		out << std::format("BehindPlayer = {}\n\n", b(OutlineBehindPlayer));

		out << "[Debug]\n"
			<< std::format("Cutaway = {}\nPointClick = {}\n", b(CutawayDebugLog), b(PointClickDebugLog));

		logs::info("settings: saved to {}", kIniPath);
		return out.good();
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
