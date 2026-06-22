# TS buy/config menu layout — RE findings (cont.69c)

Recovered from `client.dll` (.data menu strings + the builder functions). This is the
authoritative menu hierarchy; it supersedes the guessed placement of any single entry.

## Mechanism (important — corrected)

The TS buy menu is a **client-side, custom-drawn TEXT menu**, NOT VGUI and NOT the stock
`gmsgShowMenu`/`CHudMenu`.

- The client builds and draws each menu entry itself: `client.dll FUN_10011160` (config-menu
  item 1) and the sibling builders call **`FUN_10066a90(panel, idx, text, x, y, …, scale, …)`**
  — a positioned-text "button" draw. The menu strings live in `client.dll .data`.
- Selection is by **number key**; the handler sends the buy commands (`tkw`/`tki`/`tka`/
  `rst_cfg`, etc. — see `reference/BUY.md`) and/or navigates to a submenu.
- It is **not** `gmsgVGUIMenu` (that's the MOTD-style image-button panel) and **not** the
  generic `gmsgShowMenu` text menu. Our current server-side `gmsgShowMenu` config menu
  (`CBasePlayer::TSSendBuyMenu`) is a **stand-in** that mimics the top level only.

## Hierarchy

```
Config menu  (the menu shown in spectate/buy)
  1  Use New Config | Use Last Config | Empty Config   (item 1 cycles by state;
                                                         "Empty" when nothing queued)
  2  Buy            ──► Buy (weapon category) menu
  3  Remove Item
  4  Reset Config
  5  Save Config
  6  Load Config
  7  Buy Random
  0  Cancel

Buy (weapon category) menu   (from "2 Buy")
  1  Handguns       ──► per-category weapon list
  2  SMGs           ──►
  3  Rifles         ──►
  4  Shotguns       ──►
  5  Other          ──►
  6  Player Accessory ──► Player Accessory menu
  0  Cancel

Player Accessory menu   (from "6 Player Accessory")  ◄── KEVLAR LIVES HERE
  Kevlar             (toggle; shown selected when the per-row accessory flag bit 0x10 is set;
                      client cvar `cl_buykevlar`)
  Lasersight         ("%i Lasersight  %i"; cvars `cl_laser` / `cl_lasercolor`)
  Silencer           ("%i Silencer  %i")
  …

per-category weapon list (e.g. Handguns)
  <weapons in the category>
  6  Buy Akimbo      (handguns: upgrade to akimbo)
  6  Full Load       (buy the weapon with full ammo)
  0  Cancel
```

Recovered `.data` entry strings (client.dll): `1 Use New Config` / `1 Use Last Config` /
`1 Empty Config`, `2 Buy`, `3 Remove Item`, `4 Reset Config`, `5 Save Config`, `6 Load Config`,
`7 Buy Random`, `0 Cancel`; `1 Handguns`, `2 SMGs`, `3 Rifles`, `4 Shotguns`, `5 Other`,
`6 Player Accessory`; `Kevlar`, `%i Lasersight  %i`, `%i Silencer  %i`, `6 Buy Akimbo`,
`6 Full Load`. Accessory selection state read from a per-row flag (`…0xb0(panel,row*…)`,
bit 0x10 = kevlar). Builders/handlers cluster in `client.dll` ~`0x10011160`–`0x10011d00`
(text draw via `FUN_10066a90`).

## Status vs. our reconstruction

- The **config-menu top level** matches our stand-in `TSSendBuyMenu` (cont.66) — faithful enough.
- The **"2 Buy" weapon-category submenus** and the **"6 Player Accessory"** submenu are NOT built.
  Kevlar belongs in **Buy → Player Accessory → Kevlar** (NOT a top-level entry — an earlier
  invented "Buy Kevlar" top-level entry was reverted).
- The kevlar **gameplay** is reconstructed and faithful (`CBasePlayer::TakeArmor` / the
  `armor_absorb` damage reduction in `TakeDamage` / `AddItem`; `reference/BUY.md`). What's
  deferred is the faithful **client menu UI** that lets you select it (the whole custom client
  text-menu system) — a Track B (cl_dll) reconstruction.

## Remaining RE (for the full menu reconstruction)

- The exact **per-category weapon lists** (which `tsweapon_t` appear under Handguns/SMGs/Rifles/
  Shotguns/Other) and their order.
- The exact **selection action** each entry sends (map each row → `tkw`/`tki`/`tka`/cvar toggle).
- The **client draw/show mechanism** (`FUN_10066a90` signature + how the menu is opened/closed +
  the key-capture that turns a number press into the action) — needed to reconstruct it in cl_dll.
