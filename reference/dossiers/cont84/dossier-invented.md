# Dossier: the 28 "invented methods" vs the original binaries (cont.84)

Scope: audit.py's invented-method list. For each, WHERE the equivalent logic lives in the
ORIGINAL `reference/ts_i386.so` (nm addresses; Ghidra = +0x10000) / `reference/client.dll`
(imagebase 0x10000000). Raw decompiles: `inv_*.c` in this directory. No source was edited.

Verdict legend: **MOVE** = our logic belongs inlined/renamed at the original site;
**ACCEPT** = modern-SDK/prediction infrastructure with no original counterpart;
**DELETE** = no original mechanism at all.

Architecture note that recurs below: the original runs **stock gamerules objects**
(`g_pGameRules` = CHalfLifeMultiplay or CHalfLifeTeamplay, TS-patched in place) **plus a
separate non-virtual CTSGameRules singleton** (`TSGameRules` global, GOT `PTR_TSGameRules`).
Most "gamerules" work we put on CTSGameRules actually lives on CHalfLifeMultiplay/Teamplay
or in free functions.

---

## A. CTSGun group

### A1. GetWeaponData / SetWeaponData
**EVIDENCE** — the original has exactly ONE GetWeaponData: the FREE function
`GetWeaponData__FP7edict_sP13weapon_data_s` @0xa00ac (client.cpp), no class method, and
**no SetWeaponData anywhere** (`nm` has no other match). Decompile
(`inv_A_GetWeaponData_free__*.c`): zeroes 32×0x58 weapon_data slots, then loops **all 6
`m_rgpPlayerItems` slots (player+0x9f0..)** and each item chain (`item+0x84` = m_pNext) —
not only the active weapon. Per item it follows the stock SDK pattern exactly
(vtable slots verified in VTABLES.md):

```
pw = item->GetWeaponPtr();            // vt+0x158
if (pw && pw->UseDecrement()) {       // vt+0x198
    ItemInfo II={0}; pw->GetItemInfo(&II);   // vt+0x120
    if (II.iId < 32) { wd = &info[II.iId]; ... }
}
```
Fields filled (member offsets confirmed against members.tsv):
- `wd.m_iId = II.iId`; `wd.m_iClip = pw+0xb4 (m_iClip)`
- `wd.m_flNextPrimaryAttack = pw+0x9c`, `m_flNextSecondaryAttack = pw+0xa0`,
  `m_flTimeWeaponIdle = pw+0xa8` — each clamped to ≥ −0.001
- `wd.m_fInReload = pw+0xc0`; `wd.m_fInSpecialReload = pw+0x98`
- `wd.fuser1 = pw->pev->fuser2` (pev+0x254, clamped ≥ −0.001)
- `wd.fuser2 = ent+0x4c`, `wd.fuser3 = ent+0x50`,
  `wd.iuser1..3 = ent+0x54/0x58/0x5c`

**Akimbo per-barrel clip packing REFUTED**: a scan of the whole CTSGun code range
(0x70000–0x77500) and client.cpp finds **no writes** to entity offsets 0x4c–0x5c (only
`compile_weapons_info` writing a different struct, and GetWeaponData itself reading them).
These are TS-added CBaseEntity fields (region below the gcc2 vptr@0x60) that appear to be
generic/unused for CTSGun — inherited stock-shaped packing, not barrel clips. Akimbo
left/right clips live in the items array (`s_weapon_status` +0x1c/+0x20 → gun+0x130/0x134,
see A4/TakeWeapon) and reach the client via **WeaponInfo/WStatus messages, not weapon_data**.

**VERDICT**: MOVE — our GetWeaponData logic belongs in the free `GetWeaponData(edict*,
weapon_data_s*)` in client.cpp, stock-shaped, packing the fields above. Our fuser2/fuser3 =
barrel-clips packing is an invention and must go. SetWeaponData: DELETE server-side (no
original); if the modern SDK's client-prediction needs a receiving side in cl_dll, that part
is ACCEPT (client-side infrastructure), but it must consume the original wire above.
**CONFIDENCE**: high (structure/fields), medium ("0x4c–0x5c never written" — no writer found
in weapon/client.cpp code, but a memset-style writer elsewhere can't be fully excluded).

### A2. SendWeaponStatus (+ the rest of our UpdateClientData split)
**EVIDENCE** — every gameplay `gmsgTSWeaponStatus` send site is inside
`CTSGun::UpdateClientData(CBasePlayer*)` @0x758c8 (sites 0x7596e, 0x759e2, 0x75cb8,
0x75d55; the only other GOT ref is the LinkUserMessages registration block @0xc7db0).
Full structure (`inv_A_GetWeaponData_free__*.c`, 2nd function):
1. **0.1 s rate limit**: `if (gun+0x9e0 > gpGlobals->time) return 0; gun+0x9e0 = time+0.1`.
2. **Diff-and-send loop** over all 38 weapons (unrolled 2/iteration, counter 0x25→0):
   compares each item's status byte + clip against a per-gun mirror array (bytes @gun+0x9d5…,
   clip mirror in the same stride) and on change sends
   `WStatus{BYTE weaponId, BYTE status, BYTE clip}` (3 bytes, MSG_ONE). The client parser
   (`FUN_1004f990`, inv_D file) reads exactly BYTE,BYTE,BYTE and stores id/clip into its
   slot/pos grid — wire confirmed both ends.
