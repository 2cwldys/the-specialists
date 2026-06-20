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
| `CTSTimer` | `dlls/ts_timer.cpp` | ✅ built into hl.so, `ts_timer` factory exported (runtime untested) |
| `TSHudMessageAll` (helper) | `dlls/ts_utils.cpp` | ✅ built (shared HUD broadcast) |
| `CTSSlowMotion` | `dlls/ts_slowmotion.cpp` | 📝 draft, NOT in build — deps: `CTSGameRules::GlobalSlowMotion2`, TS player slow-motion virtuals |

Everything else in the table above is **not started**. Full history in the
Serena ledger (`.serena/memories/ledger.md`).

## Notes

- Work biggest-first within a subsystem: `CTSGameRules` (game mode) and
  `CTSGun`/weapon hierarchy are the core; the per-weapon `CTS*Weapon`/`CTS*Ammo`
  leaf classes are small and regular once the base `CTSGun` is recovered.
- Non-`CTS` TS-specific classes worth noting: `CBulletManager`, `CM61Grenade`,
  `CVipEscape`, `CInfoBreakable`.
- For each class follow the per-class workflow in CLAUDE.md §2.
