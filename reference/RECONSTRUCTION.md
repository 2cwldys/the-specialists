# Reconstruction work-list

Generated from `reference/` via symbol/source diff (see CLAUDE.md §2).
Regenerate the underlying data with `reference/dump-signatures.sh`.

## Scope

| Bucket | Count | Action |
|---|---|---|
| Stock HL SDK classes (in this repo's source) | 437 | **Reuse** source — do not RE |
| Classes present in `ts_i386.so` | 216 | — |
| **TS-specific classes** (in binary, not in SDK) | **60** | **Reverse-engineer** (below) |
| TS-specific functions (approx) | ~386 | — |
| TS-specific classes that are FGD entity-backed | 23 | keyvalues/spawnflags recoverable from `ts_fgd.fgd` |

## The 60 TS-specific classes

`funcs` = functions attributed to the class in the `.so` symtab (rough effort
proxy). `FGD` = has a `ts_*` entity in `ts_fgd.fgd` (member layout + spawnflags
recoverable directly).

| Class | funcs | FGD |
|---|---:|:---:|
| `CTSGun` | 44 |  |
| `CTSGameRules` | 41 |  |
| `CTSStunt` | 25 |  |
| `CTSAnimation` | 14 |  |
| `CTSFlag` | 13 |  |
| `CTSSharedEmitter` | 12 |  |
| `CTSGroundWeapon` | 12 | ✓ |
| `CTSWorldGun` | 11 |  |
| `CTSPowerUp` | 11 | ✓ |
| `CTSSlowMotion` | 10 | ✓ |
| `CTSBomb` | 10 | ✓ |
| `CTSGroundAmmo` | 9 | ✓ |
| `CTSTeamEscape` | 8 | ✓ |
| `CTSKnife` | 8 |  |
| `CTSHack` | 8 | ✓ |
| `CTSMapGlobals` | 7 | ✓ |
| `CTSBriefcase` | 7 | ✓ |
| `CTSAmmoPack` | 7 |  |
| `CBulletManager` | 7 |  |
| `CTSWinGiver` | 6 | ✓ |
| `CTSObjective` | 6 |  |
| `CTSDMHill` | 6 | ✓ |
| `CTSTimer` | 5 | ✓ |
| `CTSSlowMotionPoint` | 5 |  |
| `CTSShrapnel` | 5 |  |
| `CTSPhysicObject` | 5 |  |
| `CTSObjectiveManager` | 5 | ✓ |
| `CTSCustomTarget` | 5 | ✓ |
| `CM61Grenade` | 5 |  |
| `CTSTrigger` | 4 | ✓ |
| `CTSPlayerAmount` | 4 | ✓ |
| `CTSEmitter` | 4 | ✓ |
| `CTSDeadCorpse` | 4 |  |
| `CVipEscape` | 3 |  |
| `CTSWind` | 3 | ✓ |
| `CTSShotgunWeapon` | 3 |  |
| `CTSSellZone` | 3 | ✓ |
| `CTSScoreGiver` | 3 | ✓ |
| `CTSSMGWeapon` | 3 |  |
| `CTSRifleWeapon` | 3 |  |
| `CTSPistolWeapon` | 3 |  |
| `CTSParticle` | 3 | ✓ |
| `CTSObjectivePointer` | 3 |  |
| `CTSKnifeWeapon` | 3 |  |
| `CTSBigPistolWeapon` | 3 |  |
| `CTSModel` | 2 | ✓ |
| `CTSGrenade` | 2 |  |
| `CTSSuperJump` | 1 |  |
| `CTSSniperWeapon` | 1 |  |
| `CTSSniperAmmo` | 1 |  |
| `CTSShotgunAmmo` | 1 |  |
| `CTSSMGAmmo` | 1 |  |
| `CTSRifleAmmo` | 1 |  |
| `CTSPistolAmmo` | 1 |  |
| `CTSMGWeapon` | 1 |  |
| `CTSKatanaWeapon` | 1 |  |
| `CTSGrenadeWeapon` | 1 |  |
| `CTSFog` | 1 | ✓ |
| `CTSBigPistolAmmo` | 1 |  |
| `CInfoBreakable` | 1 |  |

## Status (live — update as classes land)

| Class | File | State |
|---|---|---|
| `CTSModel` | `dlls/ts_model.cpp` | ✅✅ **runtime-verified** — 108 `ts_model` spawned in-engine on `ts_kissmeyerbasic`, no crash (cycler-style animated studio prop) |
| `CTSMapGlobals` | `dlls/ts_mapglobals.cpp` | ✅✅ **runtime-verified** — spawned on `ts_trinityfall`; weapon-restriction `0x1f ^ spawnflags` computed correctly in-engine (Save/Restore deferred — MP has no level transitions) |
| `CTSFog` | `dlls/ts_fog.cpp` | ✅ built into hl.so, `ts_fog` factory exported (self-removing stub; not exercised by a map yet) |
| `CTSTimer` | `dlls/ts_timer.cpp` | ✅ built into hl.so, `ts_timer` factory exported (no stock map uses it → runtime untested) |
| `CTSCustomTarget` | `dlls/ts_customtarget.cpp` | ✅ built into hl.so, `ts_customtarget` factory exported (no stock map uses it → runtime untested) |
| `TSHudMessageAll`/`TSHudMessageOne` (helpers) | `dlls/ts_utils.cpp` | ✅ built (HUD broadcast / per-player) |
| CBasePlayer TS extensions | `dlls/player.h`, `dlls/ts_player.{h,cpp}` | 🟡 member block (`m_iPowerups`/`m_iKungFu`/…) + accessors (`SlowMotion`/`InfiniteClips`/`SlowPause`/`DoubleFireRate`/`KungFu`) built; no regression (server runs). Powerup application (`PlayerAddPowerUp`) deferred — needs `CTSGun`. |
| Weapon data layer | `dlls/ts_weapons.{h,cpp}` | ✅ full 38-weapon `s_weapon_info` table (models/anim-ext/fire/clip/damage/fire-type **+ rate 0x18 / spread 0x28 / recoil 0x34**), extracted by `scripts/extract_weapons.py`; `tsweapon_t` enum (real indices = FGD ids) + `tsfiretype_t`; `get_weapon_info` + `PrecacheAllTSWeapons` + `GiveTSWeapon`. |
| `CTSGun` | `dlls/ts_gun.cpp` | ✅✅ **runtime-verified — arsenal + fire-type dispatch** — the one `weapon_tsgun` (id 16) now holds a multi-weapon **arsenal** (`m_arsenalType/Clip/Reserve[]` + active slot), a clean modern stand-in for the original `items[]`/`m_pActiveItem`. `AddWeapon`/`SelectSlot`/`FindSlot`; `GiveTSWeapon` adds to the existing gun; `tsnextweapon` command cycles; per-slot clip/reserve persist across switches. `PrimaryAttack` dispatches by fire-type (semi one/press, full-auto held, pump 6-pellet, melee knife 48u/katana 64u hull-trace); `Reload` from the slot reserve. **Powerup hooks now real (cont.18):** `MultiplyClips` (inf-clip powerup doubles the active slot's reserve) + `DoubleFireRate` (dblfire powerup halves `flFireRate`). Verified on `ts_wyndham`: 3-weapon arsenal cycles, per-slot clip persists; inf-clip reserve 45→90, dblfire rate 0.100→0.050, no crash. **SLICE:** no weapon/ammo-groups or buy menu; **grenade fire-type (2) throws an M61 ([[CM61Grenade]], cont.20)**; **akimbo fire-types (4/5) dual-wield (cont.21): 2nd-of-a-single upgrades the slot to its akimbo variant (s_weapon_info +0xc8/+0xcc pairing), `AkimboFire` alternates the muzzle side**; **fire-mode system (cont.22): per-slot auto/semi/burst/pump cycled by `SwitchFireMode` (`tsfiremode`) over the +0xc mode mask — burst = 3 rounds (2 for MAC-10) driven by WeaponIdle, defaults per TakeWeapon** — single active clip, no IN_ATTACK2 both-barrel fire, no client fire-mode HUD; **per-caliber ammo (cont.23): `m_ammoReserve[16]` pooled by the weapon's `s_weapon_info+0x8` caliber (shared across same-caliber weapons), `AddAmmo`/Reload draw rounds capped at the `compile_ammo_info` max table** — start reserve = clip*3 (buy menu sets the real loadout); **shotguns now fire the real 8 pellets (iPellets = s_weapon_info+0x24) + mossberg/sawedoff fire-types fixed (cont.24)**. |
| `CM61Grenade` | `dlls/ts_grenade.cpp` | ✅✅ **runtime-verified** — the thrown M61 grenade. Functional reconstruction on the SDK `CGrenade` (the faithful `CM61Grenade : CTSGrenade : CTSPhysicObject` base classes aren't built): `Throw()` tosses an `w_m61.mdl` grenade (MOVETYPE_BOUNCE + `CGrenade::BounceTouch`/`TumbleThink`→`Detonate` after a 4s fuse). `CTSGun::PrimaryAttack` dispatches the grenade fire-type (2) → `GrenadeThrow` (one per press, 800u throw); also drives the grenade powerup (gives the M61). Verified on `ts_wyndham`: 100 grenades thrown + detonated over 30s, no crash. **Detonation now throws a `CTSShrapnel` burst (cont.29): `ts_shrapnel` MOVETYPE_BOUNCE fragments that TraceAttack on contact (14 dmg) — `M61Tumble`/`M61Detonate` replace TumbleThink→Detonate, 10 fragments + the stock blast.** // SLICE: no CTSPhysicObject physics, impact-trace early-detonation, or throw event. |
| `CTSGroundAmmo` | `dlls/ts_groundammo.cpp` | ✅✅ **runtime-verified** — `ts_groundammo` (16 placements) ammo pack: Spawn→Fall→OnGround settle (TOSS, or FLY for "Lie on wall") → persistent SOLID_TRIGGER refill station. KeyValue wduration + mags1–11 (per-caliber). Verified on `ts_supertimor_d3`: 6 settled (Can't init gone), a TS-gun-holding bot touched a pack (14 mags) → topped up 9mm then full (-1), no crash. **Per-caliber refill wired (cont.23):** `WaitTouch` now calls `TSAddAmmo(player, caliber, mags1..11)` into the gun's per-caliber `m_ammoReserve[]` (capped at the caliber max). PutOnWall = settle-in-place stub. |
| `CTSGroundWeapon` | `dlls/ts_groundweapon.cpp` | ✅✅ **runtime-verified** — `ts_groundweapon` (65 placements) spawns (Can't init gone), precaches at map load, touch-pickup works on `ts_wyndham` (bots picked up, no crash). // SLICE gives beretta regardless of type; faithful `PickUpWeapon` armament/buy TODO. |
| `CTSWorldGun` | `dlls/ts_worldgun.cpp` | ✅✅ **runtime-verified (cont.27)** — player-dropped weapon (`ts_worldgun`, runtime-created, not map-placed): falls/settles (FallThink → SOLID_TRIGGER) + USE-pickup (WaitTouch on `IN_USE` → hand the weapon back). `CTSGun::RemoveActiveWeapon` (the arsenal drop) + `TSCreateWorldGun`/`TSPickUpWeapon` + the `dropweapon` command. Verified on `ts_wyndham`: drop uzi (arsenal 2→1) → world gun settles → USE hands it back. **On-death drop wired (cont.28).** SLICE: no CTSPhysicObject bounce; carries type+clip only; no flag auto-pickup / throw arc. |
| `CTSDeadCorpse` + death hook | `dlls/ts_corpse.cpp`, `ts_gamerules.cpp` | ✅✅ **runtime-verified (cont.28)** — `ts_corpse`: a fading copy of the dead player's studio model (model/pose/skin), MOVETYPE_TOSS, `FadeThink` ramps renderamt → UTIL_Remove (15s). `CTSGameRules::PlayerKilled` override drops the dead player's arsenal as CTSWorldGuns + spawns the corpse. Verified: killing a bot (with beretta+uzi) → 1 ts_corpse + 2 ts_worldgun. SLICE: live-pose copy, no death-anim capture / gib. |
| `CTSAmmoPack` | `dlls/ts_ammopack.cpp` | ✅✅ **runtime-verified (cont.30)** — `ts_ammopack`: a CONSUMABLE dropped ammo pack carrying `m_ammo[16]` per-caliber; MOVETYPE_TOSS → settle → SOLID_TRIGGER; WaitTouch gives each caliber via `CTSGun::AddAmmo` + zeroes it, removes when empty. Dropped on death (`TSDropAmmoOnDeath` in PlayerKilled) carrying the dead player's `m_ammoReserve`. Verified: kill beretta-bot → pack carries 9mm=45 → live holder takes it (45→0) → pack removed. SLICE: no CTSPhysicObject bounce, one pack for the whole reserve. |
| `CTSShrapnel` | `dlls/ts_grenade.cpp` | ✅✅ **runtime-verified (cont.29)** — `ts_shrapnel`: grenade-blast fragments (MOVETYPE_BOUNCE, TraceAttack 14 dmg on contact / ≤2 bounces / 1.5s). `CM61Grenade::M61Detonate` spawns 10 + the stock blast. SLICE: no CTSPhysicObject, fixed count/dmg. |
| `CTSDMHill` | `dlls/ts_dmhill.cpp` | ✅ **built + synthetic-verified (cont.32)** — `ts_dmhill` king-of-the-hill brush: hold the volume ALONE `nextcheck`s → `fragamt`/`fragamtteam` frags + scoreboard + "<name> <message>". HillThink polls `Intersects` (sole=award, 0/2+=reset). 0 stock placements → synthetic test (sole bot awarded 0→5); needs a hand-made map for in-map runtime. |
| `CTSPowerUp` | `dlls/ts_powerup.cpp` | ✅✅ **runtime-verified** — `ts_powerup` (328 placements) spawns (Can't init gone); 22 spawned on `ts_theatre` with all 8 types randomly rolled from spawnflags, settle (TOSS→FL_ONGROUND→OnGround→SOLID_TRIGGER), touch-pickup applies the powerup via `PlayerAddPowerUp` + respawns; bots picked up slowpause/dblfire (timed→held), grenade (→M61), health (→150hp), no crash. **inf-clip + double-firerate now wired to the weapon (cont.18: `MultiplyClips`/`DoubleFireRate`).** // SLICE: armor/grenade use functional substitutes (no TakeArmor/PickUpWeapon yet); gmsgTSPowerUp HUD notify + lowgrav client physics-key deferred (client.dll side). |
| Powerup activation state machine | `dlls/ts_player.cpp`, `player.h`, `client.cpp` | ✅✅ **runtime-verified** — `CBasePlayer::UsePowerUp`/`UpdatePowerUpState`/`DeactivateSlowMotionFlow` + the `usepowerup` console command + a `PreThink` hook. A held timed powerup activates (`m_iPowerups` set, `SlowMotion()`/`DoubleFireRate()` accessors flip, slow-motion fields `m_flSlowMotionScale`/`Ramp`/`m_bSlowBullets` set), runs its duration, expires + restores. Verified on `ts_theatre`: slow-motion held→activate→`scale=0.6` for 3s→expire→cleared, `gmsgTSPowerUpTime`/`gmsgTSMotionBlur` registered + sent, no crash. **SLICE/deferred:** the time-dilation *movement*, `SlowMotionFlow` propagation to nearby players/projectiles (@0x86bb0), the per-entity `SetSlowMotion` virtual, client motion-blur, and the on-death powerup re-drop. |
| Cash + buy/armament | `dlls/ts_buy.cpp`, `dlls/player.h`, `dlls/client.cpp` | ✅✅ **runtime-verified (cont.36)** — the TS buy system. **`CashBased()` is hardcoded OFF in the .so**, so weapons/ammo are **FREE**; the real mechanic is a free armament (loadout) selector. `GiveCash` (cash @+0x540, clamped 0..30000), `ResetArmament`/`AddItemToArmament`/`AddAmmoToArmament`/`BuyWeapon`/`BuyAmmo`/`BuyArmament` (reuse `GiveTSWeapon`/`TSAddAmmo`). `BuyWeapon` gates the M61 (team-mode only, cap 2/life), else free. Console drivers `tsbuy`/`tsarm`/`tsbuyarmament`/… (menu UI is cl_dll, SLICE). **Verified:** tm_avana armament → arsenal {beretta,uzi}+9mm cap; M61 ×3 → 1,1,0; DM blocks M61 (0,0,0). SLICE: weapon prices/cash-charging dead (faithful), category restriction not wired (default allows all), no cl_dll menu, no starting/win cash. |
| `CTSGameRules` | `dlls/ts_gamerules.{h,cpp}`, `dlls/ts_teams.cpp` | 🟡 **skeleton + team assignment, runtime-verified** — `CTSGameRules : CHalfLifeMultiplay` is the live `g_pGameRules` (via `InstallGameRules`); `GetRandomPowerup` done; `TSGameRules()` accessor; **`Think` override + `PutPowerUp` (cont.26) — dynamic DM powerup spawning every ~20s at `info_player_deathmatch`**; `GlobalSlowMotion2`/`SetGlobalSlowMotion` (cont.15); `TeamWin` + round-restart (cont.31); **`CheckDeads`/`CallCheckDeads` (cont.37) — team-round elimination: a death resolves the round, the surviving team wins via TeamWin (nobody = draw), verified on tm_avana (kill a team → other team scores + round restarts).** **Team subsystem (cont.35, `ts_teams.cpp`):** `IsTeamMode`/`IsTeamplay` (proxy: a `ts_objective_manager` present), `InitTeams`/`RecountTeams`/`GetTeamIndex`/`GetIndexedTeamName`/`GetTeamID`/`TeamWithFewestPlayers`, `AssignToTeam` (sets `pev->team` 0-based + infokeys), `PlayerSpawn` override = **auto-balance** any unassigned player onto the fewest team, `InitHUD` sends the roster. **Verified:** 5 bots balanced 3/2 (Specialists/Mercenaries) on `tm_avana`; DM (`ts_wyndham`) assigns nothing. SLICE: faithful trigger is the client class menu (cl_dll); cash economy + buy menu + `UpdateTeamScores`/full `RestartRound`/friendly-fire still inherit MP. |
| `CTSSlowMotion` | `dlls/ts_slowmotion.cpp` | ✅✅ **runtime-verified** — `ts_slowmotion` brush zone (11 placements) spawns (Can't init gone; 4 on `ts_wyndham`), `Use()` activates without crash; touch-zone slows players via `CBasePlayer::GoSlow` (tracked in `m_hPlayers[]`, dropped on leave by `CheckPlayersDistance`), "All map" mode drives `CTSGameRules::GlobalSlowMotion2`. Verified `GlobalSlowMotion2(0.5)` → player `InSlow()=1, go=0.50`; restore → `InSlow()=0, go=1.0`. **SLICE:** the time-dilation *movement* is client-side (GoSlow only sets server fields); GlobalSlowMotion2 slows players only (projectile slow deferred — no CBaseEntity GoSlow virtual); `m_flRadius`/Spawn trigger-setup inferred (// VERIFY). |
| `CBasePlayer::GoSlow`/`InSlow` + `CTSGameRules::GlobalSlowMotion2`/`SetGlobalSlowMotion` | `dlls/ts_player.cpp`, `ts_gamerules.cpp` | ✅✅ **runtime-verified** — the slow-motion application API (GoSlow sets `m_flSlowMotionGo`/`Target`+`m_bSlowBullets`; InSlow reports it; GlobalSlowMotion2 applies/clears to all players, throttled ~0.5s). |
| `CTSSlowMotionPoint` | `dlls/ts_slowmotionpoint.cpp` | ✅✅ **runtime-verified (cont.33, synthetic)** — the slow-motion "bubble": a runtime point that `GoSlow`s every living non-owner player with line of sight for a duration, then restores them (WaitThink LOS poll + DeactivateSlow). `CBasePlayer::UsePowerUp` spawns one when a slow-motion/slow-pause powerup activates — un-SLICEs the cont.14 powerup→GoSlow (SlowMotionFlow) deferral. Verified: bubble → non-owner InSlow=1 go=0.60, owner skipped, restored at expiry. SLICE: visible time-dilation movement is still cl_dll. |
| `CTSObjective` / `CTSObjectiveManager` / `CTSObjectivePointer` / `CTSHack` / `CTSWinGiver` / `CTSTrigger` | `dlls/ts_objectives.cpp` | ✅✅ **runtime-verified (cont.25, +CTSTrigger cont.34: ts_trigger brush — fire target+message on a valid team-gated touch, recheck delay; synthetic-verified)** — — the team-mode objective entities (`ts_hack`/`ts_objective_manager`/`ts_objective_ptr`/`ts_wingiver`). All spawn on `tm_avana` (0 Can't init, was 7) + `tm_corporate`; keyvalues parse; CTSHack counts `hackingtime` down to completion (HackTouch+WaitThink → fire target+HUD); CTSWinGiver Use'd `callsnumber`× declares `winnerteam` → **`CTSGameRules::TeamWin` (cont.31): scores the team + announces + restarts the round (respawn all + re-arm objectives) after 3s**. **SLICE:** TeamWin cash + the round-time HUD + team ASSIGNMENT (per-player teams) + `ShowObjectiveOnHud` + the client objective HUD pointer are deferred. |

| `CTSFlag` | `dlls/ts_flag.cpp` | ✅✅ **runtime-verified (cont.38, synthetic)** — the CTF flag (`ts_flag`, briefcase.mdl, runtime-created), unblocked by team assignment. A team-owned 3-state flag (AT_BASE/CARRIED/DROPPED): an enemy/neutral player touches → carries it (follows them, drops if they die); a carrier touching their own flag at base → capture (return + score via TeamWin); a killed carrier drops it (PlayerKilled `TSDropFlag`), auto-returns after 30s. `tsspawnflag <team>` console-seeds bases (stand-in for the unbuilt CTSBriefcase). Verified on tm_avana: pickup→capture(score 0→1)→drop. SLICE: no carry-slow/CTSPhysicObject, capture scores via TeamWin (also restarts round), console-seeded, no flag HUD (cl_dll). |

| `CTSBriefcase` | `dlls/ts_briefcase.cpp` | ✅✅ **runtime-verified (cont.39, real map)** — the CTF base + capture zone (`ts_briefcase`, brush, FGD ownerteam 1..4 → 0-based). Seeds its team's CTSFlag at base (FlagFall) + captures a flag-carrier who reaches their own briefcase (CaptureTouch → return + score). Pairs with cont.38 CTSFlag; replaces its `tsspawnflag` console stand-in. **Verified on `ctb_deadheat` (a real stock CTB map):** 2 briefcases each seed their flag at base (team 0 + team 1), 0 Can't init, no crash. SLICE: gmsgTSHaveFlag HUD server-only, capture scores via TeamWin (restarts round), no Save/Restore. |

| `CTSBomb` | `dlls/ts_bomb.cpp` | ✅✅ **built + synthetic-verified (cont.40)** — the defuse/wire-cut objective (`ts_bomb`, brush, FGD defusetime/cutwires). A planted bomb counts down (WaitThink, rising beep) + explodes (ExplosionCreate) unless defused: hold USE for `defusetime` (FCAP_CONTINUOUS_USE) or cut the right of 3 wires (SetRandomWires: 1 defuse / 2 boobytrap→fast / 3 neutral→detonate); defuse fires target+message. `tscutwire <n>` console-drives the wires (UI is cl_dll). 0 stock placements → synthetic: cut-defuse→safe, cut-neutral→detonate, cut-boobytrap→fast, no crash. SLICE: fixed 45s countdown, arms on Spawn (faithful enable is gamerules), no Save/Restore. |

| `CTSSellZone` | `dlls/ts_sellzone.cpp` | ✅✅ **runtime-verified (cont.41, real map)** — the weapon sell zone (`ts_sellzone`, brush, base tsobjective). A dropped weapon (ts_worldgun) touching it is sold: the nearest player gets cash (cont.36 `GiveCash`) + the gun is consumed. The .so's SellTouch is a ~61-byte near-stub (cash is vestigial — CashBased off). **Verified on `ts_dust2`:** zone spawns (0 Can't init), a dropped beretta → player cash 0→500 + gun removed. Also fixed a pre-existing crash: the dynamic powerup spawner runtime-precached `models/powerup.mdl` → moved to map-load precache. SLICE: flat $500 (no per-weapon price), nearest-player credit (no recorded seller). |

| `CTSTeamEscape` | `dlls/ts_escape.cpp` | ✅✅ **built + synthetic-verified (cont.42)** — the team escort/escape zone (`ts_teamescape`, brush, base tsobjective). Completes (fire target + HUD) when `escapepercentage` of the attacker team is simultaneously inside (0 one / 1 third / 2 two-thirds / 3 all); WaitTouch counts distinct attacker-team touchers, WaitThink resets the window each second. 0 stock placements → synthetic: size 2, pct=0 one-touch completes, pct=3 needs all. SLICE: arms on Spawn (faithful enable is gamerules), no Save/Restore, fires target only (no score). |

| `CTSScoreGiver` / `CVipEscape` | `dlls/ts_objectives.cpp` | ✅✅ **built + synthetic-verified (cont.43)** — `ts_scoregiver` = CTSWinGiver but REPEATABLE (Use'd `callsnumber`× bumps the team score directly + re-arms, no round restart); `func_vip_safetyzone` = a passive VIP safety-zone brush marker (CVipEscape, no behaviour of its own). Verified: scoregiver callsnumber=2 scores 0→1→2 over repeated calls; vip zone spawns. SLICE: scoregiver "Respawn Players" flag not applied; VIP win-detection is the unbuilt team subsystem's. |

| `CTSPlayerAmount` / `CTSSuperJump` | `dlls/ts_playeramount.cpp`, `dlls/ts_powerup.cpp` | ✅✅ **built + synthetic-verified (cont.44)** — `ts_playeramount` = a point relay that fires connecttarget/disconnecttarget when the player count rises/falls to `clients` (poll-based CountThink); `ts_superjump` = a CTSPowerUp pre-set to the low-gravity/super-jump type. Verified: playeramount(clients=2) fires connecttarget when the count reaches 2; superjump spawns. SLICE: playeramount polls (no engine connect hook) + ignores spawnflags. |

Everything else in the table above is **not started**. Full history in the
Serena ledger (`.serena/memories/ledger.md`).

## Entity frequency in the 60 stock TS maps

How often each `ts_*` entity actually appears across `~/Containers/hlds/mods/ts/maps/*.bsp`
(map-content RE priority — reconstruct high-count ones first so a loaded map
actually exercises them):

| classname | placements | class | status |
|---|---:|---|---|
| `ts_model` | 858 | `CTSModel` | ✅✅ runtime-verified |
| `ts_powerup` | 328 | `CTSPowerUp` | ✅✅ runtime-verified (slice; timed-powerup activation deferred to slow-motion layer) |
| `ts_groundweapon` | 65 | `CTSGroundWeapon` | 🔒 likely blocked (weapon stack) |
| `ts_emitter` | 19 | `CTSEmitter` | 🔒 needs `CTSSharedEmitter` (networked particle system) |
| `ts_groundammo` | 16 | `CTSGroundAmmo` | ✅✅ runtime-verified (refill pack; per-caliber AddAmmo, cont.23: per-caliber TSAddAmmo) |
| `ts_particle` | 13 | `CTSParticle` | 🔒 needs `CTSSharedEmitter` |
| `ts_slowmotion` | 11 | `CTSSlowMotion` | ✅✅ runtime-verified (slow state; client time-dilation deferred) |
| `ts_mapglobals` | 9 | `CTSMapGlobals` | ✅✅ runtime-verified |
| `ts_wingiver` | 6 | `CTSWinGiver` | — |
| `ts_hack` | 5 | `CTSHack` | — |
| `ts_sellzone`/`ts_objective_*`/`ts_briefcase` | 2 each | … | — |
| `ts_wind`/`ts_fog` | 1 each | `CTSWind`/`CTSFog` | `CTSFog` ✅ built |

`ts_timer`/`ts_customtarget` appear in **0** stock maps (niche mapper relays) —
build-verified only; need a hand-made test map to exercise at runtime.

### Dependency wall (2026-06-20): foundation is now the critical path

The cheap self-contained entities are done. Dep-scanning the remaining
high-frequency classes' decompiles shows they all need foundation systems that
don't exist yet:
- `CTSPowerUp` (328) → `CTSGameRules::GetRandomPowerup` + `TSGameRules` global,
  `PlayerAddPowerUp(CBasePlayer*, …)` (TS player code), and a `TSHudMessageOne`
  helper (one-player variant of the built `TSHudMessageAll`).
- `CTSEmitter` (19) / `CTSParticle` (13) → `CTSSharedEmitter` (a networked
  particle/emitter manager: `AddEmitter`/`SV_UpdateClient`/… — needs a user
  message + the client.dll side).
- `CTSSlowMotion` (11) → `CTSGameRules::GlobalSlowMotion2` + TS player virtuals.

So the next real unblock is the **foundation**: `CTSGameRules` (41 funcs) + the
TS player extensions. `InitWeaponRestriction()` = `return 0x1f`;
`WeaponRestrictionByID(int)` is a small switch (in the .so near 0x66ad8) — both
part of the weapon-restriction subsystem the game rules consume.

## In-engine test rig (runtime verification)

Real behaviour testing now works against a local 64-bit ReHLDS (the QUIC build):

```bash
RIG=~/Containers/hlds/hlds-linux64-test          # 64-bit ReHLDS + valve content
# ts/ game dir = symlinks to original content (~/Containers/hlds/mods/ts) +
#   ts/dlls/hl.so = OUR build, + a liblist.gam whose gamedll_linux="dlls/hl.so"
#   (bypasses metamod/mp.dll). Set up once; just recopy hl.so after a rebuild:
cp ~/Projects/goldsrc.net/mod-ts/build-amd64/dlls/hl.so $RIG/ts/dlls/hl.so

cd $RIG && LD_LIBRARY_PATH=.:$LD_LIBRARY_PATH \
  timeout -s INT 30 ./hlds_linux -game ts -condebug +developer 1 \
  +sv_lan 1 +maxplayers 4 +map ts_kissmeyerbasic > /tmp/ts_run.log 2>&1
```
- Use **SIGINT** (`timeout -s INT`), NOT SIGKILL — KILL loses buffered stdout.
- Map-load banners are suppressed; proof signals: `GAME SKILL LEVEL` (DLL+map
  spawned), `Can't init <class>` (engine's marker for an **unimplemented** TS
  class — your negative control), and a temporary `ALERT(at_console,…)` probe in
  a Spawn for positive confirmation (108 hits == 108 `ts_model` on that map).
- `ts_kissmeyerbasic` has 108 `ts_model`; pick maps from the frequency table to
  exercise specific classes.

### Bots (for anything needing a live player — weapons, powerups)

The 64-bit **metamod + rcbot** stack (built by the buildchain) gives real bot
players (raw `pfnCreateFakeClient` doesn't complete the connect on this QUIC
engine). One-time rig setup (already done):
- `ts/addons/metamod/dlls/metamod_amd64.so` (Metamod-R/build-amd64) as the gamedll
  (`liblist.gam` gamedll_linux → it); `config.ini` = `gamedll dlls/hl.so` (metamod
  loads OUR build); `plugins.ini` = `linux addons/metamod/dlls/rcbot_mm_amd64.so`.
- `<rig>/rcbot/*` configs (rcbotold/rcbot); `bot_config.ini` `config min_bots 2`.
Boot normally → rcbot auto-adds bots that spawn as real CBasePlayers. To exercise
a weapon, a temp StartFrame hook can teleport a bot onto a `ts_groundweapon` /
give + `SelectItem`. (Verified: bots deploy+fire the reconstructed weapons.)

### Debugging our hl.so — use gdb, don't guess

Ghidra reads the *original* `.so`; **gdb debugs our built `hl.so`**. On a segfault:
```bash
cd <rig> && LD_LIBRARY_PATH=.:$LD_LIBRARY_PATH gdb -batch -nx \
  -ex run -ex bt --args ./hlds_linux -game ts +sv_lan 1 +maxplayers 8 +map <map>
```
gdb intercepts SIGSEGV before the engine's breakpad handler; the backtrace points
straight at the faulting function (this found the `EV_Precache` + `DeathNotice`
crashes). valgrind / rr are heavier complements.

## Notes

- Work biggest-first within a subsystem: `CTSGameRules` (game mode) and
  `CTSGun`/weapon hierarchy are the core; the per-weapon `CTS*Weapon`/`CTS*Ammo`
  leaf classes are small and regular once the base `CTSGun` is recovered.
- Non-`CTS` TS-specific classes worth noting: `CBulletManager`, `CM61Grenade`,
  `CVipEscape`, `CInfoBreakable`.
- For each class follow the per-class workflow in CLAUDE.md §2.
