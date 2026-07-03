/***
*  ts_gamerules.cpp — RECONSTRUCTED CTSGameRules (skeleton + GetRandomPowerup).
*  Functional/source-faithful (see CLAUDE.md). recovered/server/CTSGameRules.c.
***/
#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "player.h"
#include "gamerules.h"
#include "client.h"     // respawn()
#include "ts_gamerules.h"
#include "ts_bullet.h"
#include "ts_utils.h"   // TSHudMessageAll
#include "ts_objectives.h" // CTSObjective / CTSObjectiveManager (the objective subsystem)
#include "ts_func_break.h" // CInfoBreakable::RespawnThink (RespawnEntities)
#include "ts_groundweapon.h" // CTSGroundWeapon::Fall (RespawnEntities)
#include "ts_flag.h"       // CTSFlag::RespawnFlag, HaveFlag (CTF)
#include "ts_weapons.h" // TSDeployArsenal (team-mode arsenal on team switch)
#include "UserMessages.h"

// Defined in ts_objectives.cpp — fire the objective manager's targetontimeleft on round timeout.
void TSFireTimeLeftTarget();

// Round-time HUD message (remaining seconds); registered lazily on first send.
int gmsgTSRoundTime = 0;
int gmsgTSObjective = 0;      // "Objective" (-1, variable): one HUD objective entry (coord+idx+label)
int gmsgTSShowObjective = 0;  // "ShowObj" (2): toggle a HUD objective entry's visibility

// @ StartRoundTimer — arm the round timer for a fresh round and tell clients the time left.
void CTSGameRules::StartRoundTimer()
{
	// The .so reads the `roundtime` cvar (default 150), clamped [30,180].
	float t = CVAR_GET_FLOAT("roundtime");
	if (t < 30.0f) t = 30.0f;
	if (t > 180.0f) t = 180.0f;
	m_flRoundEnd = gpGlobals->time + t;

	if (gmsgTSRoundTime == 0)
		gmsgTSRoundTime = REG_USER_MSG("RoundTime", 2); // original "RoundTime", fixed size 2 (two bytes)
	MESSAGE_BEGIN(MSG_ALL, gmsgTSRoundTime);
	WRITE_BYTE((int)(m_flRoundEnd - gpGlobals->time)); // seconds remaining
	WRITE_BYTE(0);                                     // flag: 0 = round start/restart
	MESSAGE_END();
}

//============================================================================
// Objective subsystem — the HUD objective list + the round's objective managers.
//============================================================================

// @0x6abdc — reset the HUD objective list.
void CTSGameRules::ClearObjectiveList()
{
	m_iObjectiveCount = 0;
	for (int i = 0; i < 32; i++)
		m_objectiveList[i] = ObjectiveEntry();
}

// @0x6ae20 — the slot index of `pObj` in the list, or -1.
int CTSGameRules::FindIndexByObjective(CTSObjective *pObj)
{
	for (int i = 0; i < m_iObjectiveCount; i++)
		if (m_objectiveList[i].obj == pObj)
			return i;
	return -1;
}

// @0x6ac00 — append `pObj` (with its 4 per-team label string-indices) to the list, deduped. Returns
// the (existing or new) index, or -1 if the list is full.
int CTSGameRules::AddObjectiveInList(CTSObjective *pObj, const int teamStr[4])
{
	int i = FindIndexByObjective(pObj);
	if (i >= 0)
		return i;               // already listed
	if (m_iObjectiveCount >= 32)
		return -1;              // full
	int idx = m_iObjectiveCount;
	m_objectiveList[idx].obj = pObj;
	for (int t = 0; t < 4; t++)
		m_objectiveList[idx].teamStr[t] = teamStr ? teamStr[t] : 0;
	m_iObjectiveCount++;
	return idx;
}

// @0x6ac80 — send one HUD objective entry to one client (its world position, slot id, and the
// per-team label text).
void CTSGameRules::SendObjectiveToHud(int idx, CBasePlayer *pPlayer)
{
	if (!pPlayer || idx < 0 || idx >= m_iObjectiveCount)
		return;
	const ObjectiveEntry &slot = m_objectiveList[idx];

	char label[64] = {0};
	int teamIdx = GetTeamIndex(((CBaseEntity *)pPlayer)->TeamID());
	if (teamIdx >= 0 && teamIdx < 4 && slot.teamStr[teamIdx] != 0)
		strncpy(label, STRING((string_t)slot.teamStr[teamIdx]), sizeof(label) - 1);

	Vector c = slot.obj ? slot.obj->Center() : g_vecZero;
	if (gmsgTSObjective == 0)
		gmsgTSObjective = REG_USER_MSG("Objective", -1);
	MESSAGE_BEGIN(MSG_ONE, gmsgTSObjective, NULL, pPlayer->edict());
	WRITE_COORD(c.x);
	WRITE_COORD(c.y);
	WRITE_COORD(c.z);
	WRITE_BYTE(idx);
	WRITE_STRING(label);
	MESSAGE_END();
}

// @0x6add4 — send the whole list to one client (e.g. on join).
void CTSGameRules::SendObjectiveList(CBasePlayer *pPlayer)
{
	if (!pPlayer)
		return;
	for (int i = 0; i < m_iObjectiveCount; i++)
		SendObjectiveToHud(i, pPlayer);
}

// @0x6aebc — toggle one objective's HUD visibility for all clients. (CTSObjective::Freeze calls this.)
void CTSGameRules::ShowObjectiveOnHud(CTSObjective *pObj, bool show)
{
	int idx = FindIndexByObjective(pObj);
	if (gmsgTSShowObjective == 0)
		gmsgTSShowObjective = REG_USER_MSG("ShowObj", 2);
	MESSAGE_BEGIN(MSG_ALL, gmsgTSShowObjective);
	WRITE_BYTE(idx);
	WRITE_BYTE(show ? 1 : 0);
	MESSAGE_END();
}

