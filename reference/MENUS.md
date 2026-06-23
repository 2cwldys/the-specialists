# TS buy/config menu layout — RE findings (cont.69c)

> ## ⚠️⚠️ CRITICAL (cont.69k): we RE'd the WRONG (legacy) menu — the real menu is a different builder.
> The user's reference screenshots show a **cascading multi-column** buy menu (Config | Categories | Weapons |
> Detail all visible at once, each with its selected row green), with localized labels ("Special Purpose",
> "Go Back", "Random"), a **Credits/Slots** header + **per-attachment costs** (Silencer 900 / Lasersight 800 /
> Flashlight 500), and a **different weapon roster** (Handguns has Beretta 92F at #5 + Contender G2 at #10, not
> Akimbo Berettas / 9 entries). NONE of that is in the menu cluster everything below RE's (`0x10010xxx`,
> literal strings, `DAT_1007a950`) — that is the **LEGACY/DEAD** buy menu. The **REAL** displayed menu is a
> separate builder cluster at **`~0x10037xxx`** using the **localized token strings** (`0x10094xxx`
> `#Buy_UseNewConfig`/`#Buy_SpecialPurpose`/`#Buy_GoBack`/…, resolved via `ts/titles.txt`) + the draw funcs
> `FUN_1004ff10`/`FUN_1004fed0` + the panel global `&DAT_1018a6b0`. Confirmed entry point:
> **`FUN_10037540`** = the real CONFIG-menu builder (state-based item 1: `+0x2f08 ? UseNewConfig : UseLastConfig`,
> empty→`EmptyConfig` red). To reproduce the menu the user actually sees, RE the `0x10037xxx` cluster (cascade
> layout, real weapon table, costs, Credits/Slots), NOT the `0x10010xxx` legacy menu below.
> (titles.txt: `#Buy_SpecialPurpose`→"Special Purpose", `#Buy_GoBack`→"Go Back", `#Buy_Random`→"Random",
> `#Buy_Credits`→"Credits". Same binary — just localization + the real builder.)
> The current `cl_dll/vgui_TSBuyMenu.cpp` reproduction is built on the LEGACY menu; the compact-font + green-
> highlight styling (cont.69j) carries over, but the structure (cascade) + data (weapon table/costs) need the
> real `0x10037xxx` RE. **The weapon-select bar (Image #4) is the invnext/invprev loadout preview that also
> renders as the in-game weapon HUD when ALIVE (shows per-weapon attachments) — CHudTSWeaponBar.**

## ★ THE REAL BUY MENU — full RE (cont.69k). Rebuild target: a HUD element, NOT vgui.

**Rendering: it's HUD-drawn** (this is the good news — it reuses our existing TS HUD draw + font):
- Each entry is drawn by **`FUN_1004ff10(cursor, token, r, g, b, scale)`**: it draws a per-entry filled
  colour box via `(*DAT_100d5b54)(x, y, w-1, h-1, r, g, b, scale)` THEN the text via
  **`FUN_10021ce0` = `TSHud_DrawString`** (our recovered HUD string draw, `reference/HUD_DRAWCHAIN.md`) with
  font `DAT_100d5908` and `FUN_100349a0(token)` = `gEngfuncs.pfnLocalize/`titles.txt lookup. Then advances
  the y cursor by rowH. So **use `CHudTSStatus`/`TSHud_DrawString` + the TS HUD font**, not vgui.
- **`FUN_1004fed0(cursor, X, Y, rowH, width)`** sets the column draw-cursor: `cursor[0]=X`, `[4]=Y`,
  `[8]=rowH`, `[0xc]=width`. The menu cursor global is `&DAT_1018a6b0`.

**Cascade columns** (all visible at once — orchestrator `FUN_10036750`, a switch on `obj+4` = current level):
each level's builder sets its column X so they sit side by side, rowH `0x14`(20), width `0x80`(128):
config **X=8**, category **X=0x88(136)**, weapons **X=0x108(264)**, detail **X=0x188(392)**.
`FUN_10036af0` (called every frame before the switch) draws the **top header bar** (`(*DAT_100d5b54)(8,8,0x280,
0x13,…)`): `#Buy_Total` "(Total: %i)" + `#Buy_FreeSlots` "Free Slots : %i" (red when over). States:
0=config(`FUN_10037540`), 1=category(`FUN_100377d0`), 2=weapons(`FUN_10037960`), 3=detail(`FUN_10037ba0`),
4/5/6/7=accessory/save/load/remove variants, 8=off.

**Labels = `titles.txt` tokens** (drawn as "`<key> ` + localize(token)"):
- Config: `1 #Buy_UseNewConfig`/`#Buy_UseLastConfig` (state `obj+0x2f08`) or `#Buy_EmptyConfig` (red, if
  `obj+0x414<1`), `2 #Buy_Buy`, `3 #Buy_RemoveItem`, `4 #Buy_ResetConfig`, `5 #Buy_SaveConfig`,
  `6 #Buy_LoadConfig`, **`7 #Buy_Random` = always GREEN (45,195,45)**, `0 #Buy_Cancel` (blue 100,100,205, only
  when `obj+4==0`).
- Category: the 5 categories (`%c %s` per-letter) + `6 #Buy_PlayerAccessory` + `0 #Buy_GoBack` (blue).
  Names via titles.txt: Handguns/SMGs/Rifles/Shotguns/**#Buy_SpecialPurpose="Special Purpose"**.
- Detail: `1 #Buy_Buy` (green if affordable) + per available attachment **`%i #HUD_Silencer %i`** (key + name +
  **cost**: Silencer 900, Lasersight 800, Flashlight 500, Scope 1800) + Buy Akimbo/Full Load + `0 #Buy_GoBack`.
- Colours: available = **half the HUD colour** `hud_r/g/b /2` (dim); unavailable/over-budget = **red
  (195,45,45)**; Random/affordable-Buy = **green (45,195,45)**; Cancel/GoBack = **blue (100,100,205)**.

**THE REAL WEAPON ROSTER** — the menu's own table at **`obj+0x2d6c`**, indexed `(row*5 + cat)` (5 cats/row,
≤12 rows), populated by **`FUN_10036200`** (NOT the legacy `DAT_1007a950`). Decoded (matches the user's image):
| cat | category | weapon ids (tsweapon_t) | names |
|---|---|---|---|
| 0 | Handguns | 1,9,12,14,2,21,22,28,31,36 | Glock-18, SOCOM-MK23, Desert Eagle, Five-seveN, **Beretta 92F**, Golden Colts, Glock-20C, Ruger-MK1, Raging Bull, **Contender G2** |
| 1 | SMGs | 3,6,7,17,19,23 | Mini-Uzi, MP5SD, MP5K, Skorpion, MP7-PDW, UMP |
| 2 | Rifles | 5,13,15,27,18 | M4A1, AK47, STEYR-AUG, M16A4, Barrett M82A1 |
| 3 | Shotguns | 4,11,20,26,33 | Benelli M3, USAS-12, SPAS-12, Mossberg 500, Sawed-off |
| 4 | Special Purpose | 24,25,32,34,35 | M61, Combat Knife, M60E3, Katana, Seal Knife |

Header: each selected weapon shows a preview via `FUN_10038180` ("Credits"/"Slots" + the gun image — Image #3).
Attachment availability = `FUN_10038d70(menu, 1<<bit)` (reads the weapon's support/exclude masks, same 0xE0/0xE4
fields from the attachment RE above). Per-weapon price/slots: `weaponinfo[0x36]` (price), `weaponinfo+0x34`
(slot cost). `obj+0x414` = current spent, `obj+0x418` = free-slots used, `obj+0x10` = selected weapon,
`obj+0xc`/`obj+8` = selected attachment mask / category.

**Rebuild plan:** replace the vgui `CTSBuyMenuPanel` with a **`CHudBase` HUD element** that, when the buy menu
is active, draws the header + cascade columns with `TSHud_DrawString` + the TS font, using the roster above +
the token labels (localize via `gEngfuncs.pfnLocalize` / titles.txt) + the RE'd colours. Navigation: number
keys pick within the active column (advancing `obj+4` level + the selection); commit emits `rst_cfg`/`tkw`/
`tki`/`tka` (unchanged server contract). Still to RE for the rebuild: the input/level-advance handler + the
accessory column (states 4-7) + `FUN_10038180` preview geometry.


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

### Runtime inspection — UNBLOCKED + proven (cont.69f)

The original TS is running live (`hl.exe -game ts`, **PID 3093683** under Proton). gdb attaches and
reads its memory: `gdb -p 3093683 -batch -nx -ex 'find ...'` / `x/...` works.
- **client.dll runtime base = `0x0c070000`** (preferred 0x10000000 → runtime = static − `0x3F90000`).
  So `FUN_10011160` (static 0x10011160) → runtime `0x0c081160`, etc.
- The buy menu's panels are LIVE in heap: a string pool of vgui panel names `Buy_status` / `Buy_Random`
  / `Buy_Cancel` / `Buy_Handguns` / `Buy_SMGs` / `Buy_Rifles` (found `Buy_Handguns` @0x3b6facc). So the
  CONFIG + category panels ARE built — the `+0x2d84` data static-RE couldn't reach is readable at runtime.
- Button DISPLAY text is a vgui TextImage (not at object+0x104, which is a TS name copy). To read a
  button's exact `_pos`/`_fgColor`: find its `Buy_*` name string → find the pointer to it (the button's
  `_name` field) → that gives the button object → read vgui::Panel `_pos`(x,y) + `_fgColor`(RGBA). Need
  the vgui::Panel/Button field offsets (from the vgui lib) to index those.

**This converts the "config panel not RE-able" wall into a tractable runtime read.** Next: get the
vgui::Panel `_pos`/`_fgColor`/`_name` field offsets, walk the `Buy_*` config buttons, and record the real
positions/colours — then apply them to CTSBuyMenuPanel.

### Runtime capture — the exact procedure (cont.69f, KEY FINDING)

The menu panels are **built once and CACHED** — re-opening the buy menu only `setVisible`s them; it does
NOT re-run the entry-add builders. So a build-breakpoint armed *after* the first buy-menu access never
fires (confirmed: bp at FUN_100668f0 / FUN_10011230 / FUN_10011160 didn't fire across ~10 re-opens).

**To capture the real per-button values, arm the breakpoints BEFORE the first build, then trigger a FRESH
build** (reconnect to the server / restart the map / first-time enter the spectate-buy phase). Procedure
(game live, PID 3093683, client.dll base 0x0c070000):
```
# /tmp/cap.gdb : break the entry-add functions, log args, auto-continue
break *0x0c0d68f0   # FUN_100668f0(panel, idx, text, c_r, c_g, c_b, 0x32, scale, action)  [submenu entries]
  commands; silent
    printf "ADD p=%#x i=%d t='%s' r=%d g=%d b=%d s=%#x act=%d\n", $ecx,*(int*)($esp+4),*(char**)($esp+8),*(int*)($esp+0xc),*(int*)($esp+0x10),*(int*)($esp+0x14),*(int*)($esp+0x1c),*(int*)($esp+0x20)
    continue; end
break *0x0c0d6a90   # FUN_10066a90 (config item-1 colour update)
continue
# then: gdb -p 3093683 -batch -nx -x /tmp/cap.gdb  AND reconnect/restart so the buy menu rebuilds.
```
That one capture yields every entry's exact `text + RGB colour + action` for the config panel + all weapon
categories + accessories — resolving the exact-styling unknown (the +0x2d84 config panel static-RE can't
reach) in a single shot. NB FUN_100668f0 builds via panel fields for geometry; the colour args (r,g,b) are
the per-entry RGB confirmed from FUN_10010440 (avail green / unavail red / Cancel (100,100,205)).

### Action-thunk dispatch — the menu builders (cont.69f, for the weapon-list slice)

The menu's per-action vtable thunks (`mov esi,ecx; call <builder>; ret $4`, cluster 0x10011310–0x10011460)
each invoke a builder. Mapped:
- `0x1001134a` → `FUN_10012050` (apply / "Use New Config" — emits rst_cfg/tkw/tki/tka)
- `0x10011372` → `FUN_10010440` (buy-category menu: Handguns/SMGs/Rifles/Shotguns/Other)
- `0x10011382` → `FUN_10010630` (Remove-Item list — player loadout `+0xb0`)
- `0x100113be` → `FUN_10011160` (config item-1 colour update)
- `0x100113d0` → `FUN_10012c20`
- **`0x1001140e` → `FUN_10010890`** (sets `+0xc=1`, hides the buy-category panel `+0x2d88`) ·
  **`0x10011415` → `FUN_10010bb0`** (sets `+0xc=2`, hides the weapon-list panel `+0x2d8c`) ·
  **`0x1001143b` → `FUN_100108b0`** = **THE per-category weapon-list builder** (resolved below).

### Per-category weapon lists — RESOLVED (cont.69g, RE-grounded + in-engine verified)

There is **ONE** weapon-list builder, **`FUN_100108b0`**, parameterised by the selected category index
at `clientobj+0x20`; it rebuilds the weapon-list panel `+0x2d8c` (NOT three separate builders). It:
- iterates **9 slots** of a `[category][9]` int table **`DAT_1007a950`** (`*(DAT_1007a950 + (slot + cat*9)*4)`),
  dropping `-1` (empty), restricted weapons (`FUN_1000b6d0` = IsRestrictedWeapon), and — in non-team mode —
  weapon id **`0x18`(24) = M61 Grenade** (`1 < DAT_100cf97c` = team count > 1 ⇒ team mode);
- labels each entry **`sprintf("%i %s", onscreenIndex+1, name)`** (`"%i %s"` @0x10092974), colour **green
  if affordable / red `(195,45,45)` if over the free-slots cap `0x51`(81)** (the slot/cash gate), via
  `FUN_100668f0(panel, idx, label, r,g,b, 0x32, scale, action=8)`;
- ends with one **`"0 Cancel"`** entry at **`(100,100,205)`** action 0.

**The exact `DAT_1007a950` category→weapon-id table** (read straight from `client.dll`):

| category (cat) | weapon ids (tsweapon_t) |
|---|---|
| **Handguns** (0) | 1, 9, 12, 14, 8, 21, 22, 28, 31 |
| **SMGs** (1) | 3, 6, 7, 17, 19, 23 |
| **Rifles** (2) | 5, 13, 15, 27, 18 |
| **Shotguns** (3) | 4, 11, 20, 26, 33 |
| **Other** (4) | 24, 32, 34, 25, 35 |

**Display names** come from each weapon struct's `+0x44` field (client weapon table `DAT_100aa050`,
**stride `0xf7*4 = 0x3DC`**, `FUN_100071c0` = get_weapon_info), copied at init by the per-weapon init
funcs (dispatcher `FUN_10007de0` calls each with `&DAT_100aa050 + id*0x3DC`). The full id→name map
(recovered by decompiling all 36 init funcs + resolving the `+0x44` source string): 1 Glock-18, 2 Beretta 92F,
3 Mini-Uzi, 4 Benelli M3, 5 M4A1, 6 MP5SD, 7 MP5K, 8 Akimbo Berettas, 9 SOCOM-MK23, 10 Akimbo MK23,
11 USAS-12, 12 Desert Eagle, 13 AK47, 14 Five-seveN, 15 STEYR-AUG, 16 Akimbo Mini-Uzi, 17 Skorpion,
18 Barrett M82A1, 19 MP7-PDW, 20 SPAS-12, 21 Golden Colts, 22 Glock-20C, 23 UMP, 24 M61 Grenade,
25 Combat Knife, 26 Mossberg 500, 27 M16A4, 28 Ruger-MK1, 30 Akimbo Five-seveN, 31 Raging Bull,
32 M60E3, 33 Sawed-off, 34 Katana, 35 Seal Knife, 36 Contender G2, 37 Akimbo Skorpions.

### Config-menu styling — RESOLVED from the user's reference image (cont.69j)

The user supplied a screenshot of the real config page (the `+0x2d84` panel that resisted static RE).
Ground truth: **compact** tightly-packed rows, the small **command-menu font** (`Scheme::sf_primary3`),
**white** entry text on a dark translucent box, and a **green selection/hover highlight** (a green bar +
bright green text on the entry under the cursor). Labels come from localization tokens — `7 #Buy_Random`
localizes to **"Random"** (NOT "Buy Random"); item 1 is the state toggle
`#Buy_UseNewConfig`/`#Buy_UseLastConfig`/`#Buy_EmptyConfig` ("Use New/Last/Empty Config").
Applied (`CTSMenuButton`, cont.69j): `sf_primary3` font, `m_rowH = YRES(15)` (compact), green hover
highlight via the CommandButton armed-state (`bNoHighlight=false`; paint green text + a green bar instead of
the SDK orange), and the "Random" label. **Screenshot-matched to the reference.** OPEN: item 1's state-based
label (we show "Use New Config"; needs the server to report the saved-config state) + the exact menu position.

### Per-weapon attachment submenu — RESOLVED (cont.69i, RE-grounded + in-engine verified)

Clicking a weapon in the list (action 8) opens the **per-weapon attachment submenu** — the 4th panel
`+0x2d90`, built by **`FUN_10010bd0`** (state `+0xc = 4`). It reads the weapon being configured from
`clientobj+0x28` and the already-selected attachment mask from `clientobj+0x24`, and builds:
- **"1 Buy"** (`s_1_Buy`) — action **8** (queue the weapon with the selected attachments).
- one entry per **available attachment** — shown if `(bit & selected[0x24])==0 && (bit & weaponinfo[0x39])==0
  && (bit & weaponinfo[0x38])!=0` — action **10** (toggle the bit + rebuild; selecting one removes it
  from the list): bit0 **Silencer** (+900), bit1 **Lasersight** (+800), bit2 **Flashlight** (+500),
  bit3 **Scope** (+1800). Format strings `"%i Silencer  %i"` / `"%i Lasersight  %i"` / `"%i Flashlight  %i"`
  / `"%i Scope  %i"` (@0x100929b4/0x100929a0/0x1009298c/0x1009297c).
- then EITHER **"6 Buy Akimbo"** (`s_6_Buy_Akimbo`, action 8) if `weaponinfo[0x32] != -1`, OR
  **"6 Full Load"** (`s_6_Full_Load`, action 9) if `weaponinfo_byte[0x37] != 0` — mutually exclusive.
- **"0 Cancel"** (action 0) → back to the weapon list.

**The attachment availability is WEAPON DATA, not menu data** (read here, also drives the HUD's bottom-right
attached-icon row). The relevant weapon-struct fields (`DAT_100aa050 + id*0x3DC`, `FUN_100071c0`):

| field | offset | meaning |
|---|---|---|
| support mask | `0xE0` (`[0x38]`) | attachments the weapon CAN take (bit0 Sil, bit1 Las, bit2 Fla, bit3 Sco) |
| exclude mask | `0xE4` (`[0x39]`) | removed from support (integrated attachments — MP5SD silencer, AUG/Barrett scope, Ruger silencer) |
| akimbo type | `0xC8` (`[0x32]`) | akimbo variant weapon id, `-1` = none → enables "Buy Akimbo" |
| full-load    | `0xDC` (`[0x37]`) | nonzero (throwables: M61/Combat Knife/Seal Knife) → enables "Full Load" |

**Per-weapon EFFECTIVE attachment mask** (support & ~exclude) + akimbo + full, extracted from all 36 init funcs:
1 Glock-18 Sil|Las|Fla ak-1 · 2 Beretta Sil|Las|Fla ak8 · 3 Mini-Uzi Sil ak16 · 4 Benelli Las|Fla|Sco ·
5 M4A1 Sil|Las|Fla|Sco · 6 MP5SD Las|Fla|Sco (sil integrated) · 7 MP5K all4 · 8 Akimbo Berettas Sil|Las|Fla ·
9 SOCOM-MK23 Sil|Las|Fla ak10 · 10 Akimbo MK23 Sil|Las|Fla · 11 USAS-12 Las|Fla · 12 Desert Eagle Sil|Las|Sco ·
13 AK47 Sco · 14 Five-seveN Sil|Las|Fla ak30 · 15 STEYR-AUG Sil|Las (sco integrated) · 16 Akimbo Mini-Uzi Sil ·
17 Skorpion (none) ak37 · 18 Barrett Las (sco integrated) · 19 MP7-PDW Sil|Las|Sco · 20 SPAS-12 Las|Fla ·
21 Golden Colts (none) · 22 Glock-20C Sil|Las|Fla · 23 UMP all4 · 24 M61 full · 25 Combat Knife full ·
26 Mossberg Las|Fla · 27 M16A4 Las|Fla|Sco · 28 Ruger Las (sil integrated) · 30 Akimbo Five-seveN Sil|Las|Fla ·
31 Raging Bull Las|Sco · 32 M60E3 (none) · 33 Sawed-off Las · 34 Katana (none) · 35 Seal Knife full ·
36 Contender Las|Sco · 37 Akimbo Skorpions (none).

**Reproduced** (`cl_dll/vgui_TSBuyMenu.cpp` `PAGE_WEAPON_DETAIL`, cont.69i): clicking a weapon opens the
attachment submenu built from `s_weaponAttach[]` (the table above); attachments toggle client-side (selected
ones disappear, per the original), Buy/Buy Akimbo/Full Load queue the weapon (`tkw <id>_1`, akimbo → the
variant id). **In-engine verified** (e.g. MP5SD correctly shows Lasersight/Flashlight/Scope — silencer
excluded). **DEFERRED (next slice):** the exact attachment-to-server wire encoding (buried in the
`FUN_10012050` apply string-builder) + server-side attachment application + the HUD attached-icon row — the
attachments are currently selected client-side but not yet networked/applied.

### Per-category weapon lists (cont.69g)

**Reproduced** (`cl_dll/vgui_TSBuyMenu.cpp`, cont.69g): a single dynamic `PAGE_WEAPONS` rebuilt by
`ShowCategory(cat)` from the table above (with the M61 team-gate), green entries + blue Cancel; clicking
a weapon emits the real swap-contract command **`tkw <id>_1`** (queues into the server armament;
`reference/BUY.md`) — committed on "Use New Config" (menuselect 1 → spawn with `BuyArmament`). The number is
drawn by the SDK CommandButton's bound-key column (so the label is the bare name, matching the config/category
pages — the `"%i %s"` number is the same thing rendered our way). **NOT yet built** (further slice): the
per-weapon **detail submenu** (`6 Full Load` @0x10092854 / `6 Buy Akimbo` @0x10092860, the 4th panel
`+0x2d90`, action 8's real target) and the faithful **client-side loadout queue + apply sequencer**
(`FUN_10012050` emits `rst_cfg`+the full `tkw/tki/tka` list at once); we bridge per-click instead. The
client-side **free-slots cost / red-unavailable** gate (`+0x430` vs cap 81) is also deferred (weapons draw
green = available, correct for the free-buy default).
