#pragma once

// Mouse and key handling: moves the cursor, keeps the mouse from turning the camera,
// and routes the move and pickup buttons.
namespace Input
{
	void Install();   // input-handler hooks
	void Register();  // after kInputLoaded
}
