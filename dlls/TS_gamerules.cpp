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
