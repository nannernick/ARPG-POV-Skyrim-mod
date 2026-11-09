#pragma once

// Everything that touches the game world: click-to-move, facing, pickup, and the list of
// nearby things that can get a label. Runs on the main thread.
namespace Game
{
	enum class Kind : std::uint8_t
	{
		kItem,       // loose item: always labelled (per Labels.Mode)
		kContainer,  // labelled only while hovered
		kCorpse,
		kNpc
	};

	struct Target
	{
		RE::ObjectRefHandle handle;
		std::string         text;
		RE::NiPoint3        pos;    // where the label is anchored
		std::uint32_t       color;  // 0xAABBGGRR
		Kind                kind;
	};

	// World point -> screen position in 0..1 (x right, y down). False if behind the camera.
	// Only reads the camera, so the HUD may call it too.
	bool WorldToScreen(const RE::NiPoint3& a_point, float& a_x, float& a_y);

	void Install();           // per-frame hook
	void RegisterMenuSink();  // after kDataLoaded
	void RefreshGameplay();   // recompute State::gameplay (main thread)
	void Reset();             // a save is loading: drop every target

	bool OnPrimaryButton(RE::ButtonEvent* a_event);  // move/attack button; true = don't let the game attack
	void OnPickupButton();                           // pickup button pressed
	bool BlockLook();                                // mouse should not turn the camera right now
	void Toggle();                                   // point-and-click on/off, with a HUD message
}

// Shared between the main thread (Game, Input) and the HUD draw callback.
namespace State
{
	inline std::atomic<bool>  gameplay{ false };  // cursor is live: in game, third person, no menu open
	inline std::atomic<float> cursorX{ -1.0f };   // pixels; -1 = not placed yet (HUD centres it)
	inline std::atomic<float> cursorY{ -1.0f };
	inline std::atomic<float> screenW{ 1920.0f };
	inline std::atomic<float> screenH{ 1080.0f };

	inline std::atomic<bool> rotateHeld{ false };
	inline std::atomic<bool> attackInPlaceHeld{ false };
	inline std::atomic<bool> labelKeyHeld{ false };
	inline std::atomic<int>  moveKeysHeld{ 0 };  // WASD keys currently down

	inline std::mutex                lock;     // guards the two below
	inline std::vector<Game::Target> targets;  // written by Game, read by the HUD
	inline RE::ObjectRefHandle       hovered;  // written by the HUD, read by Game
}
