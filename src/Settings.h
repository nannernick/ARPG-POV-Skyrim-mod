#pragma once

// All settings live in Data/SKSE/Plugins/ARPGPOV.ini. The in-game pages edit these values
// directly (they apply live) and Save() writes them back.
// Key codes are DirectX scan codes. Mouse buttons: 0 = left, 1 = right, 2 = middle (mouse 3), 3+ = side buttons.
namespace Settings
{
	// ---------------------------------------------------------------- camera and cutaway
	// [Camera] - camera lock (formerly IsoCamLock)
	inline bool          CameraEnabled = true;        // camera lock on at game start
	inline bool          LockYaw = true;              // fixed compass heading by default (YawStepKey turns it); off = free mouse rotation
	inline bool          LockPitch = true;
	inline float         Yaw = 45.0f;                 // world heading when LockYaw: 0 = north, 90 = east
	inline float         Pitch = 30.0f;               // degrees looking down (30 = the Path of Exile 2 default)
	inline int           PitchMode = 0;               // 0 = write player pitch, 1 = write camera freeRotation.y
	inline bool          InvertPitch = false;
	inline bool          PauseOnTargetLock = false;   // let TDM hold the camera while target lock is active
	inline bool          FixInteriorRooms = true;     // draw the player's room even if the camera is outside it
	inline bool          KeepHeading = true;          // keep the camera's compass heading when True Directional Movement turns it (target lock, camera reset, auto-adjust)
	// What 3PCO used to do: a fixed camera distance, no shoulder offsets, a set field of view.
	inline bool          ManageCamera = true;         // apply the five values below (off = leave the camera to the game or another mod)
	inline float         CameraDistance = 1290.0f;    // how far the camera sits from you
	inline float         CameraFov = 50.0f;           // field of view, degrees
	inline float         FirstPersonFov = 90.0f;      // first person: set once on entering, then left alone (0 = held at CameraFov, as before 2.1)
	inline float         CameraOffsetX = 0.0f;        // sideways shoulder offset (0 = centred)
	inline float         CameraOffsetZ = 0.0f;        // height offset
	inline bool          WheelZoom = false;           // mouse wheel zooms (and can scroll into first person)

	// Camera presets, named after the game they imitate. A preset sets pitch, field of view and
	// distance (offsets 0, pitch locked). Only some of the numbers were measured from screenshots.
	struct CameraPresetDef
	{
		const char* name;
		float       pitch;
		float       fov;
		float       distance;
		const char* note;
	};
	inline constexpr CameraPresetDef CameraPresets[] = {
		{ "Default", 30.0f, 50.0f, 1290.0f, "This mod's default: the same look as Path of Exile 2." },
		{ "Skyrim ARPG", 30.0f, 90.0f, 600.0f, "The default before 1.6: wide lens, close camera." },
		{ "Diablo II", 30.0f, 50.0f, 1290.0f, "Pitch measured (2:1 floor tiles). Lens and distance estimated." },
		{ "Path of Exile", 50.0f, 50.0f, 1400.0f, "Pitch measured (ring 210 x 160 px at 1080p). Lens and distance estimated." },
		{ "Path of Exile 2", 30.0f, 50.0f, 1290.0f, "Pitch measured (ring 700 x 350 px). Lens and distance estimated." },
		{ "Diablo III", 50.0f, 50.0f, 1200.0f, "Estimated." },
		{ "Diablo IV", 55.0f, 55.0f, 1000.0f, "Estimated: steeper and closer than older ARPGs." },
		{ "Last Epoch", 45.0f, 50.0f, 1300.0f, "Estimated." },
		{ "Grim Dawn", 45.0f, 50.0f, 1350.0f, "Estimated." },
		{ "Torchlight II", 45.0f, 55.0f, 1150.0f, "Estimated." },
	};
	inline std::string CameraPreset = "Default";  // a name from the table, or Custom = the values in the INI
	bool               ApplyCameraPreset(std::string_view a_name);  // false = no such preset
	// The menu runs on the render thread and the hotkeys on the main thread. Numbers and switches
	// can be written from either; the preset's name is a string, so it goes through these two.
	std::string        PresetName();
	void               SetPresetName(std::string_view a_name);
	inline float         PitchStep = 5.0f;
	inline std::uint32_t CameraToggleKey = 0x44;      // F10
	inline std::uint32_t PitchUpKey = 0xC9;           // Page Up (0x49 is Numpad 9)
	inline std::uint32_t PitchDownKey = 0xD1;         // Page Down (0x51 is Numpad 3)
	inline std::uint32_t YawStepKey = 0x3C;           // F2: turn the camera 90 degrees (only with LockYaw). Insert (0xD2) before 1.6.1
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
	inline float         CutSlope = 0.3f;             // Automatic style: how fast the cut rises away from the camera (never below Height on screen)
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
	inline bool          ClickMarker = true;          // a small ring on the ground where a click is sending you

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
	inline int           OnAttack = 0;                // when an attack starts: 0 = stop the click-move, 1 = keep moving (aim follows the target)

	// [Pickup]
	inline std::uint32_t PickupButton = 2;            // middle mouse button (mouse 3)
	inline float         PickupRange = 150.0f;        // closer than this picks up at once; further walks there first
	inline bool          Containers = true;           // containers can be clicked
	inline bool          Actors = true;               // corpses and NPCs can be clicked
	inline bool          Activators = true;           // named furniture and activators (seats, levers) can be clicked; they get no label

