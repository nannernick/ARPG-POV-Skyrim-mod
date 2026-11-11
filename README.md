# ARPG POV

Turns Skyrim into a Path of Exile style ARPG view, in one SKSE plugin:

- **Camera:** locked at a fixed top-down angle.
- **Cutaway:** indoors, the walls and roofs between the camera and you are cut away.
- **Point and click:** an on-screen cursor, click-to-move, click to attack, loot, talk and use doors.
- **Item labels:** loot shows its name, with a loot filter.
- **Life and mana globes:** Path of Exile style globes in the bottom corners, with stamina as a curved bar on the life globe. The game's bars are hidden while they're on.
- **Outlines:** a soft glow around enemies, NPCs, followers, you and nearby loot, with optional ground circles under characters.

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
**Camera distance and view:** built in (it replaces 3PCO, which is no longer needed). Defaults: the Path of Exile 2 preset (pitch 30, distance 1290, field of view 50), no shoulder offsets, mouse-wheel zoom off. An INI you already have keeps its own preset and numbers; the old look is the *Skyrim ARPG* preset (distance 600, field of view 90). Change them under Camera in the menu or `[Camera]` in the INI. If you keep 3PCO installed, set `Manage = 0` so the two don't fight.

**True Directional Movement:** directional movement on, Auto Adjust Camera off, target lock pitch/yaw adjust speed at minimum, Reset Camera with Target Lock off, Reset Camera Pitch off. Lock on before using magic or bows.

TDM turns the camera by itself: toward a locked target, and behind your character when you press lock-on with nothing to lock (its camera reset). `KeepHeading = 1` (the default) puts the heading back every frame while the mouse isn't turning the camera, which is always the case with the cursor live or a lock on. With point and click off and no lock, the mouse owns the camera and TDM's reset still goes through, so switch Reset Camera with Target Lock off in TDM as well. Needs testing in game.

## Controls
- **Left click** the ground: move there. Hold it to keep following the cursor.
- **Left click** an enemy: turn to it and attack. **Shift + left click** attacks toward the cursor without moving.
- **Left click** an item or its label: pick it up. A door: go through it. An NPC: talk. A chest or corpse: loot it. A seat, lever or other named furniture or activator (the ferry seat): use it. If it's out of reach, you walk over first.
- **Left click never triggers the game's own left-click action** while the cursor is live, whichever hand it's bound to. It must stay bound to an attack (left or right) for the plugin to see it.
- **Hold attack** to keep attacking: one swing after another while your attack button is held (melee). Hold **Left Alt** with it for a power attack.
- **Spells and bows** aim at the cursor, or at the enemy under it, not along the tilted camera.
- **Auto-attack (optional, off by default):** while True Directional Movement's target lock is on, one press of attack keeps attacking until you press it again.
- **First person:** the camera lock and the cursor switch off; the view is levelled once on entering.
- **Middle mouse (mouse 3)** does the same as a left click on things, and never attacks or moves.
- **WASD** still works and cancels a click-move.
- **F7:** point and click on/off (off = normal WASD and mouse look).
- **Insert:** turn the camera 90 degrees. Only works with **Lock yaw** on (`LockYaw = 1`); with it off the key shows a message and does nothing.

The cursor shows what a click will do: a **red sword** attacks, a **spilling bag of gold** loots, an **exclamation mark** talks (green) or uses a door, seat or lever (white), and the white arrow moves. The click point is the sword's tip, or the small pointer beside the bag and the exclamation mark.

The cursor's click point is on the floor level you're standing on, not on physics, so the roofs the cutaway hides can't block a click. Movement is a straight line toward the click and stops if you run into a wall. The mouse moves the cursor instead of the camera; your lock-on key still works.

## Camera locker
Locks the camera's pitch (and optionally yaw) every frame. Indoors, it keeps your room drawn when the camera sits above the ceiling.
- **F10:** on/off
- **Page Up / Page Down:** more top-down / flatter
- **Insert:** with Lock yaw on, turn the locked heading 90 degrees
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
By default nearby enemies (red), your followers (blue) and you (white) glow, plus whatever the cursor is on, at full strength. NPCs (green) and loot can be switched on too. These are true outlines that follow the model, limbs and weapons included.

