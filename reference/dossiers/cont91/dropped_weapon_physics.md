# Dropped-weapon physics — root-cause dossier (cont91)

**Scope:** the four "dropped weapon on the ground" bugs. READ-ONLY investigation; no source
edited, no build. Fix specs below are ready to apply without re-RE.

## TL;DR

The dropped weapon is **`CTSWorldGun`** (runtime classname `"WorldGun"`), NOT the map-placed
`CTSGroundWeapon`/`ts_groundweapon`. It is spawned in code by
`CBasePlayer::DropWeapon`@0x84900 → `CBasePlayer::ThrowWeapon`@0x8529c →
`CTSWorldGun::CreateWeapon`@0x907ec, and lives in `dlls/ts_worldgun.cpp` / `dlls/ts_worldgun.h`.
`dlls/ts_groundweapon.h` (the map pickup) is a **red herring** for these bugs.

Two concrete divergences from `ts_i386.so` explain all four symptoms:

1. **`CTSWorldGun::BounceTouch` drops the on-ground horizontal damping** — the gun never sheds
   horizontal velocity, so it slides/bounces forever (bug a) and thereby travels off edges / into
   thin geometry (bugs b, c). `dlls/ts_worldgun.cpp:130-140`.
2. **`CTSWorldGun::ObjectCaps` returns `FCAP_IMPULSE_USE` (0x8) instead of the original's
   `FCAP_CONTINUOUS_USE` (0x10)** — pickup only fires on a precise +use tap instead of while +use is
   held, making pickup flaky, worst near edges (bug d). `dlls/ts_worldgun.h:30`.

Both are single-line/small edits.

---

## The entity (confirmed 1:1)

`CTSWorldGun::Spawn`@0x90954 (Ghidra 0xa0954) — matches our source exactly:

| property        | original value                         | ours (`ts_worldgun.cpp:82-90`) |
|-----------------|----------------------------------------|--------------------------------|
| movetype        | `10` = `MOVETYPE_BOUNCE`                | ✅ MOVETYPE_BOUNCE             |
| solid (falling) | `2` = `SOLID_BBOX`                      | ✅ SOLID_BBOX                  |
| gravity         | `0.7`                                  | ✅ 0.7                         |
| friction        | `0.8`                                  | ✅ 0.8                         |
| bbox (falling)  | `SetSize(0,0)` (point; passes players) | ✅ g_vecZero/g_vecZero         |

`CTSWorldGun::OnGround`@0x910a4 — settle box also matches:
`solid = 1 (SOLID_TRIGGER)`, `SetSize((-4,-4,0),(4,4,4))` (const `DAT_0010a788` = -4, maxs.z=4).
Ours: `ts_worldgun.cpp:206` `UTIL_SetSize(pev, Vector(-4,-4,0), Vector(4,4,4))`. ✅ Box bottom is
flush with the floor (mins.z = 0), so the box itself does **not** sink.

`CBasePlayer::ThrowWeapon`@0x8529c — drop placement (matches `ts_gun.cpp:3021-3060`):
`origin = pev->origin + view_ofs + 16*v_forward + lateral*v_right`, then
`velocity = 0.9*playerVel + S*v_forward`, `velocity.z += 0.33*S`, with
`S = (90 - v_angle.x)*2` alive / `RANDOM(60,160)` dead; `avelocity.y/.z = RANDOM(-300,300)`.
**No floor trace / DROP_TO_FLOOR exists in the original** — so parity is NOT about adding a trace.
Because `S` is 60..180, a thrown gun essentially always lands with horizontal speed ≫ 40 (this is
what makes bug a fire on every drop).

Support code confirmed equivalent: `CTSPhysicObject::Spawn`@0x7ba54, `SavePhysics`@0x7bac0,
`WaitThink`@0x7bd98 all match `ts_grenades.cpp:32-81`. `GiveSlowMul()` = `max(fuser1, 0.01)` = 1.0
when not slowed (`cbase.h:181`).

---

## Bug (a) — ENDLESS FRICTION (never stops sliding)  ★ROOT CAUSE

### Original `BounceTouch`@0x90b0c (Ghidra 0xa0b0c), on-ground branch

Disassembly (0x90b48–0x90c91), constants read from `.rodata`:

```
velocity.z *= 0.3                      ; double @0xf7fb8 (top of fn, both branches)
if (flags & FL_ONGROUND) {             ; testb $0x2, 0x1a5(pev)  (FL_ONGROUND = bit 9)
    slow  = GiveSlowMul()              ; call *0x10(vtbl)
    horiz = sqrt(vx*vx + vy*vy)
    if (horiz <= 40.0 * slow) {        ; threshold const 40.0 @0xf6d9c
        velocity  *= 0.7               ; const 0.7 @0xf7f50   (+0x20 = velocity)
        avelocity *= 0.7               ;                       (+0x5c = avelocity)
    } else {                           ; horiz > 40*slow
        velocity  *= 0.9               ; const 0.9 @0xf7f44
        avelocity *= 0.9
    }
    angles.z = 0
}
```