// @0x685d8 — round start: scan the map's objective managers, set the game mode, and start every
// objective DISABLED (a manager's Reset, via SetObjective, enables the chosen subset).
void CTSGameRules::InitObjectives()
{
	m_iManagerCount = 0;
	for (int i = 0; i < 32; i++)
		m_managers[i] = nullptr;
	m_pActiveManager = nullptr;

	CBaseEntity *pEnt = nullptr;
	while ((pEnt = UTIL_FindEntityByClassname(pEnt, "ts_objective_manager")) != nullptr)
		if (m_iManagerCount < 32)
			m_managers[m_iManagerCount++] = (CTSObjectiveManager *)pEnt;

	// objective mode iff at least one manager is present (the original sets g_teamplay 2/3).
	if (m_iManagerCount == 0)
		return;

	// Disable every objective entity up front (a manager's Reset, via SetObjective, enables the chosen
	// subset). Sweep all entities via the CBaseEntity::IsObjective() virtual (true only on objectives).
	for (int i = 1; i < gpGlobals->maxEntities; i++)
	{
		edict_t *pe = INDEXENT(i);
		if (!pe || pe->free || !pe->pvPrivateData)
			continue;
		CBaseEntity *e = CBaseEntity::Instance(pe);
		if (e && e->IsObjective())
			e->EnableObjective(false);
	}
}

// @0x690b4 — pick the round's active objective manager at random and Reset() it (which enables +
// announces that manager's objective subset).
void CTSGameRules::SetObjective()
{
	ClearObjectiveList();
	if (m_iManagerCount == 0)
		return;
	m_pActiveManager = nullptr;
	int r = RANDOM_LONG(0, m_iManagerCount - 1);
	if (m_managers[r])
	{
		m_managers[r]->Reset();
		m_pActiveManager = m_managers[r];
	}
}

// @ SendRoundTime — send the current remaining round time to one player. StartRoundTimer only
// broadcasts at arm-time, so a client that joins mid-round needs this to populate its round-time
// HUD. No-op if the timer isn't armed (m_flRoundEnd in the past / 0).
void CTSGameRules::SendRoundTime(CBasePlayer *pPlayer)
{
	if (gmsgTSRoundTime == 0 || m_flRoundEnd <= gpGlobals->time)
		return;

	MESSAGE_BEGIN(MSG_ONE, gmsgTSRoundTime, NULL, pPlayer->edict());
	WRITE_BYTE((int)(m_flRoundEnd - gpGlobals->time)); // seconds remaining
	WRITE_BYTE(0);                                     // flag (0 = mid-round join)
	MESSAGE_END();
}

// @ RoundTimeLeft — the round ran out of time: fire the objective manager's targetontimeleft, then
// declare a draw (the .so's TeamWin(2)). The draw schedules the round restart.
void CTSGameRules::RoundTimeLeft()
{
	TSFireTimeLeftTarget();
	TeamWin(2); // 2 = draw (nobody)
}
// Defined in ts_corpse.cpp / ts_gun.cpp — the TS death effects.
CBaseEntity *TSCreateDeadCorpse(CBasePlayer *pPlayer, float lifetime);
void TSDropWeaponsOnDeath(CBasePlayer *pPlayer);
void TSDropAmmoOnDeath(CBasePlayer *pPlayer);

// @ PlayerKilled — run the inherited MP scoring/death-notice, then the TS death effects:
// scatter the dead player's weapons into the world + leave a fading corpse.
void CTSGameRules::PlayerKilled(CBasePlayer *pVictim, entvars_t *pKiller, entvars_t *pInflictor)
{
	CHalfLifeMultiplay::PlayerKilled(pVictim, pKiller, pInflictor);
	pVictim->DropFlag(false); // CTF: a killed carrier drops the flag where they fell (spawns a loose flag)
	TSDropWeaponsOnDeath(pVictim);
	TSDropAmmoOnDeath(pVictim);
	TSCreateDeadCorpse(pVictim, 15.0f);
	TogglePlayerFromSlow(pVictim); // a dead player drops out of slow-motion

	// "The One" mode: if the dead player was The One, clear/hand off the role (the killer inherits it).
	if (pVictim->m_bTheOne)
	{
		CBaseEntity *pK = pKiller ? CBaseEntity::Instance(ENT(pKiller)) : nullptr;
		TheOneIsDead(pVictim, (pK && pK->IsPlayer()) ? (CBasePlayer *)pK : nullptr);
	}

	CallCheckDeads(); // team mode: a death may have eliminated a team → resolve the round
}

// @ TeamWin — record the win + announce it, then schedule a round restart 3s out (the .so
// uses param_1+0xc = time+3). // SLICE: faithful TeamWin also awards victory/defeat cash to
// each team + drives the round-time HUD; here it's the score + restart.
void CTSGameRules::TeamWin(int team)
{
	if (m_flRoundRestart != 0.0f) // a win is already locked in for this round
		return;

	// team 0/1 are the real TS teams; anything else (e.g. CheckDeads' "nobody alive") is a draw.
	bool draw = team < 0 || team > 1;
	if (!draw)
		m_teamScore[team]++;

	const char *who = draw ? nullptr : GetIndexedTeamName(team);
	if (draw || who == nullptr || who[0] == '\0')
		TSHudMessageAll(0, 0xff, 0, 8, 50,
			UTIL_VarArgs("Round draw!  (%d - %d)", m_teamScore[0], m_teamScore[1]));
	else
		TSHudMessageAll(0, 0xff, 0, 8, 50,
			UTIL_VarArgs("%s win the round!  (%d - %d)", who, m_teamScore[0], m_teamScore[1]));

	ResetRestartRound(1); // arm the 1s restart (depth 1 = restart only; the score was bumped above)
}

// @ CallCheckDeads — schedule a deads-check ~1s out (unless a round restart is already
// pending). Called on each player death.
void CTSGameRules::CallCheckDeads()
{
	if (m_flRoundRestart == 0.0f && m_flCheckDeads == 0.0f)
		m_flCheckDeads = gpGlobals->time + 1.0f;
}

