# mod-ts change ledger

Running log of reconstruction progress. Newest entries on top. Keep factual:
what changed, how verified, what's next. See `CLAUDE.md` for mission/method.

---

## 2026-06-21 (cont. 13) — CTSPowerUp + PlayerAddPowerUp (the 328-placement pickup)

**Done — `dlls/ts_powerup.cpp` CTSPowerUp : CBaseAnimating (ts_powerup, 328 placements).**
The biggest remaining `Can't init`. Full lifecycle reconstructed from the .so
(Spawn + OnGround were TRUNCATED by Ghidra noreturn artifacts at UTIL_SetOrigin —
real tails read via objdump; Spawn's `nextthink = time + 0.1` was in the cut part
and I initially missed it → powerups never thought/settled; caught by the
"22 spawned but 0 pickups" runtime signal, fixed):
- **Spawn**: model powerup.mdl, MOVETYPE_TOSS + SOLID_BBOX, render (kRenderTransColor,
  amt 255, colour 0, fx NoDissipation), SetTouch(NULL), **SetThink(Fall) + nextthink
  = time+0.1**, ResetSequenceInfo, then SetupPowerUp(m_iType, -1, 0.0, respawns=true).
- **SetupPowerUp(type, dur, fallDelay, respawns)**: if type<1 → m_iType =
  TSGameRules()->GetRandomPowerup(pev->spawnflags) (the FGD spawnflags ARE the
  allowed-powerups mask!), fallback GetRandomPowerup(0x1ff), m_bRandom=1; else use
  explicit type (re-roll low-grav/0x80). Picks pev->body icon by type (slowmo=4,
  infclip=2, kungfu=0, slowpause=9, dblfire=1, grenade=5, health=6, armor=7,
  lowgrav=8) + default duration (slowmo/infclip/dblfire=3, slowpause=2). dur =
  round(dur*m_flMult) if dur<1. m_flTimeout, m_flArmor=100, m_bRespawns.
- **Fall** (think): hide if `nopowerup`==1 (SLICE: + objective team modes deferred);
  else nextthink+0.1, if FL_ONGROUND → OnGround(); UTIL_Remove a temp powerup past
  timeout.
- **OnGround** (settle, real tail via objdump): SOLID_TRIGGER, clear EF_NODRAW + set
  EF_DIMLIGHT, UTIL_SetOrigin/SetSize, **SetTouch(WaitTouch) + SetThink(WaitThink)**
  — this is what makes it pickup-able.
- **WaitThink**: StudioFrameAdvance, nextthink+1; fade renderamt + UTIL_Remove for a
  temp powerup; map powerups wait forever.
- **WaitTouch(pOther)**: pickup gate (recovered boolean chain) — instant powerups
  (mask 0x1e6) always taken; timed ones (slowmo/slowpause/dblfire) only when the
  player holds none (special near-spent slowmo re-take), USE-button allows swap
  (re-drops the old via CreatePowerUp). On success: UTIL_Remove (temp) or re-roll +
  hide + respawn after delay (m_iRespawnTime <1→60s, <11→10s, else min(n,180)),
  TSHudMessageOne(pev->message) + FireTargets(pev->target).
- **CreatePowerUp(type, dur)** (static): GetClassPtr → Spawn → SetupPowerUp(type,
  dur, 60.0, respawns=false). For player-dropped/dynamic powerups.
- **Save/Restore**: m_iType + m_iRespawnTime (the 2 fields the orig saved; MP has no
  transitions so provenance-faithful).
- KeyValue: pwuptype→m_iType, pwupduration→m_iRespawnTime, pwupmult→m_flMult.

**Done — `PlayerAddPowerUp(CBasePlayer*, int type, CTSPowerUp*)`** (free func @0x87bc8,
in ts_powerup.cpp). Returns true if taken (→consume). INSTANT powerups applied for
real, TIMED ones stored as the player's held powerup:
- armor (0x80): SLICE adds pev->armorvalue (orig CBasePlayer::TakeArmor not built).
- kungfu (4): m_iKungFu=1. lowgrav (0x100): m_bLowGravity=1 + pev->gravity=0.4
  (SLICE: orig sets a client physics key). health (0x40): pev->health=150 +
  m_flHealthRegen. grenade (0x20): SLICE GiveTSWeapon(M61) (orig PickUpWeapon).
  infclip (2): SLICE refill active CTSGun clip (orig CTSGun::MultiplyClips — needs
  the unbuilt items-array). timed (1/8/0x10): m_iHeldPowerup=type, m_flPowerupTime=
  duration, m_iPowerups=0 (cleared; activation = next layer). Pickup sound.

