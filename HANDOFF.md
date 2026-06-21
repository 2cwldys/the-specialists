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

## Where things stand (2026-06-21)
Pipeline fully working: decompile → recover typed prototypes → write modern C++ →
**build into `hl.so`** → **run in-engine** (with bots for player-needing code) →
**gdb** any crash. Everything below is committed (`git log`); `recovered/` is
gitignored. Decision locked: **functional fidelity, NOT byte/ABI match**.

- **Map entities — runtime-verified:** `CTSModel` (108 on `ts_kissmeyerbasic`),
  `CTSMapGlobals` (weapon-restriction on `ts_trinityfall`), `CTSGroundWeapon`
  (`ts_wyndham` pickups), `CTSPowerUp` (22 on `ts_theatre`, bots pick up),
  `CTSGroundAmmo` (`ts_groundammo` refill pack, 6 on `ts_supertimor_d3`),
  `CTSSlowMotion` (`ts_slowmotion` zones, 4 on `ts_wyndham`). Built + factory-only
  (no stock map hits them): `CTSFog`, `CTSTimer`, `CTSCustomTarget`.
- **Game rules:** `CTSGameRules : CHalfLifeMultiplay` IS the live `g_pGameRules`
  (`ts_gamerules.{h,cpp}` + `InstallGameRules`). Only `GetRandomPowerup` is real;
  the other 40 methods inherit MP. `TSGameRules()` accessor + `TS_PWUP_*` bits.
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
  `gmsgTSPowerUp` HUD + lowgrav physics-key deferred (client.dll side);
  `CTSGameRules::PutPowerUp` (dynamic spawner) not wired yet.
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
- **SLICE — the visible time-dilation layer is still deferred (client-entangled):** the
  actual slow-motion *movement* (the client pmove reads the slow fields — no cl_dll
  reconstructed); `GlobalSlowMotion2` slowing projectiles (needs `GoSlow`/`InSlow` as
  **CBaseEntity** virtuals — currently CBasePlayer-only); `SlowMotionFlow` (@0x86bb0,
  the powerup→`GoSlow` propagation — so an activated slowmo powerup sets
  `m_flSlowMotionScale` but doesn't yet call `GoSlow`); `InterpolateSlowMotion`; the
  client motion-blur HUD; the on-death powerup re-drop. `CTSSlowMotion`'s exact Spawn
  trigger-setup + `m_flRadius` are inferred (// VERIFY).
- **Weapons — runtime-verified, arsenal + all 38 types + fire-type dispatch (slice):**
  `CTSGun` (`ts_gun.cpp`, entity `weapon_tsgun`, id 16) now holds a multi-weapon
  **arsenal** (`m_arsenalType/Clip/Reserve[]` + active slot — a modern stand-in for the
  original `items[]`/`m_pActiveItem`). `AddWeapon`/`SelectSlot`; `GiveTSWeapon` adds to
  the held gun; the `tsnextweapon` command cycles; per-slot clip/reserve persist across
  switches; ground-weapon pickups stack into the arsenal. `PrimaryAttack` dispatches by
  **fire-type** with recovered per-weapon rate/spread: **semi** (one/press), **full-auto**
  (held), **pump shotgun** (6 pellets), **melee** (knife 48u/katana 64u hull-trace, no
  bullets). Full `s_weapon_info` table in `ts_weapons.{h,cpp}` (incl. `flFireRate`/
  `flSpread`/`flRecoil`). Roster precached at map load via `CWorld::Precache`.
  Powerup hooks wired: inf-clip `MultiplyClips`, double-firerate halves `flFireRate`.
  **Grenade fire-type (2) throws an M61** (`CM61Grenade`, `ts_grenade.cpp`).
  **Akimbo fire-types (4/5) dual-wield** (cont.21): picking up a 2nd of a single pistol you
  already hold upgrades that arsenal slot in place to its akimbo variant (doubled clip),
  via the recovered `s_weapon_info` +0xc8/+0xcc pairing; `AkimboFire` fires full-auto-feel
  alternating the muzzle side (±5u `v_right`).
  **SLICE limits:** akimbo is a single active clip (not a true left/right barrel split) with
  no IN_ATTACK2 "fire both barrels at once"; no weapon/ammo-groups, fire-mode/burst, or buy
  menu; per-slot reserve is generic (not wired to CTSGroundAmmo's calibers); pellet count
  fixed 6 + mossberg/sawedoff fire-type data gap.
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
Map leaves done; the **core monolith** (`CTSGameRules ↔ CBasePlayer ↔ CTSGun ↔
powerups`) is up to a functional-minimal **slice** — weapons deploy+fire, ground
weapons hand out the right gun, powerups spawn + apply + **activate/expire**. To
deepen toward faithful behaviour, in rough order:
1. **The visible slow-motion time-dilation** (the slow-motion *state* now works —
   `GoSlow`/`InSlow`/`GlobalSlowMotion2`/`CTSSlowMotion` are in + verified — but
   nothing yet *reads* the slow fields to actually slow time). What's left, all
   client-entangled: (a) apply the per-player time-dilation in movement — the client
   `pm_shared`/prediction scales frametime by `m_flSlowMotionGo` (needs the **cl_dll**,
   not reconstructed); (b) promote `GoSlow`/`InSlow` to **CBaseEntity** virtuals so
   `GlobalSlowMotion2` can also slow `grenade`/`WorldGun` projectiles; (c) reconstruct
   `SlowMotionFlow` (@0x86bb0, 1758 bytes — the powerup→`GoSlow` propagation between
   nearby players via traceline; this is what makes an *activated slowmo powerup*
   actually call `GoSlow`, vs just setting `m_flSlowMotionScale`) + `InterpolateSlowMotion`
   (@0x8910c — ramp toward target by `min(rate*7,1)` when `|delta|>0.05`); (d) the
   `cl_dll` motion-blur HUD for `gmsgTSMotionBlur`/`gmsgTSPowerUpTime` (already
   registered + sent server-side). Also verify `CTSSlowMotion`'s inferred Spawn
   trigger-setup + `m_flRadius` against the real engine behaviour.
2. **Deepen the weapon arsenal** — fire-type dispatch (cont.16) + the multi-weapon
   arsenal (cont.18, `m_arsenalType/Clip/Reserve[]` + active slot, the modern stand-in
   for `items[]`/`m_pActiveItem`) are done; **all PrimaryAttack fire-types now handled**
   (semi/auto/pump/melee + grenade cont.20 + akimbo cont.21). What's left of the faithful
   engine: **fire-mode/burst** (`SwitchFireMode`, the per-weapon status byte at
   `m_pActiveItem+2`: 0 full-auto / 1 semi / 2 burst / 3 / 4 — drives the per-shot rate),
   the **buy menu** (`CBasePlayer::BuyWeapon`/`BuyArmament` + the `buyable` ground weapons +
   dead-state kung-fu loadout), and wiring the **per-caliber ammo** (CTSGroundAmmo's mags →
   the arsenal reserve by caliber, via `CTSGun::AddAmmo`). (Done cont.18/19: the arsenal,
   `MultiplyClips` inf-clip powerup, `DoubleFireRate` halving `flFireRate`.) Data gaps:
   mossberg/sawedoff fire-type extracted as 0 (are shotguns); shotgun pellet count
   hard-coded 6 (find the real field).
   - **Grenade-throw (fire-type 2)** — DONE (cont.20): `CM61Grenade` on the SDK `CGrenade`.
   - **Akimbo (fire-types 4/5)** — DONE (cont.21): the `s_weapon_info` +0xc8/+0xcc pairing
     (single↔akimbo) + `AddWeapon` in-place upgrade-on-2nd-pickup + `CTSGun::AkimboFire`
     (alternating muzzle). SLICE: single active clip, no IN_ATTACK2 both-barrel fire. The
     remaining fire-type depth is **fire-mode/burst** (`SwitchFireMode`).
3. **`CTSSharedEmitter`** for `ts_emitter`/`ts_particle` (networked particle system;
   also needs a cl_dll side). Niche cleanup: `CTSGameRules::PutPowerUp` (@0x9797c)
   to dynamically spawn powerups at `info_player_deathmatch` (wire to a rules Think).

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
