# Weapon-select flow — RE findings (cont.67)

Recovered from `client.dll` (cursor/bar) + `ts_i386.so` (server commit), to replace the
invented `tsnextweapon`/`tsprevweapon` stand-ins with the faithful protocol. The swap
contract = the **command names + their wire**, not the internal layout.

## Commands — what is REAL vs INVENTED

Real TS **server** commands (exhaustive, from `ClientCommand__FP7edict_s` @0x9d838, PIC
base ebx=0x165E10; each is an exact `strcmp(argv0, …)`):
`say`, `say_team`, `fov`, `use`, `respawn`, `powerup`, `cashalarm`(?), `rst_cfg`(?),
`tkw`, `tki`, `tka`, `cutwire_*`, `drop_*`, `drop`, `usepowerup`, `firemode`,
`useitems`, `lastinv`, `changeteam`, `jointeam`, `motd`, `fullupdate`.

Real **client** commands (registered via pfnAddCommand in client.dll):
`invnext`, `invprev`, `slot1`..`slot10`, `+attack`, `+attack2`, `cancelselect`,
`menuselect %d`, plus the buy-forward `buy`.

- ❌ **`tsnextweapon` / `tsprevweapon` DO NOT EXIST** — invented in our build. Remove.
- ✅ `firemode` is the real fire-mode cmd (our `tsfiremode` stand-in → rename).
- ✅ `drop` is the real drop cmd (our `dropweapon` stand-in → rename).
- ✅ `usepowerup` IS a real server command (the HANDOFF note that it was invented is
  wrong — see the cmd list; revisit the powerup front separately).
- `tkw`/`tki`/`tka`/`rst_cfg`/`cashalarm` are the real buy/config cmds (relevant to the
  "verify buy command names" front; they parse `weapon_%d` args via sscanf/strtol →
  AddItemToArmament/PickUpWeapon/ResetArmament).

## Client cursor model — `FUN_1004f500(this, dir)` (invnext dir=1, invprev dir=0)

The weapon-select object (the CHudTSWeaponBar singleton @0x100d0ca0):
- `this+0x73c` = **selection-active** flag (0 = bar not in select mode).
- `this+0x740` = cursor **row** (0..0x10, 17 rows), `this+0x744` = cursor **col** (1..5).
- `this+0x7c0` = a fade/timer object (FUN_100361f0 active?, FUN_10036720/750 start/refresh).
- `this+0x7b8` = the committed/active weapon type used to seed the cursor.
- arsenal grid cell = `this + (row*0xb + col)*8 + 0x164` → weapon type id (`tsweapon_t`),
  ≤0 = empty. (0xb = 11 stride.) Per weapon info (FUN_100071c0(type)): +0x85 = its column,
  +0x21 = its row → used to seed the cursor from the current active weapon.

`invnext`/`invprev` handlers (0x1004fce0/0x1004fd40) are rate-limited (this+0x371c vs
cl_time + interval) and gated by two state checks (FUN_1004c250/FUN_1004c280, ~dead/menu),
then call FUN_1004f500(dir):
1. If `this+0x73c==0`: seed cursor (row,col) from the active weapon's info, set 0x73c=1.
2. Clamp col∈[1,5], row∈[0,0x10].
3. Step (row,col) in `dir` through the 17×5 grid, **skipping empty / akimbo-paired cells**,
   wrapping row 0..0x10 then advancing col 1..5. Stops at the next owned weapon.
4. Store the new (row,col).
5. **If cvar `hud_fastswitch` != 0**: immediately COMMIT — format `"weapon_%i"`
   (string @0x10095db8) with the cell's weapon type and send it to the server (engine
   client cmd), then clear 0x73c (close the bar). If `hud_fastswitch == 0`: leave the bar
   open; the commit happens on a later key (slotN / +attack), same `"weapon_%i"`.

So the **commit transport = a `weapon_<N>` console command** (N = numeric `tsweapon_t`),
emitted by the client and forwarded to the server. (NOT `cmd->weaponselect` — TS has one
weapon entity `weapon_tsgun`; and NOT `CBasePlayer::SelectItem`, which is the stock
by-classname search and would never match a numeric `weapon_25`.)

## Server commit — `CTSGun::SelectWeapon(tsweapon_t, bool)` @0x80fb8