3. **Active item block** (`m_pActiveItem` != 0): on weapon-type change (gun+0x97a mirror) or
   reserve-ammo change (gun+0x9b4 vs `m_ammoReserve[caliber]`), snapshot the active
   `s_weapon_status` (14 dwords → gun+0x97c) and send
   `WeaponInfo{BYTE type, BYTE clip(0x980), SHORT reserve, BYTE firemode(0x97e),
   BYTE attachFlags(0x98c)}`, then send WStatus for the akimbo partner ids
   (`weapon_info+0xc8` and `+0xcc`) if owned. Then `gun+0x980 = activeItem->iClip`.
4. **No active item**: on type change send `WeaponInfo{BYTE type, BYTE round(player+0x774),
   SHORT 0, BYTE 0, BYTE 0}`.

Caller: `CBasePlayer::UpdateClientData` calls it via vtable +0x154
(= CTSGun::UpdateClientData in VTABLES.md), gated on `m_pActiveItem->m_iId == 0x10`.

**VERDICT**: MOVE — our SendWeaponStatus + WeaponInfo diffing is the body of
`CTSGun::UpdateClientData`; restructure onto that one method.
**CONFIDENCE**: high.

### A3. OwnedCount / SlotsUsed
**EVIDENCE** — there is **no recompute helper** in the original. TSSpace is a **stored
counter**: `player+0x548`, initialized to **0x51 (81)** in `TSInit` @0x82a4c, mirror
`+0x54c` diff-sent as `gmsgTSSpace{BYTE}` in `CBasePlayer::UpdateClientData` @0x81d00.
Mutation sites (inv_A3C5 file):
- `CBasePlayer::PickUpWeapon` @0x8314c: cost = `weapon_info+0xd0` (byte weight), multiplied
  by the incoming clip count when `weapon_info+0xdc` (per-unit flag) and a status is passed;
  `newSpace = player+0x548 − cost; if (newSpace < 0) fail; … player+0x548 = newSpace;`
  then `SetPlayerSpeed(player, GetSpeedBySlots(player))` — the one real helper:
  `GetSpeedBySlots__FP11CBasePlayer` @0x66bd4.
- `CBasePlayer::BuyWeapon` @0x845bc computes **cash** (info+0xd8 + attachments
  900/800/500/0x708) and delegates the slot math to PickUpWeapon.
- (The give-back on drop is the symmetric inline site in the drop path, same counter.)

**VERDICT**: DELETE the recompute helpers; MOVE to the original mechanic — a stored
`player+0x548` counter mutated inline in PickUpWeapon/drop with `GetSpeedBySlots` as the
only named helper.
**CONFIDENCE**: high.

### A4. CanDeploy / LoadActiveFromItem / SyncActiveToItem / UpdateActivePointers
**EVIDENCE** — the complete original CTSGun method set (nm `__6CTSGun`, 47 methods) contains
none of these and nothing similar: the items-array state is manipulated **inline**:
- `TakeWeapon` @0x70a04 writes `s_weapon_status` fields directly (owned @+0x114 stride 0x38,
  firemode +0x116, clip +0x118, attach flags +0x124, attach active +0x128, akimbo split
  clips +0x130/+0x134, info ptr +0x13c), including the akimbo merge of partner clips/flags.
- `SelectWeapon` @0x70fb8 is just: not-same && id<0x26 && owned && enabled →
  `HolsterWeapon(); m_weaponType = id;`.
- `DropWeapon` @0x8094c zeroes the 14-dword status and picks GetBetterWeapon/HolsterWeapon.
- `GetActiveItems` @0x71228 returns `m_pActiveItem->attachActive | (0x40 if akimbo)`.
- The active-status snapshot ("sync") is inline in `CTSGun::UpdateClientData` (A2), and the
  active pointers (`m_pActiveItem`/`m_pActiveInfo`) are set inside Deploy/SelectWeapon paths,
  not via a helper.

**VERDICT**: ACCEPT only as private modern-prediction plumbing IF the halflife-updated
prediction layer genuinely needs them; otherwise fold each back into the original methods
(TakeWeapon/SelectWeapon/Deploy/UpdateClientData). They must not appear as public
structure the audit compares — prefer folding (MOVE) where practical.
**CONFIDENCE**: high (method set is exhaustive from symbols).

---

## B. CBasePlayer TS* group

