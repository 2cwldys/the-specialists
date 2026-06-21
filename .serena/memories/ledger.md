# mod-ts change ledger

Running log of reconstruction progress. Newest entries on top. Keep factual:
what changed, how verified, what's next. See `CLAUDE.md` for mission/method.

---

## 2026-06-21 (cont. 14) — Powerup activation state machine (UsePowerUp/UpdatePowerUpState)

**Done — the held timed powerups now ACTIVATE + expire.** Reconstructed the
server-side activation state machine from `CBasePlayer::UsePowerUp` (@0x85980),
`UpdatePowerUpState` (@0x85748), `DeactivateSlowMotionFlow` (@0x86a9c):
- **`CBasePlayer::UsePowerUp()`** (in `dlls/ts_player.cpp`): the `usepowerup` console
  command. If alive + holding a timed powerup + none active → m_iPowerups =
  m_iHeldPowerup, m_flPowerupExpireTime = m_flPowerupTime + time; sends
  gmsgTSPowerUpTime (MSG_ONE, remaining seconds) + gmsgTSMotionBlur (MSG_PVS,
  entindex + 1=slowmo/2=slowpause). Returns true.
- **`CBasePlayer::UpdatePowerUpState()`**: per-frame, hooked into `CBasePlayer::PreThink`
  (player.cpp, after the game-over check — the original calls it + SlowMotionFlow
  there). While active: dblfire→lowgrav-gravity tweak; slowmo→m_flSlowMotionScale
  0.6/Ramp 0.35, m_bSlowBullets 0; slowpause→scale 0.8/ramp 0.225, bullets 1; on
  expiry/death → DeactivateSlowMotionFlow then clear all powerup state.
- **`CBasePlayer::DeactivateSlowMotionFlow()`**: SLICE = restore this player's own
  slow fields to normal (scale/ramp/bullets 0).
- **`usepowerup` ClientCommand** (client.cpp) — confirmed the activation trigger via
  the .so (ClientCommand @0x9d838 strcmp "usepowerup" → UsePowerUp). (TSUse @0x856e0
  is the alternate USE-key+cooldown path; not wired — the command path is enough.)
- **Player members added (player.h):** m_bSlowBullets (0x175), m_flSlowMotionScale
  (0x17c), m_flSlowMotionRamp (0x180). Method decls on CBasePlayer + gmsg externs in
  ts_player.h. gmsgTSPowerUpTime/gmsgTSMotionBlur registered lazily (REG_USER_MSG),
  like gmsgTSMessage.

**Verified RUNTIME (rcbot bots, ts_theatre), temp StartFrame probe (reverted):**
handed a bot a held slowmo powerup (dur 3) + called UsePowerUp → `UsePowerUp()=1,
m_iPowerups=1, expire=t+3, SlowMotion()=1`; for 3 frames-of-seconds `scale=0.60`
applied; at t+3 → `pwups=0 scale=0.00 held=0` (expired + restored on schedule). No
crash; gmsg sends clean. Final clean regression boot (probe removed, UpdatePowerUpState
runs every frame for every player incl. bots picking up powerups → held → early-return):
0 `Can't init`, 0 crashes.

**SLICE/deferred (the time-dilation EFFECT, a deeper client-entangled layer):** the
actual slow-motion MOVEMENT/animation scaling; `SlowMotionFlow` (@0x86bb0, 1758 bytes
— propagates slow-motion to nearby players/projectiles via traceline, populates the
affected-entity arrays at player+0x188/+0x208 that the full DeactivateSlowMotionFlow
walks); the new per-entity `SetSlowMotion`/`IsSlowMotion` CBaseEntity virtuals (called
by vtable+0x14/+0x18 on players + "grenade"/"WorldGun"); `CTSGameRules::
SetGlobalSlowMotion` (@0x6961c) + `GlobalSlowMotion2` (@0x69720, iterates those
entities applying the virtual) + `InterpolateSlowMotion` (@0x8910c, math: ramps a
value toward target by min(rate*7,1) when |delta|>0.05); the client motion-blur HUD;
the on-death powerup re-drop (precache-window + objective-mode dependency); the
DAT_001781b8 "slow-motion forbidden" game-mode gate. The CTSSlowMotion *entity*
(ts_slowmotion, 11 placements) draft at dlls/ts_slowmotion.cpp is still NOT in the
build (needs the per-entity virtuals + GlobalSlowMotion2).

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
- **Fall** (think): hide if `nopowerup`==1; else nextthink+0.1, if FL_ONGROUND →
  OnGround(); UTIL_Remove a temp powerup past timeout.
