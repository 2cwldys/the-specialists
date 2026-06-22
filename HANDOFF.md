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

## Where things stand (2026-06-22, through cont.50)
Pipeline fully working: decompile → recover typed prototypes → write modern C++ →
**build into `hl.so`/`client.so`** → **run in-engine** (ReHLDS for server; xash3d-fwgs
for the graphical client — this box HAS a GPU) → **gdb** any crash. Everything below is
committed (`git log`); `recovered/` is gitignored.

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

**Next slices** read messages the server already sends: the **powerup/motion-blur** indicators
(`gmsgTSPowerUpTime`/`gmsgTSMotionBlur` — FIRST verify their registered names against the `.so` strings,
as `RoundTime` needed in cont.47). Then the bigger Track B items further down (visible slow-motion,
particles, stunts). `reference/client.dll` (~1,013 exports) is the spine — see CLAUDE.md §2.
Two known stock-client gaps surfaced at runtime: `Unknown command cl_realhud / hud_r/g/b/color /
mp_teammodels` (TS client cvars not yet reconstructed — harmless).

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