### B1. TSUpdateClientData
**EVIDENCE** — it is the TS block inside `CBasePlayer::UpdateClientData` @0x81d00
(TS_player.cpp; full decompile in inv_A file, 3rd function). Exact structure/order:
1. Gate: `player+0x534` (next-update time) — 0.1 s cadence, also requires ≥0.
2. `m_fInitHUD` block: ResetHUD(0); if !`+0x9b8`: InitHUD msg,
   `g_pGameRules->InitHUD(player)` (vt+0x34), **`CTSGameRules::InitHUD(TSGameRules, player)`**
   @0x69134, `+0x9b8=1`, FireTargets "game_playerjoin" (if IsMultiplayer); then
   FireTargets "game_playerspawn"; if round active (`TSGameRules+8` = m_flRoundEnd ≠ 0 and
   `TSGameRules+0x10`): `TSRoundTime{BYTE secsLeft, BYTE 0}`; if lowgrav map+player:
   `TSPowerUp{SHORT 0x100, BYTE 0}`.
3. HideWeapon (m_iHideHUD vs +0x9ec).
4. `TSHealth{BYTE round(health clamped ≥0)}` (mirror +0x9e0).
5. `TSArmor{BYTE round(armorvalue)}` (mirror +0x9e4).
6. Train (m_iTrain & 0xc0).
7. `TSState{BYTE +0x498}` (mirror +0x499).
8. `TSCash{SHORT +0x540}` (mirror +0x544).
9. `TSSpace{BYTE +0x548}` (mirror +0x54c).
10. `TSPowerUp{SHORT m_iHeldPowerup, BYTE m_flPowerupTime}` (mirrors m_iClientHeldType/Dur).
11. `TSSlowMo/TSSlowAmt{COORD 100*amt}` — amt = 0.225 if powerup&8, 0.35 if powerup&1, else
    m_flSlowMotionTarget (mirror m_flClientSlowAmt).
12. `TSBulletTime{BYTE +0x174}` (mirror +0x175).
13. `TSActiveItems` **broadcast MSG_ALL {BYTE playerIdx, BYTE GetActiveItems()}** — gated on
    alive + active item is CTSGun (id 0x10), mirror +0x720.
14. `TSPAward` broadcast {BYTE idx, BYTE +0x74c} (mirror +0x74d).
15. `Wires{BYTE,BYTE,BYTE}` (+0x78c/d/e vs +0x78f/790/791).
16. `m_pActiveItem->UpdateClientData(this)` via vt+0x154 when id==0x10 (→ A2).
17. `+0xa14 = m_pActiveItem; +0x9f4 = m_iFOV`.

**VERDICT**: MOVE — inline as the body of CBasePlayer::UpdateClientData in this order.
**CONFIDENCE**: high.

### B2. TSEnterSpectate / TSSpectateThink / TSTryRespawn
**EVIDENCE** (inv_BC files):
- **Spectator wire settled**: the original sends stock-shaped
  `gmsgSpectator{BYTE playerIndex, BYTE flag}` — all 5 gameplay senders confirmed:
  `TSInit` @0x82a4c (idx,0 broadcast, when clearing iuser1/2), `StartObserver` @0x89634
  (idx,1 broadcast at end), `StopObserver` @0x89a14 (idx,0 broadcast),
  `ForceClientDllUpdate` @0x82594 (MSG_ONE idx, deadflag==3),
  `CTSGameRules::InitHUD` @0x69134 (per-connected-player idx, iuser1!=0, MSG_ONE to joiner).
  The client handler (client.dll `FUN_10020110` → `FUN_10065250`, inv_D file) parses
  exactly BYTE idx (1..64) + BYTE flag into a per-player spectator array + local camera
  fixes. **Our BYTE state + BYTE seconds wire is refuted — must revert to (idx, flag).**
- **Enter spectate** = `CBasePlayer::StartObserver(Vector& origin, Vector& angles)` @0x89634:
  flush TSCash; clear iuser4/slowmotion; temp-ent 0x7d; if active CTSGun: Reset+HolsterWeapon;
  DropFlag; release +0x9c4 EHANDLE (Use(…,0)); SetSuitUpdate(0); `TSActiveItems{idx,0}`;
  FOV 0 + SetFOV msg; effects=EF_NODRAW; view_ofs=0; angles=v_angle; fixangle; solid=NOT;
  takedamage=NO; movetype=NONE; clear PFLAG_DUCKING & FL_DUCKING; deadflag=DEAD_RESPAWNABLE;
  health=1; m_fInitHUD=1; TeamInfo broadcast; RemoveAllItems(1); SetOrigin;
  `Observer_SetMode(+0x76c ?: 2)`; `Spectator{idx,1}`.
  Callers: PlayerDeathThink @0x7eaec (after `TSCreateDeadCorpse(this,10.0)`, on button press
  or `+0x768` timeout, iuser1==0), CTSGameRules::RestartRound @0x67920,
  CTSGameRules::InitHUD @0x69134, CHalfLifeTeamplay::ChangePlayerTeam @0xd488c,
  StartDeathCam @0xc9518.
