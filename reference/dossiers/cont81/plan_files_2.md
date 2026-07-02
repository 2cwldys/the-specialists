# GROUP 2 move plan — TS_objectives.cpp consolidation

Branch: `restructure-source-parity`. READ-ONLY plan (nothing moved yet).
Repo: `/home/stevenlafl/Projects/goldsrc.net/mod-ts`. All paths repo-relative below.

## 0. Ground truth (verified against funcfile.tsv / ts_inventory.json / nm on ts_i386.so)

Original **`TS_objectives.cpp`** contains **10 classes** (not just the 4 in the group brief) +
**8 free functions** (the LINK_ENTITY_TO_CLASS C factories) + **0 file-scope globals**
(`ts_inventory.json['files']['TS_objectives.cpp']`):

Classes, in the ORIGINAL'S emission order (gcc2 emits in source order; nm address order
0x7b3cc→0x7db9c, one contiguous TU):

1. `CTSObjectiveManager` (Save/Restore/KeyValue/Spawn/Reset)
2. `CTSObjectivePointer` (KeyValue/Spawn/Use)
3. `CTSWinGiver` (Save/Restore/KeyValue/EnableObjective/Spawn/Use)
4. `CTSScoreGiver` (KeyValue/Spawn/Use)
5. `CTSObjective` — the base — (Save/Restore/KeyValue/Freeze/CanInteract(CBasePlayer*)/CanInteract(int))
6. `CTSBomb` (Save/Restore/Spawn/Precache/SetRandomWires/EnableObjective/CutWire/WaitThink/Use/KeyValue)
7. `CTSHack` (Save/Restore/Spawn/Precache/EnableObjective/WaitThink/Use/KeyValue)
8. `CVipEscape` (KeyValue/Precache/Spawn) — **no** `func_vip_safetyzone` factory exists in the
   original binary (we add one; see risks)
9. `CTSTeamEscape` (Save/Restore/Spawn/Precache/EnableObjective/WaitTouch/WaitThink/KeyValue)
10. `CTSBriefcase` (ctor/KeyValue/Spawn/FlagFall/WaitThink/CanInteract/CaptureTouch)

Free functions (factories): `ts_objective_manager, ts_objective_ptr, ts_wingiver, ts_scoregiver,
ts_bomb, ts_hack, ts_teamescape, ts_briefcase`.

**NOT in TS_objectives.cpp** (must move OUT of our files):
- `CTSTrigger` → original **`TS_entities.cpp`** (funcfile.tsv lines 89–92, 149, 162–163). This is
  GROUP 1's target file. Our CTSTrigger currently lives in `dlls/ts_objectives.cpp`.
- `CBasePlayer::CutWire(int)` (`CutWire__11CBasePlayeri @0x82fb4`) → original **`TS_player.cpp`**.
  Our definition currently lives in `dlls/ts_bomb.cpp:197-203`. Belongs to the TS_player group's file
  (our `dlls/ts_player.cpp`).

**MISSING free functions for this file: none** (AUDIT.md's missing-free-funcs table has no
`TS_objectives.cpp` row) — no future in-file additions to reserve space for beyond the classes above.

## 1. Target layout

