# mod-ts change ledger

Running log of reconstruction progress. Newest entries on top. Keep factual:
what changed, how verified, what's next. See `CLAUDE.md` for mission/method.

---

## 2026-06-21 (cont. 29) — CTSShrapnel (grenade-blast fragments)

**Done — `CTSShrapnel` in `dlls/ts_grenade.cpp`.** The grenade explosion (cont.20 CM61Grenade)
now throws damaging fragments, matching the .so (a grenade Detonate spawns CTSShrapnel).

RE (from the .so): `CTSShrapnel : CTSPhysicObject` (Create @0x7bf9c, Spawn @0x7c0a0 with
`models/shrapnel.mdl` + `movetype=10` MOVETYPE_BOUNCE + `solid=2`, BounceTouch @0x7c200
TraceAttacks on contact, WaitThink @0x7c678 lifetime). FUNCTIONAL reconstruction on the stock SDK
(no CTSPhysicObject):
- `CTSShrapnel : CBaseEntity` (ts_shrapnel): MOVETYPE_BOUNCE + SOLID_BBOX; BounceTouch TraceAttacks
  a damageable hit (14 dmg DMG_BULLET, then remove) or bounces off the world ≤2× before expiring;
  1.5s FadeThink. `Create(owner, origin, velocity)`.
- `CM61Grenade::M61Tumble`/`M61Detonate` replace the stock TumbleThink→Detonate: tumble in flight,
  then at the fuse spawn 10 random-direction fragments + `CGrenade::Detonate` (stock area blast).
- `models/shrapnel.mdl` precached at map load (PrecacheAllTSWeapons; valve fallback model).

**Verified RUNTIME (rcbot, ts_wyndham, temp probe reverted):** a thrown grenade detonated and
spawned the shrapnel burst (fragments bounce, several alive post-blast), no crash; clean
regression (factory `ts_shrapnel` exported).

**SLICE:** no CTSPhysicObject bounce, fixed 10 fragments/14 dmg, no fragment trail/sound.

---

## 2026-06-21 (cont. 28) — Death handling: CTSDeadCorpse + drop-arsenal-on-death

**Done — `dlls/ts_corpse.cpp` + a `CTSGameRules::PlayerKilled` hook.** Completes the on-death side
left open in cont.27 and adds the TS death body.

RE (from the .so): `CTSDeadCorpse` (Spawn @0x7040c, WaitThink @0x70460) + `TSCreateDeadCorpse`
(@0x60234). The corpse is `CBaseEntity::Create("…")` copying the player's origin/angles (rep movsl)
+ pev->model (+0xb8), modelindex (+0xb4), sequence/frame (+0x130/+0x1a8), `movetype = 6`
(MOVETYPE_TOSS @+0x108); WaitThink (0.5s) ramps renderamt down by `((fadeEnd-time)/(lifetime*0.667))*255`
then UTIL_Remove. Functional reconstruction:
- `CTSDeadCorpse : CBaseAnimating` (ts_corpse): MOVETYPE_TOSS + SOLID_NOT + kRenderTransTexture;
  `FadeThink` fades renderamt then removes (15s lifetime). `TSCreateDeadCorpse(player, lifetime)`
  copies model/origin/angles/sequence/frame(framerate 0 = frozen)/body/skin/colormap.
- `TSDropWeaponsOnDeath(player)` (ts_gun.cpp): scatter the whole arsenal into the world as
  CTSWorldGuns (random toss) — the on-death drop noted as SLICE in cont.27.
- `CTSGameRules::PlayerKilled` override: `CHalfLifeMultiplay::PlayerKilled` (scoring/death-notice),
  then drop weapons + spawn the corpse.

**Verified RUNTIME (rcbot, ts_wyndham, temp probe reverted):** a bot given beretta+uzi then
force-killed (suicide TakeDamage) produced exactly **1 ts_corpse + 2 ts_worldgun**; no crash; clean
regression (factory `ts_corpse` exported).

**SLICE:** corpse copies the live pose at PlayerKilled time (no dedicated death-anim capture);
melee/no-world-model weapons aren't dropped; no gib/blood. The on-death weapon drop now means the
CTSWorldGuns are reachable in normal play (kill someone → their guns drop, press USE to grab).

---

## 2026-06-21 (cont. 27) — CTSWorldGun (player-dropped weapons: drop + USE-pickup)

**Done — `dlls/ts_worldgun.cpp`: the dropped-weapon-in-the-world entity.** Completes the weapon
lifecycle (map pickups via CTSGroundWeapon cont.11; player drops here).

RE (from the .so): `CTSWorldGun : CTSPhysicObject` carries a tsweapon_t (+0x88) + the full
s_weapon_status (14 ints); created by `CBasePlayer::DropWeapon`/`ThrowWeapon` -> `CTSGun::DropWeapon`
(removes from the arsenal) -> `CTSWorldGun::CreateWeapon`. `GetWeapon` (@0xa1198) -> `PickUpWeapon`;
`WaitTouch` (@0xa1250) gates pickup on `pev->button & IN_USE` (0x20) (or auto for the flag, type 0x1d).
FUNCTIONAL reconstruction on the stock SDK (CM61Grenade-style; no CTSPhysicObject):
- `CTSWorldGun : CBaseEntity` carrying type + loaded clip. Spawn: w_model + MOVETYPE_TOSS;
  FallThink settles on FL_ONGROUND -> SOLID_TRIGGER + WaitTouch. WaitTouch: living player pressing
  IN_USE -> TSPickUpWeapon + UTIL_Remove.
