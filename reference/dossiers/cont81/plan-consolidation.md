# MASTER file-consolidation plan — mod-ts source parity (merged from plan_files_1..6)

Repo: `/home/stevenlafl/Projects/goldsrc.net/mod-ts`, branch `restructure-source-parity`, HEAD `05be983`.
Ground truth: `reference/coverage/funcfile.tsv`, `ts_inventory.json`, `reference/coverage/AUDIT.md`
(22 FILE mismatches, 2 MISSING files today). Every commit below is build-green
(`scripts/build.sh amd64`, plus `all` where the client is touched) and audit-monotone
(a FILE-mismatch row disappears or stays; none appears).

Detail authority: each commit cites its group plan (`plan_files_N.md §…`) for the exact
fragment tables (file:line → target). This document adds the ORDER, the conflict
resolutions, and the per-commit CMake/nm gates.

---

## 0. Cross-group conflict resolutions (binding decisions)

### 0.1 `ts_objectives.cpp` — claimed by GROUP 1 (CTSTrigger → TS_entities.cpp) and GROUP 2 (merge target)

Both plans agree CTSTrigger belongs in the original `TS_entities.cpp` (funcfile.tsv). Resolution:

- **GROUP 2 owns the objectives file** (rename → `TS_objectives.cpp`, then bomb/escape/briefcase merge in).
- **GROUP 1 owns `TS_entities.cpp`** (creates it from the 9 donor files).
- **The CTSTrigger extraction happens exactly ONCE, in its own commit (C13)**, sequenced AFTER
  both the objectives rename (C11) and the TS_entities.cpp creation (C12). It cuts the block
  (CTSTrigger banner + `SF_TS_TRIGGER_FLAGCARRIER` + class def + LINK + Spawn/WaitThink/KeyValue/WaitTouch,
  = pre-rename ts_objectives.cpp lines 72–146) from `TS_objectives.cpp` and inserts it at the TOP of the
  class sequence in `TS_entities.cpp` (original address order: ts_trigger 0x6086c is first).
  Group 1's plan already puts `#include "ts_objectives.h"` in TS_entities.cpp's include union from
  day one (C12), so C13 is a pure cut+paste — nothing is lost, nothing defined twice.
  ts_objectives include block + `SF_TS_REMOVE_ON_FIRE` stay (used by remaining classes).

### 0.2 `ts_powerup.cpp` — touched by GROUP 3 (receives CTSGameRules::GetRandomPowerup/PutPowerUp + `g_iNoLowGravPowerup`) and GROUP 4 (loses CTSSuperJump)

Disjoint regions, but line numbers shift. Resolution:

- **GROUP 3 first (C5)**: append `static byte g_iNoLowGravPowerup` + `CTSGameRules::GetRandomPowerup`
  above/near the top, and `CTSGameRules::PutPowerUp` AFTER `TSCreatePowerUpAt` (currently line 295);
  delete the stale fwd-decl from the gamerules file. This is faithful — the ORIGINAL also defines
  these two CTSGameRules methods in TS_powerup.cpp.
- **GROUP 4 second (C8)**: remove the CTSSuperJump block → hldmcompat.cpp. **Re-locate the block by
  content (`grep -n CTSSuperJump dlls/ts_powerup.cpp`), NOT by the stale plan line numbers 441–456**
  (they shift after C5). Nothing else in ts_powerup.cpp references CTSSuperJump (verified in plan 4).
- Both plans' invariant holds at every point: exactly one definition each of GetRandomPowerup,
  PutPowerUp, CTSSuperJump::Spawn, and the `ts_powerup`/`ts_superjump` factories.

### 0.3 `InitWeaponRestriction` — GROUP 1 wanted to "park" it; GROUP 3 creates its true home