// @ CheckDeads — the team-round elimination resolver. Counts live players per team; if a
// single team is left standing it wins the round, if nobody is alive it's a draw, and if two
// or more teams still have survivors the round continues. No-ops outside team mode or while a
// restart is already pending. Needs >=2 players spread over >=2 teams to resolve (so an empty
// or one-sided warmup doesn't instantly "win").
void CTSGameRules::CheckDeads()
{
	if (m_flRoundRestart != 0.0f || !IsTeamMode())
		return;

	int alive[TS_MAX_TEAMS] = {};
	int present[TS_MAX_TEAMS] = {};
	int totalPlayers = 0;

	for (int i = 1; i <= gpGlobals->maxClients; i++)
	{
		CBaseEntity *pEnt = UTIL_PlayerByIndex(i);
		if (!pEnt || !pEnt->IsPlayer())
			continue;
		int team = GetTeamIndex(pEnt->TeamID());
		if (team < 0)
			continue;
		present[team]++;
		totalPlayers++;
		if (pEnt->IsAlive()) // SLICE: the .so also counts a just-respawned grace window (+0x770)
			alive[team]++;
	}

	int teamsPresent = 0, teamsAlive = 0, lastAliveTeam = -1;
	for (int t = 0; t < m_numTeams; t++)
	{
		if (present[t] > 0)
			teamsPresent++;
		if (alive[t] > 0)
		{
			teamsAlive++;
			lastAliveTeam = t;
		}
	}

	if (totalPlayers < 2 || teamsPresent < 2)
		return;                 // not enough of a contest to resolve

	if (teamsAlive >= 2)
		return;                 // round still contested

	TeamWin(teamsAlive == 1 ? lastAliveTeam : 2); // one team left = winner; nobody = draw
}

// Re-spawn everyone + re-arm the objective entities for a fresh round.
// @0x67d6c Init — one-time map setup. Arms the FIRST round 5s out (which, via RestartRound, runs
// InitObjectives/SetObjective when the map entities exist) and seeds the objective subsystem.
CTSGameRules::CTSGameRules()
{
	Init();
}

void CTSGameRules::Init()
{
	InitTeams();
	InitTeamScore(m_teamScore); // (no-op in the original)
	m_flRoundEnd = 0;
	m_flRoundRestart = gpGlobals->time + 5.0f; // first round fires 5s into the map
	m_bRoundTimerEnabled = true;
	InitObjectives(); // (finds nothing at construction-time; the +5s RestartRound re-runs it)
	SetObjective();
}

// @0x6a154 CountPlayers — number of connected players (no team/alive filter).
int CTSGameRules::CountPlayers()
{
	int n = 0;
	for (int i = 1; i <= gpGlobals->maxClients; i++)
	{
		CBaseEntity *p = UTIL_PlayerByIndex(i);
		if (p && p->IsPlayer())
			n++;
	}
	return n;
}

// @0x67764 RespawnEntities — re-spawn the map's resettable entities by classname: breakables + flags
// always; ground weapons in team/last-man mode; powerups in last-man mode.
void CTSGameRules::RespawnEntities(bool bFirst)
{
	const bool lms = CVAR_GET_FLOAT("lms") != 0.0f;
	CBaseEntity *e;

	for (e = nullptr; (e = UTIL_FindEntityByClassname(e, "info_breakable")) != nullptr;)
		((CInfoBreakable *)e)->RespawnThink();

	if (IsTeamMode() || lms)
		for (e = nullptr; (e = UTIL_FindEntityByClassname(e, "ts_groundweapon")) != nullptr;)
			((CTSGroundWeapon *)e)->Fall();

	if (lms)
		for (e = nullptr; (e = UTIL_FindEntityByClassname(e, "ts_powerup")) != nullptr;)
			e->Spawn(); // VERIFY: the original calls CTSPowerUp::Fall() (ts_powerup.cpp-local)

	for (e = nullptr; (e = UTIL_FindEntityByClassname(e, "ts_flag")) != nullptr;)
		((CTSFlag *)e)->RespawnFlag(); // loose flags go home (the briefcase re-seeds via FlagFall)
}

// @0x67920 RestartRound — the actual round reset: re-arm the round timer, reset the objectives,
// respawn the players, recount teams, then respawn the map entities.
void CTSGameRules::RestartRound()
{
	if (CountPlayers() == 0) // nothing to restart on an empty server
		return;

	m_flRoundRestart = 0; // disarm the pending restart (always)

	// Round mechanics (clock + player respawn) apply ONLY to round-based modes (team / last-man). The
	// deferred "first round" is armed UNCONDITIONALLY in Init() — before any map entity exists, so the mode
	// can't be detected there (IsTeamMode keys off a ts_objective_manager entity). By the time this runs the
	// entities exist, so gate here: in plain deathmatch there are no rounds, and respawning/teleporting every
	// player 5s into the map (the old unconditional behaviour) was the "~5s teleport + redeploy" bug.
	const bool roundBased = IsTeamMode() || CVAR_GET_FLOAT("lms") != 0.0f;

	// (c) reset the objective subsystem — a no-op on a map with no objective entities (e.g. plain DM).
	InitObjectives();
	SetObjective();

	if (roundBased)
	{
		// (a)+(b) arm the round timer + tell clients the new clock. MUST go through StartRoundTimer:
		// it's the ONLY site that REG_USER_MSG("RoundTime") registers the message. Arming m_flRoundEnd
		// inline here (the old code) left gmsgTSRoundTime==0 — so the inline clock-send was skipped AND
		// Think's `if (m_flRoundEnd==0) StartRoundTimer()` never fired (m_flRoundEnd was already set) —
		// the message stayed unregistered and the round clock never appeared. (playtest bug #1.)
		if (m_bRoundTimerEnabled)
			StartRoundTimer();

		// (d) team banner.
		if (IsTeamMode())
			TSHudMessageAll(0, 0xff, 0, 8, 30, "Defeat the opposite team");

		// (e) respawn the players into the fresh round. SLICE: the original instead refunds the loadout's
		// cash + forces each player back into the class-select observer state; we respawn directly.
		for (int i = 1; i <= gpGlobals->maxClients; i++)
		{
			CBasePlayer *p = (CBasePlayer *)UTIL_PlayerByIndex(i);
			if (p)
			{
				p->DropFlag(true); // clear any carried flag (the briefcases re-seed fresh ones below)
				p->m_iAward = 11;  // TSPAward: respawning into the fresh round (@RestartRound +0x200)
				respawn(p->pev, false);
			}
		}
	}

	// (f) recount teams, (g) respawn the map's resettable entities.
	RecountTeams();
	RespawnEntities(true);
}

