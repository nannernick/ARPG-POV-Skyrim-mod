#pragma once

// All settings live in Data/SKSE/Plugins/ARPGPointClick.ini. The in-game page edits these
// values directly (they apply live) and Save() writes them back.
// Key codes are DirectX scan codes. Mouse buttons: 0 = left, 1 = right, 2 = middle (mouse 3), 3+ = side buttons.
namespace Settings
{
	// [General]
	inline bool          Enabled = true;              // point-and-click on at game start
	inline std::uint32_t ToggleKey = 0x41;            // F7: switch between point-and-click and normal WASD + mouse look

	// [Cursor]
	inline float         Sensitivity = 1.0f;          // cursor speed
	inline float         CursorSize = 1.0f;
	inline bool          CursorRing = true;           // ring behind the sword / bag / exclamation mark, in the symbol's colour
	inline bool          BlockMouseLook = true;       // the mouse moves the cursor, not the camera
	inline std::uint32_t RotateKey = 0;               // hold to rotate the camera with the mouse (0 = none)

	// [Move]
	inline std::uint32_t MoveButton = 0;              // left mouse button
	inline bool          HoldToMove = true;           // holding the button keeps following the cursor
	inline float         ArriveRadius = 40.0f;        // stop this close to the clicked point
	inline float         MaxDistance = 3000.0f;       // furthest a click can send you
	inline int           MoveMode = 0;                // 0 = auto, 1 = True Directional Movement, 2 = turn the player (no TDM)
	inline bool          ClickEnemies = true;         // clicking a hostile attacks it instead of moving
	inline bool          ClickInteract = true;        // clicking an item, container, corpse, door or NPC walks over and uses it
	inline std::uint32_t AttackInPlaceKey = 0x2A;     // Left Shift + click: attack toward the cursor without moving
	inline float         FaceTime = 0.15f;            // seconds spent turning toward the target when attacking

	// [Pickup]
	inline std::uint32_t PickupButton = 2;            // middle mouse button (mouse 3)
	inline float         PickupRange = 150.0f;        // closer than this picks up at once; further walks there first
	inline bool          Containers = true;           // containers can be clicked
	inline bool          Actors = true;               // corpses and NPCs can be clicked

	// [Labels]
	inline int           LabelMode = 1;               // 0 = only the hovered item, 1 = always, 2 = while LabelKey is held
	inline std::uint32_t LabelKey = 0x38;             // Left Alt (mode 2)
	inline float         LabelRadius = 1500.0f;       // how far from you items get a label
	inline float         LabelMaxHeightDiff = 300.0f; // skip items this far above/below you (other floors)
	inline int           MaxLabels = 60;
	inline float         LabelScale = 1.0f;
	inline int           LabelMinValue = 0;           // hide items worth less than this (gold is always shown)
	inline bool          ShowFood = false;            // loot filter: food and drink
	inline bool          ShowIngredients = false;     // loot filter: alchemy ingredients
	inline bool          ShowBooks = false;           // loot filter: books and notes (spell tomes always show)
	inline bool          ShowClutter = false;         // loot filter: junk (kettles, buckets, torches...)

	// [Outlines]
	inline bool  Outlines = true;
	inline bool  OutlineEnemies = true;    // red
	inline bool  OutlineNpcs = true;       // white, the player included
	inline bool  OutlineItems = true;      // the label's colour
	inline float OutlineThickness = 1.5f;  // pixels
	inline float OutlineRadius = 4000.0f;  // how far from you actors get an outline

	// [Debug]
	inline bool DebugLog = false;

	void Load();
	bool Save();
}
