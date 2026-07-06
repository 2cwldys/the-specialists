# cont89 — Teamplay / scoreboard "#Spectators" / blank-yellow team box (ts_bikini, DM)

**Symptom (original client → our server, DM map ts_bikini):** the client shows the server as
*teamplay* — a blank team indicator with a **yellow** background above the health HUD, and the
scoreboard groups **every** player under a single **"#Spectators"** header.

**Bottom line (RE-verified):** the *entire* symptom is driven by ONE client field —
`CHud::m_Teamplay` (`client.dll` `gHUD+0x5c` = `0x100cf97c`) — which is set by **exactly one wire
event: the `GameMode` user-message byte**. Team names, ScoreInfo team columns, the "team" infokey
and the Spectator message are **NOT** involved in the teamplay decision (proven below). The
original TS ships `game.cfg` with **`mp_teamplay 1`**, so the stock server sends `GameMode = 1` on
every map (DM included) — the "#Spectators/yellow-box" look is the **original's genuine behaviour**
(live-confirmed). Our server instead sends `GameMode 0` (built-binary verified). So the real
divergence is a **single field: the GameMode byte** — everything else (TeamInfo `""`, ScoreInfo
team 0, no team infokey in DM) is already faithful.

---

## 1. The client mechanism (client.dll — decompiled)

### 1.1 `m_Teamplay` is set ONLY by `GameMode`
- `MsgFunc_GameMode` → `CHud::MsgFunc_GameMode` (`FUN_10021a10`):
  `*(uint*)(gHUD + 0x5c) = READ_BYTE();`  → stores the **raw byte** (not `==1`).
- Exhaustive search of `client.dll` for writes to `0x100cf97c` (absolute + all `mov …,0x5c(reg)`
  forms): the **only** writer is the GameMode handler. `gHUD` lives in `.data` BSS tail → default
  **0**. So a fresh client is `m_Teamplay=0`; it only becomes non-zero when a server sends
  `GameMode` with a non-zero byte, and stays there (never reset on connect / level change).

### 1.2 Both offending HUD elements gate on `m_Teamplay != 0`
- **Scoreboard sort dispatcher** `FUN_1005d5f0`:
  `if (m_Teamplay == 0) SortPlayers(this,0,NULL);  else SortTeams(this);`
  - `SortPlayers(…,iTeam=0,team=NULL)` (`FUN_1005d8f0`, decompiled) → **flat list, no header rows**.
  - `SortTeams` (`FUN_1005d6a0`) draws real teams, then
    `SortPlayers(TEAM_SPECTATORS /*=2*/, NULL)` → dumps **every player with an empty teamname into
    one "#Spectators" bucket**. In DM all teamnames are empty ⇒ **everyone under "#Spectators"**.
