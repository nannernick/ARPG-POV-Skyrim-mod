#include "Settings.h"

namespace
{
	constexpr const char* kIniPath = "Data\\SKSE\\Plugins\\ARPGPointClick.ini";

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

	// Flattens the INI into "Section.Key" -> value.
	IniMap Parse()
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
			out[section + "." + key] = value;
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
		return static_cast<int>(GetFloat(a_ini, a_key, static_cast<float>(a_default), static_cast<float>(a_min), static_cast<float>(a_max)));
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
		const auto ini = Parse();

		Enabled = GetBool(ini, "General.Enabled", Enabled);
		ToggleKey = GetCode(ini, "General.ToggleKey", ToggleKey);

		Sensitivity = GetFloat(ini, "Cursor.Sensitivity", Sensitivity, 0.1f, 10.0f);
		CursorSize = GetFloat(ini, "Cursor.Size", CursorSize, 0.5f, 4.0f);
		CursorRing = GetBool(ini, "Cursor.Ring", CursorRing);
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
		ShowFood = GetBool(ini, "Labels.ShowFood", ShowFood);
		ShowIngredients = GetBool(ini, "Labels.ShowIngredients", ShowIngredients);
		ShowBooks = GetBool(ini, "Labels.ShowBooks", ShowBooks);
		ShowClutter = GetBool(ini, "Labels.ShowClutter", ShowClutter);

		Outlines = GetBool(ini, "Outlines.Enabled", Outlines);
		OutlineEnemies = GetBool(ini, "Outlines.Enemies", OutlineEnemies);
		OutlineNpcs = GetBool(ini, "Outlines.NPCs", OutlineNpcs);
		OutlineItems = GetBool(ini, "Outlines.Items", OutlineItems);
		OutlineThickness = GetInt(ini, "Outlines.Thickness", OutlineThickness, 1, 8);
		OutlineRadius = GetFloat(ini, "Outlines.Radius", OutlineRadius, 500.0f, 20000.0f);
		OutlineItemRadius = GetFloat(ini, "Outlines.ItemRadius", OutlineItemRadius, 100.0f, 10000.0f);

		DebugLog = GetBool(ini, "Debug.Log", DebugLog);

		logs::info("settings loaded: enabled={} moveButton={} pickupButton={} labelMode={}", Enabled, MoveButton, PickupButton, LabelMode);
	}

	bool Save()
	{
		std::ofstream out(kIniPath, std::ios::trunc);
		if (!out.is_open()) {
			logs::error("settings: couldn't write {}", kIniPath);
			return false;
		}
		const auto b = [](bool a_v) { return a_v ? 1 : 0; };

		out << "; ARPG Point and Click - settings. Editable here or in game (SKSE Menu Framework > ARPG Point and Click).\n";
		out << "; Key codes are DirectX scan codes. Mouse buttons: 0 = left, 1 = right, 2 = middle (mouse 3), 3+ = side buttons.\n\n";

		out << "[General]\n";
		out << "; Point-and-click on at game start. ToggleKey (0x41 = F7) switches back to WASD + mouse look.\n";
		out << std::format("Enabled = {}\nToggleKey = {:#x}\n\n", b(Enabled), ToggleKey);

		out << "[Cursor]\n";
		out << "; Cursor speed and size. The cursor can never leave the screen.\n";
		out << std::format("Sensitivity = {}\nSize = {}\n", Sensitivity, CursorSize);
		out << "; 1 = a ring behind the sword, loot bag and exclamation mark, in the symbol's colour.\n";
		out << std::format("Ring = {}\n", b(CursorRing));
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
		out << "; Loot filter. Always shown: weapons, armor, ammo, potions, scrolls, spell tomes, keys, soul gems, gold, lockpicks,\n";
		out << "; gems, ore and ingots. These add the rest: food, ingredients, other books, and clutter (kettles, buckets...).\n";
		out << std::format("ShowFood = {}\nShowIngredients = {}\nShowBooks = {}\nShowClutter = {}\n\n", b(ShowFood), b(ShowIngredients), b(ShowBooks),
			b(ShowClutter));

		out << "[Outlines]\n";
		out << "; True outlines around the model: red = enemies, white = you and NPCs, items = their label colour.\n";
		out << "; Whatever the cursor is on is always outlined.\n";
		out << std::format("Enabled = {}\nEnemies = {}\nNPCs = {}\nItems = {}\n", b(Outlines), b(OutlineEnemies), b(OutlineNpcs), b(OutlineItems));
		out << "; Thickness: 1 = thinnest, up to 8. Radius / ItemRadius: how far from you actors / items get an outline.\n";
		out << "; Each outlined thing costs a little, so lower the two radii if frames drop.\n";
		out << std::format("Thickness = {}\nRadius = {}\nItemRadius = {}\n\n", OutlineThickness, OutlineRadius, OutlineItemRadius);

		out << "[Debug]\n";
		out << std::format("Log = {}\n", b(DebugLog));

		return out.good();
	}
}
