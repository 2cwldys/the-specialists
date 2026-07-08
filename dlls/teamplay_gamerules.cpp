/***
*
*	Copyright (c) 1996-2001, Valve LLC. All rights reserved.
*	
*	This product contains software technology licensed from Id 
*	Software, Inc. ("Id Technology").  Id Technology (c) 1996 Id Software, Inc. 
*	All Rights Reserved.
*
*   Use, distribution, and modification of this source code and/or resulting
*   object code is restricted to non-commercial enhancements to products from
*   Valve LLC.  All other use, distribution, or modification is prohibited
*   without written permission from Valve LLC.
*
****/
//
// teamplay_gamerules.cpp
//
#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "player.h"
#include "weapons.h"
#include "gamerules.h"
#include "teamplay_gamerules.h"
#include "game.h"
#include "UserMessages.h"
#include "ts_gamerules.h" // R1: the standalone TSGameRules object (the TS-patched Teamplay reads it)
#include "ts_utils.h"
extern cvar_t teammodels; // ts_cvars.cpp — "mp_teammodels" (the team model table source)

//TODO: these should be members of CHalfLifeTeamplay
static char team_names[MAX_TEAMS][MAX_TEAMNAME_LENGTH];
static int team_scores[MAX_TEAMS];
static int num_teams = 0;

// R1 phase 3 — the TS-patched ctor @0xd3e48 (heavily TS; decompile
// reference/dossiers/cont96/r1_rules_split_groundtruth.md §3.4): TSGameRules.Init() runs here
// (a 2nd time after the MP base ctor's — the original's shape), the team list flows
// worldspawn "mapteams" (teamoverride-gated) -> teamlist cvar -> CTSGameRules::SetTeamList,
// m_szTeamList is REBUILT from the TSGameRules slots (canonical names + The-ONE insertion),
// m_teamLimit is ALWAYS 1 (stock derived it from the list), and the model list flows the
// g_teamModels worldspawn conduit -> teammodels cvar -> SetTeamModelList.
char g_teamModels[0x800]; // worldspawn "teammodels" conduit (CWorld::KeyValue writes it)

CHalfLifeTeamplay::CHalfLifeTeamplay()
{
	if (TSGameRules)
	{
		TSGameRules->Init();                        // @0xd3e85
		TSGameRules->InitTeamScore(team_scores);    // @0xd3e95 — empty in the original @0x69130
	}
	m_DisableDeathMessages = false;
	m_DisableDeathPenalty = false;

	memset(team_names, 0, sizeof(team_names));
	memset(team_scores, 0, sizeof(team_scores));
	num_teams = 0;

	m_szTeamList[0] = 0;
	char savedModels[0x800];
	strncpy(savedModels, g_teamModels, sizeof(savedModels) - 1);
	savedModels[sizeof(savedModels) - 1] = 0;
	g_teamModels[0] = 0; // consumed

	// worldspawn "mapteams" (teamoverride-gated) beats the teamlist cvar
	edict_t* pWorld = CWorld::World->edict();
	if (pWorld && !FStringNull(pWorld->v.team) && 0 != teamoverride.value)
	{
		const char* pTeamList = STRING(pWorld->v.team);
		if (pTeamList && 0 != strlen(pTeamList))
			strncpy(m_szTeamList, pTeamList, TEAMPLAY_TEAMLISTLENGTH);
	}
	// live path (g_teamplay is never 2/3 — dead objective-mode leftovers, dossier §4.6)
	if (0 == strlen(m_szTeamList))
		strncpy(m_szTeamList, teamlist.string, TEAMPLAY_TEAMLISTLENGTH);
	if (0 == strlen(savedModels))
	{
		strncpy(savedModels, teammodels.string, sizeof(savedModels) - 1);
		savedModels[sizeof(savedModels) - 1] = 0;
	}

	if (TSGameRules && 0 != strlen(m_szTeamList))
		TSGameRules->SetTeamList(m_szTeamList);     // @0xd403f — tokenizes into the 4 slots

	// REBUILD m_szTeamList from the TSGameRules slots (canonical names + The-ONE insertion)
	m_szTeamList[0] = 0;
	if (TSGameRules)
	{
		for (int i = 0; i < TS_MAX_TEAMS; i++)
		{
			const char* nm = TSGameRules->GetIndexedTeamName(i);
			if (nm && nm[0])
			{
				strcat(m_szTeamList, nm);
				strcat(m_szTeamList, ";");
			}
		}
	}

	m_teamLimit = true; // TS: ALWAYS 1 (@0xd3ffx)

	if (TSGameRules && 0 != strlen(savedModels))
		TSGameRules->SetTeamModelList(savedModels); // @0xd4107

	RecountTeams();
}

