#include "Hud.h"
#include "Cursor.h"
#include "Globes.h"
#include "Game.h"
#include "Hull.h"
#include "Input.h"
#include "Outline.h"
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
			case Game::Kind::kFollower:
				return Settings::Actors;
			default:
				return true;
			}
		}

		// A circle on the ground under an actor: 24 points around its feet, projected like the labels.
		// It is drawn over the picture, so it shows through walls and over the actor's own feet.
		void DrawGroundCircle(ImDrawList* a_list, const Game::Target& a_target, float a_w, float a_h)
		{
			constexpr int kPoints = 24;
			ImVec2        points[kPoints];
			const float   radius = std::clamp(Settings::CircleRadius, 10.0f, 300.0f);
			bool          onScreen = false;
			for (int i = 0; i < kPoints; ++i) {
				const float        angle = static_cast<float>(i) * (2.0f * std::numbers::pi_v<float> / static_cast<float>(kPoints));
				const RE::NiPoint3 point{ a_target.pos.x + std::cos(angle) * radius, a_target.pos.y + std::sin(angle) * radius, a_target.pos.z + 3.0f };
				float              u = 0.0f;
				float              v = 0.0f;
				if (!Game::WorldToScreen(point, u, v)) {
					return;  // partly behind the camera
				}
				points[i] = ImVec2{ u * a_w, v * a_h };
				onScreen = onScreen || (u >= 0.0f && u <= 1.0f && v >= 0.0f && v <= 1.0f);
			}
			if (!onScreen) {
				return;
			}
			const auto  alpha = static_cast<std::uint32_t>(std::clamp(Settings::OutlineOpacity, 0.05f, 1.0f) * 255.0f);
			const ImU32 color = (a_target.color & 0x00FFFFFFu) | (alpha << 24);
			ImDrawListManager::AddPolyline(a_list, points, kPoints, color, ImDrawFlags_Closed, 2.0f);
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
				case Game::Kind::kFollower:
					icon = Cursor::Icon::kMark;
					color = IM_COL32(90, 225, 110, 255);
					break;
				case Game::Kind::kDoor:
				case Game::Kind::kActivator:
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

			if (auto* globes = GetForegroundDrawList(); globes) {
				Globes::Draw(globes, GetFont(), w, h, io->DeltaTime);
			}

			if (!State::gameplay.load() || FrameworkMenuOpen()) {
				SetHovered({}, Game::Kind::kItem);
				// In the inventory and other menus, draw our cursor where the game's would be.
				if (State::menuCursor.load() && !FrameworkMenuOpen()) {
					if (auto* cursor = RE::MenuCursor::GetSingleton(); cursor) {
						const auto& data = cursor->GetRuntimeData();
						const float x = data.screenWidthX > 1.0f ? data.cursorPosX * w / data.screenWidthX : data.cursorPosX;
						const float y = data.screenWidthY > 1.0f ? data.cursorPosY * h / data.screenWidthY : data.cursorPosY;
						if (auto* list = GetForegroundDrawList(); list) {
							DrawCursor(list, x, y, false, Game::Kind::kItem);
						}
					}
				}
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
				if (target.kind != Game::Kind::kItem || target.text.empty() || target.above > Settings::OutlineMaxAbove) {
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

			// 4. Draw: ground circles, labels, then the cursor on top. The outlines themselves are drawn
			//    around the real models by the outline renderer; these simple shapes are only the
			//    fallback if that renderer couldn't start.
			if (Settings::GroundCircles && targets) {
				for (const auto& target : *targets) {
					const bool actor = target.kind == Game::Kind::kEnemy || target.kind == Game::Kind::kNpc || target.kind == Game::Kind::kFollower;
					if (actor && Game::WantsMarker(target)) {
						DrawGroundCircle(list, target, w, h);
					}
				}
			}
			if (!Outline::Active()) {
				for (std::size_t i = 0; i < shapes.size(); ++i) {
					const auto& target = *shapes[i].target;
					const bool  hot = i == hovered;
					if (hot || (Settings::Outlines && Game::WantsMarker(target))) {
						DrawOutline(list, shapes[i].outline, target.color, hot ? 2.5f : 1.5f);
					}
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
	}

	void Register()
	{
		if (!SKSEMenuFramework::IsInstalled()) {
			logs::error("[hud] SKSE Menu Framework is not installed; the cursor and labels can't be drawn, so point and click stays off");
			return;
		}
		g_available = true;
		SKSEMenuFramework::AddHudElement(Render);  // the settings pages are in Menu.cpp
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
