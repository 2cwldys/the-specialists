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
#include "ts_weapon_info.h" // Set_ammo_count (the SrvSett id-2 pair)
#include "UserMessages.h"
#include "game.h"          // g_flWeaponCheat (sv_cheats as the map started)

// Defined in ts_objectives.cpp — fire the objective manager's targetontimeleft on round timeout.
void TSFireTimeLeftTarget();

// Round-time HUD message (remaining seconds); registered lazily on first send.
int gmsgTSRoundTime = 0;
int gmsgTSVoiceMessage = 0;   // "TSHVoice" (1): per-player capture taunt (sent from TS_objectives.cpp;
                              // DEFINED here — the original homes this global in TS_gamerulesclass.cpp)
int gmsgTSObjective = 0;      // "Objective" (-1, variable): one HUD objective entry (coord+idx+label)
int gmsgTSShowObjective = 0;  // "ShowObj" (2): toggle a HUD objective entry's visibility

// (StartRoundTimer DELETED, fold-plan G4: the original has no such method — the round-timer
// arm is INLINE in RestartRound @0x67920; Think @0x77204 never arms. See both call sites.)

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
	MESSAGE_BEGIN(MSG_ALL, gmsgTSShowObjective);
	WRITE_BYTE(idx);
	WRITE_BYTE(show ? 1 : 0);
	MESSAGE_END();
}

// @0x685d8 — round start: scan the map's objective managers, set the game mode, and start every
// objective DISABLED (a manager's Reset, via SetObjective, enables the chosen subset).
void CTSGameRules::InitObjectives()
{
	// @0x68602 ENTRY GATE (§4.6): the objective subsystem is the team modes 2/3, which the shipped
	// game NEVER reaches — DecideGamePlay caps at 1 and this only re-flips 2<->3 when the mode is
	// ALREADY 2/3. It does NOT promote plain teamplay (mode 1). Our old `if (g_teamplay) mode=2/3`
	// was INVENTED and turned regular TDM into the dead round-based objective mode -> "wait til next
	// round" on a free-respawn team map. So for mode 0/1 this is a no-op (objectives stay dead;
	// teamplay stays mode 1 = free respawn). CTF/CTB works at mode 1 via the briefcase/flag entities.
	if ((unsigned char)(m_iGameMode - 2) > 1)
		return;

	m_iManagerCount = 0;
	for (int i = 0; i < 32; i++)
		m_managers[i] = nullptr;
	m_pActiveManager = nullptr;

	CBaseEntity *pEnt = nullptr;
	while ((pEnt = UTIL_FindEntityByClassname(pEnt, "ts_objective_manager")) != nullptr)
		if (m_iManagerCount < 32)
			m_managers[m_iManagerCount++] = (CTSObjectiveManager *)pEnt;

	// @0x68693 — flip 2<->3 by objective-manager presence (mode is already 2/3 to be here).
	m_iGameMode = (m_iManagerCount != 0) ? 3 : 2;
	g_teamplay = true;
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

// (SendRoundTime DELETED, fold-plan G3: the original has no such method — the joiner
// round-clock catch-up is INLINE in CTSGameRules::InitHUD @0x69134, where it now lives.)

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

// (CTSGameRules::PlayerKilled DELETED — R1/Y8: the original CTSGameRules is a STANDALONE class
// with NO PlayerKilled; the virtual is the TS-patched CHalfLifeMultiplay::PlayerKilled @0x795e4
// (multiplay_gamerules.cpp — TheOneIsDead + CallCheckDeads live there now). The weapon/ammo
// death-scatter moved to its @0x7ed5c home in CBasePlayer::Killed. The DropFlag(false) call was
// a DOUBLE mechanism — the original drops the carrier's flag at the StartObserver spectate
// transition (@0x8975f), which our StartObserver already does; a killed carrier's flag now drops
// ~3 s later than before — original-faithful timing. The corpse spawns in PlayerDeathThink.)

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
	if (m_flRoundRestart != 0.0f || !g_teamplay)
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
	// R1 phase 2: the object is STATIC (@0x1681a0 — ctor runs at static-init, before
	// gpGlobals is valid). Per-map setup happens in Init(), called from InstallGameRules —
	// the original's ctor/Init split (__12CTSGameRules + Init__12CTSGameRules @0x67d6c).
}

