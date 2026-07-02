# Analysis — group TS_pm_shared.c free functions (PseudoClimb / CanJumpHigh / FixVelocity)

Original file: `TS_pm_shared.c` (funcfile.tsv:541-553 lists the file's full population:
FixVelocity, PseudoClimb, TSStandUp, TSGoDuck, TSWallJump, TSWallJumpClimb, TSDive,
TSHandle, CanJumpHigh, PM_Jump, PM_CheckParamters, PM_Duck, PM_WalkMove — i.e. the TS
fork of pm_shared.c plus the TS stunt helpers).

All three are plain-C exports (NOT in signatures-gcc2.txt — that file only holds mangled
C++; types below are from `readelf --dyn-syms` + the decompile). All scratch vars with a
`.N` suffix (`savehull.20`, `savehull.3`, `vec_start_16`, `trace_19`, …) are **gcc2
STATIC LOCALS** (confirmed: `nm` shows `001905e4 b savehull.20`, `001904e4 b savehull.3`)
— the original author declared `static vec3_t start,end; static pmtrace_t trace; static
int savehull;` inside these functions. Behaviourally irrelevant (single-threaded pm), keep
as plain locals or statics; note for source parity.

playermove_t offsets used (all standard HL SDK pm_defs.h, 32-bit, verified by
recomputing the struct: cmd base = 0x45458, matches the disasm exactly):
0x10 frametime · 0x14/0x18 forward.x/.y · 0x38/0x3c/0x40 origin · 0x44 angles[0]=pitch ·
0x5c/0x60/0x64 velocity · 0x88 view_ofs.z · 0xbc usehull · 0xe0 onground · 0x1f4 maxspeed ·
0x208 iuser4 (stunt flags) · 0x45458 cmd (0x45468 forwardmove, 0x4546c sidemove,
0x45470 upmove, 0x45476 buttons) · 0x4f4f4 player_mins[4] (so 0x4f4fc + hull*0xc =
**player_mins[hull][2]**, i.e. the BOTTOM of the bbox / feet level).
The two indirect calls resolve to the pmove function pointers: __DT_REL[0x1e0d] slot =
`pmove->PM_PlayerTrace(trace, start, end, PM_NORMAL(0), -1)`; __DT_REL[0x1e0b] slot =
`pmove->PM_PointContents(&point, NULL)`.

---

## 1. PseudoClimb — @0xe8fc8 (nm); real body ~0x22a bytes

### SIGNATURE
`void PseudoClimb(void);`  (exported, dynsym FUNC)

### BEHAVIOR (the TS hold-jump wall-scramble / ledge-mantle)
Caller gate (in TSHandle, NOT inside PseudoClimb — see CALLERS):
`(pmove->cmd.buttons & IN_JUMP) && pmove->onground == -1` — i.e. runs every pm frame
while the player HOLDS JUMP in the air.

Body:
1. **Pitch gate**: `pitch = pmove->angles[0]` (0x44); require `-60 <= pitch <= 60`
   (fc_neg60/fc_60). Outside that → do nothing at all.
2. `climb = pitch < -30` (fc_neg30; HL negative pitch = looking UP steeply).
3. `savehull = pmove->usehull` (0xbc); `pmove->usehull = 2` (POINT hull for both traces).
4. **Trace 1 — headroom probe (must be CLEAR)**: start = origin with
   `z = origin.z + view_ofs.z` (0x40 + 0x88 = eye level); if `climb`, `z += 40`
   (fc_40 — probe 40u ABOVE the eyes when looking up). end = start +
   `forward.xy * 64` (fc_64; z unchanged — horizontal probe).
   `PM_PlayerTrace(trace, start, end, 0, -1)`. Continue only if
   `trace.fraction == 1.0` (open space at/above head height).