- **Spectate think** = the `pev->iuser1 != 0` block of `CBasePlayer::PreThink` @0x808cc
  (region ~0x81458): 1 Hz HUD messages — while `+0x770` (m_flTSRespawnTime) > time:
  countdown `"%i"` (clamped 30) or `"Press Fire To Play!"`; while `+0x7bc` (round respawn
  window) active: `"%i seconds left for respawn, press Fire to Play!"`, and when it expires:
  `+0x7bc=0; +0x770=-4.0f; +0x74c=1; "You have to wait until the next round."` +
  `CTSGameRules::CallCheckDeads()`. Then `Observer_HandleButtons` @0x89d34 (jump=mode cycle,
  duck/next=FindNextPlayer) and `Observer_Idle` @0x89ca4 (retarget dead/spectating target);
  `pev->impulse=0; return;`.
- **Try respawn** = the **"respawn" console command** in `ClientCommand__FP7edict_s`
  @0x9d9fc (disasm in dossier notes): requires `pev->iuser2 != 0`; if `+0x770 < 0` → 1 Hz
  "You have to wait until the next round."; else requires `0 < +0x770 < gpGlobals->time`,
  then `+0x770 = 0; StopObserver(); pev->impulse = 0;`.
  `StopObserver` @0x89a14 (its ONLY caller is this command): gate `+0x770 <= 0 || <= time`;
  `+0x76c = iuser1` (remember mode); `Spectator{idx,0}`; iuser1=iuser2=0; button=0;
  `+0xb34=0`; `respawn(pev, FALSE)`; nextthink=-1.
  The CLIENT sends it: client.dll CHudSpectator fn @0x10023450 — on IN_ATTACK while
  spectating (0.2 s ratelimit) → `pfnClientCmd("respawn")`. So "fire to play" = client cmd.
- **`+0x770` producers**: `SetPlayerRespawnTime(player, secs, bool)` @0x65ff4 — in team/CTB
  modes or non-LMS: `+0x770 = time+secs`; in LMS DM: respawnnumber-capped
  (`+0x7ba` respawn count vs cvar, sentinel `-4.0f` = wait-next-round) and window
  `+0x7bc = time + min(roundEndLeft,10)`; sets `+0x74c` (PAward) = 1 (waiting) or 0xb.
  Also set directly: RestartRound (`+0x770 = time+3`, `+0x7bc = time+10`, `+0x74c=0xb`),
  CTSGameRules::InitHUD (`SetPlayerRespawnTime(player, clamp(respawntime cvar,5,30), false)`),
  ChangePlayerTeam (`SetPlayerRespawnTime(player, 5, true)`).
  Consumers: the PreThink block, StopObserver, the respawn command, and
  `CTSGameRules::CheckDeads` @0x69c00 (counts a player "in" if alive **or `+0x770` ≥ 0**).

**VERDICT**: TSEnterSpectate → MOVE into StartObserver (original name/shape).
TSSpectateThink → MOVE into the PreThink iuser1-block + Observer_* methods.
TSTryRespawn → MOVE into the ClientCommand "respawn" handler + StopObserver.
Spectator wire → fix to (BYTE idx, BYTE flag).
**CONFIDENCE**: high.

### B3. TSSendBuyMenu / TSMenuSelect / TSCloseMenu
**EVIDENCE**:
- `gmsgVGUIMenu` has exactly ONE gameplay sender (GOT refs: 0x9e473 + registration):
  the **"changeteam"** command inside `ClientCommand__FP7edict_s` @0x9d838 → 
  `VGUIMenu{BYTE 2}` (MSG_ONE). There is **no buy-menu send**: the buy menu is entirely
  client-side; purchases arrive as console commands — dispatch strings in the global
  ClientCommand: `say, say_team, fov, use, respawn, powerup, mj, cashala, weapon_*,
  lasclr_*, rst_cfg, tkw, tki, tka, cutwire_*, drop_*, drop, usepowerup, firemode,
  useitems, lastinv, changeteam, jointeam, motd, fullupdate`.
- **"menuselect"** is handled in `CHalfLifeMultiplay::ClientCommand` @0xbeee8 and
  `CHalfLifeTeamplay::ClientCommand` @0xd43c8 (inv_B3C5 file): both do
  VoiceGameMgr first, then `strcmp("menuselect")`, parse `atoi(argv(1))` and **discard the
  value** — a dead swallow that just returns TRUE (stock-SDK leftover shape).
- **"jointeam N"** (global ClientCommand @0x9e4b0): `name = rules->GetIndexedTeamName(N−1)`
  (vt+0xd4) then `rules->ChangePlayerTeam(player, name, …)` (vt+0xdc).
- No "closemenu"/TSCloseMenu message or command exists anywhere in the binary (no gmsg, no
  dispatch string).

**VERDICT**: TSSendBuyMenu → DELETE (no server-side buy menu; keep only the changeteam →
VGUIMenu{2} send in ClientCommand). TSMenuSelect → MOVE (the dead-swallow menuselect branch
in the gamerules ClientCommand overrides — keep it byte-faithful, i.e. swallow). TSCloseMenu
→ DELETE.
**CONFIDENCE**: high.