**Player members added (player.h):** m_flPowerupExpireTime (0x72c), m_flHealthRegen
(0x73c). (m_iPowerups 0x724 / m_iHeldPowerup 0x728 / m_flPowerupTime 0x730 / m_iKungFu
0x74e / m_bLowGravity 0x74f already existed; accessor offsets re-confirmed via objdump.)

**Verified RUNTIME (rcbot bots, ts_theatre = 22 placements), via temp probes +
forced-touch StartFrame hook (all reverted):**
- **22 ts_powerup spawned**, all 8 types randomly rolled from spawnflags (slowpause
  most common). `Can't init ts_powerup` GONE.
- Powerups settle (TOSS→FL_ONGROUND→OnGround→SOLID_TRIGGER, 23 settled incl. a
  respawn cycle).
- **6 pickups** across slowpause/dblfire (timed→held), grenade (→M61), health
  (→150hp). 6/13 forced touches taken = the pickup GATE working (a player already
  holding a timed powerup correctly rejects further timed ones). **No crash**
  (gdb -batch, clean exit, no CTSPowerUp/PlayerAddPowerUp in any backtrace).
- Final clean regression boot (no probes): 0 `Can't init`, 0 crashes.

**Key recovered fact:** ts_powerup spawnflags = the random-powerup MASK (which
types this spawner may give), NOT a single type. pwuptype forces an exact type.

**SLICE/deferred (next layer = slow-motion):** the TIMED powerups are only *held*,
not *activated* — `CBasePlayer::UsePowerUp` (@0x95980, USE-key activates: sets
m_iPowerups, m_flPowerupExpireTime, sends gmsgTSPowerUpTime + gmsgTSMotionBlur) and
`UpdatePowerUpState` (@0x95748, per-frame expiry + DeactivateSlowMotionFlow + re-drop)
are entangled with the slow-motion system and the client.dll HUD. Also deferred:
CBasePlayer::TakeArmor, CTSGun::MultiplyClips (needs items-array), PickUpWeapon
(grenade), gmsgTSPowerUp HUD notify, lowgrav client physics key, CTSGameRules::PutPowerUp
(@0x9797c — dynamic random-spawn at info_player_deathmatch; not wired, no caller yet).

---

## 2026-06-21 (cont. 12) — Full 38-weapon table + CTSGun by type; gdb debugging

**Done — the whole arsenal is real.**
- `scripts/extract_weapons.py`: parses each Write_<weapon> in the .so (resolves
  PIC-relative string leas + immediate numeric fields) -> the full s_weapon_info
  table. Field offsets: 0x44 name, 0x132 v_model, 0x172 p_model, 0x1f2 w_model,
  0x232 anim-ext (onehanded/twohanded/akimbo!), 0x2c6 fire snd, 0x04 clip,
  0x1c damage, 0x348 fire-type. (0xcc = base weapon id, NOT table index — place by
  compile_weapons_info offset instead.)
- `ts_weapons.{h,cpp}`: slimmed s_weapon_info to what CTSGun uses + generated
  37-entry table (idx 29 flag blank). `PrecacheAllTSWeapons()` (models/sounds/
  events) + `GiveTSWeapon(player, type)`.
- `ts_gun.cpp`: CTSGun now parameterized by m_weaponType (not hardcoded beretta);
  Deploy/fire/Spawn use the table; GiveTSWeapon creates a typed CTSGun.
- `ts_groundweapon.cpp`: GetWeapon -> GiveTSWeapon(player, m_weaponType) so a
  pickup gives the RIGHT gun. `world.cpp` CWorld::Precache calls
  UTIL_PrecacheOtherWeapon("weapon_tsgun") at map load (registers ItemInfoArray[16]
  + precaches roster). Declared UTIL_PrecacheOtherWeapon in util.h.

**Verified RUNTIME (rcbot bots, ts_wyndham):** ground-weapon type 25 -> v_knife.mdl
dmg 40; type 3 -> v_uzi.mdl dmg 22. Right weapon, right model, right damage, no
crash. Regression clean.

