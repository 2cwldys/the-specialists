# GROUP 3 move plan — TS_gamerulesclass.cpp (CTSGameRules consolidation)

READ-ONLY plan (nothing moved yet). Repo: /home/stevenlafl/Projects/goldsrc.net/mod-ts, branch `restructure-source-parity`.

## 0. Ground truth (what the ORIGINAL file contains)

`reference/coverage/funcfile.tsv`: **TS_gamerulesclass.cpp = 40 symbols**, all `CTSGameRules::*` plus ONE
free function, in this binary order (this is the target intra-file order):

```
CTSGameRules() ctor, Think, RespawnEntities, UpdateTeamScores, RestartRound, Init, IsCTB,
InitTeams, SetTeamList, SetTeamModelList(const char*), SetTeamModelList(int,const char*),
GetTeamsNum, InitObjectives, ResetRestartRound, TeamWin, RoundTimeLeft, SetObjective,
InitTeamScore, InitHUD, SetGlobalSlowMotion, TogglePlayerFromSlow, GlobalSlowMotion2,
GetPlayerModel, GetTeamCash, GetTeamIndex, CallCheckDeads, CheckDeads, CountPlayers,
CalcMiniminVotants (static free fn), PlayerVote, CheckVotes, ClearObjectiveList,
AddObjectiveInList, SendObjectiveToHud, SendObjectiveList, FindIndexByObjective,
ShowObjectiveOnHud, SelectRandomTheOne, SetPlayerTheOne, TheOneIsDead
(+ _GLOBAL_.I./.D. static init/fini — compiler artifacts, not source)
```

**Critical funcfile facts that shape the plan:**
- `GetRandomPowerup__12CTSGameRulesi` and `PutPowerUp__12CTSGameRulesi` belong to **TS_powerup.cpp**
  (the original ALSO split these two CTSGameRules methods out) — they must move to our `ts_powerup.cpp`,
  NOT into TS_gamerulesclass.cpp.
- `HaveFlag__FP11CBasePlayer` belongs to the original **TS_gamerules.cpp** (the free-function file,
  a DIFFERENT original file) — it currently lives in our ts_gamerules.cpp and must move out.
- There is **no TS_teams.cpp in the original** — our ts_teams.cpp is an invented split; its entire
  contents fold into TS_gamerulesclass.cpp.
- ts_inventory.json `files` contains **zero .h entries** → original header layout is unknowable.
  **Keep our `dlls/ts_gamerules.h` as-is** (audit only checks .cpp attribution; ~a dozen includers).

**Why the audit currently lists FOUR files for CTSGameRules:** audit.py `our_class_files()` regex
(`C\w+::~?\w+\s*\(`) matches **comments too**. Real definitions exist only in `ts_gamerules.cpp` and
`ts_teams.cpp`; `ts_objectives.cpp` (lines 203/307/411) and `ts_powerup.cpp` (line 292) match via
comment text only. `file_ok = any(our_file matches orig)`, so creating TS_gamerulesclass.cpp with the
defs clears the mismatch row even if the comment-matches remain. (Optional cosmetic: reword those 4
comments to `CTSGameRules::TeamWin`-without-`(` form.)

`norm_file()` check: `TS_gamerulesclass.cpp` → `gamerulesclass` ≠ our `ts_gamerules.cpp` → `gamerules`
(current mismatch). Note `ts_powerup.cpp` ≡ `TS_powerup.cpp` under norm_file (lstrip of `t/s/_` + lower),
so NO rename of ts_powerup.cpp is needed by this group.

## 1. Target layout

| new/updated file | contents | from |
|---|---|---|
| `dlls/TS_gamerulesclass.cpp` (NEW, via `git mv dlls/ts_gamerules.cpp`) | all CTSGameRules methods (incl. the 21 from ts_teams.cpp) + static `CalcMiniminVotants` + `gmsgTSRoundTime/TSObjective/TSShowObjective` globals + fwd decls | ts_gamerules.cpp + ts_teams.cpp |
| `dlls/ts_powerup.cpp` (existing; original name `TS_powerup.cpp`, norm-equivalent — no rename) | + `static g_iNoLowGravPowerup`, `CTSGameRules::GetRandomPowerup`, `CTSGameRules::PutPowerUp` | ts_gamerules.cpp |
| `dlls/TS_gamerules.cpp` (NEW, small) | free fn `HaveFlag` (+ future free funcs, §5) | ts_gamerules.cpp |
| `dlls/ts_teams.cpp` | **deleted** (fully absorbed) | — |
| `dlls/ts_gamerules.h`, `ts_objectives.h`, `ts_powerup.h` | **unchanged** (original headers unknowable — keep ours; note in commit msg) | — |

