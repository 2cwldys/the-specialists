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
#include "extdll.h"
#include "eiface.h"
#include "util.h"
#include "client.h"
#include "game.h"
#include "filesystem_utils.h"
#include "cbase.h"
#include "ts_gamerules.h" // TSGameRules/WeaponRestrictionByID (cmd_weaponrestriction)

cvar_t displaysoundlist = {"displaysoundlist", "0"};

// multiplayer server rules
cvar_t fragsleft = {"mp_fragsleft", "0", FCVAR_SERVER | FCVAR_UNLOGGED}; // Don't spam console/log files/users with this changing
cvar_t timeleft = {"mp_timeleft", "0", FCVAR_SERVER | FCVAR_UNLOGGED};	 // "      "

// multiplayer server rules
cvar_t teamplay = {"mp_teamplay", "0", FCVAR_SERVER};
cvar_t fraglimit = {"mp_fraglimit", "0", FCVAR_SERVER};
cvar_t timelimit = {"mp_timelimit", "0", FCVAR_SERVER};
cvar_t friendlyfire = {"mp_friendlyfire", "0", FCVAR_SERVER};
cvar_t falldamage = {"mp_falldamage", "0", FCVAR_SERVER};
cvar_t weaponstay = {"mp_weaponstay", "0", FCVAR_SERVER};
cvar_t forcerespawn = {"mp_forcerespawn", "1", FCVAR_SERVER};
cvar_t aimcrosshair = {"mp_autocrosshair", "1", FCVAR_SERVER};
cvar_t decalfrequency = {"decalfrequency", "30", FCVAR_SERVER};
cvar_t teamlist = {"mp_teamlist", "hitman;gordon", FCVAR_SERVER}; // TS default (ts_i386.so @0x108dd0; stock is "hgrunt;scientist")
cvar_t teammodels = {"mp_teammodels", "", FCVAR_SERVER}; // the team model lists (CHalfLifeTeamplay's constructor)
cvar_t teamoverride = {"mp_teamoverride", "1"};
cvar_t defaultteam = {"mp_defaultteam", "0"};
cvar_t allowmonsters = {"mp_allowmonsters", "0", FCVAR_SERVER};

cvar_t mp_chattime = {"mp_chattime", "10", FCVAR_SERVER};

// The Specialists' cvars, in the original's .data order (ts_i386.so @0x108e48..0x109014), names,
// defaults and flags as its cvar_t structs. Some globals are not named after their cvar: dmcashbased
// is "usecash", cv_ammocount "ammocount", weaponstay_time "weaponstay" (the dropped gun lifetime,
// not mp_weaponstay).
// initial_cash, cashforwin, cashforlose and respawnnumber are never registered (TS 3.0's changelog:
// "Removed initial_cash. Initial cash set at 1200.", the same for the other two), so a server config
// cannot set them. Their readers use the struct as compiled: the default string where they sscanf it
// (ClientPutInServer, TeamWin), the value field, which stays 0, where they read the float.
cvar_t initial_cash = {"initial_cash", "1200", FCVAR_SERVER};
cvar_t cashforwin = {"cashforwin", "10000", FCVAR_SERVER};
cvar_t cashforlose = {"cashforlose", "1200", FCVAR_SERVER};
cvar_t roundtime = {"roundtime", "150", FCVAR_SERVER};
cvar_t respawntime = {"respawntime", "3", FCVAR_SERVER};
cvar_t damagemult = {"damagemult", "0.0", FCVAR_SERVER};
cvar_t slowmatch = {"slowmatch", "1.0", FCVAR_SERVER};
cvar_t nopowerup = {"nopowerup", "0.0", FCVAR_SERVER};
cvar_t botlearn = {"botlearn", "0.0", FCVAR_SERVER};
cvar_t realbullet = {"realbullet", "0.0", FCVAR_SERVER};
cvar_t dmcashbased = {"usecash", "0.0", FCVAR_SERVER};
cvar_t cv_ammocount = {"ammocount", "0", FCVAR_SERVER};
cvar_t bottalk = {"bottalk", "1", FCVAR_SERVER};
cvar_t weaponstay_time = {"weaponstay", "15", FCVAR_SERVER};
cvar_t mp_theonemode = {"mp_theonemode", "0", FCVAR_SERVER};
cvar_t mp_theone = {"mp_theone", "gordon", FCVAR_SERVER};
cvar_t mp_theothers = {"mp_theothers", "seal", FCVAR_SERVER};
cvar_t lastmanstanding = {"lastmanstanding", "0", FCVAR_SERVER};
cvar_t spectacularness = {"spectacularness", "1", FCVAR_SERVER};
cvar_t respawnnumber = {"respawnnumber", "1", FCVAR_SERVER};
cvar_t customweapons = {"customweapons", "", FCVAR_SERVER};
cvar_t banfortk = {"banfortk", "0", FCVAR_SERVER};
cvar_t dmgreport = {"dmgreport", "0", FCVAR_SERVER};
cvar_t botarmor = {"botarmor", "0", FCVAR_SERVER};

