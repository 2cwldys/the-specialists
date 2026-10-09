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

#include "skill.h"
#include "game.h"
#include "items.h"
#include "voice_gamemgr.h"
#include "hltv.h"
#include "trains.h"
#include "UserMessages.h"
#include "ts_player.h" // glast_* kill-flavor globals
#include "ts_utils.h"  // TSHudMessageAll
#include "ts_gun.h"    // TSGUN_WEAPON_ID / CTSGun (the kung-fu kill gate)
#include "ts_bullet.h" // glast_weaponid (the tracked slow-bullet id)
#include "ts_player.h" // glast_closecombat
#include "ts_gamerules.h" // g_theOneMode
#include "ts_slowmotionpoint.h" // a spectacular kill's slow-motion bubble

extern cvar_t slowmatch, banfortk; // ts_cvars.cpp
char GetWeaponType(tsweapon_t w);   // ts_gun.cpp

#define ITEM_RESPAWN_TIME 30
#define WEAPON_RESPAWN_TIME 20
#define AMMO_RESPAWN_TIME 20

// longest the intermission can last, in seconds
#define MAX_INTERMISSION_TIME 120

CVoiceGameMgr g_VoiceGameMgr;

class CMultiplayGameMgrHelper : public IVoiceGameMgrHelper
{
public:
	bool CanPlayerHearPlayer(CBasePlayer* pListener, CBasePlayer* pTalker) override
	{
		// @0xc0ed0 — teamplay: you can only hear teammates.
		if (g_teamplay)
		{
			if (g_pGameRules->PlayerRelationship(pListener, pTalker) != GR_TEAMMATE)
			{
				return false;
			}
		}

		// TS DELTA: last-man-standing (and the dead objective modes 2/3 — unreachable in the
		// shipped game, §4.6) isolate the dead — a dead talker (not alive + past-respawn) is
		// heard only by dead listeners; the living never hear the dead.
		if (TSGameRules
			&& ((unsigned char)(TSGameRules->m_iGameMode - 2) < 2 || g_lastManStanding != 0)
			&& !pTalker->IsAlive() && pTalker->m_flTSRespawnTime < 0.0f)
		{
			if (!pListener->IsAlive() && pListener->m_flTSRespawnTime < 0.0f)
				return true; // the dead hear the dead
			return false;    // the living cannot hear the dead
		}

		return true;
	}
};
static CMultiplayGameMgrHelper g_GameMgrHelper;

//*********************************************************
// Rules for the half-life multiplayer game.
//*********************************************************

CHalfLifeMultiplay::CHalfLifeMultiplay()
{
	// TS DELTA (@0xbee05): the MP ctor (re)initializes the standalone TS rules object for the map.
	// On a teamplay map the CHalfLifeTeamplay ctor body then Init()s it a second time (§3.4) —
	// harmless, and reproduced. Null on the deathmatch-0 / busters path (guarded).
	if (TSGameRules)
		TSGameRules->Init(); // @0x67d6c — per-map setup (InitTeams + objectives + arm round 1)

	g_VoiceGameMgr.Init(&g_GameMgrHelper, gpGlobals->maxClients);

	RefreshSkillData();

	// 11/8/98
	// Modified by YWB:  Server .cfg file is now a cvar, so that
	//  server ops can run multiple game servers, with different server .cfg files,
	//  from a single installed directory.
	// Mapcyclefile is already a cvar.

	// 3/31/99
	// Added lservercfg file cvar, since listen and dedicated servers should not
	// share a single config file. (sjb)
	if (IS_DEDICATED_SERVER())
	{
		// this code has been moved into engine, to only run server.cfg once
	}
	else
	{
		// listen server
		char* lservercfgfile = (char*)CVAR_GET_STRING("lservercfgfile");

		if (lservercfgfile && '\0' != lservercfgfile[0])
		{
			char szCommand[256];

			ALERT(at_console, "Executing listen server config file\n");
			sprintf(szCommand, "exec %s\n", lservercfgfile);
			SERVER_COMMAND(szCommand);
		}
	}
}

bool CHalfLifeMultiplay::ClientCommand(CBasePlayer* pPlayer, const char* pcmd)
{
	if (g_VoiceGameMgr.ClientCommand(pPlayer, pcmd))
		return true;

	// "menuselect" lives HERE in the original ("menuselect" @0xfd78f is referenced only by
	// CHalfLifeMultiplay::ClientCommand + CHalfLifeTeamplay::ClientCommand): the slot is
	// parsed and DISCARDED (stock-SDK leftover shape) — the TS buy menu is client-side.
	// CTSGameRules inherits this. RE dossier cont94 §2.
	if (FStrEq(pcmd, "menuselect"))
	{
		if (CMD_ARGC() < 2)
			return true;
		(void)atoi(CMD_ARGV(1));
		return true;
	}

	return CGameRules::ClientCommand(pPlayer, pcmd);
}

void CHalfLifeMultiplay::ClientUserInfoChanged(CBasePlayer* pPlayer, char* infobuffer)
{
	pPlayer->SetPrefsFromUserinfo(infobuffer);
}

//=========================================================
//=========================================================
void CHalfLifeMultiplay::RefreshSkillData()
{
	// load all default values
	CGameRules::RefreshSkillData();

	// override some values for multiplay.

	// suitcharger
	gSkillData.suitchargerCapacity = 30;

	// Crowbar whack
	gSkillData.plrDmgCrowbar = 25;

	// Glock Round
	gSkillData.plrDmg9MM = 12;

	// 357 Round — TS uses 40, not stock 50 (ts_i386.so @0xbefb8 = 0x42200000)
	gSkillData.plrDmg357 = 40;

	// MP5 Round
	gSkillData.plrDmgMP5 = 12;

	// M203 grenade
	gSkillData.plrDmgM203Grenade = 100;

	// Shotgun buckshot
	gSkillData.plrDmgBuckshot = 20; // fewer pellets in deathmatch

	// Crossbow
	gSkillData.plrDmgCrossbowClient = 20;

	// RPG
	gSkillData.plrDmgRPG = 120;

	// Egon
	gSkillData.plrDmgEgonWide = 20;
	gSkillData.plrDmgEgonNarrow = 10;

	// Hand Grendade
	gSkillData.plrDmgHandGrenade = 100;

	// Satchel Charge
	gSkillData.plrDmgSatchel = 120;

	// Tripmine
	gSkillData.plrDmgTripmine = 150;

	// hornet
	gSkillData.plrDmgHornet = 10;
}

