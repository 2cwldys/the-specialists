/***
*  TS_util.cpp — the original's server utility free functions (TS_util.cpp TU).
*
*  Landed by the freefuncs plan (reference/dossiers/cont81/dossier-freefuncs.md, F1/F2):
*  the slowed-array helpers, the weapon-param (CustomWP) pipeline, and the player
*  lifecycle utilities. Functions marked DEAD are unreferenced in the shipped Linux .so
*  (no reloc/PLT/direct call) and are kept compiled-and-uncalled for source parity.
***/
#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "player.h"
#include "ts_weapons.h"
#include "ts_weapon_info.h"
#include "ts_gamerules.h" // TSGameRules (the customweapons flag/absorbs)
#include "ts_utils.h"     // TSHudMessageOne/All

#include <string.h>

// @0x8e944 UTIL_EntInArray — first index of pEnt in pArray[0..nCount), else -1. Used by the
// slowed-entity array bookkeeping (RemoveEntityFromSlowedArray / SlowMotionFlow dedup) once
// the packed +0x208 array model lands.
int UTIL_EntInArray(CBaseEntity **pArray, int nCount, CBaseEntity *pEnt)
{
	for (int i = 0; i < nCount; i++)
	{
		if (pArray[i] == pEnt)
			return i;
	}
	return -1;
}

// @0x8e9c0 WeaponIdFromString — tsweapon_t id whose display name matches (case-insensitive),
// else -1. DEAD in the shipped .so (plausibly a compiled-out console command's helper).
int WeaponIdFromString(char *psz)
{
	for (int id = 0; id < TSW_COUNT; id++)
	{
		const s_weapon_info *wi = get_weapon_info((tsweapon_t)id);
		if (wi->pszName && strcasecmp(psz, wi->pszName) == 0)
			return id;
	}
	return -1;
}

// @0x8fa64 TSFoundRandomEntity — collect up to 32 entities of the classname, return a random
// one (NULL if none). DEAD in the shipped .so.
CBaseEntity *TSFoundRandomEntity(char *pszClassname)
{
	CBaseEntity *arr[32];
	int n = 0;
	CBaseEntity *p = nullptr;
	while (n < 32 && (p = UTIL_FindEntityByClassname(p, pszClassname)) != nullptr)
		arr[n++] = p;
	if (n == 0)
		return nullptr;
	return arr[RANDOM_LONG(0, n - 1)];
}

// @0x8fae0 PlayerIsCheating — bots never cheat; a human cheats iff their cl_lw != "1"
// (the forced-client-side-weapons check). Callers: ClientUserInfoChanged (which then forces
// cl_lw "1") and TSInit (result discarded — the original's quirk).
bool PlayerIsCheating(CBasePlayer *pPlayer, char *infobuffer)
{
	if (pPlayer->pev->flags & FL_FAKECLIENT)
		return false;
	const char *lw = g_engfuncs.pfnInfoKeyValue(infobuffer, "cl_lw");
	return !lw || strcmp(lw, "1") != 0;
}

// @0x8f238 TSCountPlayers — players on `team` (case-insensitive TeamID match; NULL/empty team =
// everyone); onlyActive counts the alive PLUS the dead-but-respawning (+0x770 >= 0).
int TSCountPlayers(const char *team, bool onlyActive)
{
	int n = 0;
	for (int i = 1; i <= gpGlobals->maxClients; i++)
	{
		CBasePlayer *p = (CBasePlayer *)UTIL_PlayerByIndex(i);
		if (!p)
			continue;
		if (team && team[0] && (!p->TeamID() || strcasecmp(team, p->TeamID()) != 0))
			continue;
		if (onlyActive && !p->IsAlive() && p->m_flTSRespawnTime < 0)
			continue;
		n++;
	}
	return n;
}

// ---- The CustomWP pipeline (dossier-freefuncs F2b / dossier-gmsg U5; analysis_ff_util §4-7) ----

// "CustomWP" (-1) — ONE WRITE_STRING: "%i %f %f %f %f %i" = id accuracy viewkick
// accuracykick damage cost. The ASCII line IS the wire format (drop-in critical).
int gmsgTSCustomWeaponParams = 0;

// @0x8eb98 SendWeaponParams — one weapon's tunables to one client (MSG_ONE) or everyone (MSG_ALL).
void SendWeaponParams(tsweapon_t id, CBaseEntity *pOnly)
{
	if ((unsigned)id >= 0x26)
		return;
	const s_weapon_info *wi = get_weapon_info(id);
	if (!wi->pszName || !wi->pszName[0])
		return;
	if (gmsgTSCustomWeaponParams == 0)
		gmsgTSCustomWeaponParams = REG_USER_MSG("CustomWP", -1);
	char buf[64];
	sprintf(buf, "%i %f %f %f %f %i", id, wi->flSpread, wi->flViewKick, wi->flRecoil,
		wi->flDamage, wi->iPrice);
	buf[63] = 0;
	if (pOnly)
		MESSAGE_BEGIN(MSG_ONE, gmsgTSCustomWeaponParams, NULL, pOnly->edict());
	else
		MESSAGE_BEGIN(MSG_ALL, gmsgTSCustomWeaponParams);
	WRITE_STRING(buf);
	MESSAGE_END();
}

// @0x8eb14 InformClientsOnWeaponsParams — the "custom weapons" banner + the full 38-slot sync.
void InformClientsOnWeaponsParams(CBaseEntity *pOnly)
{
	if (pOnly)
		TSHudMessageOne(45, 195, 45, 8, 100, "This server is using custom weapons.", pOnly->pev);
	else
		TSHudMessageAll(45, 195, 45, 8, 100, "This server is using custom weapons.");
	for (int i = 0; i < 0x26; i++)
		SendWeaponParams((tsweapon_t)i, pOnly);
}

