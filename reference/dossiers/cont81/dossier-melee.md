# MELEE CLUSTER DOSSIER — 9 missing methods, reconcile plan, live-trace list

Synthesized from the 9 per-method analyses in this scratchpad:
`analysis_updatemeleebar.md`, `analysis_closecombat.md`, `analysis_coldcock.md`,
`analysis_blockplayer.md`, `analysis_knifefire.md`, `analysis_katanafire.md`,
`analysis_katanathrow.md`, `analysis_ccombatidle.md`, `analysis_waittouch.md`.
Cross-checked against `reference/coverage/AUDIT.md` (lines 27–28, 49–51).
**NOTE:** `dossier-ctsstunt.md` was NOT present in the scratchpad — the GoWheel
backward-backkick integration point is covered here only by the
`analysis_updatemeleebar.md` fragment notes (@0x9cdb8); re-check against the stunt
dossier when it exists.

AUDIT work-list covered (MISSING → implement):
- CBasePlayer: `UpdateMeleeBar`, `CloseCombatAttack`, `ColdCockAttack`, `BlockPlayer`
  (+ dependency `CheckHull`, also on the AUDIT list)
- CTSGun: `KnifeFire`, `KatanaFire`, `KatanaThrow`, `CCombatIdle`
- CTSKnife: `WaitTouch`

INVENTED (absorb/delete): CTSGun `MeleeAttack`/`ThrowKnife`/`RegenBlockBar`;
CTSKnife `BecomePickup`/`Classify`/`PickupTouch`/`SettleThink`/`StartSettle`/`StickAt`.

---

## 1. SYSTEM OVERVIEW — how the original melee system hangs together

### 1.1 Dispatch topology

```
CBasePlayerWeapon::ItemPostFrame (stock, button+timer gates live HERE)
 ├─ PrimaryAttack (CTSGun, @0x71a20) — dispatch on m_pActiveInfo->iFireType (+0x348):
 │    no item        → CCombatAttack (kung-fu; out of this cluster's scope, but its
 │    │                 tail is rewired in step 5)
 │    iFireType == 3 → item+0x24 = 0; KnifeFire()          (knife SLASH)
 │    iFireType == 10→ KatanaFire()                         (5-stance katana slash)
 │    iFireType == 11→ gated on player+0x814 & 1; item+0x24=0; KnifeFire()  [VERIFY gate]
 │    iFireType == 2 → GrenadeFire
 ├─ SecondaryAttack (@0x717a8):
 │    no item        → CCombatAttack
 │    iFireType == 2 → item+0x24 = 1; GrenadeFire
 │    iFireType == 3 → item+0x24 = 1; KnifeFire()          (knife THROW)
 │    iFireType == 10→ KatanaThrow()   (NO item+0x24 write)
 │    iFireType == 11→ nothing; then akimbo/scope/hold branches
 ├─ AlternateAttack (+alt2, @0x71ba8): gate m_pActiveItem && info->coldcock(+0x10) > 0
 │    → ColdCock() (@0x71dd0) → CBasePlayer::ColdCockAttack (@0x86610)
 ├─ Reload (melee) → CCombatBlock (@0x85ebc/0x75ebc) — RELOAD = raise block
 └─ WeaponIdle (@0x754dc): if (!m_pActiveItem) { CCombatIdle(); return; } else armed path
```

### 1.2 The two shared player attack helpers (twins, identical signature)

`CBasePlayer::CloseCombatAttack` @0x85f88 and `CBasePlayer::ColdCockAttack` @0x86610:
`CBaseEntity* (Vector src, Vector end, Vector dir, int iHull, float dmg, int iAnim,
bool bDisarm, bool bPlayersOnly, bool bKnockback, bool bForceTeamSafe)`.
Skeleton (both): Animate(iAnim, runs on miss too) → [CloseCombat only: TSMBlur roll]
→ `CheckHull` (line trace, then hull-N fallback + FindHullIntersection) → validity gates
(takedamage==NO / FL_WORLDBRUSH / EF_NODRAW) → bPlayersOnly gate → teammate gate
((bForceTeamSafe || friendlyfire==0) && PlayerRelationship==GR_TEAMMATE → NULL) →
`VictimKungfuHold` deflect gate → disarm (`DropWeapon((tsweapon_t)-2, dmg)` gated on
victim +0x71c timer) → knockback (dmg>30: vel += dir*|attackerVel|; **vel.z = 235**) →
kungfuhit.wav on **VICTIM** CHAN_BODY attn 0.8 pitch 95+rand(0,31) → PunchScreen (no-op
in shipped binary) → `glast_closecombat`/`glast_coldcock` = 1; ClearMultiDamage;
TraceAttack; ApplyMultiDamage; glast = 0 → return victim.
Damage bits: CloseCombat `0x4001080` = DMG_CLUB|DMG_NEVERGIB|0x4000000 TS marker;
ColdCock's bits obscured in decompile [VERIFY].
Callers: CloseCombatAttack ← CTSGun::CCombatAttack (kung-fu) and CTSStunt::CloseAttack
(jump-kick); ColdCockAttack ← CTSGun::ColdCock only.