// @0x68750 ResetRestartRound — at round end (team/last-man only) arm a 1s restart + announce. With
// depth > 1 it also wipes the per-team round scores/cash. Replaces the ad-hoc restart scheduling.
void CTSGameRules::ResetRestartRound(int depth)
{
	if (!IsTeamMode() && CVAR_GET_FLOAT("lms") == 0.0f)
		return;
	m_flRoundRestart = gpGlobals->time + 1.0f;
	m_flRoundEnd = 0;

	if (m_bRoundTimerEnabled && gmsgTSRoundTime != 0)
	{
		MESSAGE_BEGIN(MSG_ALL, gmsgTSRoundTime);
		WRITE_BYTE(1);
		WRITE_BYTE(0);
		MESSAGE_END();
	}
	if (depth > 1)
		for (int i = 0; i < TS_MAX_TEAMS; i++)
		{
			m_teamScore[i] = 0;
			m_teamCash[i] = (int)CVAR_GET_FLOAT("initial_cash"); // SLICE: original also broadcasts TeamScore
			UpdateTeamScores(i, m_teamScore[i]); // (no-op in the original)
		}
	TSHudMessageAll(0, 0xff, 0, 8, 30, "Restarting Round");
}

//============================================================================
// "The One" juggernaut mode — one player is buffed into The One; killing them inherits the role.
//============================================================================

// @0x67f08 IsCTB — capture-the-briefcase mode: a ts_briefcase exists (cached). VERIFY: the original
// also gates on the game-mode byte == 1.
bool CTSGameRules::IsCTB()
{
	if (!m_bCTBChecked)
	{
		m_bHasBriefcase = UTIL_FindEntityByClassname(nullptr, "ts_briefcase") != nullptr;
		m_bCTBChecked = true;
	}
	return m_bHasBriefcase;
}

// @0x6af28 SelectRandomTheOne — every ~10s, if nobody is The One yet, crown a random eligible (alive,
// not spectating) player.
void CTSGameRules::SelectRandomTheOne()
{
	if (gpGlobals->time < m_flNextTheOneSelect)
		return;
	m_flNextTheOneSelect = gpGlobals->time + 10.0f;

	CBasePlayer *eligible[32];
	int n = 0;
	for (int i = 1; i <= gpGlobals->maxClients; i++)
	{
		CBasePlayer *p = (CBasePlayer *)UTIL_PlayerByIndex(i);
		if (!p)
			continue;
		if (p->m_bTheOne)
			return; // someone is already The One
		if (p->IsAlive() && p->pev->iuser1 == 0 && n < 32)
			eligible[n++] = p;
	}
	if (n > 0)
		SetPlayerTheOne(eligible[RANDOM_LONG(0, n - 1)]);
}

// @0x6b078 SetPlayerTheOne — buff `pPlayer` into The One (health, regen, low-gravity, speed, kung-fu)
// and announce it.
void CTSGameRules::SetPlayerTheOne(CBasePlayer *pPlayer)
{
	if (!pPlayer || !pPlayer->IsAlive() || pPlayer->pev->iuser1 != 0)
		return;
	pPlayer->m_bTheOne = 1;
	pPlayer->m_iAward = 10; // TSPAward: The One (0xa) — never overwritten by kill tiers
	pPlayer->pev->health = 150.0f;
	pPlayer->m_flHealthRegen = gpGlobals->time + 1.0f;
	pPlayer->m_iKungFu = 1;
	pPlayer->m_bLowGravity = 1;
	pPlayer->SetPlayerSpeed(330.0f); // @0x83068 — the One moves at full speed
	if (pPlayer->m_iPowerups == 0 && pPlayer->m_iHeldPowerup == 0)
	{
		pPlayer->m_iHeldPowerup = 8; // VERIFY: the recovered held-powerup value
		pPlayer->m_flPowerupExpireTime = 0;
		pPlayer->m_flPowerupTime = 3;
	}
	m_bTheOneActive = true;
	TSHudMessageAll(100, 0x96, 0xff, 1, 30,
		UTIL_VarArgs("%s is The One!", STRING(pPlayer->pev->netname)));
	UTIL_LogPrintf("Player \"%s\" triggered \"the_one\"\n", STRING(pPlayer->pev->netname));
}

// @0x6b258 TheOneIsDead — The One died: clear the role; if a player killed them, that killer becomes
// the new One.
void CTSGameRules::TheOneIsDead(CBasePlayer *pDead, CBasePlayer *pKiller)
{
	m_bTheOneActive = false;
	if (!pDead)
		return;
	pDead->m_bTheOne = 0;
	if (CVAR_GET_FLOAT("the_one") == 0.0f)
		return;
	if (!pKiller || pDead == pKiller)
		TSHudMessageAll(100, 0x96, 0xff, 1, 30, "The One is dead.");
	else
	{
		TSHudMessageAll(100, 0x96, 0xff, 1, 30,
			UTIL_VarArgs("The One was killed by %s!", STRING(pKiller->pev->netname)));
		SetPlayerTheOne(pKiller);
	}
}

//============================================================================
// Voting — per-player map / friendly-fire / teamplay votes, tallied to a majority.
//============================================================================

