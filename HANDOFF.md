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
  (`ts_wyndham` pickups), `CTSPowerUp` (22 on `ts_theatre`, bots pick up). Built +
  factory-only (no stock map hits them): `CTSFog`, `CTSTimer`, `CTSCustomTarget`.
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
- **Weapons — runtime-verified, all 38 types + fire-type dispatch (slice):** `CTSGun`
  (`ts_gun.cpp`, entity `weapon_tsgun`, id 16) parameterized by `m_weaponType`.
  `PrimaryAttack` now dispatches by **fire-type** with the recovered per-weapon
  rate/spread: **semi** (one shot/press), **full-auto** (fires while held), **pump
  shotgun** (6 pellets/shot), **melee** (knife 48u / katana 64u hull-trace via
  `MeleeAttack`, no bullets). Full `s_weapon_info` table in `ts_weapons.{h,cpp}` (now
  incl. `flFireRate`/`flSpread`/`flRecoil`), generated by `scripts/extract_weapons.py`.
  `GiveTSWeapon(player,type)`; roster precached at map load via `CWorld::Precache`.
  **SLICE limits:** grenade(2)/akimbo(4/5) fall back to bullets (need throw entity /
  dual-wield); pellet count fixed at 6 (// VERIFY); mossberg/sawedoff fire-type
  extracted as 0 (data gap); shared id 16 ⇒ one TS weapon at a time; no faithful
  dual-wield items-array (`m_pActiveItem`/`m_pActiveInfo`) / `PickUpWeapon` / buy menu.
- **Draft, not in build:** `CTSSlowMotion` (`dlls/ts_slowmotion.cpp`) — needs
  `CTSGameRules::GlobalSlowMotion2` + TS player slow-motion virtuals; still int-
  return sigs (reconcile to modern `bool`).
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
2. **Faithful weapon items-array** — the remaining `// SLICE` in `ts_gun.cpp`.
   Fire-type dispatch is now done (semi/auto/shotgun/melee, cont.16); what's left is
   the **dual-wield items-array** (`m_pActiveItem`@0xcc / `m_pActiveInfo`@0xd0 /
   `items[]`@0x45 + the `s_weapon_status` per-weapon struct holding fire-mode@+2 /
   burst@+8 — the faithful fire funcs dispatch on these, not on a single `m_weaponType`).
   That unblocks: akimbo (4/5) + grenade-throw (2) fire-types, burst/fire-mode
   (`SwitchFireMode`), `CBasePlayer::PickUpWeapon`/`BuyWeapon` armament + the buy menu
   (`buyable` ground weapons + dead-state loadout = kung-fu), and un-SLICEs
   `PlayerAddPowerUp`'s inf-clip (`CTSGun::MultiplyClips` @0x71308, reads the items-
   array) + grenade branches. Note: `DoubleFireRate()` already returns true while a
   dblfire powerup is active — wire it into the fire-rate once the path is faithful.
   Also: mossberg/sawedoff fire-type is a data gap (extracted as 0; they're shotguns),
   and the shotgun pellet count is hard-coded 6 (find the real field).
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
