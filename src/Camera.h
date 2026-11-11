#pragma once

// Camera lock: pins the third-person camera's pitch (and optionally yaw) for ARPG play,
// keeps the player's room drawn indoors, and applies the [Player]/[GameSettings] tweaks.
namespace Camera
{
	void Install();

	bool IsActive();
	void SetActive(bool a_active);
	void Toggle();                    // with a HUD message
	void AdjustPitch(float a_delta);  // clamps, saves, shows a HUD message
	void StepYaw(float a_degrees);    // turns the locked yaw (LockYaw only), saves, shows a HUD message
	void ForgetHeading();             // the kept compass heading (KeepHeading) starts again from the camera's current one

	bool IsTargetLocked();       // True Directional Movement's target lock is on
	void ApplyCameraSettings();  // distance, offsets, wheel zoom and FOV (what 3PCO used to set); after kDataLoaded and on reload
	void OnFrame();              // main thread, every frame: FOV, and levelling the view on entering first person

	void ApplyGameSettings();  // after kDataLoaded
	void ApplyPlayerValues();  // after a save loads or a new game starts
}
