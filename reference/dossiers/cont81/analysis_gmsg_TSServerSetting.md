# gmsgTSServerSetting — wire name "SrvSett"

Server global: `gmsgTSServerSetting` @ 0x1034b0 in `ts_i386.so` (nm -D confirms).
Registered wire string: **`SrvSett`**, REG_USER_MSG size **3** (HUD_MESSAGES.md
line 40; API_SURFACE.md client hooked-list contains `SrvSett`). The Ghidra/audit
name "TSServerSetting" is the gmsg GLOBAL name only — the on-wire contract name
is `SrvSett`.

## WIRE FORMAT (byte-faithful swap contract)

```
"SrvSett", registered size 3 (fixed):
  BYTE  settingId     // 0..3
  SHORT value         // little-endian, signed 16-bit read (READ_SHORT)
```

Ground truth = client handler decompile `FUN_1004b840` (client.dll):
`FUN_100299b0(pbuf,iSize)` = BEGIN_READ, `FUN_10029a10()` = READ_BYTE (settingId,
switched as unsigned), `FUN_10029a50()` = READ_SHORT (value, kept as int).
1 + 2 = 3 bytes, matching the registered fixed size. No other fields.

### Setting ids (server value source per scout intel on the server .so)

| id | server value | client action |
|---|---|---|
| 0 | `rules+0x578` rounded float — the **roundtime** setting (cvar `roundtime` in .so rodata) | `DAT_1018d16c = value` |
| 1 | `(rules+0x558 != 0)` bool — likely the **usecash** toggle (cvar `usecash` adjacent in rodata; cash-HUD on/off is the natural consumer) — **VERIFY** which cvar caches at 0x558 | `DAT_1018d170 = value` |
| 2 | `rules+0x55c`, sent after server calls `Set_ammo_count()` — the **ammocount** setting (free function `Set_ammo_count__Fi` exists in the .so = shared client/server source) | `FUN_100072c0(value)` = client-side **Set_ammo_count(int)** mirror (same shared source) — **VERIFY name**, call-shape matches |
| 3 | `rules+0x5870` — **weaponrestriction** value, set by console command `cmd_weaponrestriction__Fv` (`display_weaponrestriction__Fv` prints it; category strings "Kung Knives Pistols SMGs Rifles Shotguns All" sit near the cvar strings in .rodata) | `DAT_1018d174 = value` |

Every case ends with `*(this+0x10) |= 1` → the HUD element's `m_iFlags |= HUD_ACTIVE`
(handler `this` = CTSHud element @ 0x100d43cc, forwarder @ 0x10049a40).

## PURPOSE

Server→client broadcast of a small set of live server game-settings so the
client HUD/buy-menu reflects them: round length (id 0), the cash-economy toggle
(id 1), the ammo-count mode (id 2, applied through the shared `Set_ammo_count()`
so client purchase/ammo bookkeeping matches the server), and the current
weapon-restriction class (id 3, reflecting which weapon categories are banned).
It keeps client settings-derived state coherent with server cvars the client
cannot read.

## SERVER SEND SITES (scout intel — VERIFY in the re-project pass)

- `CTSGameRules::InitHUD` @ 0x691e0 — **MSG_ONE** to the joining player, id 0
  (roundtime), so a late joiner gets the round length. (VERIFY: only id 0 from
  InitHUD, or also 1/2/3.)
- `CTSGameRules::Think` — 3 sites @ 0x675f6 / 0x67690 / 0x6771c — **MSG_ALL**,
  ids 0, 1, 2 respectively, each sent **on change** (change-detect against a
  cached previous value; the id-2 site runs `Set_ammo_count()` server-side
  before/with the send — VERIFY exact ordering + the cached-prev member offsets).
- `cmd_weaponrestriction` @ 0xb595f (game.cpp) — **MSG_ALL**, id 3, when the
  operator issues the `weaponrestriction` console command.