#include "voice_gamemgr.h"
extern CVoiceGameMgr g_VoiceGameMgr;

void CHalfLifeTeamplay::Think()
{
	///// Check game rules /////
	static int last_frags;
	static int last_time;

	int frags_remaining = 0;
	int time_remaining = 0;

	g_VoiceGameMgr.Update(gpGlobals->frametime);

	if (g_fGameOver) // someone else quit the game already
	{
		CHalfLifeMultiplay::Think();
		return;
	}

	float flTimeLimit = CVAR_GET_FLOAT("mp_timelimit") * 60;

	time_remaining = (int)(0 != flTimeLimit ? (flTimeLimit - gpGlobals->time) : 0);

	if (flTimeLimit != 0 && gpGlobals->time >= flTimeLimit)
	{
		GoToIntermission();
		return;
	}

	float flFragLimit = fraglimit.value;
	if (0 != flFragLimit)
	{
		int bestfrags = 9999;
		int remain;

		// check if any team is over the frag limit
		for (int i = 0; i < num_teams; i++)
		{
			if (team_scores[i] >= flFragLimit)
			{
				GoToIntermission();
				return;
			}

			remain = flFragLimit - team_scores[i];
			if (remain < bestfrags)
			{
				bestfrags = remain;
			}
		}
		frags_remaining = bestfrags;
	}

	// Updates when frags change
	if (frags_remaining != last_frags)
	{
		g_engfuncs.pfnCvar_DirectSet(&fragsleft, UTIL_VarArgs("%i", frags_remaining));
	}

	// Updates once per second
	if (timeleft.value != last_time)
	{
		g_engfuncs.pfnCvar_DirectSet(&timeleft, UTIL_VarArgs("%i", time_remaining));
	}

	last_frags = frags_remaining;
	last_time = time_remaining;
}

//=========================================================
// ClientCommand
// the user has typed a command which is unrecognized by everything else;
// this check to see if the gamerules knows anything about the command
//=========================================================
bool CHalfLifeTeamplay::ClientCommand(CBasePlayer* pPlayer, const char* pcmd)
{
	if (g_VoiceGameMgr.ClientCommand(pPlayer, pcmd))
		return true;

	if (FStrEq(pcmd, "menuselect"))
	{
		if (CMD_ARGC() < 2)
			return true;

		int slot = atoi(CMD_ARGV(1));

		// select the item from the current menu

		return true;
	}

	return false;
}

void CHalfLifeTeamplay::UpdateGameMode(CBasePlayer* pPlayer)
{
	MESSAGE_BEGIN(MSG_ONE, gmsgGameMode, NULL, pPlayer->edict());
	WRITE_BYTE(1); // game mode teamplay
	MESSAGE_END();
}


// R1 phase 3 — TS-patched @0xd44ac (dossier §3.1): the team comes from the "team" INFOKEY
// (not the model), The-One mode forces the TSGameRules slot names, RecountTeams(true).
const char* CHalfLifeTeamplay::SetDefaultPlayerTeam(CBasePlayer* pPlayer)
{
	// (the original gates this on mode < 2 — ALWAYS TRUE live; modes 2/3 are dead leftovers)
	const char* src;
	if (0 == g_theOneMode)
		src = g_engfuncs.pfnInfoKeyValue(g_engfuncs.pfnGetInfoKeyBuffer(pPlayer->edict()), "team");
	else
		src = TSGameRules ? TSGameRules->GetIndexedTeamName(pPlayer->m_bTheOne ? 0 : 1) : "";
	strncpy(pPlayer->m_szTeamName, src, TEAM_NAME_LENGTH);

	RecountTeams(true);

	if (pPlayer->m_szTeamName[0] != '\0' && IsValidTeam(pPlayer->m_szTeamName) && 0 == defaultteam.value)
		return pPlayer->m_szTeamName; // keep the infokey team

	const char* pTeamName = (0 == defaultteam.value) ? TeamWithFewestPlayers() : team_names[0];
	strncpy(pPlayer->m_szTeamName, pTeamName, TEAM_NAME_LENGTH);
	return pPlayer->m_szTeamName;
}


