# mod-ts change ledger

Running log of reconstruction progress. Newest entries on top. Keep factual:
what changed, how verified, what's next. See `CLAUDE.md` for mission/method.

---

## 2026-06-22 (cont. 51) — Phase 4: TS custom HUD begins (health/armor/cash) + the user-message wire contract recovered + 3 protocol-name bugs fixed

**Phase 4 (the TS custom HUD) started.** First recovered the FULL TS user-message contract from the
`.so`, then landed the first faithful HUD elements (health/armor/cash) end-to-end (server send + client
draw), VISUALLY VERIFIED in xash3d-fwgs.

**The message contract — recovered + documented in `reference/HUD_MESSAGES.md` (new, tracked).** Parsed
`LinkUserMessages @0xc7db0` (every REG_USER_MSG name + fixed size) and decompiled the two senders —
`CBasePlayer::UpdateClientData @0x81d00` and `CTSGun::UpdateClientData @0x758c8` — for the exact wire
packing. The TS block (names ON THE WIRE, sizes, format): `TSHealth`(1)=BYTE round(health)≥0,
`TSArmor`(1)=BYTE round(armor), `TSState`(1)=BYTE state[0x498], `TSCash`(2)=SHORT cash, `TSSpace`(1)=BYTE,
`WeaponInfo`(6)=BYTE wtype,BYTE,SHORT reserve,BYTE,BYTE (from the per-arsenal-slot struct @gun+0xcc, NOT
our s_weapon_info subset), `WStatus`(3)=per-slot BYTE×3, `ActItems`(2)=MSG_ALL BYTE entindex+BYTE,
`TSPAward`(2)=MSG_ALL, `PwUp`(3), `PwUpTime`(1), `RoundTime`(2)=BYTE secs+BYTE flag(0 start/2 timeout),
`TSMBlur`(2), `TSSlowMo`(2)=COORD(slowamt*100), `TSBTime`(1), `Wires`(3), `TSFrags`(-1), `TSPSys`(-1)…
Engine write-fn GOT offsets: `190fe0`=Byte `190fe8`=Short `190fec`=Long `190ff4`=Coord.

**3 inherited PROTOCOL-NAME/SIZE bugs fixed (swap-compat — the cont.50 mandate):**
- `RoundTime`: was registered -1 + written as TWO SHORTS (server) / read as two shorts (client). Original
  is **fixed size 2 = two BYTES** (seconds, flag). Fixed `dlls/ts_gamerules.cpp` (StartRoundTimer +
  SendRoundTime → WRITE_BYTE×2, REG size 2) AND `cl_dll/roundtime.cpp` (READ_BYTE×2). Clock still
  counts down correctly (verified "1:42"); 120s round max fits a byte.
- `TSMotionBlur` → **`TSMBlur`** (size 2) — wrong NAME + size (`dlls/ts_player.cpp`).
- `TSPowerUpTime` → **`PwUpTime`** (size 1) — wrong NAME (`dlls/ts_player.cpp`).

**Health/armor/cash HUD — DONE + VISUALLY VERIFIED:**
- Server (`dlls/ts_player.cpp` `CBasePlayer::TSUpdateClientData()`, called at the end of
  `CBasePlayer::UpdateClientData`): lazily registers + sends `TSHealth`(byte)/`TSArmor`(byte)/`TSCash`
  (short) on-change, cached in new `player.h` members (`m_iClientTSHealth/Armor/Cash`, reset on HUD init
  so they resend after respawn). Byte-faithful to the original (round+clamp health, round armor). The
  original sends THESE instead of stock Health/Battery — TS has a fully custom HUD.
- Client (`cl_dll/ts_status.{cpp,h}`, new — `CHudTSStatus`): hooks TSHealth/TSArmor/TSCash, draws
  "Health: N" / "Armor: N" / "$N" as text bottom-left (DrawHudString, gold / red when health≤25). Wired
  into hud.h/hud.cpp/CMakeLists like CHudRoundTime. Built both halves (amd64), nm/strings confirm the
  factories + hook names; ran in xash3d-fwgs on tm_avana → screenshot shows **"Health: 100" / "Armor: 0"**
  bottom-left + the round clock. No "Bad message" size warnings in the log.

**STILL TO DO in Phase 4 (next):**
- **AMMO HUD (the big one) — `WeaponInfo`/`WStatus` need the per-arsenal-slot struct (@gun+0xcc, 0x70/slot)
  decoded** to pin the 6 WeaponInfo bytes byte-faithfully (weapontype + reserve are known; the clip + the
  3 middle bytes come from the slot struct, which our modern `CTSGun` stores differently). CTSGun sends
  WeaponInfo/WStatus, NEVER ClipInfo — so the current clip rides inside WeaponInfo's slot snapshot. This
  is a consult-worthy ambiguity (raised to the user/prev-agent): exact slot-struct field meanings.
- `TSState` (player state byte @0x498 — semantics TBD), `TSSlowMo` (COORD slowamt — the visible
  time-dilation HUD), `TSBTime`, `PwUp`/motion-blur HUD readers, `TSFrags`/scoreboard.
- The leftover **stock HUD strip** still bleeds through at the very bottom (faint "00 |", player icon, "0")
  — the stock sprite HUD half-loading; should be suppressed once the TS HUD is complete.

**Verify loop used:** `scripts/build.sh amd64` + `amd64 client` → `setup-xash-mod.sh` → launched xash
(killed the 26-min idle prior instance first) → `import -window root` → cropped the 2560x1080+1280+208
window (`xwininfo` for geometry). Left a fresh instance running for the user.

---

## 2026-06-22 (cont. 50) — playtest reconstruction (reload, akimbo anims, HUD res) + the COMPAT TARGET clarified + docs sync

**Compatibility target clarified by the user (now in CLAUDE.md + HANDOFF):** the goal is INTERFACE/
protocol compatibility so each half is a **drop-in swap** for the original — our client ↔ the original
server (or vice-versa) behaves identically. Byte-faithful: the engine C interface + the client↔server
protocol (user-message **names + wire formats**, **event** names/params, **command** names, **cvar**
names). NOT a target: the internal C++ ABI / member offsets / gcc-2.x quirks (we build on the newer
Itanium-ABI halflife-updated SDK). This is exactly why `TSRoundTime`→`RoundTime` mattered, and it makes
protocol-level decomp fidelity mandatory for every message/event/command/cvar going forward.

**Done this session (all decomp-grounded, committed):**
- **Akimbo fire anims (e5ca177)** — `AkimboFire` SendWeaponAnim's the matching barrel's shoot sequence
  (right=`iFireAnim`, left=`iFireAnimL`, parsed from the `v_*_akimbo.mdl`). User-confirmed: anims + flare
  side switch correctly.
- **HUD sprite-resolution fallback (e5ca177, `cl_dll/hud.cpp`)** — TS's `hud.txt` is 320/640-only; the
  modern client picks `m_iRes`=1280 at 2560-wide → matches nothing → no sprite HUD. `CHud::VidInit` now
  falls back to the largest res present. NECESSARY BUT NOT SUFFICIENT: TS has a *fully custom* client HUD,
  so there is still NO HUD at all (no health/ammo/crosshair) — that's Phase 4 below.
- **Reload (241ba55, `dlls/ts_gun.cpp` + weapon table)** — root cause of "fire til empty, no reload": the
  stock `CBasePlayerWeapon::ItemPostFrame` gates reload on the player's standard `m_rgAmmo`, which TS never
  populates (ammo is in `CTSGun::m_ammoReserve` per caliber). Added `CTSGun::ItemPostFrame` that drives the
  TS ammo system: reload on `+reload` or auto when the clip runs dry, refill from the per-caliber reserve
  after `TS_RELOAD_TIME`, plays the per-weapon `reload` viewmodel sequence (`iReloadAnim`, parsed from each
  `v_*.mdl`). Built clean, runs in xash without crash (awaiting user playtest confirmation).

