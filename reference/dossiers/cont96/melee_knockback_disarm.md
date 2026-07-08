# cont96 — Melee knockback + disarm (RE dossier, read-only session)

Bug (interop, ORIGINAL TS client vs OUR ReHLDS server): kicks/punches should send the
victim FLYING BACK; on our server victims only fly on death or when already mid-air.
Second question: do punches DISARM in the original?

All addresses are nm/file addrs in `reference/ts_i386.so` (Ghidra = +0x10000).
Decompiles used this session (typed `re` project, re.sh): 0x85f88 CloseCombatAttack,
0x76078 CCombatAttack, 0x8c630 CTSStunt::CloseAttack, 0x8bd9c CheckForBreakables,
0x7fa94 CBasePlayer::TakeDamage, 0xa2430 CBaseMonster::TakeDamage, 0x808cc PreThink,
0x84900 CBasePlayer::DropWeapon — plus raw objdump on every load-bearing branch.

---

## 1. The ORIGINAL's knockback — THREE sites, one formula

The engine-side formula appears **three** times (the only 235.0f constants in the
binary: file offs 0x863a3, 0x8697b, 0x8c3fe):

1. `CBasePlayer::CloseCombatAttack` @0x85f88 (kung-fu direct-hit resolver)
2. `CBasePlayer::ColdCockAttack` @0x86610 (pistol-whip twin — its knockback branch is
   never exercised: the coldcock caller passes bKnockback=false)
3. `CTSStunt::CheckForBreakables` @0x8bd9c — **the RUN-THROUGH resolver** (see §2 —
   this is the missing subsystem and the primary cause of the bug)

### 1a. Direct-hit knockback (CloseCombatAttack @0x85f88, disasm 0x86251–0x863a7)

Byte-verified gate order inside the `IsPlayer(victim)` branch, AFTER
VictimKungfuHold returns false and AFTER the disarm DropWeapon call:

```
if (bKnockback)  ; cmpb $0, 0xe4(%esp) @0x86251
  && flDamage > 30.0f          ; flds param; fcomps fc_30 @0x8625f  (STRICT >)
{
  speed = |attacker.pev->velocity|          (3D length)
  dir   = victim.pev->origin - attacker.pev->origin
  dir   = |dir|>0 ? dir/|dir| : (0,0,1)
  victim.pev->velocity += dir * speed        ; rep movsl @0x86398
  victim.pev->velocity.z = 235.0f            ; hard-set, @0x863a3
}
EMIT_SOUND_DYN(victim, CHAN_BODY, "player/kungfuhit.wav", 1.0, 0.8, 0, 95+rand(0,31))
```

Answers to the cont.95 key questions:

- **(a) gate BEFORE hitgroup multipliers.** The >30 compare is on the raw `flDamage`
  parameter; `TraceAttack` (where the leg x0.5 / head bullet-only bonus lives,
  @0x801cc) runs AFTER the knockback, at the tail. A leg kick still knocks back.
- **(b) location:** inside CloseCombatAttack itself (and duplicated inline in
  CheckForBreakables). TakeDamage — BOTH CBasePlayer @0x7fa94 and CBaseMonster
  @0xa2430 — has NO velocity manipulation at all (re-verified this session; the
  player override is armor/kevlar reduction + TSTakeDamage wire + suit chatter only).
- **(c) velocity source:** magnitude = |ATTACKER velocity| at hit time (3D), direction
  = attacker→victim center-to-center, and z is then unconditionally overwritten with
  235.0. A stationary attacker ⇒ pure vertical pop (0,0,235); the horizontal "flying"
  scales entirely with how fast the attacker is moving into the victim.
- **(d) bKnockback:** a plain parameter; callers verified at the push level:
  `CTSGun::CCombatAttack` @0x76cb7-0x76cc9 pushes teamSafe=0, knockback=1,
  playersOnly=0, disarm=1; `CTSStunt::CloseAttack` @0x8c8b9 passes disarm=1,
  knockback=1, teamSafe=1, iAnim=-1, hull 3, reach 48. No flag is dropped/inverted
  in our direct-hit port.