- `TSCreateWorldGun(type, clip, origin, vel)` = the drop side.
- `CTSGun::RemoveActiveWeapon(type, clip)` (@DropWeapon 0x8094c): shift the active slot out of the
  arsenal (requires >=2 weapons — no unarmed state, SLICE), wield the next; free `TSDropActiveWeapon`/
  `TSPickUpWeapon` (the latter re-adds via GiveTSWeapon + restores the carried clip).
- `dropweapon` ClientCommand drops the active weapon forward (250u + player velocity).

**Verified RUNTIME (rcbot, ts_wyndham, temp probe reverted):** a bot holding {beretta, uzi} dropped
the uzi (type 3, clip 32; arsenal 2->1) -> ts_worldgun spawned + settled (SOLID_TRIGGER); a forced
USE-touch handed the uzi back (holds weapon_tsgun, world gun removed). No crash; clean regression
(factory `ts_worldgun` exported).

**SLICE:** no CTSPhysicObject bounce, carries type+clip only (the shared caliber reserve stays with
the dropper), no on-death auto-drop, no flag auto-pickup, no throw arc.

---

## 2026-06-21 (cont. 26) — CTSGameRules::PutPowerUp (dynamic deathmatch powerup spawning)

**Done — the dynamic powerup spawner.** Un-SLICEs the `CTSGameRules::PutPowerUp` (@0x9797c)
niche left in cont.13. DM maps now spawn powerups dynamically even without ts_powerup placements.
- `CTSGameRules::PutPowerUp(type)`: collect `info_player_deathmatch` spots (cap 32), pick a
  random one, skip if a `ts_powerup` is already within 128u, else create a powerup there via a
  new `TSCreatePowerUpAt(type, origin)` free helper (ts_powerup.cpp; wraps
  `CTSPowerUp::CreatePowerUp` + UTIL_SetOrigin so it drops from the spot).
