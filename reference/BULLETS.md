# TS bullet / breakable / slow-mo / sound RE (cont.69u)

The recovered contract for the TS bullet system, breakable shatter, slow-motion bullets, and the
weapon-sound pitch. Constants: **file offset = Ghidra addr − 0x10000** (the .so is rebased +0x10000
in Ghidra); many FP constants are **doubles** the decompiler shows as `(float)`. See the
`ghidra-rodata-const-offset` memory.

## ★ LIVE fire path (NOT stock FireBulletsPlayer) — confirmed by callers
`CTSGun::StandardFire` / `ShotgunFire` / `AkimboFire` → **`CBasePlayer::ShootBulletsPlayer`**
(@0x6f69c, ours currently uses stock `FireBulletsPlayer` = the faithfulness GAP). ShootBulletsPlayer
**recurses into itself** for penetration. User confirmed this is the correct/live behavior.

### Per-shot flow (the `rbullet==0` instant-hitscan branch — normal play)
- `rbullet` gate (line ~72): `rbullet = (TSGameRules+0x578 != 0) || (player_speed < DAT_00106e28)`.
  TSGameRules+0x578 = the **slow-PAUSE** state; when set (or the player is moving very slowly), the
  shot is handed to `CBulletManager::FireBullet` (= bullets become **tracked models**, the slow-mo
  bullet-as-model effect — see below). Otherwise instant hitscan. **Both paths are LIVE**, switched
  by the slow-pause flag. (User: "slowmo doesn't get bullets as models, but slow PAUSE does.")
- per pellet: spread via `UTIL_SharedRandomFloat` (cone from ammo_info+0x1c), trace
  `UTIL_TraceLine_Expanded` (TS trace variant), range-falloff damage
  `get_range_multipler(distance_index) * base_damage`, `TraceAttack(..., DMG bits 0x1002)`,
  `UTIL_BubbleTrail`.

### ★ PENETRATION (the pierce recursion) — ShootBulletsPlayer lines ~290-329
After a pellet hits at `tr.endpos`, if the shot mode allows it (`param_8` ∈ {0,3,4,…}, i.e. NOT 1
or 2) and `damage > DAT_00106e08` (**5.0**):
1. `penDepth = min(damage * 2, DAT_00106da8)` where **DAT_00106da8 = 48.0** (max penetration step).
2. `exitPoint = tr.endpos + bulletDir * penDepth`.
3. `if (UTIL_PointContents(exitPoint) != CONTENTS_SOLID(-2))` → the surface is **thinner than the
   penetration depth** (bullet exited) → **recurse** `ShootBulletsPlayer(src=exitPoint, mode=2,
   damage=damage*adjust)` to continue and hit whatever is beyond.
The `mode==2` branch (lines ~100-165) is the "piercing exit" shot: a single TraceLine 4096u forward
(`DAT_00106e2c = 4096`), range-mult damage, `TraceAttack(..., 0x1002)`. So "fire at glass → glass
breaks AND the bullet hits the solid past it" falls straight out of this (glass is thin → exits →
recurses). **Slice thickness = enter→exit distance; penetrates iff thickness < min(dmg×2, 48).**
- Penetration constants: threshold dmg 5.0 @0x106e08(dbl); depth cap 48.0 @0x106da8(flt);
  pierce trace 4096 @0x106e2c; range index scale 0.0033333(=1/300) @0x106da0 + 10.0 @0x106da4;
  flesh/piercing dmg ×0.5 @0x106e00(dbl).

### DECALS (enter + exit) — TODO confirm placement site
User wants BOTH the entry and exit surface to get a bullethole decal. The pierce recursion gives
two TraceAttack impacts (enter + exit); the decal is likely placed per-impact (client EV_TSFire
re-trace, or server DecalGunshot@0xf0e7c). **Needs a focused RE pass** to pin the exact decal call
sites for both faces. (Not yet implemented.)

## ★ Glass shatter = sprite shower (Die__10CBreakable @0x752e0) — confirmed
`switch (pev material, this+0x78)`:
- **case 0 = GLASS → `gmsgTSCreateDebris`** (a custom user-message → CLIENT spawns a **sprite
  particle shower**). debris/bustglass{1,2}.wav.
- **case 1 (wood/crate), 2/6 (metal), 3 (flesh), 4/8 (concrete), 5 (ceiling) → standard
  `TE_BREAKMODEL`** (svc_temp_entity msg id 0x17 = the gib **MODEL**). bustcrate/metal/flesh/concrete.
This is EXACTLY the user's spec: **glass = sprites, everything else (wood…) = model.**
### gmsgTSCreateDebris payload (glass = material byte 0) — FULLY RE'd
Server `Die` (case 0) MESSAGE_BEGIN(MSG_PVS, gmsgTSCreateDebris, center) then:
WRITE_COORD origin(center) ×3 · WRITE_COORD pev->size ×3 (pane dims) · WRITE_BYTE 0 (material=glass)
· WRITE_COORD shatterDir ×3 · WRITE_COORD refPoint ×3 · WRITE_SHORT spriteModelIndex
· WRITE_COORD velMag (= base × material mult). Client `MsgFunc` (@0x1004becc) reads them in that
order and calls the spawner FUN_10047cb0(origin[3], size[3], 0, dir[3], ref[3], flags, velMag).

### Client spawner FUN_10047cb0 — the 81-fragment sprite shower (FULLY RE'd)
- Zeroes the SMALLEST of the 3 size components (= the pane normal); lays a **9×9 = 81 grid** of
  `sprites/glassfrag.spr` temp-sprites across the pane FACE (the two largest dims), stepping by
  `size_dim * _DAT_1007d740` per cell from `origin - size*_DAT_1007a870` (i.e. centred on the pane).