- **OnGround** (settle, real tail via objdump): SOLID_TRIGGER, clear EF_NODRAW + set
  EF_DIMLIGHT, UTIL_SetOrigin/SetSize, **SetTouch(WaitTouch) + SetThink(WaitThink)**.
- **WaitThink**: StudioFrameAdvance, nextthink+1; fade renderamt + UTIL_Remove for a
  temp powerup; map powerups wait forever.
- **WaitTouch(pOther)**: pickup gate (recovered boolean chain) — instant powerups
  (mask 0x1e6) always taken; timed ones only when the player holds none (special
  near-spent slowmo re-take), USE-button allows swap (re-drops old via CreatePowerUp).
  On success: UTIL_Remove (temp) or re-roll + hide + respawn after delay
  (m_iRespawnTime <1→60s, <11→10s, else min(n,180)), TSHudMessageOne + FireTargets.
- **CreatePowerUp(type, dur)** (static): GetClassPtr → Spawn → SetupPowerUp(type,
  dur, 60.0, respawns=false). For player-dropped/dynamic powerups.
- **Save/Restore**: m_iType + m_iRespawnTime. KeyValue: pwuptype/pwupduration/pwupmult.

**Done — `PlayerAddPowerUp(CBasePlayer*, int type, CTSPowerUp*)`** (free func @0x87bc8).
Returns true if taken. INSTANT powerups real, TIMED ones held:
- armor (0x80): SLICE pev->armorvalue. kungfu (4): m_iKungFu=1. lowgrav (0x100):
  m_bLowGravity=1 + pev->gravity=0.4. health (0x40): pev->health=150 + m_flHealthRegen.
  grenade (0x20): SLICE GiveTSWeapon(M61). infclip (2): SLICE refill active CTSGun clip.
  timed (1/8/0x10): m_iHeldPowerup=type, m_flPowerupTime=duration, m_iPowerups=0. Sound.

**Player members added:** m_flPowerupExpireTime (0x72c), m_flHealthRegen (0x73c).

**Verified RUNTIME (rcbot bots, ts_theatre = 22 placements):** 22 spawned, all 8 types
rolled from spawnflags; settle; 6 pickups (slowpause/dblfire→held, grenade→M61,
health→150hp). Pickup gate rejects a 2nd timed powerup correctly. No crash.

**Key recovered fact:** ts_powerup spawnflags = the random-powerup MASK, NOT a single
type. pwuptype forces an exact type.

---

## 2026-06-21 (cont. 12) — Full 38-weapon table + CTSGun by type; gdb debugging

**Done — the whole arsenal is real.**
- `scripts/extract_weapons.py`: parses each Write_<weapon> in the .so → full
  s_weapon_info table. Field offsets: 0x44 name, 0x132 v_model, 0x172 p_model,
  0x1f2 w_model, 0x232 anim-ext, 0x2c6 fire snd, 0x04 clip, 0x1c damage, 0x348
  fire-type. (0xcc = base weapon id, NOT table index.)
- `ts_weapons.{h,cpp}`: full table + PrecacheAllTSWeapons() + GiveTSWeapon(player,type).
- `ts_gun.cpp`: CTSGun parameterized by m_weaponType. `ts_groundweapon.cpp`: GetWeapon
  → GiveTSWeapon(real type). CWorld::Precache → UTIL_PrecacheOtherWeapon("weapon_tsgun")
  at map load.

**Verified RUNTIME (rcbot, ts_wyndham):** type 25→v_knife dmg 40; type 3→v_uzi dmg 22.

**Two crashes found+fixed via GDB:** (1) PRECACHE_EVENT must be at MAP LOAD → moved
weapon events into PrecacheAllTSWeapons. (2) DeathNotice strncmp segfault — pszName()
reads ItemInfoArray[m_iId], unpopulated after I'd dropped UTIL_PrecacheOtherWeapon;
restored in CWorld::Precache.

**Debugging note:** use **gdb** on our own hl.so for crashes (rig keeps debug syms;
`gdb -batch -ex run -ex bt --args ./hlds_linux ...`).

**SLICE limits:** shared weapon id 16 → one TS weapon at a time; per-weapon spread not
mapped; fire-type dispatch (grenade/knife/katana/akimbo) not implemented. Faithful
PickUpWeapon armament + buy menu TODO.

---

## 2026-06-21 (cont. 11) — CTSGroundWeapon (weapon pickups) + tsweapon_t fix

