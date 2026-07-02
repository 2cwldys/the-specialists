# Group 6 move plan — client.dll HUD files + pm_shared.c / TS_pm_shared.c

Scope: `CHudTSBuyMenu`, `CHudTSWeaponBar` (FILE-mismatch rows whose "original file" is
`client.dll`), and the two MISSING-file entries `client.dll` + `pm_shared.c`, whose real
content is the original **`TS_pm_shared.c`** (funcfile.tsv lines 541–553) with 3 free
functions we haven't built: `CanJumpHigh`, `FixVelocity`, `PseudoClimb`.

This group is TWO very different items:

- **6a (client HUD): NO file moves.** The mismatch is *unresolvable by renaming* —
  `client.dll` has no symbols, no RTTI beyond `.?AVtype_info@@`, and
  `strings reference/client.dll` yields **zero** `.cpp`/`.pdb` path strings (verified),
  so the original client source file names are unknowable. Fix belongs in `audit.py`,
  not the tree.
- **6b (pm_shared): create `pm_shared/TS_pm_shared.cpp`** by carving the existing TS
  stunt block out of `pm_shared/pm_shared.cpp`, plus a scaffold for the 3 missing
  free functions (blocked-on-RE).

Ground-truth notes discovered while planning (both matter to the audit):

1. **The `pm_shared.c` MISSING-file entry is spurious.** In `ts_inventory.json`,
   `files["pm_shared.c"]` contains exactly one "class": `__user_type_info` — the
   **gcc-2.x RTTI runtime** (methods `contained_p`, `contained_public_p`, … at
   0xf4770–0xf47d0), misattributed to pm_shared.c by filemap. It is compiler runtime,
   not TS code, and must never be "built". `audit.py` computes `missing_files` purely
   from classes, so this junk class alone makes pm_shared.c "missing".
2. **The `client.dll` MISSING-file entry is also an audit artifact**: `audit.py:63`
   synthesizes `file="client.dll"` for the HUD classes, but `missing_files`
   (`audit.py:251`) checks `orig_file_classes` built from the json `files` tree, which
   has no `client.dll` entry — so it always reports missing even though both classes
   exist and build.
3. `audit.py:36 norm_file()` drops directory + extension and lstrips `ts_`, so
   `TS_pm_shared.c` ≡ `pm_shared.cpp` ≡ `TS_pm_shared.cpp` ("pm_shared"). The new file
   can therefore be `.cpp` (our SDK is C++; stock `pm_shared.c` was already renamed
   `.cpp` by halflife-updated) without breaking name matching.
4. funcfile.tsv puts **13** functions in the original `TS_pm_shared.c`:
   `CanJumpHigh, FixVelocity, PseudoClimb, TSStandUp, TSGoDuck, TSWallJump,
   TSWallJumpClimb, TSDive, TSHandle, PM_Jump, PM_CheckParamters, PM_Duck,
   PM_WalkMove`. I.e. the original also moved its *modified copies* of 4 stock PM_
   functions into that file. Plan phases this: TS* now, the 4 PM_ moves optional later.

---

## 1. Target layout

| our file (now) | target | action |
|---|---|---|
| `cl_dll/ts_buymenu.cpp` + `ts_buymenu.h` | **unchanged** | original client file names unknowable — audit accepted-mapping instead |
| `cl_dll/ts_weaponbar.cpp` + `ts_weaponbar.h` | **unchanged** | same |
| `pm_shared/pm_shared.cpp` (TS block + TSPM_ macros) | **`pm_shared/TS_pm_shared.cpp`** (new) + **`pm_shared/TS_pm_shared.h`** (new) | carve out; see §2 |
| — | `pm_shared/TS_pm_shared.cpp` scaffold section | `CanJumpHigh`/`FixVelocity`/`PseudoClimb` — **blocked-on-RE**, scaffold only |

Headers: the original (a `.c`-era mod) almost certainly had no `TS_pm_shared.h`;
`ts_inventory.json`/funcfile.tsv give **no** header hints for either sub-item (headers
aren't in the binary). We keep our per-class headers (`ts_buymenu.h`, `ts_weaponbar.h`,
included via `cl_dll/hud.h:169/171`) and add ONE new header `TS_pm_shared.h` because the
TSPM_* flag macros are needed by BOTH TUs (PM_Duck:2098 and PM_Jump:2600 gates stay in
pm_shared.cpp in phase 1). audit.py does not track headers — note-and-keep.

