# HANDOFF — for the next Claude picking up mod-ts

You're resuming the reconstruction of **The Specialists ("ts")** as a functional,
source-faithful HL mod. This is the continuation brief — not a tutorial.

## Read these first (in order)
1. **`CLAUDE.md`** (repo root, auto-loaded on this machine) — mission, method,
   tooling, conventions. The stable reference.
2. **Serena ledger** — read **`.serena/memories/ledger.md` directly** (Read tool). The full
   chronological history of what was done and why. Newest on top.
   ⚠️ **Serena gotcha:** do NOT use `read_memory("ledger")` via the Serena MCP tool — it returns one
   giant single-line JSON (`{"result":"…"}`, ~150 KB) that overflows the read limit. Read the `.md`
   file directly, or `jq -r '.result' <savedfile>` on the file the tool saves.
3. **`reference/RECONSTRUCTION.md`** — the 60-class work-list + live status table.

If those three and this file agree, you're oriented. If they disagree, the
ledger + git log are ground truth; fix the docs.

**Reference docs (the recovered contracts + coverage maps — read the relevant one before
touching that area):**
- `reference/ENGINE_INTERFACE.md` — the COMPLETE engine↔mod contract (every DLL_FUNCTIONS
  callback + cldll export + protocol), each marked faithful/partial/stock/missing. **Start
  here for "what's left" — it lists the real interface gaps.**
- `reference/API_SURFACE.md` — every cvar (104)/command (92)/message (74)/event (11) from
  the binaries + a R/read/★wired implementation-status table (§6c).
- `reference/HUD_MESSAGES.md` — the user-message wire formats (the protocol contract).
- `reference/HUD_DRAWCHAIN.md` — the HUD render path recovered from client.dll assembly.
- `reference/VERIFICATION.md` — the playtest checklist + the (unmapped) silencer findings
  + the think-behaviour status.
- `reference/coverage/REPORT.md` — **drop-in interface coverage** (original vs ours): the
  by-name swap contract — entity classes, cldll exports, console commands, server+client cvars
  (w/ default + wire-size mismatch), user messages, events, entity keyvalues — matched/missing/
  extra. Missing = RE/implement work-list; extra = invented or HL-leftover to remove. **THE
  roadmap** — see the "Coverage harness + RE flow" section below for how to use + regenerate it.
- `reference/coverage/RECONSTRUCTION_COVERAGE.md` — per-TS-class **method-completeness** metric
  (140/368 = 38%, least-complete first: CTSStunt 0/25, CTSSharedEmitter 0/11, …). `scripts/coverage/reconstruction.py`.
- `reference/coverage/VTABLES.md` — every original class's **vtable layout** (slot→method, all
  226 classes), so a decompiled `call [vptr+0xNN]` resolves to the method. `scripts/coverage/extract_vtables.py`.
- `reference/WEAPON_SELECT.md` — the weapon-select protocol (invnext/invprev cursor → `weapon_<N>`
  + holster→deploy; the real command set vs our stand-ins).
- `reference/BUY.md` — the buy/loadout-config command protocol (`tkw`/`tki`/`tka`/`rst_cfg`/`cashala…`).
- `reference/EVENTS.md` — **the 11 TS `.sc` weapon events** (fire/reload/draw/grenade/knife/katana/
  coldcock/closecombat/pump/empty/firelinked): the complete RE map (server handles + client
  hook→handler addrs + playback sites) + the build order for the client weapon-param subsystem.
- `reference/BULLETS.md` (cont.69u) — the **bullet/breakable/slow-mo/sound** RE: live fire path
  `ShootBulletsPlayer` + penetration, glass shatter (`Die__10CBreakable` → gmsgTSCreateDebris sprite
  shower vs TE_BREAKMODEL), dive-through-glass (`CheckForBreakables`), CBulletManager slow-mo bullets,
  knife→glass (`WaitThink` swept-break), + the sound-pitch TODO. Read before touching weapons/breakables.

## ⭐⭐⭐⭐⭐⭐⭐⭐⭐⭐⭐⭐⭐⭐⭐⭐⭐⭐⭐⭐⭐⭐ START HERE (cont.87d) — pickup for the next orchestrator (= you)

**You ARE the orchestrator AND the previous agent — continue as if you never stopped.** Branch
`restructure-source-parity` (NOT merged to `main`; GPG signing OFF: keep `git config --local
commit.gpgsign false`). Every commit is build-green (`scripts/build.sh amd64 all`) + nm-T-gated +
deployed to the xash rig + user-verified. **FIRST:** regenerate the baseline `nm -D
build-amd64/dlls/hl.so | awk '$2=="T"{print $3}'|sort > /tmp/nm_T_baseline.txt`, then read the ledger
`cont.87 cont.3` entry (top) via the **Read tool on `.serena/memories/ledger.md`**.

⚠ **SERENA GOTCHA (still bites):** read the ledger/any big memory with the **Read tool on the `.md`
file directly** — NEVER `read_memory("ledger")` via the Serena MCP: it returns ONE giant single-line
`{"result":"…"}` JSON (~500KB) that overflows the read limit. (`jq -r '.result' <savedfile>` if you
must go through the tool.)

### THE MISSION (the law — the user re-stated it repeatedly this session; internalize it)
Make OUR code a **1:1 drop-in** for the original at EVERY level — **file → class → methods → variables
→ globals → free functions**. Nothing the original has that we lack, nothing we have that it lacks.
The **drop-in contract:** our client must run against the ORIGINAL server and vice-versa,
byte-identically & compatibly (wire names/formats, event/cvar/command names, MECHANISMS). The rules:
1. **A missing OR extra symbol = a LOGIC MISMATCH → re-RE it.** `audit.py`/`coverage.py` match by
   NAME, so **our file/class/method/VARIABLE/global/free-func names must EQUAL the original's**
   (FGD/save/wire names, gcc2-demangled names). A field/method/var that "doesn't exist" on one side is
   a red flag to chase, not a rounding error — **a mismatched variable name can hide that some OTHER
   code path was built against the wrong field** (this session: the knife/katana block read the empty
   `m_szAnimExtention` instead of `m_szAnimOverride`; the knife count was on the wrong field). Match
   variable names to the original.
2. **The logic often EXISTS already — moved/renamed/inlined. GREP OUR CODE and MOVE it, don't
   rewrite.** (This session the sidekick anim, block seqs, gait leg-block, PseudoClimb, TSHandle
   cmd-damp were all already present/correct — the bug was elsewhere.)
3. **MIMIC THEIRS.** "Works differently but fine" is WRONG for a drop-in. Copy the mechanism exactly.
   (This session: buttons-clean-on-spawn vs my invented swallow flag; the original's `DropWeapon` is
   ONE function not two; ShotgunReload never animates.)
4. **The TS decompile is COMPLETE** — `reference/coverage/ts_inventory.json` is the machine-checkable
   inventory of EVERY original class/method/member/global/free-func. So a symbol we lack = a REAL gap
   to RE, not "not decompiled yet." Drive it with `scripts/coverage/audit.py [Class]` (INTERNAL
   structure, both directions: MISSING=work-list, INVENTED=review-for-drift) + `coverage.py all` (the
   EXTERNAL swap contract: entities, cldll exports, commands, cvars, messages, events, keyvalues).
5. **Discovered a gap mid-fix? FLAG it (a task), FINISH the original issue, THEN report** — don't chase
   inline. (This session the drop RE surfaced the flag-drop-conditional + grenade-cook gaps; logged, not chased.)
6. **ASSET references:** every model/sprite/sequence/sound/event/cvar/user-message a decompile
   references MUST be referenced in our code (precache + use). Missing asset refs = missing behavior.
   (This session: `sprites/player/movement.spr`+`kungfu.spr` for the powerup figures — ours used a
   non-existent `superjump.spr` → SPR_Load stale handle drew the wrong sprite; `breathe2.wav`.) When
   touching a class, grep its decompile for `precache`/`.spr`/`.mdl`/`.wav`/`.sc`/`EMIT_SOUND`/
   `PLAYBACK_EVENT`/`SetSequence` and verify each is wired.
7. **`TODO`/`VERIFY`/`SLICE`/`un-RE` comments ARE the standing work-list** — `grep -rn
   "VERIFY\|TODO\|SLICE\|un-RE" dlls/ cl_dll/ pm_shared/` and resolve as you touch each area.

### METHODS THAT WORKED THIS SESSION (this is how you operate — keep using them)
- **★ DOSSIER-SUBAGENT HINT (the big scaler — used ~5× this session, kept ~600k decompile tokens out of
  context).** For any deep/uncertain RE, spawn a `general-purpose` agent (Agent tool) with a
  SELF-CONTAINED prompt: repo root, the tool recipes + addr-base rules (`re.sh 0x<nm+0x10000>` server;
  `dc.sh client|mp 0x<RVA>` Windows), known addresses, an explicit work-list, "do NOT edit source", the
  USER'S EXACT BEHAVIORAL SPEC to reconcile against, and "write the decode dossier + raw decompiles to
  `reference/dossiers/cont<NN>/` and I'll commit it." Its final message = the summary you implement
  from. ⚠ **CROSS-CHECK its labels + verify against the live playtest** — the user (running original) is
  ground truth; a dossier's first rev can read a DEAD path.
- **★ THE USER'S DIAGNOSES ARE OFTEN THE RE-CORRECT ROOT — take them seriously.** This session the user
  called: "clean buttons on spawn" (vs my swallow flag — they were right, StopObserver @0x99a14 does
  `pev->button=0`), the `mouse_oldbuttonstate` freeze, the kung-fu slot cost, the loop-forever breath.
  When the user proposes a mechanism, RE it before implementing your own.
- **★ DEBUG-LOG-IN-SOURCE beats gdb for OUR build.** A temporary `ALERT(at_console,"[TAG] …")` (server,
  `#ifndef CLIENT_DLL`) or `gEngfuncs.Con_Printf` (client) — build+deploy, user does the action + pastes
  the console line — pins the exact value/branch in one shot (this session the `[KNIFE-SV]`/`[KNIFE-CL]`
  traces). Remove after; gate on a change to avoid per-frame spam.
- **★ LIVE gdb on the ORIGINAL** (`scripts/live.py` on the running `hl.exe`) settles what static RE can't.
  The user KEEPS THE ORIGINAL TS RUNNING — reach for it. ⚠ mp.dll is MSVC → offsets differ from the `.so`.
- **★ Ghidra tooling (FULL STACK — there is a LOT):** `scripts/ghidra/re.sh <Class|0xADDR>` (server
  `ts_i386.so`, TYPED by ApplyInventory — `re.sh` addr = `nm addr + 0x10000`) · `dc.sh
  client|mp|server 0x<RVA>` (Windows binaries; client takes the RAW RVA) · `re.sh refs/vrefs`
  (direct/virtual callers) · **the Ghidra MCP** (`mcp__ghidra-mcp__*`, interactive decompile+xref on the
  typed `re` project — CLAUDE.md §4a.3; `scripts/ghidra/mcp-up.sh` to attach; the `.lock` blocks headless
  while the GUI holds it) · `binmap.py` (`.so` name → mp.dll RVA) · `scripts/coverage/*` (`audit.py`
  INTERNAL diff both ways = the parity driver; `coverage.py all` = the EXTERNAL swap contract). See
  CLAUDE.md §4/§4a/§4b for the complete recipes.

### ⭐ SCRATCHPADS / DOSSIERS — PRESERVED + COMMITTED, read before touching these areas
`reference/dossiers/` (committed): `cont85/` (kick decode, anim state-machine), `cont86/`
(superjump/slowmo/gait+shadow + `decomp/`), **`cont87/`** (this session:
`spine_blend.md`, `thirdperson_camera_reticle.md`, `hud_bottomleft.md`, `weapon_drop.md` + `decomp/`
raw decompiles). These are the primary reference to re-derive the hard-won mechanisms. Always tell a
dossier-subagent to write to `reference/dossiers/cont<NN>/` and COMMIT it so the next agent has it.

### ⭐ WHAT cont.87 DID (this large session — full detail in the ledger `cont.87 cont.3` entry)
All build-green + nm-T stable + deployed + USER-VERIFIED, commits `1cf4923..d1d9fc5`. Playtest-driven:
- **3rd-person spine twist** (split-bone yaw re-base, added `QuaternionAngles`); **reticle crosshair
  gun-only**; **knife/katana 3rd-person block** (keyed off `m_szAnimExtention` empty → use
  `m_szAnimOverride`). **TS over-the-shoulder 3rd-person CAMERA** (`cl_camtype` offset presets, smoothed
  render angles) + **screen-space RETICLE** (WorldToScreen of the eye-traced aim; new `v_aimeye`).
- **SPAWN loadout**: kung-fu (TSW_MELEE) must NOT cost inventory slots (SlotsUsed skip); spawn on the
  highest-PRIORITY weapon (GetBetterWeapon(-1), not last-bought). **Ammo HUD**: caliber/firemode full
  color, `[  a  /  b  ]` slash, font = DrawCharacter (NOT DrawConsoleString). **Knife ammo count** (rides
  the WeaponInfo reserve field → LK_COUNT above the block bar).
- **Weapon-select RIGHT-CLICK DROP** (`drop_<N>`): merged our two wrongly-split `DropWeapon()`/
  `DropWeapon(int,float)` into the original's ONE `DropWeapon(int type,float)` @0x84900; `CL_ResetButtonBits`
  now resets IN_ATTACK2. **Low-stamina breathing** (breathe2.wav loop, edge-triggered, CHAN_STATIC).
  **Spawn button clean** (StopObserver `pev->button=0`; + `inputw32.cpp` `mouse_oldbuttonstate` sync so
  the buy-menu click can't leak a phantom +attack). **Powerup FIGURE icons** (`player/kungfu.spr` +
  `player/movement.spr`) + `powerup 7` (debug kung-fu grant). **Pump-shotgun 3rd-person reload removed**
  (ShotgunReload never animates; only clip shotguns via the standard Reload).

### 📋 PARKED / DEFERRED (recreate as TaskCreate entries; none is a user-facing break — all VERIFY/TODO)
- **[flagged] DropWeapon polish** — the re-pickup delay+owner on drop-KEY throws (type<0) + the grenade
  cooked-throw (GrenadeIdle) are TODO in the merged `CBasePlayer::DropWeapon` (M61 currently drops via
  the stackable path); CTSWorldGun pickup-expire fields un-mapped. Also VERIFY: flag-drop is now
  conditional (bare-handed branch), was unconditional. See `reference/dossiers/cont87/weapon_drop.md`.
- **[flagged] Camera dynamic zoom-out ease** — the `localplayer+0x26c & 0x130c30` gate that springs
  `back` −48→−92 while moving is OMITTED (camtype 3 = back 90 already gives the framing); needs a live
  read of the gate bit. Marked `VERIFY` in `cl_dll/view.cpp`.