- **Followers** are anyone the game counts as your teammate. They have their own switch and their own colour (`FollowerColor`, a colour picker in the menu), separate from other friendly NPCs.
- **Loot** is one switch for dropped items (their label colour), containers and corpses (gold). It was called *Items* before 1.6 and only covered dropped items; an old INI's `Items` line still works. With it off they glow only under the cursor. A weapon or shield an NPC is holding is part of that NPC's model, so it is outlined with the NPC and can't have a switch of its own.
- **Ground circles** (off by default): a ring on the ground under enemies, NPCs and followers, in the outline's colour. It uses the same Enemies / NPCs / Followers switches and works with outlines off, so it can replace them or go with them. It is drawn with the labels, over the picture, so it shows through walls and over the character's feet.
- **Other levels:** characters more than `[Labels] MaxHeightDiff` above or below you get no outline at all, indoors and out. Characters more than `[Outlines] MaxBelow` (150) below your feet glow only under the cursor, which keeps things under the floor from lighting up. Raise it if enemies downhill lose their outline. Merchant chests are left out by what they are (each vendor's own chest), not by height.

- The glow is soft by default: darkened colours, part transparent, fainter with distance. Size, strength and brightness are sliders.
- Outlines show through walls. Loot that sits well above you (shelves, ledges, the floor upstairs) gets no outline, so it isn't drawn over the scene.
- Each outlined thing costs a little. If frames drop in a crowded area, lower the two outline distances.

The outline renderer is from [Highlight Lootable Corpses](https://github.com/AmazingBuff/HighlightLootableCorpses) by AmazingBuff. ENB and Community Shaders are untested with it.

## Item labels and loot filter
Loot near you shows its name. White = gear, blue = enchanted, gold = gold, keys, soul gems, gems and ore, green = potions, cyan = scrolls and spell tomes, red = taking it is stealing. Labels stack instead of overlapping.

Skyrim lets you pick up almost anything, so the filter decides what counts as loot. Always shown: weapons, armor, ammo, potions, scrolls, spell tomes, keys, soul gems, gold, lockpicks, gems, ore and ingots. Off by default, each with its own switch: food, ingredients, other books, clutter (kettles, buckets, plates...). Filtered items get no label and can't be clicked. `MinValue` hides anything worth less than a set amount.

## Settings
**In game:** open the SKSE Menu Framework menu (F1) and go to **ARPG POV**: pages for Camera, Cutaway, Controls, Outlines, Labels and Loot, HUD and Player. Changes apply live. Click **Save to INI** to keep them.

**INI:** `Data/SKSE/Plugins/ARPGPOV.ini`. The defaults in the tables below are the values in the INI that ships with the mod. **F8** reloads it in game. Keys are DirectX scan codes (0x44 = F10). Mouse buttons: 0 = left, 1 = right, 2 = middle, 3+ = side buttons.

| Key | |
|---|---|
| F7 | Point and click on/off |
| F8 | Reload the INI |
| F9 | Cutaway style: Diablo style / Automatic (experimental) |
| F10 | Camera lock on/off |
| Page Up / Page Down | Camera more top-down / flatter |
| Insert | Turn the camera 90 degrees (Lock yaw only) |

<details>
<summary><b>[Camera]</b> camera locker</summary>

| Setting | Default | |
|---|---|---|
| `Enabled` | 1 | Lock on at game start |
| `LockPitch` / `Pitch` | 1 / 30 | Lock the angle, degrees looking down |
| `LockYaw` / `Yaw` | 0 / 45 | Lock the compass direction (0 = north) |
| `Preset` | Path of Exile 2 | Camera look by game: Custom, Skyrim ARPG, Diablo II, Path of Exile, Path of Exile 2, Diablo III, Diablo IV, Last Epoch, Grim Dawn, Torchlight II. Sets pitch, FOV and distance; only the pitch of Diablo II, Path of Exile and Path of Exile 2 was measured, the rest are estimates |
| `Manage` / `Distance` / `Fov` / `OffsetX` / `OffsetZ` / `WheelZoom` | 1 / 1290 / 50 / 0 / 0 / 0 | Camera distance, field of view, shoulder offsets, mouse-wheel zoom (replaces 3PCO) |
| `FixInteriorRooms` | 1 | Draw your room when the camera is above the ceiling |
| `PauseOnTargetLock` | 0 | Let TDM hold the camera while locked on |
| `KeepHeading` | 1 | Keep the camera's compass heading when TDM turns it (target lock, its camera reset, its auto-adjust), while the mouse isn't turning the camera |
| `PitchMode` / `InvertPitch` | 0 / 0 | Leave at 0 unless the camera looks up |
| `ToggleKey` | 0x44 (F10) | Camera on/off |
| `PitchUpKey` / `PitchDownKey` / `PitchStep` | 0xC9 (PgUp) / 0xD1 (PgDn) / 5 | Change pitch in game |
| `YawStepKey` | 0xD2 (Insert) | Turn the camera 90 degrees per press. Only with `LockYaw = 1` |
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
| `Height` | 200 | Cut height above the floor (a human is ~128) |
| `Slope` | 0.3 | Automatic style only: the cut is at `Height` at the bottom of the screen and rises this fast away from the camera. Diablo style is always flat |
| `AutoLift` | 0 | Starting style: 0 = Diablo style, 1 = Automatic (experimental) |
| `StyleKey` | 0x43 (F9) | Switch style |
| `ClearDelay` / `LiftMax` | 0.75 / 1600 | Automatic: seconds of clear view before it lifts, how far it lifts |
| `ReanchorDistance` / `ReanchorDelay` | 75 / 0.5 | How far and how long you must stand on a new floor before the cut moves |
| `HeightSmoothing` / `MaxHeightSpeed` | 0.2 / 1600 | How gently height changes ease in |
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
| `[Pickup] Containers` / `Actors` / `Activators` | 1 / 1 / 1 | Chests and barrels / corpses and NPCs / named furniture and activators (seats, levers) can be clicked |
| `[Labels] Mode` / `Key` | 1 / 0x38 (Alt) | 0 = hovered only, 1 = always, 2 = while the key is held |
| `[Labels] Radius` / `MaxHeightDiff` | 2000 / 300 | How far away, and how far above/below, items get labels |
| `[Labels] Max` / `Scale` / `MinValue` | 100 / 0.6 / 15 | Label cap, text size, minimum value |
| `[Labels] StackCount` | 5 | Stacks this big or bigger read `200x Gold`; smaller ones show the name only |
| `[Labels] ShowFood` / `ShowIngredients` / `ShowBooks` / `ShowClutter` | 0 / 0 / 0 / 0 | Loot filter switches |
| `[Globes] Enabled` / `HideVanillaBars` / `Size` / `Numbers` / `Animate` | 1 / 1 / 1 / 1 / 1 | Life and mana globes, hide the game's bars, globe size, numbers, moving liquid |
| `[Outlines] Enabled` / `Enemies` / `NPCs` / `Player` / `Loot` | 1 / 1 / 0 / 1 / 0 | Outlines on, and per group (hovered things always glow). `Loot` = dropped items, containers and corpses (`Items` before 1.6) |
| `[Outlines] Followers` / `FollowerColor` | 1 / 0x50A0FF | Followers' own switch and colour (0xRRGGBB) |
| `[Outlines] Circles` / `CircleRadius` | 0 / 45 | Ground circles under enemies, NPCs and followers, and their size |
| `[Combat] HoldToAttack` / `PowerAttackKey` | 1 / 0x38 | Holding attack keeps swinging; with the key (Left Alt) held it's a power attack |
| `[Combat] AutoAttack` | 0 | While locked on, one press of attack starts auto-attack and the next stops it |
| `[Combat] AimAtCursor` | 1 | Spells and bows aim at the cursor or the hovered enemy |
| `[Labels] MaxBelow` | 60 | Items lower than this under your feet get no label and can't be clicked |
| `[Outlines] Thickness` / `Opacity` / `Brightness` / `Fade` | 5 / 0.9 / 0.9 / 1 | Glow size (1-8), glow strength, colour brightness (lower = darker), fainter with distance |
| `[Outlines] Radius` / `ItemRadius` | 1500 / 700 | How far away actors / items are outlined |
| `[Outlines] MaxParts` | 256 | Most model parts outlined in one frame (64-1024). Armoured crowds can reach it; higher outlines more of them and costs frames |
| `[Outlines] MaxAbove` | 120 | Items, containers and corpses higher than this above your feet get no outline or label |
| `[Outlines] MaxBelow` | 150 | Enemies, NPCs and followers lower than this under your feet get no outline or circle unless the cursor is on them |
| `[Outlines] BehindPlayer` | 1 | Things behind your character don't glow over it |
</details>

## Known issues
- **Interiors can crash** (seen in a cave, and going downstairs with the cutaway on). The Automatic cutaway style is suspected. A Crash Logger SSE log is needed to find it.
- **Your own outline can flip in front of / behind your character** while moving indoors.
- **Outlines can jump between characters in crowds**, most with armoured ones. 1.6 changes how the outlined set is picked and adds `[Outlines] MaxParts`; whether that cures it needs testing.
- **The pitch keys changed code in 1.6** to the real Page Up / Page Down (0xC9 / 0xD1). Before, the INI held 0x49 / 0x51, which are Numpad 9 / Numpad 3. An INI you kept still has the old codes.
- **Cobwebs and other ceiling-level props** can be missing under the cut.
- **Spells can still hit the ground** if True Directional Movement steers the projectile.
- **Path of Exile 2 is the default camera from 1.6.** The cut heights were tuned at the old distance (600) and may need retuning at 1290.
- **Furniture and activators are clickable from 1.6.** In a room full of chairs a click meant for the floor can land on one; `[Pickup] Activators = 0` switches it off.

## Credits
- **AmazingBuff**: the outline renderer (`src/render`, `src/base`), from [Highlight Lootable Corpses](https://github.com/AmazingBuff/HighlightLootableCorpses), GPL-3.0.
- **machinegod**: the cutaway idea, from *Skyrim Camera Disocclusion System*. The cutaway here is a separate implementation.
- CommonLibSSE-NG contributors, the SKSE team, meh321 (Address Library), Thiago099 (SKSE Menu Framework), Ershin (True Directional Movement).

## License
GPL-3.0-or-later, as the plugin links CommonLibSSE-NG and includes GPL-3.0 code from Highlight Lootable Corpses.
