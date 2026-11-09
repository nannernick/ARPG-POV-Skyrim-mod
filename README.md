# ARPG Point and Click

Path of Exile style mouse controls for Skyrim, made for the [Skyrim ARPG Style Camera Mod](https://github.com/nannernick/Skyrim-ARPG-Style-Camera-Mod). You get an on-screen cursor, click-to-move, click to loot, talk and use doors, outlines, and item name labels with a loot filter.

**Requires:** [SKSE](https://skse.silverlock.org/), [Address Library](https://www.nexusmods.com/skyrimspecialedition/mods/32444), [SKSE Menu Framework](https://www.nexusmods.com/skyrimspecialedition/mods/120352) (draws the cursor and labels) and [True Directional Movement](https://www.nexusmods.com/skyrimspecialedition/mods/51614). Built for Skyrim **1.7.104**.

## Controls
- **Left click** the ground: move there. Hold it to keep following the cursor.
- **Left click** an enemy: turn to it and attack. **Shift + left click** attacks toward the cursor without moving.
- **Left click** an item or its label: pick it up. A door: go through it. An NPC: talk. A chest or corpse: loot it. If it's out of reach, you walk over first.
- **Middle mouse (mouse 3)** does the same as a left click on things, and never attacks or moves.
- **WASD** still works and cancels a click-move.
- **F7:** point and click on/off (off = normal WASD and mouse look).

The cursor shows what a click will do: a **red sword** attacks, a **spilling bag of gold** loots, an **exclamation mark** talks (green) or goes through a door (white), and the white arrow moves. The click point is the sword's tip, or the small pointer beside the bag and the exclamation mark.

## Outlines
Enemies get a thin red outline, you and NPCs white, items the colour of their label. Whatever the cursor is on is outlined a little thicker. The outline is a line around the shape of the body or object, not a pixel-exact silhouette.

## Item labels and loot filter
Loot near you shows its name. White = gear, blue = enchanted, gold = gold, keys, soul gems, gems and ore, green = potions, cyan = scrolls and spell tomes, red = taking it is stealing. Labels stack instead of overlapping.

Skyrim lets you pick up almost anything, so the filter decides what counts as loot. Always shown: weapons, armor, ammo, potions, scrolls, spell tomes, keys, soul gems, gold, lockpicks, gems, ore and ingots. Off by default, each with its own switch: food, ingredients, other books, clutter (kettles, buckets, plates...). Filtered items get no label and can't be clicked. `MinValue` hides anything worth less than a set amount.

## With the camera mod
- The mouse moves the cursor instead of the camera, so the view stays put with `LockYaw` on or off. Your lock-on key still works.
- Clicks land on the floor level you're standing on, not on physics, so the roofs the cutaway hides can't block them.
- Movement is a straight line toward the click. It stops if you run into a wall.

## Install
Download **ARPGPointClick-mod** from the latest green run in **Actions**, drag it into Vortex, and launch through SKSE.

## Settings
**In game:** SKSE Menu Framework menu (F1) > **ARPG Point and Click**. **INI:** `Data/SKSE/Plugins/ARPGPointClick.ini` (buttons and keys are set here).

| Setting | Default | |
|---|---|---|
| `[General] Enabled` / `ToggleKey` | 1 / 0x41 (F7) | On at start, on/off key |
| `[Cursor] Sensitivity` / `Size` | 1 / 1 | Cursor speed and size |
| `[Cursor] Ring` | 1 | Ring behind the sword, bag and exclamation mark, in the symbol's colour |
| `[Cursor] BlockMouseLook` / `RotateKey` | 1 / 0 | Mouse moves the cursor, hold a key to rotate the camera |
| `[Move] Button` / `HoldToMove` | 0 (left) / 1 | Move button, follow the cursor while held |
| `[Move] ArriveRadius` / `MaxDistance` | 40 / 3000 | Stop distance, furthest one click sends you |
| `[Move] ClickEnemies` / `ClickInteract` / `AttackInPlaceKey` | 1 / 1 / 0x2A (Shift) | Click enemies to attack, click things to use them, attack-in-place key |
| `[Move] Mode` | 0 | 0 = auto, 1 = TDM, 2 = turn the player (no TDM) |
| `[Pickup] Button` / `Range` | 2 (middle) / 150 | Extra use button, reach before you walk |
| `[Pickup] Containers` / `Actors` | 1 / 1 | Chests and barrels / corpses and NPCs can be clicked |
| `[Labels] Mode` / `Key` | 1 / 0x38 (Alt) | 0 = hovered only, 1 = always, 2 = while the key is held |
| `[Labels] Radius` / `MaxHeightDiff` | 1500 / 300 | How far away, and how far above/below, items get labels |
| `[Labels] Max` / `Scale` / `MinValue` | 60 / 1 / 0 | Label cap, text size, minimum value |
| `[Labels] ShowFood` / `ShowIngredients` / `ShowBooks` / `ShowClutter` | 0 / 0 / 0 / 0 | Loot filter switches |
| `[Outlines] Enabled` / `Enemies` / `NPCs` / `Items` | 1 / 1 / 1 / 1 | Outlines on, and per group |
| `[Outlines] Thickness` / `Radius` | 1.5 / 4000 | Line thickness (pixels), how far away actors are outlined |

Mouse buttons: 0 = left, 1 = right, 2 = middle, 3+ = side buttons. Keys are DirectX scan codes.

## Credits
CommonLibSSE-NG contributors, the SKSE team, meh321 (Address Library), Thiago099 (SKSE Menu Framework), Ershin (True Directional Movement).

## License
GPL-3.0-or-later, as the plugin links CommonLibSSE-NG.
