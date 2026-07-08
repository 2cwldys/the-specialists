# cont.96 — R1 rules-split ground truth (READ-ONLY RE dossier)

Answers the four R1 questions with binary-anchored evidence so the split can be applied
**without re-RE**. Evidence: `reference/ts_i386.so` (nm/objdump/readelf) + typed Ghidra
decompiles (`scripts/ghidra/re.sh`, project `recovered/ghidra/re`). Conventions:
**nm/file vaddr** everywhere unless marked "ghidra" (ghidra = nm + 0x10000). GOT base =
`0x165e10`; the TSGameRules GOT slot is `0x167300` (GOT+`0x14f0`), relocation
`R_386_GLOB_DAT → 0x1681a0 TSGameRules` (readelf -r). Object size **0x5894 = 22,676 bytes**
(matches server_strips TASK 4). Builds on cont.94 `invented_method_folds.md` §0/R1/Y7/Y8/Y9 —
nothing there is re-derived; two of its lines are **corrected** below (object's home TU; the
`IsTeamMode >= 2` interim).

Decompile batches archived this session: `/tmp/re_batch1.c` (InstallGameRules, DecideGamePlay,
CTSGameRules ctor/Init/InitObjectives, static-init, MP::Think/InitHUD/ctor/ChangeLevel,
Teamplay::Think/InitHUD/SetDefaultPlayerTeam/ChangePlayerTeam/ctor, GetPlayerModel) and
`/tmp/re_batch2.c` (InitTeams, SetTeamList, InitTeamScore, TeamWin, RoundTimeLeft,
RecountTeams, Teamplay::CUIC, MP::ClientDisconnected, CanPlayerHearPlayer, CWorld::KeyValue).
Regenerate any of them with `scripts/ghidra/re.sh 0x<nm+0x10000>`.

---

## 0. Executive architecture (one paragraph)

`g_pGameRules` holds a **gutted stock object** — `new CHalfLifeRules` (**4 bytes**, vptr only),
`new CHalfLifeMultiplay` (**0xc bytes**), or `new CHalfLifeTeamplay` (**0x218 bytes**) — whose
virtuals are TS-patched two ways: six methods **relocated** into `TS_multiplay_gamerules.cpp`
(the known six, TS address range) and ~a dozen more **patched in place** in the stock files
(§5 census). ALL TS round/team/objective/vote state lives on the separate **static object
`TSGameRules` @0x1681a0** (no vtable, `base: null`), **defined in `TS_gamerules.cpp`**
(static-init evidence §4.3 — cont.94 G1 said TS_player.cpp; that was wrong), constructed at
static-init (ctor only constructs the embedded CBulletManager @+0x1864) and (re)initialized by
`CTSGameRules::Init` @0x67d6c called from the **rules-object ctors** and from
**MP::ChangeLevel**. Stock code reaches it via the GOT pointer and calls its methods
**non-virtually** (direct PLT calls — this is why every cross-object link is statically visible).

---

## 1. Q1 — the round-Think driver

### 1.1 The facts

- `0x67204` **IS the entry of `Think__12CTSGameRules`** (nm `T` symbol, TS_gamerulesclass.cpp
  per funcfile.tsv) — it is not inside some other function. Full decompile already archived:
  `reference/dossiers/cont91/decompiles/tsthink.c` (globalslow driver, CheckDeads timer,
  powerup byte@+1 machine, round restart/end, votes, SrvSett-0 realbullet change-detect —
  the `+0x578` cache is WRITTEN here @0x675f0 and read back by InitHUD, §2.4).
- Per-frame callers (PLT census, exact call sites):
  - **`CHalfLifeMultiplay::Think` @0xbf038 calls it @0xbf302**
  - **`CHalfLifeTeamplay::Think` @0xd412c calls it @0xd439a**
- `StartFrame__Fv` @0x9eaa8 (client.cpp) does **NOT** call it — StartFrame only does the stock
  `g_pGameRules->Think()` virtual (vt slot **+0xc**; its lone `g_teamplay` read @0x9eae4 is
  unrelated). So the chain is: engine → `StartFrame` → `g_pGameRules->Think()` (virtual) →
  TS-patched MP/Teamplay `Think` body → `CTSGameRules::Think(&TSGameRules)` (non-virtual,
  direct call).

### 1.2 MP::Think @0xbf038 (multiplay_gamerules.cpp, patched IN PLACE) — shape

Stock 2003 body with **three TS deltas** (decompile /tmp/re_batch1.c @000cf038):

```c
void CHalfLifeMultiplay::Think() {
    g_VoiceGameMgr.Update(gpGlobals->frametime);          // TS DELTA 1: GLOBAL g_VoiceGameMgr
                                                          // (GOT 0x177330), not a member
    if (g_fGameOver) {                                    // stock intermission block:
        // clamp mp_chattime to [10,120] via CVarSetString("mp_chattime", ...)
        m_flIntermissionEndTime = g_flIntermissionStartTime + mp_chattime.value;  // this+4
        if (time <= that) return;
        if (!m_iEndIntermissionButtonHit /*this+8*/ && time <= g_flIntermissionStartTime+120) return;
        vt[+0xf8] ChangeLevel(); return;                  // (this->vptr slot 0xf8, VTABLES.md)
    }
    // stock timelimit (timelimit.value*60) -> GoToIntermission on hit
    // stock per-player fraglimit scan -> GoToIntermission on hit
    // stock fragsleft/timeleft cvar updates (static locals last_frags_1203/last_time_1204)
    CTSGameRules::Think(&TSGameRules);                    // TS DELTA 2  @0xbf302
    if (TSGameRules.m_szEndMap[0] != '\0')                // TS DELTA 3  @0xbf30a: byte@+0x1824
        vt[+0xfc] GoToIntermission();                     // end the map
}
```

`TSGameRules+0x1824` is a **char buffer** (call it `m_szEndMap[0x40]`), not a flag:
- **Writer:** `CheckVotes` @0x6a5c4 — winning map vote: `strcpy(TSGameRules+0x1824,
  this+0xf88 + winner*0x40)` @0x6abc2-0x6abc8 (vote name table @+0xf88, stride 0x40; counts
  @+0x1788). `Init` @0x67e73 clears byte 0.  // VERIFY: TeamWin/others don't write it (grep
  of `0x1824(%e` showed only these + the readers).
