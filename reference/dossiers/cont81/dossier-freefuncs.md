# FREE-FUNCTIONS DOSSIER — 43 MISSING free functions, reconcile plan, VERIFY list

Synthesized from the 5 group analyses in this scratchpad (each is the depth authority):

- `analysis_ff_gun.md` — TS_gun.cpp ×13 (ammo/weapon-param/spread/range-damage)
- `analysis_ff_util.md` — TS_util.cpp ×13 (CustomWP pipeline, precache, disconnect, slowed array)
- `analysis_ff_gamerules.md` — TS_gamerules.cpp ×9 (mode decide, respawn, spawn spots, Victim*Hold)
- `analysis_ff_pmshared.md` — TS_pm_shared.c ×3 (PseudoClimb / CanJumpHigh / FixVelocity)
- `analysis_ff_misc.md` — 5 singletons (TS_bullet / TS_grenades / TS_multiplay_gamerules / TS_player / TS_powerup)

Cross-references: `dossier-melee.md` (Victim*Hold, CheckHull, glast_* globals),
`dossier-gmsg.md` (TSServerSetting/TSKickAlign context), `plan-consolidation.md` (C1–C20
target layout — this dossier's landing files assume those commits execute first).
AUDIT source: `reference/coverage/AUDIT.md` §"MISSING free functions" (43, table matches
exactly: 13+13+9+3+1+1+1+1+1) and §"GLOBALS — MISSING".

**Count check: 13 (gun) + 13 (util) + 9 (gamerules) + 3 (pm_shared) + 5 (misc) = 43. ✔ All covered.**

---

## 0. MISSING-GLOBALS SIGHTINGS (per AUDIT hint: TSGameRules / objects / roundtime)

- **`TSGameRules`** (AUDIT: original file TS_player.cpp) — **SIGHTED**: B-symbol @0x1681a0,
  size **0x5894**. The rules object is a **GLOBAL INSTANCE**, not heap: `SetPlayerRespawnTime`
  reads `TSGameRules.state` (byte @+0, round-state) and `TSGameRules.m_flRoundEnd` (@+8)
  directly. Our tree heap-allocates via InstallGameRules and casts `g_pGameRules`
  (ts_gamerules.h:200 `TSGameRules()` accessor). Internal-layout divergence — external
  contract unaffected; keep ours, note the provenance, but the FIELDS it reads (+0
  round-state byte, +8 m_flRoundEnd, +0x5884 custom-weapons flag, +0x5888 health_absorb,
  +0x588c armor_absorb, +0x684 per-player 0x48-stride vote blocks) must exist as members.
- **`objects`** (b 0x195064) and **`roundtime`** (D 0x108e84, 0x14 bytes) — **NOT referenced
  by ANY of the 43 functions** in these groups (verified during the gamerules group's
  static caller pass). They belong to the CTSGameRules-class / TS_gamerulesclass.cpp side —
  out of scope here; leave on the class-parity work-list.
- New globals these functions DEFINE or REQUIRE (all currently absent from our tree):
  `g_gamemode` (D 0x113dc0, written by CWorld::KeyValue "gamemode" worldspawn key),
  `g_theOneMode` (D 0x113dc4), `g_lastManStanding` (D 0x113dc8) — DecideGamePlay writes,
  SetPlayerRespawnTime/TSGetPointsForFrag read;
  `glast_weaponid` (D 0x10100c, TS_bullet.cpp TU, static-init @0x5f688 → VERIFY init −1),
  `glast_closecombat` (D 0x104480, char) — TSGetPointsForFrag reads, melee cluster writes;
  `vecBoxes` (B 0x167f60, Vector[33], TS_bullet.cpp) — UTIL_TraceLine_Expanded scratch;
  `global_ammo_info` (B 0x176e00, s_ammo_info[16], stride 0x38),
  `range_damage_multipler` (B 0x177180, float[10000]),
  `gmsgTSCustomWeaponParams` (@0x1034c4, wire name **"CustomWP"**, size −1 — a
  swap-contract user message, also on the AUDIT missing-globals list);
  `g_pLastSpawn` — ours already exists (player.h:581).

---

## 1. SYSTEM OVERVIEW

### 1.1 The weapon-param / ammo pipeline (TS_gun.cpp + TS_util.cpp, SHARED source)

One-shot init chain at startup: `compile_weapons_info()` → `Write_<weapon>` ×38 →
**`compile_ammo_info()`** (fills `global_ammo_info[16]`, then `Set_ammo_count(0)`) →
**`compile_range_damage()`** (fills the 10000-entry falloff LUT). Runtime accessors:
**`get_ammo_info(cal)`** (bounds-clamped row pointer) and **`get_range_multipler(idx)`**
(clamped LUT read). Runtime mutation: **`Set_ammo_count(mode)`** rewrites only the
+0xc max-carry column when the `ammocount` server setting changes
(CTSGameRules::Think → gmsgTSServerSetting id 2 → client runs its own shared copy).

The runtime **customweapons** layer (TS_util.cpp): `adjust_weapon_value(file)` /
`save_weapon_value(file)` read/write the weapons config (5 tunables per weapon:
s_weapon_info `+0x28` accuracy, `+0x30` view kick, `+0x34` accuracy kick, `+0x1c`
damage, `+0xd8` cost — cost is a NEW column for our struct), plus `armor_absorb`
(gamerules +0x588c) / `health_absorb` (+0x5888). Changed params reach clients via
**`SendWeaponParams(id, pOnly)`** → user message **"CustomWP"** (one WRITE_STRING:
`"%i %f %f %f %f %i"` = id acc vkick akick dmg cost) and
**`InformClientsOnWeaponsParams(pOnly)`** (green TSHudMessage "This server is using
custom weapons." + all-38 loop) — called from CTSGameRules::InitHUD when the
gamerules +0x5884 "customized" flag is set, and from cmd_loadweapons after a load.
`AdjustSpecificWeapon` + `WeaponIdFromString` are the (Linux-)dead single-weapon
console variants.

⚠ Three live divergences found in OUR tables (analysis_ff_gun.md §Divergences):
(1) `kAmmoMax` is the **wrong column** (it holds the +0x20 range units, not the +0xc
reserve caps — our 9mm caps at 80 vs original 210 and `ammocount` does nothing);
(2) `kBulletSpeedUnit` is in **compile-write order, not caliber order** (slow-mo
bullets fly at the wrong speeds); (3) `TSRoundsPerMag` is verified CORRECT.

### 1.2 Range-damage falloff — THE known gameplay gap

`compile_range_damage`: `m[i] = clamp(1/sqrt(0.1·max(i,1)), 0, 1)` — full damage
for i ≤ 10, 0.707 @20, 0.5 @40, 0.316 @100. Two index forms at the two consumers
(CBulletManager::UpdateBullet + CBasePlayer::ShootBulletsPlayer, both TS_bullet.cpp):
**Form A** (gated on bulletrec+0x3c == 2, plausibly the ammo "kind" column):
`idx = dist/30` → full damage ≤ 300 units. **Form B** (else):
`idx = dist / a_info->rangeUnit(+0x20)` → full damage ≤ 10·rangeUnit (9mm 800u,
5.56 1500u, 50BMG 2000u, .454 200u). Our bullets currently do FLAT damage at any
distance (ts_bulletmgr.cpp SLICE note (4)) — the single biggest parity win here.

### 1.3 Melee-support free functions (cross-ref dossier-melee.md §1.2–1.3, §3 S3)

- **`Victim{Kungfu,Knife,Katana}Hold`** (TS_gamerules.cpp) — the block/deflect gates
  called from CloseCombatAttack/ColdCockAttack (kungfu), KnifeFire, KatanaFire, and
  CTSStunt::CheckForBreakables. Shared skeleton: victim IsPlayer + block-window member
  open (+0x7a8 kungfu / +0x7ac knife / +0x7b0 katana) → sparks (not kungfu) +
  knife_hit.wav / kungfuhit.wav (CHAN_BODY 1.0/0.8/100, TS 8-arg EMIT_SOUND_DYN
  trailing 0.5) → clear whiff-lunge (+0x7a0) → next-attack locks (katana 0.75/0.35,
  knife+kungfu 0.5/0.25) → `UpdateMeleeBar(attacker, 33.0)` (getting blocked costs
  the SWINGER 33 bar). Kungfu adds the **counter role-swap** (attacker blocking +
  victim mid-whiff-lunge → roles swap; bar gate stays on the ORIGINAL 2nd param —
  keep the asymmetry). Knife gate is asymmetric: knife-window blocks free (no bar);
  katana-window also parries a knife but needs bar > 0.1. NB dossier-melee's earlier
  0.75/0.35 quote for VictimKnifeHold is the KATANA pair — knife is 0.5/0.25
  (corrected in analysis_ff_gamerules.md §7).
- **`FindHullIntersectionplayer`** (TS_player.cpp) — byte-identical copy of
  crowbar.cpp's static `FindHullIntersection` (×2 reach, 2×2×2 corner loop, 1e6
  sentinel), renamed; sole caller = `CBasePlayer::CheckHull` (melee hit refinement,
  melee S3). The original binary contains BOTH copies — keep crowbar's static too.
- **`GetCloseCombatVectors`** (TS_gun.cpp) — `out = src + 32·forward`; **DEAD** in the
  .so (the 32u math is re-inlined in KnifeFire). Add uncalled.
- **`UTIL_TraceLine_Expanded`** (TS_bullet.cpp) — the "generous hitbox" trace: inflate
  every player's engine absmin/absmax by ±32, UTIL_TraceLine, restore **absmax only**
  (absmin-left-deflated quirk — reproduce, do not fix). Callers: UpdateBullet +
  ShootBulletsPlayer ×2. Ours uses plain traces at ts_bulletmgr.cpp:172 /
  ts_gun.cpp:1392 — a signature TS "feel" item.

### 1.4 Movement (TS_pm_shared.c — compiles into BOTH libs)

Original file order: FixVelocity, PseudoClimb, TSStandUp, TSGoDuck, TSWallJump,
TSWallJumpClimb, TSDive, TSHandle, CanJumpHigh (+ TS-forked PM_Jump etc.).
- **`PseudoClimb`** — the hold-jump wall-scramble: gated at the TSHandle HEAD by
  `(cmd.buttons & IN_JUMP) && onground == -1`; pitch window ±60°, looking up < −30°
  raises the head probe +40u; eye-level 64u probe must be CLEAR + feet-level 32u
  probe must be BLOCKED + speed < 0.5·maxspeed → `vel.z += 1600·frametime` +
  FixupGravityVelocity; always restores usehull + FixPlayerCrouchStuck(1) +
  CatagorizePosition. **The one real gameplay gap in this group** — our TSHandle has
  everything else in the right order but lacks this head call.
- **`CanJumpHigh`** — thin-obstacle probe (128u ducked-hull trace + 164u
  PointContents), **DEAD CODE** (zero callers; refutes the "real superjump gate"
  hint — original PM_Jump keeps the stock physinfo "slj" gate, which ours matches).
  Add uncalled, with the no-usehull-restore-on-success quirk.
- **`FixVelocity`** — empty `ret` stub, called after PM_FixupGravityVelocity in
  TSDive/TSWallJump/TSWallJumpClimb. Pure source-parity cosmetics.

### 1.5 Gamerules / spawn / scoring (TS_gamerules.cpp + friends)

- **`DecideGamePlay(char*, int)`** — the mode selector called by InstallGameRules:
  clears then sets `g_theOneMode`/`g_lastManStanding`; a one-shot `g_gamemode`
  worldspawn override (1 DM / 2 teamplay / 3 LMS / 4 TheOne, consumed on every path);
  else cvar priority `mp_theonemode` > `lastmanstanding` > `teamplay` > caller default.
  Contains a dead 3-char map-prefix copy — do NOT invent a check.
- **`SetPlayerRespawnTime(player, seconds, bool)`** — every respawn-wait path funnels
  here (InitHUD/Killed/ChangePlayerTeam). Normal: `+0x770 = time + seconds`. LMS with
  round running: respawns-used (+0x7ba) ≥ `respawnnumber` → +0x770 = −1 (never);
  else clamp to round end + set +0x7bc secondary ≤10 s deadline. Always sets +0x74c
  scoreboard class (11 WAITING / 1 out-for-round).
- **`TSSelectSpawnSpot(class, player, fallback)`** — TS's spawn scorer (prefers spots
  a FAR (≥128u) enemy can see: score bands 0x1801–0x2000 / 0x1001–0x1800 / minDist),
  32-spot cap, `flFraction==1` LOS, effects&0x80 filter, and the **score<128 ⇒ 300 dmg
  DMG_NEVERGIB world telefrag in a 128u sphere**. Sole caller EntSelectSpawnPoint.
