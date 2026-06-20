# mod-ts change ledger

Running log of reconstruction progress. Newest entries on top. Keep factual:
what changed, how verified, what's next. See `CLAUDE.md` for mission/method.

---

## 2026-06-20 (cont. 4) — Target = functional; FIRST BUILD-VERIFIED class

**Decision (load-bearing):** goal is **functional / source-faithful**, NOT byte
drop-in. Original is GCC 2.x old-ABI (vptr@0x60, gnu-v2 mangling, cdecl-this,
int-return virtuals); modern toolchain = Itanium ABI + modern halflife-updated
changed virtual sigs (KeyValue/Save return bool). Engine loads the dll via a C
interface (GetEntityAPI2/LINK_ENTITY_TO_CLASS), not C++ layout, so a clean
modern rebuild that behaves identically is the bar. Recovered offsets = provenance
notes, NOT targets. CLAUDE.md mission updated accordingly.

**Done**
- Confirmed build pipeline: `scripts/build.sh amd64` builds base SDK hl.so
  (2.5MB, glibc floor 2.14) via the hlds64-buildchain:debian10 image.
- Reconstructed + **built into hl.so**:
  - `dlls/ts_utils.{h,cpp}` — `TSHudMessageAll()` (gmsgTSMessage user msg,
    MSG_ALL, rgb+effect+holdtime+string). Shared by many TS classes.
  - `dlls/ts_timer.cpp` — `CTSTimer` (ts_timer): relay timer, fires
    pev->target + HUD msg every "duration" sec; Use toggles. Modern SDK style
    (bool KeyValue), faithful to decompiled control flow.
  - Added both to `dlls/CMakeLists.txt`.

**Verified (build + symbols)**
- hl.so links clean (warnings only). `nm -D`: **`T ts_timer`** entity factory
  exported (engine-spawnable). CTSTimer methods + TSHudMessageAll present
  (Itanium-mangled). glibc 2.14.
- Still TODO: runtime/behaviour test in the engine (spawn ts_timer, confirm it
  fires). VERIFY tags: ts_timer spawnflag "Loop" vs the &1 check; effect/holdtime
  semantics of TSHudMessageAll.

**Next**
- Knock out more self-contained entity classes (CTSCustomTarget needs
  TSHudMessageAll — now available; CTSModel, CTSFog trivial).
- `CTSSlowMotion` (dlls/ts_slowmotion.cpp) stays a NON-built draft until its deps
  (CTSGameRules::GlobalSlowMotion2, TS player slow-motion virtuals) exist; also
  needs reconcile to modern bool-return sigs.

---

## 2026-06-20 (cont. 3) — First reconstructed class: CTSSlowMotion (draft, not built)
- `dlls/ts_slowmotion.cpp`: CTSSlowMotion : CBaseEntity (ts_slowmotion brush
  trigger). Members/methods recovered. NOT in build (deps: CTSGameRules, TS
  player virtuals). VERIFY tags in-file.

## 2026-06-20 (cont. 2) — GNU-v2 demangler + prototype-annotated decompiles
- `scripts/gnv2demangle.py`: 1788/2426 (74%) typed sigs ->
  reference/signatures-gcc2.{map,txt}, incl. TS classes mp.dll never exported.
- DecompExport.java annotates each function with its recovered prototype.

## 2026-06-20 (cont.) — Authorization + Ghidra pipeline
- Author **Vino** OK'd reconstruction; source lost (reference/README.md).
- `scripts/ghidra/DecompExport.java` + `scripts/decompile.sh`. Java post-script
  (Ghidra 12.1 dropped Jython, ships PyGhidra).

## 2026-06-20 — Scaffolding, reference, tooling, work-list
- Private `goldsrc-net/mod-ts`, fork of halflife-updated@64bit-demo, at
  `~/Projects/goldsrc.net/mod-ts`. Standalone `ts/` game dir.
- reference/: ts_i386.so, mp.dll, client.dll, ts_fgd.fgd + dump-signatures.sh.
- Clean separation: builds via hlds64-buildchain image (NOT a submodule).
- Tooling: Ghidra 12.1.2 + rizin rz-* on PATH.
- Work-list reference/RECONSTRUCTION.md: 60 TS-specific classes.

**Conventions:** NO Co-Authored-By trailer. CLAUDE.md local-only. Auto mode:
halt & ask on a block.
