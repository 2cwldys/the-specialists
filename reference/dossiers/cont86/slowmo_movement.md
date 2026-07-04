# Slow-motion & player MOVEMENT — how the original slows the player (cont.86)

**Question:** During slow-motion the mid-air SIDE KICK (gated on `|pev->velocity| > 270`,
plain 3D speed, no /slowfactor — verified `CCombatAttack` nm 0x76078) STILL fires in the
original but NOT in our build, because our slowmo drops the player's velocity magnitude
below 270. How does the original slow player movement, and is `pev->velocity` magnitude
reduced or preserved?

## TL;DR (high confidence — confirmed by offset arithmetic + decompiles)

- The original slows the player's MOVEMENT purely by **TIME DILATION of the movement
  integration step**: in `PM_PlayerMove` it scales `pmove->frametime *= pmove->fuser1`
  (the slow factor), gated `fuser1 >= 0.1` (below that, `frametime = 0` → freeze).
- **`pev->velocity` (and `pmove->velocity`) magnitude is PRESERVED.** The velocity VECTOR,
  in units/sec, is never scaled by the slow factor. Only the per-frame distance
  (`origin += velocity * frametime`) shrinks, so the player visibly moves slower through
  space while `|velocity|` stays high → `speed > 270` gates keep passing.
- **`maxspeed` is NOT scaled by the slow factor** in the original. It is set only by the
  encumbrance path (`GetSpeedBySlots` → `SetPlayerSpeed`, clamp [0,330]) on weapon
  change / spawn — never per-frame, never × fuser1.
- **OUR divergence:** `dlls/player.cpp:2092` does `pev->maxspeed = slotBase * pev->fuser1;`
  every PreThink. In slowmo (`fuser1 < 1`) this lowers `maxspeed`, which caps the wishspeed
  clamp in `PM_WalkMove`/`PM_AirMove` (`pm_shared.cpp:1150/1424/1523`), so `PM_Accelerate`
  reaches a LOWER terminal `|velocity|` → drops below 270 → side kick fails. We ALSO lack
  the original's `pmove->frametime *= fuser1` time-dilation entirely.

---

## (a) The definitive mechanism — time dilation of `frametime`

### `PM_PlayerMove` @0xf08d8 (Ghidra 0x1008d8), the smoking gun

```c
  *(float *)(*(int *)puVar2 + 0x10) =                                   // pmove->frametime =
       (float)dc_0_001 * (float)*(byte *)(... + 2);                     //   0.001 * cmd.msec
  iVar3 = *(int *)puVar2;
  fVar1 = fc_0;                                                         // fVar1 = 0.0
  if (fc_0_10000000149011612 <= *(float *)(iVar3 + 0x20c)) {           // if (0.1f <= pmove->fuser1)
    fVar1 = *(float *)(iVar3 + 0x10) * *(float *)(iVar3 + 0x20c);      //   fVar1 = frametime * pmove->fuser1
  }
  *(float *)(iVar3 + 0x10) = fVar1;                                     // pmove->frametime = fVar1
  PM_ReduceTimers();
```

**Offset proof that `pmove+0x20c == pmove->fuser1`** (from `PM_CheckParamters` @0xe9c94 and
the `pm_defs.h` layout): `maxspeed = pmove+0x1f4` (dec 500), `clientmaxspeed = +0x1f8`,
`iuser1 = +0x1fc`, `iuser2 = +0x200`, `iuser3 = +0x204`, `iuser4 = +0x208`,
**`fuser1 = +0x20c`**. `PM_CheckParamters` reads maxspeed at `+500`/clientmaxspeed at `+0x1f8`
and tests `iuser1` at `+0x1fc` / `iuser4` at `+0x208`, all consistent.

- Constant `fc_0_10000000149011612` = `0.1f` (gate); the else path leaves `frametime = 0.0`.
- `pmove->fuser1` is the engine's per-frame mirror of the player's networked `pev->fuser1`
  slow factor (1.0 = normal, <1 = slowed). Normal play → `fuser1 = 1.0` → `frametime` unchanged.
  Slowmo (e.g. 0.3) → `frametime *= 0.3` → the whole movement step (walk/air accel, gravity,
  origin integration) runs at 30% game-time, so the player advances 30% as far per real frame.
- Because ONLY `frametime` (the integrator dt) is scaled, `pmove->velocity` — the units/sec
  vector — is left at its full value by `PM_WalkMove`/`PM_AirMove`/`PM_Accelerate`.

