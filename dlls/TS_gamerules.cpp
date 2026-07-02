/***
*  TS_gamerules.cpp — RECONSTRUCTED free-function gamerules file (original TS_gamerules.cpp,
*  distinct from TS_gamerulesclass.cpp which holds the CTSGameRules class).
*
*  Currently holds HaveFlag (@0x67148). The original's other free functions destined here,
*  blocked-on-RE reconcile (dossier-freefuncs.md analysis_ff_gamerules.md):
*    DecideGamePlay, DoSpectacularNess, GetAccelBySlots, SetPlayerRespawnTime,
*    TSSelectSpawnSpot, VictimKatanaHold, VictimKnifeHold, VictimKungfuHold,
*    WeaponRestrictionByID, InitWeaponRestriction (funcfile.tsv).
***/
#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "player.h"
#include "ts_flag.h" // HaveFlag prototype + s_flag_status

// @0x67148 HaveFlag — is this player carrying a CTF flag? (free fn). The carry
// state is the inline s_flag_status on the player; the active byte is the answer.
bool HaveFlag(CBasePlayer *pPlayer)
{
	return pPlayer && pPlayer->m_flagStatus.active != 0;
}

// @0x?? InitWeaponRestriction — the base permitted-weapon-type mask (all five categories: knife,
// pistol, smg, rifle, shotgun). Free function in TS_gamerules.cpp (original home);
// sole caller: CTSMapGlobals::get_weapon_restriction (TS_entities.cpp). (It just returns 0x1f in the shipped game.)
int InitWeaponRestriction(void)
{
	return 0x1f;
}