```
SelectWeapon(gun, type):
  if type != gun.m_active(+0xc8) and type < 0x26 and
     (gun owns items[type] (+0x114+type*0x38) and weapon_info[type] valid) or type==0:
     HolsterWeapon(gun)         # @0x85698 — reset attack/zoom state, set next-attack time
     gun.m_active = type
     return 1                   # changed
  return 0
```
- `HolsterWeapon` does NOT deploy; it parks state and sets `player+0xa18 = gun` (pending).
- The **caller** deploys: the pattern (seen in the `lastinv` block @0xae433 inside
  ClientCommand) is `if (SelectWeapon(...)) gun->Deploy();` (vtable+0x128 = Deploy).
- `Deploy` @0x81030: sets the active item ptrs, GetBodyByItems, DefaultDeploy(view/player
  models), fires the deploy event, `CTSAnimation::Animate(player, 0x34)` (the draw anim).

So switching is **holster (current) → set active → deploy (new)**, exactly. Our
`SelectSlot` currently `Deploy()`s instantly with no holster — that's the gap.

- **`lastinv`** (server cmd): saves `player+0xa1c`=current active, selects the previously
  saved type via SelectWeapon+Deploy (last-weapon toggle).
- **`useitems`** (server cmd): `CTSGun::SwitchItems()` — the akimbo/dual-wield swap.

## Implementation plan (our SDK)

1. **Remove** invented `tsnextweapon`/`tsprevweapon`: server `dlls/client.cpp` dispatch +
   `dlls/ts_weapons.{h,cpp}` `TSCycleWeapon`; client `cl_dll/ammo.cpp`
   `UserCmd_NextWeapon/PrevWeapon` ServerCmd forwards.
2. **Server**: add a `weapon_<N>` ClientCommand handler → `CTSGun::SelectWeapon(N)` with
   **holster→deploy** (make our `SelectSlot` holster the old slot, then Deploy the new).
   Add `lastinv` (last-weapon toggle) + keep `useitems` for the akimbo swap.
3. **Client**: `cl_dll/ts_weaponbar.cpp` gains the (row,col) cursor + the FUN_1004f500
   step/skip/wrap logic, seeded from the active weapon; `invnext`/`invprev` move it;
   commit (hud_fastswitch immediate, else +attack/slotN) sends `weapon_<N>`. Draw the
   highlight on the cursor cell (the bar already RE-matches the layout).

## Status (cont.67 — IMPLEMENTED + in-engine verified)

- ✅ Removed invented `tsnextweapon`/`tsprevweapon` (server `client.cpp`/`ts_weapons.*`,
  client `ammo.cpp`).
- ✅ Server `weapon_<N>` ClientCommand → `TSSelectWeaponType` → `CTSGun::SelectSlot` with
  faithful **holster→deploy** (cancels reload, parks state, records `m_lastActive`, deploys
  with the draw anim). `lastinv` → `TSLastInv` (toggle to `m_lastActive`).
- ✅ Client `invnext`/`invprev` → `CHudTSWeaponBar::CycleType` walks OWNED weapons in the
  bar's display order (column-major) and emits `weapon_<N>` (`ServerCmd("weapon_%d\n")`).
- ✅ Verified in xash3d-fwgs (tm_avana): mouse-wheel up switched rifle→akimbo pistols,
  wheel-down stepped back; no crash, no "Unknown command".