This is classic bullet-time: the entity moves slowly through the world, but its instantaneous
speed (units/sec) is unchanged.

### `PM_CheckParamters` @0xe9c94 — does NOT scale velocity/maxspeed by fuser1

It only: (1) clamps `maxspeed = min(clientmaxspeed, maxspeed)`; (2) scales the WISH VELOCITY
(`cmd.forward/side`, the DT_REL[0x9ea] fields) down to `maxspeed` when the input exceeds it
(standard HL); (3) zeroes move input when frozen/using; (4) halves strafe when walking. No
`fuser1`. `PM_WalkMove` @0xea26c and `PM_AirMove` @0xee434 clamp `wishspeed` to
`pmove->maxspeed` (offset `+500`) and accelerate — again no `fuser1` and no velocity scaling.

## (b) Is `pev->velocity` MAGNITUDE reduced? — NO, it is PRESERVED

Confirmed across the whole original movement chain:
- `PM_PlayerMove` scales `frametime`, never `pmove->velocity`.
- `PM_WalkMove`/`PM_AirMove`/`PM_Accelerate` set velocity toward `maxspeed`, and `maxspeed`
  is NOT reduced in slowmo (see below), so the terminal `|velocity|` is unchanged.
- `CBasePlayer::PreThink` @0x808cc (full decompile in `decomp/prethink_pm.txt`) touches the
  slow factor ONLY for animation + timers — it contains **no write to `pev->maxspeed`, no
  `GetSpeedBySlots` call, and no scaling of `pev->velocity`.** What it does with `fuser1`:
  - `InterpolateSlowMotion(frametime, &pev->fuser1, m_flSlowMotionGo)` — ease current→target.
  - `pev->framerate = GiveSlowMul() = FixedSlowAmt(fuser1) = max(fuser1,0.01)` — ANIMATION
    rate only (`StudioFrameAdvance`), NOT movement.
  - `slowFrametime = FixedSlowAmt(fuser1) * frametime (×1.5 if powerup 0x10)` — used to drain
    weapon/stunt COUNTDOWN timers (fields 0x79c/0x4dc/0x4ec/0x7a8/…). Not movement.

So in the original the slow factor reaches movement ONLY via `pmove->frametime`. `|velocity|`
stays high → the side-kick `speed > 270` gate passes during slowmo. **Matches the report.**

### `SetPlayerSpeed` @0x83068 — the ONLY maxspeed writer, no fuser1

```c
void CBasePlayer::SetPlayerSpeed(float speed) {   // clamp [0,330], push to engine
    field_0x550 = speed; if (330 < speed) field_0x550 = 330; if (field_0x550 < 0) field_0x550 = 0;
    pfnSetClientMaxspeed(edict, round(field_0x550));
}
```
Callers are weapon pickup/drop/switch (`GetSpeedBySlots` encumbrance) and spawn (`330`,
`TSInit`), the One (`330`) — never PreThink, never × slow factor.

## (c) OUR exact divergence (the bug)

**`dlls/player.cpp:2089-2092`** (in `CBasePlayer::PreThink`):
```cpp
const float slotBase = GetSpeedBySlots(this);
pev->maxspeed = slotBase * pev->fuser1;     // <-- BUG: × fuser1 lowers maxspeed in slowmo
```
- In slowmo `fuser1 < 1` → `maxspeed` shrinks → the wishspeed clamp in `PM_WalkMove`
  (`pm_shared.cpp:1150-1153`), `PM_AirMove` (`1424-1427` / air `2557` bunny cap) and
  `PM_WaterMove` (`1523`) caps `|velocity|` lower → `PM_Accelerate` converges to a lower
  terminal speed → `|pev->velocity|` drops below 270 → side kick blocked.
- `maxspeed` is networked to the client (`dlls/client.cpp:2076 cd->maxspeed = pev->maxspeed`),
  so client prediction inherits the same reduced cap — consistent, but consistently wrong.
- Two invention layers vs the original: (1) the `* pev->fuser1` scaling (the bug); (2) even
  setting `pev->maxspeed = GetSpeedBySlots(...)` EVERY PreThink at all — the original sets
  maxspeed only via `SetPlayerSpeed` on weapon-change/spawn, not per-frame.

