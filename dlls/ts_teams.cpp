/***
*  ts_teams.cpp — RECONSTRUCTED CTSGameRules team subsystem (cont.35).
*  Functional/source-faithful (see CLAUDE.md). recovered/server/CTSGameRules.c:
*    InitTeams @0x77f60, SetTeamList @0x780dc, GetTeamsNum @0x785a8,
*    GetTeamIndex @0x79b04, GetPlayerModel @0x79a08, GetTeamCash @0x79aac.
*
*  The .so keeps a packed 0x8c-byte per-team struct (cash @+0x1c, scores @+0x20/+0x24,
*  name @+0x28, spawn classnames, model list @+0x38), four of them, gated on the TS
*  game-mode byte @+0 (2/3 = a team map). We model it naturally instead of transcribing
*  the byte layout: a string team list + parallel name/model arrays, sized to the live
*  player roster on demand. Team mode itself is detected by the same functional proxy the
*  dynamic-powerup gate already uses (a ts_objective_manager is present, i.e. a tm_ map).
*
*  The faithful trigger for a player ending up on a team is the client class-selection
*  menu (cl_dll, not built); the server-side stand-in here is auto-balance on first spawn
*  — the keystone that lets pev->team drive the already-built objective/CTF gating.
***/
#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "player.h"
#include "gamerules.h"
#include "ts_gamerules.h"
#include "ts_weapons.h" // TSDeployArsenal
#include "UserMessages.h"

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

// @ GetGameDescription — the mod name shown in the server browser, per game mode (the
// .so returns "The Specialists (DM/Teamplay/The One/LMS)"; recovered strings).
const char* CTSGameRules::GetGameDescription()
{
	if (IsTeamMode())
		return "The Specialists (Teamplay)";
	if (CVAR_GET_FLOAT("mp_theonemode") != 0.0f)
		return "The Specialists (The One)";
	if (CVAR_GET_FLOAT("lastmanstanding") != 0.0f)
		return "The Specialists (LMS)";
	return "The Specialists (DM)";
}

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
	g_engfuncs.pfnSetClientKeyValue(clientIndex, pInfo, "model", m_teamModels[team][0] ? m_teamModels[team] : pTeamName);
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
