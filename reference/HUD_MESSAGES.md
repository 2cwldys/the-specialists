# TS user-message contract (the client↔server protocol)

Recovered from `reference/ts_i386.so` `LinkUserMessages` (names + REG_USER_MSG
sizes) and the senders `CBasePlayer::UpdateClientData @0x81d00` /
`CTSGun::UpdateClientData @0x758c8` / `CTSGameRules` (wire formats). These are the
**swap-compatibility contract** — names + wire bytes must match the original
exactly. `-1` size = variable. Ghidra auto-named the gmsg globals; the **registered
string** is what's on the wire (left column).

## Full registration list (order as in LinkUserMessages)

Stock HL messages first (SelAmmo 4, CurWeapon 3, Geiger 1, Health 1, Damage 12,
Battery 2, Train 1, HudText -1, SayText -1, TextMsg -1, WeaponList -1, ResetHUD 1,
InitHUD 0, GameTitle 1, DeathMsg -1, ScoreInfo 9, TeamInfo -1, TeamScore -1,
GameMode 1, MOTD -1, ServerName -1, AmmoPickup 2, WeapPickup 1, ItemPickup -1,
HideWeapon 1, SetFOV 1, ScreenShake 6, ScreenFade 10, AmmoX 2, TeamNames -1),
then the **TS-specific block**:

| Name (wire) | Size | Ghidra gmsg name | Sent by | Wire format |
|---|---|---|---|---|
| `TSFade`   | -1 | gmsgTSFade | (screen fade) | variable |
| `VGUIMenu` | 1  | gmsgVGUIMenu | menus | byte |
| `WeaponInfo` | 6 | gmsgWeaponInfo | CTSGun::UpdateClientData | BYTE weapontype, BYTE arg2, SHORT reserveAmmo, BYTE arg4, BYTE arg5 |
| `ClipInfo` | 1 | gmsgClipInfo | | byte |
| `WStatus`  | 3 | gmsgTSWeaponStatus | CTSGun::UpdateClientData | BYTE slot, BYTE a, BYTE b (per arsenal slot, loop 0x25..0 step -2) |
| `ActItems` | 2 | gmsgTSActiveItems | CBasePlayer::UpdateClientData | **MSG_ALL** BYTE entindex, BYTE activeItems |
| `KFuPower` | 1 | gmsgKFuPower | | byte |
| `TSHealth` | 1 | gmsgTSHealth | CBasePlayer::UpdateClientData | BYTE round(health), clamp >=0 |
| `TSArmor`  | 1 | gmsgTSArmor | CBasePlayer::UpdateClientData | BYTE round(armorvalue) |
| `TSState`  | 1 | gmsgTSState | CBasePlayer::UpdateClientData | BYTE state (player member [0x126]) |
| `TSCash`   | 2 | gmsgTSCash | CBasePlayer::UpdateClientData | SHORT cash |
| `TSSpace`  | 1 | gmsgTSSpace | CBasePlayer::UpdateClientData | BYTE space (inventory weight/slots) |
| `NextStunt`| 1 | gmsgNextStunt | | byte |
| `PwUp`     | 3 | gmsgTSPowerUp | CBasePlayer::UpdateClientData | SHORT [0x1ca], BYTE abs([0x1cc]) (held powerup id + amount) |
| `PwUpTime` | 1 | gmsgTSPowerUpTime | CBasePlayer (powerup activate) | byte (was MIS-NAMED "TSPowerUpTime" in our server) |
| `RoundTime`| 2 | gmsgTSRoundTime | CTSGameRules | **BYTE secondsRemaining, BYTE flag** (flag 0=start/restart, 2=timeout). NOT two shorts! |
| `Objective`| -1 | gmsgObjective | | variable |
| `ShowObj`  | 2 | gmsgShowObj | | short/2-byte |
| `Wires`    | 3 | gmsgTSWires | CBasePlayer::UpdateClientData | BYTE,BYTE,BYTE (bomb wire-cut state) |
| `SrvSett`  | 3 | gmsgSrvSett | | server settings |
| `TSPAward` | 2 | gmsgTSPlayerAward | CBasePlayer::UpdateClientData | **MSG_ALL** BYTE entindex, BYTE award ([0x1d3]) |
| `TSBTime`  | 1 | gmsgTSBulletTime | CBasePlayer::UpdateClientData | BYTE bullettime ([0x5d]) |
| `TSHVoice` | 1 | gmsgTSHVoice | | byte (voice) |
| `TSFlag`   | 4 | gmsgTSFlag | CTF | 4 bytes |
| `CustomWP` | -1 | gmsgCustomWP | | custom weapon |
| `Spectator`| 2 | gmsgSpectator | | 2 bytes |
| `TSMBlur`  | 2 | gmsgTSMotionBlur | CBasePlayer (powerup) | 2 bytes (was MIS-NAMED "TSMotionBlur" in our server) |
| `TSMessage`| -1 | gmsgTSMessage | TSHudMessage | BYTE x5 + STRING |
| `TSSlowMo` | 2 | gmsgTSSlowAmt | CBasePlayer::UpdateClientData | **WRITE_COORD(slowamt*100)** (0.225 bulletslow / 0.35 slow / 1.0 normal) |
| `PTakeDam` | 13 | gmsgPTakeDam | | damage detail |
| `TSFrags`  | -1 | gmsgTSFrags | | variable |
| `PlInfo`   | 3 | gmsgPlInfo | | player info |
| `TSPSys`   | -1 | gmsgTSParticleSystem | particle | variable |

## Engine write-fn offsets seen in the decomp (enginefuncs GOT)
`0x190fd8`=MessageBegin `0x190fdc`=MessageEnd `0x190fe0`=WriteByte
`0x190fe4`=WriteChar `0x190fe8`=WriteShort `0x190fec`=WriteLong
`0x190ff0`=WriteAngle `0x190ff4`=WriteCoord `0x190ff8`=WriteString
`0x19103c`=index-of-edict (entindex for the MSG_ALL ActItems/TSPAward).

## Notes
- The original sends **TSHealth/TSArmor instead of** the stock Health/Battery for
  the player's own HUD (stock are still registered for compat / third-party).
- All per-frame sends are throttled (player [0x14d] / gun [0x9e0] ~0.1s gate) and
  only sent on change (cached prev value compared each frame).
- Inherited-name bugs fixed during Phase 4: `TSMotionBlur`→`TSMBlur`,
  `TSPowerUpTime`→`PwUpTime`, `RoundTime` size -1→2 + 2 bytes not 2 shorts.
