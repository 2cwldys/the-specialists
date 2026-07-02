# Group: TS_gamerules.cpp free functions (9 funcs)

Ground truth: ff_*.c decompiles in this scratchpad; callers resolved statically from
`ts.dis` (objdump of reference/ts_i386.so) by mapping every `call <Name>`/`call <Name@plt>`
site to its containing function via `nm` (lock-free; no Ghidra run). Ghidra addr = nm addr + 0x10000.

Shared member map used below (CBasePlayer unless noted; from members_auto.tsv + sibling melee
analyses analysis_updatemeleebar.md / analysis_closecombat.md / analysis_knifefire.md):

| offset | type | resolved meaning |
|---|---|---|
| +0x12c | float | m_flNextAttack (stock SDK player next-attack countdown; UpdateMeleeBar sets 1.5 on depletion) |
| +0x548 | int | remaining inventory slots (init 81=0x51; weapons deduct slotCost) — ours derives it (ts_gun.cpp TSPlayerFreeSlots) |
| +0x74c | int | scoreboard/player "class" state — set 0xb (=11, WAITING, our TS_SCORE_WAITING player.cpp:1338) or 1 (out-for-round). VERIFY exact name; likely feeds pev->playerclass/ScoreInfo |
| +0x770 | float | respawn-allowed time (gpGlobals->time based; -1.0f = never this round). TIME field misread as int by Ghidra |
| +0x774 | float | m_flMeleeBar — melee/block stamina 0..100 |
| +0x79c | float | whiff-lunge window |
| +0x7a0 | float | whiff-lunge force/damage (zeroed on any successful block) |
| +0x7a4 | char | whiff-lunge consumed flag |
| +0x7a8 | float | KUNGFU (bare-hands) block window |
| +0x7ac | float | KNIFE block window |
| +0x7b0 | float | KATANA block window |
| +0x7ba | char | respawns used this round (LMS counter) |
| +0x7bc | float | secondary respawn deadline = time + min(round-time-left, 10) — VERIFY name (LMS forced/last-chance spawn window) |
| CTSGameRules +0x0 | char | round-state byte (0 = round running; 2/3 = ending/restarting states) — VERIFY name |
| CTSGameRules +0x8 | float | m_flRoundEnd (curated members_re.tsv:22) |

Missing-globals sightings in THIS group (per task hint):
- **TSGameRules** (B, 0x1681a0, size **0x5894** — the rules object is a GLOBAL INSTANCE, not heap):
  read by SetPlayerRespawnTime (byte @+0 round-state, float @+8 m_flRoundEnd). Our tree instead
  heap-allocates via InstallGameRules and casts `g_pGameRules` (ts_gamerules.h:200 `TSGameRules()`).
- **g_gamemode** (D 0x113dc0, int): read+reset by DecideGamePlay; WRITTEN by `CWorld::KeyValue`
  (disasm @0xe4d45) — i.e. a **worldspawn keyvalue** ("gamemode", VERIFY key string) that forces the mode once.
- **g_theOneMode** (D 0x113dc4) / **g_lastManStanding** (D 0x113dc8): written by DecideGamePlay
  (cleared then set per mode), read by SetPlayerRespawnTime (g_lastManStanding). Neither exists in our tree.
- **g_pLastSpawn** (B 0x16da34): written by TSSelectSpawnSpot. Ours exists (player.h:581).
- `roundtime` (D 0x108e84, 0x14 bytes) and `objects` (b 0x195064): **not referenced** by any function
  in this group (they belong to the CTSGameRules-class side / TS_gamerulesclass.cpp).

---

## 1. DecideGamePlay(char*, int) @ nm 0x667ac (Ghidra 0x767ac)

**SIGNATURE**: `int DecideGamePlay(char *mapname, int defaultTeamplay)` (`DecideGamePlay__FPci`).
Returns the teamplay decision (1 = team-based, 0 = FFA) and sets the mode globals as a side effect.

