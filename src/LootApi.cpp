#include "LootApi.h"
#include "Settings.h"

#include "api/DiabloLootAPI.h"

namespace LootApi
{
	namespace
	{
		// Valid for the whole process once POErim has sent it (see the header's handshake notes).
		std::atomic<const DiabloLootAPI::InterfaceV2*> g_v2{ nullptr };

		void OnMessage(SKSE::MessagingInterface::Message* a_msg)
		{
			if (!a_msg || a_msg->type != DiabloLootAPI::kMsgHelloV2) {
				return;  // every other plugin's messages come through here too
			}
			const auto* api = static_cast<const DiabloLootAPI::InterfaceV2*>(a_msg->data);
			if (!api || a_msg->dataLen < sizeof(DiabloLootAPI::InterfaceV2) || api->structSize < sizeof(DiabloLootAPI::InterfaceV2) ||
				api->version < DiabloLootAPI::kVersion2 || !api->InfoOfRef) {
				logs::warn("[loot] POErim's DiabloLootAPI v2 message was not usable (size {}, version {}); loot colours stay as they were",
					api ? api->structSize : 0u, api ? api->version : 0u);
				return;
			}
			g_v2.store(api);
			logs::info("[loot] POErim DiabloLootAPI v{} connected: loot is coloured by rarity (UseRarityColor = {})", api->version,
				Settings::UseRarityColor);
		}
	}

	void Register()
	{
		// Unfiltered (sender nullptr), as the API header asks: POErim broadcasts its hello at kPostLoad.
		if (auto* messaging = SKSE::GetMessagingInterface(); !messaging || !messaging->RegisterListener(nullptr, OnMessage)) {
			logs::warn("[loot] couldn't listen for POErim's DiabloLootAPI; loot colours stay as they were");
		}
	}

	bool HasV2()
	{
		return g_v2.load() != nullptr;
	}

	bool ColorOfRef(RE::FormID a_ref, std::uint32_t& a_color, bool& a_currency)
	{
		a_currency = false;
		const auto* api = g_v2.load();
		if (!api || !Settings::UseRarityColor || a_ref == 0) {
			return false;
		}
		DiabloLootAPI::ItemInfo info{};
		info.structSize = sizeof(DiabloLootAPI::ItemInfo);
		api->InfoOfRef(a_ref, &info);
		a_currency = info.currency != 0 || info.rarity == DiabloLootAPI::kRarityCurrency;
		if (a_currency && Settings::CurrencyColor != 0) {
			// Settings keep 0xRRGGBB; targets carry 0xAABBGGRR.
			const auto c = Settings::CurrencyColor;
			a_color = ((c >> 16) & 0xFF) | (c & 0xFF00) | ((c & 0xFF) << 16) | 0xFF000000u;
			return true;
		}
		if (info.rarity == DiabloLootAPI::kRarityUnknown || info.rgba == 0) {
			return false;  // not POErim's to colour: the usual colour stays
		}
		a_color = info.rgba | 0xFF000000u;  // already 0xAABBGGRR (red when corrupted); always drawn opaque here
		return true;
	}
}
