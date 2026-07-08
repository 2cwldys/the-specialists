# cont.96 — knife auto-pickup live trace (resolved) + buy-menu root cause (closed)

Continuation of the cont.95 interop work (original TS client under Proton vs OUR ReHLDS server).
Two of the cont.95 open items closed this session; artifacts preserved in `decompiles/`.

## 1. Knife auto-pickup + arsenal corruption — NOT REPRODUCIBLE at HEAD (no code change)

cont.95 item #4a claimed a death-dropped knife auto-grabbed on run-over (no E) and corrupted the
arsenal. Live server trace (`decompiles/knife_trace.gdb` — non-stop logging breakpoints on
`CTSWorldGun::CreateWeapon`/`WaitTouch`(knife-conditional)/`Use`/`GetWeapon` +
`CBasePlayer::PickUpWeapon` + `CTSGun::TakeWeapon`, on a `BUILDTYPE=RelWithDebInfo` hl.so so gdb
gets typed member access) showed:

- Death-dropped knife confirmed `CTSWorldGun` type 25, `status.clip=1`
  (`TSDropWeaponsOnDeath → DropWeapon → ThrowWeapon → CreateWeapon`).
- **23 bot run-overs + the user's own original-client run-over: the WaitTouch gate refused ALL**
  (`button=0x0`, zero `GetWeapon` calls). The gate (`type==0x1d || IN_USE`) behaves exactly as the
  static RE said (byte-identical to @0xa1250).
- The `Use` path is correct: needs IN_USE (bots show 0x822/0x826/0x28), `m_flUseCooldown` blocks the
  dropper's instant re-grab, a carried clip is restored on pickup (observed clip=13/14 hand-overs).
- rcbot acquires weapons via `UTIL_makeTSweapon → CTSGroundWeapon::GetWeapon` (direct call, bypasses
  touch) — consistent with cont.95's "bots bypass the buy path" finding.

User re-tested deliberately: no auto-grab on run-over; E-pickup registers in the bar + fires
normally. **Verdict:** resolved at HEAD; the cont.95 symptoms were most plausibly the WStatus
melee-clip encoding bug fixed in `2e8ebf2` (any cell with clip<1 renders red) plus an IN_USE-latched
touch. Reopen only if it recurs — re-arm `knife_trace.gdb` and the log names the path + button bits.

## 2. Buy-menu SrvSett delivery — ROOT CAUSE CONFIRMED; the re-broadcast is LOAD-BEARING

Finished the cont.95 watchpoint plan: built with the 0.75s re-broadcast workaround disabled
(`0 &&` on the `m_flSrvSettResendTime` block in `player.cpp` PreThink), armed the hardware
watchpoint on the original client's restriction global (client.dll base `0x0c070000`, target
`0x0c1fd174`, poisoned `0x6f6f6f6f`), user reconnected:

- **ZERO writes during the entire join + spectate + buy-menu open.** InitHUD's MSG_ONE SrvSett
  catch-up never reaches the original client's handler at connect.
- **Positive control proved the apparatus:** rcon `weaponrestriction 1` → WRITE #1 `= 0`;
  `weaponrestriction 0` → WRITE #2 `= 0x1f`; both at the known sole writer (caller `0xc0bb8bc` =
  RVA `0x4b8bc`, the SrvSett handler) within ~1 s. Wire + handler + watchpoint all good.
- ⚠ Poison artifact: `0x6f6f6f6f` has the category bits set, so during the test the buy menu
  RENDERED as unrestricted — the menu look was the poison, not a landed message. The real-world
  empty-menu symptom needs a fresh client start (the global's natural default is 0 = all restricted).

**Verdict:** the `m_flSrvSettResendTime` re-broadcast (ce04557) is load-bearing — kept, comment in
`player.cpp` upgraded from "invented workaround pending root cause" to CONFIRMED. The drop is a
connect-window message-delivery issue (our InitHUD MSG_ONE fires while the original client still
drops user messages); the three original senders are replicated faithfully, so this is a documented
ours-only compensation, not a protocol divergence.

## Recipe fix: gdb vs Wine (hl.exe) — SIGUSR1 kills a batch attach

The cont.95 `client_watchpoint.gdb` recipe dies under a batch attach: Wine raises SIGUSR1 for
thread suspension and batch gdb exits on it (observed: `Thread N received SIGUSR1 … detached`).
**Required prelude** (now in `decompiles/client_watchpoint_v2.gdb`):

```
handle SIGUSR1 nostop noprint pass
handle SIGUSR2 nostop noprint pass
handle SIG33  nostop noprint pass
handle SIG34  nostop noprint pass
```

Also: `shell date +%T.%3N` inside the watch commands stamps each write with wall-clock time for
correlating against connect events.

## 3. Melee knockback/disarm (cont.95 item #4b) — dossier pending

A read-only RE agent (resumed mid-session) is producing `melee_knockback_disarm.md`
(CloseCombatAttack @0x85f88 knockback gate; m_flDisarmPressure/DropWeapon(-2) accumulation).
