# CTSGun::KnifeFire — analysis (original ts_i386.so @ nm 0x74044 / Ghidra 0x84044)

## 1. SIGNATURE

```
void CTSGun::KnifeFire()            // signatures-gcc2.txt:1132 "CTSGun::KnifeFire()"
// mangled: KnifeFire__6CTSGun, nm 0x74044 (Ghidra 0x84044), non-virtual, no args,
// spans 0x74044..0x74a13 (~0x9d0 bytes; next symbol ReloadWeapon__6CTSGuniif @0x74a14).
// Source file (filemap plate): TS_gun.cpp.
```

Related originals (all separate methods — the knife/katana system is SPLIT):
- `CTSGun::KatanaFire()` @0x733f0 (katana primary — NOT handled by KnifeFire)
- `CTSGun::KatanaThrow()` @0x73b68 (katana secondary)
- `CTSKnife::CreateKnife(CBasePlayer*, Vector, Vector, int)` @0x6d9e4 (mangled `CreateKnife__8CTSKnifeP11CBasePlayerG6VectorT2i` — pos, vel, int knife-type; **no damage param**)
- `VictimKnifeHold(CBasePlayer*, CBaseEntity*, float)` @0x66de4 (TS_gamerules.cpp)

## 2. BEHAVIOR

KnifeFire handles BOTH the knife slash and the knife throw; the branch selector is
`m_pActiveItem->[+0x24]` (decompile name "pumpToggle" — a per-status alt-mode byte),
written by the CALLER: PrimaryAttack writes 0 (slash), SecondaryAttack writes 1 (throw).
KnifeFire itself has **no button or timer gates** (ItemPostFrame + the caller gate those).

### Entry + common setup
1. `if (!m_pActiveItem /*this+0xcc*/) return;`
2. `stunting = (m_pPlayer->pev->iuser4 & 0x70C30) != 0` (TS stunt/lock-state mask).
3. `rate = m_pActiveInfo->flFireRate` (float @ info+0x18); if stunting `rate *= 1.5`
   (const @file 0xf7f24).
4. `m_flNextSecondaryAttack (this+0xa0) = m_flNextPrimaryAttack (this+0x9c)
   = UTIL_WeaponTimeBase() + rate` — BOTH timers.
5. `m_flTimeWeaponIdle (this+0xa8) = UTIL_WeaponTimeBase() + 3.0` (double @0xf76a8).
6. `m_pPlayer->[+0x4f8] (float) = rate` — un-curated player member; also written by the
   CloseCombatAttack tail (FUN_00086c8c). Meaning: "duration of the melee attack in
   flight" (VERIFY exact consumer; likely the whiff/lunge PreThink system).
7. `UTIL_MakeVectors(m_pPlayer->pev->v_angle)`.
8. A `consumed` stack flag is initialized to **0** (disasm: `movl $0,0x38(%esp)` @0x74092).

### SLASH branch (`status+0x24 == 0`)
1. `r = RANDOM_LONG(0,2)`; if `!stunting && (pev->flags & 0x200 /*FL_ONGROUND*/)`
   → `CTSAnimation::Animate(&player+0x28c, player, 0x1D + r)` (3rd-person slash anims 29–31).
2. Damage: `dmg = *(float*)(info+0x1c)` (flDamage); if stunting `dmg *= 0.666667`
   (@0xf7f20); then `dmg *= m_pPlayer->GetMeleeMultiplier()` (direct call).
3. `vecSrc = m_pPlayer->GetGunPosition()` — **virtual**, CBasePlayer vtable slot 0x168
   (VTABLES.md: `0x168 → CBasePlayer::GetGunPosition()`; the decompile's
   `vtbl[1].TraceAttack` is a slot-page artifact of exactly this call).
4. `vecEnd = vecSrc + gpGlobals->v_forward * 35.0` — melee range **35 units**.
5. `CBasePlayer::CheckHull(player, vecSrc, vecEnd, &tr, 3)` — a **hull-3 (head hull)
   sweep**, not a point traceline (`CheckHull__11CBasePlayerG6VectorT1R11TraceResulti`).