funcfile.tsv: `InitWeaponRestriction__Fv → TS_gamerules.cpp` (the free-function file). Resolution:
**no parking.** The master order runs GROUP 3's `TS_gamerules.cpp` scaffold (C6) BEFORE GROUP 1's
TS_entities merge (C12), so C12 moves `InitWeaponRestriction` (ts_mapglobals.cpp:60–66) **directly
into `dlls/TS_gamerules.cpp`** — defined once, never duplicated. Its caller `get_weapon_restriction`
moves into TS_entities.cpp (per group 1); add a one-line prototype `void InitWeaponRestriction();`
above it in TS_entities.cpp (or in ts_gamerules.h) — note it in the banner.

### 0.4 `CBasePlayer::CutWire(int)` — GROUP 2 finding, lands in `dlls/ts_player.cpp`

Original file: `TS_player.cpp`. No group otherwise touches ts_player.cpp → **GROUP 2's bomb-merge
commit (C14) does the move** (ts_bomb.cpp:197–203 → ts_player.cpp + `#include "ts_bomb.h"` there).

### 0.5 Case-only rename hazard (`ts_gamerules.cpp` vs new `TS_gamerules.cpp`)

C4 renames `ts_gamerules.cpp` → `TS_gamerulesclass.cpp` BEFORE C6 creates `TS_gamerules.cpp`.
The two names never coexist in any commit — keep this order (protects case-insensitive checkouts).

### 0.6 Audit comment false positives

`audit.py our_class_files()`'s regex matches `Class::Method(` **inside comments**. The reword pass
(C17) fixes the known offenders (`CTSGameRules::TeamWin(` at TS_objectives.cpp, plus the
`CTSGameRules::…(`-form comments in ts_powerup.cpp and two more in TS_objectives.cpp). Rule for
ALL merged section banners written during C1–C16: never write `Class::Method(` for a class not
defined in that file.

### 0.7 client.dll HUD classes — not movable

`client.dll` has no symbols/paths; original client file names are unknowable. GROUP 6a resolves the
2 FILE rows + the `client.dll`/`pm_shared.c` MISSING-file artifacts via an **audit.py patch (C19)**
(skip `__user_type_info` gcc2-RTTI junk class; accepted-mapping table for CHudTSBuyMenu/CHudTSWeaponBar),
not by touching the tree.

---

## 1. Ordered commit sequence

**Pre-flight (no commit):** baseline capture —
`scripts/build.sh amd64 all` green;
`nm -D build-amd64/dlls/hl.so | awk '$2=="T"{print $3}' | sort > /tmp/nm_T_baseline.txt`;
`nm build-amd64/dlls/hl.so | c++filt | grep -oE '\bC[A-Za-z]+::[~A-Za-z0-9_]+' | sort | uniq -c > /tmp/methods_baseline.txt`;
`python3 scripts/coverage/audit.py` (confirm 22 FILE mismatches).

**Global gate after EVERY commit:**
`scripts/build.sh amd64` green, then
`nm -D build-amd64/dlls/hl.so | awk '$2=="T"{print $3}' | sort | diff - /tmp/nm_T_baseline.txt` → **empty**
(no exported factory/function gained or lost — this whole plan is text motion), and
`python3 scripts/coverage/audit.py` → FILE-mismatch count monotonically decreasing, MISSING/INVENTED
method tables byte-identical. Per-commit checks below are IN ADDITION.