- **(e) grounded living victims: YES.** There is NO FL_ONGROUND check, NO IsAlive
  check on the victim (only takedamage!=0, !FL_WORLDBRUSH, !EF_NODRAW), and NO
  spawn-protect gate on the knockback (spawn-protect gates only the DISARM:
  `victim+0x71c < gpGlobals->time` @0x8622c wraps ONLY the DropWeapon call). The
  original launches a grounded, living, even spawn-protected victim. z=235 > the
  pm_shared unstick threshold (`velocity[2] > 180` ⇒ onground=-1, pm_shared.cpp:1656
  in ours and same in the original client) so a grounded victim does become airborne.

### 1b. What each attack passes as flDamage (whether the >30 gate fires)

- Kung-fu punches: 35 (0x23) — passes. Kicks: 45 (0x2d) — passes. Back-kick: 55
  (0x37) — passes. (CCombatAttack; flat, live-verified in cont.93. `movl
  $0x23/$0x2d/$0x37` @0x76307/0x764c0/0x765bb…; the same ints also feed
  `fildl`→×v_forward = the trace reach @0x76c10.)
- Stunt (mid-stunt light kick): 35 — passes.
- **Jump-kick (CTSStunt::CloseAttack @0x8c630): dmg = clamp(speed × 1/15, 10, 20)**
  (double @0x109b10 = 0.0666…, cap/floats via dc_10/fc_20 — read from .rodata this
  session), ×2 under the KungFu powerup. **≤20 normally ⇒ the jump-kick NEVER passes
  the >30 gate without the KungFu powerup (2×20=40 does).** Our ts_stunt.cpp:758-762
  already matches this exactly. So "jump into someone" flying-back in the original is
  NOT this gate — it's §2.

---

## 2. THE MISSING SUBSYSTEM — the whiff/run-through attack (CheckForBreakables @0x8bd9c)

This is the structural gap that IS the bug. The three player fields
`+0x79c/+0x7a0/+0x7a4` (our `m_flWhiffLungeTime/m_flWhiffLungeForce/m_iWhiffLungeType`)
are **not an attacker-velocity "lunge" at all** — nothing in the binary ever applies
them to the attacker's velocity (verified by enumerating every instruction touching
0x79c/0x7a0/0x7a4; writers: CCombatAttack miss-branch, KnifeFire, KatanaFire, GoWheel;
clearers: TSInit, HandleStunt, HolsterWeapon, PreThink, Victim*Hold; **the only
consumer is CheckForBreakables**, plus VictimKungfuHold's counter-check). They arm a
**run-through melee window**: while it is live, the attacker's own moving hull is a
weapon — CheckForBreakables (called every PreThink tick, ours player.cpp:2134, same
spot as the original @PreThink) sweeps ahead and delivers the FULL close-combat hit
package (disarm + >30 knockback + sound + TraceAttack) to any player it reaches.

The classic TS "kick sends them flying": you kick while sprinting; the instant 45u
trace usually whiffs; the window arms; your 250–400 u/s body carries into the victim;
the run-through fires with dmg 45 ⇒ knockback = your full sprint speed + z 235.
Our server implements NONE of the player branch (ts_stunt.cpp:837 "deferred") ⇒ no
flying, no run-through disarm. Mid-air/death flights still visible on our server come
from the (working) direct-hit path on airborne victims and the separate death launch
(DamageForce @0xa2d08) — matching the reported symptom.

### 2a. Decompile-exact control flow @0x8bd9c (typed decompile, this session)

