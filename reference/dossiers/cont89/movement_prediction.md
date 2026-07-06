# cont.89 — Original-client movement lag vs OUR server (pm_shared + clientdata RE)

**Symptom (user, refined):** the **original TS Steam client** joined to **our
server** feels like **pure latency** — slow to *begin* moving on `IN_FORWARD`, and
mouse-look feels laggy — **even with bare kungfu (no weapons)**. The user is explicit:
it does **NOT** rubber-band / yank back. On localhost (ping≈0) this is not the wire.

**Scope of this dossier:** movement prediction only — `pm_shared` dynamics +
the `clientdata_t` reconciliation channel (`UpdateClientData`) + a per-frame
server→client flood / prediction-disable audit. **Out of scope** (parallel agent):
`ts_gun.cpp` `weapon_data` / weapon-type transmission.

All original bodies are decompiled from `reference/ts_i386.so` (Linux gcc-2.96,
symbols intact). Ghidra addr = nm addr + 0x10000. pm_shared is compiled from the
**same source** into the original **client.dll**, **mp.dll** and this `.so`, so
what the `.so` computes is *exactly what the original client predicts*.

---

## 0. TL;DR — what the evidence says

1. **clientdata reconciliation is NOT the cause for a bare player.** Field-by-field
   diff of the original `UpdateClientData` (@0xa0294) against ours
   (`dlls/client.cpp:2032`): every basic-walk field the predictor reconciles against
   — `origin, velocity, flags, view_ofs, punchangle, bInDuck, flTimeStepSound,
   flDuckTime, flSwimTime, maxspeed, waterlevel/type, weaponanim, pushmsec` — is
   written identically. The original does **not** touch `fixangle` and neither do we.
   → clientdata does not reset/disable basic-walk prediction. (One real divergence
   exists but is **stunt/powerup-scoped**, not bare-walk — see §3.)

2. **No per-frame flood or prediction-disable found in the DLL for a bare player**
   (§4). Every TS HUD send is change-gated; `SlowMotionFlow`/`UpdatePowerUpState`
   early-return with no powerup; motion-blur is event-driven; `SetPlayerSpeed →
   pfnSetClientMaxspeed` fires on **weapon events**, not per-frame; `EF_NOINTERP` is
   toggled only on animation *change*; no `stuffcmd`/`cl_predict`/`fixangle`/
   movetype-override on the live player.