- **`TSGetPointsForFrag`** (TS_multiplay_gamerules.cpp) — stylish-kill scoring: TheOne
  gate (only the One scores), kungfu/thrown-knife = 2 pts, stunt (+iuser4 0x10c10,
  speed>80) / slide (0x20, friction<1, speed>80) = +1, green
  `"%i #TS_Frg #TS_FrgFr %s, %s"` TSHudMessageAll broadcast; needs
  glast_weaponid/glast_closecombat/g_theOneMode + CTSKnife::GetDescription.
- **`DoSpectacularNess`** — RANDOM_LONG(0,100) ≤ clamp(spectacularness cvar, 0, 100),
  inclusive (keep the 101-value off-by-one); callers TSExplosion + PlayerKilled →
  slow-mo point drops.
- **`WeaponRestrictionByID(id)`** — preset→mask (1→0x0, 2→0x1, 3→0x3, 4→0x13,
  default InitWeaponRestriction()=0x1f); sole caller `cmd_weaponrestriction`
  (a swap-contract server COMMAND). Pairs with `GetWeaponType` (akimbo→base-category
  remap: 34→knife, 33→shotgun, akimbos→pistol/SMG) + `IsRestrictedWeapon(w, mask)`
  (category bits: 0x1 knives, 0x2 pistols, 0x4 SMGs, 0x8 rifles, 0x10 shotguns) —
  the BuyWeapon gate (ts_buy.cpp:143 SLICE stub cites it).