Key point: **BOTH** on-ground branches damp `velocity` and `avelocity`. Fast slides decay ×0.9 per
ground contact until `horiz <= 40`, then decay ×0.7 until `Fall`'s `< 3` settle threshold catches
them → the gun stops within ~1–2 s near where it landed.

### Ours (`dlls/ts_worldgun.cpp:130-140`)

```cpp
else                                                        // on the ground
{
    float horiz = Vector(pev->velocity.x, pev->velocity.y, 0).Length();
    if (horiz <= 40.0f)
    {
        pev->velocity = pev->velocity * 0.7f;   // restitution
        pev->avelocity = pev->velocity;         // ← BUG: avel = 0.7*velocity, not avel*0.7
    }
    // ← BUG: NO else — horiz > 40 leaves velocity/avelocity UNTOUCHED
    pev->angles.z = 0;
}
```

**Two defects:**
1. **Missing `else` branch.** When `horiz > 40` (always true for a real throw, `S=60..180`), our
   code does not damp at all. The gun keeps full horizontal velocity while `FL_ONGROUND` is set →
   it never drops below 40 → never reaches `Fall`'s `< 3.0` settle → **slides forever.**
   (MOVETYPE_BOUNCE/TOSS have no engine ground friction, so BounceTouch is the *only* thing that can
   bleed horizontal speed once the gun is on the ground.)
2. **`avelocity = pev->velocity`** sets angular velocity to 0.7×*linear* velocity instead of
   scaling the previous avelocity. Wrong tumble (cosmetic), but still wrong vs original.

### FIX (a)  — `dlls/ts_worldgun.cpp`, replace lines 132-140 (the `else` block body)

```cpp
    else
    {
        // On the ground: bleed horizontal speed each contact so the gun settles.
        // @0xa0b0c: horiz <= 40*GiveSlowMul() -> x0.7 (strong); else -> x0.9 (light).
        // Both scale velocity AND avelocity by the same factor.
        float slow  = GiveSlowMul();
        float horiz = Vector(pev->velocity.x, pev->velocity.y, 0).Length();
        float k     = (horiz <= 40.0f * slow) ? 0.7f : 0.9f;
        pev->velocity  = pev->velocity  * k;
        pev->avelocity = pev->avelocity * k;
        pev->angles.z = 0;
    }
```

`GiveSlowMul()` is inherited from `CBaseEntity` (`cbase.h:181`) — callable here.

---

## Bug (b) — steep downward drop sinks BELOW the floor  &  Bug (c) — near-edge disappears below floor

These are **secondary effects of bug (a)**, not independent code defects (verified: the original
`ThrowWeapon`@0x8529c and `Spawn`@0x90954 do **no** floor trace / no origin clamp; our placement is
1:1). Mechanism:

- Because the on-ground branch never damps (bug a), a landed gun retains full horizontal momentum
  and keeps sliding/bouncing as a **zero-size point** (SetSize(0,0) while falling). A zero-size
  point moving fast can slip through the sliver gap at a floor **edge** between frames, and slides
  off ledges into the void → "disappears below the floor" (c). On a steep/hard drop it lands hot and
  skates past the edge before the (currently broken) settle can catch it (b).
- The original bleeds that momentum via the ×0.9/×0.7 damping and settles the gun near the landing
  point within a couple of ticks, so neither symptom occurs.

**Fix:** the BounceTouch fix (a) restores original settling and eliminates (b)/(c).
`// VERIFY` on a live steep-drop + near-edge drop after applying (a) — if a residual edge-tunnel
remains, it is an engine point-trace artifact present in the original too (behaviour parity), not a
new defect; do NOT add a floor trace the original lacks.

### Secondary parity item (low priority, slow-motion only) — `CTSWorldGun::Fall`