void CTSGameRules::Init()
{
	m_iGameMode = g_teamplay ? 1 : 0; // @0x67d83 — m_iGameMode = (byte)g_teamplay (0/1; 2/3 dead §4.6)
	InitTeams();
	InitTeamScore(m_teamScore); // (no-op in the original)
	m_flRoundEnd = 0;
	m_flRoundRestart = gpGlobals->time + 5.0f; // first round fires 5s into the map
	m_bRoundTimerEnabled = true;
	m_flNextTheOneSelect = gpGlobals->time + 10.0f; // @0x67eab (+0x587c) — first The-One crown 10s in
	m_szEndMap[0] = '\0';                            // @0x67e73 (+0x1824) — clear any pending forced map
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
	const bool lms = g_lastManStanding != 0; // DecideGamePlay's flag (the old "lms" cvar read was dead — never registered)
	CBaseEntity *e;

	for (e = nullptr; (e = UTIL_FindEntityByClassname(e, "info_breakable")) != nullptr;)
		((CInfoBreakable *)e)->RespawnThink();

	if (g_teamplay || lms)
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
	const bool roundBased = g_teamplay || g_lastManStanding != 0;

	// (c) reset the objective subsystem — a no-op on a map with no objective entities (e.g. plain DM).
	InitObjectives();
	SetObjective();

	if (roundBased)
	{
		// (a)+(b) @0x67920 (fold-plan G4, decompile-exact): arm the round timer INLINE — the
		// `roundtime` cvar clamped [30,180]; the ARM is unconditional, only the clock BROADCAST
		// is gated on the round-timer flag (field_0x10).
		{
			float t = CVAR_GET_FLOAT("roundtime");
			if (t < 30.0f) t = 30.0f;
			if (t > 180.0f) t = 180.0f;
			m_flRoundEnd = gpGlobals->time + t;

			if (m_bRoundTimerEnabled)
			{
				MESSAGE_BEGIN(MSG_ALL, gmsgTSRoundTime); // "RoundTime" 2 — registered centrally in LinkUserMessages
				WRITE_BYTE((int)(m_flRoundEnd - gpGlobals->time)); // seconds remaining
				WRITE_BYTE(0);                                     // flag: 0 = round start/restart
				MESSAGE_END();
			}
		}

		// (d) team banner.
		if (g_teamplay)
			TSHudMessageAll(0, 0xff, 0, 8, 30, "Defeat the opposite team");

		// (e) respawn the players into the fresh round. SLICE: the original instead refunds the loadout's
		// cash + forces each player back into the class-select observer state; we respawn directly.
		for (int i = 1; i <= gpGlobals->maxClients; i++)
		{
			CBasePlayer *p = (CBasePlayer *)UTIL_PlayerByIndex(i);
			if (p)
			{
				p->DropFlag(true); // clear any carried flag (the briefcases re-seed fresh ones below)
				// @0x67ae8/@0x67b1a: fresh round — hold re-armed +10, gate +3, award 11, the
				// death counter cleared (the LMS respawn budget resets each round).
				p->m_flRespawnHoldTime = gpGlobals->time + 10.0f;
				p->m_flTSRespawnTime = gpGlobals->time + 3.0f;
				p->m_iAward = 11;
				p->m_iRespawnsUsed = 0; // @0x67b2e
				respawn(p->pev, false);
			}
		}
	}

	// (f) respawn the map's resettable entities. (Team roster is stable after SetTeamList —
	// the invented RecountTeams re-tokenize was removed, R1 item-9.)
	RespawnEntities(true);
}