- **`GetRandomPosition(center)`** (TS_powerup.cpp) — powerup scatter: 16×22.5°
  sectors from a random start (RANDOM_LONG(0,16) inclusive), dist 128–1024, trace,
  back off 64u, reject CONTENTS_SOLID, 16 tries else return center. Ours spawns
  powerups ON the spawn pads instead.
- **`GetAccelBySlots`** — dead accessor (returns cached +0x548 slots); its live twin
  GetSpeedBySlots is already ours.

### 1.6 Player lifecycle / server utility (TS_util.cpp)

- **`TSPrecache()`** — CWorld::Precache's TS list (50 fixed sounds + per-weapon loop
  over FIVE model fields (+0xe8/+0x132/+0x172/+0x1b2/+0x1f2) + 2 sounds + the fixed
  tail incl. `UTIL_PrecacheOther("m61_grenade"/"shrapnel"/"ts_flag")`). Ours
  (`PrecacheAllTSWeapons`) misses ~30 assets → latent late-PRECACHE Host_Errors.
- **`TSPlayerDisconnect(player)`** — DropFlag, zero the 0x48-byte vote block, pev
  resets (SOLID_NOT/DEAD_DEAD/clear FL_FAKECLIENT/set FL_DORMANT/EF_NODRAW/frags=0),
  member resets (+0x770=−4, m_bTheOne=0, team string NUL, …), then
  `CallCheckDeads` (a departure can end a round). Ours: fragmented, vote block never
  cleared (rejoin inherits votes — live bug), CallCheckDeads not called.
- **`PlayerIsCheating(player, infobuf)`** — human + `cl_lw != 1` ⇒ true; caller
  ClientUserInfoChanged FORCES cl_lw back to "1" (no kick); TSInit calls + discards.
- **`TSCountPlayers(team, onlyActive)`** — team-string strcasecmp filter; "active" =
  alive OR respawn-pending (+0x770 ≥ 0). Ours (ts_escape.cpp inline) undercounts
  dead-but-respawning teammates → escape thresholds wrong in respawn rounds.
- **`UTIL_EntInArray` / `RemoveEntityFromSlowedArray`** — the packed
  `m_pSlowedEnts[32]`(+0x208)/count(+0x288) slowed-projectile cache: linear find;
  scrub-on-destroy called from 8 sites (CTSFlag::Destroy, CTSWorldGun ×3, CTSKnife ×2,
  CM61Grenade/CTSShrapnel WaitThink). Original has no EHANDLE — mimic the mechanism
  (memory `mimic-original-mechanism-for-dropin`) when projectile-GoSlow lands.
- **`TSFoundRandomEntity`** — dead (collect ≤32 by classname, pick random). Parity only.
- **`m61_grenade`** (TS_grenades.cpp) — the missing `LINK_ENTITY_TO_CLASS` factory
  export for our already-reconstructed CM61Grenade; also required by TSPrecache's
  `UTIL_PrecacheOther("m61_grenade")`. One-liner, contract-restoring.

---

## 2. PER-FUNCTION CONDENSED FACTS (43)

Format: name @nm-addr | signature | key constants | wiring (callers → ours). Depth: the group file.

### TS_gun.cpp (13) — analysis_ff_gun.md

| fn | @ | facts |
|---|---|---|
| `compile_ammo_info` | 0x6f51c | `void()`; fills global_ammo_info[16] (struct table in the analysis §"full table"); write order 1,5,7,2,9,4,6,8,3,10,11,12,13 (the kBulletSpeedUnit trap); ends `Set_ammo_count(0)`. Caller: compile_weapons_info @0x7005e. |
| `get_ammo_info` | 0x6f1c4 | `s_ammo_info*(int cal)`; `(unsigned)cal<16 ? &tbl[cal] : &tbl[0]`. 10 caller sites (UpdateBullet, ShootBulletsPlayer, RestartRound ×6, BuyAmmo ×5, ground/sniper/shotgun/rifle ammo spawns). |
| `Set_ammo_count` | 0x6f2cc | `void(int mode)`; writes ONLY +0xc column; mode 0/1/2/3 tables in analysis §3; default=mode 0. Callers: compile_ammo_info; CTSGameRules::Think @0x67714 (paired with TSServerSetting id 2). |
| `compile_range_damage` | 0x6f1f4 | `void()`; `m[i]=clamp(1/sqrt(0.1·max(i,1)),0,1)`, i<10000. Caller: compile_weapons_info @0x70063. |
| `get_range_multipler` | 0x6f290 | `double(int idx)`; clamp idx to [0,9999]. Consumers: UpdateBullet/ShootBulletsPlayer, Form A `dist/30` (gate +0x3c==2) / Form B `dist/rangeUnit(+0x20)`. |
| `CalcSpread` | 0x7020c | `void(float&, CBasePlayer*, float f)`; `spread *= clamp(|vel|/(100/f), 1, 3f)`. **DEAD server-side** (ScaleSpread superseded it). VERIFY client copy. |
| `DropKickback` | 0x76ea0 | `void(float*,float*,float,float)`; 2-axis kick→align decay, step `15·amt/FixedSlowMul(sf)`. **DEAD server-side**; producer VKickback IS live. VERIFY client copy. |
| `GetCloseCombatVectors` | 0x76e2c | `(int, Vector&, Vector&, Vector&, Vector&)`; `out = src + 32·forward`; a0/a4 unused. **DEAD** (math re-inlined in KnifeFire). |
| `GetBodyByItems` | 0x6f0a4 | `int(int type, int items)`; 5 body SHORTS @wi+0x128..0x130 (silencer/laser/flash/scope/laser+flash-combined); contender(36) no-scope → +1. Caller: CTSGun::Deploy @0x710c4. |
| `GetWeaponType` | 0x7705c | `char(tsweapon_t)`; specials 34→3, 33→1, 16/37→9, 10/30/8/21→8; else wi->fireType(+0x348). Callers: IsRestrictedWeapon, PlayerKilled, DropWeapon. |
| `IsWeaponAkimbo` | 0x7019c | `bool(int)`; fireType == 4. Callers: AddItem/TakeWeapon×2/GetActiveItems. Ours: inline `== TSFT_AKIMBO`. |
| `IsOnlyAkimbo` | 0x701d4 | `bool(int)`; fireType == 5. Callers: TakeWeapon/GetActiveItems. |
| `IsRestrictedWeapon` | 0x770c8 | `bool(tsweapon_t, int mask)`; w outside 1..37 → false; mask 0 → true; switch GetWeaponType: 1→&0x10, 3→&0x1, 6/7→&0x8, 8→&0x2, 9→&0x4; else false. Caller: BuyWeapon @0x84606. |