Intra-file order for TS_gamerulesclass.cpp: phase A = straight concatenation (build-green fastest);
phase B (optional, cosmetic) = reorder methods to the binary order in §0. The audit does not check
intra-file order, so phase B may be skipped or deferred.

## 2. Exhaustive fragment map (source file:lines → target)

### 2a. dlls/ts_gamerules.cpp (809 lines) — becomes TS_gamerulesclass.cpp except:

| lines | fragment | target |
|---|---|---|
| 1–18 | banner + includes | stays; rewrite banner to `TS_gamerulesclass.cpp`; ADD `#include "UserMessages.h"` + `#include "ts_weapons.h"` (needed by the ts_teams.cpp methods: gmsgTeamInfo/TeamNames/ScoreInfo, TSDeployArsenal) |
| 19–22 | comment + `static byte g_iNoLowGravPowerup = 0;` | → dlls/ts_powerup.cpp (file scope, above GetRandomPowerup) |
| 24–47 | `int CTSGameRules::GetRandomPowerup(int mask)` (+comment) | → dlls/ts_powerup.cpp |
| 49–50 | fwd decl `CBaseEntity *TSCreatePowerUpAt(int, const Vector&)` | DELETE (PutPowerUp lands in ts_powerup.cpp AFTER the definition at its line 295) |
| 52–53 | fwd decl `void TSFireTimeLeftTarget();` | stays (used by RoundTimeLeft) |
| 54–60 | free fn `bool HaveFlag(CBasePlayer*)` (+comment) | → NEW dlls/TS_gamerules.cpp (decl stays in ts_flag.h:44 — callers ts_briefcase.cpp/ts_flag.cpp unaffected) |
| 61–64 | `int gmsgTSRoundTime/gmsgTSObjective/gmsgTSShowObjective` | stays (original registers its round/objective msgs here; audit places missing globals `gmsgTSServerSetting`, `roundtime` in TS_gamerulesclass.cpp too — future additions) |
| 66–86 | StartRoundTimer | stays *(audit-INVENTED method — flag in comment)* |
| 88–94 | ClearObjectiveList | stays |
| 96–104 | FindIndexByObjective | stays |
| 106–121 | AddObjectiveInList | stays |
| 123–145 | SendObjectiveToHud | stays |
| 147–154 | SendObjectiveList | stays |
| 156–167 | ShowObjectiveOnHud | stays |
| 169–198 | InitObjectives | stays |
| 200–215 | SetObjective | stays |
| 217–228 | SendRoundTime | stays *(INVENTED)* |
| 230–234 | RoundTimeLeft | stays |
| 236–240 | fwd decls TSCreateDeadCorpse / TSDropWeaponsOnDeath / TSDropAmmoOnDeath | stays |
| 242–262 | PlayerKilled | stays for now *(INVENTED at this class — original hangs it on `CHalfLifeMultiplay` in TS_multiplay_gamerules.cpp; migrating it is METHOD-level work for the multiplay group, not this file move)* |
| 264–285 | TeamWin | stays |
| 287–296 | CallCheckDeads | stays |
| 298–343 | CheckDeads | stays |
| 345–348 | CTSGameRules() ctor | stays |
| 350–360 | Init | stays |
| 362–374 | CountPlayers | stays |
| 376–396 | RespawnEntities | stays |
| 398–448 | RestartRound | stays |
| 450–478 | ResetRestartRound | stays |
| 480–490 | IsCTB | stays |
| 492–514 | SelectRandomTheOne | stays |
| 516–538 | SetPlayerTheOne | stays |
| 540–561 | TheOneIsDead | stays |
| 563–568 | `static int CalcMiniminVotants(int)` | stays (matches original placement — keep `static`) |
| 570–613 | PlayerVote | stays |
| 615–663 | CheckVotes | stays |
| 665–691 | `bool CTSGameRules::PutPowerUp(int type)` (+comment) | → dlls/ts_powerup.cpp, placed AFTER `TSCreatePowerUpAt` (ts_powerup.cpp:295) |
| 692–764 | Think (+comment) | stays (calls GetRandomPowerup/PutPowerUp cross-file — fine, both declared in ts_gamerules.h:48/174) |
| 766–790 | GlobalSlowMotion2 | stays |
| 792–809 | SetGlobalSlowMotion | stays |

### 2b. dlls/ts_teams.cpp (330 lines) — ALL → TS_gamerulesclass.cpp, file deleted