### B4. TSSendScoreInfo
**EVIDENCE** — no TS helper exists. `gmsgScoreInfo` senders (all inline, GOT refs resolved):
- `CBasePlayer::AddPoints` @0xc9e18 (player.cpp, stock) — the canonical helper the original
  DOES have. Payload (inv_B4_AddPoints.c): **broadcast {BYTE idx, SHORT round(frags),
  SHORT deaths(+0xb30), SHORT 0 (playerclass), SHORT teamIndex+1}** where teamIndex =
  `g_pGameRules->vt+0xd0(teamName)`.
- `CHalfLifeMultiplay::PlayerKilled` @0x795e4 (×2: victim, then killer if Classify==2).
- `CHalfLifeMultiplay::InitHUD` @0xbf618 (×2, per-player loop to the joiner).
- `CTSGameRules::CheckDeads` @0x69c00 (last-man-standing +10 frags refresh).
- `CHalfLifeTeamplay::ChangePlayerTeam` @0xd488c.
- TS entities: `CTSDMHill::WaitTouch` @0x61be8 (×2), `CTSBriefcase::CaptureTouch` @0x7db9c,
  `CTSFlag::PlayerTakeFlag` @0x8e330.

**VERDICT**: DELETE our helper; MOVE the sends inline to those sites (AddPoints is the only
named sender and it is stock CBasePlayer, not CTSGameRules; the json's TS_entities.cpp
homing covers the DMHill/Briefcase/Flag inline sites).
**CONFIDENCE**: high.

---

## C. CTSGameRules group

### C1. The original CTSGameRules method set
From `nm | grep 12CTSGameRules` (42 methods): ctor, Think, RespawnEntities(b),
UpdateTeamScores(ii), RestartRound, Init, IsCTB, InitTeams, SetTeamList(PCc),
SetTeamModelList(PCc), SetTeamModelList(iPCc), GetTeamsNum, InitObjectives,
ResetRestartRound(i), TeamWin(iibT3), RoundTimeLeft, SetObjective, InitTeamScore(Pi),
InitHUD(P11CBasePlayer), SetGlobalSlowMotion(ff), TogglePlayerFromSlow, GlobalSlowMotion2,
GetPlayerModel, GetTeamCash, **GetTeamIndex(PCc)**, CallCheckDeads, CheckDeads,
CountPlayers, PlayerVote, CheckVotes, ClearObjectiveList, AddObjectiveInList,
SendObjectiveToHud, SendObjectiveList, FindIndexByObjective, ShowObjectiveOnHud,
SelectRandomTheOne, SetPlayerTheOne, TheOneIsDead, GetRandomPowerup, PutPowerUp.
Of our audited names, only **GetTeamIndex** is genuinely a CTSGameRules method
(@0x69b04 — strcasecmp against the 4 team-name slots +0x28/+0xb4/+0x140/+0x1cc,
returns 0..3 or −1). Everything else has no CTSGameRules counterpart.

### C2. PlayerKilled
**EVIDENCE** — the kill-tier tail @0x7a908 is inside
**`CHalfLifeMultiplay::PlayerKilled(CBasePlayer*, entvars_t*, entvars_t*)` @0x795e4**
(TS_multiplay_gamerules.cpp — the TS build patches the stock multiplay method; there is no
CTSGameRules::PlayerKilled). Full decompile in inv_BC file. Structure: points =
`IPointsForKill` (vt+0x6c) → `TSGetPointsForFrag` @0x79140 (headshot/etc flags out);
own-weapon-kill detect (victim's own gun, GetWeaponType not 2/3/0xb, not closecombat, killer
weapon==inflictor); doublefrag window (killer +0x748, <2 s); kill-count tiers on killer
`+0x744`: 3=is_killer (+30 hp, PAward 2), 6=is_demolition (+60 hp, PAward 3),
9=is_specialist (150 hp min, superjump+lowgrav physics, PAward 4/10, powerup 8 for 2),
10+=is_unstoppable (points×2); stopped_fury bonus (+5, victim `+0x744`>8); killed-name
buffer with weapon/"Kung Fu"/"* Stock"/"Throwing Knife"/"Shrapnel"/"M61 Grenade"
substitutions; DeathNotice (vt+0x74); UTIL_LogPrintf suicide/killed lines; game_playerdie /
game_playerkill FireTargets; TheOneIsDead; cash: killer `500*points+400` (−1000 on TK),
victim +900 (both ×? +1000/+... in gamemode ≥2); slowmatch powerup grant; TK
kick/ban (`banfortk`, 3 TKs/30 s window, victim-side counter +0x7c4/+0x7c8);
`ScoreInfo` for victim + killer; `CTSGameRules::CallCheckDeads()`.
**VERDICT**: MOVE — our CTSGameRules::PlayerKilled logic belongs on
CHalfLifeMultiplay::PlayerKilled. **CONFIDENCE**: high.

