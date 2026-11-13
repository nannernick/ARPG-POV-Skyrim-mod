#include "Menu.h"
#include "Camera.h"
#include "Input.h"
#include "LootApi.h"
#include "Occlusion.h"
#include "Settings.h"

#include <filesystem>
#include "SKSEMenuFramework.h"

// Widgets write straight into Settings, so changes apply live. The render callbacks run
// on the render thread, so anything that touches game objects is queued to the main thread.
namespace Menu
{
	namespace
	{
		using namespace ImGuiMCP;

		std::string g_status;

		void RunOnMainThread(std::function<void()> a_task)
		{
			if (auto* tasks = SKSE::GetTaskInterface()) {
				tasks->AddTask(std::move(a_task));
			}
		}

		void Hint(const char* a_text)
		{
			if (IsItemHovered()) {
				SetTooltip("%s", a_text);
			}
		}

		void Footer()
		{
			Spacing();
			Separator();
			if (Button("Save to INI")) {
				g_status = Settings::Save() ? "Saved." : "Couldn't write the INI - see the log.";
			}
			Hint("Writes every setting to Data/SKSE/Plugins/ARPGPOV.ini.");
			SameLine();
			if (Button("Reload INI")) {
				RunOnMainThread([] { Input::Reload(); });
				g_status = "Reloaded from the INI.";
			}
			Hint("Discards unsaved changes and re-reads the INI.");
			if (!g_status.empty()) {
				SameLine();
				TextDisabled("%s", g_status.c_str());
			}
		}

		void ApplyCamera()
		{
			RunOnMainThread([] { Camera::ApplyCameraSettings(); });
		}

		void __stdcall RenderCamera()
		{
			SeparatorText("Preset");
			int               current = 0;  // 0 = Custom
			const std::string preset = Settings::PresetName();
			std::vector<const char*> names{ "Custom" };
			for (int i = 0; i < static_cast<int>(std::size(Settings::CameraPresets)); ++i) {
				names.push_back(Settings::CameraPresets[i].name);
				if (preset == Settings::CameraPresets[i].name) {
					current = i + 1;
				}
			}
			if (Combo("Look", &current, names.data(), static_cast<int>(names.size()))) {
				if (current == 0) {
					Settings::SetPresetName("Custom");
				} else {
					Settings::ApplyCameraPreset(names[current]);
					ApplyCamera();
				}
			}
			Hint("Sets pitch, field of view and distance to imitate a game. Changing any of them yourself switches this to Custom.");
			if (current > 0) {
				TextDisabled("%s", Settings::CameraPresets[current - 1].note);
			}

			SeparatorText("Camera lock");
			bool active = Camera::IsActive();
			if (Checkbox("Camera lock on", &active)) {
				Camera::SetActive(active);
				Settings::CameraEnabled = active;
			}
			Hint("Also toggled with the camera key (F10 by default).");

			bool custom = false;  // a preset value was edited by hand
			custom |= Checkbox("Lock pitch", &Settings::LockPitch);
			custom |= SliderFloat("Pitch (degrees down)", &Settings::Pitch, 5.0f, 85.0f, "%.0f");
			Hint("30 = Path of Exile 2 / Diablo II, 50 = Path of Exile.");
			Checkbox("Lock yaw", &Settings::LockYaw);
			BeginDisabled(!Settings::LockYaw);
			SliderFloat("Yaw (compass degrees)", &Settings::Yaw, 0.0f, 360.0f, "%.0f");
			EndDisabled();
			SliderFloat("Pitch key step", &Settings::PitchStep, 1.0f, 15.0f, "%.0f");
			TextDisabled("The yaw key (F2 by default) turns a locked yaw 90 degrees per press.");

			SeparatorText("Distance and view");
			bool managed = false;
			managed |= Checkbox("Manage the camera", &Settings::ManageCamera);
			Hint("Sets the distance, offsets, zoom and field of view below (what 3PCO used to do). Turn off to leave them to the game or another camera mod.");
			BeginDisabled(!Settings::ManageCamera);
			bool view = false;
			view |= SliderFloat("Distance", &Settings::CameraDistance, 100.0f, 2000.0f, "%.0f");
			view |= SliderFloat("Field of view", &Settings::CameraFov, 30.0f, 120.0f, "%.0f");
			Hint("A lower field of view from further away looks flatter, like most ARPGs.");
			SliderFloat("First person field of view", &Settings::FirstPersonFov, 0.0f, 150.0f, "%.0f");
			Hint("Set once when you switch to first person, then left alone (the console's fov or another mod can change it).\n"
				 "Below 30 = first person keeps the field of view above, as before 2.1.");
			view |= SliderFloat("Sideways offset", &Settings::CameraOffsetX, -300.0f, 300.0f, "%.0f");
			view |= SliderFloat("Height offset", &Settings::CameraOffsetZ, -300.0f, 300.0f, "%.0f");
			managed |= Checkbox("Mouse wheel zooms", &Settings::WheelZoom);
			Hint("Off: the wheel does nothing and the distance stays fixed. Use the change-view key for first person.");
			EndDisabled();
			if (custom || view) {
				Settings::SetPresetName("Custom");
			}
			if (managed || view) {
				ApplyCamera();
			}

			SeparatorText("Compatibility");
			Checkbox("Fix interior rooms", &Settings::FixInteriorRooms);
			Hint("Indoors, draw the room you're in even when the camera is above the ceiling.");
			Checkbox("Pause on TDM target lock", &Settings::PauseOnTargetLock);
			Hint("Hands the whole camera (pitch and heading) to True Directional Movement while its target lock is on.");
			Checkbox("Keep the camera heading (override TDM)", &Settings::KeepHeading);
			Hint("True Directional Movement turns the camera toward a locked target, and swings it behind your character\n"
				 "when you press lock-on with nothing to lock or keep moving. On: the heading stays where you left it\n"
				 "unless you turn the camera with the mouse. Does nothing with Lock yaw (that already fixes the heading).");
			Checkbox("Invert pitch", &Settings::InvertPitch);
			Checkbox("Camera debug log", &Settings::CameraDebugLog);

			Footer();
		}

