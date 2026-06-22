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

**Architecture — a custom `vgui::Panel` subclass (NOT the MOTD `gmsgVGUIMenu`).** `FUN_10010440`
(the buy-category menu builder) allocates a panel via `FUN_10066790`, constructs it with
**`vgui::Panel::Panel(this)`**, sets its vtable to `PTR_setPos_1007aa38`, sizes it via
`FUN_100667e0(panel, count, …, width=0x80, rowH=0x14, …)`, then adds each entry with
`FUN_100668f0(panel, idx, text, x, y, …, action)`. So the buy menu IS built on the `vgui::Panel`
library (which our cl_dll links) — it just isn't the stock MOTD/`CMenuPanel` image-button menu. The
menu lives as **four panels** on the client object: `+0x2d84` (config), `+0x2d88` (buy-category),
`+0x2d8c`, `+0x2d90`; each is shown/hidden via vtable+0x24 = `setVisible(bool)`.
`FUN_10011230(this, visible)` is the **open/close**: on open it copies the saved config
(`+0x434`→`+0x2c`), rebuilds (`FUN_10011160` item 1 + `FUN_100129f0` cost), and `setVisible` on the
four panels. The buy-category build loop iterates the string table `0x10092824`→`0x10092838`
(Handguns/SMGs/Rifles/Shotguns/Other), adds "1 Buy" in team mode, then "0 Cancel".
Each entry is a **`vgui::Button`** (`FUN_100668f0` → `operator_new(0x14c)` → `FUN_10066b40(button,
text, colors…)`), held in the panel's button array (`panel+0xdc`, count `+0xbc`). So the menu is a
`vgui::Panel` of clickable text `vgui::Button` children — model it on our SDK's existing vgui menus
(`vgui_ClassMenu.cpp` / `vgui_teammenu.cpp`), NOT a hand-drawn HUD element. (This is why click AND
key both work — vgui buttons handle the cursor; the number-key path selects by index.)

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

### Apply path RE'd (`FUN_10012050`) — the swap contract

"Use New Config" (config-menu item 1) → `FUN_10012050`:
1. Validate: empty config → "Your config is empty"; over cash → "No cash left"; free-slots
   (`+0x430`) over cap `0x51`(81) → abort.
2. Sort/normalise the queued loadout (`+0xb0` armament array).
3. Emit to the server (via `pfnServerCmd`): **`rst_cfg`** first, then per queued weapon
   **`sprintf("tkw ", "%i_%i")`** (type_count) / per ammo **`tka %i_%i`** / single items **`tki`**.
   (The `weapon_%i`/`lasclr_%i` config string @0x10095db8/0x10095e08 carries the per-weapon select +
   laser colour.) These are exactly the server `ClientCommand` handlers in `reference/BUY.md`.

So the menu is a **client `vgui::Panel`** that builds a loadout (validated client-side by
`FUN_100124a0` against cash + free-slots), and on apply emits `rst_cfg`+`tkw`/`tki`/`tka` — the
existing server handlers. Kevlar (accessory, `cl_buykevlar`) is queued as the item bit 0x10 (server
`AddItem`/`TakeArmor`, already reconstructed, ledger cont.69c); the exact item-queue command on apply
is the one remaining tie-off (tki-with-item-flag vs a dedicated token — confirm in `FUN_10012050`'s
accessory branch).

### Faithful reproduction plan (model on `cl_dll/vgui_teammenu.cpp` / `vgui_ClassMenu.cpp`)

Build a `vgui::Panel`-derived buy menu (4 sub-panels: config / buy-category / accessory / weapon-list)
with `vgui::Button` text entries, opened in the spectate-wait phase, click+number-key select,
click-outside ⇒ apply. Entry actions: navigate submenu / queue weapon (the per-category lists come
from the category builders) / toggle `cl_buykevlar`-`cl_laser` / apply (emit `rst_cfg`+`tkw`/`tki`/`tka`).
Background + text via the same vgui draw the status/kevlar HUD uses. Reuse the server's existing
buy-command handlers (no new server protocol).

### Navigation/action dispatch RE'd + current reproduction state (cont.69e)

Each menu action is a small `ret $4` vtable method that builds + shows the target submenu. E.g.
`0x10011370`: `mov ecx,esi; call FUN_10010440` (build the buy-category panel `+0x2d88`); `0x10011380`:
`call 0x10010630`; etc. (cluster `0x10011310`–`0x10011440`). So clicking a config entry fires its thunk
→ the matching builder (`FUN_10010440` buy-category, `FUN_10010bd0` a weapon submenu, …), which sets that
panel's positions + per-entry colours.

RE'd + applied: panel **width 0x80(128), row height 0x14(20)** (`FUN_100667e0`); **static config text =
white**; weapon-buy entries are **green/red by availability** (`FUN_10011160`/`FUN_100110ce`). STILL
PENDING (not guessed): the exact CONFIG-panel `+0x2d84` POSITION/spacing (its builder mis-decompiles in
Ghidra, no direct store-to-`+0x2d84`) and the per-category submenu builders (the weapon lists).
