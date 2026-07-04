# SUPER-JUMP / "moon jump" ("mj" physinfo) — decode dossier (cont.86, REV 2)

**Binary:** `reference/ts_i386.so` (Linux GCC-2.96; contains the predicted
`pm_shared` code too). Ghidra addr = nm-addr + 0x10000. `.rodata`/`.data` file
offset == vaddr. GOT base `_GLOBAL_OFFSET_TABLE_ = 0x165e10`.

> **REV 2 supersedes REV 1.** REV 1 concluded "constant `pev->gravity = 0.15`
> while the powerup is held." **That was WRONG — it read a DEAD code path.** The
> user's playtest (variable, hold-to-climb, capped, release-to-drop) is correct.
> The real mechanism is a **per-frame, jump-button-gated gravity set in
> `PM_CheckParamters`**, keyed on an `iuser4` FLIGHT bit. Details below.

---

## TL;DR — the actual variable-jump-height mechanism

Four stages, exactly matching the user's spec:

1. **Pickup grants the ABILITY only — NO float.** `PlayerAddPowerUp` for the
   super-jump powerup (bit `0x100`) sets `m_bLowGravity = 1` and the physinfo
   `"mj" = "1"`, sends `gmsgTSPowerUp(0x100)`, and **RETURNS** — it does **not**
   touch `m_iPowerups`/`m_iHeldPowerup` and does **not** change gravity. Gravity
   stays 1.0. ✔ "having the powerup does not float you."

2. **DUCK+JUMP activates (one-shot, predicted).** `PM_Jump` reads `"mj"`; on
   duck + jump while on the ground it sets `velocity.z = sqrt(160000) = 400` and
   `pmove->iuser4 |= 0x200000` (launch flag).

3. **`0x200000` → `0x100000` (flight-active), server-side.** `CTSStunt::Play`
   (run every `PreThink`) sees `iuser4 & 0x200000`, sets `iuser4 |= 0x100000`,
   clears `0x200000`, plays the launch anim. `0x100000` = "in super-jump flight",
   networked in `pev->iuser4` → the client's `pmove->iuser4`.

4. **Per-frame gravity gate while `0x100000` is set — `PM_CheckParamters`
   (pm_shared, runs client AND server):**
   ```c
   if (pmove->iuser4 & 0x100000) {                 // super-jump flight active
       cmd.forwardmove *= 0.5;                      // reduced air control
       cmd.sidemove    *= 0.25;
       cmd.upmove      *= 0.25;
       if (cmd.buttons & IN_JUMP) {                 // HOLDING jump
           pmove->gravity = (velocity.z < 0) ? 0.75 // falling: mild
                                             : 0.30; // rising : strong float/climb
       } else {
           pmove->gravity = 1.0;                    // RELEASED -> normal -> DROP
       }
   }
   ```
   So holding jump keeps you climbing under **0.30** gravity; the climb self-caps
   when `velocity.z` decays to ~0 (then `0.75`); **releasing jump → 1.0 → you drop
   fast.** ✔ variable jump height, hold-to-climb, capped, release-to-drop.

5. **Flight ends (bit `0x100000` cleared) on: LANDING** (`PostThink` @ `0x81922`,
   with a `UTIL_ScreenShake` on touchdown), **taking damage** (`TakeDamage`
   @ `0x7ff54`), or **a stunt/wall-jump** (`CTSStunt` @ `0x8a1c3/0x8a84a/0x8aa92/
   0x8b287`). There is **no separate max-hold timer**; the "max" is the velocity
   decaying to zero under 0.30 gravity (≈ `400 / (0.30·800)` ≈ **1.67 s** of climb).

**Gravity constant for the climb is `0.30` (rising) / `0.75` (falling), NOT
`0.15`.** REV 1's `0.15` (constant) is what made our jump overshoot ~2×.
**Confidence: HIGH** (raw-verified below).

---

## 1. `PM_CheckParamters` — the per-frame gate (nm `0xe9c94` → Ghidra `0xf9c94`)

Raw disasm of the gate (objdump, `.so` addrs):
```
e9dcc  je   e9e52                  ; if (iuser4 & 0x100000)==0 skip  (testb $0x10,0x20a)
e9dd2  fldl -0x6f010(%ebx)         ; 0.5
e9dd8  fmuls 0x45468(%eax)         ; cmd.forwardmove *= 0.5    (usercmd+0x45468)
e9de6  fldl -0x6e780(%ebx)         ; 0.25
e9dee  fmuls 0x4546c(%eax)         ; cmd.sidemove    *= 0.25   (+0x4546c)
e9dfc  fmuls 0x45470(%eax)         ; cmd.upmove      *= 0.25   (+0x45470)
e9e0a  testb $0x2,0x45476(%eax)    ; cmd.buttons & IN_JUMP(2)  (buttons at +0x45476)
e9e11  je   e9e40                  ;   not held -> gravity = 1.0
e9e15  flds -0x6f408(%ebx)         ; 0.0
e9e1b  fcomps 0x64(%ecx)           ; compare 0.0 vs velocity.z (pmove+0x64)
e9e23  jne  e9e30                  ;   velz>=0 (rising) -> 0.30 ; velz<0 (falling) -> 0.75
e9e25  mov  -0x6df6c(%ebx),%edx    ; 0.75  (fall-through = falling)
e9e30  mov  -0x6de78(%ebx),%edx    ; 0.30  (jne = rising/equal)
e9e36  mov  %edx,0xc0(%ecx)        ; pmove->gravity = 0.30 | 0.75   (pmove+0xc0)
e9e48  movl $0x3f800000,0xc0(%eax) ; pmove->gravity = 1.0
```
Constants read RAW from the GOT (`_GLOBAL_OFFSET_TABLE_`=`0x165e10`):
- `[GOT-0x6de78]` (vaddr `0xf7f98`) = **0.30** → rising branch
- `[GOT-0x6df6c]` (vaddr `0xf7ea4`) = **0.75** → falling branch
- `[GOT-0x6f408]` (vaddr `0xf6a08`) = **0.0** → the `velz` compare constant
- `-0x6f010` = 0.5, `-0x6e780` = 0.25 (the air-control multipliers, doubles)

