// ARPG POV: the ARPG camera (pitch lock + interior cutaway), point and click (on-screen cursor,
// click-to-move, click pickup, item labels) and model outlines, in one plugin.
// Built against CommonLibSSE-NG (alandtse fork) for SE/AE/1.7.x runtimes.

#include "Camera.h"
#include "Cut.h"
#include "Game.h"
#include "Hud.h"
#include "Input.h"
#include "Menu.h"
#include "Occlusion.h"
#include "Outline.h"
#include "Settings.h"

namespace
{
	void OnMessage(SKSE::MessagingInterface::Message* a_msg)
	{
		if (!a_msg) {
			return;
		}
		switch (a_msg->type) {
		case SKSE::MessagingInterface::kPostLoad:
			Menu::Register();  // settings pages: Camera, Cutaway, Player
			Hud::Register();   // settings page: Point and Click, plus the cursor and labels
			break;
		case SKSE::MessagingInterface::kInputLoaded:
			Input::Register();
			break;
		case SKSE::MessagingInterface::kDataLoaded:
			Camera::ApplyGameSettings();
			Game::RegisterMenuSink();
			Outline::Install();
			break;
		case SKSE::MessagingInterface::kPreLoadGame:
			Occlusion::RestoreAll();
			Game::Reset();
			break;
		case SKSE::MessagingInterface::kNewGame:
			Occlusion::RestoreAll();
			Game::Reset();
			Camera::ApplyPlayerValues();
			break;
		case SKSE::MessagingInterface::kPostLoadGame:
			Camera::ApplyPlayerValues();
			if (!Hud::Available()) {
				RE::SendHUDMessage::ShowHUDMessage("ARPG POV: point and click needs SKSE Menu Framework");
			}
			break;
		default:
			break;
		}
	}
}

SKSEPluginLoad(const SKSE::LoadInterface* a_skse)
{
	SKSE::Init(a_skse, { .trampoline = true, .trampolineSize = 64 });
	logs::info("ARPG POV loading");

	Settings::Load();
	Camera::Install();
	Occlusion::Install();
	Cut::Install();
	Game::Install();
	Input::Install();

	if (auto* messaging = SKSE::GetMessagingInterface()) {
		messaging->RegisterListener(OnMessage);
	}

	logs::info("ARPG POV loaded");
	return true;
}