// @0x68750 ResetRestartRound — at round end (team/last-man only) arm a 1s restart + announce. With
// depth > 1 it also wipes the per-team round scores/cash. Replaces the ad-hoc restart scheduling.
void CTSGameRules::ResetRestartRound(int depth)
{
	if (!g_teamplay && CVAR_GET_FLOAT("lms") == 0.0f)
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
	g_engfuncs.pfnSetPhysicsKeyValue(pPlayer->edict(), "mj", "1"); // "The One": the DUCK+JUMP high jump (PM_Jump)
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
		strncpy(m_szEndMap, maps[best], sizeof(m_szEndMap) - 1);
}


// @ CTSGameRules::Think — run the inherited MP think, then (in non-team deathmatch) drive
// the dynamic powerup spawner: roll a random powerup every ~20s and PutPowerUp it (retry in
// 1s if no spawn point was free). Mirrors the .so's Think powerup block.

void CTSGameRules::Think()
{
	// (R1 phase 2: no base call — this is the original's NON-VIRTUAL Think @0x67204, invoked
	// by the TS-patched CHalfLifeMultiplay::Think @0xbf302 / CHalfLifeTeamplay::Think @0xd439a.)

	g_BulletManager.Think(); // TS: fly the slow-motion bullets-as-models each frame (ts_bullet.cpp)

	// `globalslow` command / SetGlobalSlowMotion driver (RE CTSGameRules::Think @0x67204, the FIRST
	// block): expire a timed slow, restore normal speed ONCE when it goes inactive (and clear
	// scale/end), else re-apply the scale each Think (GlobalSlowMotion2 self-throttles ~0.5s). Without
	// this, the newly-registered `globalslow` command set m_flGlobalSlowActive but nothing consumed it.
	// cont.91 (cvar_settings_audit.md fix 1). NOTE: coexists with the slowmatch block below — both call
	// GlobalSlowMotion2; they are mutually-exclusive server settings in practice. VERIFY if both on.
	if (m_flGlobalSlowEnd != 0.0f && m_flGlobalSlowEnd < gpGlobals->time)
		m_flGlobalSlowActive = 0;
	if (m_flGlobalSlowActive == 0.0f)
	{
		if (m_flGlobalSlowScale != 0.0f)
		{
			GlobalSlowMotion2(0.0f, 0); // restore normal speed once
			m_flGlobalSlowScale = 0.0f;
			m_flGlobalSlowEnd = 0.0f;
		}
	}
	else
	{
		GlobalSlowMotion2(m_flGlobalSlowScale, 0); // maintain the global slow
	}

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
	// (@0x67204 — unconditional; m_flCheckDeads is only armed in round modes.)
	if (m_flCheckDeads != 0 && gpGlobals->time >= m_flCheckDeads)
	{
		m_flCheckDeads = 0;
		CheckDeads();
	}

	// Round control — MODE-GATED exactly like the original Think (@0x67204, tsthink.c lines 96-121).
	// The round system (RestartRound / round timer / CheckDeads-on-expiry) runs ONLY for
	// last-man-standing (m_iGameMode==0 + g_lastManStanding) and the dead objective modes
	// (m_iGameMode>=2, §4.6). PLAIN DEATHMATCH (mode 0) and PLAIN TEAMPLAY/TDM (mode 1) are
	// FREE-RESPAWN — no rounds. (Fixes cont.94's over-broad `roundBased = g_teamplay || lms`
	// gate, which made mode-1 TDM round-based: "wait til next round" while alive, timer restarts.)
	if (m_iGameMode < 2)
	{
		if (g_lastManStanding != 0 && m_iGameMode == 0) // last-man-standing only
		{
			if (m_flRoundRestart != 0 && gpGlobals->time >= m_flRoundRestart)
				RestartRound(); // arms m_flRoundEnd + broadcasts RoundTime + respawns; clears m_flRoundRestart
			if (gpGlobals->time >= m_flRoundEnd && m_flRoundRestart == 0)
				CheckDeads();
		}
	}
	else // m_iGameMode >= 2 — dead objective modes (unreachable in 2.1.x, §4.6; kept for fidelity)
	{
		if (gpGlobals->time >= m_flRoundEnd && m_flRoundRestart == 0 && m_bRoundTimerEnabled)
		{
			if (m_iGameMode == 2)
				TeamWin(2);
			else
				RoundTimeLeft();
		}
		if (m_flRoundRestart != 0 && gpGlobals->time >= m_flRoundRestart)
			RestartRound();
	}

	// "The One" juggernaut mode: keep a buffed One designated (unless it's capture-the-briefcase).
	if (CVAR_GET_FLOAT("the_one") != 0.0f && !IsCTB())
		SelectRandomTheOne();

	CheckVotes(); // tally the players' map/ff/teamplay votes (self-throttled to ~10s)

	// @0x67323: a powerup not yet due skips only the spawn; the rest of Think (the SrvSett tail
	// below, where a new ammocount reaches Set_ammo_count) runs every frame.
	if (gpGlobals->time >= m_flNextPowerupSpawn)
	{
		// Only deathmatch maps get dynamic powerups. g_teamplay now resolves to the team-mode
		// proxy (a ts_objective_manager present — see CTSGameRules::IsTeamMode), so the old
		// explicit objective-manager scan is folded into it.
		if (g_teamplay)
		{
			m_flNextPowerupSpawn = gpGlobals->time + 20.0f;
		}
		else
		{
			int type = GetRandomPowerup(TS_PWUP_ALL);
			bool spawned = type != 0 && PutPowerUp(type);
			m_flNextPowerupSpawn = gpGlobals->time + (spawned ? 20.0f : 1.0f);
		}
	}

	// --- SrvSett on-change broadcasts (the tail of the original Think; star_SrvSett.md) ---
	if (gmsgTSServerSetting != 0)
	{
		int v = (int)(CVAR_GET_FLOAT("realbullet") + 0.5f);
		if (v != m_iSrvRealBullet)
		{
			m_iSrvRealBullet = v;
			MESSAGE_BEGIN(MSG_ALL, gmsgTSServerSetting);
			WRITE_BYTE(0);
			WRITE_SHORT(v);
			MESSAGE_END();
		}
		v = (int)(CVAR_GET_FLOAT("usecash") + 0.5f);
		if (v != m_iSrvUseCash)
		{
			m_iSrvUseCash = v;
			MESSAGE_BEGIN(MSG_ALL, gmsgTSServerSetting);
			WRITE_BYTE(1);
			WRITE_SHORT(v != 0 ? 1 : 0); // cached rounded, WIRED as a bool
			MESSAGE_END();
		}
		v = (int)(CVAR_GET_FLOAT("ammocount") + 0.5f);
		if (v != m_iSrvAmmoCount)
		{
			m_iSrvAmmoCount = v;
			Set_ammo_count(v); // the shared max-carry column update, BEFORE the send
			MESSAGE_BEGIN(MSG_ALL, gmsgTSServerSetting);
			WRITE_BYTE(2);
			WRITE_SHORT(v);
			MESSAGE_END();
		}
	}
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


// (IsTeamMode + IsTeamplay DELETED — R1 item-9 convergence: invented CTSGameRules stand-ins.
// The original's team gate at each site is the global `g_teamplay != 0` (== m_iGameMode>=1, since
// m_iGameMode = (byte)g_teamplay and modes 2/3 are dead §4.6); callers now read g_teamplay directly.
// The team-play VIRTUAL is stock CHalfLifeTeamplay::IsTeamplay (g_pGameRules->IsTeamplay()).)

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

// @0x680dc SetTeamList — parse a ";"-separated team list DIRECTLY into the 4 name slots (R1 item-9
// convergence, dossier §4.5): clear the slots, strtok on ";" (deduped), with The-ONE insertion
// (slot 0 forced "The ONE" when g_theOneMode; real teams start at slot 1), an ALERT per team, and
// cap = g_theOneMode ? 2 : 4. No m_szTeamList / no RecountTeams — those were invented.
void CTSGameRules::SetTeamList(const char *pList)
{
	for (int i = 0; i < TS_MAX_TEAMS; i++)
		m_teamNames[i][0] = '\0';
	m_numTeams = 0;

	if (g_theOneMode)
	{
		strncpy(m_teamNames[0], "The ONE", TS_TEAMNAME_LEN - 1);
		m_teamNames[0][TS_TEAMNAME_LEN - 1] = '\0';
		m_numTeams = 1; // real teams start at slot 1 in The-One mode
	}
	const int cap = g_theOneMode ? 2 : TS_MAX_TEAMS;

	char buf[256];
	strncpy(buf, pList ? pList : "", sizeof(buf) - 1);
	buf[sizeof(buf) - 1] = '\0';
	for (char *pName = strtok(buf, ";"); pName && *pName && m_numTeams < cap; pName = strtok(nullptr, ";"))
	{
		if (GetTeamIndex(pName) < 0) // skip duplicates
		{
			strncpy(m_teamNames[m_numTeams], pName, TS_TEAMNAME_LEN - 1);
			m_teamNames[m_numTeams][TS_TEAMNAME_LEN - 1] = '\0';
			ALERT(at_console, "created team %s\n", m_teamNames[m_numTeams]);
			m_numTeams++;
		}
	}
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
	if (!g_teamplay || !pPlayer)
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

// (RecountTeams DELETED — R1 item-9: invented CTSGameRules helper. Its ";"-tokenize now lives
// inline in SetTeamList (§4.5); the team roster is stable after SetTeamList, so RestartRound no
// longer re-tokenizes. The stock team recount is CHalfLifeTeamplay::RecountTeams on the installed
// Teamplay object.)

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

// (GetTeamID + TeamWithFewestPlayers DELETED — R1 item-9 convergence: invented CTSGameRules
// stand-ins with no callers. Team assignment / auto-balance lives on the installed TS-patched
// CHalfLifeTeamplay (SetDefaultPlayerTeam/ChangePlayerTeam/TeamWithFewestPlayers); an entity's
// team string is read via its own TeamID().)

// (AssignToTeam DELETED — R1 phase 3: the original assigns teams in the TS-patched
// CHalfLifeTeamplay::SetDefaultPlayerTeam @0xd44ac / ChangePlayerTeam @0xd488c
// (teamplay_gamerules.cpp); jointeam dispatches there @0x9e4b0. The pev->team stamp our
// version carried rides ChangePlayerTeam as a tagged shim for the objectives gate.)

// (CTSGameRules::PlayerSpawn DELETED — R1/Y9: the original CTSGameRules has NO PlayerSpawn;
// the virtual is the TS-patched CHalfLifeMultiplay::PlayerSpawn @0x790ac (multiplay_gamerules.cpp,
// where the equip core now lives). initial_cash moved to its ClientPutInServer home @0xacf44;
// the auto-balance tail is the tagged Y9-4/R1 interim in MP::PlayerSpawn.)

// @ InitHUD — send the team roster to a connecting client (so a real HUD can colour the
// scoreboard). The MP base does the rest of the HUD setup.
// "SrvSett" (3) — BYTE setting id + SHORT value. Ids: 0 realbullet, 1 usecash (bool on the
// wire), 2 ammocount, 3 weaponrestriction bitmask. InitHUD pushes ALL FOUR to a joiner
// (MSG_ONE); Think broadcasts ids 0-2 on cvar change (MSG_ALL); cmd_weaponrestriction
// broadcasts id 3 on change. star_SrvSett.md (decompile-settled).
int gmsgTSServerSetting = 0;

void CTSGameRules::InitHUD(CBasePlayer *pPlayer)
{
	// (R1 phase 2: no base call — this is the original's NON-VIRTUAL InitHUD @0x69134, invoked
	// from CBasePlayer::UpdateClientData's InitHUD block @0x81d00 AFTER g_pGameRules->InitHUD.)

	pPlayer->m_iRespawnsUsed = 0; // @0x694d1 — a joiner starts with a fresh respawn budget

	// items 1-2 of the original @0x69134 (dossier §2.4) — the cheats/damage notices come FIRST.
	if (g_flWeaponCheat != 0.0f) // @0x69147: sv_cheats as the map started
		TSHudMessageOne(0x2d, 0xc3, 0x2d, 8, 100, "This server has cheats enabled.", pPlayer->pev);
	{
		const float dm = CVAR_GET_FLOAT("damagemult");
		if (dm > 0.0f)
			TSHudMessageOne(0x2d, 0xc3, 0x2d, 8, 100,
				UTIL_VarArgs("This server has custom weapon damages (%.2f)", dm), pPlayer->pev);
	}
	// custom-weapons catch-up (@0x691d8, item 2): a joiner on a customized server gets the table.
	if (m_bWeaponParamsCustom)
		InformClientsOnWeaponsParams(pPlayer);

	// @0x69134: the joiner catches up on EVERY player's spectator state — MSG_ONE per
	// connected player: (BYTE idx, BYTE iuser1 != 0).
	for (int i = 1; i <= gpGlobals->maxClients; i++)
	{
		CBaseEntity *p = UTIL_PlayerByIndex(i);
		if (!p)
			continue;
		MESSAGE_BEGIN(MSG_ONE, gmsgSpectator, NULL, pPlayer->edict());
		WRITE_BYTE(i);
		WRITE_BYTE(p->pev->iuser1 != 0 ? 1 : 0);
		MESSAGE_END();
	}

	// SrvSett catch-up: the joiner gets all four settings (no late-joiner hole).
	// item 3: byte-0 sends the THINK-CACHED value (+0x578), not a fresh cvar read (Init
	// seeds it -1; Think refreshes on change) — dossier §2.4.
	MESSAGE_BEGIN(MSG_ONE, gmsgTSServerSetting, NULL, pPlayer->edict());
	WRITE_BYTE(0);
	WRITE_SHORT(m_iSrvRealBullet);
	MESSAGE_END();
	MESSAGE_BEGIN(MSG_ONE, gmsgTSServerSetting, NULL, pPlayer->edict());
	WRITE_BYTE(1);
	WRITE_SHORT(CVAR_GET_FLOAT("usecash") != 0.0f ? 1 : 0); // read DIRECT, sent as a bool
	MESSAGE_END();
	MESSAGE_BEGIN(MSG_ONE, gmsgTSServerSetting, NULL, pPlayer->edict());
	WRITE_BYTE(2);
	WRITE_SHORT((int)(CVAR_GET_FLOAT("ammocount") + 0.5f)); // read DIRECT (no Set_ammo_count here)
	MESSAGE_END();
	MESSAGE_BEGIN(MSG_ONE, gmsgTSServerSetting, NULL, pPlayer->edict());
	WRITE_BYTE(3);
	WRITE_SHORT(m_iWeaponRestriction);
	MESSAGE_END();

	// These MSG_ONE catch-ups fire during the spawn sequence and the original client can drop them
	// (its restriction global stays 0 -> empty buy menu). Schedule a one-shot re-broadcast once the
	// client is active (PreThink honours m_flSrvSettResendTime). See the member note in player.h.
	pPlayer->m_flSrvSettResendTime = gpGlobals->time + 0.75f;

	// (the TeamNames + TeamInfo roster catch-up is GONE from here — R1 phase 3: it lives in
	// the STOCK CHalfLifeTeamplay::InitHUD @0xd45d8, which runs on team maps now that Teamplay
	// is installed as g_pGameRules. Dossier §2.3/§2.5.)

	if (!g_teamplay)
		return;

	// @0x69134 (fold-plan G3, decompile-exact — inv_C lines 163-177): catch the new client up on
	// the round clock (the round-start broadcast only happens at arm-time). Gated on the timer
	// being ARMED (m_flRoundEnd != 0) AND the round-timer flag (field_0x10); an EXPIRED clock
	// sends a CLAMPED 0, it is not skipped.
	if (m_flRoundEnd != 0.0f && m_bRoundTimerEnabled)
	{
		float left = m_flRoundEnd - gpGlobals->time;
		if (left < 0)
			left = 0;
		MESSAGE_BEGIN(MSG_ONE, gmsgTSRoundTime, NULL, pPlayer->edict());
		WRITE_BYTE((int)(left + 0.5f));
		WRITE_BYTE(0);
		MESSAGE_END();
	}
}