6. Victim resolution: if `tr.flFraction < 1.0` (double cmp @0xf6df8): if `tr.pHit == 0`
   → `pfnPEntityOfEntOffset(0)` (worldspawn); `pEnt = pHit->pvPrivateData (edict+0x7c)`;
   if `!pEnt->IsAlive()` (vslot 0xb4) **or** `(pEnt->pev->effects & 0x80 /*EF_NODRAW*/)`
   → pEnt = NULL (world/props take NO server damage from the slash).
7. If pEnt alive: `VictimKnifeHold(m_pPlayer, pEnt, dmg)` — the melee-block gate
   (TS_gamerules.cpp, in melee_refs.txt): returns 1 (attack absorbed) iff the victim
   IsPlayer AND (`victim+0x7ac > 0` /knife block window/ OR (`victim+0x7b0 > 0`
   /katana block window/ AND `victim+0x774 > 0.1` /block bar/)); on absorb it plays
   `UTIL_Sparks(midpoint+20z)` + `"weapons/knife/knife_hit.wav"` (CHAN 4, vol 1, attn 0.8,
   pitch 100), zeroes `attacker+0x7a0`, sets `attacker+0x12c = 0.75f`,
   `victim+0x12c = 0.35f` (raw 0x3f400000... attacker 0.75/victim 0.35 — VERIFY 0x12c
   semantics: used by CCombatBlock as a next-block gate) and `UpdateMeleeBar(attacker, 33.0)`.
8. If NOT blocked: `ClearMultiDamage(); pEnt->TraceAttack(m_pPlayer->pev, dmg,
   gpGlobals->v_forward, &tr, 0x1004)` (vslot 0x54; **0x1004 = DMG_SLASH|DMG_NEVERGIB**);
   `ApplyMultiDamage(m_pPlayer->pev, m_pPlayer->pev)`.
9. If `pEnt->IsPlayer()` (vslot 0xc8) — flesh hit extras:
   - `m_pActiveItem->[+0x8] = 1` (decompile "burstOrPhase"; for knives = **bloody flag**),
   - `this->pev->body = 1` (**bloody knife body** on the world/p_ model),
   - `EMIT_SOUND_DYN(pEnt->edict(), 4 /*CHAN_BODY*/, "weapons/knife/knife_hitbody.wav",
     1.0, 0.8, 0, 95+RANDOM_LONG(0,31), 0.5)` — from the **victim's** edict. (TS's
     EMIT_SOUND_DYN has a trailing extra float 0.5 — `__FP7edict_siPCcffiif` — VERIFY
     its meaning; every melee call passes 0.5.)
10. If NO victim (miss OR non-living hit): open the **whiff window** on the player:
    - `player+0x79c (float) = 0.25 * info->flFireRate` (0.25 @0xf7e98) — window duration,
    - `player+0x7a0 (float) = dmg` — stored swing damage/lunge magnitude,
    - `player+0x7a4 (byte) = 1` — "knife-flavored" whiff (VictimKungfuHold reads it).
11. **Always** (hit or miss):
    `pfnPlaybackEvent(1 /*FEV_NOTHOST*/, player edict, m_us[knife.sc] (this+0x972), 0,
    g_vecZero, g_vecZero, fp 0, 0, iparam1 = m_weaponType (this+0xc8),
    iparam2 = status+0x8 bloody flag, bparams 0,0)`.
12. `consumed` stays 0 → the slash NEVER decrements the clip (disasm: the slash branch
    jumps from 0x74491 straight to the `cmpl $0,0x38(%esp)` gate @0x748f4).

Event-handle map (recovered from Precache__6CTSGun @0x70660): this+0x964 fire.sc,
0x966 coldcock.sc, 0x968 empty.sc, 0x96a pump.sc, 0x96c closecombat.sc,
0x96e firelinked.sc, 0x970 reload.sc, **0x972 knife.sc**, 0x974 katana.sc,
**0x976 draw.sc**, 0x978 grenade.sc.

### THROW branch (`status+0x24 != 0`)
1. `start = pev->origin + pev->view_ofs`;
   `end   = start + v_forward*32 + v_right*1`;
   `src   = start + v_right*1`;
   `UTIL_TraceLine(src, end, dont_ignore_monsters(0), player edict, &tr)`;
   spawn position = `tr.vecEndPos` (wall-clipped muzzle point).
