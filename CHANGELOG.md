# Changelog

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