### TS_util.cpp (13) — analysis_ff_util.md

| fn | @ | facts |
|---|---|---|
| `UTIL_EntInArray` | 0x8e944 | `int(CBaseEntity**, int n, CBaseEntity*)`; linear find, −1 if absent. Callers: RemoveEntityFromSlowedArray, SlowMotionFlow (dedup for +0x208 array). |
| `WeaponIdFromString` | 0x8e9c0 | `int(char*)`; strcasecmp vs wi->name(+0x44), 0..0x25, else −1. **DEAD** (Linux). |
| `AdjustSpecificWeapon` | 0x8ea3c | `void(id, acc, vkick, akick, dmg, cost)`; writes the quintet (no clamps), SendWeaponParams(id,NULL), "%s behaviour changed", sets gamerules+0x5884. **DEAD** (Linux). |
| `SendWeaponParams` | 0x8eb98 | `void(id, CBaseEntity* pOnly)`; sprintf `"%i %f %f %f %f %i"` (id, +0x28, +0x30, +0x34, +0x1c, +0xd8) into 64B buf; MSG_ONE(pOnly)/MSG_ALL + one WRITE_STRING on **"CustomWP"**. Swap-contract wire format. |
| `InformClientsOnWeaponsParams` | 0x8eb14 | `void(pOnly)`; TSHudMessage(45,195,45, eff 8, hold 100, "This server is using custom weapons.") + SendWeaponParams loop 0..0x25. Callers: InitHUD (gated +0x5884), cmd_loadweapons. |
| `adjust_weapon_value` | 0x8eca4 | `int(char* file)`; gamedir+"/"+file, "rb"; per line: trim, skip "//" ANYWHERE; armor_absorb→+0x588c (≥0), health_absorb→+0x5888 (≥0); weapon line sscanf 6 fields, <6 → ALERT "You need to run saveweapons again.\n" + return 1; acc floor **0.001**; EOF → "weapons behaviour changed by %s\n" + flag +0x5884 + return 1. Caller: cmd_loadweapons ("customweapons"). |
| `save_weapon_value` | 0x8f044 | `int(char* file)`; "w"; exact 421-byte header comment block; per weapon `"//%s\n%i %f %f %f %f %i\n\n"`; footer saves **armor_absorb only** (health_absorb parseable but never written — keep asymmetry); "weapons behaviour saved to %s\n". Caller: cmd_saveweapons. |
| `TSCountPlayers` | 0x8f238 | `int(char* team, bool onlyActive)`; strcasecmp TeamID filter; active = IsAlive OR +0x770 ≥ 0. Caller: CTSTeamEscape::WaitTouch (team, true). |
| `TSPrecache` | 0x8f320 | `void()`; exact list+order in analysis §9 (50 sounds + weapon loop 5 models/2 sounds + fixed tail + PrecacheOther m61_grenade/shrapnel/ts_flag + knife/katana sounds). Caller: CWorld::Precache. |
| `TSPlayerDisconnect` | 0x8f94c | `void(CBasePlayer*)`; DropFlag(false); zero vote block (0x684+idx·0x48, 18 dwords); pev resets incl. clear FL_FAKECLIENT + set FL_DORMANT + EF_NODRAW; +0x770=−4, +0x7b9=0, +0xb48[0]=0, +0x160=0, +0x534=−4, +0x9b8=0, +0xb30=0; CallCheckDeads. Caller: ClientDisconnected. |
| `TSFoundRandomEntity` | 0x8fa64 | `CBaseEntity*(char*)`; ≤32 FindEntityByClassname, RANDOM_LONG pick. **DEAD** (Linux). |
| `PlayerIsCheating` | 0x8fae0 | `bool(CBasePlayer*, char* infobuf)`; +0x160 (bot) → false; else cl_lw != 1. Callers: ClientUserInfoChanged (forces cl_lw "1"), TSInit (result discarded — keep). |
| `RemoveEntityFromSlowedArray` | 0x8fb4c | `void(CBaseEntity*)`; for each player with +0x288>0: UTIL_EntInArray on +0x208, NULL found slots (holes kept, count unchanged). 8 caller sites (flag/worldgun×3/knife×2/grenade/shrapnel). |

### TS_gamerules.cpp (9) — analysis_ff_gamerules.md