New `pm_shared/TS_pm_shared.h` contents:
- the TSPM_* macro block (moved from pm_shared.cpp:86–101)
- `void TSHandle();` (called from `PM_PlayerMove` at pm_shared.cpp:3278)
- (phase 2 only) prototypes for `PM_Jump/PM_CheckParamters/PM_Duck/PM_WalkMove`

## 2. Fragment-by-fragment moves (exact, current line numbers)

### Phase 1 — TS block carve-out (do now)

All from `pm_shared/pm_shared.cpp` unless noted. Move = cut from source, paste into
target in the SAME order.

| # | fragment | source lines | target |
|---|---|---|---|
| 1 | TSPM_* flag macros + 2-line comment | pm_shared.cpp 86–101 | `TS_pm_shared.h` |
| 2 | banner comment "===== TS STUNT PREDICTION HOOKS" | pm_shared.cpp 3063–3070 | `TS_pm_shared.cpp` (top) |
| 3 | `static void TSGoDuck()` | pm_shared.cpp 3071–3087 | `TS_pm_shared.cpp` |
| 4 | `static void TSStandUp()` | pm_shared.cpp 3089–3121 | `TS_pm_shared.cpp` |
| 5 | `static void TSDive()` | pm_shared.cpp 3123–3144 | `TS_pm_shared.cpp` |
| 6 | `static void TSWallJump()` | pm_shared.cpp 3146–3158 | `TS_pm_shared.cpp` |
| 7 | `static void TSWallJumpClimb()` | pm_shared.cpp 3160–3171 | `TS_pm_shared.cpp` |
| 8 | `void TSHandle()` | pm_shared.cpp 3172–3195 | `TS_pm_shared.cpp` |
| 9 | scaffold: `CanJumpHigh` / `FixVelocity` / `PseudoClimb` | — (new) | `TS_pm_shared.cpp`, TODO section |

Linkage fidelity note for #3–#7: in `ts_i386.so` ALL of these are **`T` (external)**
symbols (e.g. `CanJumpHigh` T@0xe97cc, `FixVelocity` T@0xe8fc4, `PseudoClimb`
T@0xe8fc8, `TSStandUp` T@0xe91fc, `TSHandle` T@0xe96a0). Recommend dropping `static`
on the moved TS* helpers to match the original's external linkage (harmless — names
are unique across the tree; verified no other definition via grep).

Compile support needed at the top of `TS_pm_shared.cpp` (all deps are already
non-static in pm_shared.cpp, nothing needs de-static-ing):
- includes: same prefix as pm_shared.cpp:16–31 (`Platform.h`, `mathlib.h`,
  `cdll_dll.h` — provides `VEC_VIEW` via dlls/cdll_dll.h — `const.h`, `usercmd.h`,
  `pm_defs.h`, `pm_shared.h`, `pm_movevars.h`) + `"TS_pm_shared.h"` + `<math.h>` (sqrt)
- `pmove` — already `extern playermove_s* pmove;` in pm_shared.h:44 ✔
- extern prototypes for pm_shared.cpp non-statics the TS funcs call (add to
  `TS_pm_shared.h` or pm_shared.h): `void PM_FixPlayerCrouchStuck(int);`
  `void PM_CatagorizePosition();` `void PM_FixupGravityVelocity();`
- `STUCK_MOVEUP` (pm_shared.cpp:82) — used by TSGoDuck/TSStandUp AND by remaining
  pm_shared.cpp code → **copy the `#define` into TS_pm_shared.h and keep it in
  pm_shared.cpp is a redefinition risk**; instead MOVE `STUCK_MOVEUP`/`STUCK_MOVEDOWN`
  (lines 82–83) into `TS_pm_shared.h` and have pm_shared.cpp include it (it must
  include it anyway for TSPM_STUNT_MASK at 2098/2600 and `TSHandle()` at 3278).

In `pm_shared/pm_shared.cpp` after the carve-out:
- add `#include "TS_pm_shared.h"` after line 27 (`pm_debug.h`)
- lines 86–101, 3063–3195, 82–83 removed; call site 3278 (`TSHandle();`) and gates
  2098/2600 unchanged (macros now come from the header).

#### #9 scaffold (blocked-on-RE — do NOT fake implementations)

