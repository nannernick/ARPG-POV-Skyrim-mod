# ARPG Point and Click

Path of Exile style mouse controls for Skyrim, made for the [Skyrim ARPG Style Camera Mod](https://github.com/nannernick/Skyrim-ARPG-Style-Camera-Mod). You get an on-screen cursor, click-to-move, click pickup and item name labels.

**Requires:** [SKSE](https://skse.silverlock.org/), [Address Library](https://www.nexusmods.com/skyrimspecialedition/mods/32444), [SKSE Menu Framework](https://www.nexusmods.com/skyrimspecialedition/mods/120352) (draws the cursor and labels) and [True Directional Movement](https://www.nexusmods.com/skyrimspecialedition/mods/51614). Built for Skyrim **1.7.104**.

## Controls
- **Left click** the ground: move there. Hold it to keep following the cursor.
- **Left click** an enemy: turn to it and attack. **Shift + left click** attacks toward the cursor without moving.
- **Middle mouse (mouse 3)** on an item or its label: pick it up. If it's out of reach, you walk over first. It also opens containers, loots corpses and talks to NPCs.
- **WASD** still works and cancels a click-move.
- **F7:** point and click on/off (off = normal WASD and mouse look).

## Item labels
Every loose item near you shows its name. White = normal, blue = enchanted, gold = gold, keys and soul gems, green = potions and ingredients, cyan = books and scrolls, red = taking it is stealing. Labels stack instead of overlapping. `MinValue` hides cheap clutter.

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
| `[Cursor] BlockMouseLook` / `RotateKey` | 1 / 0 | Mouse moves the cursor, hold a key to rotate the camera |
| `[Move] Button` / `HoldToMove` | 0 (left) / 1 | Move button, follow the cursor while held |
| `[Move] ArriveRadius` / `MaxDistance` | 40 / 3000 | Stop distance, furthest one click sends you |
| `[Move] ClickEnemies` / `AttackInPlaceKey` | 1 / 0x2A (Shift) | Click enemies to attack, attack-in-place key |
| `[Move] Mode` | 0 | 0 = auto, 1 = TDM, 2 = turn the player (no TDM) |
| `[Pickup] Button` / `Range` | 2 (middle) / 150 | Pickup button, reach before you walk |
| `[Pickup] LeftClickPickup` / `Containers` / `Actors` | 0 / 1 / 1 | Left click also picks up, pickup button opens chests / loots and talks |
| `[Labels] Mode` / `Key` | 1 / 0x38 (Alt) | 0 = hovered only, 1 = always, 2 = while the key is held |
| `[Labels] Radius` / `MaxHeightDiff` | 1500 / 300 | How far away, and how far above/below, items get labels |
| `[Labels] Max` / `Scale` / `MinValue` | 60 / 1 / 0 | Label cap, text size, loot filter |

Mouse buttons: 0 = left, 1 = right, 2 = middle, 3+ = side buttons. Keys are DirectX scan codes.

## Credits
CommonLibSSE-NG contributors, the SKSE team, meh321 (Address Library), Thiago099 (SKSE Menu Framework), Ershin (True Directional Movement).

## License
GPL-3.0-or-later, as the plugin links CommonLibSSE-NG.
