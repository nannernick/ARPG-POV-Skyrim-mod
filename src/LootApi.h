#pragma once

// POErim (DiabloInSkyrim) tells other plugins about its loot through DiabloLootAPI (src/api/DiabloLootAPI.h,
// vendored verbatim from POErim). Only v2 (POErim 2.1.0+) is used: it gives each item's rarity and the colour
// to draw it in. Without v2 (POErim missing or older) nothing here answers and the loot colours are as before.
namespace LootApi
{
	void Register();  // in SKSEPluginLoad: listens for POErim's hello, which it sends at kPostLoad

	bool HasV2();  // POErim 2.1.0+ answered

	// A dropped item's colour (0xAABBGGRR, the Target colour order) from its rarity, and whether it is a
	// crafting orb. False = POErim has nothing to say about it (not its item, no v2, or UseRarityColor off):
	// keep the usual colour. Main thread.
	bool ColorOfRef(RE::FormID a_ref, std::uint32_t& a_color, bool& a_currency);
}