Original `Fall`@0x90d84:
```
if (flags & FL_ONGROUND) {
    if (sqrt(vel) < 3.0 * GiveSlowMul())   ; NOTE: threshold scaled by slow-mul
        { sound; angles.x=0; angles.z=0; OnGround(); }   ; NO early return
}
if (time <= expire) { <swept-trace damage>; CTSPhysicObject::WaitThink(); }
else { RemoveFromSlowedArray; UTIL_Remove; }
```
Ours (`ts_worldgun.cpp:150-157`):
```cpp
if ((pev->flags & FL_ONGROUND) && pev->velocity.Length() < 3.0f)
{
    EMIT_SOUND_DYN(...); pev->angles.x = 0; pev->angles.z = 0;
    OnGround();
    return;                       // ← original has NO return; falls through to trace + WaitThink
}
```
Differences: (i) threshold `3.0f` should be `3.0f * GiveSlowMul()`; (ii) the early `return` skips
the swept-trace + `CTSPhysicObject::WaitThink()` on the settling frame (harmless when not slowed —
`OnGround` re-points think to `WaitThink` anyway). Also the in-air sound threshold
`ts_worldgun.cpp:123` `speed > 18.0f` should be `speed > 18.0f * GiveSlowMul()` (const 18.0 @0xf92b0,
scaled by slow-mul in the original). **These only diverge in bullet-time (fuser1 < 1)** and do not
affect the reported bugs. `// VERIFY` — apply for strict parity:
- line 123: `if (m_bDropSnd && speed > 18.0f * GiveSlowMul())`
- line 150: `... && pev->velocity.Length() < 3.0f * GiveSlowMul())` and drop the `return;` so the
  swept-trace/`WaitThink` block runs on the settle frame (matches the original fall-through).

---

## Bug (d) — near-edge weapons can't be picked up  ★ROOT CAUSE

### Original `CTSWorldGun::ObjectCaps`@0x93404

```asm
00093404 <ObjectCaps__11CTSWorldGun>:
  93404: b8 10 00 00 00   mov  $0x10,%eax     ; return 0x10 = FCAP_CONTINUOUS_USE
  93409: c3               ret
```
(`CanUsedTroughWalls`@0x9340c returns 0 — ours matches.)

### Ours (`dlls/ts_worldgun.h:30`)

```cpp
int  ObjectCaps() override { return FCAP_IMPULSE_USE; }   // @0xa3404 -> 0x10
```

`FCAP_IMPULSE_USE = 0x8`, but the original returns `0x10 = FCAP_CONTINUOUS_USE` (the inline comment
already documents 0x10 — the code just uses the wrong constant). In `CBasePlayer::PlayerUse`:
- `FCAP_CONTINUOUS_USE` → `Use()` fires every frame **`pev->button & IN_USE` is held** — walk up,
  hold +use, it grabs as soon as the sphere/cone search finds it.
- `FCAP_IMPULSE_USE` → `Use()` fires only on the **`m_afButtonPressed & IN_USE` edge** (a single
  tap while precisely aligned). Near an edge (awkward approach angle, tiny 8×8×4 grab box) the tap
  frequently misses → "can't be picked up." Compounded by bug (a) sliding the gun into a wall/edge
  where its trigger box is unreachable.

Note the map-placed twin `CTSGroundWeapon` (`ts_groundweapon.h`) already correctly returns
`FCAP_CONTINUOUS_USE`; only the player-dropped `CTSWorldGun` has the wrong constant.

### FIX (d) — `dlls/ts_worldgun.h:30`

```cpp
int  ObjectCaps() override { return FCAP_CONTINUOUS_USE; }   // @0x93404 -> 0x10
```

---

## Fix summary (apply order)

| # | file:line                     | change                                                              | bug(s) |
|---|-------------------------------|---------------------------------------------------------------------|--------|
| 1 | `dlls/ts_worldgun.h:30`       | `FCAP_IMPULSE_USE` → `FCAP_CONTINUOUS_USE`                           | d      |
| 2 | `dlls/ts_worldgun.cpp:132-140`| add `else` (×0.9) + damp `avelocity *= k` (see FIX (a))              | a,b,c  |
| 3 | `dlls/ts_worldgun.cpp:123,150`| `*= GiveSlowMul()` on the 18/3 thresholds; drop the `Fall` early `return` (VERIFY, slow-mo parity only) | parity |

## Evidence index (ts_i386.so file addresses; Ghidra = file + 0x10000)

- `CTSWorldGun::Spawn` 0x90954 · `BounceTouch` 0x90b0c · `Fall` 0x90d84 · `OnGround` 0x910a4
- `ObjectCaps` 0x93404 (→0x10) · `CanUsedTroughWalls` 0x9340c (→0)
- `CBasePlayer::DropWeapon` 0x84900 · `ThrowWeapon` 0x8529c · `CreateWeapon` 0x907ec
- `CTSPhysicObject::Spawn` 0x7ba54 · `SavePhysics` 0x7bac0 · `WaitThink` 0x7bd98
- rodata constants: 40.0 @0xf6d9c · 0.7 @0xf7f50 · 0.9 @0xf7f44 · 0.3(double) @0xf7fb8 · 18.0 @0xf92b0