**Field offsets (raw-verified):** `pmove->iuser4` @ `pmove+0x208` (byte `+0x20a`
holds bit 20 = `0x100000`); `pmove->gravity` @ `pmove+0xc0`; `velocity.z` @
`pmove+0x64`; **usercmd** block: `forwardmove` `+0x45468`, `sidemove` `+0x4546c`,
`upmove` `+0x45470`, **`buttons` `+0x45476`** (word; `IN_JUMP=0x2`).
**Confidence: HIGH.**

---

## 2. The `iuser4` bit choreography

| bit | meaning | SET by | READ by | CLEARED by |
|---|---|---|---|---|
| `0x200000` (b21) | super-jump just launched | `PM_Jump` on duck+jump (`iuser4\|=0x200000`, `0xe9c48`) | `CTSStunt::Play` (`0x8a0f5`) | `CTSStunt::Play` (converts → `0x100000`) |
| `0x100000` (b20) | in super-jump FLIGHT | `CTSStunt::Play` (`0x8a0fc` `or $0x100000`) | `PM_CheckParamters` gravity gate (`testb $0x10,0x20a`) | `PostThink` on land `0x81922`; `TakeDamage` `0x7ff54`; `CTSStunt` walljump/stunt `0x8a1c3/0x8a84a/0x8aa92/0x8b287` |

- `iuser4` is `pev->iuser4` @ **entvars+0x250** (server) == `pmove->iuser4` @
  `pmove+0x208` (client) — the same networked field.
- `CTSStunt::Play` @ `0x8a0dd`: `if (iuser4 & 0x200000) { iuser4 |= 0x100000;
  iuser4 &= ~0x200000; CTSAnimation::Animate(player, 0x39); ... }`.
- `PostThink` @ `0x818ef`: `if (iuser4 & 0x100000) { UTIL_ScreenShake(origin,
  amp=20, freq=30, dur=0.3, radius=512); iuser4 &= ~0x100000; }` — the LANDING
  event (screen shake) that ends flight. **Confidence: HIGH.**

---

## 3. `PM_Jump` — the launch impulse (unchanged from REV 1) — nm `0xe9944`

On-ground path (gated `pmove->onground != -1`, `pmove+0xe0` — **no air re-jump**):
```c
canhighjump = atoi(GetPhysicsKeyValue(pmove->physinfo/*+0x4f3f0*/, "mj"/*0xf791d*/)) == 1;
if (ducking && canhighjump && (cmd.buttons & IN_DUCK/*4*/) && flDuckTime(+0x8c) > 0) {
    velocity.z = sqrt(160000.0);   // = 400.0   (double @ .rodata 0x100bc8)
    iuser4 |= 0x200000;
} else {
    velocity.z = sqrt(56000.0);    // = 236.64  (double @ .rodata 0x100bb8) — TS base jump
}
PM_FixupGravityVelocity();          // applies a half-gravity step SAME frame
```
- **Impulse is genuinely 400** (`sqrt(160000)`). It is *not* too high on its own —
  REV 1's constant `0.15` gravity (peak ≈ `400²/(2·0.15·800)` ≈ **667 u**) is what
  overshot. With the real **0.30** rising gravity, peak ≈ `400²/(2·0.30·800)` ≈
  **333 u**. Keep the 400 impulse; fix the gravity. **Confidence: HIGH.**
- `PM_FixupGravityVelocity` runs the same frame using `pmove->gravity`. When
  `PM_CheckParamters` (which sets `pmove->gravity`) runs *before* the move in
  `PM_PlayerMove`, the launch frame already has the reduced gravity applied.

---

## 4. What is DEAD / NOT the super-jump (don't be misled again)

- **`UpdatePowerUpState` @ `0x95748` `pev->gravity = 0.15`** (raw `0x85828`, gated
  `m_iPowerups & 0x100`, restore `1.0` @ `0x858d7`): **DEAD for the super-jump.**
  `PlayerAddPowerUp`'s `0x100` case never writes `m_iPowerups`/`m_iHeldPowerup`
  (it returns after setting `m_bLowGravity`+`"mj"`), and `UsePowerUp` only copies
  `m_iHeldPowerup`→`m_iPowerups`, so `m_iPowerups & 0x100` is never true. This is
  legacy/unreachable for `0x100`. (See the `flag-overriding-and-dead-code` memory.)
