# CTSStunt DOSSIER — getup / walljump-backflip / double-jump landing

Source of truth: `reference/ts_i386.so` (typed `re` project). Decompiles captured in
`scratchpad/CTSStunt_all.txt`, `GetDirection.txt`, `TSWallJump.txt`.

## Offset legend (recurring in the decompile)
- `this->field_0x40` = `m_pPlayer` (CBasePlayer*). `this->pev` = **m_flStuntTime** (+0x00, decompiler mislabels).
- `*(int*)this->field_0x40 + N` = **pev + N**: origin@8, velocity@0x20 (z@0x28), v_angle@0x74 (x/pitch@0x74,y@0x78,z@0x7c),
  gravity@**0x120**, friction@**0x11c**, view_ofs@0x174, button@0x180, flags byte@0x1a5 (bit0x02=FL_ONGROUND(0x200), bit0x40=FL_DUCKING(0x4000)), edict@0x208, **iuser4@0x250**, deadflag@0x244.
- `this->field_0x40 + N` = **CBasePlayer member + N** (NOT pev): +0x78 = stand-up latch, +0x2a0 = landing-direction index, +0x548 = height/fall counter (Can-Dive gate), +0x814 = m_afButtonPressed (0x815&0x40 = IN_ALT1 0x4000), +0x7b9 = no-stunt/god byte, +0x28c = m_TSAnim.
- FP consts pre-resolved by value: fc_28=28, fc_16=16, fc_48=48, fc_54=54, fc_80=80, fc_neg370=-370, fc_neg680=-680;
  0x3f800000=1.0, 0x3f666666=0.9, 0x3f400000=0.75, 0x3f000000=0.5, 0x3e99999a=0.3, 0x3e800000=0.25; dc_0_35=0.35.
- **iuser4 gravity vs friction**: GoDive writes gravity@0x120 AND friction@0x11c separately; several of our funcs confuse the two.

---

## Q1 — GETUP from prone (CTSStunt::Play @0x8a014)

### What the original does (Play, decompile lines 92–129, the on-ground / not-ladder branch)
The **entire** getup is ONE block, run at the top of the on-ground stunt section, BEFORE the Handle* dispatch:

```
if ( m_flRollTime != 0                                   // a roll/prone window is/was running
     && ( m_flRollTime < 0.01                            // ... it has expired
          || m_pPlayer[+0x78] != 0 ) )                   // ... OR the stand-up latch is set
{
    iuser4 &= ~0x40000;   // clear ROLL
    iuser4 &= ~0x80000;   // clear (undefined spare bit)
    iuser4 &= ~0x4;       // clear PRONE      <<< PRONE cleared HERE
    iuser4 &= ~0x1;       // clear STOP
    iuser4 &= ~0x2;       // clear (spare)
    m_flRollTime = 0;
    m_pPlayer[+0x4dc] = 0;
    pev->gravity = 1.0;   // @0x120
    // view height, SINGLE assignment, keyed on FL_DUCKING:
    view_ofs = (flags & FL_DUCKING) ? (0,0,16) : (0,0,28);
    // *** NO CTSAnimation::Animate call in this region ***
}
```

Key facts:
1. **Trigger** = `m_flRollTime` timer expiring (`< 0.01`, it floors at 0.001 in the decrement) **OR** the CBasePlayer
   `+0x78` latch being non-zero. It is NOT an IN_JUMP/IN_ALT1/IN_DUCK *edge* here.
2. **PRONE (0x4) is cleared in this same block**, together with STOP/ROLL — atomically, before anything animates.
3. **No `Animate(0x2e)` is dispatched in Play.** The getup ANIMATION (0x2e) is played earlier, at *landing* time
   (`Landing()` roll branch, below). The standing pose simply resumes once PRONE is cleared.
4. **View height is a single write**: `FL_DUCKING ? 16 : 28`. There is no intermediate "stage-1 = 16, stage-2 = 12/28".

