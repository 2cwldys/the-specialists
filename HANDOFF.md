# HANDOFF — for the next Claude picking up mod-ts

You're resuming the reconstruction of **The Specialists ("ts")** as a functional,
source-faithful HL mod. This is the continuation brief — not a tutorial.

## Read these first (in order)
1. **`CLAUDE.md`** (repo root, auto-loaded on this machine) — mission, method,
   tooling, conventions. The stable reference.
2. **Serena ledger** — `read_memory("ledger")` (or `.serena/memories/ledger.md`).
   The full chronological history of what was done and why. Newest on top.
3. **`reference/RECONSTRUCTION.md`** — the 60-class work-list + live status table.

If those three and this file agree, you're oriented. If they disagree, the
ledger + git log are ground truth; fix the docs.

## Where things stand (2026-06-20)
- Pipeline is fully working: decompile → recover typed prototypes → write modern
  C++ → **build into `hl.so`** → verify the entity factory is exported.
- **Built + verified:** `CTSTimer` (`dlls/ts_timer.cpp`) and the shared helper
  `TSHudMessageAll` (`dlls/ts_utils.{h,cpp}`). `nm -D build-amd64/dlls/hl.so`
  shows `T ts_timer`. Runtime/behaviour in-engine is **not** yet tested.
- **Draft, not in build:** `CTSSlowMotion` (`dlls/ts_slowmotion.cpp`) — blocked on
  `CTSGameRules::GlobalSlowMotion2` + the TS player slow-motion virtuals, and
  still uses int-return virtual sigs (reconcile to modern `bool` when you wire it).
- Decision locked: **functional fidelity, NOT byte/ABI match** (CLAUDE.md mission
  explains why — gcc2 old-ABI original vs Itanium modern build).

## The loop (commands that work on this box)
```bash
cd ~/Projects/goldsrc.net/mod-ts
# decompile one class (recovered/ is gitignored; output annotated w/ prototypes)
scripts/decompile.sh reference/ts_i386.so recovered/server CTSCustomTarget
# exact prototypes for any function:
grep 'CTSCustomTarget::' reference/signatures-gcc2.txt
# entity keyvalues/spawnflags:
grep -A12 '= ts_customtarget ' reference/ts_fgd.fgd
# write dlls/<file>.cpp, add it to dlls/CMakeLists.txt, then build + verify:
scripts/build.sh amd64
nm -D build-amd64/dlls/hl.so | grep ' ts_customtarget$'      # factory exported
nm build-amd64/dlls/hl.so | c++filt | grep 'CTSCustomTarget::'
```
Tools are on PATH: `Ghidra`/`analyzeHeadless` (12.1.2, `~/Tools/ghidra`), `rz-*`
(`~/Tools/rizin`), `scripts/gnv2demangle.py`. The buildchain image
`hlds64-buildchain:debian10` is already built.

## Recommended next steps
1. **`CTSCustomTarget`** — mostly self-contained (uses `FireTargets` + the now-built
   `TSHudMessageAll`); already decompiled in `recovered/server/`. Good next class.
2. Trivial ones to clear quickly: `CTSModel`, `CTSFog` (self-removing stub).
3. Then the foundation: **`CTSGameRules`** (41 funcs) + the TS player extensions
   (slow-motion virtuals). These unblock `CTSSlowMotion`'s build and most weapons.
4. Once a handful of entities are in, do the first **runtime test**: assemble the
   `ts/` game dir, run the server, spawn an entity, confirm behaviour.

## Live gotchas (don't re-learn these)
- **gcc2 vs modern ABI**: original has vptr@0x60, gnu-v2 mangling, cdecl-this,
  int-return virtuals. Don't chase those — write natural modern C++. Recovered
  offsets in decompiles are provenance comments only.
- **Decompiler artifacts**: GCC2 pointer-to-member-functions are 8 bytes — you'll
  see paired writes like `this+0x14 = 0xffff0000; this+0x18 = <func>` for one
  `SetTouch`/`SetThink`. Heavy stack-pointer arithmetic (e.g. distance loops) is
  often best reconstructed from *intent*, not transcribed.
- **`dlls/CMakeLists.txt` is an explicit source list** (not glob) — a new file
  won't compile until you add it, but also can't break the build by just existing.
- **`gnv2demangle.py`**: ~74% coverage; misses some array/edge-case mangling and
  has ~60 approximate `T?` backrefs. Cross-check against the decompiled body.
- **Conventions**: NO `Co-Authored-By` trailers. In auto mode, **halt and ask** on
  any blocked/permission situation — don't work around it (e.g. `CLAUDE.md` is in
  the user's global gitignore — leave it local-only, never force-add).
- **Authorization**: original author **Vino ("Vinoisnotozou")** OK'd this; source
  is lost. (`reference/README.md`)

## Keep the ledger going
After each meaningful unit (class built, behaviour verified, decision made),
append a Serena ledger entry (`write_memory("ledger", …)`, newest on top) and
update the status table in `reference/RECONSTRUCTION.md`. That's how the next
handoff stays cheap.
