# mod-ts change ledger

Running log of reconstruction progress. Newest entries on top. Keep factual:
what changed, how verified, what's next. See `CLAUDE.md` for mission/method.

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
  - `dump-signatures.sh` → `signatures-mp.txt`, `signatures-client.txt`,
    `symbols-gcc2.txt`, `classes.txt`. Demangle via `llvm-undname-17`.
- Build: clean separation from the buildchain. mod-ts is NOT a submodule; it
  consumes the `hlds64-buildchain:debian10` image via `scripts/build.sh <arch>`.
  (Earlier I wired it as a buildchain submodule + Makefile targets, then
  REVERTED that per decision — buildchain stays provenance-only for the public
  server stack.)
- Tooling installed: Ghidra 12.1.2 (`~/Tools/ghidra`, on PATH as `Ghidra`/
  `ghidra`/`analyzeHeadless`, uses JDK 21); rizin `rz-*` static tools
  (`~/Tools/rizin`, on PATH; no main REPL in the static pkg).
- Reconstruction work-list computed → `reference/RECONSTRUCTION.md`:
  437 stock SDK classes, 216 in binary, **60 TS-specific to RE** (~386 funcs),
  23 of them FGD-entity-backed. Biggest: CTSGun(44), CTSGameRules(41), CTSStunt(25).

**Verified**
- Repo is private (`gh repo view`). Buildchain fully reverted (0 mod-ts refs in
  Makefile/.gitmodules). Tooling resolves on PATH; `rz-bin -v`, `analyzeHeadless`
  usage, `llvm-undname-17` MSVC demangle all confirmed working.

**Not yet done / next**
- Set up a Ghidra headless post-script to bulk-decompile `ts_i386.so` and export
  C per function, applying the MSVC prototypes.
- Begin RE of the core classes (CTSGameRules, CTSGun/weapon hierarchy).
- `scripts/build.sh` not yet run end-to-end (needs the image built locally first).