```
void CTSStunt::CheckForBreakables()   // player = this->field_0x40 (m_pPlayer)
{
  if (m_flNextBreakCheck(-1 sentinel) == -1) return;
  if (gpGlobals->time < m_flNextBreakCheck) return;
  m_flNextBreakCheck = gpGlobals->time;                 // once per server frame

  // grounded with NO window ⇒ reset the once-per-victim memory and bail.
  if ((pev->flags & FL_ONGROUND) && player->0x79c == 0) // NB: grounded WITH a live
  { player->0x798 (m_hRunThroughLast) = 0; return; }    //     window keeps going!

  speed = |pev->velocity|;
  if (speed < 64 && player->0x79c == 0) return;         // fc_64, not 100

  // the sweep
  if (player->0x79c == 0) {                             // pure run/dive sweep
      dir = velocity/speed (fallback (0,0,1)); end = origin + 64*dir;
  } else {                                              // window-active sweep
      UTIL_MakeVectors(pev->v_angle);
      reach = (player->0x7a0 < 0) ? -32.0f : 32.0f;     // -32.0 = dbl @0x109af4
      end = origin + v_forward * reach;                 // back-kick sweeps BEHIND
  }
  UTIL_TraceHull(origin, end, dont_ignore_monsters, 3 /*head_hull*/, edict, &tr);
  if (tr.flFraction >= 1) return;
  victim = Instance(tr.pHit ? tr.pHit : ENT(0)); if (!victim) return;

  if (victim->IsPlayer() && player->0x79c == 0) return; // players need the window
  if (victim->takedamage == 0) return;
  if (victim->flags & FL_WORLDBRUSH) return;
  if (victim->effects & EF_NODRAW) return;

  // base damage (the breakable/run case)
  dmg = attacker->IsAlive() ? 0.1f * speed : speed;     // vtbl+0xb4 = IsAlive;
                                                        // a DEAD body does FULL speed
  if (player->0x798 == victim) return;                  // one hit per victim/window

  bits = DMG_CLUB|DMG_NEVERGIB (0x1080); bApply = true;
  if (player->0x79c != 0) {                             // ---- window-active package
      if (player->0x7a4 == 0) { glast_closecombat = 1; bits = 0x4001080; }
      dmg = fabs(player->0x7a0);                        // window dmg OVERRIDES speed
      player->0x7a0 *= 0.5f;                            // halves per landed hit
      if (fabs(player->0x7a0) < 5) player->0x7a0 = 0;   //   (…45→22.5→11→5.6→0)
      if (victim->IsPlayer() && dmg > 0) {
          if (friendlyfire==0 && teammate) bApply = false;
          else switch (player->0x7a4) {
            case 0:  // KUNG-FU type — the only branch with disarm+knockback
              if (!VictimKungfuHold(attacker, victim, dmg, &tr)) {
                  if (victim->0x71c (m_flSpawnProtectTime) < time)
                      victim->DropWeapon(-2, dmg);      // disarm pressure (§3)
                  if (dmg > 30) {                       // same formula as §1a
                      victim->velocity += |attacker vel| * unit(victim.org−attacker.org);
                      victim->velocity.z = 235.0f;      // @0x8c3fe
                  }
                  EMIT_SOUND_DYN(victim, CHAN_BODY, "player/kungfuhit.wav",
                                 1.0, 0.8, 0, 95+rand(0,31));
              } else bApply = false;
              break;
            case 2:  // KATANA — hold check + sound ONLY (no disarm, no knockback)
              if (!VictimKatanaHold(att, vic, dmg))
                  EMIT_SOUND_DYN(victim, CHAN_BODY, "weapons/katana/hit-body.wav", …);
              else bApply = false; break;
            default: // KNIFE (1)
              if (!VictimKnifeHold(att, vic, dmg))
                  EMIT_SOUND_DYN(victim, CHAN_BODY, "weapons/knife/knife_hitbody.wav", …);
              else bApply = false; break;
          }
          // PunchScreen(dmg*0.5, attacker.org−victim.org) if m_pActiveItem &&
          // dmg>20 && activeItem->m_weaponType==0  (dead — PunchScreen is a stub)
      }
  }
  if (bApply) {
      ClearMultiDamage();
      victim->TraceAttack(attacker->pev, dmg, g_vecZero /*dir!*/, &tr, bits);
      ApplyMultiDamage(attacker->pev, attacker->pev);
  }
  glast_closecombat = 0;
  player->0x798 (m_hRunThroughLast) = victim;           // remember until reset
}
```

### 2b. Who ARMS the window (all writers, disasm-verified)

