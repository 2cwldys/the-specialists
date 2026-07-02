# CBasePlayer::CloseCombatAttack — RE analysis (ts_i386.so @0x85f88, size 0x57e)

## 1. SIGNATURE

Mangled: `CloseCombatAttack__11CBasePlayerG6VectorN21ifibN37` (file addr 0x85f88; Ghidra 0x95f88).
gnu-v2 decode: `G6Vector` = Vector by value (gcc2 passes ctor'd classes by invisible reference — the
call sites push ADDRESSES, so at ABI level these read as `Vector&`), `N21` = 2 repeats of arg 1
(→ 3 Vectors), then `i f i b`, `N37` = 3 repeats of arg 7 (→ 4 bools).

```cpp
CBaseEntity* CBasePlayer::CloseCombatAttack(
    Vector vecSrc,        // param_2 — trace start (gun position)
    Vector vecEnd,        // param_3 — trace end (src + forward * range)
    Vector vecDir,        // param_4 — aim dir, forwarded to TraceAttack
    int    iHull,         // param_5 — hull number for CheckHull (callers pass 3 = head_hull, or 1)
    float  flDamage,      // param_6
    int    iAnim,         // param_7 — CTSAnimation activity code, or -1 = no animate
    bool   bDisarm,       // param_8 — try to knock the victim's weapon out (DropWeapon)
    bool   bPlayersOnly,  // param_9 — if true, a non-player hit returns NULL (no damage at all)
    bool   bKnockback,    // param_10 — shove the victim (only when flDamage > 30)
    bool   bForceTeamSafe // param_11 — apply the teammate gate even when friendlyfire is ON
);
// returns the entity that was hit and damaged, or NULL (miss / invalid / teammate / blocked)
```

The signature is NOT in `signatures-gcc2.txt` (the `N`-repeat mangling is a demangler edge case);
decode above is from the mangled name + the Ghidra decompile + both call sites' push sequences.
Sibling with the IDENTICAL signature: `ColdCockAttack__11CBasePlayerG6VectorN21ifibN37` @0x86610
(the pistol-whip variant, kevlarhit sound — separate function, same skeleton).

## 2. BEHAVIOR

Step-by-step (decompile `melee_closecombat.c`, disasm verified at 0x85f88–0x86506):

1. **3rd-person anim** — if `iAnim != -1`: `CTSAnimation::Animate(&this->m_TSAnim /* +0x28c */, this, iAnim)`.
   (+0x28c is confirmed our `m_TSAnim`.)
2. **Motion-blur roll** — `if (RANDOM_LONG(10, 80) < flDamage)`: send user message **gmsgTSMotionBlur
   ("TSMBlur", 2 bytes)** as `MESSAGE_BEGIN(MSG_PVS=4, gmsgTSMotionBlur, this->pev->origin)`;
   `WRITE_BYTE(ENTINDEX(attacker))`; `WRITE_BYTE(flDamage > 50.0f ? 2 : 1)`. So any melee hit path
   rolls a chance (damage-scaled: dmg 35 => ~35%, dmg 55 => ~64%, >80 => always) of a level-1 blur,
   level-2 when damage > 50. Sent at the ATTACKER's PVS/origin, carrying the attacker's entindex.
3. **Trace** — `CheckHull(vecSrc, vecEnd, tr, iHull)` (@0x85e28): `UTIL_TraceLine(src, end,
   dont_ignore_monsters, edict, &tr)`; on a miss, `UTIL_TraceHull(src, end, dont_ignore_monsters,
   iHull, edict, &tr)`; on a hull hit it calls `FindHullIntersectionplayer(...)` (the classic
   crowbar find-intersection pattern). If `tr.flFraction >= 1.0` → **return NULL** (miss).
4. **Resolve entity** — `tr.pHit`; if NULL substitute `PEntityOfEntOffset(0)` (worldspawn); entity =
   `pHit->pvPrivateData` (+0x7c). NULL → return NULL.
5. **Validity gates** (any => return NULL):
   - `pev->takedamage == DAMAGE_NO (0.0f)`
   - `pev->flags & 0x2000000` = **FL_WORLDBRUSH**
   - `(char)pev->effects < 0` = **EF_NODRAW** (0x80)
