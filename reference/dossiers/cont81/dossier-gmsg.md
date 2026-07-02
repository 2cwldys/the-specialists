# Dossier: the 14 missing gmsg* user messages (+ 3 invented globals to reconcile)

Synthesized from the 14 `analysis_gmsg_*.md` scout reports, `reference/HUD_MESSAGES.md`,
`reference/coverage/AUDIT.md` (GLOBALS sections), and fresh lock-free binary checks run for this
dossier (rz-bin/raw-byte scans of `reference/ts_i386.so`). Everything below is grounded in the
binary or a client.dll handler decompile; server-side items that still need the typed `re` Ghidra
project (currently queued behind the melee run) or a live trace are in §5 VERIFY.

Scope = the AUDIT.md GLOBALS-MISSING gmsg rows (14) and the GLOBALS-INVENTED gmsg rows (3:
`gmsgStatusText`, `gmsgStatusValue`, `gmsgWeapons`). Key synthesis finding: **none of the 3
invented globals is a wrong-name substitute for any of the 14** — every analysis independently
confirmed this, so all 14 are clean ADDs (or declare-only parity adds), and the 3 invented ones
are straight REMOVALS (with one risk flag, §3.2).

---

## 1. Protocol overview table

Wire name = the swap contract (what REG_USER_MSG registers and pfnHookUserMsg matches).
The gmsg global NAME matters only for `audit.py` (matches our symbols by name).

