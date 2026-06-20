# mod-ts change ledger

Running log of reconstruction progress. Newest entries on top. Keep factual:
what changed, how verified, what's next. See `CLAUDE.md` for mission/method.

---

## 2026-06-20 (cont. 2) — GNU-v2 demangler + prototype-annotated decompiles

**Done**
- `scripts/gnv2demangle.py` — our own GNU g++ 2.x ("gnu-v2") demangler (this
  host's c++filt dropped that dialect). Needed because `mp.dll`'s 159 MSVC
  exports are mostly STOCK classes — the big TS targets (CTSGameRules, CTSGun,
  CTSStunt, CTSSlowMotion) are NOT exported, so MSVC prototypes are thin for
  exactly the code we need. The GCC2 names encode full param types for ALL 2426
  funcs. Handles member/global/ctor/dtor, P/R/C/U/G prefixes, N repeat-args,
  T (0-based) backrefs.
- Coverage: **1788/2426 (74%)** typed signatures → `reference/signatures-gcc2.map`
  (mangled => sig) + `reference/signatures-gcc2.txt`. Remaining 638 = ~444 plain
  C/CRT (legit) + ~194 array/edge-case mangling. ~60 residual `T?` backrefs.
- `DecompExport.java` enhanced: loads the sig map, annotates each decompiled
  function with its recovered C++ prototype (`// CTSGun::AkimboFire()`).
  `decompile.sh` passes the map automatically.

**Verified**
- Demangler correct on known cases: `UTIL_SetSize(entvars_s*, const Vector&,
  const Vector&)`, `CBasePlayer::ShootBulletsPlayer(unsigned char, Vector,
  Vector, Vector, float, int, int, int)`, `CTSGun::DropWeapon(int, bool)`.
- Re-ran CTSSlowMotion decompile: 28 funcs, prototypes annotated (12/14 in
  CTSSlowMotion.c).

**Next**
- (Optional) apply prototypes as actual Ghidra function signatures (param
  typing) — heavier; the comment annotation already gives the prototype.
- Start real reconstruction: CTSGameRules / CTSGun, writing SDK-style C++.

---

## 2026-06-20 (cont.) — Authorization + Ghidra decompile pipeline working

**Authorization (provenance):** original author **Vino ("Vinoisnotozou")** gave
the project owner the go-ahead; original source is lost/irrecoverable — binary
reconstruction is the only path. Recorded in `reference/README.md`.

**Done**
- Ghidra headless decompile pipeline (Java post-script — Ghidra 12.1 dropped
  Jython, ships PyGhidra; Java is zero-setup):
  - `scripts/ghidra/DecompExport.java` — decompiles, groups C by owning class.
  - `scripts/decompile.sh <binary> <outDir> [classFilter]` — analyzeHeadless
    wrapper (temp project, import+auto-analyze applies symtab, runs post-script).
- Smoke test on CTSSlowMotion: 38s analysis, 28 funcs decompiled, 0 failures;
  output named & readable (Spawn shows pev->classname=MAKE_STRING("ts_slowmotion"),
  precache, UTIL_SetSize, recoverable member offsets).

---

## 2026-06-20 — Repo scaffolding, reference capture, tooling, work-list

**Done**
- Created private `goldsrc-net/mod-ts`, fork of `halflife-updated@64bit-demo`.
  origin=mod-ts, upstream=halflife-updated, branch main, at `~/Projects/goldsrc.net/mod-ts`.
- Standalone mod layout: `game/ts/liblist.gam` (`-game ts`, fallback valve).
- `reference/`: ts_i386.so (GCC2, not stripped, 2426 funcs), mp.dll (MSVC server,
  159 exports), client.dll (MSVC client, ~1013), ts_fgd.fgd; `dump-signatures.sh`.
- Build: clean separation — mod-ts is NOT a buildchain submodule; builds via the
  `hlds64-buildchain:debian10` image (`scripts/build.sh <arch>`). (Earlier wired
  as submodule + Makefile, then REVERTED — buildchain stays provenance-only.)
- Tooling: Ghidra 12.1.2 (`~/Tools/ghidra`, PATH Ghidra/ghidra/analyzeHeadless,
  JDK 21); rizin rz-* static (`~/Tools/rizin`, no main REPL in pkg).
- Work-list `reference/RECONSTRUCTION.md`: 437 stock SDK classes, 216 in binary,
  **60 TS-specific to RE** (~386 funcs), 23 FGD-backed. CTSGun(44), CTSGameRules(41).

**Convention:** NO `Co-Authored-By` trailer (user pref, all goldsrc-net repos).
CLAUDE.md is local-only (global gitignore). In auto mode: halt & ask on a block.