Plan the section as a comment block + declarations only; implementing empty bodies
would fraudulently close the audit gap. RE pointers for the follow-up:
- `FixVelocity` @0xe8fc4 is **4 bytes** (next symbol PseudoClimb @0xe8fc8) — an
  empty `ret` stub in the original; trivially implementable as an empty function
  once eyeballed (`objdump -d --start-address=0xe8fc4`).
- `PseudoClimb` @0xe8fc8–0xe91fc (~0x234 bytes) — real function; reads
  `pmove` fields (+0x5c/0x60/0x64) into globals. RE via `re.sh 0xf8fc8`-style
  addressing (`nm addr + 0x10000` → `re.sh 0xf8fc8`).
- `CanJumpHigh` @0xe97cc — reads `pmove->…+0xbc`; likely the CTSSuperJump
  (jump-high powerup) predicate used by the modified PM_Jump. RE: `re.sh 0xf97cc`,
  callers via objdump (static free func → direct calls, `re.sh refs` works).

### Phase 2 (optional, later — full funcfile.tsv parity for TS_pm_shared.c)

The original also compiled its modified `PM_Jump`, `PM_CheckParamters`, `PM_Duck`,
`PM_WalkMove` in TS_pm_shared.c. Moving ours too is feasible (none of the four touches
a pm_shared.cpp `static` — the only file statics are `pm_shared_initialized`,
`rgv3tStuckTable`, `rgStuckLast`, `gcTextures`, `grgszTextureName`, `grgchTextureType`,
`PM_InitTrace`, `PM_TraceModel`, none referenced by these four) but needs ~14 extern
prototypes. Defer; audit.py doesn't currently check OUR free-function file placement.

| fragment | source lines (incl. comment banner) | needs externs for |
|---|---|---|
| `PM_WalkMove` | pm_shared.cpp 1112–1284 | PM_Accelerate, PM_Friction*, PM_FlyMove (+ `pmove->PM_PlayerTrace` is a fn ptr, fine) |
| `PM_Duck` | 2071–2174 | PM_UnDuck, PM_FixPlayerCrouchStuck, PM_CatagorizePosition, PM_SplineFraction |
| `PM_Jump` | 2582–2739 | PM_PreventMegaBunnyJumping, PM_PlayStepSound, PM_MapTextureTypeStepType, PM_FixupGravityVelocity, PM_CheckWaterJump, `g_onladder` (non-static @149) |
| `PM_CheckParamters` | 2953–3034 | PM_DropPunchAngle, PM_CalcRoll, `iJumpSpectator` (non-static @35) |

(*re-verify exact call sets when executing; ranges shift after phase 1.)
PM_PlayerMove/PM_Move (staying in pm_shared.cpp) then need these four prototyped in
`TS_pm_shared.h`.

### 6a — client HUD: audit change, not a move

Recommended `audit.py` edits (one small patch, fixes BOTH bogus rows AND the two
bogus MISSING files):
1. In `load_original()` (~line 44–63): **skip classes whose name starts with `__`**
   (`__user_type_info` = gcc2 RTTI runtime) → removes `pm_shared.c` from
   `missing_files` and the phantom class.
2. Treat `ofile == "client.dll"` as *unverifiable placement*: set `file_ok = True`
   (or a third state `"n/a"`) at audit.py:234, and exclude `"client.dll"` from
   `missing_files` at :251–252. Optionally add an explicit accepted-mapping table so
   the mapping is documented and checkable:
   `ACCEPTED_CLIENT_FILES = {"CHudTSBuyMenu": "cl_dll/ts_buymenu.cpp", "CHudTSWeaponBar": "cl_dll/ts_weaponbar.cpp"}`
   — flag a mismatch only if the class moves away from its accepted file.
3. Print such classes in a new "ACCEPTED mappings (original file unknowable)" section
   so they stay visible without counting as mismatches.

## 3. CMakeLists.txt edits

- `dlls/CMakeLists.txt`: add `../pm_shared/TS_pm_shared.cpp` after line 164
  (`../pm_shared/pm_shared.cpp`); add `../pm_shared/TS_pm_shared.h` after line 254
  (`../pm_shared/pm_shared.h`).
- `cl_dll/CMakeLists.txt`: add `../pm_shared/TS_pm_shared.cpp` after line 189; add
  `../pm_shared/TS_pm_shared.h` after line 287.
  (BOTH libs compile pm_shared — the stunt hooks are the shared prediction path;
  forgetting the client side silently breaks stunt prediction: see
  `predicted-weapon-gotchas` memory.)
- No edits for 6a (ts_buymenu.cpp:110 / ts_weaponbar.cpp:119 already listed).