| lines | fragment | notes |
|---|---|---|
| 1–17 | banner | fold provenance notes into TS_gamerulesclass.cpp banner, then drop |
| 18–25 | includes | union into TS_gamerulesclass.cpp (`ts_weapons.h`, `UserMessages.h` are the two not already there) |
| 27–41 | IsTeamMode | *(INVENTED — functional proxy; keep, flag)* |
| 43–46 | IsTeamplay | *(INVENTED)* |
| 48–49 | GetGameDescription provenance comment | keep |
| 51–58 | InitTeams | original member |
| 60–66 | SetTeamList | original |
| 68–80 | SetTeamModelList(const char*) | original |
| 82–89 | SetTeamModelList(int, const char*) | original |
| 91–95 | GetTeamsNum | original |
| 97–107 | GetTeamCash | original |
| 109–111 | InitTeamScore + UpdateTeamScores (no-ops) | original |
| 113–123 | GetPlayerModel | original |
| 125–132 | TogglePlayerFromSlow | original |
| 134–153 | RecountTeams | *(INVENTED)* |
| 155–166 | GetTeamIndex | original |
| 168–173 | GetIndexedTeamName | *(INVENTED)* |
| 175–180 | GetTeamID | *(INVENTED)* |
| 182–209 | TeamWithFewestPlayers | *(INVENTED)* |
| 211–243 | AssignToTeam | *(INVENTED)* |
| 245–297 | PlayerSpawn | *(INVENTED at this class — original: CHalfLifeMultiplay::PlayerSpawn, TS_multiplay_gamerules.cpp; method-level follow-up, not this move)* |
| 299–330 | InitHUD | original (`InitHUD__12CTSGameRulesP11CBasePlayer` IS in TS_gamerulesclass.cpp) |

**File-scope collision check:** ts_teams.cpp has ZERO file-scope statics/globals (only method defs) —
no collisions with ts_gamerules.cpp on merge. In ts_powerup.cpp, `g_iNoLowGravPowerup` is a new unique
name and `TS_PWUP_*`/`RANDOM_LONG`/`UTIL_FindEntity*` are already available (ts_powerup.cpp includes
util.h + ts_gamerules.h at lines 27/31). Only shared symbol is `TSCreatePowerUpAt` — resolved by
placing PutPowerUp after it and deleting the old fwd decl.

### 2c. NEW dlls/TS_gamerules.cpp (scaffold)

Seed with: banner, `#include "extdll.h"/"util.h"/"cbase.h"/"player.h"/"ts_flag.h"`, and `HaveFlag`
(from ts_gamerules.cpp:54–60). Everything else of the original TS_gamerules.cpp is OTHER groups' /
future work — see §5.

## 3. dlls/CMakeLists.txt edits

Current lines 113–114:
```
	ts_gamerules.cpp
	ts_teams.cpp
```
- Step 1: `ts_gamerules.cpp` → `TS_gamerulesclass.cpp` (line 113).
- Step 3: add `TS_gamerules.cpp` next to it.
- Step 4: delete the `ts_teams.cpp` line (114).
No cl_dll/CMakeLists.txt changes (server-only group). `ts_powerup.cpp` line 139 unchanged.

## 4. Ordering + verification (each step builds green)

Baseline first: `scripts/build.sh amd64` and record
`nm -C build-amd64/dlls/hl.so | grep -c "CTSGameRules::"` (method count) and
`nm -D build-amd64/dlls/hl.so | grep -E "^\S+ T (ts_|info_|func_)" | wc -l` (entity factories).
Note: neither ts_gamerules.cpp nor ts_teams.cpp contains any LINK_ENTITY_TO_CLASS / save table —
the only factory in touched files is `ts_powerup` + `ts_superjump` (ts_powerup.cpp:40/456), which we
only append to.

1. **Rename**: `git mv dlls/ts_gamerules.cpp dlls/TS_gamerulesclass.cpp` (preserves history) + CMake
   line 113. Build. nm counts unchanged.
2. **Powerup extraction**: move lines 19–22, 24–47, 665–691 into ts_powerup.cpp (static above
   GetRandomPowerup; PutPowerUp after TSCreatePowerUpAt @295); delete fwd decl 49–50 from
   TS_gamerulesclass.cpp. Build. Check `nm -C … | grep -E "GetRandomPowerup|PutPowerUp"` still 2 defs;
   `nm -D … | grep " T ts_powerup"` factory intact.
3. **TS_gamerules.cpp scaffold**: create with HaveFlag; remove lines 54–60 from TS_gamerulesclass.cpp;
   add to CMake. Build. `nm -C … | grep "HaveFlag"` still defined (ts_briefcase.cpp:137 links).
4. **ts_teams.cpp fold-in**: append its 21 methods (lines 27–330) to TS_gamerulesclass.cpp; union
   includes (`UserMessages.h`, `ts_weapons.h`); `git rm dlls/ts_teams.cpp`; delete CMake line. Build.
   `nm -C … | grep -c "CTSGameRules::"` == baseline.
