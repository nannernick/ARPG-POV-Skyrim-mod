#include "Globes.h"
#include "Hud.h"
#include "Settings.h"

#include "SKSEMenuFramework.h"

namespace Globes
{
	namespace
	{
		using namespace ImGuiMCP;

		struct Value
		{
			std::atomic<float> current{ 0.0f };
			std::atomic<float> max{ 1.0f };
		};

		Value             g_health;
		Value             g_magicka;
		Value             g_stamina;
		std::atomic<bool> g_visible{ false };

		void Read(RE::PlayerCharacter* a_player, RE::ActorValue a_av, Value& a_out)
		{
			auto*       owner = a_player->AsActorValueOwner();
			const float max = owner->GetPermanentActorValue(a_av) + a_player->GetActorValueModifier(RE::ACTOR_VALUE_MODIFIER::kTemporary, a_av);
			a_out.current.store(std::max(owner->GetActorValue(a_av), 0.0f));
			a_out.max.store(std::max(max, 1.0f));
		}

		// The game's own health, magicka and stamina bars.
		void ShowVanillaBars(bool a_show)
		{
			auto* ui = RE::UI::GetSingleton();
			if (!ui) {
				return;
			}
			const auto hud = ui->GetMenu(RE::HUDMenu::MENU_NAME);
			if (!hud || !hud->uiMovie) {
				return;
			}
			const RE::GFxValue visible{ a_show };
			for (const char* path : { "_root.HUDMovieBaseInstance.Health._visible", "_root.HUDMovieBaseInstance.Magica._visible",
					 "_root.HUDMovieBaseInstance.Stamina._visible" }) {
				hud->uiMovie->SetVariable(path, visible);
			}
		}

		constexpr float kPi = 3.14159265f;

		ImU32 Rgba(int a_r, int a_g, int a_b, int a_a = 255)
		{
			return IM_COL32(a_r, a_g, a_b, a_a);
		}

		// The part of a circle below a horizontal line, as a convex polygon.
		void FillBelow(ImDrawList* a_list, ImVec2 a_centre, float a_radius, float a_level, ImU32 a_color)
		{
			const float s = std::clamp((a_level - a_centre.y) / a_radius, -1.0f, 1.0f);
			if (s >= 1.0f) {
				return;
			}
			const float from = std::asin(s);
			const float to = kPi - from;
			constexpr int kSteps = 48;
			ImVec2        points[kSteps + 1];
			for (int i = 0; i <= kSteps; ++i) {
				const float a = from + (to - from) * static_cast<float>(i) / static_cast<float>(kSteps);
				points[i] = ImVec2{ a_centre.x + std::cos(a) * a_radius, a_centre.y + std::sin(a) * a_radius };
			}
			ImDrawListManager::AddConvexPolyFilled(a_list, points, kSteps + 1, a_color);
		}

		void Arc(ImDrawList* a_list, ImVec2 a_centre, float a_radius, float a_from, float a_to, ImU32 a_color, float a_thickness)
		{
			constexpr int kSteps = 40;
			ImVec2        points[kSteps + 1];
			for (int i = 0; i <= kSteps; ++i) {
				const float a = a_from + (a_to - a_from) * static_cast<float>(i) / static_cast<float>(kSteps);
				points[i] = ImVec2{ a_centre.x + std::cos(a) * a_radius, a_centre.y + std::sin(a) * a_radius };
			}
			ImDrawListManager::AddPolyline(a_list, points, kSteps + 1, a_color, 0, a_thickness);
		}

		void Text(ImDrawList* a_list, ImFont* a_font, float a_size, ImVec2 a_centre, const std::string& a_text)
		{
			const float  scale = a_size / std::max(GetFontSize(), 1.0f);
			const ImVec2 size = CalcTextSize(a_text.c_str());
			const ImVec2 at{ a_centre.x - size.x * scale * 0.5f, a_centre.y - size.y * scale * 0.5f };
			ImDrawListManager::AddText(a_list, a_font, a_size, ImVec2{ at.x + 1.0f, at.y + 1.0f }, Rgba(0, 0, 0, 220), a_text.c_str());
			ImDrawListManager::AddText(a_list, a_font, a_size, at, Rgba(235, 225, 210), a_text.c_str());
		}