- 🟡 DEFERRED (tractable refinement, needs engine input access we don't have cleanly):
  the `hud_fastswitch 0` "bar stays open, **+attack** commits" mode + the on-bar select
  *highlight* (we commit immediately on each move — the dominant mouse-wheel UX). Also the
  exact (row,col) grid cursor object (FUN_1004f500 0x740/0x744) is modelled as a
  display-order neighbour walk over our compact owned-set, not a literal 17×5 grid.
- NOT done here (separate fronts, findings recorded above): rename `tsfiremode`→`firemode`,
  `dropweapon`→`drop`; the `usepowerup` IS-real finding; the buy command names.

## Buy/config-menu availability (the open question — RE answer)

CORRECTION (the user supplied ground truth): **`buy` IS a real command** — the original
`ts/default.cfg`/`config.cfg` binds `b` → `buy` (alongside `firemode`/`useitems`/`drop`/
`usepowerup`/`invnext`/`invprev`). The `buy` command string is in client.dll at 0x10095b34
(I earlier only matched the menu *entry* strings "1 Buy"/"2 Buy"/… and wrongly concluded it
didn't exist). So cont.66's `buy` reopen command is **faithful**, not a stand-in.

The config/buy menu itself is **server-driven** (`gmsgShowMenu`/`gmsgVGUIMenu`); the player
can invoke `buy` to (re)open it. The availability window (the precise dead/alive gate the
`buy` handler enforces) is best pinned by the coverage harness + a targeted RE of the `buy`
handler — TODO. Working answer: available while the dead/spectating loadout-config is in
play (the spawn-wait / between-rounds), as part of the buy/config flow.
</content>
</invoke>

## Akimbo ownership model — full RE (cont.69o)

RE'd from `TakeWeapon@0x80a04`, `DropWeapon@0x8094c`(CTSGun)/`@0x84900`(CBasePlayer), `SwitchItems`
(=`FUN_00081564`, the `useitems` cmd), `GetBetterWeapon@0x80f40`, `IsWeaponAkimbo@0x8019c` /
`IsOnlyAkimbo@0x801d4`. The original stores the arsenal as an **items[]-by-tsweapon_t array**
(`gun + type*0x38 + 0x114`, 0x38/slot): `+0x114` owned, `+0x115` mode, `+0x116` fire-mode, `+0x118`
total clip, **`+0x130`/`+0x134` = the akimbo's two barrel clips (left/right)**, `+0x124`/`+0x128`
attachment masks, `+0xc8` = active type. Per-weapon table fields: `+0x84` byte = akimbo class
(4 = akimbo-capable single, 5 = only-akimbo variant; IsWeaponAkimbo/IsOnlyAkimbo), `DAT_0017dc08[type]`
= the akimbo PARTNER (single↔akimbo link), `DAT_0017dc0c[type]`(=`+0xcc`) = the akimbo's base single.

- **Pickup combine** (`TakeWeapon`): picking up a single you already own → take its akimbo partner
  (`DAT_0017dc08[single]`); the akimbo item COMBINES — `+0x118 = single.clip + akimbo.clip`,
  `+0x130`/`+0x134` = the two barrels. So owning the akimbo is one item that carries both barrels.
- **Drop akimbo** (`CBasePlayer::DropWeapon`): throws **TWO** of the base single (`+0xcc`), clip split
  (`+0x118/2` each), tossed ±4 left/right. ✅ IMPLEMENTED (cont.69n, a679c18, via TSAkimboBase).
- **useitems** (`SwitchItems`): toggles the active akimbo's barrel mask (`item+0x14` bits 2/4 =
  left/right/both) — the dual-wield "fire one / fire both" swap; plays a swap sound.
- **GetBetterWeapon**: after a drop, auto-selects the next owned weapon by `g_iWeaponPriority`.
- **Scroll skip**: the select cursor skips a single whose akimbo is owned + skips kung-fu (type 0).
  ✅ IMPLEMENTED (cont.69n, FindOwned).

**REMAINING (the "drop the single while holding akimbo leaves the other"):** requires migrating our
CTSGun arsenal from the compact `m_arsenalType[]` list (akimbo = one upgraded slot) to the original's
**items[]-by-type with the two barrel clips** + keeping the base single owned alongside the akimbo, so
a single can be wielded/dropped independently (leaving the other barrel). This is a sizable, careful
refactor of the verified weapon engine (fire/reload/ammo/switch/drop all read the arsenal) — scoped
here; best done as a dedicated pass with full in-engine retest.

### TakeWeapon combine — confirmed details (cont.69o)
Full read of `TakeWeapon@0x80a04`: when you own the single and pick up another, it takes the akimbo
partner (`DAT_0017dc08[single]`) and COMBINES into the akimbo item: `+0x118 = single.clip + akimbo.clip`,
`+0x130 = single.clip` (left barrel), `+0x134 = total - left` (right barrel); the attach masks of the
single and the akimbo are OR-merged into BOTH item slots (`+0x124`/`+0x128`). The single's item is
**NOT cleared** in the combine path — so both `items[single]` and `items[akimbo]` remain owned (this is
why scroll skips the single but slot can still select it). For a non-combinable refill, it just does
`+0x118 += 1`. So the faithful refactor: `items[38]` each {owned, clip, leftClip(+0x130),
rightClip(+0x134), mode(+0x116), attach(+0x124), exclMask(+0x128)}; combine keeps both items; drop-akimbo
throws 2× the base single (already done); dropping while one barrel active drops that barrel/leaves the
other; `useitems` toggles the active-barrel mask `+0x14` (bits 2/4). g_iWeaponPriority drives GetBetterWeapon.