		void __stdcall RenderCutaway()
		{
			SeparatorText("Cutaway");
			Checkbox("Enabled", &Settings::CutawayEnabled);
			Checkbox("Interiors only", &Settings::InteriorOnly);

			const char* const styles[] = { "Default", "Automatic (experimental)" };
			int               style = Occlusion::GetAutoLift() ? 1 : 0;
			if (Combo("Style", &style, styles, 2)) {
				Occlusion::SetAutoLift(style == 1);
				Settings::AutoLift = style == 1;
			}
			Hint("Default: everything above Height is always cut, flat.\n"
				 "Automatic: never cut lower than that, sloping up away from the camera, and rising further when nothing blocks the view of you.\n"
				 "Also switched with the style key (F9 by default).");

			SeparatorText("Cut shape");
			SliderFloat("Height", &Settings::CutHeight, 50.0f, 800.0f, "%.0f");
			Hint("How far above the floor walls are cut (a human is ~128).");
			BeginDisabled(!Occlusion::GetAutoLift());
			SliderFloat("Slope away from the camera", &Settings::CutSlope, 0.0f, 2.0f, "%.2f");
			EndDisabled();
			Hint("Automatic style only. The cut is at Height at the bottom of the screen and rises this fast further away, so far walls stay.\n"
				 "Nothing on screen is cut lower than in the Default style. Higher values leave taller walls near you. 0 = flat.");

			SeparatorText("Height changes");
			SliderFloat("Smoothing (s)", &Settings::HeightSmoothing, 0.0f, 3.0f, "%.2f");
			SliderFloat("Max speed", &Settings::MaxHeightSpeed, 50.0f, 3000.0f, "%.0f");
			SliderFloat("New floor distance", &Settings::ReanchorDistance, 50.0f, 1000.0f, "%.0f");
			Hint("How far above/below the current height you must stand before the cut moves.");
			SliderFloat("New floor delay (s)", &Settings::ReanchorDelay, 0.0f, 5.0f, "%.1f");

			SeparatorText("Automatic style (experimental)");
			BeginDisabled(!Occlusion::GetAutoLift());
			SliderFloat("Clear delay (s)", &Settings::ClearDelay, 0.0f, 5.0f, "%.2f");
			SliderFloat("Max rise", &Settings::LiftMax, 100.0f, 2000.0f, "%.0f");
			Hint("How far above Height the cut may rise in rooms with nothing in the way.");
			EndDisabled();

			SeparatorText("Background");
			Checkbox("Black interior fog", &Settings::BlackFog);
			Hint("Turning it off gives the room its own fog colours back at once. Turning it on applies fully the next time you enter a cell.");
			Checkbox("Hide sky indoors", &Settings::HideSky);

			SeparatorText("Advanced");
			Checkbox("All geometry (consistent depth)", &Settings::CutAllGeometry);
			Hint("Leave on. Off only cuts meshes that cross the plane, which hides props in many rooms.");
			Checkbox("Cutaway debug log", &Settings::CutawayDebugLog);
			Checkbox("Interior debug log", &Settings::InteriorDebugLog);
			Hint("Writes your outline's state indoors and at doors, and the interior fog, to ARPGPOV.log. For dark rooms and missing outlines.");

			Footer();
		}

