#include "Hud.h"
#include "Game.h"
#include "Settings.h"

#include <filesystem>
#include "SKSEMenuFramework.h"

// The draw callbacks run on the render thread. They only read the camera and the shared State;
// anything that changes the game is done by Game on the main thread.
namespace Hud
{
	namespace
	{
		using namespace ImGuiMCP;

		bool        g_available = false;
		std::string g_status;

		struct Label
		{
			const Game::Target* target;
			float               anchorX;
			float               anchorY;
			float               x0;
			float               y0;
			float               x1;
			float               y1;
		};

		bool Overlaps(const Label& a_lhs, const Label& a_rhs)
		{
			return a_lhs.x0 < a_rhs.x1 && a_rhs.x0 < a_lhs.x1 && a_lhs.y0 < a_rhs.y1 && a_rhs.y0 < a_lhs.y1;
		}

		void SetHovered(RE::ObjectRefHandle a_handle)
		{
			std::scoped_lock guard(State::lock);
			State::hovered = a_handle;
		}

		void DrawCursor(ImDrawList* a_list, float a_x, float a_y, bool a_hot)
		{
			const float  s = 18.0f * Settings::CursorSize;
			const ImVec2 tip{ a_x, a_y };
			const ImVec2 left{ a_x, a_y + s };
			const ImVec2 right{ a_x + s * 0.7f, a_y + s * 0.7f };
			ImDrawListManager::AddTriangleFilled(a_list, tip, left, right, a_hot ? IM_COL32(255, 210, 90, 255) : IM_COL32(255, 255, 255, 255));
			ImDrawListManager::AddTriangle(a_list, tip, left, right, IM_COL32(0, 0, 0, 255), 1.5f);
		}

		void __stdcall Render()
		{
			auto* io = GetIO();
			if (!io || io->DisplaySize.x < 1.0f || io->DisplaySize.y < 1.0f) {
				return;
			}
			const float w = io->DisplaySize.x;
			const float h = io->DisplaySize.y;
			State::screenW.store(w);
			State::screenH.store(h);

			if (!State::gameplay.load() || FrameworkMenuOpen()) {
				SetHovered({});
				return;
			}

			float cx = State::cursorX.load();
			float cy = State::cursorY.load();
			if (cx < 0.0f || cy < 0.0f) {
				cx = w * 0.5f;
				cy = h * 0.5f;
				State::cursorX.store(cx);
				State::cursorY.store(cy);
			}

			auto* list = GetForegroundDrawList();
			auto* font = GetFont();
			if (!list || !font) {
				return;
			}

			std::vector<Game::Target> targets;
			{
				std::scoped_lock guard(State::lock);
				targets = State::targets;
			}

			const bool  showAll = Settings::LabelMode == 1 || (Settings::LabelMode == 2 && State::labelKeyHeld.load());
			const float scale = Settings::LabelScale;
			const float fontSize = GetFontSize() * scale;
			const float padX = 5.0f * scale;
			const float padY = 2.0f * scale;
			const float hoverRadius = 28.0f * scale;

			// Lay the labels out. The list is nearest-first, so close items keep their true
			// position and further ones stack upward out of the way.
			std::vector<Label> labels;
			labels.reserve(targets.size());
			constexpr std::size_t none = static_cast<std::size_t>(-1);
			std::size_t           hovered = none;
			float hoveredDist = hoverRadius;
			for (const auto& target : targets) {
				float u = 0.0f;
				float v = 0.0f;
				if (!Game::WorldToScreen(target.pos, u, v) || u < -0.05f || u > 1.05f || v < -0.05f || v > 1.05f) {
					continue;
				}
				const float ax = u * w;
				const float ay = v * h;
				const float anchorDist = std::hypot(cx - ax, cy - ay);
				const bool  always = showAll && target.kind == Game::Kind::kItem;
				if (!always && anchorDist >= hoverRadius) {
					continue;  // only shown while the cursor is on it
				}
				if (always && static_cast<int>(labels.size()) >= Settings::MaxLabels) {
					continue;
				}

				const ImVec2 text = CalcTextSize(target.text.c_str());
				const float  width = text.x * scale + padX * 2.0f;
				const float  height = text.y * scale + padY * 2.0f;
				Label        label{ &target, ax, ay, ax - width * 0.5f, ay - height - 8.0f * scale, ax + width * 0.5f, ay - 8.0f * scale };
				for (int pass = 0; pass < 40; ++pass) {
					bool moved = false;
					for (const auto& placed : labels) {
						if (Overlaps(label, placed)) {
							label.y1 = placed.y0 - 1.0f;
							label.y0 = label.y1 - height;
							moved = true;
						}
					}
					if (!moved) {
						break;
					}
				}
				labels.push_back(label);

				// Hovered: the cursor on the item itself (nearest wins)...
				if (anchorDist < hoveredDist) {
					hoveredDist = anchorDist;
					hovered = labels.size() - 1;
				}
			}
			// ...or, first choice, the cursor on a label.
			for (std::size_t i = 0; i < labels.size(); ++i) {
				const auto& label = labels[i];
				if (cx >= label.x0 && cx <= label.x1 && cy >= label.y0 && cy <= label.y1) {
					hovered = i;
					break;
				}
			}

			for (std::size_t i = 0; i < labels.size(); ++i) {
				const auto&  label = labels[i];
				const bool   hot = i == hovered;
				const ImVec2 topLeft{ label.x0, label.y0 };
				const ImVec2 bottomRight{ label.x1, label.y1 };
				ImDrawListManager::AddRectFilled(list, topLeft, bottomRight, hot ? IM_COL32(45, 45, 45, 240) : IM_COL32(0, 0, 0, 175), 2.0f, 0);
				if (hot) {
					ImDrawListManager::AddRect(list, topLeft, bottomRight, label.target->color, 2.0f, 0, 1.0f);
				}
				ImDrawListManager::AddText(list, font, fontSize, ImVec2{ label.x0 + padX, label.y0 + padY }, label.target->color,
					label.target->text.c_str());
			}

			SetHovered(hovered != none ? labels[hovered].target->handle : RE::ObjectRefHandle{});
			DrawCursor(list, cx, cy, hovered != none);
		}