### C3. PlayerSpawn
**EVIDENCE** — `CHalfLifeMultiplay::PlayerSpawn(CBasePlayer*)` @0x790ac (thunk → local body
@0x790b8, disasm-resolved strings): `pev->weapons |= 0x80000000;` Touch() every
**"game_player_equip"** entity; if none existed: `GiveNamedItem("weapon_tsgun");
BuyArmament();` and `+0x53c = +0x540` (spawn-cash snapshot). Per-spawn TS state reset
(slowmotion, powerups, rendermode fade, TSSpace=81, wires, cheat-check, …) is
**`CBasePlayer::TSInit` @0x82a4c** (inv_BC file), and joins/round-entry go through
`CTSGameRules::InitHUD` @0x69134 (server-settings msgs, spectator sync, StartObserver,
respawn-timer arm, per-player CTSGameRules slot reset).
**VERDICT**: MOVE — split our PlayerSpawn logic between CHalfLifeMultiplay::PlayerSpawn
(equip) and the existing TSInit/InitHUD homes. **CONFIDENCE**: high.

### C4. SendRoundTime / StartRoundTimer
**EVIDENCE** — `gmsgTSRoundTime` wire = **{BYTE seconds, BYTE state}**, state 0 = running,
2 = round over/frozen. All senders (GOT refs resolved, decompiles in inv_C file):
- `CTSGameRules::RestartRound` @0x67920 — **the arm site**: `m_flRoundEnd(+8) =
  time + clamp(roundtime cvar, 30, 180)`, `m_flRoundRestart(+0xc) = 0`; if `+0x10` (round
  mode flag): broadcast {secsLeft, 0}. Also: InitObjectives/SetObjective, per-player
  carry-over cash valuation of kept weapons/attachments/ammo, respawn timers (+3 s /
  window 10 s), powerup wipe, StartObserver for live players,
  `CHalfLifeTeamplay::RecountTeams(g_pGameRules, false)`, RespawnEntities(true).
- `CTSGameRules::ResetRestartRound(int)` @0x68750 — `m_flRoundRestart = time+1,
  m_flRoundEnd = 0`; broadcast {round(m_flRoundRestart−time), 0}; wipes team scores/kills,
  resets team cash to initial_cash, TeamScore msgs; "Restarting Round".
- `CTSGameRules::TeamWin` @0x68a08 — broadcast {round(m_flRoundEnd−time), **2**} (unless
  scored-flag), then cashforwin/cashforlose, TSVoiceMessage win/lose per player, team score
  ++ + TeamScore msgs, "%s Win!"/"%s Score!"/"Game Draw".
- `CTSGameRules::CheckDeads` @0x69c00 — LMS branch: broadcast {round(m_flRoundEnd−time),
  **2**} with `m_flRoundRestart = time+3`.
- `CTSGameRules::InitHUD` @0x69134 — MSG_ONE {secsLeft, 0} to the joiner.
- `CBasePlayer::UpdateClientData` @0x81d00 — MSG_ONE {secsLeft, 0} in the InitHUD block.
**VERDICT**: SendRoundTime/StartRoundTimer → DELETE as helpers; MOVE the sends/arms inline
to those six sites with the {seconds, state} wire. **CONFIDENCE**: high.

### C5. Team machinery
**EVIDENCE** — TS teams ride the **stock CHalfLifeTeamplay** object (g_pGameRules) with
TS patches, PLUS CTSGameRules's own 4-slot team table (stride 0x8c: name @+0x28+i*0x8c,
spawn-spot classname @+0x68…, models, cash/score/kills @+0x1c/+0x20/+0x24…):
- `CTSGameRules::InitTeams` @0x67f60: defaults Team1..4 / info_player_team%i /
  defTeamModels; in gamemode 2/3 (byte @rules+0): models "seal"/"merc",
  `SetTeamList("Specialists;Mercenaries")`, team1 spawns = info_player_start.
- `CTSGameRules::SetTeamList` @0x680dc: strtok ';' into the name slots (theOne mode inserts
  "The ONE" as team 0, cap 2; else cap 4… `created team %s`).
- Assignment of a joining/changing player:
  - `CHalfLifeTeamplay::SetDefaultPlayerTeam` @0xd44ac (TS-patched): gamemode <2 → team =
    infokey "team" (or theOne-mode names from TSGameRules+0xb4/+0x28); RecountTeams(true);
    invalid/empty + defaultteam==0 → `TeamWithFewestPlayers()`.
  - `CHalfLifeTeamplay::ChangePlayerTeam` @0xd488c (TS-patched): optional suicide (gib/kill
    damage), theOne override, invalid → TeamWithFewestPlayers; **sets infokey "model" via
    `CTSGameRules::GetPlayerModel` @0x69a08 in gamemode 2/3**, SendObjectiveList; sets
    infokey "team"; RecountTeams(false); TeamInfo + ScoreInfo broadcasts; then (not kill,
    not theOne) **StartObserver + SetPlayerRespawnTime(player, 5, true)**; TeamNames msg.
  - Command surface: "jointeam N" → GetIndexedTeamName(N−1) + ChangePlayerTeam (global
    ClientCommand @0x9e4b0); "changeteam" → VGUIMenu{2}; model changes policed by
    `CHalfLifeMultiplay::ClientUserInfoChanged` @0x7b244 (TS-patched: PlayerIsCheating check
    forces cl_lw 1; non-team model → revert + "HLDM models not allowed").