static bool SV_InitServer()
{
	if (!FileSystem_LoadFileSystem())
	{
		return false;
	}

	return true;
}

// Register your console variables here
// This gets called one time when the game is initialied
// --- the `weaponrestriction` server command (@0xb58a0 cmd_weaponrestriction +
// @0xb5770 display_weaponrestriction; star_SrvSett.md). Presets via WeaponRestrictionByID,
// AND-masked by the map's allowed categories; broadcasts SrvSett id 3 ONLY on change.
static const char *const g_szRestrictions[6] = {
	"Kung Fu ", "Knives ", "Pistols ", "SMGs ", "Rifles ", "Shotguns "}; // [0] never printed

static void display_weaponrestriction()
{
	CTSGameRules *rules = TSGameRules;
	if (!rules)
		return;
	if (rules->m_iWeaponRestriction == 0)
	{
		ALERT(at_console, "All weapons are restricted.\n");
		return;
	}
	char buf[128];
	buf[0] = 0;
	for (int bit = 0; bit < 5; bit++)
		if (rules->m_iWeaponRestriction & (1 << bit))
			strcat(buf, g_szRestrictions[bit + 1]);
	ALERT(at_console, "Weapons allowed: %s\n", buf);
}

static void cmd_weaponrestriction()
{
	CTSGameRules *rules = TSGameRules;
	if (!rules)
		return;
	if (CMD_ARGC() < 2)
	{
		display_weaponrestriction();
		return;
	}
	const int old = rules->m_iWeaponRestriction;
	if (atoi(CMD_ARGV(1)) == 5)
		rules->m_iWeaponRestriction = InitWeaponRestriction();
	else
		rules->m_iWeaponRestriction = WeaponRestrictionByID(atoi(CMD_ARGV(1))) & rules->m_iAllowedCategories;
	if (rules->m_iWeaponRestriction != old && gmsgTSServerSetting != 0)
	{
		MESSAGE_BEGIN(MSG_ALL, gmsgTSServerSetting);
		WRITE_BYTE(3);
		WRITE_SHORT(rules->m_iWeaponRestriction);
		MESSAGE_END();
	}
	display_weaponrestriction();
}

// @0xb5520 cmd_addbot / @0xb5550 cmd_addcustombot — the bot commands of a dedicated server: the
// original's bots are listen-server only, so both just say so (ALERT at_console).
static void cmd_addbot()
{
	ALERT(at_console, "Bots only in listen servers.\n");
}

static void cmd_addcustombot()
{
	ALERT(at_console, "Bots only in listen servers.\n");
}

// @0xb5748 cmd_restartround — the `restartround` server command: ResetRestartRound(2), which in last man
// standing or the objective modes restarts the round in a second and clears the team scores, and
// otherwise does nothing.
static void cmd_restartround()
{
	if (TSGameRules)
		TSGameRules->ResetRestartRound(2);
}

// @0xb5580 cmd_globalslow — the "Permanent Slow-motion?" server setting (a COMMAND, not a cvar; the
// settings.scr entry sends `globalslow <percent>`). Parses percent -> a 0..2.0 scale (100% = normal =
// OFF), then drives CTSGameRules::SetGlobalSlowMotion (duration 0 = permanent).
static void cmd_globalslow()
{
	CTSGameRules *rules = TSGameRules;
	if (!rules)
		return;
	if (CMD_ARGC() < 2)
	{
		if (rules->m_flGlobalSlowActive != 0.0f)
			ALERT(at_console, "Globalslow is at %.0f%%.\n", rules->m_flGlobalSlowScale * 100.0f);
		else
			ALERT(at_console, "Globalslow is off.\n");
		return;
	}
	float scale = (float)atof(CMD_ARGV(1)) * 0.01f;
	if (scale > 2.0f) scale = 2.0f;                       // clamp(percent/100, 0, 2)
	if (scale < 0.0f) scale = 0.0f;
	if (scale > 0.999f && scale < 1.001f) scale = 0.0f;   // 100% = normal speed = OFF
	if (scale > 0.0f && scale < 0.01f)    scale = 0.01f;  // floor a tiny non-zero at 1%
	rules->SetGlobalSlowMotion(scale, 0.0f);              // duration 0 = permanent
	if (scale != 0.0f)
		ALERT(at_console, "Globalslow is at %.0f%%.\n", scale * 100.0f);
	else
		ALERT(at_console, "Globalslow is off.\n");
}