		void __stdcall RenderControls()
		{
			SeparatorText("Point and click");
			Checkbox("Enabled", &Settings::PointClickEnabled);
			Hint("Also toggled with the toggle key (F7 by default). Off = normal WASD and mouse look.");
			Checkbox("Mouse moves the cursor, not the camera", &Settings::BlockMouseLook);

			SeparatorText("Cursor");
			SliderFloat("Cursor speed", &Settings::Sensitivity, 0.1f, 5.0f, "%.2f");
			SliderFloat("Cursor size", &Settings::CursorSize, 0.5f, 4.0f, "%.1f");
			Checkbox("Ring behind the sword, bag and exclamation mark", &Settings::CursorRing);
			Checkbox("Use this cursor in menus too", &Settings::MenuCursor);
			Hint("Inventory, containers, barter and other menus (Grid Inventory included) show this cursor instead of the game's.");

			SeparatorText("Movement");
			Checkbox("Hold to keep moving", &Settings::HoldToMove);
			Checkbox("Ring where you clicked", &Settings::ClickMarker);
			Hint("A small ring on the ground where a click is sending you. It fades when you arrive, click again or take over with WASD.");
			SliderFloat("Stop distance", &Settings::ArriveRadius, 5.0f, 200.0f, "%.0f");
			SliderFloat("Max click distance", &Settings::MaxDistance, 500.0f, 10000.0f, "%.0f");
			const char* const modes[] = { "Auto", "True Directional Movement", "Turn the player (no TDM)" };
			Combo("Movement mode", &Settings::MoveMode, modes, 3);
			const char* const onAttack[] = { "Stop moving", "Keep moving" };
			Combo("When an attack starts", &Settings::OnAttack, onAttack, 2);
			Hint("Stop moving: any attack (a swing, drawing a bow, casting) cancels the click-move you are on.\n"
				 "Keep moving: you keep walking where you clicked; spells and arrows still go to the cursor.\n"
				 "With True Directional Movement your body faces the way you walk while you do.");

			SeparatorText("Clicking things");
			Checkbox("Clicking an enemy attacks it", &Settings::ClickEnemies);
			Hint("Off: every click moves. Shift + click always attacks toward the cursor.");
			Checkbox("Clicking things uses them", &Settings::ClickInteract);
			Hint("Click an item to pick it up, a door to go through, an NPC to talk, a chest or corpse to loot.\nOff: only the pickup button (mouse 3) does.");
			SliderFloat("Reach", &Settings::PickupRange, 50.0f, 500.0f, "%.0f");
			Hint("Closer than this uses the thing at once. Further away, you walk there first.");
			Checkbox("Containers can be clicked", &Settings::Containers);
			Checkbox("Corpses and NPCs can be clicked", &Settings::Actors);
			Checkbox("Furniture and activators can be clicked", &Settings::Activators);
			Hint("Seats, levers, the ferry seat: anything of that sort that has a name. They get no label and only glow under the cursor.");

			SeparatorText("Combat");
			Checkbox("Hold attack to keep attacking", &Settings::HoldToAttack);
			Hint("Holding your attack button swings again and again (melee). Hold the power attack key (Left Alt by default) with it for a power attack.");
			Checkbox("Auto-attack while locked on", &Settings::AutoAttack);
			Hint("While True Directional Movement's target lock is on, one press of attack keeps attacking until you press it again. Melee only.");
			Checkbox("Spells and bows aim at the cursor", &Settings::AimAtCursor);
			Hint("Off: they fire along the camera's tilt, which is into the ground.");
			Checkbox("Steer spells and arrows to the aim point", &Settings::AimProjectiles);
			Hint("Turns each of your projectiles toward the cursor (or the enemy under it) as it is launched.");
			Checkbox("Aim line while casting or drawing", &Settings::AimTracker);
			Checkbox("Red ring where the shot goes", &Settings::AimMarker);
			Checkbox("Levers and pull chains in the quest colour", &Settings::QuestActivators);
			Checkbox("Hide the globes in the inventory and shops", &Settings::HideGlobesInMenus);

			SeparatorText("Debug");
			Checkbox("Click debug log", &Settings::ClickRayDebugLog);
			Hint("Writes where each click lands to ARPGPOV.log: the cursor ray, every hit along it and the point chosen.");
			Checkbox("Load and menu debug log", &Settings::LoadStateDebugLog);
			Hint("Writes what is cleared at loads, load doors and menus, and the input state at that moment, to ARPGPOV.log.");

			TextDisabled("Buttons and keys are set in the INI.");
			Footer();
		}