| gmsg global | Wire name | Size | Wire format (send dest) | Purpose | Send trigger (original) | Our status |
|---|---|---|---|---|---|---|
| `gmsgSelAmmo` | `SelAmmo` | 4 | never sent (stock struct BYTEx4, provenance-only) | Stock-SDK dead registration; doubles as LinkUserMessages "already linked" guard | none — register-only, FIRST registration | ABSENT (HLU deleted it; we guard on CurWeapon) |
| `gmsgSetCurWeap` | — | — | never registered, no wire name string in binary | Stock-SDK dead global (0 relocations) | none | **EXACT PARITY ALREADY** (`dlls/UserMessages.h:45`, declared, never registered) |
| `gmsgTSKickAlign` | — | — | never registered, no name string in binary | Vestigial (name suggests abandoned kick/recoil sync) | none | ABSENT — need the dead global only |
| `gmsgClipInfo` | `ClipInfo` | 1 | BYTE clip — **never sent** (1 reloc = registration store only) | Vestigial clip update; client keeps a live handler (writes weapon-grid clip cell + HUD_ACTIVE) | none — register-only | ABSENT both sides (register + client hook needed; no send) |
| `gmsgTSState` | `TSState` | 1 | MSG_ONE: BYTE state (0 stand / 1 duck / 2 stunt-locked) | Posture figure (stand/crouch/prone sprite) + state-coloured dyn crosshair | `CBasePlayer::UpdateClientData` @0x81d00, on change (cache plausibly @0x499) | ABSENT; server value EXISTS (`m_iPlayerState` @0x498, `UpdatePlayerState()`); client uses INVENTED `g_iTSProne`+usehull substitute |
| `gmsgTSSpace` | `TSSpace` | 1 | MSG_ONE: BYTE freeSlots (0..81) | Left-HUD "free inventory space" readout | UpdateClientData, on change | ABSENT; value EXISTS (`TSPlayerFreeSlots()`, ts_gun.cpp) — never sent, never drawn |
| `gmsgTSBulletTime` | `TSBTime` | 1 | MSG_ONE: BYTE flag (client stores `!=0`) | Client bullet-time flag (DAT_1018d178) → recoil ×0.66667 in fire path (+ render variant) | UpdateClientData, on change (cache `m_iClientBulletTime` exists in our player.h:414) | ABSENT; consumer WAITS (`TS_AddViewRecoil(..., bBulletTime)` hardcodes `false` @ ev_ts.cpp:346) |
| `gmsgTSPlayerAward` | `TSPAward` | 2 | **MSG_ALL**: BYTE entindex (1..64), BYTE awardId | Point-award popup; client keeps per-player award table (stride-8 records, shared w/ ActItems-family) | UpdateClientData @0x81d00, on change of player[0x1d3] | ABSENT both sides |
| `gmsgTSPlayerInfo` | `PlInfo` | 3 | **MSG_ALL**: BYTE entindex, BYTE infoType (only 0 used), BYTE value (laser color: 1=green 2=blue else red) | Broadcast a player's laser-sight color (network half of `cl_lasercolor`; client sends `lasclr_<n>` ServerCmd on cvar change) | `ClientCommand` `lasclr_` prefix branch (~0x9dea8, client.cpp) | ABSENT; machinery exists: `cl_lasercolor` cvar (name+default match), `g_playerAttach` even-byte array, `GetLaserColor` palette; others' beams hardcode red |
| `gmsgTSServerSetting` | `SrvSett` | 3 | BYTE settingId (0..3), SHORT value. id0=roundtime, id1=usecash, id2=ammocount (client calls shared `Set_ammo_count(v)`), id3=weaponrestriction | Sync server cvar-derived settings to client HUD/buy menu | InitHUD @0x691e0 (MSG_ONE, id0); `CTSGameRules::Think` ×3 @0x675f6/0x67690/0x6771c (MSG_ALL ids 0/1/2, on change); `cmd_weaponrestriction` @0xb595f (MSG_ALL id3) | ABSENT; cvars `roundtime`/`usecash`/`ammocount` registered but inert; no `weaponrestriction` cmd, no `Set_ammo_count` |
| `gmsgTSWires` | `Wires` | 3 | MSG_ONE: BYTE×3 wire display states | Bomb wire-cut HUD: nonzero state swaps dyn crosshair for the 3-wire cut UI (`cutwire_<n>` back) | UpdateClientData (sole sender); helper `CBasePlayer::CheckForWires` computes the bytes | ABSENT message; server mechanics PARTIAL (CTSBomb + CutWire + `cutwire_` cmd exist; no CheckForWires, no send); client nothing |
| `gmsgTSTakeDamage` | `PTakeDam` | 13 | MSG_PVS @victim origin: COORD×3 hit point, COORD×3 dir, BYTE enc = min(dmg×0.5,127) + (torso/arm hit ? 128 : 0); ×8 pre-scale if dmg bit 1<<25; 0.3 s per-victim throttle (@0x750) | TS's live damage/pain-direction feedback. Original registers stock `Damage` 12 but **never sends it** | `CBasePlayer::TraceAttack` @0x801cc (sole site, 0x804d8–0x8055f) | ABSENT; **drift**: we SEND stock Damage from TakeDamage (player.cpp:4444) — original doesn't |
| `gmsgTSFade` | `TSFade` | -1 | payload always 10 B = stock ScreenFade layout (SHORT dur 4.12, SHORT hold 4.12, SHORT flags, BYTE r,g,b,a) — dur/hold **pre-divided by recipient's `GiveSlowMul()`** | Mod-owned ScreenFade replacement so the client dll (not engine) gets fades, slow-mo-corrected per player | `UTIL_ScreenFadeWrite` @0xdcc54 (MSG_ONE, `IsNetClient()` gate); all UTIL_ScreenFade/All callers route through it | PARTIAL-WRONG: we send engine `ScreenFade` (works, wrong wire name, no slow-mo scale); client has no TSFade hook (our client on orig server = NO fades) |
| `gmsgTSCustomWeaponParams` | `CustomWP` | -1 | WRITE_STRING of sprintf `"%i %f %f %f %f %i"` = weaponId, spread(+0x28), viewkick(+0x30), recoil(+0x34), damage(+0x1c), price(+0xd8). MSG_ONE (entity) or MSG_ALL (NULL) | Custom-weapons sync: pushes overridden per-weapon combat params + buy price to the client's `s_weapon_info` mirror (prediction + buy menu) | `SendWeaponParams` @0x8eb98 (single writer); `InformClientsOnWeaponsParams` @0x8eb14 (notice + loop 0..0x25) ← `CTSGameRules::InitHUD` (gated on rules+0x5884 flag) & `cmd_loadweapons` @0xb59ed; `AdjustSpecificWeapon` @0x8ea3c (live tweak → MSG_ALL) | ABSENT; `customweapons` cvar registered but inert; our `s_weapon_info` LACKS `iPrice` (+0xd8); buy menu uses a disconnected hardcoded price table |

### Invented globals (AUDIT GLOBALS-INVENTED) — reconcile verdicts