//=========================================================
//=========================================================
void CHalfLifeMultiplay::Think()
{
	g_VoiceGameMgr.Update(gpGlobals->frametime);

	///// Check game rules /////
	static int last_frags;
	static int last_time;

	int frags_remaining = 0;
	int time_remaining = 0;

	if (g_fGameOver) // someone else quit the game already
	{
		// bounds check
		int time = (int)CVAR_GET_FLOAT("mp_chattime");
		if (time < 1)
			CVAR_SET_STRING("mp_chattime", "1");
		else if (time > MAX_INTERMISSION_TIME)
			CVAR_SET_STRING("mp_chattime", UTIL_dtos1(MAX_INTERMISSION_TIME));

		m_flIntermissionEndTime = m_flIntermissionStartTime + mp_chattime.value;

		// check to see if we should change levels now
		if (m_flIntermissionEndTime < gpGlobals->time)
		{
			if (m_iEndIntermissionButtonHit // check that someone has pressed a key, or the max intermission time is over
				|| ((m_flIntermissionStartTime + MAX_INTERMISSION_TIME) < gpGlobals->time))
				ChangeLevel(); // intermission is over
		}

		return;
	}

	float flTimeLimit = timelimit.value * 60;
	float flFragLimit = fraglimit.value;

	time_remaining = (int)(0 != flTimeLimit ? (flTimeLimit - gpGlobals->time) : 0);

	if (flTimeLimit != 0 && gpGlobals->time >= flTimeLimit)
	{
		GoToIntermission();
		return;
	}

	if (0 != flFragLimit)
	{
		int bestfrags = 9999;
		int remain;

		// check if any player is over the frag limit
		for (int i = 1; i <= gpGlobals->maxClients; i++)
		{
			CBaseEntity* pPlayer = UTIL_PlayerByIndex(i);

			if (pPlayer && pPlayer->pev->frags >= flFragLimit)
			{
				GoToIntermission();
				return;
			}


			if (pPlayer)
			{
				remain = flFragLimit - pPlayer->pev->frags;
				if (remain < bestfrags)
				{
					bestfrags = remain;
				}
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

	// TS DELTA (@0xbf302): drive the standalone rules object's non-virtual Think AFTER the stock
	// fragsleft/timeleft update, then end the map if a vote/objective set the forced next map
	// (@0xbf30a). (Reached only in the !g_fGameOver path — the intermission block returns above.)
	if (TSGameRules)
	{
		TSGameRules->Think();
		if (TSGameRules->m_szEndMap[0] != '\0')
			GoToIntermission();
	}
}


//=========================================================
//=========================================================
bool CHalfLifeMultiplay::IsMultiplayer()
{
	return true;
}

// @0x7b140 — TS blacklists the reserved Half-Life model names as team names (a player can't
// name their team after a stock HL model). Any other name is valid.
bool CHalfLifeMultiplay::IsValidTeam(const char* pTeamName)
{
	static const char* const reserved[] = {
		"barney", "gina", "gman", "helmet", "hgrunt", "recon", "robo", "scientist", "zombie"};
	for (const char* name : reserved)
		if (stricmp(pTeamName, name) == 0)
			return false;
	return true;
}

// @0xc0c68 — the server-browser game description per active mode. The original order is theOne ->
// LMS -> DM with NO teamplay branch (team mode is orthogonal; it still reports its DM/LMS/theOne
// sub-mode). CTSGameRules inherits this verbatim (the original defines no CTSGameRules override).
// @.data — the mode-dependent server-browser game-name strings (defined once here; the Teamplay
// override reads g_szTDMDescription instead of g_szDMDescription). Match the original .data globals.
char g_szDMDescription[]  = "The Specialists (DM)";
char g_szTDMDescription[] = "The Specialists (Teamplay)";
char g_szTOMDescription[] = "The Specialists (The One)";
char g_szLMSDescription[] = "The Specialists (LMS)";

// @0xc0c68 (decompile-exact): TheOne -> (The One); else LMS -> (LMS); else (DM). Keyed off the runtime
// mode GLOBALS (g_theOneMode/g_lastManStanding set by DecideGamePlay), NOT the cvars.
const char* CHalfLifeMultiplay::GetGameDescription()
{
	if (g_theOneMode != 0)
		return g_szTOMDescription;
	if (g_lastManStanding != 0)
		return g_szLMSDescription;
	return g_szDMDescription;
}

//=========================================================
//=========================================================
bool CHalfLifeMultiplay::IsDeathmatch()
{
	return true;
}

//=========================================================
//=========================================================
bool CHalfLifeMultiplay::IsCoOp()
{
	return 0 != gpGlobals->coop;
}

//=========================================================
//=========================================================
bool CHalfLifeMultiplay::FShouldSwitchWeapon(CBasePlayer* pPlayer, CBasePlayerItem* pWeapon)
{
	if (!pWeapon->CanDeploy())
	{
		// that weapon can't deploy anyway.
		return false;
	}

	if (!pPlayer->m_pActiveItem)
	{
		// player doesn't have an active item!
		return true;
	}

	if (!pPlayer->m_pActiveItem->CanHolster())
	{
		// can't put away the active item.
		return false;
	}

	//Never switch
	if (pPlayer->m_iAutoWepSwitch == 0)
	{
		return false;
	}

	//Only switch if not attacking
	if (pPlayer->m_iAutoWepSwitch == 2 && (pPlayer->m_afButtonLast & (IN_ATTACK | IN_ATTACK2)) != 0)
	{
		return false;
	}

	if (pWeapon->iWeight() > pPlayer->m_pActiveItem->iWeight())
	{
		return true;
	}

	return false;
}

//=========================================================
//=========================================================
bool CHalfLifeMultiplay::ClientConnected(edict_t* pEntity, const char* pszName, const char* pszAddress, char szRejectReason[128])
{
	g_VoiceGameMgr.ClientConnected(pEntity);
	return true;
}

void CHalfLifeMultiplay::UpdateGameMode(CBasePlayer* pPlayer)
{
	MESSAGE_BEGIN(MSG_ONE, gmsgGameMode, NULL, pPlayer->edict());
	WRITE_BYTE(0); // game mode none
	MESSAGE_END();
}

void CHalfLifeMultiplay::InitHUD(CBasePlayer* pl)
{
	// notify other clients of player joining the game
	UTIL_ClientPrintAll(HUD_PRINTNOTIFY, UTIL_VarArgs("%s has joined the game\n",
											 (!FStringNull(pl->pev->netname) && STRING(pl->pev->netname)[0] != 0) ? STRING(pl->pev->netname) : "unconnected"));

	// team match?
	if (g_teamplay)
	{
		UTIL_LogPrintf("\"%s<%i><%s><%s>\" entered the game\n",
			STRING(pl->pev->netname),
			GETPLAYERUSERID(pl->edict()),
			GETPLAYERAUTHID(pl->edict()),
			g_engfuncs.pfnInfoKeyValue(g_engfuncs.pfnGetInfoKeyBuffer(pl->edict()), "model"));
	}
	else
	{
		UTIL_LogPrintf("\"%s<%i><%s><%i>\" entered the game\n",
			STRING(pl->pev->netname),
			GETPLAYERUSERID(pl->edict()),
			GETPLAYERAUTHID(pl->edict()),
			GETPLAYERUSERID(pl->edict()));
	}

	UpdateGameMode(pl);

	// sending just one score makes the hud scoreboard active;  otherwise
	// it is just disabled for single play
	MESSAGE_BEGIN(MSG_ONE, gmsgScoreInfo, NULL, pl->edict());
	WRITE_BYTE(ENTINDEX(pl->edict()));
	WRITE_SHORT(0);
	WRITE_SHORT(0);
	WRITE_SHORT(0);
	WRITE_SHORT(0);
	MESSAGE_END();

	SendMOTDToClient(pl->edict());

	// loop through all active players and send their score info to the new client
	for (int i = 1; i <= gpGlobals->maxClients; i++)
	{
		// FIXME:  Probably don't need to cast this just to read m_iDeaths
		CBasePlayer* plr = (CBasePlayer*)UTIL_PlayerByIndex(i);

		if (plr)
		{
			MESSAGE_BEGIN(MSG_ONE, gmsgScoreInfo, NULL, pl->edict());
			WRITE_BYTE(i); // client number
			WRITE_SHORT(plr->pev->frags);
			WRITE_SHORT(plr->m_iDeaths);
			WRITE_SHORT(0);
			WRITE_SHORT(GetTeamIndex(plr->m_szTeamName) + 1);
			MESSAGE_END();
		}
	}

	if (g_fGameOver)
	{
		MESSAGE_BEGIN(MSG_ONE, SVC_INTERMISSION, NULL, pl->edict());
		MESSAGE_END();
	}
}

//=========================================================
//=========================================================
void CHalfLifeMultiplay::ClientDisconnected(edict_t* pClient)
{
	if (pClient)
	{
		CBasePlayer* pPlayer = (CBasePlayer*)CBaseEntity::Instance(pClient);

		if (pPlayer)
		{
			FireTargets("game_playerleave", pPlayer, pPlayer, USE_TOGGLE, 0);

			// team match?
			if (g_teamplay)
			{
				UTIL_LogPrintf("\"%s<%i><%s><%s>\" disconnected\n",
					STRING(pPlayer->pev->netname),
					GETPLAYERUSERID(pPlayer->edict()),
					GETPLAYERAUTHID(pPlayer->edict()),
					g_engfuncs.pfnInfoKeyValue(g_engfuncs.pfnGetInfoKeyBuffer(pPlayer->edict()), "model"));
			}
			else
			{
				UTIL_LogPrintf("\"%s<%i><%s><%i>\" disconnected\n",
					STRING(pPlayer->pev->netname),
					GETPLAYERUSERID(pPlayer->edict()),
					GETPLAYERAUTHID(pPlayer->edict()),
					GETPLAYERUSERID(pPlayer->edict()));
			}

			pPlayer->RemoveAllItems(true); // destroy all of the players weapons and items

			// TS deltas (@0xbf960): drop the leaver's flag/votes/edict + re-check the round win
			// condition, then force a fresh The-One selection (the leaver may have been The One).
			TSPlayerDisconnect(pPlayer);        // @0x8f94c
			if (TSGameRules)
				TSGameRules->m_flNextTheOneSelect = 0; // @0xbfacf (+0x587c)
		}
	}
}

//=========================================================
//=========================================================
float CHalfLifeMultiplay::FlPlayerFallDamage(CBasePlayer* pPlayer)
{
	int iFallDamage = (int)falldamage.value;

	switch (iFallDamage)
	{
	case 1: //progressive
		pPlayer->m_flFallVelocity -= PLAYER_MAX_SAFE_FALL_SPEED;
		return pPlayer->m_flFallVelocity * DAMAGE_FOR_FALL_SPEED;
		break;
	default:
	case 0: // fixed
		return 10;
		break;
	}
}

//=========================================================
//=========================================================
bool CHalfLifeMultiplay::FPlayerCanTakeDamage(CBasePlayer* pPlayer, CBaseEntity* pAttacker)
{
	return true;
}

//=========================================================
//=========================================================
void CHalfLifeMultiplay::PlayerThink(CBasePlayer* pPlayer)
{
	if (g_fGameOver)
	{
		// check for button presses
		if ((pPlayer->m_afButtonPressed & (IN_DUCK | IN_ATTACK | IN_ATTACK2 | IN_USE | IN_JUMP)) != 0)
			m_iEndIntermissionButtonHit = true;

		// clear attack/use commands from player
		pPlayer->m_afButtonPressed = 0;
		pPlayer->pev->button = 0;
		pPlayer->m_afButtonReleased = 0;
	}
}

//=========================================================
//=========================================================
// R1/Y9: the TS-patched CHalfLifeMultiplay::PlayerSpawn @0x790ac (body @0x890b8,
// TS_multiplay_gamerules.cpp) — the original CTSGameRules has NO PlayerSpawn; this replaced the
// stock crowbar+9mmhandgun loadout. Shape: weapons flag → game_player_equip loop → if none:
// GiveNamedItem("weapon_tsgun") + BuyArmament (its ONLY caller, @0x79122) → spawn-cash snapshot.
// Tagged local shims kept from the dissolved override: SetHasSuit (stock-derived HUD needs the
// suit bit), the spectate-wait no-equip gate (coupled to Y7), RemoveAllItems (round-restart
// double-tsgun guard), TSDeployArsenal (engine auto-deploy skips kung-fu).
void CHalfLifeMultiplay::PlayerSpawn(CBasePlayer* pPlayer)
{
	pPlayer->pev->weapons |= 0x80000000; // @0x890c4 — the suit/weapons marker bit
	pPlayer->SetHasSuit(true);           // local shim (see header comment)

	// INTERIM (fold-plan Y9-3): team-cash seed — inert (GetTeamCash returns 0; the cash
	// economy is compiled out of the shipped game).
	if (TSGameRules)
	{
		if (int teamCash = TSGameRules->GetTeamCash(pPlayer))
			pPlayer->m_iCash = teamCash;
	}

	bool addDefault = true;
	CBaseEntity* pWeaponEntity = NULL;
	while ((pWeaponEntity = UTIL_FindEntityByClassname(pWeaponEntity, "game_player_equip")) != NULL)
	{
		pWeaponEntity->Touch(pPlayer);
		addDefault = false;
	}

	// TS (cont.65): a player entering free-look spectate-wait gets NO weapons — they equip
	// on fire-to-play (the next Spawn runs with m_fInSpectateWait clear). A joiner spawns without
	// the flag and is equipped, as in the original; InitHUD's ResetArmament takes it back.
	if (addDefault && !pPlayer->m_fInSpectateWait)
	{
		// Round restart respawns STILL-ALIVE players and Spawn does NOT strip items — clear
		// first so the re-equip is clean (one tsgun; the old double fought over WStatus).
		// Only when he still holds the arsenal: RemoveAllItems ends in UpdateClientData, which for
		// a joiner would run his first-frame InitHUD (and send him to spectate) inside
		// ClientPutInServer. The original's PlayerSpawn (@0x790ac) strips nothing.
		if (GetPlayerTSGun(pPlayer))
			pPlayer->RemoveAllItems(false);
		pPlayer->GiveNamedItem("weapon_tsgun");
		pPlayer->BuyArmament();
		// force the arsenal active + deployed (auto-deploy skips kung-fu; a kung-fu-only
		// loadout otherwise has no viewmodel and can't fire).
		TSDeployArsenal(pPlayer);
	}

	// (initial_cash moved to ClientPutInServer — its original home @0xacf44; the old
	// "re-seed any 0-cash respawn" behavior was ours-only. Fold-plan Y9-2.)

	// (the Y9-4 auto-balance interim is GONE — R1 phase 3 installs CHalfLifeTeamplay on team
	// maps; its stock InitHUD runs SetDefaultPlayerTeam + ChangePlayerTeam on join.)
}

//=========================================================
//=========================================================
bool CHalfLifeMultiplay::FPlayerCanRespawn(CBasePlayer* pPlayer)
{
	return true;
}

//=========================================================
//=========================================================
float CHalfLifeMultiplay::FlPlayerSpawnTime(CBasePlayer* pPlayer)
{
	return gpGlobals->time; //now!
}

bool CHalfLifeMultiplay::AllowAutoTargetCrosshair()
{
	return (aimcrosshair.value != 0);
}

//=========================================================
// @0x79140 TSGetPointsForFrag (TS_multiplay_gamerules.cpp, decompile-exact) — the stylish-kill
// scorer, called by PlayerKilled with the base points (IPointsForKill): negative passes through;
// THE-ONE mode: only the One scores (everyone else gets 0). For a client killer:
//  - KUNG-FU kill (2 pts, "#TS_KungFu"): inflictor==killer, glast_weaponid==-1 (not a tracked
//    slow-bullet), and bare hands — no active item, or the tsgun with NO wielded item — or the
//    kill came through the close-combat path (glast_closecombat);
//  - THROWN-BLADE kill (2 pts): inflictor is a "knife" classname — desc = its GetDescription()
//    ("Throwing ..." names); any OTHER foreign inflictor returns the base points unchanged;
//  - STUNT kill (+1, "#TS_StStl", *pStunt): killer stunting (iuser4 & 0x10c10) moving > 80 u/s
//    (the +1 skipped for close-combat kills);
//  - else SLIDE kill (+1, "#TS_SlStl", *pSlide): iuser4 & 0x20, friction < 1, speed > 80;
//  - else a styled kill gets the plain " #TS_Stl" suffix.
// pts > 1 broadcasts the green feed line: "%i #TS_Frg #TS_FrgFr %s, %s" (name, style desc).
int TSGetPointsForFrag(CBasePlayer *pUnused, entvars_t *pevKiller, entvars_t *pevInflictor,
	int pts, bool *pSlide, bool *pStunt)
{
	(void)pUnused;
	if (pts < 0)
		return pts;
	CBaseEntity *pKillerEnt = CBaseEntity::Instance(pevKiller);
	CBasePlayer *pKiller = (pKillerEnt && pKillerEnt->IsPlayer()) ? (CBasePlayer *)pKillerEnt : NULL;

	char szDesc[64] = "";
	char szName[64] = "";

	if (g_theOneMode && pKiller && !pKiller->m_bTheOne)
		return 0; // only the One scores in The-One mode

	if ((pevKiller->flags & FL_CLIENT) != 0)
	{
		strncpy(szName, STRING(pevKiller->netname), sizeof(szName) - 1);
		szName[sizeof(szName) - 1] = 0;
		if (pevInflictor)
		{
			if (pevInflictor == pevKiller)
			{
				// direct kill: the KUNG-FU check (bare hands, not a tracked slow-bullet)
				if (glast_weaponid == -1 && pKiller)
				{
					CBasePlayerItem *pItem = pKiller->m_pActiveItem; // player+0xa10
					bool kungfu = false;
					if (!pItem)
						kungfu = glast_closecombat;
					else if (pItem->m_iId == TSGUN_WEAPON_ID)
					{
						CTSGun *pGun = (CTSGun *)pItem;
						kungfu = (pGun->m_pActiveItem == NULL) || glast_closecombat;
					}
					if (kungfu)
					{
						strcpy(szDesc, "#TS_KungFu");
						pts = 2;
					}
				}
			}
			else
			{
				// a foreign inflictor: ONLY the thrown blade ("knife" classname) is styled
				if (strcmp(STRING(pevInflictor->classname), "knife") != 0)
					return pts & 0xff;
				CBaseEntity *pInf = CBaseEntity::Instance(pevInflictor);
				strncpy(szDesc, TSKnifeGetDescription(pInf), sizeof(szDesc) - 1);
				szDesc[sizeof(szDesc) - 1] = 0;
				pts = 2;
			}
		}
		const float speed = pevKiller->velocity.Length();
		if ((pevKiller->iuser4 & 0x10c10) != 0 && speed > 80.0f)
		{
			// STUNT kill
			if (!glast_closecombat && pKiller)
				pts = pts + 1;
			if (pts > 1)
			{
				if (szDesc[0])
					strcat(szDesc, " & #TS_StStl");
				else
					strcpy(szDesc, "#TS_StStl");
			}
			*pStunt = true;
		}
		else if ((pevKiller->iuser4 & 0x20) != 0 && pevKiller->friction < 1.0f && speed > 80.0f)
		{
			// SLIDE kill
			if (!glast_closecombat && pKiller)
				pts = pts + 1;
			if (pts > 1)
			{
				if (szDesc[0])
					strcat(szDesc, " & #TS_SlStl");
				else
					strcpy(szDesc, "#TS_SlStl");
			}
			*pSlide = true;
		}
		else if (szDesc[0])
		{
			strcat(szDesc, " #TS_Stl");
		}
	}

	if (pts > 1)
		TSHudMessageAll(0, 0xff, 0, 1, 30, UTIL_VarArgs("%i #TS_Frg #TS_FrgFr %s, %s", pts, szName, szDesc));
	return pts & 0xff;
}

// IPointsForKill - how many points awarded to anyone
// that kills this player?
//=========================================================
int CHalfLifeMultiplay::IPointsForKill(CBasePlayer* pAttacker, CBasePlayer* pKilled)
{
	return 1;
}


// The tags of the kill PlayerKilled is scoring, for the "killed" log line TSLogKill writes. The
// original builds the whole line in PlayerKilled (@0x79e3d-0x7a182); here PlayerKilled fills this
// before its log line and clears it after.
static char s_szKillLogTags[256] = "";
// The weapon the death notice names, which PlayerKilled works out before calling DeathNotice.
static const char* s_pszKillWeapon = nullptr;
static const char* TSKillWeaponName(CBasePlayer* pVictim, entvars_t* pKiller, entvars_t* pevInflictor);
static void TSLogKill(CBasePlayer* pVictim, entvars_t* pKiller, const char* killer_weapon_name);

//=========================================================
// PlayerKilled - someone/something killed this player
//=========================================================
void CHalfLifeMultiplay::PlayerKilled(CBasePlayer* pVictim, entvars_t* pKiller, entvars_t* pInflictor)
{
	CBasePlayer* peKiller = NULL;
	CBaseEntity* ktmp = CBaseEntity::Instance(pKiller);
	if (ktmp && (ktmp->Classify() == CLASS_PLAYER))
		peKiller = (CBasePlayer*)ktmp;
	else if (ktmp && (ktmp->Classify() == CLASS_VEHICLE))
	{
		CBasePlayer* pDriver = ((CFuncVehicle*)ktmp)->m_pDriver;
		if (pDriver != NULL)
		{
			peKiller = pDriver;
			ktmp = pDriver;
			pKiller = pDriver->pev;
		}
	}

	// @0x7968e: the base points, asked of the rules for every death.
	int pts = IPointsForKill(peKiller, pVictim);

	// @0x796b0-0x79845: a kill of another player by a player is scored before the DeathMsg. The
	// stylish-kill scorer wraps the base points (kung-fu/thrown-blade = 2, stunt/slide +1, the green
	// feed broadcast), then the streaks: a kill within 2 s of the killer's last scoring kill is a
	// double kill (time - player+0x748 < 2.0, the double at 0xf7fc0); a victim with a streak of 3 or
	// more is named in the log, and with 9 or more he is the specialist; the killer's streak goes up
	// by one for every such kill, points or not (a team kill and The One mode's 0 count too), and
	// stops at 255; his new streak picks his tier: 10 and on unstoppable, 9 specialist, 6
	// demolition, 3 killer.
	bool bSlideKill = false, bStuntKill = false;
	bool bDoubleKill = false, bKilledSpecialist = false;
	bool bUnstoppable = false, bSpecialist = false, bDemolition = false, bKillerTier = false;
	bool bOwnWeapon = false;
	const char* pszStoppedFury = nullptr;
	if (pVictim->pev != pKiller && ktmp && ktmp->IsPlayer())
	{
		pts = TSGetPointsForFrag(NULL, pKiller, pInflictor, pts, &bSlideKill, &bStuntKill);
		// (glast_weaponid reset moved to CBulletManager::UpdateBullet @0x5f311 — the original
		// resets right after ApplyMultiDamage, not here; here it went stale on non-kill hits.)
		// @0x79716-0x7977e: killed with his own weapon. The killer's active item is the arsenal with
		// a weapon out whose recorded owner (item+0x30, the buyer) is the victim, the weapon is no
		// grenade or blade (GetWeaponType not 2, 3 or 11), the kill was no close-combat blow and the
		// killer did it himself.
		CBasePlayer* pKillerPlayer = (CBasePlayer*)ktmp;
		if (pKillerPlayer->m_pActiveItem && pKillerPlayer->m_pActiveItem->m_iId == TSGUN_WEAPON_ID)
		{
			CTSGun* pGun = (CTSGun*)pKillerPlayer->m_pActiveItem;
			if (pGun->m_pActiveItem && (CBaseEntity*)pGun->m_pActiveItem->hOwner == pVictim)
			{
				const int wtype = GetWeaponType((tsweapon_t)pGun->m_weaponType);
				if ((unsigned)(wtype - 2) > 1 && wtype != 11 && !glast_closecombat && pInflictor == pKiller)
					bOwnWeapon = true;
			}
		}
		if (peKiller)
		{
			bDoubleKill = (double)gpGlobals->time - (double)peKiller->m_flLastKillTime < 2.0;
			if (pVictim->m_iKillStreak > 2)
			{
				pszStoppedFury = STRING(pVictim->pev->netname);
				if (pVictim->m_iKillStreak > 8)
					bKilledSpecialist = true;
			}
			if (peKiller->m_iKillStreak < 255)
				peKiller->m_iKillStreak++;
			if (peKiller->m_iKillStreak > 9)
				bUnstoppable = true;
			else if (peKiller->m_iKillStreak == 9)
				bSpecialist = true;
			else if (peKiller->m_iKillStreak == 6)
				bDemolition = true;
			else if (peKiller->m_iKillStreak == 3)
				bKillerTier = true;
		}
	}

	// @0x79db0-0x79fa4: the tags the original's "killed" log line carries after the weapon, in its
	// order. " (victims_weapon)" is the kill with the victim's own weapon (bOwnWeapon above).
	if ((pKiller->flags & FL_CLIENT) != 0 && pVictim->pev != pKiller)
	{
		char szFury[96] = "";
		if (pszStoppedFury)
			snprintf(szFury, sizeof(szFury), " (stopped_fury \"%s\")", pszStoppedFury);
		const char* pszTier = bUnstoppable ? " (is_unstoppable)" : bSpecialist ? " (is_specialist)" : bDemolition ? " (is_demolition)" : bKillerTier ? " (is_killer)" : "";
		snprintf(s_szKillLogTags, sizeof(s_szKillLogTags), "%s%s%s%s%s%s%s%s", bOwnWeapon ? " (victims_weapon)" : "", szFury,
			bKilledSpecialist ? " (killed_specialist)" : "", bDoubleKill ? " (double_kill)" : "",
			bStuntKill ? " (diving)" : "", bSlideKill ? " (sliding)" : "",
			pVictim->m_bTheOne ? " (killed_the_one)" : "", pszTier);
	}

	// @0x7988f-0x7a182: the weapon's name, then the rules' DeathNotice, then the log line with it
	s_pszKillWeapon = TSKillWeaponName(pVictim, pKiller, pInflictor);
	DeathNotice(pVictim, pKiller, pInflictor);
	TSLogKill(pVictim, pKiller, s_pszKillWeapon);
	s_pszKillWeapon = nullptr;
	s_szKillLogTags[0] = '\0';

	pVictim->m_iDeaths += 1;

	FireTargets("game_playerdie", pVictim, pVictim, USE_TOGGLE, 0);

	// @0x7a2e3: The One's death, whoever or whatever killed him (the killer when he is a player),
	// before any frags change; his streak ends with it.
	if (pVictim->m_bTheOne)
	{
		if (TSGameRules)
			TSGameRules->TheOneIsDead(pVictim, peKiller);
		pVictim->m_iKillStreak = 0;
	}

	if (pVictim->pev == pKiller)
	{ // killed self
		// @0x7a31b: the specialist (a streak of 9 or more) pays 5 more for it
		if (pVictim->m_iKillStreak > 8)
		{
			pKiller->frags -= 5;
			TSHudMessageAll(0, 0xff, 0, 1, 30, UTIL_VarArgs("%s, #TS_SpecialistSuicide", STRING(pVictim->pev->netname)));
		}
		pKiller->frags -= 1;
	}
	else if (ktmp && ktmp->IsPlayer())
	{
		// @0x7a3bb: a spectacular kill (the killer stunting, a close-combat blow, or 3 points or more)
		// may leave a slow-motion bubble where the victim fell, as the spectacularness roll decides.
		if ((pKiller->iuser4 & 0x10c10) != 0 || glast_closecombat || pts > 2)
		{
			if (DoSpectacularNess())
				CTSSlowMotionPoint::CreateSlowMotionPoint(pVictim->pev->origin, 0.25f, 1.0f, 0, nullptr);
		}

		// @0x7a429: the streak bonuses, only for a kill that scores. The order is the original's:
		// ((points x2 for a double kill) + 5 for the specialist) x2 for an unstoppable killer.
		if (pts > 0 && peKiller)
		{
			const char* pszKiller = STRING(pKiller->netname);
			// @0x7a434-0x7a5cd: the headshot, knife, katana and coldcock notices, each naming the
			// killer. The blade notice needs the killer's own blow with a blade out (fire type 3 knife,
			// 10 katana), not a close-combat one.
			if (glast_headshot)
				TSHudMessageAll(0, 0xff, 0, 1, 20, UTIL_VarArgs("#TS_Headshot #TS_FrgFr %s!", pszKiller));
			if (pInflictor == pKiller && peKiller->m_pActiveItem && peKiller->m_pActiveItem->m_iId == TSGUN_WEAPON_ID &&
				((CTSGun*)peKiller->m_pActiveItem)->m_pActiveItem && !glast_closecombat)
			{
				const s_weapon_info* wi = get_weapon_info((tsweapon_t)((CTSGun*)peKiller->m_pActiveItem)->m_weaponType);
				if (wi->iFireType == TSFT_KNIFE)
					TSHudMessageAll(0, 0xff, 0, 1, 20, UTIL_VarArgs("#TS_KnifeKill #TS_FrgFr %s!", pszKiller));
				else if (wi->iFireType == TSFT_KATANA)
					TSHudMessageAll(0, 0xff, 0, 1, 20, UTIL_VarArgs("#TS_KatanaKill #TS_FrgFr %s!", pszKiller));
			}
			if (glast_coldcock)
				TSHudMessageAll(0, 0xff, 0, 1, 20, UTIL_VarArgs("#TS_Coldcock #TS_FrgFr %s!", pszKiller));
			if (bDoubleKill)
			{
				pts <<= 1;
				TSHudMessageAll(0, 0xff, 0, 1, 20, UTIL_VarArgs("#TS_Doublefrag #TS_FrgFr %s!", pszKiller));
			}
			if (pszStoppedFury)
			{
				if (bKilledSpecialist)
				{
					pts += 5;
					TSHudMessageAll(0, 0xff, 0, 1, 30, UTIL_VarArgs("%i #TS_Frg #TS_FrgFr %s, #TS_KilledTheSpecialist", pts, pszKiller));
					if (peKiller->m_iPowerups == 0 && peKiller->m_iHeldPowerup == 0)
					{
						peKiller->m_iPowerups = 0;
						peKiller->m_iHeldPowerup = 8; // the slow motion powerup (@0x7a69f)
						peKiller->m_flPowerupExpireTime = 0;
						peKiller->m_flPowerupTime = 2;
					}
				}
				char szSpree[24]; // the original's buffer ends at its 24th byte (@0x7a6e5)
				snprintf(szSpree, sizeof(szSpree), "#TS_SpreeKilled%i", RANDOM_LONG(1, 10));
				TSHudMessageAll(0, 0xff, 0, 1, 30, UTIL_VarArgs("%s %s", STRING(pVictim->pev->netname), szSpree));
			}

			int spree = 0, hold = 0;
			if (bUnstoppable)
			{
				TSHudMessageAll(0, 0xff, 0, 1, 30,
					UTIL_VarArgs("#TS_Unstoppable %s! %i #TS_ConsKills", pszKiller, (int)peKiller->m_iKillStreak));
				pts <<= 1;
			}
			else if (bSpecialist)
			{
				// @0x7a7ab: the 9th kill makes him the specialist: 150 health, kung fu, superjump and a
				// slow motion powerup if he holds none; no points for it
				TSHudMessageAll(0, 0xff, 0, 1, 55, UTIL_VarArgs("%s #TS_Specialist", pszKiller));
				if (peKiller->pev->health < 150.0f)
					peKiller->pev->health = 150.0f;
				peKiller->m_iKungFu = 1;
				peKiller->m_bLowGravity = 1;
				g_engfuncs.pfnSetPhysicsKeyValue(peKiller->edict(), "mj", "1");
				extern int gmsgTSPowerUp; // ts_player.cpp
				MESSAGE_BEGIN(MSG_ONE, gmsgTSPowerUp, NULL, peKiller->pev);
				WRITE_SHORT(TS_PWUP_KUNGFU);
				WRITE_BYTE(0);
				MESSAGE_END();
				MESSAGE_BEGIN(MSG_ONE, gmsgTSPowerUp, NULL, peKiller->pev);
				WRITE_SHORT(TS_PWUP_LOWGRAV);
				WRITE_BYTE(0);
				MESSAGE_END();
				if (peKiller->m_iPowerups == 0 && peKiller->m_iHeldPowerup == 0)
				{
					peKiller->m_iPowerups = 0;
					peKiller->m_iHeldPowerup = 8;
					peKiller->m_flPowerupExpireTime = 0;
					peKiller->m_flPowerupTime = 2;
				}
				peKiller->m_iAward = (peKiller->m_iAward == 10) ? 10 : 4; // The One keeps his 10
			}
			else if (bDemolition)
			{
				peKiller->pev->health = V_min(peKiller->pev->health + 60.0f, 150.0f);
				peKiller->m_iAward = (peKiller->m_iAward == 10) ? 10 : 3;
				spree = 6;
				hold = 45;
			}
			else if (bKillerTier)
			{
				peKiller->pev->health = V_min(peKiller->pev->health + 30.0f, 150.0f);
				peKiller->m_iAward = (peKiller->m_iAward == 10) ? 10 : 2;
				spree = 3;
				hold = 35;
			}
			if (spree != 0)
			{
				char szSpree[24];
				snprintf(szSpree, sizeof(szSpree), "#TS_KillSpree%i", RANDOM_LONG(1, 16));
				TSHudMessageAll(0, 0xff, 0, 1, hold, UTIL_VarArgs("%s %s %i #TS_ConsKills", pszKiller, szSpree, spree));
			}
			if (peKiller->pev->health > 100.0f)
				peKiller->m_flHealthRegen = gpGlobals->time + 1.0f;
			// @0x7aa8c: a double kill forgets the last kill, so a third right after it is not one
			peKiller->m_flLastKillTime = bDoubleKill ? 0.0f : gpGlobals->time;
			// @0x7aab8: the kill cash, 500 a point and 400 more for the killer, 900 for the victim
			peKiller->GiveCash(pts == -1 ? -1000 : pts * 500 + 400);
			pVictim->GiveCash(900);
		}

		// @0x7aafe: in the objective modes (the rules' mode byte above 1) every kill pays 1000 more,
		// a team kill costs the killer 1000, and the victim gets 1000.
		if (TSGameRules && (unsigned char)TSGameRules->m_iGameMode > 1 && peKiller)
		{
			peKiller->GiveCash(pts != -1 ? 1000 : -1000);
			pVictim->GiveCash(1000);
		}

		// @0x7ab45: the kill with the victim's own weapon
		if (bOwnWeapon)
			TSHudMessageAll(0, 0xff, 0, 1, 30, UTIL_VarArgs("%s #TS_Killed %s #TS_OwnWeapon", STRING(pKiller->netname), STRING(pVictim->pev->netname)));

		pKiller->frags += pts;

		FireTargets("game_playerkill", ktmp, ktmp, USE_TOGGLE, 0);

		// @0x7ac15: with slowmatch at 1, a scoring kill hands the killer a slow motion powerup to
		// hold, if he has none active and holds none or a slow motion one, adding a second to its
		// hold, up to 3.
		if (slowmatch.value == 1.0f && peKiller && pts > 0 &&
			(peKiller->m_iHeldPowerup == 0 || (peKiller->m_iHeldPowerup & 1) != 0) && peKiller->m_iPowerups == 0)
		{
			peKiller->m_iPowerups = 0;
			peKiller->m_iHeldPowerup = 1;
			int hold = (int)peKiller->m_flPowerupTime + 1;
			if (hold > 3)
				hold = 3;
			peKiller->m_flPowerupTime = hold;
		}

		// @0x7ac7e: a team kill. Three within 30 s of each other (player+0x7c4, the count +0x7c8)
		// kick the killer, or ban him for banfortk minutes when that is above 0.
		if (pts == -1 && peKiller)
		{
			if (peKiller->m_flTeamKillTime == 0.0f || peKiller->m_flTeamKillTime < gpGlobals->time)
			{
				peKiller->m_flTeamKillTime = gpGlobals->time + 30.0f;
				peKiller->m_iTeamKills = 1;
			}
			else if (gpGlobals->time < peKiller->m_flTeamKillTime)
			{
				peKiller->m_flTeamKillTime = gpGlobals->time + 30.0f;
				peKiller->m_iTeamKills++;
			}
			if (peKiller->m_iTeamKills > 2)
			{
				const char* pszMsg;
				if (banfortk.value <= 0.0f)
				{
					ClientPrint(pKiller, HUD_PRINTCONSOLE, "You have been kicked for teamkilling...\n");
					SERVER_COMMAND(UTIL_VarArgs("kick #%i\n", GETPLAYERUSERID(peKiller->edict())));
					pszMsg = UTIL_VarArgs("%s kicked for teamkilling\n", STRING(pKiller->netname));
				}
				else
				{
					ClientPrint(pKiller, HUD_PRINTCONSOLE, UTIL_VarArgs("You have been banned for teamkilling (%i minutes)...\n", (int)banfortk.value));
					SERVER_COMMAND(UTIL_VarArgs("banid %i #%i kick\n", (int)banfortk.value, GETPLAYERUSERID(peKiller->edict())));
					pszMsg = UTIL_VarArgs("%s banned for teamkilling (%i minutes)\n", STRING(pKiller->netname), (int)banfortk.value);
				}
				UTIL_ClientPrintAll(HUD_PRINTTALK, pszMsg);
			}
		}
	}
	else
	{ // killed by the world
		pKiller->frags -= 1;
	}

	// update the scores
	// killed scores
	MESSAGE_BEGIN(MSG_ALL, gmsgScoreInfo);
	WRITE_BYTE(ENTINDEX(pVictim->edict()));
	WRITE_SHORT(pVictim->pev->frags);
	WRITE_SHORT(pVictim->m_iDeaths);
	WRITE_SHORT(0);
	WRITE_SHORT(GetTeamIndex(pVictim->m_szTeamName) + 1);
	MESSAGE_END();

	// killers score, if it's a player
	CBaseEntity* ep = CBaseEntity::Instance(pKiller);
	if (ep && ep->Classify() == CLASS_PLAYER)
	{
		CBasePlayer* PK = (CBasePlayer*)ep;

		MESSAGE_BEGIN(MSG_ALL, gmsgScoreInfo);
		WRITE_BYTE(ENTINDEX(PK->edict()));
		WRITE_SHORT(PK->pev->frags);
		WRITE_SHORT(PK->m_iDeaths);
		WRITE_SHORT(0);
		WRITE_SHORT(GetTeamIndex(PK->m_szTeamName) + 1);
		MESSAGE_END();

		// let the killer paint another decal as soon as he'd like.
		PK->m_flNextDecalTime = gpGlobals->time;
	}

	// NB: stock `if (HasNamedPlayerItem("weapon_satchel")) DeactivateSatchels(...)` removed —
	// DeactivateSatchels/CSatchel are absent from the original (TS has no satchel weapon; uses CTSGun).

	// R1/Y8-1: the deads-check schedule is the LAST statement of the original PlayerKilled
	// (@0x795e4 line ~730) — a death may have eliminated a team; resolve the round.
	if (TSGameRules)
		TSGameRules->CallCheckDeads();
}

//=========================================================
// The weapon a death notice names.
//=========================================================
// The chain is decompile-exact to the ORIGINAL PlayerKilled @0x795e4, which works the name out
// before calling its 4-arg DeathNotice @0x78f7c (it hands the name over) and logs the kill with it
// itself. reTS keeps the 3-arg DeathNotice (the T2Pc signature across the rules classes is parked
// for the R1 gamerules session): PlayerKilled asks this for the name and DeathNotice reads it.
static const char* TSKillWeaponName(CBasePlayer* pVictim, entvars_t* pKiller, entvars_t* pevInflictor)
{
	static char killer_weapon_buf[64];
	strcpy(killer_weapon_buf, "world"); // by default, the player is killed by the world
	const char* killer_weapon_name = killer_weapon_buf;

	if ((pKiller->flags & FL_CLIENT) != 0)
	{
		if (pevInflictor)
		{
			if (pevInflictor == pKiller)
			{
				// direct kill — the original's priority chain (@0x798da..0x7999b):
				CBasePlayer* pPlayer = (CBasePlayer*)CBaseEntity::Instance(pKiller);
				CTSGun* pGun = GetPlayerTSGun(pPlayer);
				if (glast_weaponid != -1)
				{
					// a tracked slow-mo bullet landed the blow: name what FIRED it (the killer may
					// have switched weapons while the bullet flew)
					strncpy(killer_weapon_buf, get_weapon_info((tsweapon_t)glast_weaponid)->pszName,
						sizeof(killer_weapon_buf) - 1);
				}
				else if (glast_closecombat)
				{
					strcpy(killer_weapon_buf, "Kung Fu"); // literal @0x1087f5
				}
				else if (pGun && (CBaseEntity*)pGun == (CBaseEntity*)pPlayer->m_pActiveItem &&
						 pGun->m_pActiveItem)
				{
					const s_weapon_info* wi = get_weapon_info((tsweapon_t)pGun->m_weaponType);
					if (glast_coldcock)
					{
						// pistol-whip: "<gun> Stock" (@0x79954-0x79981); PlayerKilled sends its notice
						snprintf(killer_weapon_buf, sizeof(killer_weapon_buf), "%s Stock", wi->pszName);
					}
					else
					{
						strncpy(killer_weapon_buf, wi->pszName, sizeof(killer_weapon_buf) - 1);
					}
				}
				// else: bare fists with no glast marker — stays "world" (original leaves the buffer)
			}
			else
			{
				// a foreign inflictor: a thrown blade ("knife" classname) names itself via its
				// GetDescription (@0x7994x: "Throwing Combat Knife"/"Throwing Seal Knife"/
				// "Throwing Katana"); everything else is the classname.
				if (strncmp(STRING(pevInflictor->classname), "knife", 6) == 0)
				{
					CBaseEntity* pInf = CBaseEntity::Instance(pevInflictor);
					strncpy(killer_weapon_buf, pInf ? TSKnifeGetDescription(pInf) : "",
						sizeof(killer_weapon_buf) - 1);
				}
				else
					strncpy(killer_weapon_buf, STRING(pevInflictor->classname), sizeof(killer_weapon_buf) - 1);
			}
		}
	}
	else
	{
		// non-client killer: targetname first, else classname (@0x79xxx pre-strcpy)
		if (!FStringNull(pevInflictor->targetname))
			strncpy(killer_weapon_buf, STRING(pevInflictor->targetname), sizeof(killer_weapon_buf) - 1);
		else
			strncpy(killer_weapon_buf, STRING(pevInflictor->classname), sizeof(killer_weapon_buf) - 1);
	}
	killer_weapon_buf[sizeof(killer_weapon_buf) - 1] = 0;

	// strip the monster_* or weapon_* from the inflictor's classname
	if (strncmp(killer_weapon_name, "weapon_", 7) == 0)
		killer_weapon_name += 7;
	else if (strncmp(killer_weapon_name, "monster_", 8) == 0)
		killer_weapon_name += 8;
	else if (strncmp(killer_weapon_name, "func_", 5) == 0)
		killer_weapon_name += 5;

	// the original's tail remaps (@0x79a97..): raw code names → display names
	if (strncmp(killer_weapon_name, "knife", 6) == 0)
		killer_weapon_name = "Throwing Knife";
	else if (strncmp(killer_weapon_name, "shrapnel", 9) == 0)
		killer_weapon_name = "Shrapnel";
	else if (strncmp(killer_weapon_name, "grenade", 8) == 0) // @0x10c734 = "handgrenade"+4
		killer_weapon_name = "M61 Grenade";

	return killer_weapon_name;
}

//=========================================================
// Deathnotice.
//=========================================================
void CHalfLifeMultiplay::DeathNotice(CBasePlayer* pVictim, entvars_t* pKiller, entvars_t* pevInflictor)
{
	// The weapon PlayerKilled worked out for this death (or, called from elsewhere, work it out).
	const char* killer_weapon_name = s_pszKillWeapon ? s_pszKillWeapon : TSKillWeaponName(pVictim, pKiller, pevInflictor);

	MESSAGE_BEGIN(MSG_ALL, gmsgDeathMsg);
	WRITE_BYTE((pKiller->flags & FL_CLIENT) != 0 ? ENTINDEX(ENT(pKiller)) : 0); // the killer
	WRITE_BYTE(ENTINDEX(pVictim->edict())); // the victim
	WRITE_STRING(killer_weapon_name);		// what they were killed by (should this be a string?)
	MESSAGE_END();

	MESSAGE_BEGIN(MSG_SPEC, SVC_DIRECTOR);
	WRITE_BYTE(9);							 // command length in bytes
	WRITE_BYTE(DRC_CMD_EVENT);				 // player killed
	WRITE_SHORT(ENTINDEX(pVictim->edict())); // index number of primary entity
	if (pevInflictor)
		WRITE_SHORT(ENTINDEX(ENT(pevInflictor))); // index number of secondary entity
	else
		WRITE_SHORT(ENTINDEX(ENT(pKiller))); // index number of secondary entity
	WRITE_LONG(7 | DRC_FLAG_DRAMATIC);		 // eventflags (priority and flags)
	MESSAGE_END();

	//  Print a standard message
	// TODO: make this go direct to console
	return; // just remove for now
			/*
	char	szText[ 128 ];

	if ( pKiller->flags & FL_MONSTER )
	{
		// killed by a monster
		strcpy ( szText, STRING( pVictim->pev->netname ) );
		strcat ( szText, " was killed by a monster.\n" );
		return;
	}

	if ( pKiller == pVictim->pev )
	{
		strcpy ( szText, STRING( pVictim->pev->netname ) );
		strcat ( szText, " commited suicide.\n" );
	}
	else if ( pKiller->flags & FL_CLIENT )
	{
		strcpy ( szText, STRING( pKiller->netname ) );

		strcat( szText, " : " );
		strcat( szText, killer_weapon_name );
		strcat( szText, " : " );

		strcat ( szText, STRING( pVictim->pev->netname ) );
		strcat ( szText, "\n" );
	}
	else if ( FClassnameIs ( pKiller, "worldspawn" ) )
	{
		strcpy ( szText, STRING( pVictim->pev->netname ) );
		strcat ( szText, " fell or drowned or something.\n" );
	}
	else if ( pKiller->solid == SOLID_BSP )
	{
		strcpy ( szText, STRING( pVictim->pev->netname ) );
		strcat ( szText, " was mooshed.\n" );
	}
	else
	{
		strcpy ( szText, STRING( pVictim->pev->netname ) );
		strcat ( szText, " died mysteriously.\n" );
	}

	UTIL_ClientPrintAll( szText );
*/
}

//=========================================================
// The kill's log line: PlayerKilled writes it after the DeathNotice call (@0x79aa0-0x7a182), so a
// team kill, whose teamplay DeathNotice sends "teammate" and stops, is logged too.
//=========================================================
static void TSLogKill(CBasePlayer* pVictim, entvars_t* pKiller, const char* killer_weapon_name)
{
	// the original's post-DeathMsg log transform (@0x79aa0 loop): the LOG gets the name
	// lowercased with spaces → underscores ("M61 Grenade" → "m61_grenade").
	char log_weapon_name[64];
	strncpy(log_weapon_name, killer_weapon_name, sizeof(log_weapon_name) - 1);
	log_weapon_name[sizeof(log_weapon_name) - 1] = 0;
	for (char* p = log_weapon_name; *p; p++)
	{
		if (*p >= 'A' && *p <= 'Z')
			*p += 'a' - 'A';
		else if (*p == ' ')
			*p = '_';
	}
	killer_weapon_name = log_weapon_name;

	if (pVictim->pev == pKiller)
	{
		// killed self

		// team match?
		if (g_teamplay)
		{
			UTIL_LogPrintf("\"%s<%i><%s><%s>\" committed suicide with \"%s\"\n",
				STRING(pVictim->pev->netname),
				GETPLAYERUSERID(pVictim->edict()),
				GETPLAYERAUTHID(pVictim->edict()),
				g_engfuncs.pfnInfoKeyValue(g_engfuncs.pfnGetInfoKeyBuffer(pVictim->edict()), "model"),
				killer_weapon_name);
		}
		else
		{
			UTIL_LogPrintf("\"%s<%i><%s><%i>\" committed suicide with \"%s\"\n",
				STRING(pVictim->pev->netname),
				GETPLAYERUSERID(pVictim->edict()),
				GETPLAYERAUTHID(pVictim->edict()),
				GETPLAYERUSERID(pVictim->edict()),
				killer_weapon_name);
		}
	}
	else if ((pKiller->flags & FL_CLIENT) != 0)
	{
		// team match?
		if (g_teamplay)
		{
			UTIL_LogPrintf("\"%s<%i><%s><%s>\" killed \"%s<%i><%s><%s>\" with \"%s\"%s\n",
				STRING(pKiller->netname),
				GETPLAYERUSERID(ENT(pKiller)),
				GETPLAYERAUTHID(ENT(pKiller)),
				g_engfuncs.pfnInfoKeyValue(g_engfuncs.pfnGetInfoKeyBuffer(ENT(pKiller)), "model"),
				STRING(pVictim->pev->netname),
				GETPLAYERUSERID(pVictim->edict()),
				GETPLAYERAUTHID(pVictim->edict()),
				g_engfuncs.pfnInfoKeyValue(g_engfuncs.pfnGetInfoKeyBuffer(pVictim->edict()), "model"),
				killer_weapon_name, s_szKillLogTags);
		}
		else
		{
			UTIL_LogPrintf("\"%s<%i><%s><%i>\" killed \"%s<%i><%s><%i>\" with \"%s\"%s\n",
				STRING(pKiller->netname),
				GETPLAYERUSERID(ENT(pKiller)),
				GETPLAYERAUTHID(ENT(pKiller)),
				GETPLAYERUSERID(ENT(pKiller)),
				STRING(pVictim->pev->netname),
				GETPLAYERUSERID(pVictim->edict()),
				GETPLAYERAUTHID(pVictim->edict()),
				GETPLAYERUSERID(pVictim->edict()),
				killer_weapon_name, s_szKillLogTags);
		}
	}
	else
	{
		// killed by the world

		// team match?
		if (g_teamplay)
		{
			UTIL_LogPrintf("\"%s<%i><%s><%s>\" committed suicide with \"%s\" (world)\n",
				STRING(pVictim->pev->netname),
				GETPLAYERUSERID(pVictim->edict()),
				GETPLAYERAUTHID(pVictim->edict()),
				g_engfuncs.pfnInfoKeyValue(g_engfuncs.pfnGetInfoKeyBuffer(pVictim->edict()), "model"),
				killer_weapon_name);
		}
		else
		{
			UTIL_LogPrintf("\"%s<%i><%s><%i>\" committed suicide with \"%s\" (world)\n",
				STRING(pVictim->pev->netname),
				GETPLAYERUSERID(pVictim->edict()),
				GETPLAYERAUTHID(pVictim->edict()),
				GETPLAYERUSERID(pVictim->edict()),
				killer_weapon_name);
		}
	}
}

//=========================================================
// PlayerGotWeapon - player has grabbed a weapon that was
// sitting in the world
//=========================================================
void CHalfLifeMultiplay::PlayerGotWeapon(CBasePlayer* pPlayer, CBasePlayerItem* pWeapon)
{
}

//=========================================================
// FlWeaponRespawnTime - what is the time in the future
// at which this weapon may spawn?
//=========================================================
float CHalfLifeMultiplay::FlWeaponRespawnTime(CBasePlayerItem* pWeapon)
{
	if (weaponstay.value > 0)
	{
		// make sure it's only certain weapons
		if ((pWeapon->iFlags() & ITEM_FLAG_LIMITINWORLD) == 0)
		{
			return gpGlobals->time + 0; // weapon respawns almost instantly
		}
	}

	return gpGlobals->time + WEAPON_RESPAWN_TIME;
}

// when we are within this close to running out of entities,  items
// marked with the ITEM_FLAG_LIMITINWORLD will delay their respawn
#define ENTITY_INTOLERANCE 100

//=========================================================
// FlWeaponRespawnTime - Returns 0 if the weapon can respawn
// now,  otherwise it returns the time at which it can try
// to spawn again.
//=========================================================
float CHalfLifeMultiplay::FlWeaponTryRespawn(CBasePlayerItem* pWeapon)
{
	if (pWeapon && WEAPON_NONE != pWeapon->m_iId && (pWeapon->iFlags() & ITEM_FLAG_LIMITINWORLD) != 0)
	{
		if (NUMBER_OF_ENTITIES() < (gpGlobals->maxEntities - ENTITY_INTOLERANCE))
			return 0;

		// we're past the entity tolerance level,  so delay the respawn
		return FlWeaponRespawnTime(pWeapon);
	}

	return 0;
}

//=========================================================
// VecWeaponRespawnSpot - where should this weapon spawn?
// Some game variations may choose to randomize spawn locations
//=========================================================
Vector CHalfLifeMultiplay::VecWeaponRespawnSpot(CBasePlayerItem* pWeapon)
{
	return pWeapon->pev->origin;
}

//=========================================================
// WeaponShouldRespawn - any conditions inhibiting the
// respawning of this weapon?
//=========================================================
int CHalfLifeMultiplay::WeaponShouldRespawn(CBasePlayerItem* pWeapon)
{
	if ((pWeapon->pev->spawnflags & SF_NORESPAWN) != 0)
	{
		return GR_WEAPON_RESPAWN_NO;
	}

	return GR_WEAPON_RESPAWN_YES;
}

//=========================================================
// CanHaveWeapon - returns false if the player is not allowed
// to pick up this weapon
//=========================================================
bool CHalfLifeMultiplay::CanHavePlayerItem(CBasePlayer* pPlayer, CBasePlayerItem* pItem)
{
	if (weaponstay.value > 0)
	{
		if ((pItem->iFlags() & ITEM_FLAG_LIMITINWORLD) != 0)
			return CGameRules::CanHavePlayerItem(pPlayer, pItem);

		// check if the player already has this weapon
		for (int i = 0; i < MAX_ITEM_TYPES; i++)
		{
			CBasePlayerItem* it = pPlayer->m_rgpPlayerItems[i];

			while (it != NULL)
			{
				if (it->m_iId == pItem->m_iId)
				{
					return false;
				}

				it = it->m_pNext;
			}
		}
	}

	return CGameRules::CanHavePlayerItem(pPlayer, pItem);
}

//=========================================================
//=========================================================
bool CHalfLifeMultiplay::CanHaveItem(CBasePlayer* pPlayer, CItem* pItem)
{
	return true;
}

//=========================================================
//=========================================================
void CHalfLifeMultiplay::PlayerGotItem(CBasePlayer* pPlayer, CItem* pItem)
{
}

//=========================================================
//=========================================================
int CHalfLifeMultiplay::ItemShouldRespawn(CItem* pItem)
{
	if ((pItem->pev->spawnflags & SF_NORESPAWN) != 0)
	{
		return GR_ITEM_RESPAWN_NO;
	}

	return GR_ITEM_RESPAWN_YES;
}


//=========================================================
// At what time in the future may this Item respawn?
//=========================================================
float CHalfLifeMultiplay::FlItemRespawnTime(CItem* pItem)
{
	return gpGlobals->time + ITEM_RESPAWN_TIME;
}

//=========================================================
// Where should this item respawn?
// Some game variations may choose to randomize spawn locations
//=========================================================
Vector CHalfLifeMultiplay::VecItemRespawnSpot(CItem* pItem)
{
	return pItem->pev->origin;
}

//=========================================================
//=========================================================
void CHalfLifeMultiplay::PlayerGotAmmo(CBasePlayer* pPlayer, char* szName, int iCount)
{
}

//=========================================================
//=========================================================
bool CHalfLifeMultiplay::IsAllowedToSpawn(CBaseEntity* pEntity)
{
	//	if ( pEntity->pev->flags & FL_MONSTER )
	//		return false;

	return true;
}

//=========================================================
//=========================================================
int CHalfLifeMultiplay::AmmoShouldRespawn(CBasePlayerAmmo* pAmmo)
{
	if ((pAmmo->pev->spawnflags & SF_NORESPAWN) != 0)
	{
		return GR_AMMO_RESPAWN_NO;
	}

	return GR_AMMO_RESPAWN_YES;
}

//=========================================================
//=========================================================
float CHalfLifeMultiplay::FlAmmoRespawnTime(CBasePlayerAmmo* pAmmo)
{
	return gpGlobals->time + AMMO_RESPAWN_TIME;
}

//=========================================================
//=========================================================
Vector CHalfLifeMultiplay::VecAmmoRespawnSpot(CBasePlayerAmmo* pAmmo)
{
	return pAmmo->pev->origin;
}

//=========================================================
//=========================================================
float CHalfLifeMultiplay::FlHealthChargerRechargeTime()
{
	return 60;
}


float CHalfLifeMultiplay::FlHEVChargerRechargeTime()
{
	return 30;
}

//=========================================================
//=========================================================
int CHalfLifeMultiplay::DeadPlayerWeapons(CBasePlayer* pPlayer)
{
	return GR_PLR_DROP_GUN_ACTIVE;
}

//=========================================================
//=========================================================
int CHalfLifeMultiplay::DeadPlayerAmmo(CBasePlayer* pPlayer)
{
	return GR_PLR_DROP_AMMO_ACTIVE;
}

edict_t* CHalfLifeMultiplay::GetPlayerSpawnSpot(CBasePlayer* pPlayer)
{
	edict_t* pentSpawnSpot = CGameRules::GetPlayerSpawnSpot(pPlayer);
	if (IsMultiplayer() && !FStringNull(pentSpawnSpot->v.target))
	{
		FireTargets(STRING(pentSpawnSpot->v.target), pPlayer, pPlayer, USE_TOGGLE, 0);
	}

	return pentSpawnSpot;
}


//=========================================================
//=========================================================
int CHalfLifeMultiplay::PlayerRelationship(CBaseEntity* pPlayer, CBaseEntity* pTarget)
{
	// half life deathmatch has only enemies
	return GR_NOTTEAMMATE;
}

bool CHalfLifeMultiplay::PlayFootstepSounds(CBasePlayer* pl, float fvol)
{
	if (g_footsteps && g_footsteps->value == 0)
		return false;

	if (pl->IsOnLadder() || pl->pev->velocity.Length2D() > 220)
		return true; // only make step sounds in multiplayer if the player is moving fast enough

	return false;
}

bool CHalfLifeMultiplay::FAllowFlashlight()
{
	return flashlight.value != 0;
}

//=========================================================
//=========================================================
bool CHalfLifeMultiplay::FAllowMonsters()
{
	return (allowmonsters.value != 0);
}

//=========================================================
//======== CHalfLifeMultiplay private functions ===========
#define INTERMISSION_TIME 6

void CHalfLifeMultiplay::GoToIntermission()
{
	if (g_fGameOver)
		return; // intermission has already been triggered, so ignore.

	MESSAGE_BEGIN(MSG_ALL, SVC_INTERMISSION);
	MESSAGE_END();

	// bounds check
	int time = (int)CVAR_GET_FLOAT("mp_chattime");
	if (time < 1)
		CVAR_SET_STRING("mp_chattime", "1");
	else if (time > MAX_INTERMISSION_TIME)
		CVAR_SET_STRING("mp_chattime", UTIL_dtos1(MAX_INTERMISSION_TIME));

	m_flIntermissionEndTime = gpGlobals->time + ((int)mp_chattime.value);
	m_flIntermissionStartTime = gpGlobals->time;

	g_fGameOver = true;
	m_iEndIntermissionButtonHit = false;
}

#define MAX_RULE_BUFFER 1024

typedef struct mapcycle_item_s
{
	struct mapcycle_item_s* next;

	char mapname[32];
	int minplayers, maxplayers;
	char rulebuffer[MAX_RULE_BUFFER];
} mapcycle_item_t;

typedef struct mapcycle_s
{
	struct mapcycle_item_s* items;
	struct mapcycle_item_s* next_item;
} mapcycle_t;

/*
==============
DestroyMapCycle

Clean up memory used by mapcycle when switching it
==============
*/
void DestroyMapCycle(mapcycle_t* cycle)
{
	mapcycle_item_t *p, *n, *start;
	p = cycle->items;
	if (p)
	{
		start = p;
		p = p->next;
		while (p != start)
		{
			n = p->next;
			delete p;
			p = n;
		}

		delete cycle->items;
	}
	cycle->items = NULL;
	cycle->next_item = NULL;
}

static char com_token[1500];

/*
==============
COM_Parse

Parse a token out of a string
==============
*/
char* COM_Parse(char* data)
{
	int c;
	int len;

	len = 0;
	com_token[0] = 0;

	if (!data)
		return NULL;

// skip whitespace
skipwhite:
	while ((c = *data) <= ' ')
	{
		if (c == 0)
			return NULL; // end of file;
		data++;
	}

	// skip // comments
	if (c == '/' && data[1] == '/')
	{
		while ('\0' != *data && *data != '\n')
			data++;
		goto skipwhite;
	}


	// handle quoted strings specially
	if (c == '\"')
	{
		data++;
		while (true)
		{
			c = *data++;
			if (c == '\"' || '\0' == c)
			{
				com_token[len] = 0;
				return data;
			}
			com_token[len] = c;
			len++;
		}
	}

	// parse single characters
	if (c == '{' || c == '}' || c == ')' || c == '(' || c == '\'' || c == ',')
	{
		com_token[len] = c;
		len++;
		com_token[len] = 0;
		return data + 1;
	}

	// parse a regular word
	do
	{
		com_token[len] = c;
		data++;
		len++;
		c = *data;
		if (c == '{' || c == '}' || c == ')' || c == '(' || c == '\'' || c == ',')
			break;
	} while (c > 32);

	com_token[len] = 0;
	return data;
}

/*
==============
COM_TokenWaiting

Returns 1 if additional data is waiting to be processed on this line
==============
*/
bool COM_TokenWaiting(char* buffer)
{
	char* p;

	p = buffer;
	while ('\0' != *p && *p != '\n')
	{
		if (0 == isspace(*p) || 0 != isalnum(*p))
			return true;

		p++;
	}

	return false;
}



/*
==============
ReloadMapCycleFile


Parses mapcycle.txt file into mapcycle_t structure
==============
*/
bool ReloadMapCycleFile(char* filename, mapcycle_t* cycle)
{
	char szBuffer[MAX_RULE_BUFFER];
	char szMap[32];
	int length;
	char* pFileList;
	char* aFileList = pFileList = (char*)LOAD_FILE_FOR_ME(filename, &length);
	bool hasbuffer;
	mapcycle_item_s *item, *newlist = NULL, *next;

	if (pFileList && 0 != length)
	{
		// the first map name in the file becomes the default
		while (true)
		{
			hasbuffer = false;
			memset(szBuffer, 0, MAX_RULE_BUFFER);

			pFileList = COM_Parse(pFileList);
			if (strlen(com_token) <= 0)
				break;

			strncpy(szMap, com_token, sizeof(szMap));
			szMap[sizeof(szMap) - 1] = '\0';

			// Any more tokens on this line?
			if (COM_TokenWaiting(pFileList))
			{
				pFileList = COM_Parse(pFileList);
				if (strlen(com_token) > 0)
				{
					hasbuffer = true;
					strncpy(szBuffer, com_token, sizeof(szBuffer));
					szBuffer[sizeof(szBuffer) - 1] = '\0';
				}
			}

			// Check map
			if (IS_MAP_VALID(szMap))
			{
				// Create entry
				char* s;

				item = new mapcycle_item_s;

				strcpy(item->mapname, szMap);

				item->minplayers = 0;
				item->maxplayers = 0;

				memset(item->rulebuffer, 0, MAX_RULE_BUFFER);

				if (hasbuffer)
				{
					s = g_engfuncs.pfnInfoKeyValue(szBuffer, "minplayers");
					if (s && '\0' != s[0])
					{
						item->minplayers = atoi(s);
						item->minplayers = V_max(item->minplayers, 0);
						item->minplayers = V_min(item->minplayers, gpGlobals->maxClients);
					}
					s = g_engfuncs.pfnInfoKeyValue(szBuffer, "maxplayers");
					if (s && '\0' != s[0])
					{
						item->maxplayers = atoi(s);
						item->maxplayers = V_max(item->maxplayers, 0);
						item->maxplayers = V_min(item->maxplayers, gpGlobals->maxClients);
					}

					// Remove keys
					//
					g_engfuncs.pfnInfo_RemoveKey(szBuffer, "minplayers");
					g_engfuncs.pfnInfo_RemoveKey(szBuffer, "maxplayers");

					strcpy(item->rulebuffer, szBuffer);
				}

				item->next = cycle->items;
				cycle->items = item;
			}
			else
			{
				ALERT(at_console, "Skipping %s from mapcycle, not a valid map\n", szMap);
			}
		}

		FREE_FILE(aFileList);
	}

	// Fixup circular list pointer
	item = cycle->items;

	// Reverse it to get original order
	while (item)
	{
		next = item->next;
		item->next = newlist;
		newlist = item;
		item = next;
	}
	cycle->items = newlist;
	item = cycle->items;

	// Didn't parse anything
	if (!item)
	{
		return false;
	}

	while (item->next)
	{
		item = item->next;
	}
	item->next = cycle->items;

	cycle->next_item = item->next;

	return true;
}

/*
==============
CountPlayers

Determine the current # of active players on the server for map cycling logic
==============
*/
int CountPlayers()
{
	int num = 0;

	for (int i = 1; i <= gpGlobals->maxClients; i++)
	{
		CBaseEntity* pEnt = UTIL_PlayerByIndex(i);

		if (pEnt)
		{
			num = num + 1;
		}
	}

	return num;
}

/*
==============
ExtractCommandString

Parse commands/key value pairs to issue right after map xxx command is issued on server
 level transition
==============
*/
void ExtractCommandString(char* s, char* szCommand)
{
	// Now make rules happen
	char pkey[512];
	char value[512]; // use two buffers so compares
					 // work without stomping on each other
	char* o;

	if (*s == '\\')
		s++;

	while (true)
	{
		o = pkey;
		while (*s != '\\')
		{
			if ('\0' == *s)
				return;
			*o++ = *s++;
		}
		*o = 0;
		s++;

		o = value;

		while (*s != '\\' && '\0' != *s)
		{
			if ('\0' == *s)
				return;
			*o++ = *s++;
		}
		*o = 0;

		strcat(szCommand, pkey);
		if (strlen(value) > 0)
		{
			strcat(szCommand, " ");
			strcat(szCommand, value);
		}
		strcat(szCommand, "\n");

		if ('\0' == *s)
			return;
		s++;
	}
}

/*
==============
ChangeLevel

Server is changing to a new level, check mapcycle.txt for map name and setup info
==============
*/
void CHalfLifeMultiplay::ChangeLevel()
{
	static char szPreviousMapCycleFile[256];
	static mapcycle_t mapcycle;

	char szNextMap[32];
	char szFirstMapInList[32];
	char szCommands[1500];
	char szRules[1500];
	int minplayers = 0, maxplayers = 0;
	strcpy(szFirstMapInList, "hldm1"); // the absolute default level is hldm1

	int curplayers;
	bool do_cycle = true;

	// find the map to change to
	char* mapcfile = (char*)CVAR_GET_STRING("mapcyclefile");
	ASSERT(mapcfile != NULL);

	szCommands[0] = '\0';
	szRules[0] = '\0';

	curplayers = CountPlayers();

	// Has the map cycle filename changed?
	if (stricmp(mapcfile, szPreviousMapCycleFile))
	{
		strcpy(szPreviousMapCycleFile, mapcfile);

		DestroyMapCycle(&mapcycle);

		if (!ReloadMapCycleFile(mapcfile, &mapcycle) || (!mapcycle.items))
		{
			ALERT(at_console, "Unable to load map cycle file %s\n", mapcfile);
			do_cycle = false;
		}
	}

	if (do_cycle && mapcycle.items)
	{
		bool keeplooking = false;
		bool found = false;
		mapcycle_item_s* item;

		// Assume current map
		strcpy(szNextMap, STRING(gpGlobals->mapname));
		strcpy(szFirstMapInList, STRING(gpGlobals->mapname));

		// Traverse list
		for (item = mapcycle.next_item; item->next != mapcycle.next_item; item = item->next)
		{
			keeplooking = false;

			ASSERT(item != NULL);

			if (item->minplayers != 0)
			{
				if (curplayers >= item->minplayers)
				{
					found = true;
					minplayers = item->minplayers;
				}
				else
				{
					keeplooking = true;
				}
			}

			if (item->maxplayers != 0)
			{
				if (curplayers <= item->maxplayers)
				{
					found = true;
					maxplayers = item->maxplayers;
				}
				else
				{
					keeplooking = true;
				}
			}

			if (keeplooking)
				continue;

			found = true;
			break;
		}

		if (!found)
		{
			item = mapcycle.next_item;
		}

		// Increment next item pointer
		mapcycle.next_item = item->next;

		// Perform logic on current item
		strcpy(szNextMap, item->mapname);

		ExtractCommandString(item->rulebuffer, szCommands);
		strcpy(szRules, item->rulebuffer);
	}

	// TS DELTA (@0xc097b): a forced next map (a vote winner, or an objective set it) overrides
	// the mapcycle pick — unless it is the literal "nextmap", which means "end this map, use the
	// mapcycle". Then clear it and reset the TS rules for the level change (@0xc09ce).
	if (TSGameRules)
	{
		if (TSGameRules->m_szEndMap[0] != '\0')
		{
			if (strncmp(TSGameRules->m_szEndMap, "nextmap", 8) != 0)
				strcpy(szNextMap, TSGameRules->m_szEndMap);
			TSGameRules->m_szEndMap[0] = '\0';
		}
		TSGameRules->Init(); // pre-changelevel state reset (unconditional in the original)
	}

	if (!IS_MAP_VALID(szNextMap))
	{
		strcpy(szNextMap, szFirstMapInList);
	}

	g_fGameOver = true;

	ALERT(at_console, "CHANGE LEVEL: %s\n", szNextMap);
	if (0 != minplayers || 0 != maxplayers)
	{
		ALERT(at_console, "PLAYER COUNT:  min %i max %i current %i\n", minplayers, maxplayers, curplayers);
	}
	if (strlen(szRules) > 0)
	{
		ALERT(at_console, "RULES:  %s\n", szRules);
	}

	CHANGE_LEVEL(szNextMap, NULL);
	if (strlen(szCommands) > 0)
	{
		SERVER_COMMAND(szCommands);
	}
}

#define MAX_MOTD_CHUNK 60
#define MAX_MOTD_LENGTH 1536 // (MAX_MOTD_CHUNK * 4)

void CHalfLifeMultiplay::SendMOTDToClient(edict_t* client)
{
	// read from the MOTD.txt file
	int length, char_count = 0;
	char* pFileList;
	char* aFileList = pFileList = (char*)LOAD_FILE_FOR_ME((char*)CVAR_GET_STRING("motdfile"), &length);

	// send the server name
	MESSAGE_BEGIN(MSG_ONE, gmsgServerName, NULL, client);
	WRITE_STRING(CVAR_GET_STRING("hostname"));
	MESSAGE_END();

	// Send the message of the day
	// read it chunk-by-chunk,  and send it in parts

	while (pFileList && '\0' != *pFileList && char_count < MAX_MOTD_LENGTH)
	{
		char chunk[MAX_MOTD_CHUNK + 1];

		if (strlen(pFileList) < MAX_MOTD_CHUNK)
		{
			strcpy(chunk, pFileList);
		}
		else
		{
			strncpy(chunk, pFileList, MAX_MOTD_CHUNK);
			chunk[MAX_MOTD_CHUNK] = 0; // strncpy doesn't always append the null terminator
		}

		char_count += strlen(chunk);
		if (char_count < MAX_MOTD_LENGTH)
			pFileList = aFileList + char_count;
		else
			*pFileList = 0;

		MESSAGE_BEGIN(MSG_ONE, gmsgMOTD, NULL, client);
		WRITE_BYTE(static_cast<int>('\0' == *pFileList)); // false means there is still more message to come
		WRITE_STRING(chunk);
		MESSAGE_END();
	}

	FREE_FILE(aFileList);
}

//=========================================================
//=========================================================
// Busters Gamerules
//=========================================================
//=========================================================

#define EGON_BUSTING_TIME 10

bool IsBustingGame()
{
	return sv_busters.value == 1;
}

bool IsPlayerBusting(CBaseEntity* pPlayer)
{
	if (!pPlayer || !pPlayer->IsPlayer() || !IsBustingGame())
		return false;

	return ((CBasePlayer*)pPlayer)->HasPlayerItemFromID(WEAPON_EGON);
}

bool BustingCanHaveItem(CBasePlayer* pPlayer, CBaseEntity* pItem)
{
	bool bIsWeaponOrAmmo = false;

	if (strstr(STRING(pItem->pev->classname), "weapon_") || strstr(STRING(pItem->pev->classname), "ammo_"))
	{
		bIsWeaponOrAmmo = true;
	}

	// Busting players can't have ammo nor weapons
	if (IsPlayerBusting(pPlayer) && bIsWeaponOrAmmo)
		return false;

	return true;
}

//=========================================================
CMultiplayBusters::CMultiplayBusters()
{
	m_flEgonBustingCheckTime = -1;
}

//=========================================================
void CMultiplayBusters::Think()
{
	CheckForEgons();

	CHalfLifeMultiplay::Think();
}

//=========================================================
int CMultiplayBusters::IPointsForKill(CBasePlayer* pAttacker, CBasePlayer* pKilled)
{
	// If the attacker is busting, they get a point per kill
	if (IsPlayerBusting(pAttacker))
		return 1;

	// If the victim is busting, then the attacker gets a point
	if (IsPlayerBusting(pKilled))
		return 2;

	return 0;
}

//=========================================================
void CMultiplayBusters::PlayerKilled(CBasePlayer* pVictim, entvars_t* pKiller, entvars_t* pInflictor)
{
	if (IsPlayerBusting(pVictim))
	{
		UTIL_ClientPrintAll(HUD_PRINTCENTER, "The Buster is dead!!");

		// Reset egon check time
		m_flEgonBustingCheckTime = -1;

		CBasePlayer* peKiller = NULL;
		CBaseEntity* ktmp = CBaseEntity::Instance(pKiller);

		if (ktmp && (ktmp->Classify() == CLASS_PLAYER))
		{
			peKiller = (CBasePlayer*)ktmp;
		}
		else if (ktmp && (ktmp->Classify() == CLASS_VEHICLE))
		{
			CBasePlayer* pDriver = ((CFuncVehicle*)ktmp)->m_pDriver;

			if (pDriver != NULL)
			{
				peKiller = pDriver;
				ktmp = pDriver;
				pKiller = pDriver->pev;
			}
		}

		if (peKiller)
		{
			UTIL_ClientPrintAll(HUD_PRINTTALK, UTIL_VarArgs("%s has has killed the Buster!\n", STRING(peKiller->pev->netname)));
		}

		pVictim->pev->renderfx = kRenderFxNone;
		pVictim->pev->rendercolor = g_vecZero;
		// pVictim->pev->effects &= ~EF_BRIGHTFIELD;
	}

	CHalfLifeMultiplay::PlayerKilled(pVictim, pKiller, pInflictor);
}

//=========================================================
void CMultiplayBusters::DeathNotice(CBasePlayer* pVictim, entvars_t* pKiller, entvars_t* pevInflictor)
{
	// Only death notices that the Buster was involved in in Busting game mode
	if (!IsPlayerBusting(pVictim) && !IsPlayerBusting(CBaseEntity::Instance(pKiller)))
		return;

	CHalfLifeMultiplay::DeathNotice(pVictim, pKiller, pevInflictor);
}

//=========================================================
int CMultiplayBusters::WeaponShouldRespawn(CBasePlayerItem* pWeapon)
{
	if (pWeapon->m_iId == WEAPON_EGON)
		return GR_WEAPON_RESPAWN_NO;

	return CHalfLifeMultiplay::WeaponShouldRespawn(pWeapon);
}


//=========================================================
// CheckForEgons:
// Check to see if any player has an egon
// If they don't then get the lowest player on the scoreboard and give them one
// Then check to see if any weapon boxes out there has an egon, and delete it
//=========================================================
void CMultiplayBusters::CheckForEgons()
{
	if (m_flEgonBustingCheckTime <= 0.0f)
	{
		m_flEgonBustingCheckTime = gpGlobals->time + EGON_BUSTING_TIME;
		return;
	}

	if (m_flEgonBustingCheckTime <= gpGlobals->time)
	{
		m_flEgonBustingCheckTime = -1.0f;

		for (int i = 1; i <= gpGlobals->maxClients; i++)
		{
			CBasePlayer* pPlayer = (CBasePlayer*)UTIL_PlayerByIndex(i);

			// Someone is busting, no need to continue
			if (IsPlayerBusting(pPlayer))
				return;
		}

		int bBestFrags = 9999;
		CBasePlayer* pBestPlayer = NULL;

		for (int i = 1; i <= gpGlobals->maxClients; i++)
		{
			CBasePlayer* pPlayer = (CBasePlayer*)UTIL_PlayerByIndex(i);

			if (pPlayer && pPlayer->pev->frags <= bBestFrags)
			{
				bBestFrags = pPlayer->pev->frags;
				pBestPlayer = pPlayer;
			}
		}

		if (pBestPlayer)
		{
			pBestPlayer->GiveNamedItem("weapon_egon");

			CBaseEntity* pEntity = NULL;

			// Find a weaponbox that includes an Egon, then destroy it
			while ((pEntity = UTIL_FindEntityByClassname(pEntity, "weaponbox")) != NULL)
			{
				CWeaponBox* pWeaponBox = (CWeaponBox*)pEntity;

				if (pWeaponBox)
				{
					CBasePlayerItem* pWeapon;

					for (int i = 0; i < MAX_ITEM_TYPES; i++)
					{
						pWeapon = pWeaponBox->m_rgpPlayerItems[i];

						while (pWeapon)
						{
							// There you are, bye box
							if (pWeapon->m_iId == WEAPON_EGON)
							{
								pWeaponBox->Kill();
								break;
							}

							pWeapon = pWeapon->m_pNext;
						}
					}
				}
			}
		}
	}
}

//=========================================================
bool CMultiplayBusters::CanHavePlayerItem(CBasePlayer* pPlayer, CBasePlayerItem* pItem)
{
	// Buster cannot have more weapons nor ammo
	if (BustingCanHaveItem(pPlayer, pItem) == false)
	{
		return false;
	}

	return CHalfLifeMultiplay::CanHavePlayerItem(pPlayer, pItem);
}

//=========================================================
bool CMultiplayBusters::CanHaveItem(CBasePlayer* pPlayer, CItem* pItem)
{
	// Buster cannot have more weapons nor ammo
	if (BustingCanHaveItem(pPlayer, pItem) == false)
	{
		return false;
	}

	return CHalfLifeMultiplay::CanHaveItem(pPlayer, pItem);
}

//=========================================================
void CMultiplayBusters::PlayerGotWeapon(CBasePlayer* pPlayer, CBasePlayerItem* pWeapon)
{
	if (pWeapon->m_iId == WEAPON_EGON)
	{
		pPlayer->RemoveAllItems(false);

		UTIL_ClientPrintAll(HUD_PRINTCENTER, "Long live the new Buster!");
		UTIL_ClientPrintAll(HUD_PRINTTALK, UTIL_VarArgs("%s is busting!\n", STRING(pPlayer->pev->netname)));

		SetPlayerModel(pPlayer);

		pPlayer->pev->health = pPlayer->pev->max_health;
		pPlayer->pev->armorvalue = 100;

		pPlayer->pev->renderfx = kRenderFxGlowShell;
		pPlayer->pev->renderamt = 25;
		pPlayer->pev->rendercolor = Vector(0, 75, 250);

		CBasePlayerWeapon* pEgon = (CBasePlayerWeapon*)pWeapon;

		pEgon->m_iDefaultAmmo = 100;
		pPlayer->m_rgAmmo[pEgon->m_iPrimaryAmmoType] = pEgon->m_iDefaultAmmo;

		g_engfuncs.pfnSetClientKeyValue(pPlayer->entindex(), g_engfuncs.pfnGetInfoKeyBuffer(pPlayer->edict()), "model", "ivan");
	}
}

void CMultiplayBusters::ClientUserInfoChanged(CBasePlayer* pPlayer, char* infobuffer)
{
	SetPlayerModel(pPlayer);

	// Set preferences
	pPlayer->SetPrefsFromUserinfo(infobuffer);
}

void CMultiplayBusters::PlayerSpawn(CBasePlayer* pPlayer)
{
	CHalfLifeMultiplay::PlayerSpawn(pPlayer);
	SetPlayerModel(pPlayer);
}

void CMultiplayBusters::SetPlayerModel(CBasePlayer* pPlayer)
{
	if (IsPlayerBusting(pPlayer))
	{
		g_engfuncs.pfnSetClientKeyValue(pPlayer->entindex(), g_engfuncs.pfnGetInfoKeyBuffer(pPlayer->edict()), "model", "ivan");
	}
	else
	{
		g_engfuncs.pfnSetClientKeyValue(pPlayer->entindex(), g_engfuncs.pfnGetInfoKeyBuffer(pPlayer->edict()), "model", "skeleton");
	}
}
