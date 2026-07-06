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
