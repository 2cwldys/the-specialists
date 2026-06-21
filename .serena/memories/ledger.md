# mod-ts change ledger

Running log of reconstruction progress. Newest entries on top. Keep factual:
what changed, how verified, what's next. See `CLAUDE.md` for mission/method.

---

## 2026-06-21 (cont. 15) — CTSSlowMotion entity + GoSlow/InSlow + GlobalSlowMotion2

**Done — the slow-motion STATE layer + `dlls/ts_slowmotion.cpp` CTSSlowMotion
(ts_slowmotion brush zone, 11 placements).** Reconstructed from the .so:
- **Slow-motion virtuals are `GoSlow`/`InSlow`/`GiveSlowMul`** (found via the
  CBasePlayer vtable relocations: +0x10 GiveSlowMul, +0x14 GoSlow(float,char) =
  "SetSlowMotion", +0x18 InSlow = "IsSlowMotion"). Implemented as **CBasePlayer
  methods** (NOT CBaseEntity virtuals — CTSSlowMotion only slows players):
  - `CBasePlayer::GoSlow(scale, bullets)` (ts_player.cpp) sets m_flSlowMotionGo
    (0x168) + m_flSlowMotionTarget (0x170) + m_bSlowBullets. `InSlow()` = (go!=1 ||
    target!=1). Both fields init 1.0 (= normal). DeactivateSlowMotionFlow now calls
    GoSlow(1.0) first (faithful).
- **`CTSGameRules::GlobalSlowMotion2(scale, bullets)`** (ts_gamerules.cpp): applies
  GoSlow to every player each call (throttled ~0.5s via m_flNextGlobalSlow=0x56c;
  scale 0 = restore all). **SLICE:** the original also slows "grenade"/"WorldGun"
  projectiles (needs the CBaseEntity GoSlow virtual — deferred). Plus
  `SetGlobalSlowMotion(scale, dur)` (the timed wrapper; members 0x560/0x564/0x568).