5. **Trace 2 — feet obstruction probe (must be BLOCKED)**: reuse start with
   `z = origin.z + player_mins[savehull][2]` (0x4f4fc + savehull*0xc — the bbox
   BOTTOM, feet level, using the hull the player actually had; x/y unchanged from
   trace 1's start). end = start + `forward.xy * 32` (fc_32). usehull forced to 2
   again; `PM_PlayerTrace(...)`. Continue only if `trace.fraction != 1.0`
   (a wall/ledge within 32u of the feet).
6. **Speed gate**: `Length(&pmove->velocity) < 0.5 * pmove->maxspeed`
   (dc_0_5 * value at 0x1f4). Only slow players scramble — you can't climb at speed.
7. **Climb impulse** (all three gates passed):
   - `pmove->onground = -1` (0xe0 = 0xffffffff);
   - `pmove->velocity[2] += 100.0 * pmove->frametime * 16.0`
     (dc_100 * [0x10] * fc_16 = **+1600 u/s² upward accel**, i.e. net +800 u/s²
     against gravity 800 while held — the continuous scramble);
   - `PM_FixupGravityVelocity()`.
8. **Always (whenever the pitch gate passed)**: restore `pmove->usehull = savehull`,
   then `PM_FixPlayerCrouchStuck(1)` (direction = 1 = STUCK_MOVEUP) and
   `PM_CatagorizePosition()` — even when no climb impulse was applied.

No gmsg/event emissions. Callees: pmove->PM_PlayerTrace (x2), Length,
PM_FixupGravityVelocity, PM_FixPlayerCrouchStuck, PM_CatagorizePosition.
Constants w/ meaning: ±60° pitch window · −30° "looking up" threshold · +40u raised
head probe when looking up · 64u head probe length · 32u feet probe length ·
0.5×maxspeed speed cap · 1600·frametime per-frame vertical velocity add.

### CALLERS
Exactly one, verified in disasm: **TSHandle @0xe96a0, call @0xe96ca**, as the FIRST
thing TSHandle does, gated `testb $0x2, 0x45476(pmove)` (= cmd.buttons & IN_JUMP,
IN_JUMP=2) + `cmpl $-1, 0xe0(pmove)` (onground == -1). No other refs (checked full
objdump + PLT). Chain: PM_PlayerMove (@0xf0a2c) → TSHandle → PseudoClimb.

### OURS
**MISSING entirely.** Our `pm_shared/pm_shared.cpp:3173 TSHandle()` reproduces the
original's STOP (×0.5, matches disasm const), SLIDE (fwd=0, ×0.3), TSDive (0x1c0),
TSWallJump (0x7000), TSWallJumpClimb (low-short sign), TSGoDuck (0x4), TSStandUp (0x8)
dispatch in the right order — but lacks the leading PseudoClimb call. Nothing else in
pm_shared/dlls/cl_dll does an eye-clear + feet-blocked trace pair or a held-jump
`vel.z += 1600*frametime` (grepped). Relation to the stunt system: our
`dlls/ts_stunt.cpp:541 CTSStunt::HandleWallClimb()` / `:630 CheckForWallClimb()` are the
SERVER stunt-key wall-climb (gravity 0.5 while WALLCLIMB_A, one-shot TSWallJumpClimb
kick via iuser4) — a DIFFERENT, complementary mechanic. PseudoClimb is unconditional
for every airborne jump-holder (no stunt key, no iuser4 bit) and is what actually makes
players creep up crates/ledges while holding jump. Without it, our wall-climb has only
the one-shot sqrt(112000) kick and none of the sustained scramble.

### RECONCILE
- **Write fresh** into `pm_shared/pm_shared.cpp` (→ the consolidation plan's
  TS_pm_shared file), placed with the other TS pm helpers (original file order:
  FixVelocity, PseudoClimb, TSStandUp, TSGoDuck, TSWallJump, TSWallJumpClimb, TSDive,
  TSHandle, CanJumpHigh).
- Add to the HEAD of TSHandle:
  `if ((pmove->cmd.buttons & IN_JUMP) != 0 && pmove->onground == -1) PseudoClimb();`
  (before the STOP damp — order is load-bearing, PseudoClimb runs first).
- pm_shared is compiled into BOTH libs → prediction stays consistent automatically;
  rebuild both (predicted-weapon gotcha).
- **PLAYTEST-GATED**: new player-facing mechanic (hold-jump ledge scramble); verify vs
  the live original (hold jump against a crate: creep up; note looking up < −30° RAISES
  the head probe by 40u; fails above 0.5×maxspeed and outside the ±60° pitch window).
- VERIFY (minor): PM_FixPlayerCrouchStuck arg is literal 1 (confirm our STUCK_MOVEUP
  macro == 1 before substituting the name).

---

## 2. CanJumpHigh — @0xe97cc, 373 bytes

### SIGNATURE
`int CanJumpHigh(void);`  (returns 1/0; exported dynsym FUNC, size 373)

### BEHAVIOR (a "thin obstacle ahead → high jump allowed" probe — DEAD CODE, see CALLERS)
1. `savehull = pmove->usehull`.
2. `dir = pmove->velocity` (0x5c..0x64), `VectorNormalize(&dir)` (full 3D normalize,
   but dir.z is then UNUSED).
3. start = origin; end = `origin.xy + dir.xy * 128` (fc_128), `end.z = origin.z`
   (horizontal probe 128u along the movement direction).
4. `pmove->usehull = 1` (DUCKED hull) — then `PM_PlayerTrace(trace, start, end, 0, -1)`.
5. If `trace.fraction != 1.0` (an obstacle within 128u of travel):
   - point = `origin.xy + dir.xy * 164` (fc_164), `z = origin.z`;
   - `contents = PM_PointContents(&point, 0)`;
   - if `contents != -2 (CONTENTS_SOLID) && contents != -6 (CONTENTS_SKY)` →
     **return 1** — *WITHOUT restoring usehull* (original quirk: usehull stays 1 on the
     success path; only the fail path restores). Meaning: wall ahead, but the point 164u
     out is open ⇒ the obstacle is thin ⇒ you could jump high over it.
6. Otherwise restore `pmove->usehull = savehull`, return 0.

Constants: 128u obstacle look-ahead · 164u thinness probe · hull 1 (ducked) for the
trace. Callees: VectorNormalize, pmove->PM_PlayerTrace, pmove->PM_PointContents.
No gmsg/events.

### CALLERS
**NONE — dead code in ts_i386.so.** Verified three ways: no `call` to 0xe97cc anywhere
in the full disasm (only its own PIC get-pc thunk), no `CanJumpHigh@plt` slot, no dynamic
relocation referencing the symbol. Original PM_Jump (@0xe9944) does NOT call it — its
callees are PM_PreventMegaBunnyJumping, PM_MapTextureTypeStepType, PM_PlayStepSound,
__strtol_internal (the stock `atoi(physinfo "slj")` superjump gate),
PM_FixupGravityVelocity. ⇒ **The hint "CanJumpHigh may be the real superjump gate" is
REFUTED for the shipped server binary**: the original keeps the stock slj/longjump
physinfo gate in PM_Jump, and CanJumpHigh is vestigial. VERIFY (low priority): whether
client.dll's compiled-in copy calls it (client.dll has no symbols; same TS_pm_shared.c
builds both sides, so an uncalled server copy strongly implies uncalled client copy
unless #ifdef'd — needs the clientdll Ghidra project, deferred to the refs pass).

### OURS
Missing (grep of pm_shared/dlls/cl_dll: no CanJumpHigh, no 128/164 trace pair). Our
superjump: `pm_shared/pm_shared.cpp:2695-2716` = stock HL `cansuperjump` (physinfo
"slj") in PM_Jump — this MATCHES the original mechanism (strtol call present in original
PM_Jump). The `m_bLowGravity` fuel machine in `dlls/player.cpp:1996-2027` is the
server-side powerup layer; whether THAT matches the original's powerup is a
player.cpp-group question, not settled by this function (CanJumpHigh being dead means it
contributes nothing either way).

### RECONCILE
- **Write fresh, as dead code, for source parity only**: add `int CanJumpHigh(void)`
  to the TS pm file, uncalled, exactly per the decompile (including the
  no-restore-on-success quirk). Non-static (it was exported) so no -Wunused warning.
  Behavior-preserving (zero runtime impact) — NOT playtest-gated.
- Do NOT wire it into PM_Jump or the superjump path — that would invent a call site the
  original doesn't have.
- VERIFY: client.dll liveness check when the Ghidra lock frees (refs pass).

---

## 3. FixVelocity — @0xe8fc4, size 4 (ret + pad)

### SIGNATURE
`void FixVelocity(void);`

### BEHAVIOR
Empty — a single `ret`. A stub the author kept (debug/experiment hook), compiled to
nothing. Reads/writes nothing, calls nothing.

### CALLERS
Three, all verified in disasm, each IMMEDIATELY AFTER `PM_FixupGravityVelocity()` as the
final call of the function:
- `TSWallJump` @0xe94d6 (after FixupGravity @0xe94d1)
- `TSWallJumpClimb` @0xe952a (after @0xe9525)
- `TSDive` @0xe9693 (after @0xe968e)

### OURS
Missing. Our `TSWallJump` (pm_shared.cpp:3147), `TSWallJumpClimb` (:3161), `TSDive`
(:3125) all end with `PM_FixupGravityVelocity()` and simply omit the no-op call —
behaviourally identical.

### RECONCILE
- **Write fresh** (one-line empty function) + add the call after
  `PM_FixupGravityVelocity()` in TSDive/TSWallJump/TSWallJumpClimb. Pure source-parity
  cosmetics; behavior-preserving, not playtest-gated. Place it first in the file
  (original layout: FixVelocity is the file's first function, @0xe8fc4).

---

## Group shape summary
TS_pm_shared.c = the TS fork of pm_shared.c. Its TS free functions form one system:
PM_PlayerMove → TSHandle (iuser4-flag dispatcher) → {PseudoClimb [hold-jump airborne],
STOP/SLIDE cmd damping, TSDive/TSWallJump/TSWallJumpClimb one-shot launches (each ending
FixupGravity + no-op FixVelocity), TSGoDuck/TSStandUp hull management}. CanJumpHigh is a
dead orphan probe. Our reconstruction already has the whole dispatcher tree faithfully
EXCEPT the PseudoClimb head call — the single real gameplay gap in this group (the
sustained hold-jump wall/ledge scramble). The superjump hint is refuted: original keeps
stock slj PM_Jump gating; CanJumpHigh is uncalled.