### 1.3 The block/deflect/meleebar state machine (all state on CBasePlayer)

| member | offset | semantics |
|---|---|---|
| `m_flMeleeBar` | +0x774 | float 0..100 stamina. Drains: raise block 33 · get blocked 33 · kick hit `dmg*const` · backkick 22.5. Depleted (<0.01) → bar=0 + `m_flNextAttack=1.5` (1.5 s global lockout). **Regen site UNKNOWN — live trace.** Only writer = UpdateMeleeBar. |
| whiff-lunge window | +0x79c | float; armed on a MISSED swing (knife 0.25×rate, katana 0.25, kungfu = power; backkick 0.3) |
| whiff-lunge force | +0x7a0 | float, SIGNED (back-kick/back-slash negative → lunge backward); doubled if KungFu powerup; zeroed on any successful block |
| whiff-lunge type | +0x7a4 | byte: 0 kungfu · 1 knife · 2 katana; VictimKungfuHold counter reads ==0 |
| kungfu block window | +0x7a8 | float = **0.5 (plain value)** set by CCombatBlock bare-handed; zeroed when the player attacks. **Decay/expiry site NOT FOUND — do not assume time+0.5.** |
| knife block window | +0x7ac | = 0.5 when blocking with active iFireType==3 |
| katana block window | +0x7b0 | = 0.5 when iFireType==10; VictimKnifeHold ALSO falls through to it (katana blocks a knife) |
| stop/root timer | +0x4dc | float; `BlockPlayer(dur)` sets it + `iuser4 |= TSSTUNT_STOP(1)`; PreThink counts down by `GiveSlowMul()*frametime` (×1.5 if m_iPowerups&0x10), floors 0.001; <0.01 → clear bit + zero. pm_shared halves fwd/side/upmove while bit set. |
| last melee refire | +0x4f8 | written = rate by every melee fire [VERIFY consumer] |
| duck-slash aux | +0x4f4 | = 2.0 by katana duck-slash [VERIFY consumer] |
| disarm immunity | +0x71c | time; gates DropWeapon(-2) in the twins [VERIFY reset] |
| `m_flNextAttack` | +0x12c | stock SDK global attack gate (CLIENT_WEAPONS countdown: raw 0.75/0.5/0.35/0.25 stores and WeaponTimeBase()+c are consistent since base=0) |

Deflect gates (free functions, TS_gamerules.cpp): `VictimKungfuHold` @0x66f50 /
`VictimKnifeHold` @0x66de4 / `VictimKatanaHold` @0x66c8c. Require victim IsPlayer +
matching window >0 + victim bar > 0.1. On deflect: `UTIL_Sparks(midpoint+20z)`,
knife_hit.wav (knife/katana, attacker edict) or kungfuhit.wav (kungfu), zero +0x7a0 both,
asymmetric next-attack locks (katana 0.75/0.35, knife 0.75/0.35 per knifefire analysis
[0.5/0.25 per kungfu]), **`UpdateMeleeBar(attacker, 33.0)` — being blocked drains the
ATTACKER**. KungfuHold has the COUNTER twist: attacker window open while victim mid-lunge
→ roles swap [VERIFY which bar drains post-swap].

### 1.4 Kill-flavor globals

`glast_closecombat` @0x104480, `glast_coldcock` @0x104481, `glast_headshot` @0x104482 —
set around TraceAttack/ApplyMultiDamage; read by TSGetPointsForFrag +
CHalfLifeMultiplay::PlayerKilled + CBasePlayer::Killed: death-notice weapon string
rewritten ("Kung Fu" / `<weapon> + " Stock"`) + HUD broadcast
`TSHudMessageAll(0,0xff,0,1,0x14, "#TS_Coldcock #TS_FrgFr %s!")`. **Wire-visible swap
contract** (kill feed + HUD text).

### 1.5 Client↔server protocol surface

- **User messages:** `KFuPower` (1 CHAR, MSG_ONE, sent from INSIDE UpdateMeleeBar every
  call, no change-detect) · `TSMBlur` (2 bytes: attacker entindex, level 1|2; MSG_PVS at
  attacker origin; rolled `RANDOM_LONG(10,80) < dmg`, level 2 if dmg>50) · WeaponInfo's
  "clip" byte carries `ROUND(m_flMeleeBar)` when switching to a melee weapon.