- **Team box above health** — TS status Draw (`client.dll` ~`0x1004a5f0`, the fn holding the
  `#HUD_FreeSlots` string ref at `0x1004a8b8`):
  `if (DAT_100cf97c /*m_Teamplay*/ != 0) { … draw box using teamname[local] + teamnumber[local] … }`
  It reads the local player's `teamname` (`&DAT_101901fe + idx*0x20`, **blank** in DM) and
  `teamnumber` (`&DAT_101901fc[idx*0x10]`, **0** in DM). `iTeamColors[0]` is the gold/**yellow**
  slot ⇒ **blank text + yellow background** = exactly the reported box.

So: **team box + "#Spectators" grouping ⟺ `m_Teamplay != 0` ⟺ a `GameMode` byte ≠ 0 was received.**

### 1.3 The client stores per-player extra info as (base `0x101901f4 + idx*0x20`)
| off | field | set by |
|----|-------|--------|
| +0x01 | spectator flag | `MsgFunc_Spectator` (`FUN_10065250`) |
| +0x02 | frags | `MsgFunc_ScoreInfo` (`FUN_10065090`) |
| +0x04 | deaths | ScoreInfo |
| +0x06 | playerclass | ScoreInfo (11 → "WAITING" text) |
| +0x08 | teamnumber (short, clamped ≥0) | ScoreInfo |
| +0x0a | teamname[16] | `MsgFunc_TeamInfo` (`FUN_100651c0`) |
None of these touch `m_Teamplay`. `teamnumber` only picks the team **colour**; `teamname` only the
scoreboard grouping key — and only when `SortTeams` runs, i.e. only when `m_Teamplay != 0`.

---

## 2. What the ORIGINAL server sends for a DM player (ts_i386.so — decompiled)

### 2.1 GameMode — the original DOES make it teamplay, via config
- `gmsgGameMode` is referenced **only** by `CHalfLifeMultiplay::UpdateGameMode` (writes `0`) and
  `CHalfLifeTeamplay::UpdateGameMode` (writes **`g_teamplay`**). `UpdateGameMode` is a vtable slot
  reached from `CHalfLifeMultiplay::InitHUD`.
- **`InstallGameRules__Fv` (0xb6050) execs `game.cfg`**, then for `deathmatch != 0`:
  `g_teamplay = DecideGamePlay(mapname, teamplay.value>0?1:0)`; if `g_teamplay ≥ 1` it installs the
  **teamplay** rules branch (`new CHalfLifeTeamplay`-family, 0x218), else the plain multiplay branch.
- **TS's shipped `ts/game.cfg` contains `mp_teamplay 1`.** So `teamplay.value = 1` →
  `DecideGamePlay` returns 1 for *every* map (incl. DM ts_bikini, which has no `mp_theonemode` /
  `lastmanstanding` override) → `g_teamplay = 1` → teamplay rules → `UpdateGameMode` sends
  **`GameMode = 1`**.
- **LIVE CONFIRMATION:** read the running original (`hl.exe`, ts_bikini) — `m_Teamplay = 1`, both
  players `teamname=""`, `teamnumber=0`, `playerclass=0`. i.e. the original genuinely renders the
  teamplay scoreboard with everyone under "#Spectators" and a blank/yellow team box on ts_bikini.
  This is *shipped behaviour*, not a defect in the original.

### 2.2 TeamInfo — original sends it, value `TeamID()` = `""` in DM
- `CBasePlayer::StartObserver` (0x89634, run for every joiner) sends
  `MSG_ALL gmsgTeamInfo{ idx, TeamID() }`. `CBasePlayer::TeamID` (0xcb0ac) returns `m_szTeamName`
  (player+0xb48) → **`""` in DM**. Also sends `gmsgSpectator{idx,1}` and `gmsgTSActiveItems{idx,0}`.
- **Ours matches:** our `StartObserver` (player.cpp:1378) sends `gmsgTeamInfo{ idx, "" }` (hard-coded
  `""`; functionally identical to `TeamID()==""` in DM).

### 2.3 ScoreInfo / team column — original's InitHUD sends NONE of the stock rows
- The original `CTSGameRules::InitHUD` (0x69134) is **fully custom**: it sends TSServerSetting×4,
  TSRoundTime, a per-player `gmsgSpectator{idx,iuser1!=0}` catch-up loop, then `StartObserver` +
  respawn/armament setup. It **does NOT** call `CHalfLifeMultiplay::InitHUD`, so it sends **no
  GameMode, no join `ScoreInfo{idx,0,0,0,0}`, and no per-player ScoreInfo loop**.
- ScoreInfo for a DM player is sent elsewhere (spectate-enter/leave + AddPoints), always with
  team column = `TeamID`-index = **0** in DM.

### 2.4 "team" infokey — NOT set for a DM player, either side
- Original `StartObserver`/DM spawn path: **no** `pfnSetClientKeyValue("team",…)`.
- Ours: the only `SetClientKeyValue("team",…)` is in `CTSGameRules::AssignToTeam`
  (TS_gamerulesclass.cpp:997), and `AssignToTeam` is **team-mode-only** (`PlayerSpawn` returns early
  `if (!IsTeamMode())`). ts_bikini is pure DM — verified: its entity lump has **no
  `ts_objective_manager`** (only `info_player_deathmatch` ×20, `ts_powerup` ×16) → `IsTeamMode()`
  is false → `AssignToTeam` never runs → **we set no team infokey in DM.** No divergence here.

---

## 3. What OUR server sends (built hl.so verified) — the single divergence

Our `CTSGameRules : public CHalfLifeMultiplay` and does **not** override `UpdateGameMode`. Our
`CTSGameRules::InitHUD` (TS_gamerulesclass.cpp:1076) **chains `CHalfLifeMultiplay::InitHUD`**, which
calls `UpdateGameMode(pl)` → `CHalfLifeMultiplay::UpdateGameMode`. Disassembly of the built
`build-amd64/dlls/hl.so` `_ZN18CHalfLifeMultiplay14UpdateGameModeE`:
```
mov  $0x1,%edi           ; MSG_ONE
call pfnMessageBegin(gmsgGameMode)
xor  %edi,%edi           ; -> 0
call pfnWriteByte(0)     ; <-- GameMode byte = 0  (individual)
jmp  pfnMessageEnd
```
So **our server sends `GameMode = 0` unconditionally**, regardless of `g_teamplay`/`m_iGameMode`.
(Our repo also ships **no `ts/game.cfg`** — only `README.md`+`liblist.gam` — so on our server
`mp_teamplay` defaults 0 and `g_teamplay=0` anyway.)

### Original vs ours — the DM contract, field by field
| wire item | ORIGINAL (ts_bikini, mp_teamplay 1) | OURS | match? |
|---|---|---|---|
| **GameMode** | **1** (`g_teamplay`, via teamplay rules) | **0** (literal, `CHalfLifeMultiplay`) | **NO — the only divergence** |
| TeamInfo (join/observer) | `{idx, TeamID()=""}` | `{idx, ""}` | yes |
| ScoreInfo team column | 0 | 0 | yes |
| ScoreInfo join/loop in InitHUD | none (custom InitHUD) | stock `{…,0,0}` sent (extra, but team=0) | harmless |
| "team" infokey (DM) | not set | not set | yes |
| Spectator, playerclass(11=WAITING) | sent | sent | yes |

---

## 4. Root cause

The team box + "#Spectators" grouping are a **pure function of `m_Teamplay`**, and `m_Teamplay` is a
pure function of the **`GameMode` byte**. TeamInfo/ScoreInfo/infokey are red herrings — both servers
send an empty team for a DM player; those fields only choose colour/label *once teamplay is on*.

- The **original** shows teamplay+"#Spectators"+yellow box on ts_bikini because **`ts/game.cfg`
  ships `mp_teamplay 1`** and the stock rules emit **`GameMode = g_teamplay = 1`**. (Live-verified.)
- **Our** server emits **`GameMode = 0`** (built-verified) because `CTSGameRules` inherits
  `CHalfLifeMultiplay::UpdateGameMode` (literal 0) instead of the teamplay one, so it can never
  reproduce the original's byte — our server renders **individual/flat** on ts_bikini.

**These are opposite.** The user's report ("teamplay on our server") is therefore *not* caused by
our GameMode send (which forces individual). The most likely explanation is **stale client state**:
`m_Teamplay` is never reset on the client, so a client that previously held `m_Teamplay=1` (e.g.
from the original server, which sends 1) keeps the teamplay scoreboard until a `GameMode 0` clears
it. If our `GameMode 0` is dropped/late — or if the source-parity restructure removes the
`CHalfLifeMultiplay::InitHUD` chain (matching the original's custom InitHUD, which sends **no
GameMode at all**) — then nothing clears the stale `1` and our server *appears* teamplay. (This is
the `[[map-change-state-reset]]` pattern: client per-connection state not reset; server never
re-broadcasts.)

---

## 5. The concrete fix — choose the target, then it's one field

The lever is the **GameMode byte** and (for full parity) the shipped `mp_teamplay`. Pick the target:

### 5.A Drop-in FAITHFUL to stock TS (reproduce the original's teamplay+"#Spectators")
1. Ship `game/ts/game.cfg` with `mp_teamplay 1` (+ `mp_teamlist "humans;zombies"`,
   `mp_teammodels "seal;agent"`) — matches the original TS install.
2. Make `CTSGameRules` emit the **teamplay** GameMode: add a `CTSGameRules::UpdateGameMode` override
   that writes `g_teamplay` (mirror `CHalfLifeTeamplay::UpdateGameMode`, dlls/teamplay_gamerules.cpp:164),
   instead of inheriting the literal-0 `CHalfLifeMultiplay::UpdateGameMode`
   (dlls/multiplay_gamerules.cpp:358).
   - Files: dlls/ts_gamerules.h (declare override), dlls/TS_gamerulesclass.cpp (define:
     `WRITE_BYTE(g_teamplay ? 1 : 0)`), plus the game.cfg above.
   - Result: original client shows the identical teamplay scoreboard + team box on ts_bikini.

### 5.B Product goal = individual DM (what the task/coordinator asked for; a DEVIATION from stock TS)
Our server **already** sends `GameMode 0` → individual. To make it *robust* (kill any stale
`m_Teamplay=1` a joining client carries, and survive the parity restructure):
1. Keep an explicit `GameMode 0` send on every InitHUD/spawn. If the restructure drops the
   `CHalfLifeMultiplay::InitHUD` chain (TS_gamerulesclass.cpp:1076) to match the original's custom
   InitHUD, **re-add an explicit `UpdateGameMode(pPlayer)` (byte 0)** in `CTSGameRules::InitHUD` so
   the joiner's stale teamplay flag is cleared. (The original omits it only because *it wants
   teamplay* — for individual DM we must actively clear.)
2. Do **not** ship `mp_teamplay 1`; keep `g_teamplay=0` on DM maps.
3. Leave TeamInfo `""` / ScoreInfo team 0 as-is — already correct; they cause nothing on their own.

**Recommended:** because the mission is *drop-in fidelity* and the original genuinely shows teamplay
on ts_bikini (mp_teamplay 1, live-confirmed), **5.A is the faithful answer.** If the desired product
UX is individual DM, that is a deliberate config deviation (mp_teamplay 0) — document it as such, not
as a bug.

### Polarity check to run (definitive A/B)
Connect the original `hl.exe` client to **our** dedicated server on ts_bikini *from a fresh client
launch* (so `m_Teamplay` starts 0), then read `client.dll+0xcf97c`:
- `0` ⇒ our `GameMode 0` landed → individual (our current code path). The user's earlier teamplay
  sighting was stale client state.
- `1` ⇒ our server did **not** clear it (GameMode 0 dropped or not sent) → fix 5.B step 1.
`scripts/live.py` recipe: `base = region_start`; `m_Teamplay` live-addr = `0x100cf97c - 0x10000000 +
<client.dll base>`.

---

## 6. Evidence index (for re-verification)
- Client: `FUN_1005d5f0` (sort dispatch, `m_Teamplay==0?flat:teams`), `FUN_1005d8f0` (flat
  SortPlayers), `FUN_1005db70` (FillGrid, "#Spectators" render), status Draw @≈`0x1004a5f0`
  (team box, `if m_Teamplay!=0`), `FUN_10021a10`/`FUN_10065090`/`FUN_100651c0`/`FUN_10065250`
  (GameMode/ScoreInfo/TeamInfo/Spectator handlers), extra_player_info stride 0x20 @ `0x101901f4`.
- Server: `CTSGameRules::InitHUD` @0x69134 (custom, no GameMode), `CBasePlayer::StartObserver`
  @0x89634 (`TeamInfo{idx,TeamID()}`), `CBasePlayer::TeamID` @0xcb0ac (`m_szTeamName`),
  `CHalfLifeMultiplay::UpdateGameMode` @0xbf5c8 (byte 0), `CHalfLifeTeamplay::UpdateGameMode`
  @0xd4458 (byte `g_teamplay`), `InstallGameRules` @0xb6050 (execs game.cfg; teamplay branch),
  `DecideGamePlay` @0x667ac.
- Config: `<TS>/ts/game.cfg` → `mp_teamplay 1`. ts_bikini.bsp entity lump → no `ts_objective_manager`.
- Built: `build-amd64/dlls/hl.so` `CHalfLifeMultiplay::UpdateGameMode` → `pfnWriteByte(0)`.
- Live: original `hl.exe` on ts_bikini → `m_Teamplay=1`, all players `teamname=""`, `team=0`, `class=0`.
