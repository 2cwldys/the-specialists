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