- **Events** (gun handle block from Precache @0x70660): +0x964 fire.sc · **+0x966
  coldcock.sc** · +0x968 empty.sc · +0x96a pump.sc · **+0x96c closecombat.sc** ·
  +0x96e firelinked.sc · +0x970 reload.sc · **+0x972 knife.sc** · **+0x974 katana.sc** ·
  **+0x976 draw.sc** · +0x978 grenade.sc.
  - knife.sc: EVERY slash, FEV_NOTHOST, iparam1=weaponType, iparam2=bloodyFlag, no coords.
  - katana.sc: EVERY swing, FEV_NOTHOST, iparam1=vmAnim 15/16/17, iparam2=bloody.
  - coldcock.sc: fired even on miss (gated only on owning the item), fparam1/2 = 0.0
    (never-written static — original source bug, replicate), iparam1=weaponType,
    iparam2=attachEquipped.
  - closecombat.sc: from CCombatBlock (iparam2 = 1 bare / 0x12 armed, fparam1=weaponType)
    and from the CCombatAttack tail.
  - draw.sc: knife-throw redraw, iparam1=weaponType, iparam2=attachEquipped.
- **Sounds** (audible contract): melee hit sounds ride the VICTIM's edict CHAN_BODY,
  attn 0.8, pitch 95+rand(0,31), via TS's 8-arg `EMIT_SOUND_DYN(..., 0.5f)` (trailing
  float = slowmo pitch weight vs pev->fuser1 [VERIFY formula]). NO server whoosh on
  slash/kungfu (the event carries presentation); knife_miss.wav = the knife-THROW whoosh;
  katana/miss.wav = katana throw. kevlarhit.wav (pitch 60–91) = coldcock attacker confirm.
- **`pev->iuser4` bit 1 (STOP)** is networked state consumed by pm_shared on both sides
  (half-speed move) — our pm_shared already implements the consumer.

### 1.6 CTSKnife projectile lifecycle

`CreateKnife(CBasePlayer*, Vector pos, Vector vel, int typeCode)` — typeCode =
`s_weapon_info+0x20`: knife 6 · sealknife 7 · katana 8 (drives model/gravity/pickup id;
stored in knife +0xa0). Damage computed INSIDE CreateKnife [VERIFY source]. Throwers set
orientation after spawn: `angles = VecToAngles(normalize(vel))`, `angles.z = 90`,
`avelocity.x = -1300` (KnifeFire, speed 1900) / `-700` (KatanaThrow, speed 1000).
`BounceTouch` settle tail: sparks + knife_hit.wav (CHAN_BODY pitch 100) →
`SetTouch(WaitTouch)` + `SetThink(WaitThink)` + nextthink +0.1 + gravity 0.75 +
in-flight(+0x90)=0 + expire(+0x88)=time+10. `WaitTouch` = touch-driven damping
(FL_ONGROUND: vel×0.5; |vel|<32 → flatten pitch/roll, vel=0; keeps MOVETYPE_BOUNCE).
Pickup exclusively +USE (`ObjectCaps` FCAP_CONTINUOUS_USE → `Use` with gates: in-flight,
IsPlayer/IsAlive/!EF_NODRAW, katana(+0x91) requires `KungFu(player)`, expire → silent
remove, PickUpWeapon-failure → knife stays, `RemoveEntityFromSlowedArray` on remove).

### 1.7 GoWheel backkick dependency (stunt cluster)

The backkick tail @0x9cdb8 (dir==3): `UpdateMeleeBar(22.5)` then arm whiff-lunge
(+0x79c=0.3, +0x7a0=±dmg doubled-if-KungFu, +0x7a4=0, +0x7a8=0). This is the chain that
currently BLOCKS the GoWheel backward-backkick combo in our build (ts_stunt.cpp:454
TODO). Implementing UpdateMeleeBar + the lunge members unblocks it.

---

## 2. PER-METHOD KEY FACTS (depth → the analysis_*.md files)

### CBasePlayer::UpdateMeleeBar(float flCost) — @0x8650c, TS_player.cpp
`bar -= cost`; `<0.01` → bar=0 + m_flNextAttack=1.5; then ALWAYS send KFuPower
(MSG_ONE, WRITE_CHAR(round(bar))). No upper clamp inside. No regen caller found —
**regen site/rate = live trace, our 25/s is user-tuned**. Callers: CCombatBlock(33),
Victim*Hold(attacker, 33), kick @0x9c70c (dmg*const), backkick @0x9cdb8 (22.5).
→ `analysis_updatemeleebar.md`

### CBasePlayer::CloseCombatAttack(...) — @0x85f88, TS_player.cpp
Full skeleton §1.2 + TSMBlur roll + VictimKungfuHold + DMG bits 0x4001080 +
glast_closecombat. 2 callers: CTSGun::CCombatAttack (hull = class==3?1:3, disarm=1,
playersOnly=0, knockback=1, teamSafe=0), CTSStunt::CloseAttack (hull 3, iAnim=-1,
disarm=1, knockback=1, teamSafe=1). → `analysis_closecombat.md`