**AND** our `pm_shared/pm_shared.cpp` PM_PlayerMove (line 3074) does **NOT** apply the
original's `pmove->frametime *= pmove->fuser1` time-dilation:
```cpp
pmove->frametime = pmove->cmd.msec * 0.001;   // line 3074 — no fuser1 scaling follows
PM_ReduceTimers();
```
So we slow the player the WRONG way (velocity cap) and skip the RIGHT way (frametime).

## (d) Fix recommendation (match the original)

**1. Stop scaling maxspeed by the slow factor — `dlls/player.cpp:2089-2092`.**
   Faithful (recommended): remove the per-frame maxspeed block entirely and rely on
   `SetPlayerSpeed(GetSpeedBySlots(...))` at the weapon-change/spawn sites (already wired:
   `ts_player.cpp:643`, `ts_gun.cpp:564/1413/2026/2222`, `TSInit`). Minimal (drop the multiply):
   ```cpp
   pev->maxspeed = slotBase;   // encumbrance only — NEVER × fuser1 (velocity magnitude preserved)
   ```
   Either preserves `|velocity|` in slowmo so `speed > 270` gates pass.

**2. Add the original's time-dilation in `pm_shared/pm_shared.cpp` PM_PlayerMove**, right after
   line 3074 (`pmove->frametime = pmove->cmd.msec * 0.001;`), before `PM_ReduceTimers()` —
   exactly where the original places it. This is shared code, so it fixes BOTH server and
   client-prediction movement identically:
   ```cpp
   pmove->frametime = pmove->cmd.msec * 0.001;
   // TS slow-motion time-dilation (orig PM_PlayerMove @0xf08d8): scale the movement
   // integration step by the player's slow factor. Velocity VECTOR (units/sec) is UNCHANGED
   // — only distance-per-frame shrinks — so |velocity| stays high and speed-gated kung-fu
   // (side kick > 270) still fires in slowmo. Floored: fuser1 < 0.1 freezes movement.
   if (pmove->fuser1 >= 0.1f)
       pmove->frametime *= pmove->fuser1;
   else
       pmove->frametime = 0.0f;
   ```

**⚠ ONE THING TO VERIFY at runtime before shipping fix #2:** that the engine populates
`pmove->fuser1` from `pev->fuser1` on BOTH sides (server `PM_Move` and client prediction).
The original relies on it, and our client already reads `from->playerstate.fuser1` for weapon
timers (`cl_dll/hl/hl_weapons.cpp:838`), so it should be present and `= 1.0` in normal play
(→ `frametime` unchanged). But CONFIRM `pmove->fuser1 == 1.0` during normal (non-slow) movement
before relying on it — if the halflife-updated engine leaves `pmove->fuser1 == 0`, the `< 0.1`
branch would zero `frametime` and FREEZE all movement. If it is NOT populated, mirror it in the
DLL's move setup: server side copy `pev->fuser1 → pmove->fuser1` (in `CmdStart`/`PM_Move` glue),
client side from `playerstate.fuser1`, before `PM_PlayerMove` runs.

## (e) Raw decompiles saved alongside
- `decomp/prethink_pm.txt` — `CBasePlayer::PreThink` @0x808cc, `PM_CheckParamters` @0xe9c94,
  `PM_WalkMove` @0xea26c, `PM_AirMove` @0xee434.
- `decomp/pm_playermove_setspeed.txt` — `PM_PlayerMove` @0xf08d8, `SetPlayerSpeed` @0x83068.
- `decomp/player_slowmo.txt` — `PostThink` tail @0x81610, `CBasePlayer::GiveSlowMul` @0x82960,
  `CBaseEntity::GiveSlowMul` @0x9c5b4, `SlowMotionFlow` @0x86bb0, `InterpolateSlowMotion`,
  `FixedSlowAmt`/`FixedSlowMul`.

## Confidence
- Mechanism (frametime × fuser1, velocity/maxspeed preserved): **HIGH** — direct decompile +
  offset arithmetic proving `pmove+0x20c == pmove->fuser1`, corroborated by PreThink/SetPlayerSpeed
  containing no velocity/maxspeed slow-scaling.
- Our divergence at `player.cpp:2092`: **HIGH** — the `* pev->fuser1` is explicit in source.
- `pmove->fuser1` being engine-populated on halflife-updated: **MEDIUM** — must be verified at
  runtime (flagged above); everything else is source/binary-grounded.