- Stock methods that exist and carry the logic (all `17CHalfLifeTeamplay`):
  `RecountTeams(bool)` @0xd5608, `TeamWithFewestPlayers` @0xd5510, `IsTeamplay` @0xd5200,
  `GetTeamID(CBaseEntity*)` @0xd53f4 (+ CHalfLifeMultiplay::GetTeamID @0xc0d9c W),
  `GetIndexedTeamName(int)` @0xd54a0, `IsValidTeam` @0x7b140 (TS-patched, in
  TS_multiplay_gamerules.cpp).

Per-method verdicts:
| ours | verdict | original home |
|---|---|---|
| AssignToTeam | MOVE | CHalfLifeTeamplay::SetDefaultPlayerTeam @0xd44ac + ChangePlayerTeam @0xd488c |
| GetIndexedTeamName | MOVE | CHalfLifeTeamplay::GetIndexedTeamName @0xd54a0 (stock) |
| GetTeamID | MOVE | CHalfLifeTeamplay::GetTeamID @0xd53f4 / CHalfLifeMultiplay @0xc0d9c |
| GetTeamIndex | KEEP (not invented) | CTSGameRules::GetTeamIndex @0x69b04 |
| IsTeamMode | DELETE | no original — gamemode byte (rules+0) compared inline; IsCTB @0x67f08 exists for mode 3 |
| IsTeamplay | MOVE | CHalfLifeTeamplay::IsTeamplay @0xd5200 (stock; g_teamplay global) |
| RecountTeams | MOVE | CHalfLifeTeamplay::RecountTeams(bool) @0xd5608 (stock) |
| TeamWithFewestPlayers | MOVE | CHalfLifeTeamplay::TeamWithFewestPlayers @0xd5510 (stock) |
**CONFIDENCE**: high.

---

## D. Client-side wire contracts

### D1. draw.sc (client handler FUN_100425b0 + FUN_10007110)
**EVIDENCE** (inv_D file) — event handler reads `args->iparam1` (weapon id, → weapon-info
row), `args->iparam2` (**a 4-bit mask**), `args->entindex`. For the local player it plays
the draw animation: base draw anim from the model + `FUN_10007110(id, mask)` which sums
per-attachment draw-anim offsets from the client weapon-info table:
bit 1 → +0x178 short, bit 2 → +0x17a, bit 4 → +0x17c, bit 8 → +0x17e, with a combined
(bit2|bit4) entry +0x180 used when ≠ −1; weapon 0x24 gets +1 when bit 8 is clear.
So **iparam2 = the active-attachment mask** — the same value as
`CTSGun::GetActiveItems()`'s low bits (`m_pActiveItem->attachActive`; note GetActiveItems
additionally ORs 0x40 for akimbo, which the draw handler never reads).
**VERDICT**: our server iparam2 = "active barrel mask" is CONFIRMED in substance iff our
mask == attachActive bits 1/2/4/8 (it is an attachment/active-items mask, not a per-barrel
clip selector; the 0x40 akimbo bit is TSActiveItems-only). Keep the member, name it after
attachActive semantics. **CONFIDENCE**: high.

### D2. "Spectator" client handler
**EVIDENCE** (inv_D file) — hook registered at client.dll 0x100202dc:
`HookUserMsg("Spectator", 0x10020110)`; the handler defers to `FUN_10065250`:
`BEGIN_READ; idx = READ_BYTE(); if (idx<1||idx>64) return 1; flag = READ_BYTE();
g_PlayerSpectator[idx] = flag;` and if idx == local player: sets the local is-spectator
bool (= flag==1), third-person/camera fixes, and mirrors the flag into
extra-player-info[idx]. **Payload = (BYTE playerIndex, BYTE flag)** — settles B2: the
original is the stock (idx, flag) broadcast.
**VERDICT**: fix our wire to (idx, flag). **CONFIDENCE**: high.

---

# ORCHESTRATOR SUMMARY