**Two crashes found+fixed via GDB (not guessing):**
1. EV_Precache (PRECACHE_EVENT) segfault — events must be precached at MAP LOAD;
   moved the 3 weapon events into PrecacheAllTSWeapons.
2. CHalfLifeMultiplay::DeathNotice strncmp segfault on a kill — pszName() reads
   ItemInfoArray[m_iId], which was never populated because I'd dropped the
   UTIL_PrecacheOtherWeapon registration. Restored it in CWorld::Precache.
   (gdb backtrace: PrimaryAttack->FireBulletsPlayer->...->DeathNotice.)

**Debugging note:** use **gdb** on our own hl.so for crashes (the rig keeps debug
syms; `gdb -batch -ex run -ex bt --args ./hlds_linux ...`). This is the server-side
counterpart to Ghidra (which reads the *original* .so).

**SLICE limits still open:** shared weapon id 16 -> one TS weapon held at a time;
per-weapon spread not mapped (0.04 default); fire-type dispatch (grenade/knife/
katana/akimbo) not implemented (all semi-auto bullets). Faithful PickUpWeapon
armament + buy menu still TODO.

---

## 2026-06-21 (cont. 11) — CTSGroundWeapon (weapon pickups) + tsweapon_t fix

**FIXED tsweapon_t indices (cont.10 enum was WRONG).** Real index = (lea offset −
0x7d30)/0x3DC, MATCHES the FGD `tsweaponid` choices. Corrected ts_weapons.h:
beretta=2, uzi=3, mk23=9, knife=25, etc. Index 29 = flag (CTF).

**Done — `dlls/ts_groundweapon.cpp` CTSGroundWeapon (ts_groundweapon, 65 placements)**
KeyValue tsweaponid/wduration/wextraclip/buyable. Spawn: world model + SOLID_TRIGGER
/MOVETYPE_TOSS + SetTouch(GetWeapon); Precache UTIL_PrecacheOther("weapon_tsgun") at
map load. GetWeapon (touch): SLICE gives weapon_tsgun; respawn after wduration.
VERIFIED RUNTIME (rcbot, ts_wyndham): Can't init/create gone, pickup works, no crash.

---

## 2026-06-20 (cont. 10) — CTSGun recon + weapon data layer (vertical-slice start)

**Decision:** weapon subsystem as a VERTICAL SLICE (beretta first), then scale.
**CTSGun is a subsystem:** ONE unified CBasePlayerWeapon (id 16) morphing by
tsweapon_t with fire dispatch (Standard/Shotgun/Knife/Katana/Grenade/Akimbo).

**tsweapon_t enum (38, real order from compile_weapons_info @0x6fe28):**
0 melee,1 glock18,2 uzi,3 uzi_akimbo,4 benellim3,5 m4,6 mp5sd,7 mp5k,
8 beretta_akimbo,9 beretta,10 mk23,11 mk23akimbo,12 usas12,13 desert,14 ak47,
15 fnh,16 aug,17 skorpion,18 barrett,19 pdw,20 spas12,21 gold,22 glock22,
23 mac10,24 m61,25 knife,26 mossberg,27 m16,28 ruger,29 fnhakimbo,30 bull,
31 m60,32 sawedoff,33 katana,34 sealknife,35 contender,36 skorpion_akimbo,37 flag.

The FAITHFUL CTSGun engine (dual-wield items array m_pActiveItem@0xcc /
m_pActiveInfo@0xd0 / items[]@0x45, fire-type dispatch via s_weapon_info+0x348,
weapon-param networking, CTSAnimation, akimbo/firemode/zoom) replaces the // SLICE
bodies incrementally. TS weapons must be precached at MAP LOAD (PRECACHE_MODEL is
spawn-only) — roster precache in CWorld::Precache.

**Bot test rig (working, reusable):** in ~/Containers/hlds/hlds-linux64-test:
- ts/addons/metamod/dlls/metamod_amd64.so (Metamod-R/build-amd64) as gamedll
  (liblist.gam gamedll_linux → it); config.ini = `gamedll dlls/hl.so` (metamod loads
  OUR build); plugins.ini = `linux addons/metamod/dlls/rcbot_mm_amd64.so`.
