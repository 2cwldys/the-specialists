# mod-ts change ledger

Running log of reconstruction progress. Newest entries on top. Keep factual:
what changed, how verified, what's next. See `CLAUDE.md` for mission/method.

---

## 2026-06-20 (cont.) — Authorization + Ghidra decompile pipeline working

**Authorization (provenance):** the original author **Vino ("Vinoisnotozou")**
gave the project owner the go-ahead to reconstruct the mod. Original source is
lost / irrecoverable — binary reconstruction is the only path. Recorded in
`reference/README.md`.

**Done**
- Ghidra headless decompile pipeline (written in Java — Ghidra 12.1 dropped
  Jython, ships PyGhidra; Java post-scripts are zero-setup):
  - `scripts/ghidra/DecompExport.java` — decompiles functions, groups output by
    owning class (parsed from GCC2 `__<len><Class>` names), writes `<Class>.c`.
  - `scripts/decompile.sh <binary> <outDir> [classFilter]` — wraps
    `analyzeHeadless`: temp project, import + auto-analyze (applies symtab),
    runs the post-script.
- Smoke test: `scripts/decompile.sh reference/ts_i386.so /tmp/recovered CTSSlowMotion`
  → 38s analysis, **28 functions decompiled, 0 failures**. Output named &
  readable: `Spawn__13CTSSlowMotion` shows `pev->classname=MAKE_STRING("ts_slowmotion")`
  (confirms FGD↔class mapping), precache call, `UTIL_SetSize(pev,mins,maxs)`,
  recoverable member offsets.

**Verified**
- Decompiled C is symbol-named (symtab applied) and shows recognizable HL SDK
  idioms + entity member offsets → good reconstruction substrate.

**Not yet done / next**
- Apply MSVC prototypes (from signatures-mp/client.txt) into Ghidra as function
  signatures to fix `this`/param/return types + `__thiscall` (refinement on top
  of the working pipeline).
- Recover struct/member layout (offsets like pev+0x208) into named fields.
- Start real RE: CTSGameRules, then CTSGun/weapon hierarchy.
- `recovered/` output is gitignored for now (WIP); commit reconstructed source
  as classes become real.

---

## 2026-06-20 — Repo scaffolding, reference capture, tooling, work-list

**Done**
- Created private `goldsrc-net/mod-ts`, forked from `goldsrc-net/halflife-updated@64bit-demo`.
  Remotes: `origin`=mod-ts, `upstream`=halflife-updated. Work on `main`.
  Lives at `~/Projects/goldsrc.net/mod-ts` (sibling of the other repos).
- Standalone mod layout: `game/ts/liblist.gam` (`-game ts`, `fallback_dir valve`).
- `reference/` = source-of-truth originals + extraction tooling:
  - `ts_i386.so` (GCC2 server, not stripped, 2426 funcs), `mp.dll` (MSVC server,
    159 typed exports), `client.dll` (MSVC client, ~1013 exports), `ts_fgd.fgd`.
  - `dump-signatures.sh` → signatures-mp/client.txt, symbols-gcc2.txt, classes.txt.
    Demangle via `llvm-undname-17`.
- Build: clean separation from the buildchain. mod-ts is NOT a submodule; it
  consumes the `hlds64-buildchain:debian10` image via `scripts/build.sh <arch>`.
  (Earlier wired as a buildchain submodule + Makefile targets, then REVERTED —
  buildchain stays provenance-only for the public server stack.)
- Tooling: Ghidra 12.1.2 (`~/Tools/ghidra`, PATH: `Ghidra`/`ghidra`/`analyzeHeadless`,
  JDK 21); rizin `rz-*` static tools (`~/Tools/rizin`, PATH; no main REPL in pkg).
- Work-list → `reference/RECONSTRUCTION.md`: 437 stock SDK classes, 216 in binary,
  **60 TS-specific to RE** (~386 funcs), 23 FGD-backed. Biggest: CTSGun(44),
  CTSGameRules(41), CTSStunt(25).

**Verified**
- Repo private. Buildchain fully reverted (0 mod-ts refs). Tooling resolves on PATH.

**Convention:** commits carry NO `Co-Authored-By` trailer (user preference, all
goldsrc-net repos). CLAUDE.md is local-only (global gitignore).