//=========================================================
// InitHUD
//=========================================================
void CHalfLifeTeamplay::InitHUD(CBasePlayer* pPlayer)
{
	int i;

	SetDefaultPlayerTeam(pPlayer);
	CHalfLifeMultiplay::InitHUD(pPlayer);

	// Send down the team names
	MESSAGE_BEGIN(MSG_ONE, gmsgTeamNames, NULL, pPlayer->edict());
	WRITE_BYTE(num_teams);
	for (i = 0; i < num_teams; i++)
	{
		WRITE_STRING(team_names[i]);
	}
	MESSAGE_END();

	RecountTeams();

	char* mdls = g_engfuncs.pfnInfoKeyValue(g_engfuncs.pfnGetInfoKeyBuffer(pPlayer->edict()), "model");
	// update the current player of the team he is joining
	char text[1024];
	if (0 == strcmp(mdls, pPlayer->m_szTeamName))
	{
		sprintf(text, "* you are on team \'%s\'\n", pPlayer->m_szTeamName);
	}
	else
	{
		sprintf(text, "* assigned to team %s\n", pPlayer->m_szTeamName);
	}

	ChangePlayerTeam(pPlayer, pPlayer->m_szTeamName, false, false);
	UTIL_SayText(text, pPlayer);
	int clientIndex = pPlayer->entindex();
	RecountTeams();
	// update this player with all the other players team info
	// loop through all active players and send their team info to the new client
	for (i = 1; i <= gpGlobals->maxClients; i++)
	{
		CBaseEntity* plr = UTIL_PlayerByIndex(i);
		if (plr && IsValidTeam(plr->TeamID()))
		{
			MESSAGE_BEGIN(MSG_ONE, gmsgTeamInfo, NULL, pPlayer->edict());
			WRITE_BYTE(plr->entindex());
			WRITE_STRING(plr->TeamID());
			MESSAGE_END();
		}
	}
}


// R1 phase 3 — TS-patched @0xd488c (dossier §3.2): The-One forces the slot names; an empty/
// invalid name falls to TeamWithFewestPlayers; the MODEL infokey is stamped from
// CTSGameRules::GetPlayerModel (m_teamLimit==1 always — the live path @0xd4ad3); a NO-KILL
// change sends the player to spectate with a forced 5s respawn gate; TeamNames rebroadcasts.
void CHalfLifeTeamplay::ChangePlayerTeam(CBasePlayer* pPlayer, const char* pTeamName, bool bKill, bool bGib)
{
	int damageFlags = bGib ? DMG_ALWAYSGIB : DMG_NEVERGIB;
	int clientIndex = pPlayer->entindex();

	if (bKill)
	{
		m_DisableDeathMessages = true;
		m_DisableDeathPenalty = true;
		pPlayer->TakeDamage(CWorld::World->pev, CWorld::World->pev, 900, damageFlags);
		m_DisableDeathMessages = false;
		m_DisableDeathPenalty = false;
	}

	if (0 != g_theOneMode) // TS DELTA A: The-One mode forces the slot names
		pTeamName = TSGameRules ? TSGameRules->GetIndexedTeamName(pPlayer->m_bTheOne ? 0 : 1) : "";
	else if (!pTeamName || pTeamName[0] == '\0' || !IsValidTeam(pTeamName))
		pTeamName = TeamWithFewestPlayers();

	strncpy(pPlayer->m_szTeamName, pTeamName, TEAM_NAME_LENGTH);

	// TS DELTA B: the model infokey comes from the TEAM MODEL table (m_teamLimit==1 always in
	// TS — @0xd4ad3 is the live path; the stock model-as-team stamp is dead).
	const char* mdl = TSGameRules ? TSGameRules->GetPlayerModel(pPlayer) : pPlayer->m_szTeamName;
	if (!mdl || !mdl[0])
		mdl = pPlayer->m_szTeamName;
	g_engfuncs.pfnSetClientKeyValue(clientIndex, g_engfuncs.pfnGetInfoKeyBuffer(pPlayer->edict()), "model", (char*)mdl);
	g_engfuncs.pfnSetClientKeyValue(clientIndex, g_engfuncs.pfnGetInfoKeyBuffer(pPlayer->edict()), "team", pPlayer->m_szTeamName);
	// LOCAL SHIM (kept from the dissolved AssignToTeam): our objectives gate reads the 0-based
	// pev->team (ts_objectives.cpp CanInteract). // VERIFY the original CanInteract's team source.
	pPlayer->pev->team = g_pGameRules->GetTeamIndex(pPlayer->m_szTeamName);

	RecountTeams();

	// notify everyone's HUD of the team change
	MESSAGE_BEGIN(MSG_ALL, gmsgTeamInfo);
	WRITE_BYTE(clientIndex);
	WRITE_STRING(pPlayer->m_szTeamName);
	MESSAGE_END();

	MESSAGE_BEGIN(MSG_ALL, gmsgScoreInfo);
	WRITE_BYTE(clientIndex);
	WRITE_SHORT(pPlayer->pev->frags);
	WRITE_SHORT(pPlayer->m_iDeaths);
	WRITE_SHORT(0);
	WRITE_SHORT(g_pGameRules->GetTeamIndex(pPlayer->m_szTeamName) + 1);
	MESSAGE_END();

	// TS DELTA C: a NO-KILL team change (join/rebalance) sends the player to spectate with a
	// forced 5s respawn gate (@0xe4b3c region).
	if (!bKill && 0 == g_theOneMode)
	{
		pPlayer->StartObserver(pPlayer->pev->origin, pPlayer->pev->v_angle);
		SetPlayerRespawnTime(pPlayer, 5, true);
	}

	// TS DELTA D: rebroadcast the team roster
	MESSAGE_BEGIN(MSG_ALL, gmsgTeamNames);
	WRITE_BYTE(num_teams);
	for (int i = 0; i < num_teams; i++)
		WRITE_STRING(team_names[i]);
	MESSAGE_END();
}


