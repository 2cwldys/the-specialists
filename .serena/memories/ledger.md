# mod-ts change ledger

Running log of reconstruction progress. Newest entries on top. Keep factual:
what changed, how verified, what's next. See `CLAUDE.md` for mission/method.

---

## 2026-06-21 (cont. 11) — CTSGroundWeapon (weapon pickups) + tsweapon_t fix

**FIXED tsweapon_t indices (cont.10 enum was WRONG).** The Write_<weapon> calls in
compile_weapons_info are NOT in table-index order. Real index = (lea offset −
0x7d30)/0x3DC, and it MATCHES the FGD `tsweaponid` choices exactly. Corrected
ts_weapons.h: beretta=2 (was 9), uzi=3, mk23=9, knife=25, etc. (full list in the
enum). Index 29 = flag (CTF; not in compile_weapons_info). Build self-consistent so
the beretta slice still worked, but the indices are now faithful + FGD-aligned.

**Done — `dlls/ts_groundweapon.cpp` CTSGroundWeapon (ts_groundweapon, 65 map placements)**
- KeyValue: tsweaponid->m_weaponType (this+0xa4), wduration->m_respawnTime,
  wextraclip, buyable. Spawn: precache + world model (get_weapon_info; null-guarded
  -> w_weaponbox fallback since only beretta data is filled) + SOLID_TRIGGER/
  MOVETYPE_TOSS + SetTouch(GetWeapon). Precache calls UTIL_PrecacheOther("weapon_
  tsgun") at MAP LOAD -> fixes the runtime-precache crash from cont.10.
- GetWeapon (touch): SLICE gives weapon_tsgun (faithful uses CBasePlayer::
  PickUpWeapon armament — deferred); respawn after wduration (hide -> Materialize).

**Verified RUNTIME (rcbot bots, ts_wyndham):**
- `Can't init ts_groundweapon` GONE (15 map weapons spawn) AND `Can't create entity:
  ts_groundweapon` GONE (rcbot's bot-arming path unblocked). No crash.
- Pickup works: temp hook teleported bots onto ground weapons -> "ts_groundweapon
  (type 25) picked up by Sean/Kyle" -> GiveNamedItem weapon_tsgun, no crash. (Temp
  hook + probe reverted; client.cpp clean.)
- SLICE limitation: gives beretta regardless of m_weaponType (CTSGun is hardcoded
  beretta + only beretta data filled). Honoring the real type needs CTSGun to take a
  weaponType + more Write_<weapon> data. Faithful PickUpWeapon (armament/buy) TODO.

---

## 2026-06-20 (cont. 10) — CTSGun recon + weapon data layer (vertical-slice start)

**Decision:** tackle the weapon subsystem as a VERTICAL SLICE (one weapon
end-to-end = beretta), then scale. (User chose this over workflow/solo-grind.)

**CTSGun is a subsystem, not a class:** CTSGun (2430 lines, 44 funcs) is ONE
unified CBasePlayerWeapon (weapon id 16) that morphs by tsweapon_t with fire
dispatch (Standard/Shotgun/Knife/Katana/Grenade/Akimbo), fire modes, zoom,
reload. Backed by: 38 Write_<weapon> stat initializers, a 988-byte s_weapon_info
(get_weapon_info @0x6f190 indexes table[w], w in 0..0x25), and CBasePlayer
armament (BuyWeapon/PickUpWeapon/DropWeapon/ThrowWeapon/AddItemToArmament).

**RECOVERED — tsweapon_t enum (38, real order from compile_weapons_info @0x6fe28):**
0 melee,1 glock18,2 uzi,3 uzi_akimbo,4 benellim3,5 m4,6 mp5sd,7 mp5k,
8 beretta_akimbo,9 beretta,10 mk23,11 mk23akimbo,12 usas12,13 desert,14 ak47,
15 fnh,16 aug,17 skorpion,18 barrett,19 pdw,20 spas12,21 gold,22 glock22,
23 mac10,24 m61,25 knife,26 mossberg,27 m16,28 ruger,29 fnhakimbo,30 bull,
31 m60,32 sawedoff,33 katana,34 sealknife,35 contender,36 skorpion_akimbo,37 flag.

**Done — `dlls/ts_weapons.{h,cpp}` (data layer, compiles + links)**
- `enum tsweapon_t` (TSW_*) + clean modern `s_weapon_info` (subset of the 988B
  orig — functional not byte-exact). Fields recovered from Write_beretta @0x5e4a8
  + the beretta strings: name/hudsprite/v_model/p_model/p_model_sil/w_model/
  fire/fire_sil + iClip=15, flDamage=29, flSpread=0.015, flRecoil=3, iFireRate=900
  (last 3 numeric semantics VERIFY).
- `get_weapon_info(w)` + compile_weapons_info() with ONLY beretta filled so far
  (others TODO). Built into hl.so.

**Done — `dlls/ts_gun.cpp` CTSGun spine (compiles, links, registers weapon_tsgun)**
- `CTSGun : CBasePlayerWeapon` (entity weapon_tsgun, id 16). Faithful GetItemInfo
  (@0x85dd0: slot 1, pos 2, id 16), Spawn (classname weapon_tsgun, world model,
  FallInit), Precache (events + beretta models). Deploy/PrimaryAttack/Reload/
  WeaponIdle are FUNCTIONAL-MINIMAL placeholders (stock CBasePlayerWeapon mechanics
  on the beretta, modeled on glock) so a TS weapon is holdable+fireable. Marked
  // SLICE; the FAITHFUL engine (dual-wield items array m_pActiveItem@0xcc /
  m_pActiveInfo@0xd0 / items[]@0x45, fire-type dispatch via s_weapon_info+0x348,
  weapon-param networking, CTSAnimation, akimbo/firemode/zoom) replaces them
  incrementally. PlayerAddPowerUp's MultiplyClips/PickUpWeapon hooks still pending.