### CBasePlayer::ColdCockAttack(...) — @0x86610, sibling twin
Same skeleton, glast_coldcock, victim kungfuhit + `PunchScreen(dmg*0.5)` always (no-op).
Caller CTSGun::ColdCock: range **35**, hull 3, per-weapon dmg = info+0x10 (aug/benelli/
m16/m4 35 · barrett 70 · deagle 25 · m60/sawedoff/usas 50 · mac10/mp5sd 30), ×2/3 while
stunting (iuser4 & 0x70c30), cadence 0.725 (×1.5 stunting), all four gun timers + player
m_flNextAttack, Unzoom/StopReload/GrenadeIdle, anim 39 (−1 stunting), all four bools
false, kevlarhit confirm pitch 60–91, coldcock.sc even on miss. AlternateAttack gate =
`m_pActiveItem && coldcock>0` (NOT clip, NOT timers — dispatcher gates [VERIFY]).
Death notice `" Stock"` + `#TS_Coldcock` broadcast. → `analysis_coldcock.md`

### CBasePlayer::BlockPlayer(float dur) — @0x85b58, 24 bytes
`+0x4dc = dur; iuser4 |= 1`. NOT the deflect-block — a movement self-root. Lifecycle in
PreThink (countdown ×GiveSlowMul×frametime, ×1.5 powerup&0x10, floor 0.001, expiry <0.01
clears bit); reset by TSInit + CTSStunt::Play (2 sites). Callers: CCombatAttack tail
(`rootFlag && !(iuser4&1)` → `BlockPlayer(refire*0.75)` on the ATTACKER — our old
"victim block-stun" comment is WRONG); KatanaFire inline copy (delay×0.9, back/duck
slash only). Knife gets none. → `analysis_blockplayer.md`

### CTSGun::KnifeFire() — @0x74044, TS_gun.cpp
Handles slash AND throw on `item+0x24` (0=slash from Primary, 1=throw from Secondary).
Common: stunting ×1.5 rate; both next-attacks +rate; idle +3.0; player+0x4f8=rate.
SLASH: anims 29–31, dmg = flDamage ×(2/3 stunting) ×GetMeleeMultiplier, CheckHull hull-3
range **35**, victim must IsAlive && !EF_NODRAW (props take NO slash damage),
VictimKnifeHold gate, `DMG_SLASH|DMG_NEVERGIB (0x1004)`, player-hit → bloody (item+0x8=1,
pev->body=1) + knife_hitbody.wav victim CHAN_BODY; miss → whiff (+0x79c=0.25×rate,
+0x7a0=dmg, +0x7a4=1); knife.sc ALWAYS; clip untouched.
THROW: wall-clipped spawn trace (eye+right×1 → +fwd×32), CreateKnife @1900, roll 90,
avel.x −1300, anim 32 (mask 0x30c30), knife_miss.wav always, consume gated
`!InfiniteClips` (slots += slotCost; SetPlayerSpeed(GetSpeedBySlots); clip−−), 0 knives →
`DropWeapon(type, true)`, else model-derived redraw SendWeaponAnim + draw.sc + idle 2.0.
→ `analysis_knifefire.md`

### CTSGun::KatanaFire() — @0x733f0
5-stance table {reach, dmg, delay, vmAnim, bodyAnim, lock}: STUNT 35/35/1.0/17/−1 ·
BACK 55/**−75**/1.0/15/35/lock · DUCK 50/65/0.9/17/37/lock (+PRONE|STOP, view_ofs 16,
+0x4f4=2.0) · NORMAL 50/45/0.7/17|16/33|34 (m_lastMeleeAnim 1↔2 cycle) · AIR
50/clamp(speed×2/13,35,55)/1.1/17/36. movingBack = speed≤100 ? IN_BACK :
dot(fwd,velNorm)≤−0.5. dmg ×GetMeleeMultiplier; signed dmg → trace direction; CheckHull
(dmg<0?1:3); VictimKatanaHold gate; DMG 0x1080; bloody state; hit-body.wav victim;
miss → whiff (+0x79c=0.25, +0x7a0=signed dmg, +0x7a4=2); lock → +0x4dc=delay×0.9 + STOP;
katana.sc ALWAYS (iparam1=vmAnim, iparam2=bloody); both timers +delay, idle +3.0.
Duck condition reads FL_DUCKING off the **GUN's** pev (original-source slip — replicate
as-is [VERIFY live]). → `analysis_katanafire.md`

### CTSGun::KatanaThrow() — @0x73b68
Gate m_pActiveItem only. Timers +info->flSpread(0.7)/idle+3. Same 32-unit eye trace;
CreateKnife @**1000**, roll 90, avel.x **−700**; Animate(0x20=TSA_CC_KNIFELAUNCH) gated
!(iuser4&0x30c30)&&ONGROUND; katana/miss.wav CHAN_BODY pitch 95+rand31; consume:
`!InfiniteClips` → slots += 10 + SetPlayerSpeed + `DropWeapon(m_weaponType, true)`.
NO SendWeaponAnim, NO event, NO clip logic. Protocol impact: none.
→ `analysis_katanathrow.md`