## 4. Ordering + verification (build stays green each step)

1. **Step A** — create `TS_pm_shared.h` (macros #1 + STUCK_MOVEUP/DOWN + `TSHandle()`
   + the 3 PM_ extern prototypes); create `TS_pm_shared.cpp` with fragments #2–#8;
   delete moved lines from pm_shared.cpp; add `#include "TS_pm_shared.h"`; both
   CMakeLists edits. This must be ONE commit-sized step (the carve-out is atomic —
   the file won't compile half-moved).
   Verify: `scripts/build.sh amd64 all` (server + client) then
   - `nm build-amd64/dlls/hl.so | grep -E " [Tt] (TSHandle|TSGoDuck|TSStandUp|TSDive|TSWallJump|TSWallJumpClimb)$"` → all 6 present
   - same grep on the built client (`build-amd64/cl_dll/client.so` or the i386 pair)
   - factory regression guard (none expected to change — this group has NO
     LINK_ENTITY_TO_CLASS): `nm -D build-amd64/dlls/hl.so | awk '$2=="T"' | wc -l`
     identical before/after.
   - `python3 scripts/coverage/audit.py` → FILE sections unchanged (placement of free
     funcs isn't checked); the 3 free funcs still MISSING (expected until RE).
   Runtime: xash listenserver, dive + prone + wall-jump still predict (stunts work
   with no rubber-banding) — the moved code is the shared prediction path.
2. **Step B** — audit.py patch for 6a (+ `__user_type_info` skip). Verify:
   `audit.py` output drops both MISSING files and the 2 client.dll FILE-mismatch
   rows; totals go 22→20 file mismatches; `audit.py --strict` unaffected.
3. **Step C (blocked-on-RE)** — RE + implement `FixVelocity` (trivial), `PseudoClimb`,
   `CanJumpHigh` in `TS_pm_shared.cpp`; re-run `audit.py` → "MISSING free functions /
   TS_pm_shared.c" row disappears. Runtime verify against original behaviour
   (live.py on the running original if semantics unclear).
4. **Step D (optional phase 2)** — move the 4 PM_ functions; re-verify Step A checks +
   `cmp`-level behaviour testing (walk/jump/duck are the hottest movement paths).

## 5. RISKS

- **Prediction split-brain**: TS_pm_shared.cpp must be added to BOTH dlls and cl_dll
  CMake lists in the same step, or server and client predict different physics
  (invisible at build time, shows as movement warping online).
- **Macro duplication**: TSPM_* and STUCK_MOVEUP must be MOVED to the header, not
  copied — a stale copy in pm_shared.cpp later drifting is a classic silent bug.
  `dlls/ts_stunt.h` holds a PARALLEL set (TSSTUNT_*, same values) — unchanged here,
  but any future value edit must touch both (pre-existing risk, now three files).
- **Statics collision**: none — the moved TS* names are unique repo-wide (verified by
  grep); pm_shared.cpp's remaining statics stay put. If TS* are de-static'd to match
  the original's `T` linkage, still no collision (no other definitions).
- **Include-order**: TS_pm_shared.h needs `pm_defs.h`/`pm_shared.h` before it for
  `playermove_s`; either #include them in it (guarded) or document the order. VEC_VIEW
  comes from `dlls/cdll_dll.h` (both TUs already include it — keep that include).
- **Scaffold temptation**: do NOT commit empty CanJumpHigh/PseudoClimb bodies to make
  the audit green — `FixVelocity` alone is verifiably empty in the binary; the other
  two are real logic (blocked-on-RE).
- **Name-based references**: `scripts/coverage/srcparse.py:17` already scans
  `pm_shared/` (picks the new file up automatically); no script/doc references
  `TS_pm_shared` yet; HANDOFF/STUNT.md mention "pm_shared.cpp" prose lines that become
  slightly stale (cosmetic). `cl_dll/hud.h:169/171` include the HUD headers — untouched.
- **audit.py `lstrip("ts_")` quirk**: it strips a CHARACTER SET, so e.g. a future file
  named `TS_shared.cpp` would normalize oddly ("hared"); irrelevant for `pm_shared`
  (starts with 'p'... note `pm_shared`.lstrip("ts_") → "pm_shared", safe), but keep in
  mind when naming future TS_*.cpp files starting with t/s/_.
- **Phase 2 line-range drift**: all phase-2 line numbers shift after phase 1 —
  re-derive before executing.