- **[flagged] `TSA_CC_STOCK`** (rifle-butt melee, `ts_animation.cpp`) appends `m_szAnimExtention`; the
  original @0x5d8d0 case 0x27 appends `field_0xb58` (un-RE'd twin of `m_szAnimOverride`). Rare edge case.
- **[flagged] spawn `player+0xb34`** — un-named transient cleared with `pev->button` in StopObserver, no
  read site recovered (we cleared `m_afButtonPressed` as the likely edge-cache). RE if it resurfaces.
- **[open decision] `iuser4` on the wire (`delta.lst`)** — reverted to clean upstream `3b1d963`; iuser4
  not networked → the client can't PREDICT powerup-gated behavior (double-fire is server-authoritative).
  The original TS table `19af2487` nets iuser4 at 32 bits (+iuser3/playerclass). Decide.
- **[standing driver] audit.py per-class 1:1 parity loop** — `scripts/coverage/audit.py [Class]` +
  `coverage.py all`. The structural grind: drive each TS class to N/N methods/members/globals/free-funcs;
  a missing/extra symbol = re-RE (rule #1). This is the path to "nothing missing, nothing added."
- **[flagged] Trim `CBaseMonster` to the original's 42-method set (source parity).** RE-PROVEN this
  session (cont.88): the original **removed at source** the entire monster-AI schedule runtime
  (`RunTask`/`StartTask`/`RunAI`/`MonsterThink`/`GetScheduleOfType`) + the scripted-sequence system
  (`CineCleanup`/`ExitScriptedSequence`/`CCineMonster`/`CScriptedSentence`) from `CBaseMonster`. Proof it's
  source-removal not linker DCE: the original binary RETAINS dead code (unreachable legacy vgui buy-menu in
  `client.dll` @`0x10010xxx` — 0 xrefs; dead-but-present `CVipEscape` in `ts_i386.so`), yet those methods are
  absent → never compiled. Original `CBaseMonster` = **42 leaf behavioral methods** (`nm ts_i386.so | grep
  __12CBaseMonster`; list: TakeDamage/Killed/**ChangeYaw** (HL25 turn-speed fix KEPT)/GibMonster/BloodColor/…).
  To match: trim our `CBaseMonster` to that set + drop `scripted.cpp` and the `nodes.cpp` entity classes,
  across ~6 shared base files (`basemonster.h`, `monsters.cpp`, `schedule.cpp`, `defaultai.cpp`,
  `monsterstate.cpp`, `talkmonster.cpp`). **Risk:** `CBasePlayer` derives from `CBaseMonster` — verify no
  player call sites into the stripped methods; build + ReHLDS smoke + `audit.py CBaseMonster` after. **Carve-out
  rule (user, cont.88): keep anniversary/HL25 code even where it diverges from the 2007 original — this AI
  system is classic (2013 seed), so it's in scope to cut; `ChangeYaw`'s HL25 fix stays (it's in the kept 42).**
  Entity-level parity is ALREADY exact (EXTRA/MISSING = none); this is the method-level tail.

### ⚠ RIG STATE (unchanged from cont.86b — the CLEAN ORIGINAL)
`~/Containers/hlds/mods/ts` = the clean original TS 3.0 (1127 files, 29 `ts_*` maps; `tm_avana` GONE).
Playtest on `ts_dojo`/`ts_lobby`/`ts_metro`. Launch: `cd
~/Projects/goldsrc.net/xash3d-fwgs/dist/client-x64 && LD_LIBRARY_PATH=. ./xash3d -dev 2 -game ts
+deathmatch 1 +maxplayers 8 +map ts_dojo`. `setup-xash-mod.sh` deploys after every rebuild + pushes
`network/delta.lst` THROUGH the rig symlink (repo file is source of truth). **The user drives ALL
playtests — give numbered protocols, NEVER launch xash/xdotool, and HALT after asking them to do
something in hl.exe** (they lose focus if you keep running commands).

### 💬 MESSAGE TO SEND THE NEXT AGENT (paste this to it)
> You're the cont.87d orchestrator on mod-ts, continuing `restructure-source-parity` as if you never
> stopped — you ARE the previous agent. FIRST read HANDOFF.md → the "START HERE (cont.87d)" block, then
> the ledger `cont.87 cont.3` entry. ⚠ Serena gotcha: read the ledger with the Read tool on
> `.serena/memories/ledger.md` — NEVER `read_memory("ledger")` (returns one giant single-line
> `{"result":"…"}` JSON that overflows). The mission (the law): a **1:1 drop-in** at EVERY level — file →
> class → methods → variables → globals → free functions; nothing missing, nothing added; our
> client/server must swap for the original byte-compatibly. A missing/extra symbol = a logic mismatch →
> re-RE it (match VARIABLE names too — a wrong name hides a wrong-field code path). The logic often
> already EXISTS (moved/renamed) — grep + MOVE it, don't rewrite. MIMIC theirs. The TS decompile is
> COMPLETE (`ts_inventory.json`), so a symbol we lack is a REAL gap. Every asset a decompile references
> (models/sprites/sequences/sounds/events/cvars/messages) must be referenced in our code. Resolve the
> in-code TODO/VERIFY/SLICE work-list. Discovered a gap mid-fix? FLAG it (a task), finish the ORIGINAL
> issue, THEN report — don't chase inline. Methods: the dossier-subagent hint (self-contained prompt →
> writes a dossier + raw decompiles to `reference/dossiers/cont<NN>/` + COMMIT; cross-check its labels vs
> the user's live playtest — the user, running the original, is ground truth); debug-log-in-source
> (`ALERT`/`Con_Printf` → user pastes the console line); live gdb on the running hl.exe (`scripts/live.py`)
> for what static can't settle; the full Ghidra stack (`re.sh`/`dc.sh`/`refs`/`vrefs` + the Ghidra MCP
> `mcp__ghidra-mcp__*`) + `scripts/coverage/{audit,coverage}.py`. Scratchpads/dossiers are preserved under
> `reference/dossiers/`. The user drives ALL playtests — numbered protocols, never launch xash, HALT after
> asking. cont.87d (me) is available for consult — reply/SendMessage if you need context on anything above.

---

## ⭐⭐⭐⭐⭐⭐⭐⭐⭐⭐⭐⭐⭐⭐⭐⭐⭐⭐⭐⭐⭐ START HERE (cont.86d) — pickup for the next orchestrator (= you)

**You ARE the orchestrator and the previous agent — continue as if you never stopped.** Branch
`restructure-source-parity` (NOT merged to `main`; GPG signing OFF: keep `git config --local commit.gpgsign false`).
Every commit is build-green (`scripts/build.sh amd64 all`) + nm-T-gated. **FIRST:** regenerate the baseline
`nm -D build-amd64/dlls/hl.so | awk '$2=="T"{print $3}'|sort > /tmp/nm_T_baseline.txt`, then read the ledger
`cont.86 cont.2` entry (top).

⚠ **SERENA GOTCHA:** read the ledger with the **Read tool on `.serena/memories/ledger.md`** — NEVER
`read_memory("ledger")` (Serena MCP returns one giant single-line `{"result":"…"}` JSON ~500KB that overflows
the read limit). Same for any big memory.

### THE MISSION (the law — the user re-stated it this session; internalize it)
Make OUR code a **1:1 drop-in** for the original at EVERY level — **file → class → methods → variables → globals →
free functions**: nothing the original has that we lack, nothing we have that it lacks. The **drop-in contract**:
our client must run against the ORIGINAL server and vice-versa, byte-identically & compatibly. So:
1. **A missing OR extra symbol = a LOGIC MISMATCH → re-RE it.** `audit.py` matches by NAME, so **our method/class/
   file/variable/global/free-func names must EQUAL the original's** (FGD/save/wire names, the gcc2-demangled names).
   A field/method/var that "doesn't exist" on one side is a red flag to chase, not a rounding error. Match VARIABLE
   names too — a mismatched name can hide that some OTHER code path was reconstructed against the wrong field.
2. **The logic often EXISTS already — moved/renamed/inlined. GREP OUR CODE and MOVE it, don't rewrite.** (This
   session: the sidekick anim, block seqs, gait leg-block, PseudoClimb, and the whole TSHandle cmd-damp were all
   ALREADY present/correct — the bug was elsewhere.)
3. **MIMIC THEIRS.** "Works differently but fine" is WRONG for a drop-in. Copy the mechanism exactly.
4. **The TS decompile is COMPLETE** — `ts_inventory.json` is the machine-checkable inventory of every original
   class/method/member/global/free-func. So a symbol we lack = a REAL gap to RE, not "not decompiled yet."
5. **Discovered a gap mid-fix? FLAG it (a task), FINISH the original issue, THEN report** — don't chase it inline.
6. **ASSET references:** every model/sprite/sequence/sound/event/cvar/user-message a decompile references MUST be
   referenced in our code too (precache + use). Missing asset refs = missing behavior. (This is how the ombra.spr
   shadow + the morfy/superjump_land anims were found missing.) When touching a class, grep its decompile for
   `precache`, `.spr`/`.mdl`/`.wav`/`.sc`, `EMIT_SOUND`, `PLAYBACK_EVENT`, `SetSequence`/anim ids — verify each is wired.
7. **`TODO`/`VERIFY`/`SLICE`/`VERIFY`-tagged comments in code are the standing work-list** — grep them
   (`grep -rn "VERIFY\|TODO\|SLICE\|un-RE" dlls/ cl_dll/ pm_shared/`) and resolve as you touch each area.

### METHODS THAT WORKED THIS SESSION (keep using them — this is how you operate)
- **★ DOSSIER-SUBAGENT HINT (the big win — used 3× this session, kept ~500k decompile tokens out of context).**
  For any deep/uncertain RE, spawn a `general-purpose` agent (Agent tool) with a SELF-CONTAINED prompt: repo root,
  the tool recipes + addr-base rules (`re.sh 0x<nm+0x10000>` server; `dc.sh client|mp 0x<RVA>` Windows), known
  addresses, an explicit work-list, "do NOT edit source", the USER'S EXACT BEHAVIORAL SPEC to reconcile against,
  and "write the decode dossier + raw decompiles to `reference/dossiers/cont<NN>/`". Its final message = the
  summary table you implement from. ⚠ **CROSS-CHECK its labels + verify against the live playtest** — the superjump
  agent's FIRST rev was WRONG (read a DEAD code path); a `SendMessage` follow-up with the user's precise behavior
  got REV 2 right. The user (the running original) is ground truth. [[dead-code-path-gravity-trap]] [[live-original-ts-debuggable]]
- **★ DEBUG-LOG-IN-SOURCE beats gdb for OUR build.** A temporary `ALERT(at_console,"[TAG] ...")` (server,
  `#ifndef CLIENT_DLL`; client `ALERT` == AlertMessage, prints "cl:") — build+deploy, user does the action + pastes
  the console line — gave exact branch/state data in one shot (the `[CCK]`/`[HCC]`/`[SYNC]`/`[ST]` traces isolated
  every kick/anim/state divergence). Remove after.
- **★ LIVE gdb on the ORIGINAL** (`scripts/live.py` on the running `hl.exe`) settles what static RE can't
  (player+0x78 = m_fSequenceFinished; the recoil). ⚠ mp.dll is MSVC → offsets DIFFER from the `.so` (pev@player+4,
  CTSStunt@player+0x4dc). See CLAUDE.md §4b. HALT after arming a watch — tell the user, then STOP.
- **★ Ghidra tooling (full stack):** `scripts/ghidra/re.sh <Class|0xADDR>` (server, typed by ApplyInventory) ·
  `dc.sh client|mp|server 0x<RVA>` (Windows binaries; client takes the RAW RVA) · `re.sh refs/vrefs` (callers) ·
  the **Ghidra MCP** (`mcp__ghidra-mcp__*`, interactive xrefs on the typed `re` project — CLAUDE.md §4a.3; the
  `.lock` blocks headless while the GUI holds it) · `binmap.py` (`.so` name → mp.dll RVA) · `scripts/coverage/*`
  (`audit.py` INTERNAL-structure diff BOTH ways = the parity driver; `coverage.py all` = the EXTERNAL swap contract).

### ⭐ SCRATCHPADS / DOSSIERS — PRESERVED, read them before touching these areas
Committed at **`reference/dossiers/`**: `cont85/` (kick decode + gait/anim state-machine), `cont86/`
(`superjump_moongravity.md`, `slowmo_movement.md`, `gait_and_shadow.md` + `decomp/` raw decompiles). These are the
primary reference to re-derive the hard-won mechanisms. When you spawn a dossier agent, tell it to write to
`reference/dossiers/cont<NN>/` and COMMIT it so the next agent has it.

### ⭐ SUPER-JUMP + SLOW-MOTION reconstructed to the REAL mechanisms (cont.86c, committed + user-verified) — DO NOT re-break
Dossiers `reference/dossiers/cont86/{superjump_moongravity.md, slowmo_movement.md}`.
- **Super-jump = physinfo `"mj"` + a pm_shared jump gate (NOT a lift/hover):** grant sets `"mj"="1"` (ability only,
  gravity normal). `PM_Jump` reads `"mj"` (NOT stock `"slj"`) → DUCK+JUMP one-shot `velocity.z=sqrt(160000)=400`
  + `iuser4|=0x200000`. `Play` → WHEEL_DONE (0x100000) + morfy pose. `PM_CheckParamters` @0xe9c94 = VARIABLE HEIGHT:
  while `iuser4&0x100000`, gate gravity on `IN_JUMP` (held 0.30 rising/0.75 falling, released 1.0) + reduced air
  control. ⚠ `UpdatePowerUpState`'s `m_iPowerups&0x100→gravity=0.15` is **DEAD CODE for superjump** — do not use it.
- **Slow-motion slows TIME not velocity:** `PM_PlayerMove` `frametime *= fuser1`; `pev->velocity` magnitude is
  PRESERVED (so the side-kick `speed>270` gate passes in slowmo). Never scale `maxspeed`/velocity by `fuser1`.

### 🔧 IN-PROGRESS THIS SESSION (uncommitted, deployed, PENDING USER VERIFY) — finish these first
Dossier `reference/dossiers/cont86/gait_and_shadow.md`. Uncommitted files: `cl_dll/StudioModelRenderer.cpp`,
`cl_dll/ts_attachments.cpp` (+`.h`), `cl_dll/tri.cpp`.
- **#14 walk2/run2 gait too fast** — applied Fix 1 (`StudioModelRenderer.cpp:1441` in-place branch `fps*dt` →
  `curstate.framerate*fps*dt`, matching original `StudioProcessGait @0x2250`). ⚠ CAVEAT: our SERVER never sets
  `pev->framerate` by move speed (only `=0` at death), so `curstate.framerate` may be a constant 1.0 → Fix 1 may
  no-op. If the walk is STILL too fast: the real cause is DIVERGENCE 2 (`m_fGaitEstimation=true` @`StudioModelRenderer.cpp:89`
  → origin-delta gait-movement; original lays the velocity path as the flag==0 case — dossier says verify the live
  `g_StudioRenderer+0x1c` flag before flipping), OR the server must set framerate by speed. Playtest-driven.
- **#15 player blob shadow** — added `TS_Shadows_Draw` (`ts_attachments.cpp`, called from `tri.cpp`): `ombra.spr`
  ground blob, `cl_shadow`-gated, trace-down-8192 to ground, `dist=|origin-hit|-12`, skip `>=200`, `alpha=1-dist*0.005`,
  24u-half quad. Orig = client.dll `FUN_10055760` (master) + `FUN_10055500` (per-blob) from `HUD_DrawTransparentTriangles
  @0x34e50`. OPEN Qs to the user: split into master+per-blob to mirror the original shape? blob size 24 tunable?
  NOTE: won't audit as "invented" — client.dll is stripped → the audit "unverifiable (client.dll)" bucket.

### 📋 REMAINING WORK-LIST (recreate as TaskCreate entries)
- **[in_progress] #14 walk2/run2 gait speed** + **[in_progress] #15 player blob shadow** — the two uncommitted
  in-progress items above; verify with the user, then commit.
- **[pending] HUD bottom-left layout** — `ts_status.cpp`: missing TEAM label above health + "X free slots" section
  right of health + our health box too big vs original. Client VGUI → live-RE `client.dll`.
- **[pending] HUD powerup icons** — `ts_powerup_hud.cpp`: too bright (less opaque), and with BOTH powerups shows a
  weapon icon / flips to "numbers" sprite on invnext/prev — want TWO icons (kfu + superjump), same opacity.
- **[pending] Per-class 1:1 parity loop (audit.py driver)** — the STANDING driver: `scripts/coverage/audit.py [Class]`
  + `coverage.py all`. Last headline: **0 missing methods/funcs/members/files · 25 invented methods (all legit helper
  decompositions) · 1 missing global (`TSGameRules` singleton @0x1681a0 — we use stock `g_pGameRules`) · a few
  invented globals**. Drive each TS class to N/N; a missing/extra symbol = re-RE (mission rule #1).
- **[pending] OPEN DECISION — iuser4 back on the wire (`delta.lst`)** — currently reverted to clean upstream
  `3b1d963`; iuser4 not networked → the client can't predict powerup-gated behavior (double-fire server-authoritative
  now works but isn't predicted). The original TS table `19af2487` nets iuser4 at 32 bits (+iuser3/playerclass). Decide.
- **[flagged, not chased] gaps surfaced this session** (rule #5): `m_iSlideStandUp`/`m_fSequenceFinished` reads in
  `Animate`(x13)/`PlayerDeathThink`/`BulletDance` (verified covered); Play ladder/water WHEEL_DONE branch (fixed);
  the `SetPlayerTheOne` gravity (LOW-confidence, untraced); TS normal-jump height 56000 (v≈236.6) vs our stock 268.

### ⚠ RIG STATE CHANGED THIS SESSION (cont.86b)
- **`~/Containers/hlds/mods/ts` is now the CLEAN ORIGINAL** (1127 files, 29 `ts_*` maps). **`tm_avana` is GONE**
  (was a TSHD3 custom map) — **playtest on `ts_dojo`/`ts_lobby`/`ts_metro`**. Launch:
  `cd …/dist/client-x64 && LD_LIBRARY_PATH=. ./xash3d -dev 2 -game ts +deathmatch 1 +maxplayers 8 +map ts_dojo`.
  The old TSHD3+customs (1.2G) is backed up at `~/Containers/hlds/mods/ts-TSHD3-backup-cont86.tar` (restore if needed).
- **`network/delta.lst` reverted to `3b1d963`** (clean halflife-updated base). ⚠ **iuser4 is no longer on the
  wire** → the client can't predict powerup-gated behavior (double-fire is server-authoritative). **OPEN
  DECISION:** put iuser4 back (the original TS table `19af2487` nets it at 32 bits, also nets iuser3/playerclass)
  vs stay clean upstream. `setup-xash-mod.sh` deploys `network/delta.lst` THROUGH the rig symlink into Containers
  (so it clobbers whatever's there — the repo file is the source of truth).
- Full-package finding: the ts-3-final download (Linux tar + Windows Clickteam exe) == our `reference/` byte-for-byte,
  one cross-platform 1127-file set incl. all 4 binaries. cicdec unzipped at scratchpad; wine prefix at
  `<scratchpad>/wineprefix` has the Windows install (cleanable).

### METHODS (unchanged, keep using)
Every commit build-green (`scripts/build.sh amd64 all`) + nm-T-gated.
REGENERATE the baseline first thing: `nm -D build-amd64/dlls/hl.so | awk '$2=="T"{print $3}' | sort >
/tmp/nm_T_baseline.txt`. **Read the `.serena/memories/ledger.md` cont.85 entry (top) via the Read tool.**

⚠ **SERENA GOTCHA (unchanged, still bites):** read the ledger with the **Read tool on
`.serena/memories/ledger.md`** — NEVER `read_memory("ledger")` (Serena MCP), which returns one giant
single-line `{"result":"…"}` JSON (~500 KB) that overflows the read limit. Read the `.md` file directly.

### THE MISSION (the user's directives — the law; unchanged from cont.85, re-affirmed cont.86)
Make OUR code a **1:1 drop-in** for the original at EVERY level — **file → class → methods → variables →
globals → free functions**: nothing the original has that we lack, nothing we have that it lacks. The
**drop-in contract**: our client must run against the ORIGINAL server and vice-versa, byte-identically —
so wire names/formats, event/cvar/command names, and MECHANISMS must match. The user re-stated the five
rules this session; internalize them:
1. **A missing/extra symbol SIGNALS a logic mismatch → re-RE it.** Match by NAME (audit.py matches by name),
   so **our method/class/file/variable/global names must equal the original's** (FGD/save/wire names, the
   gcc2-demangled names). A field/method that "doesn't exist" in one side is a red flag, not a rounding error.
2. **The logic often EXISTS already — moved/renamed/inlined. MOVE it, don't rewrite.** Grep OUR code first
   before reconstructing (this session: the sidekick anim, block sequences, and the gait leg-block were all
   already present/correct — the bug was elsewhere).
3. **MIMIC THEIRS.** When ours "works differently but fine", it is WRONG for a drop-in. Copy the mechanism.
4. **The USER drives all playtests** — give numbered protocols; **never launch xash/xdotool to sim input**.
   And **HALT after you ask them to do something in hl.exe** — they lose focus if you keep running commands;
   set up the capture, tell them, and STOP until they report (this bit us repeatedly this session).
5. **When verbatim code STILL misbehaves, suspect the BASE SDK / ENGINE / CLIENT PREDICTION** (halflife-updated
   diverges from 2003 stock; the local player's view-model + view_ofs are client-predicted, not networked).
6. **The TS decompile is COMPLETE** (`ts_inventory.json` is the machine-checkable inventory of every original
   class/method/member/global/free-func). So a symbol we lack = a real gap to RE, not "not decompiled yet".
7. **Discovered gaps: FLAG, don't ignore — but finish the ORIGINAL issue first, THEN report.** (This session:
   the kick recover dig surfaced the FPV client-prediction bug + the HUD gaps — logged as tasks, not chased mid-fix.)

### ⭐ WHAT cont.85 DID (commits `6b3b3db..d7f74f9` on `restructure-source-parity`)
All build-green, nm-T stable, deployed to the xash rig each time. Detail in the ledger cont.85 entry.
- **CRASH FIX — W_Precache (in `HEAD` from the compaction, verified):** the inherited stock-HL `W_Precache`
  precached `weapon_shotgun` etc., which TS **remaps** onto `CTSGroundWeapon` subclasses (`hldmcompat.cpp`,
  `CBaseEntity`-derived, no `GetItemInfo` vtable slot) → `UTIL_PrecacheOtherWeapon`'s `((CBasePlayerItem*)e)->
  GetItemInfo()` jumped into the typeinfo → SIGSEGV on any map load. Reconstructed `W_Precache` decompile-exact
  (`ts_i386.so @0xe12e8`): only `item_suit` + the TS entities via plain `UTIL_PrecacheOther` + ONE
  `UTIL_PrecacheOtherWeapon("weapon_tsgun")`. (`dlls/weapons.cpp`.)
- **`29bb434` CCombatAttack (kung-fu) rebuilt decompile-exact** (`ts_i386.so @0x76078`; decode dossier
  `reference/dossiers/cont85/kick_decode.md`). Reads IN_ATTACK/IN_ATTACK2 in one fn (dropped `fSecondary`).
  Key: flDamage = the SPEED-SCALED lunge (also the whiff-lunge on a miss), NOT flat 35/45/55 (that's the
  trace REACH); iAnim = the MOVE-ID (0x0a..0x19 == m_lastMeleeAnim, HandleCloseCombat case), not 1/2/3.
- **`8f496cf` CTSAnimation 3rd-person gait** reconciled vs `Animate @0x5b470` (dossier `anim_statemachine.md`):
  the original picks TWO seqs/frame — TORSO (`pev->sequence`) + LEGS (`pev->gaitsequence`) — with a body→legs
  sync for bare hands; fire/reload FALL THROUGH to the gait block (run-and-gun); jump keyed on GRIP.
- **`479122d` animation "snapping" fixed:** the leg block used `SetAnim(TSSA_LEGS)`, and `TSSA_LEGS` (0x01) has
  no INTERP bit, so SetAnim ran `pev->effects |= EF_NOINTERP` EVERY grounded frame → killed model interpolation.
  Original sets `gaitsequence` DIRECTLY (inline, no helper). (User caught my invented `SetGait` helper — inlined it.)
- **`d850ba6`/`dd3f9d3` sawed-off shell-retained reload** (user-approved improvement; use `m_iClip` LIVE not
  `m_pActiveItem->clip` which is a parked mirror) — send event phase 8 when a shell is chambered → block[8].
- **`6ef364b`→`34a94c6` weapon grips = original `szAnimExt`:** m61/knife/katana/sealknife/flag are **empty ""**
  (not `nullptr`→"onehanded"). ⚠ I INVENTED `"m61_grenade"` as the m61 grip first — that string is the thrown-
  grenade ENTITY CLASSNAME (`fn @0x6c850`/`CM61Grenade`), NOT an anim-ext. **Lesson: a `.rodata` string existing
  ≠ it's the field's value; trace the ACTUAL assignment/xref.** [[weapon-table-verify-source]]
- **DATA AUDIT (verified clean):** the **Sparta table** (`Sparta()` @0x6fa78, 32 firearms × spread/viewKick/
  recoil/dmg) matches the original **byte-exact** (hex-float diff, 0 divergences). Write-table numerics
  (clip/firetype/models) are correct — the `extract_weapons.py` numeric columns are UNRELIABLE (wrong offsets;
  verify at the real `Write_<weapon>` store instead). Write vs Sparta are TWO datasets each matching its own original.

### ⭐ UNRESOLVED — the live pickup (in priority order)

1. **THE KICK RECOVER — still flat ~2s (the current in-progress fight; commit `d7f74f9` is the WIP).** The
   duck/side-kick "land low, get up": ours holds the low view (`view_ofs=16 + PRONE + m_flRollTime=2.0`) and
   the getup is `CTSStunt::Play`'s roll-timer (`m_flRollTime < 0.01`). **The ORIGINAL is height/air-time
   dependent** — the roll timer decrements DURING the fall, so a high fall (2nd story) → roll≈0 by landing →
   near-instant getup; standing kick (no fall) → full ~2s; normal jump → in between. **Our `Play()` decrements
   `m_flRollTime` unconditionally every frame from `PreThink` (ts_stunt.cpp:111), so the SERVER should be
   height-dependent — yet in-game it's a FLAT ~2s.** ⇒ **STRONG SUSPECT: the LOCAL player's VIEW is
   CLIENT-PREDICTED, and the getup/roll-decrement doesn't run in the client predictor** (CTSStunt::Play is a
   server PreThink call; the client's predicted view stays low the flat 2s until the server networks it).
   **NEXT: verify whether Play/the getup runs client-side for the local view; if not, that's the fix.** (fuser1
   is confirmed **1.0** live, roll arm confirmed **2.0** in the decompile — so the SERVER math is 2.0s; the flat-
   vs-height difference is the prediction layer.) DO NOT re-fudge the arm to 1.0 (user: "no stopgaps").
2. **SIDEKICK FPV plays the REGULAR kick, not "sidekick".** Confirmed via a debug ALERT: the SERVER's
   CCombatAttack picks B3 (sidekick, `evAnim=8` → melee `animBlock[8]=6=sidekick`) correctly. But the FPV shows
   the regular kick (v_melee seq 5). ⇒ **the CLIENT's PREDICTED CCombatAttack hits a DIFFERENT branch** (B1
   duckkick `evAnim=6` or B4 circlekick `evAnim=9`, both → animBlock→seq 5) — a client-vs-server `onGround`/
   `speed`/`fwdDot` disagreement during the jump. The FPV for the local player rides the `FEV_NOTHOST`
   closecombat.sc event fired during CLIENT prediction. **NEXT: make the client-side branch selection match the
   server's (predicted state parity), OR RE how the original routes the local FPV.** (Mappings all verified
   correct: `animBlock[8]=6=sidekick`, block seqs knife=hold/katana=block/sealknife=block/melee=block_low —
   these were a VERIFY request, confirmed, no change needed.)
3. **HUD color bluish (task #10) — nearly done:** all 3 TS HUD elements draw via `TSHud_Color()` reading
   `hud_r/g/b` (correct — the ORIGINAL config also uses them, 150/175/255). Our xash **config.cfg has stale
   125/150/165** (washed gray) overriding our 150/175/255 default. It's a stale-archive, not a code bug — the
   original's config has 150/175/255. Fix = reset the cfg values or ensure our default.cfg ships the right ones
   (no game/ts/*.cfg sets them, so it's the runtime archive). Low-risk.
4. **HUD bottom-left layout (task #11):** `ts_status.cpp` — missing the TEAM label ABOVE health (lighter/more-
   opaque bg, e.g. "humans"), missing an "X free slots" section to the RIGHT of health, and our health box is a
   giant mostly-empty panel (original's is MUCH smaller). Client-side VGUI → **live-RE the original client.dll**.
5. **`m_iStandUpLatch` getup change (task #9):** commit `13d808d` made Play's getup fire on `m_flRollTime<0.01
   || m_iStandUpLatch` (matching the decompile). Faithful, but re-verify it doesn't make actual dives/slides
   stand up too fast (their landing sets the latch in HandleSlide).

### Task-list entries to recreate (via TaskCreate — the cont.86 tracker)
- `[in_progress]` **"Kick recover — client-prediction of the roll-timer getup (flat 2s bug)"** — #1 above.
- `[pending]` **"Sidekick FPV — client/server CCombatAttack branch parity"** — #2 above.
- `[pending]` **"HUD color bluish (hud_r/g/b stale-cfg)"** — #3.
- `[pending]` **"HUD bottom-left layout (team + free-slots + smaller health)"** — #4.
- `[pending]` **"VERIFY getup-latch vs dives/slides"** — #5.
- `[pending]` **"Superjump: hold-space-only after launch (preserve stock duck+jump)"** — RE the pm_shared/stunt
  jump path; CAREFUL to not break the stock HL duck+jump (box-hopping).
- `[pending]` **"HUD powerup icons — opacity + two-icon (kfu + superjump)"** — `ts_powerup_hud.cpp`; too bright
  (less opaque), and with BOTH powerups it wrongly shows a weapon icon / flips to the "numbers" sprite on
  invnext/prev — want TWO icons, same opacity.
- `[pending]` **"Per-class 1:1 parity loop (audit.py driver)"** — the standing driver; `audit.py [Class]` +
  `coverage.py all`; headline at cont.85 close was **0 missing methods/funcs/members/files · 25 invented methods
  · 1 missing global (TSGameRules singleton @0x1681a0) · 3 invented globals**.

### ⭐ THE METHODS THAT WORKED THIS SESSION (keep using them)
- **★ DEBUG-LOG-IN-SOURCE beats gdb for OUR build.** When live gdb on the running process kept failing on
  attach/timing/offsets, adding a temporary `ALERT(at_console, "[CCK] move=0x%x evAnim=%d ... iuser4=0x%x
  rollT=%.2f\n", …)` (server, `#ifndef CLIENT_DLL`) in the function, build+deploy, and having the USER do the
  action + paste the console line, was FAR more reliable and gave exact branch/state data in one shot. Remove it
  after. **Reach for this FIRST for runtime questions in our build.**
- **★ LIVE gdb on the ORIGINAL (hl.exe/mp.dll) — the MSVC offset gotcha (this cost real time).** `scripts/live.py`
  works (`pid`/`base`/`resolve`/`bt`/`read`/`break`). The traps: (a) **mp.dll is MSVC, `.so` is gcc2 — member
  offsets DIFFER**; pev is at `player+4` in mp.dll (vtable@0, pev@4), NOT `player+0` like the `.so`. (b) The
  CTSStunt EMBED offset differs: `.so` = player+0x4f0, **mp.dll = player+0x4dc** (compute it live: `CTSStunt −
  m_pPlayer` from a Play breakpoint). (c) `entvars_t` offsets: **iuser4=`pev+0x250`, fuser1=`pev+0x254`,
  flags=`pev+0x1a4`, gravity=0x11c, friction=0x120** (my progdefs parser DRIFTS after friction — trust the
  mp.dll decompile's offsets). (d) Verify a read with a KNOWN field first (flags should read `0x208`
  ONGROUND|CLIENT). (e) **Use a conditional breakpoint** (`break *ADDR if <cond>`) so the game stays smooth when
  idle. (f) **`set logging file … / redirect on / on`** so the capture survives the `timeout` kill (stdout is
  lost on SIGTERM). (g) Play @ mp.dll base `0x1e12ffb0` + RVA `0x68440` = `0x1e1983f0` (verify base each session
  — it changes). AND: **HALT after arming — tell the user, then STOP; don't run more commands while they switch
  to hl.exe and do the action** (rule #4).
- **★ THE DOSSIER-SUBAGENT HINT (used twice this session, ~330k tokens kept out of context).** Spawn a
  `general-purpose` subagent (Agent tool) with a SELF-CONTAINED prompt: repo root, the tool recipes incl. the
  addr-base rules (`re.sh 0x<nm+0x10000>` server, `dc.sh client|mp 0x<RVA>`), known addresses, an explicit
  work-list, "do NOT edit source", "decode the DENORMAL floats (N·1.4013e-45 = int N)", and "write the decode
  dossier + every raw decompile to `reference/dossiers/cont85/`". Its final message = the summary table you
  implement from centrally. ⚠ **its LABELS can be WRONG — cross-check** (this session it caught TWO of MY
  "established facts" inverted via objdump, and I still had to re-verify the arm value). Resume via SendMessage.
- **★ SCRATCHPADS ARE PRESERVED at `reference/dossiers/cont85/`** (committed `d7f74f9`): `kick_decode.md`
  (the full CCombatAttack per-branch decode), `anim_statemachine.md` (the gait/SetAnim/blend spec), and the raw
  decompiles (`anim_animate.c`, `anim_setanim_blend.c`, `anim_handleclosecombat.c`, `anim_gaitclosecombat.c`,
  `animate_and_closecombat.raw.c`, `animation_helpers.c`). READ these before touching the kick/gait code.
- **Ghidra tooling (full stack):** `re.sh <Class|0xADDR>` (server, typed by ApplyInventory) · `dc.sh
  client|mp|server 0x<RVA>` (Windows binaries; client takes the RAW RVA, e.g. `0x43200` not `0x10043200`) ·
  `re.sh refs/vrefs` (callers) · **the Ghidra MCP** (`mcp__ghidra-mcp__*`, interactive xrefs on the typed `re`
  project — see CLAUDE.md §4a.3; single-writer `.lock`, don't run re.sh while the GUI holds it) · `live.py`
  (runtime) · `binmap.py <name>` (`.so` name → mp.dll RVA; ⚠ heed the `string?` AMBIGUOUS flag; string-poor fns
  don't match — fall back to a caller) · `scripts/coverage/*` (`audit.py` INTERNAL-structure diff BOTH ways =
  the parity driver; `coverage.py all` EXTERNAL swap contract). **client.dll decompiles TRUNCATE in Ghidra**
  (the big handlers) — read the raw disasm (objdump) for the tail / SendWeaponAnim args.
- **Weapon-table audit method** [[weapon-table-verify-source]]: verify each value at its REAL `Write_<weapon>`
  store (`re.sh`), NOT the `extract_weapons.py` numeric columns (wrong offsets). Sparta (`Sparta()`) = the FINAL
  firearm combat stats; diff hex-floats. Empty grip (`szAnimExt=DAT_00106657=""`) ≠ "onehanded".

---

## ⭐⭐⭐⭐⭐⭐⭐⭐⭐⭐⭐⭐⭐⭐⭐⭐⭐ START HERE (cont.85) — pickup for the next orchestrator (cont.84 = me; available for consult)

**You ARE the orchestrator and the previous agent (cont.84) — continue as if you never stopped.** Branch
`restructure-source-parity` (NOT merged to `main`; GPG signing OFF — keep `git config --local
commit.gpgsign false`). Every commit this session is build-green (`scripts/build.sh amd64 all`) +
nm-T-gated. REGENERATE the baseline first thing (the T-set changed once, intentionally):
`nm -D build-amd64/dlls/hl.so | awk '$2=="T"{print $3}' | sort > /tmp/nm_T_baseline.txt`.

⚠ **SERENA GOTCHA:** read the ledger with the **Read tool on `.serena/memories/ledger.md`** — NEVER
`read_memory("ledger")`, which returns one giant single-line `{"result":"…"}` JSON (~500 KB) that
overflows the read limit. The cont.84 chunks 1–5 (top) carry this whole session's forensics.

### THE MISSION (the user's directives — the law)
Make OUR code a **1:1 drop-in** for the original at EVERY level — **file → class → methods → variables →
globals → free functions**: nothing the original has that we lack, nothing we have that it lacks.
Driver = `scripts/coverage/audit.py [<Class>]` (+ `coverage.py all` for the external contract). It
matches BY NAME, so **names must match the original's** (FGD/save/wire names; the cvar structs were
renamed to the original's .data symbols this session for exactly this reason).
1. **A missing/extra symbol SIGNALS a logic mismatch → re-RE it.** This session alone: the "invented"
   Spectator wire hid a wrong payload; the "missing" gmsg globals were dead-symbol parity; the corpse
   that wouldn't render hid a whole render-fx contract (index-in-rendercolor.r).
2. **The logic often EXISTS already — moved/renamed/inlined. MOVE it, don't rewrite.** Grep OUR code
   first (SendWeaponStatus was UpdateClientData's tail; the menu machinery was dead scaffolding).
3. **MIMIC THEIRS (the drop-in cardinal rule):** our client must swap against the original server and
   vice-versa — wire names/formats byte-exact, MECHANISMS exact (the tkw `type_attach` grammar; the
   dead-player corpse fx; the stock-spectator re-skin — we deleted a whole parallel UI because the
   original PATCHES stock instead of replacing it. When ours "works differently but fine", it is WRONG).
4. **The USER drives all playtests** (numbered protocols; never launch xash yourself — but killing a
   hung xash when asked is fine: `pgrep -x xash3d | xargs kill`). Their live-original knowledge is a
   primary evidence source — this session it decoded the death-cam timing (N-2 countdown), the kick
   spec, the superjump ordering, the scope presentation. ANSWER each report with binary RE before
   coding; when you implement from user spec without RE, SAY SO and verify after.
5. **When verbatim code STILL misbehaves, suspect the BASE SDK / the ENGINE** — this session's harvest:
   the stock prediction viewmodel gate, the stock dead-roll, the stock renderer's renderamt-as-index
   read, xash's unimplemented SPR_DrawGeneric blend args, delta.lst bit-widths (TWICE). See "Tooling
   notes" below.

**HEADLINE:** 0 missing methods/free funcs/members/files · **25 invented methods** · 1 missing global
(**`TSGameRules` the singleton @0x1681a0** — THE remaining refactor) · 3 invented globals
(g_TSGun/g_tsActive/g_tsPool) · server cvars/commands 0 missing · the silencer/scope/laser/spectate/
death surfaces user-confirmed against the live original ("perfect"/"flawless").

### Task-list entries to recreate (via TaskCreate)
- `[in_progress]` **"Per-class 1:1 parity loop (audit→RE→reconcile→build→audit→commit)"** — the driver.
- `[in_progress]` **"PLAYTEST gate before merging restructure-source-parity to main"** — deferred
  confirms: bots (telefrag/kill-feed/spawn-scorer), two-client laser colors (PlInfo), breakable-door
  respawn, the LMS window, TSFrags shatter regression, the death-cam VERIFY constants (80/24).
- `[in_progress]` **"Task #6 INVENTED REVIEW — disposition every audit delta"** (the remainder below).
- `[pending]` **"Round-8 implementation: the kick chain rebuild"** (first — fully decoded, see below).
- `[pending]` **"Round-8 RE: the camera pass (floating reticle + chase cam + death cam constants)"**.
- `[pending]` **"Round-8 RE: viewmodel anim blending"**.
- `[pending]` **"Task #7 stunt/pm tail"** (PM_Jump/PM_Duck diffs, morfy anim, directional death,
  player+0x78 consumer, getup-anim question).
- `[pending]` **"Task #8 VERIFY batch"** (list in the cont.84 section below + ledger; MINUS the
  FLY-animIdx item — the kick rebuild retires it; PLUS: ClientKill's exact gate field pev+0x244 & its
  1 s delayed-suicide consumer · the press-fire prompt's original carrier · WaitThink fade @0x70460
  exactness · the original's dead-body angle mechanism (likely a pm dead gate)).

### THE QUEUE (in order)
1. **The KICK CHAIN rebuild** — fully decoded, implementation-ready: rebuild CTSGun's kick selection
   from `reference/dossiers/cont84/kick_ccombatattack.c` (515 lines, typed). Decoded so far: bVar9 =
   2 + movingBack (speed<=100: IN_BACK held; else velocity·v_forward dot <= -0.5) · STUNT kick
   (iuser4 & 0x50c30) → move 4, dmg 15, refire .55 · DUCK-KICK (ONGROUND + (IN_DUCK||FL_DUCKING)) →
   move 6, dmg 45, refire 1.0, blockstun .35, iuser4|=4|1, view_ofs=(0,0,16) + stunt+0x4 timer 2.0 ·
   BACKKICK (bVar9==3) → move 7, **dmg = speed*0.303 clamp[15,80]** (ours flat 55), refire 1.35
   (1.0 grounded) · **FRONT KICK** (airborne + speed3D>270 — a running jump CLEARS it via the z
   component; ours has identical 3D math so the trigger fires — + IN_DUCK + fwd-dot>0.5) → **move 8**
   (ours sent 0x10 = the wrong anim — THE "no front kick" feel) + **dmg = speed*0.1538 clamp[15,45]**
   + THE VIEW DIP (view_ofs=(0,0,16), timer 2.0) · then the ground combo/fly region (moves 0x3-0x5,
   0x8-0xb) — map the remaining cases from the file. Speed-scaled damages + view dips + iuser4 bits
   throughout; retires the task-#8 FLY-animIdx VERIFY.
2. **dossier-invented remaining folds** (`reference/dossiers/cont84/dossier-invented.md`, read A2/A3/
   B1/B4/C5 + the inv_*.c raws before coding):
   - `TSSendScoreInfo` → DELETE + inline sends; canonical payload = CBasePlayer::AddPoints @0xc9e18
     {BYTE idx, SHORT frags, SHORT deaths, SHORT 0, SHORT teamIdx+1}. ⚠ FIRST find what feeds the
     client scoreboard's WAITING row (playerclass 11/case 0xb) in the original — ours carries it in
     ScoreInfo playerclass; AddPoints writes 0 there.
   - `OwnedCount/SlotsUsed` → DELETE: the original keeps a MAINTAINED counter player+0x548 (TSSpace;
     init 81 in TSInit; weight decrements inline in PickUpWeapon @0x8314c ×clip for stacks; increments
     at drop sites; GetSpeedBySlots @0x66bd4 the only helper). ⚠ drift risk vs the playtested
     knife-stack economy — diff every mutate site.
   - `IsTeamMode` → the gamemode-byte comparisons (rules+0, DecideGamePlay's output; IsCTB @0x67f08 =
     mode 3). OURS carries the lazy ts_objective_manager probe + InitTeams seeding across 24 sites —
     NOT a rename; RE the mode-byte values (0/1/2/3) first, move InitTeams where the original seeds.
   - `TSUpdateClientData` → MOVE into CBasePlayer::UpdateClientData (B1 has the EXACT send order).
   - Team methods (AssignToTeam/GetIndexedTeamName/GetTeamID/IsTeamplay/RecountTeams/
     TeamWithFewestPlayers/PlayerKilled/PlayerSpawn) → the STOCK CHalfLifeMultiplay/Teamplay
     (C2/C3/C5 have addresses + the TS patches) — pairs with #3.
3. **THE TSGameRules SINGLETON SPLIT** (the architecture finding, dossier-invented bottom): the
   original runs stock g_pGameRules (CHalfLifeMultiplay/Teamplay, TS-patched in place) PLUS a separate
   NON-VIRTUAL CTSGameRules singleton (`TSGameRules` global @0x1681a0, json-homed TS_player.cpp). Most
   of our CTSGameRules belongs on the stock classes; the singleton keeps the TS round/objective
   machine. Its own chunk; audit drives.
4. **The camera RE pass** (client.dll): ONE pass covers three user specs — (a) the 3rd-person FLOATING
   reticle = a world-projected aim point (gun-forward trace → WorldToScreen sprite; trails while the
   camera smooths — same projection kit as the laser dot FUN_1003d600 §4b; our TS_PlayerGunAtt cache
   gives the anchor); (b) the head-clearing CHASE-CAM offset; (c) the DEATH-CAM exact distance/lift
   (ours: 80 back/24 up placeholders) + whether it wall-traces. Find the original V_CalcRefdef-analogue
   + its dead-view branch.
5. **Viewmodel anim BLENDING** (round 8): idle↔fire↔reload transitions BLEND in the original, snap in
   ours. RE the original client's vm draw/weaponanim path for the blend window — the viewmodel analogue
   of the cont.81 player blend re-latch.
6. **CTSGun prediction group** (A1/A4): GetWeaponData → the free @0xa00ac shape. ⚠ our fuser2/3 barrel
   clips + m_iWeaponState attach mask are WORKING divergences the original lacks — document, do NOT
   break prediction. CanDeploy/LoadActiveFromItem/SyncActiveToItem/UpdateActivePointers = ACCEPT-or-fold
   (A4: the state is inline in TakeWeapon/SelectWeapon/DropWeapon).
7. **The stock-HL purge**: 19 extra events + 16 extra server commands + the stock weapon sources in
   both builds (hl_weapons.cpp refs g_Glock etc. — trim registrations first) + IsSpawnPointValid +
   sk_* cvar sweep + mp_flashlight/allow_spectators (absent from the original binary).
8. Tasks #7/#8 (above).

### ⭐ THE METHODS THAT WORKED (keep using them)
- **Decompile BEFORE coding, batched**: `scripts/ghidra/re.sh 0x<nm+0x10000> [more]` (server, typed by
  ApplyInventory — the `re` project is CURRENT, do NOT run `re.sh setup`) · `scripts/ghidra/dc.sh
  client|mp 0x<RVA>` (Windows side; client takes the raw RVA form — 0x5455d not 0x1005455d) ·
  `re.sh refs/vrefs` for callers · the **Ghidra MCP** for interactive xrefs (CLAUDE.md §4a.3; single-
  writer .lock — don't run re.sh while the GUI holds `re`).
- **THE DOSSIER-SUBAGENT HINT (used twice this session, ~450k tokens kept out of context)**: spawn a
  `general-purpose` subagent with a SELF-CONTAINED prompt (repo root, tool recipes incl. the addr-base
  rules, known addresses, an explicit work-list with EVIDENCE/VERDICT/CONFIDENCE format, "do NOT edit
  source", "write the dossier + every raw decompile to `reference/dossiers/cont85/`"). It runs in the
  background on the clientdll/mpdll (or re) project lock while you keep editing centrally; its final
  message = the orchestrator summary table. ⚠ its LABELS can be wrong — cross-check against models/
  disasm (two cont.83 guntail labels were swapped; this session's "linked bang = alt sound" was
  corrected to "silenced = iparam2&1"). Resume it via SendMessage if needed.
- **Ground truth beats guessing**: model struct-reads (bodygroups settled the mp5k scope; header
  offsets 204/212!), titles.txt/live-dir checks (the spectator help text), the xash SOURCE tree at
  `~/Projects/goldsrc.net/xash3d-fwgs/engine/` (SPR_DrawGeneric's #if 0), and the USER's live hl.exe
  (`live.py`/`binmap.py`, CLAUDE.md §4b) when static RE dead-ends.
- **User debugging loop**: `GDB=1 ./test.sh` (interactive attach; `NOBUILD=1` preserves a debug build;
  `BUILDTYPE=RelWithDebInfo scripts/build.sh` for line numbers) — the user's two identical-PC SIGINT
  samples + our disasm of the deployed .so nailed the sed-self-capture hang.
- **The per-fix cadence**: user report → RE the exact original mechanism → apply → build both libs →
  nm-T gate → `setup-xash-mod.sh` (ALWAYS — it now syncs delta.lst + generated sprites) → commit with
  provenance (addresses!) → hand a numbered retest.
- ⚠ **Bulk sed renames SELF-CAPTURE** (the TS_EvWeaponAnim tail-recursion freeze): after any bulk
  rename, grep the new helper's body for the replacement.
- ⚠ **delta.lst is a silent truncator** (bit twice): any NEW prediction/clientdata field needs a
  delta.lst entry with enough BITS (m_iWeaponState 2→4; clientdata iuser4 was ABSENT entirely).

## ⭐⭐⭐⭐⭐⭐⭐⭐⭐⭐⭐⭐⭐⭐⭐⭐ START HERE (cont.84) — pickup for the next orchestrator (cont.83 = me; available for consult)

**You ARE the orchestrator and the previous agent (cont.83) — continue as if you never stopped.** Branch
`restructure-source-parity` (NOT merged to `main`; GPG signing OFF — keep `git config --local commit.gpgsign
false`). ~25 commits this session (`f67c6eb..`), every one build-green (`scripts/build.sh amd64 all`) +
nm-T-gated — REGENERATE the baseline first thing:
`nm -D build-amd64/dlls/hl.so | awk '$2=="T"{print $3}' | sort > /tmp/nm_T_baseline.txt`.

⚠ **SERENA GOTCHA:** read the ledger with the **Read tool on `.serena/memories/ledger.md`** (never
`read_memory("ledger")` — one giant single-line `{"result":"…"}` JSON that overflows the read limit).

### THE MISSION (the user's directives — the law; the cont.79 section far below has the long form)
Make OUR code a **1:1 drop-in** for the original at EVERY level — **file → class → methods → variables →
globals → free functions**: nothing the original has that we lack, nothing we have that it lacks. Driver =
`scripts/coverage/audit.py [<Class>]`; it matches BY NAME, so **names must match the original's**
(FGD/save/wire names) — a faithful-but-misnamed symbol reads as missing+invented.
1. **A missing/extra symbol SIGNALS a logic mismatch → re-RE it.** Paid off the WHOLE session: the 3
   missing CTSGun methods were hiding the entire base-ItemPostFrame frame machine; the "missing"
   MultiplyClips branch was hiding the knife-stack slot economy; the invented ammo grant was hiding the
   fill-to-cap BuyAmmo (the user's 18/58-vs-18/120 catch); the mislabeled ScaleSpread arg was hiding the
   SILENCER accuracy/damage system.
2. **The logic often EXISTS already — moved/renamed/inlined — MOVE it, don't rewrite.** Grep OUR code
   first (TSFireBulletsPenetrate WAS ShootBulletsPlayer; TSPickUpWeapon WAS PickUpWeapon; the invented
   m_flSpawnGateTime was the original's +0x770).
3. **MIMIC THEIRS (the drop-in cardinal rule):** our client must swap against the original server and
   vice-versa — wire names/formats byte-exact (bparam2 = the FireBullets SEED, not our side extension;
   the reload.sc fparam1 phases; +alt2 is the IN_CANCEL BUTTON, not a command), mechanisms exact.
4. **The USER drives all playtests** (numbered protocols; never launch xash yourself) and their live-game
   knowledge caught ~15 real divergences this session — but ANSWER each report with binary RE before
   coding (the round-6 akimbo model: user-specified, then VERIFIED against AkimboFire @0x729c0, which
   confirmed it AND added 5 corrections). When you implement from user spec without RE, SAY SO and verify
   after.
5. **When verbatim code STILL misbehaves, suspect the BASE SDK** ([[base-sdk-divergence-check]]) and the
   gcc2 world (calloc'd entity data = absent keyvalues are 0). And **presentation disputes are settled by
   the MODELS** (sequence names/embedded events/bone names/attachments — python struct reads).

**HEADLINE:** 0 missing/0 invented classes · 0 missing methods · **32 invented methods** · 0 missing
members · 0 missing free funcs · 0 file mismatches · **6 missing globals · 3 invented globals** ·
**12/12 TS events wired (19 extra = stock-HL leftovers)** · server commands 0 missing/16 extra.

### ⭐ WHAT cont.83 DID (detail: the ledger cont.83 entry — LONG; the dossier README indexes the decompiles)
- **0 missing methods**: the gun FRAME MACHINE (the TS-rewritten BASE CBasePlayerWeapon::ItemPostFrame in
  weapons_shared.cpp replaces our invented CTSGun override; EmptySound/StopReload/ReloadWeapon; Reload/
  ShotgunReload/WeaponIdle/Fire/MultiplyClips/ColdCock decompile-exact; +alt2 = IN_CANCEL predicted via
  fuser4; m_burstOrPhase + pumpToggle member merges) · ShootBulletsPlayer (its own TS_combat.cpp TU;
  polar cubic spread, ONE MultiDamage batch/volley, depth-1 pierce, flFactor2 decay, REALBULLET gate) ·
  TSInit (Spawn's first call; SPAWN PROTECTION + map-global superjump + 0xff cache force-sends) ·
  PickUpWeapon (all five call-site contracts; the knife-stack merge; no-switch-on-recovery; TakeWeapon
  reshape — arg3 = ATTACH MASK, no ammo grant).
- **0 missing free funcs**: SetPlayerRespawnTime (m_flSpawnGateTime MERGED into m_flTSRespawnTime;
  +0x7ba/+0x7bc; clamp floor 5) · RemoveEntityFromSlowedArray + m_pSlowedEnts[32]/+0x288 + 8 scrub sites ·
  TSPlayerDisconnect (vote-rejoin + The-One leak + CallCheckDeads) · PseudoClimb (TSHandle head, both
  libs) · TSGetPointsForFrag + glast_weaponid (init -1, captured at fire) · TSSelectSpawnSpot +
  EntSelectSpawnPoint replaced (bands + crowd telefrag).
- **12/12 EVENTS**: coldcock/closecombat swapped (CCombatBlock four-gate; CCombatAttack per-move args by
  disasm slot-tracking; the 3-jab vm cycling was invented) + the five-handler subagent dossier applied
  (empty = pistol-empty.wav; reload fparam1 IS the block index + the brass theatre; pump = gun+0x96a with
  the ENT-1 no-channel-cut rack — mossberg racks with m3-pump.wav; grenade = in-eye-only; firelinked =
  gun+0x96e both-barrels). ALL presentation stand-ins retired. The FULL 29-short anim blocks (both
  stances, movl-PAIR-aware) + pszAltSound(+0x306 triple-duty)/pszShellModel(+0xe8)/iShellSetback(+0x34c)
  columns landed.
- **The AMMO ECONOMY to the binary** (user-caught): NO TakeWeapon grant; BuyAmmo = FILL-TO-CAP per mag;
  iPellets(+0x24) = the stackable CARRY CAP (knife 5/seal 10); +1 stack merges; BuyArmament loops the
  queued count (2 identical pistols = the akimbo combine); the ground weapon tops only an EMPTY reserve.
- **THE ITALIAN-BONE GAIT SPLIT** (the big renderer find): TS player skeletons name the spine "Bip01
  Dorso" — the stock "Bip01 Spine" gait split matched nothing, so the run gait owned the whole skeleton
  and NO fire/aim torso showed while moving. The original client.dll ships BOTH strings. + the
  directional slide-fire (slide_<dir>_<grip>_shoot) re-enabled (+0x51c = m_bDirForward, not dead).
- **The AKIMBO saga (user-driven, VERIFY-RE'd, closed "perfect")**: free modes = per-barrel triggers
  (left click = LEFT barrel via item+0x24; each barrel drains ITS OWN clip + dry-clicks); plain SEMI
  alternates; plain AUTO = both at once; both-buttons-in-free = LINKED; the hold stance for akimbos
  (v_berettas 14/15, v_mk23_akimbo 11/12); the REAL barrel clips ride weapon_data fuser2/3 (the balanced
  SetWeaponData reconstruction was feeding prediction half-full barrels); the anim sides + attachment
  pairs proven by the MODELS ([22/23]=RIGHT [24/25]=LEFT; attachments 0/1 right 2/3 left); the flare =
  the fire events ALWAYS + the model-event 5001-family SUPPRESSED in HUD_StudioEvent (round 6d/6e: defer
  to R_MuzzleFlash and the visible flare vanishes — it's the faint engine flash). ScaleSpread's 4th arg =
  SILENCED (sil tightens x0.875/x0.775 + costs x0.775 damage; recoil x5 moving >175 u/s, x2 linked).
- **SPARTA tripwires** in-code everywhere (struct/table/Sparta()/extractor/CustomWP = the runtime THIRD
  stage) + default_fov 80 (and the two stock 90-floor clamps removed).

### ⭐ WHERE TO GO (the queue — recreate these as the task list via TaskCreate)
- `[in_progress]` **"Per-class 1:1 parity loop (audit→RE→reconcile→build→audit→commit)"** — the driver.
- `[in_progress]` **"PLAYTEST gate before merging restructure-source-parity to main"** — rounds 1-6 all
  PASSED (user: "perfect"); the still-unplaytested surface: the reload/pump/dry-click/firelinked event
  presentation (round 7 protocol: dry click, 3rd-person reload brass, pump rack no-cutoff, linked bang,
  silenced spread/damage feel, the x5 moving bloom), spawn protection, the spawn scorer + telefrag,
  PseudoClimb, the stylish-kill feed, the 5s respawn floor.
- `[pending]` **"Task #6 INVENTED REVIEW"** — the audit's remaining deltas, EVERY one dispositioned
  (prove-inlined-and-MOVE / accept-as-infrastructure-with-a-comment / DELETE):
  · 32 invented methods: CTSGun CanDeploy/GetWeaponData/SetWeaponData/LoadActiveFromItem/SyncActiveToItem/
    UpdateActivePointers/OwnedCount/SlotsUsed/SendWeaponStatus/SwitchBarrel(NOW-UNCALLED — delete?) ·
    CBasePlayer TSCloseMenu/TSEnterSpectate/TSMenuSelect/TSSendBuyMenu/TSSendScoreInfo/TSSpectateThink/
    TSTryRespawn/TSUpdateClientData · CTSGameRules AssignToTeam/GetIndexedTeamName/GetTeamID/IsTeamMode/
    IsTeamplay/PlayerKilled/PlayerSpawn/RecountTeams/SendRoundTime/StartRoundTimer/TeamWithFewestPlayers ·
    CBaseDoor::RespawnThink (ride the info_breakable helper) · CBaseEntity::MyTogglePointer ·
    CBasePlayerWeapon::CallDoRetireWeapon. For each: find the original's home (re.sh/dc.sh + vrefs) — the
    logic is probably THEIRS under a different name/site.
  · 6 missing globals: **TSGameRules the INSTANCE @0x1681a0** (the original's rules object is a GLOBAL,
    not heap — dossier-freefuncs §0; big refactor, plan it) · gmsgFlashlight/gmsgFlashBattery/gmsgSetCurWeap
    (registration-parity: the original REGISTERS them — find where; ours deleted the regs in U0, re-add
    send-less) · objects · roundtime (TS_gamerulesclass.cpp — likely the round clock global).
  · 3 invented globals: g_TSGun/g_tsActive (client infra — accept+comment?) · g_tsPool (review).
  · 19 EXTRA events + 16 EXTRA server commands + the stock-HL weapon sources still in both builds —
    the removal pass (careful: hl_weapons.cpp references g_Glock etc.; trim the registrations first).
  · The "Spectator" WIRE reconcile (ours = state+seconds; the original = the stock (idx,flag) broadcast —
    TSInit @0x92a4c sends it; our spectate-wait HUD needs a different carrier).
  · IsSpawnPointValid now uncalled (stock — leave or trim).
- `[pending]` **"Task #7 stunt/pm tail"**: PM_Jump/PM_Duck diffs vs the original, morfy anim, directional
  death, the player+0x78 consumer, the getup-anim divergence question FOR THE USER. (PseudoClimb DONE.)
- `[pending]` **"Task #8 VERIFY batch"** (grown): TraceAttack's spawn-grace refusal + the 0x10c10 stunt
  dmg mult (@0x801eb) · PlayerKilled's kill-TIER/slow-mo-reward tail (@0x7a908.. — m_iAward 2/3/4 with
  The One's 10 preserved + the reward grant off the out-bools/DoSpectacularNess — needs a typed pass) ·
  the V11 LMS countdown HUD msg (PreThink +0x754 throttle) · TSItem::hOwner consumer (sell-zone?) ·
  ResetRestartRound still reads the dead "lms" cvar · burstOrPhase/pump prediction packing · the
  FLY-family closecombat animIdx (register-carried — disasm slot-track it) · gold/skorpion_akimbo's
  EMPTY wi+0x306 in the linked bang (our fallback = the normal fire) · the reload.sc phase-0 brass
  R_TempModel exact args · gun+0xa0c cook-timer consumer · CBreakable+0x110 shard factor · the SrvSett
  client consumers · TSFade slow-mo-tint coexistence · L13 katana pickup · the +0x94/98/9c CTSKnife triple.

### ⭐ THE METHODS THAT WORKED (keep using them — full recipes in reference/dossiers/cont83/README.md)
- **dc.sh batch typed decompiles BEFORE coding** (multiple addrs per run; the `re` project stayed current
  all session — no setup needed). **Disasm caller/arg push-sequence decode** when the decompiler garbles
  stack slots (resolved every call-site contract this session). **re.sh vrefs** for virtual callers.
- **The Write_ column extraction** — movl-PAIR-aware (a movl at a short-array offset = TWO shorts; the
  single-value regex hid half the anim blocks). ⚠ SPARTA: +0x1c/+0x28/+0x30/+0x34 in Write_ bodies are
  the DEAD stage. ⚠ loose lea-pairing BLEEDS — adjudicate strict-run agreement or hand-read.
- **MODEL dumps settle presentation disputes** (python struct reads of .mdl): sequence NAMES, EMBEDDED
  EVENTS (the 5001-family), BONE names, attachment pairings. This closed the gait split, the anim sides,
  the flare doubling, and the akimbo hold sets.
- **The dossier-SUBAGENT template** (kept ~195k tokens of client.dll RE out of my context — the gun-tail
  dossier): spawn a `general-purpose` subagent on the SEPARATE clientdll/mpdll project lock while you
  edit source centrally. Template in the cont.81 section below (adapt the target list); have it WRITE the
  dossier + raw captures into `reference/dossiers/cont84/` so they survive the session. It's resumable
  via SendMessage. ⚠ its LABELS can be wrong — cross-check against the models/disasm before trusting
  side/index mappings (two guntail labels were swapped).
- **User-driven playtest loop**: they report → you RE the exact original mechanism → apply → commit with
  provenance. **Ghidra MCP** for interactive xrefs when the GUI holds `re` (CLAUDE.md §4a.3);
  **live.py/binmap.py** on the always-running original hl.exe when static RE dead-ends (§4b).
- ⚠ python brace/anchor edits: anchor on FULL unique text, never rfind("/*") (it clobbered player.cpp
  once; git checkout saved it). ⚠ scratchpads are SESSION-LOCAL: copy anything durable into
  `reference/dossiers/cont84/` before closing.

## ⭐⭐⭐⭐⭐⭐⭐⭐⭐⭐⭐⭐⭐⭐⭐ START HERE (cont.83) — pickup for the next orchestrator (cont.82 = me; available for consult)

**You ARE the orchestrator and the previous agent (cont.82) — continue as if you never stopped.** Branch
`restructure-source-parity` (NOT merged to `main`; GPG signing OFF — keep `git config --local commit.gpgsign
false`). ~30 commits this session (`d80d29a..`), every one builds `scripts/build.sh amd64 all` clean + gated on
`nm -D` T-export identity — REGENERATE the baseline first thing:
`nm -D build-amd64/dlls/hl.so | awk '$2=="T"{print $3}' | sort > /tmp/nm_T_baseline.txt`.

⚠ **SERENA ONE-LINE-JSON GOTCHA:** read the ledger with the **Read tool on `.serena/memories/ledger.md`**, NOT
`read_memory("ledger")` — the MCP returns one giant single-line `{"result":"…"}` (~400 KB) that overflows the
read limit. Same for any large memory.

### THE MISSION + the user's directives (the law — the cont.79 section far below has the long form)
Make OUR code a **1:1 drop-in** for the original at EVERY level — **file → class → methods → variables →
globals → free functions**. Driver = `scripts/coverage/audit.py [<Class>]`. Current headline:
**6 missing methods · 33 invented methods · 6 missing free funcs · 9 missing globals · 3 invented globals ·
0 file mismatches · 0 missing files · 0 class/member gaps.** The exact remaining lists:
- **Missing methods (6)**: CTSGun EmptySound/ReloadWeapon/StopReload · CBasePlayer PickUpWeapon/
  ShootBulletsPlayer/TSInit. Most are MOVES: our PlayEmptySound/Reload-tail/pump-interrupt/
  TSPickUpWeapon/TSFireBulletsPenetrate/the-spawn-reset-block already hold the logic — rename/reshape
  onto the original names+signatures (grep OUR code first, always).
- **Missing free funcs (6)**: SetPlayerRespawnTime (completes the TSPAward 1/11 producers!),
  TSSelectSpawnSpot, TSGetPointsForFrag, TSPlayerDisconnect (the vote-rejoin fix),
  RemoveEntityFromSlowedArray (+ the packed slowed array player+0x208[32]/+0x288 model — completes
  CTSKnife::Use), PseudoClimb (pm). ALL have condensed facts in `dossier-freefuncs.md` §2 + ff_*.c decompiles.
- **Missing globals (9) / invented (3)**: see AUDIT.md GLOBALS sections (TSGameRules the global instance
  @0x1681a0 among them — task #5 scope).
1. **Names must match the original's** (FGD/save/wire names) — audit matches BY NAME; a faithful-but-misnamed
   symbol reads as missing+invented.
2. **A missing/extra symbol SIGNALS a logic mismatch → re-RE it.** Paid off ALL session: the "wrong-column
   kAmmoMax" was hiding the whole ammo-cap system; the missing GrenadeIdle was hiding the entire grenade
   COOK machine; the missing DecideGamePlay was hiding the rules+0 gamemode byte the Wires system gates on.
3. **The logic often EXISTS already — moved/renamed/inlined — so MOVE it, don't rewrite.** This session:
   SecondaryAttack was inlined in ItemPostFrame; HolsterWeapon was inlined in SelectWeapon; the knife
   settle machinery existed under invented names. Grep OUR code first.
4. **MIMIC THEIRS (the drop-in cardinal rule):** our client must swap against the original server and
   vice-versa — wire names/formats byte-exact, mechanisms exact (knife.sc's format swap, the CustomWP
   ASCII line, PTakeDam's encoding).
5. **Don't guess vs the RE; the USER drives all playtests** (numbered protocols; never launch xash yourself).
   The user's live-original knowledge OUTRANKS a static read twice removed — but get the binary evidence
   first: this session BOTH binaries said dropped guns don't glow, the user's live game says they do →
   implemented user-directed + documented at the site (ts_worldgun.cpp). When they conflict, say so, tag it.
6. **When verbatim code STILL misbehaves, suspect the BASE SDK** (halflife-updated ≠ 2003 stock) — see
   [[base-sdk-divergence-check]]. AND the gcc2 world: **entity data is CALLOC'D — absent keyvalues = 0**
   (the breakable-respawn default bug: our -1 default vs the original's implicit 0 = 30s respawn).

### ⭐ WHAT cont.82 DID (full detail: ledger cont.82 entries, top of `.serena/memories/ledger.md`)
- **Melee S7a/b/c + S8 + S10 + HolsterWeapon** — the whole blade split, BOTH sides (SecondaryAttack/
  KatanaThrow/KatanaFire/KnifeFire decompile-exact; CTSKnife lifecycle re-transcribed — embed =
  SOLID_NOT+MOVETYPE_FLY, in-flight clears <700 u/s, katana +USE gated on KungFu (L13 playtest flag);
  the client EV_TSKnife/EV_TSKatana against the five-handler dossier).
- **ALL gmsg units U0–U5**: registration parity (StatusText/StatusValue/Weapons/Flashlight/FlashBat
  deleted, PLAYTEST-GATED), TSState/TSSpace/TSBTime, TSPAward (+partial producers), PlInfo, PTakeDam
  (stock Damage send removed), SrvSett (+the weaponrestriction command), Wires (+CheckForWires + the
  gamemode byte), TSFade (slow-stretched, engine-exact client parse), CustomWP (+iPrice + live buy prices
  + loadweapons/saveweapons).
- **Freefuncs F1–F4 + most F5**: the dead batch + m61_grenade factory; the s_ammo_info table (TWO live
  bug fixes: reserve caps + slow-mo bullet speeds); ★ the range-damage falloff WIRED (both bullet paths +
  UTIL_TraceLine_Expanded); the restriction system; DecideGamePlay + the mode globals; GetRandomPosition/
  PlayerIsCheating/TSCountPlayers/GetBodyByItems(+Deploy body); TSPrecache (kills every late-precache).
- **The USER-DRIVEN live-playtest loop** (the second half of the session — KEEP THIS WORKING MODE):
  powerup fixes (EF_MUZZLEFLASH not DIMLIGHT; origin-at-floor bbox), the M61 GRENADE COOK MACHINE
  (GrenadeFire = pin only, GrenadeIdle = throw-on-release, fuse 3.0 FROM THE THROW, underhand roll,
  holster force-throw) + the TS explosion (TSCreateDebris material-1 + client explo/explode1-4 pitched
  100+300*slow) + the ~2% DoSpectacularNess slow-mo bubble, the m61 type-code fix (iKnifeType=4),
  drop glow on ALL guns (user-directed divergence — both binaries gate it to the briefcase; documented),
  blast glass shards scale with the killing damage (VERIFY the +0x110 factor), and the
  BREAKABLE-RESPAWN DEFAULT fix (0 not -1 → every unmarked breakable/door respawns in 30s).

### ⭐ WHERE TO GO (the queue; the RE for most of it is DONE — dossiers/decompiles first)
**Typed decompiles: `reference/dossiers/cont81/` (the big dossiers + ev_* client handlers) and
`reference/dossiers/cont82/` (this session's — see its README, incl. the Write_* column-extraction
one-liner pattern). Do NOT re-decompile what's there.**
1. **AWAIT the user's playtest** of the round-2 fixes (drop glow, explosion pitch, blast shards,
   breakable respawn, m61 flight, grenade.sc precache after a fresh setup-xash-mod.sh run of BOTH libs).
   Fold results in first — the user feeds corrections live; answer each with binary RE before coding.
2. **The 6 missing methods** (mostly moves — see the lists above). ShootBulletsPlayer = rename/reshape
   TSFireBulletsPenetrate onto CBasePlayer (it IS the original's method); TSInit absorbs the spawn-reset
   block (player.cpp ~3136 comment names it); PickUpWeapon absorbs TSPickUpWeapon (mind the knife-stack
   merge gap noted in S8); EmptySound/StopReload/ReloadWeapon = CTSGun tail (grep the inline logic).
3. **The 6 missing free funcs** (facts in dossier-freefuncs §2): SetPlayerRespawnTime FIRST (it completes
   the TSPAward out-of-round/kill-tier producers + the +0x770 semantics TSCountPlayers already reads).
4. **Task #8 the gun-event tail**: empty/pump/firelinked/grenade/closecombat/coldcock.sc client handlers —
   closecombat/coldcock are FULLY RE'd in dossier-ev-melee.md; the blocker is extracting each GUN's anim
   block [28] (+ the kungfu block indices) from the Write_* disasm (the README one-liner).
5. **Task #5 invented review** (33 methods incl. SwitchBarrel now-uncalled, CBaseDoor::RespawnThink →
   should ride the info_breakable helper; 9 missing globals incl. TSGameRules-the-instance) and
   **task #6 stunt/pm tail** (PseudoClimb, PM_Jump/PM_Duck diffs, morfy anim, directional death,
   +0x78 consumer, the getup-anim divergence question for the user).
6. **VERIFY ledger items**: gun+0xa0c cook-timer consumer (reads @0xbfe54/0xcb09d), CBreakable+0x110
   shard factor, the SrvSett client consumers (cash-HUD hide/buy filter), the TSFade slow-mo-tint
   coexistence, L13 katana pickup, the +0x94/98/9c CTSKnife triple.

### ⭐ THE METHODS THAT WORKED (keep using them)
- **dc.sh/re.sh typed decompiles BEFORE coding** — every unit this session started with a fresh decompile
  (`scripts/ghidra/dc.sh server 0x<nm-addr>`; re project single-writer; clientdll/mpdll separate locks).
  Raw-disasm cross-check via `recovered/scratch-cont81/ts.dis` (regenerate: `objdump -d reference/ts_i386.so`)
  — jumptables/FPU consts the decompiler mangles are hand-decoded from there (DecideGamePlay's mapping,
  the OnGround bbox floats via `dd`+struct.unpack at GOT-relative offsets).
- **The dossier-subagent template** (kept ~200k tokens of client.dll RE out of my context this session —
  the five-handler dossier). Template in the cont.81 section below; clientdll project = a separate lock,
  safe to run while you edit source.
- **The Write_* column extraction** (cont82 README) for any missing s_weapon_info column.
- **User-driven playtest loop**: they report → you RE the exact original mechanism → apply → commit with
  the provenance. Their game knowledge caught 6 real divergences this session.
- **Ghidra MCP** for interactive xrefs when the GUI holds the `re` project (CLAUDE.md §4a.3);
  **live.py/binmap.py** on the running original when static RE dead-ends (CLAUDE.md §4b).

### Task-list entries to recreate (via TaskCreate) — the cont.83 tracker
- `[in_progress]` **"Per-class 1:1 parity loop (audit→RE→reconcile→build→audit→commit)"** — the driver.
- `[in_progress]` **"PLAYTEST gate before merging restructure-source-parity to main"** — round-2 fixes +
  the whole gmsg/gunfeel/blade surface still need the user's pass.
- `[pending]` **"6 missing methods (mostly MOVES): EmptySound/ReloadWeapon/StopReload +
  PickUpWeapon/ShootBulletsPlayer/TSInit"**.
- `[pending]` **"6 missing free funcs: SetPlayerRespawnTime (TSPAward producers) first, then
  TSSelectSpawnSpot/TSGetPointsForFrag/TSPlayerDisconnect/RemoveEntityFromSlowedArray(+packed array)/
  PseudoClimb"**.
- `[pending]` **"Gun-event tail (#8): empty/pump/firelinked/grenade + closecombat/coldcock.sc swaps —
  per-gun anim block extraction first"**.
- `[pending]` **"Invented review (#5): 33 methods + 9 missing globals + TSGameRules instance"**.
- `[pending]` **"Stunt/pm tail (#6)"**.

---

## ⭐⭐⭐⭐⭐⭐⭐⭐⭐⭐⭐⭐⭐⭐ START HERE (cont.82) — pickup for the next orchestrator (cont.81 = me; available for consult)

**You ARE the orchestrator and the previous agent (cont.81) — continue as if you never stopped.** Branch
`restructure-source-parity` (NOT merged to `main`; GPG signing OFF — keep `git config --local commit.gpgsign
false`). 55 commits this session (`05be983..`), every one builds `scripts/build.sh amd64 all` clean + gated on
`nm -D` T-export identity (baseline `/tmp/nm_T_baseline.txt` — regenerate it first thing:
`nm -D build-amd64/dlls/hl.so | awk '$2=="T"{print $3}' | sort > /tmp/nm_T_baseline.txt`).

⚠ **SERENA ONE-LINE-JSON GOTCHA:** read the ledger with the **Read tool on `.serena/memories/ledger.md`**, NOT
`read_memory("ledger")` — the MCP returns one giant single-line `{"result":"…"}` (~350 KB) that overflows the
read limit. Same for any large memory.

### THE MISSION + the user's directives (the law — reread the cont.79 section far below for the long form)
Make OUR code a **1:1 drop-in** for the original at EVERY level — **file → class → methods → variables →
globals → free functions**. Driver = `scripts/coverage/audit.py [<Class>]`. Current headline:
**13 missing methods · 41 invented methods · 39 missing free funcs · 17 missing globals · 0 file mismatches ·
0 missing files · 0 class/member gaps.**
1. **Names must match the original's** (FGD/save/wire names) — audit matches BY NAME; a faithful-but-misnamed
   symbol reads as missing+invented.
2. **A missing/extra symbol SIGNALS a logic mismatch → re-RE it.** This paid off relentlessly: "never-written
   player+0x510" was really m_vecStuntDir through the EMBEDDED CTSStunt@+0x4f0 (memory
   [[embedded-object-offset-blindness]]); the "dead" +0x51c was m_bDirForward; the missing HolsterWeapon
   decompile revealed the whiff-clear-on-switch.
3. **The logic often EXISTS already — moved/renamed/inlined — so MOVE it, don't rewrite.** Grep our code first.
4. **MIMIC THEIRS (the drop-in cardinal rule):** our client must swap against the original server and
   vice-versa. Wire names byte-exact (the "TSFlag"-not-"TSHaveFlag" shipped bug), mechanisms exact.
5. **Don't invent vs the RE.** When static RE dead-ends: a 5-line runtime PROBE the user drives beats hours of
   decompile re-reads (this session's blend + friction root causes both fell to user-run probes). The user
   DRIVES all playtests — hand them a NUMBERED protocol; never launch xash/xdotool yourself (memory
   [[user-drives-playtests]]).
6. **⭐ NEW (cont.81): when verbatim code STILL misbehaves, suspect the BASE SDK** — halflife-updated tracks the
   HL ANNIVERSARY SDK, not the 2003 stock the original targets (user: expected). Three hits this session, worst:
   pm_shared dropped the stock `!pmove->multiplayer` gate on the per-move friction reset, silently erasing EVERY
   stunt friction. Memory [[base-sdk-divergence-check]]: probe the value at the consumption point, then diff
   base-vs-stock-vs-original.

### ⭐ WHAT cont.81 DID (full detail: ledger cont.81 entries, top of `.serena/memories/ledger.md`)
- **File-consolidation plan C1–C20 FULLY APPLIED** — 22→0 file mismatches, 2→0 missing files; the tree now
  mirrors the original TU layout (TS_entities.cpp, TS_objectives.cpp, TS_gamerulesclass.cpp + the free-fn
  TS_gamerules.cpp scaffold, ts_bullet/ts_worldgun/ts_grenades/ts_emitter merges, pm_shared/TS_pm_shared.{h,cpp}
  both libs, audit.py ACCEPTED_CLIENT_FILES for the 2 unknowable client.dll rows).
- **Melee dossier S0–S6 + S9 APPLIED** (19→13 missing methods): UpdateMeleeBar (+ the RE-proven PreThink regen
  15/s + block-window value decay), BlockPlayer, CheckHull + FindHullIntersectionplayer + the Victim*Hold trio,
  the CloseCombatAttack/ColdCockAttack twins, the DropWeapon(-2) disarm-pressure accumulator (player+0x7b4),
  the CCombatAttack/CloseAttack rewire + **the GoWheel dir==3 BACKKICK combo (wired)**, the ColdCock wrapper +
  kill-feed flavor ("Kung Fu"/"<weapon> Stock" + #TS_Coldcock), CCombatIdle. The **"TSFlag"(4)** wire-name fix.
- **The ENTIRE stunt/prone/movement chapter, user-validated "PERFECT"**: directional poses off m_vecStuntDir
  (wheel_b standing, lowwheel_f/l/r, roll_l/r/f/b), GetDirection VERBATIM + normalized callers, HandleWheel as a
  LANDING handler, the prone LYING system (case 3 slide_<dir>_<grip> + aim-tracking BlendAnimationsForSlide +
  the strafe-roll direction re-anchor + case 0x29 = the slide_*_end getup family), HandleSlide VERBATIM
  (+player+0x78 m_iStandUpLatch writer found), the gravity/friction map CORRECTED (gravity=0x11c,
  friction=0x120 — offsetof-proven; cont.80 had it inverted), and the **pm phase-2 deltas**: PM_Friction
  (drop x0.75, edgefriction only at friction 1.0), PM_FlyMove (backoff 1+0.01*(1-friction) — no stunt
  ricochet), PM_WalkMove (backpedal x0.8), and THE ROOT CAUSE fix (the !multiplayer friction-reset gate).
  Blend-snap fixed via a client re-latch shim (xash's 1ms CL_CompareTimestamps quantization vs GoldSrc's
  100fps cadence). Stamina HUD wired to the WeaponInfo melee byte (regen/respawn refill visible).

### ⭐ WHERE TO GO (the queue, in order — the RE for ALL of it is DONE; only application remains)
**The dossiers + plans + typed decompiles are COMMITTED at `reference/dossiers/cont81/` (see its README —
regenerate anything else via re.sh).** Do NOT redo the RE fan-out; apply:
1. ✅ **DONE (cont.82, d80d29a..): Melee S7a/b/c + S8 + S10 + HolsterWeapon** — the blade split landed BOTH
   sides (headline 13→8 missing · 41→33 invented; CTSKnife 9/9; ledger cont.82 entry has the detail). The
   client handler contract for ALL FIVE melee/draw events is now RE'd at `dossier-ev-melee.md` (+ev_*.c).
   LEFT OVER: the closecombat.sc/coldcock.sc event swaps (handlers RE'd; need a bulk Write_* extraction of
   the per-GUN anim block [28] first) + CTSGun EmptySound/GrenadeIdle/ReloadWeapon/StopReload.
2. **gmsg (PARTIAL — cont.82)**: ✅ U0 registration parity (b57fe1c) · ✅ U1 TSState/TSSpace/TSBTime
   (96a7e68) · ✅ U1d TSPAward (8be3cda) · ✅ U2 PlInfo (693c5c5) · ✅ U4a PTakeDam (9cd2a5d).
   REMAINING: **U3 SrvSett (UNBLOCKED — Set_ammo_count exists now; star_SrvSett.md is implementation-ready:
   id0=realbullet, id1=usecash bool, id2=ammocount+Set_ammo_count, id3=weaponrestriction; InitHUD sends all
   four; pairs with F4's cmd_weaponrestriction)** · U3b Wires (needs CBasePlayer::CheckForWires — decompile
   first) · U4b TSFade (decompile UTIL_ScreenFadeWrite @0xdcc54 first) · U5 CustomWP (pairs with F2b).
3. **Freefuncs (PARTIAL — cont.82)**: ✅ F1 dead/trivial batch + m61_grenade factory (bec9d0f) · ✅ F2a
   s_ammo_info/Set_ammo_count/falloff-LUT + the two live table-bug fixes (d58efed) · ✅ F3 ★ the range
   falloff WIRED both bullet paths + UTIL_TraceLine_Expanded (189f11f). REMAINING: **F2b CustomWP pipeline**
   (iPrice extraction via the Write_* disasm + SendWeaponParams family in TS_util.cpp + client MsgFunc —
   pairs with gmsg U5) · **F4 restriction** (GetWeaponType/IsRestrictedWeapon/WeaponRestrictionByID/
   cmd_weaponrestriction — pairs with U3 id3) · F5+ per dossier-freefuncs §3 (GetBodyByItems→Deploy,
   TSGetPointsForFrag, TSSelectSpawnSpot, SetPlayerRespawnTime→completes the TSPAward producers,
   DecideGamePlay, TSPrecache, TSPlayerDisconnect, TSCountPlayers, PlayerIsCheating,
   RemoveEntityFromSlowedArray→completes CTSKnife::Use, PseudoClimb).
4. **Task #8**: the 41 invented methods review (CTSGameRules 11 + CBasePlayer TS* 8 + misc) + 17 missing
   globals tail (TSGameRules global instance @0x1681a0, objects, roundtime) + m61_grenade classname.
5. **Stunt tail remainder** (small): superjump morfy anim (Animate 0x39), directional death (field_0x10c +
   die_spinback range), the +0x78 latch CONSUMER, PseudoClimb (pm scaffold), PM_Jump/PM_Duck full diffs, the
   getup-anim-on-deliberate-standup divergence question (ask the user).

### ⭐ THE METHODS THAT WORKED THIS SESSION (use them)
- **Workflows for RE fan-out** (user-approved this session; ultracode-style opt-in came from the user): the
  Workflow tool with a SERIAL decompile stage (the Ghidra `re` project is SINGLE-WRITER) then parallel analysts
  then a synthesis agent. 42 subagents, ~4.4M tokens kept out of the orchestrator. Pattern per workflow:
  decompile-stage agent runs ONE `re.sh 0xA 0xB …` batch → per-item analyst agents (READ-ONLY, no Ghidra) diff
  the decompile vs OUR source → synthesis writes the dossier + build-green plan. Apply CENTRALLY yourself,
  one nm-gated commit per unit. The `clientdll`/`mpdll` Ghidra projects are separate locks (parallel-safe).
- **The dossier-subagent prompt template** (for one-off big classes) is in the cont.79 section below.
- **Single general-purpose agents** for serial star-passes (the gmsg ★ decompiles).
- **User-driven probes**: build a 5-line Con_Printf/instrumentation, hand a numbered protocol, read the pasted
  output (the SEQSWAP + PMFRIC probes cracked both engine-side root causes). gdb attach to the ORIGINAL hl.exe
  requires asking (user approves; wrap in `timeout`, verify the int3 actually planted — see the ledger's probe
  saga; run_in_background Bash tasks, NOT `&`).
- **Lock-free binary archaeology**: objdump offset scans + nearest-nm-symbol + GOT-relative .rodata reads
  settle member/write questions WITHOUT Ghidra (but see [[embedded-object-offset-blindness]] — decompose
  composite offsets vs the embedded-object table first: CTSStunt@player+0x4f0, CTSAnimation@player+0x28c).
- **offsetof ground truth**: compile a tiny -m32 offsetof check against engine/progdefs.h before trusting ANY
  entvars offset mapping (gravity=0x11c, friction=0x120, v_angle=0x74, view_ofs=0x174, button=0x180,
  flags=0x1a4, iuser4=0x250, fuser1=0x254).
- **Model ground truth**: parse the player .mdl sequence table (python studiohdr parse — the script pattern is
  in the ledger) when pose names/families are in question.

### Task-list entries to recreate (via TaskCreate) — the cont.82 tracker
- `[in_progress]` **"Per-class 1:1 parity loop (audit→RE→reconcile→build→audit→commit)"** — the driver.
- `[in_progress]` **"PLAYTEST gate before merging restructure-source-parity to main"** — the stunt/movement
  chapter + melee S0-S6 basics are user-validated; everything else still needs the final playtest.
- `[pending]` **"Melee S7a/b/c blade split + S8 knife settle/pickup + S10 close-out (dossier-melee §3;
  client event handlers move WITH the wire swaps; fold HolsterWeapon in)"**.
- `[pending]` **"gmsg U0-U5 (dossier-gmsg + star addenda; U0 registration parity first; gmsgWeapons removal
  playtest-gated)"**.
- `[pending]` **"Freefuncs F1-F10 (dossier-freefuncs; F3 falloff = the gameplay headline, playtest-gated)"**.
- `[pending]` **"41 invented methods review + 17 missing globals + m61_grenade"**.
- `[pending]` **"Stunt/pm tail: morfy anim, directional death, +0x78 consumer, PseudoClimb, PM_Jump/PM_Duck
  diffs, getup-anim divergence question"**.

---

## ⭐⭐⭐⭐⭐⭐⭐⭐⭐⭐⭐⭐⭐ START HERE (cont.81, 2026-07-02) — pickup for the next orchestrator (cont.80 = me; available for consult)

**You ARE the orchestrator and the previous agent (cont.80) — continue as if you never stopped.** Branch
`restructure-source-parity` (NOT merged to `main`; GPG signing OFF — `git config --local commit.gpgsign false`,
keep it off). Read this section + the **`.serena/memories/ledger.md` cont.80 entry (top)** first — it has the
full per-commit detail of the whole CTSStunt/CTSAnimation stunt-system reconstruction done this session. 10
commits `fea34f9..07494ea`, every one builds `scripts/build.sh amd64 all` clean (glibc 2.14).

⚠ **SERENA ONE-LINE-JSON GOTCHA:** read the ledger with the **Read tool on `.serena/memories/ledger.md`**, NOT
`read_memory("ledger")` — the MCP returns one giant single-line `{"result":"…"}` (~300 KB) that overflows the
read limit. Same for any large memory (`jq -r '.result'` a saved copy if you must use the MCP).

### THE MISSION (yours) + the user's directives — these are the law (unchanged from cont.79 below; re-read them)
Make OUR code a **1:1 drop-in** for the original at **every level — file → class → methods → variables → globals →
free functions**. The exhaustive statement + the 5 baked-in directives are in the **cont.79 START HERE section
directly below** — READ IT. The essence, in the user's own words this session:
1. **Audit EVERYTHING bidirectionally** (filenames, the classes in them, each class's methods + VARIABLES, the
   globals, the free functions) — nothing the original has that we lack, nothing we have that it lacks (pure-TS
   surface; stock-SDK keeps its stock surface). **Driver = `scripts/coverage/audit.py <Class>` / `audit.py`** (see
   cont.79 for the full trustworthy-at-every-level description; current headline **0 missing/0 invented classes ·
   19 missing methods · 42 invented · 0 missing members · 43 real missing free funcs · 22 file mismatches · 2
   missing files**). Variable/member/file/global **NAMES must match** the original's FGD/save names — audit
   matches BY NAME, so a faithfully-functional-but-mis-named symbol reads as missing+invented.
2. **A missing/extra symbol is a SIGNAL of a potential LOGIC mismatch → re-RE it.** This paid off ALL SESSION:
   `GoRoll`/`GoWheel` had ZERO callers (dead code) = the "double-jump does nothing" bug; a `player+0x78` latch,
   `player+0x2a0`, `player+0x510` etc. each flagged real behaviour. When a symbol is missing, first assume the
   logic exists elsewhere and re-RE to find where.
3. **The logic is OFTEN already implemented — just `static`/inlined/renamed/as-a-method/misplaced/mis-wired. So a
   "missing" symbol usually means MOVE/re-wire it, NOT rewrite.** This session: `GoRoll` existed but wasn't called
   from `Duck`; `GoWheel` existed but wasn't called from `Jump`; the getup logic was there but dispatched from the
   wrong place. ALWAYS grep OUR code first.
4. **MIMIC THEIRS for drop-in compatibility (the cardinal rule).** Our client must be a drop-in swap for the
   original client, our server for the original server — run our client vs the original SERVER (or vice-versa) and
   behaviour is identical. When the original does something a particular way — ESPECIALLY across the client↔server
   boundary (commands, user-messages, events, button/usercmd handling, asset names, who-decides-what) — do NOT
   substitute a different-but-equivalent mechanism; RE theirs and mimic it EXACTLY. The user reinforced this
   session: *"if you're reading the full original, try to match it… drop-in replace and behave identically."*
5. **Don't make shit up vs the RE.** When static RE is uncertain, **live-trace** it (`scripts/live.py` on the
   ORIGINAL `hl.exe`, always running; OR the NEW live-trace of OUR OWN build, below) or decode the raw asm/.rodata.
   The DECOMPILE is ground truth (a hardware-bp live trace LIED this session — missed rare per-shot calls).

### ⭐ WHERE TO GO — the work is now TWO tracks; keep both moving
**Track A — the audit-driven parity grind (the core mission, `audit.py`).** 19 missing methods · 42 invented · 43
free funcs · 22 file mismatches · 2 missing files. This is the systematic 1:1 reconciliation the whole restructure
is about. Loop per class/file: `audit.py <Class>` → for each MISSING first GREP OUR CODE (move/rename the existing
logic) → else RE it (small: `re.sh <Class>`; big/cluster: dossier subagent) → reconcile into the original's EXACT
file/class/method/variable/global/NAME layout, mimicking the mechanism → `scripts/build.sh amd64 [all]` → `audit.py`
0/0 → commit (one unit). Many of the 19 missing methods are the INLINED melee/bullet path + the CTSGun tail (see
cont.79 #3/#4). `CBasePlayer::UpdateMeleeBar` specifically is now BLOCKING the GoWheel backward-backkick combo.

**Track B — the CTSStunt/CTSAnimation behaviour tail (this session's thread; user is actively playtesting).** The
whole stunt input→move system now works and is user-validated (double-tap-DUCK roll, double-tap-JUMP wheel-flip,
+alt1-wall backflip, getup, prone). **The OPEN items are listed in the ledger cont.80 "STILL OPEN" block** — pick
them up: GoWheel pitch tiers (awaiting re-test), the backward backkick combo (needs UpdateMeleeBar), directional
roll variants (roll_l/r/b — re-verify player+0x510), the getup ANIMATION on deliberate standup (ask the user — the
original has none), HandleSlide full auto-slide, the stand-up latch player+0x78, the superjump ANIM (m_bLowGravity
is a non-faithful reconstruction → RE the real morfy superjump), directional death. **The user drives playtests:**
they fire our build in graphical xash (`./test.sh`) and keep the ORIGINAL running in `hl.exe` for live gdb/MCP —
reach for a live trace whenever a "what does the original do here" question is uncertain. **Don't guess; RE + trace.**

### ⭐ THE GHIDRA / RUNTIME TOOLCHAIN (there is a LOT — the cont.79 section below documents ALL of it; use it)
- **`scripts/ghidra/re.sh`** — server decompile front-end on the typed `re` project (ApplyInventory: typed
  `this`/args/returns, `pev->`, member structs, vtables). `re.sh <Class>`, `re.sh 0x<ghidra-addr>` (= nm addr +
  0x10000), `re.sh refs <Class|addr>` (direct callers — used it this session to prove GoRoll's only caller is
  Duck), `re.sh vrefs <Method>` (VIRTUAL callers), `re.sh members`, `re.sh export <Class>`, `re.sh setup` (rebuild
  FRESH — single-writer). **Single-writer lock:** only ONE thing touches the `re` project at a time.
- **`scripts/ghidra/dc.sh <client|mp|server> 0x<addr>…`** — decompile-by-address for client.dll/mp.dll/the .so.
- **Ghidra MCP** (`mcp__ghidra-mcp__*`, 184 tools) — interactive decompile + **xref/callgraph** on a LIVE Ghidra
  GUI. First reach for "read/trace this fn + who calls it" once the GUI has the typed `re` project open
  (`scripts/ghidra/mcp-up.sh` prints the steps). ⚠ holds the `re` lock → close it before `re.sh`/`dc.sh server`.
- **`scripts/live.py`** — drive the running ORIGINAL `hl.exe` (server=mp.dll, client=client.dll). `pid`/`base`/
  `resolve`/`bt`/`read`/`readabs`/`vmethod`/`watch`/`break`. **`scripts/binmap.py <name>`** → live mp.dll RVA.
- **★ NEW this session — live-trace OUR OWN build (server .so, full symbols):** `BUILDTYPE=RelWithDebInfo
  scripts/build.sh amd64 all` (build.sh now honors `$BUILDTYPE`; default **Release has NO DWARF** so gdb resolves
  function symbols but not TYPES/MEMBERS — this blocks member reads). `test.sh GDB=<script.gdb>` launches xash
  UNDER gdb batch (gdb=parent satisfies `ptrace_scope=1`, **no sudo**). Probe pattern (scratchpad/anim_log.gdb): a
  python `gdb.Breakpoint`, `stop()` returns False (logs, never halts, ~full speed), reads args from ENTRY registers
  ($rsi/$edx — the named params are optimized out of scope at -O2; break at the first source line where the entry
  regs are still intact), casts via `gdb.parse_and_eval("(CBasePlayer*)$rsi")` in the SELECTED frame
  (`gdb.newest_frame().select()`; global `lookup_type` fails). This is how m_fSequenceFinished was PROVEN to work.
- **`scripts/coverage/*`** — the gap harness / inventory / completeness metric (`audit.py`, `coverage.py`,
  `reconstruction.py`, `extract_vtables.py`, `run.py`→`export.py`→`ts_inventory.json`, `members*.tsv`).

### ⭐ THE SUBAGENT-DOSSIER HINT (how this session scaled the CTSStunt RE — KEEP USING IT)
For a big class or interrelated cluster spawn a **`general-purpose`** subagent so the raw decompiles stay OUT of
YOUR context. It's RESUMABLE via `SendMessage(to: <agentId>, …)` — I resumed the CTSStunt dossier agent twice with
follow-up questions and it re-read its own cached scratchpad decompiles. Prompt template (adapt per target):
> "READ-ONLY reverse-engineering for mod-ts (reconstructing The Specialists — binaries the user OWNS — from
> `reference/ts_i386.so`). Produce a tight markdown DOSSIER; do NOT edit source. DECOMPILE `<methods/classes>` via
> the typed Ghidra `re` project (ALREADY BUILT — do NOT `re.sh setup`; single-writer, run ONE `re.sh`/`dc.sh` at a
> time). Capture each decompile to `…/scratchpad/<x>.c` and grep/read it YOURSELF — do NOT paste raw decompile.
> For EACH method: signature, step-by-step, the OTHER fns it calls, member accesses (offset+meaning), gmsg wire
> formats, string/const literals, and (crucial this session) the EXACT input/button gates + iuser4 bit sets/clears.
> ENUMERATE every member (offset/type/real-name) + globals + FREE FUNCTIONS. WRITE the dossier to
> `…/scratchpad/dossier-<x>.md` with a DIFF-vs-our-source + a RECONCILE PLAN (behavior-preserving vs playtest).
> Reply with ONLY a ~15-line concrete summary (triggers, gates, bit ops, formulas)."
⚠ **Single-writer lock:** a Ghidra dossier subagent can't overlap your own `re.sh`/`dc.sh` on the `re` project.
The scratchpad is SESSION-LOCAL — dossiers/decompiles from a prior session are gone; regenerate. This session's
cached CTSStunt decompiles (CTSStunt_all.txt, gowheel.txt, goroll.txt, jump.txt, Duck.txt) will NOT survive.

### Task-list entries to recreate (via TaskCreate) — the cont.81 tracker
- `[in_progress]` **"Per-class 1:1 parity loop (audit→RE→reconcile→build→audit→commit)"** — THE recurring driver.
- `[in_progress]` **"PLAYTEST gate before merging restructure-source-parity to main"** — everything is UNPLAYTESTED
  except what the user validated this session (the stunt system + the cont.78 bug fixes).
- `[pending]` **"CTSStunt/CTSAnimation behaviour tail (ledger cont.80 STILL-OPEN list): GoWheel pitch re-test,
  backward backkick (needs UpdateMeleeBar), directional roll roll_l/r/b, getup animation, HandleSlide auto-slide,
  superjump morfy anim, directional death"**.
- `[pending]` **"CBasePlayer 19 missing methods (inlined melee/bullet — UpdateMeleeBar blocks GoWheel backkick) +
  CTSGun fire/melee/grenade tail"**.
- `[pending]` **"43 real missing free funcs (range-damage falloff = gameplay gap; weapon-param tuning; gamerules/
  pm_shared) + m61_grenade classname"**.
- `[pending]` **"Review 6 invented-method DRIFT items (CTSGun::SwitchBarrel, CTSKnife settle/pickup)"**.
- `[pending]` **"17 missing globals = unimplemented gmsg* protocol (client HUD track)"**.
- `[pending]` **"22 file mismatches + 2 missing files — entity-file consolidation + client.dll HUD + pm_shared.c"**.

---

## ⭐⭐⭐⭐⭐⭐⭐⭐⭐⭐⭐⭐ START HERE (cont.79, 2026-06-30) — pickup for the next orchestrator (cont.78 = me; available for consult)

**You ARE the orchestrator and the previous agent — continue as if you never stopped.** Branch
`restructure-source-parity` (NOT merged to `main`; GPG signing OFF — `git config --local commit.gpgsign false`,
keep it off). Read this whole section + the `.serena/memories/ledger.md` cont.77/78/79 entries first. ~30 commits
this session (`3649f43..0d4572e`), every one builds `scripts/build.sh amd64 all` clean (glibc 2.14).

⚠ **SERENA ONE-LINE-JSON GOTCHA:** read the ledger with the **Read tool on `.serena/memories/ledger.md`**, NOT
`read_memory("ledger")` — the MCP returns one giant single-line `{"result":"…"}` (~250 KB) that overflows the
read limit. Same for any large memory (`jq -r '.result'` a saved copy if you must use the MCP).

### THE MISSION (yours) + the user's directives — read this twice, these are the law
Make OUR code match the original's structure **1:1 at every level — file → class → methods → variables → globals →
free functions** — nothing the original has that we lack, nothing we have that it doesn't (for pure-TS surface;
stock-SDK classes keep their stock surface). The user's exact, baked-in directives:
1. **Audit EVERYTHING, bidirectionally** (filenames, the classes in them, each class's methods + variables, the
   globals, the free functions). **If something the original has doesn't exist in ours, that is a SIGNAL of a
   potential LOGIC mismatch → re-RE it.** A missing/extra member or global frequently flags a logic mismatch
   ELSEWHERE (the worldgun `s_weapon_status` that exposed the drop-the-full-item path; the akimbo per-barrel
   clips that exposed the silent-fire prediction gap). Drive to N/N; **never paper over**. (memory
   `members-globals-signal-hidden-logic`.)
2. **The logic is OFTEN already implemented — just `static`/inlined/renamed/as-a-method/misplaced. So a "missing"
   symbol usually means MOVE it into the faithful shape, NOT rewrite it.** ALWAYS grep our code first (this
   session: Sparta/SetSpartaWeapon were inlined; CheckPowerups/CalcMiniminVotants were static; the gait logic
   was scattered). (memory `match-original-names-for-coverage`.)
3. **VARIABLE/MEMBER/FILE/global NAMES must match the original's FGD/save names** — `audit.py` matches BY NAME,
   so a faithfully-functional-but-mis-named symbol reads as missing+invented. Recover the name from the
   decompiled `KeyValue`/save-table and rename ours (e.g. `gmsgActItems`→`gmsgTSActiveItems`,
   `GrenadeThrow`→`GrenadeFire`, `slow_ammount` keeping the orig's typo). (memory `match-original-names-for-coverage`.)
4. **MIMIC THEIRS for drop-in compatibility (the cardinal rule).** The goal: our client is a drop-in swap for the
   original client, our server for the original server — run our client against the original SERVER (or
   vice-versa) and behaviour is identical. So when the original does something a particular way — ESPECIALLY
   anything crossing the client↔server boundary (commands, user-messages, events, button/usercmd handling, asset
   names, who-decides-what) — **do NOT substitute a different-but-equivalent mechanism; RE theirs and mimic it
   exactly.** If you catch yourself "doing it our way," VERIFY which side does what in the original first.
   (memory `mimic-original-mechanism-for-dropin`.)
5. **Don't make shit up vs the RE.** When static RE dead-ends (esp. a constant/formula/control-flow), trace the
   running original (`scripts/live.py`, the user keeps `hl.exe` running) or decode the raw asm + `.rodata`.
   **Static reading is fallible** — this session a hardware-bp live trace LIED (missed per-shot Animate calls)
   and I built a whole wrong "breakthrough" on it; the **decompile is ground truth**, cross-check live traces
   against it. (memories `flag-unfound-re-early`, `live-original-ts-debuggable`.)
- **Keep going until ~90% context.** One class/cluster at a time, bottom-up (dependencies first).

### ⭐ THE DRIVER — `audit.py` is now TRUSTWORTHY at every level (the cont.76/77 win — trust the numbers)
`scripts/coverage/audit.py <Class>` (one class) / `audit.py` (full `reference/coverage/AUDIT.md` + headline) /
`audit.py --strict` (CI gate). It bidirectionally diffs OUR build vs the original inventory (`ts_inventory.json`)
at **file → class → method → member → global → free-function**. Backed by `srcparse.py` (brace-aware single-pass
tokenizer; scans `.cpp` AND `.h` AND `pm_shared/`; sees pointer-to-member fields, DEFINE_ARRAY members, inline
methods, Allman/K&R free-fn braces). Transparency buckets so MISSING/INVENTED are REAL, not noise: **UNVERIFIABLE**
(client.dll classes, no symbols), **STOCK-SDK** (exact upstream `Class::Method` via `stock_methods.py`),
**CONSOLIDATED** (the 38 `Write_<weapon>` data-table fns we deliberately folded into `g_weaponInfo`+`Sparta()`).
**Current honest headline:** `0 missing/0 invented classes · 19 missing methods · 42 invented · 0 missing members ·
43 real missing free funcs (+38 Write_ consolidated) · 22 file mismatches · 17 missing globals · 6 invented globals`.
Companion scripts in `scripts/coverage/`: `reconstruction.py` (per-class method %), `extract_vtables.py` (all 226
vtables), `audit.py` is the driver. Regenerate the whole inventory with `scripts/coverage/run.py` (then `export.py`
→ `ts_inventory.json`); curator gotcha — `members_re.tsv` `keep`-persists rows even past a class's binary footprint
(a mis-hoist; verify offset is in `members_auto.tsv` for that class). (memory `audit-py-blind-spots`.)

### ⭐ THE LOOP (per class/cluster) — repeat this
`audit.py <Class>` → for each MISSING: **first grep OUR code** (static/inlined/renamed/free-fn/method? → MOVE/
rename it — the most common case) → else RE it (small: `re.sh <Class>` yourself; big class/cluster: spawn a
**dossier subagent**, hint below) → reconcile into the original's exact method/member/NAME layout, **mimicking the
original's mechanism** → `scripts/build.sh amd64 [all if client.so too]` → `audit.py <Class>` 0/0 → commit (one
unit per commit). The free-function level uses the same loop per `TS_*.cpp` file.

### ⭐ THE SUBAGENT-DOSSIER HINT (how this whole session scaled — KEEP USING IT)
For a big class or interrelated cluster (CTF, the worldgun throw chain, the 104-invented-method classification,
the 94-free-func categorization, the full CTSAnimation RE — all done this way), spawn a **`general-purpose`**
subagent so the raw decompiles stay out of YOUR context. Prompt template (adapt per target):
> "READ-ONLY reverse-engineering for mod-ts (reconstructing The Specialists — binaries the user OWNS — from
> `reference/ts_i386.so`). Produce a tight markdown DOSSIER; do NOT edit source. DECOMPILE `<methods/classes>` via
> the typed Ghidra `re` project (ALREADY BUILT — do NOT `re.sh setup`; it's **single-writer**, run ONE
> `re.sh`/`dc.sh` at a time). `scripts/ghidra/dc.sh server 0x<nm-addr>` (adds 0x10000) per addr, or `re.sh <Class>`
> for a whole class. Capture each decompile to `…/scratchpad/<x>.c` and grep/read it YOURSELF — do NOT paste raw
> decompile. For EACH method: signature, step-by-step, the OTHER methods it calls, member accesses (offset+meaning),
> gmsg user-messages (name + exact wire format), string/const literals. ENUMERATE every member (offset/type/real-
> name) + file-scope globals + FREE FUNCTIONS. WRITE the dossier to `…/scratchpad/dossier-<x>.md` with per-method +
> MEMBERS + GLOBALS + FREE-FUNCS + a DIFF-vs-our-source + a RECONCILE PLAN (note behavior-preserving vs needs-
> playtest). Reply with only a ~12-line summary."
⚠ **Single-writer lock:** only ONE thing touches the `re` Ghidra project at a time. A Ghidra subagent can't
overlap your own `re.sh`/`dc.sh`. The Ghidra GUI/MCP holding `re` blocks headless too (close it first). `dc.sh
client`/`dc.sh mp` use separate projects. The scratchpad is SESSION-LOCAL — dossiers from a prior session are
gone; regenerate via a subagent. (memory `ghidra-mcp-vs-own-tooling`.)

### ⭐ THE GHIDRA / RUNTIME TOOLCHAIN (there is a LOT — use it)
- **`scripts/ghidra/re.sh`** — server decompile front-end on the typed `re` project (ApplyInventory: typed
  `this`/args/returns, `pev->`, member structs, vtables). `re.sh setup` (rebuild FRESH — single-writer),
  `re.sh <Class>`, `re.sh refs <Class|addr>` (direct callers), `re.sh vrefs <Method>` (VIRTUAL callers), `re.sh
  members`, `re.sh export <Class>`, `re.sh 0x<ghidra-addr>` (= nm addr + 0x10000).
- **`scripts/ghidra/dc.sh <client|mp|server> 0x<addr>…`** — decompile-by-address for the Windows binaries
  (client.dll/mp.dll, separate projects) and the server `.so`. Feed it an RVA from a live trace.
- **Ghidra MCP** (`mcp__ghidra-mcp__*`) — interactive decompile + **xref/callgraph** on a LIVE Ghidra GUI. The
  user runs the GUI (`scripts/ghidra/mcp-up.sh` prints the steps; open `recovered/ghidra/re/re.gpr` + the
  `ts_i386.so` program, then `list_instances`→`connect_instance("re")`→`open_program`). First reach for
  "read/trace this function + who calls it". ⚠ holds the `re` lock → close it before `re.sh`/`dc.sh server`.
- **`scripts/live.py`** — drive the running ORIGINAL (`hl.exe` under Proton; server=mp.dll, client=client.dll).
  `pid`/`base <module>`/`resolve`/`bt`/`read`/`readabs`/`vmethod`/`watch`/`break`. **`scripts/binmap.py <name>`**
  resolves a `.so` fn NAME → live mp.dll RVA. **The user KEEPS hl.exe RUNNING — reach for it whenever static RE is
  uncertain.** ⚠⚠ **but hardware breakpoints can MISS rapid calls** (this session a `break`/probe on Animate
  showed "only idle/run, no fire calls" — WRONG; the decompile showed 34 weapon→Animate call sites). **Always
  cross-check a live trace against the decompile; the decompile wins.** gdb recipe for logging a value over many
  hits / watchpointing a player field: see `scratchpad/anim_probe.gdb`/`watchseq.gdb` + the ledger CTSAnimation
  entries (e.g. watch player `pev->sequence` at entvars offset **296**, `pev->gaitsequence` 300; the player's
  `CTSAnimation m_TSAnim` is at player+**0x28c**).

### Done this session (cont.78–79) — scoreboard (FULL per-unit detail + hashes in the ledger)
- **First PLAYTEST of cont.74–77** (user-driven, graphical xash) — VALIDATED + a run of real bug fixes:
  - ✅ DM ~5s teleport = the round-restart subsystem firing in plain DM (`dff06ec`); ✅ round clock missing on
    team maps (`f721c00`, RestartRound→StartRoundTimer registers the msg); ✅ stock bottom HUD duplicated TS HUD
    (`b543be6`, suppress stock CHudHealth/Ammo/AmmoSecondary Draw); ✅ brass casings stopped (`3b3708c`, TS never
    precached models/shell.mdl); ✅ dropped weapons vanished in 5s (`1fcd627`, registered the original's
    `weaponstay` cvar default 15); ✅ akimbo/gold fire SILENT until reload (`7999cc8` — per-barrel leftClip/
    rightClip are server-only, not in the prediction data; the client's predicted AkimboFire saw 0/0 → empty;
    reconstruct from the synced m_iClip in SetWeaponData).
- **CTSAnimation 3rd-person** (the deep work — see below): RE'd the full `Animate@0x5b470`, recovered the real
  model, implemented the `player+0x478` anim-override (`a9871ff`, knife/katana dedicated idle), made SetAnimation
  an empty no-op + all anim via direct Animate calls (`de29687`, `fb142f0`), the per-frame STATE-driven torso
  (`0d4572e`). A full 4-region rewrite (`482a099`) was tried and REVERTED (`6a8ca20`) — it relied on
  `m_fSequenceFinished`, which is NOT advanced for the server-side player.
- **Memory saved:** `mimic-original-mechanism-for-dropin`, `audit-py-blind-spots`, `members-globals-signal-hidden-logic`.

### ⭐ CTSANIMATION — the model recovered (read before touching player anim)
`CBasePlayer::SetAnimation@0xd8f50 is an EMPTY no-op in TS.` The original drives player animation by calling
`CTSAnimation::Animate(this, activity)` **directly** from the weapon (`CTSGun::Fire@0x71c78`→Animate(0/8/9),
KnifeFire/KatanaFire/Reload/Deploy) and movement (PostThink locomotion 1/6, PM_Jump 4, death 7) — 34 verified
call sites. `Animate` (`m_TSAnim` at player+**0x28c**) is one big function: Region A continue-blend · Region B
`switch(m_iLastActivity)` HOLD guards · Region C `switch(a1)` set body (case 0/8/9 FIRE→`shoot_`, case 0x28 DIVE,
…) · Region D leg gait (`pev->gaitsequence`). **Region C case 1/6 (run/idle, called EVERY frame) is the per-frame
STATE-driven torso:** reads `player+0x478`(anim-override string: knife_idle/katana_idle/briefcase_idle, set in
`DefaultDeploy@0x90178` per m_weaponType) > weapon-pose-raised > grip > `aim_`/`idle_` by `(m_flAimTimer||fov)`.
We now mirror all of this (SetAnimation empty, direct calls, the override, the state-driven torso). **STILL OPEN
(playtest-gated, the user reported these vs the original):** (1) the 3rd-person FIRE pose still may not show right
— next step is a `pev->sequence` watch on OUR build (xash rig) vs the original on the same shot; (2) the DIVE-hold
(must gate on a leg-timer / iuser4 stunt bit, NOT m_fSequenceFinished); (3) getup leaves the player ducked (likely
TS_pm_shared unduck, not Animate); (4) the melee BLOCK on reload (kungfu/knife/katana — HandleCloseCombat reads
player+0x478 for the kat/kni pick; now that we SET 0x478 it's wireable). **EVERYTHING cont.74–79 is UNPLAYTESTED
except the 6 fixed bugs above — a live pass is the GATE before merge.**

### NEXT — the work-list (audit.py-driven). Numbers: 19 missing methods · 42 invented · 43 real free funcs · 22 file mismatches.
1. **PLAYTEST the latest CTSAnimation + akimbo + round/HUD/casing/weaponstay fixes** (the GATE). The user fires
   changes live in xash (`./test.sh`); the original runs in `hl.exe` for live gdb/MCP. Confirm no regression in
   walk/run/idle/jump/death (the empty-SetAnimation refactor is behaviour-identical) + the new wins.
2. **CTSAnimation tail** — the 4 open items above (fire-pose pev->sequence compare; dive leg-timer hold; getup
   unduck; melee block via player+0x478). Use the live `pev->sequence` watch (scripts saved) vs the decompile.
3. **CBasePlayer 19 missing methods** — mostly INLINED in the playtest-gated melee/bullet path (ShootBulletsPlayer,
   UpdateMeleeBar, BlockPlayer, CloseCombatAttack, ColdCockAttack, CheckHull/CheckForWires); CTSGun tail
   (CCombatIdle/EmptySound/GrenadeFire/GrenadeIdle/HolsterWeapon/KatanaFire/KatanaThrow/KnifeFire/ReloadWeapon/
   StopReload); CTSKnife::WaitTouch (the original's TOUCH-based settle vs our invented think-based StartSettle/
   SettleThink — a mechanism drift, see #4). Extract ON a green baseline.
4. **42 invented methods** — classified (`scratchpad/dossier-invented-methods.md` if regenerated): 22 keepable
   helpers, ~12 stock-virtual overrides (visible by design), **6 DRIFT to reconcile** (CTSGun::SwitchBarrel =
   invented akimbo barrel-cycle replacing the orig SecondaryAttack@0x817a8 fire-the-other-hand; CTSKnife
   BecomePickup/PickupTouch/StickAt/StartSettle/SettleThink = think-based settle vs the orig touch-based
   WaitTouch/WaitThink@0x6dce8). All playtest-gated combat mechanisms.
5. **43 real missing free funcs** — genuine RE: the weapon-param tuning subsystem (TS_util.cpp Adjust/save/
   SendWeaponParams + gmsgTSCustomWeaponParams), the **range-damage falloff** (TS_gun.cpp compile_range_damage/
   get_range_multipler — bulletmgr uses RAW damage = a real gameplay gap), gamerules (DecideGamePlay/
   TSSelectSpawnSpot/GetAccelBySlots), pm_shared (CanJumpHigh/FixVelocity/PseudoClimb), melee Victim*Hold/
   FindHullIntersectionplayer (gated). UNSURE: CalcSpread vs our ScaleSpread; DropKickback vs our VKickback. The
   **m61_grenade** classname (CreateGrenade sets "grenade" not "m61_grenade" — RE which before renaming).
6. **17 missing globals** — the gmsg* ones map to the MISSING user-message list (server-send + client-hook
   unimplemented = real protocol RE, overlaps the client-HUD track: Objective/ShowObj/Wires/TSState/TSFade/…).
7. **22 file mismatches + 2 missing files** — structural (the original groups entities into TS_entities.cpp/
   TS_objectives.cpp/TS_grenades.cpp/TS_worldgun.cpp; we split per-class). High-churn behaviour-preserving moves;
   the 2 missing files are client.dll (HUD track) + pm_shared.c (movement free-fns, partly credited now).

### Task-list entries to recreate (via TaskCreate) — the cont.79 tracker
- `[in_progress]` **"Per-class 1:1 parity loop (audit→RE→reconcile→build→audit→commit)"** — THE recurring driver; keep open.
- `[in_progress]` **"PLAYTEST gate before merging restructure-source-parity to main (everything cont.74–79 UNPLAYTESTED bar the 6 fixed bugs)"**.
- `[pending]` **"CTSAnimation tail: 3rd-person fire-pose pev->sequence compare; dive leg-timer hold; getup-unduck; melee block via player+0x478"**.
- `[pending]` **"CBasePlayer 19 missing methods (inlined melee/bullet — playtest-gated) + CTSGun fire/melee/grenade tail"**.
- `[pending]` **"43 real missing free funcs (range-damage falloff = gameplay gap; weapon-param tuning; gamerules/pm_shared) + m61_grenade classname"**.
- `[pending]` **"Review the 6 invented-method DRIFT items (CTSGun::SwitchBarrel, CTSKnife settle/pickup — mechanism drift, playtest-gated)"**.
- `[pending]` **"17 missing globals = unimplemented gmsg* protocol (client HUD track: Objective/ShowObj/Wires/TSState/…)"**.
- `[pending]` **"22 file mismatches + 2 missing files — entity-file consolidation + client.dll HUD + pm_shared.c"**.

---

## ⭐⭐⭐⭐⭐⭐⭐⭐⭐⭐⭐ START HERE (cont.78, 2026-06-29) — playtest fixes landed; CTSAnimation needs a LIVE-TRACE pass

Branch `restructure-source-parity` (NOT merged; GPG off). The user PLAYTESTED cont.74-77 in graphical xash and
the session fixed a run of real bugs (all committed, build `scripts/build.sh amd64 all` clean). Read the
**`.serena/memories/ledger.md`** tail (the playtest findings + the CTSAnimation spec) first.

### Playtest bugs FIXED this session (validated or pending re-test — see ledger)
- ✅ DM ~5s teleport (round-restart fired in plain DM) — gated to round modes (`dff06ec`, user-validated).
- ✅ Round clock missing on team maps — RestartRound now routes through StartRoundTimer (`f721c00`).
- ✅ Stock bottom HUD duplicated TS HUD — suppressed stock CHudHealth/Ammo/AmmoSecondary Draw (`b543be6`).
- ✅ Brass casings stopped — TS never precached models/shell.mdl (`3b3708c`).
- ✅ Dropped weapons vanished in 5s — registered the original's `weaponstay` cvar (default 15) (`1fcd627`).
- ✅ Akimbo/gold fire SILENT until reload — per-barrel leftClip/rightClip aren't in the prediction data; the
  client's predicted AkimboFire saw 0/0 + empty-returned. Reconstruct them in SetWeaponData from m_iClip (`7999cc8`).

### ⭐ IMMEDIATE NEXT: CTSAnimation 3rd-person — do a LIVE-TRACE pass (the user is waiting on this)
The 4-region Animate rewrite was REVERTED (`6a8ca20`) — it relied on `m_fSequenceFinished` for the hold guards,
but that's NOT reliably advanced for the SERVER-side player model → froze stunts/idle. The full RE is sound
(`scratchpad/dossier-ctsanimation.md` — REGENERATE, scratchpad is session-local). The user gave 6 precise
3rd-person observations (dive-crouch, getup-stays-ducked, fire-anim-doesnt-play [PRE-EXISTING], knife idle =
kungfu combat_idle, katana dedicated idle, melee BLOCK on reload) — recorded as a SPEC in the ledger
("CTSAnimation — 3rd-person playtest observations"). DO NOT guess (guessing caused the regressions): live.py +
binmap CTSAnimation::Animate on the running original, user reproduces each action, read the activity arg +
resulting pev->sequence/gaitsequence, THEN reconcile minimally. The hold guard must NOT use m_fSequenceFinished
(use the leg-timer / an iuser4 stunt bit).

### Audit still clean (the trustworthy driver)
`0 missing/invented classes · 19 missing methods · 42 invented · 0 missing members · 43 real missing free funcs
(+38 Write_ consolidated) · 22 file mismatches`. Remaining = genuine-RE (needs Ghidra) or playtest-gated.

---

## ⭐⭐⭐⭐⭐⭐⭐⭐⭐⭐ START HERE (cont.77, 2026-06-29) — pickup for the next orchestrator (cont.76 = me; available for consult)

**You ARE the orchestrator — continue as if you never stopped.** Branch `restructure-source-parity` (NOT merged;
GPG signing OFF — keep `git config --local commit.gpgsign false`). Read the **`.serena/memories/ledger.md` cont.77
entry** first (full detail). Same mission + 5 user directives as cont.76 below (audit everything bidirectionally;
move-existing-logic before RE; names match original; MIMIC THEIRS for drop-in; don't guess vs the RE). 14 commits
this session (`3649f43..03191a0`), every one builds `scripts/build.sh amd64 all` clean.

### What changed: the audit is now TRUSTWORTHY AT EVERY LEVEL (the cont.77 win)
The driver's false positives are gone — the headline is now a real work-list:
`0 missing/invented classes · 19 missing methods · 42 invented methods · 0 missing members · 43 missing free funcs
(+38 Write_<weapon> consolidated) · 22 file mismatches · 17 missing globals · 6 invented globals`, plus transparent
**+46 client-unverifiable** & **+15 stock-SDK** method buckets. Tooling fixes (all in `scripts/coverage/`): srcparse
now sees pointer-to-member fields + array-index (DEFINE_ARRAY) members + SOURCE-level free funcs (Allman/K&R braces,
`pm_shared/` dir); audit buckets client.dll-no-symbol methods (UNVERIFIABLE), exact-upstream stock methods (STOCK-SDK,
via `stock_methods.py`→`stock_methods.tsv`), and Write_<weapon> data-init (CONSOLIDATED). **Trust the numbers now.**

### Done this session (cont.77) — scoreboard (full detail + hashes in the ledger)
- **Audit trustworthiness**: members 19→0 (pointer-to-member + array-collapse + CTSPhysicObject curator mis-hoist
  removed from `members_re.tsv`+inventory); invented 104→42 (client/stock bucketed); free funcs 94→43 real
  (source-credit + Write_ consolidation); globals 8→6 invented (gmsg renames).
- **2 latent bugs fixed** (`acf5ed3`, the "missing symbol = logic signal" paying off): deadcorpse precache (linked
  ts_corpse, precached "deadcorpse" → corpse never precached) + knife worldgun touch-ignore (ts_worldgun checks
  "knife", we linked ts_knife). Renamed entity classnames to the original (deadcorpse/knife/shrapnel).
- **Parity moves**: GrenadeThrow→GrenadeFire (double win), gmsg vars→gmsgTSActiveItems/gmsgTSWeaponStatus,
  Sparta()/SetSpartaWeapon extracted from compile_weapons_info. **Invented-method review (task #5) COMPLETE.**

### NEXT — the remaining work is genuine-RE or PLAYTEST-GATED (the easy moves are done)
1. **PLAYTEST (#2) is STILL THE GATE.** Everything cont.74–77 UNPLAYTESTED; the cont.77 corpse/knife classname
   fixes are behaviour-affecting and want confirmation. Plan + in-game checklist were given to the user.
2. **43 real missing free funcs** — genuine RE: the weapon-param tuning subsystem (TS_util.cpp Adjust/save/SendWeaponParams
   + gmsgTSCustomWeaponParams), the **range-damage falloff** (TS_gun.cpp compile_range_damage/get_range_multipler —
   bulletmgr uses RAW damage = a real gameplay gap), gamerules (DecideGamePlay/TSSelectSpawnSpot/GetAccelBySlots),
   pm_shared (CanJumpHigh/FixVelocity/PseudoClimb — need Ghidra), melee Victim*Hold/FindHullIntersectionplayer (gated).
   UNSURE to settle via Ghidra: CalcSpread vs our ScaleSpread, DropKickback vs our VKickback. **m61_grenade** LINK + classname (RE the classname first).
3. **19 missing methods** (all playtest-gated melee/bullet/throw/spawn), **6 invented-method drift** (CTSGun::SwitchBarrel,
   CTSKnife pickup/settle — playtest-gated), **CTSGun tail** (#6), **client HUD handlers** (#7), **22 file mismatches** (held, high-churn).

---

## ⭐⭐⭐⭐⭐⭐⭐⭐⭐ START HERE (cont.76, 2026-06-29) — pickup for the next orchestrator (cont.75 = me; available for consult)

**You ARE the orchestrator and the previous agent (cont.75) — continue as if you never stopped.** Branch
`restructure-source-parity` (NOT merged to `main`; GPG signing OFF — `git config --local commit.gpgsign false`, keep it
off). Read this section + the `.serena/memories/ledger.md` cont.75 entries first. ~32 commits this session
(`2969215..b34ef52`), every one builds `scripts/build.sh amd64 all` clean (glibc 2.14). **EVERYTHING THIS SESSION IS
UNPLAYTESTED — a live pass is the gate before merge (task #9).**

⚠ **SERENA ONE-LINE-JSON GOTCHA:** read the ledger with the **Read tool on `.serena/memories/ledger.md`**, NOT
`read_memory("ledger")` — the MCP returns one giant single-line `{"result":"…"}` (~200 KB) that overflows the read
limit. Same for any large memory (`jq -r '.result'` a saved copy if you must use the MCP).

### THE MISSION (yours), with the user's sharpened feedback baked in — read this twice
Make OUR code match the original's structure **1:1 at every level — file → class → methods → variables → globals →
free functions** — nothing the original has that we lack, nothing we have that it doesn't (for pure-TS surface; stock
SDK classes keep their stock surface). The DRIVER is now **trustworthy** (see below). The user's exact directives:
1. **Audit EVERYTHING, bidirectionally.** Filenames, the classes in them, each class's methods + variables, the globals,
   the free functions. **If something the original has doesn't exist in ours, that is a SIGNAL of a potential logic
   mismatch → re-RE it.** A missing/extra member or global often flags mismatched LOGIC elsewhere (the worldgun
   `s_weapon_status` that turned a "missing member" into the whole drop-the-full-item path; the `CashBased()` buy-gate
   that consumed the "unused" members). Drive to N/N — never paper over. (memory `members-globals-signal-hidden-logic`.)
2. **The logic is OFTEN already implemented — just `static`/inlined/renamed/as-a-method/misplaced. So a "missing"
   symbol usually means MOVE it into the faithful shape, NOT rewrite it.** This was the dominant pattern this session
   (slow-math was scattered static+inlined; GetSpeedBySlots was our `TSSlotBaseSpeed`; DropWeapon/CutWire were free
   functions; StopObserver was our TSLeaveSpectate). ALWAYS grep our code for the logic before RE-ing from scratch.
3. **VARIABLE/MEMBER NAMES must match the original's FGD/save names** — `audit.py` matches members BY NAME, so a
   faithfully-functional-but-mis-named member reads as missing+invented. Recover the name from the decompiled
   `KeyValue`/save-table and rename ours (we did `m_flDuration`→`duration`, `m_flSlowAmount`→`slow_ammount` keeping the
   orig's typo, `TSSlotBaseSpeed`→`GetSpeedBySlots`, etc.). (memory `match-original-names-for-coverage`.)
4. **MIMIC THEIRS for drop-in compatibility (the cardinal rule).** The goal: our client is a drop-in swap for the
   original client, our server for the original server — run our client against the original server (or vice-versa) and
   behaviour is identical. So when the original does something a particular way — ESPECIALLY anything crossing the
   client↔server boundary (commands, user-messages, events, button/usercmd handling, asset names, who-decides-what) —
   **do NOT substitute a different-but-equivalent mechanism; RE theirs and mimic it exactly.** If you catch yourself
   "doing it our way," VERIFY which side does what in the original first. (memory `mimic-original-mechanism-for-dropin`.
   Worked example this session: powerup double-tap +use — verified the original uses the `usepowerup` COMMAND (client
   detects → server handles), `TSUse` is uncalled dead code, so our command path already matches; AND the drop-sound
   `weapons/weapondrop.wav` vs our wrong `items/weapondrop1.wav` was a real swap-contract bug, fixed.)
5. **Don't make shit up vs the RE.** When static RE dead-ends (esp. on a constant/formula), trace the running original
   (`scripts/live.py`, the user keeps `hl.exe` running) or decode the raw x87 asm + `.rodata` constants — DON'T guess.
   This session the ThrowWeapon throw-velocity was flagged "needs live trace," then resolved by decoding the asm +
   reading the float constants (0.9/0.33/90/16) — ground truth from the binary, no guess. (memories `flag-unfound-re-early`,
   `live-original-ts-debuggable`.)
- **Keep going until ~90% context.** One class/cluster at a time, bottom-up (dependencies first).

### ⭐ THE DRIVER IS NOW TRUSTWORTHY — `audit.py` + `srcparse.py` (the cont.76 tooling win)
The audit had blind spots that polluted MISSING/INVENTED with false positives (so "is anything missing?" couldn't be
answered cleanly). FIXED this session — **the numbers are now real**:
- **NEW `scripts/coverage/srcparse.py`** — a single-pass tokenizer (handles line/block comments + string/char literals
  so braces never desync) that brace-scans BOTH `.cpp` and `.h`, yielding per-class **members** (from `.cpp` too — the
  big old blind spot), the class→base **hierarchy**, **in-body (inline) methods**, and file-scope **free functions**.
  (A naive regex was leaking quotes from `//` comments across newlines and silently dropping whole classes — class
  discovery 240→505. Don't regress this.)
- **`scripts/coverage/audit.py`** now: members from `.cpp` too; methods resolve through **inheritance + inline** (so
  CLadder/CTriggerPush/GetNextBestWeapon etc. no longer false-flag); a NEW **free-function level** diffs each
  `TS_*.cpp`'s free/static functions vs our build symbols. Run `audit.py <Class>` for one class (reliable now),
  `audit.py` for the full `reference/coverage/AUDIT.md` + headline, `audit.py --strict` as the CI gate.
- **Current honest headline:** `0 missing / 0 invented classes · 20 missing methods · 104 invented methods · 19 missing
  members · 94 missing free functions · 22 file mismatches · 2 missing files`. The remaining are now REAL work, NOT
  tooling noise (the few exceptions — `.cpp`-declared stock classes' member counts, the CTSObjectiveManager DEFINE_ARRAY
  artifact, the CTSPhysicObject curator mis-hoist — are known-false, see the per-class output / memory `audit-py-blind-spots`).

### ⭐ THE LOOP (per class/cluster) — repeat this
`audit.py <Class>` → for each MISSING: **first grep OUR code** (is it static/inlined/renamed/a-free-function/a-method?
→ MOVE/rename it; the loop's most common case) → else RE it (small: `scripts/ghidra/re.sh <Class>` yourself; big class
or cluster: spawn a **dossier subagent**, see the hint below) → reconcile into the original's exact method/member/name
layout, **mimicking the original's mechanism** → `scripts/build.sh amd64 [all if client.so too]` → `audit.py <Class>`
0/0 → commit. The free-function level uses the same loop per `TS_*.cpp` file.

### ⭐ THE SUBAGENT-DOSSIER HINT (this is how the whole session scaled — KEEP USING IT)
For a big class or an interrelated cluster (CTF cluster, CTSGun tail, the stock-class gaps, the worldgun throw/drop/
create chain — all done this way), spawn a **`general-purpose`** subagent so the raw decompiles stay out of YOUR
context. Prompt template (adapt per target):
> "READ-ONLY reverse-engineering for mod-ts (reconstructing The Specialists — binaries the user OWNS — from
> `reference/ts_i386.so`). Produce a tight markdown DOSSIER; do NOT edit source. DECOMPILE `<methods/classes>` via the
> typed Ghidra `re` project (ALREADY BUILT — do NOT `re.sh setup`; it's **single-writer**, run ONE `re.sh`/`dc.sh` at a
> time). `scripts/ghidra/dc.sh server 0x<nm-addr>` (adds 0x10000) per addr, or `re.sh <Class>` for a whole class.
> Capture each decompile to `…/scratchpad/<x>.c` and grep/read it YOURSELF — do NOT paste raw decompile. For EACH
> method: signature, step-by-step, the OTHER methods it calls, member accesses (offset+meaning), gmsg user-messages
> (name + exact wire format), string/const literals. ENUMERATE every member (offset/type/real-name) + file-scope
> globals + FREE FUNCTIONS. WRITE the dossier to `…/scratchpad/dossier-<x>.md` with per-method + MEMBERS + GLOBALS +
> FREE-FUNCS + a DIFF-vs-our-source + a RECONCILE PLAN (note behavior-preserving vs needs-playtest). Reply with only a
> ~12-line summary."
⚠ **Single-writer lock:** only ONE thing touches the `re` project at a time. A Ghidra subagent can't overlap your own
`re.sh`/`dc.sh`. The Ghidra GUI/MCP holding `re` blocks headless too (close it first). `dc.sh client`/`dc.sh mp` use
separate projects. The scratchpad is SESSION-LOCAL — dossiers from a prior session are gone; regenerate via a subagent.

### Done this session (cont.76) — FULL per-unit detail + commit hashes in the ledger; scoreboard:
- **Trustworthy audit** (`a77ff52`): srcparse.py + audit.py free-function level + inheritance/inline + the c++filt
  spacing-bug fix. THE foundational win — the audit is now a reliable work-list.
- **CTF cluster → 1:1** (`a4254ec`): s_flag_status value-carry model, CTSBriefcase 6/6, CTSFlag 15/15, CBasePlayer
  TakeFlag/DropFlag/HaveFlag, the -20 slot-weight encumbrance.
- **Stock-SDK class gaps closed + match-verified** (`50a6963`,`6b722f3`,`c3960d8`,`7374852`): CBaseEntity TS base
  virtuals (GiveSlowMul/GoSlow/InSlow/CanUsedTroughWalls/ShouldCollide/BreakableRespawn/OnFreeEntPrivateData/eoffset,
  72→80); CBasePlayerWeapon AddWeapon/AlternateAttack; CFuncPlat targettop/targetbot + KeyValue/Save/Restore;
  CHalfLifeMultiplay::IsValidTeam; **breakable doors** CBaseDoor/CRotDoor (TraceAttack+BreakableRespawn — UNPLAYTESTED,
  touches every map's doors); **GetGameDescription divergence RESOLVED** (moved to CHalfLifeMultiplay, theOne→LMS→DM, no
  teamplay branch — the user's "confirm match vs rewritten" ask).
- **Move-existing-logic parity grind:** slow-math → `ts_shared.cpp` (`70172f1`); GetSpeedBySlots+InitWeaponRestriction
  (`b79559e`); **CBasePlayer 96→100/120** (DropWeapon/CutWire/SetPlayerSpeed/SetPlayerGravity/PunchScreen/
  GetMeleeMultiplier/DropArmor/DropAmmo/UpdatePlayerState/Observer_Idle/StopObserver/TSUse/ThrowWeapon — commits
  `0c89442`,`03adb27`,`44fc97e`,`5236d40`,`945a343`,`0832bda`,`5004e1a`,`578536f`).
- **Worldgun throw/drop reconcile** (`cd892da`+`b34ef52`): split our 4-arg CreateWeapon → the original 3-arg
  `(owner,type,status)` factory + restored owner/skin/glow; added the real `CBasePlayer::ThrowWeapon`; fixed the
  drop-sound swap-contract; recovered the exact throw-velocity formula from the asm (`0.9*playerVel + S*fwd + 0.33*S`
  up; alive S=(90-pitch)*2, dead S=RandomLong(60,160)). Dossier `scratchpad/dossier-throwchain.md`.
- **Memory saved:** `mimic-original-mechanism-for-dropin` (the drop-in compatibility principle).

### NEXT — work-list (audit.py-driven; recreate the task entries below). Metrics: 20 missing methods, 104 invented, 94 missing free funcs.
1. **PLAYTEST PASS (task #9, the GATE).** Everything is validated-vs-RE but UNPLAYTESTED. Confirm a green baseline in
   graphical xash BEFORE the playtest-gated reshapes: breakable doors, CTF, slow-motion, the weapon throw (arcs
   forward+up, spins, right sound), CTSGameRules round/objective/team/TheOne/voting, grenade, AmmoPack armor. The user
   fires changes live in xash (`test.sh`); the original TS runs in `hl.exe` for live gdb traces (`scripts/live.py`).
2. **CBasePlayer 20 missing methods (task #7)** — mostly INLINED in the playtested melee/bullet path (ShootBulletsPlayer,
   UpdateMeleeBar, BlockPlayer, CloseCombatAttack, ColdCockAttack) → extract carefully ON a green baseline; PickUpWeapon
   (signature differs from our slice); TSInit (1208B spawn reset — big blast radius); CheckHull/CheckForWires.
3. **Free-function grind (task #8)** — 94 missing; ~38 are the deliberate `Write_<weapon>` data-table consolidation
   (NOT gaps). The rest split move-existing (VictimKungfuHold/KnifeHold/KatanaHold inlined in melee; TS_util.cpp helpers)
   vs genuine-RE (TS_pm_shared.c movement/stunt TSDive/TSWallJump; TS_gun.cpp Sparta/compile_weapons_info/CalcSpread;
   DecideGamePlay/TSSelectSpawnSpot/GetAccelBySlots).
4. **Review the 104 invented methods (task #4)** — confirm legit-helper vs drift vs name-mismatch vs needs-moving.
5. **19 missing members** — mostly known-false (CTSObjectiveManager DEFINE_ARRAY artifact, CTSPhysicObject mis-hoist,
   `.cpp`-declared stock-class member counts); each still wants a 1-line confirm.
6. **22 file mismatches, 2 missing files** (CBulletManager grouping, the HUD client.dll classes — client-side track).

### Task-list entries to recreate (the cont.76 tracker — recreate via TaskCreate):
- `[in_progress]` **"Per-class 1:1 parity loop (audit→RE→reconcile→build→audit→commit)"** — THE recurring driver; keep open.
- `[pending]` **"PLAYTEST pass before merging restructure-source-parity to main (everything UNPLAYTESTED)"** — the GATE.
- `[pending]` **"CBasePlayer remaining 20 missing methods (mostly inlined melee/bullet — playtest-gated)"**.
- `[pending]` **"Free-function move-existing-logic grind (94 missing — ~38 are Write_<weapon> consolidation)"**.
- `[pending]` **"Review the 104 invented methods (confirm legit-helper vs drift vs name-mismatch)"**.
- `[pending]` **"CTSGun tail 37/47 (predicted fire/melee/grenade — playtest-gated)"**.
- `[pending]` **"Client-side parity: HUD handlers for Objective/ShowObj/Wires + per-fire-type events (dc.sh client)"**.

---

## ⭐⭐⭐⭐⭐⭐⭐⭐ START HERE (cont.75, 2026-06-29) — pickup for the next orchestrator (cont.74 = me; available for consult)

**You ARE the orchestrator and the previous agent (cont.74) — continue as if you never stopped.** Branch
`restructure-source-parity` (NOT merged to `main`; GPG signing OFF — `git config --local commit.gpgsign false`, keep it
off). Read this section + the **`.serena/memories/ledger.md`** cont.74 entries first.

⚠ **SERENA ONE-LINE-JSON GOTCHA:** read the ledger with the **Read tool on `.serena/memories/ledger.md`**, NOT
`read_memory("ledger")` — the MCP returns one giant single-line `{"result":"…"}` (~200 KB) that overflows the read
limit. Same for any large memory (`jq -r '.result'` a saved copy if you must use the MCP).

### THE LOOP (per class) — full detail in the cont.73 section below; the short version:
`python3 scripts/coverage/audit.py <Class>` → RE it (small class: `scripts/ghidra/re.sh <Class>` yourself; big class/
cluster: spawn a **dossier subagent**, see the hint below) → reconcile OUR working logic into the original's exact
method/member layout → `scripts/build.sh amd64 [all if the class is in client.so too]` → `audit.py <Class>` 0/0 → commit.

### THE DIRECTIVE (yours), with the user's sharpened feedback baked in
- **ONE class at a time, methods + variables 1:1** — nothing missing, nothing added (for pure-TS classes; stock-SDK
  classes keep their stock surface). Do dependencies FIRST (bottom-up).
- **VARIABLE/MEMBER NAMES must match the original's FGD/save names** — `audit.py` matches members BY NAME, so a
  faithfully-functional-but-mis-named member reads as missing+invented. Rename to the original (we did `m_iType`→
  `poweruptype`, `m_objTargets`→`targets`, `m_winnerTeam`→`winnerteam`, etc. — recover the names from the decompiled
  `KeyValue`/save-table). (memory `match-original-names-for-coverage`.)
- **ANY missing/extra member OR global = a hidden-logic inconsistency → re-audit + Ghidra, never paper over.** A
  mismatch in one place often flags mismatched LOGIC elsewhere (the CTSGroundWeapon `CashBased()` buy-gate that consumed
  the "unused" attackerteam/buyable members; the worldgun s_weapon_status that turned a "missing member" into the whole
  drop-the-full-item path). Drive members to N/N, not just methods. Unused member = flag + FIX immediately. (memory
  `members-globals-signal-hidden-logic`.)
- Drop each class's keepalive by building its real wiring (caller/hierarchy), not by leaving an anchor.
- **Keep going until ~90% context.**

### ⚠ audit.py BLIND SPOTS (memory `audit-py-blind-spots`) — don't trust a "gap" without reading the source:
- `our_members()` only scans **`.h` files** → a class declared in a `.cpp` reports "0 ours / MISSING named: <all>"
  FALSELY (CTSFlag, and any .cpp-only class). VERIFY member gaps by reading the `.cpp`. (Extracting the class to a header
  — as we did for CTSObjective/CTSObjectiveManager/CTSWorldGun — fixes the blind spot AND lets sibling .cpp files derive it.)
- **Inline** methods (defined in the header) read as MISSING; the original usually has them out-of-line, so moving ours
  out-of-line is MORE faithful (CBreakable's Explodable; CTSObjective's CanInteract/Freeze).
- **Invented `ObjectCaps` pattern:** many TS classes override ObjectCaps where the original uses the base → confirm via
  decompile (no ObjectCaps in its method list) and REMOVE ours (the FCAP diff is inert for runtime-created / MP-only ents).
- **DEFINE_ARRAY** save members read as `name[0..N]` per-element vs our array name — a naming artifact, data saves fine.

### ⭐ THE SUBAGENT-DOSSIER HINT (this is how the whole session scaled — keep using it)
For a big class or a cluster, spawn a **`general-purpose`** subagent so the raw decompiles stay out of YOUR context.
Prompt template (adapt per target):
> "READ-ONLY reverse-engineering for mod-ts (reconstructing The Specialists — binaries the user OWNS — from
> `reference/ts_i386.so`). Produce a tight markdown DOSSIER; do NOT edit source. DECOMPILE `<methods/classes>` via the
> typed Ghidra `re` project (already built — **do NOT `re.sh setup`**; it's **single-writer**, run ONE `re.sh`/`dc.sh` at
> a time). Resolve a non-resolving method by `nm reference/ts_i386.so | grep '<name>__12<Class>'` then
> `scripts/ghidra/dc.sh server 0x<nm-addr>` (it adds 0x10000). Capture each decompile to a scratchpad file
> (`…/<session>/scratchpad/<x>.c`) and grep/read it yourself — do NOT paste raw decompile. For EACH method: signature,
> step-by-step behavior, the OTHER methods it calls, member accesses (offset+meaning), gmsg user-messages (name + exact
> WriteByte/Short/String wire format + REG size), string/const literals. ENUMERATE every member (offset/type/real-name-
> if-recoverable/which method uses it). Note file-scope globals. WRITE the dossier to `…/scratchpad/dossier-<x>.md` with
> per-method + MEMBERS + GLOBALS + a DIFF-vs-our-source + a reconstruction PLAN. Reply with only a ~10-line summary."
⚠ **Single-writer lock:** only ONE thing touches the `re` project at a time. A dossier subagent that uses Ghidra cannot
overlap your own `re.sh`; a non-Ghidra subagent can. If you open the project in the Ghidra GUI/MCP, `re.sh`/`dc.sh` fail
on the lock (close it first). `dc.sh client`/`dc.sh mp` use separate projects (clientdll/mpdll).

### Done this session (cont.74) — FULL per-class detail + commit hashes are in the ledger; scoreboard:
**CTSGameRules 12/39 → 39/39 METHOD-COMPLETE** (5 subsystems: objective `AddObjectiveInList`/`SendObjectiveToHud`/
`ShowObjectiveOnHud`/`InitObjectives`/`SetObjective` + `Objective`/`ShowObj` gmsgs; round-restart `Init`/`RestartRound`/
`ResetRestartRound`/`RespawnEntities`/`CountPlayers`; team-cash/list `GetTeamsNum`/`SetTeamList`/`SetTeamModelList`/
`GetTeamCash`/`InitTeamScore`/`UpdateTeamScores`; "TheOne" `IsCTB`/`SelectRandomTheOne`/`SetPlayerTheOne`/`TheOneIsDead`;
voting `PlayerVote`/`CheckVotes`). **Whole OBJECTIVES CLUSTER end-to-end** (CTSObjective base → Bomb/TeamEscape/Hack/
Manager/Pointer/WinGiver all derive it w/ Save/Restore; the HUD wiring through the gamerules subsystem). **CBaseEntity
70→72/85** (added the original's `IsObjective`/`EnableObjective` virtuals). Plus **3 classes→1:1 + 3 keepalives dropped**
(CTSWorldGun cluster, CM61Grenade, CBreakable), a small-class batch (CTSPowerUp/SlowMotionPoint/Briefcase/DMHill), and
**CTSGun 28→30/47 (STARTED)**. Metrics: invented methods **131→117**, file mismatches **23→22**, **0 missing / 0 invented
classes**. ~37 commits; every one builds `scripts/build.sh amd64 all`, glibc 2.14. (memory `audit-py-blind-spots`,
`members-globals-signal-hidden-logic` saved.) The bullets below are the EARLY-session detail; the objectives cluster +
CTSGameRules are in the ledger (cont.74 entries).
- **CTSWorldGun cluster 4/13+3/7 → 13/13+7/7** (`0df448f`): merged ts_ammopack.cpp INTO ts_worldgun.cpp (+ts_worldgun.h)
  = TS_worldgun.cpp file parity. **CTSWorldGun now derives CTSPhysicObject**; real settle/bounce machine (CreateWeapon
  static factory carrying the full s_weapon_status = our **TSItem**; MOVETYPE_BOUNCE; Fall swept-trace impact damage;
  ShouldCollide pass-through-players wired via a new **gNewDLLFunctions pfnShouldCollide** hook); classname "WorldGun".
  CTSAmmoPack carries ARMOR (TakeArmor). Drop helpers in ts_gun.cpp guarded `#ifndef CLIENT_DLL`.
- **CM61Grenade 0/6 → 6/6** (`2dffd07`): real chain **CM61Grenade : CTSGrenade : CTSPhysicObject**; CreateGrenade/Spawn/
  WaitThink/BounceTouch/Use/ObjectCaps. **Dropped the ts_grenades_keep_physic keepalive** (CTSGrenade kept via inheritance).
- **CBreakable 17/22 → 22/22** (`d63559d`): the TS breakable-RESPAWN system, all additive in TAILS so the playtest-
  confirmed glass/debris (gmsgTSCreateDebris) Die path is untouched. Killed override (attacker global → Die; TakeDamage
  now calls Killed ALONE, was Killed+Die → double-Die), BreakableRespawn(bool) box-clear retry (@0x64e84), Spawn creates+
  cross-links the CInfoBreakable helper (respawntime>=0), Die's tail hides+arms it (default 30s, 5s retry if blocked).
  **CInfoBreakable: added LINK_ENTITY_TO_CLASS(info_breakable), RespawnThink delegates to BreakableRespawn(false), DROPPED
  the ts_func_break_keep keepalive.** New members respawnhealth/hRespawner/respawntime/respawntarget (not saved); keyvalues
  respawntime/respawntarget. (3 keepalives now dropped/slimmed; only the legit compiled-but-unlinked pistols remain.)
- **⚠ KEY RE CORRECTION:** CTSPhysicObject's curated trio (`m_flExpire@0x88`/`m_pOwner@0x8c`/`m_iType@0xa0`) is a
  **curator MIS-HOIST** — they're grenade-subclass members (m_flExpire=fuse limit, 0x8c=fuse-elapsed counter NOT an
  edict, 0xa0 spurious) that COLLIDE with CTSWorldGun's 0x88/0x8c. **Do NOT add them to the CTSPhysicObject base.**
  (Verified via decompile, not the label — the `members-globals` rule in action. Consider fixing `curate_members`/the
  inventory so audit stops reporting CTSPhysicObject 5/10 + CTSGrenade/CM61Grenade member gaps from this artifact.)

- **CTSGun 28/47 → 30/47** (`a6a2669` + chunk3a): STARTED the central weapon class's name-map restructure in SAFE
  chunks (it's client-predicted + heavily playtested). Driver = `scratchpad/dossier-CTSGun-namemap.md` (the OUR→REAL
  map + A/B/C/D groups + chunk order). Done: CloseCombatBlock→CCombatBlock (rename), the 2 missing NAMED members
  m_pActiveItem/m_pActiveInfo (synced by UpdateActivePointers), ShouldWeaponIdle override. **Continue the dossier's
  chunks 3b→6 IN ORDER** (next: CCombatIdle/EmptySound/Reset/HolsterWeapon need the g2.c/g3.c decompiles; then the
  DropWeapon/SelectWeapon reshapes; then the melee split + SecondaryAttack keystone). Group D = KEEP (legit glue/stock
  virtuals, NOT drift): CanDeploy/Get+SetWeaponData, OwnedCount/SlotsUsed/Sync+LoadActiveFromItem/RegenBlockBar/
  SwitchBarrel/SendWeaponStatus/UpdateActivePointers.

- **Small-class batch** (`3f81e4a`,`5e56977`): **CTSPowerUp 11/11**, **CTSSlowMotionPoint 5/5**, **CTSBriefcase 6/6**, all
  0-invented. **Reusable pattern — invented ObjectCaps:** many TS classes override ObjectCaps where the original has
  none (decompile shows no ObjectCaps in the method set) → REMOVE ours (the FCAP diff is inert for runtime-created /
  MP-only entities). Also name matches (Create→CreateSlowMotionPoint; powerup members → poweruptype/pwupduration/pwupmult).
  ⚠ **audit.py our_members() only scans .h files** — classes declared in a .cpp report "0 ours / MISSING named: all"
  FALSELY (CTSPowerUp/CTSFlag/CTSSlowMotionPoint/CTSBriefcase/…); verify member gaps by reading the .cpp, don't trust it.

- **CTSDMHill 7/7** (`179777b`): reshaped to the original's touch-based king-of-the-hill (WaitTouch holder tracking +
  WaitThink + Save/Restore + member name match). **CTSObjective 7/7** (`b878bb9`): the objectives-cluster BASE
  (CanInteract×2 / Freeze / IsObjective / EnableObjective / Save/Restore). ⚠ **Objectives-cluster structural finding:**
  the original's CTSBomb/CTSHack/CTSTeamEscape DERIVE CTSObjective; ours are CBaseEntity-direct with DUPLICATED
  attacker_team/CanInteract → the cluster reconcile (make them derive CTSObjective + build CTSGameRules' objective
  methods they call) is the next bottom-up step, interdependent with CTSGameRules 12/39. (Decompiles: scratchpad/obj.c, hill.c.)

### NEXT — work-list (bottom-up, audit.py-driven). `reference/coverage/AUDIT.md` headline: 0 missing / 0 invented classes.
1. **CTSGun tail (IN PROGRESS, 30/47)** — resume the name-map dossier `scratchpad/dossier-CTSGun-namemap.md` (the OUR→REAL
   map + A/B/C/D groups + chunk order; captured decompiles `scratchpad/g1.c`/`g2.c`/`g3.c`). Chunks 1–3a are DONE
   (CloseCombatBlock→CCombatBlock, the 2 named members m_pActiveItem/m_pActiveInfo, ShouldWeaponIdle). ⚠ **The remaining
   chunks were BLOCKED — re-check now:** `Reset`@0x805e0 needed a CTSGameRules reset path (✓ NOW available — `RestartRound`/
   `Init` exist; wire `Reset` into them); `CCombatIdle`@0x85e54 plays `get_weapon_info(0)->[wi+0x252]` (the active-anim-
   block idle index — needs the weapon-table data extraction, else a VERIFY guess); `EmptySound`@0x71be4 plays a predicted
   client event (needs the event registered); `HolsterWeapon`@0x85698 + the chunk-4 `DropType→DropWeapon`/`SelectWeaponType
   →SelectWeapon` reshapes move `Deploy` out to every caller of the playtested weapon-switch flow (wants a playtest). The
   melee split (`KnifeFire`/`KatanaFire`/`KatanaThrow` from MeleeAttack+ThrowKnife) + the `SecondaryAttack` keystone is the
   deepest reshape, do last. Group D = KEEP (legit, not drift): CanDeploy/Get+SetWeaponData, OwnedCount/SlotsUsed/Sync+
   LoadActiveFromItem/RegenBlockBar/SwitchBarrel/SendWeaponStatus/UpdateActivePointers.
2. **⚠ CTF CLUSTER — bottom-up, NOT one class.** Dossier `scratchpad/dossier-CTSFlag.md`. CTSFlag derives CTSPhysicObject
   (reuse the CTSWorldGun settle machine) BUT the carry model differs: pickup copies an `s_flag_status` onto the player
   (`CBasePlayer::TakeFlag`@0x93830) + **Destroys** the flag; a drop respawns a fresh `ts_flag` via `CreateFlag`. Our
   "flag follows the carrier" `CarryThink` has NO original equivalent. Couples to a SEPARATE **CTSBriefcase** (`ts_briefcase`
   FGD entity — owns `FlagFall` + the flag registry). **Build order:** (1) `s_flag_status` struct + a CBasePlayer member;
   (2) `CBasePlayer::TakeFlag`/`DropFlag(bool)`@0x8334c; (3) **CTSBriefcase** (FlagFall/CaptureTouch/WaitThink/CanInteract/
   registry); (4) **CTSFlag** (15 methods incl. embedded s_flag_status@0x90, m_hBriefcase@0xa4 = handle to the CTSBriefcase
   NOT the carrier; gmsg ScoreInfo on capture +5 frags). Remove invented free helpers TSCreateFlag/TSDropFlag/
   TSPlayerHasFlag/TSReturnCarriedFlag. NB our existing CTSBriefcase (ts_briefcase.cpp, 6/6) is the SLICE version — the
   real one owns the registry the flag uses.
3. **The ~117 "invented methods"** in `AUDIT.md` — REVIEW each (most are legit SDK virtual overrides like ItemPostFrame/
   GetWeaponData, or our TS team-helper names like AssignToTeam/IsTeamMode/GetIndexedTeamName that the original has
   inlined/under other names; some may be drift to rename/remove — confirm vs the RE before touching). This + the member-
   name matching is where the user's "mismatch flags other mismatched logic" rule pays off.
4. **CTSGameRules SLICEs to firm up** (all VERIFY-flagged in the cont.74 commits): RestartRound's loadout cash-refund +
   force-class-select-observer (we respawn directly); ResetRestartRound's per-team cash + TeamScore broadcast; the voting
   ff/teamplay console commands + applying the voted next-map; the_one's exact powerup value (8) + SetPlayerSpeed; IsCTB's
   game-mode-byte gate. The class is METHOD-complete (39/39) but these internal behaviors are slices.
5. **Client-side structural parity = a SEPARATE MANUAL track** (`audit.py` is SERVER-ONLY). Use `dc.sh client` + the
   `clientdll` Ghidra project. NEW this session: the objective subsystem added the **`Objective`/`ShowObj`** user-messages
   (+ the bomb's `Wires`) that need a cl_dll HUD handler (world marker + label per idx; toggle visibility). Also the
   per-fire-type client handlers (EVENTS.md). **CTSGun is client-predicted** — when you reshape its fire/melee methods,
   build `all` and update `cl_dll/ts_gun_stubs.cpp` if you change a server-only signature.
6. File mismatches 22 (mostly the original's coarser TS_*.cpp grouping vs our per-class files — OK; single-class renames
   are free wins).

### ⚠ EVERYTHING this session is UNPLAYTESTED — flag to the user BEFORE merging to `main`
The entire CTSGameRules round/objective/team/TheOne/voting flow, the whole objectives cluster, the dropped-weapon bounce
physics + swept impact damage, AmmoPack armor, CM61Grenade slow-mo + the recovered M61 blast literals (damage 300 / radius
1024), the breakable respawn, and the CTSGun chunks all need a live pass. The user fires changes live in graphical xash
(see `test.sh`); the original TS runs in `hl.exe` (Proton/Wine) for live gdb traces (`scripts/live.py`, memory
`live-original-ts-debuggable`) when a runtime/"what actually happens" question comes up.

### Task-list entries to recreate (the cont.74 tracker — recreate via TaskCreate):
- `[in_progress]` **"Per-class 1:1 parity loop (audit→RE→reconcile→build→audit→commit)"** — THE recurring driver task; keep it open.
- `[pending]` **"CTSGun tail toward 1:1 (30/47 — name-map chunks 3b→6; re-check the now-unblocked Reset)"**.
- `[pending]` **"CTF cluster (s_flag_status → CBasePlayer::TakeFlag/DropFlag → CTSBriefcase → CTSFlag) — bottom-up"**.
- `[pending]` **"Review the ~117 invented methods (confirm legit-helper vs drift vs name-mismatch)"**.
- `[pending]` **"Client-side parity: HUD handlers for Objective/ShowObj/Wires + the per-fire-type events (dc.sh client)"**.

---

## ⭐⭐⭐⭐⭐⭐ START HERE (cont.73, 2026-06-28) — SOURCE-PARITY RESTRUCTURE: ALL 24 missing classes CLOSED (24→0)

**You are the orchestrator. Continue exactly as I (the previous cont.73 agent) was working.** This session executed the
code-restructure to source parity that cont.72 set up. Branch **`restructure-source-parity`** (NOT merged to `main` yet —
21 commits `69c21ab`→the ledger commit; every commit builds `scripts/build.sh amd64 all`, glibc 2.14). GPG signing is OFF
for this work (`git config --local commit.gpgsign false`) — keep it off so commits don't prompt.

### The mission + method (this is your job too)
**Driver = `scripts/coverage/audit.py`.** Goal: make OUR code match the original's structure 1:1 — every method + variable
the original has, none it doesn't (for pure-TS classes; stock SDK classes keep their stock surface). The user's exact
directive, which is YOUR directive:
> "our code has a lot of working logic — it was structured based on the RE, but the layout is wrong a lot of places…
> Start with ONE CLASS AT A TIME, fully make it match 1:1 for methods, variables. Nothing missing, nothing added. This
> includes all missing-class closures. If there are dependent classes/methods, implement those FIRST (bottom-up: build
> ones without dependencies, then ones with resolved dependencies)."
Extra rules the user gave: **unused member variables = missing/mismatched logic → flag + FIX immediately** (don't leave
dead members — see the CTSGroundWeapon `CashBased()` buy-gate fix). **Keep going until ~90% context.**

### ⭐ THE LOOP (per class) — repeat this
1. `python3 scripts/coverage/audit.py <Class>` → its MISSING (orig has, we don't) + INVENTED (we have, orig doesn't)
   methods/members + whether our file matches.
2. **RE it.** For a SMALL class (≤~5 methods): `scripts/ghidra/re.sh <Class> > scratchpad/<Class>.decomp.c` and read it
   yourself. For a BIG class or a CLUSTER: **spawn a subagent that decompiles via `re.sh` and writes a `.md` DOSSIER to
   scratchpad** (`/tmp/claude-1000/-home-stevenlafl-Projects-goldsrc-net-mod-ts/<session>/scratchpad/`), then YOU read the
   dossier — this keeps raw decompiles out of your context. **⚠ Ghidra `re` project is single-writer — run only ONE
   `re.sh`/dossier-subagent at a time** (serialize; a dossier subagent that uses no Ghidra can overlap an impl pass).
3. Reconcile OUR working logic into the original's exact method/member layout. Build `scripts/build.sh amd64 [all if the
   class is in client.so too — CTSGun/CTSAnimation are]`. `audit.py <Class>` to confirm 0/0. Commit (per class or tight group).

### ⭐ THE SUBAGENT-DOSSIER PATTERN (use this — it's how the whole session scaled)
For each cluster I spawned a `general-purpose` subagent with a prompt like: "READ-ONLY RE. Decompile <classes> via
`scripts/ghidra/re.sh <Class>` (the typed `re` project is already built — do NOT `re.sh setup`; run ONE at a time).
Capture to scratchpad files + grep — don't paste raw decompile in your reply. WRITE a tight markdown dossier to
`scratchpad/dossier-<x>.md` with, per class: confirmed BASE, LINK_ENTITY_TO_CLASS name, each method's behavior (members
w/ offsets, strings/consts, control flow, gmsg wire format), and a diff vs our source. Reply with a ~6-line summary."
Dossiers I produced (REUSE them): `dossier-hldmcompat.md`, `dossier-worldgun.md`, `dossier-emitter.md`,
`dossier-CTSAnimation.md`, `dossier-CTSStunt.md`/`-callers.md` (scratchpad — may be gone next session; re-spawn if so).
**A live RE/live-debug consult is available — see the user's message; ping the previous agent with questions.**

### ⚠ GOTCHAS to carry forward
- **Serena one-line-JSON gotcha:** do NOT read the ledger via `read_memory("ledger")` (MCP) — it returns one giant
  single-line `{"result":"…"}` (~150 KB+) that overflows the read limit. Read **`.serena/memories/ledger.md` directly**
  with the Read tool (or `jq -r '.result'` on a saved file). Same for any large memory.
- **audit.py member detector (FIXED this session):** `our_members()` now also detects non-`m_`-prefixed members (the
  original's FGD/save names are unprefixed, e.g. `tsweaponid`/`respawntime`). If a faithfully-named member shows
  false-"missing", that's the area.
- **KEEPALIVE pattern:** a class the original COMPILES but our `--gc-sections` drops (unlinked + unreferenced) needs an
  EXPORTED never-called anchor referencing it (`hldmcompat_keep_unlinked`, `ts_grenades_keep_physic`, `ts_func_break_keep`).
  Honest stopgap; the REAL fix is to build its hierarchy/caller so the anchor can be deleted (see "follow-ups").
- **LINK conflicts:** the stock SDK weapon/ammo files LINK the same entity names the hldmcompat TS spawners need — I
  commented out those stock `LINK_ENTITY_TO_CLASS` lines (10 files) so the TS spawners own the names (the original has no
  stock HL weapons). Watch for the same with future stock-name remaps.
- **⚠⚠ EVERYTHING this session is UNPLAYTESTED.** The worldgun pickup physics, stock-name weapon/ammo remap, slow-bullet
  refactor, grenade bounce, particle/wind system, and the CTSAnimation/CTSStunt restructures all need a playtest pass
  (the user fires changes live in graphical xash — see `test.sh`). Flag this to the user before merging to `main`.

### What's DONE (24→0 missing classes; 7 drifted → 1:1)
- Drifted→1:1: **CTSAnimation** 13/13, **CTSStunt** 25/25, **CTSDeadCorpse** 5/5 (+file rename), **CTSShrapnel** 6/6
  (+ts_grenade→ts_grenades), **CTSPlayerAmount** 4/4, **CTSTrigger** 5/5, **CTSScoreGiver** 3/3.
- Missing→built (bottom-up): **CTSGroundWeapon** 14/14 (+Save table +the `CashBased()` buy gate), **CTSGroundAmmo** 11/11,
  the **16 hldmcompat** weapon+ammo spawners, **CTSPhysicObject** + **CTSGrenade**, **CBulletManager** (wrapped from free
  fns), the **emitter system** (CTSSharedEmitter/CTSEmitter/CTSParticle/CTSWind + gmsgTSParticleSystem), **CInfoBreakable**.
- Tooling: the `audit.py` member-detector fix.
Full per-class detail + commit hashes are in the **Serena ledger** (`.serena/memories/ledger.md`, entries `cont.73`,
`cont.73 cont.`, `cont.73 cont.2`, `cont.73 cont.3`). Read it first.

### What's NEXT (your work-list, audit.py-driven) — `reference/coverage/AUDIT.md` headline: 0 missing / 0 invented classes
1. **Drifted classes' MISSING methods** (the big remaining gap; one at a time, bottom-up): **CTSGameRules 12/39**,
   **CTSGun 28/47**, **CM61Grenade** (rename M61WaitThink→WaitThink etc.), **CTSFlag**, **CTSWorldGun 4/13** (rederive
   **CTSPhysicObject** — that also lets you DROP its keepalive), **CBreakable 17/22** = the **breakable-respawn cluster**
   (`BreakableRespawn` @0x74e84 + `Die` creating/arming `CInfoBreakable` + the respawn-time members → drops CInfoBreakable's
   keepalive; ⚠ touches playtest-confirmed glass/breakable code). The objectives cluster (CTSObjective/Manager/Pointer/Hack/
   WinGiver) is INTERDEPENDENT with CTSGameRules' objective methods — do those together.
2. **~131 "invented methods"** in `AUDIT.md` — REVIEW each (most are legit SDK helpers like GetWeaponData/ItemPostFrame;
   some may be drift to remove/rename — confirm vs the RE before deleting).
3. **DROP the keepalives** by building the real wiring (CTSWorldGun:CTSPhysicObject, grenades:CTSGrenade,
   Die→CInfoBreakable, a CTSPistolWeapon referencer if ever linked).
4. **23 file mismatches** in `AUDIT.md` — single-class renames are free wins (took corpse/grenades this session); the
   grouped `TS_entities/objectives/worldgun.cpp` merges are OK to defer per the original's coarser file grouping.
5. **Client-side structural parity** is a SEPARATE MANUAL track — `audit.py` is SERVER-ONLY (`ts_i386.so`). The per-fire-type
   client handlers (EVENTS.md) etc. need `dc.sh client` + the `clientdll` Ghidra project (see cont.72 §4b).
6. When the branch is solid + playtested: merge `restructure-source-parity` → `main`.

### Task-tool entries to recreate (the session's tracker):
- `[done]` hldmcompat spawner family · CTSAnimation · CTSStunt · emitter/CInfoBreakable.
- `[ongoing]` **"Per-class 1:1 parity loop (dossier .md → reconcile → build → audit → commit)"** — this is THE recurring
  task; keep it open and keep feeding it the drifted classes above.

---

## START HERE (cont.72, 2026-06-28) — RE-TOOLING PASS (committed) + NEXT: refactor our code to the original layout

This session built out the **machine-checkable RE inventory + audit toolchain** and made it deterministic. Almost
everything is committed (10 commits `77cb586`→`1f839c8` on `main`). **NEXT SESSION'S JOB: use the audit tools to
RESTRUCTURE our code around the original's layout — move/rename classes/methods/files to match `TS_*.cpp`/the
original class+method names, closing the gaps `audit.py` reports, to get as close to source parity as possible.**

### 🎯 The next-session task: code restructure to source parity (driven by `audit.py`)
- **`scripts/coverage/audit.py`** is the driver. It diffs OUR build vs the original structure (`ts_inventory.json`),
  BOTH directions, at file/class/method/member/global level → `reference/coverage/AUDIT.md`.
  - `audit.py` → the full report. `audit.py <Class>` → one class's MISSING (orig has, we don't) + INVENTED (we
    have, orig doesn't) methods+members + whether our file matches the original's. `audit.py --strict` → CI gate.
  - Current state to chip away at: **24 missing classes** (`CBulletManager`, `CTSGrenade`, per-weapon subclasses
    `CTSPistolWeapon`/`CTSShotgunWeapon`/…), **211 missing methods**, **54 missing named members**, **25 file
    mismatches**, **24 missing globals**. **0 invented classes** (good — we fabricated none). **145 invented
    methods** = REVIEW (most are legit SDK-required helpers like `GetWeaponData`/`ItemPostFrame`; some may be
    misplacements/drift — confirm each against the RE before keeping).
  - Workflow per class: `audit.py <Class>` → read the original via `re.sh <Class>` (typed decompile) or the MCP →
    move/rename to match → rebuild → `audit.py <Class>` again to confirm the gap closed. The original groups classes
    by `TS_*.cpp` file; our files are finer-grained (per-class) — match the NAMES, file-grouping divergence is OK.
- **⚠️ SCOPE: `audit.py` is SERVER-ONLY** (`ts_i386.so`'s file→class→method tree). `client.dll` has no
  symbols/RTTI/file-map, so **client-side STRUCTURAL divergences are invisible to it** — only client member
  *layouts* are recovered (`client_members.py`/`members_auto_client.tsv`). Client source-parity is a SEPARATE,
  MANUAL track (RE the client via `dc.sh client` + the `clientdll` Ghidra project). **There is NO TS Linux
  `client.so`** (it would've shipped unstripped like the server `.so` — don't go hunting for one). The Windows
  `reference/client.dll` (MSVC release: only ~505 exports named, NO RTTI, NO STT_FILE map — symbol-poor by
  format/build, not by our copy) is the ONLY client reference. So `binmap`-matching OUR built `client.so` →
  `client.dll` (`client_members.py`) is THE definitive client method, not a fallback. **Concrete example found
  this session:** the original splits the CLIENT fire path into PER-FIRE-TYPE handlers (dispatcher `FUN_1003de40`
  → pump `FUN_10040c20` / akimbo `FUN_1003f090` / standard `FUN_1003dfa0`), each computing its OWN recoil; WE
  collapsed all fire into ONE `EV_TSFire` (only `fire.sc` is wired — `EVENTS.md` cont.69s) and pass the recoil
  magnitude predicted-side via the event `fparam2`. Faithful behaviour, divergent structure — `audit.py` won't
  flag it. The 11 `.sc` events (EVENTS.md) splitting into faithful per-type client handlers is the client-side
  parity work-list.
- **The inventory it audits against** (all deterministic + complete now): `ts_inventory.json` = file→class→
  {methods (sig/addr/assets/cvars/cmds), members (saved+curated+auto, with `source`), vtable, hierarchy} + globals
  + the `client` HUD member skeleton + the external `contract`. `coverage.py` audits the external contract
  (cmds/cvars/msgs/events) separately; `audit.py` + `coverage.py` together make the WHOLE json checkable vs our code.

### ✅ COMMITTED this session (`77cb586`→`1f839c8`) — the RE toolchain
- **`re.sh vrefs <Method>`** (`vrefs.py`): reverse VIRTUAL-call finder (vtable dispatch defeats `refs`/`get_function_callers`).
- **`curate_members.py`**: producer for `members_re.tsv` (was an orphan) — scrapes our source's RE'd offsets,
  cross-checked vs the binary skeleton. Wired into `re.sh members`.
- **member-inventory completeness**: `DumpMembers` now parses the opaque `*(t*)(this+0xNN)` form → recovers the big
  transient-only classes (CTSGameRules/CTSStunt/CTSAnimation) that were invisible. 860→1030 rows / 137 classes.
- **`s_weapon_info`/`s_weapon_status` structs** applied → weapon code reads `this->m_pActiveInfo->iFireType` as source.
- **client member skeletons** (`client_members.py`): binmap-match our `client.so`→`client.dll` → type `this` →
  DumpMembers → `members_auto_client.tsv` (51 classes / 1319 members incl. 11 `CHud*`). `ApplyClientThis.java`.
- **`binmap.py`**: `.so`↔`mp.dll` (and client) function matcher (export + string-anchor) → resolve a name to a live
  RVA for `live.py break`. Env-parameterized (`BINMAP_SO/MP/GOT/MAP/OUT`).
- **DETERMINISM**: the member pipeline is now 0-diff across runs. Cached single-threaded `re-base` (`re.sh analyze`,
  built once, COPIED per `setup` — also faster), `-max-cpu 1` on setup/members (member SET), and a CANONICAL type
  column in `DumpMembers` (the integer width/signedness guess wobbles in Ghidra's decompiler, unreachable by
  `-max-cpu`; float/double/ptr are stable → key off presence, collapse integers to `int`).
- **`ts_inventory.json` complete**: per-class members folded in (saved+curated+auto, 1402; was 327 saved-only) + the
  client skeleton + corrected the stale "HUD unrecoverable" note. **`audit.py`** (the driver above).
- **Ghidra MCP** is set up as an interactive front-end on the typed `re` project — see CLAUDE.md §4a.3 (+ the `.lock`
  gotcha) and the `ghidra-mcp-vs-own-tooling` memory. `scripts/ghidra/mcp-up.sh` = the connect preflight.

### 🟡 UNCOMMITTED / IN PROGRESS — pump-action shotgun (gameplay; `dlls/ts_gun.{cpp,h}`, `ts_weapons.cpp`)
Held for the user's playtest (NOT committed). Current disk state = the **RE-confirmed faithful model**:
- **HELD / auto-alternate** (NOT press-edge). RE (`ShotgunFire`@0x82108 + `PrimaryAttack`@0x81a20 + `TakeWeapon`
  default-mode = AUTO>PUMP>BURST>SEMI): all 3 pump shotguns DEFAULT to PUMP mode; `ShotgunFire` ALTERNATES a
  fire-step / rack-step via the toggle (`item+0x24`, stored in the predicted `m_fInSpecialReload`). So holding =
  shot→rack→shot→rack; a TAP = one step (= "click to pump"). Same code, both behaviours.
- Rack sound on **CHAN_ITEM** (so it doesn't cut the CHAN_WEAPON gunshot). Pump anim seq 8 (m3/spas12) / 2 (mossberg).
- ⚠️ **OPEN FIDELITY FIX (flagged, NOT done):** the original **reduces pump-shotgun VKickback to 0.35×**
  (`ShotgunFire`: `fVar12 = ((iFireModes&3) && fireMode!=1) ? 0.35 : 1.0; VKickback(fVar12*flViewKick,…)`). Ours
  routes through `StandardFire` → FULL `flViewKick`, so **our pump recoil is ~3× too strong.** Easy fix; the user
  will feel it. Also minor: semi-mode rate (orig 0.15 vs our flFireRate) and the DoubleFireRate /1.5 double-apply.
- **Always `scripts/build.sh amd64 all`** (CTSGun is in BOTH hl.so + client.so) and never gate FIRE on non-predicted
  state. See `[[predicted-weapon-gotchas]]`.

### Task-tool entries: NONE (this session ran without TaskCreate; the open work is the two bullets above).

## ⭐⭐⭐⭐⭐ START HERE (cont.71 cont., 2026-06-27) — long live-playtest pass: laser/HUD/weapons (MOSTLY COMMITTED) + pump-action (IN PROGRESS)

A very long interactive live-playtest session driven entirely by the user firing each change in the graphical
xash. The user runs the game himself and reports; **build BOTH libs every time** (`scripts/build.sh amd64 all`)
— `dlls/ts_gun.cpp` compiles into BOTH `hl.so` AND `client.so` (CTSGun is client-predicted), so a server-only
build leaves the client predicting stale behaviour (this caused a "pump infinite-fires" red herring). Then
`scripts/setup-xash-mod.sh` → launch (`+deathmatch 1 +maxplayers 8` REQUIRED). `test.sh` = the user's launcher,
DO NOT commit. The previous agent (me) keeps the ORIGINAL TS running in hl.exe for live gdb traces (scripts/live.py).

### ✅ COMMITTED this session (commits `8c27afa` → `0b45726` on `main`)
- **8c27afa** lasersight map-change persistence (client no longer clears `g_playerAttach` on VidInit) + per-weapon
  attachment binding (buy menu emits `tkw`/`tki` INTERLEAVED so a laser lands on its weapon, not the last one).
- **45b1120** `useitems` = ATTACHMENT TOGGLE (not akimbo swap; akimbo swap = right-click) — RE `FUN_0x71564` state
  machine, item +0x10 equipped / +0x14 active; ActItems broadcasts ACTIVE, WeaponInfo flags = EQUIPPED. Katana
  right-click THROW; stuck katanas NOT pickup-able; throw caps knife 5/seal 10/katana 1. Melee BLOCK BAR =
  gmsgKFuPower stamina (all melee block on RELOAD; bar shown only for melee, not health). See [[useitems-is-attachment-toggle]].
- **c5bceef** numbers.spr digit font (health/powerup/bottom-box ammo/top-bar slots) + bottom-box attachment list
  (item_installed/not_installed dots) + the melee bar fades colour (not threshold switch).
- **487d592** laser/flashlight/muzzle visual pass: local laser is a HUD dot (others' = world beam+dot; RE
  `FUN_1003d600` skips local), beam uses `laserbeamanim.spr`; flashlight = light only + `flash.spr` on the gun;
  muzzle flash = `gun_muzzle.spr`(sil)/`gun_muzzle2.spr`; **viewmodel aims UP with ground move speed** (view.cpp).
- **5dcdd26** laser dot screenspace-anchored (eye-ray → distance-independent, stays centred up close); halved
  weapon sway; removed the fabricated weapon-switch sound.
- **0b45726** weapon UNHOLSTER anim on switch (per-weapon draw-seq table parsed from viewmodels + viewmodel-change
  replay force for same-index pairs e.g. MP5K/AK47=2); shotgun PELLET DECALS (client traces all 8 pellets);
  spectator top-left inset (pip/overview) forced OFF (`spec_pip` 0).

### 🟡 IN PROGRESS — UNCOMMITTED ON DISK (pump-action shotgun; builds clean both libs)
Pump shotguns (Benelli/SPAS/Mossberg, fire-type `TSFT_PUMP`) — `dlls/ts_gun.{cpp,h}` + `ts_weapons.cpp` precaches.
- **WORKS:** fire is now ONE SHOT PER PULL (moved `TSFM_PUMP` from the `held` to the `pressed` branch in
  PrimaryAttack — same path as semi, which predicts cleanly). No more full-auto / infinite-fire.
- **NEEDS PLAYTEST CONFIRM:** the cosmetic RACK (pump anim + sound between shots). First attempt used a custom
  `m_flPumpRackTime` timer — that FAILED because **client prediction only decrements the STANDARD weapon timers**
  (`m_flNextPrimaryAttack`/`m_flTimeWeaponIdle`/`m_flNextSecondaryAttack`), not custom fields, so it never tripped
  client-side. Reworked to a cosmetic `bool m_bNeedRack` + the PREDICTED `m_flTimeWeaponIdle` (StandardFire sets
  needRack + idle=+0.12; WeaponIdle plays the rack: SendWeaponAnim(pump seq 8/8/2) + rack sound server-only to
  avoid a host double). **Just built (pid was 808684), the user hasn't confirmed the rack shows yet** — verify it
  racks between shots; if not, the next lead is that WeaponIdle only runs when the trigger is RELEASED (our
  ItemPostFrame, the `!(button & ATTACK|ATTACK2)` branch), so a held trigger won't rack — drive it from the fire
  path or an event if needed. ⚠️ **Hard lesson (twice this session): NEVER gate the FIRE on non-predicted state —
  the first pump attempt (an `m_bPumped` chamber state machine gating Fire) desynced and infinite-fired. Keep
  pump cosmetics cosmetic.** The dead `m_bPumped`/`PumpAction` scaffolding was already removed.

### 🔴 OPEN / DEFERRED (user-flagged, not done)
- **Pump rack** — confirm + polish (above). Decide commit once the rack is verified.
- **Benelli default fire-mode** is `3` (TSFM_PUMP) though its `iFireModes` mask (0x23) doesn't include bit 3 —
  TSDefaultFireMode picks an out-of-mask mode. Works now (PUMP=pressed) but the mode-select RE is loose.
- Earlier deferred: glass double-pane penetration; golden-colts no-fire-sound-until-reload; 3rd-person stunt poses
  (need the TS specialist player models, not in this data extraction). See the older START HERE sections below.

### Verified-faithful via RE this session (no change needed)
- UMP is genuinely a **2-round burst** (`PrimaryAttack`: `burst = (type != 0x17) + 2`); everything else = 3.
- Double-fire powerup = **1.5×** fire speed (`PostThink@0x81610`: `timer *= 1.5`), not 2×.

### Task-tool entries: NONE open (TaskList empty).

## ⭐⭐⭐⭐ START HERE (cont.71, 2026-06-26) — CTSStunt (COMMITTED) + DIVE RETICLE + numbers.spr (UNCOMMITTED)

Two big things this session. Read the two newest `.serena/memories/ledger.md` entries (cont.70 cont. = CTSStunt;
cont.70 cont.2 = reticle). Build: `scripts/build.sh amd64 all` (clean, glibc 2.14) → `scripts/setup-xash-mod.sh`
→ launch (the user playtests; `+deathmatch 1 +maxplayers 8` REQUIRED). `test.sh` = the user's launcher, DO NOT commit.

### ✅ COMMITTED this session
- **CTSStunt** (dive/prone/roll/wall-climb/wall-jump/bullet-time) FULLY RE'd + reconstructed: `dlls/ts_stunt.{cpp,h}`
  + pm_shared TS hooks (predicted hull/view/launch via `pev->iuser4`). Commits **b8080fb** (cont.70 checkpoint:
  attachments/powerups/sounds) + **514a50a** (CTSStunt). Full RE map: `reference/STUNT.md`.

### 🟡 UNCOMMITTED ON DISK (the reticle session — builds clean, on top of 514a50a)
The TS **dive reticle** (`cl_dll/ts_reticle.{cpp,h}`, wired in hud.h/hud.cpp/CMakeLists) + a **numbers.spr font**
(`TSHud_DrawNumber` in cl_dll/ts_hud.{cpp,h}). Live-playtest-tuned with the user; RE foothold in
`scratchpad/RETICLE_RE.md`. Donut crosshair (cross2) + left "(" arc of 3 weight-colored dive-tier thirds + a
pitch dot + a lasersight distance readout. Decide whether to commit (recommend yes — it's a clean checkpoint;
the laser-distance map-change bug is pre-existing-style, not a reason to hold the commit).

### 🔴 OPEN WORK-LIST (pick up here)
1. **Lasersight lost on MAP CHANGE** (the #1 reported bug). Client `g_playerAttach` resets on the new map; the
   server `ActItems` is change-detected on `CTSGun::m_iClientAttach` (survives the level) → never re-sends. A
   naive `m_iClientAttach=-1` in `Deploy()` REGRESSED it (laser gone on fresh buy) and was reverted. Fix the
   re-broadcast without breaking first-deploy (force from the post-changelevel player activate, or client
   re-request). This is a RECURRING pattern — see the `map-change-state-reset` memory + cont.70 sprite-cache fix.
2. **Wire numbers.spr into health / ammo-max / powerup (2/3/5)** — the user asked; `TSHud_DrawNumber` is ready.
3. **Reticle polish**: gate the laser readout on the laser's ACTIVE toggle (useitem; starts on) not just
   equipped; the distance divisor (`/52`→metres) + the number position next to the reticle are tuned guesses.
   The reticle draw funcs decompile to garbage, so colors/positions are screenshot-tuned, not byte-RE'd.
4. **CTSStunt 🔴 carried**: 3rd-person stunt POSES need the TS specialist player models (not in this data
   extraction — `Animate(0x28..0x39)` wired but the model lacks some); glass double-pane penetration;
   golden-colts no-fire-sound-until-reload (live-trace it).

### Open task-tool entries (this session)
See the TaskList. The reticle/numbers/map-change items are tracked there.

## ⭐⭐⭐ START HERE (cont.70, 2026-06-26) — ATTACHMENTS + POWERUPS + SLOW-MO/DAMAGE SOUNDS + STUNTS — **UNCOMMITTED ON DISK**

A very long interactive playtest session. **NOTHING IS COMMITTED** — all work is on disk on top of `6ecf77b`
(cont.69z): 21 modified files + 2 new (`cl_dll/ts_attachments.{cpp,h}`). Builds clean (`scripts/build.sh
amd64 all`, glibc 2.14). The pre-existing `reference/coverage/*` edits + `test.sh` (the user's launcher —
DO NOT commit) are NOT this session's work. **Decide whether to commit before continuing** (recommend yes —
it's a big clean checkpoint). The user runs the graphical xash himself and live-playtests every change.

### ✅ DONE this session (all RE-grounded + live-confirmed unless noted)
- **#11 silencer + scope + lasersight + flashlight (gun attachments)** — the swap contract is `fire.sc`
  `iparam2 = ATTACH MASK` (bit0 silencer), `bparam1 = clip==0` (RE StandardFire@0x824a4); barrel side moved
  to `bparam2`. Silencer → `<weapon>/fire-sil.wav` + muzzle suppression (`cl_dll/ev_ts.cpp`). Scope → FOV
  cycle `0→zoomFov1→zoomFov2→0` on secondary (RE SecondaryAttack@0x717a8 + Zoom/Unzoom); AUG/Barrett ship a
  built-in scope, MP5SD/Ruger a built-in silencer (new `s_weapon_info` fields `zoomFov1/2`,`attachSupported`
  (+0xe0),`attachBuiltin` (+0xe4)). **Lasersight + flashlight = NEW `cl_dll/ts_attachments.cpp`**: server
  broadcasts each player's attach via the original `ActItems` msg (BYTE idx, BYTE attach — RE FUN_1004fa70),
  client draws a TriAPI laser beam+dot (colour 1=green/2=blue/else=red, RE FUN_100582f0) / flashlight dlight.
  `CTSGun::AddItem`@0x80794 validates supported/built-in; buy routes via `tki`/`CBasePlayer::AddItem`.
- **Buy menu** — fixed the bit-INDEX-vs-VALUE bug in BOTH the display loop AND the mouse hit-test (pass
  `1<<bit` to AttachAvailable); Buy/Akimbo/Full are green; akimbo: 2nd buy of an akimbo-capable single
  COMBINES (shows BOTH the single + akimbo icons), RemoveSlot steps akimbo→single→gone; fresh akimbo loads
  both barrels.
- **Powerups (independent, coexist with the held slot)** — super-jump (`m_bLowGravity`) + kung-fu
  (`m_iKungFu`) granted by `PlayerAddPowerUp`, send `PwUp(type,0)` → bottom-center WHITE indicators
  (`ts_powerup_hud.cpp`). Kung-fu reload BLOCK: `CloseCombatBlock` opens a 0.5s window (`m_flKungFuBlock`),
  `CloseCombatAttack` deflects a kick on a blocker (kungfuhit.wav + 0.5s attacker stun — RE CCombatBlock@
  0x85ebc + VictimKungfuHold@0x66f50). Kung-fu powerup → secondary does a kick while armed.
- **Slow-mo presentation** — `goslow.wav` on entry via **`EV_PlaySound`** (RE FUN_1004bef0; not
  PlaySoundByName) + **4 fading heartbeats** (1.0→0.25, 0.75s apart, `ts_powerup_hud.cpp`). Tint made bluer
  (`view.cpp` G60/B160; the RE doc says the original is motion-blur not a fixed colour, so it's user-tuned).
- **Damage sounds** — `bodyhit1-4`/`headshot`+`1-2`(head)/`kevlarhit`(armored torso) on the victim in
  `CBasePlayer::TraceAttack` (RE TraceAttack@0x901cc, CHAN_BODY); `pain1-4` voice via `Pain()` in TakeDamage.
  **whizz1-5** bullet-flyby (`ev_ts.cpp TS_BulletWhizz`, closest-point-on-segment ≤56u, RE FUN_100450c0).
- **Double-fire powerup** → EVERY weapon full-auto (fire while held), incl. semi/burst.
- **Two bug fixes**: (1) impact sound "hear it as you walk up to old impacts" = the ric was
  `EMIT_AMBIENT_SOUND` on the WORLD entity (engine spawns a PERSISTENT static sound). Moved CLIENT-side
  (`EV_TSBulletImpact` → `EV_PlaySound(0, pos, …)` = entity-0 positional, distance-attenuated, one-shot).
  (2) Map-change broke ALL impact/muzzle sprites (cached `SPR_Load`/model handles go stale on a new map) —
  added `TSParticles_Reset()` + `TS_Attachments_VidInit()` + muzzle re-resolve, all called from `CHud::VidInit`.

### 🔴 INCOMPLETE — the CTSStunt system (double-jump roll / wall-climb / +alt1 dive / prone) — **NOT FAITHFUL YET**
Implemented as **ad-hoc approximations in `dlls/player.cpp` PreThink** (search `m_bDiveRolling`/`m_flWallClimbFuel`/
`m_bProneSize`). The user playtested repeatedly — **it's still wrong** ("are you sure you RE'd this enough?").
**THE CORE BLOCKER:** the dive/prone must make the player a SMALLER bbox + LOWER view that the **CLIENT
PREDICTS** (others can land on you; your eye drops to the floor). Forcing `pev->flags|FL_DUCKING` /
`UTIL_SetSize` / `pev->view_ofs` **in the DLL does NOT stick** — `pm_shared` (client prediction) recomputes
the hull/view every frame and overwrites it. **The faithful fix needs `pm_shared` work**: reconstruct the TS
dive/prone movement gated by the `pev->iuser4` stunt flags (networked), so the client predicts the small
hull/view + dive trajectory. RE already done (decompiles in `scratchpad/`):
- `CTSStunt::HandleStunt`@0x8b000 = per-frame dispatcher: `IN_JUMP held` + climbable wall → GoWallClimb
  (paced by the stunt timer `CTSStunt+0` → AUTO-climb while held, NOT re-press); `+alt1` = `player+0x815 &
  0x40` → GoDive (moving≥80) / GoBulletTime (still); wall-jump when `+alt1` + not jumping + wall.
- `GoDive`@0x8b1a0: sets `iuser4`(player+0x250) bits **0x4 (prone) + 0x10** + a dive-height bit (0x40 high /
  0x100 medium / 0x80 low, via CanDiveHigh/Medium/Low — gated on `player+0x74` v_angle pitch), sets
  `pev->gravity`(+0x120) + `pev->friction`(+0x11c) per height, zeros punchangle(+0x174), `CTSAnimation::
  Animate(0x28)`, `NextDive`→stunt timer, `gmsgTSNextStunt` + `gmsgTSMotionBlur`, `player/dive.wav`; dive
  DIRECTION (`CTSStunt+0x20`) = normalized velocity, or `v_forward` if `iuser4(player+0x252)&0x80`.
- `GoRoll`@0x8cfe8 (anim 0x2f, land.wav, prone flags `iuser4|0x40005`, GetDirection fwd/back/left/right),
  `GoWallClimb`@0x8aa1c (anim 0x38, **player/airwheel.wav**, fuel=`max(accum,0.5)`), `HandleWallClimb`@0x8b644,
  `HandleDive`@0x8b7e4, `FBecomeProne`@0xdc6f8 (`m_afPhysicsFlags|4`). Full CTSStunt method list: Init/Play/
  HandleStunt/HandleDive/HandleSlide/HandleWheel/HandleWallClimb/HandleWallJump/Landing/Go{Dive,Roll,WallClimb,
  WallJump,Wheel,BulletTime}/Can Dive{High,Medium,Low}/NextDive/CheckForWall{Jump,Dive,Climb}/CheckForBreakables.
- **NEXT STEP: reconstruct `CTSStunt` as a real class + add the dive/prone to `pm_shared` (decompile
  `PM_PlayerMove`@0xf08d8 + the duck path to see how `iuser4` flags drive `usehull`/view/movement).** The
  3rd-person stunt ANIM poses (Animate 0x28/0x2f/0x38) need the TS specialist player models — **NOT in this
  data extraction** (only `valve/player.mdl`); currently using stock `longjump`/`crouch_idle` stand-ins.

### 🔴 PENDING (user-requested, NOT started — need RE)
- **Glass DOUBLE-PANE penetration**: some calibers/guns break 2 panes, others 1; and whether the round
  DAMAGES the glass + decals the surface behind. RE the gating (likely a per-caliber penetration value vs
  the glass). Start from `ShootBulletsPlayer`@0x5f69c penetration + `CheckForBreakables` + `Die__10CBreakable`.
- **Golden-colts NO fire-sound until a reload** — gold (akimbo-only, type 21) is silent on first deploy,
  works after a reload. Suspect a client-prediction active-weapon-type sync on deploy (the predictor's
  weaponType is stale until a weapon_data update). Live-trace it.
- **Wall-climb weight** — currently scaled by `pev->maxspeed` (slot-based, a proxy); verify the real formula.

### Open task-tool entries (this session)
#4 superjump · #5 kung-fu powerup · #6 kung-fu block — all DONE. #7 slow-mo sounds · #8 tint · #9 rolls ·
#10 damage sounds — all DONE (rolls = approximation, see 🔴 above). The live work-list = the 🔴 sections.

## ⭐⭐ START HERE (cont.69z, 2026-06-26) — WEAPON-FEEL / SLOTS / GRENADE / IMPACT pass — COMMITTED + LIVE-PLAYTESTED
Committed to `main` (`--no-gpg-sign`, NO Co-Authored-By): **6f9b9d4** (this session) on top of cont.69y's
8b2a42f/635c9ca. Uncommitted on disk (intentionally NOT committed): `test.sh` (the user's launcher — DO NOT
commit) + pre-existing `reference/coverage/*` edits (not this session's work). Full RE detail in the cont.69z
ledger entries (newest two). **No task-tool entries are open** — the work-list lives in this file (🔴 below).

**Workflow this session:** the user runs the graphical xash himself and live-playtests each change ("launch but
let me do the testing" — build + `scripts/setup-xash-mod.sh` + launch, then they verify; don't drive it via
xdotool). `+deathmatch 1 +maxplayers 8` REQUIRED. The user keeps the ORIGINAL TS running for live gdb traces.

### ✅ DONE this session (cont.69z, all RE-grounded + live-confirmed — see the ledger for addrs)
- **#14 hold-position toggle** (right-click stance swap) + **#13 shotgun reload** (3-phase + per-shell racking) — both verified.
- **Weapon SLOTS → move speed** (GetSpeedBySlots@0x66bd4): 38 slot costs (s_weapon_info+0xd0)→`slotCost`;
  `TSSlotBaseSpeed` = 210+(120/81)*remaining, scaled by slow-mo in player.cpp PreThink; pickup gated by the
  81-slot capacity (AddWeapon→bool, GiveTSWeapon→NULL, ground pickup leaves it); no re-pickup of owned (except akimbo).
- **Double-firerate = 1.5x** not 2x (PostThink@0x81610). **Health powerup** over-heal decays 1/sec→max (PreThink@0x808cc).
- **Slow-mo sound pitch** split: `TSSlowPitch` (normal-recorded 100*slow) vs `TSRecordedSlowPitch` (recorded-slow
  100+300*slow). Applied to knife impacts, per-shell racking, bullet ric. **(This SUPERSEDES the old sawed-off #
  open item — the formula split is the mechanism; per-weapon assignment still tunable.)**
- **Slow bullets**: per-caliber speed (40*ammo_info+0x24), short trail dash riding the head + shotgun_pellets sprite, debris/ric ATTN_STATIC.
- **Grenade** (CM61Grenade + TSExplosion@0x6d01c): breaks+flies THROUGH glass (swept WaitThink like the knife),
  slowed-time fuse, faithful TSExplosion (shake + falloff radius dmg + screen-flash), have/don't-have ammo (vanishes when thrown).
- **Bullet-impact shower removed + decal made compat-safe**: the ~50-dot shower was the engine's `TE_GUNSHOTDECAL`
  (server `DecalGunshot`→`UTIL_GunshotDecalTrace`). VERIFIED original lays NO server decal; decal is CLIENT-side
  (`DamageDecal` FUN_10001000 → `{break`/`{shot`/`{bproof1` by classnumber/rendermode). Moved decal to client
  (`EV_TSBulletImpact`), removed server decal. ⚠️ decals.wad gotcha: bare `{shot`/`{splat` don't exist (numbered
  only); bare `{break` is a DEBUG "BREAK"-text lump → use `{shot1-5`, `{break1-3`, `{bproof1`.

### 🔴 INCOMPLETE / SLICES from THIS session (pick up here)
- **Grenade TSExplosion visual**: uses a stock `TE_EXPLOSION` fireball. The ORIGINAL sends `gmsgTSCreateDebris`
  **material 1** for a client fireball-debris effect (the client handler branch isn't written — only material 0
  = glass exists in `cl_dll/ts_debris.cpp`). Also the **slow-mo-point-on-spectacular-kill** (CreateSlowMotionPoint
  via DoSpectacularNess) is deferred. Blast **radius=400 / magnitude=110** are TUNED (the struct-Vector ABI
  obscured the exact `TSExplosion` damage/radius params — re-RE if exactness wanted).
- **Slow-mo bullet decal** still laid SERVER-side (ts_bulletmgr `DecalGunshot`=TE_DECAL) — minor compat gap vs the
  client-side instant-bullet decal; the original's CBulletManager decal path isn't pinned.
- **No bullet exit-hole decal** anymore (client traces first hit only — matches the original; flagged in case it's wanted).

### 🔴 STILL OPEN (carried from cont.69y — not touched this session)
- **#10 Double-firerate EXPIRY "permanent"** — distinct from the 1.5x RATE fix above. The held-powerup expiry/
  countdown may not clear. Get the precise symptom or live-gdb it (does the green text count down + clear?).
- **#11 BOUGHT-silencer `-sil` sounds** — silenced-base (mp5sd) done; bought silencers need the `<weapon>/fire-sil.wav`
  table in EV_TSFire + the fire event aligned (orig: iparam2=attach-mask, **bparam1**=barrel side; ours mis-packs).
- **#12 Thrown knife/katana FLYING speed** should slow in slow-mo (like the slow bullets); pitch already done.
- **#15 Akimbo L/R independent fire** (modes 4/5 "Free Auto": L-click one gun, R-click the other). AUTO-both done.
- **Scope ZOOM branch** of SecondaryAttack (hold-toggle done; the `attach&8`→Zoom path needs scope-attach plumbing).

## START HERE (cont.69y, 2026-06-25) — POWERUP / SLOW-MOTION / SLOW-BULLET / WEAPON work — COMMITTED
A large session. Two commits on `main` (`--no-gpg-sign`): **8b2a42f** (kung-fu KICKS dispatch) + **635c9ca**
(powerup/slow-motion subsystem, slow-mo bullets, weapon fixes). Still uncommitted on disk: `test.sh` (the
user's launcher — DO NOT commit) + pre-existing `reference/coverage/*` edits (not this session's work).

**The user is doing a long live-playtest loop** (graphical xash, see CLAUDE.md §3 — `+deathmatch 1
+maxplayers 8`). Test powerups with the `powerup <N>` cheat: 6/anything=Slow-Motion, 1=Slow-Pause,
5=Double-Fire, 2=grenade, 3=health, 4=inf-clip. Double-tap +use (E) activates a held timed powerup.

### ✅ DONE this session (RE-grounded; the RE maps are in `scratchpad/`)
- **Kung-fu KICKS** (commit 8b2a42f): RE-faithful CCombatAttack port — see the cont.69y kung-fu section below.
- **Powerups + slow-motion** (RE: `scratchpad/POWERUP_RE.md`, `SLOWMO_PRESENTATION_RE.md`):
  - Held-powerup HUD `cl_dll/ts_powerup_hud.{cpp,h}` — green top-left "Slow Motion N" (PwUp msg) + PwUpTime
    countdown. Double-tap +use in `cl_dll/input.cpp` CL_CreateMove (0.30s window, cvar `cl_usepowerup`).
  - `SlowMotionFlow`/`DeactivateSlowMotionFlow` (`dlls/ts_player.cpp`): slow self 0.6/0.8 + LOS-visible enemies
    0.35/0.225 every 0.5s. Mutual exclusion = one held slot (structural).
  - **Time dilation = `pev->fuser1`** (networked, eased toward the GoSlow target in PreThink) → scales
    maxspeed, `pev->framerate` (animation), and the weapon-timer decrement on BOTH server (`player.cpp`
    PostThink) and client prediction (`cl_dll/hl/hl_weapons.cpp`). So fire-rate + anim slow in slow-mo.
  - **Sound pitch**: guns `100+300*slow` (`ev_ts.cpp` EV_TSFire — for "recorded-slow" wavs, ~255 normal →
    drops in slow-mo); melee `100*slow` (`ts_gun.cpp` TSMeleePitch — for normal-recorded wavs, slowed not
    sped). ⚠️ See OPEN #sawed-off below: some gun wavs are ALSO normal-recorded.
  - **Teal screen tint** (`cl_dll/view.cpp` V_CalcRefdef): engine screen-fade RGBA(**0,100,125**), alpha
    `(1-slow)*50`, gated by `cl_slowfade` (NOT a green FillRGBA — that was wrong). `TSSlowMo` msg = perceived
    world slow. ⚠️ The user wanted green earlier but the RE'd color is teal; confirm.
- **Slow-mo bullets** `dlls/ts_bulletmgr.cpp` (RE: `scratchpad/bulletmgr.c`): shooter slowed (<0.99) → each
  pellet flies as a tracked bullet (GetBulletVelocityMult: slow-mo ≈ slow / slow-pause ÷10 = dodgeable); a
  CBeam `bullet_trail.spr` extends muzzle→bullet; HITS glass + decals; vanishes when slow-mo ends. Driven
  from `CTSGameRules::Think`. SLICE: no pierce re-seed; full speed approximated (6000, TUNE).
- **Weapon fixes**: akimbo AUTO fires BOTH guns (uzi `akimbo_shoot1`=12 / beretta+gold `linkedshoot`; from
  the viewmodel seq dump) not alternate; mp5sd + mossberg fire sounds restored; ammo powerup → clip = 2× MAX.

### 🔴 OPEN / IN-PROGRESS — the live task-tool list (#10–15) + new items, all need RE-grounding
- **#10 Double-firerate timeout** — user says it's "permanent"; the `UpdatePowerUpState` expiry code LOOKS
  correct on inspection. NEED the precise symptom (does the green text count down + clear? can you pick up
  slow-mo after?) or a live gdb test to pin it.
- **#11 Silencer "tic" for BOUGHT silencers** — silenced-base (mp5sd) DONE. RE (`scratchpad` agent summary,
  see ledger): the client picks `<weapon>/fire-sil.wav` when silenced; the silencer bit is bit0 of the attach
  mask. Our buy doesn't wire `attach` yet, and our fire EVENT packs barrel-side in iparam2 but the original
  uses iparam2=attach-mask / **bparam1**=barrel side — align the event + add the `-sil` table to EV_TSFire.
- **#12 Melee THROWN speed** — pitch DONE; the thrown knife/katana FLYING speed should slow in slow-mo
  (differently slow-mo vs slow-pause), like the slow bullets. Not done.
- **#13 Shotgun reload anim** — ✅ DONE + live-playtested (cont.69z, committed 6f9b9d4). See the cont.69z block above.
- **#14 Right-click HOLD-POSITION toggle** — ✅ DONE + live-playtested (cont.69z, committed 6f9b9d4). See above.
  (SLICE still open: the scope-ZOOM branch of SecondaryAttack — needs scope-attach plumbing.)
- **#15 Akimbo L/R independent fire** (modes 4/5 = "Free Auto": L-click one gun, R-click the other). The
  AUTO-both is DONE; the L/R-per-button split is NOT.
- **#sawed-off pitch (NEW, unresolved)** — the sawed-off gun-fire doesn't audibly pitch in slow-mo. Diagnosis:
  it uses the SAME `EV_TSFire` pitched path as every gun, so it's not a code branch — its wav is apparently
  NORMAL-recorded (not recorded-slow), so `100+300*slow` is the wrong formula; it (and maybe other shotguns)
  should use the melee-style `100*slow`. **Waiting on the user to say which weapons sound wrong**, then flag
  those to the slowed formula (add a per-weapon "recorded-slow" bool or a sound-name set in EV_TSFire).
- **Deviation flagged in-code**: slow-pause self-slow set to **0.6** (= slow-mo, user-directed) vs the RE's
  **0.8** — so your fire/move don't speed up in slow-pause, only the bullets go slower. Live-trace to pin if
  exactness wanted.

### Method reminders specific to this work
- The slow factor lives in **`pev->fuser1`** (current, networked) eased toward `m_flSlowMotionGo` (target).
  `FixedSlowAmt(x)=max(x,0.01)`. The CLIENT smoothed slow is `g_flClientSlow` (`ts_powerup_hud.cpp`, eased in
  its Draw). Perceived-world slow (for pitch/overlay) = the others-scale (0.35/0.225), NOT the 0.6 self.
- `scratchpad/WEAPON_SECONDARY_RE.md` was the agent's target but the file write didn't persist — the full
  findings are in the cont.69y ledger entry (akimbo/right-click/silencer/fire-mode decompiles).

## ✅ DONE (cont.69x) — the TS client PARTICLE / impact-effect subsystem is reconstructed + LIVE-VERIFIED
The cont.69w next-task is **complete**. The original's custom client particle pool (NOT engine tempents,
NOT the old R_SparkShower) is byte-ported in **`cl_dll/ts_particles.{h,cpp}`**, hooked into
`HUD_DrawTransparentTriangles` (`cl_dll/tri.cpp` → `TSParticles_Update()`), and routed from
`cl_dll/ev_ts.cpp`: `EV_TSBulletImpact` → `TS_ImpactEffect` (per-surface dispatcher), `EV_TSMelee` →
`TS_MeleeImpact` (glint + sparks only). Full RE map: **`scratchpad/impact-effects-re.md`** (on-disk,
gitignored). What's in it, all verified by **live gdb traces of the running original** (not guessed):
- **Crux render/update** = `FUN_100520d0(pool)` (the handoff's stated blocker) — located via xrefs to the
  pool object `DAT_1018bde8` from `HUD_DrawTransparentTriangles`. Per-frame: integrate `pos += vel*frametime*
  _DAT_10079e88(10)`, gravity `vel.z -= dt*gravity*0.1*sv_gravity(800)=×80`, drag, brightness fade
  (`-=dt*fade*100`), size accel, frame anim, TriAPI billboard draw (`gEngfuncs.pTriAPI`).
- Manager (`FUN_1004ffe0`/`100509d0`/`100509a0`/`10050b60`), billboard (`FUN_10050ce0`/`10050080`, camera-
  facing + velocity-STRETCH), 5 spawners (smoke `46680` / debris `46bd0` / scintille `46f50` / spark-shower
  `492b0` / pellets `46a70`), dispatcher `FUN_10048f60` (per-surface char switch + 33/50/70 RandomLong gating).
- **Smoke = a conic flare**: 4 puffs marching out `pos=origin+normal*size*0.8*i` (size ×0.8/step), each
  velocity-STRETCHED (stretch 7) + growing (accel 0.1); **velocity divisor DECREMENTS** `/(count-i)` so tip
  puffs eject faster; frameRate `20±10` jitter. (Spawn args + every motion field byte-matched live.)
- Bullet decals = **server-side** `DecalGunshot` (entry + exit) so all clients see them; resolves the TS
  `{shot` from `ts/decals.wad` (the Valve `{shot` asterisk only appears if that wad isn't resolved). The
  glass-crack/`{bproof` decal on a non-breaking glass hit is handled by `CBreakable::DamageDecal` on this path.

### ⚠ Known approximations / deviations (faithful BEHAVIOUR, not byte-exact rendering)
- **Billboard velocity-stretch geometry** (`FUN_10050080`) is reconstructed, not byte-ported corner-for-corner.
- **Debris tumble**: implemented as a screen-space roll (angle += spin*dt*10, spin=RandomLong(-180,180)); the
  original rolls via the engine billboard fn `DAT_100d5b88` = `hw.dll`. Visual, close enough.
- **Concrete debris** renders `kRenderTransAlpha(4)` not the RE's `TransColor(1)` — our TriAPI draws INDEXALPHA
  TransColor as an unmasked square; (4) masks it to the flake. Engine-quirk workaround.

## ✅ DONE (cont.69y) — kung-fu KICKS dispatch is now RE-faithful (CCombatAttack__6CTSGun @0x86078)
Rewrote `CTSGun::CloseCombatAttack` (dlls/ts_gun.cpp) as a faithful port of the decompiled dispatch
(scratchpad/ccombatattack.c). Decoded the untyped pev offsets: button@0x180, flags byte@0x1a5 (FL_ONGROUND
0x200 / FL_DUCKING 0x4000), iuser4@0x250 (lock mask 0x50c30); pulled every DAT_ constant from the binary
(damage 35/45/55). Kung-fu is FULL-AUTO; refire = `local_108` (normal punch **0.25s** = 4/sec cap, uppercut
0.65, basic kick 0.3, finisher 0.5, heavy kicks 0.75–1.35). **NOTE:** my first pass read refire as `local_10c`
(0.1s) — WRONG: Ghidra mis-attributed the load (the `CCombatAttack` tail mis-symbolizes as `SlowMotionFlow`,
boundary-confused frame analysis shifted the slot 4 bytes). The user live-confirmed the original = ~4/sec
full-auto (hold == click) = 0.25 = `local_108` (DAT_00107e98) EXACTLY; `local_10c` is the victim block-stun.
KICK selection now mirrors the original: **duckkick** (onGround+duck) → **back-kick**
(moving-backward: +back key when slow, vel·facing≤−0.5 when fast) → **dropkick** (airborne+speed>270+duck+
moving-forward) → **3-kick ground combo → spin finisher** (the `m_kickCombo`/this+0xa00 counter, no-repeat
cycle 0xa/0xe/0xd/0xf) → **flying kick** (fast+airborne, cycle 0x13/0x12/0xf). PUNCH side also made faithful:
persistent `m_comboStep`/0x9fc counter (3 jabs → uppercut; reset only on finisher, NO time-window), air-punch
when airborne, uppercut on combo>2 OR a duck-tap; all punches 35 dmg. Builds clean (hl.so + client.so, glibc
2.14); **USER playtested + confirmed "this looks good"** (full-auto 4/sec, visible 3-jab→uppercut). Also
recovered the canonical attack-id→body-anim names from `HandleCloseCombat` (0x10=sidekick, 0x14=duckkick, …).
SLICEs (documented in-code): the forward whiff-LUNGE (orig
local_110 → player+0x79c, a decaying PreThink velocity push on a miss) is a separate PreThink subsystem not
yet ported; the iuser4 lock-state (0x50c30) and player+0x814/0x815 state bits (special-punch 0x17 / forced
finisher) default to "clear". Original ids 0xa..0x19 remapped to v_melee.mdl's 11 seqs (dispatch/dmg/timing
byte-faithful, anim a remap).

## ⭐ NEXT TASK — the remaining WEAPON-FEEL TODOs (see the 🔴 list below)
Kung-fu kicks done; open work carried from cont.69v: dynamic-spread edge cases, weapon sway, coldcock whip,
breakable respawn, katana throw. Use the same **live-trace-don't-guess** method (`scripts/live.py`).

### Task-tool entries — ALL COMPLETE (cont.69x)
#1 locate update/render fn · #2 struct/pool · #3 manager · #4 update/render+hook · #5 spawners (RE-audited
from raw ASM) · #6 dispatcher+wrapper · #7 route events / drop R_SparkShower · #8 glass-crack decal — all ✅.

### ✅ COMPLETED + DEPLOYED this session (cont.69w) — see the ledger for detail
- **Angle-recoil** reconstructed (cl_dll/view.cpp TS_AddViewRecoil/TS_UpdateViewRecoil): per-shot aim climb,
  ease `frametime*15` (cap 1), persistent, snap <0.1°. **The cont.69v "0x648c0/vtable[0x38]" lead was WRONG**
  (that's a vgui Panel getAbsExtents). Real source: FUN_10008230 (client twin of server VKickback) → recoil
  TARGET accumulator → eased in V_CalcRefdef (FUN_10057ed0). Guns = climb only (NO shake).
- **Melee/kung-fu "bounce-back" shake** (the screen-shake — melee only, guns don't shake): knife/katana
  amp=flDamage*0.15, kung-fu amp=min(power,45)*0.35 (punch 20/kick 25), dur 0.2/freq 10, transient render-
  only. view.cpp TS_TriggerViewShake/TS_ApplyMeleeShake; triggered in CTSGun::MeleeAttack/CloseCombatAttack.
- **Bullet penetration** (dlls/ts_gun.cpp TSFireBulletsPenetrate): rewrote to the original ShootBulletsPlayer
  (@nm 0x5f69c) uniform PointContents loop — penDepth=min(dmg*2,48); pass-through gated by world-solid
  thickness (so depends on weapon dmg + enter/exit angle). Removed the special glass branch.
- **Glass** no longer always breaks: PointContents ignores entity brushes so bullets pass through glass;
  it shatters only if dmg >= pane health (=20 in TS maps). **cont.69x FIX:** per-pierce damage_adjust =
  the `damagemult` cvar value (default 0, RE-matched to ShootBulletsPlayer — `*(damagemult+0xc)`), NOT 0.75
  — so two glass panes break only the first and DECAL the second (was breaking both at 0.75).
- **Weapon table now byte-exact** (dlls/ts_weapon_info.cpp): the Write_<weapon> values are an EARLIER stage
  OVERWRITTEN by Sparta()/SetSpartaWeapon (server) == client FUN_10007a80 (identical). Added SetSparta()
  + 32 calls (spread/viewkick/recoil/damage); fixed m61/knife/katana/sealknife/flag Write_ outliers; added
  Write_flag (idx 29). Verified 0 mismatches vs binary Write_ AND Sparta. See [[sparta-overrides-write-weapon]].

### 🔴 STILL-OPEN weapon-feel TODOs (carried from cont.69v, not done)
- ✅ **Kung-fu KICKS dispatch** — DONE (cont.69y), RE-faithful port of CCombatAttack__6CTSGun (above).
- ⏳ **Dynamic spread edge cases**: m60 5× recoil (field_0xc8==0x20), barrett max-spread-moving (==0x12).
- ⏳ **Weapon sway**: amplitude smoothing + per-weapon grip + forward term not ported (figure-8 IS done).
- ⏳ **Coldcock viewmodel whip** (coldcock.sc LAB_10043800); **breakable periodic respawn**; **Katana
  right-click = CTSGun::KatanaThrow**.

### 🔴 LIVE-original debugging — scripted: `scripts/live.py` (+ CLAUDE.md §4b, `live-original-ts-debuggable` memory)
The user **keeps the original TS running in `hl.exe` (Proton/Wine), ALWAYS available to gdb-attach.** When
static RE dead-ends on a virtual/indirect/client-side path, **live-trace it — don't guess** (guessing = the
cardinal sin). **`scripts/live.py`** codifies the workflow (built cont.69v):
`live.py pid` / `base <mp.dll|client.dll>` / `resolve 0xADDR…` (→ module+RVA+nearest export) / `bt` (pipe a
gdb backtrace, resolves every frame) / `read <module> 0xRVA [f|d|u|p]` / `readabs 0xADDR` / **`vmethod 0xOBJ
<slot>`** (this→vtable[slot] → resolve) / **`watch <module|abs> 0xRVA|0xADDR --go "msg"`** and **`break …
--go`** — run these BACKGROUNDED: they print `>>> ARMED — <msg>` once set (tell the user the action), block
until the trip, then print the resolved backtrace; a value-`watch` fires only on CHANGE (no mouse → only the
recoil moves `cl.viewangles`). **The recoil step is now ~3 commands:** `break client.dll 0x648c0` (read
`param_1`/the object from the regs+stack dump) → `vmethod <param_1> 14` (→ the `vtable[0x38]` recoil fn) →
decompile + reimplement → `watch abs 0x2d8aec4 --go "fire, no mouse"` to verify the kick matches the original.

### In-flight TODO (no formal task-tool entries this session — tracked here; newest weapon-feel work)
- 🥇 **Angle recoil** — located (above); reconstruct `vtable[0x38]`'s math. THE next task.
- ⏳ **Kung-fu KICKS (right-click) not RE-faithful** — the FIRE punch combo IS done (`field_0x9fc`: 3 jabs →
  uppercut). The kick side still uses our OLD condition approximation, not the RE'd `field_0xa00` 3-kicks+
  finisher counter + the back/drop/side/spin/air-kick dispatch. Full RE table is in CCombatAttack @0x86078
  (decompile via `scripts/ghidra/re.sh 0x96078`; the cont.69v ledger has the decoded move table). Model seqs
  are authoritative for anim indices (v_melee.mdl: punch_left2 punch_right3 uppercut7 kick5 sidekick6 …).
- ⏳ **Dynamic spread — edge cases skipped**: `field_0xc8==0x20` (m60 = 5× recoil), `==0x12` (barrett =
  max-spread-when-moving). The core (ScaleSpread grow + PreThink decay) IS wired.
- ⏳ **Weapon sway — simplified vs the RE**: amplitude SMOOTHING (state DAT_10191d6c/d78), per-weapon GRIP
  scaling (0.1/0.3/0.5/0.7/1.0 by anim-ext), and the forward base-position term `forward·(0.75−amp·0.5)`.
  The figure-8 pendulum + mid-air stop ARE done (constants read live from client.dll FUN_10066e70).
- ⏳ **Kung-fu wall smoke** is a server `TE_SPRITE` approximation; the original likely spawns it client-side
  via the impact dispatcher (same path as bullets) — RE the kung-fu client event if you want exact fidelity.
- ⏳ **Knife slash viewmodel cycle** uses deterministic `m_slashStep` (verified seq indices: knife {3,4,5},
  katana/sealknife {2,3,4}); the original cycles 3 body anims + the knife.sc client event — fine, but VERIFY.
- ⏳ Older queue (still open): **Coldcock viewmodel whip** (RE coldcock.sc LAB_10043800), **breakable periodic
  respawn** (`BreakableRespawn`/`RespawnThink`/CBreakable::Die respawn branch), **Katana right-click** =
  `CTSGun::KatanaThrow`.

### Big tooling win this session — `scripts/ghidra/re.sh` (server-RE orchestrator) + ApplyInventory
`re.sh setup` (fresh project + ApplyInventory: member/vtable structs, typed engine API, FP consts, clean
names) then `re.sh <Class>` / `re.sh 0x<ghidraAddr>` / `re.sh refs <Class>` decompiles read like source.
**The `re.sh` output filter was fixed cont.69v** (it was dropping multi-line decompile bodies — now full
typed bodies print). See CLAUDE.md §4a.1. (re.sh is server-only / `ts_i386.so`; for **client.dll / mp.dll**
decompile-by-address use **`scripts/ghidra/dc.sh <client|mp|server> 0x<rva>`** — built cont.69v; feed it the
RVA straight from `live.py resolve`/`bt`.) `extract_weapons.py` is UNRELIABLE (stack-offset collisions corrupt values) — the cont.69v
weapon-table was re-verified by decompiling all 38 `Write_<weapon>` functions; trust those, not the heuristic.

**Method reminders:** RE-ground every value (the user REVERTS guessed work). ⚠️ FILE OFFSET for `.rodata`
constants = **Ghidra addr − 0x10000** (the FP pool reads 0 at the naive offset; many consts are DOUBLES the
decompiler shows as `(float)`). Tooling: `scripts/ghidra/{DecompAt,DecompRefs,FindStr}.java` (server proj
`recovered/ghidra/server` name `serverso`, client `recovered/ghidra/client` `clientdll`, Windows
`recovered/ghidra/mpdll` `mpdll`); Ghidra addr = `nm` + 0x10000 for the server `.so`. `scratchpad/` is
gitignored (decompiles/RE notes live there). Commits are `--no-gpg-sign` (gpg passphrase expires mid-session);
work on `main`; no `Co-Authored-By` trailers. Don't commit the user's `test.sh` (their launcher).

## Where things stand (2026-06-24, through cont.69w) — cont.69w (recoil/shake/penetration/glass/weapon-table)
## was BUILT + DEPLOYED to the xash rig AND COMMITTED on `main` as **72e96fa** (`--no-gpg-sign`). Still
## uncommitted on disk: `test.sh` (the user's launcher — DO NOT commit) + pre-existing `reference/coverage/*`
## edits (not cont.69w work). Older state below is through cont.69t:

### cont.69t — weapon-effect polish batch (RE-grounded; user playtests). Read the ledger top for full detail.
- **Bullet impact smoke** (8430ea3) — client `EV_TSBulletImpact` (cl_dll/ev_ts.cpp): a SMOKE puff per surface
  material at the hit (RE: client dispatcher `FUN_10048f60` — wood/dirt→wood_smoke_add, metal/glass/grate/vent/
  computer→gun_smoke_add+R_SparkShower, concrete/default→gun_smoke, flesh→none). Server already did damage+
  texture-sound+decal+blood; this adds the visual. Reuses `PM_FindTextureType` + `EV_TraceTexture`.
- **knife.sc melee spark** (6876988) — `MeleeAttack` plays `events/knife.sc` on a non-flesh surface hit →
  client `EV_TSMelee` R_SparkShower at the impact (origin carries the point).
- **Predicted melee/grenade anims** (62d754e) — knife/katana slash (SendWeaponAnim slash1 = v_knife 3 /
  v_katana 2) + swing/hit sounds; grenade throw seq. (Once CTSGun is predicted, these stick.)
- **Knife right-click THROW** (dee455c + c16e960) — `dlls/ts_knife.cpp` = `CTSKnife` projectile. SecondaryAttack
  (fire-type KNIFE) → `CTSGun::ThrowKnife` (spends m_iClip, throw seq v_knife 7 / v_sealknife 5, auto-switch when
  dry). COLLISION (RE: BounceTouch@0x6decc): STICK in non-breakable surfaces (embed), BOUNCE off unbreakable
  breakables, BREAK-THROUGH breakables it can kill + stick in the wall beyond (ignore_glass forward trace),
  stab flesh. Pickup = press-E (IN_USE) in a trigger radius (NOT run-over). **Fixed a server crash** (Host_Error
  "trigger in clipping list" — relink after changing pev->solid).
- **Dropped-weapon GLOW** = WHITE `kRenderFxGlowShell` (CTSWorldGun + stuck knife) — model shell, not a light.
- **Breakable shatter** — ALREADY WORKS (stock HL CBreakable::Die TE_BREAKMODEL by material; verified glass→
  glassgibs on ts_kissmeyerbasic). No change needed.

### cont.69s — ✅ CTSGun is CLIENT-PREDICTED; the `.sc` weapon-prediction subsystem is COMPLETE. Read `reference/EVENTS.md`.
- Steps 1–2: shared `s_weapon_info` table into client (`dlls/ts_weapon_info.{h,cpp}` built into both DLLs) +
  the active-weapon param cache (`cl_dll/ts_weapon_predict.{h,cpp}`, `g_tsActive`) + the draw event.
- **3a–3d: CTSGun is now a real CLIENT-PREDICTED weapon** so its viewmodel anims STICK. `ts_gun.cpp` compiles
  into client.so (class extracted to `dlls/ts_gun.h`; client stubs in `cl_dll/ts_gun_stubs.cpp`); `g_TSGun`
  registered in `cl_dll/hl/hl_weapons.cpp` (HUD_InitClientWeapons + the TSGUN_WEAPON_ID case); state shipped
  via `CTSGun::GetWeaponData/SetWeaponData` (type/reserve/firemode). **The key fix = TIME BASE:** CTSGun's
  timers reverted from absolute `gpGlobals->time` to `UTIL_WeaponTimeBase()` (34 sites) + `UseDecrement()`
  returns true, so the engine/server decrement loop drives the 0-base correctly. Fire/akimbo events use
  FEV_NOTHOST (no double muzzle). Verified predicted (listenserver) AND server-only (dedicated rig, bots).

## Where things stood (2026-06-23, through cont.69r) — all committed (git log: 1ca0ca4 / c675ef0 / 7a66e5c)

### cont.69q–r — kung-fu close-combat + many playtest fixes (all RE-grounded, in-engine/user-verified, COMMITTED)
- **Akimbo OWNERSHIP refactor** (items[]-by-type) — see cont.69p note below; done + verified.
- **Firing fixed** (was fire-once-stuck): CLIENT_WEAPONS makes `UTIL_WeaponTimeBase()`==0, but CTSGun is
  server-authoritative → all its timers now use `gpGlobals->time`. **Loadout fixed:** batched `tkw/tki/tka`
  parse ALL argv (client sends one space-separated cmd); `BuyArmament` no longer resets so the config
  persists across spawns/round-restarts. **Deploy fixed:** `TSDeployArsenal` forces the tsgun active +
  `CanDeploy()` override (TS uses its own ammo). **Round-restart weapon-loss fixed:** `PlayerSpawn`
  `RemoveAllItems` before re-equip (was making a 2nd tsgun → arsenal vanished).
- **Kung-fu** (TSW_MELEE = bare hands → `CCombatAttack`): contextual dispatch (FIRE = punches only —
  uppercut on duck, combo on hold, single punch on tap; +ATTACK2 = kicks only — sidekick/spinkick/backkick/
  kick); reload = `CCombatBlock` "hooah" (player/block.wav) + stamina → breathe2.wav; flesh/surface/miss
  sounds (kungfuhit / closecombat+pl_step / closecombat); v_melee anims remapped to the model's 11 seqs;
  3rd-person BODY anims via `CTSAnimation::PlayKungFu` (the player models carry punchleft/sidekick/… by
  name) + `combat_idle` stance; no phantom 3rd-person held weapon (empty weaponmodel when no p_model).
- **Muzzle flares**: `ts_muzzleflash1h/6` sprite at the muzzle attachment (akimbo per-barrel). **Shotgun**:
  per-shell incremental reload (interruptible by firing). **`thirdperson`** is now a toggle.
- **`.sc` events**: complete RE map + subsystem plan committed (the NEXT TASK above).

### ✅ DONE (cont.69p) — the akimbo OWNERSHIP refactor is IMPLEMENTED + in-engine verified.

### ✅ DONE (cont.69p) — the akimbo OWNERSHIP refactor is IMPLEMENTED + in-engine verified.
`CTSGun`'s arsenal is now the original's **items[]-by-tsweapon_t model** (`struct TSItem m_items[TSW_COUNT]`
in dlls/ts_gun.cpp = {owned, clip, leftClip, rightClip, mode, attach, barrelMask}). A single can be
wielded/dropped independently of its akimbo pair. RE in `reference/WEAPON_SELECT.md` (TakeWeapon@0x80a04 /
CTSGun::DropWeapon@0x8094c / CBasePlayer::DropWeapon@0x84900 / SwitchItems=FUN_00081564 /
GetBetterWeapon@0x80f40 / g_iWeaponPriority). Combine keeps BOTH items[single]+items[akimbo] owned (scroll
skips the single, slot1-5 still picks it); dropping a single while owning its akimbo throws one + keeps the
other (clip = akimbo.rightClip) + removes the akimbo; dropping the akimbo throws two singles + removes both;
useitems cycles the active barrel mask; GetBetterWeapon auto-selects by priority. Verified in xash3d
(combine via `tsbuy 2`×2, slot-select-single-while-owning-akimbo, drop-single-keeps-other; no crashes).
**Plus three cont.69p playtest fixes (see ledger):** (1) the spawn→spectate "crash" was a launch-flag
issue — the listenserver needs **`+deathmatch 1 +maxplayers 8`** (else stock respawn() does
`SERVER_COMMAND("reload")`); CLAUDE.md §3 updated. (2) kung-fu no longer drawn red in the select bar
(clipless weapons can't be "out of ammo"). (3) dropped weapons no longer block the player while falling
(RE: original gives the falling world gun a ZERO-size bbox; ts_worldgun.cpp falling SetSize → g_vecZero).
**NOT yet committed.** Remaining: exhaustive 38-weapon in-engine retest.

### Recent playtest fixes (cont.69m–n) — all RE-grounded + in-engine/user-confirmed
- **Weapon-select REBUILT** (`cl_dll/ts_weaponbar.cpp` + `ammo.cpp`): the stock yellow `CHudAmmo` menu is
  retired; the TS bar IS the select UI (RE: FUN_1004f500/1004f220/1004d2cf/1004e030). slot1-5 + invnext/
  invprev open it + move a cursor; **FIRE commits** `weapon_<N>` (server holster→deploy, plays
  weapons/switch.wav); closes on commit/cancel (no timer). Bar ALWAYS draws but very transparent; selecting
  brightens it, cursor most opaque (3-level additive opacity, NO box); cursor shows curclip/maxclip.
- **Death = spectate** (`dlls/player.cpp PlayerDeathThink`): on death, after the anim, re-enter the free-look
  spectate/buy phase (TSEnterSpectate); respawn on FIRE after the gate (NOT a timer). Verified via `kill`.
- **Spectator chrome suppressed** in the TS phase (`tri.cpp` overview grid; `vgui_SpectatorPanel.cpp`
  #Spec_Duck). The dark bottom bar uses `pfnFillRGBABlend` (pfnFillRGBA is ADDITIVE → black was invisible),
  geometry = the stock bottom border (YRES_HD(64), label at YRES_HD(6)).
- **Buy menu**: first open per map randomizes the loadout ("Use New Config"); later opens reload the last
  config ("Use Last Config"); Open() guards on already-open (the server resends the spectate msg each tick —
  was causing a reopen-flicker after apply). **TSMessage SVC notification** client handler added
  (`cl_dll/ts_message.{cpp,h}`, CHudTSMessage — fixes the "No pfn TSMessage" error); "Using new/last config"
  + "Press fire to play" route through it (centred, fades out). Kung-fu given on every spawn. Status panel
  hidden while dead/spectating.

### ✅ THE REAL BUY MENU — REBUILT as a HUD element (cont.69l). cl_dll/ts_buymenu.{cpp,h}.
The real cascade buy menu (the `0x10037xxx` cluster) is **rebuilt + in-engine verified**: a `CHudTSBuyMenu :
CHudBase` that draws the header + cascade columns (Config|Category|Weapons|Detail) with `TSHud_DrawString`
+ the TS HUD font, navigated by **number keys (HUD_Key_Event) AND mouse (IN_MouseEvent — user-confirmed
working)**. The legacy vgui `vgui_TSBuyMenu.*` is **deleted**. Full RE in `reference/MENUS.md` "★★ THE REAL
BUY MENU" (rendering, input state machine FUN_10038840, mouse FUN_10038360/10038410, accounting, apply,
the weapon cost table). **Verified vs the user's reference screenshots**: real roster (Beretta 92F #5 +
Contender G2 #10 in Handguns), "Special Purpose"/"Go Back", green "7 Random", blue Cancel, the 1-5 loadout
preview columns (bought items by HUD column, in add-order, count-left-of-icon, `(Sil ,Las ,Fla ,Sco )`),
`Free Slots = 81 - used` (remaining), Total/slots accounting all match.
**Auto-ammo DONE (runtime-captured).** The menu auto-fills each weapon's caliber to max + adds the cost to
Total + shows the rounds left of the preview icon (Glock 210 / M16 120) + emits `tka <cal>_<units>`. The
per-caliber **max** field reads 0 statically (load-time only), so the caliber table `DAT_1009ffc0` + per-weapon
caliber (`weapon_info+0x08`) were read from the LIVE hl.exe (`/proc/<pid>/mem`, client.dll base 0x0c070000,
table @0x0c10ffc0) — full table in `reference/MENUS.md` "AUTO-AMMO". Verified Glock → 350cr ammo → Total 3400.

**Spectate HUD fixed (cont.69l):** TS has NO stock spectator chrome — the inherited HL/TFC `SpectatorPanel`
top "Map:"/timer/score bar was bleeding through. `SpectatorPanel::Update()` now suppresses it (top+bottom
borders + Map/timer/scores) during the TS spectate phase, and `CHudSpectate::Draw()` draws the dark bottom
bar + "Free-Look" + the control help in the HUD layer (no longer hidden under the vgui overlay).
Screenshot-verified vs the original (no top bar; bottom bar matches).

### (historical) THE BUY MENU course-correction
**Read `reference/MENUS.md` "★ THE REAL BUY MENU" — the rebuild spec.** The TS client.dll has
**two** buy-menu code paths: the **legacy/dead** one (`0x10010xxx`, a **vgui::Panel**, literal strings, the
`DAT_1007a950` table) — which is what `cl_dll/vgui_TSBuyMenu.{cpp,h}` currently reproduces (cont.69e–j:
config + category + weapon-list + per-weapon attachment submenu + mouse/keys + compact green styling, all
built + in-engine verified) — and the **REAL** one the game actually draws (`0x10037xxx`). The user's
reference screenshots (cascade columns, "Special Purpose", "Go Back", attachment costs, Beretta 92F +
Contender G2 in Handguns) are the REAL menu. All on-disk client.dll copies incl. the Steam install are
byte-identical (688128b) — **same binary, we just RE'd the wrong builder.** **User decision: RE + rebuild
the real cascade menu.** RE is COMPLETE (cont.69k); the **rebuild is the next task**.

**The real menu (full RE in MENUS.md — the rebuild target is a `CHudBase` HUD element, NOT vgui):**
- **HUD-drawn** — entries via `FUN_1004ff10` → **`FUN_10021ce0` = `TSHud_DrawString`** + the TS HUD font
  `DAT_100d5908` + `titles.txt` token localization. So it reuses our existing HUD draw/font (it'll look right).
- **Cascade**: orchestrator `FUN_10036750` (switch on `obj+4` = level 0 config/1 category/2 weapons/3 detail/
  4-7 accessory&c/8 off); columns side-by-side at X=8/0x88/0x108/0x188, rowH 0x14, width 0x80; top header bar
  (`FUN_10036af0`: Total + Free Slots). Builders: config `FUN_10037540`, category `FUN_100377d0`, weapons
  `FUN_10037960`, detail `FUN_10037ba0`.
- **Real weapon roster** (`obj+0x2d6c`, indexed `row*5+cat`, filled by `FUN_10036200`): Handguns
  [1,9,12,14,2,21,22,28,31,36], SMGs [3,6,7,17,19,23], Rifles [5,13,15,27,18], Shotguns [4,11,20,26,33],
  Special Purpose [24,25,32,34,35]. **Attachment costs**: Silencer 900, Lasersight 800, Flashlight 500,
  Scope 1800. Colours: available=half-hud dim, over-budget=red(195,45,45), Random/affordable-Buy=green
  (45,195,45), Cancel/GoBack=blue(100,100,205).
- **Still to RE for the rebuild**: the input/level-advance handler, the accessory column (states 4-7), the
  `FUN_10038180` weapon-preview geometry. The legacy-menu RE findings on per-weapon attachment data
  (0xE0/0xE4 masks, akimbo 0xC8, full 0xDC) + the styling (compact font, colours) CARRY OVER.
- **Also raised by the user (separate, not started):** the weapon-select bar (invnext/invprev) doubles as the
  loadout preview (bought items before applying config) AND the in-game weapon HUD when ALIVE, showing
  per-weapon attachment icons — `CHudTSWeaponBar` (Image #4 in the consult thread).

### Where things stood (through cont.69f)
Pipeline fully working: decompile → recover typed prototypes → write modern C++ →
**build into `hl.so`/`client.so`** → **run in-engine** (ReHLDS for server; xash3d-fwgs
for the graphical client — this box HAS a GPU) → **gdb** any crash. Everything below is
committed (`git log`); `recovered/` is gitignored.

### cont.69 (this batch) — kevlar gameplay + the TS buy menu. Read `reference/MENUS.md` first for the menu.
- **DONE + verified:** `powerup`/`cashala` cheat cmds (server cmds 0 missing); the **TS status panel**
  (posture figure + health♥ + Kevlar) replacing the stock HEV (`cl_dll/ts_status.cpp`); **stock HEV/battery
  suppressed**; **kevlar GAMEPLAY** — `CBasePlayer::TakeArmor` (binary flag + armorvalue) + the `armor_absorb`
  flat % damage reduction in `TakeDamage` (NOT a depleting pool; bullets-only, melee/fall/blast bypass via
  mask 0x80040e4) + `AddItem` + buy routing (`dlls/ts_buy.cpp`, `dlls/player.cpp`, `dlls/ts_gamerules.h`
  `m_flArmorAbsorb`). All RE-grounded, in-engine verified.
- **TS BUY MENU — IN PROGRESS (reproduction, `cl_dll/vgui_TSBuyMenu.{cpp,h}`).** Mechanism FULLY RE'd
  (`reference/MENUS.md`): it's a **vgui::Panel of vgui::Button** entries (NOT MOTD/gmsgVGUIMenu, NOT
  gmsgShowMenu), nav by mouse-click OR bound number key, apply emits `rst_cfg`/`tkw`/`tki`/`tka`. Built +
  committed: **Config + Buy-category + Player Accessory pages** (RE'd from FUN_10010440/@0x10010dd0) + the
  **number-key routing**. **NOT done:** (1) per-category **weapon lists** — builders identified
  (`FUN_10010890`/`FUN_100108b0`/`FUN_10010bb0`, the action-`0xb` thunks — see MENUS.md); (2) the config
  panel's **exact colours/positions** — provably not statically RE-able (the `+0x2d84` config panel has no
  alloc/store/sizer in the disasm); resolved via a **runtime hbreak capture of the live original** (TS runs
  under Proton; SOFTWARE breakpoints don't fire in wow64 but HARDWARE `hbreak` do; the menu is built ONCE and
  cached, so arm the hbreak BEFORE a clean relaunch+first-open — exact procedure in MENUS.md). Placeholder
  values applied (dims 128×20, white config text) are clearly flagged, not guessed.
- **HARD RULE reaffirmed by the user this batch (RE before code, no guessing):** every menu entry/colour/
  position/command must come from the decompiled builder or the runtime capture — do NOT improvise styling
  or layout. The user reverted earlier guessed work (orange→green→white text, made-up geometry, a top-level
  "Buy Kevlar") until it was RE-grounded.

### ⚠️ Serena ledger read gotcha
`read_memory("ledger")` returns ONE giant single-line JSON (`{"result":"…"}`, ~150 KB) that overflows the
read limit. **Don't read it via the MCP tool** — read `.serena/memories/ledger.md` directly (newest on top),
or `jq -r '.result' <savedfile> | head -c 9000` on the file the tool saves.

(Original cont.68l baseline pipeline note retained below.)

> ### ⚠️ READ FIRST — hard requirement (cont.66, user-set): **RE before you code.**
> On any user feedback, **check the decompilation FIRST** — do not improvise. Every
> **console command** the client sends is part of the swap contract, so verify it exists
> in the original before using it (server cmds = `ts_i386.so` ClientCommand dispatch;
> client cmds = `client.dll` `pfnAddCommand`; `strings`/`signatures-client.txt`). The
> original uses **standard inputs** (`+use`/`IN_USE`, `+attack`, `invnext`/`invprev`,
> `+alt1/2`) + server-side **Think** logic — NOT custom `ts*` commands. `menuselect` +
> `gmsgShowMenu` ARE the real buy-menu mechanism. See the memory `check-re-on-feedback`.

### ⭐ Coverage harness + Ghidra RE flow (cont.67–68 — the way to continue)
The drop-in goal is now **measured + driven by a harness**, and the Ghidra decompiles are
**typed + de-truncated**. This is the loop a new agent should use:

**1. Find the next gap — the coverage harness (`scripts/coverage/`, pure objdump/nm/readelf, no Ghidra dep):**
```bash
python3 scripts/coverage/coverage.py all     # -> reference/coverage/{original,ours}.json + REPORT.md
python3 scripts/coverage/reconstruction.py   # -> reference/coverage/RECONSTRUCTION_COVERAGE.md (method %)
python3 scripts/coverage/extract_vtables.py  # -> reference/coverage/{vtables_original.json, VTABLES.md}
```
`REPORT.md` diffs the EXTERNAL swap contract (the only thing that must match for drop-in — NOT the
internal C++ ABI, per CLAUDE.md): entity classes, cldll exports, server+client console commands,
server+client cvars (w/ default + message wire-size mismatch), user messages, events, entity
keyvalues, + a `default.cfg` input cross-check. **MISSING = implement/RE; EXTRA = invented or
HL-leftover to remove.** Cvars/usermsg-sizes come from the BINARY (pfnRegisterVariable / cvar_t
structs / LinkUserMessages), NOT config.cfg (config.cfg holds only the archived subset — that bit us).

**2. RE the gap — the Ghidra flow (persistent, typed):**

**Server side — prefer the `re.sh` orchestrator** (one entry point; applied project at `recovered/ghidra/re`):
```bash
scripts/ghidra/re.sh setup            # run.py (refresh inventory) + fresh import/analyze + ApplyInventory.
                                      #   Do once, and after coverage/script changes. FRESH each run (rename
                                      #   step isn't idempotent). ApplyInventory ⊃ ApplyTypes: builds member +
                                      #   vtable structs, types entvars_t/enginefuncs_t/FP consts, clean names.
scripts/ghidra/re.sh <Class>          # decompile every method of a class (addrs via filemap.py = nm + 0x10000)
scripts/ghidra/re.sh 0x<addr> ...     # decompile by address
scripts/ghidra/re.sh refs <Class|addr># who CALLS/uses it (DecompRefs over all the class's methods)
scripts/ghidra/re.sh export <Class>   # -> recovered/decomp/<Class>.c
scripts/ghidra/re.sh members          # DumpMembers -> reference/coverage/members_auto.tsv (transient members:
                                      #   offset+type, placeholder names — curate into members_re.tsv)
```
Reads now show `this->m_flExpire`, `this->vtbl->Precache(this)`, `g_engfuncs->pfnSetModel(...)`, `dc_0_1`.
See CLAUDE.md §4a.1 for what ApplyInventory/DumpMembers/DecompRefs do + the correctness guards.

**Client / mp.dll / lower-level** — `setup-ghidra.sh` builds the persistent `recovered/ghidra/{server,client}`
projects (DisableNoReturn + ApplyTypes), and you can still drive a single decompile by hand:
```bash
analyzeHeadless recovered/ghidra/client clientdll -process client.dll -noanalysis \
  -scriptPath scripts/ghidra -postScript DecompAt.java 0x<ghidraAddr>
python3 scripts/coverage/vtcall.py CTSGun 0x128      # resolve a vtable-dispatch offset -> method
python3 scripts/coverage/vtcall.py --file recovered/server/CTSGun.c CTSGun   # annotate a decompile's vt calls
```
Ghidra scripts (`scripts/ghidra/`): **ApplyInventory** (the omni applier; superset of ApplyTypes — see §4a.1),
**DumpMembers** (transient-member offset+type skeleton), **DisableNoReturn** (preScript — stops the noreturn
truncation), **FixNoReturn** (postScript), **ApplyTypes** (gcc2 prototypes + `entvars_t` + MSVC returns),
**DecompAt** (decompile by addr, auto-creates a function if Ghidra missed it), **DecompRefs** (callers; takes
multiple addrs), **DecompExport** (decompile a whole class to a `.c`), **FindStr** (string search).
NOTE: `ClientCommand` is a huge function Ghidra still truncates — for its command dispatch use **objdump**
disassembly + resolve the PIC string operands via the GOT (`_GLOBAL_OFFSET_TABLE_` = 0x165E10): the pattern
is `lea -0xoff(%ebx) ; … ; call <strcmp@plt>` → string at `0x165E10 - off`. The relocation table
(`readelf -W -r`) is the master key — it resolves vtables, cvar_t name/default pointers, and message-name
pointers that all read as ZERO in the file (they're R_386_32/RELATIVE relocs filled at load).

**3. Implement + verify the loop closes:** write modern C++ → `scripts/build.sh amd64 [all]` → re-run
`coverage.py all` (the gap should move from missing/extra → matched) → run in xash for visual items
(screenshot; this box has a GPU) → commit. **Commands done this way (cont.67–68): weapon-select
(`weapon_<N>`/`lastinv`), `firemode`/`drop`/`useitems`, the buy cmds (`tkw`/`tki`/`tka`/`rst_cfg`/
`cutwire_`), `respawn`/`motd`/`jointeam`/`changeteam`, + the WAITING scoreboard, + the `powerup`/`cashala` cheats
(cont.69).** Server console-command coverage: 12 missing → **0**; 0 unanswered default.cfg binds.

### Track B cont.66 — TS join / spectate / buy-config / weapon-select flow (IN PROGRESS)
The faithful join flow (no straight-to-play spawn) is built + verified; the buy menu and
weapon-select are partly done with **known unfaithful stand-ins to fix**. Plan file:
`~/.claude/plans/replicated-discovering-aho.md`.

**DONE + verified (committed `0d4f2d0`/`60a8c44`/`d1b0974`/`a1a8149`):**
- **Spectate-gate (server):** a joining/respawning player is held in **free-look observer**
  (`CBasePlayer::TSEnterSpectate` → `StartObserver`/`OBS_ROAMING`, respawn gate =
  `respawntime` cvar clamp 3..30; `dlls/player.cpp`). `CTSGameRules::PlayerSpawn` skips the
  loadout while `m_fInSpectateWait`; `CBasePlayer::Spawn` ends by entering spectate;
  `ClientPutInServer` sets the flag pre-Spawn. **Press fire → `TSLeaveSpectate` → respawn
  into play** with the chosen loadout. gmsg `Spectator`(2)/`VGUIMenu`(1) registered.
- **Free-look HUD (client, `cl_dll/ts_spectate.{cpp,h}`):** centre countdown → "Press fire
  to play" + the free-look help line. Reads `Spectator`; hooked via `__MsgFunc_Spectator`
  repointed to it.
- **Buy/config menu — the FAITHFUL mechanism:** compact **`gmsgShowMenu` + `menuselect`**
  list (NOT a VGUI panel — RE'd from `client.dll FUN_10011160`: item 1 toggles Use New/Last/
  Empty Config, available items green). `TSSendBuyMenu`/`TSMenuSelect`/`TSCloseMenu` in
  `dlls/player.cpp`; client `CHudMenu` draws it (`\g` green added in `cl_dll/menu.cpp`).
  "Click anywhere" (+attack while menu up) = item 1. The `buy` command reopens it
  (spectate-only).
- **Weapon-bar layout (client, `cl_dll/ts_weaponbar.cpp`):** RE'd from `FUN_1004e030` —
  5 columns `x=col*0x80-0x78`, header panel + **slot number** (alpha 100 active/50 idle),
  weapons stacked 0x30/row with sprite icons (red when clip empty), active weapon name+ammo
  to the right. Verified vs the user's reference.

**KNOWN-UNFAITHFUL / OPEN (fix RE-first — see the ⚠️ above):**
- ✅ **Weapon switching — DONE (cont.67, RE-grounded + in-engine verified;
  `reference/WEAPON_SELECT.md`).** Removed the invented `tsnextweapon`/`tsprevweapon`.
  Faithful: `invnext`/`invprev` (client `CHudTSWeaponBar::CycleType`) step through the owned
  arsenal and commit via the original client's transport **`weapon_<N>`** (numeric
  `tsweapon_t`, from `FUN_1004f500`'s `hud_fastswitch` commit); server `weapon_<N>`
  ClientCommand → `TSSelectWeaponType` → `CTSGun::SelectSlot` with **holster→deploy**;
  `lastinv` toggles to the previous weapon. DEFERRED: the `hud_fastswitch 0` open-bar +
  on-bar highlight + literal (row,col) grid cursor (we commit immediately + walk the compact
  owned-set).
- ✅ **Command names — DONE (cont.67–68, RE-grounded, coverage-verified):** `firemode`/`drop`/
  `useitems` (renamed from tsfiremode/dropweapon; `useitems`→akimbo swap), the buy/config cmds
  `tkw`/`tki`/`tka`/`rst_cfg`/`cutwire_<n>` (renamed from tsarm/tsarmammo/tsresetarmament/tscutwire —
  arg formats RE'd, `reference/BUY.md`), `respawn`/`motd`/`jointeam`/`changeteam`. `usepowerup` IS a
  real cmd (kept). Server console-command coverage 12 missing → **2** (`powerup`, `cashala` remain);
  0 unanswered default.cfg binds. `tsbuy`/`tsbuyammo`/`tsbuyarmament`/`tsspawnflag` remain dev
  stand-ins (no 1:1 original — the original buys via queue tkw/tki/tka → apply `cashala` config).
- ✅ **Scoreboard "WAITING" — DONE (cont.68, verified):** spectate-waiting players show "WAITING"
  in the scoreboard class column (DM + teamplay), playerclass 11. The per-team **locked model** is set
  by `AssignToTeam`. OPEN: confirm our inherited `ClientUserInfoChanged` enforces the model-lock the
  original does (resets a changed `model` infokey — see cont.68k ledger).
- **OPEN — `powerup` + `cashala` commands** (need RE): `powerup` (distinct from `usepowerup`); `cashala…`
  = the buy config-apply (parses `weapon_%i`/`lasclr_%i` — finishes `reference/BUY.md`).
- **OPEN — timed-powerup activation**: still on the `usepowerup` command path; the original ALSO has the
  double-tap `+use`(`IN_USE`)-in-PreThink path (RE `UsePowerUp`/PreThink) — reconcile.
- **Not yet built:** bar **faint-when-idle fade** (`FUN_1004d3f0` select-active); active
  **clip/maxclip "8/8"**; **powerup-timer HUD** (Slow Motion / Double Fire Rate text, green,
  above the weapons, from `PwUpTime`); **kung-fu / super-jump icons** (centre-bottom);
  buy-menu **cursor + click-on-entries**; the **"2 Buy" weapon-category submenu**;
  menu **background box**.
- **Open question (cont.67):** `buy` IS real — original `ts/default.cfg` binds `b`→`buy`
  (also `firemode`/`useitems`/`drop`/`usepowerup`/`invnext`/`invprev`); the menu is
  server-driven (`gmsgShowMenu`). cont.66's `buy` reopen is faithful. The precise dead/alive
  availability gate is a TODO for the coverage harness + a targeted `buy`-handler RE. See
  `reference/WEAPON_SELECT.md`.

**Target = INTERFACE/protocol compatibility, so each half is a drop-in swap for the
original** (our client ↔ original server, or vice-versa, behaves identically). What must
match the original byte-faithfully: the engine C interface + the client↔server protocol
(user-message **names + wire formats**, **event** names/params, **command** names, **cvar**
names). What must NOT be chased: the internal C++ ABI / member offsets / gcc-2.x quirks — we
build on the newer Itanium-ABI `halflife-updated` SDK. See CLAUDE.md "compatibility target".

**Big picture: the entire server side is reconstructed (~40 TS classes); Track A is COMPLETE and
Track B (the client) has its build foundation.** Done + verified: the full weapon engine, powerups
(dynamic spawn + slow-motion bubble), the death loop (weapons/ammo/corpse + grenade shrapnel), the
**complete team subsystem** — team assignment (cont.35), cash + free buy/armament (cont.36, the
`CashBased()`-is-off finding), all three round-end conditions (elimination cont.37, objective cont.31,
**timeout** cont.45) — **every CTF/objective/escape entity** (`CTSFlag`/`CTSBriefcase`/`CTSBomb`/
`CTSSellZone`/`CTSTeamEscape`/`CTSScoreGiver`/`CVipEscape`, cont.38–43), and the map/relay entities.
Exercised on real maps: tm_avana, ctb_deadheat, ts_dust2, ts_theatre, ts_wyndham.
**What remains is Track B — the cl_dll (client):** the client BUILDS (both 32- and 64-bit, cont.46) and
the **first TS client HUD has landed — the round-time countdown `CHudRoundTime` (cont.47, `cl_dll/
roundtime.{cpp,h}`)**, the first reader of a server user message. Still to do: the *visible* slow-motion
time-dilation, the particle system, the stunts, and the rest of the HUDs (the server already sends the
user messages; the client just needs the readers). The client now **runs + renders in xash3d-fwgs on
this box** (it has a GPU — the old "headless" note was wrong; see Track B below for the screenshot loop).
**Watch the gmsg NAMES:** the inherited server named
the round-time message `"TSRoundTime"` but the original `.so`/client.dll use `"RoundTime"` (fixed in
cont.47) — check every other inherited gmsg name (`TSPowerUpTime`/`TSMotionBlur`/team-info) against the
`.so` strings before writing its client reader (`strings reference/ts_i386.so | grep -E '^TS|^Round'`).
Small server leftovers: `CTSPlayerAmount`/`CTSSuperJump` done (cont.44); `CInfoBreakable` (thin) +
`CBulletManager` (bullet-time — best done alongside Track B) remain. See "Recommended next steps".

### Track B playtest reconstruction — IN PROGRESS (cont.47–50)
The client runs in xash3d-fwgs and the user is playtesting it; each report exposes that the stock HL
client + a partial weapon/player reconstruction is showing through. Fixed so far (all decomp-grounded):
**round-time HUD** (cont.47), **client weapon-prediction crash** (cont.48, NULL `AllocString`),
**spawn loadout** (cont.49, `weapon_tsgun`+`BuyArmament`, no HL weapons), **always-mouselook** (cont.49,
fixed the "can't look up/down + jolt"), **fire animations** (cont.49, `SendWeaponAnim` from per-weapon
`shoot*` model sequences), **akimbo fire anims** (cont.50, right/left barrel), **HUD sprite-res fallback**
(cont.50), **reload** (cont.50, `CTSGun::ItemPostFrame` drives the per-caliber reserve — the stock base
gated reload on standard ammo TS never uses).

**The user asked for a WHOLESALE faithful reconstruction of the player/weapon/input/HUD systems from the
decomp (not symptom-patching). The 5-phase plan (tracked as tasks):**
1. **Weapon firing/reload/ammo/fire-rate** (CTSGun) — reload ✅; TODO fire-rate gate, `SecondaryAttack`/
   `+alt1`, what networks to the ammo HUD.
2. **`CBasePlayer` thinks/state + the low-pitch "slow-mo" fire sound** — likely the slow-motion time-scale
   (TS lowers sound pitch in slow-mo); decompile `PreThink`/`PostThink`/`TSUpdate` + the slow fields.
3. **All TS console commands / cvars / input toggles** (`+alt1`, `+reload`, `hud_color`, `cl_realhud`, …)
   enumerated from the `.so` and wired server+client (fixes "Unknown command" spam + missing keys).
4. **The TS client HUD** (the reason there is NO HUD at all): TS ships a *fully custom* HUD — `hud.txt` is
   320/640-only, no stock crosshair/ammo/health path. Reconstruct from `client.dll` + the `gmsg` readers
   (`TSHealth`/`TSArmor`/`TSCash`/`TSState`/`TSMBlur`/`TSSlowMo`/`TSBTime`/`TSPSys`/`RoundTime`).
   **IN PROGRESS (cont.51): health/armor/cash DONE + visually verified** — the full TS user-message wire
   contract is recovered in **`reference/HUD_MESSAGES.md`**; server sends `TSHealth`/`TSArmor`/`TSCash`
   from `CBasePlayer::TSUpdateClientData` (`dlls/ts_player.cpp`); client `CHudTSStatus`
   (`cl_dll/ts_status.{cpp,h}`) draws them bottom-left. Also fixed 3 inherited protocol-name/size bugs:
   `RoundTime` (-1→size 2, 2 BYTES not 2 shorts), `TSMotionBlur`→`TSMBlur`, `TSPowerUpTime`→`PwUpTime`.
   **NEXT: the ammo HUD** (`WeaponInfo`(6)/`WStatus`(3)) — needs the per-arsenal-slot struct (@gun+0xcc,
   0x70/slot) decoded to pin the bytes; CTSGun never sends `ClipInfo`, so clip rides inside WeaponInfo.
   Then `TSState`/`TSSlowMo`/`TSBTime`/powerup HUDs, and suppress the leftover stock HUD strip.
5. **Client view (bob/sway) + predicted fire events** — viewmodel "snap" (`cl_bob 3` + stock `V_CalcBob`;
   TS bob math differs) and moving fire to the faithful predicted `events/fire.sc` (`EV_TSFire`).

- **Map entities — runtime-verified:** `CTSModel` (108 on `ts_kissmeyerbasic`),
  `CTSMapGlobals` (weapon-restriction on `ts_trinityfall`), `CTSGroundWeapon`
  (`ts_wyndham` pickups), `CTSPowerUp` (22 on `ts_theatre`, bots pick up),
  `CTSGroundAmmo` (`ts_groundammo` refill pack, 6 on `ts_supertimor_d3`),
  `CTSSlowMotion` (`ts_slowmotion` zones, 4 on `ts_wyndham`). Built + synthetic/factory-only
  (no stock map hits them): `CTSFog`, `CTSTimer`, `CTSCustomTarget`, `CTSDMHill` (cont.32),
  `CTSTrigger` (cont.34), `CTSSlowMotionPoint`/`CTSShrapnel`/`CTSWorldGun`/`CTSAmmoPack`/
  `CTSDeadCorpse`/`CTSFlag` (cont.38, the CTF flag — carry/capture/drop) (runtime-created, not
  map-placed). **`CTSBriefcase`** (cont.39, `ts_briefcase`) is map-placed — verified on the real CTB
  map `ctb_deadheat` (2 briefcases each seed their team's CTSFlag at base + capture there).
- **Objective entities (team-mode maps) — runtime-verified:** `dlls/ts_objectives.cpp` —
  `CTSObjectiveManager`/`CTSObjectivePointer`/`CTSHack`/`CTSWinGiver`/`CTSTrigger` (+ the
  `CTSObjective` base). All spawn on `tm_avana` (0 Can't init, was 7) + `tm_corporate`; CTSHack
  counts `hackingtime` down to completion (fire target+HUD); CTSWinGiver Use declares the
  `winnerteam` → **`CTSGameRules::TeamWin` scores + restarts the round (cont.31)**; CTSTrigger
  fires its target on a valid team-gated touch (cont.34) — now driven by real `pev->team` (cont.35).
  **SLICE:** TeamWin cash + `ShowObjectiveOnHud` + the client objective HUD pointer are deferred.
- **Game rules:** `CTSGameRules : CHalfLifeMultiplay` IS the live `g_pGameRules`
  (`ts_gamerules.{h,cpp}` + `InstallGameRules`). Real TS overrides now: `GetRandomPowerup`,
  `Think` (dynamic powerups), `PlayerKilled` (death loot + `CallCheckDeads`), `TeamWin` +
  round-restart, `CheckDeads`/`CallCheckDeads` (cont.37 — team-round elimination win condition),
  `GlobalSlowMotion2`/`SetGlobalSlowMotion`, `PutPowerUp`. **Team subsystem (cont.35,
  `dlls/ts_teams.cpp`):** `IsTeamMode`/`IsTeamplay` (team mode iff a `ts_objective_manager` is
  present — the same DM proxy cont.26 used), `InitTeams`/`RecountTeams`/`GetTeamIndex`/
  `GetIndexedTeamName`/`GetTeamID`/`TeamWithFewestPlayers`, `AssignToTeam`, and a `PlayerSpawn`
  override that **auto-balances** any unassigned player onto the fewest team (server-side stand-in
  for the faithful client class menu). `pev->team` is **0-based** (0 spec, 1 merc) to match the
  FGD/objective gating. **Verified:** 5 bots balanced 3/2 on `tm_avana`; DM assigns nothing.
  Cash + free buy/armament are built (cont.36, `dlls/ts_buy.cpp` — `CashBased()` is off so weapons are
  free; `tsbuy`/`tsarm`/`tsbuyarmament` console-drive it, the menu UI is cl_dll); the round timer is
  built (cont.45, `StartRoundTimer`/`RoundTimeLeft` → draw on timeout). Still inheriting MP / deferred:
  the faithful `UpdateTeamScores`/`InitTeamScore` (vs the current `m_teamScore[]`), the full
  `RestartRound`@0x67920, and friendly-fire. `TSGameRules()` accessor + `TS_PWUP_*` bits.
- **Player:** no `CTSPlayer` — TS state lives on `CBasePlayer` (`player.h` member
  block `m_iPowerups`/`m_iHeldPowerup`/`m_flPowerupTime`/`m_flPowerupExpireTime`/
  `m_flHealthRegen`/`m_iKungFu`/`m_bLowGravity`) + accessors in `ts_player.{h,cpp}`.
  `TSHudMessageAll`/`TSHudMessageOne` in `ts_utils`.
- **Powerups — runtime-verified (slice):** `CTSPowerUp` (`ts_powerup.cpp`, entity
  `ts_powerup`, 328 placements) — full lifecycle (TOSS-drop → settle → touch-pickup
  → respawn); spawnflags = the random-powerup **mask** (not a single type). Picks a
  random type from `GetRandomPowerup`, applies it via free func `PlayerAddPowerUp`.
  **Instant powerups real** (kungfu/lowgrav/health/armor/grenade/inf-clip), **timed
  ones held** (`m_iHeldPowerup` + duration). **SLICE:** armor/inf-clip/grenade use
  functional substitutes (no `TakeArmor`/`MultiplyClips`/`PickUpWeapon`);
  `gmsgTSPowerUp` HUD + lowgrav physics-key deferred (client.dll side).
  **Dynamic spawning done (cont.26):** `CTSGameRules::PutPowerUp` + a `Think` override spawn
  a random powerup at a DM spawn every ~20s (DM maps only).
- **Powerup activation — runtime-verified (slice):** `CBasePlayer::UsePowerUp`
  (`usepowerup` console command) + `UpdatePowerUpState` (per-frame in `PreThink`) +
  `DeactivateSlowMotionFlow`, all in `ts_player.cpp`. A held timed powerup activates
  → `m_iPowerups` set (so `SlowMotion()`/`DoubleFireRate()` accessors flip + the slow
  fields `m_flSlowMotionScale`/`Ramp`/`m_bSlowBullets` are applied each frame) →
  expires after its duration → restores. `gmsgTSPowerUpTime`/`gmsgTSMotionBlur`
  registered + sent.
- **Slow-motion state + zones — runtime-verified (slice):** `CBasePlayer::GoSlow`/
  `InSlow` (`ts_player.cpp`; the TS slow-motion virtuals, as plain player methods —
  `GoSlow` sets `m_flSlowMotionGo`/`Target`+`m_bSlowBullets`, `InSlow` reports it),
  `CTSGameRules::GlobalSlowMotion2`/`SetGlobalSlowMotion` (`ts_gamerules.cpp`; slow
  all players, throttled ~0.5s), and the **`CTSSlowMotion`** brush zone (`ts_slowmotion`,
  11 placements, `ts_slowmotion.cpp`) — touch-slows players, tracks + drops them on
  leave, "All map" mode → `GlobalSlowMotion2`. Verified on `ts_wyndham`: zones spawn,
  `Use()` activates, `GlobalSlowMotion2(0.5)` flips `InSlow()`/`go=0.5`, restore → 1.0.
- **Powerup→slow propagation DONE (cont.33):** `CTSSlowMotionPoint` (`dlls/ts_slowmotionpoint.cpp`)
  is the slow-motion bubble — an activated slow-motion/slow-pause powerup spawns one at the player
  (`UsePowerUp`), and it `GoSlow`s every non-owner player with line of sight for the duration, then
  restores them. (Un-SLICEs the old "activated slowmo sets `m_flSlowMotionScale` but doesn't `GoSlow`".)
- **SLICE — the visible time-dilation layer is still deferred (client-entangled):** the
  actual slow-motion *movement* (the client pmove reads the slow fields — no cl_dll
  reconstructed); `GlobalSlowMotion2` slowing projectiles (needs `GoSlow`/`InSlow` as
  **CBaseEntity** virtuals — currently CBasePlayer-only); `InterpolateSlowMotion`; the
  client motion-blur HUD; the on-death powerup re-drop. `CTSSlowMotion`'s exact Spawn
  trigger-setup + `m_flRadius` are inferred (// VERIFY).
- **Weapons — runtime-verified, arsenal + all 38 types + fire-type dispatch (slice):**
  `CTSGun` (`ts_gun.cpp`, entity `weapon_tsgun`, id 16) holds a multi-weapon
  **arsenal** (`m_arsenalType/Clip/Mode[]` + active slot — a modern stand-in for the
  original `items[]`/`m_pActiveItem`; **ammo is per-CALIBER, `m_ammoReserve[16]`, shared**).
  `AddWeapon`/`SelectSlot`; `GiveTSWeapon` adds to the held gun; the `tsnextweapon` command
  cycles; per-slot clip/mode persist across switches; ground-weapon pickups stack into the
  arsenal. `PrimaryAttack` → fire-mode gate → `Fire()` dispatches by **fire-type** with
  recovered per-weapon rate/spread: **semi** (one/press), **full-auto** (held), **shotgun**
  (8 pellets), **melee** (knife 48u/katana 64u hull-trace, no bullets). Full `s_weapon_info`
  table in `ts_weapons.{h,cpp}` (incl. `flFireRate`/`flSpread`/`flRecoil`/`iCaliber`/`iPellets`/
  `iFireModes`). Roster precached at map load via `CWorld::Precache`.
  Powerup hooks wired: inf-clip `MultiplyClips`, double-firerate halves `flFireRate`.
  **Grenade fire-type (2) throws an M61** (`CM61Grenade`, `ts_grenade.cpp`); its blast now
  throws a **`CTSShrapnel`** fragment burst (cont.29).
  **Akimbo fire-types (4/5) dual-wield** (cont.21): picking up a 2nd of a single pistol you
  already hold upgrades that arsenal slot in place to its akimbo variant (doubled clip),
  via the recovered `s_weapon_info` +0xc8/+0xcc pairing; `AkimboFire` fires full-auto-feel
  alternating the muzzle side (±5u `v_right`).
  **Fire-mode system** (cont.22): a per-slot fire-mode (auto/semi/burst/pump) gates the
  trigger, cycled by `SwitchFireMode`/the `tsfiremode` command over the recovered
  `s_weapon_info+0xc` mode mask; `PrimaryAttack` → `Fire()` → `StandardFire`/`AkimboFire`;
  burst = 3 rounds (2 for the MAC-10), driven by `WeaponIdle`; defaults per `TakeWeapon`.
  **Per-caliber ammo** (cont.23): reserves are pooled by the weapon's `s_weapon_info+0x8`
  caliber (`m_ammoReserve[16]`, shared across same-caliber weapons); `AddAmmo`/Reload draw
  rounds capped at the `compile_ammo_info` max table; CTSGroundAmmo refills per caliber via
  `TSAddAmmo`.
  **Shotguns** (cont.24): fire the real **8 pellets** (`iPellets` = `s_weapon_info+0x24`);
  mossberg/sawedoff fire-types fixed (1 pump / 8 semi).
  **Dropped weapons** (cont.27, `dlls/ts_worldgun.cpp`): `CTSWorldGun` + the `dropweapon`
  command — drop the active weapon into the world (`RemoveActiveWeapon`), it falls/settles,
  another player presses **USE** to pick it up (`TSPickUpWeapon`).
- **Death handling — runtime-verified (cont.28, 30):** `CTSGameRules::PlayerKilled` override
  (`ts_gamerules.cpp`) drops the dead player's arsenal as `CTSWorldGun`s + a `CTSAmmoPack`
  (`dlls/ts_ammopack.cpp`, the per-caliber reserve, consumable) + spawns a fading `CTSDeadCorpse`
  (`dlls/ts_corpse.cpp`, the frozen studio pose, fades over 15s). Verified: kill a bot → 1 corpse
  + its weapons + an ammo pack drop (USE to grab guns; walk over the pack). SLICE: live-pose copy,
  no gib/blood, melee not dropped.
- **Objective win → round restart (cont.31):** `CTSGameRules::TeamWin` (`ts_gamerules.cpp`) — a
  `ts_wingiver` completing now scores the team + announces + restarts the round (respawn all +
  re-arm objectives) after 3s. SLICE: no cash, no round-timer HUD, no per-player team assignment.
- **King-of-the-hill (cont.32):** `CTSDMHill` (`dlls/ts_dmhill.cpp`, `ts_dmhill`) — hold the
  brush volume alone `nextcheck`s → `fragamt` frags + announce. Built + synthetic-verified (0
  stock placements; a hand-made map would confirm in-map).
  **SLICE limits:** akimbo is a single active clip (not a true left/right barrel split) with
  no IN_ATTACK2 "fire both barrels at once"; no buy menu (start reserve = clip*3); no
  client-side fire-mode/ammo HUD.
- **WATCH OUT:** `recovered/` decompiles can be **truncated** by Ghidra noreturn
  artifacts (cut `CTSModel::Spawn` at `UTIL_SetSize`, `CTSPowerUp::Spawn`/`OnGround`
  at `UTIL_SetOrigin` — the cut tail held a `nextthink` set + the `SetTouch`/`SetThink`
  arming). Cross-check raw `objdump -d` at the symbol's real `.so` addr (Ghidra adds
  0x10000 vs `nm`) when a decompile looks too short. Runtime crashes → **gdb the
  backtrace**; a "spawns but nothing happens" symptom → check for a missing
  `pev->nextthink` (think never fires).

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
`hlds64-buildchain:debian10` is already built. Ghidra/rizin/objdump read the
**original** `ts_i386.so`; **`gdb` debugs our own `hl.so`** — when a runtime test
segfaults, `gdb -batch -ex run -ex bt --args ./hlds_linux …` for the backtrace
instead of guessing (it found the `EV_Precache`/`DeathNotice` crashes). See
CLAUDE.md §4. For code needing a live **player** (weapons/powerups), the rig has a
working 64-bit **metamod+rcbot** bot setup (procedure + boot/gdb commands in
`reference/RECONSTRUCTION.md` "In-engine test rig"). For bulk **data tables** from
the `.so`, `scripts/extract_weapons.py` is the pattern (PIC string + numeric
extraction → C table).

## Recommended next steps
**The whole server side is done — Track A is COMPLETE (cont.35–45).** The clear next focus is
**Track B (the client/cl_dll)**, whose build foundation just landed (cont.46). Track A below is kept
for reference (all ✅) + its remaining *fidelity polish* (optional, low value). Read Track B first.

**→ Track B — the cl_dll (IN PROGRESS, now RUN + VISUALLY verifiable on this box).** The client builds
(`build.sh <arch> client`, 32- and 64-bit) AND **runs + renders in native xash3d-fwgs** — the cont.47
**`CHudRoundTime`** HUD is **visually confirmed** (a yellow `1:51` M:SS clock, top-centre, cont.48).

**IMPORTANT — this box is NOT headless** (the cont.46 caveat was wrong): it has an AMD GPU + X display
(`DISPLAY=:0`). So mod-ts client work is **self-verifiable here with a screenshot** — no need to wait on
the user. The loop:
```bash
scripts/build.sh amd64 client          # (and amd64 for the server)
scripts/setup-xash-mod.sh              # assemble the native xash game root (re-run after every rebuild)
cd ~/Projects/goldsrc.net/xash3d-fwgs/dist/client-x64
LD_LIBRARY_PATH=. ./xash3d -dev 2 -game ts +map tm_avana +sv_lan 1 +maxplayers 4 &
# wait for "client connected" in the log, then:  import -window root /tmp/shot.png
# crop the game window's top-centre (window @ 1280,180, 2560x1080):
#   convert /tmp/shot.png -crop 1000x220+2060+180 +repage /tmp/clock.png
```
gdb the client the same way as hl.so when it crashes (it found the cont.48 CGlock/AllocString crash):
`LD_LIBRARY_PATH=. gdb -batch -nx -ex run -ex bt --args ./xash3d -game ts +map tm_avana`.

**→ The 5-phase reconstruction — STATUS THROUGH cont.64** (full chronological detail in the Serena ledger
cont.51–64; the recovered contracts are in `reference/ENGINE_INTERFACE.md` / `API_SURFACE.md` /
`HUD_MESSAGES.md` / `HUD_DRAWCHAIN.md` / `VERIFICATION.md`).
- **Phase 4 — TS client HUD ✅ DONE + playtest-verified (cont.51–53).** Three regions from `client.dll`
  assembly: **weapon-select bar** (`CHudTSWeaponBar`, real `sprites/weapons/*.spr` icons, fed by `WStatus`),
  **ammo box** (`CHudTSAmmo` ← `WeaponInfo`), **health bar** (`TSHud_HealthBar`). Exact geometry/colour/
  alpha/font; HUD colour `hud_r/g/b`=150/175/255; per-char clipped `TSHud_DrawString` (= `FUN_10021ce0`).
  Polish left: attachment rows (need the WeaponInfo flags byte + silencer), active-highlight, left block.
  **Status panel ✅ (cont.69b, `cl_dll/ts_status.{cpp,h}`):** the bottom-left **TS status panel** that
  replaces the stock HEV health/battery — team name + a **posture figure** (`sprites/player/{stand,crouched,
  prone}.spr`, RE'd from the @0x10049d1f sprite-load block) + **health number + heart** (`health.spr`) +
  a binary **"Kevlar"** indicator (`kevlar.spr`, drawn only when `m_iArmor>0`; armor is kevlar-or-not, NOT
  a number — the old "Armor: N" was wrong). Crouch live via local `curstate.usehull==1`; **prone is
  CTSStunt-gated** (dormant `g_iTSProne` hook). Screenshot-verified stand/crouch/100+heart/Kevlar. OPEN:
  the buy/loadout doesn't yet grant kevlar armor (server gap); prone posture awaits CTSStunt.
- **Phase 3 — commands/cvars ✅ DONE (cont.54).** ~25 client + ~25 server cvars registered with exact
  defaults; commands registered. No "Unknown command" spam. Several cvars now **wired to gameplay**:
  `roundtime`, `slowmatch` (→ global slow-motion match), `damagemult` (→ weapon damage), `initial_cash`
  (→ starting cash, cash HUD). Status table: `API_SURFACE.md` §6c.
- **Phase 5 — predicted fire event 🟡 PARTIAL (cont.56, fixed cont.64).** Firing uses `events/fire.sc` →
  `EV_TSFire` (`cl_dll/ev_ts.cpp`) for the **fire sound + brass shell + viewmodel muzzleflash**; the
  **viewmodel "shoot" anim is server-driven via `SendWeaponAnim`** (cont.64 — the event's EV_WeaponAnimation
  doesn't stick since CTSGun isn't client-predicted). True client prediction (CTSGun in HUD_PostRunCmd) TODO.
- **Phase 2 — low-pitch fire sound ✅ INVESTIGATED, no defect (cont.55).** Sound path correct; re-test.
- **Phase 1 — weapon firing/reload/ammo ✅.** `+alt2` coldcock done (cont.57/64). TODO: fire-rate gate,
  `+alt1`/silencer (UNMAPPED — `VERIFICATION.md` §B), akimbo sound via the event.

**Other (cont.57–64):** `+alt2` coldcock pistol-whip; **thirdperson fixed** (MP gate removed + camera
behind the player, cont.61/64); `GetGameDescription` → "The Specialists (mode)"; **VISIBLE slow-motion
movement** (PreThink scales speed cap by the slow factor, cont.62 — the marquee effect).

**NEXT FRONTIER (all gameplay/subsystem, see `ENGINE_INTERFACE.md` "notable gaps"):**
1. ✅ **`CTSAnimation` — DONE (cont.65, `dlls/ts_animation.{h,cpp}`).** The player 3rd-person gait
   (idle/walk/run/crouch/jump/fire/reload by grip) is reconstructed + **visually verified** (the always-idle
   bug is fixed). The original `SetAnimation` is a no-op stub; TS drives the body via `CTSAnimation::Animate`
   (now an embedded `m_TSAnim` member, driven from PostThink/fire/reload/death). pev->sequence = torso
   (`idle_/aim_/shoot_/relo_/draw_<grip>` + crouch_/duck_ variants, jump, die_spin, grenade); pev->gaitsequence
   = legs (run2/walk2/crawl). **Deferred (CTSStunt-gated, never fires until CTSStunt lands):** dives/cartwheels/
   walljump/morfy/rolls (activities 0x28-0x32,0x36-0x39); also TODO: directional-aim strafe blend
   (BlendAnimationsByDir, 8-way), death-by-direction variants (we use die_spin), confirm the fire/aim torso
   pose (hard from the behind-camera — needs a front view or rcbot).
2. **Full pmove time-dilation** — slow-motion movement is wired (speed cap); gravity/jump-arc/anim-rate
   time-dilation needs the slow value in client-predicted `pm_shared`.
3. **`CTSStunt`** (dives/rolls/wall-flips, 25 funcs, PostThink — unlocks the deferred CTSAnimation branches) +
   **true client weapon prediction** (HUD_PostRunCmd) + the **`+alt1` silencer** (unmapped) + the **buy
   menu** (cl_dll VGUI).
4. **rcbot-in-xash rig** would make combat-gated things testable (damagemult, coldcock damage, fire path).

**Workflow:** build → `setup-xash-mod.sh` → run in xash (the loop above) → screenshot/gdb to verify →
commit → update docs. Every message/event/command/cvar name must be **byte-faithful to the `.so`** (the
swap-compatibility contract — see "Where things stand"); check `strings reference/ts_i386.so` /
`reference/client.dll` before naming anything.

**Pattern for a TS HUD element (cont.47, follow `cl_dll/roundtime.{cpp,h}` + the stock `battery.cpp`):**
(1) confirm the server's REG_USER_MSG string matches the original `.so` (`strings reference/ts_i386.so`);
(2) `class CHud<X> : CHudBase` with `Init`/`VidInit`/`Draw`/`Reset`/`MsgFunc_<Msg>`; `DECLARE_MESSAGE` +
`HOOK_MESSAGE(<Msg>)` in `Init` + `gHUD.AddHudElem(this)`; (3) read with `BEGIN_READ`/`READ_SHORT`;
(4) wire `#include` + `CHud<X> m_<X>;` into `hud.h`, `m_<X>.Init()`/`.VidInit()` into `hud.cpp`, and the
`.cpp` into `cl_dll/CMakeLists.txt`; (5) `build.sh amd64 client` + `i386 client`, `nm|c++filt` for the
methods, `strings` for the hook name.

---

**Track A — the `CTSGameRules` team subsystem + cash (✅ COMPLETE, cont.35–45).** Kept for reference;
all steps done. Remaining is fidelity polish only (UpdateTeamScores/InitTeamScore, the full faithful
RestartRound, ShowObjectiveOnHud, CheckDeads grace/CTB/last-man branches) — diminishing returns.
1. ✅ **Team assignment (DONE, cont.35)** — `dlls/ts_teams.cpp`: `pev->team` set 0-based via
   auto-balance on spawn (`PlayerSpawn` → `TeamWithFewestPlayers`); the team roster + GetTeamIndex/
   GetTeamID mirror `CHalfLifeTeamplay`. Verified: bots get balanced teams. The CTF/escape entities
   (step 4) are now unblocked. (Faithful join/class menu is cl_dll — deferred.)
2. ✅ **Cash + buy/armament (DONE, cont.36)** — `dlls/ts_buy.cpp`. **Major finding: `CashBased()` is
   hardcoded OFF in the .so**, so weapons are FREE — the buy system is a free armament (loadout)
   selector, not an economy. `GiveCash`/cash member kept (cash-for-win/lose, future sell zone) but
   never charged. `ResetArmament`/`AddItemToArmament`/`AddAmmoToArmament`/`BuyWeapon`/`BuyAmmo`/
   `BuyArmament` reuse `GiveTSWeapon`/`TSAddAmmo`; M61 is team-mode-only + capped 2/life. Driven by
   console commands (`tsbuy`/`tsarm`/`tsbuyarmament`/…) since the menu UI is cl_dll. Verified on
   tm_avana + DM. **Remaining buy SLICE:** the cl_dll buy menu, the category weapon restriction
   (`IsRestrictedWeapon` over CTSMapGlobals — default 0x1f allows all), starting/win cash, and the
   dead price field (`s_weapon_info+0xd8`, only live if a cash economy is ever turned on).
3. **Team scores + the full round** — ✅ elimination (cont.37) + ✅ the **round timer** (cont.45):
   `CheckDeads`/`CallCheckDeads` end a round when a team is wiped out; `StartRoundTimer`/`RoundTimeLeft`
   time the round out (default 120s) → fire `targetontimeleft` + declare a draw. Rounds now end three
   ways (elimination / objective / timeout), all via `TeamWin` + restart. **Still TODO:** the faithful
   `UpdateTeamScores`/`InitTeamScore` (vs the current `m_teamScore[]`), wiring the `roundtime` cvar
   (currently a fixed 120s), the full `RestartRound`@0x67920 (game-mode member + per-player round
   setup), `CTSObjective::Freeze`→`ShowObjectiveOnHud`, and the just-respawned grace + the CTB
   (mode-3) and last-man-standing branches in CheckDeads. These are fidelity polish, not new systems.
4. The **CTF/escape entities** (all need teams; ≈0 stock BSP placements → synthetic-verify only):
   ✅ **`CTSFlag` DONE (cont.38, `dlls/ts_flag.cpp`)** — the carry/capture/drop CTF loop, team-gated.
   ✅ **`CTSBriefcase` DONE (cont.39, `dlls/ts_briefcase.cpp`)** — the map-placed CTF base/capture
   zone: seeds its team's CTSFlag at base (FlagFall) + captures a carrier at their own briefcase.
   Verified on the real stock map **`ctb_deadheat`** (2 briefcases each seed their flag, team 0 + 1).
   This replaces CTSFlag's `tsspawnflag` console stand-in for CTB maps.
   ✅ **`CTSBomb` DONE (cont.40, `dlls/ts_bomb.cpp`)** — the defuse/wire-cut objective: a planted
   bomb counts down + explodes unless defused (hold USE, or cut the right of 3 wires via the
   `tscutwire <n>` console command — the wire UI is cl_dll). Synthetic-verified (0 stock placements).
   ✅ **`CTSSellZone` DONE (cont.41, `dlls/ts_sellzone.cpp`)** — sells a dropped weapon (ts_worldgun)
   touching it for cash to the nearest player (the one place the cont.36 cash flows). Verified on the
   real map `ts_dust2`. (Also fixed a pre-existing crash there: the dynamic powerup spawner
   runtime-precached `models/powerup.mdl` → moved to map-load precache.)
   ✅ **`CTSTeamEscape` DONE (cont.42, `dlls/ts_escape.cpp`)** — escort zone, completes when
   `escapepercentage` of the attacker team is simultaneously inside.
   ✅ **`CTSScoreGiver` + `CVipEscape` DONE (cont.43, `dlls/ts_objectives.cpp`)** — `ts_scoregiver`
   = a repeatable team-score giver; `func_vip_safetyzone` = a passive VIP safety-zone brush marker.
   **→ The Track A FGD entity list is COMPLETE.** What's left in Track A is faithful polish, not new
   entities: `UpdateTeamScores`/`InitTeamScore` (vs the current `m_teamScore[]`), the round timer
   (`gmsgTSRoundTime`) + the full `RestartRound`@0x67920, `CTSObjective::Freeze`→`ShowObjectiveOnHud`,
   the CheckDeads grace/CTB/last-man branches, and the fact that CTF capture scores via `TeamWin`
   (which restarts the round) + CTSBomb's detonation just blasts. **Non-FGD leftovers:**
   ✅ `CTSPlayerAmount` + `CTSSuperJump` DONE (cont.44). Remaining: `CInfoBreakable` (a respawning
   breakable — thin, low value) and `CBulletManager` (the traveling-bullet / bullet-time system —
   substantive but DEFERRED: it would rewrite the verified instant-hitscan fire path and isn't visible
   without a cl_dll; best done alongside Track B so the slow bullets can actually be seen).

**Track B — the cl_dll (the client half of TS).** ✅ **FOUNDATION DONE (cont.46): the client now
BUILDS, both 32-bit and 64-bit** — `scripts/build.sh <arch> client` (or `all`). Three gcc-8.3
portability fixes (memory_resource shim, GL enum defines, a 64-bit pointer cast) + dual-arch VGUI:
i386 links Valve's `vgui.so`, amd64 links openvgui's ABI-compatible `libvgui.so` (vendored
`linux/release/vgui64.so`, from xash3d-fwgs) + a 64-bit SDL2. No TS client code yet — the stock HL
client is the base, mirroring how the server started. **VERIFICATION CAVEAT:** this headless box can
build but not RENDER the client (no graphics; the browser client is the separate WASM xash build),
so TS client work is build/load-verified here — visual behaviour needs a graphical client run by the
user. The server already registers + sends the user messages; the client just needs the TS readers.
This unlocks:
- The **visible slow-motion time-dilation** — the server state is complete (`GoSlow`/`InSlow`/
  `CTSSlowMotionPoint`/`GlobalSlowMotion2`), but nothing *reads* the slow fields to scale time. The
  client `pm_shared`/prediction must scale frametime by `m_flSlowMotionGo`; also the motion-blur HUD
  (`gmsgTSMotionBlur`/`gmsgTSPowerUpTime`), the ammo/fire-mode HUDs, the objective HUD pointer.
- The **particle system**: `CTSSharedEmitter` (12) + `CTSEmitter`/`CTSParticle` (the `ts_emitter`/
  `ts_particle` Can't-inits, ~32 placements — the most common remaining) + `CTSWind` (snow), all via
  `gmsgTSParticleSystem`. Needs the cl_dll renderer.
- `CTSStunt` (25 — the signature acrobatics: dives/rolls/wall-flips) is movement prediction →
  client-entangled; `CTSAnimation` (14) similar. `reference/client.dll` (~1,013 exports) is the
  spine — see CLAUDE.md §2.

**Small server-side leftovers** (optional, low value): `CTSPlayerAmount` (player-count relay),
`CTSScoreGiver`, `CTSSuperJump`, `CInfoBreakable`, `CBulletManager`; the per-weapon leaf classes
(`CTSPistolAmmo` etc.) are redundant with the unified `CTSGun`/`ts_weapons` table.

Loop per class: decompile → write `dlls/*.cpp` → add to `dlls/CMakeLists.txt` →
`scripts/build.sh amd64` → `cp` into the rig → boot a map that uses it (frequency
table in `reference/RECONSTRUCTION.md`) → confirm `Can't init <class>` gone (+ temp
`ALERT` probe / bots for behaviour) → **gdb any crash** → revert temp scaffolding,
rebuild clean → commit + update docs.

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

## Keep the docs current (this file is the living pickup brief)
**Context runs out between sessions — a fresh agent must be able to resume from
this file alone.** So at every checkpoint/commit, update *all three* together:
- **`HANDOFF.md`** (this file) — rewrite "Where things stand" + "Recommended next
  steps" to match reality. If it disagrees with the code/git log, it's a bug — fix
  it. Don't let it drift to "predates the last N commits".
- **Serena ledger** (`write_memory("ledger", …)`, newest on top) — append what was
  done, how verified (with the runtime evidence), what's next.
- **`reference/RECONSTRUCTION.md`** — the live status table + frequency/dep tables.

Rule of thumb: if you just committed code, you're not done until these three
reflect it. (`CLAUDE.md` is local-only/gitignored — it carries stable
mission/tooling, but the *durable, tracked* pickup state lives in the three above.)