		void __stdcall RenderOutlines()
		{
			SeparatorText("What gets an outline");
			Checkbox("Outlines", &Settings::Outlines);
			Hint("Off: only the thing under the cursor is outlined.");
			Checkbox("Enemies (red)", &Settings::OutlineEnemies);
			SameLine();
			Checkbox("NPCs (green)", &Settings::OutlineNpcs);
			Checkbox("Followers", &Settings::OutlineFollowers);
			Hint("Your followers and anyone else on your side as a teammate. Other friendly NPCs use the NPCs tick.");
			SameLine();
			float follower[3] = { static_cast<float>((Settings::OutlineFollowerColor >> 16) & 0xFF) / 255.0f,
				static_cast<float>((Settings::OutlineFollowerColor >> 8) & 0xFF) / 255.0f, static_cast<float>(Settings::OutlineFollowerColor & 0xFF) / 255.0f };
			if (ColorEdit3("Follower colour", follower, 0)) {
				const auto channel = [](float a_v) { return static_cast<std::uint32_t>(std::lround(std::clamp(a_v, 0.0f, 1.0f) * 255.0f)); };
				Settings::OutlineFollowerColor = (channel(follower[0]) << 16) | (channel(follower[1]) << 8) | channel(follower[2]);
			}
			Checkbox("Your character (white)", &Settings::OutlinePlayer);
			SameLine();
			Checkbox("Loot", &Settings::OutlineLoot);
			Hint("Dropped items (in their label's colour), containers and corpses (gold). Off: they only glow under the cursor.\n"
				 "A weapon or shield an NPC is holding is part of that NPC and can't be outlined by itself.");
			Checkbox("Quest items", &Settings::OutlineQuestItems);
			Hint("Quest items lying in the world always get a label and an outline in their own colour, with Loot off too and\n"
				 "whatever the loot filter says. The item distance and the height limits still apply.");
			SameLine();
			float quest[3] = { static_cast<float>((Settings::OutlineQuestItemColor >> 16) & 0xFF) / 255.0f,
				static_cast<float>((Settings::OutlineQuestItemColor >> 8) & 0xFF) / 255.0f, static_cast<float>(Settings::OutlineQuestItemColor & 0xFF) / 255.0f };
			if (ColorEdit3("Quest item colour", quest, 0)) {
				const auto channel = [](float a_v) { return static_cast<std::uint32_t>(std::lround(std::clamp(a_v, 0.0f, 1.0f) * 255.0f)); };
				Settings::OutlineQuestItemColor = (channel(quest[0]) << 16) | (channel(quest[1]) << 8) | channel(quest[2]);
			}

			SeparatorText("Ground circles");
			Checkbox("Circles under enemies, NPCs and followers", &Settings::GroundCircles);
			Hint("A ring on the ground in the outline's colour. Uses the Enemies, NPCs and Followers ticks above and works with\n"
				 "Outlines off, so it can replace the outlines or go with them. It is drawn over the picture, so it shows through walls.");
			BeginDisabled(!Settings::GroundCircles);
			SliderFloat("Circle size", &Settings::CircleRadius, 10.0f, 150.0f, "%.0f");
			EndDisabled();

			SeparatorText("Glow");
			SliderInt("Glow size", &Settings::OutlineThickness, 1, 8);
			Hint("1 = a thin line. Higher widens the line and the glow around it.");
			SliderFloat("Glow strength", &Settings::OutlineOpacity, 0.05f, 1.0f, "%.2f");
			SliderFloat("Colour brightness", &Settings::OutlineBrightness, 0.1f, 1.0f, "%.2f");
			Hint("Lower = darker, subtler colours. The thing under the cursor always glows at full strength.");
			Checkbox("Fainter with distance", &Settings::OutlineFade);
			Checkbox("Things behind you don't glow over you", &Settings::OutlineBehindPlayer);

			SeparatorText("Distances");
			SliderFloat("Actor outline distance", &Settings::OutlineRadius, 500.0f, 10000.0f, "%.0f");
			SliderFloat("Item outline distance", &Settings::OutlineItemRadius, 100.0f, 5000.0f, "%.0f");
			Hint("Items, containers and corpses. Items further than this still get a label, just no outline. Lower both distances if frames drop.");
			SliderInt("Most model parts per frame", &Settings::OutlineMaxParts, 64, 1024);
			Hint("An armoured character is many parts, so a crowd can reach this; past it the furthest characters get no outline.\n"
				 "Higher outlines more of a crowd and costs frames.");
			Checkbox("Outline half-read characters", &Settings::OutlinePartial);
			Hint("Off: a character gets its whole outline or none. If part of its model can't be read (still loading, or a mesh the\n"
				 "outline renderer can't handle) it is left out, except under the cursor. On: whatever was read is outlined, even if\n"
				 "that is only a helmet or a pair of boots.");
			SliderFloat("No actors lower than", &Settings::OutlineMaxBelow, 0.0f, 1000.0f, "%.0f");
			Hint("Enemies, NPCs and followers this far below your feet get no outline or circle unless the cursor is on them\n"
				 "(things under the floor, the storey below). Raise it if enemies downhill lose their outline.");

			Footer();
		}

