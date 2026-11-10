#pragma once

namespace ImGuiMCP
{
	struct ImDrawList;
	struct ImFont;
}

// Path of Exile style life and mana globes, with stamina as a curved bar on the life globe.
namespace Globes
{
	void Update();  // main thread: read the player's values, hide or show the game's bars
	void Draw(ImGuiMCP::ImDrawList* a_list, ImGuiMCP::ImFont* a_font, float a_width, float a_height, float a_delta);  // render thread
}