| Invented | What it is | Binary verdict (fresh check this dossier) | Action |
|---|---|---|---|
| `gmsgStatusText` | `REG_USER_MSG("StatusText", -1)` UserMessages.cpp:68 + sends player.cpp:1906/1919 (UpdateStatusBar) | **No `StatusText` string anywhere in ts_i386.so** (raw byte scan) → original never registers, never sends | REMOVE registration + UpdateStatusBar sends (server). Client hook may stay (harmless; never fires) |
| `gmsgStatusValue` | `REG_USER_MSG("StatusValue", 3)` UserMessages.cpp:69 + send player.cpp:1935 | Same — no string in binary | REMOVE registration + send (server) |
| `gmsgWeapons` | `REG_USER_MSG("Weapons", 8)` UserMessages.cpp:71 + send player.cpp:4421 + client hook hud.cpp:295 (`MsgFunc_Weapons`) — halflife-updated's 64-bit m_WeaponBits transport | No `gmsgWeapons` global in original; `"Weapons\0"` @ .rodata 0xfe7db exists but is NOT in a registration context (likely a menu label — VERIFY-low) | REMOVE registration + send + client hook. **RISK**: HLU uses it to carry weapon bits >31; TS weapon ids go to 0x25(37). Playtest weapon-select/HUD after removal; if high bits are load-bearing, route through engine clientdata like the original must have (VERIFY how the original client learns its arsenal — answer: WStatus/WeaponInfo, already implemented) |

Adjacent registration-set findings (same fresh scan; not in the 14 but same cleanup commit):
- **`Flashlight`/`FlashBat`**: no strings in ts_i386.so → original does NOT register them. Ours
  does (UserMessages.cpp:32-33). Remove (+ any flashlight ImpulseCommands sends) for
  registered-set parity. Playtest-gate: TS has no stock flashlight (attachment system instead).
- **`ShowMenu`** IS original (string in the .rodata registration run @0xfe473) — keep ours.
- The full .rodata registration-name run (0xfe3aa–0xfe58f) recovered verbatim:
  `SelAmmo CurWeapon Geiger HudText SayText TextMsg WeaponList ResetHUD GameTitle DeathMsg
  ScoreInfo TeamInfo TeamScore GameMode MOTD ServerName AmmoPickup WeapPickup ItemPickup
  HideWeapon SetFOV ShowMenu ScreenShake ScreenFade TSFade AmmoX TeamNames VGUIMenu WeaponInfo
  ClipInfo WStatus ActItems KFuPower TSHealth TSArmor TSState TSCash TSSpace NextStunt PwUp
  PwUpTime RoundTime ShowObj Wires SrvSett TSPAward TSBTime TSHVoice CustomWP TSMBlur TSMessage
  TSSlowMo PTakeDam TSFrags PlInfo TSPSys` (names like Health/Damage/Objective/Spectator dedupe
  against other literals elsewhere — their registration is separately proven in HUD_MESSAGES.md).
- **`TSFlag` wire-name question (adjacent VERIFY)**: neither `TSFlag\0` nor `TSHaveFlag\0`
  appears in .rodata; HUD_MESSAGES.md says wire `TSFlag` 4, our ts_flag.cpp registers
  `"TSHaveFlag"` 4. One of these is wrong — confirm the actual literal at the LinkUserMessages
  registration site in the `re` project (it may be built or live in a merged literal). Not one
  of the 14, but a potential swap-contract bug in ALREADY-landed code.

---

## 2. Implementation units (grouping)

- **U0 — Registration-set parity (no behaviour)**: SelAmmo (register-first + guard swap),
  ClipInfo (register + client hook, never send), SetCurWeap (no-op), TSKickAlign (dead global
  declaration only), REMOVE StatusText/StatusValue/Weapons (+adjacent Flashlight/FlashBat).
  All binary-proven; zero VERIFY dependencies.
- **U1 — Player-state block** (per-frame `TSUpdateClientData` sends, MSG_ONE on-change, same
  pattern as landed TSHealth/TSArmor/TSCash): **TSState, TSSpace, TSBTime** + the MSG_ALL
  sibling **TSPAward**. Client side: CHudTSStatus (TSState/TSSpace), TS hud element +
  `g_bBulletTime` (TSBTime), per-player table (TSPAward). Values already exist server-side
  for TSState/TSSpace; TSBTime unblocks the waiting recoil consumer.
- **U2 — Player info round-trip**: **PlInfo** (client `lasclr_` sender ← cvar change; server
  ClientCommand rebroadcast; client per-player laser color consumed by ts_attachments beam loop).
- **U3 — Game-settings sync**: **SrvSett** (CTSGameRules InitHUD/Think + `weaponrestriction`
  command + shared `Set_ammo_count`); client 4-way switch + consumers (cash HUD gate, buy-menu
  restriction, round HUD).
- **U4 — Damage/fade feedback**: **PTakeDam** (TraceAttack send + retire our stock-Damage send;
  client pain-direction feed) and **TSFade** (UTIL_ScreenFadeWrite rework with per-recipient
  GiveSlowMul division; client handler + fade application).