3. **The one concrete bare-kungfu movement-dynamics divergence is
   `PM_Accelerate`** (§1): the original scales ground acceleration by
   **`PM_GetAccelBySpeed(maxspeed)` = 1.1** at maxspeed 330 (kungfu); **ours does
   not** (stock `accelspeed`). The original *client* predicts 1.1× ground accel; our
   server computes 1.0×. That is a **persistent ~10 %/frame prediction error while
   moving**. GoldSrc **smooths** a small persistent prediction error over several
   frames instead of snapping — which is felt as **sluggish input latency with no
   visible rubber-band**, and (because the camera *origin* is what's being smoothed)
   makes even local mouse-look feel laggy. This matches the reported symptom exactly,
   and is the recommended first fix. (Confidence: high on the RE; medium that it is
   THE dominant cause vs. a config/engine factor — a 30-second live disambiguation is
   in §5.)

---

## 1. PRIMARY: `PM_Accelerate` is missing the TS accel-by-speed multiplier

### Original `PM_Accelerate` (@0xedd08) + `PM_GetAccelBySpeed` (@0xedcc4)

```c
// PM_GetAccelBySpeed(float maxspeed)  @0xedcc4  — TS-SPECIFIC (not in stock HL)
//   returns 1.1                                   if maxspeed >= 330
//   returns (maxspeed-210)*(1/120)*0.5 + 0.6      otherwise
//   == 0.6 + (maxspeed-210)*0.0041667   → 0.6 @210 … 1.1 @330 (clamped 1.1)

// PM_Accelerate(wishdir, wishspeed, accel)  @0xedd08
if (pmove->dead) return;                       // +0xd0
if (pmove->waterjumptime != 0) return;         // +0xcc
addspeed = wishspeed - DotProduct(velocity, wishdir);
if (addspeed <= 0) return;
accelspeed = accel * frametime * wishspeed * pmove->friction;   // +0x10, +0xc4  (== stock)
if ((pmove->iuser4 & 0x20) == 0 && (pmove->flags & FL_DUCKING) == 0)   // +0x208&0x20, +0xb8 bit14
    accelspeed *= PM_GetAccelBySpeed(pmove->maxspeed);          // +0x1f4   ← THE TS MULTIPLIER
if (accelspeed > addspeed) accelspeed = addspeed;
velocity += accelspeed * wishdir;
```

Offsets verified against `pm_shared/pm_defs.h`: `frametime@0x10`, `friction@0xc4`,
`flags@0xb8` (byte 0xb9 & 0x40 = bit 14 = **FL_DUCKING** `1<<14` = 0x4000),
`iuser4@0x208` (& 0x20 = **TSPM_SLIDE**), `maxspeed@0x1f4`=500, `dead@0xd0`,
`waterjumptime@0xcc`. So the multiplier is applied **unless sliding or ducking**.

### Ours — `pm_shared/pm_shared.cpp:1062` (STOCK, no multiplier)

```c
// line 1086 — stops here, no accel-by-speed scaling:
accelspeed = accel * pmove->frametime * wishspeed * pmove->friction;
if (accelspeed > addspeed) accelspeed = addspeed;
```

### Impact (bare kungfu, maxspeed 330)

- `PM_GetAccelBySpeed(330) = 1.1`. Original client predicts ground accel **×1.1**;
  our server computes **×1.0**. Every ground-accel frame the client ends slightly
  ahead of the server → the server's authoritative `origin/velocity` lags the
  client's prediction by a small amount → **error-smoothing** (see §0.3) → sluggish
  start + laggy camera, no snap.
- This is `maxspeed`-dependent, so the same fix is correct for weapon-laden players
  (0.6→1.1 across maxspeed 210→330) — it matches whatever the client predicts.
- `PM_AirAccelerate` (@0xedfdc): original caps `wishspd` at 30 and does **NOT**
  scale — our `pm_shared.cpp:1353` is identical. ✅ No air-accel divergence.

---

## 2. SECONDARY (NOT the bare-kungfu bug): `PM_Friction` edge-gate has an extra condition

Our `PM_Friction` (`pm_shared.cpp:1275`) already carries the two TS friction mods:
- the **×0.75** drop scale (line 1330) — **matches** original @0xede08. ✅
- BUT the edge-friction gate (line 1318) reads
  `if (trace.fraction == 1.0 && pmove->friction == 1.0f)`, whereas the **original is
  just `if (trace.fraction == 1.0)`** (decompile: `if (fStack_90 == 1.0) fVar1 =
  movevars->friction * movevars->edgefriction; else fVar1 = movevars->friction;`).
  The extra `&& pmove->friction == 1.0f` was added in cont.81 to stop stunt/prone
  rolls (pev->friction 0.3) decaying too fast.

**Why it is not the bare-kungfu bug:** a normal running player has `pmove->friction
== 1.0`, so both forms are equivalent → no effect on bare walking.

**Why it is still a drop-in divergence (for later):** the original *client* predicts
friction WITH the edge-doubling even at friction 0.3 (same shared source), so during
a stunt the client predicts more ground friction than our server applies → stunt
prediction mismatch. The cont.81 "rolls decay 2.7× too fast" observation was almost
certainly our server matching the client and both being correct; gating it made the
**server** diverge from the client. **Flag for a stunt-specific playtest before
touching** — it is orthogonal to this dossier's fix and to bare-kungfu.

`PM_PreventMegaBunnyJumping` (@0xf00d4): original is `1.7 * maxspeed` cap, scale
`0.65` — **byte-identical** to our `pm_shared.cpp:2541` (BUNNYJUMP_MAX_SPEED_FACTOR
1.7, 0.65). **NOT a divergence — keep it.** So there is no top-speed-cap difference.

---

## 3. clientdata reconciliation channel — audited, CLEAN for basic walk

Original free-C `UpdateClientData(edict, sendweapons, cd)` @0xa0294 vs ours
(`dlls/client.cpp:2032`). Mapping cd offsets ↔ our field names:

| cd off | field | original source | ours | match |
|---|---|---|---|---|
| 0x00 | origin | pev->origin | pev->origin | ✅ |
| 0x0c | velocity | pev->velocity | pev->velocity | ✅ |
| 0x1c | punchangle | pev->punchangle | pev->punchangle | ✅ |
| 0x28 | flags | pev->flags | pev->flags | ✅ |
| 0x2c/0x30 | water level/type | pev->… | pev->… | ✅ |
| 0x34 | view_ofs | pev->view_ofs | pev->view_ofs | ✅ |
| 0x40 | health | pev->health | pev->health | ✅ |
| 0x44 | bInDuck | pev->bInDuck | pev->bInDuck | ✅ |
| 0x48 | weapons | pev->weapons | m_WeaponBits/weapons | ✅ (equiv) |
| 0x4c/0x50/0x54 | step/duck/swim time | pev->… | pev->… | ✅ |
| 0x58 | waterjumptime | **ROUND**(pev->teleport_time) | pev->teleport_time (no round) | ⚠ minor |
| 0x5c | maxspeed | pev->maxspeed | pev->maxspeed | ✅ |
| 0x64 | weaponanim | pev->weaponanim | pev->weaponanim | ✅ |
| 0x84 | pushmsec | pev->pushmsec | pev->pushmsec | ✅ |
| 0x18c/0x190 | iuser1/iuser2 | pev->iuser1/2 (uncond.) | pev->iuser1/2 (uncond.) | ✅ |
| **0x198** | **iuser4** | **pev->iuser4 (UNCONDITIONAL)** | **m_iPowerups, gated behind `sendweapons`** | ❌ |
| 0x19c | fuser1 | pev->fuser1 (uncond.) | *(not set here)* | ⚠ |

- The **basic-walk baseline the predictor reconciles against is identical** →
  clientdata is **not** the cause of bare-walk latency. Rules out coordinator
  hypothesis #2/#3 for a bare player.
- **`cd->iuser4` divergence (real):** original copies `pev->iuser4` **every frame,
  before the `sendweapons` gate** — this is the **stunt-flag channel** into
  `pmove->iuser4` that `TSHandle()` predicts on. Ours instead writes
  `pl->m_iPowerups` **inside** `#if CLIENT_WEAPONS && sendweapons`
  (`dlls/client.cpp:2122`), repurposing iuser4 for the DoubleFireRate weapon gate.
  Consequence: the original client's `pmove->iuser4` receives **powerup bits, not
  stunt bits** → stunt prediction (dive/slide/wall-jump/prone) is fed the wrong
  flags. **Only affects stunts/powerups, not bare walking**, but it is a genuine
  drop-in break and collides with the weapon agent's use of iuser4 — must be
  reconciled jointly (move the powerup/DoubleFire bit off iuser4, restore
  `cd->iuser4 = pev->iuser4`).
- `ROUND(waterjumptime)` and the unset `fuser1` are cosmetic for movement; note only.

---

## 4. Per-frame flood / prediction-disable audit (bare kungfu) — NEGATIVE

Coordinator hypothesis #1/#3. Findings across `dlls/`:

- **All TS HUD sends are change-gated.** `CBasePlayer::UpdateClientData` (our
  `TSUpdateClientData`, `dlls/ts_player.cpp:59`) — every one of TSHealth/TSArmor/
  TSState/TSCash/TSSpace/PwUp/TSSlowAmt/TSBTime/TSPAward/Wires sends only on a cached
  value change. For a bare, undamaged, standing player nothing changes → nothing sent.
- **`SetPlayerSpeed → pfnSetClientMaxspeed` is event-driven, not per-frame.** Call
  sites: weapon pickup/drop/deploy/attack (`ts_gun.cpp`), `BuyWeapon`, Spawn,
  gamerules "the One". No PreThink/PostThink call. `cd->maxspeed` is delta-compressed;
  stable for bare kungfu → not resent. Hypothesis #1 (maxspeed flood) **refuted**.
- **`SlowMotionFlow`/`UpdatePowerUpState`** (per-frame from PreThink) **early-return**
  with no active powerup (`m_flSlowMotionScale==0` / `m_iHeldPowerup==0`).
- **Motion-blur** (`gmsgTSMotionBlur`, MSG_PVS) is sent from `UsePowerUp` / stunt
  events, never per-frame.
- **`EF_NOINTERP`** on the player is set only inside `CTSAnimation::SetAnim`, which
  no-ops when the sequence is unchanged (`ts_animation.cpp:86`) → not per-frame.
- **No `stuffcmd`, `cl_predict`, `SET_VIEW`(self), `fixangle`, or non-WALK movetype**
  is written to the live player each frame. `fixangle` only at spawn/teleport
  (`player.cpp:1311/1365/3281`), matching stock.

→ A DLL-level netchannel flood or prediction-disable for a bare player is **not
evident**. If a flood exists it is at the engine/entity-state/config layer, not the
game DLL — see §5 to confirm.

---

## 5. Reconciliation plan + a decisive live disambiguation

### Confirm the cause first (30 s, on the original client vs our server)
- `cl_showerror 1` (or `net_graph 3`): if it **spikes prediction error** while
  running forward on flat ground with bare kungfu → it's the §1 pm_shared accel
  mismatch (error-smoothing = the "latency"). If error stays ~0 but the **net_graph
  shows large per-frame packets / choke** → it's an engine/entity-state flood (not in
  the DLL). This single test decides §1 vs §4-at-engine-level.

