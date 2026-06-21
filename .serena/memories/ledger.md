# mod-ts change ledger

Running log of reconstruction progress. Newest entries on top. Keep factual:
what changed, how verified, what's next. See `CLAUDE.md` for mission/method.

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
