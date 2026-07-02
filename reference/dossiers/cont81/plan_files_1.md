# GROUP 1 move plan — consolidate into `dlls/TS_entities.cpp`

Repo: `/home/stevenlafl/Projects/goldsrc.net/mod-ts` (branch `restructure-source-parity`).
Ground truth: `reference/coverage/funcfile.tsv` + `ts_inventory.json['files']['TS_entities.cpp']` + `AUDIT.md` FILE-mismatch table (lines 68–84).

## 0. Scope correction vs. the task brief

The original `TS_entities.cpp` contains **10** classes, not 9: the 9 listed **plus `CTSTrigger`**
(funcfile.tsv + `ts_inventory.json` both place it in TS_entities.cpp; AUDIT.md line 84 flags it as a
FILE mismatch: it currently lives in `dlls/ts_objectives.cpp`). This plan moves it too — if the
group handling `TS_objectives.cpp` also claims it, resolve in favor of THIS plan (ground truth says
TS_entities.cpp).

Two cross-file findings discovered while enumerating:

1. **`InitWeaponRestriction()`** (free fn, currently `dlls/ts_mapglobals.cpp:63-66`) belongs to the
   original **`TS_gamerules.cpp`** (`InitWeaponRestriction__Fv → TS_gamerules.cpp` in funcfile.tsv).
   It must NOT go into TS_entities.cpp — hand it to the CTSGameRules group (group 3). Until that
   group lands, park it verbatim in TS_entities.cpp with a `// TODO(group3): belongs in TS_gamerules.cpp`
   comment (keeps the build green; single definition either way — coordinate so it is defined once).
