# Changelog

## 2.1.1

### Fixed
- **A click went past the spot under the cursor on stairs, rocks, ledges and slopes.** The click was put on a flat plane at your feet: on anything higher, the cursor's line of sight went past the surface and met that plane further on, so the ring and the walk landed beyond the cursor (up to ~225 px at 2560 wide), and the higher the surface the further off. A click now goes to the first surface under the cursor, found with a physics ray along the cursor's own line of sight. It starts 200 units above your feet, so roofs hidden by the cutaway are not hit. Characters, invisible walls and the undersides of things are looked past. A wall, pillar or rock face sends you to the floor at its foot on your side, not behind it. The ring and the walk use the same point. Nothing usable on the ray, or further than *Max click distance*: the old flat-plane point, as before.
- **Held keys and a click-move carried across a load door.** Every load screen (a door as well as a save) now stops the click-move, drops the marker, and forgets every key it thought was held (WASD, rotate, attack in place, power attack, labels). A key let go during the load screen used to stay "held", which blocks click-to-move (WASD) or freezes the cursor (rotate).
- **The interior was set up before the new cell had loaded.** The cutaway, fog and sky now wait for the new cell to be attached and the load screen to be down; the old cell's changes are handed back meanwhile. The outlines read every model again when the load screen goes down.

### New settings

| Setting | Default | What it does |
|---|---|---|
| `[Debug] ClickRay` | 0 | Where each click lands, in `ARPGPOV.log`: cursor pixel, the ray, every hit with its collision layer and height, the point chosen and the rule that chose it, and that point back on screen |
| `[Debug] LoadState` | 0 | What is cleared at save loads, load screens and menus, and the input state at that moment, in `ARPGPOV.log` |

Both are also in F1 → Controls → Debug.

### Known limits
- With `OnAttack = 1` and True Directional Movement, your character faces the way you walk while attacking. Unchanged.
- Spell and arrow aiming (the red ring) still uses the flat plane at your feet.

## 2.1.0

### Changed defaults
- **Lock yaw is on by default** (`[Camera] LockYaw = 1`), so F2 turns the camera 90° out of the box. An INI you kept still has your old value.
- **First person has its own field of view: 90**, set once when you switch to first person and then left alone, so the console's `fov` and other mods can change it. Before, first person was held at the third-person `Fov` (50) every frame.
- **Attacking stops a click-move.** Any attack (a swing, drawing a bow, casting, with any button) cancels the click-move you are on, and a held move button stops following until you press it again.

### Fixed
- **Moving while shooting went at an angle** between where you clicked and where you aimed. The aim turned your character every frame, and True Directional Movement moves the character along its facing, so the walk bent toward the target. With `OnAttack = 1` the walk now keeps the heading; spells and arrows still go to the cursor.
- **The click ring could float or sink** on slopes, stairs and lower floors. It took the height of your feet; it now sits on the floor under the spot you clicked.
- **Your outline could stay off after a door.** A character whose model was read short several times in a row kept that answer for good; a door or a load now has every model read again, and your own model keeps being re-read while it is short.
- **Unticking Black interior fog did nothing until the next room.** The room's own fog colours now come back at once.

### Added
- **POErim rarity colours.** With POErim 2.1.0 or later (its DiabloLootAPI v2), labels and outlines use each item's rarity colour from POErim (normal, magic, rare, currency, corrupted), and its orbs always show as loot. Normal-rarity POErim items that carry an enchantment are no longer drawn magic blue. Without POErim 2.1.0 nothing changes.
- **Interior debug log** (`[Debug] Interior`): your outline's state once a second (drawn or why not), what happens to it at doors, and the fog in use indoors, for tracking down dark rooms and missing outlines.
- Menu: *First person field of view* (Camera), *When an attack starts* (Controls), *POErim* section (Labels and Loot), *Interior debug log* (Cutaway).

### New settings

| Setting | Default | What it does |
|---|---|---|
| `[Camera] FirstPersonFov` | 90 | First person field of view, set once on entering first person. Below 30 = held at `Fov` as before |
| `[Move] OnAttack` | 0 | When an attack starts: 0 = stop moving, 1 = keep moving where you clicked |
| `[Labels] UseRarityColor` | 1 | POErim 2.1.0+ only: rarity colours for labels and outlines, orbs always shown |
| `[Labels] CurrencyColor` | 0x000000 | Orb colour (0xRRGGBB); 0 = POErim's own |
| `[Debug] Interior` | 0 | Interior outline and fog diagnostics in `ARPGPOV.log` |

Changed default: `[Camera] LockYaw` 0 → 1.

### Known limits
- With `OnAttack = 1` and True Directional Movement, your character faces the way you walk while attacking (TDM moves the body along its facing). Shots are steered to the cursor; melee swings go the way you walk.
- Interior darkness is not fixed yet: 2.1 adds the log to find it (see the README's Known issues).