| fn | @ | facts |
|---|---|---|
| `DecideGamePlay` | 0x667ac | `int(char*, int def)`; clears g_theOneMode/g_lastManStanding; g_gamemode 1/2/3/4 override (consumed, `g_gamemode=0` on EVERY path); else mp_theonemode > lastmanstanding > teamplay > def; dead 3-char mapname strncpy. Caller: InstallGameRules @0xb6100. |
| `DoSpectacularNess` | 0x66b48 | `bool()`; RANDOM_LONG(0,100) ≤ round(clamp(spectacularness,0,100)), and ≠0 guard; inclusive ≤. Callers: TSExplosion @0x6d964, PlayerKilled @0x7a3e0. |
| `GetAccelBySlots` | 0x66c4c | `int(CBasePlayer*)`; returns +0x548 cached slots. **DEAD**. Live twin GetSpeedBySlots already ours (ts_gun.cpp:121). |
| `SetPlayerRespawnTime` | 0x65ff4 | `void(CBasePlayer*, int sec, bool)` (bool dead); non-LMS/abnormal state → +0x770 = time+sec; LMS running: +0x7ba ≥ max(respawnnumber,1) → +0x770=−1; else clamp to m_flRoundEnd + set +0x7bc = time+min(roundLeft,10); always +0x74c = (+0x770<0) ? 1 : 11. Callers: InitHUD/Killed/ChangePlayerTeam. |
| `TSSelectSpawnSpot` | 0x66244 | `edict_t*(const char*, CBaseEntity*, const char*)`; fallback classname; ≤32 IsTriggered spots; LOS `flFraction==1` + relationship vcall(+0xcc) + dist≥128 → band 0x1801–0x2000; unseen → 0x1001–0x1800; else minDist (init 4096); no spots → info_player_deathmatch/start/NULL; **winning score<128 ⇒ 300 dmg DMG_NEVERGIB 128u-sphere telefrag**; sets g_pLastSpawn. Caller: EntSelectSpawnPoint @0x661ed. |
| `VictimKatanaHold` | 0x66c8c | `bool(att, vict, float)`; gates: IsPlayer + katana window +0x7b0>0 + bar +0x774>0.1; sparks midpoint+20z + knife_hit.wav; att +0x7a0=0, att lock 0.75, vict 0.35; UpdateMeleeBar(att, 33). Callers: KatanaFire @0x7397b, CheckForBreakables @0x8c462. |
| `VictimKnifeHold` | 0x66de4 | same skeleton; gate: knife window +0x7ac>0 (FREE, no bar) OR (katana window + bar>0.1); locks 0.5/0.25. Callers: KnifeFire @0x74311, CheckForBreakables @0x8c4be. |
| `VictimKungfuHold` | 0x66f50 | `bool(att, vict, float, TraceResult*)`; role resolution incl. COUNTER swap (att +0x7a8>0 && vict +0x7a4==0 && vict +0x79c>0); bar gate on ORIGINAL 2nd param (asymmetry, keep); kungfuhit.wav, NO sparks; both +0x7a0=0; locks 0.5/0.25; UpdateMeleeBar(post-swap att, 33); dead MakeVectors — do not invent knockback. Callers: CloseCombatAttack @0x8620a, ColdCockAttack @0x867dc, CheckForBreakables @0x8c27f. |
| `WeaponRestrictionByID` | 0x66ad8 | `int(int)`; 1→0x0, 2→0x1, 3→0x3, 4→0x13, default→InitWeaponRestriction() (0x1f). Caller: cmd_weaponrestriction @0xb5937 (swap-contract command name). |

### TS_pm_shared.c (3) — analysis_ff_pmshared.md

| fn | @ | facts |
|---|---|---|
| `PseudoClimb` | 0xe8fc8 | `void()`; pitch ±60°, up<−30° → head probe +40u; eye 64u clear + feet 32u blocked + speed<0.5·maxspeed → onground=−1, `vel.z += 100·frametime·16`; always restore usehull + FixPlayerCrouchStuck(1) + CatagorizePosition. Caller: TSHandle HEAD @0xe96ca gated IN_JUMP && onground==−1. |
| `CanJumpHigh` | 0xe97cc | `int()`; 128u ducked-hull trace + 164u PointContents (≠SOLID/SKY → 1, **usehull NOT restored on success**). **DEAD** — superjump-gate hint REFUTED (PM_Jump keeps stock slj). |
| `FixVelocity` | 0xe8fc4 | `void()`; empty. 3 callers, each right after PM_FixupGravityVelocity in TSWallJump/TSWallJumpClimb/TSDive. |

### Misc singletons (5) — analysis_ff_misc.md

| fn | @ | facts |
|---|---|---|
| `UTIL_TraceLine_Expanded` (TS_bullet.cpp) | 0x5f4f4 | `(const Vector&, const Vector&, IGNORE_MONSTERS, edict_t*, TraceResult*)`; ±32u absbox inflate per player (vecBoxes[33] save), stock trace, restore **absmax only**. Callers: UpdateBullet @0x5f0d2, ShootBulletsPlayer @0x5f878+0x5fcf0. Ours: plain traces ts_bulletmgr.cpp:172, ts_gun.cpp:1392. |
| `m61_grenade` (TS_grenades.cpp) | 0x6c850 | LINK_ENTITY_TO_CLASS factory export for CM61Grenade (engine dlsym + TSPrecache PrecacheOther). Ours: class exists, factory missing. |
| `TSGetPointsForFrag` (TS_multiplay_gamerules.cpp) | 0x79140 | `int(CBasePlayer* unused, entvars_t* killer, entvars_t* inflictor, int pts, bool* slide, bool* stunt)`; pts<0 passthrough; TheOne gate (+0x7b9); kungfu (glast_weaponid==−1 + no/bare active item or glast_closecombat) = 2, desc "#TS_KungFu"; inflictor≠killer: only "knife"-prefix classname → 2, desc GetDescription/"Throwing"; stunt iuser4&0x10c10 & speed>80 → +1 "#TS_StStl"; slide 0x20 & friction<1 & speed>80 → +1 "#TS_SlStl"; else " #TS_Stl" suffix; pts>1 → green TSHudMessageAll(0,255,0,1,30,"%i #TS_Frg #TS_FrgFr %s, %s"); return pts&0xff. Caller: CHalfLifeMultiplay::PlayerKilled @0x7970d. |
| `FindHullIntersectionplayer` (TS_player.cpp) | 0x85bac | = crowbar.cpp:85 static, renamed, `float*` mins/maxs; ×2 reach, 2×2×2 corners, 1e6 sentinel. Caller: CBasePlayer::CheckHull @0x85f60 (melee cluster S3). |
| `GetRandomPosition` (TS_powerup.cpp) | 0x874f4 | `Vector(Vector&)` (gcc2 sret); 22.5° ×16 sectors, start RANDOM_LONG(0,**16**), dist 128–1024, trace, endpos − dir·64, reject SOLID, 16 tries → center fallback. Caller: CTSGameRules::PutPowerUp @0x87a76. |