### CTSGun::CCombatIdle() — @0x75e54, 104 bytes
Kung-fu viewmodel idle: idle-timer gate → idle = base+3.0 →
`SendWeaponAnim(*(short*)(get_weapon_info(TSW_MELEE)+0x252))` (== 0 in the data; keep as
table read → new `iIdleAnim` field). Sole caller: WeaponIdle's leading
`if (!m_pActiveItem) { CCombatIdle(); return; }`. No protocol. → `analysis_ccombatidle.md`

### CTSKnife::WaitTouch(CBaseEntity*) — @0x6dce8, TS_grenades.cpp
~10 lines: FL_ONGROUND gate → vel ×= 0.5 → |vel|<32 → angles.x=angles.z=0, vel=0.
Installed only by BounceTouch's settle tail; engine-dispatched. Original has NO pickup
conversion (no glow/trigger); settled knife expires +10 s; pickup = +USE only.
Katana IS pickable by KungFu players (+0x91 gate) — contradicts our comment [VERIFY live].
→ `analysis_waittouch.md`

---

## 3. RECONCILE PLAN (ordered; every step builds green; playtest gates marked)

Rename/absorb map (audit INVENTED → original):

| ours (invented) | fate |
|---|---|
| `CTSGun::MeleeAttack` | SPLIT → `KnifeFire` (slash half) + `KatanaFire` (rewritten, 5-stance) — then delete |
| `CTSGun::ThrowKnife` | knife half absorbed into `KnifeFire` throw branch; katana half → new `KatanaThrow` — then delete |
| `CTSGun::RegenBlockBar` (+`m_blockBar`, `m_blockBarTime`, `m_blockCooldown`, `m_iClientBlockBar`) | DELETE; bar moves to `CBasePlayer::m_flMeleeBar` (+0x774); regen becomes a flagged provisional (live-trace gap) |
| `CTSKnife::StartSettle`/`SettleThink` | DELETE → `WaitTouch` + rewired BounceTouch tail |
| `CTSKnife::BecomePickup`/`PickupTouch` | DELETE → original `Use` gates (+USE only, no glow) |
| `CTSKnife::StickAt` | re-inline into BounceTouch (original has no such function) or keep marked non-original helper |
| `CTSKnife::Classify` | DELETE (not in the original's method set: Spawn/BounceTouch/WaitTouch/WaitThink/Use/GetDescription/ShouldCollide/ObjectCaps/CreateKnife — same pattern as the invented-ObjectCaps removal). Confirm no behavioral dependency before removal. |

Genuinely NEW code (nothing to absorb): `CBasePlayer::CheckHull`, `CloseCombatAttack`,
`ColdCockAttack`, `UpdateMeleeBar`, `BlockPlayer`; `Victim{Kungfu,Knife,Katana}Hold`
free functions; `CTSGun::CCombatIdle`, `KatanaThrow`, `SecondaryAttack` (dispatch
method); `glast_*` globals; weapon-table columns.

### Step order

**S0 — inert state (behavior-preserving, no playtest).**
Files: `dlls/player.h`, `dlls/ts_weapon_info.h/.cpp`, a globals home (ts_gamerules or
ts_player).
- CBasePlayer members (transient, NOT saved; provenance comments): `m_flMeleeBar` +0x774,
  whiff cluster +0x79c/+0x7a0/+0x7a4, block windows +0x7ac/+0x7b0 (keep +0x7a8 as
  existing `m_flKungFuBlock` — do NOT convert its semantics yet, see live-trace L2),
  `m_flBlockTime` +0x4dc, disarm timer +0x71c, +0x4f8, +0x4f4.
- Globals `glast_closecombat`/`glast_coldcock`/`glast_headshot`.
- s_weapon_info: `iColdCockDamage` (+0x10, populate 11 rows), `iKnifeType` (+0x20:
  knife 6 / sealknife 7 / katana 8), `iIdleAnim` (+0x252, all 0).
- Precache `player/kevlarhit.wav`, `weapons/katana/miss.wav`, `weapons/katana/hit-body.wav`
  (verify which already exist).
Build: green (nothing consumed).

**S1 — UpdateMeleeBar + bar migration (PLAYTEST: block feel, HUD bar).**
Files: `dlls/ts_player.cpp` (+player.h), `dlls/ts_gun.cpp`.
- Write `CBasePlayer::UpdateMeleeBar(float)` exactly (subtract; <0.01 → 0 +
  m_flNextAttack=1.5; unconditional KFuPower MSG_ONE CHAR). Move gmsgKFuPower
  registration to player scope.
- Re-point `GetMeleeMultiplier` (ts_player.cpp:519) to the player member (kills the
  GetPlayerTSGun reach-around).
- Rework `CCombatBlock` (ts_gun.cpp:1247–1280): keep cost 33 + timers +0.5/+3.0; add the
  missing gates (m_flNextAttack, tertiary timer, `iuser4 & 0x70c30`, bar ≥ 0.01); set the
  per-fire-type window (0x7a8/0x7ac/0x7b0 = 0.5); call `UpdateMeleeBar(33.0f)`; DELETE
  the invented breathe2/cooldown branch. **Defer** the block.wav→closecombat.sc event
  swap to S7b (protocol step) if you want this step lower-risk.
- DELETE `RegenBlockBar` + the 4 invented members + the UpdateClientData change-detect
  KFuPower send. Add a provisional regen (keep the user-tuned 25/s) clearly tagged
  `// PROVISIONAL — regen site unverified, see live-trace L1`.
- WeaponInfo melee seed: write `ROUND(m_flMeleeBar)` instead of m_iClip when switching
  to a melee weapon (ts_gun.cpp:1783).

**S2 — BlockPlayer + PreThink lifecycle (PLAYTEST: post-swing root feel).**
Files: `dlls/player.cpp`/`player.h`, `dlls/ts_stunt.cpp`.
- `BlockPlayer(float)` verbatim (2 statements). PreThink countdown/expiry block (with the
  ×1.5 powerup&0x10 branch); zero `m_flBlockTime` in `CTSStunt::Play` reset points and
  TSInit-equivalent. This is also where the whiff-lunge (+0x79c) countdown belongs —
  implement the decrement together, but the lunge EXECUTION (PreThink consumer that
  applies +0x7a0) is stunt-cluster scope; stub it with a flagged TODO if the stunt
  dossier isn't reconciled yet.
- Not yet called from melee → behavior change is nil until S5/S6. Build green.

**S3 — CheckHull + Victim*Hold (behavior-preserving: compiled, uncalled).**
Files: `dlls/ts_player.cpp` (CheckHull), `dlls/ts_gamerules.cpp` (the 3 Hold free
functions — original TU per plates).
- `CheckHull(Vector, Vector, TraceResult&, int)`: TraceLine → miss → TraceHull(iHull) →
  FindHullIntersection. Its post-hull tail has an unread vtable-slot-46 check — decompile
  it fully first (own `re.sh` pass) or port the classic crowbar pattern with a VERIFY tag.
- `VictimKungfuHold/VictimKnifeHold/VictimKatanaHold` per §1.3 (sparks, sounds, +0x7a0
  zeroing, asymmetric next-attack stamps, `UpdateMeleeBar(attacker, 33)`); the KungfuHold
  counter-swap bar-drain side carries a VERIFY tag (L11).
Build green (nothing calls them yet).

**S4 — the twins CloseCombatAttack + ColdCockAttack (compiled, uncalled).**
Files: `dlls/ts_player.cpp`, plus `CBasePlayer::DropWeapon` signature fix to
`(tsweapon_t, float)` with the −2 active-weapon sentinel (internals VERIFY-gated: until
DropWeapon(-2,dmg)'s roll is RE'd, implement the call but keep our current DropWeapon
body behind the new signature, tagged).
- Both per §1.2 skeleton; TSMBlur send in CloseCombatAttack (registered already; now
  sent); ColdCock damage bits: use DMG_CLUB|DMG_NEVERGIB pending L5 verify.
Build green.

**S5 — rewire kung-fu callers (PLAYTEST: kung-fu combat end-to-end).**
Files: `dlls/ts_gun.cpp` (CCombatAttack), `dlls/ts_stunt.cpp` (CloseAttack).
- `CTSGun::CCombatAttack`: delete the inlined trace/block/damage block (ts_gun.cpp:
  1144–1221) → call `m_pPlayer->CloseCombatAttack(src, end, v_forward, class==3?1:3,
  dmg, bodyAct, true, false, true, false)`; tail per @0x76c8c: whiff-lunge arm on miss
  (negate for class 3), `BlockPlayer(refire*0.75)` under rootFlag (rootFlag per-move
  mapping VERIFY L7 — until verified, wire only where the disasm is unambiguous and
  flag the rest), closecombat.sc playback, timers, +0x7a8=0, +0x4f8=refire. DELETE the
  invented miss/surface sounds + TE_SPRITE smoke (original plays nothing on miss —
  re-check against the client .sc, L6).
- `CTSStunt::CloseAttack`: replace inlined copy with `CloseCombatAttack(..., 3, dmg, -1,
  true, false, true, true)` + faithful tail (random 0xa/0xd/0xe, next-attack top-up,
  kungfuhit, `UpdateMeleeBar(dmg*const)` — const VERIFY L10). Also wire the GoWheel
  backkick tail (`UpdateMeleeBar(22.5)` + lunge arm) — unblocks ts_stunt.cpp:454.

**S6 — ColdCock wrapper + AlternateAttack (PLAYTEST: pistol-whip).**
Files: `dlls/ts_gun.cpp`, `cl_dll` event hook.
- Rewrite `CTSGun::ColdCock` per §2-coldcock (stunt mask, timers, Unzoom/StopReload/
  GrenadeIdle, range 35, per-weapon dmg, four-false call, kevlarhit confirm, coldcock.sc
  with literal 0.0 fparams). Fix `AlternateAttack` gate; keep our timer check only until
  the dispatcher VERIFY (L9) lands.
- Precache+hook `events/coldcock.sc` — client handler content VERIFY (L12) before
  writing the cl_dll side; server-side send is safe to land first (original client
  already handles it — that's the drop-in direction that must work).
- PlayerKilled: `glast_coldcock`/`glast_closecombat` branches (" Stock"/"Kung Fu" +
  `#TS_Coldcock` TSHudMessageAll) — byte-faithful strings.

**S7 — knife/katana split (PLAYTEST: the whole blade sandbox). Largest step; land as
three commits:**
- **S7a `KatanaThrow` + `SecondaryAttack`:** add `CTSGun::KatanaThrow()` per §2; add
  `CTSGun::SecondaryAttack()` with the original dispatch (incl. item+0x24 writes) and
  make ItemPostFrame call it (absorbs ts_gun.cpp:1870–1930). Stop routing TSFT_KATANA
  through ThrowKnife. CreateKnife contract change: `(player, pos, vel, int knifeType)`,
  damage inside CreateKnife (internals VERIFY L8 — until then keep our damage value
  computed inside CreateKnife from the same table, tagged), orientation moved to the
  throwers (remove from CTSKnife::Spawn to avoid doubling).
- **S7b `KatanaFire`:** write fresh per the 5-stance table; katana leaves MeleeAttack.
  Protocol: katana.sc every swing (client hook against original handler RVA 0x44230 —
  VERIFY L12); drop server whoosh/SendWeaponAnim for katana; wire VictimKatanaHold +
  bloody blade + STOP lock + whiff writes. Replicate the gun-pev FL_DUCKING slip as-is
  (flagged, L4).
- **S7c `KnifeFire`:** rewrite MeleeAttack's knife half + ThrowKnife's knife half into
  one `KnifeFire()` on item+0x24; DELETE `MeleeAttack`, `ThrowKnife`, `m_slashStep`.
  PrimaryAttack: 3→KnifeFire, 10→KatanaFire; fire-type 11 route ONLY after the
  player+0x814&1 gate is verified (L3) — until then leave 11 unrouted with a flag.
  knife.sc wire format change (every slash, FEV_NOTHOST, weaponType/bloody) — client
  side simultaneously.

**S8 — CTSKnife settle/pickup (PLAYTEST: throw→bounce→settle→pickup loop).**
Files: `dlls/ts_knife.cpp`.
- Write `WaitTouch` fresh (~10 lines, keep MOVETYPE_BOUNCE); rewire BounceTouch settle
  tail (SetTouch(WaitTouch), SetThink(WaitThink), nextthink+0.1, gravity 0.75,
  m_bInFlight=false, m_flExpire=time+10; per-branch CHAN_BODY pitch-100 knife_hit —
  verify channel/pitch per branch while there). DELETE StartSettle/SettleThink/
  BecomePickup/PickupTouch/Classify; inline StickAt.
- Fix `Use` to the original gates (in-flight, expire→silent remove, katana→KungFu()
  helper, PickUpWeapon-failure keeps knife, slowed-array removal — our slowmo array
  equivalent VERIFY). Add members `m_bInFlight` (+0x90), katana bool (+0x91); store the
  6/7/8 type code (+0xa0) or document mapping. Curate into members_re.tsv.
- Accepted behavior changes: settled knife expires ~10 s, no glow; katana recoverable by
  KungFu players (live-verify L13 first — user memory says otherwise).

**S9 — CCombatIdle (low risk; PLAYTEST: bare-hands idle anim visually).**
- Add `CCombatIdle()` + WeaponIdle leading branch. Predicted-weapon check (L14): if the
  client shared code runs it, mirror in both libs per the predicted-weapon memory.

**S10 — audit/coverage close-out.** `audit.py CBasePlayer/CTSGun/CTSKnife` shows the 9
MISSING closed and the 10 INVENTED gone; `coverage.py all` confirms the event/user-message
contract (coldcock.sc precache now present; KFuPower/TSMBlur unchanged names/sizes).
Update members_re.tsv; ledger entry.

### Playtest-gated vs behavior-preserving summary
- Behavior-preserving (build+nm check only): S0, S3, S4, S10.
- Playtest-gated: S1 (block/bar/HUD), S2+S5 (kung-fu, root feel, GoWheel backkick),
  S6 (coldcock + kill feed), S7a/b/c (blades — biggest), S8 (knife lifecycle), S9 (idle).
- Wire-visible (test against ORIGINAL client if possible — that's the swap contract):
  TSMBlur send, coldcock.sc/katana.sc/knife.sc/draw.sc event formats, KFuPower locus,
  WeaponInfo melee clip byte, death-notice strings, iuser4 STOP bit.

---

## 4. LIVE-TRACE LIST (do NOT guess — scripts/live.py on original hl.exe, or gdb on ours)

L1. **Melee-bar REGEN site + rate** — no caller passes a negative amount; only
    UpdateMeleeBar writes +0x774. `live.py watch` player+0x774 while idle (bar depleted)
    in the original; also check whether the CLIENT regens the bar locally between
    KFuPower messages (affects HUD smoothing design). Blocks finalizing S1's regen.
L2. **Block-window decay** (+0x7a8/+0x7ac/+0x7b0 stored as VALUE 0.5) — find the
    countdown/expiry site (PreThink/ItemPostFrame decompile or `live.py watch`
    player+0x7a8 after raising a block). Blocks converting `m_flKungFuBlock` from our
    absolute-time semantics.
L3. **player+0x814 & 1** (fire-type-11 KnifeFire gate) — trace with a flag carried.
L4. **Katana duck-slash FL_DUCKING on the GUN's pev** — live check: katana swing while
    ducked without holding IN_DUCK mid-swing; expect the duck branch NOT to trigger
    (confirms the source slip; we then replicate as-is).
L5. **ColdCockAttack TraceAttack bitsDamageType** — raw disasm @0x86610 or live read;
    likely DMG_CLUB(+marker?) but obscured in decompile.
L6. **Client .sc handlers** — `dc.sh client` on: coldcock.sc handler, katana.sc
    (RVA 0x44230), knife.sc, closecombat.sc; confirms viewmodel anim/sound split and
    whether any miss/whoosh sounds live client-side (justifies deleting our server-side
    invented sounds).
L7. **CCombatAttack per-move rootFlag** — which of the ~13 kung-fu moves set the
    BlockPlayer self-lock bool (per-move dispatch re-read upstream in the huge function,
    or live break at BlockPlayer and enumerate moves).
L8. **CreateKnife internal damage** — reads player+0x180 + vtable calls knife+0x14 /
    player+0x10; `re.sh` CreateKnife decompile + live break to read the computed value.
L9. **AlternateAttack cooldown dispatcher** — who enforces the coldcock cadence (spam
    +alt2 with `live.py break` on mp.dll ColdCock via binmap).
L10. **Exact drain constants** — kick `UpdateMeleeBar(dmg*const)` @0x9c70c and the
    backkick 22.5 product @0x9cdb8: read the rodata doubles (file offset = Ghidra
    addr − 0x10000) — static, but must be read, not assumed.
L11. **VictimKungfuHold counter-swap** — which side's bar drains after the role swap
    (decompile ambiguity; live trace a counter situation).
L12. **EMIT_SOUND_DYN trailing float** — decompile the wrapper @0xd0a8c (formula vs
    pev->fuser1) so our equivalent (TSSlowPitch?) is provably the same; every melee site
    passes 0.5.
L13. **Katana +USE pickup by KungFu players** — binary is unambiguous but contradicts
    the user's recollection; confirm in the live original before shipping S8's change.
L14. **CCombatIdle client prediction** — does client.dll's shared weapon path run it
    (idle anim predicted vs server-sent)? Affects S9 placement (both libs).
L15. **DropWeapon(-2, dmg) internals + +0x71c reset** — the disarm roll and immunity
    re-arm (re.sh DropWeapon + live break while getting kicked).
L16. **status+0x28 record** (+0x28 short = draw anim) — which struct the knife redraw
    reads.
L17. **player+0x4f8 / +0x4f4 consumers** — likely the lunge/anim systems; needed before
    naming them properly.
L18. **PlayerKilled glast_coldcock full conditions** — the KO handling around +0x728/
    +0x724 in @0x7ac30.
L19. **CTSKnife +0x94/+0x98/+0x9c** — unresolved members (`re.sh CTSKnife` when the
    Ghidra project is free).
L20. **CheckHull tail** — the vtable-slot-46 check + hull box re-derivation after a hull
    hit (own decompile pass before S3 finalizes).

---

## 5. BLOCKERS / GAPS

- **`dossier-ctsstunt.md` missing** from the scratchpad: the whiff-lunge PreThink
  CONSUMER (what applies +0x7a0 as a lunge) and the kick fragment @0x9c70c ownership are
  stunt-cluster scope. S2/S5 can land with the lunge writes + a stubbed consumer, but
  the GoWheel backkick combo isn't fully verifiable until the stunt side is reconciled.
- **L1 (regen) and L2 (window decay)** are the only uncertainties that gate the SHAPE of
  code being written (not just constants) — do these two live traces before S1/S3 are
  declared done; everything else can land flagged.
- No build/tooling blockers: all changes are ordinary dlls/ + cl_dll/ edits through
  `scripts/build.sh`; the Ghidra `re` project may be GUI-locked for the re.sh follow-ups
  (mcp-up.sh path documented in CLAUDE.md §4a.3).