**User mandate: reconstruct the player/weapon/input/HUD systems WHOLESALE from the decomp** (stop the
symptom-patching). The 5-phase plan is tracked as tasks #6–#10 and in HANDOFF "Track B playtest
reconstruction": (1) weapon firing/reload/ammo/fire-rate [reload done], (2) CBasePlayer thinks/state + the
low-pitch fire sound (suspect slow-motion time-scale), (3) all TS commands/cvars/+toggles server+client,
(4) the TS client HUD (why there's no HUD), (5) client view bob/sway + predicted fire events.

**Open playtest reports (unfixed):** no HUD at all (Phase 4), regular fire sound is LOW-PITCHED ("slow-mo",
Phase 2 — confirmed pitch not the file: `beretta/fire.wav` is normal 22kHz), akimbo fire SOUND missing
(`g_weaponInfo[8].pszFireSound` is null — akimbo should fall back to the single's sound; quick Phase-1
fix), viewmodel bob "snap" (user confirmed `cl_bob 0` stops it; Phase 5), fire-rate feels ungated (Phase 1).

**Docs synced (this entry):** CLAUDE.md (compat-target rewrite + a Running/testing section: xash run loop,
`pgrep -x xash3d` not `pkill -f`, GPU-present), HANDOFF.md (compat target + the 5-phase plan + cont.47–50
status), RECONSTRUCTION.md (status rows). CLAUDE.md stays local-only (gitignored); the rest committed.

---

## 2026-06-21 (cont. 49) — TRACK B playtest pass: loadout, mouselook, fire animations (decomp-driven)

The user playtested the running client (cont.48) and reported HL leftovers. Fixed three, each
decomp-grounded (the user explicitly asked to check the decomp, not guess). The dev box launches the
game for the user via `setsid env LD_LIBRARY_PATH=. ./xash3d -game ts +map tm_avana &` (kill the old one
with `pgrep -x xash3d` — NOT `pkill -f client-x64/xash3d`, which matches the shell's own cmdline and
SIGTERMs your session). A detached/unfocused instance exits clean (~exit 0) after a bit; it stays up while
the user interacts.

1. **Spawn loadout (commit 54dac48, `dlls/ts_teams.cpp`).** Players spawned with HL crowbar+glock because
   `CTSGameRules::PlayerSpawn` chained `CHalfLifeMultiplay::PlayerSpawn`. Decompiled the real
   `CHalfLifeMultiplay::PlayerSpawn @0x790ac`: it honours `game_player_equip`, else
   `GiveNamedItem("weapon_tsgun")` (PIC strings resolved via GOT-0x6dda6) + `BuyArmament()` (cont.36).
   Reconstructed exactly → no HL weapons; spawn gives the unified TS gun (default `m_weaponType` =
   TSW_BERETTA, so you get a beretta) + the armament. (`CHalfLifeMultiplay::PlayerSpawn` itself is an
   11-byte forwarder to FUN_000890b8; `CBasePlayer::TSInit @0x92a4c` inits CTSAnimation/CTSStunt +
   `SetPlayerSpeed(330)`, gives no weapons.)
2. **Mouselook (commit + `cl_dll/inputw32.cpp`).** "Can't look up/down, only side-to-side; moving jolts
   the view fwd/back" = mouse-look OFF (mouse-Y walks you instead of pitching). The rig config never
   issues `+mlook`; TS is always-mouselook. Forced `in_mlook.state |= 1` in `IN_Init`. User CONFIRMED
   fixed. (Note: the *viewmodel* jolting back-and-forth is SEPARATE — still open; it's the client view
   bob/lag/sway, `cl_bob "3"` in the TS config + stock V_CalcBob/V_CalcViewModelLag — needs the client
   view reconstruction.)
3. **Fire animations (commit c607b75, `dlls/ts_weapons.{h,cpp}` + `ts_gun.cpp`).** Weapons fired
   (bullets/sound/muzzleflash already present) but the viewmodel never animated. Added `iFireAnim` to
   `s_weapon_info`, populated per-weapon from the actual "shoot*" sequence index parsed out of each
   `v_*.mdl` (studiohdr numseq/seqindex at offsets 164/168; 176-byte seqdesc; beretta shoot=1, glock=3,
   m4=3, aug=2, shotguns shootsemi=6, …; -1 for melee/grenade/akimbo). `StandardFire` now
   `SendWeaponAnim(wi->iFireAnim)`. SLICE: server-driven (visible, not predicted).

**Firing event system — fully MAPPED from the decomp (the faithful follow-up, task #6).** `CTSGun`
precaches 11 events (`events/fire.sc` → `m_usFire` @0x964, +coldcock@0x966/pump@0x96a/closecombat@0x96c/
firelinked@0x96e/reload@0x970/knife@0x972/katana@0x974/draw@0x976/grenade@0x978/empty@0x968). `StandardFire
@0x824a4` plays it: `PLAYBACK_EVENT(flags=1, player_pev, m_usFire, delay=0, vecZero, vecZero, fparam1=spread,
fparam2=spread, iparam1=m_weaponType(0xc8), iparam2=item+0x10, bparam1=(clip==0), bparam2=player[0x1fb])`.
The faithful client side = hook `events/fire.sc` → `EV_TSFire` reading iparam1=weapontype → predicted
sound + viewmodel anim + muzzleflash. NOT yet built (needs the client.dll EV callback). Our current sound
is a server EMIT_SOUND of `weapons/<w>/fire.wav` (22kHz, correct format); the user's "slow-mo sound"
report is unresolved (headless = no audio) — clarify low-pitch vs wrong-file vs droning.

**Still open (Track B client reconstruction):** the viewmodel jolt/bob, the predicted fire event (sound+
anim), the akimbo/melee/grenade fire anims, and the HL weapon-select HUD bar.

---

## 2026-06-21 (cont. 48) — TRACK B: the client RUNS + RENDERS in xash3d-fwgs; round-time HUD VISUALLY VERIFIED

**Headline: the dev box is NOT headless — it has a real GPU + X display (AMD RX 9070 XT, DISPLAY=:0).**
The cont.46 "headless, build-only, visual needs the user" caveat was WRONG. mod-ts client work can be
**self-verified with screenshots here** (`import -window root`). This changes the Track B workflow:
build → run in native xash3d-fwgs → screenshot → confirm. Use it.

**Done — mod-ts now runs end-to-end in native xash3d-fwgs (server + OUR client + content), and the
cont.47 round-time HUD is confirmed on-screen: a yellow `1:51` M:SS countdown at top-centre.**

1. **`scripts/setup-xash-mod.sh` (new) — the deliverable** ("configure xash to run this mod"). Assembles a
   native game root at `xash3d-fwgs/dist/client-x64` (which already holds the built engine: xash3d +
   libxash + libref_gl + libmenu + filesystem_stdio + libvgui). It builds `ts/` (content symlinks to
   `~/Containers/hlds/mods/ts`, our libs, a direct-gamedll liblist.gam — no metamod), completes the
   `valve/` fallback, and exposes the openvgui lib as `vgui.so`. **Three gotchas it encodes:**
   - xash on **x86-64 looks up the ARCH-SUFFIXED name** `hl_amd64.so` / `client_amd64.so` (the
     `COM_GenerateServerLibraryPath` `#else` branch — `XASH_X86` is 32-bit only). Misnamed `hl.so` was
     SILENTLY skipped → it loaded `valve/dlls/hl_amd64.so` instead (symptom: `No spawn function for
     "ts_hack"/"ts_sellzone"/...` for the LATER-added TS entities while older ones worked). Fix: name our
     libs `*_amd64.so` (bare name kept as an alias).
   - the cont.46 client links `DT_NEEDED vgui.so`; the engine ships the identical openvgui as
     `libvgui.so` (byte-for-byte = mod-ts/linux/release/vgui64.so) → symlink `vgui.so -> libvgui.so` and
     launch with `LD_LIBRARY_PATH=.`. libSDL2 resolves from the system.
   - Launch: `cd dist/client-x64 && LD_LIBRARY_PATH=. ./xash3d -dev 2 -game ts +map tm_avana`.
2. **Client crash fix (commit f484842) — `cl_dll/com_weapons.{cpp,h}` + `cl_dll/hl/hl_weapons.cpp`.**
   First actual RUN of the client SIGSEGV'd (user caught the "Xash Error: signal 11" dialog). gdb
   backtrace: `HUD_PostRunCmd → HUD_InitClientWeapons → HUD_PrepEntity → CGlock::Spawn → 0x0`. Pinned the
   exact insn (`call *g_engfuncs+536` right after loading the "weapon_9mmhandgun" string): on **64-bit**,
   `MAKE_STRING()` = `ALLOC_STRING()` = `(*g_engfuncs.pfnAllocString)` (util.h `#if __x86_64__`), but
   `HUD_InitClientWeapons` populates only a SUBSET of `g_engfuncs` and never sets `pfnAllocString` → the
   first `MAKE_STRING()` in any predicted weapon's Spawn deref'd NULL. Fix: a small client string pool +
   `stub_AllocString`/`stub_StringBase` (com_weapons), with `gpGlobals->pStringBase` pointed at the pool
   in HUD_InitClientWeapons so MAKE_STRING/STRING stay consistent (offset 0 reserved as ""). This is a
   generic halflife-updated 64-bit client-prediction bug, not TS-specific.
3. **Round-time-on-join (commit fd5146a) — `dlls/ts_gamerules.{h,cpp}` + `ts_teams.cpp`.** `StartRoundTimer`
   only broadcasts `RoundTime` at arm-time, so a client joining mid-round saw a blank HUD. Added
   `CTSGameRules::SendRoundTime(player)`, called from `InitHUD`, to catch a joiner up (no-op if unarmed).

**Verified (RUNTIME + VISUAL):** `setup-xash-mod.sh` → `LD_LIBRARY_PATH=. ./xash3d -game ts +map tm_avana`
→ strace confirms it loads OUR `ts/dlls/hl_amd64.so` + `ts/cl_dlls/client_amd64.so`; gdb-clean for a full
35s session (only SIGINT, no SIGSEGV); screenshot shows the world rendering (viewmodel, map) + the
**yellow `1:51` round clock** top-centre, window titled "mod-ts (faithful reconstruction)". The round
timer armed (counting from 120s) and the client read+drew the message. Server-side regression: tm_avana +
ts_dust2 load with 0 "No spawn function" once the dll name was fixed.

**Next Track B slices (now screenshot-verifiable):** the powerup/motion-blur indicators
(`TSPowerUpTime`/`TSMBlur` — verify names vs the .so first), the ammo/fire-mode HUDs, then the visible
slow-motion time-dilation + the particle system. Also noted but deferred: the stock client logs
`Unknown command cl_realhud / hud_r/g/b/color / mp_teammodels` — TS client cvars the original defined,
not yet reconstructed (harmless). The bg game process exits cleanly (exit 0) after a bit when launched
detached/unfocused — not a crash; interactive runs stay up.

---

## 2026-06-21 (cont. 47) — TRACK B: first TS client HUD — the round-time countdown (CHudRoundTime)

**Done — the first piece of TS client code: `cl_dll/roundtime.{cpp,h}` (CHudRoundTime).** The round-time
countdown HUD, reading the message the server already sends (cont.45). This is the first TS reader in
the cl_dll — proves the Track B loop (server sends a user message → client HUD element hooks + reads +
draws) end to end at build/load level.

**Fidelity fix to the server first (`dlls/ts_gamerules.cpp`):** the original `ts_i386.so` registers the
round-time user message as the string **`"RoundTime"`** (the C++ global is named `gmsgTSRoundTime` — both
appear in the unstripped symtab/.rodata; the registered STRING is "RoundTime", confirmed at .so .rodata
0x95900 where client.dll also hooks it). The inherited cont.45 server registered `"TSRoundTime"` — a
fidelity bug (client.dll hooks "RoundTime"). Changed `REG_USER_MSG("TSRoundTime")` → `REG_USER_MSG("RoundTime")`;
kept the var name `gmsgTSRoundTime` (faithful). Wire format unchanged: two shorts — `WRITE_SHORT(seconds
remaining, rounded)` + `WRITE_SHORT(0)` (the second is a flag word, 0 in the shipped binary; matches the
original `RestartRound` send @ the .so).

**Client element (mirrors the stock CHudBattery/Health pattern):**
- `CHudRoundTime : CHudBase` — `Init` hooks the `RoundTime` message (`HOOK_MESSAGE(RoundTime)`) + adds
  itself to the HUD list; `MsgFunc_RoundTime` reads the two shorts, turns `seconds` into an absolute
  client-time expiry (`m_flEndTime = gHUD.m_flTime + seconds`) so the clock keeps ticking locally between
  server updates (seconds<=0 clears it + drops HUD_ACTIVE); `Draw` renders `M:SS` (sprintf `%d:%02d`)
  centred just below the top of the screen via `DrawHudString` + `GetConsoleStringSize`, yellowish
  (RGB_YELLOWISH), flashing red (RGB_REDISH) in the last 10s; `Reset` clears it.
- Wired in: `cl_dll/hud.h` (`#include "roundtime.h"` + `CHudRoundTime m_RoundTime;`), `cl_dll/hud.cpp`
  (`m_RoundTime.Init()` + `.VidInit()`), `cl_dll/CMakeLists.txt` (`roundtime.cpp` in the source list).

**Verified (BUILD + LOAD-level, per the cont.46 caveat — this headless box can't RENDER the client):**
- `build.sh amd64 client` → 64-bit client.so (openvgui), glibc floor 2.14; `build.sh i386 client` →
  32-bit client.so (Valve vgui), glibc floor 2.3 — both clean. `nm | c++filt` shows the linked
  `CHudRoundTime::{Init,VidInit,Draw,Reset,MsgFunc_RoundTime}`; `strings client.so` shows the `RoundTime`
  hook string.
- `build.sh amd64` (server) → hl.so clean; `strings hl.so` shows `RoundTime` (not TSRoundTime).
- Server no-regression: booted `tm_avana` on the rig (our hl.so) → GAME SKILL LEVEL reached, 0 Can't init,
  no crash over 20s. (The message rename is server-string-only; doesn't touch the verified cont.45 timer
  logic.)

**VISUAL CONFIRMATION STILL OWED (coordinate with the user):** the actual on-screen M:SS clock needs a
graphical GoldSrc/xash client run by the user — build/load here can't show pixels. The likely visual
tweaks once seen: exact screen position/size, whether TS draws it as a clock sprite vs. text, and the
flash threshold. Treat position/colour as // VERIFY.

**Next Track B slices** (all read messages the server already sends): the powerup/motion-blur indicators
(`gmsgTSPowerUpTime` / `gmsgTSMotionBlur` — the .so registers `TSMBlur`, `TSSlowMo`, `TSState`, `TSHealth`,
`TSArmor`, `TSCash`, `TSFrags`, `TSBTime`, `TSPSys` — CHECK each inherited gmsg name against the .so
strings the same way RoundTime was fixed), then the bigger visible-slow-motion time-dilation (client
pmove scales frametime by the slow fields) and the particle system (CTSSharedEmitter/CTSEmitter/CTSParticle
via the `TSPSys` message).

---

## 2026-06-21 (cont. 46) — TRACK B START: the client (cl_dll) now builds, both 32-bit + 64-bit

**Done — the cl_dll build foundation.** The mod-ts client library (`cl_dll/` → `client.so`) now
builds in the buildchain for BOTH i386 and amd64. This is the prerequisite for ALL Track B (TS client
HUDs, visible slow-motion, particles, stunts) — none of which could start before the client built.

**Key finding (answers "how did halflife-updated link vgui for 64-bit?"):** it never did. `build.sh`
only ever built `--target hl` (the server); NO `client.so` exists anywhere in halflife-updated's build
tree. The bundled `vgui.so`/`libSDL2` were always 32-bit; the 32-bit-VGUI wall was simply never hit
because nobody built the client target in the 64-bit buildchain. Track B is the first time.

Three portability fixes (committed 8f3b248) to compile the stock client under the buildchain's gcc-8.3:
- `cl_dll/particleman/CMiniMem.h`: replaced `std::pmr::unsynchronized_pool_resource` (<memory_resource>,
  gcc-9+) with a minimal `operator new`-backed pool shim (same allocate/deallocate/release surface).
- `cl_dll/particleman/CFrustum.cpp`: dropped `<GL/gl.h>` (absent in buildchain) — it only needs the
  GL_MODELVIEW_MATRIX/GL_PROJECTION_MATRIX enum values for the engine TriAPI; defined locally.
- `cl_dll/entity.cpp`: `(int)pTemp` (FTENT_SPIRAL phase) is a 64-bit-pointer→int truncation → route
  through intptr_t.

Dual-arch VGUI (this commit), per the user's pointer to xash3d-fwgs:
- **32-bit** (i386) links Valve's bundled `vgui.so` (Steam HL); **64-bit** (amd64) links openvgui's
  ABI-compatible `libvgui.so` — vendored as `linux/release/vgui64.so` (from xash3d-fwgs, x86-64), plus
  a 64-bit `libSDL2` vendored as `linux/libSDL2-2.0-x64.so.0`. openvgui is a drop-in: the client
  compiles against the STOCK `../utils/vgui/include` headers and links clean against it (no source
  changes — just a relink). Both are only load-time linked (the engine supplies the real libs).
- `cl_dll/CMakeLists.txt`: selects the arch-appropriate vgui/SDL2 by `CMAKE_SIZEOF_VOID_P`; build-dir
  filenames stay `vgui.so`/`libSDL2-2.0.so.0` so DT_NEEDED is uniform (the runtime provides the
  arch-correct lib under that name). `scripts/build.sh` gained a target arg: `build.sh <arch> [hl|
  client|all]` (default hl, backward-compatible). `linux/.gitignore` allows the two new vendored libs.

**Verified (BUILD-ONLY):** `build.sh amd64 client` → 64-bit client.so (openvgui), glibc floor 2.14;
`build.sh i386 client` → 32-bit client.so (Valve vgui), glibc floor 2.3; `build.sh amd64` still builds
the server (backward compat); `build.sh amd64 all` builds both. DT_NEEDED = vgui.so + libSDL2-2.0.so.0
for both arches.

**VERIFICATION GAP (important for Track B):** this headless box can BUILD client.so but cannot RENDER
it — the dedicated-server rig has no graphics, and the browser-playable client is the separate WASM
xash build, not this native client.so. So TS client work is build-verified here; visual/runtime
behaviour needs a graphical GoldSrc/xash client run by the user. Next Track B slice: a first TS HUD
(e.g. the round-time countdown reading gmsgTSRoundTime) — build + load verifiable, visual by the user.

---

## 2026-06-21 (cont. 45) — Round timer (CTSGameRules::RoundTimeLeft + StartRoundTimer)

**Done — the team-round time limit.** Completes the round lifecycle: team rounds now end three ways —
elimination (cont.37), objective (cont.25/31), and now TIME-OUT. Part of Track A's round polish.

RE (recovered/server/CTSGameRules.c): RestartRound @0x77920 sets the round end time +8 =
clamp(`roundtime` cvar, 30, 180) + time, clears +0xc..0xf, and (if the round-time flag +0x10)
sends gmsgTSRoundTime (remaining seconds). RoundTimeLeft @0x79024: when +8 elapses, fire the objective
manager's `targetontimeleft` (+0x554 → +0x94) then `TeamWin(2)` = a DRAW. Functional reconstruction:
- `CTSGameRules::StartRoundTimer()`: m_flRoundEnd = time + clamp(TS_ROUNDTIME=120, 30, 180) + register
  & send gmsgTSRoundTime (remaining seconds) to all (lazy REG_USER_MSG "TSRoundTime").
- `CTSGameRules::RoundTimeLeft()`: `TSFireTimeLeftTarget()` (new free helper in ts_objectives.cpp —
  fires the objective manager's m_targetTimeLeft) + `TeamWin(2)` (draw → schedules the round restart).
- `Think`: in team mode with no win pending, arm the timer on the first frame (m_flRoundEnd==0 →
  StartRoundTimer) and run RoundTimeLeft when it expires. `TSRestartRound` zeroes m_flRoundEnd so the
  next round re-arms.

**Verified RUNTIME (rcbot, tm_avana, temp probe reverted — shortened the timer to 4s):** full cycle
armed(4s) → expired → RoundTimeLeft draw (score [0,0] UNCHANGED = correct, a draw doesn't score) →
restart scheduled (+3s) → respawn → timer re-armed → … cycling cleanly. No crash; clean regression
(tm_avana + ctb_deadheat 0 Can't init, ts_wyndham only ts_emitter/ts_particle). (Ran on -port 27018.)

**SLICE:** TS_ROUNDTIME is a fixed 120s (the .so reads a `roundtime` cvar — VERIFY/wire it); the
round-time-enabled flag is implied by team mode (no per-mode gate); gmsgTSRoundTime is sent but no
cl_dll reads it (no round-time HUD). This leaves UpdateTeamScores/InitTeamScore + the full faithful
RestartRound (game-mode member + per-player round setup) + ShowObjectiveOnHud as the last Track A
polish; the big frontier is Track B (cl_dll).

---

## 2026-06-21 (cont. 44) — CTSPlayerAmount + CTSSuperJump (small self-contained leftovers)

**Done — `dlls/ts_playeramount.cpp` (CTSPlayerAmount) + `CTSSuperJump` in `dlls/ts_powerup.cpp`.**
Two of the small non-FGD-table leftovers; both self-contained, synthetic-verified.

RE: `CTSPlayerAmount` (Spawn @0x72a78, ClientConnect @0x72aec / ClientDisconnect) is a point relay
(ts_playeramount) — a per-entity counter (+0x6c) that fires `connecttarget` (+0x74) when the count
rises to `clients` (+0x70) and `disconnecttarget` (+0xd8) when it falls back to it, driven off the
engine connect/disconnect events. `CTSSuperJump` (Spawn @0xc9f44) is `CTSSuperJump : CTSPowerUp` —
sets m_iType = 0x100 (TS_PWUP_LOWGRAV) then chains CTSPowerUp::Spawn, so it's a map powerup that
always gives the super-jump (low-gravity) effect. Reconstruction:
- `CTSPlayerAmount : CBaseEntity` — KeyValue clients/connecttarget/disconnecttarget; a 0.5s CountThink
  polls the live connected-player count and fires connecttarget on the rising edge to `clients` /
  disconnecttarget on the falling edge (a poll equivalent of the .so's connect/disconnect events, so
  no engine hook needed).
- `CTSSuperJump : CTSPowerUp` — Spawn forces m_iType = TS_PWUP_LOWGRAV then CTSPowerUp::Spawn.

**Verified RUNTIME (rcbot, ts_wyndham, SYNTHETIC — temp probe reverted):** a ts_playeramount(clients=2)
fired connecttarget the moment the live count rose to 2 (rising-edge); ts_superjump spawned as a
powerup without crashing. Clean regression (tm_avana 0 Can't init; ts_theatre's 328 powerups still 0
Can't init — CTSSuperJump didn't disturb the powerup path). (Ran on -port 27018; 27015 held by an
unrelated app.)

**SLICE:** CTSPlayerAmount uses a poll instead of the engine connect/disconnect hooks + ignores its
spawnflags; CTSSuperJump inherits CTSPowerUp's Save/Restore as-is. Remaining leftovers: CInfoBreakable
(a respawning breakable — thin, deferred), CBulletManager (the traveling-bullet/bullet-time system —
deferred: it rewrites the verified instant-hitscan fire path + isn't visible without a cl_dll).

---

## 2026-06-21 (cont. 43) — CTSScoreGiver + CVipEscape (the last team-objective leftovers)

**Done — `CTSScoreGiver` + `CVipEscape` in `dlls/ts_objectives.cpp`.** The two small remaining
team-objective entities; closes out the Track A entity list. Both FGD-listed, 0 stock placements →
synthetic-verified.

RE: `CTSScoreGiver` (Spawn @0x8c0b8, Use @0x8c158) is `@PointClass base(WinGiver)` — identical to
CTSWinGiver (cont.25) but its Use RESETS the call counter after firing (param+0x19 = 0) so it scores
REPEATEDLY (vs WinGiver latching done), and a "Respawn Players" spawnflag. `CVipEscape` (Spawn
@0x8d23c) is classname **`func_vip_safetyzone`** (not ts_vipescape) — a passive brush marker (KeyValue
stub, empty Precache, non-solid SET_MODEL) the unbuilt VIP-escort logic checks against; it has no
touch/use of its own. Reconstruction:
- `CTSScoreGiver : CTSWinGiver` — Use counts to callsnumber, then bumps `CTSGameRules::m_teamScore`
  + HUD message + re-arms (m_calls=0). Scores DIRECTLY (not via TeamWin) so it's genuinely repeatable
  and doesn't restart the round (a score giver gives points without ending the round, unlike the win
  giver). // SLICE: the .so routes through TeamWin w/ cash; "Respawn Players" spawnflag not applied.
- `CVipEscape : CBaseEntity` (func_vip_safetyzone) — a non-solid brush marker. // SLICE: the VIP win
  detection is in the unbuilt team subsystem.

**Verified RUNTIME (rcbot, tm_avana, SYNTHETIC — temp probe reverted):** ts_scoregiver callsnumber=2:
team0 score 0 → 0 (1 call) → 1 (2 calls = scored) → 2 (4 calls = re-armed + scored again, REPEATABLE,
no round-restart block); func_vip_safetyzone created without crash. Clean regression (tm_avana +
ctb_deadheat + ts_dust2 0 Can't init). (Ran on -port 27018; 27015 held by an unrelated app.)

**TRACK A ENTITY LIST COMPLETE.** All FGD team/CTF/objective entities are now reconstructed: teams
(cont.35) + cash/buy (36) + elimination (37) + CTSFlag (38) + CTSBriefcase (39) + CTSBomb (40) +
CTSSellZone (41) + CTSTeamEscape (42) + CTSScoreGiver/CVipEscape (43). Remaining Track A polish is
faithful niceties (UpdateTeamScores/InitTeamScore, the round timer, ShowObjectiveOnHud, CheckDeads
grace/CTB/last-man, capture-via-TeamWin-restarts-round). The big remaining frontier is **Track B —
the cl_dll** (visible slow-motion, the particle system CTSSharedEmitter/CTSEmitter/CTSParticle,
CTSStunt, all HUDs) — NONE built yet. Also the small non-FGD leftovers: CTSPlayerAmount,
CTSSuperJump, CInfoBreakable, CBulletManager.

---

## 2026-06-21 (cont. 42) — CTSTeamEscape (team escort/escape zone)

**Done — `dlls/ts_escape.cpp` (ts_teamescape).** A team escort objective: when enough of the
attacker team is simultaneously inside the zone, it completes (fire target + HUD message). FGD-backed
(`@SolidClass`, brush, base tsobjective); 0 stock placements → synthetic-verified.

RE (recovered/escape/CTSTeamEscape.c): Spawn @0x8d390 (brush SET_MODEL + SetSize), EnableObjective
@0x8d4a0 (enable + clear the 33-slot presence array + count), WaitTouch @0x8d4ec (attacker-team
player → track by entindex, count distinct; threshold from escapepercentage @+0xf4 over the attacker
TEAM SIZE: 0→1, 1→size/3, 2→2*size/3, 3→size; complete → FireTargets+HUD+EnableObjective(false)),
WaitThink @0x8d6b0 (clear the presence array + count every 1s — so it counts SIMULTANEOUS presence).
Functional reconstruction onto the stock SDK (inlined attacker-team gate like ts_objectives.cpp; team
size = live attacker-team players):
- `CTSTeamEscape : CBaseEntity` (brush SOLID_TRIGGER). m_present[33] (EHANDLE by entindex) + m_count.
- WaitTouch accumulates distinct attacker-team touchers; threshold per escapepercentage switch;
  completion fires target + message + disables. WaitThink resets the window each second.

**Verified RUNTIME (rcbot, tm_avana, SYNTHETIC — temp probe reverted):** team size 2 — escapepercentage
0 (one member): one touch → completed (enabled=0); escapepercentage 3 (all): one touch stays active
(enabled=1), all touches → completed (enabled=0). No crash; clean regression (tm_avana + ts_dust2 0
Can't init). (Ran on -port 27018; 27015 still held by an unrelated app.)

**SLICE:** enable is faithfully gamerules-driven (here Spawn); no Save/Restore (the .so has
m_SaveData); completion fires the target only (no team score / round-win — that's the objective
manager's job); team size = live players (the .so reads the team struct via TSCountPlayers).

---

## 2026-06-21 (cont. 41) — CTSSellZone (sell dropped weapons for cash) + a powerup-precache fix

**Done — `dlls/ts_sellzone.cpp` (ts_sellzone).** The one entity that makes the cont.36 cash actually
flow: a brush zone that sells dropped weapons. FGD-backed (`@SolidClass`, base tsobjective; ts_dust2
places one).

RE (recovered/sellzone/CTSSellZone.c): KeyValue @0x726c0 (attackerteam), Spawn @0x72720 (brush
SET_MODEL + SetSize + SetTouch), SellTouch @0x727e8 — a ~61-byte NEAR-STUB that only does
`strcmp(toucher->classname, "WorldGun")`. Consistent with cont.36 (CashBased OFF → the sell economy
is vestigial in the shipped binary). Functional reconstruction:
- `CTSSellZone : CBaseEntity` (brush SOLID_TRIGGER). SellTouch: a touching `ts_worldgun` (a dropped
  weapon, cont.27) → credit the nearest living player (on the zone's team, if set) a flat
  SELLZONE_VALUE ($500) via `CBasePlayer::GiveCash` + a center-print, then UTIL_Remove the gun.

**Also fixed a pre-existing latent crash (surfaced by ts_dust2):** the dynamic DM powerup spawner
(cont.26 PutPowerUp → CTSPowerUp::Spawn) PRECACHE_MODELs `models/powerup.mdl` at RUNTIME — fine on
maps with ts_powerup placements (precached at load), but Host_Errors on a DM map with none (ts_dust2:
no ts_powerup, no objective_manager → DM → spawner runs → crash). Added `models/powerup.mdl` to
PrecacheAllTSWeapons (map load), same fix pattern as cont.38/40.

**Verified RUNTIME (rcbot, ts_dust2 — a REAL map with a sell zone, temp probe reverted):** the zone
spawns (0 Can't init, was 1 + the powerup crash gone); a dropped beretta worldgun dropped at a player
→ SellTouch credited the player cash 0→500 + marked the gun removed (FL_KILLME). No crash; clean
regression (ts_dust2 + tm_avana + ctb_deadheat all 0 Can't init).

**SLICE:** flat sale value (the per-weapon price s_weapon_info+0xd8 isn't extracted + is dead while
CashBased off); the dropped gun records no seller (no pev->owner) so the nearest player is credited;
no team-score / round effect.

---

## 2026-06-21 (cont. 40) — CTSBomb: the defuse / wire-cut objective

**Done — `dlls/ts_bomb.cpp` (ts_bomb).** A self-contained team objective: a planted bomb that counts
down and explodes unless defused. FGD-backed (`@SolidClass`, base tsobjective). 0 stock placements →
synthetic-verified (like ts_dmhill cont.32 / ts_trigger cont.34).

RE (recovered/bomb/CTSBomb.c): Spawn @0x8c470, Precache @0x8c570 (misc/beep2.wav), SetRandomWires
@0x8c598 (3 wires: one DEFUSE(1), one BOOBYTRAP(2), rest NEUTRAL(3)), EnableObjective @0x8c608 (arm/
safe), CutWire @0x8c70c (1→defuse+fire target/msg, 2→fast countdown, 3→timer to 0 = detonate),
WaitThink @0x8c860 (−1/tick, 1s or 0.1s when fast, beep rising pitch, detonate at 0 = TSExplosion),
Use @0x8c9a4 (hold USE `defusetime`s → defuse, FCAP_CONTINUOUS_USE=0x10), KeyValue defusetime/cutwires.
Functional reconstruction onto the stock SDK:
- `CTSBomb : CBaseEntity` (brush, inlined attackerteam gate like ts_objectives.cpp). Arms on Spawn
  (SLICE: faithful enable is gamerules InitObjectives). EnableObjective(true)=arm+SetRandomWires+
  WaitThink; (false)=safe.
- WaitThink: countdown −1/tick (1s, or 0.1s after a boobytrap), beep, `Explode` at 0 via
  `ExplosionCreate` (the SDK env_explosion blast — assets already precached by the weapon/grenade path).
- CutWire(0..2): defuse → FireTargets(target)+HUD msg+safe; boobytrap → m_bFast; neutral → timer≈0
  (next WaitThink detonates). `tscutwire <n>` console command (cl_dll wire UI is a SLICE).
- Use: continuous-USE accumulator over defusetime → defuse (FireTargets+msg).
- misc/beep2.wav added to PrecacheAllTSWeapons (map load) — a runtime-armed bomb's PRECACHE_SOUND
  Host_Errors (gdb/log-caught, same class as cont.38); dropped the Spawn-time Precache() call.

**Verified RUNTIME (rcbot, tm_avana, SYNTHETIC — temp probe reverted):** armed bomb wires=[2 1 3]
(one each); cut DEFUSE wire → enabled=0 (defused); cut NEUTRAL wire → timer 0.01 → WaitThink Explode
(ExplosionCreate runs at runtime, enabled=0, no crash); cut BOOBYTRAP wire → fast=1 still armed. No
crash; clean regression (tm_avana + ctb_deadheat 0 Can't init, ts_wyndham only ts_emitter/ts_particle).
(Test ran on -port 27018; 27015 was held by an unrelated app.)

**SLICE:** bomb countdown is a fixed 45s (the .so reads a bomb-time global DAT_001781a8 — VERIFY);
faithful arm is gamerules-driven (here Spawn); wire-cut needs the cl_dll UI (console-driven); no
Save/Restore (the .so has CTSBomb m_SaveData); detonation doesn't end the round / score (just blasts).

---

## 2026-06-21 (cont. 39) — CTSBriefcase: the CTF base/capture zone (seeds CTSFlag on a real map)

**Done — `dlls/ts_briefcase.cpp` (ts_briefcase).** The map-placed CTF objective that pairs with the
cont.38 CTSFlag: each team's briefcase seeds its flag at base and is the capture zone. Replaces
cont.38's `tsspawnflag` console stand-in — **first CTF entity verified on a real stock map**
(ctb_deadheat). FGD-backed `@SolidClass` (brush), ownerteam 1..4.

RE (recovered/briefcase/CTSBriefcase.c): ctor @0x8d788 (per-briefcase flag id s_nextFlagID 1..15,
team @+0x68 = -1, s_flag_status @+0x90), KeyValue @0x8d7f4 (contents/message/ownerteam), Spawn
@0x8d88c (brush SET_MODEL + SetSize), FlagFall @0x8da00 (CreateFlag(0, &s_flag_status) at the brush
centre + gmsgTSHaveFlag HUD + WaitThink), CanInteract @0x8db48 (team @+0x68 == player team),
CaptureTouch @0x8db9c (a living HaveFlag player who CanInteract → capture the carried flag).
Functional reconstruction onto the cont.38 CTSFlag:
- `CTSBriefcase : CBaseEntity` (brush SOLID_TRIGGER). KeyValue ownerteam is **1-based** in the FGD →
  stored 0-based (`m_iOwnerTeam = atoi-1`) to match pev->team / the objective gating; + contents +
  message (capture announce).
- Spawn: brush model + SOLID_TRIGGER; SetTouch(CaptureTouch); SetThink(FlagFall) @+0.5s.
- FlagFall: `TSCreateFlag(m_iOwnerTeam, Center())` — seeds this team's carriable flag at base (delayed
  so the runtime SET_MODEL follows the precache pass), then idle WaitThink.
- CaptureTouch: a living player carrying a flag (`TSPlayerHasFlag`) at THEIR own briefcase
  (`CanInteract`) → `TSReturnCarriedFlag` (return the carried flag) + `TeamWin(team)` + announce
  pev->message. (New shared helpers `TSPlayerHasFlag`/`TSReturnCarriedFlag` added to ts_flag.cpp.)

**Verified RUNTIME (rcbot, ctb_deadheat — a REAL stock CTB map, temp probe reverted):** 2 briefcases
spawn (0 Can't init, was 2), each FlagFall-seeds its flag at base with the correct 0-based team —
team 0 (Specialists) @(128 2048 192) + team 1 (Mercenaries) @(-128 -2048 192) (FGD ownerteam 1/2 →
0/1). Capture reuses the cont.38-verified TSReturnCarriedFlag + TeamWin. No crash; clean regression
(ctb_deadheat + tm_avana 0 Can't init, ts_wyndham only the known ts_emitter/ts_particle).

**SLICE:** the per-flag network id + gmsgTSHaveFlag HUD is server-only (no cl_dll flag indicator);
capture scores via TeamWin (which restarts the round — faithfully a capture awards cash + a HUD
without ending the round); no s_flag_status / contents drop; Save/Restore not wired (FCAP_DONT_SAVE).

---

## 2026-06-21 (cont. 38) — CTSFlag: the CTF flag (carry / capture / drop), unblocked by teams

**Done — `dlls/ts_flag.cpp` (ts_flag).** The first CTF entity, now possible because team
assignment (cont.35) gives a real pev->team. The classic capture-the-flag loop, gated on team.

RE (recovered/flag/CTSFlag.c): `CTSFlag : CTSPhysicObject`, model `briefcase.mdl`, runtime-created
(CreateFlag @0x9d874 — NOT map-placed: no FGD entity, 0 BSP placements; the CTSBriefcase map
objective seeds the bases). m_iFlagTeam @+0x9c, carrier EHANDLE @+0xa4, s_flag_status (7 ints @+0x90)
mirrored onto the carrier (player+0x7d0) via CBasePlayer::TakeFlag @0x93830 / DropFlag @0x9334c +
GetSpeedBySlots (carrying slows you). CanInteract @0x9e2d4 = flag.team==-1 || ==player.team.
PlayerTakeFlag @0x9e330 (Use/WaitTouch): living player; own-team flag → capture/return branch, else
TakeFlag (grab). FUNCTIONAL reconstruction onto the stock SDK (no CTSPhysicObject, no speed-by-slots):
- `CTSFlag : CBaseEntity` with a team + a 3-state machine (AT_BASE / CARRIED / DROPPED).
- FlagTouch: an enemy/neutral flag + a non-carrying living player → AttachTo (carry: SOLID_NOT,
  CarryThink rides the carrier's back, follows + drops if the carrier dies/leaves). Own-team flag at
  base + you're carrying the enemy flag → CAPTURE (return the carried flag + score via TeamWin).
- DropAt (the PlayerKilled `TSDropFlag` hook + CarryThink-on-death): flag falls where the carrier
  died, SOLID_TRIGGER, ReturnThink auto-returns it to base after 30s. ReturnToBase resets it home.
- `CTSFlag::Create(team, base)` + free `TSCreateFlag`/`TSDropFlag`; a `tsspawnflag <team>` console
  command seeds bases (stand-in for the CTSBriefcase map objective). briefcase.mdl + the pickup/drop
  sounds added to PrecacheAllTSWeapons (map-load) — a runtime PRECACHE_MODEL Host_Errors (gdb-caught:
  PF_precache_model_I → Host_Error → ShutdownServer; same class of bug as cont.12).

**Verified RUNTIME (rcbot, tm_avana, SYNTHETIC — temp probe reverted):** seeded a spec(0) + merc(1)
flag at a merc bot; merc touches the spec flag → carried=1 state=CARRIED; merc (carrying) touches its
own merc flag at base → CAPTURE: spec flag returned to base (state=AT_BASE), team-1 score 0→1; re-grab
→ TSDropFlag → state=DROPPED, carrier cleared. No crash; clean regression (tm_avana 0 Can't init,
ts_wyndham only the known ts_emitter/ts_particle).

**SLICE:** carrying doesn't slow the carrier (no GetSpeedBySlots / s_flag_status); no CTSPhysicObject
bounce physics; capture scores via TeamWin's team score (which also schedules a round restart — the
faithful capture awards cash + a HUD pointer without ending the round); flags are console-seeded (the
map-placed CTSBriefcase that creates them isn't built); no flag HUD/carrier icon (cl_dll).

---

## 2026-06-21 (cont. 37) — CheckDeads: team-round elimination win condition

**Done — `CTSGameRules::CheckDeads`/`CallCheckDeads` (ts_gamerules.{h,cpp}).** The automatic team-
round win condition: a round ends + the surviving team wins when the other team is wiped out. Builds
on cont.31's TeamWin/round-restart + cont.35's team assignment — team mode now RESOLVES rounds on its
own (independent of the objective entities, which remain an alternate win path; both call TeamWin).

RE (recovered/server/CTSGameRules.c): CheckDeads @0x79c00 (1335 bytes), CallCheckDeads @0x79bc0.
CallCheckDeads: if no restart pending (+0xc==0) schedule CheckDeads at time+1 (+0x5880). CheckDeads
(team branch, game-mode 2/3): count alive players per team into local_3c[4] via GetTeamIndex(TeamID());
mode-3 (CTB) fires objective targets; else winner = the team with survivors — local_3c[1]<1 → team 0
wins, local_3c[0]>0 → return (contested), else team 1 wins; nobody alive → team 2 (draw); then
TeamWin(winner,0,1). Functional reconstruction:
- `CallCheckDeads()`: if `m_flRoundRestart==0 && m_flCheckDeads==0` → `m_flCheckDeads = time+1`.
- `CheckDeads()`: team mode + no restart pending; count alive/present per team (general over
  m_numTeams); require ≥2 players over ≥2 teams (so warmup/one-sided doesn't insta-win); `teamsAlive>=2`
  → return (contested); `==1` → TeamWin(lastAliveTeam); `0` → TeamWin(2)=draw.
- `Think`: fire CheckDeads when `m_flCheckDeads` elapses (before the restart check).
- `PlayerKilled`: `CallCheckDeads()` after the death loot.
- `TeamWin` refined: guard against a double-win in a round (`m_flRoundRestart!=0` → return); team 0/1
  bump m_teamScore + announce `GetIndexedTeamName`, anything else = a draw announce (no score).

**Verified RUNTIME (rcbot, tm_avana, temp probes reverted):** balanced 1v1 → force-killed the
Mercenary → 1s later CheckDeads (present[1,1] alive[1,0] teamsAlive=1) → TeamWin(0): Specialists
score [1,0], restart scheduled +3s → round restarted (respawn all + objectives re-armed) → next round
2v2 = contested (no premature win). No crash; clean regression (tm_avana 0 Can't init, ts_wyndham only
the known ts_emitter/ts_particle).

**SLICE:** no just-respawned grace window (the .so counts +0x770>=0 as still-alive; we use IsAlive());
mode-3 (CTB) objective-target firing not reconstructed (treated as plain elimination); no round timer
(gmsgTSRoundTime) or last-man-standing DM mode (g_lastManStanding branch); TeamWin still lacks the
faithful UpdateTeamScores/InitTeamScore + the full RestartRound@0x67920 (round timer + game-mode member).

---

## 2026-06-21 (cont. 36) — Cash + buy/armament system (free loadout; CashBased is OFF)

**Done — `dlls/ts_buy.cpp`: the TS cash + buy/armament loadout (Track A step 2).** Reuses the
built `GiveTSWeapon`/`TSAddAmmo` arsenal.

**KEY RE FINDING:** `CashBased__Fv` @0x66abc is hardcoded `return 0` in the shipped .so — the cash
economy is COMPILED OUT. Every `if (CashBased())` cost-deduction branch in BuyWeapon/BuyAmmo/AddItem
is dead, so the buy system gives weapons/ammo **FREE**. The real mechanic is a free ARMAMENT
(loadout) selector: the client menu queues weapons+ammo and `BuyArmament` applies them. Cash is
still tracked (the `cashforwin`/`cashforlose` cvars + a future sell zone) but never charged.

RE (recovered/buy/CBasePlayer.c): cash @+0x540 (clamped 0..30000, GiveCash @0x9310c); a pending
armament = ammo array @+0x554 (16 ints, rounds) + a weapon list @+0x594 (stride 0xc ×32: type/count/
gift). ResetArmament @0x93e88 (zeroes 0x70 ints), AddItemToArmament @0x93ea4 (first free slot),
AddAmmoToArmament @0x93ef4, BuyArmament @0x93f10 (each weapon → BuyWeapon or free AddItem; each
caliber → BuyAmmo), BuyAmmo @0x941e8, BuyWeapon @0x945bc (IsRestrictedWeapon @0x770c8 gate + the M61
special: type 0x18=24 is team-mode-only `1<TSGameRules` and capped 2/life via player+0x714, else free
PickUpWeapon). Functional reconstruction onto CBasePlayer + the cont.18 arsenal:
- `CashBased()` → false; `CBasePlayer::GiveCash` (clamped); members `m_iCash`/`m_iGrenadeBuys`/
  `m_armamentAmmo[16]`/`m_armament[32]` in player.h.
- `ResetArmament`/`AddItemToArmament`/`AddAmmoToArmament`/`BuyWeapon`/`BuyAmmo`/`BuyArmament`.
  `BuyWeapon` = M61 team-mode+cap-2 rule then free `GiveTSWeapon`; `BuyAmmo` = free `TSAddAmmo`.
- Console drivers in client.cpp (the faithful menu is cl_dll/`menuselect`, a SLICE): `tsbuy <type>`,
  `tsbuyammo <cal> <rounds>`, `tsarm <type> [count]`, `tsarmammo <cal> <rounds>`, `tsbuyarmament`,
  `tsresetarmament`.

**Verified RUNTIME (rcbot, temp probe reverted):** tm_avana — queue {beretta(2),uzi(3)} + 50×9mm,
GiveCash(1000)→1000, BuyArmament → arsenal=2 [2,3], 9mm reserve filled to the cap 80; M61 ×3 → 1,1,0
(grenadeBuys cap 2). ts_wyndham (DM) — armament still applies free (arsenal=2), M61 ×3 → 0,0,0
(team-mode block). No crash; clean regression (only the known ts_emitter/ts_particle Can't-inits).

**SLICE/deferred:** weapons are free (faithful — CashBased off); the category-by-bitmask
`IsRestrictedWeapon` over CTSMapGlobals' restriction isn't wired (default 0x1f allows all → allow-all
stub is faithful for stock maps); no cl_dll buy menu (console-driven); armament ammo treated as rounds
(no per-caliber mag-size table); no starting cash / cash-for-win-lose award yet; price field
(s_weapon_info+0xd8) not extracted (dead while CashBased off).

---

## 2026-06-21 (cont. 35) — CTSGameRules team subsystem: team ASSIGNMENT (the keystone)

**Done — `dlls/ts_teams.cpp`: a real team layer on CTSGameRules + auto-balance assignment.**
First piece of Track A. Players now get a `pev->team`, which is what the already-built objective/CTF
gating reads (cont.25/34) — so team mode is no longer just inheriting MP.

RE (from the .so): the team subsystem is a packed 0x8c-byte per-team struct ×4 (cash @+0x1c, scores
@+0x20/+0x24, name @+0x28, spawn classnames @+0x68/+0x88, model list @+0x38), gated on the TS
game-mode byte @+0 (2/3 = a team map). InitTeams @0x77f60 builds "Specialists;Mercenaries"
(models "spec"/"merc") for a team map; GetTeamIndex @0x79b04 = case-insensitive name→index (0-based);
GetTeamsNum @0x785a8; GetPlayerModel @0x79a08 / GetTeamCash @0x79aac read the per-team struct. The
faithful trigger for a player landing on a team is the client class-selection menu (cl_dll, NOT built).

Functional reconstruction (modeled naturally, NOT transcribing the byte struct; mirrors the SDK
`CHalfLifeTeamplay` team half, which is the same logic):
- `CTSGameRules` keeps `m_szTeamList` + parallel `m_teamNames[]/m_teamModels[]` + `m_numTeams`.
- `IsTeamMode()` (cached tri-state `m_iTeamMode`): a `ts_objective_manager` is present (the same DM
  proxy cont.26 used). Resolved lazily — worldspawn installs the rules before objective ents spawn.
  `IsTeamplay()` override now == `IsTeamMode()`; folds the old explicit objective-manager scan out of
  the cont.26 Think powerup gate.
- `InitTeams`/`RecountTeams`/`GetTeamIndex`(override)/`GetIndexedTeamName`(override)/`GetTeamID`
  (override → TeamID())/`TeamWithFewestPlayers` — mirror the SDK team subsystem.
- `AssignToTeam(player, name)`: set `pev->team` **0-based** (0 spec, 1 merc — the FGD/objective
  convention, NOT engine-1-based) + `m_szTeamName` + "model"/"team" infokeys + broadcast gmsgTeamInfo
  + gmsgScoreInfo. `PlayerSpawn` override: MP spawn, then (team mode, if not yet on a valid team)
  auto-balance onto `TeamWithFewestPlayers()` — idempotent across respawns/round restarts.
  `InitHUD` override sends gmsgTeamNames + everyone's team to a connecting client.

**Verified RUNTIME (rcbot, tm_avana, temp probe reverted):** 5 bots auto-balanced fewest-first →
Specialists{Chuck,Sean,sebastian}=3, Mercenaries{Alec,Matt}=2 (exact alternating 0,1,0,1,0 with
ties→lower index). 0 Can't init, no crash. DM regression (ts_wyndham): **0** assignments (IsTeamplay
false), powerup spawner still gated correctly, only the known ts_emitter/ts_particle Can't-inits, no
crash. Clean rebuild boot of tm_avana: probe gone, 0 Can't init, clean shutdown.

**SLICE/next (rest of Track A):** assignment is server auto-balance (faithful = client class menu,
cl_dll); the per-team **cash economy** (GetTeamCash/initial_cash @+0x1c, CashBased(), price
s_weapon_info+0xd8) + the **buy menu** server half (AddItemToArmament@0x83ea4 / BuyArmament@0x83f10 /
BuyWeapon@0x845bc) are not built; team **scores** still go through cont.31's `m_teamScore[]` (not the
faithful UpdateTeamScores/InitTeamScore); full `RestartRound`@0x67920 (round timer + game-mode member
+ InitObjectives/SetObjective) deferred; no friendly-fire (FPlayerCanTakeDamage) yet. Now that
pev->team is real, the **CTF/escape entities** (CTSFlag/CTSBriefcase/CTSBomb/CTSTeamEscape/CVipEscape/
CTSSellZone/CTSScoreGiver) are unblocked.

---

## 2026-06-21 (cont. 34) — CTSTrigger (TS brush trigger; team-gated, recheck delay)

**Done — `CTSTrigger` in `dlls/ts_objectives.cpp` (ts_trigger).** A reusable TS brush trigger.
RE (from the .so): Spawn @0x709b8 (brush SET_MODEL), WaitTouch @0x70ad8 — cooldown gate (param+0x41),
living-player check, attacker-team gate (GetTeamIndex vs +0x42), "Flag Carrier Only" spawnflag (0x1a0
& 8 → HaveFlag), FireTargets(target) + a sprintf'd HUD message, re-arm at time + max(0.5, nextcheck).
Functional reconstruction as `CTSTrigger : CTSObjective` (reuses cont.25's attackerteam + CanInteract):
KeyValue nextcheck (+ attackerteam); WaitTouch: cooldown → CanInteract(pev->team) → FireTargets +
TSHudMessageAll → arm next-allowed.

**Verified RUNTIME (rcbot, ts_wyndham, SYNTHETIC — 0 stock placements):** a touch fired + armed the
cooldown (nextAllowed → t+2); an immediate re-touch was blocked. Factory `ts_trigger` exported; no
crash; clean regression (tm_avana, 0 Can't init).

**SLICE:** the "Flag Carrier Only" spawnflag needs the CTF flag system (not built) → ignored; the team
gate uses pev->team (now real as of cont.35).

**Session status:** ~25 TS classes reconstructed (the self-contained server-side gameplay is
essentially complete). Remaining = the big/client-entangled subsystems: faithful team mode + cash
(team assignment, SetTeamList, scores, the full RestartRound), the buy menu, the CTF entities
(CTSFlag/CTSBriefcase/CTSBomb/CTSTeamEscape/CVipEscape — need teams), CTSStunt (movement, client),
and the cl_dll side (visible time-dilation, particles CTSSharedEmitter/CTSEmitter/CTSParticle, HUDs).

---

## 2026-06-21 (cont. 33) — CTSSlowMotionPoint (the slow-motion bubble; powerup→GoSlow link)

**Done — `dlls/ts_slowmotionpoint.cpp`.** Un-SLICEs the deferred SlowMotionFlow link from
cont.14/15: an activated slow-motion powerup now actually drops nearby players into slow-motion.

RE (from the .so): CTSSlowMotionPoint (CreateSlowMotionPoint @0x69c60, ActivateSlow @0x663d0 sets
+0xe8 endtime/+0xec scale/+0xf4 bullets, WaitThink @0x71728). WaitThink polls every 0.5s: for each
living player, if `!InSlow()` and a TraceLine from the point to the player's eye is clear (LOS) →
`GoSlow(scale, bullets)` + track; at the end time → DeactivateSlow (restore tracked) + UTIL_Remove.
A runtime entity (0 FGD placements), created by the powerup/SlowMotionFlow.
Functional reconstruction on cont.15's CBasePlayer::GoSlow/InSlow:
- `CTSSlowMotionPoint : CBaseEntity`: ActivateSlow + WaitThink (LOS GoSlow) + DeactivateSlow (restore
  m_hSlowed[]). `TSCreateSlowMotionPoint(origin, scale, duration, bullets, owner)`.
- `CBasePlayer::UsePowerUp`: a slow-motion(0.6)/slow-pause(0.8) powerup spawns a bubble at the player
  (owner = activator, the powerup duration).

**Verified RUNTIME (rcbot, ts_wyndham, SYNTHETIC — 0 stock placements):** a bubble at bot1 (owner)
with bot2 adjacent → bot2 `InSlow()=1 go=0.60`, owner bot1 `InSlow()=0` (skipped); at expiry the
bubble removed itself + restored bot2 (InSlow=0). No crash; clean regression (ts_theatre, powerups).

**SLICE:** the visible time-dilation MOVEMENT is still client-side (GoSlow sets server fields only);
the owner is skipped (stays "fast" = bullet-time feel) vs the .so's exact LOS set; no projectile slow.

---

## 2026-06-21 (cont. 32) — CTSDMHill (king-of-the-hill brush volume)

**Done — `dlls/ts_dmhill.cpp` (ts_dmhill).** A self-contained DM game-mode entity (no team/client
dependency). The `tm_`/team subsystem is the big remaining work, so this is a clean faithful
self-contained increment.

RE (from the .so): CTSDMHill (Spawn @0x71a8c brush SET_MODEL + SetSize, KeyValue @0x719b8, WaitTouch
@0x71be8 1433 bytes — tracks a single holder, awards `fragamt` (DM) / `fragamtteam` (team) frags to
pev->frags + gmsgScoreInfo + a "<netname> <message>" broadcast once held `nextcheck` seconds).
Functional reconstruction: a 0.5s `HillThink` polls the volume (`Intersects`) — exactly one living
player inside earns the timed award; 0 or 2+ (empty/contested) resets the king. KeyValue
nextcheck/fragamt/fragamtteam (+ message via the Target base).

**Verified RUNTIME (rcbot, ts_wyndham, SYNTHETIC — ts_dmhill has 0 stock placements):** a hill
instantiated around a sole bot awarded fragamt on the timer (frags 0→5). Factory `ts_dmhill`
exported; no crash; clean regression. (Build + synthetic only, like ts_timer/ts_customtarget — no
stock map exercises it; a hand-made test map would confirm in-map.)

**SLICE:** the .so's same-team-holders-don't-reset nuance simplified to "sole occupant".

---

## 2026-06-21 (cont. 31) — CTSGameRules::TeamWin + round restart (objective win ends the round)

**Done — the objective→win→round-restart chain.** Completes cont.25, where `CTSWinGiver::Use`
was a SLICE that only announced. The win now goes through the game rules (first piece of the
team subsystem).
- `CTSGameRules::TeamWin(team)`: bump `m_teamScore[team]`, announce the winner (TSHudMessageAll),
  schedule a restart 3s out (`m_flRoundRestart`; the .so TeamWin @0x68e00 uses param_1+0xc = time+3).
- `CTSGameRules::Think`: when `m_flRoundRestart` elapses, `TSRestartRound` respawns every player
  (SDK `respawn()`) + re-arms the objectives (re-Spawn each ts_wingiver/ts_hack) + announces.
- `CTSWinGiver::Use` → `TSGameRules()->TeamWin(m_winnerTeam)` (the inline announce removed).

**Verified RUNTIME (tm_avana, temp probe reverted):** firing the wingiver (winnerteam 2 = draw →
teams 0/1 unchanged, correct) scheduled a restart (pending=12.0); 3s later it fired → players
respawned + the wingiver re-armed (done=0). No crash; clean regression (ts_wyndham + tm_avana).

**SLICE:** faithful TeamWin awards victory/defeat cash + drives the round-time HUD; the full round
timer + the TS game-mode member + InitObjectives/SetObjective remain the team-subsystem TODO
(RestartRound @0x67920 ~1100 bytes, TeamWin @0x68e00 ~1564). No team ASSIGNMENT yet (scores track
the winnerteam the objective names, not per-player teams).

---

## 2026-06-21 (cont. 30) — CTSAmmoPack (drop ammo on death; consumable per-caliber pack)

**Done — `dlls/ts_ammopack.cpp`.** Completes the death-loot story: weapons (cont.27) + corpse
(cont.28) + now ammo. The dead player's per-caliber reserve drops as one consumable pack.

RE (from the .so): `CTSAmmoPack` (CreateAmmoPack @0xa1334 copies a 16-int per-caliber array;
Spawn @0xa13f4 model `ammopack.mdl` + `movetype=6` TOSS + `solid=2`; WaitTouch @0xa16c0 loops the
16 calibers calling `CTSGun::AddAmmo` and ZEROES each given entry — CONSUMABLE, vs the persistent
CTSGroundAmmo station cont.17). FUNCTIONAL reconstruction on the stock SDK (no CTSPhysicObject):
- `CTSAmmoPack : CBaseEntity` (ts_ammopack) carrying `m_ammo[16]`: MOVETYPE_TOSS → FallThink settle
  → SOLID_TRIGGER + WaitTouch. WaitTouch: for a TS-gun holder, `TSAddAmmo` each caliber, zero what
  was taken, UTIL_Remove when emptied. `TSCreateAmmoPack(owner, counts[16], origin, vel)`.
- `TSDropAmmoOnDeath(player)` (ts_gun.cpp): drop the gun's `m_ammoReserve[16]` as one pack.
- `CTSGameRules::PlayerKilled` now also calls it. `ammopack.mdl` precached at map load.

**Verified RUNTIME (rcbot, ts_wyndham, temp probe reverted):** killing a beretta-holding bot
dropped a pack carrying 9mm=45; a live tsgun holder touching it received the 9mm (pack m_ammo[1]
45→0) and the emptied pack was UTIL_Remove'd (deferred to frame end). No crash; clean regression
(factory `ts_ammopack` exported).

**SLICE:** no CTSPhysicObject bounce; one pack for the whole reserve (not per-mag); no thrown-pack.

---

## 2026-06-21 (cont. 29) — CTSShrapnel (grenade-blast fragments)

**Done — `CTSShrapnel` in `dlls/ts_grenade.cpp`.** The grenade explosion (cont.20 CM61Grenade)
now throws damaging fragments, matching the .so (a grenade Detonate spawns CTSShrapnel).

RE (from the .so): `CTSShrapnel : CTSPhysicObject` (Create @0x7bf9c, Spawn @0x7c0a0 with
`models/shrapnel.mdl` + `movetype=10` MOVETYPE_BOUNCE + `solid=2`, BounceTouch @0x7c200
TraceAttacks on contact, WaitThink @0x7c678 lifetime). FUNCTIONAL reconstruction on the stock SDK
(no CTSPhysicObject):
- `CTSShrapnel : CBaseEntity` (ts_shrapnel): MOVETYPE_BOUNCE + SOLID_BBOX; BounceTouch TraceAttacks
  a damageable hit (14 dmg DMG_BULLET, then remove) or bounces off the world ≤2× before expiring;
  1.5s FadeThink. `Create(owner, origin, velocity)`.
- `CM61Grenade::M61Tumble`/`M61Detonate` replace the stock TumbleThink→Detonate: tumble in flight,
  then at the fuse spawn 10 random-direction fragments + `CGrenade::Detonate` (stock area blast).
- `models/shrapnel.mdl` precached at map load (PrecacheAllTSWeapons; valve fallback model).

**Verified RUNTIME (rcbot, ts_wyndham, temp probe reverted):** a thrown grenade detonated and
spawned the shrapnel burst (fragments bounce, several alive post-blast), no crash; clean
regression (factory `ts_shrapnel` exported).

**SLICE:** no CTSPhysicObject bounce, fixed 10 fragments/14 dmg, no fragment trail/sound.

---

## 2026-06-21 (cont. 28) — Death handling: CTSDeadCorpse + drop-arsenal-on-death

**Done — `dlls/ts_corpse.cpp` + a `CTSGameRules::PlayerKilled` hook.** Completes the on-death side
left open in cont.27 and adds the TS death body.

RE (from the .so): `CTSDeadCorpse` (Spawn @0x7040c, WaitThink @0x70460) + `TSCreateDeadCorpse`
(@0x60234). The corpse is `CBaseEntity::Create("…")` copying the player's origin/angles (rep movsl)
+ pev->model (+0xb8), modelindex (+0xb4), sequence/frame (+0x130/+0x1a8), `movetype = 6`
(MOVETYPE_TOSS @+0x108); WaitThink (0.5s) ramps renderamt down by `((fadeEnd-time)/(lifetime*0.667))*255`
then UTIL_Remove. Functional reconstruction:
- `CTSDeadCorpse : CBaseAnimating` (ts_corpse): MOVETYPE_TOSS + SOLID_NOT + kRenderTransTexture;
  `FadeThink` fades renderamt then removes (15s lifetime). `TSCreateDeadCorpse(player, lifetime)`
  copies model/origin/angles/sequence/frame(framerate 0 = frozen)/body/skin/colormap.
- `TSDropWeaponsOnDeath(player)` (ts_gun.cpp): scatter the whole arsenal into the world as
  CTSWorldGuns (random toss) — the on-death drop noted as SLICE in cont.27.
- `CTSGameRules::PlayerKilled` override: `CHalfLifeMultiplay::PlayerKilled` (scoring/death-notice),
  then drop weapons + spawn the corpse.

**Verified RUNTIME (rcbot, ts_wyndham, temp probe reverted):** a bot given beretta+uzi then
force-killed (suicide TakeDamage) produced exactly **1 ts_corpse + 2 ts_worldgun**; no crash; clean
regression (factory `ts_corpse` exported).

**SLICE:** corpse copies the live pose at PlayerKilled time (no dedicated death-anim capture);
melee/no-world-model weapons aren't dropped; no gib/blood. The on-death weapon drop now means the
CTSWorldGuns are reachable in normal play (kill someone → their guns drop, press USE to grab).

---

## 2026-06-21 (cont. 27) — CTSWorldGun (player-dropped weapons: drop + USE-pickup)

**Done — `dlls/ts_worldgun.cpp`: the dropped-weapon-in-the-world entity.** Completes the weapon
lifecycle (map pickups via CTSGroundWeapon cont.11; player drops here).

RE (from the .so): `CTSWorldGun : CTSPhysicObject` carries a tsweapon_t (+0x88) + the full
s_weapon_status (14 ints); created by `CBasePlayer::DropWeapon`/`ThrowWeapon` -> `CTSGun::DropWeapon`
(removes from the arsenal) -> `CTSWorldGun::CreateWeapon`. `GetWeapon` (@0xa1198) -> `PickUpWeapon`;
`WaitTouch` (@0xa1250) gates pickup on `pev->button & IN_USE` (0x20) (or auto for the flag, type 0x1d).
FUNCTIONAL reconstruction on the stock SDK (CM61Grenade-style; no CTSPhysicObject):
- `CTSWorldGun : CBaseEntity` carrying type + loaded clip. Spawn: w_model + MOVETYPE_TOSS;
  FallThink settles on FL_ONGROUND -> SOLID_TRIGGER + WaitTouch. WaitTouch: living player pressing
  IN_USE -> TSPickUpWeapon + UTIL_Remove.
- `TSCreateWorldGun(type, clip, origin, vel)` = the drop side.
- `CTSGun::RemoveActiveWeapon(type, clip)` (@DropWeapon 0x8094c): shift the active slot out of the
  arsenal (requires >=2 weapons — no unarmed state, SLICE), wield the next; free `TSDropActiveWeapon`/
  `TSPickUpWeapon` (the latter re-adds via GiveTSWeapon + restores the carried clip).
- `dropweapon` ClientCommand drops the active weapon forward (250u + player velocity).

**Verified RUNTIME (rcbot, ts_wyndham, temp probe reverted):** a bot holding {beretta, uzi} dropped
the uzi (type 3, clip 32; arsenal 2->1) -> ts_worldgun spawned + settled (SOLID_TRIGGER); a forced
USE-touch handed the uzi back (holds weapon_tsgun, world gun removed). No crash; clean regression
(factory `ts_worldgun` exported).

**SLICE:** no CTSPhysicObject bounce, carries type+clip only (the shared caliber reserve stays with
the dropper), no on-death auto-drop, no flag auto-pickup, no throw arc.

---

## 2026-06-21 (cont. 26) — CTSGameRules::PutPowerUp (dynamic deathmatch powerup spawning)

**Done — the dynamic powerup spawner.** Un-SLICEs the `CTSGameRules::PutPowerUp` (@0x9797c)
niche left in cont.13. DM maps now spawn powerups dynamically even without ts_powerup placements.
- `CTSGameRules::PutPowerUp(type)`: collect `info_player_deathmatch` spots (cap 32), pick a
  random one, skip if a `ts_powerup` is already within 128u, else create a powerup there via a
  new `TSCreatePowerUpAt(type, origin)` free helper (ts_powerup.cpp; wraps
  `CTSPowerUp::CreatePowerUp` + UTIL_SetOrigin so it drops from the spot).
- `CTSGameRules::Think` override (the rules object now runs TS think): `CHalfLifeMultiplay::Think()`
  then, in DM only, roll `GetRandomPowerup(0x1ff)` every ~20s and PutPowerUp it (1s retry on
  fail). Mirrors the .so Think powerup block (`if mode<2 … every 20s`). DM gate = `!IsTeamplay()`
  (which as of cont.35 == the objective-manager proxy).

**Verified RUNTIME (temp probe reverted):** ts_wyndham (DM) spawned a random powerup every 20s
(t=1 kungfu, t=21 slowmo, t=41 kungfu); tm_avana (objective map) correctly skipped (0 spawns).
No crash; clean regression.

---

## 2026-06-21 (cont. 25) — Objective subsystem (ts_hack/wingiver/manager/ptr) — team-mode maps

**Done — `dlls/ts_objectives.cpp`: the 4 objective entity types the `tm_` maps place.** Loading
a team-mode map (tm_avana) showed 7 Can't-inits: ts_hack(2), ts_objective_manager(1),
ts_objective_ptr(1), ts_wingiver(3). All cleared.

RE (from the .so): these are point/brush entities tied into a CTSGameRules team subsystem
(`TeamWin` @0x68a08 1564 bytes, `ShowObjectiveOnHud`, `AddObjectiveInList`) that ISN'T built —
the interaction "who is hacking" + the win scoring/round-restart are gamerules-driven. Built a
FUNCTIONAL standalone reconstruction instead:
- `CTSObjective` (base @0x8c2b0): `attackerteam` keyvalue (+0x68, -1=any) + CanInteract(team)/
  Freeze(bool). (.so Freeze also calls ShowObjectiveOnHud — SLICE.)
- `CTSObjectiveManager` (@0x8b7bc, ts_objective_manager): objptargeta..h / messagea..d /
  targeton{timeleft,specdead,mercdead} keyvalues; `Reset()` HUDs the messages + FireTargets the
  8 objptargets (matches @Reset 0x8b850).
- `CTSObjectivePointer` (@0x8bb44, ts_objective_ptr): per-team objmsg{spec,merc,team3,team4,all}
  (the visible HUD arrow is cl_dll — server stub).
- `CTSHack` (@0x8cccc, ts_hack): brush; SET_MODEL + SOLID_TRIGGER; an attacker-team living player
  in the volume (HackTouch) drives a `hackingtime` countdown (WaitThink, 1s cadence like the .so);
  on 0 → fire pev->target + HUD pev->message + Freeze. SLICE: faithful start-hack is gamerules-driven.
- `CTSWinGiver` (@0x8bf10, ts_wingiver): Use'd `callsnumber` times → declares `winnerteam` (HUD).
  SLICE: faithful Use → CTSGameRules::TeamWin(team, victory/defeat cash, …) + round restart.

**Verified RUNTIME (tm_avana, temp probe reverted):** 2 hack / 3 wingiver / 1 manager / 1 ptr
spawn (0 Can't init, was 7); keyvalues parse (hack time=10 attackerteam=-1, wingiver team=2
calls 1); a forced-hacker hack counted 10→0 and completed (done=1); wingiver Use → declares the
winner (done=1). tm_corporate also 0 Can't init. No crash; clean.

**Next:** the real payoff is the **CTSGameRules team subsystem** (TeamWin/team assignment/scores/
RestartRound) — it turns these standalone entities into an actual scored team game mode and
unblocks the rest of the tm_ maps (ts_briefcase/ts_bomb/ts_flag CTF). Still also: the buy menu,
CTSSharedEmitter (cl_dll), the visible slow-motion (cl_dll).

---

## 2026-06-21 (cont. 24) — Shotgun fidelity: 8-pellet count + mossberg/sawedoff fire-types

**Done — closed the two known weapon data gaps.**
- s_weapon_info gains `iPellets` (bullets-per-shot, `s_weapon_info+0x24`): **8 for the five
  shotguns** (benelli/usas/spas/mossberg/sawed-off), 1 otherwise (extracted from each
  Write_<weapon>). `StandardFire` uses it instead of the hardcoded `(ft==PUMP)?6:1` guess —
  which was both wrong (8, not 6) and missed usas/sawedoff (not pump fire-type but 8 pellets).
- Fixed two fire-types the OLD extract_weapons.py mis-read as 0 (reading the Write_ funcs
  directly gives the truth): **mossberg 0→1 (pump), sawedoff 0→8 (semi, twin-barrel)**.

**Verified RUNTIME (rcbot, ts_wyndham, temp probe reverted):** mossberg fireType=1 pellets=8,
sawedoff fireType=8 pellets=8, usas fireType=6 pellets=8, beretta control pellets=1; all fire
without crash. Clean regression.

**Note on the bigger picture:** the weapon engine is now functionally complete (arsenal,
all fire-types, akimbo, fire-mode/burst, per-caliber ammo, correct pellets). The remaining TS
gameplay systems are all LARGE/interconnected or client-entangled: the buy menu (cash economy
@BuyWeapon checks CashBased()/weapon price s_weapon_info+0xd8/player cash +0x540 + a client
menu UI), the team/objective subsystem (CTSGameRules::TeamWin @0x68a08 is 1564 bytes atop
SetTeamList/UpdateTeamScores/RestartRound/InitTeamScore — all the ts_wingiver/ts_hack/
ts_briefcase entities live in tm_ maps needing it), the networked particles (CTSSharedEmitter,
needs cl_dll), and the visible slow-motion time-dilation (needs cl_dll). Next agent: pick the
team foundation (biggest gameplay unlock, server-side) or the buy menu server half.

---

## 2026-06-21 (cont. 23) — Per-caliber ammo (CTSGun::AddAmmo + CTSGroundAmmo wiring)

**Done — the faithful per-caliber ammo system.** Un-SLICEs the per-slot generic reserve +
the CTSGroundAmmo 9mm-only refill; ammo is now pooled per caliber and shared across same-
caliber weapons (a single beretta + akimbo berettas draw from one 9mm pool).

RE findings (from the .so):
- Each weapon names a CALIBER byte at `s_weapon_info+0x8` (extracted from each Write_<weapon>):
  0 none(melee/grenade), 1 9x19, 2 .45, 3 SHELLS, 4 5.56, 5 .50AE, 6 7.62, 7 5.7, 8 50BMG,
  9 10mm, 10 .22, 11 .454, 12 .32, 13 .45LC. The gun holds 16 per-caliber reserves at `+0xd4`.
- `compile_ammo_info` (@0x6f51c) builds a per-caliber table (stride 0x38): +0x10 = magazine
  size, **+0x20 = max reserve (rounds)** — 9mm 80, .45 90, shells 90, 5.56 150, 7.62 170,
  50BMG 200, … (full table extracted).
- `AddAmmo(caliber,count)` (@0x808ac): `reserve[caliber] = min(reserve+count, max[caliber])`.
- `ReloadWeapon` (@0x84a14): `take = min(clipSize - clip, reserve[caliber])` → reserve is in
  ROUNDS, drawn into the clip. (Settled the rounds-vs-mags question.)
- `CTSGroundAmmo::WaitTouch` (@0xa2f40): loops the 16 calibers calling `AddAmmo(gun, cal,
  m_mags[cal])` — the FGD `magsN` keyvalues add raw rounds to caliber N (the station persists,
  topping up on every touch; real maps set magsN = 3–5).

Functional reconstruction:
- `ts_weapons.{h,cpp}`: s_weapon_info gains `iCaliber` (the +0x8 byte, full table); `kAmmoMax[16]`
  + `TSAmmoMax(caliber)`.
- `ts_gun.cpp` CTSGun: **replaced the per-slot `m_arsenalReserve[]` with per-caliber
  `m_ammoReserve[16]`**. `AddAmmo` method (capped). Spawn/AddWeapon seed the caliber reserve
  (clip*3); the akimbo upgrade adds the 2nd pistol's rounds to the shared pool; Reload draws
  from `m_ammoReserve[caliber]`; MultiplyClips (inf-clip powerup) tops the active caliber.
  Free `TSAddAmmo(player, caliber, count)`.
- `ts_groundammo.cpp` WaitTouch: per-caliber `TSAddAmmo(player, cal, m_mags[cal-1])` (was the
  9mm-only SLICE).

**Verified RUNTIME (rcbot, ts_wyndham, temp probe reverted):** beretta → clip 15 / 9mm reserve
45; emptied clip → Reload drew 15 from the 9mm pool → 15/30; 2nd beretta → akimbo (type 8) +
SHARES the 9mm pool (reserve 30+15=45); +m16 → 5.56 reserve 54, 9mm unchanged (separate pools);
40× AddAmmo(1,3) capped at max 80. Clean regression on ts_supertimor_d3 (6 ground-ammo packs):
0 Can't init, 0 crashes.

**SLICE/deferred:** starting reserve = clip*3 (the faithful buy menu / BuyArmament sets the real
loadout); calibers 12/13 (skorpion/contender) have no ground-ammo magN; no client ammo HUD;
grenade (caliber 0) still uses its own m_iClip count (no caliber reserve).

---

## 2026-06-21 (cont. 22) — Fire-mode system (auto/semi/burst/pump + SwitchFireMode)

**Done — the faithful trigger engine.** Auto/semi/burst/pump is now decided by a per-slot
FIRE-MODE (orthogonal to fire-type, which still picks the fire *function*) — replacing the
old fire-type-based auto/semi approximation.

RE findings (from the .so):
- `PrimaryAttack` (@0x81a20): melee/grenade/katana/flag dispatch by fire-type FIRST (outside
  the mode system); the bullet/akimbo path then gates on the fire-mode byte `m_pActiveItem+2`:
  mode 0 full-auto (held), 1 semi (per-press; dblfire powerup → held), 2 burst (set count on
  press, return — fired by WeaponIdle), 3 pump (held), 4 akimbo-alt (per-press), 5 akimbo-both
  (held). Burst count = `(weaponType != 0x17) + 2` → **3 rounds, 2 for the MAC-10 (0x17)**.
- `WeaponIdle` (@0x854dc): while firemode==2 and burst count>0 and the idle timer elapsed →
  `Fire()` + count-- (this completes a tapped burst after the trigger is released).
- `StandardFire` (@0x824a4): burst rate = 0.03 between rounds, 0.2 after the last.
- `SwitchFireMode` (@0x8169c): cycle the mode byte to the next bit set in the weapon's mode
  MASK at `s_weapon_info+0xc`, modes 0..5 wrapping. Default mode (`TakeWeapon`): auto if
  offered, else pump, else burst, else semi. Masks extracted from each `Write_<weapon>`
  (`movl $imm,0xc(%eax)`): e.g. beretta=2 (semi), uzi=3 (auto+semi), m16=6 (semi+burst,
  defaults burst), mp5sd/mp5k/mac10=7 (auto+semi+burst), akimbos 18/33.

Functional reconstruction:
- `ts_weapons.{h,cpp}`: s_weapon_info gains `iFireModes` (the +0xc mask, full table extracted);
  `enum tsfiremode_mode_t`; `TSDefaultFireMode(mask)`.
- `ts_gun.cpp` CTSGun: per-slot `m_arsenalMode[]` + active `m_fireMode` + `m_burstCount`
  (saved/loaded by SelectSlot; defaulted in Spawn/AddWeapon/akimbo-upgrade). PrimaryAttack
  rewritten to gate by m_fireMode → `Fire()` (dispatches by fire-type: AkimboFire or the new
  `StandardFire`, factored from the old inline bullet code). WeaponIdle drives the burst.
  `SwitchFireMode()` + free `TSSwitchFireMode` + the `tsfiremode` ClientCommand.

**Verified RUNTIME (rcbot, ts_wyndham, temp probe reverted):** default modes correct for all
masks (beretta→1 semi, uzi→0 auto, m16→2 burst, mp5sd→0 auto); `SwitchFireMode` cycles only
the available modes (beretta stays 1; uzi 1↔0; m16 1↔2; mp5sd 1→2→0); m16 burst fired exactly
3 rounds per tap (clip 18→15, burst 3→0). No crash; clean regression (only the known
`ts_emitter`/`ts_particle` Can't-inits remain).

**SLICE/deferred:** burst sustain depends on the SDK ItemPostFrame/WeaponIdle cadence (a held
burst fires via PrimaryAttack, a tapped one finishes via WeaponIdle); no client-side fire-mode
HUD indicator (the .so writes `mode+1` to player+0x49a per weapon); pump (mode 3) doesn't yet
play a pump-action animation; the dblfire→semi-auto-continuous interaction is approximated.

---

## 2026-06-21 (cont. 21) — Akimbo (fire-types 4/5): dual-wield upgrade + AkimboFire

**Done — akimbo dual-wielding.** Un-SLICEs fire-types 4 (combinable) & 5 (gold-only),
the last unhandled PrimaryAttack fire-type.

RE findings (from the .so):
- `IsWeaponAkimbo(t)` = `get_weapon_info(t)->iFireType==4`; `IsOnlyAkimbo(t)` = `==5`
  (objdump @0x7019c/0x701d4 — both just compare the +0x348 fire-type byte).
- Akimbo PAIRING fields in s_weapon_info: **+0xc8** = the akimbo variant a single pistol
  upgrades into (-1 if none); **+0xcc** = the single base of an akimbo (= self for
  non-akimbo). Read from each `Write_<weapon>` (`movl $imm,0xc8/0xcc(%eax)`; the table is
  .bss so file reads are garbage — must read the Write_ funcs). Pairs: 2↔8 (beretta),
  3↔16 (uzi), 9↔10 (mk23), 14↔30 (fnh), 17↔37 (skorpion); gold(21) is ft-5, no single.
- `TakeWeapon` (@0x80a04): owning the single + picking up an identical 2nd → `local_12`
  becomes the +0xc8 akimbo → combines in place (clip = sum of both barrels). `AkimboFire`
  (@0x829c0) splits the clip into left/right barrels (m_pActiveItem+0x1c/+0x20), alternates
  the firing barrel (one per IN_ATTACK/IN_ATTACK2) from a ±5u right-vector muzzle offset;
  rate = flFireRate, or fixed 0.1 in alternating fire-mode.

Functional reconstruction (on the cont.18 single-clip arsenal):
- `ts_weapons.{h,cpp}`: s_weapon_info gains `iAkimboType`/`iSingleType`; set in
  compile_weapons_info. Added `TSFT_AKIMBO_ONLY=5`.
- `CTSGun::AddWeapon`: if you already hold the single + it has an akimbo upgrade + don't
  hold the akimbo → upgrade the slot in place (SelectSlot first so m_iClip is live, then
  type→akimbo, clip = single's rounds + a 2nd pistol capped at the doubled clip, reserve
  += a 2nd pistol, re-Deploy). Slot count stays the same (not a new slot).
- `CTSGun::AkimboFire(wi)` (dispatched for ft 4/5): full-auto-feel (fires while held — no
  IN_ATTACK2 plumbed), `m_akimboSide ^= 1` each shot → ±5u v_right muzzle offset, one
  bullet/clip-- per shot, flSpread/flDamage/flFireRate (+ DoubleFireRate powerup halving).

**Verified RUNTIME (rcbot, ts_wyndham, temp probe reverted):** gave a bot beretta, then a
2nd beretta → upgraded in place to **type 8 (beretta_akimbo), clip 30** (15+15 capped),
reserve 60, arsenal count stayed 1. Held trigger → AkimboFire fired (clip 30→26→16→6, then
reload from reserve → 26, side toggled 0↔1), active item = the gun. No crash; clean
regression (0 crashes; only the known `ts_emitter`/`ts_particle` Can't-init remain).

**SLICE/deferred:** single active clip (not a true L/R barrel split); no IN_ATTACK2
"fire both barrels at once" (firemode 5 path); no per-barrel reload; the akimbo's separate
view-model akimbo animation events are not wired. Fire-mode/burst (`SwitchFireMode`) and the
buy menu remain the next weapon-engine items.

---

## 2026-06-21 (cont. 20) — CM61Grenade (grenade fire-type 2 throws a grenade)

**Done — `dlls/ts_grenade.cpp` CM61Grenade + the grenade fire-type.** Un-SLICEs
fire-type 2 (was falling back to bullets) + makes the grenade powerup useful.
- The faithful CM61Grenade is `CM61Grenade : CTSGrenade : CTSPhysicObject` (TS physics
  base classes, not built) with event-driven creation + a custom WaitThink (traces 64u
  ahead for near-impact detonation). FUNCTIONAL reconstruction onto the stock SDK
  `CGrenade` instead (which already does arc/bounce/timed-detonate): `CM61Grenade::Throw`
  replicates `CGrenade::ShootTimed` but with `w_m61.mdl` (so no models/grenade.mdl
  precache dependency) — MOVETYPE_BOUNCE, `CGrenade::BounceTouch`, `TumbleThink`→
  `Detonate` at dmgtime, gravity 0.5/friction 0.8. Free `TSThrowM61(pevOwner, src, vel,
  fuse, dmg)`.
- `CTSGun::PrimaryAttack` dispatches TSFT_GRENADE (2) → new `GrenadeThrow(wi)`: one throw
  per press, decrement clip, toss from gun-pos+16 forward at 800u + player velocity, 4s
  fuse, damage = table flDamage*4 (~120). (m61 table dmg 30 is the per-pickup value; the
  explosion is area damage.)

**Verified RUNTIME (rcbot, ts_wyndham, temp probe reverted):** a bot with the M61 threw
100 grenades over 30s (valid entities, clip decrementing); TumbleThink switches to
Detonate at dmgtime (model-independent: `if (dmgtime <= time) SetThink(Detonate)`), so
they arc/bounce/explode. No crash; clean regression.

**SLICE/deferred:** no CTSPhysicObject physics, no impact-trace early detonation, no
throw event/animation; akimbo (fire-types 4/5) still falls back to bullets.

---

## 2026-06-21 (cont. 19) — Powerup→weapon wiring: MultiplyClips + DoubleFireRate

**Done — two now-easy powerup effects, faithful on the cont.18 arsenal.** Completes
the powerup→weapon connection.
- `CTSGun::MultiplyClips()` (@0x71308, the infinite-clip powerup): doubles the active
  arsenal slot's reserve (>= a full resupply, capped ~6 clips). Free `TSMultiplyClips
  (player)` wraps it (GetPlayerTSGun). `PlayerAddPowerUp`'s inf-clip branch now calls it
  (was a SLICE clip-refill).
- `CTSGun::PrimaryAttack`: `if (DoubleFireRate(m_pPlayer)) flRate *= 0.5` — the
  double-firerate powerup now halves the fire interval (the accessor already flips true
  while the dblfire powerup is active; just wired into the rate).

**Verified RUNTIME (rcbot, ts_wyndham, temp probe reverted):** beretta inf-clip reserve
45→90 (doubled); dblfire baseRate 0.100 → rate 0.050 (halved), 105 fires at the doubled
rate. No crash; clean regression.

---

## 2026-06-21 (cont. 18) — CTSGun weapon arsenal (the items-array, modern reconstruction)

**Done — the player's weapon_tsgun now holds a multi-weapon ARSENAL.** A clean
MODERN reconstruction of the original items-array (the faithful one is a 0x38-byte
s_weapon_status items[] @CTSGun+0x114 with m_pActiveItem@0xcc / m_pActiveInfo@0xd0 +
weapon/ammo-group masks + akimbo pairing — heavy byte layout, reconstructed from
intent, not transcribed).
- `dlls/ts_gun.cpp` CTSGun: added parallel arrays `m_arsenalType/Clip/Reserve[12]` +
  `m_arsenalCount` + `m_active`. `m_weaponType` now tracks the active slot (so the
  fire-type dispatch + Deploy are unchanged).
- `FindSlot(type)` / `AddWeapon(type)` (@AddItem__6CTSGun: add a slot or resupply,
  then wield) / `SelectSlot(slot)` (@SelectWeapon__6CTSGun: save the wielded clip,
  load the new slot's, re-Deploy if held).
- `Spawn` seeds the arsenal with the placed type (clip full, reserve = 3 clips).
- `PrimaryAttack` unchanged (operates on m_iClip = the active slot's clip). `Reload`
  now refills m_iClip from the active slot's own reserve (not DefaultReload).
- `GiveTSWeapon(player, type)`: if the player already holds weapon_tsgun, AddWeapon
  to it; else create + pick up. (Helper `GetPlayerTSGun` walks m_rgpPlayerItems.)
- `CTSGroundWeapon::GetWeapon`: dropped the "one TS weapon at a time" guard — picking
  up a ground weapon now ADDS it to the arsenal.
- `tsnextweapon` ClientCommand → `TSCycleWeapon` cycles the active slot.

**Verified RUNTIME (rcbot, ts_wyndham, temp probe reverted):** a bot built a 3-weapon
arsenal (beretta→+uzi→+knife), `tsnextweapon` cycled 0→1→2→0 through all three, each
deploying its own view model with its own clip/reserve (beretta 15/45, uzi 32/96,
knife 18/54). PER-SLOT CLIP PERSISTS: the bot fired the uzi (clip 32→30) and the count
survived switching away+back. No crash; clean regression (0 Can't init, 0 crashes).

**SLICE/deferred:** no akimbo pairing (single active slot — the faithful engine wields
two for fire-types 4/5), no weapon/ammo-groups, no fire-mode/burst state, no buy menu
(`BuyWeapon`/`BuyArmament`); the per-caliber ammo reserve is still per-slot generic (not
wired to CTSGroundAmmo's calibers). The grenade/akimbo PrimaryAttack fire-types still
fall back to bullets.

---

## 2026-06-21 (cont. 17) — CTSGroundAmmo (ts_groundammo refill pack, 16 placements)

**Done — `dlls/ts_groundammo.cpp` CTSGroundAmmo : CBaseEntity.** A map-placed ammo
pack; lifecycle mirrors CTSPowerUp (Spawn→Fall→OnGround settle→WaitTouch/WaitThink),
recovered from the .so (Spawn/OnGround tails truncated by Ghidra noreturn at
UTIL_SetOrigin → read via objdump):
- Spawn: model `models/ammopack.mdl`; movetype = MOVETYPE_FLY (wall, spawnflag 16) or
  MOVETYPE_TOSS (floor); solid SOLID_BBOX; SetThink(Fall) + nextthink time+0.1.
- Fall: settle on FL_ONGROUND → OnGround (or PutOnWall for the wall variant) + drop
  sound. OnGround: SOLID_TRIGGER, MOVETYPE_FLY (stop), clear EF_NODRAW + EF_DIMLIGHT,
  SetTouch(WaitTouch)+SetThink(WaitThink). WaitThink idles (nextthink+1).
- KeyValue: wduration → m_respawnTime; mags1..mags11 (parse the index off "mags") →
  m_mags[0..10] (per-caliber magazine counts: 9x19/.45/SHELLS/5.56/.50AE/7.62/5.7/
  50BMG/10mm/.22/.454).
- WaitTouch: a visible pack refills a touching ALIVE player who holds a TS gun. The
  pack PERSISTS (tops players up when they have room; not consumed on pickup — matches
  the .so, which has no hide/remove in WaitTouch).
- PutOnWall: SLICE = settle-in-place stub.

**SLICE:** faithful WaitTouch loops the 16 ammo-type slots calling `CTSGun::AddAmmo(
caliber, mags)` on the active gun (the per-caliber reserve = part of the unbuilt
weapon items-array; gate is m_pActiveItem id == 0x10). Here it sums the mags and tops
up the held weapon's **9mm** reserve via `CBasePlayer::GiveAmmo(n, "9mm", _9MM_MAX_CARRY)`.

**Verified RUNTIME (rcbot bots, ts_supertimor_d3 = 6 placements), temp probe (reverted):**
6 settled (`Can't init ts_groundammo` gone); a weapon_tsgun-holding bot touched a pack
(14 mags) → GiveAmmo topped up 9mm by 2 then returned -1 (full) on repeat = correct
capped refill-station behaviour. No crash (gdb -batch, no CTSGroundAmmo in backtrace).
Final clean regression boot: 0 `Can't init`, 0 crashes.

---

## 2026-06-21 (cont. 16) — CTSGun fire-type dispatch (semi/auto/shotgun/melee)

**Done — weapons now fire faithfully by fire-type.** Un-SLICEs "everything is identical
semi-auto bullets".
- `scripts/extract_weapons.py` extended: also pulls per-weapon fire-rate (0x18), base
  spread (0x28), recoil (0x34). Regenerated the table.
- `ts_weapons.{h,cpp}`: s_weapon_info gains flFireRate/flSpread/flRecoil; added
  `enum tsfiretype_t` (0/8 semi, 1 pump, 2 grenade, 3 knife, 4/5 akimbo, 6 auto-shotgun/
  rifle, 7 sniper, 9 SMG-auto, 10 katana, 11 flag).
- `CTSGun::PrimaryAttack` dispatches by iFireType: melee (3/10) → new MeleeAttack()
  hull-trace (knife 48u/katana 64u) + TraceAttack(DMG_CLUB), no bullets; full-auto (9/6)
  fires while held; semi (0/8/7/1 + SLICE akimbo/grenade) one shot per press
  (m_afButtonPressed); pump shotgun (1) → 6 pellets. Uses real flSpread + flFireRate.
- SLICE: grenade(2)/akimbo(4/5) fall back to bullets (need throw entity / items-array,
  where the faithful fire funcs read fire-mode@m_pActiveItem+2 / burst@+8).

**Verified (rcbot, ts_wyndham, probe reverted):** beretta ft=8 SEMI 43 fires (one/press)
vs uzi ft=9 AUTO 201 (held); benellim3 ft=1 6-pellet; knife/katana hull-trace MELEE hit
func_breakable/worldspawn, no bullets. No crash. **Data gaps:** mossberg(26)/sawedoff(33)
fire-type extracted as 0 (are shotguns); pellet count fixed 6; m61 rate/spread suspect.

---

## 2026-06-21 (cont. 15) — CTSSlowMotion entity + GoSlow/InSlow + GlobalSlowMotion2

**Done — slow-motion STATE layer + `dlls/ts_slowmotion.cpp` (ts_slowmotion brush zone,
11 placements).**
- Slow virtuals `GoSlow`/`InSlow`/`GiveSlowMul` (found via CBasePlayer vtable relocs:
  +0x14 GoSlow, +0x18 InSlow). As **CBasePlayer methods**: GoSlow(scale,bullets) sets
  m_flSlowMotionGo (0x168) + m_flSlowMotionTarget (0x170) + m_bSlowBullets; InSlow() =
  (go!=1||target!=1); both init 1.0.
- `CTSGameRules::GlobalSlowMotion2(scale,bullets)` + SetGlobalSlowMotion: GoSlow every
  player, throttled ~0.5s (members 0x560/0x564/0x568/0x56c). SLICE: no projectiles.
- CTSSlowMotion : CBaseEntity (replaces old draft): Spawn (brush trigger; setup beyond
  classname/model/size INFERRED // VERIFY); KeyValue duration/slowamt; WaitTouch (GoSlow
  toucher, track m_hPlayers[33]); ActivateSlow/DeactivateSlow; WaitThink (0.5s poll);
  CheckPlayersDistance (drop beyond m_flRadius from Center()); Use; Save/Restore.

**Verified (rcbot, ts_wyndham=4):** 4 zones; Use() no crash; GlobalSlowMotion2(0.5,1) →
InSlow()=1 go=0.50; (0,0) → InSlow()=0 go=1.0. No crash.

**SLICE/deferred (visible time-dilation, client-entangled):** actual slow MOVEMENT
(client pmove — no cl_dll); projectile slow; SlowMotionFlow (@0x86bb0, powerup→GoSlow);
InterpolateSlowMotion (@0x8910c); motion-blur HUD; cmd_globalslow; Spawn/m_flRadius inferred.

---

## 2026-06-21 (cont. 14) — Powerup activation state machine (UsePowerUp/UpdatePowerUpState)

**Done — held timed powerups ACTIVATE + expire.** From UsePowerUp (@0x85980),
UpdatePowerUpState (@0x85748), DeactivateSlowMotionFlow (@0x86a9c):
- CBasePlayer::UsePowerUp() (ts_player.cpp): `usepowerup` command. Alive + holding +
  none active → m_iPowerups = m_iHeldPowerup, expire = m_flPowerupTime + time; sends
  gmsgTSPowerUpTime + gmsgTSMotionBlur. UpdatePowerUpState() per-frame in PreThink:
  apply slow fields (slowmo 0.6, slowpause 0.8) / expire + clear.
- `usepowerup` ClientCommand (confirmed via .so @0x9d838). Players +0x175/0x17c/0x180.

**Verified (ts_theatre):** bot held slowmo (dur 3) + UsePowerUp → m_iPowerups=1,
SlowMotion()=1, scale 0.60 for 3s, expired + cleared. No crash. (Note: activation sets
m_flSlowMotionScale, NOT GoSlow — the powerup→GoSlow link is SlowMotionFlow, deferred.)

---

## 2026-06-21 (cont. 13) — CTSPowerUp + PlayerAddPowerUp (the 328-placement pickup)

**`dlls/ts_powerup.cpp` CTSPowerUp : CBaseAnimating (ts_powerup, 328 placements).** Full
lifecycle (Spawn + OnGround TRUNCATED by Ghidra noreturn at UTIL_SetOrigin — real tails
via objdump; Spawn's `nextthink = time+0.1` missed → never settled, caught by "22
spawned 0 pickups"). Spawn→Fall→OnGround(SOLID_TRIGGER,SetTouch(WaitTouch))→WaitTouch
(pickup gate: instant mask 0x1e6 always, timed only if none held; re-roll+respawn).
SetupPowerUp: spawnflags = the random MASK (GetRandomPowerup), pwuptype forces exact.
**PlayerAddPowerUp** (@0x87bc8): instant real (armor/kungfu/lowgrav/health/grenade→
GiveTSWeapon(M61) SLICE/infclip SLICE), timed held. Members 0x72c/0x73c. VERIFIED
(ts_theatre=22): 22 spawned all 8 types, 6 pickups, gate works, no crash.

---

## 2026-06-21 (cont. 12) — Full 38-weapon table + CTSGun by type; gdb debugging

`scripts/extract_weapons.py` → full s_weapon_info table (0x44 name, 0x132 v_model,
0x172 p_model, 0x1f2 w_model, 0x232 anim-ext, 0x2c6 fire snd, 0x04 clip, 0x1c damage,
0x348 fire-type; cont.16 added 0x18/0x28/0x34). CTSGun parameterized by m_weaponType;
ground weapon gives real type. CWorld::Precache → UTIL_PrecacheOtherWeapon at map load.
VERIFIED (ts_wyndham): type 25→v_knife dmg40, type 3→v_uzi dmg22. Two GDB-found crashes
fixed (PRECACHE_EVENT map-load; DeathNotice ItemInfoArray). **Use gdb** for crashes.

---

## 2026-06-21 (cont. 11) — CTSGroundWeapon (weapon pickups) + tsweapon_t fix

tsweapon_t index = (lea offset − 0x7d30)/0x3DC, MATCHES FGD tsweaponid. beretta=2, uzi=3,
knife=25, idx 29=flag. ts_groundweapon.cpp (65 placements): SLICE gives weapon_tsgun;
respawn after wduration. VERIFIED (ts_wyndham).

---

## 2026-06-20 (cont. 10) — CTSGun recon + weapon data layer (vertical-slice start)

**CTSGun is a subsystem:** ONE unified CBasePlayerWeapon (id 16) morphing by tsweapon_t.
**tsweapon_t enum (38, compile_weapons_info @0x6fe28):** 0 melee,1 glock18,2 uzi,
3 uzi_akimbo,4 benellim3,5 m4,6 mp5sd,7 mp5k,8 beretta_akimbo,9 beretta,10 mk23,
11 mk23akimbo,12 usas12,13 desert,14 ak47,15 fnh,16 aug,17 skorpion,18 barrett,19 pdw,
20 spas12,21 gold,22 glock22,23 mac10,24 m61,25 knife,26 mossberg,27 m16,28 ruger,
29 fnhakimbo,30 bull,31 m60,32 sawedoff,33 katana,34 sealknife,35 contender,
36 skorpion_akimbo,37 flag.
FAITHFUL CTSGun engine (items array m_pActiveItem@0xcc/m_pActiveInfo@0xd0/items[]@0x45,
fire-type dispatch s_weapon_info+0x348, akimbo/firemode/zoom) replaces // SLICE.

**Bot test rig (working, reusable):** ~/Containers/hlds/hlds-linux64-test:
metamod_amd64.so (Metamod-R) as gamedll (liblist.gam → it); config.ini=`gamedll
dlls/hl.so`; plugins.ini=`linux addons/metamod/dlls/rcbot_mm_amd64.so`. <rig>/rcbot/*;
min_bots 2. Boot → rcbot bots = real CBasePlayers. Temp StartFrame hook can give/select/
teleport/fire on a bot (gate on a player existing — bots connect ~t>6-8; pulse IN_ATTACK
via m_afButtonPressed to test semi vs auto; slide an entity onto a bot to force touch).
Revert hooks after. Probe output → <rig>/ts/qconsole.log with -condebug.

---

## 2026-06-20 (cont. 9) — TSHudMessageOne + CBasePlayer TS extensions (step 1)

`dlls/ts_utils.{h,cpp}` TSHudMessageOne. CBasePlayer TS member block (player.h):
m_iPowerups (0x724), m_iHeldPowerup (0x728), m_flPowerupTime (0x730), m_iKungFu (0x74e),
m_bLowGravity (0x74f) + accessors. (cont.13 +0x72c/0x73c; cont.14 +0x175/0x17c/0x180;
cont.15 +0x168/0x170 GoSlow/InSlow.)

**The core is a monolith.** Sequencing done: (1) player block+accessors; (2) CTSGun
base; (3) PlayerAddPowerUp + CTSPowerUp; (4) UsePowerUp/UpdatePowerUpState; (5) GoSlow/
InSlow + GlobalSlowMotion2 + CTSSlowMotion; (6) CTSGun fire-type dispatch; (7)
CTSGroundAmmo. NEXT: the faithful weapon items-array (m_pActiveItem/m_pActiveInfo +
s_weapon_status + AddAmmo + PickUpWeapon/BuyWeapon armament + buy menu — un-SLICEs
akimbo/grenade/burst-fire, the per-caliber ammo, and the inf-clip/grenade powerup
branches) and/or the visible slow-motion time-dilation (needs cl_dll); objective/CTF
entities (CTSHack/CTSBriefcase/CTSBomb/CTSFlag/CTSWinGiver) for team modes. Fill
CTSGameRules methods as needed.

---

## 2026-06-20 (cont. 8) — CTSGameRules skeleton installed + runtime-verified

**No CTSPlayer class** — TS extends stock CBasePlayer. `CTSGameRules : CHalfLifeMultiplay`
is the live g_pGameRules (InstallGameRules). GetRandomPowerup(mask) reconstructed.
Caveat: tm_ teamplay maps use MP base; revisit when TS team logic lands.

## 2026-06-20 (cont. 7) — CTSMapGlobals runtime-verified
get_weapon_restriction() = 0x1f ^ spawnflags. (ts_trinityfall).

## 2026-06-20 (cont. 6) — CTSModel + CTSFog; FIRST in-engine runtime verification
`dlls/ts_model.cpp` cycler-style prop (858 placements). Decompile TRUNCATED by Ghidra
noreturn at UTIL_SetSize; real body via objdump (Ghidra adds 0x10000 vs nm).
**Runtime rig:** 64-bit ReHLDS (QUIC) at ~/Containers/hlds/hlds-linux64-test. Boot:
`LD_LIBRARY_PATH=.:$LD_LIBRARY_PATH timeout -s INT 30 ./hlds_linux -game ts -condebug
+developer 1 +sv_lan 1 +maxplayers 4 +map <map>`. SIGINT not KILL. `Can't init <class>`
= negative control. With -condebug, ALERT(at_console) probe → <rig>/ts/qconsole.log.

## 2026-06-20 (cont. 5) — CTSCustomTarget built (point relay; no stock map uses it).
## 2026-06-20 (cont. 4) — DECISION: functional/source-faithful, NOT byte drop-in.
   ts_utils TSHudMessageAll + ts_timer CTSTimer. glibc 2.14.
## 2026-06-20 (cont. 3) — CTSSlowMotion draft (superseded by cont.15).
## 2026-06-20 (cont. 2) — gnv2demangle.py (1788/2426 typed sigs) + DecompExport.java.
## 2026-06-20 (cont.) — Vino OK'd reconstruction; Ghidra pipeline (decompile.sh).
## 2026-06-20 — Scaffolding: fork halflife-updated@64bit-demo; reference/; tooling;
   work-list RECONSTRUCTION.md (60 TS classes). Builds via hlds64-buildchain image.

**Conventions:** NO Co-Authored-By trailer. CLAUDE.md local-only. Auto mode:
halt & ask on a block.
