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

	m_flRoundRestart = gpGlobals->time + 3.0f;
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
static void TSRestartRound()
{
	for (int i = 1; i <= gpGlobals->maxClients; i++)
	{
		CBasePlayer *p = (CBasePlayer *)UTIL_PlayerByIndex(i);
		if (p)
			respawn(p->pev, false);
	}
	CBaseEntity *e = nullptr;
	while ((e = UTIL_FindEntityByClassname(e, "ts_wingiver")) != nullptr)
		e->Spawn();
	e = nullptr;
	while ((e = UTIL_FindEntityByClassname(e, "ts_hack")) != nullptr)
		e->Spawn();
	TSHudMessageAll(0, 0xff, 0, 8, 30, "Round restart!");
	if (TSGameRules())
		TSGameRules()->m_flRoundEnd = 0; // re-arm the round timer for the new round (next Think)
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
		m_flRoundRestart = 0;
		TSRestartRound();
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
