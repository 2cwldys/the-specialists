# cont85 — CTSAnimation third-person state machine (decompile-exact spec)

Source: `reference/ts_i386.so` (gcc2), decompiled via `re.sh` (typed by ApplyInventory).
Raw decompiles alongside: `anim_animate.c` (the 7218-byte dispatcher, full — no truncation),
`anim_setanim_blend.c` (SetAnim + BlendJumpAnimations + BlendAnimationsForSlide),
`anim_handleclosecombat.c`, `anim_gaitclosecombat.c`.

## Field map used below (Ghidra typed `this`/`a0` as opaque structs)
`this` = CTSAnimation, `a0` = CBasePlayer, `a1` = activity code.
- `this->pev` (Ghidra artifact = field_0x0) = **m_iLastActivity**  — NOTE: every `switch(this->pev)` / `this->pev == (entvars_t*)N` is `m_iLastActivity == N`.
- `this->field_0x4` = m_iDuckLatch · `0x8` = m_flLegTimer(3.0=0x40400000) · `0xc` = m_flAimTimer(2.0) · `0x10` = m_flGrenadeFlag(1.0) · `0x14` = m_iDirection
- `this->field_0x1c/0x5c/0x9c/0xdc/0x11c/0x15c/0x19c` = the 7 ctor pose tables (jump/dive/slide/prone/getup/roll/diveend)
- `this->field_0x1e4` = m_bBulletDance · `0x1e8` = m_bCCStarted
- `a0->field_0x478` = **m_szAnimOverride** (checked for empty; a whole seq name) · `a0->field_0xb58` = **grip** (m_szAnimExtention, strcat'd suffix)
- `a0->field_0x510..518` = m_vecStuntDir · `0x51c` = m_bDirForward
- `DAT_00106657` = `""` (the empty-string operand of every `strcmp(field,"")` idiom).
- fc_0=0.0 fc_1=1.0 fc_80=80 fc_96=96 fc_120=120 fc_180=180 ; 0.01=fc_0_00999… ; 0.001=fc_0_00100…

---

## D1 — SetAnim(player, name, flags, frame): bit semantics  [HIGH]

Decompiled body (`anim_setanim_blend.c`, FUN 0x6d7e4):

```
seq = LookupSequence(name); if (seq==-1) seq=0;
if (pev->sequence==seq && (char)flags >= 0) return 0;      // the GUARD
if (flags & 0x10) pev->effects &= ~EF_NOINTERP; else pev->effects |= EF_NOINTERP;
if (flags & 0x04) pev->gaitsequence = 0;                   // clear legs
else if (flags & 0x01) { pev->gaitsequence = seq; return 1; }   // legs-ONLY, early return
pev->sequence = seq;
if (flags & 0x20) pev->frame = (float)frame;               // explicit frame
else if ((flags & 0x100)==0) pev->frame = 0;               // else default: RESET to 0
ResetSequenceInfo();
return 1;
```

Bit table (only bits the code reads):

| bit | name | effect |
|----|------|--------|
| 0x01 | LEGS | write `gaitsequence=seq` and **return** (only if 0x04 clear) |
| 0x04 | CLEARLEGS | `gaitsequence=0` |
| 0x10 | INTERP | set→clear EF_NOINTERP (engine interpolates); **clear→set EF_NOINTERP (hard snap)** |
| 0x20 | SETFRAME | `frame = frameArg` |
| 0x80 | FORCE | **the guard is `(char)flags>=0`, i.e. bit 0x80 CLEAR.** Setting 0x80 makes `(char)flags<0` → guard bypassed → restart even if same sequence |
| 0x100 | KEEPFRAME | when 0x20 clear: skip the `frame=0` reset (preserve current frame) |
| 0x02, others | (unused — no code path reads them) |

**★ The blend/snap key.** Two independent controls, and NEITHER is EF_NOINTERP in practice
(every flag actually used sets 0x10, so EF_NOINTERP is always CLEARED — interpolation is
always on):
1. **The guard** `pev->sequence==seq && !(flags&0x80)` → *no-op* (returns 0, doesn't touch
   frame/ResetSequenceInfo). This is what makes a repeated same-sequence call BLEND (the anim
   keeps playing, uninterrupted). Adding **FORCE (0x80)** removes the guard → the sequence
   RESTARTS.
2. On a restart, **frame handling** decides snap vs cross-fade: default (no 0x20/0x100) →
   `frame=0` + ResetSequenceInfo = **restart from frame 0 = the visible snap**. With
   **KEEPFRAME (0x100)** → the new sequence starts at the *current* frame = smooth cross-fade
   between related variants.

Flag-value decode (values actually used):

| value | = bits | meaning | used by |
|------|--------|---------|---------|
| **0x12** | 0x10\|0x02 | INTERP, **guard active**, no clearlegs, frame→0 | idle/aim/duck torso (case1/6), m_szAnimOverride, melee idle |
| **0x14** | 0x10\|0x04 | INTERP, CLEARLEGS, guard active, frame→0 | death, slide-back-roll |
| **0x92** | 0x80\|0x10\|0x02 | FORCE, INTERP, **keep legs**, frame→0 | shoot/reload/draw/grenade + gait-CC |
| **0x94** | 0x80\|0x10\|0x04 | FORCE, INTERP, CLEARLEGS, frame→0 (snap-restart) | jump, dives, wheels, walljump, bullettime, slide-fire + non-gait-CC |
| **0x194** | 0x100\|0x80\|0x10\|0x04 | FORCE, INTERP, CLEARLEGS, **KEEPFRAME** (cross-fade) | directional re-blends: BlendJump / BlendForSlide / BlendByDir |

The re-blends being **0x194 (keep frame)** vs the initial set being **0x94 (frame→0)** is
exactly the "hold the frame while I swap left→forward" cross-fade. Our reconstruction's
SetAnim + flag enum reproduce all of this correctly.

---

## D2 — movement / idle / gait selection (onground, non-close-combat)  [HIGH]

**★ Structural fact that resolves the playtester bug:** the original picks TWO sequences every
frame that reaches the bottom of Animate — the **TORSO** (`pev->sequence`, chosen in the
per-activity switch) and the **LEGS** (`pev->gaitsequence`, chosen in a *single* block at the
very bottom, lines 977–1093). For **bare hands (kungfu)** there is a third step: a **body-sync**
that copies the leg sequence into the torso. combat_idle is NOT a hard-coded torso — it is one
leaf of the LEG selection, used only when standing still.

### The bottom LEG (gait) block  — runs when `a1 ∉ {4,0x39,5,7,0x29,0x2e,0x2f,0x30,0x36}`, and (`!IsInCloseCombat(a1)` or `GaitCloseCombat(a1)`), and `(iuser4 & 0x30c30)==0`:

```
if (!(pev->flags & FL_DUCKING)) {          // 0x4000
    if      (speed  > 180.0)      leg = "run2";
    else if (speed  > 0.0)        leg = "walk2";
    else {                          // standing still
        if      (m_szAnimOverride[0])  leg = m_szAnimOverride;
        else if (grip[0]==0)           leg = (m_flAimTimer>=0.01) ? "combat_idle" : "combat_idle_low";
        else                            leg = "idle_" + grip;
    }
} else {                                    // DUCKING
    m_iDuckLatch = 0;
    leg = (speed==0.0) ? "crouch_idle" : "crawl";
}
pev->gaitsequence = LookupSequence(leg);
```

`speed` = `velocity.Length2D()` (x,y only). Thresholds: **>180 run2, (0,180] walk2, ==0/idle**.

### The kungfu body-sync (immediately after, lines 1057–1092):

```
if (m_szAnimOverride[0]==0 && grip[0]==0                 // bare hands
    && GaitCloseCombat(a1)==0
    && (m_flAimTimer < 0.01 || m_fSequenceFinished)      // no fresh attack/aim holding
    && pev->sequence != pev->gaitsequence) {
    pev->sequence = pev->gaitsequence;                   // TORSO follows LEGS
    pev->frame = 0; ResetSequenceInfo();
}
```

### The TORSO for the ARMED case (Phase-C `case 1: case 6:`)  — for `a1==RUN(1)/IDLE(6)`:

```
if (!onground && !IsOnLadder) return;
if (m_szAnimOverride[0]) { SetAnim(m_szAnimOverride, 0x12); }           // e.g. knife_idle
else if (grip[0]==0)     { /* KUNGFU: break — torso set by leg-block + body-sync */ }
else if (m_iLastActivity ∈ {0,8,9,0x33,0x34} && !m_fSequenceFinished) { a1 = m_iLastActivity; /*keep fire/reload/draw playing*/ }
else {
    aiming = (m_flAimTimer>=0.01 || pev->fov!=0);
    if (!ducking) torso = aiming ? "aim_"        : "idle_";
    else          torso = aiming ? "crouch_aim_" : "duck_idle_";
    torso += grip;
    SetAnim(torso, 0x12);       // guard-active blend, no clearlegs
}
```

### Leaf table — what the ORIGINAL plays

| state (onground) | LEG (gaitsequence) | TORSO (sequence) |
|---|---|---|
| **kungfu, moving, speed>180** | run2 | **run2** (synced) |
| **kungfu, moving, 0<speed≤180** | walk2 | **walk2** (synced) |
| **kungfu, standing, recent combat (aimTimer≥.01)** | combat_idle | combat_idle |
| **kungfu, standing, settled (aimTimer<.01)** | combat_idle_low | combat_idle_low |
| kungfu, ducking, moving | crawl | crawl (synced) |
| kungfu, ducking, still | crouch_idle | crouch_idle (synced) |
| armed, moving, speed>180 | run2 | aim_/idle_ + grip |
| armed, moving, 0<speed≤180 | walk2 | aim_/idle_ + grip |
| armed, standing | idle_+grip | aim_/idle_ + grip |
| armed, ducking, moving | crawl | crouch_aim_/duck_idle_ + grip |
| armed, ducking, still | crouch_idle | crouch_aim_/duck_idle_ + grip |

**CRITICAL ANSWER (bare-hands moving):** the original plays **run2 / walk2 as the FULL BODY**
(legs via the gait block, torso via the body-sync). `combat_idle` is played ONLY when
STANDING STILL (aimTimer≥.01) and `combat_idle_low` only when standing still & settled. The
kung-fu fighting stance is NEVER the torso while running. The playtester is right.

Aiming condition = `(m_flAimTimer >= 0.01 || fov != 0)` — NOT the attack button. [HIGH]

Also note: because case 0/8/9 (fire) falls THROUGH to the bottom gait block, the original updates
the LEGS during firing (shoot torso + run2/walk2/idle legs = run-and-gun). [HIGH]

---

## D3 — jump per grip + landing  [HIGH]

### Initial jump — Phase-C `case 4:` (lines 498–562):
```
if (iuser4 & 0x20 /*SLIDE*/) return;
if (iuser4 & 0x04 /*PRONE*/) return;
if (grip[0]==0) name = "standard_jump";              // BARE HANDS = standard_jump, NO direction, NO akimbo-substitute
else { dir = GetDirection(norm(velocity), v_angle, 0.6, 0.3);
       m_iDirection = dir;
       name = jumpTable[dir] + grip;                 // jump_left_/jump_right_/jump_forward_ + grip  (dir 2/3/4 → forward)
     }
SetAnim(name, 0x94, 0);                              // snap-restart, clear legs
```
There is **no speed threshold** in the initial dispatch — the branch is purely `grip empty`.
Direction table (`field_0x1c`): 0=jump_left_, 1=jump_right_, 2/3/4=jump_forward_ (see
BlendJumpAnimations switch, `anim_setanim_blend.c`).

### Per-frame directional re-blend — Phase-A (lines 73–98) → BlendJumpAnimations (0x6d630):
Gate: `!(flags & FROZEN 0x200) && IsAlive && m_iLastActivity==JUMP(4) && speed>96 && !IsInCloseCombat(a1)`.
BlendJumpAnimations itself: `if (grip[0]==0) return;` (bare hands never re-blend), then
`d = GetDirection(norm(velocity), v_angle, 0.6, 0.3); if (d != m_iDirection) { SetAnim(jumpTable[d]+grip, 0x194, 0); m_iDirection=d; }`.
So airborne strafing cross-fades jump_left_↔jump_forward_ via **0x194 (keep frame)**.

### The landing transition:
Region-B `case 4` (m_iLastActivity==JUMP, lines 136–158) HOLDS the jump anim only while
`m_fSequenceFinished==0 && !onground(!(flags&0x200)) && IsAlive && a1∉{stunts/cc}`. The moment
the player is **onground** (`flags&0x200` set) the hold condition fails → it `break`s to
Phase-C for the incoming activity. **There is NO land-specific blend anim** — if the landing
activity is a kick (close-combat), HandleCloseCombat sets it with **0x94/0x92 (FORCE, frame→0)**
= a deliberate restart-from-0. That restart is by-design in the original; the "click" the tester
sees is downstream (see D5): our broken kungfu torso means the frames around the kick are
combat_idle, so the kick reads as a snap between two wrong poses.

OPEN QUESTION: the exact per-frame cross-fade smoothness of jump→land→run is engine
interpolation (EF_NOINTERP is cleared throughout) — needs a live trace to confirm no extra
land pose exists that a static read missed. [MED]

---

## D4 — duck  [HIGH]

Ducking is gated by `pev->flags & FL_DUCKING (0x4000)` and takes the ducking arm of the bottom
gait block: `m_iDuckLatch=0; leg = (speed==0.0) ? "crouch_idle" : "crawl"`. So:
- **legs, ducking + moving → "crawl"**; **ducking + still → "crouch_idle"** (NOT cleared).
- torso (armed) → `crouch_aim_`/`duck_idle_` + grip (case 1/6).
- torso (kungfu ducking) → **crouch_idle / crawl (synced from legs)** — kungfu ducking does NOT
  use combat_idle_low. combat_idle_low is the STANDING settled-kungfu idle only.
- The one-shot goduck transition (`case 5`, `SetAnim("goduck",0x94)` + m_iDuckLatch=1) is
  separate; m_iDuckLatch is reset to 0 inside the ducking gait branch.

There is no other duck-specific gait.

---

## D5 — where dlls/ts_animation.cpp DIVERGES (the fix list)

1. **[HIGH, the reported bug] kungfu torso.** Our Phase D (lines 771–776) always
   `SetAnim(ducking?combat_idle_low:combat_idle, 0x12)` as the TORSO for bare hands, regardless
   of speed → running shows the frozen fighting stance. **Original:** for kungfu the torso is
   *synced to the leg gait* (run2/walk2 when moving, combat_idle when standing). Fix: don't set
   a combat_idle torso when moving; select the leg first, then copy leg→torso for bare hands
   (the body-sync block) when `m_flAimTimer<0.01 || m_fSequenceFinished`.

2. **[HIGH] combat_idle vs combat_idle_low picked by the wrong condition.** Ours keys it on
   ducking (combat_idle standing / combat_idle_low ducking). **Original:** both are the
   NOT-ducking standing-still kungfu leaf, chosen by the **aim timer**: `m_flAimTimer>=0.01 →
   combat_idle`, else `combat_idle_low`. Ducking kungfu uses crouch_idle/crawl, never
   combat_idle_low.

3. **[HIGH] legs not updated during fire/reload/etc.** Our fire case (line 487) and several
   others `return` after setting the torso, so `gaitsequence` is never refreshed → legs freeze
   while firing/moving. **Original:** those cases (0/8/9, 0x33/0x34/0x35 …) fall THROUGH to the
   bottom gait block, so the legs keep running (run-and-gun). Fix: run the leg/gait selection
   after setting the fire/reload/grenade torso, not `return`.

4. **[HIGH] jump direction gated on speed instead of grip.** Ours (line 493) uses `speed>16` to
   choose directional-vs-standard_jump. **Original case 4:** `grip empty → standard_jump`, else
   `jump_<dir>_<grip>` — no speed test. Bare hands must be **standard_jump** (ours emits
   `jump_left_`+"" with empty grip = a missing/wrong sequence). Also add the early returns
   `iuser4 & 0x20 (SLIDE) → return` and `iuser4 & 0x04 (PRONE) → return` that ours omits.

5. **[MED] ducking-still legs.** Ours clears the legs when ducking + still (`SetAnim("",LEGS)`).
   **Original:** sets `crouch_idle`. And ducking + moving uses `crawl` (matches ours).

6. **[MED] moving-speed threshold.** Ours only sets legs when `speed>1.0` and then run2/walk2 by
   the 180 split; below 1.0 clears legs. **Original:** any `speed>0` → walk2 (or run2 >180);
   `speed<=0` → the idle leaf. Minor, but 0<speed≤1 should be walk2, not cleared.

7. **[MED] m_szAnimOverride as a standing leg.** When standing still with a non-empty
   m_szAnimOverride, the original also uses it as the LEG (gaitsequence). Ours applies the
   override only as a torso. Edge case (melee idle poses).

8. **[LOW] gait-block skip mask.** The original guards the whole leg block with
   `(iuser4 & 0x30c30)==0` (stunt-active bits) plus the activity exclusion list
   `{4,0x39,5,7,0x29,0x2e,0x2f,0x30,0x36}` and the CC-gait test. Our Phase D uses a different
   `!onground → return` + per-case returns; verify the net set of "don't touch legs this frame"
   matches (esp. that stunts don't get run2/walk2 legs stamped on them).

### Confidence & open questions
- All sequence names, thresholds, flag values above are read directly from the decompile: HIGH.
- The precise *feel* of jump→land→run blending and whether any land pose is applied by a caller
  outside CTSAnimation (e.g. the stunt/PM_ jump path) is not fully resolvable statically — a
  live gdb trace of the original (mp.dll `Animate`/`SetAnim`) around a jump-land-run and a
  bare-hands run would confirm the run2 body-sync and the absence of a land anim. [OPEN]
- `field_0x520` cleared in Region-B case 0x36 (bullettime) is unnamed here; not movement-relevant. [OPEN]
</content>
</invoke>