// @0x8ea3c AdjustSpecificWeapon — set one weapon's tunable quintet (no clamps), broadcast it,
// announce, and flag the rules custom. DEAD in the shipped Linux .so (the console path was
// compiled out) — kept callable for parity.
void AdjustSpecificWeapon(tsweapon_t id, float acc, float vkick, float akick, float dmg, int cost)
{
	if ((unsigned)id >= 0x26)
		return;
	s_weapon_info *wi = (s_weapon_info *)get_weapon_info(id);
	wi->flSpread = acc;
	wi->flViewKick = vkick;
	wi->flRecoil = akick;
	wi->flDamage = dmg;
	wi->iPrice = cost;
	SendWeaponParams(id, NULL);
	UTIL_ClientPrintAll(HUD_PRINTTALK, UTIL_VarArgs("%s behaviour changed\n", wi->pszName));
	if (TSGameRules())
		TSGameRules()->m_bWeaponParamsCustom = 1;
}

// @0x8eca4 adjust_weapon_value — load + apply a weapons config file from the game dir.
// Exact behaviors kept: "//" ANYWHERE = comment; armor_absorb/health_absorb lines (clamped
// >= 0) land in the rules; a weapon line with fewer than 6 fields ABORTS with the original
// console string but still returns 1; accuracy floors at 0.001 (the only clamp); EOF
// announces + sets the custom flag. Returns 0 only when the file can't open.
int adjust_weapon_value(char *pszFile)
{
	char dir[64];
	GET_GAME_DIR(dir);
	char path[192];
	sprintf(path, "%s%s%s", dir, "/", pszFile);
	FILE *fp = fopen(path, "rb");
	if (!fp)
		return 0;

	char raw[1024];
	while (fgets(raw, sizeof(raw), fp))
	{
		char line[256];
		strncpy(line, raw, sizeof(line) - 1);
		line[sizeof(line) - 1] = 0;
		int len = (int)strlen(line);
		while (len > 0 && isspace(line[len - 1]))
			line[--len] = 0;
		if (len == 0)
			continue;
		if (strstr(line, "//"))
			continue; // comment anywhere kills the line
		float v;
		if (strstr(line, "armor_absorb"))
		{
			if (sscanf(strstr(line, "armor_absorb") + 12, " %f ", &v) == 1)
			{
				if (v < 0)
					v = 0;
				if (TSGameRules())
					TSGameRules()->m_flArmorAbsorb = v; // rules+0x588c
			}
			continue;
		}
		if (strstr(line, "health_absorb"))
		{
			if (sscanf(strstr(line, "health_absorb") + 13, " %f ", &v) == 1)
			{
				if (v < 0)
					v = 0;
				if (TSGameRules())
					TSGameRules()->m_flHealthAbsorb = v; // rules+0x5888
			}
			continue;
		}
		int id, cost;
		float acc, vkick, akick, dmg;
		if (sscanf(line, " %i %f %f %f %f %i ", &id, &acc, &vkick, &akick, &dmg, &cost) < 6)
		{
			ALERT(at_console, "You need to run saveweapons again.\n");
			fclose(fp);
			return 1; // mid-file abort still reports handled (the original's literal behavior)
		}
		if ((unsigned)id < 0x26)
		{
			s_weapon_info *wi = (s_weapon_info *)get_weapon_info((tsweapon_t)id);
			if (wi->pszName && wi->pszName[0])
			{
				wi->flSpread = (acc < 0.001f) ? 0.001f : acc; // the only clamp
				wi->flViewKick = vkick;
				wi->flRecoil = akick;
				wi->flDamage = dmg;
				wi->iPrice = cost;
			}
		}
	}
	fclose(fp);
	UTIL_ClientPrintAll(HUD_PRINTTALK, UTIL_VarArgs("weapons behaviour changed by %s\n", pszFile));
	if (TSGameRules())
		TSGameRules()->m_bWeaponParamsCustom = 1;
	return 1;
}

// @0x8f044 save_weapon_value — write the weapons config file (the exact original header;
// footer saves armor_absorb ONLY — health_absorb is parseable but never written, asymmetry kept).
int save_weapon_value(char *pszFile)
{
	char dir[64];
	GET_GAME_DIR(dir);
	char path[192];
	sprintf(path, "%s%s%s", dir, "/", pszFile);
	FILE *fp = fopen(path, "w");
	if (!fp)
		return 0;

	fputs(
		"//===========================================\n"
		"//values are : |weapon id|accuracy|view kick|accuracy kick|damage|cost|\n\n"
		"//IMPORTANT:\n"
		"//-dont change weapon id!\n"
		"//-accuracy cant go under 0.001\n"
		"//-if you put an accuracy kick of 0 accuracy will not be scaled\n\n"
		"//If you want use different weapon settings use the 'customweapons' cvar.\n"
		"//Example: 'customweapons weapons_custom.txt'\n"
		"//===========================================\n\n",
		fp);
	for (int i = 0; i < 0x26; i++)
	{
		const s_weapon_info *wi = get_weapon_info((tsweapon_t)i);
		if (!wi->pszName || !wi->pszName[0])
			continue;
		fprintf(fp, "//%s\n%i %f %f %f %f %i\n\n", wi->pszName, i, wi->flSpread,
			wi->flViewKick, wi->flRecoil, wi->flDamage, wi->iPrice);
	}
	// footer: armor_absorb ONLY (health_absorb is parseable by the loader but never saved —
	// the original's asymmetry, kept). .rodata format "armor_absorb %f".
	fprintf(fp, "armor_absorb %f\n", TSGameRules() ? TSGameRules()->m_flArmorAbsorb : 0.0f);
	fclose(fp);
	return 1;
}