- <rig>/rcbot/* configs (rcbotold/rcbot); bot_config.ini `config min_bots 2`.
Boot normally → rcbot auto-adds bots that spawn as real CBasePlayers. To force a
weapon/powerup pickup, a temp StartFrame hook can teleport a bot/entity (revert after).

---

## 2026-06-20 (cont. 9) — TSHudMessageOne + CBasePlayer TS extensions (step 1)

`dlls/ts_utils.{h,cpp}` TSHudMessageOne (MSG_ONE variant). CBasePlayer TS member block
(player.h): m_iPowerups (0x724 bitfield), m_iHeldPowerup (0x728), m_flPowerupTime
(0x730), m_iKungFu (0x74e), m_bLowGravity (0x74f) + accessors (SlowMotion=&1/
InfiniteClips=&2/SlowPause=&8/DoubleFireRate=&0x10/KungFu). (cont.13 added 0x72c, 0x73c
and implemented PlayerAddPowerUp.)

**The core is a monolith.** Sequencing: (1) player member block+accessors ✓; (2) CTSGun
base ✓; (3) PlayerAddPowerUp + CTSPowerUp ✓ (cont.13); (4) GlobalSlowMotion2/
SetGlobalSlowMotion -> CTSSlowMotion + UsePowerUp/UpdatePowerUpState (NEXT); fill
CTSGameRules methods as needed.

---

## 2026-06-20 (cont. 8) — Foundation: CTSGameRules skeleton installed + runtime-verified

**No CTSPlayer class** — TS extends stock CBasePlayer directly. `dlls/ts_gamerules.{h,cpp}`
`CTSGameRules : CHalfLifeMultiplay` is the live g_pGameRules (InstallGameRules).
GetRandomPowerup(mask) reconstructed. 40 other methods inherit MP. TS_PWUP_* +
TSGameRules() accessor. Verified runtime, no crash. Caveat: tm_ teamplay maps now use
MP base not CHalfLifeTeamplay; revisit when TS team logic lands.

---

## 2026-06-20 (cont. 7) — CTSMapGlobals runtime-verified

`dlls/ts_mapglobals.cpp` `CTSMapGlobals : CBaseEntity`. Config in spawnflags.
get_weapon_restriction() = 0x1f ^ pev->spawnflags. VERIFIED (ts_trinityfall).

---

## 2026-06-20 (cont. 6) — CTSModel + CTSFog; FIRST in-engine runtime verification

`dlls/ts_model.cpp` cycler-style animated prop (858 placements). Recovered decompile
TRUNCATED by Ghidra noreturn at UTIL_SetSize; real body via objdump (Ghidra adds
0x10000 vs nm). `dlls/ts_fog.cpp`: Spawn=UTIL_Remove.

**Runtime rig:** 64-bit ReHLDS (QUIC) at ~/Containers/hlds/hlds-linux64-test. Boot:
`LD_LIBRARY_PATH=.:$LD_LIBRARY_PATH timeout -s INT 30 ./hlds_linux -game ts -condebug
+developer 1 +sv_lan 1 +maxplayers 4 +map <map>`. SIGINT not KILL. `Can't init <class>`
= engine marker for an unimplemented TS class (negative control). With -condebug,
ALERT(at_console) probe output lands in <rig>/ts/qconsole.log, NOT stdout.

---

## 2026-06-20 (cont. 5) — CTSCustomTarget built (point relay; no stock map uses it).
## 2026-06-20 (cont. 4) — DECISION: functional/source-faithful, NOT byte drop-in.
   Done: ts_utils TSHudMessageAll + ts_timer CTSTimer. glibc 2.14.
## 2026-06-20 (cont. 3) — CTSSlowMotion draft (not built; deps CTSGameRules+player).
## 2026-06-20 (cont. 2) — gnv2demangle.py (1788/2426 typed sigs) + DecompExport.java.
## 2026-06-20 (cont.) — Vino OK'd reconstruction; Ghidra pipeline (decompile.sh).
## 2026-06-20 — Scaffolding: fork halflife-updated@64bit-demo; reference/; tooling;
   work-list RECONSTRUCTION.md (60 TS classes). Builds via hlds64-buildchain image.

**Conventions:** NO Co-Authored-By trailer. CLAUDE.md local-only. Auto mode:
halt & ask on a block.