		void __stdcall RenderLabels()
		{
			SeparatorText("Item labels");
			const char* const labelModes[] = { "Only under the cursor", "Always", "While the label key is held" };
			Combo("Show", &Settings::LabelMode, labelModes, 3);
			SliderFloat("Label distance", &Settings::LabelRadius, 200.0f, 5000.0f, "%.0f");
			SliderFloat("Text size", &Settings::LabelScale, 0.5f, 3.0f, "%.2f");
			SliderInt("Most labels", &Settings::MaxLabels, 5, 200);
			SliderInt("Show the count from", &Settings::StackCount, 2, 100);
			Hint("Stacks at least this big read \"200x Gold\". Smaller stacks show the name only. 2 = always show the count.");

			SeparatorText("Other floors");
			SliderFloat("Nothing higher than", &Settings::OutlineMaxAbove, 0.0f, 500.0f, "%.0f");
			Hint("Items, containers and corpses this far above your feet get no label or outline (a human is ~128 tall).");
			SliderFloat("Nothing lower than", &Settings::LabelMaxBelow, 0.0f, 500.0f, "%.0f");
			Hint("Items this far below your feet get no label and can't be clicked (floors below, hidden merchant chest rooms).");

			SeparatorText("Loot filter");
			SliderInt("Hide items worth less than", &Settings::LabelMinValue, 0, 500);
			Hint("Gold is always shown.");
			TextDisabled("Always shown: gear, potions, scrolls, spell tomes, keys, soul gems, gold, gems, ore.");
			Checkbox("Food", &Settings::ShowFood);
			SameLine();
			Checkbox("Ingredients", &Settings::ShowIngredients);
			SameLine();
			Checkbox("Books", &Settings::ShowBooks);
			SameLine();
			Checkbox("Clutter", &Settings::ShowClutter);
			Hint("Kettles, buckets, plates, torches and other junk.");

			SeparatorText("POErim");
			if (LootApi::HasV2()) {
				TextDisabled("POErim 2.1.0+ found.");
			} else {
				TextDisabled("POErim 2.1.0+ not found: these do nothing.");
			}
			Checkbox("Rarity colours", &Settings::UseRarityColor);
			Hint("Labels and outlines take the item's rarity colour from POErim: normal, magic, rare, currency, corrupted.\n"
				 "Its orbs always show as loot, whatever the filter says.");
			bool  ownCurrency = Settings::CurrencyColor != 0;
			float currency[3] = { static_cast<float>((Settings::CurrencyColor >> 16) & 0xFF) / 255.0f,
				static_cast<float>((Settings::CurrencyColor >> 8) & 0xFF) / 255.0f, static_cast<float>(Settings::CurrencyColor & 0xFF) / 255.0f };
			if (Checkbox("Own currency colour", &ownCurrency)) {
				Settings::CurrencyColor = ownCurrency ? 0xE6C364u : 0u;
			}
			Hint("Off: orbs use POErim's own currency colour.");
			if (ownCurrency) {
				SameLine();
				if (ColorEdit3("Currency colour", currency, 0)) {
					const auto channel = [](float a_v) { return static_cast<std::uint32_t>(std::lround(std::clamp(a_v, 0.0f, 1.0f) * 255.0f)); };
					// Pure black is the "use POErim's colour" value, so it is kept one step off black.
					Settings::CurrencyColor = std::max((channel(currency[0]) << 16) | (channel(currency[1]) << 8) | channel(currency[2]), 0x000001u);
				}
			}

			Footer();
		}