| Our current file | Fate |
|---|---|
| `dlls/ts_objectives.cpp` | `git mv` → **`dlls/TS_objectives.cpp`** (case-exact original name; becomes the merge target) — MINUS the CTSTrigger block, which moves out |
| `dlls/ts_bomb.cpp` | merged into `dlls/TS_objectives.cpp`, then deleted (except `CBasePlayer::CutWire` → `dlls/ts_player.cpp`) |
| `dlls/ts_briefcase.cpp` | merged into `dlls/TS_objectives.cpp`, then deleted |
| `dlls/ts_escape.cpp` | merged into `dlls/TS_objectives.cpp`, then deleted |
| CTSTrigger fragment (from ts_objectives.cpp) | → **`dlls/TS_entities.cpp`** (group 1's file — coordinate; see §4 step C) |

Note: `audit.py`'s `norm_file()` lowercases + strips `ts_`, so `ts_objectives.cpp` already
normalizes equal to `TS_objectives.cpp`; the case-exact rename is for SOURCE parity, not needed to
clear the audit. What clears the audit rows for `CTSBomb`/`CTSBriefcase`/`CTSTeamEscape` is the
MERGE; what clears `CTSTrigger` is its move to TS_entities.cpp.

### Headers: KEEP ours (all six), unchanged names

`ts_inventory.json` / `funcfile.tsv` carry **no header information** (the binary only witnesses
.cpp translation units) — the original's header layout is unknowable. Keep:
- `dlls/ts_objectives.h` (CTSObjective base + CTSObjectiveManager + TS_TEAM_ANY) — included by
  `ts_gamerules.cpp`, `ts_hack.h`, `ts_wingiver.h`, `ts_bomb.h`, `ts_escape.h`
- `dlls/ts_bomb.h`, `dlls/ts_escape.h`, `dlls/ts_hack.h`, `dlls/ts_wingiver.h`
- `dlls/ts_briefcase.h` — included by `ts_flag.cpp` (external consumer — must keep)

`audit.py our_class_files()` only scans `.cpp`, so headers never affect FILE-mismatch rows.
(Optional later cleanup: fold ts_bomb.h/ts_escape.h/ts_hack.h/ts_wingiver.h into ts_objectives.h;
NOT part of this move — keeping them preserves the audit's member visibility per the
`audit-py blind spots` memory: members must stay declared in a `.h`.)

## 2. Exhaustive fragment inventory (source file:lines → target)

### 2a. `dlls/ts_objectives.cpp` (559 lines) → `dlls/TS_objectives.cpp`

| Fragment | Lines | Target / action |
|---|---|---|
| Banner comment | 1–15 | keep; extend to note the file now holds the full original TU (bomb/hack/escape/briefcase merged in) |
| Includes | 16–25 | keep; UNION with incoming includes (see §2e) |
| `#define SF_TS_REMOVE_ON_FIRE 1` | 27 | keep (file-scope, unused elsewhere) |
| CTSObjective save table + IMPLEMENT_SAVERESTORE | 29–32 | keep, position per original order (§3) |
| `CTSObjective::KeyValue` | 34–43 | keep |
| `CTSObjective::CanInteract(int)` | 45–51 | keep |
| `CTSObjective::CanInteract(CBasePlayer*)` | 53–57 | keep |
| `CTSObjective::Freeze` | 59–65 | keep |
| CTSObjectiveManager banner | 67–70 | keep |
| **CTSTrigger banner + `#define SF_TS_TRIGGER_FLAGCARRIER 8`** | 72–78 | **MOVE → dlls/TS_entities.cpp** |
| **`class CTSTrigger` definition (file-local)** | 80–91 | **MOVE → dlls/TS_entities.cpp** |
| **`LINK_ENTITY_TO_CLASS(ts_trigger, CTSTrigger);`** | 93 | **MOVE → dlls/TS_entities.cpp** |
| **`CTSTrigger::Spawn`** | 95–108 | **MOVE → dlls/TS_entities.cpp** |
| **`CTSTrigger::WaitThink`** | 110–115 | **MOVE → dlls/TS_entities.cpp** |
| **`CTSTrigger::KeyValue`** | 117–125 | **MOVE → dlls/TS_entities.cpp** (chains to `CTSObjective::KeyValue` → TS_entities.cpp must `#include "ts_objectives.h"` + link gate note in §5) |
| **`CTSTrigger::WaitTouch`** | 127–146 | **MOVE → dlls/TS_entities.cpp** |
| `LINK_ENTITY_TO_CLASS(ts_objective_manager, …)` | 148–149 | keep |
| CTSObjectiveManager save table + IMPLEMENT_SAVERESTORE | 151–157 | keep |
| `CTSObjectiveManager::Spawn` | 159–164 | keep |
| `CTSObjectiveManager::KeyValue` | 166–188 | keep |
| `CTSObjectiveManager::Reset` | 190–200 | keep |
| `TSFireTimeLeftTarget()` (free fn, OUR invention — no original counterpart) | 202–212 | keep in TS_objectives.cpp for now; flag as INVENTED (future group-3 work folds it into `CTSGameRules::RoundTimeLeft`) |
| CTSObjectivePointer banner + file-local class def | 214–226 | keep |
| `LINK_ENTITY_TO_CLASS(ts_objective_ptr, …)` | 228 | keep |
| `CTSObjectivePointer::Spawn` | 230–235 | keep |
| `CTSObjectivePointer::Use` | 237–262 | keep |
| `CTSObjectivePointer::KeyValue` | 264–270 | keep (has a file-local `static const char *const keys[5]` INSIDE the fn — safe) |
| CTSHack banner + LINK + save table | 272–285 | keep |
| `CTSHack::Precache/Spawn/KeyValue/EnableObjective/WaitThink/Use` | 287–407 | keep |
| CTSWinGiver banner + LINK + save table | 409–426 | keep |
| `CTSWinGiver::EnableObjective/Spawn/KeyValue/Use` | 428–471 | keep — **REWORD line 411 comment** `CTSGameRules::TeamWin(team, cash, …)` → e.g. `CTSGameRules TeamWin (team, cash, …)`: audit's `our_class_files()` regex `\bC\w+::\w+\s*\(` matches COMMENTS, and this is exactly why AUDIT.md lists `dlls/ts_objectives.cpp` under CTSGameRules' FILE mismatch (no CTSGameRules method is actually defined here — verified by grep). Rewording removes a false positive from group 3's table for free. |
| CTSScoreGiver banner + `#define SF_SCOREGIVER_RESPAWN 1` + LINK | 473–483 | keep |
| `CTSScoreGiver::KeyValue/Spawn/Use` | 485–516 | keep |
| CVipEscape banner + file-local class def + LINK | 518–532 | keep (LINK is an invention vs binary — see §5) |
| `CVipEscape::KeyValue/Precache/Spawn` | 534–557 | keep |

### 2b. `dlls/ts_bomb.cpp` (205 lines) → merge into `dlls/TS_objectives.cpp` (CTSBomb section)

| Fragment | Lines | Target |
|---|---|---|
| Banner (provenance addrs) | 1–18 | fold the address annotations into the CTSBomb section banner |
| Includes | 19–27 | union (§2e) — brings in `explode.h` (ExplosionCreate) |
| `#define BOMB_TEAM_ANY (-1)` | 29 | CTSBomb section top (unused — candidate for deletion, but keep = zero-risk; no collision with `TS_TEAM_ANY` in ts_objectives.h) |
| `#define BOMB_COUNTDOWN 45.0f` | 30 | CTSBomb section top |
| `enum tsbomb_wire_t { … }` | 32–33 | CTSBomb section top (file-scope enum; no collision — unique to bomb) |
| `#include "ts_bomb.h"` | 35 | into the merged include block (§2e) |
| `LINK_ENTITY_TO_CLASS(ts_bomb, CTSBomb)` | 37 | CTSBomb section |
| Save table + IMPLEMENT_SAVERESTORE | 39–43 | CTSBomb section (FIRST, matching original Save/Restore-first order) |
| `CTSBomb::Precache` | 45–48 | CTSBomb section |
| `CTSBomb::KeyValue` | 50–56 | CTSBomb section |
| `CTSBomb::Spawn` | 58–68 | CTSBomb section |
| `CTSBomb::SetRandomWires` | 70–79 | CTSBomb section |
| `CTSBomb::EnableObjective` | 81–106 | CTSBomb section |
| `CTSBomb::CutWire` | 108–138 | CTSBomb section |
| `CTSBomb::Use` | 140–169 | CTSBomb section |
| `CTSBomb::WaitThink` | 171–195 | CTSBomb section |
| **`CBasePlayer::CutWire(int)`** | 197–203 | **MOVE → `dlls/ts_player.cpp`** (original: `TS_player.cpp` per funcfile.tsv). Needs `ts_bomb.h` (for the `CTSBomb*` cast + `CutWire(int, CBasePlayer*)`) — add `#include "ts_bomb.h"` to ts_player.cpp. If the TS_player group is planned separately, hand this fragment to it; otherwise do it in this group's step B. |

Then `git rm dlls/ts_bomb.cpp`.

### 2c. `dlls/ts_briefcase.cpp` (209 lines) → merge into `dlls/TS_objectives.cpp` (CTSBriefcase section, LAST class)

| Fragment | Lines | Target |
|---|---|---|
| Banner (addrs 0x7d788…0x7db9c) | 1–12 | fold into CTSBriefcase section banner |
| Includes | 13–22 | union (§2e) — brings in `ts_flag.h`, `ts_briefcase.h`, `UserMessages.h` |
| `int CTSBriefcase::s_nextFlagID = 0;` (static member def, @0x1779a0) | 24–25 | CTSBriefcase section top |
| `extern int gmsgTSHaveFlag;` | 27–28 | CTSBriefcase section top |
| `static int gmsgTSVoiceMessage = 0;` (file-scope static) | 30–31 | CTSBriefcase section top — no collision (grep: only ts_briefcase.cpp defines it) |
| `LINK_ENTITY_TO_CLASS(ts_briefcase, CTSBriefcase)` | 33 | CTSBriefcase section |
| ctor `CTSBriefcase::CTSBriefcase()` | 35–41 | CTSBriefcase section |
| `CTSBriefcase::KeyValue` | 43–59 | CTSBriefcase section |
| `CTSBriefcase::Spawn` | 61–88 | CTSBriefcase section |
| `CTSBriefcase::FlagFall` | 90–112 | CTSBriefcase section |
| `CTSBriefcase::WaitThink` | 114–118 | CTSBriefcase section |
| `CTSBriefcase::CanInteract` | 120–126 | CTSBriefcase section |
| `CTSBriefcase::CaptureTouch` | 128–209 | CTSBriefcase section |

Then `git rm dlls/ts_briefcase.cpp`.

### 2d. `dlls/ts_escape.cpp` (129 lines) → merge into `dlls/TS_objectives.cpp` (CTSTeamEscape section)

| Fragment | Lines | Target |
|---|---|---|
| Banner (addrs 0x8d390…) | 1–15 | fold into CTSTeamEscape section banner |
| Includes | 16–25 | union (§2e) — brings in `ts_escape.h`; rest already present |
| `LINK_ENTITY_TO_CLASS(ts_teamescape, CTSTeamEscape)` | 27 | CTSTeamEscape section |
| Save table + IMPLEMENT_SAVERESTORE | 29–32 | CTSTeamEscape section (first) |
| `CTSTeamEscape::KeyValue` | 34–38 | CTSTeamEscape section |
| `CTSTeamEscape::Spawn` | 40–47 | CTSTeamEscape section |
| `CTSTeamEscape::EnableObjective` | 49–70 | CTSTeamEscape section |
| `CTSTeamEscape::WaitTouch` | 72–119 | CTSTeamEscape section |
| `CTSTeamEscape::WaitThink` | 121–128 | CTSTeamEscape section |

Then `git rm dlls/ts_escape.cpp`.

### 2e. Merged include block for `dlls/TS_objectives.cpp` (order matters)

```c
#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "player.h"
#include "gamerules.h"
#include "ts_gamerules.h"    // TSGameRules()->TeamWin / GetTeamIndex / m_teamScore
#include "ts_utils.h"        // TSHudMessageAll
#include "explode.h"         // ExplosionCreate (CTSBomb detonation)
#include "UserMessages.h"    // gmsgScoreInfo, gmsgTeamScore (CTSBriefcase)
#include "ts_flag.h"         // CTSFlag::CreateFlag, HaveFlag, s_flag_status — MUST precede ts_briefcase.h
#include "ts_objectives.h"   // CTSObjective base + CTSObjectiveManager + TS_TEAM_ANY
#include "ts_hack.h"         // CTSHack
#include "ts_wingiver.h"     // CTSWinGiver / CTSScoreGiver
#include "ts_bomb.h"         // CTSBomb
#include "ts_escape.h"       // CTSTeamEscape
#include "ts_briefcase.h"    // CTSBriefcase (needs s_flag_status from ts_flag.h)
```

Include-order dependency (the one real one): `ts_briefcase.h` uses `s_flag_status` by value and
does NOT include `ts_flag.h` itself — `ts_flag.h` must come first (ts_briefcase.cpp already relied
on this order). `ts_bomb.h`/`ts_escape.h`/`ts_hack.h`/`ts_wingiver.h` each self-include
`ts_objectives.h` (guarded), so their relative order is free. `enum tsbomb_wire_t` is NOT used by
`ts_bomb.h` (members are `int m_wires[3]`), so the enum may sit in the CTSBomb section rather than
above the include block.

### 2f. File-scope name-collision audit for the merge — result: NO collisions

Checked every file-scope symbol/macro across the four files:
- macros: `SF_TS_REMOVE_ON_FIRE`(objectives), `SF_SCOREGIVER_RESPAWN`(objectives),
  `BOMB_TEAM_ANY`/`BOMB_COUNTDOWN`(bomb), `SF_TS_TRIGGER_FLAGCARRIER`(moves out with CTSTrigger) — all unique.
- file-scope types: `enum tsbomb_wire_t`(bomb), file-local classes `CTSObjectivePointer`,
  `CVipEscape` (objectives; stay file-local) — unique.
- file-scope statics/globals: `static int gmsgTSVoiceMessage`(briefcase),
  `int CTSBriefcase::s_nextFlagID`(briefcase), `extern int gmsgTSHaveFlag`(briefcase, defined in ts_flag.cpp) — unique.
- both `TS_TEAM_ANY`(-1, ts_objectives.h) and `BOMB_TEAM_ANY`(-1, bomb) exist post-merge — different
  names, same value, no macro clash (cleanup candidate only).

## 3. Class order inside the new `dlls/TS_objectives.cpp` (mirror the original TU)

1. includes + `SF_TS_REMOVE_ON_FIRE`
2. `CTSObjectiveManager` (LINK, save table, Spawn, KeyValue, Reset) + `TSFireTimeLeftTarget` (our helper, keep adjacent to the manager)
3. `CTSObjectivePointer` (file-local class def, LINK, Spawn, Use, KeyValue)
4. `CTSWinGiver` (LINK, save table, EnableObjective, Spawn, KeyValue, Use)
5. `CTSScoreGiver` (`SF_SCOREGIVER_RESPAWN`, LINK, KeyValue, Spawn, Use)
6. `CTSObjective` base (save table, KeyValue, CanInteract×2, Freeze)
7. `CTSBomb` (defines/enum, LINK, save table, Precache, KeyValue, Spawn, SetRandomWires, EnableObjective, CutWire, Use, WaitThink)
8. `CTSHack` (LINK, save table, Precache, Spawn, KeyValue, EnableObjective, WaitThink, Use)
9. `CVipEscape` (file-local class def, LINK, KeyValue, Precache, Spawn)
10. `CTSTeamEscape` (LINK, save table, KeyValue, Spawn, EnableObjective, WaitTouch, WaitThink)
11. `CTSBriefcase` (static-member def, gmsg statics, LINK, ctor, KeyValue, Spawn, FlagFall, WaitThink, CanInteract, CaptureTouch)

(Original per-class method emission order is listed in §0; matching it inside each section is
cheap while we're moving the text anyway, but the audit does not check intra-file order.)

## 4. Execution order + CMakeLists edits + verification (build stays green after each step)

`dlls/CMakeLists.txt` current lines: 118 `ts_briefcase.cpp`, 119 `ts_bomb.cpp`,
121 `ts_escape.cpp`, 138 `ts_objectives.cpp`.

**Step A — rename the anchor file.**
`git mv dlls/ts_objectives.cpp dlls/TS_objectives.cpp`; edit CMakeLists line 138
`ts_objectives.cpp` → `TS_objectives.cpp`.
Build: `scripts/build.sh amd64`. Verify:
`nm -D build-amd64/dlls/hl.so | grep -E " T (ts_objective_manager|ts_objective_ptr|ts_hack|ts_wingiver|ts_scoregiver|ts_trigger|func_vip_safetyzone)$"` → 7 factories.

**Step B — merge ts_bomb.cpp (and re-home CBasePlayer::CutWire).**
Append the CTSBomb section (fragments §2b) to `dlls/TS_objectives.cpp`; move
`CBasePlayer::CutWire` (ts_bomb.cpp:197–203) to `dlls/ts_player.cpp` (+ `#include "ts_bomb.h"`
there); add `explode.h`+`ts_bomb.h` to the include block; `git rm dlls/ts_bomb.cpp`; delete
CMakeLists line 119.
Build. Verify: `nm -D … | grep " T ts_bomb$"` present; `nm build-amd64/dlls/hl.so | c++filt |
grep "CBasePlayer::CutWire"` present; the 7 factories from step A unchanged.

**Step C — move CTSTrigger out (coordinate with GROUP 1).**
Move ts_objectives.cpp fragments lines 72–146 (pre-rename numbering; the block is identical after
step A) into `dlls/TS_entities.cpp`. If group 1 has already created `dlls/TS_entities.cpp`, append
there (its include block must gain `ts_objectives.h` + `ts_utils.h`); if not, create
`dlls/TS_entities.cpp` containing just the CTSTrigger block + includes
(`extdll/util/cbase/player/gamerules/ts_gamerules/ts_utils/ts_objectives.h`) and add it to
CMakeLists (group 1 then merges the other 9 TS_entities classes into it).
Build. Verify: `nm -D … | grep " T ts_trigger$"` still exported.

**Step D — merge ts_escape.cpp.**
Append CTSTeamEscape section (§2d) — insert BEFORE the CTSBriefcase section once D+E are done, or
just append and reorder in step F; add `ts_escape.h` include; `git rm dlls/ts_escape.cpp`; delete
CMakeLists line 121. Build. Verify `" T ts_teamescape$"`.

**Step E — merge ts_briefcase.cpp.**
Append CTSBriefcase section (§2c); add `ts_flag.h`/`UserMessages.h`/`ts_briefcase.h` includes
(ts_flag.h BEFORE ts_briefcase.h); `git rm dlls/ts_briefcase.cpp`; delete CMakeLists line 118.
Build. Verify `" T ts_briefcase$"`.

**Step F — order + comment pass (no code motion across files).**
Arrange sections per §3; reword the `CTSGameRules::TeamWin(` comment (§2a) so audit stops
attributing CTSGameRules to this file. Build once more.

**Final verification:**
1. `scripts/build.sh amd64` green; `nm -D build-amd64/dlls/hl.so | grep -E " T (ts_bomb|ts_briefcase|ts_teamescape|ts_trigger|ts_objective_manager|ts_objective_ptr|ts_hack|ts_wingiver|ts_scoregiver|func_vip_safetyzone)$" | wc -l` → **10** (same set as the pre-move baseline, captured above).
2. `nm build-amd64/dlls/hl.so | c++filt | grep -cE "(CTSBomb|CTSBriefcase|CTSTeamEscape|CTSTrigger|CTSObjective|CTSHack|CTSWinGiver|CTSScoreGiver|CVipEscape)::"` — compare count before/after (no methods dropped).
3. `python3 scripts/coverage/audit.py` → AUDIT.md FILE-mismatch rows for `CTSBomb`, `CTSBriefcase`, `CTSTeamEscape`, `CTSTrigger` gone; no NEW rows.
4. Runtime smoke (per CLAUDE.md §3): load `tm_avana` (places ts_hack/ts_objective_manager/ts_objective_ptr/ts_wingiver) — no Host_Error/"Can't init".

## 5. RISKS

- **Cross-group coupling (CTSTrigger)**: CTSTrigger derives `CTSObjective` and chains
  `CTSObjective::KeyValue`, so group 1's `TS_entities.cpp` gains a dependency on
  `ts_objectives.h` (and on `hl.so` linking TS_objectives.cpp — same lib, fine). Whichever group
  executes first creates/edits `dlls/TS_entities.cpp`; the other must rebase. Same for
  `CBasePlayer::CutWire` → `dlls/ts_player.cpp` (TS_player group): ts_player.cpp gains
  `#include "ts_bomb.h"`.
- **Include order**: `ts_flag.h` must precede `ts_briefcase.h` (s_flag_status by value); everything
  else is order-free (all headers self-include ts_objectives.h with guards).
- **Statics/macros**: audited §2f — zero collisions. Watch only that `static int gmsgTSVoiceMessage`
  stays `static` (a second definition exists nowhere, but if a later flag/voice refactor adds a
  global of the same name, the static shadows it silently).
- **Audit false positive via comments**: `our_class_files()` regex matches `Class::Method(`
  in COMMENTS. Line 411's `CTSGameRules::TeamWin(…)` is the sole reason AUDIT lists
  ts_objectives.cpp under CTSGameRules today. Reword it (step F) or the mismatch follows the
  renamed file and pollutes group 3's table. When writing merged section banners, avoid
  `Class::Method(` for any class not defined in this file.
- **Known deviations that will remain (deliberate, document in the banner)**:
  - `LINK_ENTITY_TO_CLASS(func_vip_safetyzone, CVipEscape)` — the original binary exports NO
    factory for CVipEscape (`nm` confirms; inventory free-funcs list has only 8). Ours is an
    invention (audit counts it as an extra free func). Keep for map-spawnability, or drop to match
    the binary — decide with the audit owner; NOT silently.
  - `TSFireTimeLeftTarget()` — invented helper (original fires the manager's target inside
    CTSGameRules::RoundTimeLeft). Group 3's gamerules work should absorb it.
  - `ObjectCaps` overrides live in headers (inline) — original emits `ObjectCaps__7CTSBomb` etc.
    in the TU; ours are inline in .h. Audit tolerates (methods matched by name), noting per the
    `audit-py blind spots` memory that inline methods are invisible to some views.
- **Docs/scripts referencing the old file names** (update or accept staleness — none are
  load-bearing for the build): `HANDOFF.md`, `reference/RECONSTRUCTION.md`,
  `reference/coverage/{AUDIT,LAYOUT_MAP,INVENTORY,SDK_DIVERGENCE}.md` (all regenerated or prose),
  `.serena/memories/ledger.md` (historical — leave). No script hardcodes `ts_bomb.cpp` etc.
  (grep verified: only generated coverage artifacts + prose docs).
- **Case-only rename**: fine on this Linux checkout; if the repo is ever checked out
  case-insensitively (macOS/Windows), `ts_objectives.cpp` → `TS_objectives.cpp` in one commit via
  `git mv` is still safe (git tracks it as a rename).
- **Save/restore**: all TYPEDESCRIPTION tables + IMPLEMENT_SAVERESTORE move verbatim — no
  field-name or class-hierarchy change, so saved games stay compatible.

## Approx fragment move count

ts_bomb.cpp 16 + ts_briefcase.cpp 13 + ts_escape.cpp 9 + CTSTrigger block out 7 + include-block
union 1 + comment reword 1 + CMake edits 4 (1 rename + 3 removals) ≈ **51 fragments**.
