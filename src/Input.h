#pragma once

// Mouse and key handling: moves the cursor, keeps the mouse from turning the camera,
// routes the move and pickup buttons, and handles every hotkey (point and click on/off,
// camera lock on/off, cutaway style, pitch up/down, reload).
namespace Input
{
	void Install();   // input-handler hooks
	void Register();  // after kInputLoaded
	void Reload();    // re-read the INI and re-apply everything (main thread)

	// Forget every key we think is held (WASD, rotate, attack in place, power attack, labels). A key let go
	// during a load screen never reaches us, and a "held" one left behind blocks click-to-move (WASD) or
	// freezes the cursor (rotate) for good. Called at loads, load doors and when a menu with input closes.
	void ResetHeld(const char* a_why);
}