//=========================================================
// ClientUserInfoChanged
//=========================================================
// INVENTED-INTEROP (cont.96, same class as the SrvSett re-broadcast): the connect-window
// MSG_ONE drop also eats Teamplay::InitHUD's TeamNames/TeamInfo catch-up on the ORIGINAL
// client -> blank team headings until some MSG_ALL rebroadcast. Resend the roster to the
// now-active joiner (idempotent; the client just re-stores names/teams).
void CHalfLifeTeamplay::ResendTeamRoster(CBasePlayer* pPlayer)
{
	MESSAGE_BEGIN(MSG_ONE, gmsgTeamNames, NULL, pPlayer->edict());
	WRITE_BYTE(num_teams);
	for (int i = 0; i < num_teams; i++)
		WRITE_STRING(team_names[i]);
	MESSAGE_END();

	for (int i = 1; i <= gpGlobals->maxClients; i++)
	{
		CBaseEntity* plr = UTIL_PlayerByIndex(i);
		if (plr && plr->TeamID() && plr->TeamID()[0] && IsValidTeam(plr->TeamID()))
		{
			MESSAGE_BEGIN(MSG_ONE, gmsgTeamInfo, NULL, pPlayer->edict());
			WRITE_BYTE(plr->entindex());
			WRITE_STRING(plr->TeamID());
			MESSAGE_END();
		}
	}
}

// R1 phase 3 — TS-patched @0xd4d18 (dossier §3.3): calls the TS-relocated MP::CUIC first;
// the live path (m_teamLimit==1 always) RE-STAMPS the model from GetPlayerModel — the stock
// model-as-team change mechanism is DEAD in TS (team changes go through `jointeam` ->
// ChangePlayerTeam instead).
void CHalfLifeTeamplay::ClientUserInfoChanged(CBasePlayer* pPlayer, char* infobuffer)
{
	CHalfLifeMultiplay::ClientUserInfoChanged(pPlayer, infobuffer); // @0x7b244 (the known six)

	char* mdls = g_engfuncs.pfnInfoKeyValue(infobuffer, "model");
	if (!stricmp(mdls, pPlayer->m_szTeamName))
		return;

	// live path (m_teamLimit == 1 always in TS): the model is server-owned — re-stamp it
	// from the team model table (@0xd4e46). The stock defaultteam/IsValidTeam change flow
	// below it is unreachable in TS (m_teamLimit never 0) — dropped, dossier §3.3.
	const char* mdl = TSGameRules ? TSGameRules->GetPlayerModel(pPlayer) : pPlayer->m_szTeamName;
	if (!mdl || !mdl[0])
		mdl = pPlayer->m_szTeamName;
	g_engfuncs.pfnSetClientKeyValue(pPlayer->entindex(),
		g_engfuncs.pfnGetInfoKeyBuffer(pPlayer->edict()), "model", (char*)mdl);
}