- ⚠ Map-change gotcha (project memory "map-change-state-reset"): Think's
  change-detect may never re-fire for late joiners on ids 1–3 (InitHUD covers
  only id 0 per scout). Verify against the original and REPRODUCE its behaviour
  (drop-in fidelity), noting the hole if real.

## CLIENT HANDLER BEHAVIOR (ground truth, FUN_1004b840)

```c
BEGIN_READ(pbuf, iSize);
id  = READ_BYTE();
val = READ_SHORT();
switch (id) {
  case 0: g_roundtimeSetting  = val; break;  // DAT_1018d16c
  case 1: g_usecashSetting    = val; break;  // DAT_1018d170 (VERIFY name)
  case 2: Set_ammo_count(val); break;        // FUN_100072c0, shared source
  case 3: g_weaponRestriction = val; break;  // DAT_1018d174
}
m_iFlags |= HUD_ACTIVE;                      // this+0x10 |= 1, all paths
```
Unknown ids fall through to just setting HUD_ACTIVE (tolerated).

## OURS

**ABSENT.** No `SrvSett` / `TSServerSetting` / `gmsgTSServerSetting` anywhere in
`dlls/` or `cl_dll/` (grep: zero hits). Related pieces that exist but never
broadcast:
- `dlls/ts_cvars.cpp` registers `roundtime` (line 20, default "150"),
  `usecash` (line 31), `ammocount` (line 32) — cvars only; nothing sends them.
- No `weaponrestriction` command, no `Set_ammo_count`, no client hook.
- The audit's invented globals are NOT substitutes for this message:
  `gmsgStatusText`/`gmsgStatusValue` (UserMessages.cpp:68-69) are stock-HL
  multiplay messages (also in the original's stock hooked list — keep);
  `gmsgWeapons` ("Weapons", 8, UserMessages.cpp:71) is a TFC leftover with a
  leftover client hook (`__MsgFunc_Weapons`, hud.cpp:128) — unrelated to
  SrvSett, a separate removal candidate. Reconcile = clean ADD, not a rename.

## RECONCILE

Server side (match original file placement per AUDIT: global lives in
TS_gamerulesclass.cpp; command in game.cpp):
1. `int gmsgTSServerSetting = 0;` + `REG_USER_MSG("SrvSett", 3)` in
   LinkUserMessages at the original's position in the TS block (after `Wires`,
   before `TSPAward` — keep registration ORDER faithful).
2. `CTSGameRules::InitHUD`: MSG_ONE to the joiner —
   `WRITE_BYTE(0); WRITE_SHORT((int)round(roundtime.value));`.
3. `CTSGameRules::Think`: on-change MSG_ALL sends — id 0 roundtime (rounded
   float), id 1 `usecash != 0`, id 2 ammocount (call `Set_ammo_count(v)` then
   send) — each gated on a cached previous value, mirroring the 3 Think sites.
4. Add `weaponrestriction` console command (game.cpp) → store in gamerules →
   MSG_ALL `WRITE_BYTE(3); WRITE_SHORT(value);` (+ `display_weaponrestriction`
   output — VERIFY text format from the .so first).
5. Re-project pass: decompile 0x691e0 / 0x675f6 / 0x67690 / 0x6771c / 0xb595f
   (`re.sh` addr = nm addr + 0x10000) to pin change-detect members, rounding,
   and MSG_ONE vs MSG_ALL details flagged VERIFY above.

Client side:
6. `HOOK_MESSAGE(SrvSett)`; handler = the switch above verbatim (READ_BYTE id,
   READ_SHORT value); case 2 calls a shared `Set_ammo_count(int)` (one
   definition compiled into both libs, like the original); cases 0/1/3 store
   globals (round HUD, cash-HUD visibility gate, buy-menu restriction gate);
   always `m_iFlags |= HUD_ACTIVE` on the owning CHud element.
7. Consumers to wire once stored (VERIFY against client decompiles when the
   Ghidra projects free up): cash HUD hide-when-usecash-0 (id 1), buy-menu
   restriction filtering (id 3), round HUD (id 0).