		struct Colours
		{
			ImU32 deep;     // bottom of the liquid
			ImU32 bright;   // its lit middle
			ImU32 surface;  // the line on top
			ImU32 trail;    // just-lost amount, fading
		};

		// One globe: a dark glass ball in a metal rim, filled to a_fill (0..1).
		void Globe(ImDrawList* a_list, ImFont* a_font, ImVec2 a_centre, float a_radius, float a_fill, float a_trail, const Colours& a_colours,
			const Value& a_value, float a_scale, const char* a_name)
		{
			const float r = a_radius;
			// Rim and empty glass.
			ImDrawListManager::AddCircleFilled(a_list, a_centre, r + 7.0f * a_scale, Rgba(18, 14, 10, 235), 64);
			ImDrawListManager::AddCircleFilled(a_list, a_centre, r, Rgba(12, 8, 8, 235), 64);

			// Recently lost, then the liquid: a darker body with a lit core.
			const float top = a_centre.y + r - 2.0f * r * a_fill;
			if (a_trail > a_fill + 0.002f) {
				FillBelow(a_list, a_centre, r, a_centre.y + r - 2.0f * r * a_trail, a_colours.trail);
			}
			FillBelow(a_list, a_centre, r, top, a_colours.deep);
			FillBelow(a_list, ImVec2{ a_centre.x, a_centre.y - r * 0.08f }, r * 0.78f, std::max(top, a_centre.y - r * 0.86f), a_colours.bright);
			if (a_fill > 0.01f && a_fill < 0.99f) {
				const float dy = top - a_centre.y;
				const float half = std::sqrt(std::max(r * r - dy * dy, 0.0f));
				ImDrawListManager::AddLine(a_list, ImVec2{ a_centre.x - half + 2.0f, top }, ImVec2{ a_centre.x + half - 2.0f, top }, a_colours.surface, 1.5f * a_scale);
			}

			// Glass: a soft highlight top-left and a shadowed edge.
			ImDrawListManager::AddEllipseFilled(a_list, ImVec2{ a_centre.x - r * 0.30f, a_centre.y - r * 0.48f }, ImVec2{ r * 0.42f, r * 0.22f },
				Rgba(255, 255, 255, 28), -0.5f, 32);
			ImDrawListManager::AddCircle(a_list, a_centre, r - 1.5f * a_scale, Rgba(0, 0, 0, 120), 64, 3.0f * a_scale);
			// Metal rim: dark band with a bronze line.
			ImDrawListManager::AddCircle(a_list, a_centre, r + 3.5f * a_scale, Rgba(120, 95, 60, 255), 64, 2.5f * a_scale);
			ImDrawListManager::AddCircle(a_list, a_centre, r + 7.0f * a_scale, Rgba(60, 45, 28, 255), 64, 1.5f * a_scale);

			if (Settings::GlobeNumbers) {
				const auto text = std::format("{} {:.0f}/{:.0f}", a_name, std::ceil(a_value.current.load()), a_value.max.load());
				Text(a_list, a_font, 17.0f * a_scale, ImVec2{ a_centre.x, a_centre.y - r - 18.0f * a_scale }, text);
			}
		}

		// Eases a shown value toward the real one; the trail drops slower, so losses are visible.
		struct Shown
		{
			float fill{ -1.0f };
			float trail{ -1.0f };

			void Step(float a_target, float a_delta)
			{
				if (fill < 0.0f) {
					fill = trail = a_target;
					return;
				}
				fill += (a_target - fill) * std::min(a_delta * 12.0f, 1.0f);
				if (a_target >= trail) {
					trail = fill;
				} else {
					trail = std::max(a_target, trail - a_delta * 0.35f);
				}
			}
		};
	}