**BEHAVIOR** (jumptable was UNRECOVERED by Ghidra — recovered by hand from ts.dis; table @file 0x66800,
entries are `GOT(0x165e10) − entry`):
1. `g_theOneMode = 0; g_lastManStanding = 0;` (always cleared first).
2. If `g_gamemode > 0` (one-shot forced override, set by CWorld::KeyValue):
   - case **1** → return 0 (forced plain DM)
   - case **2** → return 1 (forced teamplay)
   - case **3** → `g_lastManStanding = 1`, return 0 (forced LMS)
   - case **4** → `g_theOneMode = 1`, return 1 (forced The One — note: theOne returns 1)
   - >4 → return 0.
3. Else (cvar-driven): `strncpy(local, gpGlobals->mapname, 3)` — the 3-char map prefix is copied but
   **NEVER READ** (dead code in the shipped binary; likely a vestigial prefix check, do NOT invent one).
   Then priority order:
   - `mp_theonemode` != 0 → `g_theOneMode = 1`, return 1;
   - else `lastmanstanding` != 0 → `g_lastManStanding = 1`, return 0;
   - else `teamplay` != 0 → return 1;
   - else → return `defaultTeamplay` (the caller's passed-in default, unchanged).
4. Epilogue on EVERY path: `g_gamemode = 0` (the override is consumed once).
   The `char*` first param is never read (dead param; presumably the map name).

**CALLERS**: `InstallGameRules__Fv` @0xb6100 (gamerules.cpp in the original). VERIFIED static.

**OURS**: no equivalent function. `InstallGameRules` (dlls/gamerules.cpp:396) sets
`g_teamplay = (teamplay.value > 0)` only; theOne/LMS are probed via `CVAR_GET_FLOAT` at each use
site (dlls/multiplay_gamerules.cpp:281–283 GetGameDescription, and scattered). No `g_gamemode`
worldspawn override, no `g_theOneMode`/`g_lastManStanding` globals.

**RECONCILE**: write fresh into the to-be-created `dlls/TS_gamerules.cpp` (consolidation plan):
add the three globals + DecideGamePlay with the exact priority order + one-shot g_gamemode consume;
call it from InstallGameRules to set g_teamplay; add the `gamemode` keyvalue to CWorld::KeyValue
(VERIFY the exact key string by decompiling CWorld::KeyValue in the refs pass). Replace point-of-use
CVAR probes for mode detection with the globals where the original reads them (SetPlayerRespawnTime
reads g_lastManStanding). Behavior-preserving for default servers; playtest-gated for theOne/LMS.
VERIFY: whether the original passes mapname or something else as arg0 (dead either way).

---

## 2. DoSpectacularNess() @ nm 0x66b48

**SIGNATURE**: `bool DoSpectacularNess(void)` (`DoSpectacularNess__Fv`).

**BEHAVIOR**: the stunt/kill style-points dice roll.
1. `roll = RANDOM_LONG(0, 100)`.
2. `chance = clamp(spectacularness.value, 0, 100)` (cvar; our default "1" per ts_cvars.cpp:38 —
   binary-derived, i.e. ~1% by default).
3. return `round(chance) != 0 && roll <= round(chance)`.
   Note both the `!= 0` guard (cvar 0 disables entirely) and the **inclusive `<=` over 0..100**
   (a 101-value roll — chance N gives (N+1)/101 probability; keep the off-by-one faithful).

**CALLERS** (static, verified): `TSExplosion(Vector, entvars_s*, entvars_s*, float, int, float, float)`
@0x6d964 and `CHalfLifeMultiplay::PlayerKilled(CBasePlayer*, entvars_s*, entvars_s*)` @0x7a3e0.
So both explosion kills and regular kills roll it — on success they drop the brief slow-motion
point (CTSSlowMotionPoint) / style bonus (see those functions' own dossiers).

**OURS**: MISSING. dlls/ts_grenades.cpp:209 explicitly defers it ("drops a brief CTSSlowMotionPoint
on a 'spectacular' kill via DoSpectacularNess — deferred"). Cvar IS registered
(dlls/ts_cvars.cpp:38 `spectacularness` default "1", FCVAR_SERVER).

**RECONCILE**: write fresh (4-liner) in TS_gamerules.cpp; then wire the two call sites when
TSExplosion / PlayerKilled are reconciled (their dossiers). Behavior-preserving to add the function;
the call-site wiring is playtest-visible (spawns slowmo points on kills) → playtest-gated.

---

## 3. GetAccelBySlots(CBasePlayer*) @ nm 0x66c4c

**SIGNATURE**: `int GetAccelBySlots(CBasePlayer *pPlayer)` (`GetAccelBySlots__FP11CBasePlayer`).

**BEHAVIOR**: trivial accessor — returns `pPlayer->+0x548` (the cached remaining-inventory-slots
int, init 81). Despite the "Accel" name it returns raw slots (its sibling
`GetSpeedBySlots(CBasePlayer*)` @0x66bd4 does the 210 + (120/81)*slots speed math — already
reconstructed at dlls/ts_gun.cpp:121).

**CALLERS**: **NONE in the binary** (only its own PIC-prologue self-call; scanned every
`call` in ts.dis). It is exported but dead — probably a leftover twin of GetSpeedBySlots.
VERIFY only if the refs pass finds an indirect/function-pointer use (none expected).

**OURS**: equivalent live logic exists as `TSPlayerFreeSlots(CBasePlayer*)`
(dlls/ts_gun.cpp:137–147) which derives remaining slots from `CTSGun::SlotsUsed()`
(ts_gun.cpp:280) instead of caching at +0x548; used by CTSStunt dive gating.

**RECONCILE**: for name-driven audit parity add `GetAccelBySlots` as a thin alias in
TS_gamerules.cpp (forward to the derived computation) OR record it as intentionally
dead. Zero gameplay impact (dead in the original) — behavior-preserving. Do NOT rename
TSPlayerFreeSlots' semantics; our derived-vs-cached divergence is internal-only
(the external contract is unaffected).

---

## 4. SetPlayerRespawnTime(CBasePlayer*, int, bool) @ nm 0x65ff4

**SIGNATURE**: `void SetPlayerRespawnTime(CBasePlayer *pPlayer, int seconds, bool a2)`
(`SetPlayerRespawnTime__FP11CBasePlayerib`). The `bool` param is **never read** in the
decompile (dead — VERIFY against callers' intent in the refs pass; do not invent a use).

**BEHAVIOR**:
1. Gate (de-morganed): if `TSGameRules.state(+0) != 0` (round not in normal play — states 2/3
   explicitly listed) **or** `g_lastManStanding == 0`:
   → normal respawn: `+0x770 = gpGlobals->time + seconds`.
2. Else (**LMS, round running**):
   - `lim = max(respawnnumber.value, 1)` (cvar, our default "1").
   - if `(float)(char)pPlayer->+0x7ba >= lim` (respawns used up) → `+0x770 = -1.0f`
     (raw 0xBF800000; sentinel: no respawn this round).
   - else: `+0x770 = time + min(seconds, TSGameRules.m_flRoundEnd(+8) − time)` (never respawn
     after the round ends), and `+0x7bc = time + min(m_flRoundEnd − time, 10.0)` (a second,
     ≤10 s deadline — VERIFY its consumer; looks like the "last chance to spawn into this
     round" window).
3. Always: `+0x74c = (+0x770 < 0) ? 1 : 0xb` — player class/state: 1 = out-for-the-round,
   0xb (11) = WAITING (matches our TS_SCORE_WAITING scoreboard value, player.cpp:1338).

Constants: min respawnnumber 1; round-end clamp; 10.0 s cap on +0x7bc; −1.0 sentinel; class 11/1.

**CALLERS** (static, verified): `CTSGameRules::InitHUD` @0x6956b (join),
`CBasePlayer::Killed` @0x7efe3 (death), `CHalfLifeTeamplay::ChangePlayerTeam` @0xd4c9c
(team switch). Every path into "waiting to respawn" funnels through here.

**OURS**: partial-equivalent `CBasePlayer::TSEnterSpectate` (dlls/player.cpp:1352–1378):
sets `m_flSpawnGateTime = time + clamp(respawntime cvar, 3, 30)` and playerclass WAITING=11.
Gaps: (a) our gate seconds come from the `respawntime` cvar instead of a caller-passed value;
(b) NO LMS respawn-count limit (`respawnnumber` registered ts_cvars.cpp:39 but unused, no
+0x7ba counter member); (c) no round-end clamp (needs TSGameRules.m_flRoundEnd — our
CTSGameRules round timer skeleton exists, ts_gamerules.cpp StartRoundTimer); (d) no −1/never
sentinel nor the class=1 out-for-round scoreboard state; (e) no +0x7bc secondary deadline.

**RECONCILE**: move/rename — extract the respawn-time computation out of TSEnterSpectate into a
faithful `SetPlayerRespawnTime(CBasePlayer*, int, bool)` in TS_gamerules.cpp; add player members
for +0x770 (respawn-at time — our m_flSpawnGateTime is the natural carrier), +0x7ba respawn
counter, +0x7bc deadline (VERIFY consumer first), and route Killed/InitHUD/ChangePlayerTeam
callers through it. LMS pieces are playtest-gated (LMS not yet playable end-to-end); the DM
path must stay behavior-preserving with today's fire-to-play flow. VERIFY: name/consumer of
+0x74c (class) and +0x7bc; what `seconds` each caller passes (refs pass / live trace).

---

## 5. TSSelectSpawnSpot(const char*, CBaseEntity*, const char*) @ nm 0x66244

**SIGNATURE**: `edict_t* TSSelectSpawnSpot(const char *classname, CBaseEntity *pPlayer,
const char *fallbackClassname)` (`TSSelectSpawnSpot__FPCcP11CBaseEntityT0`). Returns
`pSpot->pev->pContainingEntity` (entvars+0x208) = the spot's edict.

**BEHAVIOR** — TS's replacement spawn-spot scorer (prefers spawning IN VIEW of a distant enemy):
1. If `fallbackClassname` given and no entity of `classname` exists → use the fallback name.
2. Collect up to **32** spots of that class passing `pSpot->IsTriggered(pPlayer)` (vtbl call).
3. Per spot, scan all "player" entities that are: not self, `IsPlayer()`, `IsAlive()`, and
   visible (`!(pev->effects & 0x80)` i.e. not EF_NODRAW/observer).
   - LOS test (only while no "watcher" found yet): `UTIL_TraceLine(spot->origin,
     player eye (origin+view_ofs), ignore_monsters, ignore_glass)`; if `flFraction == 1.0`
     (clear LOS):
     - if `g_pGameRules->vtbl[+0xcc](pPlayer, other) == 1` **and** dist(spot, other) >= **128**
       → flag `farEnemySeesSpot` (+0xcc/4 = slot 51 — VERIFY which method; by its
       `(pPlayer, other)` shape it is PlayerRelationship/FPlayerCanTakeDamage-like; check
       VTABLES.md in the refs pass).
     - else (LOS but hostile-check failed or < 128 away) → flag `spotWatched` (bad).
   - Track `minDist` = nearest such player distance (init 0x1000 = 4096).
4. Score per spot (stored off-by-one at `scores[i+1]`, consistently read back the same way):
   - `farEnemySeesSpot && !spotWatched` → `RANDOM_LONG(0x1801, 0x2000)` (6145–8192, best tier:
     spawn into the action, visible to a FAR enemy);
   - `!spotWatched` → `RANDOM_LONG(0x1001, 0x1800)` (4097–6144, unseen spot);
   - else → `minDist` (≤4096; farthest-from-players fallback).
   Pick the max-scoring spot.
5. If NO candidate spots at all: return first `info_player_deathmatch` else `info_player_start`
   else NULL (0).
6. **Telefrag**: if the winning score < **128** (i.e. it was a distance score with a player
   within 128 units): every alive visible player ≠ self within a **128-radius sphere** of the
   spot takes `TakeDamage(VARS(INDEXENT(0)), VARS(INDEXENT(0)), 300.0, 0x1000)` — 300 damage
   from the WORLD with **DMG_NEVERGIB** (0x1000).
7. `g_pLastSpawn = pSpot`; return the spot's edict.

**CALLERS** (static, verified): `EntSelectSpawnPoint(CBaseEntity*)` @0x661ed only — the stock
SDK chooser delegates to this in TS (presumably passing "info_player_deathmatch"/team spot
names + fallback; confirm in the EntSelectSpawnPoint dossier / refs pass).

**OURS**: MISSING — dlls/player.cpp:3047 `EntSelectSpawnPoint` is the STOCK HL algorithm
(nearest-cycling + stock telefrag). Same g_pLastSpawn global exists (player.h:581).

**RECONCILE**: write fresh `TSSelectSpawnSpot` in TS_gamerules.cpp and re-point our
EntSelectSpawnPoint's deathmatch branch at it (keep the stock function shell — the original
kept EntSelectSpawnPoint too). Keep the quirks EXACT: the 32-spot cap, the score bands
0x1001..0x1800 / 0x1801..0x2000, the >=128 far-enemy rule, `flFraction==1`,
effects&0x80 filter, score<128 ⇒ 300 dmg DMG_NEVERGIB world telefrag, and the off-by-one
score indexing (self-consistent as write+read agree — reproduce the logic; it is not a bug
observable externally). Playtest-gated (spawn feel is very visible). VERIFY: vtbl slot
+0xcc identity; the exact classnames EntSelectSpawnPoint passes.

---

## 6. VictimKatanaHold(CBasePlayer*, CBaseEntity*, float) @ nm 0x66c8c

**SIGNATURE**: `bool VictimKatanaHold(CBasePlayer *pAttacker, CBaseEntity *pVictim, float flDamage)`
(`VictimKatanaHold__FP11CBasePlayerP11CBaseEntityf`). Third param (the incoming swing damage)
unused in the body. Returns 1 = attack was BLOCKED (caller aborts damage).

**BEHAVIOR** (katana-swing-vs-blocking-victim):
1. Gates: victim `IsPlayer()`; victim **katana block window +0x7b0 > 0**; victim melee bar
   **+0x774 > 0.1**. (A knife-block window does NOT stop a katana.)
2. Effects: `UTIL_Sparks(midpoint(attacker.origin, victim.origin) + (0,0,20))`;
   `EMIT_SOUND_DYN(attacker edict, CHAN_BODY(4), "weapons/knife/knife_hit.wav", vol 1.0,
   attn 0.8, flags 0, pitch 100, 0.5)` — note the TS 8-arg EMIT_SOUND_DYN (trailing float
   0.5, signatures-gcc2.txt; VERIFY its meaning — TS-modified util).
3. State: `pAttacker->+0x7a0 = 0` (whiff-lunge force cleared);
   `pAttacker->m_flNextAttack(+0x12c) = 0.75` (attacker stunned 0.75);
   `pVictim->+0x12c = 0.35` (blocker locked 0.35);
   `CBasePlayer::UpdateMeleeBar(pAttacker, 33.0)` — the **ATTACKER's** bar drains 33
   (getting blocked costs the swinger stamina; UpdateMeleeBar dossier).
4. Return 1.

**CALLERS** (static, verified): `CTSGun::KatanaFire` @0x7397b and
`CTSStunt::CheckForBreakables` @0x8c462 (stunt barges can be katana-blocked too).

**OURS**: MISSING as such — dlls/ts_gun.cpp:1177 has only the KUNG-FU block path inside the
melee attack (time-based `m_flKungFuBlock`), and the block bar lives on the GUN
(`m_blockBar` ts_gun.h:148) not the player. No katana window (+0x7b0), no attacker-bar
drain, no 0.75/0.35 next-attack split, no sparks/knife_hit on block.

**RECONCILE**: write fresh in TS_gamerules.cpp per the melee dossier's consolidated plan
(analysis_closecombat.md / analysis_knifefire.md reconcile steps): move the block bar to
CBasePlayer(+0x774), add the three block windows, then call this from KatanaFire and
CheckForBreakables. Playtest-gated (melee feel). Constants to keep exact: bar>0.1 gate,
33.0 drain, 0.75/0.35 locks, +20z sparks, CHAN_BODY/1.0/0.8/100/(0.5).

---

## 7. VictimKnifeHold(CBasePlayer*, CBaseEntity*, float) @ nm 0x66de4

**SIGNATURE**: `bool VictimKnifeHold(CBasePlayer *pAttacker, CBaseEntity *pVictim, float flDamage)`
(`VictimKnifeHold__FP11CBasePlayerP11CBaseEntityf`). flDamage unused. Returns 1 = blocked.

**BEHAVIOR** — same skeleton as KatanaHold with a different gate and softer locks:
1. Gates: victim `IsPlayer()`; then **(knife window +0x7ac > 0)** — unconditional block, NO
   bar check — **or (katana window +0x7b0 > 0 AND bar +0x774 > 0.1)**. I.e. a knife-block
   stops a knife for free; a katana-block also parries a knife but needs bar.
2. Same sparks (+20z midpoint) + same "weapons/knife/knife_hit.wav" EMIT_SOUND_DYN
   (CHAN_BODY, 1.0, 0.8, 0, 100, 0.5).
3. `pAttacker->+0x7a0 = 0`; `pAttacker->+0x12c = 0.5` (raw 0x3f000000);
   `pVictim->+0x12c = 0.25` (raw 0x3e800000); `UpdateMeleeBar(pAttacker, 33.0)`.
   (Knife pair is 0.5/0.25 vs katana's 0.75/0.35 — NB analysis_knifefire.md quoted
   0.75/0.35 for this function; those are the KATANA values. Correct when merging dossiers.)
4. Return 1.

**CALLERS** (static, verified): `CTSGun::KnifeFire` @0x74311 and
`CTSStunt::CheckForBreakables` @0x8c4be.

**OURS**: MISSING (same situation as KatanaHold; ts_gun.cpp melee has no knife/katana block
windows at all — analysis_knifefire.md:188–190).

**RECONCILE**: write fresh alongside KatanaHold; wire into KnifeFire + CheckForBreakables.
Playtest-gated. Keep the asymmetric gate (free knife-block vs bar-gated katana-block) exact.

---

## 8. VictimKungfuHold(CBasePlayer*, CBaseEntity*, float, TraceResult*) @ nm 0x66f50

**SIGNATURE**: `bool VictimKungfuHold(CBasePlayer *pAttacker, CBaseEntity *pVictim,
float flDamage, TraceResult *ptr)` (`…fP11TraceResult`). flDamage and ptr unused in the
recovered body. Returns 1 = blocked/countered.

**BEHAVIOR** — bare-hands block WITH the counter role-swap:
1. Victim `IsPlayer()` required.
2. Role resolution:
   - if `pVictim->+0x7a8 > 0` (victim's kung-fu block window open) → blocker = victim.
   - else if `pAttacker->+0x7a8 > 0` (ATTACKER blocking) AND `pVictim->+0x7a4 == 0`
     (victim's lunge not consumed) AND `pVictim->+0x79c > 0` (victim mid-whiff-lunge)
     → **COUNTER**: blocker = original attacker, attacker = original victim (roles swap).
   - else → return 0.
3. Computes normalized (attacker − blocker) direction and `UTIL_MakeVectors(blocker->pev->v_angle)`
   — results are then UNUSED in the shipped code (dead/optimized-out knockback; do NOT invent
   a knockback here).
4. Gate: `pVictim(param a1)->+0x774 >= 0.1` — NB checked on the ORIGINAL second param even
   after the role swap (in the counter branch that is the lunger's bar, not the blocker's —
   keep the asymmetry faithful).
5. Effects/state: `EMIT_SOUND_DYN(attacker edict, CHAN_BODY(4), "player/kungfuhit.wav",
   1.0, 0.8, 0, 100, 0.5)` (NO sparks for kung-fu); `blocker->+0x7a0 = 0` and
   `attacker->+0x7a0 = 0` (both lunges cleared); `attacker->+0x12c = 0.5`;
   `blocker->+0x12c = 0.25`; `UpdateMeleeBar(attacker, 33.0)` (post-swap attacker pays).
6. Return 1.

**CALLERS** (static, verified): `CBasePlayer::CloseCombatAttack` @0x8620a,
`CBasePlayer::ColdCockAttack` @0x867dc, `CTSStunt::CheckForBreakables` @0x8c27f.

**OURS**: PARTIAL — dlls/ts_gun.cpp:1177–1186 implements a simplified kung-fu block inside
the melee attack path ("attacker stunned (VictimKungfuHold)" — the 0.5 next-attack exists) off
`m_flKungFuBlock` (player.h:418 ≈ +0x7a8) but: bar on the gun not player, no counter
role-swap (+0x79c/+0x7a4 whiff-lunge members absent), no kungfuhit.wav/33-drain/0.25 lock.

**RECONCILE**: replace the inline ts_gun.cpp block with a faithful free function in
TS_gamerules.cpp; call from CloseCombatAttack/ColdCockAttack (their dossiers,
analysis_closecombat.md/analysis_coldcock.md, already plan this) and CheckForBreakables
(ts_stunt.cpp:723). Playtest-gated. Soft spots: the dead direction/MakeVectors computation
(keep out) and the EMIT_SOUND_DYN trailing 0.5 (VERIFY meaning once, globally).

---

## 9. WeaponRestrictionByID(int) @ nm 0x66ad8

**SIGNATURE**: `int WeaponRestrictionByID(int id)` (`WeaponRestrictionByID__Fi`).
Maps a restriction preset id → allowed-weapon-category bitmask.

**BEHAVIOR**: switch(id): 1 → **0x0** (nothing allowed), 2 → **0x1**,
3 → **0x3**, 4 → **0x13**; default → `InitWeaponRestriction()` (= all five category bits,
0x1f; ours at dlls/ts_mapglobals.cpp:63). Category bit meanings per ts_mapglobals (knife/
pistol/etc.; 0x13 = bits 0,1,4 — VERIFY exact bit→category mapping when wiring).

**CALLERS** (static, verified): `cmd_weaponrestriction__Fv` @0xb5937 — the
**`weaponrestriction` server console command** (original game.cpp registers it). Flow:
server op runs `weaponrestriction <1..4>` → preset mask (anything else = reset to all).

**OURS**: MISSING both this mapper and the `cmd_weaponrestriction` command (no
`weaponrestriction` hits in dlls/game.cpp / client.cpp; ts_buy.cpp:143 notes the restriction
mask isn't wired into buying either). We DO have `InitWeaponRestriction` and the map-driven
`CTSMapGlobals::get_weapon_restriction` (ts_mapglobals.cpp:70).

**RECONCILE**: write fresh: WeaponRestrictionByID in TS_gamerules.cpp +
`cmd_weaponrestriction` in game.cpp (`pfnAddServerCommand("weaponrestriction", …)`)
storing into the active restriction global, and wire IsRestrictedWeapon into BuyWeapon
(separate ts_buy gap). The command NAME is part of the external swap contract (coverage
harness diffs server commands) → required for parity. Behavior-preserving to add; the
buy-gating wiring is playtest-gated. VERIFY: where cmd_weaponrestriction stores the mask
(decompile in the refs pass) and the exact category-bit semantics of 0x13.