### Fix 1 (primary, do first) — restore the TS accel curve
In `pm_shared/pm_shared.cpp` `PM_Accelerate` (line 1086), after computing
`accelspeed`, before the addspeed cap, add the original's gate + multiplier:
```c
accelspeed = accel * pmove->frametime * wishspeed * pmove->friction;
if ((pmove->iuser4 & TSPM_SLIDE) == 0 && (pmove->flags & FL_DUCKING) == 0)
    accelspeed *= PM_GetAccelBySpeed(pmove->maxspeed);
```
Add `PM_GetAccelBySpeed`:
```c
static float PM_GetAccelBySpeed(float maxspeed) {   // @0xedcc4
    if (maxspeed >= 330.0f) return 1.1f;
    return (maxspeed - 210.0f) * (1.0f/120.0f) * 0.5f + 0.6f;   // 0.6 @210 … 1.1 @330
}
```
This is TS-specific and preserves every anniversary fix (bunnyhop cap, NaN checks,
etc.) — it only closes the ground-accel gap the original client predicts.

### Fix 2 (stunt drop-in, separate playtest) — friction edge-gate
Drop the extra `&& pmove->friction == 1.0f` at `pm_shared.cpp:1318` to match the
original (`trace.fraction == 1.0` only). **Verify stunt/roll feel against the
original client** (this reverses a cont.81 change — do it under a stunt playtest, not
as part of the bare-walk fix). Not needed for bare kungfu.

### Fix 3 (stunt/powerup prediction, coordinate with weapon agent) — cd->iuser4
Restore `cd->iuser4 = pev->iuser4` **unconditionally** (before the `sendweapons`
gate) in `dlls/client.cpp UpdateClientData` so `TSHandle()` predicts on the real
stunt flags; relocate the DoubleFireRate powerup bit the current code stuffs into
iuser4 onto a different spare clientdata field. Not the bare-walk bug, but a genuine
drop-in break — must be jointly resolved with the weapon-data agent.

### Anniversary-fix carve-out
Nothing above removes an anniversary/HL25 fix. `PM_PreventMegaBunnyJumping` stays
(original has the identical cap). Fixes 1–2 are TS-specific behaviors the original
client predicts; matching them is exactly the drop-in contract.

---

## Appendix — verified original constants / offsets
- `PM_GetAccelBySpeed`: 1.1 (≥330), else `(ms-210)/240 + 0.6`; consts .rodata
  `fc_330, fc_210, 1/120, 0.5, 0.6, 1.1`.
- `PM_Accelerate` gate: `iuser4@0x208 & 0x20 (TSPM_SLIDE)` and `flags@0xb8 bit14
  (FL_DUCKING)`; scale on maxspeed@0x1f4.
- `PM_Friction`: drop scale ×0.75 (present in ours); edge = `movevars->friction *
  movevars->edgefriction` when `trace.fraction==1.0` (original has NO
  `pmove->friction==1.0` sub-gate).
- `PM_PreventMegaBunnyJumping`: 1.7 factor, 0.65 scale (== ours == stock HL25).
- `UpdateClientData` @0xa0294: basic-walk cd fields all match; `cd->iuser4 =
  pev->iuser4` unconditional (ours: m_iPowerups, gated) is the sole movement-relevant
  divergence, and it is stunt/powerup-scoped.

---

## cont.89b — residual 10-20u divergence (after the PM_Accelerate fix)

**Task:** the PM_Accelerate accel-by-speed fix (§1) is committed, yet the original
client → our server still shows a **persistent 10–20u `cl_showerror`** while moving
(bare kungfu, maxspeed 330). Find the *residual* pm_shared divergence.

**Method:** function-by-function static diff of our `pm_shared/pm_shared.cpp` against
the original `pm_shared` in `reference/ts_i386.so` (symboled; Ghidra addr = nm+0x10000),
via `re.sh <addr>` decompiles + raw `objdump -d` where the decompile was ambiguous.
Every function on the ground-movement path the predictor reconciles was checked.

### 0. Headline: the flat-ground bare-run path is BYTE-IDENTICAL

After the accel fix, **on flat open ground our server computes exactly what the
original client predicts.** Verified identical, function by function:

| fn | orig addr | verdict |
|---|---|---|
| `PM_Accelerate` | @0xedd08 | ✅ EXACT — decompiled: `accelspeed = accel*frametime*wishspeed*pmove->friction`, then `if (!(iuser4&0x20) && !(flags&0x4000)) accelspeed *= PM_GetAccelBySpeed(maxspeed)`. Matches our committed fix byte-for-byte. |
| `PM_GetAccelBySpeed` | @0xedcc4 | ✅ 1.1 @≥330, `(ms-210)/240+0.6` below. |
| `PM_Friction` | @0xede08 | ✅ EXACT — **confirmed by raw asm** (`ede08`–`edf5d`): `control = max(speed, stopspeed)` (`flds speed; fcoms stopspeed@movevars+4; jae keep; else load stopspeed`), `drop = control * movevars->friction[*edgefriction] * pmove->friction@0xc4 * 0.75 * frametime`. Our lines 1334-1346 identical. (The Ghidra decompile's `local_a4=0` was a slot-coalescing artifact; the asm proves `control`=speed.) |
| `PM_WalkMove` | @0xea26c | ✅ backpedal ×0.8 (`fmove<0 → *0.8`), accel-then-basevel-add, `Length<1 → clear`, single-trace flat path, stepsize up/down + downdist/updist. Identical. |
| `PM_AirAccelerate` | @0xedfdc | ✅ wishspd cap 30, no accel-by-speed scale. Identical (dossier §1). |
| `PM_AirMove` | @0xee434 | ✅ identical. |
| `PM_CatagorizePosition` | @0xee760 | ✅ `velocity[2]>180 → onground=-1`; else trace origin→origin-2, `normal[2]>=0.7 → onground=ent` else -1; `waterlevel<2 && !startsolid && !allsolid → snap to endpos`; `ent>0 → AddToTouched`. Identical. |
| `PM_CheckVelocity` | @0xed0b8 | ✅ NaN velocity/origin zero + clamp to ±movevars->maxvelocity. Identical. |
| `PM_CheckParamters` (bare path) | @0xe9c94 | ✅ spd from cmd, `maxspeed = min(clientmaxspeed, maxspeed)`, FROZEN/ONTRAIN/dead zero-cmd, DropPunchAngle, v_angle = viewangles+punch, `angles[ROLL]=PM_CalcRoll(...)*4`, yaw>180 wrap. Bare-run path identical. |
| `PM_ReduceTimers` | @0xf07f8 | ✅ reduce flTimeStepSound/flDuckTime/flSwimTime by cmd.msec. |
| `PM_Duck` | @0xe9f7c | ✅ does NOTHING for a standing non-ducking runner (all blocks gated on IN_DUCK/bInDuck/FL_DUCKING/iuser4). |
| `PM_Move` wrapper | @0xf1188 | ✅ friction-reset gated `!multiplayer` (matches). |
| friction edge-trace stop dist | .rodata @0x100c90 | ✅ = 34.0 (matches our `start[2]-34`). |

**On truly flat ground the WalkMove move is a single `origin += velocity*frametime`
trace (first trace has fraction==1, returns early) — `PM_FlyMove` is never entered.**
So flat-ground horizontal error is purely velocity-driven, and velocity is byte-identical.
**⇒ a persistent 10-20u on flat ground is NOT explained by the ground dynamics.**

### 1. The remaining pm_shared divergences are AIR- / STUNT- / USE-scoped

Ranked by plausible contribution to the residual error *during real movement*:

**#1 — `PM_FlyMove` reflect-branch guard `numplanes == 1` (pm_shared.cpp:974).**
The original @0xed408 reflect condition is `movetype==MOVETYPE_WALK && (onground==-1 ||
friction!=1.0)` with **NO `numplanes==1` term** — it reflects over *all* accumulated
clip planes. Ours adds `numplanes == 1 &&` (the HL25 "acute corner" fix, comment
lines 972-973). Effect:
- For a **single-plane** air/stunt collision (`numplanes==1`) both agree — the
  overbounce math (`>0.7 → ClipVelocity(...,1)`, else `1+0.01*(1-friction)`) is identical.
- They **diverge only on multi-plane (corner) collisions** in air (`onground==-1`) or
  stunt (`friction!=1`): the original reflects on every plane, ours falls to the
  standard crease-slide else-branch. Different resulting velocity → next-frame origin
  drifts.
- Relevance: TS kungfu is air-heavy (bunnyhops, wall-brush, slope-clip while airborne),
  so this is the **top pm_shared candidate** if the measured "moving" includes the air
  phase — but it is bounded to corner/multi-plane cases, so it likely cannot alone
  produce a *steady* 10-20u on open running. Confidence: medium the RE; low it is THE
  dominant cause of a *flat-ground* number.

**#2 — `PM_Friction` edge-gate `&& pmove->friction == 1.0f` (line 1334).** Original has
NO such sub-gate (asm `edefe`: `je → friction*=edgefriction` on `trace.fraction==1.0`
only). No-op for bare walking (friction==1.0); during stunts the original client
predicts edge-doubled friction our server suppresses → stunt-only drop-in break. (Same
as dossier §2; re-confirmed by asm.)

**#3 — `PM_CheckParamters` IN_USE `maxspeed *= 1/3` (lines 2981-2984, "JoshA").** The
original @0xe9c94 has **no** IN_USE slowdown in CheckParamters (nor was one seen in the
walk block). Anniversary addition. Fires only with `+use` held on ground → not the
bare-run bug, but a genuine drop-in divergence when a player holds use.

**#4 — `PM_CheckParamters` missing iuser4 stunt handling.** The original zeroes the move
command when `iuser4 & 0x10c10` (TS stunt-freeze bits) — ours zeroes only on
`FL_FROZEN|FL_ONTRAIN|dead`. The original also gates the angle recompute on `!(iuser4 &
2)` and, when frozen, restores `angles[YAW]` from an `OldrotArray[playerindex]` (a
per-player yaw lock during stunts); ours always recomputes. All stunt-scoped (iuser4==0
for bare run).

**#5 (VERIFY) — `PM_CheckParamters` cmd-clamp gate `iuser1 > 0`.** The original's
"scale cmd down when spd>maxspeed" appears gated on `*(pmove+0x1fc) > 0` (= iuser1);
ours is unconditional. Since `PM_WalkMove` re-clamps `wishspeed` to maxspeed regardless,
the net effect on a straight run is nil; may perturb diagonal input near maxspeed. Low
confidence — decompiler comma-expression, worth a live check before acting.

### 2. Where the flat-ground residual most likely actually lives (out of pm_shared math)

Since the ground *math* is byte-identical, a steady flat-ground 10-20u points to a
**pm_shared INPUT the server feeds differently than the original client predicts** —
outside pm_shared's own code, so invisible to this diff:
- **`maxspeed` → `PM_GetAccelBySpeed` is steep** (0.6→1.1 across ms 210→330, slope
  0.00417/unit). If the server's `pev->maxspeed` and the client's predicted
  `pmove->maxspeed`/`clientmaxspeed` differ by even ~20u at any instant (SetPlayerSpeed
  timing, the `min(clientmaxspeed, maxspeed)` clamp resolving differently, or the
  "maxspd" physinfo key), the accel multiplier AND the WalkMove top-speed clamp diverge
  every frame → exactly a persistent small error. **This is the most likely real cause
  of a flat-ground number and should be checked first at runtime.**
- `pmove->friction` (pev->friction) seeding: if the server ever leaves pev->friction
  ≠ 1.0 on a bare MP player (the `!multiplayer` reset is skipped in MP), the friction
  drop and the FlyMove reflect gate both change vs the client's predicted 1.0.

### 3. Reconciliation plan

1. **Confirm the phase (30s, original client vs our server, do first).** `cl_showerror 1`:
   run *straight on flat ground without jumping* — if error stays ~0 and only spikes when
   you **bunnyhop / brush walls / slope-clip in air**, the pm_shared culprit is **#1**
   (`PM_FlyMove numplanes==1`). If error is a steady 10-20u even on flat ground with no
   air time, it is **not** pm_shared math → go to step 3 (maxspeed/friction input).
2. **Fix #1 (pm_shared, if air-phase):** drop the `numplanes == 1 &&` at `pm_shared.cpp:974`
   so the reflect runs on all planes like the original. This *reverts* the HL25 acute-corner
   behavior — it is a behavioral change, so verify air-strafe-into-corner feel; keep only if
   it closes measured air-phase error. (Preserve every other anniversary fix.)
3. **If flat-ground steady (most likely):** instrument the maxspeed seam — `live.py`/gdb
   watch `pmove->maxspeed` and `pmove->friction` on BOTH our server and the original client
   for the same bare player; any per-frame delta feeds `PM_GetAccelBySpeed` a different
   multiplier. Fix is in the server's speed/friction transmission (`dlls/`, out of this
   dossier's pm_shared scope) — hand to the clientdata/weapon-data track.
4. Fixes #2–#5 are stunt/use-scoped drop-in breaks — resolve under stunt/+use playtests,
   not as part of the bare-run residual.

### Appendix — cont.89b verified constants / offsets
- `PM_Friction` control (asm `edf3b`–`edf49`): `flds speed@[esp+0xc]; fcoms stopspeed@movevars+4;
  jae keep-speed; else stopspeed` = `max(speed,stopspeed)`. drop mul chain `edf29`–`edf59`:
  `*pmove->friction@0xc4 *0.75(-0x6df6c) *frametime@0x10`. edge stop `-0x65180` = 34.0.
- `PM_FlyMove` reflect (@0xed408 decompile): `movetype@0xdc==3 && (onground@0xe0==-1 ||
  friction@0xc4!=1.0)`; iterates all `numplanes` (no `==1` guard); overbounce `1 + 0.01*(1-friction)`.
- `PM_Accelerate` gate (asm/decompile): `iuser4@0x208 & 0x20` (TSPM_SLIDE), `flags byte@0xb9 & 0x40`
  (FL_DUCKING), maxspeed@0x1f4. friction@0xc4, frametime@0x10.
- `PM_CheckParamters` (@0xe9c94): no IN_USE 1/3 block; zero-cmd on `flags@0xb8 & 0x1001000`
  (FL_FROZEN|FL_ONTRAIN) | dead@0xd0 | `iuser4@0x208 & 0x10c10`; angle recompute gated
  `dead==0 && !(iuser4 & 2)`; `OldrotArray` yaw save/restore; cmd-scale gate `iuser1@0x1fc > 0`.

---

## cont.89c — maxspeed transmission seam + client.dll accel confirmation

**Two questions from the coordinator, both answered from the binaries.**

### A. ★ DOES client.dll's PM_Accelerate apply PM_GetAccelBySpeed? → **YES. Do NOT revert the server fix.**

pm_shared is one source compiled into `client.dll`, `mp.dll`, `ts_i386.so` — and the
client copy is confirmed to contain and CALL the multiplier:

- **`PM_GetAccelBySpeed` exists in client.dll** as a real compiled function at RVA
  `0x2b7f0` (VA 0x1002b7f0). Its constant pool sits consecutively in `.rdata` at file
  0x7d558-0x7d568 = `1.1, 0.6, 1/120, 210.0, 330.0`. Decoded body:
  `if (maxspeed >= 330.0) return 1.1; return (maxspeed-210.0)*(1/120)*0.5 + 0.6;`
  — **byte-identical** to ts_i386.so @0xedcc4 and to our reconstruction.
- **client.dll's PM_Accelerate CALLS it.** Call site file/RVA `0x2b8bc → 0x2b7f0`,
  immediately preceded by the exact same gate as the server:
  `test byte [ecx+0x208],0x20` (iuser4 & TSPM_SLIDE) → `jne skip`;
  `mov eax,[ecx+0xb8]; test ah,0x40` (flags & FL_DUCKING 0x4000) → `jne skip`;
  `mov eax,[ecx+0x1f4]` (maxspeed); `push eax; call 0x2b7f0`; then `fmul` the result
  into accelspeed. Identical to `PM_Accelerate` @0xedd08 and our fix.

**Conclusion:** the original CLIENT predicts the ×PM_GetAccelBySpeed(maxspeed) ground
accel. The committed server PM_Accelerate fix is **correct and must stay** — reverting
it makes our server compute ×1.0 while the original client predicts ×1.1 → a *constant,
everywhere* prediction error (which is exactly the newly-reported symptom, but the fix
for it is the opposite of reverting). **The accel multiplier is NOT the seam.**

### B. Maxspeed transmission — the ORIGINAL mechanism (RE)

- **`CBasePlayer::PreThink` @0x808cc does NOT touch maxspeed at all.** Full disasm of
  PreThink contains no `GetSpeedBySlots`, no `SetPlayerSpeed`, no `pfnSetClientMaxspeed`,
  no `pev->maxspeed` write. **Maxspeed is EVENT-DRIVEN only.** The complete caller set of
  `GetSpeedBySlots→SetPlayerSpeed` (from the disasm): `CBasePlayer::TSInit` (spawn),
  `PickUpWeapon`, `DropWeapon`, `CTSGun::{KnifeFire, KatanaThrow, GrenadeIdle,
  MultiplyClips, ...}` (fire/reload), `DropFlag`, `TakeFlag`, `TakeArmor`,
  `CTSGameRules::SetPlayerTheOne`. Never per-frame.
- **`SetPlayerSpeed(float)` @0x83068:** clamp to [0,330], store the clamped float in a
  member (`m@0x550`), then `pfnSetClientMaxspeed(edict, (float)(int)ROUND(clamped))` —
  the value pushed to the engine is **integer-rounded**. (Our `ts_player.cpp:550`
  matches: `(float)(int)(m_flSetMaxSpeed + 0.5f)`.)
- **`GetSpeedBySlots` @0x66bd4:**
  ```c
  float GetSpeedBySlots(CBasePlayer *p) {
      if (p->field_0x7b9 != 0 || p->field_0x548 > 80)   // 0x548 = FREE slots, init 0x51=81
          return 330.0f;                                 // empty hands (81 > 80) → flat max
      return round(0.012345679f * p->field_0x548 * 120.0f + 210.0f); // = round(210 + (120/81)*free)
  }
  ```
  free=0 → 210, free=80 → 329, free=81 (empty) → **330** (via the `>80` branch), plus an
  override flag `field_0x7b9` (→330). The result is **rounded to an int**.

**How the client's prediction gets maxspeed:** off **`pmove->clientmaxspeed`**, which the
engine seeds from the last **`pfnSetClientMaxspeed`** value — NOT from `cd->maxspeed`.
(Confirmed empirically earlier: a raw `pev->maxspeed=` write "left the client fast";
routing through `pfnSetClientMaxspeed` fixed it.) `PM_CheckParamters` then does
`pmove->maxspeed = min(clientmaxspeed, pmove->maxspeed)`. Because the original ONLY ever
sets speed via `pfnSetClientMaxspeed(round(gsbs))`, the server's `pev->maxspeed`, the
sent `cd->maxspeed`, and the client's `clientmaxspeed` are **all the same rounded integer**
→ no seam; `PM_GetAccelBySpeed` and the WalkMove clamp use identical values both sides.

### C. THE SEAM (why our code mispredicts every frame)

Our `player.cpp:2105` currently does a **per-frame RAW `pev->maxspeed = GetSpeedBySlots(this)`**.
That updates the SERVER's `pev->maxspeed` (and `cd->maxspeed`) but **never calls
`pfnSetClientMaxspeed`**, so the client's `clientmaxspeed` stays at whatever the last
*event* set (often the engine default `sv_maxspeed` 330). Result:
- **Constant `cl_showerror` everywhere:** server integrates at the loadout speed; the
  original client predicts at its stale `clientmaxspeed` → they disagree every frame, on
  ground and in air (matches the report).
- **"Loadout range feels too small":** the client stays pinned at ~330 regardless of
  loadout because it never receives the per-loadout `pfnSetClientMaxspeed` value.
- **"Kungfu feels slower":** separate value bug — our `GetSpeedBySlots` returns the
  `320.0f` null-gun default (`ts_gun.cpp:233`) for a fresh bare player instead of the
  original's **330** (empty hands = 81 free slots → `>80` branch). It also omits the
  `round(...)` and the explicit `>80→330` / `field_0x7b9→330` branches.

### D. Concrete fix

1. **Keep the PM_Accelerate server fix** (client.dll predicts it — §A). Do not revert.
2. **`player.cpp:2105` — replace the raw write with `SetPlayerSpeed(GetSpeedBySlots(this))`**
   so the value reaches the client via `pfnSetClientMaxspeed`. This closes the seam
   (both sides then use the same rounded integer). The coordinator's SetPlayerSpeed change
   was the RIGHT channel — **restore it; do not revert to the raw assignment.**
   - **Most faithful:** REMOVE the maxspeed update from PreThink entirely and update it
     only on the loadout-change events the original uses (TSInit/spawn, PickUpWeapon,
     DropWeapon, weapon fire/reload, DropFlag/TakeFlag/TakeArmor, SetPlayerTheOne — most
     already call `SetPlayerSpeed(GetSpeedBySlots)` in `ts_gun.cpp`/`ts_player.cpp`).
     Per-frame `SetPlayerSpeed` is *safe* for a constant loadout (it re-sends the same
     rounded int), so it's an acceptable interim, but the original is event-driven.
3. **Fix `GetSpeedBySlots` (`ts_gun.cpp:229`) to the original:** null-gun default → **330**
   (not 320); add the `round()`; make the empty case return 330 via the `>80` branch;
   otherwise `round(210 + (120/81)*free)`. This fixes "kungfu slower / range too small"
   (a value bug independent of the seam — it shifts felt speed even when `cl_showerror`≈0).
4. **Keep `cd->maxspeed = pev->maxspeed`** (`client.cpp:2096`) — it's the reconciliation
   baseline and is consistent once `pfnSetClientMaxspeed` sets `pev->maxspeed` too.

### E. One-line confirm test (original client → our server)
`cl_showerror 1` and run forward on flat ground: **after fix #2** the constant error must
collapse to ~0 (seam was maxspeed). If a residual only appears when brushing walls/corners
in air, that's the separate `PM_FlyMove numplanes==1` item (cont.89b §1); if the error is
still constant everywhere, re-check that `pfnSetClientMaxspeed` is actually firing (server
`developer 1` should show no per-frame maxspeed churn once event-driven).

### Appendix — cont.89c verified addresses/constants
- client.dll `PM_GetAccelBySpeed` @RVA 0x2b7f0 (const pool .rdata 0x7d558: 1.1,0.6,1/120,210,330);
  caller (PM_Accelerate) @0x2b8bc, gate `[ecx+0x208]&0x20`, `[ecx+0xb8]&0x4000`, arg `[ecx+0x1f4]`.
- `PreThink` @0x808cc: no maxspeed write. `SetPlayerSpeed` @0x83068: clamp[0,330], `pfnSetClientMaxspeed(round)`.
- `GetSpeedBySlots` @0x66bd4: `(field_0x7b9 || field_0x548>80) ? 330 : round(0.012345679*field_0x548*120+210)`;
  field_0x548 = free slots (init 0x51=81); false-branch const .rodata 0xf7620 = 330.0.

---

## cont.89d — ★ ROOT CAUSE of the constant 10-20u: unsent `cd->fuser1` (frametime seam)

The residual constant error (robust to accel-multiplier on/off and to maxspeed, present on
ground AND air, on flat ground where pm_shared is byte-identical) is a **frametime seam**:
our server never transmits `cd->fuser1`, so the original client predicts with the wrong
per-frame `frametime`.

### The mechanism (RE, definitive)

**Original `PM_PlayerMove` @0xf08d8 scales the ENTIRE move by fuser1** (asm f0911-f094e):
```c
frametime = 0.001f * cmd.msec;          // f0911-f0923 (msec@byte, ×0.001 double const)
if (pmove->fuser1 >= 0.1f)              // f0928 flds fuser1@0x20c; fcomps 0.1; jne→0.0
    frametime *= pmove->fuser1;         // f093b-f093e
else
    frametime = 0.0f;                   // f0948 (freeze below 0.1)
```
`frametime` feeds EVERY integration step (`origin += velocity*frametime`, PM_Accelerate,
PM_AirAccelerate, gravity) — so any fuser1 mismatch scales the whole move, on ground and
in air, independent of accel/maxspeed. **Exactly the reported signature.**

**Original `UpdateClientData` @0xb0294 sends fuser1 to the client** (decompile):
`*(cd + 0x19c) = *(pev + 0x2d4)` → **`cd->fuser1 = pev->fuser1`, unconditional.** (Flagged
as unsent in cont.89b §3.)

**Ours does NOT.** `grep 'cd->fuser1' dlls/client.cpp` → nothing (only an unrelated
`item->fuser1` weapon-data field). So:
- **Our SERVER:** `pev->fuser1 = 1.0` (PreThink sets it if 0), `pmove->fuser1 = 1.0` →
  our (guarded) pm_shared → `frametime *= 1.0` = `msec*0.001`. Moves normally.
- **The ORIGINAL CLIENT (predicting):** `pmove->fuser1 = cd->fuser1`, but we never write
  `cd->fuser1` → the client predicts with fuser1 ≈ 0 (unset) / stale, so its
  `PM_PlayerMove` takes the `< 0.1 → frametime = 0` path (or a scaled-wrong dt) → its
  predicted per-frame displacement diverges from the server's every frame → **constant
  prediction error, bounded by the interp/smoother window (~10-20u), 0 when stopped.**

The coordinator's earlier fuser1 check verified the SERVER side (`pev->fuser1 = 1.0`, our
guard) but not that **the original client never receives fuser1** — that transmission gap
is the seam. It survived the accel/maxspeed tests because frametime is UPSTREAM of both.

