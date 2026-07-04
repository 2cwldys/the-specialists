# mod-ts

**Mission: produce a functional, source-faithful reconstruction of the original
"ts" (The Specialists) Half-Life mod — both the server and client game libraries
— as clean modern C++ on the HL SDK, that compiles, loads in the engine, and
behaves identically to the original.**

We recover the SOURCE that produced the shipped binaries (not a port). The bar
is behavioural fidelity: same entities (`LINK_ENTITY_TO_CLASS`), keyvalues, game
mechanics, save fields — verified by **build + run + behaviour**, NOT byte/vtable
match.

**The compatibility target is the EXTERNAL INTERFACE, not the internal C++ ABI.**
The real goal: our server and our client must each be a **drop-in swap** for the
original — run our client against the original server (or vice-versa) and behaviour
is identical. That makes the *interface* the contract that must match the original
exactly:
- the engine-facing **C interface** (`GetEntityAPI2`/`DLL_FUNCTIONS`/
  `LINK_ENTITY_TO_CLASS` entity factories, `HUD_*`/`ClientFactory` on the client);
- the **client↔server protocol**: user-message **names + wire formats** (e.g.
  `RoundTime`, `TSHealth`, …), **event** names + param packing (`events/fire.sc`),
  **console command** names, and **cvar** names. These are the swap contract — get
  them byte-faithful (this is why the inherited `TSRoundTime` had to become the
  original's `RoundTime`).

**NOT a target: the internal C++ ABI or gcc-2.x quirks.** The original used the
GCC 2.x old ABI (vtable ptr @0x60, `gnu-v2` mangling, `cdecl`-this, `int`-returning
virtuals); we build on the Itanium-ABI `halflife-updated` SDK (vptr @0, `bool`
`KeyValue`/`Save`, …), which is intentionally newer than the original's 2007 SDK.
So we do **not** chase member offsets, vtable layout, or old-compiler quirks —
recovered offsets are provenance notes only; write members naturally in modern-SDK
style. Behaviour + interface match, verified by **build + run + behaviour**.

This repo is part of the **goldsrc.net** effort (see `../CLAUDE.md` in the
parent for the wider browser-playable GoldSrc architecture). It is **private**;
the buildchain and the other goldsrc-net forks are public.

---

## 1. What this repo is

A fork of **`goldsrc-net/halflife-updated`** (branch `64bit-demo`), which is the
modern open HL SDK we reconstruct *onto*. Remotes:

- `origin`   → `git@github.com:goldsrc-net/mod-ts.git` (private)
- `upstream` → `git@github.com:goldsrc-net/halflife-updated.git` (pull SDK fixes
  via `git merge upstream/64bit-demo`)

Work happens on `main`.

The mod ships as a **standalone game directory** `ts/` (`-game ts`,
`fallback_dir "valve"`). The manifest lives at `game/ts/liblist.gam`; only files
the mod overrides live under `ts/`, everything else falls back to `valve/`.

---

## 2. Reconstruction strategy

The originals + extracted data live in **`reference/`** (see its README). Three
complementary symbol sources, all from one shared codebase (client + server are
built from the same tree, as all HL SDK projects are):

| Artifact | Toolchain | Gives us |
|---|---|---|
| `reference/ts_i386.so` | Linux GCC 2.96, **not stripped** | Complete **server** function inventory (2,426 funcs) + addresses → Ghidra auto-names them. No DWARF/STABS for mod code. |
| `reference/mp.dll` | Windows MSVC (server) | **159** exports → fully-typed signatures (return, params, `__thiscall`, const) via `llvm-undname`. |
| `reference/client.dll` | Windows MSVC (client) | **~1,013** exports incl. HL client iface (`HUD_*`, `ClientFactory`) + VGUI. The client-side ABI spine. |
| `reference/ts_fgd.fgd` | Hammer/Worldcraft | 177 entity defs → keyvalues (member vars), spawnflags (flag bits), I/O for the `CTS*` entity classes. |

`reference/dump-signatures.sh` regenerates the demangled signatures + class
inventory.

### Scope (measured, not estimated)

Diffing the binary's classes against the classes defined in this repo's SDK
source (`comm`):

- **437** stock HL SDK classes (already have source — reuse, do not RE).
- **216** classes present in the `.so` binary.
- **60** classes are **TS-specific** = the reverse-engineering work-list
  (`reference/RECONSTRUCTION.md`). ~**386** functions across them.
- **23** of the 60 are FGD entity-backed (the FGD has 24 `ts_*` entities) →
  keyvalues/spawnflags recoverable directly from the FGD, which anchors the RE
  of those classes.

Biggest TS-specific classes by recovered function count: `CTSGun` (44),
`CTSGameRules` (41), `CTSStunt` (25), `CTSAnimation` (14), `CTSFlag` (13), …

### Per-class workflow

1. Confirm the class is TS-specific (in `reference/RECONSTRUCTION.md`, not in SDK
   source). If it exists in the SDK, reuse/extend that source instead.
2. Decompile its functions: `scripts/decompile.sh reference/ts_i386.so recovered/server <ClassName>`
   → `recovered/server/<ClassName>.c`, each function annotated with its recovered
   prototype (symbols auto-applied; `recovered/` is gitignored).
3. Read the prototype from `reference/signatures-gcc2.txt` (server, all funcs) or
   `signatures-mp.txt`/`signatures-client.txt` (MSVC, fully-typed) for exact
   return/param types.
4. For entity classes, cross-check members against the FGD keyvalues + spawnflags
   in `reference/ts_fgd.fgd`.
5. Write the class into `dlls/` (server) / `cl_dll/` (client) in **modern-SDK
   style** — natural member declarations, `bool`-returning `KeyValue`/`Save`,
   `SetThink`/`SetTouch`, `pev->`. Recovered offsets are provenance comments, not
   targets. Mark uncertain spots with `VERIFY`.
6. Add the file(s) to `dlls/CMakeLists.txt` (explicit source list), then
   **build + verify**: `scripts/build.sh amd64`, then
   `nm -D build-amd64/dlls/hl.so | grep <entity_name>` shows the `T <entity>`
   C factory (engine-spawnable) and `nm … | c++filt | grep <Class>::` shows the
   linked methods. Runtime/behaviour test (spawn the entity in the engine) is the
   final check once enough of the mod is assembled.

---

## 3. Building

mod-ts is **not** a buildchain submodule (clean separation — the buildchain is
provenance-only for the public server stack). It consumes the buildchain's
Docker image so binaries keep the glibc-2.28 floor (run on glibc ≥ 2.28).

```bash
# one-time: build the toolchain image from the buildchain
( cd ../hlds64-buildchain && make image )      # → hlds64-buildchain:debian10

# build hl.so for an arch (default amd64)
scripts/build.sh amd64        # → build-amd64/dlls/hl.so
scripts/build.sh aarch64
scripts/build.sh i386
```

Always build deployables through the image, never natively (a native build on
this host links glibc 2.38+ and won't run on Debian 12 / AL2023). Verify:
`objdump -T build-<arch>/dlls/hl.so | grep -oE 'GLIBC_[0-9.]+' | sort -V | tail -1`.

The base SDK **builds clean** (`build-amd64/dlls/hl.so`, glibc floor 2.14), and
reconstructed classes are added to `dlls/CMakeLists.txt` and built into it. The
image `hlds64-buildchain:debian10` is already built on this host.

The **client** builds too: `scripts/build.sh <arch> client` (i386 → Valve `vgui.so`,
amd64 → openvgui; see HANDOFF cont.46). `scripts/build.sh <arch> all` builds both.

**Keep the 4 build source lists in sync.** We compile `hl.so`/`client.so` through four
parallel build systems, each with its OWN explicit `.cpp` list: the CMake authority
(`dlls/CMakeLists.txt` + `cl_dll/CMakeLists.txt`, used by the amd64/aarch64/win64 CI),
the legacy i386 Makefiles (`linux/Makefile.hldll` + `Makefile.hl_cdll`, the `linux-i386`
CI job), and the MSVC projects (`projects/vs2019/hldll.vcxproj` + `hl_cdll.vcxproj` +
their `.filters`, the `win32` CI job). Adding a reconstructed file to CMake ONLY (the
default habit) silently breaks the Makefile/MSBuild CI jobs. **Whenever you add/remove a
source file, run `scripts/check-source-lists.py`** — it normalizes every `.cpp` each build
compiles and set-compares the Makefile+vcxproj against the CMake authority, printing a
per-file MISSING/EXTRA report and exiting non-zero on any drift (CI/pre-commit friendly).

### Running / testing (this box HAS a GPU + display — not headless)

Two engines run the mod:
- **xash3d-fwgs (graphical client + server, for visual testing).** `scripts/setup-xash-mod.sh`
  assembles a native game root at `~/Projects/goldsrc.net/xash3d-fwgs/dist/client-x64` with
  our `client.so` + `hl.so` (re-run after every rebuild). Launch:
  `cd …/dist/client-x64 && LD_LIBRARY_PATH=. ./xash3d -dev 2 -game ts +deathmatch 1 +maxplayers 8 +map tm_avana`.
  **`+deathmatch 1 +maxplayers 8` is REQUIRED** — without it the listenserver runs single-player
  rules, so stock `respawn()` (dlls/client.cpp) does `SERVER_COMMAND("reload")` instead of
  re-spawning, and pressing fire-to-play bounces you straight back to spectate in an endless
  map-reload loop (looks like a crash; it is not). It also installs CHalfLifeRules instead of
  CTSGameRules. With `+deathmatch 1` you spawn clean.
  Kill an old instance with `pgrep -x xash3d | xargs kill` (NOT `pkill -f client-x64/xash3d` —
  that SIGTERMs the shell; also avoid `pgrep -f gdb`, it matches its own command line). Screenshot
  a focused window: `WID=$(xdotool search --class xash | head -1); import -window $WID out.png`.
  NB: xash on x86-64 needs the ARCH-SUFFIXED lib names `hl_amd64.so`/`client_amd64.so` (the setup
  script handles it).
- **ReHLDS (dedicated server + rcbot, for server behaviour).** The rig at
  `~/Containers/hlds/hlds-linux64-test` — see `reference/RECONSTRUCTION.md` "In-engine test rig".

gdb both `hl.so` (server) and `client.so` (client) the same way — see §4. The full pickup
state + the current reconstruction roadmap live in `HANDOFF.md`.

---

## 4. Tooling (installed this session)

Two sides: **Ghidra/rizin/objdump read the *original* `ts_i386.so`** (what we're
recovering); **gdb debugs *our own* built `hl.so`** (what we're producing). Reach
for gdb the moment a runtime test crashes — don't guess from the symptom.

- **Ghidra 12.1.2** — primary decompiler. On PATH as `Ghidra` / `ghidra` (GUI)
  and `analyzeHeadless` (batch). Installed at `~/Tools/ghidra`; uses the
  system JDK 21. Drive headless:
  `analyzeHeadless <proj_dir> <proj> -import reference/ts_i386.so -postScript <script>`.
- **rizin `rz-*` tools** (static, `~/Tools/rizin`, on PATH: `rz-bin`, `rz-asm`,
  `rz-find`, …) — fast sidecar for symbol/export/section/disasm queries. NOTE:
  the static package has **no** main `rizin` REPL binary; use Ghidra for deep
  analysis.
- **`llvm-undname-17`** — MSVC demangler (used by `dump-signatures.sh`) for the
  `mp.dll`/`client.dll` exports.
- **`scripts/gnv2demangle.py`** — our own GNU g++ 2.x demangler (this host's
  `c++filt` dropped `gnu-v2`). Recovers typed C++ signatures for **all** server
  functions including the TS-specific classes that `mp.dll` never exported.
  ~74% of the 2426 `.so` symbols (1788) resolve; the rest are plain C/CRT or
  rare array/edge-case mangling. Output: `reference/signatures-gcc2.{map,txt}`.
  `scripts/decompile.sh` feeds the map into Ghidra so each decompiled function
  is annotated with its real prototype (`// CTSSlowMotion::Spawn()`).
- **`scripts/extract_weapons.py`** — pulls the full `s_weapon_info` weapon table
  (38 weapons: models/anim-ext/fire/clip/damage/fire-type) out of each
  `Write_<weapon>` in the `.so` by resolving PIC string `lea`s + numeric
  immediates. Emits the C table for `dlls/ts_weapons.cpp`. Pattern for bulk
  data-table recovery.
- **objdump** — trivial dumps.
- **gdb** — debug **our own `hl.so`** (the engine loads it; the rig keeps debug
  syms). The server-side counterpart to Ghidra. When a runtime test segfaults, get
  the backtrace instead of guessing:
  `cd <rig> && LD_LIBRARY_PATH=.:$LD_LIBRARY_PATH gdb -batch -nx -ex run -ex bt`
  `  --args ./hlds_linux -game ts +sv_lan 1 +maxplayers 8 +map <map>`
  (gdb intercepts SIGSEGV before the engine's breakpad handler). This caught the
  `EV_Precache` and `DeathNotice` crashes directly. valgrind (memory errors) / rr
  (reverse-debug nondeterministic crashes) are heavier complements if needed.

### 4a. Drop-in coverage harness + the typed-Ghidra RE flow (cont.67–68)

The reconstruction is now **gap-driven by a coverage harness** and the Ghidra decompiles are
**typed + de-truncated**. Full how-to in `HANDOFF.md` ("⭐ Coverage harness + Ghidra RE flow").
The loop: `coverage.py` finds a gap → Ghidra (typed) REs it → implement → re-run the harness to
confirm the gap closed.

- **`scripts/coverage/coverage.py all`** → `reference/coverage/REPORT.md`: diffs the EXTERNAL swap
  contract (entity classes, cldll exports, server+client commands, server+client cvars w/ default +
  message wire-size mismatch, user messages, events, entity keyvalues + a default.cfg cross-check).
  MISSING = implement/RE; EXTRA = invented/HL-leftover. Cvars/sizes come from the BINARY, not config.cfg.
  Pure objdump/nm/readelf — no Ghidra dep. Also `reconstruction.py` (per-class method %) + `extract_vtables.py`
  (all 226 vtables → slot→method) + `vtcall.py` (resolve/annotate vtable-dispatch offsets).
- **`scripts/coverage/audit.py`** (cont.72) → `reference/coverage/AUDIT.md`: the INTERNAL-structure audit —
  diffs OUR build vs the original (`ts_inventory.json`) BOTH directions at file/class/method/member/global level.
  **MISSING** (orig has, we don't = work-list) + **INVENTED** (we have, orig doesn't = review for drift) + FILE
  mismatches. `audit.py <Class>` = one class's full bidirectional view; `audit.py --strict` = CI gate (exit 1 on
  any invention). TS-scoped (stock-SDK diffs are `divergence.py`'s job). This is the DRIVER for the code-restructure
  to source parity. `coverage.py` audits the EXTERNAL contract; the two together make the whole json checkable.
- **`scripts/coverage/client_members.py`** (cont.72): member skeletons for the CLIENT HUD/prediction classes
  (client.dll has no symbols/RTTI) — binmap-matches our built `client.so` → `client.dll`, types `this`
  (`ApplyClientThis.java`), runs `DumpMembers` → `members_auto_client.tsv`. See §4b.
- **DETERMINISM (cont.72):** the member pipeline is now 0-diff across runs. `re.sh analyze` builds a cached
  single-threaded `re-base`; each `re.sh setup` COPIES it (pins the analysis = reproducible AND faster, no
  re-analyze). `-max-cpu 1` on setup/members pins the member SET; `DumpMembers` canonicalizes the type column
  (Ghidra's decompiler integer-type inference wobbles, unreachable by -max-cpu; float/double/ptr stable). Re-run
  `re.sh analyze` only on a binary/analysis change.
- **`scripts/coverage/setup-ghidra.sh`** builds persistent projects under `recovered/ghidra/{server,client}`
  (gitignored, reboot-safe) with the noreturn truncation disabled + the gcc2 signatures/entvars_t/MSVC
  return types applied (`scripts/ghidra/{DisableNoReturn,FixNoReturn,ApplyTypes,DecompAt,DecompRefs}.java`),
  so decompiles read with typed `this`/args/returns + `pev->fields`. The huge `ClientCommand` still
  truncates in Ghidra — use objdump + GOT (`_GLOBAL_OFFSET_TABLE_`=0x165E10) for its dispatch; `readelf -W -r`
  resolves the .data pointers (vtables, cvar_t, message names) that read as zero in the file.

#### 4a.1 The `re.sh` orchestrator + ApplyInventory (the current server-RE front-end)

`scripts/ghidra/re.sh` is **one entry point** for the whole server pipeline — prefer it over hand-chaining
`analyzeHeadless` for the server `.so`. The applied project lives at `recovered/ghidra/re` (gitignored).

```bash
scripts/ghidra/re.sh setup            # (re)build the applied project: run.py (refresh inventory) +
                                      #   fresh import/analyze + ApplyInventory. Do once, and after
                                      #   coverage/script changes. FRESH each time on purpose (below).
scripts/ghidra/re.sh members          # DumpMembers -> reference/coverage/members_auto.tsv skeleton
scripts/ghidra/re.sh <Class>          # decompile every method of a class (addrs via filemap.py)
scripts/ghidra/re.sh 0x7ea30 0x7decc  # decompile by Ghidra address (= nm addr + 0x10000)
scripts/ghidra/re.sh refs <Class|addr># who CALLS/uses it — decompile every referencing function
scripts/ghidra/re.sh export <Class>   # write the class's decompile to recovered/decomp/<Class>.c
```

- **`ApplyInventory.java`** is a **superset of `ApplyTypes.java`** — the JSON/tsv-driven script that makes
  the server decompiles read like source. It is driven by `reference/coverage/` artifacts
  (`ts_inventory.json` omni dump + `members*.tsv`/`hierarchy.tsv`/`precache.tsv`/`funcfile.tsv` + `VTABLES.md`
  + `signatures-gcc2.map` + MSVC returns + `progdefs.h`/`eiface.h`). Per class it builds a member **struct**
  from the save tables (merged up the chain, `pev`@0 → `this->poweruptype`) and a **vtable** struct (typed fn
  ptr per slot at vptr `@0x60` → `this->vtbl->Precache(this)`). Program-wide it applies typed gcc2+MSVC
  prototypes, `entvars_t` (`this->pev->takedamage`, `gpGlobals->time`), `enginefuncs_t` + the PIC GOT slots
  (`g_engfuncs->pfnSetModel(...)`), typed `.rodata` FP constants relabelled with their value (`dc_0_1`/`fc_64`),
  clean demangled per-class names, and a plate comment per function (its `.cpp` + assets/cvars/commands it refs).
- **Not idempotent** — ApplyInventory *renames* functions, so re-applying breaks the mangled-name lookup.
  That's why `re.sh setup` always rebuilds the project FRESH; the decompile/refs/export paths only read it (fast).
- **`DumpMembers.java`** recovers transient (unsaved) members: their names aren't in the binary, but their
  OFFSET + TYPE are recoverable from how each class's methods access `this`. It emits `members_auto.tsv` (a
  placeholder-name skeleton, excluding the save-table offsets). **Curate** those placeholders into
  `members_re.tsv` with RE-grounded names (e.g. `field_0x88` → `m_flExpire`), which ApplyInventory then layers
  on top. (Workflow: `run.py`→`members.tsv` = saved/named/authoritative; ApplyInventory+DumpMembers→
  `members_auto.tsv` = unsaved offset+type; you curate → `members_re.tsv`.)
- **`DecompRefs.java`** now takes **multiple** addresses and decompiles every unique referencing function —
  the static caller-finder ("who calls/uses this?"). `re.sh refs <Class>` resolves a class to all its methods
  first. **Correctness guards** baked into ApplyInventory (don't trust an unverified label): class structs are
  PADDED so an unsaved member renders honestly as `this->field_0x88` (never wraps to `this[1].<wrong field>`),
  and `entvars_t` array-size macros (`controller[NUM_ENT_CONTROLLERS]` etc.) are resolved before parsing so
  entvars doesn't drift and mislabel every field past `~scale`. See `[[ghidra-correctness-gotchas]]`.

#### 4a.2 `dc.sh` — decompile-by-address for client.dll / mp.dll (the re.sh counterpart)

`re.sh` is server-only (`ts_i386.so`, the `re` project). For the **Windows binaries** use
**`scripts/ghidra/dc.sh <client|mp|server> 0x<addr> [0x<addr>…]`** (built cont.69v) — it picks the right
Ghidra project (`recovered/ghidra/{client clientdll, mpdll mpdll, re re}`), applies the correct address base
(PE imagebase `0x10000000 + RVA` for client/mp; `addr + 0x10000` for the server `.so`), runs `DecompAt.java`,
and cleans the output with the same multi-line filter as re.sh. Feed it the **RVA straight from a live trace**
(`live.py resolve`/`bt`). It replaces the long hand-typed `analyzeHeadless …` incantation. For the server,
prefer `re.sh <Class>` (it resolves Class→methods + applies the full inventory); `dc.sh server` is the quick
by-address path. The mp.dll has 159 typed MSVC exports → readable `Class::Method` signatures (use it to name a
live-trace RVA, then find the SAME function in `ts_i386.so` and decompile it with the typed `re.sh` flow).

The whole client/server RE + live-debug toolchain, at a glance: `re.sh` (server decompile, typed) · `dc.sh`
(client/mp/server decompile by addr) · **`scripts/live.py`** (drive the running original — `pid`/`base`/
`resolve`/`bt`/`read`/`readabs`/`vmethod`/`watch`/`break`; see §4b) · `scripts/coverage/*` (the gap harness).

#### 4a.2b Caller-finding: `refs` (direct) vs `vrefs` (virtual) + the member-name producer (cont.71 cont.3)

- **`re.sh vrefs <Method>`** (`scripts/coverage/vrefs.py`) — the **reverse virtual-call finder**. A virtual call
  goes through `this->vtbl[slot]` and NEVER references the method's address, so `re.sh refs`/`DecompRefs.java`
  AND Ghidra's `get_function_callers` all find NOTHING for an overridden method (this is why tracing
  `CTSGun::PrimaryAttack`'s callers returned empty). `vrefs` inverts `vtables_original.json` (method→slots),
  scans the objdump disasm for `call *0xSLOT(%reg)` sites, and ranks: **self-dispatch** (the calling fn's own
  class has the method at that slot — high confidence, e.g. `vrefs PrimaryAttack` → `CBasePlayerWeapon::ItemPostFrame`)
  vs same-slot-other-class (possible). Use `refs` for non-virtual/direct, `vrefs` for virtual. (Neither is the
  receiver-exact truth that a live gdb bt gives — see §4b — but `vrefs` is the static first cut.)
- **`scripts/coverage/curate_members.py`** — the **producer for `members_re.tsv`** (which was an ORPHAN: no
  script made it, so it stayed empty and every transient member rendered as `this->field_0xNN`). It scrapes our
  reconstruction headers' offset provenance (`m_weaponType … (gun+0xc8)`, struct-relative `// +0xNN`, `name@0xNN`)
  and **cross-checks each against the BINARY-derived `members_auto.tsv`** — emitting only corroborated
  `(class,offset)` rows (type from the binary, name from our source), and reporting the rest as UNVERIFIED rather
  than inventing a wrong-but-plausible label. Wired into **`re.sh members`** (runs after `DumpMembers`); re-apply
  with `re.sh setup`. The member loop is now: `members.py`(saved) → `DumpMembers`(unsaved skeleton) →
  **`curate_members.py`**(names, from our RE) → `ApplyInventory`(merges saved>curated>auto).
- ✅ **FIXED (cont.71 cont.4):** `members_auto.tsv` used to **miss the biggest transient-only TS classes** —
  `CTSGameRules`/`CTSStunt`/`CTSAnimation` had ZERO recovered members. Root cause was NOT demangling (all their
  symbols demangle): `ApplyInventory` only gives a class a real struct (so the decompiler renders `this->field_0xNN`)
  when it's in `members.tsv`/`hierarchy.tsv`; a class of pure transient state is in neither, so its `this` stays an
  OPAQUE pointer and accesses read `*(t*)(this + 0xNN)`. **Fix = teach `DumpMembers` to ALSO parse that opaque form**
  (not make every class a giant struct — that was a dead end: padding all 223 classes to 0x8000 overflowed the
  decompiler's datatype budget and made it FAIL on stock functions, cratering coverage to 174). With the opaque
  parse, `members_auto` went 860→1028 rows / 125→**137 classes**, recovering CTSStunt/CTSGameRules/CTSAnimation/
  CTSSharedEmitter/CBulletManager with **zero regression**, and `curate_members` then named them from our source.
- **Completeness over speed (cont.71 cont.4):** all Ghidra-script decompile timeouts raised to **900s/fn**
  (`DumpMembers`/`DecompAt`/`DecompRefs`/`DecompExport`) so a slow function is never silently dropped.

#### 4a.3 The Ghidra **MCP** — interactive front-end ON TOP of our tooling (cont.71 cont.3)

There's a **Ghidra MCP** server (`mcp__ghidra-mcp__*`, 184 tools) that attaches to a **live Ghidra GUI
instance** over a UDS socket. It is **not a replacement** for `re.sh`/`dc.sh`/`coverage` — it's a faster,
interactive *front-end* on the SAME projects those scripts build. Use it for **reading + navigating**;
keep the scripts for **building the typed project + tracking completeness**. Three tools, three jobs:

- **Ghidra MCP** → interactive decompile + **xref/callgraph** navigation. `decompile_function`,
  `get_xrefs_to`, `get_function_callers/callees`, `get_full_call_graph`, `search_functions`, `list_*`.
  Returns a decompile in ~1s (no per-call `analyzeHeadless`/JVM spin-up), and it does the thing `re.sh`
  CAN'T: "who calls this / what xrefs / follow the chain" without dropping to `objdump`+grep. **This is now
  the first reach for "read/trace this function and who calls it."**
- **`re.sh` / `dc.sh`** → BUILD the typed `re`/`clientdll`/`mpdll` projects (ApplyInventory) + batch-decompile
  a whole class + the reproducible/scriptable path. The MCP only has the typed view BECAUSE `re.sh setup`
  built it; a fresh MCP `import_file` is **untyped** (`*(int*)(param_1+0xcc)`, not `this->m_pActiveItem`).
- **`scripts/coverage/*`** → the gap harness / inventory / completeness metric. The MCP does NONE of this.
- **`scripts/live.py` (gdb)** → RUNTIME behaviour. No static tool — MCP included — answers held-vs-click,
  recoil, or any "what actually happens at run time" question. That's always the live trace (§4b).

**Setup to get the BEST of both:** run **`scripts/ghidra/mcp-up.sh`** first — it checks the project lock, finds
the live MCP socket, and prints the exact steps. Then in the Ghidra GUI open **`recovered/ghidra/re/re.gpr`** +
the `ts_i386.so` program; then `list_instances` → `connect_instance("re")` → `open_program("/ts_i386.so")` →
`unload_tool_group("malware")` (drop the malware/crypto/IOC tools we never use — less noise). Now MCP decompiles
are typed (`CTSGun::ShotgunFire(CTSGun *this)`, `pev->`, `/* TS_gun.cpp */` plate) AND you get interactive
xrefs. (A blank instance with no project open → `import_file` fails "No project is currently open"; open/create
a project first.)

- ⚠️ **`.lock` gotcha (the real one):** a Ghidra project (`*.rep/`) is **single-writer** — it takes a lock
  when opened. So **you cannot run `re.sh`/`dc.sh` against a project the GUI currently has open, and vice-versa**
  — the headless `analyzeHeadless` fails on the lock (and `re.sh setup`, which rebuilds `re` FRESH, would
  clobber what the GUI sees). While the GUI holds `re`, drive it via the **MCP** (reads are fine); to go back
  to `re.sh <Class>`/`dc.sh server`/`re.sh setup`, **close the program/project in the GUI first**. (`dc.sh
  client`/`dc.sh mp` use the separate `clientdll`/`mpdll` projects, so they're unaffected unless the GUI also
  has THOSE open.) Corollary: **only READ via the MCP on the `re` project — don't rename/mutate it**, or it
  drifts from the `re.sh`-generated state that `re.sh <Class>`/`export` rely on (mutate a scratch project).

### 4b. When static RE dead-ends: LIVE gdb trace + the Windows `mp.dll` cross-reference (cont.69u)

Static reading dead-ends on **virtual/indirect calls** (you can't follow `entity->vtable[slot](...)` by
eye) — and a wrong guess there is "making shit up vs the RE" (the cardinal sin; the goal is drop-in
fidelity). **The user KEEPS THE ORIGINAL TS RUNNING in `hl.exe` (Proton/Wine) and it is ALWAYS available
for you to debug live — reach for it as a first-class tool whenever static RE is uncertain or dead-ends.
Don't guess; trace it.** Two tools settle it definitively:

- **★ Live gdb backtrace on the running ORIGINAL.** The real TS runs in `hl.exe` under Proton/Wine (the
  server is `ts/dlls/mp.dll`, the client `ts/cl_dlls/client.dll`). This box can `gdb -p <pid>` it
  (ptrace_scope=1 but same user works; gdb shows the 32-bit code as garbage by default — read raw bytes,
  not the auto-disasm). **`scripts/live.py` scripts the whole boilerplate**: `live.py pid` / `base <module>`
  (reloc-free prologue scan) / `resolve 0xADDR…` and `bt` (raw live addr / piped gdb backtrace → module +
  RVA + nearest export) / `read <module> 0xRVA [f|d|u|p]` / `readabs 0xADDR` / `vmethod 0xOBJ <slot>`
  (this→vtable[slot] → resolve) / **`watch <module|abs> 0xRVA|0xADDR --go "msg"`** and **`break … --go`**
  (run BACKGROUNDED: prints `>>> ARMED — <msg>` once set so you tell the user the action, blocks until it
  trips, then prints the resolved backtrace). A value-`watch` fires only on CHANGE → e.g. watching
  `cl.viewangles` with no mouse isolates the recoil (this is how cont.69v located the angle-recoil). Recipe:
  0. **Find the live PID each session** (it changes): `pgrep -f 'Half-Life/hl.exe'` (pick the actual
     `hl.exe` Wine process — the big RSS one, not the launcher/reaper). Ask the user to confirm it's
     running if `pgrep` is empty.
  1. `cat /proc/<pid>/maps | grep mp.dll` → the load region. **Wine maps PE by RVA, and the live base ≠
     `region_start + file_offset`.** So find a known function's TRUE address by searching process memory
     for its prologue bytes: read `reference/mp.dll` at the function's file offset (`rz-bin -E` gives
     paddr/vaddr), then `python3` scan `/proc/<pid>/mem` over the r-xp region for those bytes →
     `live_addr`; **effective base = `live_addr − RVA`** (RVA = `vaddr − 0x10000000`). (Client side:
     same with `client.dll`.)
  2. `gdb -p <pid> -batch -x script.gdb` with a **HARDWARE** bp (`hbreak *<addr>`; hardware = no code
     patching, auto-clears on detach — safe for the user's live game), `continue`, then on hit dump
     `x/200xw $esp` + regs + `detach`. Run it backgrounded with a `timeout`; tell the user to reproduce
     the exact action (e.g. "throw a knife at glass — only the knife") and wait for the hit.
  3. Resolve the stack's return addresses (values in the mp.dll r-xp range) to RVAs (`addr − base`) →
     names via `rz-bin -E reference/mp.dll`. That's the EXACT call chain — the definitive caller-finder.
     (Read args at a fn entry via the stack: `__thiscall` = `this` in `ecx`, args at `[esp+4]`, `[esp+8]`…)
- **`mp.dll` cross-reference (Ghidra project `recovered/ghidra/mpdll`).** mp.dll has **159 fully-typed
  MSVC exports** (readable `Class::Method` signatures, far clearer than gcc2 mangling). Use it to (a)
  resolve live-trace RVAs to readable names, and (b) confirm a function's name so you can find the SAME
  function in `ts_i386.so` (same source, gcc2) and decompile it with the typed flow above. Build it like
  the others: `analyzeHeadless recovered/ghidra/mpdll mpdll -import reference/mp.dll -preScript
  DisableNoReturn.java`, then `DecompAt.java` at `0x10000000+RVA`.
- **★ `scripts/binmap.py` — resolve a `.so` function NAME to its live `mp.dll` RVA (cont.71 cont.4).** mp.dll
  exports only ~325 of ~2400 functions, so to `live.py break` a NON-exported one (e.g. `ShotgunFire`) you need
  its address. `binmap.py <name>` matches the two binaries (same source) two ways: **export-name** (exact, 325)
  + **string-anchor** (a fn referencing `"events/pump.sc"` in the `.so` references it in mp.dll — 286 confident
  + 170 flagged `string?` ambiguous, when several siblings share strings). `binmap.py CTSGun::Precache` →
  `mp.dll 0x54190` → feed to `live.py break mp.dll 0x54190`. ⚠ heed the `string?` AMBIGUOUS flag — verify those
  with a live trace (or breakpoint a uniquely-string'd caller and step in); a string-poor fn like `ShotgunFire`
  won't match at all (fall back to a caller). Cache: `binmap.py --build` → `reference/coverage/binmap.tsv`.
  binmap is **env-parameterizable** (`BINMAP_SO/MP/GOT/MAP/OUT`) so the SAME machinery does the CLIENT pair.
- **★ `scripts/coverage/client_members.py` — member skeletons for the CLIENT (HUD/prediction) classes (cont.71 cont.4).**
  `client.dll` has NO symbols, NO RTTI (only `.?AVtype_info@@`), and the HUD classes aren't exported — so the
  server's symbol-driven member recovery doesn't transfer. Instead it brings the NAMES over from OUR build:
  (1) binmap-match our `build-i386/cl_dll/client.so` (full symbols) → `client.dll` RVAs (→ `binmap_client.tsv`);
  (2) `ApplyClientThis.java` marks those functions `__thiscall` + renames them into class namespaces;
  (3) `DumpMembers.java` (it now reads the class from the NAMESPACE and parses the `(int)this + 0xNN` cast form
  that MSVC's `void*` auto-`this` produces) → `members_auto_client.tsv`. Yields the CHud* skeletons (CHud/
  CHudHealth/CHudSpectator/CHudTSWeaponBar/…, ~51 classes). Gotchas hit building it: Ghidra's `__thiscall this`
  is an AUTO-param (`setDataType` throws "Auto-parameter may not be modified" — use `updateFunction`), and a
  pointer to a SIZE-0 opaque struct renders `void *this` (pad it, or just read the namespace + `(int)this` form).

**Lesson (the knife→glass break):** I claimed I "couldn't find" it and nearly invented an angle/
accumulation model — but it was in `ts_i386.so` all along (`WaitThink__8CTSKnife`, a per-frame swept
`TakeDamage`); I'd just missed it in a grep. The live trace + mp.dll names pointed straight at it. **So
when a static trace dead-ends, do the live gdb backtrace BEFORE reconstructing — don't guess.** (See the
`flag-unfound-re-early` memory.)

---

## 5. Change ledger (Serena)

We maintain a running ledger of reconstruction progress as **Serena memories**
(MCP) scoped to this project — one entry per meaningful step (class reconstructed,
ABI verified, decision made). At the start of a session, read the Serena memories
to see what's done; append a new ledger entry when you complete a unit of work.
Keep the ledger factual: what was reconstructed, how it was verified, what's next.

---

## 6. Conventions

- **No `Co-Authored-By` trailers in commits** (user preference across all
  goldsrc-net repos).
- Match the SDK's surrounding code style when writing reconstructed source.
- `reference/` is read-only source-of-truth — never modify the original
  binaries; regenerate derived data with `dump-signatures.sh`.
- A class is "built" when it compiles + links + exports its entity factory; it's
  "done" when its **behaviour** is verified in the engine — not just when it
  compiles. Be honest about which.

---

## 7. Picking this up

`CLAUDE.md` is **local-only** (matched by the user's global `~/.gitignore`), so a
fresh clone won't have this file. The durable, tracked pickup doc is
**`HANDOFF.md`** (repo root) — read it first. Current progress lives in the
Serena ledger (`.serena/memories/ledger.md`) and `reference/RECONSTRUCTION.md`.
