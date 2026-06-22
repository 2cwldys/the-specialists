# TS client HUD draw chain (recovered from client.dll assembly)

The exact per-frame HUD render path, reverse-engineered from `reference/client.dll`
(task #4). This is the ground truth for making our `cl_dll` HUD match the original
pixel-for-pixel. Re-derive with `scripts/ghidra/DecompAt.java` (decompiles a function
by address) — see the cont.53 ledger entry.

## The dispatch

- `HUD_Redraw` (export `0x1001aa90`) → `CHud::Redraw` `FUN_10021aa0(gHUD=0x100cf920, time, intermission)`.
- `CHud::Redraw` walks the HUD-element linked list; for each element it calls
  **`elem->vtable[+0xc](time)`** = the element's **`Draw`**. Element `m_iFlags` is at
  `elem+0x10` (bit 1 = HUD_ACTIVE/draw-when-alive, bit 2 = draw-in-intermission).

## The TS HUD singleton

- One global object **`0x100d43cc`** holds the whole TS HUD. Message handlers are thin
  forwarders (`mov ecx,0x100d43cc; call <real>`) registered in its `Init` (`0x10049b80`,
  which `HookUserMsg`s every message). Field offsets (from the handlers `0x1004b920+`):
  - `+0x10` m_iFlags · `+0x20` health (TSHealth byte) · `+0x21` armor · `+0x22` state ·
    `+0x24` cash (int) · `+0x744` active weapon slot · `+0x740` active position ·
    `+0x7b8` active weapon id/type · `+0x164` arsenal slot array (stride per (slot,pos)).
- Its **vtable = `0x1007cae4`**: `[0]0x1001acc0(dtor) [4]0x1004c9d0(Init?) [8]0x1004cb50(VidInit?)
  [0xc]0x1004d3f0(Draw) [0x10]0x1004ce10 [0x14]0x1004cd60 [0x18]0x1005ab70`.

## Draw `0x1004d3f0` — the dispatcher

Calls, in order (guarded by slow-motion check `*(DAT_100df50c+0xc)==normal`, dead/spec
checks `FUN_1004c250`/`FUN_1004c280`):
1. `FUN_1004e6f0` — **static crosshair** (always). 2×2 `FillRGBA` dots at screen centre
   (`DAT_1018a660/710`), colour from `cl_lasercolor` via `FUN_100582f0`.
2. `FUN_1004e030(this, dead?)` — **top weapon-select bar** (reference Image #1). Loops
   slots 1..5 (`DAT_1018a678`) × positions 0..0x10 (`DAT_1018a644`): slot panel
   `FillRGBA(x = slot*0x80-0x78, y=8, w=0x80, h=0x14, …, alpha=100 or 0xce)`; weapon
   name text `FUN_10021ce0(x, y, maxw=DAT_100d5908, str, r,g,b)`; weapon icon via
   `SPR_Set 0x100d5b38` + `SPR_Draw 0x100d5b44`. Empty-slot/no-ammo text uses red
   `0xc3,0x2d,0x2d`.
3. mode draw: `FUN_1004d700` (`+0x7b8`==0, no weapon) **or** `FUN_1004d7e0` (held weapon
   — the **status + ammo box**, see below).
4. `FUN_1004e9f0` — **dynamic crosshair** (spread bars, expand with `DAT_1018a6ec`),
   coloured by state (`DAT_1018a60c`: 1→red 0xc3,0x2d,0x2d) — **or** `FUN_1004d550`
   (bomb wires) when wires active (`this+0x36fc..fe`).

## `FUN_1004d7e0` — the status + ammo box (the right-side panel)

The reference Five-seveN/MP5K panel. Each line is its own `FillRGBA` strip + text:
- **Panel strips:** `FillRGBA(x=DAT_1018a67c, y=DAT_1018a700, w=0x80 (128px), h=DAT_1018a6f8-1,
  r=DAT_1018a648, g=DAT_1018a70c, b=DAT_1018a65c, alpha=0xf)` — **alpha 0x0f (15) ≈ 6%**,
  i.e. a very faint tint, NOT an opaque panel. (The weapon-bar slots use alpha 100/0xce.)
- **Colour:** `DAT_1018a648/70c/65c` = the `hud_r/g/b` colour (150/175/255). **Labels are
  drawn dimmed at HALF brightness** (`r/2,g/2,b/2` → 75/87/127); **values at full**.
- **Text:** `FUN_10021ce0(x+2, y, maxw, string, r,g,b)` — a left-inset (+2px) string draw
  (the TS bitmap font; `bitmapfonts 1`). Numbers via `FUN_1004c290(gHUD, x, y, digits, value, r,g,b)`.
- Line layout: name (slot+0x11 string), caliber (`&DAT_10095e7c` " " etc), fire-mode,
  then clip/reserve number — each a 0x80-wide strip stacked at `y=DAT_1018a700` advancing
  by the line height. Anchor `DAT_1018a67c`/`DAT_1018a700` are computed in the box-setup
  (right side); icons via SPR.

## Engine function-pointer globals (individual, MSVC style)

`0x100d5b54` FillRGBA · `0x100d5b38` SPR_Set · `0x100d5b44` SPR_Draw · `0x100d5b34`
SPR_Height · `0x100d5b74` ClientCmd · `0x100d5bf4` GetViewAngles/GetLocalPlayer-ish.
Text: `FUN_10021ce0` = a DrawHudString-equivalent; `FUN_1004c290` = DrawHudNumber-ish.

## Box anchor constants — RESOLVED (from the top of FUN_1004d7e0)

Raw pixels off the actual screen size (`DAT_100d5908`=ScreenWidth, `DAT_100d590c`=ScreenHeight):
- **x = ScreenWidth − 0x88 (136)**, box **width 0x80 (128)** → right-anchored, 8px from edge.
- Lines **stack upward** from **y = ScreenHeight − 8**; each line `y −= 0x14 (20)` then draws.
- **Line height = 0x14 (20px)**; strip height = lineH − 1 = 19.
- Strip = `FillRGBA(x, y, 128, 19, hud_r, hud_g, hud_b, alpha)`, **alpha = 0x0f** (0x2d when the
  attachment row is active). Descriptive lines (caliber/fire-mode) text at **half brightness**
  (`color/2`); name + ammo numbers at full.
- The 0..3 loop draws attachment rows (Silencer/Lasersight/Flashlight/Scope) gated by
  `param_1+0x798 & (1<<n)` — needs the WeaponInfo flags byte (server currently sends 0).
- **APPLIED** (cont.53) to `cl_dll/ts_ammo.cpp` — verified on-screen: Beretta 92F / 9 x 19mm /
  Semi Auto / 15 / 45, right-anchored, half-bright labels, faint strips.

## Health bar — RESOLVED + APPLIED (FUN_1004d5f0)

A fill **bar**, not text: `FillRGBA(x, y, 0x80, 0x13, hud_r,g,b, alpha 0x1e)` background +
`FillRGBA(x+2, y+2, fillW, 0x10, colour, …)` fill where fillW ∝ health (red when low,
`this+0x3700 <= threshold`). Drawn bottom-right, stacked **above** the weapon/ammo box
(both `FUN_1004d700` unarmed and `FUN_1004d7e0` armed call it at the running y). Applied
in `cl_dll/ts_hud.cpp::TSHud_HealthBar` + drawn by `CHudTSAmmo` above the box (cont.53).

## Weapon-select bar (top) — MAPPED, not yet implemented (the 3rd region)

`FUN_1004e030`: a grid at `this+0x164`, entry index `= (slot + pos*0xb) * 8` (8 bytes:
type + state), i.e. up to 11 select-slots × positions. Draws slots **1..5** (`DAT_1018a678`):
slot panel `FillRGBA(x = slot*0x80 - 0x78, y = 8, w = 0x80, h = 0x14, …, alpha 100 / 0xce)`,
weapon name text + icon (SPR_Set `0x100d5b38` / SPR_Draw `0x100d5b44`), active slot
highlighted; empty/no-ammo in red `0xc3,0x2d,0x2d`. Position pitch `DAT_1018a668 = 0x30`.

**Populated by the `WStatus` message** (server `CTSGun::UpdateClientData`, per arsenal
slot, on change): `WStatus(3) = BYTE slotIndex (0x25..0 = weapon-type index), BYTE clip
(slot+4), BYTE caliber (slot+8)`. The client maps weapon-type → (select-slot, position)
via a static table (in client.dll, TODO to recover) to fill the grid.

**To implement (the next chunk):** (1) server — send `WStatus` from our `CTSGun` for each
held arsenal weapon (slotIndex = tsweapon_t, clip, caliber); (2) client `CHudTSWeaponBar`
— read WStatus into a type→owned map, draw the top bar per the geometry above; (3) the
type→select-slot/position mapping + weapon icons (SPR). Our arsenal is a flat
`m_arsenalType[]`, so map each to its category slot.

## STILL TODO (to pixel-match)
- The **TS bitmap font** (`FUN_10021ce0` + `bitmapfonts` cvar) — all text currently uses
  the console font, so glyphs differ. Biggest remaining cross-element visual gap.
- The **weapon-select bar** (above) — server WStatus + client grid + icons.
- The **left status block** armor/cash exact placement (health is done as the bar).
- The WeaponInfo **flags byte** server-side (attachments) so the Silencer/Lasersight rows draw.
- The **bitmap font** `FUN_10021ce0` (it's not the console font — `bitmapfonts` cvar);
  decide whether to approximate with DrawConsoleString or load the TS font sprites.
- Apply: panel width 0x80, alpha 0x0f, half-bright labels / full-bright values, the
  steel-blue colour (already applied), right-side anchor.