### Two-stage timer / durations (where each `m_flRollTime` is armed)
- `Landing()` ROLL branch: **m_flRollTime = 0.5625** + `Animate(0x2e)` (dive→roll→feet).  ← the getup anim
- `HandleSlide()` deliberate-prone branch: **m_flRollTime = 0.75**, sets `m_pPlayer[+0x78] = 1`, `Animate(0x29)` (prone).
- `HandleWheel()` roll: m_flRollTime = 0.375.
- `HandleWallJump()`/`HandleWallClimb()`: m_flRollTime = **0.2625**.
So the "two-stage" feel is: (stage A) Landing/Slide arms the timer + plays the roll/prone anim; (stage B) Play detects the
timer expired (or the +0x78 latch) and does the *state* cleanup (clear PRONE + set standing view). The animation and the
state-clear are in DIFFERENT functions, never both in Play.

### The `+0x78` latch (UNRESOLVED — flag for live verify)
Only writer found in CTSStunt is `HandleSlide` prone branch (`=1`); no clear in CTSStunt. It gates the *early* getup path.
Its exact set/clear lifecycle (probably cleared in CBasePlayer spawn/PostThink) needs a live trace — do not invent it.

### DIFF vs our `dlls/ts_stunt.cpp` (lines ~157–186)
- ❌ **We invented a two-stage getup and call `Animate(0x2e)` from Play (line 169) with PRONE still set.** The original
  never animates in Play. Dispatching 0x2e while PRONE(0x4) is still set is exactly the case Region B of `Animate` SWALLOWS
  → this is almost certainly the "getup doesn't play / sticks" bug.
- ❌ Our stage-1 gate is `m_afButtonPressed & (IN_JUMP|IN_ALT1|IN_DUCK)`; original gate is `m_flRollTime<0.01 || pl[+0x78]!=0`.
- ❌ Our view is `IN_DUCK ? 12 : 28` (two-stage 16 then 12/28); original is a single `FL_DUCKING ? 16 : 28`.
- ✅ Bit-clear set matches (PRONE|STOP|ROLL + spares) and `gravity = 1.0` matches.

### RECONCILE PLAN (Q1)
1. **Collapse to one block** (behavior-preserving toward the RE): remove the stage-1 branch entirely; **delete the
   `Animate(0x2e)` call from Play**. Keep only the single cleanup: `iuser4 &= ~(ROLL|PRONE|STOP|0x2|0x80000);
   m_flRollTime=0; gravity=1.0; view_ofs.z = (flags&FL_DUCKING)?16:28;`.
2. Gate it as the original: `if (m_flRollTime != 0 && (m_flRollTime < 0.01f || m_pPlayer-member-0x78 != 0))`. Map `+0x78`
   to the correct CBasePlayer member; if unknown, **live-trace it first** (needs-playtest).
3. Ensure the getup ANIMATION stays where the original puts it: `Landing()` roll branch already has `Animate(0x2e)` — keep it.
   (needs-playtest to confirm the 0x2e no longer gets swallowed once Play stops re-dispatching it.)

---

## Q2 — WALLJUMP BACKFLIP (CheckForWallJump @0x8ac14 → GoWallJump @0x8a7d4; velocity in pm_shared TSWallJump)

### Gate: HandleStunt (@0x8b000, lines 816–835)
```
if ( m_flStuntTime <= 0 && m_flRollTime <= 0 && (iuser4 & 0xc30) == 0 ) {
    ...
    if ( (button & IN_JUMP(0x2)) == 0 ) {                 // JUMP must NOT be held
        if ( m_afButtonPressed & IN_ALT1(0x4000) ) {     // +alt1 EDGE (pl[0x815]&0x40)
            if (CheckForWallJump()) { GoWallJump(); return; }
            if (CheckForWallDive()) { iuser4 |= WALLDIVE; GoDive(); return; }
        }
    } else { if (CheckForWallClimb()) GoWallClimb(); }    // JUMP held → climb path instead
}
```
So walljump = **+alt1 edge while NOT holding jump**. (Our `HandleStunt` matches this exactly.)