| our method | verdict | original home (fn @nm addr) |
|---|---|---|
| CTSGun::GetWeaponData | MOVE | free `GetWeaponData(edict*,weapon_data_s*)` @0xa00ac (client.cpp); loops ALL items, stock-shaped; NO barrel-clip fuser packing |
| CTSGun::SetWeaponData | DELETE (server) / ACCEPT (cl_dll prediction receive only) | none in binary |
| CTSGun::SendWeaponStatus | MOVE | CTSGun::UpdateClientData @0x758c8 (diff loop, WStatus{id,status,clip}) |
| CTSGun::CanDeploy | ACCEPT/fold | none — inline in SelectWeapon @0x70fb8 / Deploy @0x71030 |
| CTSGun::LoadActiveFromItem | ACCEPT/fold | none — inline snapshot in UpdateClientData @0x758c8 |
| CTSGun::SyncActiveToItem | ACCEPT/fold | none — items[] written directly in TakeWeapon @0x70a04 etc. |
| CTSGun::UpdateActivePointers | ACCEPT/fold | none — set in Deploy/SelectWeapon paths |
| CTSGun::OwnedCount | DELETE | none — TSSpace is stored counter player+0x548 |
| CTSGun::SlotsUsed | DELETE | none — counter mutated in PickUpWeapon @0x8314c (+ GetSpeedBySlots @0x66bd4) |
| CBasePlayer::TSUpdateClientData | MOVE | CBasePlayer::UpdateClientData @0x81d00 (send order documented in B1) |
| CBasePlayer::TSEnterSpectate | MOVE | CBasePlayer::StartObserver @0x89634 |
| CBasePlayer::TSSpectateThink | MOVE | PreThink @0x808cc iuser1-block + Observer_HandleButtons @0x89d34 / Observer_Idle @0x89ca4 |
| CBasePlayer::TSTryRespawn | MOVE | "respawn" cmd in ClientCommand @0x9d9fc → StopObserver @0x89a14 (client sends cmd on +attack @cl 0x10023450) |
| CBasePlayer::TSSendBuyMenu | DELETE | none — buy menu is client-side; only "changeteam"→VGUIMenu{BYTE 2} @0x9e473 |
| CBasePlayer::TSMenuSelect | MOVE | dead-swallow "menuselect" in CHalfLifeMultiplay::ClientCommand @0xbeee8 / Teamplay @0xd43c8 |
| CBasePlayer::TSCloseMenu | DELETE | none |
| CBasePlayer::TSSendScoreInfo | DELETE | inline sends; canonical helper = CBasePlayer::AddPoints @0xc9e18 {idx,frags,deaths,0,team+1} |
| CTSGameRules::PlayerKilled | MOVE | CHalfLifeMultiplay::PlayerKilled @0x795e4 (kill tiers @0x7a908 inside it) |
| CTSGameRules::PlayerSpawn | MOVE | CHalfLifeMultiplay::PlayerSpawn @0x790ac (equip: game_player_equip / weapon_tsgun+BuyArmament); state reset = TSInit @0x82a4c |
| CTSGameRules::SendRoundTime | DELETE | inline at 6 sites; wire {BYTE secs, BYTE state(0 run/2 over)} |
| CTSGameRules::StartRoundTimer | DELETE | arm inline in RestartRound @0x67920 (m_flRoundEnd = time+clamp(roundtime,30,180)) |
| CTSGameRules::AssignToTeam | MOVE | CHalfLifeTeamplay::SetDefaultPlayerTeam @0xd44ac + ChangePlayerTeam @0xd488c |
| CTSGameRules::GetIndexedTeamName | MOVE | CHalfLifeTeamplay::GetIndexedTeamName @0xd54a0 |
| CTSGameRules::GetTeamID | MOVE | CHalfLifeTeamplay::GetTeamID @0xd53f4 / CHalfLifeMultiplay @0xc0d9c |
| CTSGameRules::GetTeamIndex | KEEP | CTSGameRules::GetTeamIndex @0x69b04 (exists — not invented) |
| CTSGameRules::IsTeamMode | DELETE | none — inline gamemode-byte checks; IsCTB @0x67f08 |
| CTSGameRules::IsTeamplay | MOVE | CHalfLifeTeamplay::IsTeamplay @0xd5200 |
| CTSGameRules::RecountTeams | MOVE | CHalfLifeTeamplay::RecountTeams(bool) @0xd5608 |
| CTSGameRules::TeamWithFewestPlayers | MOVE | CHalfLifeTeamplay::TeamWithFewestPlayers @0xd5510 |
| (D1) barrelMask member | KEEP w/ rename | draw.sc iparam2 = attachActive mask bits 1/2/4/8 (cl FUN_10007110) |
| (D2) Spectator wire | FIX | {BYTE playerIdx, BYTE flag} both ends (sv 5 senders; cl handler @0x10065250) |

UNRESOLVED / caveats:
- What writes TS-CBaseEntity fields +0x4c..+0x5c (the fuser2/3, iuser1..3 sources in
  GetWeaponData) was not found in weapon/client.cpp code — likely never written for CTSGun
  (stock-shaped dead packing). A memset-style writer elsewhere can't be 100 % excluded.
- `wd.m_fInSpecialReload ← pw+0x98`: 0x98 is one slot before m_flNextPrimaryAttack (0x9c);
  it is plausibly m_fInSpecialReload in the gcc2 layout but the name is inferred, not
  symbol-proven.
- CTSGun::Deploy @0x71030 was not decompiled this session (assumed the m_pActiveItem/
  m_pActiveInfo setter per call-shape); decompile before folding UpdateActivePointers.
