#include "Hud.h"
#include "Cursor.h"
#include "Game.h"
#include "Hull.h"
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

		// Something on screen: its outline (also its click area).
		struct Shape
		{
			const Game::Target*     target;
			std::vector<Hull::Vec2> outline;
			float                   area;
		};

		struct Label
		{
			std::size_t shape;
			float       x0;
			float       y0;
			float       x1;
			float       y1;
		};

		constexpr std::size_t kNone = static_cast<std::size_t>(-1);

		bool Overlaps(const Label& a_lhs, const Label& a_rhs)
		{
			return a_lhs.x0 < a_rhs.x1 && a_rhs.x0 < a_lhs.x1 && a_lhs.y0 < a_rhs.y1 && a_rhs.y0 < a_lhs.y1;
		}

		void SetHovered(RE::ObjectRefHandle a_handle, Game::Kind a_kind)
		{
			std::scoped_lock guard(State::lock);
			State::hovered = a_handle;
			State::hoveredKind = a_kind;
		}

		// Projects a target's points and wraps them in an outline. False if it's off screen.
		bool MakeShape(const Game::Target& a_target, float a_w, float a_h, Shape& a_out)
		{
			if (a_target.points.empty()) {
				return false;
			}
			std::vector<Hull::Vec2> points;
			points.reserve(a_target.points.size());
			bool onScreen = false;
			for (const auto& point : a_target.points) {
				float u = 0.0f;
				float v = 0.0f;
				if (!Game::WorldToScreen(point, u, v)) {
					return false;  // partly behind the camera
				}
				points.push_back({ u * a_w, v * a_h });
				onScreen = onScreen || (u >= 0.0f && u <= 1.0f && v >= 0.0f && v <= 1.0f);
			}
			if (!onScreen) {
				return false;
			}

			// How many pixels one Skyrim unit covers here, to turn the padding into pixels.
			float       pixelsPerUnit = 0.0f;
			const auto& origin = a_target.points.front();
			float       u0 = 0.0f;
			float       v0 = 0.0f;
			if (Game::WorldToScreen(origin, u0, v0)) {
				const RE::NiPoint3 steps[3] = { { 100.0f, 0.0f, 0.0f }, { 0.0f, 100.0f, 0.0f }, { 0.0f, 0.0f, 100.0f } };
				for (const auto& step : steps) {
					float u = 0.0f;
					float v = 0.0f;
					if (Game::WorldToScreen(origin + step, u, v)) {
						pixelsPerUnit = std::max(pixelsPerUnit, std::hypot((u - u0) * a_w, (v - v0) * a_h) / 100.0f);
					}
				}
			}
			const float pad = std::clamp(a_target.pad * pixelsPerUnit, 2.0f, 40.0f);

			a_out.target = &a_target;
			a_out.outline = Hull::Inflate(Hull::Convex(std::move(points)), pad);
			a_out.area = Hull::Area(a_out.outline);
			return a_out.outline.size() >= 3;
		}

		bool Clickable(Game::Kind a_kind)
		{
			switch (a_kind) {
			case Game::Kind::kPlayer:
				return false;
			case Game::Kind::kCorpse:
			case Game::Kind::kNpc:
				return Settings::Actors;
			default:
				return true;
			}
		}

		// Whether a target's outline is always drawn (otherwise only while the cursor is on it).
		bool AlwaysOutlined(Game::Kind a_kind)
		{
			if (!Settings::Outlines) {
				return false;
			}
			switch (a_kind) {
			case Game::Kind::kEnemy:
				return Settings::OutlineEnemies;
			case Game::Kind::kNpc:
			case Game::Kind::kPlayer:
				return Settings::OutlineNpcs;
			case Game::Kind::kItem:
				return Settings::OutlineItems;
			default:
				return false;
			}
		}

		// Lets the cursor icons draw onto the HUD.
		struct Painter
		{
			ImDrawList* list;

			void Line(Cursor::P a_a, Cursor::P a_b, std::uint32_t a_color, float a_thickness) const
			{
				ImDrawListManager::AddLine(list, ImVec2{ a_a.x, a_a.y }, ImVec2{ a_b.x, a_b.y }, a_color, a_thickness);
			}
			void Tri(Cursor::P a_a, Cursor::P a_b, Cursor::P a_c, std::uint32_t a_color) const
			{
				ImDrawListManager::AddTriangleFilled(list, ImVec2{ a_a.x, a_a.y }, ImVec2{ a_b.x, a_b.y }, ImVec2{ a_c.x, a_c.y }, a_color);
			}
			void Quad(Cursor::P a_a, Cursor::P a_b, Cursor::P a_c, Cursor::P a_d, std::uint32_t a_color) const
			{
				ImDrawListManager::AddQuadFilled(list, ImVec2{ a_a.x, a_a.y }, ImVec2{ a_b.x, a_b.y }, ImVec2{ a_c.x, a_c.y }, ImVec2{ a_d.x, a_d.y }, a_color);
			}
			void Circle(Cursor::P a_centre, float a_radius, std::uint32_t a_color) const
			{
				ImDrawListManager::AddCircleFilled(list, ImVec2{ a_centre.x, a_centre.y }, a_radius, a_color, 0);
			}
			// Part of a circle, from one angle to another (radians, 0 = right, clockwise on screen).
			void Arc(Cursor::P a_centre, float a_radius, float a_from, float a_to, std::uint32_t a_color, float a_thickness) const
			{
				constexpr int kSteps = 32;
				ImVec2        points[kSteps + 1];
				for (int i = 0; i <= kSteps; ++i) {
					const float angle = a_from + (a_to - a_from) * static_cast<float>(i) / static_cast<float>(kSteps);
					points[i] = ImVec2{ a_centre.x + std::cos(angle) * a_radius, a_centre.y + std::sin(angle) * a_radius };
				}
				ImDrawListManager::AddPolyline(list, points, kSteps + 1, a_color, 0, a_thickness);
			}
		};

		// The cursor shows what a click will do: a red sword attacks, a gold bag loots,
		// an exclamation mark talks (green) or goes through a door (white), the arrow moves.
		void DrawCursor(ImDrawList* a_list, float a_x, float a_y, bool a_hovering, Game::Kind a_kind)
		{
			constexpr ImU32 white = IM_COL32(255, 255, 255, 255);
			auto            icon = Cursor::Icon::kArrow;
			ImU32           color = white;
			if (State::attackInPlaceHeld.load() || (a_hovering && a_kind == Game::Kind::kEnemy && Settings::ClickEnemies)) {
				icon = Cursor::Icon::kSword;
				color = IM_COL32(255, 60, 60, 255);
			} else if (a_hovering) {
				switch (a_kind) {
				case Game::Kind::kItem:
				case Game::Kind::kContainer:
				case Game::Kind::kCorpse:
					icon = Cursor::Icon::kBag;
					color = IM_COL32(255, 205, 80, 255);
					break;
				case Game::Kind::kNpc:
					icon = Cursor::Icon::kMark;
					color = IM_COL32(90, 225, 110, 255);
					break;
				case Game::Kind::kDoor:
					icon = Cursor::Icon::kMark;
					break;
				default:
					break;
				}
			}
			Painter painter{ a_list };
			Cursor::Draw(painter, icon, a_x, a_y, 18.0f * Settings::CursorSize, color, Settings::CursorRing);
		}

		void DrawOutline(ImDrawList* a_list, const std::vector<Hull::Vec2>& a_outline, ImU32 a_color, float a_thickness)
		{
			static std::vector<ImVec2> points;  // only the HUD callback draws, so one buffer is enough
			points.clear();
			for (const auto& p : a_outline) {
				points.push_back(ImVec2{ p.x, p.y });
			}
			ImDrawListManager::AddPolyline(a_list, points.data(), static_cast<int>(points.size()), a_color, ImDrawFlags_Closed, a_thickness);
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
				SetHovered({}, Game::Kind::kItem);
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

			std::shared_ptr<const std::vector<Game::Target>> targets;
			{
				std::scoped_lock guard(State::lock);
				targets = State::targets;
			}

			// 1. Where everything is on screen.
			std::vector<Shape> shapes;
			if (targets) {
				shapes.reserve(targets->size());
				for (const auto& target : *targets) {
					Shape shape;
					if (MakeShape(target, w, h, shape)) {
						shapes.push_back(std::move(shape));
					}
				}
			}

			// 2. What the cursor is on. An enemy wins; otherwise the smallest thing under it, so
			//    an item in front of a door or next to a follower is still easy to hit.
			std::size_t hovered = kNone;
			for (std::size_t i = 0; i < shapes.size(); ++i) {
				const auto kind = shapes[i].target->kind;
				if (!Clickable(kind) || !Hull::Contains(shapes[i].outline, cx, cy)) {
					continue;
				}
				if (hovered == kNone) {
					hovered = i;
					continue;
				}
				const bool enemy = kind == Game::Kind::kEnemy;
				const bool bestEnemy = shapes[hovered].target->kind == Game::Kind::kEnemy;
				if (enemy != bestEnemy ? enemy : shapes[i].area < shapes[hovered].area) {
					hovered = i;
				}
			}

			// 3. Item labels. The list is nearest-first, so close items keep their true position
			//    and further ones stack upward out of the way.
			const bool  showAll = Settings::LabelMode == 1 || (Settings::LabelMode == 2 && State::labelKeyHeld.load());
			const float scale = Settings::LabelScale;
			const float fontSize = GetFontSize() * scale;
			const float padX = 5.0f * scale;
			const float padY = 2.0f * scale;

			std::vector<Label> labels;
			for (std::size_t i = 0; i < shapes.size(); ++i) {
				const auto& target = *shapes[i].target;
				if (target.kind != Game::Kind::kItem || target.text.empty()) {
					continue;
				}
				if (!showAll && i != hovered) {
					continue;
				}
				if (static_cast<int>(labels.size()) >= Settings::MaxLabels && i != hovered) {
					continue;
				}
				float u = 0.0f;
				float v = 0.0f;
				if (!Game::WorldToScreen(target.pos, u, v)) {
					continue;
				}
				const float  ax = u * w;
				const float  ay = v * h;
				const ImVec2 text = CalcTextSize(target.text.c_str());
				const float  width = text.x * scale + padX * 2.0f;
				const float  height = text.y * scale + padY * 2.0f;
				Label        label{ i, ax - width * 0.5f, ay - height - 8.0f * scale, ax + width * 0.5f, ay - 8.0f * scale };
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
			}
			// The cursor on a label counts as on its item, unless it's on an enemy.
			if (hovered == kNone || shapes[hovered].target->kind != Game::Kind::kEnemy) {
				for (const auto& label : labels) {
					if (cx >= label.x0 && cx <= label.x1 && cy >= label.y0 && cy <= label.y1) {
						hovered = label.shape;
						break;
					}
				}
			}

			// 4. Draw: outlines, then labels, then the cursor on top.
			const float thickness = Settings::OutlineThickness;
			for (std::size_t i = 0; i < shapes.size(); ++i) {
				const auto& target = *shapes[i].target;
				const bool  hot = i == hovered;
				if (hot || AlwaysOutlined(target.kind)) {
					DrawOutline(list, shapes[i].outline, target.color, hot ? thickness + 1.0f : thickness);
				}
			}
			for (const auto& label : labels) {
				const auto&  target = *shapes[label.shape].target;
				const bool   hot = label.shape == hovered;
				const ImVec2 topLeft{ label.x0, label.y0 };
				const ImVec2 bottomRight{ label.x1, label.y1 };
				ImDrawListManager::AddRectFilled(list, topLeft, bottomRight, hot ? IM_COL32(45, 45, 45, 240) : IM_COL32(0, 0, 0, 175), 2.0f, 0);
				if (hot) {
					ImDrawListManager::AddRect(list, topLeft, bottomRight, target.color, 2.0f, 0, 1.0f);
				}
				ImDrawListManager::AddText(list, font, fontSize, ImVec2{ label.x0 + padX, label.y0 + padY }, target.color, target.text.c_str());
			}

			const bool hovering = hovered != kNone;
			const auto kind = hovering ? shapes[hovered].target->kind : Game::Kind::kItem;
			SetHovered(hovering ? shapes[hovered].target->handle : RE::ObjectRefHandle{}, kind);
			DrawCursor(list, cx, cy, hovering, kind);
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
			Checkbox("Ring behind the sword, bag and exclamation mark", &Settings::CursorRing);
			Checkbox("Mouse moves the cursor, not the camera", &Settings::BlockMouseLook);

			SeparatorText("Movement");
			Checkbox("Hold to keep moving", &Settings::HoldToMove);
			SliderFloat("Stop distance", &Settings::ArriveRadius, 5.0f, 200.0f, "%.0f");
			SliderFloat("Max click distance", &Settings::MaxDistance, 500.0f, 10000.0f, "%.0f");
			Checkbox("Clicking an enemy attacks it", &Settings::ClickEnemies);
			Hint("Off: every click moves. Shift + click always attacks toward the cursor.");
			Checkbox("Clicking things uses them", &Settings::ClickInteract);
			Hint("Click an item to pick it up, a door to go through, an NPC to talk, a chest or corpse to loot.\nOff: only the pickup button (mouse 3) does.");
			const char* const modes[] = { "Auto", "True Directional Movement", "Turn the player (no TDM)" };
			Combo("Movement mode", &Settings::MoveMode, modes, 3);

			SeparatorText("Using things");
			SliderFloat("Reach", &Settings::PickupRange, 50.0f, 500.0f, "%.0f");
			Hint("Closer than this uses the thing at once. Further away, you walk there first.");
			Checkbox("Containers can be clicked", &Settings::Containers);
			Checkbox("Corpses and NPCs can be clicked", &Settings::Actors);

			SeparatorText("Outlines");
			Checkbox("Outlines", &Settings::Outlines);
			Hint("Off: only the thing under the cursor is outlined.");
			Checkbox("Enemies (red)", &Settings::OutlineEnemies);
			SameLine();
			Checkbox("You and NPCs (white)", &Settings::OutlineNpcs);
			SameLine();
			Checkbox("Items", &Settings::OutlineItems);
			SliderFloat("Line thickness", &Settings::OutlineThickness, 0.5f, 6.0f, "%.1f");
			SliderFloat("Outline distance", &Settings::OutlineRadius, 500.0f, 10000.0f, "%.0f");

			SeparatorText("Item labels");
			const char* const labelModes[] = { "Only under the cursor", "Always", "While the label key is held" };
			Combo("Show", &Settings::LabelMode, labelModes, 3);
			SliderFloat("Label distance", &Settings::LabelRadius, 200.0f, 5000.0f, "%.0f");
			SliderFloat("Text size", &Settings::LabelScale, 0.5f, 3.0f, "%.2f");
			SliderInt("Most labels", &Settings::MaxLabels, 5, 200);
			SliderInt("Hide items worth less than", &Settings::LabelMinValue, 0, 500);
			Hint("Gold is always shown.");

			SeparatorText("Loot filter");
			TextDisabled("Always shown: gear, potions, scrolls, spell tomes, keys, soul gems, gold, gems, ore.");
			Checkbox("Food", &Settings::ShowFood);
			SameLine();
			Checkbox("Ingredients", &Settings::ShowIngredients);
			SameLine();
			Checkbox("Books", &Settings::ShowBooks);
			SameLine();
			Checkbox("Clutter", &Settings::ShowClutter);
			Hint("Kettles, buckets, plates, torches and other junk.");

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
