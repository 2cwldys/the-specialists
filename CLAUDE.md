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

### 4b. When static RE dead-ends: LIVE gdb trace + the Windows `mp.dll` cross-reference (cont.69u)

Static reading dead-ends on **virtual/indirect calls** (you can't follow `entity->vtable[slot](...)` by
eye) — and a wrong guess there is "making shit up vs the RE" (the cardinal sin; the goal is drop-in
fidelity). **The user KEEPS THE ORIGINAL TS RUNNING in `hl.exe` (Proton/Wine) and it is ALWAYS available
for you to debug live — reach for it as a first-class tool whenever static RE is uncertain or dead-ends.
Don't guess; trace it.** Two tools settle it definitively:

- **★ Live gdb backtrace on the running ORIGINAL.** The real TS runs in `hl.exe` under Proton/Wine (the
  server is `ts/dlls/mp.dll`, the client `ts/cl_dlls/client.dll`). This box can `gdb -p <pid>` it
  (ptrace_scope=1 but same user works; gdb shows the 32-bit code as garbage by default — read raw bytes,
  not the auto-disasm). Recipe:
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