| Writer | 0x79c (window) | 0x7a0 (dmg, signed) | 0x7a4 (type) |
|---|---|---|---|
| CCombatAttack miss-branch @0x76d02-0x76d50 (only if resolver returned NULL **and** scaled-value > 0.001) | the ×GetMeleeMultiplier, ×2-KungFu **scaled per-move value** (10–30; back-kick up to 80) | ±**flat 35/45/55** (negative for the back-kick, `xor 0x80000000` @0x76d30) // VERIFY §5a | 0 |
| KnifeFire (miss) @0x74426 | computed (ours: 0.25×fireRate, ts_gun.cpp:1369) | dmg | 1 |
| KatanaFire (back/duck slash) @0x73a77 | **0.25** (movl $0x3e800000) | dmg (fstps) | 2 |
| GoWheel (wheel-kick) @0x8ce34 | **0.3** (movl $0x3e99999a) | ±22.5 (×2 KungFu — `fadd %st(0)` @0x8ce2f) | 0 |

Clearers: TSInit @0x82db7 (triple), HandleStunt @0x8b04b (triple, near head after a
gate — clears when a stunt engages), HolsterWeapon @0x75740-0x75776 (triple, only when
window live AND type != 0 — ours ts_gun.cpp:646-650 matches), Victim*Holds zero 0x7a0
(ours TS_gamerules.cpp:295, 334-335 match), PreThink (§2c).

### 2c. PreThink lifecycle (@0x808cc, typed decompile lines 79-84 + 192-198)

```
0x79c -= GiveSlowMul()*frametime (×1.5 dblfire); FLOOR 0.001 (not 0!)
…
if (0x79c != 0) {
    if (0x79c < 0.01 || 0x7a0 == 0) { 0x79c = 0; 0x7a0 = 0; 0x7a4 = 0; }
    0x7a8 (m_flKungFuBlock) = 0;      // EVERY frame the window is live —
}                                     // you cannot hold a kungfu block mid-window
```
Decay ≈ 1 unit/s ⇒ a kung-fu window value of 10–30 effectively lasts until CONSUMED
(0x7a0 halves to <5 ⇒ 0 ⇒ the `0x7a0==0` arm kills it) or cleared by
holster/stunt/hold/TSInit — not by the timer. Katana/wheel (0.25/0.3) are true
sub-second windows. NOTE the asymmetric semantics per weapon are the original's own.

---

## 3. DISARM — the truth (@0x84900 CBasePlayer::DropWeapon, -2 branch)

**Yes: kicks and punches disarm in the original — one solid kick usually suffices.**

Decompile-exact (typed, this session; disasm cross-checked):

```
DropWeapon(-2, flDamage):
  gun = m_pActiveItem; if (!gun || gun->m_iId != 0x10) return;
  if (gun->m_weaponType == 0x1d || gun->m_weaponType < 1) return;  // bare hands /
                                                                   // type 29 undisarmable
  m_flDisarmPressure (0x7b4) += flDamage;                          // ACCUMULATE
  thr = 30;                                    // weaponType 0x22 (34)
  if (weaponType != 0x22) {
      thr = 25;                                // GetWeaponType()==3 (melee class)
      if (class != 3)
          thr = slotCost < 21 ? 20 : min(slotCost, 50);   // clamp(slotCost,20,50)
  }
  if (m_flDisarmPressure < thr) return;        // below threshold: keep the gun
  m_flDisarmPressure = 0; fThrow=false;        // DISARM: fall through into the
  …                                            // normal active-weapon drop path
```

- **Accumulators/feeders** (every DropWeapon(-2) call site, PLT-verified):
  `CloseCombatAttack` @0x86249 (⇒ kung-fu punches 35 / kicks 45 / back-kick 55 /
  stunt-kick 35 **and** the jump-kick 10–20(×2) via CTSStunt::CloseAttack — the only
  two CloseCombatAttack callers), `ColdCockAttack` @0x86820 (pistol-whip), and
  `CheckForBreakables` @0x8c2ae (run-through, kung-fu type only). Knife/katana
  direct slashes and their run-through do NOT disarm.
