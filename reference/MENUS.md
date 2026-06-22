# TS buy/config menu layout — RE findings (cont.69c)

Recovered from `client.dll` (.data menu strings + the builder functions). This is the
authoritative menu hierarchy; it supersedes the guessed placement of any single entry.

## Mechanism (important — corrected)

The TS buy menu is a **client-side, custom-drawn TEXT menu**, NOT VGUI and NOT the stock
`gmsgShowMenu`/`CHudMenu`.

- The client builds and draws each menu entry itself: `client.dll FUN_10011160` (config-menu
  item 1) and the sibling builders call **`FUN_10066a90(panel, idx, text, x, y, …, scale, …)`**
  — a positioned-text **clickable** "button" draw. The menu strings live in `client.dll .data`.
- Selection works **two ways, both active at once**: (a) **mouse click** on a menu entry — in
  the spectate-wait phase the observer cursor is free, so you move the mouse and click an entry;
  and (b) the matching **number key**. Either fires that entry's action (send a buy command
  `tkw`/`tki`/`tka`/`rst_cfg` — see `reference/BUY.md` — and/or open a submenu).
- **Clicking anywhere that is NOT a menu entry** = the default **"apply new config"** (item 1):
  it commits the queued loadout and arms you, so the **next click spawns** you into play. (This
  is the cont.66 "click anywhere ⇒ item 1" behavior, now confirmed.)
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

## Mechanism RE (in progress — needed before implementing; do NOT improvise it)

The implementation mechanism (not just the layout) must be RE'd before coding — an earlier
improvised cl_dll menu (guessed geometry/input/triggers) was reverted. Recovered so far:

**Menu entry object model** (`client.dll`):
- Each menu entry is an object. `FUN_10066a90(panel, idx, text, x, y, color, scale, …, actionId,
  actionValue)` populates entry `idx`: copies `text`→entry+0x104, calls `FUN_10066bb0` to store
  geometry/color (entry+0xf4=x, +0xf8=y, +0xfc, +0x100=scale), and stores the **action**
  (entry+0x144=actionId, +0x148=actionValue).
- The menu **panel** object is at `clientobj+0x2d84`; it holds the entry-pointer array at
  `panel+0xdc` and the entry count at `panel+0xbc`.
- `FUN_10011160` builds config-menu **item 1** (the "Use New/Last/Empty Config" toggle, chosen by
  state: `clientobj+0x10` flag + queued-count `clientobj+0x42c`). 8 call sites rebuild the menu.

**Client buy validation** (`FUN_100124a0(this, type, count, isItem)`): before an item is queued it
checks affordability (`CashBased`, cash) AND the **free-slots resource** `this+0x430` against the
cap `0x51` (81) — this is the **"Free Slots : %i"** HUD value and the `GetSpeedBySlots` speed input
(speed = 210 + freeslots·factor). So the loadout is slot-limited client-side; kevlar/weapons each
consume slots. (Cross-ref: the deferred kevlar free-slots cost + speed penalty, ledger cont.69c.)

## Remaining RE targets (resumable work-list)

1. **Panel draw + background** — the function that renders `panel+0xdc` entries (real x/y geometry,
   the background panel, font/scale). Gives the faithful visual layout.
2. **Open/close trigger** — what shows the menu (entering spectate-wait? a key/command?) and hides it.
3. **Input capture** — how a mouse-click hit-tests an entry (entry x/y rects) + the number-key path,
   and the **click-outside ⇒ "Use New Config"** logic.
4. **Per-entry action dispatch** — `actionId` (entry+0x144) → what each entry does (navigate submenu /
   send a buy command / toggle `cl_buykevlar`/`cl_laser` / apply).
5. **The client→server buy commands** (the SWAP CONTRACT) — map each weapon/accessory selection to the
   exact emitted command (`tkw`/`tki`/`tka` + the item-queue/apply command). The per-category weapon
   lists (which `tsweapon_t` under Handguns/SMGs/Rifles/Shotguns/Other) come out of the category builders.

Only after 1–5 are RE'd should the cl_dll menu be (re)built — faithfully, not improvised.