| # | commit (group §) | what moves | CMakeLists delta (dlls/ unless noted) | nm / audit check |
|---|---|---|---|---|
| **C1** | bullet rename (G5 §2A) | `git mv ts_bulletmgr.cpp ts_bullet.cpp` + `ts_bulletmgr.h→ts_bullet.h`; fix guard + 3 include sites (ts_bullet.cpp, ts_gun.cpp:23, ts_gamerules.cpp:12) | line 141 `ts_bulletmgr.cpp`→`ts_bullet.cpp` | `nm -C hl.so \| grep -c 'CBulletManager::'` ≥6; `grep -rn ts_bulletmgr dlls/ cl_dll/` → comments only. Clears **CBulletManager** |
| **C2** | fog merge (G5 §2B) | ts_fog.cpp (whole, 26 L) → end of ts_emitter.cpp; `git rm ts_fog.cpp` | delete line 111 `ts_fog.cpp` | `nm -D \| grep -wE 'T (ts_fog\|ts_emitter\|ts_particle\|ts_wind)'` → 4; `CTSFog::Spawn` present. Clears **CTSFog** |
| **C3** | knife merge (G5 §2C) | ts_knife.cpp (whole, 334 L) → end of ts_grenades.cpp (+`#include "ts_gun.h"` there; statics KnifeHitPitch/KnifeGlances precede callers); `git rm ts_knife.cpp` | delete line 143 `ts_knife.cpp` | `T knife` + `T shrapnel` exported; `grep -c 'CTSKnife::'` == baseline (12); `TSThrowKnife` T. Clears **CTSKnife** |
| **C4** | gamerules rename (G3 step 1) | `git mv ts_gamerules.cpp TS_gamerulesclass.cpp` (content untouched) | line 113 → `TS_gamerulesclass.cpp` | nm counts unchanged. Clears **CTSGameRules** (file_ok now true; ts_teams still listed as extra our_file — harmless until C7) |
| **C5** | powerup extraction (G3 step 2; conflict 0.2 first-mover) | `g_iNoLowGravPowerup` + `CTSGameRules::GetRandomPowerup` (+comments) + `CTSGameRules::PutPowerUp` → ts_powerup.cpp (PutPowerUp AFTER `TSCreatePowerUpAt`); delete the `TSCreatePowerUpAt` fwd-decl from TS_gamerulesclass.cpp | none | `nm -C \| grep -E 'GetRandomPowerup\|PutPowerUp'` → 2 defs; `T ts_powerup` intact |
| **C6** | TS_gamerules.cpp scaffold (G3 step 3; enables 0.3) | new `dlls/TS_gamerules.cpp` (banner + extdll/util/cbase/player/ts_flag.h + `HaveFlag` from TS_gamerulesclass.cpp:54–60); banner records the 9 blocked-on-RE free funcs destined here | add `TS_gamerules.cpp` next to line 113 | `nm -C \| grep HaveFlag` still defined (ts_briefcase.cpp:137 links) |
| **C7** | ts_teams fold-in (G3 step 4) | all 21 methods (ts_teams.cpp:27–330) → TS_gamerulesclass.cpp; include union adds `UserMessages.h` + `ts_weapons.h`; `git rm ts_teams.cpp` | delete line 114 `ts_teams.cpp` | `nm -C \| grep -c 'CTSGameRules::'` == baseline |
| **C8** | CTSSuperJump → hldmcompat.cpp (G4 move C; conflict 0.2 second-mover) | CTSSuperJump banner + class + `LINK(ts_superjump,…)` out of ts_powerup.cpp (LOCATE BY GREP, lines shifted by C5) → hldmcompat.cpp after last spawner, before `hldmcompat_keep_unlinked()`; hldmcompat gains `#include "ts_gamerules.h"` THEN `"ts_powerup.h"` (order: TS_PWUP_* macros) | none | `T ts_superjump` exported; `CTSSuperJump::` present (Spawn may be W/weak — was inline before too). Clears **CTSSuperJump** |
| **C9** | groundweapon merge (G4 move A) | ts_groundweapon.cpp (whole, 230 L) → ts_worldgun.cpp after CTSAmmoPack; ts_worldgun.cpp gains `#include "ts_gamerules.h"` + `"ts_groundweapon.h"`; `git rm` | delete line 130 | `T ts_groundweapon` + `TSWorldGun` + `TSAmmoPack`; all 12 `CTSGroundWeapon::` methods. Clears **CTSGroundWeapon** |
| **C10** | groundammo merge (G4 move B) | ts_groundammo.cpp (whole, 156 L) → ts_worldgun.cpp after CTSGroundWeapon (original order, last); add `#include "ts_groundammo.h"`; `git rm` | delete line 129 | `T ts_groundammo`; `CTSGroundAmmo::` methods. Clears **CTSGroundAmmo** |
| **C11** | objectives rename (G2 step A) | `git mv ts_objectives.cpp TS_objectives.cpp` | line 138 → `TS_objectives.cpp` | 7 factories: `ts_objective_manager ts_objective_ptr ts_hack ts_wingiver ts_scoregiver ts_trigger func_vip_safetyzone` |
| **C12** | TS_entities.cpp 9-file merge (G1 step 1 + conflict 0.3) | `git mv ts_slowmotion.cpp TS_entities.cpp` (blame anchor), append in original address order: slowmotionpoint, dmhill, model, mapglobals (**minus InitWeaponRestriction → `TS_gamerules.cpp` directly**, + `void InitWeaponRestriction();` prototype before `get_weapon_restriction`), sellzone, playeramount, timer, customtarget; hoist include union (incl. `ts_objectives.h`, ready for C13); `git rm` the other 8; `// CTSTrigger lands here (C13)` marker at top of class sequence. KEEP all 5 per-class headers (client.cpp:43 includes ts_playeramount.h) | delete lines 108,109,110,112,120,122,126,127,137; add `TS_entities.cpp` near line 138 | 9 factories `T ts_slowmotion ts_slowmotionpoint ts_dmhill ts_model ts_mapglobals ts_sellzone ts_playeramount ts_timer ts_customtarget`; `nm -C \| grep InitWeaponRestriction` → 1 def. Clears **9 rows** (CTSSlowMotion, CTSSlowMotionPoint, CTSDMHill, CTSModel, CTSMapGlobals, CTSSellZone, CTSPlayerAmount, CTSTimer, CTSCustomTarget) |
| **C13** | CTSTrigger extraction (conflict 0.1 — single owner; = G1 step 2 = G2 step C) | cut CTSTrigger block from TS_objectives.cpp → top of TS_entities.cpp class sequence; ts_objectives includes + `SF_TS_REMOVE_ON_FIRE` untouched | none | `T ts_trigger` still exported; `CTSTrigger::` methods present; TS_objectives.cpp still builds. Clears **CTSTrigger** |
| **C14** | bomb merge + CutWire re-home (G2 step B + conflict 0.4) | ts_bomb.cpp → CTSBomb section of TS_objectives.cpp (include union adds `explode.h` + `ts_bomb.h`); `CBasePlayer::CutWire` (ts_bomb.cpp:197–203) → ts_player.cpp (+`#include "ts_bomb.h"`); `git rm ts_bomb.cpp` | delete line 119 | `T ts_bomb`; `nm -C \| grep 'CBasePlayer::CutWire'` present. Clears **CTSBomb** |
| **C15** | escape merge (G2 step D) | ts_escape.cpp → CTSTeamEscape section (+`ts_escape.h` include); `git rm` | delete line 121 | `T ts_teamescape`. Clears **CTSTeamEscape** |
| **C16** | briefcase merge (G2 step E) | ts_briefcase.cpp → CTSBriefcase section, LAST class (include order: `ts_flag.h` BEFORE `ts_briefcase.h` — s_flag_status; also `UserMessages.h`); statics `s_nextFlagID` def + `static gmsgTSVoiceMessage` + `extern gmsgTSHaveFlag` move with it; `git rm` | delete line 118 | `T ts_briefcase`; `CTSBriefcase::` count unchanged. Clears **CTSBriefcase** |
| **C17** | order + comment pass (G2 step F + conflict 0.6; optional G3 step 6) | arrange TS_objectives.cpp sections to the original TU order (§3 of plan 2); reword `CTSGameRules::TeamWin(` + the other 3 `CTSGameRules::…(` comment offenders (TS_objectives.cpp ×3, ts_powerup.cpp:292); optionally reorder TS_gamerulesclass.cpp methods to binary order | none | build green; nm identical; audit our_files for CTSGameRules no longer lists TS_objectives.cpp/ts_powerup.cpp |
| **C18** | pm_shared carve-out (G6b step A — ATOMIC, touches BOTH libs) | new `pm_shared/TS_pm_shared.{h,cpp}`: TSPM_* macros + STUCK_MOVEUP/DOWN **moved** (not copied) to the .h; TSGoDuck/TSStandUp/TSDive/TSWallJump/TSWallJumpClimb/TSHandle (pm_shared.cpp:3063–3195) to the .cpp (drop `static` — original exports them `T`); pm_shared.cpp gains `#include "TS_pm_shared.h"`; scaffold comment (NO fake bodies) for CanJumpHigh/FixVelocity/PseudoClimb | **dlls/**: add `../pm_shared/TS_pm_shared.cpp` after line 164 + header after 254; **cl_dll/**: add after line 189 + header after 287 (BOTH or prediction splits) | `scripts/build.sh amd64 all`; the 6 TS* symbols in BOTH hl.so and client.so; T-export diff vs baseline gains exactly the de-static'd TS* set (update `/tmp/nm_T_baseline.txt` accordingly — this is the ONE deliberate export delta, matching the original's linkage); runtime: stunts predict without rubber-banding |
| **C19** | audit.py patch (G6a step B) | skip classes named `__*` in `load_original()` (kills phantom `pm_shared.c` missing-file = gcc2 `__user_type_info` RTTI junk); `client.dll` origin → accepted-mapping (`ACCEPTED_CLIENT_FILES` table, new "ACCEPTED mappings" report section); exclude client.dll from missing_files | none | `audit.py` → FILE mismatches 2→0; MISSING files 2→0; `--strict` semantics unchanged |
| **C20** | regenerate + ledger | `python3 scripts/coverage/audit.py` + `scripts/coverage/coverage.py all` → commit regenerated `reference/coverage/{AUDIT,REPORT}.md` etc. (never hand-edit); update RECONSTRUCTION.md current-status file names; Serena ledger entry | none | AUDIT.md: **0 FILE mismatches, 0 MISSING files**; coverage.py external contract byte-identical to pre-plan baseline |

Grouping note: C1–C3 are fully independent of everything else (safe warm-up); the only hard order
edges are **C4 < C6** (case rename, 0.5), **C5 < C8** (ts_powerup.cpp, 0.2), **C6 < C12** (0.3),
**C11 & C12 < C13** (0.1), **C11 < C14 < {C15,C16} < C17** (objectives merge chain). Everything else
may be reordered/parallelized if needed — but the linear order above is what to execute.

Runtime smoke points (cheap, per CLAUDE.md §3; re-run `scripts/setup-xash-mod.sh` first):
after C7 (tm_avana — team assign + round restart), after C13 (ts_wyndham — 4 ts_slowmotion, no
"Can't init" spam; tm_avana — hack/objective entities), after C16 (briefcase/CTB map), after C18
(dive/prone/wall-jump prediction).

---

## 2. What each commit does NOT do (deliberate non-goals, do not fold in)

- **Factory-contract fixes** (behavioral, separate change + runtime test each):
  `item_longjump` vs our extra `ts_superjump` export; `weapon_9MMAR` case;
  the invented `func_vip_safetyzone` factory (original has none); missing `m61_grenade` factory
  (verify original classname via RE first).
- **Method-level relocations**: CTSGameRules::PlayerSpawn/PlayerKilled → CHalfLifeMultiplay
  (TS_multiplay_gamerules.cpp) — resolves 2 of the 11 invented CTSGameRules methods; multiplay group.
- **pm_shared phase 2**: moving modified PM_Jump/PM_CheckParamters/PM_Duck/PM_WalkMove into
  TS_pm_shared.cpp (original did; ~14 extern prototypes; line ranges shift after C18 — re-derive).
- **Header folds** (5 TS_entities headers → one; ts_bomb/escape/hack/wingiver.h → ts_objectives.h):
  zero audit benefit, breaks includers; skip.
- **ShouldCollide → cbase.cpp** (G4 move D, original location per funcfile.tsv:902): optional parity
  bonus, can ride with C10 or be dropped — no audit effect.
- Implementing ANY blocked-on-RE function with a guessed body (scaffold comments only — see 0.7 /
  `flag-unfound-re-early`). Exception candidate: `FixVelocity` is verifiably a 4-byte empty ret in
  the binary — implement only after eyeballing `objdump -d --start-address=0xe8fc4`.

## 3. Residual audit state after C20 (expected, honest)

FILE mismatches: **0** (20 cleared by moves C1–C16; 2 client.dll rows via accepted-mapping C19).
MISSING files: **0** (both were audit artifacts, fixed in C19).
Unchanged by this plan (the remaining work-list, all RE-blocked or review items):

- **19 MISSING methods**: CBasePlayer ×9 (BlockPlayer, CheckForWires, CheckHull, CloseCombatAttack,
  ColdCockAttack, PickUpWeapon, ShootBulletsPlayer, TSInit, UpdateMeleeBar), CTSGun ×9 (CCombatIdle,
  EmptySound, GrenadeIdle, HolsterWeapon, KatanaFire, KatanaThrow, KnifeFire, ReloadWeapon,
  StopReload), CTSKnife::WaitTouch.
- **43 MISSING free functions** (per-file, banners in the new files record their destinations):
  TS_gun.cpp ×13, TS_util.cpp ×13, TS_gamerules.cpp ×9 (→ the C6 scaffold),
  TS_pm_shared.c ×3 (→ C18 scaffold), UTIL_TraceLine_Expanded (→ ts_bullet.cpp),
  m61_grenade (→ ts_grenades.cpp), TSGetPointsForFrag, FindHullIntersectionplayer,
  GetRandomPosition (→ ts_powerup.cpp).
- **17 MISSING globals** (gmsgTS* mostly; `gmsgTSServerSetting`+`roundtime` → TS_gamerulesclass.cpp
  when RE'd; `gmsgScoreInfo` attribution to TS_entities.cpp = accepted divergence, ours lives in
  UserMessages.h — needs an accepted-mapping note like C19's).
- **42 INVENTED methods + 6 INVENTED globals**: review list, untouched (2 CTSGameRules ones resolvable
  by the multiplay relocation; CTSKnife's 6 = documented think-based divergence; TSCreateSlowMotionPoint
  + TSFireTimeLeftTarget helpers kept with INVENTED banners).
- **UNVERIFIABLE**: 46 client.dll HUD methods (CHudTSBuyMenu/CHudTSWeaponBar) — permanently
  name-unmatchable (no client symbols); visible in the new ACCEPTED section, not counted.
- Deferred behavior fixes listed in §2 (factory contracts, PM_ phase 2).

## 4. Top execution risks (merged)

1. **Cross-group sequencing** — the five order edges in §1; violating 0.2/0.3/0.5 produces duplicate
   symbols or case-collisions. Execute linearly.
2. **Stale line numbers** — every plan's line numbers are HEAD `05be983`; C5/C8 (ts_powerup) and C13
   (post-rename TS_objectives) MUST re-locate fragments by content, not line.
3. **Prediction split-brain (C18)** — TS_pm_shared.cpp must enter BOTH CMakeLists in the same commit;
   macros must be MOVED not copied.
4. **Include-order deps** — ts_gamerules.h before ts_powerup.h (TS_PWUP_*); ts_flag.h before
   ts_briefcase.h (s_flag_status); extdll/util/cbase first everywhere.
5. **Audit regex on comments** — new section banners must avoid `OtherClass::Method(` text or new
   false FILE rows appear (0.6).
6. **Export-set drift** — the global nm gate catches any dropped factory/method immediately; C18 is
   the only commit allowed to change the T-export set (TS* de-static, matching the original).
7. **Generated docs** — AUDIT/REPORT/LAYOUT_MAP etc. are outputs; regenerate (C20), never hand-edit;
   HANDOFF/ledger references to old filenames are historical — leave.