**RUNTIME-VERIFIED with bots (CORRECTION to an earlier wrong claim).** The
buildchain DOES ship 64-bit metamod+rcbot (Metamod-R `-DBUILD_64BIT=ON`, rcbotold
amd64/aarch64). My earlier "rcbot is 32-bit/incompatible" was wrong — I hadn't
checked the Makefile. Raw pfnCreateFakeClient failed only because I didn't do the
proper connect flow; rcbot does it right and bots stay connected.

**Bot test rig (now working, reusable):** in ~/Containers/hlds/hlds-linux64-test:
- ts/addons/metamod/dlls/metamod_amd64.so (from Metamod-R/build-amd64)
- ts/addons/metamod/config.ini = `gamedll dlls/hl.so` (metamod loads OUR build)
- ts/addons/metamod/plugins.ini = `linux addons/metamod/dlls/rcbot_mm_amd64.so`
- ts/addons/metamod/dlls/rcbot_mm_amd64.so (from rcbotold/build_amd64)
- <rig>/rcbot/* = rcbot configs (from rcbotold/rcbot), bot_config.ini min_bots 2
- ts/liblist.gam gamedll_linux = addons/metamod/dlls/metamod_amd64.so
- Boot normally; rcbot auto-adds bots that spawn as real CBasePlayers.

**VERIFIED — CTSGun beretta deploy+fire in-engine:** via a temp StartFrame hook
(give weapon_tsgun to alive bots + SelectItem + IN_ATTACK; reverted) the bots
deployed and fired the reconstructed beretta: 4 Deploy events, **35 PrimaryAttack
fires** across 4 bots (clip 15->down, dmg 29), no crash. CTSGun is now
behaviour-verified, not just built.

**Real bug found+understood by the runtime test:** giving a weapon at runtime
crashed (Host_Error: PRECACHE_MODEL only allowed in spawn functions). TS weapons
must be precached at MAP LOAD. The temp test precached via UTIL_PrecacheOther in
ServerActivate; the FAITHFUL fix belongs with the weapon-distribution system
(buy menu / pickups) which precaches the roster at map load — TODO with that.
Also set m_iClip=wi->iClip in CTSGun::Spawn (SLICE) so it deploys loaded.

---

## 2026-06-20 (cont. 9) — TSHudMessageOne + CBasePlayer TS extensions (step 1)

**Done — `dlls/ts_utils.{h,cpp}`**
- `TSHudMessageOne(r,g,b,effect,holdtime,text,entvars* pPlayer)` — MSG_ONE variant
  of TSHudMessageAll (confirmed via objdump @0x668d4: same payload, MESSAGE_BEGIN
  MSG_ONE with the player edict). Built + linked. Needed by CTSPowerUp + others.

**Done — CBasePlayer TS member block + accessors (step 1 of player foundation)**
- `dlls/player.h`: mod-ts member block on CBasePlayer (in-class initializers):
  m_iPowerups (0x724 bitfield), m_iHeldPowerup (0x728), m_flPowerupTime (0x730),
  m_iKungFu (0x74e), m_bLowGravity (0x74f).
- `dlls/ts_player.{h,cpp}`: accessor free funcs (faithful): SlowMotion=&1,
  InfiniteClips=&2, SlowPause=&8, DoubleFireRate=&0x10, KungFu=m_iKungFu.
- Verified NO REGRESSION: server runs full duration, ts_model spawns, no crash.
  (Players don't spawn headless w/o a client, so accessors not behaviour-exercised.)
- PlayerAddPowerUp fully decoded @0x87bc8 but NOT yet implemented (needs CTSGun
  for grenade/infclip; the held/activation system comes with CTSPowerUp).

**Mapped the CBasePlayer TS-extension surface** (from objdump of the player free
funcs — these are pure additions to stock CBasePlayer, no subclass):
- player `+0x724` = powerups bitfield (m_iPowerups). Accessors are 1-liners:
  SlowMotion()=&1, InfiniteClips()=&2, SlowPause()=&8, DoubleFireRate()=&0x10
  (bits == TS_PWUP_*). @0x65fa8/fb8/fc8/fe4.
- player `+0x74e` (byte) = kung-fu state. KungFu() returns it. @0x65fd8.
- `PlayerAddPowerUp__FP11CBasePlayeriP10CTSPowerUp` (0x87bc8, 557B) writes a
  member block 0x724,0x728,0x72c,0x730,0x73c,0x74e,0x74f (bitfield + per-powerup
  timers) AND calls **CTSGun::MultiplyClips** + **CBasePlayer::PickUpWeapon
  (tsweapon_t)** + TakeArmor + EMIT_SOUND_DYN. => a FULL powerup application is
  entangled with the weapon stack (CTSGun).
- `SetGlobalSlowMotion__12CTSGameRulesff` @0x6961c; `InterpolateSlowMotion__Ff...`
  @0x8910c (small math helper).

**Conclusion — the core is a monolith.** CTSGameRules <-> CBasePlayer extensions
<-> CTSGun (weapons) <-> CTSPowerUp/CTSSlowMotion are mutually dependent; can't
cleanly finish one without the others. The 5 done entities were leaves. Suggested
sequencing for the core: (1) CBasePlayer TS member block + trivial accessors
[low-risk additive]; (2) CTSGun base (weapon stack); (3) PlayerAddPowerUp +
PutPowerUp -> CTSPowerUp; (4) GlobalSlowMotion2/SetGlobalSlowMotion -> 
CTSSlowMotion; fill CTSGameRules methods (Think/teams/objectives) as needed.

**Commit status:** session work (5 entities + gamerules + TSHudMessageOne + docs)
staged; commit pending USER GPG passphrase (commit.gpgsign=true, pinentry can't
prompt headless). Msg saved at /tmp/ts_commit_msg.txt.

---

## 2026-06-20 (cont. 8) — Foundation: CTSGameRules skeleton installed + runtime-verified

**Key structural finding:** there is **no CTSPlayer class** — TS extends the stock
`CBasePlayer` directly (added members/methods + free functions like
`PlayerAddPowerUp__FP11CBasePlayer...`). The classic HL-mod pattern: TS edits
player.cpp/player.h. CTSGameRules is a global *object* in the original
(`&TSGameRules`), huge (~22KB instance; offsets into the thousands), with
game-mode globals (`g_theOneMode`, `g_lastManStanding`).

**Done — `dlls/ts_gamerules.{h,cpp}`**
- `CTSGameRules : public CHalfLifeMultiplay` — a real, installable rules object.
  Skeleton: round/team/objective behaviour inherited from CHalfLifeMultiplay; the
  40 TS-specific methods get filled in incrementally as deps land.
- `GetRandomPowerup(int mask)` reconstructed faithfully (self-contained): collect
  permitted powerup bits from mask (skip unused 0x80; gate 0x100 low-gravity
  behind DAT_0017da25, zero-init so allowed), random one via RANDOM_LONG, else 0.
- TS_PWUP_* #defines + `TSGameRules()` = static_cast<CTSGameRules*>(g_pGameRules).
- Wired install: `dlls/gamerules.cpp` InstallGameRules returns `new CTSGameRules`
  for the (multiplayer-only) TS mod's deathmatch/teamplay path.

**Verified RUNTIME** (ts_kissmeyerbasic): probe `InstallGameRules -> CTSGameRules`;
CTSGameRules IS g_pGameRules; ts_model still spawns; no crash. Probe removed.
Caveat: teamplay tm_ maps now use CTSGameRules (=MP base) not CHalfLifeTeamplay;
revisit when TS team logic lands.

---

## 2026-06-20 (cont. 7) — CTSMapGlobals runtime-verified; CTSPowerUp dep-blocked

`dlls/ts_mapglobals.cpp` `CTSMapGlobals : CBaseEntity`. Invisible per-map config;
all config in spawnflags. Spawn: movetype NONE, solid NOT, UTIL_SetOrigin.
KeyValue no-op. get_weapon_restriction() = 0x1f ^ pev->spawnflags (orig
InitWeaponRestriction()=`return 0x1f`). Deferred custom Save/Restore. 9 maps.
VERIFIED RUNTIME (ts_trinityfall): `spawnflags=8 -> 0x17`, no crash.
KEY FINDING: cheap standalone entities exhausted; rest blocked on foundation.

---

## 2026-06-20 (cont. 6) — CTSModel + CTSFog; FIRST in-engine runtime verification

`dlls/ts_model.cpp` `CTSModel : CBaseAnimating`: animated studio prop =
generic-cycler pattern (cf. CCycler). **858 placements = most common TS entity.**
NOTE: recovered decompile TRUNCATED by Ghidra noreturn artifact at UTIL_SetSize;
real body via objdump of Spawn__8CTSModel (real .so addr 0x622e8; Ghidra +0x10000).
`dlls/ts_fog.cpp` `CTSFog`: Spawn=UTIL_Remove(this).

**Runtime rig (new capability):** 64-bit ReHLDS (QUIC build) at
~/Containers/hlds/hlds-linux64-test. ts/ = symlinks to original TS content
(~/Containers/hlds/mods/ts) + ts/dlls/hl.so = OUR build + liblist.gam
gamedll_linux="dlls/hl.so" (bypasses metamod). (Separate from-scratch reimagining
at ~/Projects/the-specialists — NOT authoritative for faithful sigs.)
Boot: `cd $RIG && LD_LIBRARY_PATH=.:$LD_LIBRARY_PATH timeout -s INT 30
./hlds_linux -game ts -condebug +developer 1 +sv_lan 1 +maxplayers 4
+map <map> >log 2>&1`. SIGINT not KILL (KILL drops stdout buf).
Probe in CTSModel::Spawn fired 108x = 108 ts_model on map; no crash.
`Can't init <class>` = engine marker for unimplemented TS class (neg control).

---

## 2026-06-20 (cont. 5) — CTSCustomTarget built (ts_customtarget point relay;
   all/random/sequential fire). `T ts_customtarget`. No stock map uses it.

## 2026-06-20 (cont. 4) — DECISION: functional/source-faithful, NOT byte drop-in
   (gcc2 old-ABI vs Itanium; engine loads via C interface; recovered offsets =
   provenance). Done: ts_utils TSHudMessageAll + ts_timer CTSTimer. glibc 2.14.

## 2026-06-20 (cont. 3) — CTSSlowMotion draft (not built; deps CTSGameRules+player)
## 2026-06-20 (cont. 2) — gnv2demangle.py (1788/2426 typed sigs) + DecompExport.java
## 2026-06-20 (cont.) — Vino OK'd reconstruction; Ghidra pipeline (decompile.sh)
## 2026-06-20 — Scaffolding: fork halflife-updated@64bit-demo; reference/; tooling;
   work-list RECONSTRUCTION.md (60 TS classes). Builds via hlds64-buildchain image.

**Conventions:** NO Co-Authored-By trailer. CLAUDE.md local-only. Auto mode:
halt & ask on a block.
