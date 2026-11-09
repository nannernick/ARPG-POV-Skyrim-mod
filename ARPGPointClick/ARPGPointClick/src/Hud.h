#pragma once

// Everything drawn on screen: the cursor, the item labels and the settings page.
// Drawn through SKSE Menu Framework, which is required.
namespace Hud
{
	void Register();           // after kPostLoad
	bool Available();          // SKSE Menu Framework is installed
	bool FrameworkMenuOpen();  // its own menu is open (it has its own cursor)
}
