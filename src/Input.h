#pragma once

// Mouse and key handling: moves the cursor, keeps the mouse from turning the camera,
// routes the move and pickup buttons, and handles every hotkey (point and click on/off,
// camera lock on/off, cutaway style, pitch up/down, reload).
namespace Input
{
	void Install();   // input-handler hooks
	void Register();  // after kInputLoaded
	void Reload();    // re-read the INI and re-apply everything (main thread)
}
