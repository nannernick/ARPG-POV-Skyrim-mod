// ARPG Point and Click: on-screen cursor, click-to-move, click pickup and item labels.
// Built against CommonLibSSE-NG (alandtse fork) for SE/AE/1.7.x runtimes.

#include "Game.h"
#include "Hud.h"
#include "Input.h"
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
			Hud::Register();
			break;
		case SKSE::MessagingInterface::kInputLoaded:
			Input::Register();
			break;
		case SKSE::MessagingInterface::kDataLoaded:
			Game::RegisterMenuSink();
			break;
		case SKSE::MessagingInterface::kPreLoadGame:
		case SKSE::MessagingInterface::kNewGame:
			Game::Reset();
			break;
		case SKSE::MessagingInterface::kPostLoadGame:
			if (!Hud::Available()) {
				RE::SendHUDMessage::ShowHUDMessage("ARPG Point and Click needs SKSE Menu Framework");
			}
			break;
		default:
			break;
		}
	}
}

SKSEPluginLoad(const SKSE::LoadInterface* a_skse)
{
	SKSE::Init(a_skse);
	logs::info("ARPG Point and Click loading");

	Settings::Load();
	Game::Install();
	Input::Install();

	if (auto* messaging = SKSE::GetMessagingInterface()) {
		messaging->RegisterListener(OnMessage);
	}

	logs::info("ARPG Point and Click loaded");
	return true;
}