// @0x6a1b0 CalcMiniminVotants — votes needed to pass = half the votants (at least 1).
static int CalcMiniminVotants(int votants)
{
	int n = votants / 2;
	return n < 1 ? 1 : n;
}

// @0x6a1c8 PlayerVote — a player's `vote <...>` console command: reset / stats / map <name>.
int CTSGameRules::PlayerVote(edict_t *pEntity, const char *pCmd)
{
	if (!pEntity || !pCmd)
		return 0;
	int idx = ENTINDEX(pEntity) - 1; // 0-based player slot
	if (idx < 0 || idx >= 32)
		return 0;
	entvars_t *pev = &pEntity->v;

	if (strstr(pCmd, "reset"))
	{
		m_voteMap[idx][0] = '\0';
		m_voteFF[idx] = 0;
		m_voteTP[idx] = 0;
		ClientPrint(pev, HUD_PRINTTALK, "Your votes are resetted.\n");
		return 1;
	}
	if (strstr(pCmd, "stats"))
	{
		ClientPrint(pev, HUD_PRINTTALK, "No votes in progress.\n"); // SLICE: original lists live tallies
		return 1;
	}
	const char *pMap = strstr(pCmd, "map");
	if (!pMap)
		return 0;
	char name[32] = {0};
	strncpy(name, pMap + 3, sizeof(name) - 1);          // after "map"
	while (name[0] == ' ') memmove(name, name + 1, strlen(name)); // ltrim
	for (int i = (int)strlen(name) - 1; i >= 0 && (name[i] == ' ' || name[i] == '\n'); i--)
		name[i] = '\0';                                  // rtrim

	if (!stricmp(name, STRING(gpGlobals->mapname)))
		ClientPrint(pev, HUD_PRINTTALK, UTIL_VarArgs("%s is the current map!\n", name));
	else if (IS_MAP_VALID(name))
	{
		strncpy(m_voteMap[idx], name, sizeof(m_voteMap[idx]) - 1);
		UTIL_ClientPrintAll(HUD_PRINTTALK, UTIL_VarArgs("%s vote for map %s\n", STRING(pev->netname), name));
	}
	else
		ClientPrint(pev, HUD_PRINTTALK, UTIL_VarArgs("%s is an invalid map\n", name));
	return 1;
}

// @0x6a5c4 CheckVotes — every ~10s, tally the players' votes; apply each category that reached the
// majority (mp_friendlyfire / mp_teamplay cvars, or the next map).
void CTSGameRules::CheckVotes()
{
	if (gpGlobals->time < m_flNextVoteCheck)
		return;
	m_flNextVoteCheck = gpGlobals->time + 10.0f;

	int votants = 0, ffOn = 0, ffOff = 0, tpOn = 0, tpOff = 0;
	char maps[32][32];
	int  mapVotes[32];
	int  nMaps = 0;

	for (int i = 1; i <= gpGlobals->maxClients; i++)
	{
		CBaseEntity *p = UTIL_PlayerByIndex(i);
		if (!p || !p->IsNetClient())
			continue;
		int idx = i - 1;
		if (idx < 0 || idx >= 32)
			continue;
		votants++;
		if (m_voteFF[idx] == 1) ffOn++;  else if (m_voteFF[idx] == 2) ffOff++;
		if (m_voteTP[idx] == 1) tpOn++;  else if (m_voteTP[idx] == 2) tpOff++;
		if (m_voteMap[idx][0])
		{
			int m = 0;
			for (; m < nMaps; m++)
				if (!strcmp(maps[m], m_voteMap[idx])) { mapVotes[m]++; break; }
			if (m == nMaps && nMaps < 32)
			{
				strncpy(maps[nMaps], m_voteMap[idx], sizeof(maps[0]) - 1);
				maps[nMaps][sizeof(maps[0]) - 1] = '\0';
				mapVotes[nMaps++] = 1;
			}
		}
	}

	const int need = CalcMiniminVotants(votants);
	if (ffOn >= need)  { CVAR_SET_FLOAT("mp_friendlyfire", 1); UTIL_ClientPrintAll(HUD_PRINTCENTER, "Friendly fire enabled.\n"); }
	else if (ffOff >= need) { CVAR_SET_FLOAT("mp_friendlyfire", 0); UTIL_ClientPrintAll(HUD_PRINTCENTER, "Friendly fire disabled.\n"); }
	if (tpOn >= need)  { CVAR_SET_FLOAT("mp_teamplay", 1); UTIL_ClientPrintAll(HUD_PRINTCENTER, "TeamPlay in DM maps enabled, wait next map.\n"); }
	else if (tpOff >= need) { CVAR_SET_FLOAT("mp_teamplay", 0); UTIL_ClientPrintAll(HUD_PRINTCENTER, "TeamPlay in DM maps disabled, wait next map.\n"); }

	// the most-voted map, if it reached the majority -> the next map (applied at map end).
	int best = -1, bestVotes = 0;
	for (int m = 0; m < nMaps; m++)
		if (mapVotes[m] > bestVotes) { bestVotes = mapVotes[m]; best = m; }
	if (best != -1 && bestVotes >= need)
		strncpy(m_szNextMap, maps[best], sizeof(m_szNextMap) - 1);
}


// @ CTSGameRules::Think — run the inherited MP think, then (in non-team deathmatch) drive
// the dynamic powerup spawner: roll a random powerup every ~20s and PutPowerUp it (retry in
// 1s if no spawn point was free). Mirrors the .so's Think powerup block.

