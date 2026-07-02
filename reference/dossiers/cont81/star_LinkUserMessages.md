# ★ LinkUserMessages — the COMPLETE registration set (gmsg unit U0 ground truth)

Source: typed `re` decompile of `LinkUserMessages__Fv` @ nm **0xc7db0** (Ghidra 0xd7db0;
function body runs to ~0xc8452, next symbol `player` @0xc8458 — the "size=158" header in the
decomp dump is bogus). Full decompile: `star_decomp.txt` lines 1–210. UNK string literals
resolved by raw file read at (Ghidra addr − 0x10000).

## Guard

```c
if (gmsgSelAmmo == 0) { ...all 64 registrations... }
```
**The already-linked guard variable is `gmsgSelAmmo`** (the FIRST registration), exactly as the
dossier planned for Step 0 (our current guard on gmsgCurWeapon must swap).

## The complete registration order (all 64, verbatim, with sizes)

| # | wire name | size | gmsg global | | # | wire name | size | gmsg global |
|---|---|---|---|---|---|---|---|---|
| 1 | `SelAmmo` | 4 | gmsgSelAmmo | | 33 | `VGUIMenu` | 1 | gmsgVGUIMenu |
| 2 | `CurWeapon` | 3 | gmsgCurWeapon | | 34 | `WeaponInfo` | 6 | gmsgWeaponInfo |
| 3 | `Geiger` | 1 | gmsgGeigerRange | | 35 | `ClipInfo` | 1 | gmsgClipInfo |
| 4 | `Health` | 1 | gmsgHealth | | 36 | `WStatus` | 3 | gmsgTSWeaponStatus |
| 5 | `Damage` | 12 | gmsgDamage | | 37 | `ActItems` | 2 | gmsgTSActiveItems |
| 6 | `Battery` | 2 | gmsgBattery | | 38 | `KFuPower` | 1 | gmsgKFuPower |
| 7 | `Train` | 1 | gmsgTrain | | 39 | `TSHealth` | 1 | gmsgTSHealth |
| 8 | `HudText` | -1 | gmsgHudText | | 40 | `TSArmor` | 1 | gmsgTSArmor |
| 9 | `SayText` | -1 | gmsgSayText | | 41 | `TSState` | 1 | gmsgTSState |
| 10 | `TextMsg` | -1 | gmsgTextMsg | | 42 | `TSCash` | 2 | gmsgTSCash |
| 11 | `WeaponList` | -1 | gmsgWeaponList | | 43 | `TSSpace` | 1 | gmsgTSSpace |
| 12 | `ResetHUD` | 1 | gmsgResetHUD | | 44 | `NextStunt` | 1 | gmsgTSNextStunt |
| 13 | `InitHUD` | 0 | gmsgInitHUD | | 45 | `PwUp` | 3 | gmsgTSPowerUp |
| 14 | `GameTitle` | 1 | gmsgShowGameTitle | | 46 | `PwUpTime` | 1 | gmsgTSPowerUpTime |
| 15 | `DeathMsg` | -1 | gmsgDeathMsg | | 47 | `RoundTime` | 2 | gmsgTSRoundTime |
| 16 | `ScoreInfo` | 9 | gmsgScoreInfo | | 48 | `Objective` | -1 | gmsgTSObjective |
| 17 | `TeamInfo` | -1 | gmsgTeamInfo | | 49 | `ShowObj` | 2 | gmsgTSShowObjective |
| 18 | `TeamScore` | -1 | gmsgTeamScore | | 50 | `Wires` | 3 | gmsgTSWires |
| 19 | `GameMode` | 1 | gmsgGameMode | | 51 | `SrvSett` | 3 | gmsgTSServerSetting |
| 20 | `MOTD` | -1 | gmsgMOTD | | 52 | `TSPAward` | 2 | gmsgTSPlayerAward |
| 21 | `ServerName` | -1 | gmsgServerName | | 53 | `TSBTime` | 1 | gmsgTSBulletTime |
| 22 | `AmmoPickup` | 2 | gmsgAmmoPickup | | 54 | `TSHVoice` | 1 | gmsgTSVoiceMessage |
| 23 | `WeapPickup` | 1 | gmsgWeapPickup | | 55 | **`TSFlag`** | **4** | **gmsgTSHaveFlag** |
| 24 | `ItemPickup` | -1 | gmsgItemPickup | | 56 | `CustomWP` | -1 | gmsgTSCustomWeaponParams |
| 25 | `HideWeapon` | 1 | gmsgHideWeapon | | 57 | `Spectator` | 2 | gmsgSpectator |
| 26 | `SetFOV` | 1 | gmsgSetFOV | | 58 | `TSMBlur` | 2 | gmsgTSMotionBlur |
| 27 | `ShowMenu` | -1 | gmsgShowMenu | | 59 | `TSMessage` | -1 | gmsgTSMessage |
| 28 | `ScreenShake` | 6 | gmsgShake | | 60 | `TSSlowMo` | 2 | gmsgTSSlowAmt |
| 29 | `ScreenFade` | 10 | gmsgFade | | 61 | `PTakeDam` | 13 | gmsgTSTakeDamage |
| 30 | `TSFade` | -1 | gmsgTSFade | | 62 | `TSFrags` | -1 | gmsgTSCreateDebris |
| 31 | `AmmoX` | 2 | gmsgAmmoX | | 63 | `PlInfo` | 3 | gmsgTSPlayerInfo |
| 32 | `TeamNames` | -1 | gmsgTeamNames | | 64 | `TSPSys` | -1 | gmsgTSParticleSystem |

(UNK literals verified byte-for-byte from .rodata: HudText@0xfe3cb, TextMsg@0xfe3db, MOTD@0xfe430,
SetFOV@0xfe46c, TSFade@0xfe493, AmmoX@0xfe49a, PwUp@0xfe513, ShowObj@0xfe52b, SrvSett@0xfe539,
TSMBlur@0xfe564, TSPSys@0xfe597.)

## Verdicts on the dossier's open items

- **`TSFlag`(4) → `gmsgTSHaveFlag` CONFIRMED** at the registration site (decomp line 188-189:
  `pfnRegUserMsg("TSFlag",4)` stored to `PTR_gmsgTSHaveFlag_00177d30`). **Our ts_flag.cpp
  registering `"TSHaveFlag"` is a real swap-contract BUG — the wire name is `TSFlag`.**
- **`SetCurWeap` does NOT appear** — declared-only dead global, exact parity already (dossier right).
- **No `StatusText`, `StatusValue`, `Weapons`, `Flashlight`, `FlashBat`** anywhere in the function —
  removal list binary-proven at the registration site (dossier right).
- **`Spectator`(2) IS registered** (deduped literal question closed) and **`Objective`(-1)** too.
- Guard = `gmsgSelAmmo == 0` (dossier Step-0 plan correct).
- Note stock entries the dossier's .rodata run missed by dedup: `Health` 1, `Damage` 12, `Battery` 2,
  `Train` 1, `InitHUD` 0, `Objective` -1, `Spectator` 2 — all registered (order above is complete).
- `RoundTime`(2) confirms the already-landed TSRoundTime→RoundTime rename.