**FIXED tsweapon_t indices.** Real index = (lea offset − 0x7d30)/0x3DC, MATCHES the FGD
`tsweaponid`. beretta=2, uzi=3, mk23=9, knife=25, etc. Index 29 = flag (CTF).
**`dlls/ts_groundweapon.cpp` CTSGroundWeapon (ts_groundweapon, 65 placements)**: SLICE
gives weapon_tsgun; respawn after wduration. VERIFIED RUNTIME (rcbot, ts_wyndham).

---

## 2026-06-20 (cont. 10) — CTSGun recon + weapon data layer (vertical-slice start)

**CTSGun is a subsystem:** ONE unified CBasePlayerWeapon (id 16) morphing by tsweapon_t.
**tsweapon_t enum (38, from compile_weapons_info @0x6fe28):** 0 melee,1 glock18,2 uzi,
3 uzi_akimbo,4 benellim3,5 m4,6 mp5sd,7 mp5k,8 beretta_akimbo,9 beretta,10 mk23,
11 mk23akimbo,12 usas12,13 desert,14 ak47,15 fnh,16 aug,17 skorpion,18 barrett,19 pdw,
20 spas12,21 gold,22 glock22,23 mac10,24 m61,25 knife,26 mossberg,27 m16,28 ruger,
29 fnhakimbo,30 bull,31 m60,32 sawedoff,33 katana,34 sealknife,35 contender,
36 skorpion_akimbo,37 flag.
FAITHFUL CTSGun engine (items array m_pActiveItem@0xcc/m_pActiveInfo@0xd0/items[]@0x45,
fire-type dispatch s_weapon_info+0x348, akimbo/firemode/zoom) replaces // SLICE
incrementally. TS weapons precached at MAP LOAD (CWorld::Precache).

**Bot test rig (working, reusable):** ~/Containers/hlds/hlds-linux64-test:
- ts/addons/metamod/dlls/metamod_amd64.so (Metamod-R/build-amd64) as gamedll
  (liblist.gam gamedll_linux → it); config.ini = `gamedll dlls/hl.so`; plugins.ini =
  `linux addons/metamod/dlls/rcbot_mm_amd64.so`. <rig>/rcbot/* configs; min_bots 2.
Boot normally → rcbot bots spawn as real CBasePlayers. Temp StartFrame hook can
teleport/give to a bot to force a pickup/activation (revert after).

---

## 2026-06-20 (cont. 9) — TSHudMessageOne + CBasePlayer TS extensions (step 1)

`dlls/ts_utils.{h,cpp}` TSHudMessageOne. CBasePlayer TS member block (player.h):
m_iPowerups (0x724), m_iHeldPowerup (0x728), m_flPowerupTime (0x730), m_iKungFu (0x74e),
m_bLowGravity (0x74f) + accessors. (cont.13 added 0x72c/0x73c + PlayerAddPowerUp;
cont.14 added 0x175/0x17c/0x180 + the activation methods.)

**The core is a monolith.** Sequencing: (1) player block+accessors ✓; (2) CTSGun base ✓;
(3) PlayerAddPowerUp + CTSPowerUp ✓ (cont.13); (4) UsePowerUp/UpdatePowerUpState
activation ✓ (cont.14); NEXT: the slow-motion EFFECT (movement scaling + SlowMotionFlow
+ GlobalSlowMotion2 + per-entity virtuals + client motion-blur) and/or the faithful
weapon engine; fill CTSGameRules methods as needed.

---

## 2026-06-20 (cont. 8) — CTSGameRules skeleton installed + runtime-verified

**No CTSPlayer class** — TS extends stock CBasePlayer. `CTSGameRules : CHalfLifeMultiplay`
is the live g_pGameRules (InstallGameRules). GetRandomPowerup(mask) reconstructed. 40
other methods inherit MP. Caveat: tm_ teamplay maps use MP base; revisit when TS team
logic lands.

---

## 2026-06-20 (cont. 7) — CTSMapGlobals runtime-verified
`dlls/ts_mapglobals.cpp`. get_weapon_restriction() = 0x1f ^ spawnflags. (ts_trinityfall).

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
## 2026-06-20 (cont. 3) — CTSSlowMotion draft (not built; deps CTSGameRules+player).
## 2026-06-20 (cont. 2) — gnv2demangle.py (1788/2426 typed sigs) + DecompExport.java.
## 2026-06-20 (cont.) — Vino OK'd reconstruction; Ghidra pipeline (decompile.sh).
## 2026-06-20 — Scaffolding: fork halflife-updated@64bit-demo; reference/; tooling;
   work-list RECONSTRUCTION.md (60 TS classes). Builds via hlds64-buildchain image.

**Conventions:** NO Co-Authored-By trailer. CLAUDE.md local-only. Auto mode:
halt & ask on a block.