- **U5 — Custom weapon params**: **CustomWP** (`iPrice` field into s_weapon_info + extractor;
  SendWeaponParams / InformClientsOnWeaponsParams / AdjustSpecificWeapon; InitHUD gate; client
  handler into the SHARED weapon table; buy-menu price rewire; follow-up: loadweapons/
  saveweapons/adjust_weapon_value + customweapons cvar read).

---

## 3. THE IMPLEMENTATION PLAN (ordered; every step builds green + is independently committable)

Convention per message: server = global with the ORIGINAL gmsg name (audit matches by name) +
`REG_USER_MSG("<exact wire name>", <exact size>)`; client = `HOOK_MESSAGE` + handler that mirrors
the decompile byte-for-byte; caches reset on respawn/ResetHUD (map-change-state-reset memory).
Registration ORDER is not part of the swap contract (SVC_NEWUSERMSG negotiates by name), but we
keep the original order where a home file makes it free.

### Step 0 — U0: registration-set parity + invented-global removal (1 commit)
Server `dlls/UserMessages.h/.cpp` (+ player.cpp):
1. `gmsgSelAmmo = REG_USER_MSG("SelAmmo", 4);` as the FIRST registration; switch the
   already-linked guard from `gmsgCurWeapon` to `gmsgSelAmmo`. No send, no struct needed.
2. `gmsgClipInfo = REG_USER_MSG("ClipInfo", 1);` right after `WeaponInfo`'s registration
   (once that block exists; else after the stock block). **Never send it.**
3. `int gmsgTSKickAlign = 0; // DEAD in original: never registered/sent — parity only`
   (external linkage so audit.py sees the symbol; home: ts_player.cpp next to the TS gmsg block
   or UserMessages.h beside gmsgSetCurWeap).
4. `gmsgSetCurWeap`: no change (already exact parity).
5. REMOVE: `gmsgStatusText`/`gmsgStatusValue` registrations (UserMessages.cpp:68-69) + the
   UpdateStatusBar MESSAGE_BEGINs (player.cpp:1906/1919/1935 — neuter or delete UpdateStatusBar
   body); `gmsgWeapons` registration (UserMessages.cpp:71) + send (player.cpp:4421) + client
   hook/`MsgFunc_Weapons` (hud.cpp:128/295). Adjacent: remove `Flashlight`/`FlashBat`
   registrations (UserMessages.cpp:32-33) + their send sites.
Client: `HOOK_MESSAGE(ClipInfo)` on `CHudTSWeaponBar` + `MsgFunc_ClipInfo`: READ_BYTE →
   `m_iClip[curWeaponType]` (our representation of the original grid cell) + `HUD_ACTIVE`.
Gate: build both libs; `coverage.py all` (ClipInfo/SelAmmo rows flip); **playtest-gated**:
   weapon-select HUD + arsenal after gmsgWeapons removal, flashlight impulse dead-path.

### Step 1 — U1a: TSState (1 commit)
Server: `static int gmsgTSState` in ts_player.cpp, lazy `REG_USER_MSG("TSState", 1)`;
send in `TSUpdateClientData()` after the TSArmor block (original order TSArmor→TSState→TSCash):
on `m_iPlayerState != m_iClientTSState` → MSG_ONE WRITE_BYTE. Add `int m_iClientTSState`
cache beside the siblings, reset on ResetHUD/respawn. (`UpdatePlayerState()` already runs first.)
Client: `CHudTSStatus::MsgFunc_TSState` (READ_BYTE → `m_iState`, HUD_ACTIVE); **replace the
invented posture derivation** (ts_status.cpp:143-156): `posture = m_iState`; DELETE `g_iTSProne`
and the `curstate.usehull` check (and any writers of g_iTSProne).
Playtest-gated: posture figure (stand/crouch/prone) during duck + stunts.
Deferred-VERIFY: dyn-crosshair state colour wiring (client Draw FUN_1004e9f0).

### Step 2 — U1b: TSSpace (1 commit)
Server: `static int gmsgTSSpace` in ts_player.cpp, `REG_USER_MSG("TSSpace", 1)`; on-change
MSG_ONE `WRITE_BYTE(TSPlayerFreeSlots(this))`; cache `m_iClientTSSpace` (reset -1 with siblings).
Client: `CHudTSStatus::MsgFunc_TSSpace` → `m_iSpace` + HUD_ACTIVE; draw in the left status
panel beside health/armor/cash (exact label/position VERIFY when styling).
VERIFY (non-blocking): whether original reads player+0x548 directly vs recomputes.