2. `vel = v_forward * 1900.0` (float @vaddr 0x107f68→file 0xf7f68 = **1900**). The
   `*(int*)(info+0x20) == 6` if/else picks between two IDENTICAL 1900 loads — dead
   differentiation (knife vs seal-knife in source, same constant). **No player-velocity
   term.**
3. `pKnife = CTSKnife::CreateKnife(m_pPlayer, pos, vel, *(int*)(info+0x20))` — the 4th
   arg is the knife-type int at info+0x20 (== 6 for one type; VERIFY which).
4. If pKnife: orient the projectile:
   - `pKnife->pev->angles = UTIL_VecToAngles(vel.Normalize())` (writes entvars+0x50..0x58),
   - `pKnife->pev->angles.z = 90.0` (raw 0x42b40000 → +0x58),
   - `pKnife->pev->avelocity.x = -1300.0` (raw 0xc4a28000 → +0x5c) — end-over-end spin.
5. `consumed = 1` — set at the merge point 0x74865, i.e. **even if CreateKnife failed**
   (the null-check `je 0x74865` lands ON the flag write).
6. If `!(iuser4 & 0x30C30)` (narrower mask than the slash's 0x70C30) `&& FL_ONGROUND`
   → `Animate(player, 0x20)` (throw anim 32).
7. `EMIT_SOUND_DYN(player edict, 4, "weapons/knife/knife_miss.wav", 1.0, 0.8, 0,
   95+RANDOM_LONG(0,31), 0.5)` — always (the throw "whoosh").
8. **No PlaybackEvent in the throw branch itself** (only the epilogue's draw.sc below).

### Epilogue (`if (consumed)` — throw only)
1. `if (!InfiniteClips(m_pPlayer))` (@0x65fc8, powerups & 2):
   - `player+0x548 += (byte)info->slotCost (info+0xd0)` — the thrown knife frees carry
     slots (0x548 drives move speed),
   - `CBasePlayer::SetPlayerSpeed(GetSpeedBySlots(player))` — speed recomputed,
   - `m_pActiveItem->iClip-- (status+0x4)`.
2. `if (iClip <= 0)` → `DropWeapon(m_weaponType, true)` (threw the last knife: the
   weapon leaves the arsenal; `jle`; DropWeapon__6CTSGunib bparam 1).
3. else (knives remain) — redraw the next knife:
   - `SendWeaponAnim(status+0x28 ? *(short*)((*(int*)(status+0x28)) + 0x28) : 0, 1, 0)`
     (**virtual**, vslot 0x174 = CBasePlayerWeapon::SendWeaponAnim(int,int,int)) — the
     draw-anim index read through the status' model/info pointer at +0x28 (VERIFY the
     struct: a weapon-model record whose +0x28 short is the draw sequence),
   - `pfnPlaybackEvent(1, player edict, m_us[draw.sc] (this+0x976), 0, g_vecZero,
     g_vecZero, 0, 0, iparam1 = m_weaponType, iparam2 = status->attachEquipped (+0x10),
     0, 0)` — the client redraw event,
   - `m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 2.0` (@0xf7fc0).

### Constants table
| value | meaning |
|---|---|
| 0x70C30 | iuser4 stunt/lock mask: slows melee (rate ×1.5, dmg ×2/3), blocks slash anim |
| 0x30C30 | iuser4 mask gating only the 3rd-person throw anim |
| 1.5 / 0.666667 | stunting fire-rate / damage multipliers (@0xf7f24 / @0xf7f20) |
| 35.0 | slash reach (units), hull-3 sweep |
| 0x1004 | DMG_SLASH \| DMG_NEVERGIB |
| 0.25 × flFireRate | whiff-window duration (player+0x79c) |
| 3.0 / 2.0 | idle delay after fire / after redraw |
| 1900 | throw speed along v_forward |
| 32 / 1 | throw muzzle trace: forward / right offsets |
| 90.0 / −1300.0 | thrown knife angles.z roll / avelocity.x spin |
| 29–31 (0x1D+r), 32 (0x20) | 3rd-person slash anims (random of 3) / throw anim |
| CHAN 4, vol 1.0, attn 0.8, pitch 95+rand(0,31), trailing 0.5 | both EMIT_SOUND_DYN calls |
| 33.0 | UpdateMeleeBar drain on a successful block (in VictimKnifeHold) |

## 3. CALLERS

Direct calls (objdump; only these three — `refs` is empty because there are no others):
- **CTSGun::PrimaryAttack @0x71a20** — the fire-type dispatcher (byte @info+0x348 = iFireType):
  - `iFireType == 3` (KNIFE): `status+0x24 = 0` then `KnifeFire()` → SLASH.
  - `iFireType == 11`: gated on `player+0x814 & 1` (a player state byte — our
    CCombatAttack notes call 0x814/0x815 "special-punch/forced-kick" bits; our enum
    calls fire-type 11 TSFT_FLAG. **VERIFY** semantics before wiring), then
    `status+0x24 = 0` → `KnifeFire()`.
  - (==2 grenade → GrenadeFire; ==10 katana → **KatanaFire**, a separate method;
    no item → CCombatAttack.)
- **CTSGun::SecondaryAttack @0x717a8**:
  - `iFireType == 3`: `status+0x24 = 1` then `KnifeFire()` → THROW.
  - (==2 → status+0x24=1, GrenadeFire; ==10 → **KatanaThrow** — the katana never goes
    through KnifeFire; ==11 → other path; no item → CCombatAttack.)
- Primary/SecondaryAttack are virtual, run from CBasePlayerWeapon::ItemPostFrame under
  IN_ATTACK/IN_ATTACK2 + the m_flNext*Attack gates — since KnifeFire re-arms BOTH
  timers with flFireRate, holding either button auto-repeats at the fire rate (no
  press-only gate anywhere).
- melee_refs.txt context: `CCombatBlock` @0x75ebc arms the block windows KnifeFire's
  VictimKnifeHold reads (`iFireType 10 → player+0x7b0 = 0.5`, `iFireType 3 →
  player+0x7ac = 0.5`, bare hands → `player+0x7a8 = 0.5`, gated on block bar
  `player+0x774 >= 0.01` and four attack timers incl. this+0xa4);
  VictimKatanaHold/VictimKungfuHold are the sibling gates for the other melee fires.

## 4. OURS

The logic exists, split across two INVENTED-name methods plus divergent support state:

- **`dlls/ts_gun.cpp:909 CTSGun::MeleeAttack(const s_weapon_info*)`** (decl ts_gun.h:92)
  = the slash, but merged with the katana (original: separate KatanaFire). Diffs:
  - Trace: `UTIL_TraceLine` point trace, range 48/64 (ts_gun.cpp:916-920) vs original
    **CheckHull hull-3 sweep, range 35** (CheckHull absent from our tree).
  - Damage: `wi->flDamage * TSDamageMult()` + `DMG_CLUB|DMG_NEVERGIB` (ts_gun.cpp:969-970)
    vs original `flDamage × [stunt 2/3] × GetMeleeMultiplier()` + **DMG_SLASH**|DMG_NEVERGIB
    (0x1004 ≠ our 0x1080 — wrong damage bits).
  - No stunting (iuser4 & 0x70C30) rate/damage modifiers at all.
  - Timers: only m_flNextPrimaryAttack (+rate) and idle +5.0 (ts_gun.cpp:942-943) vs
    original primary AND secondary +rate, idle +3.0, plus player+0x4f8 = rate.
  - No `VictimKnifeHold` block gate (function absent; our block system is CCombatBlock +
    `m_blockBar` — which lives on **CTSGun** (ts_gun.h:148) while the original bar is
    **player+0x774**; windows player+0x7ac/0x7b0 absent, only `m_flKungFuBlock` =
    player+0x7a8 exists, player.h:418).
  - No whiff window (player+0x79c/0x7a0/0x7a4 — noted as unported SLICE at
    ts_gun.cpp:994 and ts_stunt.cpp:455).
  - No bloody-knife state (status bloody flag / `pev->body = 1`).
  - Sounds: whoosh every swing CHAN_WEAPON + hit sound CHAN_ITEM from the ATTACKER
    (ts_gun.cpp:939,956) vs original: NO server whoosh on slash, `knife_hitbody.wav`
    CHAN_BODY from the **VICTIM** edict only on a player hit (surface "knife_hit.wav"
    belongs to VictimKnifeHold's block, not the slash).
  - Event: `m_usMelee`(knife.sc) played only on hard-surface hit, flags 0, coords in
    origin/angles (ts_gun.cpp:963-966) vs original: knife.sc EVERY slash, FEV_NOTHOST,
    `iparam1=weaponType, iparam2=bloody` — **wire-format mismatch**.
  - Viewmodel: predicted `SendWeaponAnim(slashBase+m_slashStep%3)` (ts_gun.cpp:929,
    invented `m_slashStep` ts_gun.h:146) vs original: no server SendWeaponAnim on slash
    (the client event drives the viewmodel); 3rd-person Animate(0x1D+RANDOM(0,2)) is
    approximated by `m_TSAnim.Animate(TSA_FIRE)`.
- **`dlls/ts_gun.cpp:1518 CTSGun::ThrowKnife()`** (decl ts_gun.h:97) = the throw branch
  (also handles katana → original uses KatanaThrow). Diffs:
  - Invented one-per-press gate `m_afButtonPressed & IN_ATTACK2` + empty-clip
    PlayEmptySound branch (ts_gun.cpp:1521-1528); original has neither (auto-drops at 0).
  - Spawn: `GetGunPosition() + fwd*16` vs original wall-clipped
    `TraceLine(eye+right*1 → eye+fwd*32+right*1).vecEndPos`.
  - Velocity 1900 matches (ts_gun.cpp:1533). Our CreateKnife signature adds a damage
    param (ts_knife.cpp:291) — original passes `(player, pos, vel, info+0x20 type)`.
  - Projectile orientation (angles=VecToAngles(dir), angles.z=90, avelocity.x=−1300) is
    done in KnifeFire in the original — VERIFY where our CTSKnife sets it (if at all).
  - Timers: fixed +0.5/+0.5, idle +5.0 (ts_gun.cpp:1541-1543) vs +flFireRate both, idle
    3.0 → 2.0 on redraw.
  - Consume: unconditional `m_iClip--` (ts_gun.cpp:1529) vs original
    `if (!InfiniteClips)` + **player+0x548 += slotCost + SetPlayerSpeed(GetSpeedBySlots)**
    (slot/speed recompute entirely missing in ours).
  - Out of knives: `m_items[].owned=false; GetBetterWeapon()` (ts_gun.cpp:1546-1550) vs
    original `DropWeapon(m_weaponType, true)` — we HAVE `DropWeapon(int,bool)`
    (ts_gun.h:113) but don't call it here.
  - Redraw: hardcoded `SendWeaponAnim(7/5)` vs original model-derived draw anim +
    **draw.sc playback (weaponType, attachEquipped)** — missing.
  - No throw Animate(0x20) gate, no stunting modifiers, no player+0x4f8.
- Dispatch: ts_gun.cpp:608-610 (PrimaryAttack → MeleeAttack for TSFT_KNIFE|TSFT_KATANA)
  and ts_gun.cpp:1875-1878 (ItemPostFrame right-click → ThrowKnife) — the original
  dispatch lives in Primary/SecondaryAttack with the status+0x24 mode byte, and
  fire-type 11 (our TSFT_FLAG, ts_weapon_info.h:198) can ALSO reach KnifeFire.
- Support fns present: `GetMeleeMultiplier` (player.h:479, unused here),
  `GetSpeedBySlots` (ts_weapons.h:58), `InfiniteClips` (ts_player.h:17),
  `CTSKnife::CreateKnife` (ts_knife.cpp:291). Absent: `VictimKnifeHold`,
  `VictimKatanaHold`, `UpdateMeleeBar`, `CBasePlayer::CheckHull`, the whiff-window
  fields, the player-side block bar.

## 5. RECONCILE

Rename/move (absorb the invented names):
1. Split `MeleeAttack` + `ThrowKnife` into the original method set:
   **`CTSGun::KnifeFire()`** (slash+throw on the status mode byte), and move the katana
   halves into **`CTSGun::KatanaFire()` / `CTSGun::KatanaThrow()`** (RE those two
   separately — this analysis covers only KnifeFire). Delete `ThrowKnife`;
   `m_slashStep` goes away (original: RANDOM_LONG(0,2) body anim; the viewmodel is
   driven by the knife.sc client event, not SendWeaponAnim).
2. Add the per-status alt-mode byte (status+0x24) to our TSItem/active-status and set it
   in Primary(=0)/Secondary(=1)Attack for fire-type 3, exactly like the original
   dispatcher; route fire-type 11 → KnifeFire behind the player+0x814&1 gate
   (**VERIFY** what sets 0x814&1 before enabling — flag-carrier melee?).
3. Move the block bar to CBasePlayer (orig 0x774) and add the three block windows
   (0x7a8 bare / 0x7ac knife / 0x7b0 katana — 0x7a8 already exists as m_flKungFuBlock);
   implement `VictimKnifeHold`/`UpdateMeleeBar` (TS_gamerules.cpp globals — decompiles
   already in melee_refs.txt) and gate the slash damage on it.
4. Add the whiff-window fields (player 0x79c float, 0x7a0 float, 0x7a4 byte) + writer
   here and consumer in the PreThink lunge system (shared with CCombatAttack — see the
   ts_gun.cpp:994 SLICE note).

Write fresh / fix in place (KnifeFire body):
5. Slash: CheckHull hull-3 sweep at range **35** (implement CBasePlayer::CheckHull),
   stunting modifiers (×1.5 rate / ×2/3 dmg on iuser4&0x70C30), damage
   `flDamage × GetMeleeMultiplier()`, **DMG_SLASH**|DMG_NEVERGIB, victim-edict CHAN_BODY
   knife_hitbody.wav (pitch 95+rand31) only on player hit, bloody state (status flag +
   `pev->body = 1`), both attack timers + idle 3.0; drop our server-side whoosh/
   hit-sound pair on slash.
6. Throw: wall-clipped spawn trace (eye+right*1 → +fwd*32), keep 1900, set
   angles=VecToAngles(dir), angles.z=90, avelocity.x=−1300 **in KnifeFire** (align
   CreateKnife's signature to `(player, pos, vel, type)` — damage belongs inside
   CTSKnife); consume gated on `!InfiniteClips` **with the slot/speed recompute**
   (player slots += slotCost; SetPlayerSpeed(GetSpeedBySlots())); `DropWeapon(type,
   true)` at 0 knives (not owned=false+GetBetterWeapon); redraw via model-derived
   SendWeaponAnim + idle 2.0.

Client↔server protocol (byte-faithful — swap contract):
7. `events/knife.sc` must fire on EVERY slash with flags **FEV_NOTHOST (1)** and
   `iparam1 = m_weaponType, iparam2 = bloodyFlag` (no coords). Our current
   surface-hit-only, coord-carrying knife.sc playback is wire-incompatible with the
   original client — rework both sides together (**VERIFY** the original client.dll
   knife.sc handler via dc.sh before changing: the whoosh/spark/viewmodel-anim split
   lives there).
8. The throw's redraw must fire `events/draw.sc` (this+0x976 = the SAME m_usDraw handle
   Deploy uses) with `iparam1 = weaponType, iparam2 = attachEquipped`.
9. Sounds are part of the audible contract: slash emits NO server whoosh;
   `knife_miss.wav` is the THROW sound (CHAN_BODY, vol 1.0, attn 0.8, pitch 95+rand31).

Explicit VERIFY list (do not guess):
- player+0x814 & 1 (fire-type-11 gate) semantics — live-trace with a flag carried.
- The trailing 0.5 float in TS's 8-arg EMIT_SOUND_DYN.
- status+0x28 pointer struct (+0x28 short = draw anim) — which record it is.
- player+0x12c writes (0.75 attacker / 0.35 victim) in VictimKnifeHold — consumed by
  CCombatBlock's gate; confirm whether it is an absolute time or a countdown.
- player+0x4f8 consumer.
- Whether our CTSKnife already applies the 90° roll / −1300 spin internally (avoid doubling).
- info+0x20 knife-type values (the ==6 dead branch) — knife vs seal-knife id.