### Fix

1. **PRIMARY — send it.** In `dlls/client.cpp UpdateClientData`, add (unconditional, before
   the `sendweapons` gate, matching @0xb0294):
   ```c
   cd->fuser1 = pev->fuser1;
   ```
   With `pev->fuser1 = 1.0` for a normal player, the original client then predicts
   `frametime = msec*0.001*1.0` = the server's frametime → **error resolves**. It also makes
   slow-motion (fuser1 0.1..1.0) predict correctly on the client instead of desyncing.
2. **SECONDARY — align the pm_shared guard to the original** (drop-in hygiene): change
   `pm_shared.cpp:3112-3118` from the extra `if (fuser1 > 0){...}` wrapper to the original's
   flat form: `if (pmove->fuser1 >= 0.1f) pmove->frametime *= pmove->fuser1; else
   pmove->frametime = 0.0f;`. Safe because PreThink guarantees `pev->fuser1 >= 0.1` for a
   live player (0→1.0) and we now transmit it; this matches what the original CLIENT
   computes exactly (the client freezes below 0.1, so the server must too for slow-mo parity).
   *(If you keep the `>0` wrapper, the bare fix still works since fuser1=1.0 on both — but the
   client-vs-server slow-mo edge would then diverge.)*

### Confirm test (original client → our server)
After fix #1, `cl_showerror 1` and run: the constant 10-20u must collapse to ~0 on ground
AND in air. (If it doesn't, the client's `pmove->fuser1` is seeded from the entity_state
rather than clientdata — then also ensure `pev->fuser1` is in the transmitted entity delta;
but `cd->fuser1` is the channel the original uses.)