### Step 3 — U1c: TSBTime (1 commit)
Server: `static int gmsgTSBulletTime` (ts_player.cpp), `REG_USER_MSG("TSBTime", 1)`; on-change
MSG_ONE WRITE_BYTE(flag) using the EXISTING `m_iClientBulletTime` cache (player.h:414); reset
with siblings. **VERIFY the exact source member before commit** (0x175 byte vs [0x5d]≡0x174/0x175
dword-index ambiguity; candidates set by GoBulletTime/SlowMotionFlow) — decompile
UpdateClientData @0x81d00 in the `re` project; do NOT conflate with `m_bSlowBullets`.
Client: hook on the TS hud element (pattern of `MsgFunc_TSSlowMo`); handler sets new global
`bool g_bBulletTime = READ_BYTE() != 0` + HUD_ACTIVE. **Absorb**: change ev_ts.cpp:346 to pass
`g_bBulletTime` instead of hardcoded `false` in `TS_AddViewRecoil` (closes that VERIFY).
Playtest-gated: recoil reduction while in bullet-time.

### Step 4 — U1d: TSPAward (1 commit)
Server: `int gmsgTSPlayerAward` (ts_player.cpp), `REG_USER_MSG("TSPAward", 2)`; new byte member
`m_iAward` (provenance player+0x1d3) + last-sent cache; on-change in TSUpdateClientData:
MSG_ALL WRITE_BYTE(entindex()) WRITE_BYTE(m_iAward); resend after ResetHUD.
Client: hook on the CTSHud-equivalent; bounds-check idx 1..64; store into a per-player
extra-info array (8-byte-stride mirror — share with the ActItems-family table when restructured);
HUD_ACTIVE.
VERIFY-gated (send stays inert until then): the award PRODUCER — writers of player+0x1d3 are
inline in kill/gamerules/stunt paths (no symbol); needs `re` project refs pass. The popup DRAW
is a separate client-RE task. Land the wire half now (byte-faithful contract), producer later.

### Step 5 — U2: PlInfo (1 commit)
Client sender: in the CTSHud per-frame think (ts_hud.cpp): cache `m_iLastLaserColor`; on
`(int)cl_lasercolor->value` change → `sprintf("lasclr_%i", v)` → **pfnServerCmd** (not ClientCmd).
Server: `int gmsgTSPlayerInfo` (defined in client.cpp per AUDIT file attribution),
`REG_USER_MSG("PlInfo", 3)`; ClientCommand prefix branch in the original strstr idiom
(`strstr(pcmd,"lasclr_")==pcmd`): MSG_ALL WRITE_BYTE(ENTINDEX) WRITE_BYTE(0)
WRITE_BYTE(atoi(pcmd+7)) — no clamp (client stores raw byte).
Client handler: in ts_attachments.cpp (owns the paired even byte): `g_playerLaserColor[]` next
to `g_playerAttach`; pfnHookUserMsg("PlInfo", ...); store only if idx∈[1,64] && type==0.
Consume: beam loop ~ts_attachments.cpp:287 → `GetLaserColor(isLocal ? cvar : g_playerLaserColor[i])`;
delete the SLICE comment. Zero the array wherever g_playerAttach resets. Do NOT invent a
late-joiner resend (original has none — a joiner sees red until the owner re-sends).
Playtest-gated: two-client laser color visibility.

