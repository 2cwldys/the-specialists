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
#include "ts_bulletmgr.h"
#include "ts_utils.h"   // TSHudMessageAll
#include "ts_objectives.h" // CTSObjective / CTSObjectiveManager (the objective subsystem)
#include "ts_func_break.h" // CInfoBreakable::RespawnThink (RespawnEntities)
#include "ts_groundweapon.h" // CTSGroundWeapon::Fall (RespawnEntities)

// DAT_0017da25 in the original: a cached flag that, when nonzero, removes the
// low-gravity (0x100) powerup from the random pool. Zero-init in the binary, so
// low-gravity is permitted by default. VERIFY: which cvar feeds this.
static byte g_iNoLowGravPowerup = 0;

// @ GetRandomPowerup__12CTSGameRulesi — collect the permitted powerup bits from
// `mask` and return a random one (0 if the pool is empty). Faithful to the .so:
// the 0x80 slot is always skipped, and 0x100 (low gravity) is gated.
int CTSGameRules::GetRandomPowerup(int mask)
{
	int list[9];
	int count = 0;

	for (int bit = 1; bit <= TS_PWUP_LOWGRAV; bit <<= 1)
	{
		if ((mask & bit) == 0)
			continue;
		if (bit == 0x80) // unused powerup slot
			continue;
		if (bit == TS_PWUP_LOWGRAV && g_iNoLowGravPowerup != 0)
			continue;
		list[count++] = bit;
	}

	if (count == 0)
		return 0;

	return list[RANDOM_LONG(0, count - 1)];
}

// Defined in ts_powerup.cpp — create a temporary powerup of `type` at `vecOrigin`.
CBaseEntity *TSCreatePowerUpAt(int type, const Vector &vecOrigin);
// Defined in ts_flag.cpp — drop a carried CTF flag on death.
void TSDropFlag(CBasePlayer *pPlayer);
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
	TSDropFlag(pVictim); // CTF: a killed carrier drops the flag where they fell
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
		e->Spawn(); // VERIFY: the original calls CTSFlag::RespawnFlag() (ts_flag.cpp-local)
}

// @0x67920 RestartRound — the actual round reset: re-arm the round timer, reset the objectives,
// respawn the players, recount teams, then respawn the map entities.
void CTSGameRules::RestartRound()
{
	if (CountPlayers() == 0) // nothing to restart on an empty server
		return;

	// (a) re-arm the round timer inline + disarm the pending restart.
	float t = CVAR_GET_FLOAT("roundtime");
	if (t < 30.0f) t = 30.0f;
	if (t > 180.0f) t = 180.0f;
	m_flRoundEnd = gpGlobals->time + t;
	m_flRoundRestart = 0;

	// (b) tell clients the new round clock.
	if (m_bRoundTimerEnabled && gmsgTSRoundTime != 0)
	{
		MESSAGE_BEGIN(MSG_ALL, gmsgTSRoundTime);
		WRITE_BYTE((int)(m_flRoundEnd - gpGlobals->time));
		WRITE_BYTE(0);
		MESSAGE_END();
	}

	// (c) reset the objectives, (d) the team-mode banner.
	InitObjectives();
	SetObjective();
	if (IsTeamMode())
		TSHudMessageAll(0, 0xff, 0, 8, 30, "Defeat the opposite team");

	// (e) respawn the players. SLICE: the original instead refunds the loadout's cash + forces each
	// player back into the class-select observer state; we respawn directly (no cash economy / class menu).
	for (int i = 1; i <= gpGlobals->maxClients; i++)
	{
		CBasePlayer *p = (CBasePlayer *)UTIL_PlayerByIndex(i);
		if (p)
			respawn(p->pev, false);
	}

	// (f) recount teams, (g) respawn the map entities.
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
	pPlayer->pev->health = 150.0f;
	pPlayer->m_flHealthRegen = gpGlobals->time + 1.0f;
	pPlayer->m_iKungFu = 1;
	pPlayer->m_bLowGravity = 1;
	pPlayer->pev->maxspeed = 330.0f; // SLICE: original CBasePlayer::SetPlayerSpeed(330)
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

// @ PutPowerUp__12CTSGameRulesi — spawn a powerup at a random info_player_deathmatch
// spawn point. Skips the spot if a ts_powerup is already nearby (so dynamically-spawned
// powerups don't stack). Returns true if one was created.
bool CTSGameRules::PutPowerUp(int type)
{
	// Collect the deathmatch spawn points (the .so caps at 32).
	CBaseEntity *spawns[32];
	int count = 0;
	CBaseEntity *pSpot = nullptr;
	while (count < 32 &&
		(pSpot = UTIL_FindEntityByClassname(pSpot, "info_player_deathmatch")) != nullptr)
		spawns[count++] = pSpot;

	if (count == 0)
		return false;

	Vector vecOrigin = spawns[RANDOM_LONG(0, count - 1)]->pev->origin + Vector(0, 0, 16);

	// Don't pile powerups onto a spot that already has one.
	CBaseEntity *pNear = nullptr;
	while ((pNear = UTIL_FindEntityInSphere(pNear, vecOrigin, 128.0f)) != nullptr)
		if (FClassnameIs(pNear->pev, "ts_powerup"))
			return false;

	return TSCreatePowerUpAt(type, vecOrigin) != nullptr;
}

// @ CTSGameRules::Think — run the inherited MP think, then (in non-team deathmatch) drive
// the dynamic powerup spawner: roll a random powerup every ~20s and PutPowerUp it (retry in
// 1s if no spawn point was free). Mirrors the .so's Think powerup block.

void CTSGameRules::Think()
{
	CHalfLifeMultiplay::Think();

	g_BulletManager.Think(); // TS: fly the slow-motion bullets-as-models each frame (ts_bulletmgr.cpp)

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