### Also — SEPARATE confirmed bug: `cd->iuser4 = m_iPowerups` collides with pm_shared masks
Not the bare-kungfu cause (m_iPowerups=0 for bare — and the accel-off test rules it out:
if `m_iPowerups & 0x20` set TSPM_SLIDE, accel-off would have fixed it), but a real drop-in
break for POWERUP states: **TS_PWUP_GRENADE = 0x20 == TSPM_SLIDE**, and TS_PWUP_KUNGFU(0x4)/
SLOWPAUSE(0x8)/DBLFIRE(0x10)/GRENADE(0x20) all fall inside **TSPM_STUNT_MASK (0x030C3C)**. So
when a powerup with one of those bits is active, the original client (reading
`cd->iuser4 = m_iPowerups`) trips a pm_shared stunt/slide branch the server doesn't (server
`pmove->iuser4 = pev->iuser4` = stunt flags). **Fix:** restore `cd->iuser4 = pev->iuser4`
(the stunt channel the original uses) and move the DoubleFireRate powerup gate off iuser4
onto a spare clientdata field (e.g. an unused fuserN/iuser2 bit). Coordinate with the
weapon-data track (cont.89b §3, Fix 3).

### Appendix — cont.89d verified addresses
- `PM_PlayerMove` @0xf08d8: `frametime = 0.001*msec` (f0911-f0923), `frametime *= fuser1@0x20c`
  if `fuser1 >= 0.1` (f0928 fcomps 0.1, f093e fmul) else `= 0.0` (f0948).