### CheckForWallJump (@0x8ac14) — the exact wall gate
```
if (!CanDiveHigh()) return false;                        // height/airborne gate (pl[+0x548] test)
UTIL_MakeVectors(pev->v_angle);
end = origin + 48 * v_forward;                            // fc_48, trace FORWARD 48u
UTIL_TraceLine(origin, end, IGNORE_MONSTERS(1), self, &tr);
if (!(tr.flFraction < 1.0)) return false;                // must hit within 48u
if (!(-0.5 < tr.vecPlaneNormal.z < 0.5)) return false;   // wall roughly VERTICAL (|nz|<0.5)
if (!(pev->velocity.z > -370.0)) return false;           // fc_neg370: not falling faster than 370
return true;                                             // NO SOLID_BSP check (unlike wall-CLIMB)
```
- **No explicit `airborne`/IN_JUMP/IN_ALT1 test inside CheckForWallJump** — airborne is implied by `CanDiveHigh()`
  (height counter) and the caller's button gate. `CanDiveHigh` = `pl[+0x7b9]!=0 ? true : (0x51 - SlotBonus - pl[+0x548]) < 0x2a`.
- `CheckForWallDive` is identical but traces **backward** (`-48*v_forward`) and requires `!(flags&FL_DUCKING)`.
- `CheckForWallClimb` traces **54u** forward, uses `IGNORE_MONSTERS(0)` + a hit-entity/**BSP** check, and needs the
  double-jump 0.3s window (`m_flWallClimbNextTime`).

### GoWallJump (@0x8a7d4) — arming (NO velocity set here)
```
UTIL_MakeVectors(v_angle);  m_vecStuntDir = -v_forward;   // away from wall
iuser4 &= ~0x100000(WHEEL_DONE);
iuser4 |= 0x4(PRONE) | 0x1000(WALLJUMP) | 0x10000(WALLJUMP_A);
iuser4 &= ~0x20000(WALLCLIMB_A);
view_ofs = 0;
Animate(0x37);                                            // WALLJUMP anim = the backflip
pev->friction = 0.75;    // @0x11c  <<< FRICTION, not gravity; gravity is UNTOUCHED
m_flWallClimbFuel += 0.35;
m_flStuntTime = max(m_flWallClimbFuel, 0.5);
TS_SendNextStunt(round(10*m_flStuntTime));
EMIT "player/airwheel.wav";
if (10*timescale + rand(0,10) > 5) TS_MotionBlur(2);      // fc_5 threshold
```

### The launch VELOCITY (pm_shared `TSWallJump` @0xe9444, fires on iuser4 bit 0x1000)
```
pmove->onground = -1;                                     // leave ground
if (pmove_iuser4 & 0x1000 /*WALLJUMP*/) {
    speed = 210.0;
    for (i=0..1) pmove->velocity[i] = -movedir[i] * 210.0;   // horizontal 210 AWAY from wall
    pmove->velocity.z = SQRT(56000.0);  // = 236.64            // vertical up
    iuser4 &= ~0x1000;
}
PM_FixupGravityVelocity(); FixVelocity();
```
- **Horizontal launch = 210 u/s along `-forward` (backward, away from the faced wall).**
- **Vertical launch = √56000 ≈ 236.6 u/s up.**  (const at ghidra 0x110bb8 stored as double 56000.0, cast to float, sqrt'd.)
- Wall-CLIMB counterpart `TSWallJumpClimb` (@0xe94e0, bit 0x8000): only `velocity.z = √112000 ≈ 334.66`, no horizontal.

### DIFF vs our source (GoWallJump lines 364–384; TSWallTrace/CheckForWallJump 556–578)
- ❌ **`GoWallJump` sets `pev->gravity = 0.75f` (line 376) — WRONG. Original sets `pev->friction = 0.75` and leaves
  gravity alone.** Fix: `pev->friction = 0.75f;` remove the gravity write.
- ✅ Everything else in GoWallJump matches (dir=-forward, bits, Animate 0x37, fuel+=0.35, StuntTime, sounds, motionblur@5).
- ✅ CheckForWallJump matches (CanDiveHigh, 48u forward, |nz|<0.5, velz>-370, no BSP).
- ⚠️ **Verify the pm_shared side** (`pm_shared`/`pm_ts`): the backflip velocity (210 horiz, √56000≈236.6 vert; climb √112000≈334.7)
  must be present there — GoWallJump only sets bits; the impulse is 100% prediction-side.

### RECONCILE PLAN (Q2)
1. `dlls/ts_stunt.cpp` GoWallJump: change `pev->gravity = 0.75f;` → `pev->friction = 0.75f;` (behavior-preserving fidelity fix).
2. Confirm/port pm_shared `TSWallJump`: `vel.xy = -forward*210; vel.z = sqrtf(56000.f) (≈236.64)` on the 0x1000 bit;
   `TSWallJumpClimb`: `vel.z = sqrtf(112000.f) (≈334.66)` on the 0x8000 bit. (needs-playtest to confirm arc height.)

---

## Q3 — DOUBLE-JUMP LANDING: prone (fwd/left/right) vs feet (backward)

### Where direction is computed & stored (CTSStunt::Landing @0x8bbf4, and HandleSlide/HandleWheel)
`Landing()` runs from `HandleDive()` every frame; it acts only when `FL_ONGROUND && (iuser4 & DIVE 0x10)`:
```
if ( (button & 0x4006 [IN_JUMP|IN_DUCK|IN_ALT1]) == 0 ) {   // ROLL vs SLIDE is BUTTON-gated, not direction
    iuser4 &= ~DIVE(0x10);  iuser4 |= SLIDE(0x20);  view_ofs = 0;
    dir = GetDirection(m_vecStuntDir/velocity, v_angle, 0.6, 0.3);
    m_bDirForward = (dir != 2);                              // CTSStunt+0x2c
    m_pPlayer[+0x2a0] = dir;                                 // <<< landing direction index (0..3)
    // NOTE: original SLIDE branch does NOT Animate here — HandleSlide picks the pose next frames
} else {
    iuser4 &= ~DIVE; iuser4 |= ROLL(0x40000) | STOP(0x1);
    gravity = 0.5; view_ofs = 0; m_flRollTime = 0.5625; Animate(0x2e);  // roll to feet
}
```

### GetDirection (@0x5b008, TS_animation.cpp) — the exact direction test
```
MakeVectors( (0, angles.yaw, angles.roll) );               // yaw-only basis
r = dot(v_right,   moveDir);
f = dot(v_forward, moveDir);
if (r < -0.6)      return 0;   // LEFT
if (r >  0.6)      return 1;   // RIGHT
if (f < -0.3)      return 3;   // BACKWARD   <<< the user's "backward"
else               return 2;   // FORWARD
```
→ **directions: 0=LEFT, 1=RIGHT, 2=FORWARD, 3=BACKWARD** (confirmed; thresholds ±0.6 lateral, −0.3 fwd/back).
`m_bDirForward = (dir != 2)` (true for L/R/back), and the raw index is stashed in `m_pPlayer[+0x2a0]`.

### Where prone-vs-feet actually diverges by direction
Within CTSStunt the roll/slide choice is **button-gated (0x4006), not direction-gated** — both fwd and back can slide.
The **prone (fwd/L/R) vs on-feet (back) selection lives in `CTSAnimation::Animate`**, keyed on `m_pPlayer[+0x2a0]`
(the index Landing/HandleSlide store). HandleSlide feeds it further pose codes:
- `HandleSlide` (@0x8b9f8): no move keys → `Animate(3)`; +moveright early → `Animate(0x2b)`; +moveleft early → `Animate(0x2a)`;
  jump/duck/alt1 held early → deliberate **prone**: `Animate(0x29)`, `m_flRollTime=0.75`, `m_pPlayer[+0x78]=1`, view=28.
So the swap contract for "backward lands on feet" is: `Animate` sees `dir==3` and selects the feet/standup pose instead of
the prone dive-land pose. **The decisive branch is in `CTSAnimation::Animate`, not CTSStunt.**

### DIFF vs our source (Landing 523–550; HandleSlide 468–476)
- ⚠️ **Our `Landing()` SLIDE branch calls `Animate(0x29)` (line 536); the original does NOT animate in that branch** —
  it only sets SLIDE + stores dir; `HandleSlide` chooses the pose (3 / 0x2a / 0x2b / 0x29) from buttons+direction the next frame.
- ❌ **Our `HandleSlide()` is gutted** (just `Animate(0x29); friction=1.0`). The original HandleSlide is the button+direction
  pose selector described above (Animate 3/0x2a/0x2b, and the deliberate-prone branch that sets `+0x78`, `m_flRollTime=0.75`).
  This gap is why direction-dependent landing (and the +0x78 getup latch) don't behave like the original.
- ✅ `GetDirection` call args match (`velocity, v_angle, 0.6, 0.3`) and `m_bDirForward = (dir!=2)` matches.
- ✅ Landing ROLL branch matches (0x4006 gate, ROLL|STOP, 0.5625, Animate 0x2e, gravity 1.0 — original sets gravity 0.5 in the
  roll block THEN Play resets to 1.0; **minor: original roll branch gravity=0.5**, ours sets gravity=1.0 at the shared tail. verify).

### RECONCILE PLAN (Q3)
1. **Rebuild `HandleSlide()`** to the original: gate on FL_ONGROUND; IN_BACK → gravity 1.0; then
   `if ((button&0x4006)==0 || m_flSlideTime>=0.01)` → pose select `Animate(3)` / moveright`Animate(0x2b)` / moveleft`Animate(0x2a)`
   (each sets m_vecStuntDir=v_forward, m_bDirForward=0, m_pPlayer[+0x2a0]=2); `else` (jump/duck/alt1 early) → deliberate prone:
   `iuser4|=STOP; iuser4&=~SLIDE; view=28; m_flRollTime=0.75; m_pPlayer[+0x78]=1; Animate(0x29); gravity=1.0`. (needs-playtest.)
2. **Remove the `Animate(0x29)` from `Landing()` SLIDE branch** — defer all slide posing to HandleSlide (fidelity).
3. Keep GetDirection as-is; ensure `m_pPlayer[+0x2a0]` is the member `CTSAnimation::Animate` reads for dir==3 (backward→feet).
4. The concrete prone-vs-feet-by-direction branch is in `CTSAnimation::Animate` — **decompile its dive-land region
   (`re.sh CTSAnimation`) or live-trace it** to lock the `dir==3 → standing/feet` mapping. (needs-playtest / RE.)

---

## Summary of concrete divergences to fix in `dlls/ts_stunt.cpp`
1. Play getup: delete the invented `Animate(0x2e)` + two-stage; single block clears PRONE and sets `view = FL_DUCKING?16:28`. (root of getup-swallow)
2. GoWallJump: `pev->gravity=0.75` → `pev->friction=0.75` (no gravity change).
3. HandleWallJump/HandleWallClimb: original sets **gravity=0.5 + friction=1.0** (we set friction=0.5, no gravity). m_flRollTime 0.2625.
4. Landing SLIDE branch: remove `Animate(0x29)`; rebuild the real `HandleSlide()` pose selector (Animate 3/0x2a/0x2b/0x29 + `+0x78` latch).
5. Walljump impulse is pm_shared: horiz `-forward*210`, vert `√56000≈236.6`; climb vert `√112000≈334.7`.

================================================================================
# FOLLOW-UP (playtest: double-jump does nothing / walljump no backflip / getup no anim)
================================================================================
New decompiles captured: `Duck.txt` (CBasePlayer::Duck @0x827fc), `PostThink_roll.txt` (CBasePlayer::PostThink
GoRoll caller), `Animate.txt` (CTSAnimation::Animate @0x5b470), `GetDirection.txt`, `TSWallJump.txt`.
Call-site map (objdump): GoDive ← HandleStunt(0x8b18c)+HandleWallClimb(0x8b7d3); GoWallJump ← HandleStunt(0x8b0b8)+
HandleWallClimb(0x8b7a8); **GoRoll ← CBasePlayer::Duck(0x82826)+CBasePlayer::PostThink(0x81962) — NOT from CTSStunt.**

## Q1 — DIVE INPUT TRIGGERS (there are TWO prone moves; neither is a 2nd-IN_JUMP)

### (i) The dive-ROLL = **IN_DUCK EDGE** → `CBasePlayer::Duck` → `CTSStunt::GoRoll` (@0x827fc, TS_player.cpp)
```
void CBasePlayer::Duck() {
    if ((m_afButtonPressed & IN_DUCK/*4*/) && GoRoll(&m_stunt, 0x30) != 0) return;   // <<< DUCK EDGE fires the roll
    if ((pev->button & IN_DUCK) && !(pev->flags & FL_DUCKING/*0x4000*/))
        m_TSAnim.Animate(this, 5);                                                    // crouch idle
}
```
`GoRoll` (@0x8cfe8) rolls only if: speed(x,y) ≥ 80 (fc_80), `!(iuser4 & 0x130c30)` (no active stunt), `m_flRollTime<=0`,
not on ladder, `m_flStunCooldown<=0`, and `m_flRollNextTime >= time` (a fresh window); else it stamps
`m_flRollNextTime = time + 0.3` and returns 0. On success: `m_vecStuntDir = normalize(velocity)`;
`dir = GetDirection(...)` (0=L,1=R,2=Fwd,3=Back); `gravity = 0.3 (0x3e99999a)`; `iuser4 |= 0x40005` (ROLL|STOP|PRONE);
`view_ofs=0`; `m_flRollTime = 0.75`; `Animate(0x2f)` (roll); land.wav; `m_flRollNextTime = 0`; MotionBlur.
**This is the direction-dependent prone/feet move** (Q3-of-first-dossier). It is DUCK-triggered.

### (ii) The superhero DIVE = **IN_ALT1 EDGE** → `HandleStunt` → `GoDive` (already covered)
`altPressed(m_afButtonPressed & IN_ALT1 0x4000) && !(iuser4 & 0x130000) && (onGround || recent)`; `recent` =
`velz≥-680 && time-m_flLastStunt ≤ 0.7/timescale`. `speed≥80 → GoDive` else `GoBulletTime`. No jump involved.

### (iii) The 2nd-IN_JUMP is used ONLY for wall-CLIMB
`CheckForWallClimb` (@0x8ae6c): if NOT already climbing, requires `m_afButtonPressed & IN_JUMP` (`((uint*)pl)[0x205]&2`
= m_afButtonPressed & 0x2) AND being inside the 0.3s window (`m_flWallClimbNextTime`). First jump-press arms the window
(returns false), a 2nd press within 0.3s + a BSP wall at 54u passes → GoWallClimb.

### ⇒ Root cause of "double-jump does nothing"
**Our `CBasePlayer::Duck()` (player.cpp:1772) NEVER calls `GoRoll` — it just plays TSA_RUN.** `CTSStunt::GoRoll` is
defined but has ZERO callers in our tree. So the DUCK dive-roll (the direction→prone/feet move) is completely dead.
(Answer to the coordinator's hypothesis: the dive is NOT a 2nd-IN_JUMP in the original — it's IN_ALT1 (superdive) or
IN_DUCK-edge (roll). The 2nd-IN_JUMP only drives wall-climb, which needs a wall. In open air a 2nd jump does nothing
in the original either.)

### FIX (Q1)
Rewrite `CBasePlayer::Duck()` to the original:
```
if ((m_afButtonPressed & IN_DUCK) && m_stunt.GoRoll(0x30) != 0) return;
if ((pev->button & IN_DUCK) && !(pev->flags & FL_DUCKING)) m_TSAnim.Animate(this, 5);
```
Also wire the PostThink super-jump/wheel landing GoRoll (see below). (needs-playtest.)

### PostThink GoRoll (@0x81962) — the super-jump / cartwheel LANDING
On landing (`m_flFallVelocity` reset) with `iuser4 & WHEEL_DONE (0x100000)`: ScreenShake, clear WHEEL_DONE; if
`button & IN_DUCK` → `GoRoll(0)` (roll out) + set a 0.75s weapon-time lock; **else** hard-land prone: `iuser4 |= PRONE|STOP`,
`view_ofs.z=16`, friction member=0.75, `Animate(0x3a)` (MORFY landing), "player/superjump-land.wav".

## Q2 — WALLJUMP BACKFLIP full trigger chain (two IN_ALT1-edge paths)

### Path A — standing/airborne at wall, **NOT holding jump** (HandleStunt)
`Play` calls `HandleStunt` when `button & 0x4002` (IN_JUMP|IN_ALT1). Inside, gate `m_flStuntTime<=0 && m_flRollTime<=0
&& !(iuser4&0xc30)`, then:
`if !(button & IN_JUMP) { if (m_afButtonPressed & IN_ALT1) { if CheckForWallJump() GoWallJump(); ... } }`
⇒ **release jump, tap +alt1 facing a wall.**

### Path B — **during an active wall-climb** (holding jump), HandleWallClimb
While `WALLCLIMB_A (0x20000)` set, `Play` calls `HandleWallClimb` each frame. If airborne (`!FL_ONGROUND`):
`if (m_afButtonPressed & IN_ALT1) { if CheckForWallJump() GoWallJump(); else if CheckForWallDive() {iuser4|=WALLDIVE; GoDive();} }`
⇒ **while climbing, tap +alt1** → backflip off. (Our HandleWallClimb, lines 502–521, already implements this path.)

### Gates that can block it
- `CheckForWallJump` needs `CanDiveHigh()` = (in our build) `TSPlayerFreeSlots > 39` — a LOADOUT/slot test, not a height
  counter; passes with a light loadout, fails when heavily loaded (`pl[+0x7b9]!=0` force-passes). Then trace 48u forward,
  `|planeNormal.z| < 0.5`, `velz > -370`. NO SOLID_BSP requirement (unlike wall-CLIMB).
- To reach Path B you must first get into a wall-climb: `CheckForWallClimb` needs the **double-jump window** (2nd IN_JUMP
  within 0.3s) + a **BSP** wall at **54u**. If the double-jump window/`m_afButtonPressed & IN_JUMP` isn't firing, you never
  climb, so the mid-climb walljump never triggers.

### The actual launch (backflip velocity) — pm_shared, ALREADY CORRECT in our tree
`pm_shared/pm_shared.cpp` `TSWallJump()` (lines 3147–3155): `vel.xy = -forward*210; vel.z = sqrt(56000)≈236.64` on
`TSPM_WALLJUMP (0x1000)`. `TSWallJumpClimb()` `vel.z = sqrt(112000)≈334.66`. **These match the RE.** So the *impulse*
is fine; a missing backflip is a TRIGGER/ANIM/networking problem, not the velocity.

### Why "no backflip" (most likely, in order)
1. **Input routing**: if the player HOLDS jump while tapping alt1 at a wall, HandleStunt takes the *jump-held* branch →
   `CheckForWallClimb` (NOT walljump). Walljump-via-HandleStunt needs jump RELEASED. Mid-air-climb walljump needs an
   ACTIVE climb first (Path B).
2. **GoWallJump friction/gravity bug**: our GoWallJump sets `pev->gravity=0.75` — original sets `pev->friction=0.75`
   (offset 0x11c) and leaves gravity. Wrong arc/feel but not a hard block.
3. **The backflip ANIMATION is `Animate(0x37)`** — dispatched inside GoWallJump. Confirm it isn't swallowed (guard case
   0x37 holds only vs another 0x37/0x38/0x28 while PRONE set + airborne; from a fall pose it plays).

## Q3 — GETUP ANIMATION + the Region-B swallow (CTSAnimation::Animate @0x5b470)

`Animate` has a GUARD switch on **last-activity** (`this->pev` = stored m_Activity) that decides whether the NEW code
`a1` is SWALLOWED (early `return`, keep current pose). The relevant cases:
- **last == 0x28 (DIVE)**: swallow unless `a1 == 0x29 (prone)` or `a1 == 0x2e (getup)`. ⇒ **getup 0x2e is ALLOWED out of a dive.**
- **last ∈ {0x29 prone, 0x2e getup, 0x2f roll}**: `if (seq not finished && alive && (iuser4 & PRONE 0x4)) return;`
  ⇒ **NO exceptions — ANY new code (incl. 0x2e) is SWALLOWED while PRONE(0x4) is set and the sequence is unfinished.**
- last == 0x32 (lowwheel): swallow unless `a1==0x29 || a1==0x2e`.
- default (last-activity not a guarded case, e.g. **2** which `HandleDive` sets): NOT swallowed → the pose is set.

### Does the original animate the getup?
- **YES on the dive-land ROLL** (`Landing()` roll branch): dispatches `Animate(0x2e)`. It works because at that instant
  last-activity is `2` (HandleDive's per-frame `Animate(2)`) or `0x28` — both allow 0x2e. **PRONE(0x4) stays set** (from
  GoDive; Landing sets ROLL|STOP but does NOT clear PRONE), so the immediately-following `Animate(2)` inside HandleDive
  (`Landing(); Animate(2);`) is SWALLOWED by the last==0x2e guard → the getup HOLDS and plays to completion.
- **NO on the deliberate prone-standup** (`Play` getup region): it clears PRONE + sets standing view and dispatches
  **NO Animate** at all. There is no getup animation on that path in the original — the standing idle simply resumes
  once PRONE is cleared.
- The dive-ROLL via DUCK (`GoRoll`) animates with `Animate(0x2f)` (roll), also protected by PRONE.

### ⇒ Root cause of "getup plays no animation"
Our `Play` getup stage-1 (ts_stunt.cpp:169) dispatches `Animate(0x2e)` while **last-activity is 0x29 (prone)** (because
our `Landing` SLIDE branch and our `HandleSlide` call `Animate(0x29)`) and **PRONE(0x4) is still set** → hits the
`last∈{0x29,0x2e,0x2f} && PRONE` guard → **SWALLOWED**. So no getup plays. The original never dispatches 0x2e from a
prone last-activity; it only does it from the dive (last 2/0x28) at Landing.

### FIX (Q3)
1. **Delete the `Animate(0x2e)` from `Play`** (both stages) — the getup anim belongs in `Landing()` roll branch only.
2. **Remove `Animate(0x29)` from `Landing()` SLIDE branch** — defer posing to HandleSlide (see mapping below), so the
   last-activity feeding a later 0x2e isn't forced to prone.
3. Keep `Landing()` roll `Animate(0x2e)` and `GoRoll` `Animate(0x2f)`; ensure **PRONE stays set** across them so the
   trailing `Animate(2)`/idle is swallowed and the getup/roll holds. Let `Play`'s standup just clear PRONE (no anim).

### HandleSlide move-key → Animate mapping (confirmed, @0x8b9f8)
While SLIDE active + FL_ONGROUND, when `(button & 0x4006)==0 || m_flSlideTime>=0.01`:
- IN_MOVELEFT (button 0x200) held & slidetime<0.01 → **`Animate(0x2a)`** (roll/slide LEFT), dir stored, m_bDirForward=0.
- IN_MOVERIGHT (button 0x400) held & slidetime<0.01 → **`Animate(0x2b)`** (roll/slide RIGHT).
- neither → **`Animate(3)`** (plain slide).
Else (jump/duck/alt1 held & slidetime<0.01) → deliberate **PRONE**: `iuser4|=STOP; iuser4&=~SLIDE; view=28;
m_flRollTime=0.75; pl[+0x78]=1; Animate(0x29)`. ⇒ **Yes, strafing while prone-sliding rolls you left/right (0x2a/0x2b).**

## FOLLOW-UP fix checklist (dlls/)
1. `CBasePlayer::Duck` (player.cpp:1772): call `m_stunt.GoRoll(0x30)` on IN_DUCK edge; Animate(5) crouch — **fixes double-jump/dive-roll dead move.**
2. Wire the PostThink WHEEL_DONE landing GoRoll / hard-land prone (Animate 0x3a) if not present.
3. `GoWallJump`: `pev->gravity=0.75` → `pev->friction=0.75`.
4. Remove `Play` getup `Animate(0x2e)`; remove `Landing` SLIDE-branch `Animate(0x29)`; rebuild real `HandleSlide` pose selector — **fixes getup-no-anim.**
5. Walljump: verify jump is RELEASED for HandleStunt path / an active climb exists for the mid-climb path; pm_shared impulse already correct.