- **Readers:** MP::Think/Teamplay::Think (nonempty → GoToIntermission);
  **MP::ChangeLevel @0xc0784**: if nonempty (@0xc097b) and `strncmp(buf,"nextmap",8) != 0` →
  `strcpy(szNextMap, buf)` (forced next map; the literal value `"nextmap"` means "end map, use
  mapcycle"), then clears it @0xc09be and calls **`CTSGameRules::Init(&TSGameRules)` @0xc09ce**
  (pre-changelevel state reset — cf. the map-change-state-reset memory).

### 1.3 Teamplay::Think @0xd412c (teamplay_gamerules.cpp, patched IN PLACE) — shape

Does **not** delegate to MP::Think except in the game-over case (decompile @000e412c):

```c
void CHalfLifeTeamplay::Think() {
    g_VoiceGameMgr.Update(frametime);
    if (g_fGameOver) { CHalfLifeMultiplay::Think(); return; }   // reuse intermission logic
    // timelimit via CVarGetFloat("mp_timelimit")*60 -> GoToIntermission (label @0xe43af)
    if (fraglimit.value) {
        if (!TSGameRules.IsCTB()) {                       // @0xd4236 TS DELTA: mode gate
            // stock: scan global team_scores[num_teams] vs fraglimit
        } else {
            // TS DELTA: scan the TSGameRules 4-team table instead:
            // for slot i in 0..3 (stride 0x8c): name@+0x28+i*0x8c nonempty ->
            //   compare score int @+0x24+i*0x8c vs fraglimit
        }
    }
    // stock fragsleft/timeleft updates (static locals last_frags_1191/last_time_1192)
    CTSGameRules::Think(&TSGameRules);                    // @0xd439a
    if (TSGameRules.m_szEndMap[0]) GoToIntermission();    // @0xd43a2
}
```

### 1.4 Wiring spec (our tree)

1. `CTSGameRules::Think` body stays in `dlls/TS_gamerulesclass.cpp` **as a plain non-virtual
   method of the standalone class** — it is already file-homed correctly; delete the
   `override` relationship to CHalfLifeMultiplay::Think when the class stops deriving.
2. Our `CHalfLifeMultiplay::Think` (dlls/multiplay_gamerules.cpp:185): keep the stock body;
   append, after the fragsleft/timeleft update block and before return:
   `TSGameRules.Think(); if (TSGameRules.m_szEndMap[0] != '\0') GoToIntermission();`
   (interim, while TSGameRules is still the G1 pointer: `TSGameRules->Think()` + null guard).
3. Our `CHalfLifeTeamplay::Think` (dlls/teamplay_gamerules.cpp:72): reshape to §1.3 —
   game-over → base Think; otherwise its OWN timelimit/fraglimit/cvar-update body (do not call
   base) + the same two-line TS tail. The IsCTB fraglimit fork can carry the dead-CTB branch
   as a tagged block (IsCTB is reachable: §4.6).
4. `m_szEndMap[0x40]` (offset provenance +0x1824) replaces any invented end-map flag; wire
   CheckVotes' winning-map strcpy, Init's clear, ChangeLevel's consume-then-clear-then-Init.
5. Delete our `CTSGameRules::Think` **override** wiring only after 2+3 are in (StartFrame's
   virtual dispatch must land on the MP/Teamplay bodies, which then drive the TS think).

---

## 2. Q2 — CTSGameRules::InitHUD's original home(s)

### 2.1 The dispatch truth (who calls what, in order)

`InitHUD__12CTSGameRules` @0x69134 has **exactly one caller**:
**`CBasePlayer::UpdateClientData` @0x81d00 (TS_player.cpp), call @0x81dfa**, inside the
`m_fInitHUD` block, **immediately after** the virtual `g_pGameRules->InitHUD(this)` (vt+0x34).
Decompile (cont.84 `inv_A_GetWeaponData_free__CTSGun_UCD__CBasePlayer_UCD.c` lines 260-306):

```c
if (m_fInitHUD) {
    m_fInitHUD = false; gInitHUD = false;
    MSG_ONE ResetHUD {BYTE 0};
    if (!m_fGameHUDInitialized /*+0x9b8*/) {
        MSG_ONE InitHUD {};
        g_pGameRules->InitHUD(this);              // virtual -> stock MP/Teamplay chain (§2.2/2.3)
        CTSGameRules::InitHUD(&TSGameRules, this);// @0x81dfa — NON-VIRTUAL TS catch-up (§2.4)
        m_fGameHUDInitialized = true;
        if (g_pGameRules->IsMultiplayer())        // vt+0x1c
            FireTargets("game_playerjoin", ...);
    }
    FireTargets("game_playerspawn", ...);
    // TS DELTA: RoundTime re-send: if (TSGameRules.m_flRoundEnd/*+8*/ != 0 &&
    //   TSGameRules.m_bRoundTimerEnabled/*byte +0x10*/) MSG_ONE TSRoundTime
    //   {BYTE clamp0(round(end-time)), BYTE 0}
    // TS DELTA: if (TSGameRules.byte@+0x5885 && this->m_bLowGravity)
    //   MSG_ONE TSPowerUp {SHORT 0x100, BYTE 0}   // low-gravity catch-up
}
```

Note the joiner therefore receives TSRoundTime **twice** (once inside CTSGameRules::InitHUD,
once from the UCD tail) — reproduce both; drop-in clients tolerate it because byte-1==0 both
times.

### 2.2 MP::InitHUD @0xbf618 — verdict: **stock** (one cosmetic TS delta)

Decompile @000cf618. Stock 2003 sequence: `"%s has joined the game"` ClientPrintAll →
LogPrintf `"...entered the game"` (**TS delta:** when `g_teamplay != 0` the 4th log field is
the **model infokey**, not userid — format `"\"%s<%i><%s><%s>\""`) → virtual
`UpdateGameMode(pl)` (vt+0x3c) → MSG_ONE ScoreInfo{idx,0,0,0,0} self-zero → `SendMOTDToClient`
→ roster loop: per player MSG_ONE ScoreInfo{idx, frags, deaths(player+0xb30), 0,
GetTeamIndex(player->m_szTeamName@+0xb48)+1 (vt+0xd0)} → if g_fGameOver MSG_ONE
SVC_INTERMISSION(0x1e). **No TS messages here.**

### 2.3 Teamplay::InitHUD @0xd45d8 — verdict: **stock**

Decompile @000e45d8, exactly the stock shape: `SetDefaultPlayerTeam(pl)` (vt+0xe0) → base
`CHalfLifeMultiplay::InitHUD` → MSG_ONE **TeamNames**{BYTE num_teams, STRING×n (global
`team_names`, stride 0x10)} → `RecountTeams(false)` → model-vs-team compare → SayText
`"* you are on team '%s'"` / `"* assigned to team %s"` → **`ChangePlayerTeam(pl,
pl->m_szTeamName, false, false)`** (vt+0xdc) → `RecountTeams(false)` → roster loop: per player
with `IsValidTeam(TeamID())` (vt+0xd8 / player vt+0xd0) MSG_ONE **TeamInfo**{BYTE idx, STRING
teamname}. **This is where the joiner's team roster comes from** — not from any CTSGameRules
method.

### 2.4 CTSGameRules::InitHUD @0x69134 — full original statement list

(decompile cont.84 `inv_C_RoundTime_InitHUD_SetPlayerRespawnTime.c` lines 100-240; the
"@0x691e0 SrvSett catch-up" is statements 3-6 of this function)

1. if `g_flWeaponCheat != 0`: `TSHudMessageOne(0x2d,0xc3,0x2d,8,100,"This server has cheats
   enabled.")`; and if `damagemult.value > 0`: TSHudMessageOne(...,"This server has custom
   weapon damages (%.2f)").
2. if byte@+0x5884 (`m_bWeaponParamsCustom`): `InformClientsOnWeaponsParams(pl)`.
3. MSG_ONE TSServerSetting {BYTE 0, SHORT **this->+0x578**} — the CACHED realbullet value
   (written by Think @0x675f0 on change; Init seeds it **-1**). NOT a fresh
   `CVAR_GET_FLOAT("realbullet")` read — our re-read is a divergence to fix.
4. MSG_ONE TSServerSetting {BYTE 1, SHORT (**dmcashbased**.value != 0)} — cvar_t
   `dmcashbased` (GOT 0x177290), **not** "usecash". // VERIFY our usecash naming against
   COMMANDS/GLOBALS coverage before sed'ing.
5. MSG_ONE TSServerSetting {BYTE 2, SHORT round(`cv_ammocount`.value)}.
6. MSG_ONE TSServerSetting {BYTE 3, SHORT this->+0x5870 (current weapon restriction)}.
7. RoundTime catch-up: if `m_flRoundEnd != 0 && byte@+0x10`: MSG_ONE TSRoundTime{BYTE
   clamp0(round(m_flRoundEnd - time)), BYTE 0} (fold-plan G3, already applied in our tree).
8. Spectator catch-up loop: per player if `IsPlayer()` (player vt+0xc8): MSG_ONE
   Spectator{BYTE ENTINDEX, BYTE (pev->iuser1 != 0)}.
9. `StartObserver(pl, pev->origin, pev->v_angle)` — the joiner enters spectate HERE.
10. Join-state resets on the player: byte+0x7ba=0, `m_iGrenadeBuys`=0, +0x53c=0 (spawn-cash
    snapshot), +0x538=0, +0x7c4=0, +0x7c8=0, +0x7cc=0.
11. `SetPlayerRespawnTime(pl, clamp(respawntime.value, 5, 30), false)` (TS_gamerules.cpp free
    fn).
12. `memset(player+0x49a, 0, 0x40)`.
13. `ResetArmament(pl)`.
14. if `g_teamplay ∈ {2,3}`: player+0x758 = 0; `SendObjectiveList(pl)` — **dead branch** in
    the shipped binary (§4.6).
15. Per-player TSGameRules slot init: if ENTINDEX < 0x20: zero 0x12 ints @+0x684+idx*0x48;
    `(+0x6c8)[idx*0x12] = idx+1` (vote/per-player table).

### 2.5 Dissolution map for OUR override (dlls/TS_gamerulesclass.cpp:1034-1117)

| our statement | original home | action |
|---|---|---|
| `m_iRespawnsUsed = 0` (:1036) | not in InitHUD @0x69134 — // VERIFY its true home (@0x694d1 was cited in-tree; 0x694d1 is inside InitHUD's range… it corresponds to item 10's +0x7ba clear region) | keep inside CTSGameRules::InitHUD, re-anchor comment |
| Spectator catch-up loop (:1040-1050) | item 8 — but original order is SrvSett→RoundTime→Spectator | keep in CTSGameRules::InitHUD, **move below the SrvSett/RoundTime block** |
| SrvSett 0 with `CVAR_GET_FLOAT("realbullet")` re-read (:1053-…) | item 3 — sends the **+0x578 Think cache** | switch to the cached member; seed -1 in Init |
| SrvSett 1 "usecash" (:…) | item 4 — `dmcashbased.value` | rename/alias per cvar coverage // VERIFY |
| SrvSett 2/3 | items 5/6 | keep |
| `m_flSrvSettResendTime` one-shot re-broadcast (:1072-1076) | **no original counterpart** (our interop fix, ledger cont.95) | keep, tagged INVENTED-INTEROP |
| `InformClientsOnWeaponsParams` gate (:1079) | item 2 — original does it FIRST (before SrvSett) | reorder to top |
| cheats/damagemult HUD messages | item 1 — **missing in ours** | add |
| `if (!IsTeamMode()) return;` + TeamNames + TeamInfo roster (:1081-1101) | **CHalfLifeTeamplay::InitHUD @0xd45d8** (stock!, §2.3) | DELETE from CTSGameRules::InitHUD; comes free once Teamplay is installed on team maps |
| RoundTime catch-up (:1106-1116) | item 7 | keep |
| (absent in ours) StartObserver + join resets + respawn gate + ResetArmament (items 9-13) | THIS function | move in from TSEnterSpectate per cont.94 Y7 (same homes Y7 already pinned) |
| (absent) per-player slot init (item 15) | THIS function | add when the +0x684 table is reconstructed // VERIFY semantics |

Additionally: our UCD block (dlls/player.cpp:4354-4380) must gain the **non-virtual
`TSGameRules.InitHUD(this)` call right after `g_pGameRules->InitHUD(this)`**, plus the UCD-tail
RoundTime re-send + TSPowerUp low-gravity catch-up (§2.1) — and once Teamplay/MP are installed
as g_pGameRules, the virtual call stops landing on our merged override entirely.

---

## 3. Q3 — the TS-patched Teamplay pair (+ the ctor, CUIC, RecountTeams)

All in `teamplay_gamerules.cpp` (stock file, patched in place — their nm addresses are
stock-range 0xd3e48-0xd5a40; the "TS range" heuristic does NOT catch these).

### 3.1 `CHalfLifeTeamplay::SetDefaultPlayerTeam` @0xd44ac (decompile @000e44ac)

```c
const char* CHalfLifeTeamplay::SetDefaultPlayerTeam(CBasePlayer* pl) {
    if (TSGameRules.m_iGameMode /*byte@+0*/ < 2) {          // ALWAYS TRUE live (§4.6)
        const char* src;
        if (g_theOneMode == 0)
            src = engine InfoKeyValue(GetInfoKeyBuffer(pl->edict()), "team");
        else                                                 // The-One mode:
            src = pl->byte@+0x7b9 ? TSGameRules+0x28         //   the-one flag -> team0 "The ONE"
                                  : TSGameRules+0xb4;        //   else team1 (slot name stride 0x8c)
        strncpy(pl->m_szTeamName /*+0xb48*/, src, 16);
    }
    RecountTeams(true);                                      // stock (but bResendInfo=TRUE)
    if (pl->m_szTeamName[0]) {
        if (IsValidTeam(pl->m_szTeamName) /*vt+0xd8*/ && defaultteam.value == 0)
            return pl->m_szTeamName;                         // keep the infokey team
    }
    // fall through: assign
    const char* team = (defaultteam.value == 0) ? TeamWithFewestPlayers()   // stock @0xd5510
                                                : team_names /*global slot 0*/;
    strncpy(pl->m_szTeamName, team, 16);
    return pl->m_szTeamName;
}
```

TS deltas vs stock: (a) the `mode < 2` gate around the infokey read, (b) the The-One forced
team from the TSGameRules name slots, (c) `RecountTeams(true)`. Everything after is stock.
NOTE the defaultteam!=0 branch honors the infokey team only when defaultteam==0 — decompile
condition is `(IsValidTeam && defaultteam==0) -> done`, else reassign; with defaultteam!=0 the
assignment source is `team_names[0]` (first team). Matches stock.

### 3.2 `CHalfLifeTeamplay::ChangePlayerTeam(pl, name, bKill, bGib)` @0xd488c (decompile @000e488c)

```c
1  idx = ENTINDEX(pl);
2  dmg = bGib ? DMG_ALWAYSGIB(0x2000) : DMG_NEVERGIB(0x1000);
3  if (bKill) {                                       // stock suicide block
4      m_DisableDeathMessages /*+0x20c*/ = true; m_DisableDeathPenalty /*+0x210*/ = true;
5      pl->TakeDamage(INDEXENT(0)->v ..., 900, dmg);
6      both flags = false;
7  }
8  if (g_theOneMode) {                                // TS DELTA A: forced name
9      name = pl->byte@+0x7b9 ? TSGameRules+0x28 : TSGameRules+0xb4;   // (see 3.1)
10 } else if (*name == '\0' || !IsValidTeam(name))    // (empty-string compare vs DAT_00106657)
11     name = TeamWithFewestPlayers();
12 strncpy(pl->m_szTeamName, name, 16);
13 // model infokey — three-way:                      // TS DELTA B
14 if (g_teamplay ∈ {2,3}) {                          //   dead live-path (§4.6)
15     SetClientKeyValue(idx, buf, "model", CTSGameRules::GetPlayerModel(&TSGameRules, pl)); // @0xd4a5c
16     CTSGameRules::SendObjectiveList(&TSGameRules, pl);                                    // @0xd4a7f
17 } else if (m_teamLimit /*+0x214*/ == 0)
18     SetClientKeyValue(idx, buf, "model", pl->m_szTeamName);          // stock behavior
19 else
20     SetClientKeyValue(idx, buf, "model", GetPlayerModel(&TSGameRules, pl));  // @0xd4ad3 — LIVE
21 SetClientKeyValue(idx, buf, "team", pl->m_szTeamName);               // stock
22 RecountTeams(false);
23 MSG_ALL TeamInfo  {BYTE idx, STRING m_szTeamName};                   // stock
24 MSG_ALL ScoreInfo {BYTE idx, SHORT frags, SHORT deaths(+0xb30), SHORT 0,
25                    SHORT g_pGameRules->GetTeamIndex(m_szTeamName)/*vt+0xd0*/ + 1};
26 if (!bKill && g_theOneMode == 0) {                 // TS DELTA C: no-kill team change ->
27     StartObserver(pl, pev->origin, pev->v_angle);  //   spectate + 5s respawn gate
28     SetPlayerRespawnTime(pl, 5, true);             //   @0xe4b3c region, bForce=true
29 }
30 MSG_ALL TeamNames {BYTE num_teams, STRING team_names[i] ×n};  // TS DELTA D: rebroadcast
```

Since Teamplay ctor sets `m_teamLimit = 1` unconditionally (§3.4), **line 20 is the live model
path**: every team (re)assignment stamps the player's `model` infokey from
`CTSGameRules::GetPlayerModel` @0x69a08 (decompile @00079a08): `i = GetTeamIndex(pl->m_szTeamName)`;
count non-empty of the team's 3 model slots (+0x38/+0x48/+0x58 + i*0x8c, 16 bytes each);
RANDOM_LONG(0, n-1); store pick in `pl->+0x758`; return the model string (empty string if none).
Stock's `"...joined team..."` LogPrintf is **absent** here (it lives in CUIC's change path,
§3.3).

### 3.3 `CHalfLifeTeamplay::ClientUserInfoChanged` @0xd4d18 (decompile @000e4d18)

Calls **`CHalfLifeMultiplay::ClientUserInfoChanged` first** (the TS-relocated one @0x7b244 —
known six), then the stock model-policing reshaped around GetPlayerModel:

```c
mdl = InfoKeyValue(infobuffer, "model");
if (strcasecmp(mdl, pl->m_szTeamName) == 0) return;
if (g_teamplay ∈ {2,3})           SetClientKeyValue(..., "model", GetPlayerModel(...)); // dead
else if (m_teamLimit != 0)        SetClientKeyValue(..., "model", GetPlayerModel(...)); // LIVE @0xd4e46
else /* m_teamLimit == 0 */ {
    if (defaultteam.value != 0) { force model+team back to m_szTeamName; SayText "* Not allowed to change teams in this game!\n"; }
    else if (!IsValidTeam(mdl))  { force model back; SayText "* Can't change team to '%s'" + "* Server limits teams to '%s'" (m_szTeamList +0xc); }
    else { SayTextAll "* %s has changed to team '%s'"; LogPrintf "joined team"; ChangePlayerTeam(pl, mdl, 0, 0) /*vt+0xdc*/; RecountTeams(true); }
}
```

Live path (m_teamLimit==1) = **model always re-stamped from GetPlayerModel**; the model-as-team
change mechanism is dead in TS. Team changes in TS instead go through the `jointeam` console
command → `ChangePlayerTeam` (client.cpp dispatch @0x9e4b0, cont.94 R1).

### 3.4 `CHalfLifeTeamplay::CHalfLifeTeamplay()` @0xd3e48 (decompile @000e3e48) — heavily TS

```c
CHalfLifeMultiplay();                                  // base ctor (§4.4) — ALREADY calls TSGameRules.Init()
vptr = __vt_17CHalfLifeTeamplay;
CTSGameRules::Init(&TSGameRules);                      // @0xd3e85 — Init runs a 2nd time
CTSGameRules::InitTeamScore(&TSGameRules, team_scores);// @0xd3e95 — EMPTY function (ret) @0x69130
m_DisableDeathMessages /*+0x20c*/ = 0; m_DisableDeathPenalty /*+0x210*/ = 0;
memset(team_names, 0, 0x200); zero team_scores[0x20]; num_teams = 0;
m_szTeamList[0] /*+0xc*/ = 0;
savedModels[0x800] = g_teamModels (global, filled by worldspawn "teammodels" key §5.3); then g_teamModels[0] = 0;
if (world edict(0) pev->team && teamoverride.value && STRING(world->v.team))
    strncpy(m_szTeamList, STRING(world->v.team), 0x200);   // worldspawn "mapteams"
if (g_teamplay ∉ {2,3}) {                              // live path
    if (!m_szTeamList[0]) strncpy(m_szTeamList, teamlist.string, 0x200);
    if (!savedModels[0])  strncpy(savedModels, teammodels.string, 0x800);
}
if (m_szTeamList[0]) CTSGameRules::SetTeamList(&TSGameRules, m_szTeamList);  // @0xd403f
// REBUILD m_szTeamList from the TSGameRules slots (canonical names + The-ONE insertion):
m_szTeamList[0] = 0;
for slot i in 0..3: if (name@+0x28+i*0x8c nonempty) { strcat(m_szTeamList, name); strcat(";"); }
m_teamLimit /*+0x214*/ = 1;                            // TS: ALWAYS 1 (stock derives it from the list)
if (savedModels[0]) CTSGameRules::SetTeamModelList(&TSGameRules, savedModels); // @0xd4107
RecountTeams(false);
```

Object layout (0x218 total): `+0 vptr, +4 m_flIntermissionEndTime, +8
m_iEndIntermissionButtonHit` (base), `+0xc char m_szTeamList[0x200], +0x20c
m_DisableDeathMessages, +0x210 m_DisableDeathPenalty, +0x214 m_teamLimit`.

### 3.5 `CHalfLifeTeamplay::RecountTeams(bool bResendInfo)` @0xd5608 — patched

Stock tokenize/count/score-accumulate body (global `team_names`/`team_scores`/`num_teams`),
plus a TS tail inside the `bResendInfo` per-player TeamInfo resend: after each TeamInfo, if
`TSGameRules.IsCTB()` @0xd58aa, send MSG_ONE **TeamScore**{STRING name@+0x28+i*0x8c, SHORT
score int@+0x24+i*0x8c, SHORT 0} for each nonempty team slot (decompile @000e5608 lines
395-427). Signature is `RecountTeams(bool)` — stock SDK's default-arg form compiled to the
explicit bool.

### 3.6 Reconstruction spec for our `dlls/teamplay_gamerules.cpp`

Our file is hlu-stock (619 lines). Patches to apply, keeping everything else stock:
1. ctor → §3.4 shape (needs: TSGameRules Init/InitTeamScore(empty)/SetTeamList/SetTeamModelList,
   the g_teamModels global (worldspawn "teammodels" conduit, home: world.cpp/TS globals —
   // VERIFY exact TU), teamoverride/teamlist/teammodels cvars (stock), `m_teamLimit = 1`).
2. `SetDefaultPlayerTeam` → §3.1 (kills our `AssignToTeam`'s join half).
3. `ChangePlayerTeam` → §3.2 (kills the rest of AssignToTeam; our client.cpp `jointeam`
   handler should end in `GetIndexedTeamName` → `ChangePlayerTeam(pl, name, false, false)`
   per the original dispatch @0x9e4b0).
4. `ClientUserInfoChanged` → §3.3.
5. `InitHUD` → **stock body kept** (§2.3) — delete the TeamNames/TeamInfo code from our
   CTSGameRules::InitHUD instead.
6. `Think` → §1.3.
7. `RecountTeams(bool)` → add the IsCTB TeamScore tail (§3.5).
Our `CTSGameRules::{IsTeamplay, GetTeamID, GetIndexedTeamName, RecountTeams,
TeamWithFewestPlayers, AssignToTeam}` overrides then die exactly as cont.94 R1 predicted —
the virtuals resolve on the installed Teamplay object.

---

## 4. Q4 — InstallGameRules @0xb6050 + the TSGameRules lifecycle

### 4.1 InstallGameRules (gamerules.cpp) — full decompile (typed, /tmp/re_batch1.c @000c6050)

```c
CGameRules* InstallGameRules() {
    SERVER_COMMAND("exec game.cfg\n"); SERVER_EXECUTE();
    if (gpGlobals->deathmatch == 0) { g_teamplay = 0; return new CHalfLifeRules; }   // 4 bytes
    g_teamplay = DecideGamePlay(STRING(gpGlobals->mapname), (int)teamplay.value);    // @0xb6100
    if (g_teamplay < 1) {
        // decompiled as TWO identical branches on round(deathmatch)==1 — source was almost
        // certainly stock's  if(deathmatch)…else coop…  collapsed; both arms:
        g_teamplay = 0; return new CHalfLifeMultiplay;                               // 0xc bytes
    }
    return new CHalfLifeTeamplay;                                                    // 0x218 bytes
}
```

No CTSGameRules is EVER newed. **`TSGameRules.Init()` is not called here either** — it is
called from the ctors of the objects being newed (§4.4) and from ChangeLevel (§1.2).

### 4.2 DecideGamePlay @0x667ac (TS_gamerules.cpp) — return semantics **0/1 ONLY**

`int DecideGamePlay(const char* mapname, int teamplay)`. Full disasm decode (jumptable at file
0x66800 recovered by hand; Ghidra lost it):

```c
g_theOneMode = 0; g_lastManStanding = 0;
if (g_gamemode >= 1) {                    // g_gamemode = worldspawn "gamemode" key (§5.3)
    switch (g_gamemode) {                 // jumptable @0x66800: entries GOT-relative
    case 1: ret = 0; break;                          // -> 0x6689a  "DeathMatch"
    case 2: ret = 1; break;                          // -> 0x668ba  "Team DeathMatch"
    case 3: g_lastManStanding = 1; ret = 0; break;   // -> 0x6688e  "Last Man Standing"
    case 4: g_theOneMode = 1;      ret = 1; break;   // -> 0x66810  "The One Mode"
    default: return 0;                               // g_gamemode-1 > 3
    }
    g_gamemode = 0;                       // consume the forced mode
    return ret;
}
char pfx[4]; pfx[0]=0; strncpy(pfx, mapname, 3); pfx[3]=0;   // @0x66824-0x66849: DEAD —
                                                             // copied, never compared (vestige
                                                             // of a removed map-prefix mode check)
if (mp_theonemode.value != 0)      { g_theOneMode = 1;      g_gamemode = 0; return 1; }
if (lastmanstanding.value != 0)    { g_lastManStanding = 1; g_gamemode = 0; return 0; }
if (teamplay.value != 0)           {                        g_gamemode = 0; return 1; }
g_gamemode = 0; return teamplay /*the passed-in int, == 0 here*/;
```

FGD cross-check (`reference/ts_fgd.fgd` worldspawn): `gamemode` choices 0 "Use server
settings" / 1 DM / 2 Team DM / 3 LMS / 4 The One — matches the switch 1:1. **The second
parameter is the pre-rounded `teamplay` cvar int, and the function NEVER returns 2 or 3.**
Mode is therefore: **return value = "install Teamplay?" boolean**; the LMS/TheOne flavors ride
on the `g_lastManStanding`/`g_theOneMode` globals, NOT on the mode number.

### 4.3 Ctor timing — static init in **TS_gamerules.cpp** (correction to cont.94 G1)

`__static_initialization_and_destruction_0` @0x67154 (local `t` symbol; its
`_GLOBAL_.I.SlowMotion__FP11CBasePlayer` @0x671b4 names the TU's first function `SlowMotion`,
which funcfile.tsv homes in **TS_gamerules.cpp**) calls `CTSGameRules::CTSGameRules(&TSGameRules)`
@0x67181 — the ONLY ctor call in the binary. So the object is **defined in TS_gamerules.cpp**
(`CTSGameRules TSGameRules;` at file scope), NOT TS_player.cpp as G1 guessed. The ctor @0x671dc
(TS_gamerulesclass.cpp) does exactly one thing: construct the embedded
`CBulletManager` @+0x1864. The matching `_GLOBAL_.D.` destructs it. **All real init lives in
`Init()`**, so C++ static-init-order is a non-issue — mirror that: keep our ctor trivial.

### 4.4 `CTSGameRules::Init` @0x67d6c — callers and body

Callers (PLT census): **`CHalfLifeMultiplay::CHalfLifeMultiplay` @0xbee05** (i.e. every
MP/Teamplay install), **`CHalfLifeTeamplay::CHalfLifeTeamplay` @0xd3e85** (so a teamplay map
runs Init TWICE — harmless, reproduce the double call), **`CHalfLifeMultiplay::ChangeLevel`
@0xc09ce** (pre-changelevel reset). **CHalfLifeRules installs never Init it** (deathmatch-0 is
outside TS's supported config).

MP ctor body (decompile @000cedd0): set vptr(s) → `CTSGameRules::Init(&TSGameRules)` →
`CVoiceGameMgr::Init(&g_VoiceGameMgr /*GLOBAL, GOT 0x177330*/, &g_GameMgrHelper, maxClients)` →
`RefreshSkillData()` → `m_flIntermissionEndTime /*+4*/ = 0` → `g_flIntermissionStartTime
/*GLOBAL*/ = 0` → stock servercfgfile/lservercfgfile exec. **Stock-vs-TS member census for the
0xc-byte MP object:** keeps `{vptr@0, m_flIntermissionEndTime@+4, m_iEndIntermissionButtonHit@+8}`;
the 2003-SDK member `m_VoiceGameMgr` was **moved to global `g_VoiceGameMgr`**
(g_flIntermissionStartTime is already a global in the stock SDK). Anything else our
hlu-derived CHalfLifeMultiplay carries (m_flIntermissionStartTime member, coop/skill extras)
is SDK-drift to strip or ignore per divergence policy.

Init body (decompile @00077d6c, member names where pinned; the rest keep offset provenance):

```
byte@+0   m_iGameMode        = (byte)g_teamplay        // disasm 0x67d83-0x67d8b: verbatim
InitTeams()                                            // @0x67d91 (§4.5)
byte@+1   (powerup-enable)   = 0xff                    // Think latches 0xff -> 1
+0x56c,+0x560,+0x564,+0x568 (globalslow: NextGlobalSlow/Active/Scale/End) = 0
+0x574 = -1.0f;  +0x578 (SrvSett0 realbullet cache) = -1;  +0x55c = -1;  +0x558 = -1
zero 0x21 ints @+0x57c;      +0x570 = time + 60
+0x4 = 0 (powerup timer);    +0x14 = 0;   byte@+0x18 = 0
m_flRoundEnd/*+8*/ = 0;  m_flRoundRestart/*+0xc*/ = time + 5;   byte@+0x10 (timer-enabled) = 1
InitObjectives(); SetObjective()                       // @0x67e42/@0x67e4a
zero 0x240 ints @+0x684 (32 × 0x48 per-player blocks); +0xf84 = time + 10
m_szEndMap[0]/*+0x1824*/ = 0;   CBulletManager::Reset(+0x1864)
+0x586c = +0x5870 = InitWeaponRestriction()            // TS_gamerules.cpp @0x66ad0
byte@+0x5878 = 0;  +0x587c = time + 10;  m_flCheckDeads = 0
bytes +0x5884(customparams) +0x5885 +0x5875 +0x5874 +0x5876 = 0
+0x5888 = 0.1f;  +0x588c = 0.2125f;  byte+0x5890(m_bIsCTB)=0; byte+0x5891(m_bCTBCached)=0
```

**The first-round bootstrap is `m_flRoundRestart = time + 5` → Think fires RestartRound** —
cont.94 G4's tagged invented arm in our Think can be removed once this Init shape ships.

### 4.5 InitTeams @0x67f60 + SetTeamList @0x680dc (team-slot ground truth)

InitTeams (decompile /tmp/re_batch2.c): zero 0x8c ints @+0x1c, then for each of 4 slots
(stride **0x8c**, slot base = +0x1c + i*0x8c): cash@+0x1c = round(initial_cash.value),
+0x20 = 0, wins@+0x24 = 0, name@+0x28 = "Team%i", spawn1@+0x68 = "info_player_team%i",
spawn2@+0x88 = **"info_player_deathmatch"** (23-byte copy from .rodata 0xf7630),
`SetTeamModelList(i, defTeamModels + i*0x200)`. Then **if mode ∈ {2,3}** (dead, §4.6):
models seal/merc, `SetTeamList("Specialists;Mercenaries")`, spawn2 = "info_player_start".

SetTeamList (matches cont.94 R1's description): clears the four name slots
(+0x28/+0xb4/+0x140/+0x1cc), strtok on ";" into slot names, **The-ONE insertion** (slot 0
forced to "The ONE" when g_theOneMode, real teams start at slot 1), per-team ALERT
`"created team %s\n"`, cap = `g_theOneMode ? 2 : 4`. No m_szTeamList member, no RecountTeams
call — those are ours to delete (R1 already flags it).

Slot record (0x8c): `{+0 int cash, +4 int ?, +8 int wins/score, +0xc char name[16],
+0x1c char models[3][16] (@+0x38/+0x48/+0x58 abs), +0x4c char spawn1[32] (@+0x68 abs),
+0x6c char spawn2[?] (@+0x88 abs)}` — abs offsets = +0x1c+i*0x8c base. TeamWin/Teamplay::Think
read score as int@abs +0x24+i*0x8c ⇒ score is the +8 field; the "cash" +0 field is reset by
TeamWin's <2-players branch. // VERIFY the +4 field (zeroed by InitTeams and TeamWin, never
seen read).

### 4.6 The m_iGameMode seeding truth — **modes 2/3 are DEAD in the shipped binary**

- Only writers of `g_teamplay` (GOT+0x210c census): InstallGameRules (0 / DecideGamePlay's
  0-or-1) and InitObjectives (=2 @0x686a2-context, =3 @0x686b6-context).
- Only writers of the mode byte (TSGameRules+0): Init (`= (byte)g_teamplay`) and
  InitObjectives (movb $2 / movb $3 — the ONLY `movb $0x2/$0x3,(%reg)` in the whole .so).
- InitObjectives' entry gate (disasm @0x68602-0x68609): `if ((byte)(mode-2) > 1) return;` —
  it only RESCANS/flips **between** 2 and 3 (counts `ts_objective_manager` entities into
  +0x250[0x20]/count +0x24c; count==0 → mode=2,g_teamplay=2; else mode=3,g_teamplay=3).
- DecideGamePlay never returns ≥2 (§4.2, jumptable decoded; FGD gamemode caps at 4→TheOne).
⇒ **No path ever takes mode from {0,1} to {2,3}.** The dead `strncpy(pfx, mapname, 3)` in
DecideGamePlay is the fossil of the removed promotion (an old map-prefix check). Therefore all
`mode-2U < 2` / `g_teamplay-2U < 2` branches (Specialists;Mercenaries defaults,
SendObjectiveList in InitHUD/ChangePlayerTeam, "Defeat the opposite team", the objective
manager scan, info_player_start spawn override) are **unreachable objective-mode leftovers**.
- **IsCTB @0x67f08 is NOT mode≥2** (disasm decoded): `return mode == 1 &&
  (cached: UTIL_FindEntityByClassname(0, "ts_briefcase") != NULL)` — cache byte +0x5890,
  cached-flag +0x5891 (both cleared by Init). CTB = teamplay map containing a `ts_briefcase`.
- **Consequence for our interim:** `IsTeamMode() ≡ m_iGameMode >= 2` (cont.94 R1's sketch) is
  WRONG for live play — on every real team map mode is **1** and the live gates are
  `mode == 1` / `g_teamplay != 0` / `IsCTB()`, per site:
  | original gate | live? | where seen |
  |---|---|---|
  | `mode < 2` | always-true | SetDefaultPlayerTeam infokey branch |
  | `g_teamplay != 0` | teamplay bool | log formats (MP::InitHUD/ClientDisconnected), CanPlayerHearPlayer relationship gate, client.cpp CUIC |
  | `mode == 1 && ts_briefcase` (IsCTB) | CTB | Teamplay::Think fraglimit source, RecountTeams TeamScore tail |
  | `mode-2U < 2` / `g_teamplay-2U < 2` | DEAD | InitTeams/TeamWin/RestartRound/InitObjectives/ChangePlayerTeam/CUIC/CTSGameRules::InitHUD@0x69592 |
  Each of our 15 `IsTeamMode()` sites must be re-graded against this table when folded — most
  of our uses (buy zones, spawn selection, flags/objectives) need the ORIGINAL site's own read
  re-derived; do NOT sed them all to one predicate. // VERIFY per site during apply.
- Seeding spec: `m_iGameMode` = byte at +0, `= (uint8)g_teamplay` in Init; keep the (dead)
  InitObjectives promotion verbatim for source fidelity, tagged `// unreachable in 2.1.x —
  see cont96 r1 dossier §4.6`.

### 4.7 InstallGameRules wiring spec for our tree (dlls/gamerules.cpp:210-240)

1. Restore the original three-way install (§4.1): CHalfLifeRules when `deathmatch==0`
   (keep our sv_busters branch only if the busters strip hasn't landed), else
   `g_teamplay = DecideGamePlay(STRING(mapname), (int)teamplay.value)` (int, NOT clamped to
   0/1 — semantics are already 0/1) → `new CHalfLifeMultiplay` / `new CHalfLifeTeamplay`.
2. Kill `pRules->m_iGameMode = ...` seeding here — Init does it (from the ctor).
3. TSGameRules object: target state = static `CTSGameRules TSGameRules;` **defined in our
   TS_gamerules.cpp** (create the file if the split hasn't; DecideGamePlay/SlowMotion/HaveFlag/
   SetPlayerRespawnTime/InitWeaponRestriction are its other residents per funcfile.tsv), with
   the G1 pointer interim retired. Ctor stays trivial (§4.3).
4. Init call sites: our CHalfLifeMultiplay ctor + CHalfLifeTeamplay ctor + MP::ChangeLevel
   (§1.2, together with the m_szEndMap consume).
5. Note deathmatch==1-vs-else both newing MP (§4.1) — write it as stock's
   `if (deathmatch) … else /*coop leftover*/ …` shape; behaviorally identical.

---

## 5. The full TS-patch census on stock gamerules classes (beyond the known six)

Method: (a) nm range check (TS range 0x60000-0xb0000 = relocated into TS files) — for
MP/Teamplay this finds **exactly the known six** (DeathNotice @0x78f7c, PlayerSpawn @0x790ac,
PlayerKilled @0x795e4, PlayFootstepSounds @0x7b0e4, IsValidTeam @0x7b140,
ClientUserInfoChanged @0x7b244, all funcfile'd TS_multiplay_gamerules.cpp) and nothing else;
(b) **TSGameRules GOT-slot access census** (every `0x14f0(%ebx)` site) + CTSGameRules PLT-call
census — this finds the IN-PLACE patches the range heuristic misses:

### 5.1 In-place-patched stock methods (hidden-gap list — each needs its §)

| function | addr | TS deltas | this dossier |
|---|---|---|---|
| `CHalfLifeMultiplay::CHalfLifeMultiplay` | 0xbedd0 | TSGameRules.Init(); VoiceGameMgr→global | §4.4 |
| `CHalfLifeMultiplay::Think` | 0xbf038 | CTSGameRules::Think call + m_szEndMap check; VoiceGameMgr global | §1.2 |
| `CHalfLifeMultiplay::InitHUD` | 0xbf618 | log-format model field when g_teamplay (cosmetic) | §2.2 |
| `CHalfLifeMultiplay::ClientDisconnected` | 0xbf960 | FireTargets "game_playerleave"; teamplay log w/ model; `RemoveAllItems(pl,1)`; `TSPlayerDisconnect(pl)` @0x8f94c; `TSGameRules.+0x587c = 0` | decompile @000cf960 |
| `CHalfLifeMultiplay::ChangeLevel` | 0xc0784 | m_szEndMap forced-map consume; `TSGameRules.Init()` | §1.2 |
| `CHalfLifeTeamplay::CHalfLifeTeamplay` | 0xd3e48 | full TS rework | §3.4 |
| `CHalfLifeTeamplay::Think` | 0xd412c | IsCTB team-score source; CTSGameRules::Think; m_szEndMap | §1.3 |
| `CHalfLifeTeamplay::SetDefaultPlayerTeam` | 0xd44ac | mode gate + The-One slots + RecountTeams(true) | §3.1 |
| `CHalfLifeTeamplay::ChangePlayerTeam` | 0xd488c | GetPlayerModel infokey; SendObjectiveList (dead); StartObserver+SetPlayerRespawnTime(5,true); TeamNames rebroadcast | §3.2 |
| `CHalfLifeTeamplay::ClientUserInfoChanged` | 0xd4d18 | GetPlayerModel enforcement three-way | §3.3 |
| `CHalfLifeTeamplay::RecountTeams(bool)` | 0xd5608 | IsCTB TeamScore-per-slot tail | §3.5 |
| `CMultiplayGameMgrHelper::CanPlayerHearPlayer` | 0xc0ed0 | teamplay: PlayerRelationship (vt+0xcc) must ==1; then `(mode-2U<2 ∥ g_lastManStanding)` && both-dead check (player vt+0xb4 + pev->+0x770 < 0) → dead can only hear dead. Live effect: LMS dead-talk isolation | decompile @000d0ed0 |
| `CWorld::KeyValue` | 0xe4a18 | new keys: `gamemode`→g_gamemode int; `mapteams`→pev->team + `TSGameRules.SetTeamList(STRING(team))` @0xe4cd9; `teammodels`→g_teamModels buffer | decompile @000f4a18 |

Verdict **stock (no TSGameRules/no TS calls)**: Teamplay::InitHUD @0xd45d8 (§2.3),
Teamplay::DeathNotice @0xd509c / PlayerKilled @0xd51b8 (DisableDeathMessages/Penalty wrappers —
// VERIFY by eye if touched), MP::GoToIntermission @0xc0050, and the rest of the MP block
0xbeee8-0xc0dbc. `Spawn__12CTSSuperJump` @0xb9f6b, `cmd_globalslow/cmd_restartround/
cmd_weaponrestriction/display_weaponrestriction` (funcfile: **game.cpp** — TS console handlers
touching TSGameRules; game.cpp is patched too, outside this dossier's scope) also appear in
the stock-range census but are not MP/Teamplay methods.

### 5.2 CTSGameRules non-virtual API consumed by stock-range code (the split's link surface)

`Init` (MP ctor / Teamplay ctor / ChangeLevel), `Think` (MP::Think / Teamplay::Think),
`IsCTB` (Teamplay::Think / RecountTeams), `InitTeamScore` (Teamplay ctor; EMPTY body),
`SetTeamList` (Teamplay ctor / CWorld::KeyValue), `SetTeamModelList(char*)` (Teamplay ctor),
`GetPlayerModel` (ChangePlayerTeam ×2 / Teamplay::CUIC ×2), `SendObjectiveList`
(ChangePlayerTeam, dead branch), `InitHUD` (CBasePlayer::UpdateClientData),
`SetGlobalSlowMotion` (cmd_globalslow), `ResetRestartRound` (cmd_restartround),
`PlayerVote` (Host_Say), `CallCheckDeads` (MP::PlayerKilled tail @0x7b0d4 /
CBasePlayer::PreThink @0x81022 / TSPlayerDisconnect @0x8fa58), `TheOneIsDead` +
`UpdateTeamScores` + `GetTeamIndex` (MP::PlayerKilled @0x7a2fa/@0x7abea/@0x7abe0),
`GetTeamIndex` (EntSelectSpawnPoint / CTSTrigger / CTSSellZone / objective+flag entities).
Everything is a direct call — the split introduces **zero** new virtuals.

### 5.3 Worldspawn key flow (map → rules)

`gamemode` → `g_gamemode` (int global) → consumed+zeroed by DecideGamePlay's switch.
`mapteams` → `world->pev->team` (AllocString) **and** an immediate `SetTeamList` (pre-install;
the ctor's Init/InitTeams wipes it — the DURABLE path is the Teamplay ctor re-reading
`STRING(world->v.team)` gated on `teamoverride.value`). `teammodels` → `g_teamModels[0x800]`
global → consumed+cleared by the Teamplay ctor → `SetTeamModelList`.

---

## 6. Apply-order sketch (for the R1 executor; playtest-gated)

1. Standalone class + static object in TS_gamerules.cpp; trivial ctor; Init per §4.4;
   InitTeams/SetTeamList per §4.5. Retire the G1 pointer.
2. InstallGameRules per §4.7 (stock objects installed; our CTSGameRules no longer derives
   from CHalfLifeMultiplay and is no longer g_pGameRules).
3. MP patches: ctor / Think / ClientDisconnected / ChangeLevel / InitHUD-log (§5.1 rows 1-5).
4. Teamplay patches: §3.6.
5. UCD patch: §2.5 tail (non-virtual InitHUD call + RoundTime/PowerUp catch-ups); dissolve our
   InitHUD override per the §2.5 table.
6. Rehome the Y8/Y9 pieces onto the (now TS-relocated-six) MP methods exactly as cont.94
   already pinned; retire IsTeamMode per §4.6's per-site table.
7. Gates: both libs build; audit.py INVENTED drop (the R1 quintet + AssignToTeam + IsTeamMode);
   full join/spawn/team-change/round/LMS/TheOne playtest protocol (user-driven).

// VERIFY ledger (everything above not disasm-pinned): dmcashbased-vs-usecash naming (§2.4);
m_iRespawnsUsed's exact original write site (§2.5); the +4 team-slot field (§4.5); Teamplay
DeathNotice/PlayerKilled stock-ness (§5.1); g_teamModels' defining TU (§3.6); per-site
IsTeamMode regrades (§4.6).