// --- customweapons load/save (@cmd_loadweapons/@cmd_saveweapons, game.cpp home). The console
// command NAMES are swap contract. loadweapons: adjust_weapon_value + broadcast the table;
// saveweapons: dump the current table. Default file per the original example.
int adjust_weapon_value(char *pszFile);   // TS_util.cpp
int save_weapon_value(char *pszFile);     // TS_util.cpp
void InformClientsOnWeaponsParams(CBaseEntity *pOnly); // TS_util.cpp

static void cmd_loadweapons()
{
	char *file = (char *)(CMD_ARGC() > 1 ? CMD_ARGV(1) : "weapons_custom.txt");
	if (adjust_weapon_value(file))
		InformClientsOnWeaponsParams(NULL);
	else
		ALERT(at_console, "Couldn't open %s\n", file);
}

static void cmd_saveweapons()
{
	char *file = (char *)(CMD_ARGC() > 1 ? CMD_ARGV(1) : "weapons_custom.txt");
	if (!save_weapon_value(file))
		ALERT(at_console, "Couldn't write %s\n", file);
}

void GameDLLInit()
{
	// @0xb5aa0: the server commands first, then the engine cvar pointers, then the cvars
	g_engfuncs.pfnAddServerCommand("addbot", cmd_addbot);
	g_engfuncs.pfnAddServerCommand("addcustombot", cmd_addcustombot);
	g_engfuncs.pfnAddServerCommand("globalslow", cmd_globalslow);
	g_engfuncs.pfnAddServerCommand("restartround", cmd_restartround);
	g_engfuncs.pfnAddServerCommand("weaponrestriction", cmd_weaponrestriction);
	g_engfuncs.pfnAddServerCommand("loadweapons", cmd_loadweapons);
	g_engfuncs.pfnAddServerCommand("saveweapons", cmd_saveweapons);

	g_psv_gravity = CVAR_GET_POINTER("sv_gravity");
	g_psv_aim = CVAR_GET_POINTER("sv_aim");
	g_psv_allow_autoaim = CVAR_GET_POINTER("sv_allow_autoaim");
	g_footsteps = CVAR_GET_POINTER("mp_footsteps");
	g_psv_cheats = CVAR_GET_POINTER("sv_cheats");

	if (!SV_InitServer())
	{
		g_engfuncs.pfnServerPrint("Error initializing server\n");
		//Shut the game down as soon as possible.
		SERVER_COMMAND("quit\n");
		return;
	}

	CVAR_REGISTER(&displaysoundlist);

	CVAR_REGISTER(&teamplay);
	CVAR_REGISTER(&fraglimit);
	CVAR_REGISTER(&timelimit);

	CVAR_REGISTER(&fragsleft);
	CVAR_REGISTER(&timeleft);

	CVAR_REGISTER(&friendlyfire);
	CVAR_REGISTER(&falldamage);
	CVAR_REGISTER(&weaponstay);
	CVAR_REGISTER(&forcerespawn);
	CVAR_REGISTER(&aimcrosshair);
	CVAR_REGISTER(&decalfrequency);
	CVAR_REGISTER(&teamlist);
	CVAR_REGISTER(&teammodels);
	CVAR_REGISTER(&teamoverride);
	CVAR_REGISTER(&defaultteam);
	CVAR_REGISTER(&allowmonsters);

	CVAR_REGISTER(&mp_chattime);

	CVAR_REGISTER(&roundtime);
	CVAR_REGISTER(&respawntime);
	CVAR_REGISTER(&damagemult);
	CVAR_REGISTER(&slowmatch);
	CVAR_REGISTER(&botlearn);
	CVAR_REGISTER(&nopowerup);
	CVAR_REGISTER(&realbullet);
	CVAR_REGISTER(&dmcashbased);
	CVAR_REGISTER(&cv_ammocount);
	CVAR_REGISTER(&bottalk);
	CVAR_REGISTER(&weaponstay_time);
	CVAR_REGISTER(&mp_theonemode);
	CVAR_REGISTER(&mp_theone);
	CVAR_REGISTER(&mp_theothers);
	CVAR_REGISTER(&lastmanstanding);
	CVAR_REGISTER(&spectacularness);
	CVAR_REGISTER(&customweapons);
	CVAR_REGISTER(&banfortk);
	CVAR_REGISTER(&dmgreport); // USER-APPROVED IMPROVEMENT: the original never registers it, so its cheat-gated prints are unreachable there
	CVAR_REGISTER(&botarmor);

	SERVER_COMMAND("exec skill.cfg\n");
}

void GameDLLShutdown()
{
	FileSystem_FreeFileSystem();
}