void CTSGameRules::Think()
{
	CHalfLifeMultiplay::Think();

	g_BulletManager.Think(); // TS: fly the slow-motion bullets-as-models each frame (ts_bullet.cpp)

	// `slowmatch` cvar (default 1.0): a value in (0,1) runs the entire match in global
	// slow-motion at that scale (the .so's "SlowMatch" mode). GlobalSlowMotion2 is self-
	// throttled (~0.5s) and re-applies the slow to every player, so calling it each Think
	// keeps the match slowed; when slowmatch returns to >=1.0 we restore once.
	{
		const float sm = CVAR_GET_FLOAT("slowmatch");
		if (sm > 0.0f && sm < 1.0f)
		{
			GlobalSlowMotion2(sm, 0);
			m_bSlowMatchActive = true;
		}
		else if (m_bSlowMatchActive)
		{
			m_bSlowMatchActive = false;
			GlobalSlowMotion2(0, 0); // restore normal speed
		}
	}

	// A scheduled deads-check (from CallCheckDeads, on a player death) resolves the round.
	if (m_flCheckDeads != 0 && gpGlobals->time >= m_flCheckDeads)
	{
		m_flCheckDeads = 0;
		CheckDeads();
	}

	// A scheduled round restart (from TeamWin) fires once its delay elapses.
	if (m_flRoundRestart != 0 && gpGlobals->time >= m_flRoundRestart)
	{
		RestartRound(); // sets m_flRoundRestart = 0 itself
	}

	// Team-round timer: arm it on the first team-mode frame (and after each restart), and end the
	// round in a draw when it expires (unless a win is already pending).
	if (IsTeamMode() && m_flRoundRestart == 0)
	{
		if (m_flRoundEnd == 0)
			StartRoundTimer();
		else if (gpGlobals->time >= m_flRoundEnd)
			RoundTimeLeft();
	}

	// "The One" juggernaut mode: keep a buffed One designated (unless it's capture-the-briefcase).
	if (CVAR_GET_FLOAT("the_one") != 0.0f && !IsCTB())
		SelectRandomTheOne();

	CheckVotes(); // tally the players' map/ff/teamplay votes (self-throttled to ~10s)

	if (gpGlobals->time < m_flNextPowerupSpawn)
		return;

	// Only deathmatch maps get dynamic powerups. IsTeamplay() now resolves to the team-mode
	// proxy (a ts_objective_manager present — see CTSGameRules::IsTeamMode), so the old
	// explicit objective-manager scan is folded into it.
	if (IsTeamplay())
	{
		m_flNextPowerupSpawn = gpGlobals->time + 20.0f;
		return;
	}

	int type = GetRandomPowerup(TS_PWUP_ALL);
	bool spawned = type != 0 && PutPowerUp(type);
	m_flNextPowerupSpawn = gpGlobals->time + (spawned ? 20.0f : 1.0f);
}

// @ GlobalSlowMotion2__12CTSGameRulesfb — apply (or clear, scale 0) a global slow-
// motion to every player. Throttled to ~0.5s except when turning off.
// SLICE: the original also slows live "grenade"/"WorldGun" projectiles; deferred
// (the per-entity GoSlow virtual on CBaseEntity isn't built — only CBasePlayer has it).
void CTSGameRules::GlobalSlowMotion2(float scale, int slowBullets)
{
	if (m_flNextGlobalSlow > gpGlobals->time && scale != 0.0f)
		return;
	m_flNextGlobalSlow = gpGlobals->time + 0.5f;

	for (int i = 1; i <= gpGlobals->maxClients; i++)
	{
		CBaseEntity *pEnt = UTIL_PlayerByIndex(i);
		if (!pEnt || !pEnt->IsPlayer())
			continue;
		CBasePlayer *pPlayer = (CBasePlayer *)pEnt;
		if (!pPlayer->InSlow())
		{
			pPlayer->GoSlow(scale, 0);
			pPlayer->m_bSlowBullets = slowBullets != 0;
		}
		if (scale == 0.0f)
			pPlayer->GoSlow(1.0f, 0); // restore
	}
}

// @ SetGlobalSlowMotion__12CTSGameRulesff — start/stop a timed global slow-motion.
void CTSGameRules::SetGlobalSlowMotion(float scale, float duration)
{
	if (scale == m_flGlobalSlowScale)
		return;

	if (scale == 0.0f)
	{
		m_flGlobalSlowActive = 0;
		return;
	}

	GlobalSlowMotion2(0, 0); // reset everyone first
	m_flNextGlobalSlow = gpGlobals->time;
	m_flGlobalSlowScale = scale;
	m_flGlobalSlowActive = 1.0f;
	m_flGlobalSlowEnd = (duration == 0.0f) ? 0.0f : duration + gpGlobals->time;
}


// @ IsTeamMode — is this a team-play map? Faithful gate is the TS game-mode member;
// functional proxy is the presence of a ts_objective_manager (placed only by tm_ maps).
// Resolved lazily and cached: the rules object is installed at worldspawn, before the
// objective entities spawn, so we can't decide at construction time.
bool CTSGameRules::IsTeamMode()
{
	if (m_iTeamMode == -1)
	{
		bool bTeam = UTIL_FindEntityByClassname(nullptr, "ts_objective_manager") != nullptr;
		m_iTeamMode = bTeam ? 1 : 0;
		if (bTeam)
			InitTeams();
	}
	return m_iTeamMode == 1;
}

bool CTSGameRules::IsTeamplay()
{
	return IsTeamMode();
}

// GetGameDescription is inherited from CHalfLifeMultiplay (theOne->LMS->DM, no teamplay branch) —
// the original defines no CTSGameRules override. See multiplay_gamerules.cpp.

// @ InitTeams — seed the TS default team list. The .so builds "Specialists;Mercenaries"
// (models "spec"/"merc") for a team map; we keep only the names/models the rest of the
// reconstruction actually consumes.
void CTSGameRules::InitTeams()
{
	SetTeamList("Specialists;Mercenaries");        // -> the team-name slots + RecountTeams
	SetTeamModelList("spec|merc");                 // -> the per-team player models
}

// @0x680dc SetTeamList — parse a ";"-separated team list into the roster (then RecountTeams).
void CTSGameRules::SetTeamList(const char *pList)
{
	strncpy(m_szTeamList, pList ? pList : "", sizeof(m_szTeamList) - 1);
	m_szTeamList[sizeof(m_szTeamList) - 1] = '\0';
	RecountTeams();
}