- `CTSGameRules::Think` override (the rules object now runs TS think): `CHalfLifeMultiplay::Think()`
  then, in DM only, roll `GetRandomPowerup(0x1ff)` every ~20s and PutPowerUp it (1s retry on
  fail). Mirrors the .so Think powerup block (`if mode<2 … every 20s`). DM gate = `!IsTeamplay()`
  AND no `ts_objective_manager` present (a functional proxy — the real TS game-mode member @+0
  isn't reconstructed; objective maps place their own pickups).

**Verified RUNTIME (temp probe reverted):** ts_wyndham (DM) spawned a random powerup every 20s
(t=1 kungfu, t=21 slowmo, t=41 kungfu); tm_avana (objective map) correctly skipped (0 spawns).
No crash; clean regression.

---

## 2026-06-21 (cont. 25) — Objective subsystem (ts_hack/wingiver/manager/ptr) — team-mode maps

**Done — `dlls/ts_objectives.cpp`: the 4 objective entity types the `tm_` maps place.** Loading
a team-mode map (tm_avana) showed 7 Can't-inits: ts_hack(2), ts_objective_manager(1),
ts_objective_ptr(1), ts_wingiver(3). All cleared.

RE (from the .so): these are point/brush entities tied into a CTSGameRules team subsystem
(`TeamWin` @0x68a08 1564 bytes, `ShowObjectiveOnHud`, `AddObjectiveInList`) that ISN'T built —
the interaction "who is hacking" + the win scoring/round-restart are gamerules-driven. Built a
FUNCTIONAL standalone reconstruction instead:
- `CTSObjective` (base @0x8c2b0): `attackerteam` keyvalue (+0x68, -1=any) + CanInteract(team)/
  Freeze(bool). (.so Freeze also calls ShowObjectiveOnHud — SLICE.)
- `CTSObjectiveManager` (@0x8b7bc, ts_objective_manager): objptargeta..h / messagea..d /
  targeton{timeleft,specdead,mercdead} keyvalues; `Reset()` HUDs the messages + FireTargets the
  8 objptargets (matches @Reset 0x8b850).
- `CTSObjectivePointer` (@0x8bb44, ts_objective_ptr): per-team objmsg{spec,merc,team3,team4,all}
  (the visible HUD arrow is cl_dll — server stub).
- `CTSHack` (@0x8cccc, ts_hack): brush; SET_MODEL + SOLID_TRIGGER; an attacker-team living player
  in the volume (HackTouch) drives a `hackingtime` countdown (WaitThink, 1s cadence like the .so);
  on 0 → fire pev->target + HUD pev->message + Freeze. SLICE: faithful start-hack is gamerules-driven.
- `CTSWinGiver` (@0x8bf10, ts_wingiver): Use'd `callsnumber` times → declares `winnerteam` (HUD).
  SLICE: faithful Use → CTSGameRules::TeamWin(team, victory/defeat cash, …) + round restart.

**Verified RUNTIME (tm_avana, temp probe reverted):** 2 hack / 3 wingiver / 1 manager / 1 ptr
spawn (0 Can't init, was 7); keyvalues parse (hack time=10 attackerteam=-1, wingiver team=2
calls 1); a forced-hacker hack counted 10→0 and completed (done=1); wingiver Use → declares the
winner (done=1). tm_corporate also 0 Can't init. No crash; clean.

**Next:** the real payoff is the **CTSGameRules team subsystem** (TeamWin/team assignment/scores/
RestartRound) — it turns these standalone entities into an actual scored team game mode and
unblocks the rest of the tm_ maps (ts_briefcase/ts_bomb/ts_flag CTF). Still also: the buy menu,
CTSSharedEmitter (cl_dll), the visible slow-motion (cl_dll).

---

## 2026-06-21 (cont. 24) — Shotgun fidelity: 8-pellet count + mossberg/sawedoff fire-types

**Done — closed the two known weapon data gaps.**
- s_weapon_info gains `iPellets` (bullets-per-shot, `s_weapon_info+0x24`): **8 for the five
  shotguns** (benelli/usas/spas/mossberg/sawed-off), 1 otherwise (extracted from each
  Write_<weapon>). `StandardFire` uses it instead of the hardcoded `(ft==PUMP)?6:1` guess —
  which was both wrong (8, not 6) and missed usas/sawedoff (not pump fire-type but 8 pellets).
- Fixed two fire-types the OLD extract_weapons.py mis-read as 0 (reading the Write_ funcs
  directly gives the truth): **mossberg 0→1 (pump), sawedoff 0→8 (semi, twin-barrel)**.

**Verified RUNTIME (rcbot, ts_wyndham, temp probe reverted):** mossberg fireType=1 pellets=8,
sawedoff fireType=8 pellets=8, usas fireType=6 pellets=8, beretta control pellets=1; all fire
without crash. Clean regression.

**Note on the bigger picture:** the weapon engine is now functionally complete (arsenal,
all fire-types, akimbo, fire-mode/burst, per-caliber ammo, correct pellets). The remaining TS
gameplay systems are all LARGE/interconnected or client-entangled: the buy menu (cash economy
@BuyWeapon checks CashBased()/weapon price s_weapon_info+0xd8/player cash +0x540 + a client
menu UI), the team/objective subsystem (CTSGameRules::TeamWin @0x68a08 is 1564 bytes atop
SetTeamList/UpdateTeamScores/RestartRound/InitTeamScore — all the ts_wingiver/ts_hack/
ts_briefcase entities live in tm_ maps needing it), the networked particles (CTSSharedEmitter,
needs cl_dll), and the visible slow-motion time-dilation (needs cl_dll). Next agent: pick the
team foundation (biggest gameplay unlock, server-side) or the buy menu server half.

---

## 2026-06-21 (cont. 23) — Per-caliber ammo (CTSGun::AddAmmo + CTSGroundAmmo wiring)

**Done — the faithful per-caliber ammo system.** Un-SLICEs the per-slot generic reserve +
the CTSGroundAmmo 9mm-only refill; ammo is now pooled per caliber and shared across same-
caliber weapons (a single beretta + akimbo berettas draw from one 9mm pool).

RE findings (from the .so):
- Each weapon names a CALIBER byte at `s_weapon_info+0x8` (extracted from each Write_<weapon>):
  0 none(melee/grenade), 1 9x19, 2 .45, 3 SHELLS, 4 5.56, 5 .50AE, 6 7.62, 7 5.7, 8 50BMG,
  9 10mm, 10 .22, 11 .454, 12 .32, 13 .45LC. The gun holds 16 per-caliber reserves at `+0xd4`.
- `compile_ammo_info` (@0x6f51c) builds a per-caliber table (stride 0x38): +0x10 = magazine
  size, **+0x20 = max reserve (rounds)** — 9mm 80, .45 90, shells 90, 5.56 150, 7.62 170,
  50BMG 200, … (full table extracted).
- `AddAmmo(caliber,count)` (@0x808ac): `reserve[caliber] = min(reserve+count, max[caliber])`.
- `ReloadWeapon` (@0x84a14): `take = min(clipSize - clip, reserve[caliber])` → reserve is in
  ROUNDS, drawn into the clip. (Settled the rounds-vs-mags question.)
- `CTSGroundAmmo::WaitTouch` (@0xa2f40): loops the 16 calibers calling `AddAmmo(gun, cal,
  m_mags[cal])` — the FGD `magsN` keyvalues add raw rounds to caliber N (the station persists,
  topping up on every touch; real maps set magsN = 3–5).

Functional reconstruction:
- `ts_weapons.{h,cpp}`: s_weapon_info gains `iCaliber` (the +0x8 byte, full table); `kAmmoMax[16]`
  + `TSAmmoMax(caliber)`.
- `ts_gun.cpp` CTSGun: **replaced the per-slot `m_arsenalReserve[]` with per-caliber
  `m_ammoReserve[16]`**. `AddAmmo` method (capped). Spawn/AddWeapon seed the caliber reserve
  (clip*3); the akimbo upgrade adds the 2nd pistol's rounds to the shared pool; Reload draws
  from `m_ammoReserve[caliber]`; MultiplyClips (inf-clip powerup) tops the active caliber.
  Free `TSAddAmmo(player, caliber, count)`.
- `ts_groundammo.cpp` WaitTouch: per-caliber `TSAddAmmo(player, cal, m_mags[cal-1])` (was the
  9mm-only SLICE).

**Verified RUNTIME (rcbot, ts_wyndham, temp probe reverted):** beretta → clip 15 / 9mm reserve
45; emptied clip → Reload drew 15 from the 9mm pool → 15/30; 2nd beretta → akimbo (type 8) +
SHARES the 9mm pool (reserve 30+15=45); +m16 → 5.56 reserve 54, 9mm unchanged (separate pools);
40× AddAmmo(1,3) capped at max 80. Clean regression on ts_supertimor_d3 (6 ground-ammo packs):
0 Can't init, 0 crashes.

**SLICE/deferred:** starting reserve = clip*3 (the faithful buy menu / BuyArmament sets the real
loadout); calibers 12/13 (skorpion/contender) have no ground-ammo magN; no client ammo HUD;
grenade (caliber 0) still uses its own m_iClip count (no caliber reserve).

---

## 2026-06-21 (cont. 22) — Fire-mode system (auto/semi/burst/pump + SwitchFireMode)

**Done — the faithful trigger engine.** Auto/semi/burst/pump is now decided by a per-slot
FIRE-MODE (orthogonal to fire-type, which still picks the fire *function*) — replacing the
old fire-type-based auto/semi approximation.

RE findings (from the .so):
- `PrimaryAttack` (@0x81a20): melee/grenade/katana/flag dispatch by fire-type FIRST (outside
  the mode system); the bullet/akimbo path then gates on the fire-mode byte `m_pActiveItem+2`:
  mode 0 full-auto (held), 1 semi (per-press; dblfire powerup → held), 2 burst (set count on
  press, return — fired by WeaponIdle), 3 pump (held), 4 akimbo-alt (per-press), 5 akimbo-both
  (held). Burst count = `(weaponType != 0x17) + 2` → **3 rounds, 2 for the MAC-10 (0x17)**.
- `WeaponIdle` (@0x854dc): while firemode==2 and burst count>0 and the idle timer elapsed →
  `Fire()` + count-- (this completes a tapped burst after the trigger is released).
- `StandardFire` (@0x824a4): burst rate = 0.03 between rounds, 0.2 after the last.
- `SwitchFireMode` (@0x8169c): cycle the mode byte to the next bit set in the weapon's mode
  MASK at `s_weapon_info+0xc`, modes 0..5 wrapping. Default mode (`TakeWeapon`): auto if
  offered, else pump, else burst, else semi. Masks extracted from each `Write_<weapon>`
  (`movl $imm,0xc(%eax)`): e.g. beretta=2 (semi), uzi=3 (auto+semi), m16=6 (semi+burst,
  defaults burst), mp5sd/mp5k/mac10=7 (auto+semi+burst), akimbos 18/33.

Functional reconstruction:
- `ts_weapons.{h,cpp}`: s_weapon_info gains `iFireModes` (the +0xc mask, full table extracted);
  `enum tsfiremode_mode_t`; `TSDefaultFireMode(mask)`.
- `ts_gun.cpp` CTSGun: per-slot `m_arsenalMode[]` + active `m_fireMode` + `m_burstCount`
  (saved/loaded by SelectSlot; defaulted in Spawn/AddWeapon/akimbo-upgrade). PrimaryAttack
  rewritten to gate by m_fireMode → `Fire()` (dispatches by fire-type: AkimboFire or the new
  `StandardFire`, factored from the old inline bullet code). WeaponIdle drives the burst.
  `SwitchFireMode()` + free `TSSwitchFireMode` + the `tsfiremode` ClientCommand.

**Verified RUNTIME (rcbot, ts_wyndham, temp probe reverted):** default modes correct for all
masks (beretta→1 semi, uzi→0 auto, m16→2 burst, mp5sd→0 auto); `SwitchFireMode` cycles only
the available modes (beretta stays 1; uzi 1↔0; m16 1↔2; mp5sd 1→2→0); m16 burst fired exactly
3 rounds per tap (clip 18→15, burst 3→0). No crash; clean regression (only the known
`ts_emitter`/`ts_particle` Can't-inits remain).

**SLICE/deferred:** burst sustain depends on the SDK ItemPostFrame/WeaponIdle cadence (a held
burst fires via PrimaryAttack, a tapped one finishes via WeaponIdle); no client-side fire-mode
HUD indicator (the .so writes `mode+1` to player+0x49a per weapon); pump (mode 3) doesn't yet
play a pump-action animation; the dblfire→semi-auto-continuous interaction is approximated.

---

## 2026-06-21 (cont. 21) — Akimbo (fire-types 4/5): dual-wield upgrade + AkimboFire

**Done — akimbo dual-wielding.** Un-SLICEs fire-types 4 (combinable) & 5 (gold-only),
the last unhandled PrimaryAttack fire-type.

RE findings (from the .so):
- `IsWeaponAkimbo(t)` = `get_weapon_info(t)->iFireType==4`; `IsOnlyAkimbo(t)` = `==5`
  (objdump @0x7019c/0x701d4 — both just compare the +0x348 fire-type byte).
- Akimbo PAIRING fields in s_weapon_info: **+0xc8** = the akimbo variant a single pistol
  upgrades into (-1 if none); **+0xcc** = the single base of an akimbo (= self for
  non-akimbo). Read from each `Write_<weapon>` (`movl $imm,0xc8/0xcc(%eax)`; the table is
  .bss so file reads are garbage — must read the Write_ funcs). Pairs: 2↔8 (beretta),
  3↔16 (uzi), 9↔10 (mk23), 14↔30 (fnh), 17↔37 (skorpion); gold(21) is ft-5, no single.
- `TakeWeapon` (@0x80a04): owning the single + picking up an identical 2nd → `local_12`
  becomes the +0xc8 akimbo → combines in place (clip = sum of both barrels). `AkimboFire`
  (@0x829c0) splits the clip into left/right barrels (m_pActiveItem+0x1c/+0x20), alternates
  the firing barrel (one per IN_ATTACK/IN_ATTACK2) from a ±5u right-vector muzzle offset;
  rate = flFireRate, or fixed 0.1 in alternating fire-mode.

Functional reconstruction (on the cont.18 single-clip arsenal):
- `ts_weapons.{h,cpp}`: s_weapon_info gains `iAkimboType`/`iSingleType`; set in
  compile_weapons_info. Added `TSFT_AKIMBO_ONLY=5`.
- `CTSGun::AddWeapon`: if you already hold the single + it has an akimbo upgrade + don't
  hold the akimbo → upgrade the slot in place (SelectSlot first so m_iClip is live, then
  type→akimbo, clip = single's rounds + a 2nd pistol capped at the doubled clip, reserve
  += a 2nd pistol, re-Deploy). Slot count stays the same (not a new slot).
- `CTSGun::AkimboFire(wi)` (dispatched for ft 4/5): full-auto-feel (fires while held — no
  IN_ATTACK2 plumbed), `m_akimboSide ^= 1` each shot → ±5u v_right muzzle offset, one
  bullet/clip-- per shot, flSpread/flDamage/flFireRate (+ DoubleFireRate powerup halving).

**Verified RUNTIME (rcbot, ts_wyndham, temp probe reverted):** gave a bot beretta, then a
2nd beretta → upgraded in place to **type 8 (beretta_akimbo), clip 30** (15+15 capped),
reserve 60, arsenal count stayed 1. Held trigger → AkimboFire fired (clip 30→26→16→6, then
reload from reserve → 26, side toggled 0↔1), active item = the gun. No crash; clean
regression (0 crashes; only the known `ts_emitter`/`ts_particle` Can't-init remain).

**SLICE/deferred:** single active clip (not a true L/R barrel split); no IN_ATTACK2
"fire both barrels at once" (firemode 5 path); no per-barrel reload; the akimbo's separate
view-model akimbo animation events are not wired. Fire-mode/burst (`SwitchFireMode`) and the
buy menu remain the next weapon-engine items.

---

## 2026-06-21 (cont. 20) — CM61Grenade (grenade fire-type 2 throws a grenade)

**Done — `dlls/ts_grenade.cpp` CM61Grenade + the grenade fire-type.** Un-SLICEs
fire-type 2 (was falling back to bullets) + makes the grenade powerup useful.
- The faithful CM61Grenade is `CM61Grenade : CTSGrenade : CTSPhysicObject` (TS physics
  base classes, not built) with event-driven creation + a custom WaitThink (traces 64u
  ahead for near-impact detonation). FUNCTIONAL reconstruction onto the stock SDK
  `CGrenade` instead (which already does arc/bounce/timed-detonate): `CM61Grenade::Throw`
  replicates `CGrenade::ShootTimed` but with `w_m61.mdl` (so no models/grenade.mdl
  precache dependency) — MOVETYPE_BOUNCE, `CGrenade::BounceTouch`, `TumbleThink`→
  `Detonate` at dmgtime, gravity 0.5/friction 0.8. Free `TSThrowM61(pevOwner, src, vel,
  fuse, dmg)`.
- `CTSGun::PrimaryAttack` dispatches TSFT_GRENADE (2) → new `GrenadeThrow(wi)`: one throw
  per press, decrement clip, toss from gun-pos+16 forward at 800u + player velocity, 4s
  fuse, damage = table flDamage*4 (~120). (m61 table dmg 30 is the per-pickup value; the
  explosion is area damage.)

**Verified RUNTIME (rcbot, ts_wyndham, temp probe reverted):** a bot with the M61 threw
100 grenades over 30s (valid entities, clip decrementing); TumbleThink switches to
Detonate at dmgtime (model-independent: `if (dmgtime <= time) SetThink(Detonate)`), so
they arc/bounce/explode. No crash; clean regression.

**SLICE/deferred:** no CTSPhysicObject physics, no impact-trace early detonation, no
throw event/animation; akimbo (fire-types 4/5) still falls back to bullets.

---

## 2026-06-21 (cont. 19) — Powerup→weapon wiring: MultiplyClips + DoubleFireRate

**Done — two now-easy powerup effects, faithful on the cont.18 arsenal.** Completes
the powerup→weapon connection.
- `CTSGun::MultiplyClips()` (@0x71308, the infinite-clip powerup): doubles the active
  arsenal slot's reserve (>= a full resupply, capped ~6 clips). Free `TSMultiplyClips
  (player)` wraps it (GetPlayerTSGun). `PlayerAddPowerUp`'s inf-clip branch now calls it
  (was a SLICE clip-refill).
- `CTSGun::PrimaryAttack`: `if (DoubleFireRate(m_pPlayer)) flRate *= 0.5` — the
  double-firerate powerup now halves the fire interval (the accessor already flips true
  while the dblfire powerup is active; just wired into the rate).

**Verified RUNTIME (rcbot, ts_wyndham, temp probe reverted):** beretta inf-clip reserve
45→90 (doubled); dblfire baseRate 0.100 → rate 0.050 (halved), 105 fires at the doubled
rate. No crash; clean regression.

---

## 2026-06-21 (cont. 18) — CTSGun weapon arsenal (the items-array, modern reconstruction)

**Done — the player's weapon_tsgun now holds a multi-weapon ARSENAL.** A clean
MODERN reconstruction of the original items-array (the faithful one is a 0x38-byte
s_weapon_status items[] @CTSGun+0x114 with m_pActiveItem@0xcc / m_pActiveInfo@0xd0 +
weapon/ammo-group masks + akimbo pairing — heavy byte layout, reconstructed from
intent, not transcribed).
- `dlls/ts_gun.cpp` CTSGun: added parallel arrays `m_arsenalType/Clip/Reserve[12]` +
  `m_arsenalCount` + `m_active`. `m_weaponType` now tracks the active slot (so the
  fire-type dispatch + Deploy are unchanged).
- `FindSlot(type)` / `AddWeapon(type)` (@AddItem__6CTSGun: add a slot or resupply,
  then wield) / `SelectSlot(slot)` (@SelectWeapon__6CTSGun: save the wielded clip,
  load the new slot's, re-Deploy if held).
- `Spawn` seeds the arsenal with the placed type (clip full, reserve = 3 clips).
- `PrimaryAttack` unchanged (operates on m_iClip = the active slot's clip). `Reload`
  now refills m_iClip from the active slot's own reserve (not DefaultReload).
- `GiveTSWeapon(player, type)`: if the player already holds weapon_tsgun, AddWeapon
  to it; else create + pick up. (Helper `GetPlayerTSGun` walks m_rgpPlayerItems.)
- `CTSGroundWeapon::GetWeapon`: dropped the "one TS weapon at a time" guard — picking
  up a ground weapon now ADDS it to the arsenal.
- `tsnextweapon` ClientCommand → `TSCycleWeapon` cycles the active slot.

**Verified RUNTIME (rcbot, ts_wyndham, temp probe reverted):** a bot built a 3-weapon
arsenal (beretta→+uzi→+knife), `tsnextweapon` cycled 0→1→2→0 through all three, each
deploying its own view model with its own clip/reserve (beretta 15/45, uzi 32/96,
knife 18/54). PER-SLOT CLIP PERSISTS: the bot fired the uzi (clip 32→30) and the count
survived switching away+back. No crash; clean regression (0 Can't init, 0 crashes).

**SLICE/deferred:** no akimbo pairing (single active slot — the faithful engine wields
two for fire-types 4/5), no weapon/ammo-groups, no fire-mode/burst state, no buy menu
(`BuyWeapon`/`BuyArmament`); the per-caliber ammo reserve is still per-slot generic (not
wired to CTSGroundAmmo's calibers). The grenade/akimbo PrimaryAttack fire-types still
fall back to bullets.

---

## 2026-06-21 (cont. 17) — CTSGroundAmmo (ts_groundammo refill pack, 16 placements)

**Done — `dlls/ts_groundammo.cpp` CTSGroundAmmo : CBaseEntity.** A map-placed ammo
pack; lifecycle mirrors CTSPowerUp (Spawn→Fall→OnGround settle→WaitTouch/WaitThink),
recovered from the .so (Spawn/OnGround tails truncated by Ghidra noreturn at
UTIL_SetOrigin → read via objdump):
- Spawn: model `models/ammopack.mdl`; movetype = MOVETYPE_FLY (wall, spawnflag 16) or
  MOVETYPE_TOSS (floor); solid SOLID_BBOX; SetThink(Fall) + nextthink time+0.1.
- Fall: settle on FL_ONGROUND → OnGround (or PutOnWall for the wall variant) + drop
  sound. OnGround: SOLID_TRIGGER, MOVETYPE_FLY (stop), clear EF_NODRAW + EF_DIMLIGHT,
  SetTouch(WaitTouch)+SetThink(WaitThink). WaitThink idles (nextthink+1).
- KeyValue: wduration → m_respawnTime; mags1..mags11 (parse the index off "mags") →
  m_mags[0..10] (per-caliber magazine counts: 9x19/.45/SHELLS/5.56/.50AE/7.62/5.7/
  50BMG/10mm/.22/.454).
- WaitTouch: a visible pack refills a touching ALIVE player who holds a TS gun. The
  pack PERSISTS (tops players up when they have room; not consumed on pickup — matches
  the .so, which has no hide/remove in WaitTouch).
- PutOnWall: SLICE = settle-in-place stub.

**SLICE:** faithful WaitTouch loops the 16 ammo-type slots calling `CTSGun::AddAmmo(
caliber, mags)` on the active gun (the per-caliber reserve = part of the unbuilt
weapon items-array; gate is m_pActiveItem id == 0x10). Here it sums the mags and tops
up the held weapon's **9mm** reserve via `CBasePlayer::GiveAmmo(n, "9mm", _9MM_MAX_CARRY)`.

**Verified RUNTIME (rcbot bots, ts_supertimor_d3 = 6 placements), temp probe (reverted):**
6 settled (`Can't init ts_groundammo` gone); a weapon_tsgun-holding bot touched a pack
(14 mags) → GiveAmmo topped up 9mm by 2 then returned -1 (full) on repeat = correct
capped refill-station behaviour. No crash (gdb -batch, no CTSGroundAmmo in backtrace).
Final clean regression boot: 0 `Can't init`, 0 crashes.

---

## 2026-06-21 (cont. 16) — CTSGun fire-type dispatch (semi/auto/shotgun/melee)

**Done — weapons now fire faithfully by fire-type.** Un-SLICEs "everything is identical
semi-auto bullets".
- `scripts/extract_weapons.py` extended: also pulls per-weapon fire-rate (0x18), base
  spread (0x28), recoil (0x34). Regenerated the table.
- `ts_weapons.{h,cpp}`: s_weapon_info gains flFireRate/flSpread/flRecoil; added
  `enum tsfiretype_t` (0/8 semi, 1 pump, 2 grenade, 3 knife, 4/5 akimbo, 6 auto-shotgun/
  rifle, 7 sniper, 9 SMG-auto, 10 katana, 11 flag).
- `CTSGun::PrimaryAttack` dispatches by iFireType: melee (3/10) → new MeleeAttack()
  hull-trace (knife 48u/katana 64u) + TraceAttack(DMG_CLUB), no bullets; full-auto (9/6)
  fires while held; semi (0/8/7/1 + SLICE akimbo/grenade) one shot per press
  (m_afButtonPressed); pump shotgun (1) → 6 pellets. Uses real flSpread + flFireRate.
- SLICE: grenade(2)/akimbo(4/5) fall back to bullets (need throw entity / items-array,
  where the faithful fire funcs read fire-mode@m_pActiveItem+2 / burst@+8).

**Verified (rcbot, ts_wyndham, probe reverted):** beretta ft=8 SEMI 43 fires (one/press)
vs uzi ft=9 AUTO 201 (held); benellim3 ft=1 6-pellet; knife/katana hull-trace MELEE hit
func_breakable/worldspawn, no bullets. No crash. **Data gaps:** mossberg(26)/sawedoff(33)
fire-type extracted as 0 (are shotguns); pellet count fixed 6; m61 rate/spread suspect.

---

## 2026-06-21 (cont. 15) — CTSSlowMotion entity + GoSlow/InSlow + GlobalSlowMotion2

**Done — slow-motion STATE layer + `dlls/ts_slowmotion.cpp` (ts_slowmotion brush zone,
11 placements).**
- Slow virtuals `GoSlow`/`InSlow`/`GiveSlowMul` (found via CBasePlayer vtable relocs:
  +0x14 GoSlow, +0x18 InSlow). As **CBasePlayer methods**: GoSlow(scale,bullets) sets
  m_flSlowMotionGo (0x168) + m_flSlowMotionTarget (0x170) + m_bSlowBullets; InSlow() =
  (go!=1||target!=1); both init 1.0.
- `CTSGameRules::GlobalSlowMotion2(scale,bullets)` + SetGlobalSlowMotion: GoSlow every
  player, throttled ~0.5s (members 0x560/0x564/0x568/0x56c). SLICE: no projectiles.
- CTSSlowMotion : CBaseEntity (replaces old draft): Spawn (brush trigger; setup beyond
  classname/model/size INFERRED // VERIFY); KeyValue duration/slowamt; WaitTouch (GoSlow
  toucher, track m_hPlayers[33]); ActivateSlow/DeactivateSlow; WaitThink (0.5s poll);
  CheckPlayersDistance (drop beyond m_flRadius from Center()); Use; Save/Restore.

**Verified (rcbot, ts_wyndham=4):** 4 zones; Use() no crash; GlobalSlowMotion2(0.5,1) →
InSlow()=1 go=0.50; (0,0) → InSlow()=0 go=1.0. No crash.

**SLICE/deferred (visible time-dilation, client-entangled):** actual slow MOVEMENT
(client pmove — no cl_dll); projectile slow; SlowMotionFlow (@0x86bb0, powerup→GoSlow);
InterpolateSlowMotion (@0x8910c); motion-blur HUD; cmd_globalslow; Spawn/m_flRadius inferred.

---

## 2026-06-21 (cont. 14) — Powerup activation state machine (UsePowerUp/UpdatePowerUpState)

**Done — held timed powerups ACTIVATE + expire.** From UsePowerUp (@0x85980),
UpdatePowerUpState (@0x85748), DeactivateSlowMotionFlow (@0x86a9c):
- CBasePlayer::UsePowerUp() (ts_player.cpp): `usepowerup` command. Alive + holding +
  none active → m_iPowerups = m_iHeldPowerup, expire = m_flPowerupTime + time; sends
  gmsgTSPowerUpTime + gmsgTSMotionBlur. UpdatePowerUpState() per-frame in PreThink:
  apply slow fields (slowmo 0.6, slowpause 0.8) / expire + clear.
- `usepowerup` ClientCommand (confirmed via .so @0x9d838). Players +0x175/0x17c/0x180.

**Verified (ts_theatre):** bot held slowmo (dur 3) + UsePowerUp → m_iPowerups=1,
SlowMotion()=1, scale 0.60 for 3s, expired + cleared. No crash. (Note: activation sets
m_flSlowMotionScale, NOT GoSlow — the powerup→GoSlow link is SlowMotionFlow, deferred.)

---

## 2026-06-21 (cont. 13) — CTSPowerUp + PlayerAddPowerUp (the 328-placement pickup)

**`dlls/ts_powerup.cpp` CTSPowerUp : CBaseAnimating (ts_powerup, 328 placements).** Full
lifecycle (Spawn + OnGround TRUNCATED by Ghidra noreturn at UTIL_SetOrigin — real tails
via objdump; Spawn's `nextthink = time+0.1` missed → never settled, caught by "22
spawned 0 pickups"). Spawn→Fall→OnGround(SOLID_TRIGGER,SetTouch(WaitTouch))→WaitTouch
(pickup gate: instant mask 0x1e6 always, timed only if none held; re-roll+respawn).
SetupPowerUp: spawnflags = the random MASK (GetRandomPowerup), pwuptype forces exact.
**PlayerAddPowerUp** (@0x87bc8): instant real (armor/kungfu/lowgrav/health/grenade→
GiveTSWeapon(M61) SLICE/infclip SLICE), timed held. Members 0x72c/0x73c. VERIFIED
(ts_theatre=22): 22 spawned all 8 types, 6 pickups, gate works, no crash.

---

## 2026-06-21 (cont. 12) — Full 38-weapon table + CTSGun by type; gdb debugging

`scripts/extract_weapons.py` → full s_weapon_info table (0x44 name, 0x132 v_model,
0x172 p_model, 0x1f2 w_model, 0x232 anim-ext, 0x2c6 fire snd, 0x04 clip, 0x1c damage,
0x348 fire-type; cont.16 added 0x18/0x28/0x34). CTSGun parameterized by m_weaponType;
ground weapon gives real type. CWorld::Precache → UTIL_PrecacheOtherWeapon at map load.
VERIFIED (ts_wyndham): type 25→v_knife dmg40, type 3→v_uzi dmg22. Two GDB-found crashes
fixed (PRECACHE_EVENT map-load; DeathNotice ItemInfoArray). **Use gdb** for crashes.

---

## 2026-06-21 (cont. 11) — CTSGroundWeapon (weapon pickups) + tsweapon_t fix

tsweapon_t index = (lea offset − 0x7d30)/0x3DC, MATCHES FGD tsweaponid. beretta=2, uzi=3,
knife=25, idx 29=flag. ts_groundweapon.cpp (65 placements): SLICE gives weapon_tsgun;
respawn after wduration. VERIFIED (ts_wyndham).

---

## 2026-06-20 (cont. 10) — CTSGun recon + weapon data layer (vertical-slice start)

**CTSGun is a subsystem:** ONE unified CBasePlayerWeapon (id 16) morphing by tsweapon_t.
**tsweapon_t enum (38, compile_weapons_info @0x6fe28):** 0 melee,1 glock18,2 uzi,
3 uzi_akimbo,4 benellim3,5 m4,6 mp5sd,7 mp5k,8 beretta_akimbo,9 beretta,10 mk23,
11 mk23akimbo,12 usas12,13 desert,14 ak47,15 fnh,16 aug,17 skorpion,18 barrett,19 pdw,
20 spas12,21 gold,22 glock22,23 mac10,24 m61,25 knife,26 mossberg,27 m16,28 ruger,
29 fnhakimbo,30 bull,31 m60,32 sawedoff,33 katana,34 sealknife,35 contender,
36 skorpion_akimbo,37 flag.
FAITHFUL CTSGun engine (items array m_pActiveItem@0xcc/m_pActiveInfo@0xd0/items[]@0x45,
fire-type dispatch s_weapon_info+0x348, akimbo/firemode/zoom) replaces // SLICE.

**Bot test rig (working, reusable):** ~/Containers/hlds/hlds-linux64-test:
metamod_amd64.so (Metamod-R) as gamedll (liblist.gam → it); config.ini=`gamedll
dlls/hl.so`; plugins.ini=`linux addons/metamod/dlls/rcbot_mm_amd64.so`. <rig>/rcbot/*;
min_bots 2. Boot → rcbot bots = real CBasePlayers. Temp StartFrame hook can give/select/
teleport/fire on a bot (gate on a player existing — bots connect ~t>6-8; pulse IN_ATTACK
via m_afButtonPressed to test semi vs auto; slide an entity onto a bot to force touch).
Revert hooks after. Probe output → <rig>/ts/qconsole.log with -condebug.

---

## 2026-06-20 (cont. 9) — TSHudMessageOne + CBasePlayer TS extensions (step 1)

`dlls/ts_utils.{h,cpp}` TSHudMessageOne. CBasePlayer TS member block (player.h):
m_iPowerups (0x724), m_iHeldPowerup (0x728), m_flPowerupTime (0x730), m_iKungFu (0x74e),
m_bLowGravity (0x74f) + accessors. (cont.13 +0x72c/0x73c; cont.14 +0x175/0x17c/0x180;
cont.15 +0x168/0x170 GoSlow/InSlow.)

**The core is a monolith.** Sequencing done: (1) player block+accessors; (2) CTSGun
base; (3) PlayerAddPowerUp + CTSPowerUp; (4) UsePowerUp/UpdatePowerUpState; (5) GoSlow/
InSlow + GlobalSlowMotion2 + CTSSlowMotion; (6) CTSGun fire-type dispatch; (7)
CTSGroundAmmo. NEXT: the faithful weapon items-array (m_pActiveItem/m_pActiveInfo +
s_weapon_status + AddAmmo + PickUpWeapon/BuyWeapon armament + buy menu — un-SLICEs
akimbo/grenade/burst-fire, the per-caliber ammo, and the inf-clip/grenade powerup
branches) and/or the visible slow-motion time-dilation (needs cl_dll); objective/CTF
entities (CTSHack/CTSBriefcase/CTSBomb/CTSFlag/CTSWinGiver) for team modes. Fill
CTSGameRules methods as needed.

---

## 2026-06-20 (cont. 8) — CTSGameRules skeleton installed + runtime-verified

**No CTSPlayer class** — TS extends stock CBasePlayer. `CTSGameRules : CHalfLifeMultiplay`
is the live g_pGameRules (InstallGameRules). GetRandomPowerup(mask) reconstructed.
Caveat: tm_ teamplay maps use MP base; revisit when TS team logic lands.

## 2026-06-20 (cont. 7) — CTSMapGlobals runtime-verified
get_weapon_restriction() = 0x1f ^ spawnflags. (ts_trinityfall).

## 2026-06-20 (cont. 6) — CTSModel + CTSFog; FIRST in-engine runtime verification
`dlls/ts_model.cpp` cycler-style prop (858 placements). Decompile TRUNCATED by Ghidra
noreturn at UTIL_SetSize; real body via objdump (Ghidra adds 0x10000 vs nm).
**Runtime rig:** 64-bit ReHLDS (QUIC) at ~/Containers/hlds/hlds-linux64-test. Boot:
`LD_LIBRARY_PATH=.:$LD_LIBRARY_PATH timeout -s INT 30 ./hlds_linux -game ts -condebug
+developer 1 +sv_lan 1 +maxplayers 4 +map <map>`. SIGINT not KILL. `Can't init <class>`
= negative control. With -condebug, ALERT(at_console) probe → <rig>/ts/qconsole.log.

## 2026-06-20 (cont. 5) — CTSCustomTarget built (point relay; no stock map uses it).
## 2026-06-20 (cont. 4) — DECISION: functional/source-faithful, NOT byte drop-in.
   ts_utils TSHudMessageAll + ts_timer CTSTimer. glibc 2.14.
## 2026-06-20 (cont. 3) — CTSSlowMotion draft (superseded by cont.15).
## 2026-06-20 (cont. 2) — gnv2demangle.py (1788/2426 typed sigs) + DecompExport.java.
## 2026-06-20 (cont.) — Vino OK'd reconstruction; Ghidra pipeline (decompile.sh).
## 2026-06-20 — Scaffolding: fork halflife-updated@64bit-demo; reference/; tooling;
   work-list RECONSTRUCTION.md (60 TS classes). Builds via hlds64-buildchain image.

**Conventions:** NO Co-Authored-By trailer. CLAUDE.md local-only. Auto mode:
halt & ask on a block.