2. **`gmsgScoreInfo`** — the inventory attributes this `.data` global to TS_entities.cpp
   (`{'name': 'gmsgScoreInfo', 'addr': 0x10340c, 'file': 'TS_entities.cpp'}`). In our SDK it is
   `inline int gmsgScoreInfo` in `dlls/UserMessages.h` (registered in UserMessages.cpp). Do NOT move
   it — the modern SDK centralizes user-message ids; note as an accepted divergence for the globals
   audit (the original's attribution is just where the linker placed the definition).

AUDIT.md lists **no missing free functions and no missing methods** for TS_entities.cpp classes —
this group is purely a file-consolidation, no RE blockers.

## 1. Target layout

**New file: `dlls/TS_entities.cpp`** (exact original basename, capital `TS_`; `audit.py`'s
`norm_file()` lowercases so either case matches, but use the original's casing for convention).

Class order = original binary address order (factories: ts_trigger 0x6086c → ts_slowmotion 0x60cb0 →
ts_slowmotionpoint 0x614b4 → ts_dmhill 0x618f4 → ts_model 0x622c4 → ts_mapglobals 0x62464 →
ts_sellzone 0x6269c → ts_playeramount 0x6299c → ts_timer 0x62d2c → ts_customtarget 0x62f90):

| # | class | pulled from | notes |
|---|---|---|---|
| 1 | `CTSTrigger` | `dlls/ts_objectives.cpp` (extract) | derives `CTSObjective` (stays declared in `ts_objectives.h`) |
| 2 | `CTSSlowMotion` | `dlls/ts_slowmotion.cpp` (whole file) | |
| 3 | `CTSSlowMotionPoint` | `dlls/ts_slowmotionpoint.cpp` (whole file) | incl. our `TSCreateSlowMotionPoint` helper (INVENTED wrapper, currently uncalled — keep, or drop it in a follow-up audit-of-inventions pass) |
| 4 | `CTSDMHill` | `dlls/ts_dmhill.cpp` (whole file) | |
| 5 | `CTSModel` | `dlls/ts_model.cpp` (whole file) | class declared in-.cpp — stays that way |
| 6 | `CTSMapGlobals` | `dlls/ts_mapglobals.cpp` (whole file MINUS `InitWeaponRestriction`) | see §0.1 |
| 7 | `CTSSellZone` | `dlls/ts_sellzone.cpp` (whole file) | class declared in-.cpp |
| 8 | `CTSPlayerAmount` | `dlls/ts_playeramount.cpp` (whole file) | |
| 9 | `CTSTimer` | `dlls/ts_timer.cpp` (whole file) | class declared in-.cpp |
| 10 | `CTSCustomTarget` | `dlls/ts_customtarget.cpp` (whole file) | class declared in-.cpp |

After the merge **delete** the 9 donor `.cpp` files (`git rm`); `ts_objectives.cpp` survives (loses
only the CTSTrigger block).

### Headers — KEEP all five per-class headers

`ts_inventory.json`/`funcfile.tsv` carry **no header information** (headers emit no symbols; the
original's header layout is unknowable — plausibly a single `TS_entities.h` or decls in the .cpp,
but there is no evidence either way). So keep ours and note it:

- **Keep**: `dlls/ts_slowmotion.h`, `ts_slowmotionpoint.h`, `ts_dmhill.h`, `ts_mapglobals.h`,
  `ts_playeramount.h`. Reasons: (a) `dlls/client.cpp:43` includes `ts_playeramount.h` (connect/
  disconnect relay hooks); (b) the headers were created deliberately so the member audit sees the
  fields; (c) `audit.py::our_class_files()` matches on where the out-of-line **method defs** live,
  so a kept header does not re-trigger the FILE mismatch (`file_ok = any(...)` over our files).
- Classes declared inside their .cpp (CTSModel, CTSSellZone, CTSTimer, CTSCustomTarget, CTSTrigger)
  keep their in-file declarations — just move the declaration block with the methods.
- Optional later cleanup (NOT this pass): fold the five headers into one `TS_entities.h`. Requires
  touching `client.cpp`; zero audit benefit; skip.

### File prologue of the new TS_entities.cpp

One banner comment (merge provenance: "original TS_entities.cpp — 10 entity classes, recovered from
ts_i386.so; see per-class provenance blocks below"), then the **union of includes, once, in this
order** (SDK convention: extdll → util → cbase → others):

```c
#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "player.h"
#include "gamerules.h"
#include "ts_gamerules.h"     // TSGameRules(), GlobalSlowMotion2, GetTeamIndex, IsTeamMode
#include "ts_utils.h"         // TSHudMessageAll
#include "UserMessages.h"     // gmsgScoreInfo (CTSDMHill payout)
#include "ts_objectives.h"    // CTSObjective — CTSTrigger's base
#include "ts_slowmotion.h"
#include "ts_slowmotionpoint.h"
#include "ts_dmhill.h"
#include "ts_mapglobals.h"
#include "ts_playeramount.h"
```

Per-class `#define`s move with their class block (they are all unique — see §5 collisions).
Keep each donor file's provenance banner as a per-class section comment.

## 2. Exhaustive fragment map (source `file:lines` → target)

Target is always `dlls/TS_entities.cpp` (in the § order above) unless noted. Line numbers verified
against the working tree at commit `05be983`.

### 2.1 From `dlls/ts_objectives.cpp` (extract; the rest of the file stays)
| lines | fragment |
|---|---|
| 73–79 | CTSTrigger banner comment + `#define SF_TS_TRIGGER_FLAGCARRIER 8` (line 78) |
| 80–91 | `class CTSTrigger : public CTSObjective { ... }` declaration (incl. inline `ObjectCaps`) |
| 93 | `LINK_ENTITY_TO_CLASS(ts_trigger, CTSTrigger);` |
| 95–108 | `CTSTrigger::Spawn()` |
| 110–115 | `CTSTrigger::WaitThink()` |
| 117–125 | `CTSTrigger::KeyValue()` |
| 127–146 | `CTSTrigger::WaitTouch()` |

Do NOT touch ts_objectives.cpp's include block (lines 16–25): `ts_utils.h`/`ts_gamerules.h`/
`player.h` are all still used by the remaining classes (ObjectiveManager/Hack/WinGiver…).
`#define SF_TS_REMOVE_ON_FIRE 1` (line 27) stays — used by the remaining classes.

### 2.2 `dlls/ts_slowmotion.cpp` → whole file (212 lines)
| lines | fragment |
|---|---|
| 1–18 | provenance banner → becomes the CTSSlowMotion section comment |
| 19–25, 31 | includes → subsumed by the merged include block (drop duplicates) |
| 27–29 | `#define SF_SLOMO_START_ON/ALL_MAP/SLOW_BULLETS` |
| 33 | `LINK_ENTITY_TO_CLASS(ts_slowmotion, CTSSlowMotion);` |
| 35–39 | `TYPEDESCRIPTION CTSSlowMotion::m_SaveData[]` + `IMPLEMENT_SAVERESTORE` |
| 41–54 | `KeyValue` |
| 56–77 | `Spawn` |
| 79–96 | `ActivateSlow` |
| 98–119 | `DeactivateSlow` |
| 121–140 | `WaitTouch` |
| 142–171 | `WaitThink` |
| 173–196 | `CheckPlayersDistance` |
| 198–211 | `Use` |

### 2.3 `dlls/ts_slowmotionpoint.cpp` → whole file (106 lines)
| lines | fragment |
|---|---|
| 1–13 | banner → section comment |
| 14–19 | includes → subsumed |
| 21 | `LINK_ENTITY_TO_CLASS(ts_slowmotionpoint, CTSSlowMotionPoint);` |
| 23–30 | `Spawn` |
| 32–40 | `ActivateSlow` |
| 42–73 | `WaitThink` |
| 75–85 | `DeactivateSlow` |
| 87–99 | static `CreateSlowMotionPoint` |
| 101–106 | free fn `TSCreateSlowMotionPoint` (our helper; no callers today — keep with a `// INVENTED helper` note) |

### 2.4 `dlls/ts_dmhill.cpp` → whole file (196 lines)
| lines | fragment |
|---|---|
| 1–12 | banner → section comment |
| 13–22 | includes → subsumed (this is the only donor needing `UserMessages.h`) |
| 24 | `LINK_ENTITY_TO_CLASS(ts_dmhill, CTSDMHill);` |
| 26–31 | `m_SaveData[]` + `IMPLEMENT_SAVERESTORE` |
| 33–43 | `KeyValue` |
| 45–69 | `Spawn` |
| 71–76 | `WaitThink` |
| 78–196 | `WaitTouch` (payout logic; uses `gmsgScoreInfo`, `g_pGameRules`) |

### 2.5 `dlls/ts_model.cpp` → whole file (68 lines)
| lines | fragment |
|---|---|
| 1–14 | banner → section comment |
| 15–17 | includes → subsumed |
| 19–24 | `class CTSModel : public CBaseAnimating { ... }` (in-.cpp declaration — moves as-is) |
| 26 | `LINK_ENTITY_TO_CLASS(ts_model, CTSModel);` |
| 28–32 | `Precache` |
| 34–68 | `Spawn` |

### 2.6 `dlls/ts_mapglobals.cpp` → whole file EXCEPT InitWeaponRestriction (80 lines)
| lines | fragment |
|---|---|
| 1–12 | banner → section comment |
| 13–16 | includes → subsumed |
| 18 | `LINK_ENTITY_TO_CLASS(ts_mapglobals, CTSMapGlobals);` |
| 20–25 | `m_SaveData[]` + `IMPLEMENT_SAVERESTORE` |
| 27–32 | `KeyValue` |
| 34–36 | `Precache` |
| 38–58 | `Spawn` |
| 60–66 | **`InitWeaponRestriction()` → belongs to `TS_gamerules.cpp` (group 3).** Interim: park in TS_entities.cpp next to its sole caller with the TODO comment; group 3 relocates it. |
| 68–73 | `get_weapon_restriction` (calls InitWeaponRestriction) |
| 75–80 | `WaitThink` |

### 2.7 `dlls/ts_sellzone.cpp` → whole file (93 lines)
| lines | fragment |
|---|---|
| 1–16 | banner → section comment |
| 17–22 | includes → subsumed |
| 24 | `#define SELLZONE_VALUE 500` |
| 26–34 | `class CTSSellZone : public CBaseEntity { ... }` (in-.cpp) |
| 36 | `LINK_ENTITY_TO_CLASS(ts_sellzone, CTSSellZone)` (note: no trailing `;` — harmless) |
| 38–46 | `KeyValue` |
| 48–57 | `Spawn` |
| 59–92 | `SellTouch` |

### 2.8 `dlls/ts_playeramount.cpp` → whole file (62 lines)
| lines | fragment |
|---|---|
| 1–11 | banner → section comment |
| 12–16 | includes → subsumed (`ts_playeramount.h` stays a real header — client.cpp uses it) |
| 18 | `LINK_ENTITY_TO_CLASS(ts_playeramount, CTSPlayerAmount)` |
| 20–26 | `KeyValue` |
| 28–35 | `Spawn` |
| 37–49 | `ClientConnect` |
| 51–62 | `ClientDisconnect` |

### 2.9 `dlls/ts_timer.cpp` → whole file (106 lines)
| lines | fragment |
|---|---|
| 1–12 | banner → section comment |
| 13–16 | includes → subsumed |
| 18–19 | `#define SF_TIMER_START_OFF 1` / `SF_TIMER_LOOP 2` |
| 21–32 | `class CTSTimer : public CBaseEntity { ... }` (in-.cpp) |
| 34 | `LINK_ENTITY_TO_CLASS(ts_timer, CTSTimer);` |
| 36–44 | `KeyValue` |
| 46–62 | `Spawn` |
| 64–84 | `ToggleTimer` |
| 86–101 | `WaitThink` |
| 103–106 | `Use` |

### 2.10 `dlls/ts_customtarget.cpp` → whole file (156 lines)
| lines | fragment |
|---|---|
| 1–14 | banner → section comment |
| 15–18 | includes → subsumed |
| 20–28 | `#define TS_MAX_TARGETS/TS_TARGET_LEN/TS_FIRE_ALL/TS_FIRE_RANDOM/TS_FIRE_SEQ` |
| 30–46 | `class CTSCustomTarget : public CBaseEntity { ... }` (in-.cpp) |
| 48 | `LINK_ENTITY_TO_CLASS(ts_customtarget, CTSCustomTarget);` |
| 50–74 | `GetTargets` |
| 76–95 | `KeyValue` |
| 97–110 | `Spawn` |
| 112–124 | `FireTarget` |
| 126–156 | `Use` |

**File-scope statics: NONE in any of the 10 donors** (verified `grep -c "static " → 0` everywhere;
the only statics are class-static `m_SaveData[]` tables, which are class-qualified — no collision).
**Free functions moving: 2** (`TSCreateSlowMotionPoint`, `InitWeaponRestriction` — both unique names).
**Macro collisions: NONE** — all moved `#define`s are unique across the union (SF_SLOMO_*,
SF_TIMER_*, SF_TS_TRIGGER_FLAGCARRIER, SELLZONE_VALUE, TS_MAX_TARGETS, TS_TARGET_LEN, TS_FIRE_*).
Header-provided macros (TS_SLOMO_MAX_PLAYERS, MAX_SLOWED) stay in their headers.

## 3. `dlls/CMakeLists.txt` edits

Remove these 9 lines from the source list (current line numbers): 108 `ts_timer.cpp`,
109 `ts_customtarget.cpp`, 110 `ts_model.cpp`, 112 `ts_mapglobals.cpp`, 120 `ts_sellzone.cpp`,
122 `ts_playeramount.cpp`, 126 `ts_slowmotion.cpp`, 127 `ts_slowmotionpoint.cpp`, 137 `ts_dmhill.cpp`.
Add one line `TS_entities.cpp` (suggest: next to `ts_objectives.cpp` at line 138, keeping the TS
block together). `ts_objectives.cpp` (line 138) stays. No `cl_dll/CMakeLists.txt` changes (all-server
group). No header install/list changes (headers aren't listed in CMake).

## 4. Ordering + verification (build green after every step)

Baseline first: `scripts/build.sh amd64` + capture
`nm -D build-amd64/dlls/hl.so | grep -E " T (ts_customtarget|ts_dmhill|ts_mapglobals|ts_model|ts_playeramount|ts_sellzone|ts_slowmotion|ts_slowmotionpoint|ts_timer|ts_trigger)$" | sort`
(today: all 10 factories exported) and
`nm build-amd64/dlls/hl.so | c++filt | grep -cE "(CTSTrigger|CTSSlowMotion|CTSSlowMotionPoint|CTSDMHill|CTSModel|CTSMapGlobals|CTSSellZone|CTSPlayerAmount|CTSTimer|CTSCustomTarget)::"`
(the method count to hold constant).

- **Step 1 — the 9-file merge.** `git mv dlls/ts_slowmotion.cpp dlls/TS_entities.cpp` (keeps history
  on the largest donor), then append the other 8 donors' content in the §1 order (CTSSlowMotionPoint,
  CTSDMHill, CTSModel, CTSMapGlobals, CTSSellZone, CTSPlayerAmount, CTSTimer, CTSCustomTarget —
  CTSSlowMotion is already first-after-Trigger; leave a `// CTSTrigger lands here in step 2` marker
  at the top), hoist the include union to the top, delete per-donor duplicate includes, `git rm` the
  8 remaining donors, apply the CMakeLists edit. Build + nm: 9 factories (all but the unchanged
  ts_trigger) still `T`-exported, method count unchanged.
- **Step 2 — extract CTSTrigger.** Cut ts_objectives.cpp lines 73–146 into the top of the class
  sequence in TS_entities.cpp (after the include block, before CTSSlowMotion). Build + nm:
  `T ts_trigger` still exported; `CTSTrigger::` methods still present; ts_objectives.cpp still
  builds (its includes untouched).
- **Step 3 — audit re-check.** `python3 scripts/coverage/audit.py` → the 10 FILE-mismatch rows for
  TS_entities.cpp must be gone; MISSING/INVENTED tables unchanged. Then `scripts/coverage/coverage.py all`
  → external contract unchanged (10 entity classnames still present).
- **Step 4 (deferred / group-3 handoff)** — move `InitWeaponRestriction` into the consolidated
  `TS_gamerules.cpp` when that group runs; single-line cut, build + `nm | c++filt | grep InitWeaponRestriction`.
- Optional runtime spot-check (several of these are runtime-verified classes): xash listenserver on
  `ts_wyndham` (4 ts_slowmotion placements) — no "Can't init" spam.

Each step is a separate commit so a red build bisects to one move.

## 5. RISKS

- **CTSTrigger double-claim**: also listed under any TS_objectives.cpp group plan — coordinate;
  funcfile.tsv says TS_entities.cpp wins. Extracting it must NOT remove ts_objectives.cpp includes
  or `SF_TS_REMOVE_ON_FIRE` (still used by remaining classes there).
- **InitWeaponRestriction cross-group ownership** (original: TS_gamerules.cpp): risk of duplicate
  definition (link error) or lost function if both groups move it. This plan parks it in
  TS_entities.cpp with a TODO; group 3 owns the final move.
- **client.cpp → ts_playeramount.h**: do not delete the five kept headers; `dlls/client.cpp:43`
  breaks otherwise.
- **Include-order deps**: `ts_objectives.h` (CTSTrigger base) needs `cbase.h`/`player.h` first;
  `ts_slowmotion.h`/`ts_dmhill.h` use `EHANDLE`/`TYPEDESCRIPTION` (cbase.h); `ts_playeramount.h`
  needs `CBaseDelay` (cbase.h). The §1 include order satisfies all; keep `extdll/util/cbase` first.
- **Statics/macros**: verified zero file-scope statics and zero macro collisions (§2). The residual
  risk is future donors adding one — re-grep before executing.
- **gmsgScoreInfo globals-audit noise**: inventory attributes it to TS_entities.cpp; ours lives in
  UserMessages.h. Accepted divergence — do not relocate; may need an accepted-mapping note in the
  globals audit.
- **Generated docs**: `reference/coverage/{AUDIT,LAYOUT_MAP,INVENTORY,SDK_DIVERGENCE}.md` reference
  the old paths — regenerate (audit.py/coverage.py), don't hand-edit. `reference/RECONSTRUCTION.md`
  references the old paths in prose (lines 89–129) — hand-update or leave as historical provenance.
  No scripts/CMake/tests reference the donor filenames besides `dlls/CMakeLists.txt`.
- **Serena/ledger drift**: memories reference per-class files; append a ledger entry after the move.
- **git blame/history**: a 9-into-1 merge loses per-file blame except for the `git mv`'d donor;
  acceptable (per-class provenance banners are preserved in-file).