---

## 3. RECONCILE PLAN — ordered, build-green at every step

**Preconditions**: the consolidation plan (plan-consolidation.md) C1 (ts_bullet.cpp),
C4/C6 (TS_gamerulesclass.cpp + TS_gamerules.cpp scaffold), C18 (pm_shared/TS_pm_shared.cpp
scaffold) create the landing files. Steps below assume those exist; F1 (dead-code batch)
could run before them by parking in current files, but DON'T — land in final homes to keep
audit monotone. Gate for every step: `scripts/build.sh amd64` green (`all` when the client
TU is touched — F2/F3/F9 are shared-source), `audit.py` missing-free-fn count strictly
decreasing, `coverage.py all` external contract unchanged except where noted (F2 adds
"CustomWP"; F5 adds `weaponrestriction`/`loadweapons`/`saveweapons` commands; F4 adds the
`m61_grenade` factory — all flips of existing MISSING rows, never new EXTRA rows).

### F1 — dead-code + trivial batch (pure additions, zero behavior, NOT playtest-gated)

Clears 10 of 43 in one commit; every function compiled + exported, uncalled exactly like the original.
- `dlls/TS_gun.cpp` (current ts_gun.cpp): `CalcSpread`, `DropKickback`,
  `GetCloseCombatVectors`, `IsWeaponAkimbo`, `IsOnlyAkimbo` (+ mechanical swap of the
  inline `== TSFT_AKIMBO/_ONLY` comparisons at the sites matching the original callers —
  AddItem/TakeWeapon/GetActiveItems).
- `dlls/TS_gamerules.cpp` (C6 scaffold): `GetAccelBySlots` (thin forwarder to the
  TSPlayerFreeSlots computation; document derived-vs-cached divergence),
  `DoSpectacularNess` (function only; call sites land with TSExplosion/PlayerKilled work).
- `dlls/TS_util.cpp` (**new file**, add to dlls/CMakeLists.txt): `UTIL_EntInArray`,
  `WeaponIdFromString`, `TSFoundRandomEntity` (dead), `AdjustSpecificWeapon`
  (needs SendWeaponParams → move to F2 if you want F1 fully standalone; otherwise land
  F1's TS_util.cpp with a fwd decl and F2 immediately after).
- `pm_shared/TS_pm_shared.cpp` (C18 scaffold): `FixVelocity` (+ the 3 calls after
  PM_FixupGravityVelocity in TSDive/TSWallJump/TSWallJumpClimb — no-op, safe),
  `CanJumpHigh` (dead, non-static, keep the usehull quirk; do NOT wire into PM_Jump).
- `dlls/ts_grenades.cpp` (TS_grenades.cpp target after C3): **`m61_grenade`** =
  `LINK_ENTITY_TO_CLASS(m61_grenade, CM61Grenade);` — contract-restoring; verify
  `nm -D build-amd64/dlls/hl.so | grep m61_grenade`. (VERIFY V6 classname before
  changing CreateGrenade's "grenade" string — the factory add itself is safe.)

### F2 — ammo/param tables + CustomWP pipeline (absorb/rename; mostly behavior-preserving)

The big shared-source commit (BOTH libs — predicted-weapon gotcha, rebuild `all`):
1. `s_ammo_info` struct (0x38 layout as provenance comments, modern members) +
   `global_ammo_info[16]` + `compile_ammo_info()` + `get_ammo_info()` +
   `Set_ammo_count()` + `compile_range_damage()` + `get_range_multipler()` in
   TS_gun.cpp, chained from compile_weapons_info like the original.
2. **Absorb/retire**: `kRoundsPerMag` (correct → becomes +0x10 column),
   `kAmmoMax` (WRONG column — its values become the +0x20 range units; real caps = +0xc),
   `kBulletSpeedUnit` (reorder to caliber order → +0x24). Swap call sites:
   ts_gun.cpp:371 AddAmmo, hldmcompat.cpp:157, ts_groundammo.cpp:48, ts_buy.cpp BuyAmmo,
   ts_bulletmgr/ts_bullet speed reads.
3. Add `+0xd8 cost` (int) to `s_weapon_info` (recover per weapon via
   extract_weapons.py pattern) — unify with ts_buy pricing (VERIFY V7 first).
4. CustomWP: register `gmsgTSCustomWeaponParams = REG_USER_MSG("CustomWP", -1)`;
   `SendWeaponParams` + `InformClientsOnWeaponsParams` + `adjust_weapon_value` +
   `save_weapon_value` in TS_util.cpp; `cmd_loadweapons`/`cmd_saveweapons` in game.cpp;
   client `MsgFunc_CustomWP` (sscanf the 6-field line → g_weaponInfo fields + HUD flag);
   CTSGameRules members `m_bWeaponParamsCustom` (+0x5884) + `m_flHealthAbsorb` (+0x5888,
   member only until V8 resolves its reader); InitHUD gate + InformClients call.
   Wire format byte-faithful (drop-in critical). Server: Think ammocount change-detect →
   Set_ammo_count + TSServerSetting id 2 (coordinates with dossier-gmsg task #4).
- **Playtest-gated inside this step**: the reserve-cap fix (big carried-ammo buff — but
  it IS the original) and slow-mo bullet speeds. Everything else inert on stock servers.

### F3 — range-damage falloff wiring ★ (playtest-gated, biggest parity win)

After V1 (the +0x3c gate) is resolved: apply `get_range_multipler` in BOTH our bullet
damage paths (CBulletManager::UpdateBullet slow-mo step + TSFireBulletsPenetrate /
ShootBulletsPlayer hitscan) with Form A (`dist/30`, kind-2 gate) and Form B
(`dist/rangeUnit`). Ship together with **`UTIL_TraceLine_Expanded`** + `vecBoxes[33]`
in ts_bullet.cpp and swap the 2 (3 original) trace sites — both change every gunfight;
playtest as one "TS gunfeel" gate vs the live original.

### F4 — weapon categories + restriction (contract + gameplay)

`GetWeaponType` + `IsRestrictedWeapon` in TS_gun.cpp; `WeaponRestrictionByID` in
TS_gamerules.cpp; `cmd_weaponrestriction` in game.cpp (command NAME = swap contract);
mask storage in gamerules/mapglobals (VERIFY V9 where the original stores it); gate
BuyWeapon (replace the ts_buy.cpp:143 allow-all stub). Audit GetWeaponType's other
callers (PlayerKilled/DropWeapon) for category logic. Behavior-preserving at default
mask 0x1f; restriction path playtest with the console command.

### F5 — GetBodyByItems + Deploy body (visual)

Add the 5 body shorts to s_weapon_info (extract via the extract_weapons.py pattern),
implement GetBodyByItems, set body in CTSGun::Deploy + on attachment toggle
(useitems). Verify on-screen (silenced glock body). Behavior-preserving otherwise.

### F6 — gamerules mode/respawn/spawn (absorb + playtest)

1. `DecideGamePlay` + globals g_gamemode/g_theOneMode/g_lastManStanding in
   TS_gamerules.cpp; call from InstallGameRules; `gamemode` keyvalue in CWorld::KeyValue
   (VERIFY V10 exact key string). Replace point-of-use CVAR probes where the original
   reads the globals. Behavior-preserving default; theOne/LMS playtest-gated.
2. `SetPlayerRespawnTime` — **absorb** TSEnterSpectate's respawn computation (rename,
   keep TSEnterSpectate as caller); player members m_flRespawnAllowed(+0x770, carrier =
   m_flSpawnGateTime), m_iRespawnsUsed(+0x7ba), +0x7bc deadline (V11 consumer first);
   route Killed/InitHUD/ChangePlayerTeam. DM path must stay behavior-identical; LMS
   pieces playtest-gated.
3. `TSSelectSpawnSpot` — write fresh, re-point EntSelectSpawnPoint's DM branch (keep
   the shell). Keep score bands / 128u far-enemy rule / 300-dmg NEVERGIB telefrag /
   off-by-one score indexing exact. Playtest-gated (spawn feel). V12: vtbl +0xcc slot;
   V13: classnames passed.
4. `GetRandomPosition` in ts_powerup.cpp + PutPowerUp call (V14: re-audit our PutPowerUp
   vs @0x8797c — the +(0,0,16) and 128-sphere rejection may be invention). Playtest-gated.

### F7 — scoring (with melee-cluster globals)

`TSGetPointsForFrag` in the TS_multiplay_gamerules target (our multiplay_gamerules.cpp),
called from CHalfLifeMultiplay::PlayerKilled (V15: replace-vs-augment IPointsForKill +
what PlayerKilled does with the out-bools). **Sequence AFTER (or with) the melee
cluster's `glast_weaponid`/`glast_closecombat` writers** (dossier-melee.md §1.4) +
`g_theOneMode` (F6.1) + `CTSKnife::GetDescription`. Playtest-gated (2–3 pt kills +
broadcast).

### F8 — player lifecycle / util (mixed)

1. `TSPrecache` in TS_util.cpp, called from CWorld::Precache; shrink
   PrecacheAllTSWeapons to CTSGun::Precache's original ownership (cross-check
   precache.tsv). Fixes ~30 latent missing precaches. Mostly behavior-preserving.
   Depends on F1's m61_grenade factory (PrecacheOther).