//=========================================================
// Deathnotice.
//=========================================================
void CHalfLifeTeamplay::DeathNotice(CBasePlayer* pVictim, entvars_t* pKiller, entvars_t* pevInflictor)
{
	if (m_DisableDeathMessages)
		return;

	if (pVictim && pKiller && (pKiller->flags & FL_CLIENT) != 0)
	{
		CBasePlayer* pk = (CBasePlayer*)CBaseEntity::Instance(pKiller);

		if (pk)
		{
			if ((pk != pVictim) && (PlayerRelationship(pVictim, pk) == GR_TEAMMATE))
			{
				MESSAGE_BEGIN(MSG_ALL, gmsgDeathMsg);
				WRITE_BYTE(ENTINDEX(ENT(pKiller)));		// the killer
				WRITE_BYTE(ENTINDEX(pVictim->edict())); // the victim
				WRITE_STRING("teammate");				// flag this as a teammate kill
				MESSAGE_END();
				return;
			}
		}
	}

	CHalfLifeMultiplay::DeathNotice(pVictim, pKiller, pevInflictor);
}

//=========================================================
//=========================================================
void CHalfLifeTeamplay::PlayerKilled(CBasePlayer* pVictim, entvars_t* pKiller, entvars_t* pInflictor)
{
	if (!m_DisableDeathPenalty)
	{
		CHalfLifeMultiplay::PlayerKilled(pVictim, pKiller, pInflictor);
		RecountTeams();
	}
}


//=========================================================
// IsTeamplay
//=========================================================
bool CHalfLifeTeamplay::IsTeamplay()
{
	return true;
}

bool CHalfLifeTeamplay::FPlayerCanTakeDamage(CBasePlayer* pPlayer, CBaseEntity* pAttacker)
{
	if (pAttacker && PlayerRelationship(pPlayer, pAttacker) == GR_TEAMMATE)
	{
		// my teammate hit me.
		if ((friendlyfire.value == 0) && (pAttacker != pPlayer))
		{
			// friendly fire is off, and this hit came from someone other than myself,  then don't get hurt
			return false;
		}
	}

	return CHalfLifeMultiplay::FPlayerCanTakeDamage(pPlayer, pAttacker);
}

//=========================================================
//=========================================================
int CHalfLifeTeamplay::PlayerRelationship(CBaseEntity* pPlayer, CBaseEntity* pTarget)
{
	// half life multiplay has a simple concept of Player Relationships.
	// you are either on another player's team, or you are not.
	if (!pPlayer || !pTarget || !pTarget->IsPlayer())
		return GR_NOTTEAMMATE;

	if ((*GetTeamID(pPlayer) != '\0') && (*GetTeamID(pTarget) != '\0') && !stricmp(GetTeamID(pPlayer), GetTeamID(pTarget)))
	{
		return GR_TEAMMATE;
	}

	return GR_NOTTEAMMATE;
}

//=========================================================
//=========================================================
bool CHalfLifeTeamplay::ShouldAutoAim(CBasePlayer* pPlayer, edict_t* target)
{
	// always autoaim, unless target is a teammate
	CBaseEntity* pTgt = CBaseEntity::Instance(target);
	if (pTgt && pTgt->IsPlayer())
	{
		if (PlayerRelationship(pPlayer, pTgt) == GR_TEAMMATE)
			return false; // don't autoaim at teammates
	}

	return CHalfLifeMultiplay::ShouldAutoAim(pPlayer, target);
}

//=========================================================
//=========================================================
int CHalfLifeTeamplay::IPointsForKill(CBasePlayer* pAttacker, CBasePlayer* pKilled)
{
	if (!pKilled)
		return 0;

	if (!pAttacker)
		return 1;

	if (pAttacker != pKilled && PlayerRelationship(pAttacker, pKilled) == GR_TEAMMATE)
		return -1;

	return 1;
}

//=========================================================
//=========================================================
const char* CHalfLifeTeamplay::GetTeamID(CBaseEntity* pEntity)
{
	if (pEntity == NULL || pEntity->pev == NULL)
		return "";

	// return their team name
	return pEntity->TeamID();
}