// @0x68258 SetTeamModelList — parse a "|"-separated model list into the per-team model slots
// (the original splits up to 4, then calls the per-slot SetTeamModelList(i, model)).
void CTSGameRules::SetTeamModelList(const char *pList)
{
	char buf[2048];
	strncpy(buf, pList ? pList : "", sizeof(buf) - 1);
	buf[sizeof(buf) - 1] = '\0';
	for (int i = 0; i < TS_MAX_TEAMS; i++)
		m_teamModels[i][0] = '\0';
	int i = 0;
	for (char *p = strtok(buf, "|"); p && i < TS_MAX_TEAMS; p = strtok(nullptr, "|"), i++)
		SetTeamModelList(i, p);
}

// per-slot model setter (the original's SetTeamModelList(int, char*)).
void CTSGameRules::SetTeamModelList(int team, const char *pModel)
{
	if (team < 0 || team >= TS_MAX_TEAMS || !pModel)
		return;
	strncpy(m_teamModels[team], pModel, TS_TEAMNAME_LEN - 1);
	m_teamModels[team][TS_TEAMNAME_LEN - 1] = '\0';
}

// @0x685a8 GetTeamsNum — number of active (named) teams.
int CTSGameRules::GetTeamsNum()
{
	return m_numTeams;
}

// @0x69aac GetTeamCash — the player's team cash pool (0 outside team mode / no team). The cash
// economy is hard-OFF in the shipped game, so this returns 0 in practice.
int CTSGameRules::GetTeamCash(CBasePlayer *pPlayer)
{
	if (!IsTeamMode() || !pPlayer)
		return 0;
	int team = GetTeamIndex(pPlayer->TeamID());
	if (team < 0 || team >= TS_MAX_TEAMS)
		return 0;
	return m_teamCash[team];
}

// @0x69130 InitTeamScore / @0x67908 UpdateTeamScores — no-ops in the original (kept for parity).
void CTSGameRules::InitTeamScore(int *pScores) {}
void CTSGameRules::UpdateTeamScores(int a, int b) {}

// @0x69a08 GetPlayerModel — a model for the player's team. The original picks at random among the
// team's (up to 3) models; our roster carries one model per team, so we return that.
const char *CTSGameRules::GetPlayerModel(CBasePlayer *pPlayer)
{
	if (!pPlayer)
		return "";
	int team = GetTeamIndex(pPlayer->TeamID());
	if (team < 0 || team >= TS_MAX_TEAMS || m_teamModels[team][0] == '\0')
		return "";
	return m_teamModels[team];
}

// @0x696c8 TogglePlayerFromSlow — take a player out of slow-motion. The original removes them from
// the gamerules' slowed-player registry; our GlobalSlowMotion2 re-applies each frame (no persistent
// array), so we just restore the player to normal speed.
void CTSGameRules::TogglePlayerFromSlow(CBasePlayer *pPlayer)
{
	if (pPlayer)
		pPlayer->GoSlow(1.0f, 0);
}

// @ RecountTeams — re-tokenize m_szTeamList (";"-separated) into m_teamNames/m_numTeams.
// Mirrors CHalfLifeTeamplay::RecountTeams' parse half (strtok is destructive → copy first).
void CTSGameRules::RecountTeams()
{
	char teamlist[256];
	strncpy(teamlist, m_szTeamList, sizeof(teamlist) - 1);
	teamlist[sizeof(teamlist) - 1] = '\0';

	m_numTeams = 0;
	for (char *pName = strtok(teamlist, ";"); pName && *pName; pName = strtok(nullptr, ";"))
	{
		if (m_numTeams >= TS_MAX_TEAMS)
			break;
		if (GetTeamIndex(pName) < 0) // skip duplicates
		{
			strncpy(m_teamNames[m_numTeams], pName, TS_TEAMNAME_LEN - 1);
			m_numTeams++;
		}
	}
}

// @ GetTeamIndex — 0-based index of `pTeamName` in the roster, or -1. Case-insensitive,
// matching the .so's strcasecmp over the team-name fields.
int CTSGameRules::GetTeamIndex(const char *pTeamName)
{
	if (pTeamName && *pTeamName)
	{
		for (int tm = 0; tm < m_numTeams; tm++)
			if (!stricmp(m_teamNames[tm], pTeamName))
				return tm;
	}
	return -1;
}

const char *CTSGameRules::GetIndexedTeamName(int teamIndex)
{
	if (teamIndex < 0 || teamIndex >= m_numTeams)
		return "";
	return m_teamNames[teamIndex];
}

const char *CTSGameRules::GetTeamID(CBaseEntity *pEntity)
{
	if (pEntity == nullptr || pEntity->pev == nullptr)
		return "";
	return pEntity->TeamID();
}

// @ TeamWithFewestPlayers — the team currently holding the fewest live players (auto-balance
// pick). Mirrors CHalfLifeTeamplay::TeamWithFewestPlayers.
const char *CTSGameRules::TeamWithFewestPlayers()
{
	int teamCount[TS_MAX_TEAMS] = {};

	for (int i = 1; i <= gpGlobals->maxClients; i++)
	{
		CBaseEntity *plr = UTIL_PlayerByIndex(i);
		if (!plr)
			continue;
		int team = GetTeamIndex(plr->TeamID());
		if (team >= 0)
			teamCount[team]++;
	}

	int minPlayers = TS_MAX_TEAMS + 1;
	const char *pTeamName = m_numTeams > 0 ? m_teamNames[0] : "";
	for (int i = 0; i < m_numTeams; i++)
	{
		if (teamCount[i] < minPlayers)
		{
			minPlayers = teamCount[i];
			pTeamName = m_teamNames[i];
		}
	}
	return pTeamName;
}

