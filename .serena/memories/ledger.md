# mod-ts change ledger

Running log of reconstruction progress. Newest entries on top. Keep factual:
what changed, how verified, what's next. See `CLAUDE.md` for mission/method.

---

## 2026-06-21 (cont. 16) — CTSGun fire-type dispatch (semi/auto/shotgun/melee)

**Done — weapons now fire faithfully by fire-type.** Un-SLICEs the "everything is
identical semi-auto bullets" limitation.
- **`scripts/extract_weapons.py`** extended: also pulls per-weapon fire-rate (0x18),
  base spread (0x28), recoil (0x34) from each Write_<weapon> (they were already in the
  numeric-immediate scan; just emitted now). Regenerated the table.
- **`ts_weapons.{h,cpp}`**: s_weapon_info gains flFireRate/flSpread/flRecoil; added
  `enum tsfiretype_t` (0/8 semi, 1 pump-shotgun, 2 grenade, 3 knife, 4/5 akimbo, 6
  auto-shotgun/rifle, 7 sniper, 9 SMG-auto, 10 katana, 11 flag). Write() takes 3 more args.
- **`CTSGun::PrimaryAttack`** (ts_gun.cpp) dispatches by `get_weapon_info(m_weaponType)
  ->iFireType`:
  - melee (3 knife / 10 katana) → new `MeleeAttack(wi)`: a forward hull-trace
    (knife 48u, katana 64u), TraceAttack(flDamage, DMG_CLUB) on a hit, one slash per
    press. No bullets. SLICE: no swing/hit sounds (TS knife sounds not precached), no
    katana throw/charge.
  - full-auto (9 SMG / 6 rifle) → fires while IN_ATTACK held.
  - semi (0/8 pistols, 7 sniper, 1 pump, + SLICE akimbo/grenade) → one shot per press
    (gated on m_pPlayer->m_afButtonPressed & IN_ATTACK).
  - pump shotgun (1) → 6 pellets/shot (cPellets; // VERIFY exact count). Others 1.
  - uses the weapon's real flSpread + flFireRate (GetNextAttackDelay).
  - SLICE: grenade (2) / akimbo (4/5) fall through to bullets (need the throw entity /
    dual-wield items-array). The faithful path dispatches on m_pActiveItem/m_pActiveInfo
    (the items-array) reading +0x348 fire-type / +0x18 rate / +0x28 spread; we dispatch
    on the single held m_weaponType instead.

**Verified RUNTIME (rcbot bots, ts_wyndham), temp StartFrame probe (armed bots with
varied weapon types + pulsed IN_ATTACK; reverted):**
- beretta ft=8 SEMI: 43 fires (one per press, spread 0.015) vs uzi ft=9 AUTO: 201
  fires (continuous while held) — clear semi-vs-auto distinction.
- benellim3 ft=1 SHOTGUN: pellets=6, spread 0.040 per shot.
- knife ft=3 (range 48) + katana ft=10 (range 64): hull-trace MELEE, hit func_breakable
  / worldspawn, dmg 30/40, NO bullets. 24 melee attacks, no crash.
- Final clean regression boot (probes removed): 0 crashes.

**Data gaps (// VERIFY):** mossberg(26)/sawedoff(33) fire-type extracted as 0 (semi)
though they're pump shotguns — 0x348 not captured for them; pellet count fixed at 6
(real count not yet mapped); m61 rate/spread look copied from a neighbour.

**Recovered fire param offsets:** s_weapon_info +0x18 fire-rate, +0x28 base spread,
+0x34 recoil/spread-scale, +0x348 fire-type. The fire dispatch funcs in the .so:
PrimaryAttack→Fire→{Standard,Shotgun,Akimbo,Grenade,Katana,Knife}Fire (@0x71c78 Fire);
they read m_pActiveItem(+0xcc)+2 fire-mode / +8 burst, m_pActiveInfo(+0xd0)+0x348/0x18/
0x28. The semi-vs-auto + burst/fire-mode is in m_pActiveItem (the s_weapon_status
struct) — part of the deferred items-array.

---

## 2026-06-21 (cont. 15) — CTSSlowMotion entity + GoSlow/InSlow + GlobalSlowMotion2

**Done — the slow-motion STATE layer + `dlls/ts_slowmotion.cpp` CTSSlowMotion
(ts_slowmotion brush zone, 11 placements).** Reconstructed from the .so:
- **Slow-motion virtuals are `GoSlow`/`InSlow`/`GiveSlowMul`** (found via the
  CBasePlayer vtable relocations: +0x10 GiveSlowMul, +0x14 GoSlow(float,char), +0x18
  InSlow). Implemented as **CBasePlayer methods** (CTSSlowMotion only slows players):
  GoSlow(scale, bullets) sets m_flSlowMotionGo (0x168) + m_flSlowMotionTarget (0x170) +
  m_bSlowBullets; InSlow() = (go!=1 || target!=1). Both fields init 1.0. DeactivateSlow-
  MotionFlow now calls GoSlow(1.0) first.
- **`CTSGameRules::GlobalSlowMotion2(scale, bullets)`** + `SetGlobalSlowMotion(scale,
  dur)` (ts_gamerules.cpp): applies GoSlow to every player, throttled ~0.5s (members
  0x560/0x564/0x568/0x56c). SLICE: doesn't slow projectiles (needs CBaseEntity virtual).
- **CTSSlowMotion : CBaseEntity** (replaces the old draft): Spawn (brush trigger;
  setup beyond classname/model/size INFERRED, // VERIFY); KeyValue duration(0xf8)/
  slowamt(0xfc); WaitTouch (GoSlow toucher, track in m_hPlayers[33]); ActivateSlow
  (timed/toggle); DeactivateSlow; WaitThink (0.5s poll); CheckPlayersDistance (drop
  players beyond m_flRadius from Center(), intent-reconstructed); Use; Save/Restore.
  EHANDLE array for safety. Spawnflags 1 Start ON / 2 All map / 4 Slow bullets.

**Verified RUNTIME (rcbot, ts_wyndham = 4):** 4 zones spawned; Use() activated no
crash; GlobalSlowMotion2(0.5,1) → player InSlow()=1 go=0.50; (0,0) → InSlow()=0 go=1.0.
No crash; clean regression.

**SLICE/deferred (visible time-dilation, client-entangled):** actual slow MOVEMENT
(client pmove reads the fields — no cl_dll); GlobalSlowMotion2 slowing projectiles;
SlowMotionFlow (@0x86bb0, powerup→GoSlow propagation); InterpolateSlowMotion (@0x8910c);
motion-blur HUD; cmd_globalslow; CTSSlowMotion Spawn/m_flRadius inferred.

---

## 2026-06-21 (cont. 14) — Powerup activation state machine (UsePowerUp/UpdatePowerUpState)

**Done — held timed powerups ACTIVATE + expire.** From UsePowerUp (@0x85980),
UpdatePowerUpState (@0x85748), DeactivateSlowMotionFlow (@0x86a9c):
- CBasePlayer::UsePowerUp() (ts_player.cpp): `usepowerup` command. Alive + holding +
  none active → m_iPowerups = m_iHeldPowerup, expire = m_flPowerupTime + time; sends
  gmsgTSPowerUpTime + gmsgTSMotionBlur. UpdatePowerUpState() per-frame in PreThink:
  apply slow fields (slowmo scale 0.6, slowpause 0.8) / expire + clear.
- `usepowerup` ClientCommand (confirmed via .so ClientCommand @0x9d838).
- Player members: m_bSlowBullets (0x175), m_flSlowMotionScale (0x17c), m_flSlowMotionRamp
  (0x180). gmsg registered lazily.

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
fire a weapon on a bot (gate on a player existing — bots connect ~t>6-8; pulse IN_ATTACK
on/off via m_afButtonPressed to test semi vs auto). Revert hooks after.

---

## 2026-06-20 (cont. 9) — TSHudMessageOne + CBasePlayer TS extensions (step 1)

`dlls/ts_utils.{h,cpp}` TSHudMessageOne. CBasePlayer TS member block (player.h):
m_iPowerups (0x724), m_iHeldPowerup (0x728), m_flPowerupTime (0x730), m_iKungFu (0x74e),
m_bLowGravity (0x74f) + accessors. (cont.13 +0x72c/0x73c; cont.14 +0x175/0x17c/0x180;
cont.15 +0x168/0x170 GoSlow/InSlow.)

**The core is a monolith.** Sequencing done: (1) player block+accessors; (2) CTSGun
base; (3) PlayerAddPowerUp + CTSPowerUp; (4) UsePowerUp/UpdatePowerUpState; (5) GoSlow/
InSlow + GlobalSlowMotion2 + CTSSlowMotion; (6) CTSGun fire-type dispatch (cont.16).
NEXT: the faithful weapon items-array (m_pActiveItem/m_pActiveInfo + s_weapon_status +
PickUpWeapon/BuyWeapon armament + buy menu — un-SLICEs akimbo/grenade/burst-fire and
the inf-clip/grenade powerup branches) and/or the visible slow-motion time-dilation
(needs cl_dll); objective/CTF entities (CTSHack/CTSBriefcase/CTSBomb/CTSFlag) for team
modes. Fill CTSGameRules methods as needed.

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