2. `TSPlayerDisconnect` + wire from ClientDisconnected (move client.cpp:148 DropFlag
   into it); fixes the vote-block-rejoin bug + m_bTheOne leak; CallCheckDeads on leave =
   original behavior (lightly playtest-gated: rounds can now end on disconnect).
3. `PlayerIsCheating` + ClientUserInfoChanged cl_lw enforcement + vestigial TSInit call
   (V16: bot-flag carrier — +0x160 vs FL_FAKECLIENT for our bots).
4. `TSCountPlayers` — **absorb** the ts_escape.cpp:83–92 inline (semantics change:
   dead-but-respawning count; V17 = +0x770 meaning, resolved by F6.2's member work).
   Playtest-gated (escape thresholds).

### F9 — PseudoClimb (both libs, playtest-gated)

`PseudoClimb` into pm_shared/TS_pm_shared.cpp in original file position; add the head
call in TSHandle (`IN_JUMP && onground == -1`, BEFORE the STOP damp — order
load-bearing). Rebuild both libs. Playtest vs live original (crate scramble; −30°
look-up probe raise; 0.5·maxspeed cutoff). V18: STUCK_MOVEUP == 1.

### F10 — slowed-array parity (rides the projectile-GoSlow feature, playtest-gated)

`RemoveEntityFromSlowedArray` + adopt the packed `m_pSlowedEnts[32]`/count model
(+0x208/+0x288 — fix player.h:410's wrong "0x188" provenance comment) + the 8 scrub
call sites. Mimic the mechanism exactly (memory: mimic-original-mechanism-for-dropin).
UTIL_EntInArray (F1) becomes load-bearing here.

### Melee-owned (tracked in dossier-melee.md, listed here for the 43-count)

`Victim{Kungfu,Knife,Katana}Hold` (TS_gamerules.cpp) + `FindHullIntersectionplayer`
(ts_player.cpp, copy-don't-invent from crowbar.cpp:85, keep both copies) land as melee
step **S3** ("compiled, uncalled" = behavior-preserving), wired later by
KnifeFire/KatanaFire/CloseCombatAttack/ColdCockAttack/CheckForBreakables
(playtest-gated there). Use the CORRECTED knife locks 0.5/0.25 (not 0.75/0.35).

### Landing-file summary (respecting plan-consolidation targets)

| target file | functions |
|---|---|
| `dlls/ts_gun.cpp` (TS_gun.cpp) | compile_ammo_info, get_ammo_info, Set_ammo_count, compile_range_damage, get_range_multipler, CalcSpread†, DropKickback†, GetCloseCombatVectors†, GetBodyByItems, GetWeaponType, IsWeaponAkimbo, IsOnlyAkimbo, IsRestrictedWeapon |
| `dlls/TS_util.cpp` (NEW) | UTIL_EntInArray, WeaponIdFromString†, AdjustSpecificWeapon†, SendWeaponParams, InformClientsOnWeaponsParams, adjust_weapon_value, save_weapon_value, TSCountPlayers, TSPrecache, TSPlayerDisconnect, TSFoundRandomEntity†, PlayerIsCheating, RemoveEntityFromSlowedArray |
| `dlls/TS_gamerules.cpp` (C6 scaffold) | DecideGamePlay, DoSpectacularNess, GetAccelBySlots†, SetPlayerRespawnTime, TSSelectSpawnSpot, VictimKatanaHold, VictimKnifeHold, VictimKungfuHold, WeaponRestrictionByID |
| `pm_shared/TS_pm_shared.cpp` (C18 scaffold) | PseudoClimb, CanJumpHigh†, FixVelocity |
| `dlls/ts_bullet.cpp` (C1) | UTIL_TraceLine_Expanded |
| `dlls/ts_grenades.cpp` (TS_grenades.cpp) | m61_grenade |
| `dlls/multiplay_gamerules.cpp` (TS_multiplay_gamerules.cpp) | TSGetPointsForFrag |
| `dlls/ts_player.cpp` (TS_player.cpp) | FindHullIntersectionplayer |
| `dlls/ts_powerup.cpp` (TS_powerup.cpp) | GetRandomPosition |

† = dead/uncalled in the original Linux binary — add uncalled, never invent call sites.

### Playtest gates (grouped for efficient sessions vs the live original)

1. **Gunfeel** (F2 caps/speeds + F3 falloff + expanded traces) — range duels, slow-mo bullets.
2. **Restriction/buy** (F4) — weaponrestriction presets 1–4, customweapons load/save + CustomWP on join.
3. **Spawn/powerup/mode** (F6) — spawn placement + telefrag, powerup scatter, LMS/TheOne respawn limits.
4. **Scoring** (F7) — kungfu/knife/stunt/slide kill points + #TS_Frg broadcast.
5. **Movement** (F9) — hold-jump scramble parity.
6. **Melee blocks** (melee S3 wiring) + **slow-mo projectiles** (F10) — with the melee dossier gates.
7. **Lifecycle** (F8) — disconnect mid-round ends round; escape counts; cl_lw forcing.

---

## 4. CONSOLIDATED VERIFY LIST → live-trace (scripts/live.py) / refs (re.sh, needs GUI to release the `re` project lock) / extraction

| # | item | method |
|---|---|---|
| V1 | bulletrec+0x3c identity (ammo kind→Form A gate) + which ShootBulletsPlayer path carries which form | re.sh on UpdateBullet/ShootBulletsPlayer; or live.py watch on `damage_adjust.1154` @0x168184-region while shooting rifle vs pistol at range |
| V2 | Does client.dll call its copies of CalcSpread / DropKickback (recoil decay)? | dc.sh client / binmap client pair; live.py watch cl.viewangles decay |
| V3 | mp.dll references to the 3 Linux-dead util fns (WeaponIdFromString/AdjustSpecificWeapon/TSFoundRandomEntity) + CanJumpHigh client liveness | binmap.py / mpdll Ghidra project |
| V4 | s_ammo_info +0x00/+0x08/+0x14/+0x18/+0x1c meanings + +0x14 (8192 travel) consumer | re.sh refs on global_ammo_info readers |
| V5 | s_weapon_info +0xe8/+0x1b2 model fields + +0x306 second sound (silenced?) | extract_weapons.py extension |
| V6 | CreateGrenade classname "grenade" vs "m61_grenade" @0x7c874 | re.sh 0x8c874 (nm+0x10000) |
| V7 | ts_buy price source ≡ wi+0xd8 (customweapons must reprice buy menu) | code read + re.sh on BuyWeapon @0x84606 |
| V8 | gamerules +0x5888 health_absorb read site (TakeDamage health scaling?) | re.sh refs 0x5888-offset readers / TakeDamage decompile |
| V9 | where cmd_weaponrestriction stores the mask + 0x13 preset bit semantics | re.sh 0xc5937 (cmd_weaponrestriction) |
| V10 | CWorld::KeyValue "gamemode" exact key string | re.sh CWorld::KeyValue (write @0xe4d45) |
| V11 | +0x7bc secondary respawn-deadline consumer; +0x74c exact name/feed (pev->playerclass?); seconds each SetPlayerRespawnTime caller passes | re.sh refs; live.py break SetPlayerRespawnTime (binmap RVA) reading [esp+8] |
| V12 | TSSelectSpawnSpot gamerules vtbl slot +0xcc identity (PlayerRelationship?) | VTABLES.md lookup |
| V13 | EntSelectSpawnPoint's classnames/fallback args into TSSelectSpawnSpot | re.sh 0x761ed |
| V14 | original PutPowerUp @0x8797c: vecCenter passed, +z lift, 128-sphere rejection existence, return semantics | re.sh 0x9797c |
| V15 | PlayerKilled @0x7970d plumbing: TSGetPointsForFrag replaces or augments IPointsForKill; use of pbSlideKill/pbStuntKill | re.sh 0x8970d / dossier-melee L-list overlap |
| V16 | CBasePlayer +0x160 = is-bot flag (vs FL_FAKECLIENT); names for +0x534/+0x9b8/+0xb30 | members_re curation + disconnect-path refs |
| V17 | +0x770 exact semantics (respawn-allowed time; −4 sentinel) — pre-req for TSCountPlayers + SetPlayerRespawnTime | resolved together in F6.2; live.py read on a dead player |
| V18 | STUCK_MOVEUP macro == 1 (PM_FixPlayerCrouchStuck arg) | header read (trivial) |
| V19 | TS 8-arg EMIT_SOUND_DYN trailing 0.5 meaning (global, once) | re.sh on the TS EMIT_SOUND_DYN util |
| V20 | vote-block stride: our m_voteMap[32][32] vs original 0x48 (64-char names) | audit-level; fix when touching TSPlayerDisconnect |
| V21 | glast_weaponid init value (−1?) from _GLOBAL_.I.glast_weaponid @0x5f688 | objdump the static-init fn |
| V22 | our UpdateBulletTimeSlice ignore_glass flag vs original UpdateBullet's trace flags | re.sh UpdateBullet |

---

## 5. BLOCKERS

- **Ghidra `re` project lock**: several VERIFY items need `re.sh`/headless decompiles;
  blocked while the GUI holds `recovered/ghidra/re` (use the MCP read-only, or close the
  GUI program first). Static objdump paths in the analyses are lock-free ground truth.
- **Sequencing on other tasks**: F7 (TSGetPointsForFrag) blocks on the melee cluster's
  glast_* writers (task #3); F2's Set_ammo_count client mirror + CustomWP client hook
  coordinate with the gmsg task (#4); all landing files assume consolidation commits
  C1/C4/C6/C18 (task #6) executed first.
- **Playtest capacity** (task #2): gates 1–7 above need live-original side-by-side
  sessions; nothing else blocks code landing (behavior-preserving steps can merge
  ahead of playtests).