- `UpdateClientData` @0xb0294: `cd[0x19c] (fuser1) = pev[0x2d4] (fuser1)`, unconditional.
- Ours: `client.cpp` writes no `cd->fuser1`; `pm_shared.cpp:3112` has an extra `if(fuser1>0)` wrapper.
- iuser4 masks: TSPM_SLIDE 0x20, TSPM_STUNT_MASK 0x030C3C (TS_pm_shared.h); TS_PWUP_ bits ts_gamerules.h.

---

## cont.89e — pm_shared exactness batch (jump velocity + CheckParamters + friction + flymove)

Ranked by user-visible impact. All are shared `pm_shared` → one fix covers hl.so AND client.so.

### 1. ★ HIGH — PM_Jump normal-jump velocity is 45u; the original is **35u** (`sqrt 56000`)

**The concrete new "0.5-3u every jump" bug.** Original `PM_Jump` @0xe9944 sets the ground-jump
velocity from two `.rodata` doubles (asm e9c3b/e9c67, `fldl; fsqrt`):
- superjump (canhighjump): `sqrt(_UNK_0x100bc8)` = `sqrt(160000.0)` = **400** (100-unit).  ✅ ours matches.
- normal (canhighjump==0): `sqrt(_UNK_0x100bb8)` = `sqrt(56000.0)` = **236.6432** (= `sqrt(2*800*35)`, a **35-unit** jump).