	void Update()
	{
		static bool s_hidden = false;
		auto*       ui = RE::UI::GetSingleton();
		auto*       player = RE::PlayerCharacter::GetSingleton();

		const bool visible = Settings::Globes && ui && player && player->Is3DLoaded() &&
		                     !ui->GameIsPaused() &&
		                     !ui->IsMenuOpen(RE::LoadingMenu::MENU_NAME) &&
		                     !ui->IsMenuOpen(RE::MainMenu::MENU_NAME) &&
		                     !ui->IsMenuOpen(RE::MapMenu::MENU_NAME) &&
		                     !ui->IsMenuOpen(RE::RaceSexMenu::MENU_NAME) &&
		                     !Hud::FrameworkMenuOpen();
		g_visible.store(visible);
		if (player && player->Is3DLoaded()) {
			Read(player, RE::ActorValue::kHealth, g_health);
			Read(player, RE::ActorValue::kMagicka, g_magicka);
			Read(player, RE::ActorValue::kStamina, g_stamina);
		}

		// Hide the game's bars while the globes are on; give them back when switched off.
		// Re-applied now and then, since the game can rebuild its HUD.
		static int s_frames = 0;
		const bool hide = Settings::Globes && Settings::HideVanillaBars && Hud::Available();
		if (hide && (!s_hidden || ++s_frames >= 60)) {
			s_frames = 0;
			ShowVanillaBars(false);
			s_hidden = true;
		} else if (!hide && s_hidden) {
			ShowVanillaBars(true);
			s_hidden = false;
		}
	}

	void Draw(ImDrawList* a_list, ImFont* a_font, float a_width, float a_height, float a_delta)
	{
		if (!g_visible.load() || !a_list || !a_font) {
			return;
		}
		const float scale = a_height / 1080.0f * std::clamp(Settings::GlobeSize, 0.5f, 2.0f);
		const float r = 78.0f * scale;
		const float margin = 28.0f * scale;
		const ImVec2 life{ margin + r + 8.0f * scale, a_height - margin - r };
		const ImVec2 mana{ a_width - margin - r - 8.0f * scale, a_height - margin - r };

		static Shown s_health, s_magicka, s_stamina;
		s_health.Step(g_health.current.load() / g_health.max.load(), a_delta);
		s_magicka.Step(g_magicka.current.load() / g_magicka.max.load(), a_delta);
		s_stamina.Step(g_stamina.current.load() / g_stamina.max.load(), a_delta);

		const Colours red{ Rgba(105, 8, 8, 240), Rgba(175, 22, 18, 235), Rgba(240, 90, 70, 220), Rgba(200, 160, 140, 110) };
		const Colours blue{ Rgba(10, 25, 110, 240), Rgba(30, 70, 190, 235), Rgba(110, 160, 255, 220), Rgba(160, 175, 220, 110) };
		Globe(a_list, a_font, life, r, std::clamp(s_health.fill, 0.0f, 1.0f), std::clamp(s_health.trail, 0.0f, 1.0f), red, g_health, scale, "Health");
		Globe(a_list, a_font, mana, r, std::clamp(s_magicka.fill, 0.0f, 1.0f), std::clamp(s_magicka.trail, 0.0f, 1.0f), blue, g_magicka, scale, "Mana");

		// Stamina: a curved bar hugging the right side of the life globe, filling bottom to top.
		const float bar = r + 15.0f * scale;
		const float thick = 8.0f * scale;
		const float bottom = kPi * 0.36f;  // lower right
		const float top = -kPi * 0.36f;    // upper right
		Arc(a_list, life, bar, bottom, top, Rgba(14, 11, 8, 235), thick + 4.0f * scale);
		const float stamina = std::clamp(s_stamina.fill, 0.0f, 1.0f);
		if (stamina > 0.005f) {
			Arc(a_list, life, bar, bottom, bottom + (top - bottom) * stamina, Rgba(70, 160, 60, 240), thick);
			Arc(a_list, life, bar - thick * 0.25f, bottom, bottom + (top - bottom) * stamina, Rgba(150, 220, 110, 120), thick * 0.3f);
		}
		if (Settings::GlobeNumbers) {
			// Beside the bar, level with the bottom of the life globe.
			const auto  text = std::format("Stamina {:.0f}/{:.0f}", std::ceil(g_stamina.current.load()), g_stamina.max.load());
			const float size = 17.0f * scale;
			const float width = CalcTextSize(text.c_str()).x * size / std::max(GetFontSize(), 1.0f);
			Text(a_list, a_font, size, ImVec2{ life.x + bar + 14.0f * scale + width * 0.5f, life.y + r * 0.62f }, text);
		}
	}
}