int CHalfLifeTeamplay::GetTeamIndex(const char* pTeamName)
{
	if (pTeamName && *pTeamName != 0)
	{
		// try to find existing team
		for (int tm = 0; tm < num_teams; tm++)
		{
			if (!stricmp(team_names[tm], pTeamName))
				return tm;
		}
	}

	return -1; // No match
}


const char* CHalfLifeTeamplay::GetIndexedTeamName(int teamIndex)
{
	if (teamIndex < 0 || teamIndex >= num_teams)
		return "";

	return team_names[teamIndex];
}


bool CHalfLifeTeamplay::IsValidTeam(const char* pTeamName)
{
	if (!m_teamLimit) // Any team is valid if the teamlist isn't set
		return true;

	return (GetTeamIndex(pTeamName) != -1) ? true : false;
}

const char* CHalfLifeTeamplay::TeamWithFewestPlayers()
{
	int i;
	int minPlayers = MAX_TEAMS;
	int teamCount[MAX_TEAMS];
	char* pTeamName = NULL;

	memset(teamCount, 0, MAX_TEAMS * sizeof(int));

	// loop through all clients, count number of players on each team
	for (i = 1; i <= gpGlobals->maxClients; i++)
	{
		CBaseEntity* plr = UTIL_PlayerByIndex(i);

		if (plr)
		{
			int team = GetTeamIndex(plr->TeamID());
			if (team >= 0)
				teamCount[team]++;
		}
	}

	// Find team with least players
	for (i = 0; i < num_teams; i++)
	{
		if (teamCount[i] < minPlayers)
		{
			minPlayers = teamCount[i];
			pTeamName = team_names[i];
		}
	}

	return pTeamName;
}


//=========================================================
//=========================================================
void CHalfLifeTeamplay::RecountTeams(bool bResendInfo)
{
	char* pName;
	char teamlist[TEAMPLAY_TEAMLISTLENGTH];

	// loop through all teams, recounting everything
	num_teams = 0;

	// Copy all of the teams from the teamlist
	// make a copy because strtok is destructive
	strcpy(teamlist, m_szTeamList);
	pName = teamlist;
	pName = strtok(pName, ";");
	while (pName != NULL && '\0' != *pName)
	{
		if (GetTeamIndex(pName) < 0)
		{
			strcpy(team_names[num_teams], pName);
			num_teams++;
		}
		pName = strtok(NULL, ";");
	}

	if (num_teams < 2)
	{
		num_teams = 0;
		m_teamLimit = false;
	}

	// Sanity check
	memset(team_scores, 0, sizeof(team_scores));

	// loop through all clients
	for (int i = 1; i <= gpGlobals->maxClients; i++)
	{
		CBaseEntity* plr = UTIL_PlayerByIndex(i);

		if (plr)
		{
			const char* pTeamName = plr->TeamID();
			// try add to existing team
			int tm = GetTeamIndex(pTeamName);

			if (tm < 0) // no team match found
			{
				if (!m_teamLimit)
				{
					// add to new team
					tm = num_teams;
					num_teams++;
					team_scores[tm] = 0;
					strncpy(team_names[tm], pTeamName, MAX_TEAMNAME_LENGTH);
				}
			}

			if (tm >= 0)
			{
				team_scores[tm] += plr->pev->frags;
			}

			if (bResendInfo) //Someone's info changed, let's send the team info again.
			{
				if (plr && IsValidTeam(plr->TeamID()))
				{
					MESSAGE_BEGIN(MSG_ALL, gmsgTeamInfo, NULL);
					WRITE_BYTE(plr->entindex());
					WRITE_STRING(plr->TeamID());
					MESSAGE_END();

					// R1 phase 3 — the TS tail @0xd58aa (dossier §3.5): in capture-the-
					// briefcase mode, follow each TeamInfo with the per-team TeamScore rows
					// (MSG_ONE to this player) from the TSGameRules score slots.
					if (TSGameRules && TSGameRules->IsCTB())
					{
						for (int ti = 0; ti < TS_MAX_TEAMS; ti++)
						{
							const char* tn = TSGameRules->GetIndexedTeamName(ti);
							if (!tn || !tn[0])
								continue;
							MESSAGE_BEGIN(MSG_ONE, gmsgTeamScore, NULL, plr->edict());
							WRITE_STRING(tn);
							WRITE_SHORT(TSGameRules->m_teamScore[ti]);
							WRITE_SHORT(0);
							MESSAGE_END();
						}
					}
				}
			}
		}
	}
}
