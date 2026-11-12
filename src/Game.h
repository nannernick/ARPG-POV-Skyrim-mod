#pragma once

// Everything that touches the game world: click-to-move, facing, using things, and the list
// of nearby things that get an outline, a label or a click. Runs on the main thread.
namespace Game
{
	enum class Kind : std::uint8_t
	{
		kItem,       // loose item that passed the loot filter: label + outline
		kContainer,
		kCorpse,
		kNpc,        // living, not hostile
		kEnemy,
		kDoor,
		kPlayer,     // outline only
		kFollower,   // living teammate: its own outline colour
		kActivator   // named furniture or activator (a seat, a lever): click only, no label
	};

	// Enemies, NPCs, followers and the player: they move, so their outline points are refreshed every frame.
	constexpr bool IsLiving(Kind a_kind)
	{
		return a_kind == Kind::kEnemy || a_kind == Kind::kNpc || a_kind == Kind::kFollower || a_kind == Kind::kPlayer;
	}

	struct Target
	{
		RE::ObjectRefHandle       handle;
		RE::FormID                formID{ 0 };
		std::string               text;                  // items only: the label
		RE::NiPoint3              pos;                   // label anchor
		std::vector<RE::NiPoint3> points;                // outermost points; on screen their outline is the outline drawn and the click area
		float                     distance{ 0.0f };      // from the player, at the last scan
		float                     above{ 0.0f };         // how far above the player's feet it sits, at the last scan
		float                     pad{ 3.0f };           // how far outside those points the outline sits (Skyrim units)
		std::uint32_t             color{ 0xFFFFFFFF };   // 0xAABBGGRR
		Kind                      kind{ Kind::kItem };
		bool                      quest{ false };        // items only: a quest object (its own colour, always marked)
	};

	// World point -> screen position in 0..1 (x right, y down). False if behind the camera.
	// Only reads the camera, so the HUD may call it too.
	bool WorldToScreen(const RE::NiPoint3& a_point, float& a_x, float& a_y);

	// Whether a target is marked all the time (an outline, a ground circle) and not only while the
	// cursor is on it: its kind's switch, its distance, and how far above or below the player it is.
	// The one rule the outline renderer, the ground circles and the fallback shapes share. Reads settings only.
	bool WantsMarker(const Target& a_target);

	void Install();           // per-frame hook
	void RegisterMenuSink();  // after kDataLoaded
	void RefreshGameplay();   // recompute State::gameplay (main thread)
	void Reset();             // a save is loading: drop every target

	bool OnPrimaryButton(RE::ButtonEvent* a_event);  // move/attack button; true = don't let the game attack
	bool OnAttackEvent(RE::ButtonEvent* a_event);    // every attack/block button event; true = the game must not see it
	void OnPickupButton();                           // pickup button pressed
	bool BlockLook();                                // mouse should not turn the camera right now
	void Toggle();                                   // point-and-click on/off, with a HUD message
}

// Shared between the main thread (Game, Input) and the HUD draw callback.
namespace State
{
	inline std::atomic<bool>  gameplay{ false };  // cursor is live: in game, third person, no menu open
	inline std::atomic<bool>  menuCursor{ false };  // a menu with the game's cursor is open and ours is drawn in its place
	inline std::atomic<float> cursorX{ -1.0f };   // pixels; -1 = not placed yet (HUD centres it)
	inline std::atomic<float> cursorY{ -1.0f };
	inline std::atomic<float> screenW{ 1920.0f };
	inline std::atomic<float> screenH{ 1080.0f };

	inline std::atomic<bool> rotateHeld{ false };
	inline std::atomic<bool> attackInPlaceHeld{ false };
	inline std::atomic<bool> powerAttackHeld{ false };   // PowerAttackKey is down: holding attack is a power attack, as in the base game
	inline std::atomic<bool>  aiming{ false };           // casting or drawing a bow: the player's aim follows aimPitch, not the camera tilt
	inline std::atomic<float> aimPitch{ 0.0f };          // radians, positive = down
	// The aim point while casting or drawing, for the HUD's aim line and red ring.
	inline std::atomic<bool>  aimShown{ false };
	inline std::atomic<float> aimFromX{ 0.0f }, aimFromY{ 0.0f }, aimFromZ{ 0.0f };
	inline std::atomic<float> aimToX{ 0.0f }, aimToY{ 0.0f }, aimToZ{ 0.0f };      // where the shot goes
	inline std::atomic<float> aimGroundX{ 0.0f }, aimGroundY{ 0.0f }, aimGroundZ{ 0.0f };  // under it, for the ring
	inline std::atomic<bool>  inventoryOpen{ false };    // inventory, container, shop, gift or magic menu (Grid Inventory too)
	inline std::atomic<bool> labelKeyHeld{ false };
	inline std::atomic<int>  moveKeysHeld{ 0 };  // WASD keys currently down

	// Where a click is sending the player, for the HUD's destination ring. Written by Game, read by the HUD.
	// The three coordinates are stored one by one, so the HUD may see a half-updated point for a frame.
	inline std::atomic<bool>  markerOn{ false };
	inline std::atomic<float> markerX{ 0.0f };
	inline std::atomic<float> markerY{ 0.0f };
	inline std::atomic<float> markerZ{ 0.0f };

	inline std::mutex                                      lock;     // guards the three below
	inline std::shared_ptr<const std::vector<Game::Target>> targets;  // written by Game every frame, read by the HUD
	inline RE::ObjectRefHandle                             hovered;  // written by the HUD, read by Game
	inline Game::Kind                                      hoveredKind{ Game::Kind::kItem };
}
