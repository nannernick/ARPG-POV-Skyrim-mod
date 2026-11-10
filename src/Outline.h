#pragma once

// True silhouette outlines. The renderer itself (src/render) is from Highlight Lootable
// Corpses by AmazingBuff; this file feeds it this mod's targets and colours.
namespace Outline
{
	void Install();  // after kDataLoaded
	bool Active();   // the renderer is hooked in and working (otherwise the HUD draws its simple shapes)
}