- **`SetPlayerGravity` @ `0x85b70` + `field_0x4ec` timer** (`PreThink` decrements
  `field_0x4ec`; when `<0.01` → `pev->gravity = 1.0`): a *separate*, virtual
  gravity-timer facility. `SetPlayerGravity` is **virtual** (no direct/`vrefs`
  caller found; only ref is a vtable slot) and its `a1` gravity value is not a
  literal `0.15` anywhere reachable. **Not the super-jump path.** (Likely a
  trigger_gravity / other-powerup facility; flagged, not chased — out of scope.)
  `field_0x4ec` = the timer; `SetPlayerGravity(a0=duration, a1=grav)` arms it,
  `pev->gravity`@`+0x11c`. **Confidence: MED** (correct reading, unknown caller).

---

## 5. Implementation recipe for our reconstruction

### (a) `dlls/ts_powerup.cpp` — grant (PlayerAddPowerUp, super-jump `0x100`)
- Set `m_bLowGravity = true;` and `SET_PHYSICS_KEY_VALUE(edict, "mj", "1");`
  (literal key — swap contract). Send `gmsgTSPowerUp` with `0x100`.
- **Do NOT** set `m_iPowerups`/`m_iHeldPowerup`, and **do NOT** change gravity here.

### (b) `pm_shared/pm_shared.cpp` `PM_Jump` (+ `TS_pm_shared.cpp`)
- On-ground, duck-held, `atoi(mj)==1`: `pmove->velocity[2] = sqrt(160000.0);
  pmove->iuser4 |= 0x200000;` else `pmove->velocity[2] = sqrt(56000.0);`
  Keep both as `sqrt(...)` on doubles. No air re-jump.

### (c) `pm_shared/pm_shared.cpp` `PM_CheckParamters` — **THE GATE (new/most important)**
```c
if (pmove->iuser4 & 0x100000) {           // super-jump flight
    pmove->cmd.forwardmove *= 0.5f;
    pmove->cmd.sidemove    *= 0.25f;
    pmove->cmd.upmove      *= 0.25f;
    if (pmove->cmd.buttons & IN_JUMP)
        pmove->gravity = (pmove->velocity[2] < 0.0f) ? 0.75f : 0.30f;
    else
        pmove->gravity = 1.0f;
}
```
Place it where stock `PM_CheckParamters` clamps params (after the velocity clamp,
before the move). This must run in pm_shared so it predicts identically client &
server.

### (d) `dlls/player.cpp` — `CTSStunt::Play` (or wherever we own it) + `PostThink`
- In the per-`PreThink` stunt update: `if (iuser4 & 0x200000) { iuser4 |= 0x100000;
  iuser4 &= ~0x200000; /* play launch anim */ }`.
- In `PostThink` landing: `if (iuser4 & 0x100000) { UTIL_ScreenShake(origin, 20,
  30, 0.3, 512); iuser4 &= ~0x100000; }`. Also clear `0x100000` on `TakeDamage`
  and on stunt/wall-jump, per the table in §2.

### Do NOT
- Do not set a constant `pev->gravity = 0.15` on pickup (the REV 1 bug).
- Do not add a max-hold *timer* — the cap is emergent (velocity decay + landing).
- Do not lower the impulse below 400 — fix the gravity (0.30/0.75/1.0) instead.

---

## Raw decompiles saved — `reference/dossiers/cont86/decomp/`
`PM_CheckParamters.c` (the gate), `PM_Jump.c`, `PlayerAddPowerUp.c`,
`PreThink.c`, `PostThink`(in `raw_flight_bit.txt`), `UpdatePowerUpState.c`
(dead path), `SetPlayerGravity.c`, `CBasePlayer_Jump.c`, `CTSSuperJump.c`,
`gravity_funcs.c`, `raw_flight_bit.txt` (CTSStunt conversion + gate + landing raw).

## Uncertainty ledger
- **HIGH:** `PM_CheckParamters` gate (0.30 rising / 0.75 falling / 1.0 released),
  gated on `iuser4 & 0x100000` + `IN_JUMP` + `velz` sign; air-control 0.5/0.25/0.25;
  usercmd offsets; the `0x200000`→`0x100000` conversion in `CTSStunt::Play`;
  landing clear in `PostThink`; `PlayerAddPowerUp` grants ability only (no gravity);
  400/236.64 impulse constants; `UpdatePowerUpState` 0.15 is DEAD for the powerup.
- **MED:** exact set of every `0x100000` clear site's *condition* (landing/damage/
  stunt confirmed by containing function; each function's precise guard not fully
  transcribed). The order of `PM_CheckParamters` vs `PM_Jump` within
  `PM_PlayerMove` (so whether the launch frame uses reduced gravity) — inferred,
  not step-traced.
- **LOW / out of scope:** `SetPlayerGravity`/`field_0x4ec` virtual caller +
  purpose (separate facility, not super-jump).
