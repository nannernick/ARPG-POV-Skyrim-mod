#pragma once

#include <string_view>

// Namespace and identity for the outline renderer in src/render, src/base and src/config,
// which comes from Highlight Lootable Corpses by AmazingBuff (GPL-3.0):
// https://github.com/AmazingBuff/HighlightLootableCorpses
#define PLUGIN_NAMESPACE Amazing::HighlightLootableCorpses
#define PLUGIN_NAMESPACE_BEGIN namespace PLUGIN_NAMESPACE {
#define PLUGIN_NAMESPACE_END }

namespace Plugin
{
	inline constexpr std::string_view Plugin_Author = "AmazingBuff";
}