### Step 6 — U3: SrvSett (1-2 commits; **decompile-first**)
Pre-req (queued `re` pass): decompile 0x691e0 (InitHUD site) / 0x675f6, 0x67690, 0x6771c
(Think sites) / 0xb595f (cmd_weaponrestriction) to pin change-detect members, rounding, and
whether InitHUD sends only id 0.
Then server: `int gmsgTSServerSetting` (TS_gamerulesclass.cpp home → our ts_gamerules file),
`REG_USER_MSG("SrvSett", 3)`; InitHUD → MSG_ONE id0 `WRITE_SHORT((int)round(roundtime.value))`;
Think → on-change MSG_ALL ids 0/1/2 (id2 calls server `Set_ammo_count(v)` around the send);
add `weaponrestriction` console command (game.cpp home) → MSG_ALL id3 (+
`display_weaponrestriction` text, format from .so).
Client: hook `SrvSett`; handler = the 4-way switch verbatim (READ_BYTE id, READ_SHORT val);
case 2 calls a SHARED `Set_ammo_count(int)` (one definition compiled into both libs, as the
original's shared source does); cases 0/1/3 → globals; always HUD_ACTIVE (unknown ids tolerated).
Consumers (VERIFY client decompiles, then wire): cash-HUD hide when usecash==0, buy-menu
restriction filter, round HUD.

### Step 7 — U3b/U1e: Wires (1 commit; **decompile-first for the encoder**)
Pre-req: `re` decompile of UpdateClientData's Wires block + `CheckForWires__11CBasePlayer`
(byte encoding, trigger condition, change-detect, clear-on-walk-away).
Server: `int gmsgTSWires` + `REG_USER_MSG("Wires", 3)` (UserMessages home; AUDIT file
TS_player.cpp for the global); implement `CBasePlayer::CheckForWires()` (name-faithful) from the
decompile; send MSG_ONE on change from UpdateClientData; cache triple resets on level change.
Cross-check our `CBasePlayer::CutWire` all-bombs loop vs the original targeting.
Client: `CHudTSWeaponBar` (same original object as WStatus): `MsgFunc_Wires` → `byte m_wires[3]`
(provenance +0x36fc..fe) + HUD_ACTIVE; draw: when any wire nonzero, swap the dynamic crosshair
(ts_reticle.cpp) for the wires UI — reconstruct `FUN_1004d550` via `dc.sh client 0x1004d550`
first. Playtest-gated: bomb map cut-wire flow end-to-end.

### Step 8 — U4a: PTakeDam (1 commit)
Server: `inline int gmsgTSTakeDamage` (UserMessages.h) + `REG_USER_MSG("PTakeDam", 13)` in
LinkUserMessages (original registers there, not lazily). KEEP `REG_USER_MSG("Damage", 12)`
(original registers it) but **REMOVE the gmsgDamage SEND** in `CBasePlayer::TakeDamage`
(player.cpp:4444) — original never sends it. Add transient `float m_flNextPTakeDamTime`
(provenance this@0x750, invented name — comment it). In `CBasePlayer::TraceAttack` after the
hitgroup switch (we already have m_LastHitGroup + the body-impact sound from the same original
fn), insert the throttled MSG_PVS send exactly as disassembled:
```c
if (m_flNextPTakeDamTime <= gpGlobals->time) {
    m_flNextPTakeDamTime = gpGlobals->time + 0.3f;
    float enc = flDamage * (((bitsDamageType & (1 << 25)) != 0) ? 8.0f : 1.0f) * 0.5f;
    if (enc > 127.0f) enc = 127.0f;
    if (m_LastHitGroup >= HITGROUP_CHEST && m_LastHitGroup <= HITGROUP_RIGHTARM) enc += 128.0f;
    MESSAGE_BEGIN(MSG_PVS, gmsgTSTakeDamage, pev->origin);
    WRITE_COORD(ptr->vecEndPos.x); WRITE_COORD(ptr->vecEndPos.y); WRITE_COORD(ptr->vecEndPos.z);
    WRITE_COORD(vecDir.x); WRITE_COORD(vecDir.y); WRITE_COORD(vecDir.z);
    WRITE_BYTE((int)enc);
    MESSAGE_END();
}
```
(The surrounding TraceAttack damage-calc parity items — ×0.7 kevlar/iuser4 bits 0x10c10, timed
bullet-resist @0x524, hitgroup multipliers — are a SEPARATE reconcile, VERIFY-gated.)
Client: `HOOK_MESSAGE(PTakeDam)` on CHudHealth (owns CalcDamageDirection/DrawPain): read 6
COORDs + BYTE; feed direction into the pain indicator; HUD_ACTIVE. Keep the stock Damage hook
registered (original client hooks both). Retire the health.cpp:211 placeholder note.
VERIFY-gated fidelity tail: client `FUN_10048840` internals (bit7 decode + hit-point effect).
Playtest-gated: pain direction + no double-feedback (Damage send removed).

### Step 9 — U4b: TSFade (1 commit; **decompile-assisted**)
Pre-req (recommended before commit): `re` decompile of UTIL_ScreenFadeWrite @0xdcc54 (whether
the GiveSlowMul division hits floats pre-FixedUnsigned16 or the fixed shorts; Build/Write split)
+ client reader of 0x101914a8/0x10190b5c (application mechanism).
Server: `int gmsgTSFade` (defined in player.cpp per AUDIT; extern in UserMessages.h);
`REG_USER_MSG("TSFade", -1)` (KEEP stock ScreenFade registration); rework
`UTIL_ScreenFadeWrite(const ScreenFade&, CBaseEntity*)` to send on gmsgTSFade — same 7 writes,
duration+holdTime divided by `pEntity->GiveSlowMul()` (already exists, cbase.h:181)
per-recipient (so ScreenFadeAll scales per player); keep IsNetClient() gate. No caller changes.
Client: `MsgFunc_TSFade` on CHud: READ_SHORT×3 + READ_BYTE×4, store + latch receive time
(mirror of the handler); apply via pfnSetScreenFade without fighting view.cpp's slow-mo tint
(which rewrites the fade per frame) — application details from the client VERIFY.
Playtest-gated: flashbang/env_fade normal + under slow-mo; slow-mo tint coexistence.

### Step 10 — U5: CustomWP (2 commits: table+server, then client)
Server: (a) add `int iPrice; // s_weapon_info+0xd8` to s_weapon_info; extend
`scripts/extract_weapons.py` to pull +0xd8 from each Write_<weapon> and regenerate the table
(spot-checks: knife 100, skorpion 850, barrett 9000, M60 5000). (b) `inline int
gmsgTSCustomWeaponParams` + `REG_USER_MSG("CustomWP", -1)`; implement in our TS_util home,
names EXACT: `SendWeaponParams(tsweapon_t, CBaseEntity*)` (guards w>0x25 / empty slot;
`sprintf(buf, "%i %f %f %f %f %i", ...)` — default %f 6-decimal ASCII IS the wire format;
MSG_ONE/MSG_ALL by pEntity NULLness); `InformClientsOnWeaponsParams(CBaseEntity*)`
(green 45,195,45 TSHudMessage "This server is using custom weapons." + loop 0..0x25);
`AdjustSpecificWeapon(tsweapon_t, float, float, float, float, int)` (store 5 fields →
SendWeaponParams(w,NULL) → `UTIL_ClientPrintAll(3, "%s behaviour changed")` → set flag);
`CTSGameRules::m_iCustomWeapons` flag (provenance +0x5884) + InitHUD:
`if (flag) InformClientsOnWeaponsParams(pl);`.
Client: `HOOK_MESSAGE(CustomWP)` from the buy-menu element: READ_STRING → sscanf
`"%i %f %f %f %f %i"` → write the 5 fields into the SHARED `g_weaponInfo[id]` (prediction picks
them up for free); set buy-menu dirty flag. Mimic the original no-clamp (bad id mutates slot 0
via get_weapon_info — do NOT fix). Rewire ts_buymenu.cpp prices to `get_weapon_info(w)->iPrice`
(drop its local hardcoded table).
Follow-up (separate task, VERIFY-gated): `cmd_loadweapons`/`cmd_saveweapons`/
`adjust_weapon_value(char*)` + reading the `customweapons` cvar at rules init (file line format
`" %i %f %f %f %f %i "`), and the console command that reaches adjust_weapon_value.
Playtest-gated: adjust a weapon live → client price + predicted spread change.

After every step: `scripts/build.sh amd64 all` + `scripts/coverage/coverage.py all` +
`scripts/coverage/audit.py` (the 14 MISSING gmsg rows burn down; INVENTED gmsg rows go to zero
at Step 0). Full playtest gate remains task #2 before merging restructure-source-parity.

---

## 4. Reconcile/rename map (summary)

| Ours today | Fate |
|---|---|
| `gmsgStatusText`/`gmsgStatusValue` reg + UpdateStatusBar sends | DELETE (binary-proven never registered) |
| `gmsgWeapons` reg + send + client hook | DELETE (risk-flagged, playtest) |
| `Flashlight`/`FlashBat` regs | DELETE (adjacent; binary-proven absent) |
| stock `gmsgDamage` SEND (player.cpp:4444) | DELETE send, KEEP registration (PTakeDam replaces) |
| stock ScreenFade SEND path (UTIL_ScreenFadeWrite → gmsgFade) | REROUTE to gmsgTSFade + GiveSlowMul; KEEP ScreenFade registration |
| `g_iTSProne` + usehull posture derivation (ts_status.cpp) | DELETE — replaced by TSState byte |
| `TS_AddViewRecoil(..., false)` hardcode (ev_ts.cpp:346) | REPLACE with `g_bBulletTime` (TSBTime) |
| ts_buymenu.cpp local price table | REPLACE with shared `iPrice` (CustomWP) |
| laser beam loop hardcoded red for others | REPLACE with `g_playerLaserColor[]` (PlInfo) |
| `m_iClientBulletTime` (player.h:414, unused) | ABSORB as the TSBTime change-detect cache |
| `TSPlayerFreeSlots()` (ts_gun.cpp, unsent) | ABSORB as the TSSpace value source |
| `m_iPlayerState`/`UpdatePlayerState()` (unsent) | ABSORB as the TSState value source |
| `cl_lasercolor` cvar, `g_playerAttach`, `GetLaserColor` | ABSORB into the PlInfo round-trip |
| `customweapons` cvar (inert) | ABSORB into the CustomWP follow-up (loadweapons) |
| CTSBomb `CutWire`/`SetRandomWires`/`cutwire_` cmd | ABSORB under Wires; add `CheckForWires` |

---

## 5. VERIFY list (server-send-site uncertainties → `re` Ghidra project, queued behind the melee run, or live trace)

Blocking-before-commit items marked ★; the rest are fidelity-tail / non-blocking.

**TSBTime** ★ exact source member for the sent byte — UpdateClientData @0x81d00 decompile
(0x175 byte vs [0x5d] dword-index = 0x174/0x175; do not conflate with m_bSlowBullets); which
paths set/clear it (GoBulletTime / SlowMotionFlow / SetGlobalSlowMotion). Non-blocking: other
client readers of DAT_1018d178 (render overlay) — clientdll pass.

**TSState** — confirm change-cache @0x499 and ResetHUD-forced resend (pattern-implied);
MSG_ONE (near-certain). Client: which of CTSHud+0x22 vs DAT_1018a60c feeds the dyn-crosshair
colour (FUN_1004e9f0, client project).

**TSSpace** — value expression: +0x548 direct read vs recompute (our TSPlayerFreeSlots), clamp
at WRITE_BYTE; cache offset; MSG_ONE vs MSG_ONE_UNRELIABLE.

**TSPAward** ★ (for the producer half) — writers of player+0x1d3 (inline, no symbol → `re.sh
refs` / Ghidra xrefs on the member offset); change-cache offset; reset-after-delay semantics;
respawn/map-change resend. Client: CTSHud Draw chain for the popup presentation.

**PlInfo** — atoi/clamp details at the ClientCommand branch (~0x9dea8); MSG_ALL vs broadcast
reliability; whether m_iLastLaserColor (+0x3728) resets on map change (live trace: change
cl_lasercolor mid-map + map change on the original). Client: FUN_10056b60's caller (table zero).

**SrvSett** ★ — decompile 0x691e0 / 0x675f6 / 0x67690 / 0x6771c / 0xb595f: change-detect member
offsets (rules+0x578/0x558/0x55c/0x5870 caches), InitHUD id set (only 0?), Set_ammo_count
ordering vs the id2 send, roundtime rounding, `display_weaponrestriction` text format, which
cvar caches at rules+0x558 (usecash assumed). Map-change hole for ids 1–3 late joiners —
confirm and REPRODUCE (drop-in rule).

**Wires** ★ — CheckForWires + the UpdateClientData Wires block: byte value encoding (must not
leak the defuse wire), trigger condition (proximity/cancutwire/bomb-active), change-detect,
clear-on-walk-away (0,0,0?). Our CutWire all-bombs loop vs original targeting. Client:
FUN_1004d550 wires-UI internals (`dc.sh client 0x1004d550`).

**PTakeDam** — DMG bit 0x2000000 meaning; entry-gate timer @0x71c and bullet-resist timer
@0x524; iuser4 kevlar bits 0x10c10 (TraceAttack damage-calc parity, separate reconcile);
whether the throttle skip target (0x80830) also skips the blood section. Client: FUN_10048840
(bit7 decode + hit-point effect). Wire half is fully proven (disasm 0x804d8–0x8055f) — send
site NOT verify-blocked.

**TSFade** ★ (division placement) — UTIL_ScreenFadeWrite @0xdcc54: float-vs-fixed division
order, Build/Write split, per-recipient scaling in ScreenFadeAll; whether ANY path still emits
stock ScreenFade (assume none); TS caller list (`re.sh refs UTIL_ScreenFadeWrite`). Client:
reader of 0x101914a8/0x10190b5c → exact application (pfnSetScreenFade?) + 4.12 decode.

**CustomWP** — InitHUD gate member home (+0x5884 via `this` vs GOT global); cmd_loadweapons
NULL/MSG_ALL + whether it sets +0x5884; adjust_weapon_value caller chain + the console command
reaching AdjustSpecificWeapon; TSHudMessage params (8,100 = channel/hold?) exact meaning;
cmd_saveweapons file format tail. Writer/parser field mapping fully proven — core send NOT
verify-blocked.

**ClipInfo / SelAmmo / SetCurWeap / TSKickAlign** — no blocking verifies. Cosmetic: original
defining TU of gmsgClipInfo/gmsgSelAmmo (player.cpp per AUDIT) and of the dead-global block
(affects file placement only).

**Adjacent** ★ — the TSFlag vs TSHaveFlag wire-name conflict (already-shipped ts_flag.cpp code;
confirm the literal at the registration site in `re`); the `"Weapons\0"` @0xfe7db string's
actual use (expected: menu label, not a registration); `"Spectator"` registration-string
location (HUD_MESSAGES lists it registered but no .rodata literal found — likely merged/suffix
literal; confirm at the registration site).