**Ours (`pm_shared.cpp:2720,2725`) uses `sqrt(2 * 800 * 45.0)` = 268.3282 (45-unit)** in BOTH the
ducking-non-super and standing branches. The original has NO ducking-specific jump velocity — every
non-superjump is `sqrt(56000)`. (The nearby `.rodata` 112000.0 = 70-unit is used by a DIFFERENT
function, NOT PM_Jump — verified no `fldl` of it in PM_Jump's range.)

pm_shared is shared, so our client+server AGREE at 45u (no self-error) but both are **wrong vs the
original**, so in the drop-in cross-test the original client predicts a 236.6 launch while our server
computes 268.3 → a ~31.7 u/s vertical-velocity mismatch that integrates to a **~0.5-3u transient
prediction error per jump** (and the jump feels ~10u too high). 

**FIX:** both `sqrt(2 * 800 * 45.0)` → `sqrt(2 * 800 * 35.0)` (== `sqrt(56000.0)` == 236.6432).

Minor (LOW): on the stunt-mask early-return (`iuser4 & PRONE|SLIDE|DIVE|walljump|...`), the original
does a PLAIN `return` (does NOT set `oldbuttons |= IN_JUMP`); ours sets it. Affects only jump-buffering
out of a stunt — leave for a stunt exactness pass. (`PM_PreventMegaBunnyJumping` 1.7/0.65 and its
call order — once, before velocity set — are byte-identical; not a divergence.)

### 2. MED — PM_CheckParamters: drop the anniversary IN_USE 1/3, add the iuser4 stunt handling

Original `PM_CheckParamters` @0xe9c94 (verified cont.89c, re-confirmed):
- **NO IN_USE `maxspeed *= 1/3`.** Our `pm_shared.cpp:2981-2984` ("JoshA" anniversary) has it — remove
  it. The original client (predicting) never slows on +use, so our server's 1/3 slowdown mispredicts
  every frame +use is held on ground. (No +use velocity-scale exists elsewhere in the original
  PM_PlayerMove either — safe to just delete.)
- **Zero-cmd condition** is `flags & 0x1001000 (FL_FROZEN|FL_ONTRAIN) || dead || (iuser4 & 0x10c10)`.
  Ours (`~line 3012`) only tests `FL_FROZEN|FL_ONTRAIN|dead` — add `|| (pmove->iuser4 & 0x10c10)`
  (the stunt-freeze bits). Now relevant since iuser4 carries real stunt flags (cont.89d fix).
- **Angle recompute gate** is `dead == 0 && !(iuser4 & 0x2)`. Ours (`line 3025`) is only `dead == 0`
  — add `&& (pmove->iuser4 & 0x2) == 0` so a view-locked stunt state freezes the angles.
- **OldrotArray yaw lock** (LOW/defer): when it recomputes angles the original stores `angles[YAW]`
  into a per-player `OldrotArray[player_index]` and, on the frozen/locked branch, RESTORES YAW from it
  (keeps your facing through a stunt). Ours has no equivalent. Needs a `static int OldrotArray[32]`
  indexed by the player; verify the index source (`*(int*)pmove`) before implementing. Stunt-yaw
  polish — lower priority than the mask/gate fixes above.

### 3. MED-LOW — PM_Friction: remove the extra `&& pmove->friction == 1.0f` (reverts cont.81)

Original `PM_Friction` @0xede08 edge-gate is `if (trace.fraction == 1.0) friction *= edgefriction`
with **no `pmove->friction == 1.0` sub-gate** (confirmed in raw asm cont.89b: `edefe: je` on
`trace.fraction==1.0` only). Ours (`pm_shared.cpp:1334`) adds `&& pmove->friction == 1.0f` (cont.81,
to stop rolls decaying fast). **VERDICT: remove it** — change to `if (trace.fraction == 1.0)`.
Rationale: the cont.81 "rolls decay 2.7× too fast" was a self-consistent-test misread — the ORIGINAL
client predicts edge-doubled friction even at `pev->friction` 0.3, so our gate made the SERVER diverge
from the client during rolls. Now that stunt iuser4 is correct (cont.89d), removing it restores parity
(both sides edge-double). It IS a behavioral revert of cont.81 — confirm roll feel matches the ORIGINAL
client (expect the original's own faster decay, which is correct), not the earlier "feels too fast" note.

### 4. LOW / KEEP — PM_FlyMove `numplanes == 1` (M1): keep the anniversary carve-out, SKIP

The original reflect branch (@0xed408) runs on ALL planes; ours guards `numplanes == 1`. **But
single-plane air collisions (the common case) already AGREE** — they diverge only on multi-plane
acute-corner air/stunt collisions (rare). The user confirms the air/corner residual matches parity
(within GoldSrc prediction noise). **VERDICT: KEEP the HL25 `numplanes==1` fix (anniversary carve-out),
SKIP the revert.** Strict byte-exactness would remove the guard, but the drop-in payoff is nil and the
guard prevents a real acute-corner stuck bug. Revisit only if a specific air-corner desync is reported.

### Appendix — cont.89e verified constants
- PM_Jump @0xe9944: normal `sqrt(56000.0)`=236.6432 (`.rodata` 0x100bb8), super `sqrt(160000.0)`=400
  (0x100bc8); PM_PreventMegaBunnyJumping once before velocity set; 112000.0@0x100bc0 NOT used here.
- PM_CheckParamters @0xe9c94: no IN_USE 1/3; zero-cmd `flags&0x1001000 | dead | iuser4&0x10c10`;
  angle gate `dead==0 && !(iuser4&2)`; OldrotArray yaw save/restore.
- PM_Friction @0xede08: edge-gate `trace.fraction==1.0` only (no `friction==1.0` sub-gate).