		void __stdcall RenderHud()
		{
			SeparatorText("Life and mana globes");
			Checkbox("Show globes", &Settings::Globes);
			Checkbox("Hide the game's bars", &Settings::HideVanillaBars);
			SliderFloat("Globe size", &Settings::GlobeSize, 0.5f, 2.0f, "%.2f");
			Checkbox("Health, mana and stamina numbers", &Settings::GlobeNumbers);
			Checkbox("Moving liquid", &Settings::GlobeAnimate);
			Hint("A wave on the surface and rising bubbles. Off: a still surface.");

			Footer();
		}

		void __stdcall RenderPlayer()
		{
			SeparatorText("Player (applied on every load)");
			SliderFloat("Carry weight", &Settings::CarryWeight, 0.0f, 20000.0f, "%.0f");
			Hint("0 = leave the game's value alone.");
			SliderFloat("Speed mult", &Settings::SpeedMult, 0.0f, 300.0f, "%.0f");
			Hint("100 = vanilla. 0 = leave the game's value alone.");
			if (Button("Apply now")) {
				RunOnMainThread([] { Camera::ApplyPlayerValues(); });
				g_status = "Player values applied.";
			}

			SeparatorText("Keys");
			TextWrapped("Camera on/off: %#x   Cutaway style: %#x   Pitch up/down: %#x / %#x   Yaw 90 degrees: %#x   Reload: %#x",
				Settings::CameraToggleKey, Settings::StyleKey, Settings::PitchUpKey, Settings::PitchDownKey, Settings::YawStepKey, Settings::ReloadKey);
			TextDisabled("Change keys in the INI (DirectX scan codes), then Reload.");

			Footer();
		}
	}

	void Register()
	{
		if (!SKSEMenuFramework::IsInstalled()) {
			logs::info("[menu] SKSE Menu Framework not installed; no in-game settings pages (the INI still works)");
			return;
		}
		SKSEMenuFramework::SetSection("ARPG POV");
		SKSEMenuFramework::AddSectionItem("Camera", RenderCamera);
		SKSEMenuFramework::AddSectionItem("Cutaway", RenderCutaway);
		SKSEMenuFramework::AddSectionItem("Controls", RenderControls);
		SKSEMenuFramework::AddSectionItem("Outlines", RenderOutlines);
		SKSEMenuFramework::AddSectionItem("Labels and Loot", RenderLabels);
		SKSEMenuFramework::AddSectionItem("HUD", RenderHud);
		SKSEMenuFramework::AddSectionItem("Player", RenderPlayer);
		logs::info("[menu] registered with SKSE Menu Framework");
	}
}