- Each fragment (engine TEMPENTITY via the pEfxAPI alloc/add = FUN_1004ffe0/FUN_10050b60):
  rendermode kRenderTransAdd-ish, gravity puVar6[0x38]=**0.05**, scale puVar6[0x42] from pane area
  (capped), velocity = **dir × velMag** with `RANDOM_FLOAT` spread (flag bit 0x8000002 switches to a
  radial-from-refPoint blast), rotation puVar6[0x13]=RANDOM(-90,90), **life puVar6[0x36]=RANDOM(2.0,
  5.0)s**.
- **Shatter direction = `g_vecAttackDir`** (a server global set every hit in `CBreakable::TakeDamage`
  @0xc1370 = normalize(breakableCenter − attackerOrigin) ≈ the bullet travel direction). Confirms the
  user's "shatters in the bullet entry direction."
- **Reconstruct client-side** with `gEngfuncs.pEfxAPI->R_TempSprite` per fragment (origin, dir×velMag
  velocity, glassfrag model index, kRenderTransAdd, life 2-5, gravity), OR the TEMPENTITY API directly.
- **Remaining server unknown:** the velMag base + spriteModelIndex source (this+0x110/0x114 — likely a
  keyvalue/material default or env_debris; Spawn doesn't set them). Pick a sensible velMag (≈ pane size
  / a small constant) until pinned; the sprite is `sprites/glassfrag.spr` (precache it).

## Slow-mo bullets as models — CBulletManager (@CTSGameRules+0x1864)
`FireBullet` (@0x6eda4) allocates a slot in a 256-bullet ring (`this[i*0x10]`), storing
owner/origin/dir/velocity/weaponid/spawn-time. `UpdateBullet` (@0x6ef5c) steps each bullet per
think: advances by `velocityMult * speed * frametime` (velocity from `GetBulletVelocityMult` — slows
as it travels), traces, on hit applies damage (with the SAME pierce recursion: depth ∝ damage,
`PointContents` exit check, re-seed the bullet past the surface). `Think`/`UpdateBulletTimeSlice`
drive it under slow-mo. Active only when `rbullet` (slow-pause) — see the gate above.

## Weapon sound pitch ("all weapons recorded slow → sped up in game") — HYPOTHESIS, not yet pinned
User report: the weapon WAVs are recorded at a slowed pitch and played back **sped up** in normal
play, so that during slow-motion (lower pitch) they reach their intended sound. The fire SOUND is
**client-side** (the fire event handler `FUN_1003dfa0`, not a server EMIT). The pitch is presumably
set from the client's slow-mo factor (play fast at factor 1, slow toward the recorded pitch as the
factor drops). **The exact pitch computation / where it reads the slow factor is NOT yet pinned** —
needs decompiling the client fire-sound emit (the PlaySound call in FUN_1003dfa0) + the slow-mo
factor it multiplies. (Breakable Die uses ordinary pitch ~95-124 random — unrelated.)

## Dive/run THROUGH breakables — CTSStunt::CheckForBreakables (@0x9bd9c) — RE'd, not impl
Called every think from `CBasePlayer::PreThink` (throttled once/frame via this+0x18). This is the
"jump/dive before reaching the glass and it shatters" mechanic (distinct from the jump-kick, which
plays a kick anim from CloseAttack):
- gate: player speed must be ≥ **64** (`DAT_00107ea0`) (or be mid-stunt-damage).
- hull-traces forward (TraceHull, hull 3 = head_hull): **in the VELOCITY direction** ×64u when not
  aiming (player+0x1e7==0), else along v_forward ×32u (`DAT_00106dac`).
- on a valid hit (takedamage≠0, !EF_NODRAW, alive, not the last-hit this+0x798): `TraceAttack(damage,
  DMG bits 0x1080 / 0x4001080)` + ApplyMultiDamage, where **damage = speed** (× a 0.1 factor when
  grounded vs full when airborne — `DAT_00107138`). Knockback (>30) + screen-punch (>20) + the
  kungfu/knife/katana "victim hold" sounds for people. Stores the hit as this+0x798 to not re-hit.
So: **fly fast into glass → it breaks (damage = your speed), letting you dive through**; run into a
person → shove/hold them. No viewmodel kick. Hook = a per-think check in PreThink (we have the
jump-kick in Jump already; this is a separate, velocity-directed barge). NOT implemented yet.

## Implementation status / order (proposed)
1. ✅ (none yet implemented from this doc.)
2. **Glass sprite shatter** (task #4) — most self-contained: server Die override routes glass →
   a debris message; client spawns the sprite spray in the bullet-entry direction. Need: the client
   gmsgTSCreateDebris handler RE + the glass sprite model + the init velocity base.
3. **Bullet penetration + dual decals** (task #5) — reconstruct ShootBulletsPlayer (replace stock
   FireBulletsPlayer in StandardFire/ShotgunFire/AkimboFire): range falloff + the pierce recursion;
   enter+exit decals.
4. **Slow-mo bullets-as-models + sound pitch** — larger; tied to the powerup/slow-pause state.

**Flag (per user guidance — live vs dead code):** ShootBulletsPlayer IS live (StandardFire et al.
call it; our stock FireBulletsPlayer is the gap). CBulletManager IS live (slow-pause gated).
gmsgTSCreateDebris IS live (Die case 0). None of these are dead-code paths.
