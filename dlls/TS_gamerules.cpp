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
#include "ts_gamerules.h" // Victim*Hold prototypes
#include "ts_weapons.h"   // TSPlayerFreeSlots (GetAccelBySlots)

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

// ---- melee deflect gates (RE dossier-melee.md §1.3 + analysis_ff_gamerules.md §6-8) ----
// Each returns true = the swing was BLOCKED (the caller aborts its damage). Getting blocked
// drains the ATTACKER 33 stamina (UpdateMeleeBar handles depletion + the KFuPower send) and
// stamps asymmetric next-attack locks. The block windows are the PreThink-decayed VALUES on
// CBasePlayer (+0x7a8 kungfu / +0x7ac knife / +0x7b0 katana).
// NB: the original emits these sounds via TS's 8-arg EMIT_SOUND_DYN (trailing 0.5f slow-mo
// pitch weight) — VERIFY that arg's formula once, globally; flat pitch 100 kept meanwhile.

// @0x66c8c VictimKatanaHold — a katana swing vs a blocking victim: needs the victim's KATANA
// window open AND melee bar > 0.1 (a knife window does NOT stop a katana). Locks 0.75/0.35.
bool VictimKatanaHold(CBasePlayer *pAttacker, CBaseEntity *pVictim, float flDamage)
{
	if (!pVictim->IsPlayer())
		return false;
	CBasePlayer *pBlocker = (CBasePlayer *)pVictim;
	if (pBlocker->m_flKatanaBlock <= 0.0f || pBlocker->m_flMeleeBar <= 0.1f)
		return false;

	UTIL_Sparks((pAttacker->pev->origin + pBlocker->pev->origin) * 0.5f + Vector(0, 0, 20));
	EMIT_SOUND_DYN(ENT(pAttacker->pev), CHAN_BODY, "weapons/knife/knife_hit.wav", VOL_NORM, 0.8f, 0, 100);
	pAttacker->m_flWhiffLungeForce = 0.0f;
	pAttacker->m_flNextAttack = 0.75f; // the swinger is stunned
	pBlocker->m_flNextAttack = 0.35f;  // the blocker recovers faster
	pAttacker->UpdateMeleeBar(33.0f);  // getting blocked costs the swinger
	return true;
}

// @0x66de4 VictimKnifeHold — a knife slash vs a blocking victim: a KNIFE window blocks for
// FREE (no bar check); a KATANA window also parries a knife but needs bar > 0.1. Locks 0.5/0.25.
bool VictimKnifeHold(CBasePlayer *pAttacker, CBaseEntity *pVictim, float flDamage)
{
	if (!pVictim->IsPlayer())
		return false;
	CBasePlayer *pBlocker = (CBasePlayer *)pVictim;
	if (!(pBlocker->m_flKnifeBlock > 0.0f ||
		  (pBlocker->m_flKatanaBlock > 0.0f && pBlocker->m_flMeleeBar > 0.1f)))
		return false;

	UTIL_Sparks((pAttacker->pev->origin + pBlocker->pev->origin) * 0.5f + Vector(0, 0, 20));
	EMIT_SOUND_DYN(ENT(pAttacker->pev), CHAN_BODY, "weapons/knife/knife_hit.wav", VOL_NORM, 0.8f, 0, 100);
	pAttacker->m_flWhiffLungeForce = 0.0f;
	pAttacker->m_flNextAttack = 0.5f;
	pBlocker->m_flNextAttack = 0.25f;
	pAttacker->UpdateMeleeBar(33.0f);
	return true;
}

// @0x66f50 VictimKungfuHold — the bare-hands block, WITH the counter role-swap: if the VICTIM's
// kungfu window is open they block; else if the ATTACKER's window is open while the victim is
// mid-whiff-lunge (kungfu-type lunge only), the roles SWAP — the original attacker counters the
// lunger. No sparks for kung-fu. The melee-bar gate stays on the ORIGINAL second param even
// after the swap (in the counter branch that is the LUNGER's bar — asymmetry kept faithful).
bool VictimKungfuHold(CBasePlayer *pAttacker, CBaseEntity *pVictim, float flDamage, TraceResult *ptr)
{
	if (!pVictim->IsPlayer())
		return false;
	CBasePlayer *pVic = (CBasePlayer *)pVictim;

	CBasePlayer *pBlocker, *pSwinger;
	if (pVic->m_flKungFuBlock > 0.0f)
	{
		pBlocker = pVic;
		pSwinger = pAttacker;
	}
	else if (pAttacker->m_flKungFuBlock > 0.0f && pVic->m_iWhiffLungeType == 0 &&
			 pVic->m_flWhiffLungeTime > 0.0f)
	{
		pBlocker = pAttacker; // COUNTER: roles swap
		pSwinger = pVic;
	}
	else
		return false;

	if (pVic->m_flMeleeBar < 0.1f)
		return false;

	// (the original computes a blocker->attacker direction + MakeVectors here and never uses
	// them — dead/optimized-out knockback; deliberately NOT reconstructed)
	EMIT_SOUND_DYN(ENT(pSwinger->pev), CHAN_BODY, "player/kungfuhit.wav", VOL_NORM, 0.8f, 0, 100);
	pBlocker->m_flWhiffLungeForce = 0.0f;
	pSwinger->m_flWhiffLungeForce = 0.0f;
	pSwinger->m_flNextAttack = 0.5f;   // the (post-swap) swinger is stunned
	pBlocker->m_flNextAttack = 0.25f;
	pSwinger->UpdateMeleeBar(33.0f);   // the post-swap swinger pays
	return true;
}

// @0x66c4c GetAccelBySlots — returns the player's free-slots count (the original reads the
// cached int at player+0x548). DEAD in the .so; the live twin is GetSpeedBySlots. Ours derives
// the value from the arsenal instead of a maintained cache (documented divergence).
int GetAccelBySlots(CBasePlayer *pPlayer)
{
	return TSPlayerFreeSlots(pPlayer);
}

// @0x76b48 DoSpectacularNess — roll RANDOM_LONG(0,100) against the `spectacularness` cvar
// (clamped 0..100, rounded; 0 disables, comparison INCLUSIVE). Callers land with the
// TSExplosion/PlayerKilled passes.
bool DoSpectacularNess(void)
{
	float v = CVAR_GET_FLOAT("spectacularness");
	if (v < 0.0f)
		v = 0.0f;
	if (v > 100.0f)
		v = 100.0f;
	const int r = (int)(v + 0.5f);
	return r != 0 && RANDOM_LONG(0, 100) <= r;
}