	// [Labels]
	inline int           LabelMode = 1;               // 0 = only the hovered item, 1 = always, 2 = while LabelKey is held
	inline std::uint32_t LabelKey = 0x38;             // Left Alt (mode 2)
	inline float         LabelRadius = 1500.0f;       // how far from you items get a label
	inline float         LabelMaxHeightDiff = 300.0f; // skip items this far above/below you (other floors)
	inline float         LabelMaxBelow = 60.0f;       // items lower than this under your feet get no label (floors below, hidden rooms)
	inline int           MaxLabels = 60;
	inline float         LabelScale = 1.0f;
	inline int           LabelMinValue = 0;           // hide items worth less than this (gold is always shown)
	inline int           StackCount = 5;              // stacks of at least this many read "200x Gold" (smaller stacks show the name only)
	inline bool          ShowFood = false;            // loot filter: food and drink
	inline bool          ShowIngredients = false;     // loot filter: alchemy ingredients
	inline bool          ShowBooks = false;           // loot filter: books and notes (spell tomes always show)
	inline bool          ShowClutter = false;         // loot filter: junk (kettles, buckets, torches...)
	// POErim (DiabloLootAPI v2, POErim 2.1.0+). Without it nothing here changes anything.
	inline bool          UseRarityColor = true;       // labels and outlines in POErim's rarity colour; orbs always show as loot
	inline std::uint32_t CurrencyColor = 0;           // 0xRRGGBB for orbs; 0 = POErim's own currency colour

	// [Combat]
	inline bool          HoldToAttack = true;      // holding the attack button swings again and again (melee)
	inline std::uint32_t PowerAttackKey = 0x38;    // held with the attack button: a power attack, as in the base game (0x38 = Left Alt)
	inline bool          AutoAttack = false;       // while locked on (TDM), one press starts attacking by itself; press again to stop
	inline bool          AimProjectiles = true;    // steer your spells and arrows at the aim point the moment they leave your hands
	inline bool          AimTracker = true;        // a dashed line from you to the aim point while casting or drawing
	inline bool          AimMarker = true;         // a red ring where the shot is going, fading out after release
	inline bool          QuestActivators = true;   // levers, pull chains and quest activators get the quest colour and an outline
	inline bool          HideGlobesInMenus = true; // the life/mana globes hide while the inventory, a container or a shop is open
	inline bool          AimAtCursor = true;       // spells and bows aim at the cursor (or the hovered enemy), not along the tilted camera

	// [Globes]
	inline bool  Globes = true;            // Path of Exile style life and mana globes, stamina bar on the life globe
	inline bool  HideVanillaBars = true;   // hide the game's health, magicka and stamina bars while the globes are on
	inline float GlobeSize = 1.0f;
	inline bool  GlobeNumbers = true;      // current/max above each globe
	inline bool  GlobeAnimate = true;      // moving liquid: a wave on the surface and rising bubbles

	// [Outlines]
	inline bool  Outlines = true;
	inline bool  OutlineEnemies = true;    // red
	inline bool  OutlineNpcs = false;      // NPCs green (off: only when hovered)
	inline bool  OutlineFollowers = true;  // followers / teammates, in FollowerColor (off: only when hovered)
	inline std::uint32_t OutlineFollowerColor = 0x50A0FF;  // 0xRRGGBB
	inline bool  OutlinePlayer = true;     // you, white
	inline bool  OutlineLoot = false;      // dropped items (their label's colour), containers and corpses (gold). Off: only when hovered
	inline bool  OutlineQuestItems = true; // quest items lying in the world: always labelled and outlined (within OutlineItemRadius), Loot switch or not
	inline std::uint32_t OutlineQuestItemColor = 0x39FF14;  // 0xRRGGBB, bright green (the NPC green is darker)
	inline int   OutlineThickness = 3;        // glow size: 1 = a thin line; higher widens the line and its glow
	inline float OutlineOpacity = 0.6f;       // how strong the glow is (1 = solid)
	inline float OutlineBrightness = 0.7f;    // how bright its colours are (1 = full colour, lower = darker)
	inline bool  OutlineFade = true;          // outlines get fainter with distance
	inline float OutlineMaxAbove = 120.0f;    // items, containers and corpses higher than this above your feet get no outline or label
	inline float OutlineMaxBelow = 150.0f;    // enemies, NPCs and followers lower than this under your feet get no outline or circle unless hovered
	inline bool  GroundCircles = false;       // a circle on the ground under enemies, NPCs and followers (drawn with the labels, no shader)
	inline float CircleRadius = 45.0f;        // Skyrim units
	inline bool  OutlineBehindPlayer = true;  // things behind you don't glow over you
	inline float OutlineRadius = 1500.0f;     // how far from you actors get an outline
	inline float OutlineItemRadius = 700.0f;  // how far from you items get an outline (their labels reach further)
	inline int   OutlineMaxParts = 256;       // most model parts outlined in one frame (an armoured character is many parts); past it the furthest are left out
	inline bool  OutlinePartial = false;      // also outline characters whose model could only be read in part (off: all or nothing)

	// [Debug]
	inline bool CutawayDebugLog = false;
	inline bool PointClickDebugLog = false;
	inline bool InteriorDebugLog = false;  // interior outline, fog and darkness diagnostics

	

	void Load();
	bool Save();  // writes every setting back to the INI (comments are regenerated)
}