// @ AssignToTeam — put a player on `pTeamName`: set pev->team (0-based, the convention the
// objective gating reads), the team-name string, the model/team infokeys (so a real client
// would render the team model), and broadcast the HUD team + score info.
void CTSGameRules::AssignToTeam(CBasePlayer *pPlayer, const char *pTeamName)
{
	int team = GetTeamIndex(pTeamName);
	if (team < 0)
		return;

	strncpy(pPlayer->m_szTeamName, pTeamName, TEAM_NAME_LENGTH - 1);
	pPlayer->m_szTeamName[TEAM_NAME_LENGTH - 1] = '\0';
	pPlayer->pev->team = team; // 0-based (cf. ts_objectives.cpp CanInteract / pev->team gate)

	int clientIndex = pPlayer->entindex();
	char *pInfo = g_engfuncs.pfnGetInfoKeyBuffer(pPlayer->edict());
	const char *pModel = GetPlayerModel(pPlayer);
	g_engfuncs.pfnSetClientKeyValue(clientIndex, pInfo, "model", (pModel && pModel[0]) ? pModel : pTeamName);
	g_engfuncs.pfnSetClientKeyValue(clientIndex, pInfo, "team", pPlayer->m_szTeamName);

	// notify everyone's HUD of the team membership + score row
	MESSAGE_BEGIN(MSG_ALL, gmsgTeamInfo);
	WRITE_BYTE(clientIndex);
	WRITE_STRING(pPlayer->m_szTeamName);
	MESSAGE_END();

	MESSAGE_BEGIN(MSG_ALL, gmsgScoreInfo);
	WRITE_BYTE(clientIndex);
	WRITE_SHORT((int)pPlayer->pev->frags);
	WRITE_SHORT(pPlayer->m_iDeaths);
	WRITE_SHORT(pPlayer->pev->playerclass); // 11=WAITING while spectate-waiting, else 0
	WRITE_SHORT(team + 1); // scoreboard team column is 1-based
	MESSAGE_END();
}

// @ PlayerSpawn — the MP spawn, then (team mode only) auto-balance any not-yet-assigned
// player onto the team with the fewest players. Idempotent: a player who already holds a
// valid team keeps it across respawns/round restarts.
void CTSGameRules::PlayerSpawn(CBasePlayer *pPlayer)
{
	// Faithful to the original CHalfLifeMultiplay::PlayerSpawn @0x790ac (the TS build
	// replaced the stock crowbar+9mmhandgun loadout): honour game_player_equip, else give
	// the unified TS weapon (weapon_tsgun — the arsenal, which defaults to TSW_MELEE / fists)
	// and apply the player's armament (cont.36 BuyArmament). NO stock HL weapons.
	pPlayer->SetHasSuit(true); // VERIFY: the .so does an early virtual here; our stock-derived HUD needs the suit bit

	// In team modes the player's starting wallet seeds from the team pool (0 in the shipped game,
	// where the cash economy is compiled out — so this is inert; GetTeamCash returns 0).
	if (int teamCash = GetTeamCash(pPlayer))
		pPlayer->m_iCash = teamCash;

	bool addDefault = true;
	CBaseEntity *pEquip = NULL;
	while ((pEquip = UTIL_FindEntityByClassname(pEquip, "game_player_equip")) != NULL)
	{
		pEquip->Touch(pPlayer);
		addDefault = false;
	}

	// TS (cont.65): a player entering free-look spectate-wait gets NO weapons — they
	// equip on fire-to-play (the next Spawn runs with m_fInSpectateWait clear).
	if (addDefault && !pPlayer->m_fInSpectateWait)
	{
		// Round restart (TSRestartRound) respawns STILL-ALIVE players, but CBasePlayer::Spawn does
		// NOT strip items — so without this the old weapon_tsgun survives and GiveNamedItem creates a
		// SECOND one. The two tsguns then fight over active/WStatus and the arsenal "disappears"
		// inconsistently on round restart. Clear first so the re-equip is clean (one tsgun).
		pPlayer->RemoveAllItems(false);
		pPlayer->GiveNamedItem("weapon_tsgun");
		pPlayer->BuyArmament();
		// The tsgun is the player's single weapon entity, but the engine's auto-deploy skips
		// kung-fu, so force it active + deployed (else a kung-fu-only loadout has no viewmodel
		// and can't fire — ItemPostFrame only runs on the active item).
		TSDeployArsenal(pPlayer);
	}

	// `initial_cash` cvar: the player's starting cash. Given once (when they have none) so
	// it doesn't accumulate across respawns. (CashBased() is off — cont.36 — so cash isn't
	// charged; this seeds the cash HUD / the win/lose economy cvars.)
	if (pPlayer->m_iCash == 0)
		pPlayer->GiveCash((int)CVAR_GET_FLOAT("initial_cash"));

	if (!IsTeamMode())
		return;

	if (GetTeamIndex(pPlayer->m_szTeamName) < 0)
		AssignToTeam(pPlayer, TeamWithFewestPlayers());
}

// @ InitHUD — send the team roster to a connecting client (so a real HUD can colour the
// scoreboard). The MP base does the rest of the HUD setup.
void CTSGameRules::InitHUD(CBasePlayer *pPlayer)
{
	CHalfLifeMultiplay::InitHUD(pPlayer);

	if (!IsTeamMode())
		return;

	const int nTeams = GetTeamsNum();
	MESSAGE_BEGIN(MSG_ONE, gmsgTeamNames, NULL, pPlayer->edict());
	WRITE_BYTE(nTeams);
	for (int i = 0; i < nTeams; i++)
		WRITE_STRING(m_teamNames[i]);
	MESSAGE_END();

	// catch the new client up on everyone's current team
	for (int i = 1; i <= gpGlobals->maxClients; i++)
	{
		CBaseEntity *plr = UTIL_PlayerByIndex(i);
		if (plr && plr->TeamID() && *plr->TeamID())
		{
			MESSAGE_BEGIN(MSG_ONE, gmsgTeamInfo, NULL, pPlayer->edict());
			WRITE_BYTE(plr->entindex());
			WRITE_STRING(plr->TeamID());
			MESSAGE_END();
		}
	}

	// catch the new client up on the round clock (StartRoundTimer only broadcasts at arm-time)
	SendRoundTime(pPlayer);
}