6. **Player-vs-nonplayer split** (`victim->IsPlayer()` virtual):
   - non-player && `bPlayersOnly` → return NULL (nothing happens, not even damage).
   - player victim: **teammate gate** — `if ((bForceTeamSafe || friendlyfire.value == 0.0f) &&
     g_pGameRules->PlayerRelationship(attacker /*+ victim*/) == GR_TEAMMATE /*1*/)` → return NULL.
     (vtable dispatch at gamerules vtbl+0xcc — confirmed `PlayerRelationship` in VTABLES.md.)
7. **Player-victim extras** (skipped entirely for non-players):
   - **Block check**: `VictimKungfuHold(attacker, victim, flDamage, &tr)` (@0x66f50). Nonzero =>
     the hit was DEFLECTED → return NULL (no damage; the Hold function itself plays
     "player/kungfuhit.wav" CHAN_BODY pitch 100, clears the lunge (+0x7a0=0 both), sets attacker
     `m_flNextAttack` (+0x12c) = **0.5**, blocker = **0.25** (countdown semantics — m_flNextAttack
     is decremented in ItemPostFrame), and `UpdateMeleeBar(attacker, 33.0)`. Block window = victim
     +0x7a8 > 0; a COUNTER branch also triggers when the ATTACKER's +0x7a8 > 0 while the victim is
     mid-lunge (+0x79c > 0, +0x7a4 == 0) — roles swap. Requires the blocker's bar +0x774 >= 0.1.)
   - **Disarm**: `if (bDisarm && (float)victim->field_0x71c < gpGlobals->time)` →
     `CBasePlayer::DropWeapon(victim, (tsweapon_t)0xfffffffe /* -2 = active-weapon sentinel */,
     flDamage)`. +0x71c is a per-victim disarm-immunity/re-disarm timer. **VERIFY**: whether
     DropWeapon(-2, dmg) rolls a damage-scaled chance internally and what re-arms +0x71c.
   - **Knockback**: `if (bKnockback && flDamage > 30.0f)`: `speed = |attacker.velocity|`;
     `dir = normalize(victim.origin − attacker.origin)` (degenerate => (0,0,1));
     `victim.velocity += dir * speed`; then **`victim.velocity.z = 235.0`** (hard-set, disasm
     0x863a0 = 0x436b0000). So every >30-dmg knockback hit pops the victim up 235 u/s.
   - **Hit sound**: `EMIT_SOUND_DYN(victim->edict(), CHAN_BODY /*4*/, "player/kungfuhit.wav",
     1.0, 0.8 /*attn*/, 0, 95 + RANDOM_LONG(0, 31) /*pitch 95..126*/, 0.5 /*trailing TS float arg*/)`.
     NOTE: on the **VICTIM's** entity/body channel, not the attacker's weapon channel.
   - **PunchScreen** (dead code in the shipped .so): if attacker `m_pActiveItem` (+0xa10) != NULL
     && flDamage > 20 && activeItem->m_weaponType (+0xc8) == 0 (TSW_MELEE/kung-fu) →
     `PunchScreen(victim, flDamage * 0.5f, attacker.origin − victim.origin)`. PunchScreen @0x85f70
     is an EMPTY 0x18-byte stub — a disabled effect (our ts_player.cpp:487 already models this).
8. **Damage** (players AND non-players, e.g. breakables): set global byte **`glast_closecombat`
   (@0x104480) = 1**; `ClearMultiDamage()`; virtual `victim->TraceAttack(attacker->pev, flDamage,
   vecDir, &tr, 0x4001080)`; `ApplyMultiDamage(attacker->pev, attacker->pev)`;
   `glast_closecombat = 0`. Damage bits **0x4001080 = DMG_CLUB (0x80) | DMG_NEVERGIB (0x1000) |
   0x4000000 (TS-custom "close-combat" marker bit)**. `glast_closecombat` is READ by
   `TSGetPointsForFrag`, `CHalfLifeMultiplay::PlayerKilled` and `CBasePlayer::Killed` (melee-kill
   scoring / death-notice flavor — siblings `glast_coldcock` @0x104481, `glast_headshot` @0x104482);
   it is also set by `CTSStunt::CheckForBreakables`.
9. **Return** the damaged entity (or NULL through the gate label).

Numeric constants: RANDOM_LONG(10,80) blur roll; 50 = blur level-2 threshold; 30 = knockback
threshold; 235.0 = vertical pop; 20 = PunchScreen threshold; 0.5 = PunchScreen damage scale;
sound = vol 1.0 / attn 0.8 / pitch 95+rand(0,31); dmg bits 0x4001080; DropWeapon type -2.

Member map (player unless noted): +0x12c `m_flNextAttack` (countdown) · +0x28c `m_TSAnim` ·
+0x4f8 last-refire store (caller) · +0x71c disarm-immunity time (**VERIFY name**) · +0x774 melee
block bar (0..1 scale — gates at 0.1/0.01; **VERIFY scale vs UpdateMeleeBar(33.0)**) · +0x79c
whiff-lunge window · +0x7a0 lunge magnitude · +0x7a4 lunge flag byte · +0x7a8 kung-fu block window
· +0x7ac knife block window · +0x7b0 katana block window · +0xa10 `m_pActiveItem` · gun +0xc8
`m_weaponType` · gun +0x96c event handle (see CALLERS).

## 3. CALLERS

Exactly **two** call sites (objdump, `call 58c20 <CloseCombatAttack@plt>`):

1. **`CTSGun::CCombatAttack` @0x76078, call @0x76cf6** — the kung-fu punch/kick executor (our
   `CTSGun::CCombatAttack`). Args from the push sequence @0x76cb4–0x76cf6:
   `this = gun->m_pPlayer (gun+0x80)`; vecSrc/vecEnd (= src + v_forward*range) / vecDir (=
   gpGlobals->v_forward) from locals; **iHull = (attackClass == 3 ? 1 : 3)** (attack class 3 = the
   back-kick family → human hull 1, everything else head hull 3); flDamage + iAnim from the
   per-move locals; **bDisarm=1, bPlayersOnly=0, bKnockback=1, bForceTeamSafe=0**. After return
   (fragment FUN_00086c8c = 0x76c8c):
   - MISS (ret NULL) && lungeMag > threshold → arm the whiff-lunge: player+0x79c = window,
     player+0x7a0 = ±magnitude (**negated when attackClass==3** — back-kick lunges backward),
     player+0x7a4 = 0. (The PreThink lunge subsystem consumes these.)
   - a per-move bool && `!(attacker pev->iuser4 & 1)` → `BlockPlayer(m_pPlayer, moveStun * 0.75f)`
     — called on the ATTACKER (a post-swing self-lock; moveStun = the 0.1–0.35 per-move value).
     **VERIFY the exact semantic of BlockPlayer here** (our old comment guessed victim-stun).
   - `PLAYBACK_EVENT(1, m_pPlayer->edict(), gun+0x96c event, ...)` — the close-combat
     event (**VERIFY which .sc**; gun+0x96c is also fired by CCombatBlock with iparam m_weaponType).
   - gun m_flNextPrimary/SecondaryAttack (+0x9c/+0xa0) = base + refire; idle (+0xa8) = base + const;
     player+0x7a8 = 0 (attacking closes your own block window); player+0x4f8 = refire.
2. **`CTSStunt::CloseAttack` @0x8c630, call @0x8c8b9** — the jump-kick (our `CTSStunt::CloseAttack`,
   ts_stunt.cpp:658). Args (fragment FUN_0009c70c): iHull=3, flDamage = speed-scaled clamp
   (x2 under the KungFu powerup — `KungFu__FP11CBasePlayer`), **iAnim=-1** (it animates only on a
   hit), **bDisarm=1, bPlayersOnly=0, bKnockback=1, bForceTeamSafe=1**. On a hit: clears its own
   +0x7a8, `Animate(random 0xa/0xd/0xe)`, tops up m_flNextAttack (+0x12c) if expired, plays
   kungfuhit (pitch 95..126) itself, `UpdateMeleeBar(dmg * const)`.

NOT callers: `KnifeFire`/`KatanaFire` do their own `CheckHull` + `Victim{Knife,Katana}Hold` +
inline damage (no CloseCombatAttack); `ColdCock` @0x71dd0 calls the sibling `ColdCockAttack`
@0x86610 (call @0x72042). `GoWheel` @0x8c9bc only shares UpdateMeleeBar + the lunge writes.
Dispatch context: CCombatAttack is reached from PrimaryAttack/ItemPostFrame when bare-handed
(kung-fu); CloseAttack from the stunt/Jump path.

## 4. OURS

**The shared method does not exist** — its body is INLINED (differently, three times):

- `dlls/ts_gun.cpp:1144–1221` — `CTSGun::CCombatAttack` hit resolution. Plain `UTIL_TraceLine`
  (no CheckHull hull fallback, no FindHullIntersection), flesh test = `takedamage != DAMAGE_NO`
  (no FL_WORLDBRUSH / EF_NODRAW gates), block check inlined against our invented
  `m_flKungFuBlock` (gpGlobals->time-based) instead of `VictimKungfuHold` (+0x7a8 window +
  bar>=0.1 + the counter branch + m_flNextAttack 0.5/0.25 countdown + UpdateMeleeBar(33) drain on
  the blocked ATTACKER), kungfuhit emitted on the ATTACKER CHAN_WEAPON with `TSSlowPitch` (orig:
  VICTIM CHAN_BODY, attn 0.8, pitch 95+rand(0,31)), damage bits `DMG_CLUB|DMG_NEVERGIB` (missing
  the 0x4000000 marker), **no TSMBlur roll, no disarm (DropWeapon -2), no knockback/235-pop, no
  teammate/friendlyfire PlayerRelationship gate, no glast_closecombat**. Our invented
  miss/surface sounds ("player/closecombat.wav", pl_step*, TE_SPRITE smoke) live here too —
  the original CloseCombatAttack plays NOTHING on a miss or a non-player hit (**VERIFY** whether
  those sounds exist elsewhere in the original, e.g. in the .sc event).
- `dlls/ts_stunt.cpp:658–715` — `CTSStunt::CloseAttack` inlines its own copy (comment at :657
  admits it). This one DOES have the line→hull fallback and the knockback + 235 pop (close), but:
  knockback requires `IsAlive && (IsPlayer||MyMonsterPointer)` (invented), no disarm, no
  VictimKungfuHold, no TSMBlur, no teammate gate, no glast_closecombat, kungfuhit missing
  (plays "player/closecombat.wav" instead), bits missing 0x4000000.
- `dlls/ts_gun.cpp:1298–1322` — `CTSGun::ColdCock` inlines a mini version (that one should map to
  the sibling `ColdCockAttack`, out of scope here but same reconcile shape).
- `dlls/ts_gun.cpp:909–973` — `CTSGun::MeleeAttack` (knife/katana) is correctly SEPARATE from
  CloseCombatAttack (matches the original KnifeFire/KatanaFire structure), but uses TraceLine
  instead of CheckHull and lacks the Victim{Knife,Katana}Hold deflect path (the +0x7ac/+0x7b0
  windows and the sparks + knife_hit.wav deflect don't exist).
- Support pieces present: `m_TSAnim` (+0x28c) OK; `gmsgTSMotionBlur` "TSMBlur" registered
  (ts_player.cpp:21, ts_stunt.cpp:62) but never sent from melee (reg yes / send no);
  `m_flKungFuBlock` (player.h:418) ~= +0x7a8 but wrong mechanism (time-based, no bar coupling);
  block bar lives on the GUN (`m_blockBar` 0..100, ts_gun.h:148) instead of the PLAYER (+0x774)
  and `UpdateMeleeBar`/`BlockPlayer`/`CheckHull` don't exist as player methods (`PunchScreen`
  itself exists as the faithful no-op, ts_player.cpp:487 OK);
  `CBasePlayer::DropWeapon()` exists (ts_gun.cpp:2355) but with the wrong signature — the original
  is `DropWeapon(tsweapon_t, float)` and is what the disarm calls with -2.

**Verdict: exists-partial** (fragments under invented structure; the shared function itself missing).

## 5. RECONCILE

1. **Create `CBaseEntity* CBasePlayer::CloseCombatAttack(Vector, Vector, Vector, int, float, int,
   bool, bool, bool, bool)`** in the player melee file (original file per funcfile: TS_player.cpp
   family — VERIFY via filemap.py), implementing section 2 verbatim: Animate → TSMBlur roll
   (RANDOM_LONG(10,80) < dmg; level = dmg>50 ? 2:1; MSG_PVS at attacker origin, byte entindex,
   byte level) → CheckHull → validity gates (takedamage / FL_WORLDBRUSH / EF_NODRAW) →
   bPlayersOnly → teammate gate ((bForceTeamSafe || friendlyfire==0) && PlayerRelationship==
   GR_TEAMMATE) → VictimKungfuHold → disarm (DropWeapon(victim, (tsweapon_t)-2, dmg) gated on
   victim +0x71c timer) → knockback (dmg>30: +=dir*|attackerVel|, z=235) → kungfuhit on VICTIM
   CHAN_BODY attn 0.8 pitch 95+rand(0,31) → (PunchScreen call kept, it's a no-op) →
   glast_closecombat=1; ClearMultiDamage; TraceAttack(dmg, dir, DMG_CLUB|DMG_NEVERGIB|0x4000000);
   ApplyMultiDamage; glast_closecombat=0 → return victim.
2. **Add `CBasePlayer::CheckHull(Vector, Vector, TraceResult&, int)`** (line → hull(iHull) →
   FindHullIntersection) and use it here + in MeleeAttack + ColdCock (replaces our bare TraceLine).
3. **Re-wire `CTSGun::CCombatAttack`**: delete its inlined trace/block/damage block (ts_gun.cpp:
   1144–1221) and call `m_pPlayer->CloseCombatAttack(src, end, v_forward, class==3?1:3, dmg,
   bodyAct, true, false, true, false)`; keep/move the post-call pieces to match 0x76cfb+: whiff
   lunge on miss (+0x79c/±0x7a0/0x7a4 — negate for the back-kick class), BlockPlayer(attacker,
   stun*0.75) self-lock, the gun+0x96c PLAYBACK_EVENT, timers, player+0x7a8=0, +0x4f8=refire.
   This ABSORBS our invented in-CCombatAttack block-deflect, hit sounds, and TE_SPRITE smoke
   (remove or re-justify each against the original — the miss/surface sounds are INVENTED here).
4. **Re-wire `CTSStunt::CloseAttack`** to call the real method (hull 3, dmg, -1, true, false,
   true, true) and keep only its faithful post-hit tail (random 0xa/0xd/0xe Animate, m_flNextAttack
   top-up, kungfuhit, UpdateMeleeBar) — drops its inlined knockback/damage copy.
5. **Add the missing player-side melee state + methods**: `UpdateMeleeBar(float)` (+0x774 bar on
   the PLAYER + gmsgKFuPower — migrate `m_blockBar` off the gun), `BlockPlayer(float)`,
   `Victim{Kungfu,Knife,Katana}Hold` free functions (TS_gamerules.cpp per the plate), the lunge
   members (+0x79c/0x7a0/0x7a4/0x7a8/0x7ac/0x7b0), the disarm timer (+0x71c), and the
   `glast_closecombat/glast_coldcock/glast_headshot` globals (needed by TSGetPointsForFrag /
   PlayerKilled — scoring fidelity). Fix `CBasePlayer::DropWeapon` to the original
   `(tsweapon_t, float)` signature (-2 = active-weapon sentinel; **VERIFY** its internal
   damage-chance behavior before implementing the disarm roll).
6. **Protocol implications** (byte-faithful): TSMBlur (2 bytes: entindex, 1|2) — already
   registered with the right name/size; melee must now SEND it (client already renders on receive).
   gmsgKFuPower stays. The gun+0x96c close-combat event needs its .sc name recovered from
   CTSGun::Precache (**VERIFY** — likely a kungfu/closecombat event; our m_usMelee "events/knife.sc"
   is a different handle). Damage-bit 0x4000000 is server-internal (safe), but glast_closecombat
   affects DeathNotice/score messages — wire-visible, so implement it.
7. **VERIFY list** (do NOT guess): +0x71c name/reset; +0x774 scale (0..1 vs 0..100) and
   UpdateMeleeBar's exact math (33.0 arg vs 0.1 gate); BlockPlayer's body; DropWeapon(-2,dmg)
   internals; the gun+0x96c event .sc; whether any miss/surface sound exists in the original
   CCombatAttack path; ColdCockAttack (sibling) diffs; the exact per-move bool that gates the
   attacker BlockPlayer self-lock.