- **Increment** = the hit's flDamage. **Decay** = PreThink @+0x237: `0x7b4 -=
  GiveSlowMul()*frametime*5` floor 0 (≈5/s). So a 45 kick ≥ thr for every weapon with
  slotCost ≤ 45; heavy 50-cost guns need two hits inside ~3 s; a 35 punch disarms
  pistols/SMGs (thr 20-ish) outright.
- **Spawn-protect:** gates the CALL at every site (`victim+0x71c < time`), so a
  spawn-protected victim neither drops NOR accumulates pressure.
- **Ours:** the core is already faithful — ts_gun.cpp:3129-3147 (accumulate,
  thresholds 30/25/clamp(slotCost,20,50), reset, fThrow=false), player.cpp:2065
  (×5 decay), ts_player.cpp:799/877 (spawn-protect-gated -2 calls, kung-fu +
  coldcock), ts_stunt.cpp:772-773 (jump-kick passes bDisarm=true). **The only
  missing feeder is the §2 run-through.**

---

## 4. LINE-BY-LINE DIFF — the mismatches that are the bug

**A. ts_player.cpp:757-828 `CBasePlayer::CloseCombatAttack` — MATCHES the binary.**
Gate (`bKnockback && flDamage > 30`), formula, order (disarm→knockback→sound→
TraceAttack), spawn-protect on disarm only: all byte-equivalent. (Cosmetic-only
divergence: the original's dead PunchScreen call has an extra
`m_pActiveItem->m_weaponType==0` condition; PunchScreen is an empty stub.) Same for
ColdCockAttack:834-909. **Do not touch.**

**B. ts_stunt.cpp:804-853 `CTSStunt::CheckForBreakables` — the gap.** Diffs vs §2a:
- :816-820 bails on FL_ONGROUND unconditionally → original bails only when
  `FL_ONGROUND && window==0` (a grounded kicker's window must still sweep).
- :823 speed floor 100 → original 64, and the check is bypassed when the window is
  live (`speed<64 && window==0`).
- :827-828 sweep is always velocity·100u → original: velocity·64u only when NO
  window; window-active uses v_angle MakeVectors ±32u (sign of 0x7a0).
- :838-839 returns for ALL players → original only when window==0.
- missing FL_WORLDBRUSH check.
- :850 dmg=speed always → original `IsAlive() ? 0.1*speed : speed` (living runner
  does 30 @300 u/s to breakables; a hurled corpse does full speed).
- :850 dir=velocity-dir → original passes `g_vecZero` as TraceAttack dir.
- missing the whole window-active player package (Victim*Hold, DropWeapon(-2),
  >30 knockback+z235, per-type sounds, glast_closecombat + 0x4001080 bits, 0x7a0
  halving).
- :844/852 `m_pLastBreakVictim` (INVENTED member on CTSStunt) → original uses the
  PLAYER's `+0x798 m_hRunThroughLast` (already declared, player.h:454) and resets it
  in the grounded-no-window branch.

**C. ts_gun.cpp:1657-1662 CCombatAttack miss-branch — WRONG VALUES stored.**
Ours: `m_flWhiffLungeTime = whiffT (0.1–0.35)`, `m_flWhiffLungeForce = ±lunge
(scaled 10–30)`. Original @0x76d02-0x76d50: `0x79c = the scaled per-move value
(10–30, = our `lunge`)`, `0x7a0 = ±flat (35/45/55, = our `reach`)` // VERIFY §5a.
`whiffT` (local_10c) is ONLY the BlockPlayer scalar (ts_gun.cpp:1690 is correct).
Consequence even after B lands: with ours, run-through dmg ≤30 ⇒ the >30 knockback
would never fire and the window would die in ~0.1-0.35 s.

**D. player.cpp:2080-2090 PreThink whiff block — two divergences.**
- The decay floors at 0 and expires inside the same `-=` flow; original floors the
  subtract at **0.001** and runs the expiry as a separate check (shape difference,
  behaviorally near-equal — optional).
- `m_flKungFuBlock = 0` sits INSIDE the expiry branch (:2088) → original zeroes it
  **every frame the window is non-zero** (@0x80e5e-ish, after the expiry if, not in
  it).

**E. ts_stunt.cpp:207 HandleStunt — missing the whiff triple clear** the original has
near its head (@0x8b04b: `0x79c=0; 0x7a0=0; 0x7a4=0` when the handler engages a
stunt path). HolsterWeapon's clear exists and matches (ts_gun.cpp:646-650).

**F. (comment rot) ts_player.cpp:609** says m_hRunThroughLast is "0x788 region" —
the binary says **0x798** (player.h:454 already correct).

---

## 5. FIX SPEC (apply without re-RE)

### 5.1 dlls/ts_gun.cpp — CCombatAttack miss-branch (:1657-1662)
Replace the stores with the original's values (names kept):
```cpp
if (pHit == nullptr && lunge > 0.001f)           // gate is on the SCALED value
{
    m_pPlayer->m_flWhiffLungeTime  = lunge;                        // 0x79c ← scaled 10–30
    m_pPlayer->m_flWhiffLungeForce = (bVar9 == 3) ? -reach : reach; // 0x7a0 ← ±flat 35/45/55  // VERIFY §5a
    m_pPlayer->m_iWhiffLungeType   = 0;
}
```
(`whiffT` remains only in `BlockPlayer(whiffT * 0.75f)` — unchanged.)

### 5.2 dlls/ts_stunt.cpp — CheckForBreakables (:804-853): rewrite to §2a
Exact edits:
1. Replace the `FL_ONGROUND` bail with:
   `if ((pev->flags & FL_ONGROUND) && m_pPlayer->m_flWhiffLungeTime == 0.0f)
    { m_pPlayer->m_hRunThroughLast = NULL; return; }`
2. Speed gate: `if (speed < 64.0f && m_pPlayer->m_flWhiffLungeTime == 0.0f) return;`
3. Sweep build:
   ```cpp
   Vector end;
   if (m_pPlayer->m_flWhiffLungeTime == 0.0f) {
       Vector dir = (speed != 0.0f) ? pev->velocity * (1.0f/speed) : Vector(0,0,1);
       end = pev->origin + dir * 64.0f;
   } else {
       UTIL_MakeVectors(pev->v_angle);
       end = pev->origin + gpGlobals->v_forward *
             ((m_pPlayer->m_flWhiffLungeForce < 0.0f) ? -32.0f : 32.0f);
   }
   ```
   TraceHull stays `dont_ignore_monsters, head_hull` from `pev->origin`.
4. Victim gates: `Instance(tr.pHit ? tr.pHit : ENT(0))`; then
   `if (pHit->IsPlayer() && m_pPlayer->m_flWhiffLungeTime == 0.0f) return;`
   then takedamage / `(pHit->pev->flags & FL_WORLDBRUSH)` / EF_NODRAW checks.
5. Base damage: `float dmg = m_pPlayer->IsAlive() ? 0.1f * speed : speed;`
6. Dedupe: `if (m_pPlayer->m_hRunThroughLast == pHit) return;` — and DELETE
   `CTSStunt::m_pLastBreakVictim` (ts_stunt.h:111, invented; audit INVENTED item).
7. Window-active package (before the TraceAttack tail), transliterating §2a:
   ```cpp
   int bits = DMG_CLUB | DMG_NEVERGIB;
   bool bApply = true;
   if (m_pPlayer->m_flWhiffLungeTime != 0.0f)
   {
       if (m_pPlayer->m_iWhiffLungeType == 0) { glast_closecombat = true; bits |= DMG_TSCLOSECOMBAT; }
       dmg = fabsf(m_pPlayer->m_flWhiffLungeForce);
       m_pPlayer->m_flWhiffLungeForce *= 0.5f;
       if (fabsf(m_pPlayer->m_flWhiffLungeForce) < 5.0f) m_pPlayer->m_flWhiffLungeForce = 0.0f;
       if (pHit->IsPlayer() && dmg > 0.0f)
       {
           if (friendlyfire.value == 0.0f &&
               g_pGameRules->PlayerRelationship(pHit, m_pPlayer) == GR_TEAMMATE)
               bApply = false;
           else if (m_pPlayer->m_iWhiffLungeType == 0)
           {
               if (!VictimKungfuHold(m_pPlayer, pHit, dmg, &tr))
               {
                   CBasePlayer *pVic = (CBasePlayer *)pHit;
                   if (pVic->m_flSpawnProtectTime < gpGlobals->time)
                       pVic->DropWeapon(-2, dmg);
                   if (dmg > 30.0f)
                   {
                       float aspeed = pev->velocity.Length();
                       Vector kdir = pHit->pev->origin - pev->origin;
                       kdir = kdir.Length() > 0.0f ? kdir.Normalize() : Vector(0,0,1);
                       pHit->pev->velocity = pHit->pev->velocity + kdir * aspeed;
                       pHit->pev->velocity.z = 235.0f;
                   }
                   EMIT_SOUND_DYN(ENT(pHit->pev), CHAN_BODY, "player/kungfuhit.wav",
                                  VOL_NORM, 0.8f, 0, 95 + RANDOM_LONG(0, 31));
               } else bApply = false;
           }
           else if (m_pPlayer->m_iWhiffLungeType == 2)
           {
               if (!VictimKatanaHold(m_pPlayer, pHit, dmg))
                   EMIT_SOUND_DYN(ENT(pHit->pev), CHAN_BODY, "weapons/katana/hit-body.wav",
                                  VOL_NORM, 0.8f, 0, 95 + RANDOM_LONG(0, 31));
               else bApply = false;
           }
           else // knife (1)
           {
               if (!VictimKnifeHold(m_pPlayer, pHit, dmg))
                   EMIT_SOUND_DYN(ENT(pHit->pev), CHAN_BODY, "weapons/knife/knife_hitbody.wav",
                                  VOL_NORM, 0.8f, 0, 95 + RANDOM_LONG(0, 31));
               else bApply = false;
           }
           // PunchScreen branch is a dead stub in the .so — omit or keep shape-faithful.
       }
   }
   if (bApply)
   {
       ClearMultiDamage();
       pHit->TraceAttack(pev, dmg, g_vecZero, &tr, bits);   // dir = g_vecZero
       ApplyMultiDamage(pev, pev);
   }
   glast_closecombat = false;
   m_pPlayer->m_hRunThroughLast = pHit;
   ```
   All three sounds are already precached (ts_gun.cpp:750, ts_weapons.cpp:106/109);
   VictimKungfuHold/VictimKnifeHold/VictimKatanaHold and DMG_TSCLOSECOMBAT
   (ts_player.cpp:745 — hoist to a shared header or re-#define) already exist.

### 5.3 dlls/ts_stunt.cpp — HandleStunt (:207): add the whiff triple clear
Mirror @0x8b04b: at the point the handler commits to a stunt path (right after the
early-out gates), set `m_flWhiffLungeTime = 0; m_flWhiffLungeForce = 0;
m_iWhiffLungeType = 0;` on m_pPlayer. // VERIFY exact gate placement (0x8b04b sits
~0x4b into the function, behind one `jne`).

### 5.4 dlls/player.cpp — PreThink (:2080-2090)
Move `m_flKungFuBlock = 0.0f` OUT of the expiry if — it must run every frame that
`m_flWhiffLungeTime != 0`. Optional shape-parity: decay with floor 0.001 + separate
expiry check (<0.01 || force==0).

### 5.5 Comment/audit hygiene
- ts_player.cpp:609: fix "0x788 region" → 0x798.
- ts_stunt.h:111: remove `m_pLastBreakVictim` (replaced by player m_hRunThroughLast).
- Member-name note for the audit: 0x79c/0x7a0/0x7a4 semantics are
  window/damage/type of the RUN-THROUGH attack, not an attacker lunge. Renaming
  (e.g. m_flRunThroughWindow/m_flRunThroughDamage/m_iRunThroughType) is
  RECOMMENDED for truthfulness but the original names are unknown (no symbols for
  members) — keep current names if churn is a concern; update the comments either way.

### 5a. // VERIFY (needs ONE live trace — do not guess further)
**Which value lands in 0x7a0 on a kung-fu whiff: ±flat 35/45/55 (Ghidra's read,
used above) or ±scaled 10–30 (what our code currently stores).** Ghidra's decompile
renders both flDamage and 0x7a0 as the same local (the flat value, and flDamage=flat
is live-verified from cont.93); the raw-disasm esp-tracking of the reused stack slots
was not fully conclusive this session. Settle on the live original:
`pgrep -f 'Half-Life/hl.exe'` → `scripts/live.py watch <player+0x7a0 abs addr> --go
"whiff a KICK into empty air (no hit), then freeze"` — or simpler, `live.py read` of
victimless player+0x7a0 right after a whiffed kick: **45.0 ⇒ flat (keep 5.1); ~15–30
⇒ scaled (then 5.1 stores ±lunge and the >30 run-through knockback fires only for
back-kick/kungfu-powerup)**. This single readout decides the constant; everything
else in this dossier is disasm-anchored.

Also unresolved statically (secondary):
- Whether OUR direct-hit z=235 pop on a grounded victim reproduces at runtime
  (statically our path is byte-equivalent; if playtest step 3 below fails on our
  server, gdb OUR hl.so at ts_player.cpp:802 and check pVictim->pev->velocity after
  the write, then the victim's next pmove — suspect list: rcbot velocity stomps).
- The iAnim arg garble in the original CCombatAttack call (Ghidra shows local_104;
  ours passes moveId) — cosmetic (3rd-person anim choice), not knockback-relevant.

### 5.6 Playtest protocol (user-driven, numbered)
Original client vs OUR server, `friendlyfire` off, one live victim (human 2nd client
if possible; bots can mask velocity effects):
1. **Standing punch, both still, point-blank:** victim should POP UP (~235 z, ≈¾ jump
   height) and take 35. Same on the ORIGINAL server for comparison.
2. **Sprint-kick THROUGH the victim (start 2-3 m away, hold W, kick just before
   contact):** victim must fly BACK hard (your sprint speed horizontally + up) —
   this exercises the new run-through; expect kungfuhit.wav and 45 dmg.
3. **Same as 2 on a grounded victim near a wall:** confirms grounded launch.
4. **Back-kick whiff then reverse into someone behind you:** the −32u sweep should
   connect (55 dmg, knockback).
5. **Disarm:** give the victim a pistol; one kick (45 ≥ thr) should send the gun
   flying (weapondrop.wav, weapon at feet); with a heavy rifle (slotCost ≥ 50) it
   should take two quick kicks; a freshly spawned (spawn-protected) victim must NOT
   drop.
6. **Jump-kick (no KungFu powerup):** damage 10–20, NO fly-back (matches original —
   report this as expected, not a bug); with the KungFu powerup it doubles and CAN
   knock back.
7. **Blocked counter:** victim holds kung-fu block (reload key) — your kick must be
   deflected (clack, no damage, no knockback, no disarm).
8. Cross-check 1-5 against the ORIGINAL server for drop-in identity.

---

## 6. One-paragraph summary

The direct-hit knockback (CloseCombatAttack @0x85f88) is already byte-faithful in our
tree — gate `bKnockback && flDamage > 30` on the raw pre-multiplier damage, velocity
+= |attacker vel|·unit(victim−attacker), z hard-set 235, applied to grounded living
victims with no extra conditions. What's MISSING is the original's second melee
delivery channel: the whiff/run-through attack — a missed kung-fu swing arms
player+0x79c/0x7a0/0x7a4 (window / ±damage 35/45/55 / type) and CheckForBreakables
@0x8bd9c then turns the attacker's moving hull into the weapon, delivering
disarm + the same >30 knockback at full sprint speed; our CheckForBreakables skips
players entirely and our CCombatAttack stores the wrong two values into the window
fields. Disarm in the original is real and strong: every kung-fu hit (and the
run-through, pistol-whip, jump-kick) pumps m_flDisarmPressure (+0x7b4, decays 5/s);
threshold = 30 (type 0x22) / 25 (melee class) / clamp(slotCost, 20, 50), so one
45-kick disarms most guns; spawn-protected victims are exempt. Our DropWeapon(-2)
core already matches — the run-through feeder is the only missing piece.