		void Hint(const char* a_text)
		{
			if (IsItemHovered()) {
				SetTooltip("%s", a_text);
			}
		}

		void __stdcall RenderSettings()
		{
			SeparatorText("Point and click");
			Checkbox("Enabled", &Settings::Enabled);
			Hint("Also toggled with the toggle key (F7 by default). Off = normal WASD and mouse look.");
			SliderFloat("Cursor speed", &Settings::Sensitivity, 0.1f, 5.0f, "%.2f");
			SliderFloat("Cursor size", &Settings::CursorSize, 0.5f, 4.0f, "%.1f");
			Checkbox("Mouse moves the cursor, not the camera", &Settings::BlockMouseLook);

			SeparatorText("Movement");
			Checkbox("Hold to keep moving", &Settings::HoldToMove);
			SliderFloat("Stop distance", &Settings::ArriveRadius, 5.0f, 200.0f, "%.0f");
			SliderFloat("Max click distance", &Settings::MaxDistance, 500.0f, 10000.0f, "%.0f");
			Checkbox("Clicking an enemy attacks it", &Settings::ClickEnemies);
			Hint("Off: every click moves. Shift + click always attacks toward the cursor.");
			const char* const modes[] = { "Auto", "True Directional Movement", "Turn the player (no TDM)" };
			Combo("Movement mode", &Settings::MoveMode, modes, 3);

			SeparatorText("Pickup");
			SliderFloat("Pickup range", &Settings::PickupRange, 50.0f, 500.0f, "%.0f");
			Hint("Closer than this picks up at once. Further away, you walk there first.");
			Checkbox("Move button also picks up", &Settings::LeftClickPickup);
			Checkbox("Pickup button opens containers", &Settings::Containers);
			Checkbox("Pickup button loots corpses and talks", &Settings::Actors);

			SeparatorText("Item labels");
			const char* const labelModes[] = { "Only under the cursor", "Always", "While the label key is held" };
			Combo("Show", &Settings::LabelMode, labelModes, 3);
			SliderFloat("Label distance", &Settings::LabelRadius, 200.0f, 5000.0f, "%.0f");
			SliderFloat("Text size", &Settings::LabelScale, 0.5f, 3.0f, "%.2f");
			SliderInt("Most labels", &Settings::MaxLabels, 5, 200);
			SliderInt("Hide items worth less than", &Settings::LabelMinValue, 0, 500);
			Hint("A simple loot filter. Gold is always shown.");

			Spacing();
			Separator();
			if (Button("Save to INI")) {
				g_status = Settings::Save() ? "Saved." : "Couldn't write the INI - see the log.";
			}
			Hint("Writes every setting to Data/SKSE/Plugins/ARPGPointClick.ini.");
			SameLine();
			if (Button("Reload INI")) {
				Settings::Load();
				g_status = "Reloaded from the INI.";
			}
			if (!g_status.empty()) {
				SameLine();
				TextDisabled("%s", g_status.c_str());
			}
			TextDisabled("Buttons and keys are set in the INI.");
		}
	}

	void Register()
	{
		if (!SKSEMenuFramework::IsInstalled()) {
			logs::error("[hud] SKSE Menu Framework is not installed; the cursor and labels can't be drawn, so point and click stays off");
			return;
		}
		g_available = true;
		SKSEMenuFramework::SetSection("ARPG Point and Click");
		SKSEMenuFramework::AddSectionItem("Settings", RenderSettings);
		SKSEMenuFramework::AddHudElement(Render);
		logs::info("[hud] registered with SKSE Menu Framework");
	}

	bool Available()
	{
		return g_available;
	}

	bool FrameworkMenuOpen()
	{
		return g_available && SKSEMenuFramework::IsAnyBlockingWindowOpened();
	}
}
