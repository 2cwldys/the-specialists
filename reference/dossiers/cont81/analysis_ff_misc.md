# Free functions — group "original file misc" (TS_bullet / TS_grenades / TS_multiplay_gamerules / TS_player / TS_powerup)

Ground truth: ff_*.c decompiles in this scratchpad; binary = reference/ts_i386.so
(nm addr = Ghidra addr − 0x10000). Callers resolved LOCK-FREE via objdump of the
original .so (PLT call sites → containing function by nm-sorted symbol), so they
are BINARY-verified, not guesses.

---

## 1. UTIL_TraceLine_Expanded (TS_bullet.cpp) — @0x5f4f4 (nm), size 354

### SIGNATURE
```cpp
void UTIL_TraceLine_Expanded(const Vector &vecStart, const Vector &vecEnd,
                             IGNORE_MONSTERS igmon, edict_t *pentIgnore, TraceResult *ptr);
```
(mangled `UTIL_TraceLine_Expanded__FRC6VectorT015IGNORE_MONSTERSP7edict_sP11TraceResult`;
not in signatures-gcc2.txt's demangled list but the mangling is unambiguous — same
shape as the SDK's 5-arg `UTIL_TraceLine` ignore-monsters variant.)

### BEHAVIOR
The "generous player hitbox" trace. For every connected player it temporarily
inflates the engine-side bounding box by ±32 units, runs a normal
`UTIL_TraceLine`, then restores:

1. Loop i = 1 .. `gpGlobals->maxClients`:
   - `UTIL_PlayerByIndex(i)`; skip NULL.
   - Save the player's **absmax** (entvars +0xd0/0xd4/0xd8 = `pev->absmax`)
     into global `Vector vecBoxes[]` at index i (global `vecBoxes` @0x167f60 BSS;
     loop cursor starts at byte 0xc = index 1, players are 1-based).
   - `pev->absmax += (32,32,32)` and `pev->absmin -= (32,32,32)`
     (absmin = entvars +0xc4/0xc8/0xcc). Constant `fc_32` = **32.0f** — the hit
     expansion radius.
2. `UTIL_TraceLine(vecStart, vecEnd, igmon, pentIgnore, ptr)` — the stock SDK trace.
3. Loop players again and restore **absmax only** from `vecBoxes[i]`.

**Quirk (reproduce faithfully):** only absmax is saved/restored; **absmin is left
deflated by 32** after the call. It self-heals on the next engine relink
(SV_LinkEdict recomputes absmin/absmax every move), but within the same server
frame later traces see the shrunken absmin. Do NOT "fix" this — mimic exactly
(save absmax → expand both → trace → restore absmax).

Note: every player is expanded including the shooter; the shooter is excluded
from the trace via `pentIgnore`, not via the loop.

Globals: `vecBoxes` (B @0x167f60, TS_bullet.cpp TU — the same TU also holds
`glast_weaponid` D @0x10100c with its static init `_GLOBAL_.I.glast_weaponid`
@0x5f688). No members, no messages.

### CALLERS (binary-verified via objdump call sites → PLT @0x579d0)
- `CBulletManager::UpdateBullet(int, TraceResult*)` — call @0x5f0d2 (the slow-mo
  bullet flight step, TS_bullet.cpp).
- `CBasePlayer::ShootBulletsPlayer(uchar, Vector, Vector, Vector, float, int, int, int)`
  — TWO call sites @0x5f878 and @0x5fcf0 (the hitscan/pierce loop, TS_bullet.cpp).
No other callers.

### OURS
MISSING. Both of our replacement paths use the plain trace:
- `dlls/ts_bulletmgr.cpp:172` `UpdateBulletTimeSlice` → `UTIL_TraceLine(b.pos, newpos,
  dont_ignore_monsters, dont_ignore_glass, owner->edict(), &tr)` — no box expansion.
- `dlls/ts_gun.cpp:1392` `TSFireBulletsPenetrate` (our ShootBulletsPlayer) →
  `UTIL_TraceLine(start, start + dir*flDist, dont_ignore_monsters, ENT(pevAttacker), &tr)`.
No ±32 expansion, no `vecBoxes` anywhere in dlls/pm_shared/cl_dll.

### RECONCILE
- **Write fresh** (small, self-contained): `UTIL_TraceLine_Expanded` + file-static
  `Vector vecBoxes[33]` into the TS_bullet.cpp consolidation target (currently
  ts_bulletmgr.cpp / ts_gun.cpp bullet code; plan C-file "TS_bullet.cpp").
- **Swap call sites**: ts_bulletmgr.cpp:172 and ts_gun.cpp:1392 (and any second
  trace in the pierce loop — the original has TWO expanded call sites in
  ShootBulletsPlayer; verify our single-loop structure covers both) must call the
  expanded variant. VERIFY: our UpdateBulletTimeSlice passes ignore_glass
  (2-flag variant) while the original expanded fn only takes IGNORE_MONSTERS —
  check the original UpdateBullet's actual flag argument during the refs pass.
- **Playtest-gated**: this measurably widens player hit detection (a signature
  TS feel item — generous slow-mo/hitscan hits). Gameplay-tuning change.
- Preserve the absmin-not-restored quirk. Flag in code comment.

---

## 2. m61_grenade (TS_grenades.cpp) — @0x6c850 (nm), size 33

### SIGNATURE
```cpp
// LINK_ENTITY_TO_CLASS(m61_grenade, CM61Grenade) expansion:
extern "C" void m61_grenade(entvars_t *pev);
```

### BEHAVIOR
One line: `GetClassPtr((CM61Grenade *)pev)` — the standard `LINK_ENTITY_TO_CLASS`
entity-factory export. Engine-facing C symbol; part of the **external swap
contract** (the engine dlsym's `"m61_grenade"` to spawn that classname).

### CALLERS
- The ENGINE (dlsym on classname spawn) — no in-binary callers.
- `TSPrecache` (ff_TSPrecache.c:248) does `UTIL_PrecacheOther("m61_grenade")`,
  which goes through the engine → this factory. So the original DOES spawn a
  temp entity by the classname string at precache time; without the factory the
  engine logs "Can't create entity" and the precache is skipped.

### OURS
**MISSING — this is the task-flagged gap.** `dlls/ts_grenades.cpp` fully
reconstructs `CM61Grenade` (Spawn @0x7c974, WaitThink, BounceTouch, Use,
CreateGrenade @0x7c874) but the ONLY link macro in the file is
`LINK_ENTITY_TO_CLASS(shrapnel, CTSShrapnel)` (ts_grenades.cpp:142). There is no
`LINK_ENTITY_TO_CLASS(m61_grenade, CM61Grenade)` and `nm` on our hl.so would show
no `T m61_grenade`. Also:
- `dlls/ts_grenades.cpp:301` CreateGrenade sets `pev->classname = MAKE_STRING("grenade")`
  — VERIFY against the original CreateGrenade @0x7c874 (needs the re project;
  plausible the original sets "grenade" for killfeed/stock-code compat, but if it
  relies on the factory the classname would be "m61_grenade"). Killfeed/classname
  checks elsewhere hang off this.
- No `UTIL_PrecacheOther("m61_grenade")` anywhere in our dlls (grep: 0 hits) —
  the TSPrecache reconciliation (other group) needs the factory to exist first.

### RECONCILE
- **One-liner add** to dlls/ts_grenades.cpp (→ future TS_grenades.cpp):
  `LINK_ENTITY_TO_CLASS(m61_grenade, CM61Grenade);`. CM61Grenade is currently a
  file-local class — the macro lives in the same file, fine.
- Behavior-preserving + contract-restoring (coverage.py entity-class list should
  flip m61_grenade from MISSING). Verify post-build:
  `nm -D build-amd64/dlls/hl.so | grep m61_grenade`.
- VERIFY classname set in CreateGrenade ("grenade" vs "m61_grenade") during the
  refs pass on @0x7c874.

---

## 3. TSGetPointsForFrag (TS_multiplay_gamerules.cpp) — @0x79140 (nm), size 1172

### SIGNATURE (signatures-gcc2.txt)
```cpp
int TSGetPointsForFrag(CBasePlayer *pVictim, entvars_t *pevKiller, entvars_t *pevInflictor,
                       int iPoints, bool *pbSlideKill, bool *pbStuntKill);
// returns the adjusted frag points (returned as a3 & 0xff)
```
(a0 = pVictim is NEVER read in the body — parameter exists for interface shape only.)

### BEHAVIOR
The stylish-kill frag-point multiplier + the green "%i frags for <name>, <style>"
broadcast. Step by step (a1 = killer pev, a2 = inflictor pev, a3 = base points):

1. `if (iPoints < 0) return iPoints;` (suicide/world −1 passes through untouched).
2. Resolve killer CBaseEntity from `a1->pContainingEntity` private data (+0x7c).
3. **The One gate**: if global `g_theOneMode` (D @0x113dc4) != 0 AND
   killer `IsPlayer()` (vtable byte 0xc8 = IsPlayer per VTABLES.md) AND
   `*(char*)(killer+0x7b9) == 0` → **return 0**. +0x7b9 = the player's
   "is The One" bool (our `CBasePlayer::m_bTheOne`): in The One mode only the
   One's kills score.
4. If killer is a client (`a1->flags & FL_CLIENT(8)`): build killer name
   (`strcpy(name, STRING(a1->netname))`) and a style/weapon description `desc`:
   - **inflictor == killer** (direct weapon): if global `glast_weaponid`
     (D @0x10100c, defined in TS_bullet.cpp TU) **== −1** (last damage not from a
     gun-bullet path):
     - active item = killer `+0xa10` = `m_pActiveItem` (members.tsv).
     - if no active item AND `glast_closecombat` (D @0x104480, char, set by the
       close-combat attack path — see dossier-melee.md) → `desc = "#TS_KungFu"`
       (rodata @f8787), **points = 2**.
     - else if active item `m_iId`(+0x88) == **16** (= TSGUN_WEAPON_ID, all CTSGun,
       dlls/ts_gun.h:15) AND (item `+0xcc` == 0 (CTSGun+0xcc = the s_weapon_status*
       per members_re.tsv — i.e. bare hands / no gun equipped) OR
       `glast_closecombat`) → `desc = "#TS_KungFu"`, **points = 2**.
   - **inflictor != killer**: if the inflictor classname's first 6 bytes !=
     `"knife\0"` (rodata @f77e5) → **return points & 0xff** unchanged
     (grenade/other inflictors get no bonus). Else (thrown knife kill):
     desc = `CTSKnife::GetDescription(inflictor)` (strcpy'd) or literal
     `"Throwing"` if the knife edict has no private data; **points = 2**.
5. **Stunt bonus** — killer pev checks:
   - if `(a1->iuser4 & 0x10c10)` AND `|a1->velocity| > 80` (fc_80): the STUNT
     branch. 0x10c10 = TSSTUNT_DIVE(0x10) | TSSTUNT_WHEEL_HI|LO(0xc00) |
     TSSTUNT_WALLJUMP_A(0x10000) (dlls/ts_stunt.h) — killing mid-dive/cartwheel/
     wall-jump at speed > 80 u/s. If `!glast_closecombat` AND killer IsPlayer()
     → **points += 1**. If points > 1: desc = `"#TS_StStl"` (or
     `strcat(desc, " & #TS_StStl")` if a weapon desc already set).
     `*pbStuntKill = true` (unconditional in branch).
   - else if `(a1->iuser4 & 0x20)` (TSSTUNT_SLIDE) AND `a1->friction < 1.0` AND
     speed > 80: the SLIDE branch, same shape: `!glast_closecombat` && IsPlayer →
     **points += 1**; desc = `"#TS_SlStl"` (@f87ab) / `" & #TS_SlStl"`;
     `*pbSlideKill = true`.
   - else, if desc non-empty: `strcat(desc, " #TS_Stl")` (plain "with style"
     suffix on kungfu/knife kills done without a stunt).
6. Finally (reached also for non-client killers via the goto): if points > 1:
   killer `IsPlayer()` is called and the result DISCARDED (dead call), then
   `TSHudMessageAll(0, 255, 0, 1, 30, UTIL_VarArgs("%i #TS_Frg #TS_FrgFr %s, %s", points, name, desc))`
   — green, channel 1, holdtime 30, the localized "N frags for <killer>, <style>"
   broadcast (#TS_* tokens resolve via titles.txt; format string @f87be).
7. `return points & 0xff` (byte-clamped).

Constants: points **2** for kungfu/thrown-knife, **+1** for stunt/slide (max 3),
speed gate **80 u/s**, friction gate **< 1.0**, HUD channel 1 / holdtime **30** /
color green (0,255,0).

### CALLERS (binary-verified)
- `CHalfLifeMultiplay::PlayerKilled(CBasePlayer*, entvars_t*, entvars_t*)` —
  call @0x7970d (TS's modified stock multiplay gamerules,
  TS_multiplay_gamerules.cpp). Presumably replaces/augments the stock
  `pKiller->frags += IPointsForKill(...)`; the two bool out-flags feed the rest
  of PlayerKilled (slide/stunt-specific award/sound — dossier-melee.md L342 notes
  the glast_coldcock/closecombat branches there). Single caller.

### OURS
MISSING entirely:
- `dlls/multiplay_gamerules.cpp:563` PlayerKilled is stock SDK
  (`pKiller->frags += IPointsForKill(peKiller, pVictim)`); no TS hook.
- `dlls/ts_gamerules.cpp:242` `CTSGameRules::PlayerKilled` calls the inherited MP
  version then does TS death effects + `TheOneIsDead` — no points adjustment, no
  #TS_Frg broadcast.
- We DO have the ingredients: `m_bTheOne` (ts_gamerules.cpp:520), the TSSTUNT_*
  iuser4 bits (dlls/ts_stunt.h:20-41), `TSHudMessageAll` (ts_utils.h; used with the
  exact same (0,255,0,1,30,…) shape at dlls/ts_timer.cpp:93), and CTSKnife.
- We do NOT have: `glast_weaponid`, `glast_closecombat` (both known to
  dossier-melee.md / plan_files_5.md as future globals), `g_theOneMode` as a
  global (our The-One state is `CTSGameRules::m_bTheOneActive` — VERIFY how the
  original sets `g_theOneMode`, likely from the gamemode decide path),
  `CTSKnife::GetDescription`.

### RECONCILE
- **Write fresh** as a free function in the TS_multiplay_gamerules.cpp
  consolidation target, called from `CHalfLifeMultiplay::PlayerKilled` exactly
  where the original does (around IPointsForKill, before frags +=). Our
  CTSGameRules inherits it, matching the original structure.
- Dependencies to land with it (shared with the melee cluster, task #3):
  `int glast_weaponid` (define in the TS_bullet.cpp TU per the binary — VERIFY
  init value from _GLOBAL_.I.glast_weaponid @0x5f688, likely −1),
  `char glast_closecombat` (TS_player.cpp region), `g_theOneMode`,
  `CTSKnife::GetDescription`. Until the writers of glast_* exist (bullet path
  sets glast_weaponid, close-combat sets glast_closecombat), the kungfu-desc
  branch will misfire — sequence AFTER (or with) the melee-cluster globals.
- **Playtest-gated** (scoring change: stylish kills worth 2–3 + broadcast).
- VERIFY: the exact call-site plumbing in PlayerKilled (does the return value
  REPLACE IPointsForKill or take it as the base arg? needs the refs pass on
  @0x7970d) and what PlayerKilled does with pbSlideKill/pbStuntKill afterwards.

---

## 4. FindHullIntersectionplayer (TS_player.cpp) — @0x85bac (nm), size 633

### SIGNATURE (signatures-gcc2.txt)
```cpp
void FindHullIntersectionplayer(const Vector &vecSrc, TraceResult &tr,
                                float *mins, float *maxs, edict_t *pEntity);
```

### BEHAVIOR
Byte-for-byte the stock SDK crowbar helper `FindHullIntersection`, copied into
TS_player.cpp and renamed (avoids the static-linkage collision with crowbar.cpp's).
Algorithm, identical to dlls/crowbar.cpp:85:
1. `vecHullEnd = vecSrc + (tr.vecEndPos − vecSrc) * 2` (fc_2; double the reach).
2. `UTIL_TraceLine(vecSrc, vecHullEnd, dont_ignore_monsters(0), pEntity, &tmpTrace)`;
   if `tmpTrace.flFraction < 1.0` → copy tmpTrace into tr (0xe dwords = 56 bytes
   = sizeof(TraceResult)), done.
3. Else 2×2×2 corner loop: `minmaxs[2] = {mins, maxs}`; for i,j,k in {0,1}:
   `vecEnd = vecHullEnd + (minmaxs[i][0], minmaxs[j][1], minmaxs[k][2])`; trace;
   keep the hit whose endpoint is closest to vecSrc (`distance` sentinel starts
   at fc_1000000 = 1e6). Copy the winning TraceResult into tr.
No members/globals; only UTIL_TraceLine. Constants: ×2 reach, 1e6 sentinel.
(Only difference from crowbar.cpp's: parameter type `float*` instead of
`const Vector&` — same memory layout; decompile confirms identical use.)

### CALLERS (binary-verified)
- `CBasePlayer::CheckHull(Vector, Vector, TraceResult&, int)` — call @0x85f60
  (TS_player.cpp). CheckHull is the TS melee (close-combat / knife-slash) hit
  refinement: line trace → miss → TraceHull(iHull) → FindHullIntersectionplayer
  (dossier-melee.md L55/L301). Single caller. FindHullIntersectionplayer itself
  reads NO TSGameRules globals (it is pure); the caller CheckHull's tail has an
  unread vtable-slot-46 quirk (dossier L302/L448) — CheckHull's problem, not
  this function's.

### OURS
PARTIAL (algorithm exists, symbol/caller don't): the identical routine is the
file-static `FindHullIntersection` at dlls/crowbar.cpp:85 (used only by the stock
crowbar, crowbar.cpp:174). Our close-combat swing (dlls/ts_gun.cpp:1145-1149)
does a bare `UTIL_TraceLine` with NO hull fallback — melee whiffs on near-misses
the original's CheckHull would land. No `CheckHull` in our dlls (grep: 0).

### RECONCILE
- **Copy, don't invent**: duplicate crowbar.cpp's static into dlls/ts_player.cpp
  (→ TS_player.cpp target) under the original name `FindHullIntersectionplayer`
  with the original `float*` signature. Keep crowbar.cpp's static untouched —
  the original binary contains BOTH copies.
- Land together with `CBasePlayer::CheckHull` — already planned as melee-cluster
  step S3 (dossier-melee.md L298: "behavior-preserving: compiled, uncalled",
  wired later by CloseCombatAttack/slash). Adding this function is
  behavior-preserving; the gameplay change comes when CheckHull replaces the
  bare trace in ts_gun.cpp melee (playtest-gated there).

---

## 5. GetRandomPosition (TS_powerup.cpp) — @0x874f4 (nm), size 919

### SIGNATURE (signatures-gcc2.map)
```cpp
Vector GetRandomPosition(Vector &vecCenter);   // gcc2 sret: hidden Vector* return slot
```

### BEHAVIOR
Pick a clear point in a random horizontal direction 128–1024 units out from
`vecCenter` (for the dynamic powerup spawner):
1. Sector step = `360 / 16` = **22.5°** (rodata int **16** @0x109760, loaded and
   divided into dc_360). Start angle = `RANDOM_LONG(0,16) * 22.5` — note
   **0..16 inclusive** (17 values; 16 ≡ 0 mod 360). Reproduce as-is.
2. Up to **16 attempts** (counter 1..16), advancing the angle by 22.5° each
   retry (a full-circle sweep):
   - dist = `RANDOM_LONG(0x80, 0x400)` = **128..1024** units.
   - dir = normalize(cos(angle°), sin(angle°), 0) (deg→rad via
     0.017453292519943295; z stays 0; the normalize has the SDK zero-length →
     (0,0,1) fallback, unreachable here).
   - `UTIL_TraceLine(vecCenter, vecCenter + dir*dist, ignore_monsters=1, NULL, &tr)`.
   - candidate = `tr.vecEndPos − dir * 64` (fc_64 — back off **64 units** toward
     the center from wherever the trace stopped; on no hit vecEndPos is the full
     endpoint). The hit and no-hit branches compute the SAME candidate
     (duplicated code in the binary).
   - if `UTIL_PointContents(candidate) != CONTENTS_SOLID (−2)` → write candidate
     into the return Vector and return.
3. All 16 attempts solid → **return vecCenter itself** (fallback).

Constants: 16 sectors / 22.5°, radius 128–1024, 64-unit backoff, 16 tries.
No members/globals/messages; callees: UTIL_TraceLine, UTIL_PointContents,
engine pfnRandomLong, libm fcos/fsin.

### CALLERS (binary-verified)
- `CTSGameRules::PutPowerUp(int)` — call @0x87a76 (inside PutPowerUp @0x8797c).
  Single caller.

### OURS
MISSING. `dlls/ts_gamerules.cpp:668` `CTSGameRules::PutPowerUp(int)` instead:
collects up to 32 `info_player_deathmatch` spots, picks one at random, places the
powerup at `spot->pev->origin + (0,0,16)`, rejects if a `ts_powerup` is already
within a 128-unit sphere, then `TSCreatePowerUpAt(type, vecOrigin)`
(dlls/ts_powerup.cpp:295). So our powerups spawn EXACTLY on spawn points; the
original scatters them 128–1024 units away in a random clear direction — a
visible gameplay/pacing difference (powerups appear in corridors/rooms around
spawns, not on the pads).

### RECONCILE
- **Write fresh** `Vector GetRandomPosition(Vector &)` into dlls/ts_powerup.cpp
  (→ TS_powerup.cpp target — funcfile.tsv places it there, NOT in the gamerules
  file), and call it from `CTSGameRules::PutPowerUp` on the chosen origin.
- VERIFY (needs the re-project decompile of PutPowerUp @0x8797c, currently
  locked): what vecCenter the original passes (random spawn point origin? +z
  lift?), whether our +(0,0,16) and the 128-sphere "already a ts_powerup here"
  rejection exist in the original or are invention, and PutPowerUp's return
  semantics. Re-audit our PutPowerUp body in the same pass.
- **Playtest-gated** (powerup placement changes map flow).

---

## Group verdict table

| Function | Verdict |
|---|---|
| UTIL_TraceLine_Expanded | missing (call sites exist: ts_bulletmgr.cpp:172, ts_gun.cpp:1392 use plain UTIL_TraceLine) |
| m61_grenade | missing (CM61Grenade class exists in ts_grenades.cpp; the LINK_ENTITY_TO_CLASS factory export does not) |
| TSGetPointsForFrag | missing (stock IPointsForKill only; TheOne/stunt-bit/HUD ingredients exist) |
| FindHullIntersectionplayer | partial (identical algorithm = crowbar.cpp:85 static; not available to the missing CBasePlayer::CheckHull) |
| GetRandomPosition | missing (our PutPowerUp spawns on the pad instead of scattering 128–1024u) |
