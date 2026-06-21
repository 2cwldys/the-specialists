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
  C++ → **build into `hl.so`** → verify factory export → **run in-engine**.
- **Runtime-verified in-engine:** `CTSModel` (108 `ts_model` spawned on
  `ts_kissmeyerbasic`) and `CTSMapGlobals` (weapon-restriction correct on
  `ts_trinityfall`). Test-rig section in `reference/RECONSTRUCTION.md`.
- **Foundation installed:** `CTSGameRules : CHalfLifeMultiplay` is now the live
  `g_pGameRules` (`ts_gamerules.{h,cpp}` + `InstallGameRules`); `GetRandomPowerup`
  done, rest inherits MP for now. `CBasePlayer` TS member block + powerup
  accessors (`player.h` + `ts_player.{h,cpp}`) — no regression. `TSHudMessageAll`
  + `TSHudMessageOne` in `ts_utils`.
- **Built + factory-verified** (no stock map exercises them yet): `CTSFog`,
  `CTSTimer`, `CTSCustomTarget`.
- **Draft, not in build:** `CTSSlowMotion` (`dlls/ts_slowmotion.cpp`) — blocked on
  `CTSGameRules::GlobalSlowMotion2` + the TS player slow-motion virtuals, and
  still uses int-return virtual sigs (reconcile to modern `bool` when you wire it).
- Decision locked: **functional fidelity, NOT byte/ABI match** (CLAUDE.md mission
  explains why — gcc2 old-ABI original vs Itanium modern build).
- **WATCH OUT:** the `recovered/` decompiles can be **truncated** by Ghidra
  noreturn artifacts (it cut `CTSModel::Spawn` off at `UTIL_SetSize`, hiding the
  whole cycler tail). Cross-check the raw disassembly (`objdump -d` at the
  symbol's real `.so` address — note Ghidra adds a 0x10000 base vs `nm`) when a
  decompile looks suspiciously short.

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
**The trivial standalone entities are done. The foundation is now the critical
path** — dep-scanning showed every remaining high-frequency map entity is blocked:
`CTSPowerUp` (328) needs `CTSGameRules::GetRandomPowerup` + `TSGameRules` global +
`PlayerAddPowerUp` (TS player) + `TSHudMessageOne`; `CTSEmitter`/`CTSParticle`
need the networked `CTSSharedEmitter`; `CTSSlowMotion` needs
`CTSGameRules::GlobalSlowMotion2` + TS player virtuals. (Details + the dep table
in `reference/RECONSTRUCTION.md`.)

CTSGameRules skeleton + the CBasePlayer member block/accessors are now in (done).
The core is a monolith — `CTSGameRules ↔ CBasePlayer ↔ CTSGun ↔ powerups` — so:
1. **`CTSGun`** (44 funcs, biggest class) — the weapon stack. `PlayerAddPowerUp`
   (decoded @0x87bc8) needs `CTSGun::MultiplyClips` + `CBasePlayer::PickUpWeapon`
   for the grenade/infinite-clip powerups, so CTSGun gates a full `CTSPowerUp`.
2. Then `PlayerAddPowerUp` (full) + `PutPowerUp` → **`CTSPowerUp`** (clears the
   last `Can't init` on the test map; 328 placements).
3. `GlobalSlowMotion2`/`SetGlobalSlowMotion` → **`CTSSlowMotion`**; then
   `CTSSharedEmitter` (emitter/particle, also needs a client.dll side).

After each class: rebuild, `cp hl.so` into the rig, boot a map that uses it, grep
the log for `Can't init <class>` disappearing (+ a temp `ALERT` probe for positive
proof). Maps per class: frequency table in `reference/RECONSTRUCTION.md`.

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
