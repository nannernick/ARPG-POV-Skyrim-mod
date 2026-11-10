# ARPG POV

Turns Skyrim into a Path of Exile style ARPG view, in one SKSE plugin:

- **Camera:** locked at a fixed top-down angle.
- **Cutaway:** indoors, the walls and roofs between the camera and you are cut away.
- **Point and click:** an on-screen cursor, click-to-move, click to attack, loot, talk and use doors.
- **Item labels:** loot shows its name, with a loot filter.
- **Life and mana globes:** Path of Exile style globes in the bottom corners, with stamina as a curved bar on the life globe. The game's bars are hidden while they're on.
- **Outlines:** a soft glow around enemies, NPCs, you and nearby loot.

Play with the mouse, with **WASD and a lock-on key** (True Directional Movement's target lock), or both. Tested on Skyrim **1.7.104**.

This mod replaces *Skyrim ARPG Style Camera Mod* and *ARPG Point and Click*. Uninstall both before installing it.

**Requires:** [SKSE](https://skse.silverlock.org/), [Address Library](https://www.nexusmods.com/skyrimspecialedition/mods/32444), [SKSE Menu Framework](https://www.nexusmods.com/skyrimspecialedition/mods/120352) (draws the cursor and labels, and gives the settings pages) and [True Directional Movement](https://www.nexusmods.com/skyrimspecialedition/mods/51614). Without SKSE Menu Framework the camera and cutaway still work; point and click stays off.

## Install
Download **ARPGPOV-mod** from the latest green run in **Actions**, drag it into Vortex, and launch through SKSE.

Coming from the two older mods: your settings don't carry over by themselves. Copy any `[GameSettings]`, `[Cells]` and `[Player]` lines you added from `SkyrimARPGCamera.ini` into `ARPGPOV.ini`.

## Suggested modpack
- **Base:** [SKSE](https://skse.silverlock.org/), [Address Library](https://www.nexusmods.com/skyrimspecialedition/mods/32444), [SkyUI](https://www.nexusmods.com/skyrimspecialedition/mods/12604), [MCM Helper](https://www.nexusmods.com/skyrimspecialedition/mods/53000), Pandora Behaviour Engine
- **Camera:** this mod, [SKSE Menu Framework](https://www.nexusmods.com/skyrimspecialedition/mods/120352), [True Directional Movement](https://www.nexusmods.com/skyrimspecialedition/mods/51614), [Some Collision Camera](https://www.nexusmods.com/skyrimspecialedition/mods/57864), [Motion Sensitive Fix](https://www.nexusmods.com/skyrimspecialedition/mods/35833)
- **Combat:** [Precision](https://www.nexusmods.com/skyrimspecialedition/mods/72347)
- **Skills and spells:** Ordinator, Apocalypse, [Summermyst](https://www.nexusmods.com/skyrimspecialedition/mods/6285)
- **Loot:** [Grid Inventory - DiabloRim](https://www.nexusmods.com/skyrimspecialedition/mods/190634), [Socketing](https://www.nexusmods.com/skyrimspecialedition/mods/195740)

Run Pandora, then launch with `skse64_loader.exe`.

## Camera mod settings required
**Camera distance and view:** built in (it replaces 3PCO, which is no longer needed). Defaults: distance 600, field of view 90, no shoulder offsets, mouse-wheel zoom off. Change them under Camera in the menu or `[Camera]` in the INI. If you keep 3PCO installed, set `Manage = 0` so the two don't fight.

**True Directional Movement:** directional movement on, Auto Adjust Camera off, target lock pitch/yaw adjust speed at minimum. Reset Camera with Target Lock on, Reset Camera Pitch off. Lock on before using magic or bows.

## Controls
- **Left click** the ground: move there. Hold it to keep following the cursor.
- **Left click** an enemy: turn to it and attack. **Shift + left click** attacks toward the cursor without moving.
- **Left click** an item or its label: pick it up. A door: go through it. An NPC: talk. A chest or corpse: loot it. If it's out of reach, you walk over first.
- **Left click never triggers the game's own left-click action** while the cursor is live, whichever hand it's bound to. It must stay bound to an attack (left or right) for the plugin to see it.
- **Hold attack** to keep attacking: one swing after another while your attack button is held (melee). Hold **Left Alt** with it for a power attack.
- **Spells and bows** aim at the cursor, or at the enemy under it, not along the tilted camera.
- **Auto-attack (optional, off by default):** while True Directional Movement's target lock is on, one press of attack keeps attacking until you press it again.
- **First person:** the camera lock and the cursor switch off; the view is levelled once on entering.
- **Middle mouse (mouse 3)** does the same as a left click on things, and never attacks or moves.
- **WASD** still works and cancels a click-move.
- **F7:** point and click on/off (off = normal WASD and mouse look).

The cursor shows what a click will do: a **red sword** attacks, a **spilling bag of gold** loots, an **exclamation mark** talks (green) or goes through a door (white), and the white arrow moves. The click point is the sword's tip, or the small pointer beside the bag and the exclamation mark.

The cursor's click point is on the floor level you're standing on, not on physics, so the roofs the cutaway hides can't block a click. Movement is a straight line toward the click and stops if you run into a wall. The mouse moves the cursor instead of the camera; your lock-on key still works.

## Camera locker
Locks the camera's pitch (and optionally yaw) every frame. Indoors, it keeps your room drawn when the camera sits above the ceiling.
- **F10:** on/off
- **Page Up / Page Down:** more top-down / flatter
- Pitch 30 = Path of Exile 2 / Diablo II, 50 = Path of Exile

<details>
<summary><b>Angle math</b></summary>

| Game | Pitch |
|---|---|
| Diablo II | 30 (2:1 floor tiles: arcsin(1 ÷ 2) = 30°) |
| Path of Exile | 50 (ring 210 × 160 px at 1080p: arcsin(160 ÷ 210) = 49.6°) |
| Path of Exile 2 | 30 (ring ~700 × 350 px: arcsin(350 ÷ 700) = 30°) |
| Diablo IV | not measured yet |

![Path of Exile ring measured at 1920×1080](images/poe1-measurement.jpg)

**Pitch = arcsin(height ÷ width)**

To measure any game, screenshot a circle on the ground (an AoE ring) near the center of the screen, then measure its width and height in pixels. ARPGs use a narrow lens from far away, so also lower 3PCO's FOV (~40–55) and raise the zoom.
</details>

## Camera splitter
Indoors, everything above a cut line is removed by the GPU. Walls end cleanly and roofs vanish, while floors, furniture and characters stay. The background goes black.
- **F9:** *Diablo style* (everything above the cut height is always removed, the default) or *Automatic* (experimental: never cut lower than Diablo style, sloping up away from the camera, and rising further when nothing blocks the view of you)
- The cut stays at one height per floor. Jumps don't move it, and changes ease in.
- The cut rises behind you, so far walls stay full height.
- Idea from machinegod's *Skyrim Camera Disocclusion System*. This is a separate implementation.

## Outlines
By default only nearby enemies (red) glow, plus whatever the cursor is on, at full strength. NPCs (green), you (white) and nearby items (their label colour) can be switched on too. These are true outlines that follow the model, limbs and weapons included.

- The glow is soft by default: darkened colours, part transparent, fainter with distance. Size, strength and brightness are sliders.
- Outlines show through walls. Loot that sits well above you (shelves, ledges, the floor upstairs) gets no outline, so it isn't drawn over the scene.
- Each outlined thing costs a little. If frames drop in a crowded area, lower the two outline distances.

The outline renderer is from [Highlight Lootable Corpses](https://github.com/AmazingBuff/HighlightLootableCorpses) by AmazingBuff. ENB and Community Shaders are untested with it.

## Item labels and loot filter
Loot near you shows its name. White = gear, blue = enchanted, gold = gold, keys, soul gems, gems and ore, green = potions, cyan = scrolls and spell tomes, red = taking it is stealing. Labels stack instead of overlapping.

Skyrim lets you pick up almost anything, so the filter decides what counts as loot. Always shown: weapons, armor, ammo, potions, scrolls, spell tomes, keys, soul gems, gold, lockpicks, gems, ore and ingots. Off by default, each with its own switch: food, ingredients, other books, clutter (kettles, buckets, plates...). Filtered items get no label and can't be clicked. `MinValue` hides anything worth less than a set amount.

## Settings
**In game:** open the SKSE Menu Framework menu (F1) and go to **ARPG POV**: pages for Camera, Cutaway, Player and Point and Click. Changes apply live. Click **Save to INI** to keep them.

**INI:** `Data/SKSE/Plugins/ARPGPOV.ini`. **F8** reloads it in game. Keys are DirectX scan codes (0x44 = F10). Mouse buttons: 0 = left, 1 = right, 2 = middle, 3+ = side buttons.

| Key | |
|---|---|
| F7 | Point and click on/off |
| F8 | Reload the INI |
| F9 | Cutaway style: Diablo style / Automatic (experimental) |
| F10 | Camera lock on/off |
| Page Up / Page Down | Camera more top-down / flatter |

<details>
<summary><b>[Camera]</b> camera locker</summary>

| Setting | Default | |
|---|---|---|
| `Enabled` | 1 | Lock on at game start |
| `LockPitch` / `Pitch` | 1 / 30 | Lock the angle, degrees looking down |
| `LockYaw` / `Yaw` | 0 / 45 | Lock the compass direction (0 = north) |
| `Preset` | Skyrim ARPG | Camera look by game: Custom, Skyrim ARPG, Diablo II, Path of Exile, Path of Exile 2, Diablo III, Diablo IV, Last Epoch, Grim Dawn, Torchlight II. Sets pitch, FOV and distance; only the pitch of Diablo II, Path of Exile and Path of Exile 2 was measured, the rest are estimates |
| `Manage` / `Distance` / `Fov` / `OffsetX` / `OffsetZ` / `WheelZoom` | 1 / 600 / 90 / 0 / 0 / 0 | Camera distance, field of view, shoulder offsets, mouse-wheel zoom (replaces 3PCO) |
| `FixInteriorRooms` | 1 | Draw your room when the camera is above the ceiling |
| `PauseOnTargetLock` | 0 | Let TDM hold the camera while locked on |
| `PitchMode` / `InvertPitch` | 0 / 0 | Leave at 0 unless the camera looks up |
| `ToggleKey` | 0x44 (F10) | Camera on/off |
| `PitchUpKey` / `PitchDownKey` / `PitchStep` | PgUp / PgDn / 5 | Change pitch in game |
| `ReloadKey` | 0x42 (F8) | Reload the INI |
| `DebugLog` | 0 | Log camera values once a second |
</details>

<details>
<summary><b>[Player]</b> and <b>[GameSettings]</b></summary>

| Setting | Default | |
|---|---|---|
| `CarryWeight` | 10000 | Applied on every load (0 = leave alone) |
| `SpeedMult` | 130 | Movement speed, 100 = vanilla (0 = leave alone) |
| `[GameSettings]` | `fMoveEncumEffect = 0`, `fMoveEncumEffectNoWeapon = 0` | Any engine setting (the names `setgs` uses), applied every launch |
</details>

<details>
<summary><b>[Cutaway]</b> and <b>[Cut]</b> camera splitter</summary>

| Setting | Default | |
|---|---|---|
| `Enabled` | 1 | Splitter on/off |
| `InteriorOnly` | 1 | Only split indoors |
| `Height` | 220 | Cut height above the floor (a human is ~128) |
| `Slope` | 0.3 | Automatic style only: the cut is at `Height` at the bottom of the screen and rises this fast away from the camera. Diablo style is always flat |
| `AutoLift` | 0 | Starting style: 0 = Diablo style, 1 = Automatic (experimental) |
| `StyleKey` | 0x43 (F9) | Switch style |
| `ClearDelay` / `LiftMax` | 0.75 / 600 | Automatic: seconds of clear view before it lifts, how far it lifts |
| `ReanchorDistance` / `ReanchorDelay` | 150 / 1 | How far and how long you must stand on a new floor before the cut moves |
| `HeightSmoothing` / `MaxHeightSpeed` | 0.6 / 800 | How gently height changes ease in |
| `AllGeometry` | 1 | Leave on (keeps props and characters visible) |
</details>

<details>
<summary><b>[Cells]</b>, <b>[Background]</b>, <b>[Debug]</b></summary>

| Setting | Default | |
|---|---|---|
| `[Cells] EditorID = 260` or `= off` | none | Per-interior cut height or off. Editor IDs show in the log. |
| `BlackFog` | 1 | Black interior fog while inside |
| `HideSky` | 1 | Hide the sky dome while inside |
| `[Debug] Cutaway` / `PointClick` | 0 / 0 | Write diagnostics to `ARPGPOV.log` |
</details>

<details>
<summary><b>[PointClick]</b>, <b>[Cursor]</b>, <b>[Move]</b>, <b>[Pickup]</b>, <b>[Labels]</b>, <b>[Outlines]</b></summary>

| Setting | Default | |
|---|---|---|
| `[PointClick] Enabled` / `ToggleKey` | 1 / 0x41 (F7) | On at start, on/off key |
| `[Cursor] Sensitivity` / `Size` | 1 / 1 | Cursor speed and size |
| `[Cursor] Ring` | 1 | Ring behind the sword, bag and exclamation mark, in the symbol's colour |
| `[Cursor] InMenus` | 1 | Use this cursor in the inventory, containers, barter and other menus, starting where it was |
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
| `[Labels] StackCount` | 5 | Stacks this big or bigger read `200x Gold`; smaller ones show the name only |
| `[Labels] ShowFood` / `ShowIngredients` / `ShowBooks` / `ShowClutter` | 0 / 0 / 0 / 0 | Loot filter switches |
| `[Globes] Enabled` / `HideVanillaBars` / `Size` / `Numbers` / `Animate` | 1 / 1 / 1 / 1 / 1 | Life and mana globes, hide the game's bars, globe size, numbers, moving liquid |
| `[Outlines] Enabled` / `Enemies` / `NPCs` / `Player` / `Items` | 1 / 1 / 0 / 1 / 0 | Outlines on, and per group (hovered things always glow) |
| `[Combat] HoldToAttack` / `PowerAttackKey` | 1 / 0x38 | Holding attack keeps swinging; with the key (Left Alt) held it's a power attack |
| `[Combat] AutoAttack` | 0 | While locked on, one press of attack starts auto-attack and the next stops it |
| `[Combat] AimAtCursor` | 1 | Spells and bows aim at the cursor or the hovered enemy |
| `[Labels] MaxBelow` | 60 | Items lower than this under your feet get no label and can't be clicked |
| `[Outlines] Thickness` / `Opacity` / `Brightness` / `Fade` | 3 / 0.6 / 0.7 / 1 | Glow size (1-8), glow strength, colour brightness (lower = darker), fainter with distance |
| `[Outlines] Radius` / `ItemRadius` | 1500 / 700 | How far away actors / items are outlined |
| `[Outlines] MaxAbove` | 120 | Items, containers and corpses higher than this above your feet get no outline or label |
| `[Outlines] BehindPlayer` | 1 | Things behind your character don't glow over it |
</details>

## Credits
- **AmazingBuff**: the outline renderer (`src/render`, `src/base`), from [Highlight Lootable Corpses](https://github.com/AmazingBuff/HighlightLootableCorpses), GPL-3.0.
- **machinegod**: the cutaway idea, from *Skyrim Camera Disocclusion System*. The cutaway here is a separate implementation.
- CommonLibSSE-NG contributors, the SKSE team, meh321 (Address Library), Thiago099 (SKSE Menu Framework), Ershin (True Directional Movement).

## License
GPL-3.0-or-later, as the plugin links CommonLibSSE-NG and includes GPL-3.0 code from Highlight Lootable Corpses.
