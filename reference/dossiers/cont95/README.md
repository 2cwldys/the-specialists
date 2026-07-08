# cont.95 — interop playtest (ORIGINAL client vs OUR ReHLDS server) + live-RE recipes

Preserved artifacts + findings from the cont.95 interop session. The user playtests the ORIGINAL TS
client (`hl.exe` under Proton) against OUR ReHLDS server (the true drop-in test) — the first thorough
interop pass, which surfaced LATENT bugs our own client had masked.

## Preserved gdb scripts (`decompiles/`)
- `server_crash_capture.gdb` — run the rig under gdb with SIGSEGV capture (full bt + registers + rsi
  string + raw stack + mappings). Used to catch the FindEntityByString crash. Recipe:
  `cd <rig> && LD_LIBRARY_PATH=.:$LD_LIBRARY_PATH gdb -batch -nx -x server_crash_capture.gdb --args ./hlds_linux -game ts +sv_lan 1 +maxplayers 8 +rcon_password lol +map ts_hammertime`
- `client_watchpoint.gdb` — hardware watchpoint on the ORIGINAL client's restriction global. Template
  for watching ANY client.dll global from the live `hl.exe`. **Recipe (addresses change per session):**
  1. `PID=$(pgrep -f 'hl.exe' | ...)` — pick the big-RSS Wine proc (was 22494).
  2. `client.dll` base = the FIRST `r-xp` mapping of client.dll in `/proc/$PID/maps` (file offset 0;
     was `0x0c070000` — Wine relocates from the `0x10000000` PE imagebase).
  3. live addr = base + (PE_RVA). E.g. the restriction global DAT_1018d174 → RVA `0x18d174` →
     `0x0c070000 + 0x18d174 = 0x0c1fd174`.
  4. Edit the script's `attach <PID>` and the two `0x0c1fd174` addresses.
  5. `set *(int*)ADDR = 0x6f6f6f6f` POISONS it first so gdb reports even a same-value write (gdb's
     hardware `watch` SUPPRESSES writes that don't change the value — the poison defeats that).
  6. Resolve a caller frame: `RVA = live_addr - client.dll_base`, then name it via `rz-bin -E reference/client.dll` / `dc.sh client 0x<rva>`.

## Watchpoint RESULT (buy-menu root cause, item #2)
The restriction global's ONLY writer in client.dll is the SrvSett handler (`FUN_1004b840` case 3, the
store at RVA `0x4b8b7`; caller frame resolved to RVA `0x4b8bc`). The watchpoint PROVED our SrvSett
message reaches + applies on the original client, and the applied value tracks `m_iWeaponRestriction`
exactly (wrote `0` under `weaponrestriction 1`, `0x1f` under `weaponrestriction 0`). **Still open:** gdb
suppresses the second same-value write, so I couldn't tell whether InitHUD's early send or the 0.75s
re-broadcast (the `m_flSrvSettResendTime` workaround, ce04557) is the one that lands. FINISH = build with
the workaround DISABLED, reconnect, re-watch: `0x1f` lands → InitHUD works, remove the workaround; empty
→ InitHUD dropped, keep it.

## Knife auto-pickup + arsenal-corruption bug (item #4a, UNFIXED — needs a LIVE SERVER TRACE)
User ran over ANOTHER player's DEATH-DROPPED knife (no E) → it auto-grabbed AND corrupted the arsenal:
the knife viewmodel deploys (m_weaponType=knife) but it's NOT in the WStatus bar (m_items[knife] wrong),
firing gives EMPTY-CLICK, and the kung-fu bar cell goes RED (a WStatus regression). STATIC RE CONTRADICTS
the observed auto-pickup:
- Death drop = `DropWeapon → ThrowWeapon → CTSWorldGun::CreateWeapon` with `m_weaponType = TSW_KNIFE = 25`.
- EVERY `WaitTouch` gates auto-pickup on `type == 0x1d (TSW_FLAG=29) || (button & IN_USE 0x20)` —
  `CTSWorldGun::WaitTouch` (ts_worldgun.cpp:257) AND `CTSGroundWeapon::WaitTouch` (ts_worldgun.cpp:544).
  Our WaitTouch is BYTE-IDENTICAL to the original @0xa1250 (verified) — FAITHFUL, FLAG-only.
- `CTSKnife::WaitTouch` (thrown-knife, ts_grenades.cpp:657) only damps velocity — no pickup.
- `ThrowWeapon` always makes a `CTSWorldGun` (no knife special-case).
So a type-25 knife auto-grabbing on touch (no IN_USE) is UNEXPECTED per the static code → the real pickup
path must be OBSERVED. **NEXT AGENT:** run the rig under gdb, break on `CTSWorldGun::WaitTouch` /
`GetWeapon` / `CBasePlayer::PickUpWeapon` (nm addrs in ts_worldgun.cpp / ts_gun.cpp headers), have the user
run over the knife, capture the actual call chain + why the IN_USE gate passes. Then fix the half-pickup
(a stackable-knife PickUpWeapon that sets active without registering m_items/WStatus). Files:
`dlls/ts_worldgun.cpp` (WaitTouch/Use/GetWeapon @0xa1158/@0xa1198), `dlls/ts_gun.cpp`
(PickUpWeapon @0x83318 / TakeWeapon @0x80a04 — the stackable-merge path).

## Melee knockback / disarm (item #4b, UNFIXED)
User: kicks/punches should send the victim FLYING BACK; currently victims only fly on death (or mid-air),
and it "doesn't match the original." Also wants punches to possibly DISARM. RE targets:
- `CBasePlayer::CloseCombatAttack @0x85f88` has the >30-dmg knockback (`|attacker vel|·dir`, `z=235`,
  ts_player.cpp:880). Check why it doesn't fire for GROUNDED LIVING victims — the `flDamage > 30` gate?
  the flat `reach` damage passed (35/45/55 > 30 should pass)? Is `bKnockback` true on the kick path?
- Disarm: `m_flDisarmPressure` (player+0x7b4, PreThink-decayed ×5) + `DropWeapon(-2)` threshold in the
  melee twins. RE whether kicks/punches accumulate disarm pressure vs only the CloseCombatAttack twins.
- Reconcile against `m_flSpawnProtectTime` (player+0x71c) gate — a spawn-protected victim can't be disarmed.