- **CTSSlowMotion : CBaseEntity** (ts_slowmotion.cpp, replaces the old draft):
  Spawn (brush: classname/SET_MODEL/SetSize from the .so + SOLID_TRIGGER/MOVETYPE_NONE/
  EF_NODRAW + WaitThink poll + SF_START_ON→ActivateSlow — the trigger setup beyond
  classname/model/size is INFERRED, the real Spawn is minimal; // VERIFY); KeyValue
  duration(0xf8)/slowamt(0xfc); WaitTouch (GoSlow the toucher if active + not already
  InSlow, track in m_hPlayers[33] by ENTINDEX); ActivateSlow (duration>0 = timed
  stopTime, else toggle); DeactivateSlow (GoSlow(1.0) all tracked, or GlobalSlowMotion2(0)
  for all-map); WaitThink (0.5s poll: CheckPlayersDistance or GlobalSlowMotion2, time
  out → DeactivateSlow); CheckPlayersDistance (drop players beyond m_flRadius from
  Center() — reconstructed from intent, the decompile is heavy stack arithmetic;
  m_flRadius = half brush diagonal, // VERIFY); Use (activate + TSHudMessageAll +
  FireTargets); Save/Restore (duration + slow_ammount). Used EHANDLE m_hPlayers[] for
  safety (orig used raw ptrs). Spawnflags: 1 Start ON, 2 All map, 4 Slow bullets.

**Player members added (player.h):** m_flSlowMotionGo (0x168, init 1.0),
m_flSlowMotionTarget (0x170, init 1.0). CTSGameRules members: m_flNextGlobalSlow
(0x56c), m_flGlobalSlow{Active,Scale,End} (0x560/0x564/0x568).

**Verified RUNTIME (rcbot bots, ts_wyndham = 4 placements), temp StartFrame probe
(reverted):** 4 ts_slowmotion spawned (`Can't init` gone); `Use()` on a zone
activated without crash; `GlobalSlowMotion2(0.5,1)` → player `InSlow()=1, go=0.50,
bullets=1`; `GlobalSlowMotion2(0,0)` → `InSlow()=0, go=1.0` (restored). No crash
(gdb -batch, no CTSSlowMotion/GoSlow in any backtrace). Final clean regression boot:
0 `Can't init ts_slowmotion`, 0 crashes.

**SLICE/deferred (the visible time-dilation, client-entangled):** the actual
slow-motion MOVEMENT (GoSlow only sets server fields that the client pmove reads — no
cl_dll); GlobalSlowMotion2 slowing projectiles (needs CBaseEntity GoSlow/InSlow
virtuals); `SlowMotionFlow` (@0x86bb0, 1758 bytes — the powerup→GoSlow propagation
between nearby players, so an *activated slowmo powerup* still doesn't call GoSlow yet);
`InterpolateSlowMotion` (@0x8910c); client motion-blur HUD; cmd_globalslow debug cmd;
CTSSlowMotion's exact Spawn trigger-setup + m_flRadius (inferred). FixedSlowAmt/
GiveSlowMul (slow→multiplier clamp) essentially unused server-side, skipped.

---

## 2026-06-21 (cont. 14) — Powerup activation state machine (UsePowerUp/UpdatePowerUpState)

**Done — the held timed powerups now ACTIVATE + expire.** Reconstructed from
`CBasePlayer::UsePowerUp` (@0x85980), `UpdatePowerUpState` (@0x85748),
`DeactivateSlowMotionFlow` (@0x86a9c):
- **`CBasePlayer::UsePowerUp()`** (ts_player.cpp): the `usepowerup` console command.
  Alive + holding a timed powerup + none active → m_iPowerups = m_iHeldPowerup,
  m_flPowerupExpireTime = m_flPowerupTime + time; sends gmsgTSPowerUpTime (MSG_ONE) +
  gmsgTSMotionBlur (MSG_PVS, entindex + 1=slowmo/2=slowpause). Returns true.
- **`CBasePlayer::UpdatePowerUpState()`**: per-frame, hooked into `CBasePlayer::PreThink`.
  While active: dblfire→lowgrav gravity tweak; slowmo→m_flSlowMotionScale 0.6/Ramp 0.35;
  slowpause→scale 0.8/ramp 0.225, bullets 1; on expiry/death → DeactivateSlowMotionFlow
  + clear all powerup state.
- **`usepowerup` ClientCommand** (client.cpp) — confirmed via the .so (ClientCommand
  @0x9d838 strcmp "usepowerup" → UsePowerUp). (TSUse @0x856e0 = the USE-key+cooldown
  path; not wired — the command is enough.)
- Player members added: m_bSlowBullets (0x175), m_flSlowMotionScale (0x17c),
  m_flSlowMotionRamp (0x180). gmsgTSPowerUpTime/gmsgTSMotionBlur registered lazily.

**Verified RUNTIME (rcbot, ts_theatre):** handed a bot a held slowmo powerup (dur 3) +
called UsePowerUp → m_iPowerups=1, SlowMotion()=1, scale=0.60 for 3s, then expired +
cleared on schedule. No crash. (cont.15 note: the activation sets m_flSlowMotionScale,
NOT GoSlow — the powerup→GoSlow link is SlowMotionFlow, still deferred.)

---

## 2026-06-21 (cont. 13) — CTSPowerUp + PlayerAddPowerUp (the 328-placement pickup)

**Done — `dlls/ts_powerup.cpp` CTSPowerUp : CBaseAnimating (ts_powerup, 328 placements).**
Full lifecycle from the .so (Spawn + OnGround TRUNCATED by Ghidra noreturn at
UTIL_SetOrigin — real tails via objdump; Spawn's `nextthink = time + 0.1` was in the
cut part, missed → powerups never settled; caught by "22 spawned but 0 pickups"):
- Spawn: powerup.mdl, MOVETYPE_TOSS+SOLID_BBOX, render setup, SetThink(Fall)+nextthink
  time+0.1, SetupPowerUp(type, -1, 0.0, respawns=true).
- SetupPowerUp: type<1 → GetRandomPowerup(pev->spawnflags) (spawnflags = the random
  MASK!), else explicit; pev->body icon by type + default duration. m_flTimeout/Armor/
  m_bRespawns.
- Fall→OnGround (SOLID_TRIGGER, SetTouch(WaitTouch)+SetThink(WaitThink))→WaitThink
  (fade temp)→WaitTouch (pickup gate: instant 0x1e6 always, timed only if none held;
  re-drop swap; UTIL_Remove temp or re-roll+respawn). CreatePowerUp static factory.
  Save/Restore m_iType+m_iRespawnTime. KeyValue pwuptype/pwupduration/pwupmult.

**Done — `PlayerAddPowerUp(CBasePlayer*, type, CTSPowerUp*)`** (@0x87bc8): instant
powerups real (armor→armorvalue SLICE, kungfu, lowgrav→gravity 0.4, health→150,
grenade→GiveTSWeapon(M61) SLICE, infclip→refill active gun SLICE), timed (1/8/0x10)
held. Player members m_flPowerupExpireTime (0x72c), m_flHealthRegen (0x73c).

**Verified RUNTIME (ts_theatre = 22):** 22 spawned all 8 types from spawnflags; 6
pickups; gate rejects 2nd timed powerup. No crash.

---

## 2026-06-21 (cont. 12) — Full 38-weapon table + CTSGun by type; gdb debugging

`scripts/extract_weapons.py` → full s_weapon_info table (field offsets: 0x44 name,
0x132 v_model, 0x172 p_model, 0x1f2 w_model, 0x232 anim-ext, 0x2c6 fire snd, 0x04 clip,
0x1c damage, 0x348 fire-type). ts_weapons.{h,cpp} full table + PrecacheAllTSWeapons +
GiveTSWeapon. ts_gun.cpp parameterized by m_weaponType; ground weapon gives real type.
CWorld::Precache → UTIL_PrecacheOtherWeapon at map load. VERIFIED (ts_wyndham): type
25→v_knife dmg40, type 3→v_uzi dmg22. Two GDB-found crashes fixed (PRECACHE_EVENT must
be map-load; DeathNotice ItemInfoArray needs the precache registration).
**Use gdb** on our hl.so for crashes (`gdb -batch -ex run -ex bt --args ./hlds_linux`).
SLICE: shared id 16 = one weapon at a time; no fire-type dispatch/spread; buy menu TODO.

---

## 2026-06-21 (cont. 11) — CTSGroundWeapon (weapon pickups) + tsweapon_t fix

tsweapon_t index = (lea offset − 0x7d30)/0x3DC, MATCHES FGD tsweaponid. beretta=2,
uzi=3, knife=25, idx 29=flag. ts_groundweapon.cpp (65 placements): SLICE gives
weapon_tsgun; respawn after wduration. VERIFIED (ts_wyndham).

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
TS weapons precached at MAP LOAD (CWorld::Precache).

**Bot test rig (working, reusable):** ~/Containers/hlds/hlds-linux64-test:
ts/addons/metamod/dlls/metamod_amd64.so (Metamod-R) as gamedll (liblist.gam → it);
config.ini=`gamedll dlls/hl.so`; plugins.ini=`linux addons/metamod/dlls/rcbot_mm_amd64.so`.
<rig>/rcbot/* configs; min_bots 2. Boot normally → rcbot bots = real CBasePlayers. Temp
StartFrame hook can teleport/give to a bot to force pickup/activation (revert after).
NOTE: bots may not be connected until ~t>6+; gate probes on a player existing.

---

## 2026-06-20 (cont. 9) — TSHudMessageOne + CBasePlayer TS extensions (step 1)

`dlls/ts_utils.{h,cpp}` TSHudMessageOne. CBasePlayer TS member block (player.h):
m_iPowerups (0x724), m_iHeldPowerup (0x728), m_flPowerupTime (0x730), m_iKungFu (0x74e),
m_bLowGravity (0x74f) + accessors. (cont.13 +0x72c/0x73c +PlayerAddPowerUp; cont.14
+0x175/0x17c/0x180 +activation; cont.15 +0x168/0x170 +GoSlow/InSlow.)

**The core is a monolith.** Sequencing: (1) player block+accessors ✓; (2) CTSGun base ✓;
(3) PlayerAddPowerUp + CTSPowerUp ✓; (4) UsePowerUp/UpdatePowerUpState activation ✓;
(5) GoSlow/InSlow + GlobalSlowMotion2 + CTSSlowMotion ✓ (cont.15); NEXT: the visible
time-dilation (client pmove + SlowMotionFlow propagation + CBaseEntity slow virtuals +
motion-blur cl_dll) and/or the faithful weapon engine (un-SLICE CTSGun); fill
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
