#pragma once

// All settings live in Data/SKSE/Plugins/ARPGPOV.ini. The in-game pages edit these values
// directly (they apply live) and Save() writes them back.
// Key codes are DirectX scan codes. Mouse buttons: 0 = left, 1 = right, 2 = middle (mouse 3), 3+ = side buttons.
namespace Settings
{
	// ---------------------------------------------------------------- camera and cutaway
	// [Camera] - camera lock (formerly IsoCamLock)
	inline bool          CameraEnabled = true;        // camera lock on at game start
	inline bool          LockYaw = false;             // free mouse rotation by default
	inline bool          LockPitch = true;
	inline float         Yaw = 45.0f;                 // world heading when LockYaw: 0 = north, 90 = east
	inline float         Pitch = 30.0f;               // degrees looking down
	inline int           PitchMode = 0;               // 0 = write player pitch, 1 = write camera freeRotation.y
	inline bool          InvertPitch = false;
	inline bool          PauseOnTargetLock = false;   // let TDM hold the camera while target lock is active
	inline bool          FixInteriorRooms = true;     // draw the player's room even if the camera is outside it
	inline float         PitchStep = 5.0f;
	inline std::uint32_t CameraToggleKey = 0x44;      // F10
	inline std::uint32_t PitchUpKey = 0x49;           // Page Up
	inline std::uint32_t PitchDownKey = 0x51;         // Page Down
	inline std::uint32_t ReloadKey = 0x42;            // F8
	inline bool          CameraDebugLog = false;

	// [Player] - applied on save load / new game. 0 = leave the game's value alone.
	inline float CarryWeight = 0.0f;
	inline float SpeedMult = 0.0f;

	// [GameSettings] - engine settings (setgs names), re-applied every launch.
	inline std::vector<std::pair<std::string, std::string>> GameSettings;

	// [Cutaway]
	inline bool CutawayEnabled = true;
	inline bool InteriorOnly = true;
	inline int  CutawayMode = 1;                             // 1 = horizontal cut (clip plane), 0 = hide whole meshes (legacy)

	// [Cut] - Mode 1
	inline float         CutHeight = 220.0f;          // cut height above the floor at the player
	inline float         CutRadius = 2500.0f;         // selective cut only
	inline bool          CutAllGeometry = true;       // every main-camera mesh uses the cut projection (consistent depth)
	inline float         CutSlope = 1.0f;             // how fast the cut rises behind you (1 = 45 deg, 0 = flat)
	inline float         ReanchorDistance = 150.0f;   // cut height only moves if you stand this far above/below it...
	inline float         ReanchorDelay = 1.0f;        // ...for this many seconds, on the ground
	inline float         HeightSmoothing = 0.6f;      // seconds every height change eases in over (0 = instant)
	inline float         MaxHeightSpeed = 800.0f;     // units/second cap on any height change
	inline bool          AutoLift = false;            // style: false = always cut (Diablo look, default), true = automatic (experimental)
	inline std::uint32_t StyleKey = 0x43;             // F9: switch style in game
	inline float         ClearDelay = 0.75f;          // auto style: seconds of clear view before the cut rises
	inline float         LiftMax = 600.0f;            // auto style: how far it rises; at the top the cut switches off

	// [Cells] - per-interior overrides, keyed by lowercase editor ID
	struct CellOverride
	{
		bool  off{ false };
		float height{ 0.0f };
	};
	inline std::unordered_map<std::string, CellOverride> CellOverrides;
	const CellOverride* FindCellOverride(const char* a_editorID);

	// [Sweep] / [Hide] - Mode 0 (legacy per-mesh hiding)
	inline float HeadHeight = 120.0f;
	inline float CorridorRadius = 400.0f;
	inline float SweepInterval = 0.05f;
	inline float MinCameraDistance = 50.0f;
	inline float MinHeight = 200.0f;
	inline float CeilingHeight = 350.0f;
	inline float BehindMargin = 100.0f;
	inline float MaxShapeRadius = 700.0f;
	inline float Linger = 1.0f;
	inline float Hysteresis = 75.0f;

	// [Background]
	inline bool BlackFog = true;                      // black interior fog while inside
	inline bool HideSky = true;                       // hide the sky dome while inside


	

	// ---------------------------------------------------------------- point and click
	// [PointClick]
	inline bool          PointClickEnabled = true;    // point-and-click on at game start
	inline std::uint32_t ToggleKey = 0x41;            // F7: switch between point-and-click and normal WASD + mouse look

	// [Cursor]
	inline float         Sensitivity = 1.0f;          // cursor speed
	inline float         CursorSize = 1.0f;
	inline bool          CursorRing = true;           // ring behind the sword / bag / exclamation mark, in the symbol's colour
	inline bool          MenuCursor = true;           // draw this cursor in inventory-type menus too, in place of the game's
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
	inline int           StackCount = 5;              // stacks of at least this many read "200x Gold" (smaller stacks show the name only)
	inline bool          ShowFood = false;            // loot filter: food and drink
	inline bool          ShowIngredients = false;     // loot filter: alchemy ingredients
	inline bool          ShowBooks = false;           // loot filter: books and notes (spell tomes always show)
	inline bool          ShowClutter = false;         // loot filter: junk (kettles, buckets, torches...)

	// [Outlines]
	inline bool  Outlines = true;
	inline bool  OutlineEnemies = true;    // red
	inline bool  OutlineNpcs = false;      // NPCs green, you white (off: only when hovered)
	inline bool  OutlineItems = false;     // the label's colour (off: only when hovered)
	inline int   OutlineThickness = 3;        // glow size: 1 = a thin line; higher widens the line and its glow
	inline float OutlineOpacity = 0.6f;       // how strong the glow is (1 = solid)
	inline float OutlineBrightness = 0.7f;    // how bright its colours are (1 = full colour, lower = darker)
	inline bool  OutlineFade = true;          // outlines get fainter with distance
	inline float OutlineMaxAbove = 120.0f;    // items, containers and corpses higher than this above your feet get no outline or label
	inline bool  OutlineBehindPlayer = true;  // things behind you don't glow over you
	inline float OutlineRadius = 1500.0f;     // how far from you actors get an outline
	inline float OutlineItemRadius = 700.0f;  // how far from you items get an outline (their labels reach further)

	// [Debug]
	inline bool CutawayDebugLog = false;
	inline bool PointClickDebugLog = false;

	

	void Load();
	bool Save();  // writes every setting back to the INI (comments are regenerated)
}