5. **Audit**: rebuild, run `python3 scripts/coverage/audit.py` → the
   `CTSGameRules | TS_gamerulesclass.cpp | …` FILE-mismatch row must be gone (file_ok via
   TS_gamerulesclass.cpp; norm `gamerulesclass` == `gamerulesclass`).
6. *(optional, cosmetic)* Reorder methods inside TS_gamerulesclass.cpp to the §0 binary order; reword
   the `CTSGameRules::X(`-form comments in ts_objectives.cpp:203/307/411 + ts_powerup.cpp:292 so the
   audit's our_files list stops attributing those files. Build once more.
7. Runtime smoke (unchanged behaviour expected — pure file moves): xash listenserver per CLAUDE.md §3
   on tm_avana (team assignment + round restart still work).

## 5. Future in-file additions (NOT this move — blocked on RE / other groups)

- **TS_gamerulesclass.cpp missing globals** (AUDIT.md): `gmsgTSServerSetting`, `roundtime` — add here
  when RE'd.
- **Original TS_gamerules.cpp free functions** (AUDIT MISSING, blocked on RE): DecideGamePlay,
  DoSpectacularNess, GetAccelBySlots, SetPlayerRespawnTime, TSSelectSpawnSpot, VictimKatanaHold,
  VictimKnifeHold, VictimKungfuHold, WeaponRestrictionByID → all belong in the new dlls/TS_gamerules.cpp.
  ALSO already-implemented free funcs that funcfile places in TS_gamerules.cpp but which live elsewhere
  in our tree (candidates for a later free-function consolidation pass, coordinate with whichever group
  owns those files): SlowMotion/SlowPause/InfiniteClips/DoubleFireRate/KungFu (ts_player.cpp:118–147),
  TSHudMessageOne/All (ts_utils.cpp:16/42), CashBased (ts_buy.cpp:32), InitWeaponRestriction
  (ts_mapglobals.cpp:63), GetSpeedBySlots (ts_gun.cpp:121), EntSelectSpawnPoint (stock override —
  currently gamerules.cpp/player.cpp).
- **TS_powerup.cpp missing free fn**: `GetRandomPosition` (blocked on RE) — future addition to
  ts_powerup.cpp.
- **Method-level relocation** of PlayerSpawn/PlayerKilled to CHalfLifeMultiplay
  (TS_multiplay_gamerules.cpp) — resolves 2 of the 11 INVENTED CTSGameRules methods; belongs to the
  multiplay-gamerules group, do NOT do it as part of this file move.

## 6. RISKS

- **Include order**: merged TS_gamerulesclass.cpp must add `UserMessages.h` + `ts_weapons.h` or the
  ts_teams methods (gmsgTeamInfo/gmsgTeamNames/gmsgScoreInfo, TSDeployArsenal) fail to compile.
  ts_powerup.h's TS_PWUP_INSTANT macro requires ts_gamerules.h first — already satisfied in
  ts_powerup.cpp (31 before 38).
- **Statics**: only `CalcMiniminVotants` (stays) and `g_iNoLowGravPowerup` (moves) — no collisions
  found in targets. `TSSendPowerupNotify` static in ts_powerup.cpp does not clash.
- **Case-only-ish naming**: new `TS_gamerules.cpp` vs old `ts_gamerules.cpp` differ only in case — on a
  case-insensitive checkout (macOS/Win) a git checkout crossing these commits could collide; the old
  name is renamed away in step 1 BEFORE step 3 creates the new one, so never coexist — keep that order.
- **Comment-based audit attribution** keeps ts_objectives.cpp/ts_powerup.cpp listed as our_files for
  CTSGameRules (harmless once file_ok is true); ts_powerup.cpp will now be a REAL second file — which
  is faithful (the original defines GetRandomPowerup/PutPowerUp there too).
- **Docs referencing old names**: RECONSTRUCTION.md:102/111/113 and HANDOFF.md:1457/1692/1696/1731/
  1778/1783/1916 name `ts_gamerules.cpp`/`ts_teams.cpp`. HANDOFF entries are historical ledger — leave;
  update RECONSTRUCTION.md's current-status table rows. No scripts reference these filenames (audit.py
  scans directories; setup-xash-mod.sh copies built .so only).
- **ts_gamerules.h keeps its name** — 10+ includers (ts_powerup.cpp, ts_objectives.cpp, ts_teams.cpp,
  ts_buy.cpp, …); the inventory has no header ground truth, so renaming would be churn with no audit
  payoff. Note this in the commit message.
- **No entity factories / save tables move** in this group — lowest-risk of the file groups apart from
  the sheer size of the CTSGameRules method set; the two-method powerup extraction is the only piece
  that changes translation units for real code, and both methods are declared in ts_gamerules.h so
  callers (Think, CTSPowerUp::Spawn) are unaffected.
