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
#include "game.h"         // the cvars
#include <string.h>

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

// @0x66ad8 WeaponRestrictionByID — the weaponrestriction command's preset masks:
// 1 -> nothing (0x0), 2 -> knives (0x1), 3 -> knives+pistols (0x3), 4 -> +rifles (0x13),
// anything else -> everything (InitWeaponRestriction).
// Mode-decision globals (@.data 0x1a6c/0x12cc/0x152c GOT disps): g_gamemode is a ONE-SHOT
// override (1 DM / 2 Team / 3 LMS / 4 TheOne), consumed (zeroed) by EVERY DecideGamePlay call;
// the two flags carry LMS/TheOne into the rules.
int g_gamemode = 0;
int g_theOneMode = 0;
int g_lastManStanding = 0;

// @0x66244 TSSelectSpawnSpot(class, player, fallback) — TS's spawn scorer (decompile-exact).
// Collects up to 32 IsTriggered spots of `class` (or `fallback` when none of `class` exist) and
// scores each against every OTHER live, visible player:
//  - a TEAMMATE who can SEE the spot (glass-ignoring trace) from >= 128u away puts it in the TOP
//    band (random 0x1801..0x2000 — spawn near covering teammates);
//  - a spot NOBODY can see scores the middle band (random 0x1001..0x1800);
//  - a spot seen by an enemy (or a too-close teammate) scores its distance to the NEAREST player
//    (init 4096) — the low band, maximizing separation.
// Highest score wins. If even the winner scored < 128 (someone visible within touch range), every
// live player inside a 128u sphere of it is TELEFRAGGED (300 dmg, DMG_NEVERGIB, world attacker).
// No spots at all -> info_player_deathmatch -> info_player_start -> NULL. Sets g_pLastSpawn.
// (The original scores into a 33-slot array at [i+1] — the off-by-one kept as plain indexing.)
edict_t *TSSelectSpawnSpot(const char *szClass, CBaseEntity *pPlayer, const char *szFallback)
{
	char szSpot[32];
	strncpy(szSpot, szClass, sizeof(szSpot) - 1);
	szSpot[sizeof(szSpot) - 1] = 0;
	if (szFallback && !UTIL_FindEntityByClassname(NULL, szClass))
	{
		strncpy(szSpot, szFallback, sizeof(szSpot) - 1);
		szSpot[sizeof(szSpot) - 1] = 0;
	}

	CBaseEntity *spots[32] = {};
	int scores[33] = {};
	int n = 0;
	CBaseEntity *pSpot = NULL;
	while (n < 32)
	{
		pSpot = UTIL_FindEntityByClassname(pSpot, szSpot);
		if (!pSpot)
			break;
		if (!pSpot->IsTriggered(pPlayer))
			continue;

		bool unseen = true;
		bool teammateFar = false;
		int minDist = 0x1000;
		CBaseEntity *pOther = NULL;
		while ((pOther = UTIL_FindEntityByClassname(pOther, "player")) != NULL)
		{
			if (pOther == pPlayer || !pOther->IsPlayer() || !pOther->IsAlive() ||
				(pOther->pev->effects & EF_NODRAW) != 0)
				continue;
			if (unseen)
			{
				Vector vecEye = pOther->pev->origin + pOther->pev->view_ofs;
				TraceResult tr;
				UTIL_TraceLine(pSpot->pev->origin, vecEye, ignore_monsters, ignore_glass,
					ENT(pSpot->pev), &tr);
				if (tr.flFraction == 1.0f)
				{
					if (g_pGameRules->PlayerRelationship(pPlayer, pOther) == GR_TEAMMATE &&
						(pSpot->pev->origin - pOther->pev->origin).Length() >= 128.0f)
					{
						teammateFar = true; // a covering teammate sees it from range
					}
					else
					{
						unseen = false; // an enemy (or a crowding teammate) sees it
					}
				}
			}
			const int dist = (int)((pSpot->pev->origin - pOther->pev->origin).Length() + 0.5f);
			if (dist < minDist)
				minDist = dist;
		}

		spots[n] = pSpot;
		n++;
		if (teammateFar && unseen)
			scores[n] = RANDOM_LONG(0x1801, 0x2000);
		else if (unseen)
			scores[n] = RANDOM_LONG(0x1001, 0x1800);
		else
			scores[n] = minDist;
	}

	if (n < 1)
	{
		CBaseEntity *pFall = UTIL_FindEntityByClassname(NULL, "info_player_deathmatch");
		if (!pFall)
			pFall = UTIL_FindEntityByClassname(NULL, "info_player_start");
		return pFall ? ENT(pFall->pev) : NULL;
	}

	int iBest = 0, best = 0;
	for (int i = 0; i < n; i++)
	{
		if (scores[i + 1] > best)
		{
			best = scores[i + 1];
			iBest = i;
		}
	}
	CBaseEntity *pWin = spots[iBest];

	if (scores[iBest + 1] < 0x80)
	{
		// even the best spot is CROWDED — telefrag whoever stands there (world attacker)
		CBaseEntity *pKill = NULL;
		while ((pKill = UTIL_FindEntityInSphere(pKill, pWin->pev->origin, 128.0f)) != NULL)
		{
			if (pKill->IsPlayer() && pKill != pPlayer && pKill->IsAlive() &&
				(pKill->pev->effects & EF_NODRAW) == 0)
				pKill->TakeDamage(VARS(INDEXENT(0)), VARS(INDEXENT(0)), 300.0f, DMG_NEVERGIB);
		}
	}

	g_pLastSpawn = pWin;
	return ENT(pWin->pev);
}

// @0x65ff4 SetPlayerRespawnTime(player, seconds, bool) — EVERY respawn-wait path funnels here
// (InitHUD join / Killed death / ChangePlayerTeam(5s)). The bool arg is dead in the original.
// Plain modes: the gate (+0x770) = now + seconds. LAST-MAN-STANDING (mode 0 + lms): respawns
// are BUDGETED — past max(respawnnumber's value, 1) the gate goes NEGATIVE (-1.0 = out for the
// round); respawnnumber is never registered, so its value field stays 0 and the budget is 1; otherwise the wait clamps to the round's remaining time and the re-entry HOLD
// (+0x7bc) arms at min(roundLeft, 10)s. Always stamps the broadcast award byte: 1 out-of-round
// / 11 respawning (the TSPAward producers).
void SetPlayerRespawnTime(CBasePlayer *pPlayer, int sec, bool bUnused)
{
	(void)bUnused;
	CTSGameRules *pRules = TSGameRules; // @0x6600a: the rules object (TSGameRules)
	const int mode = pRules->m_iGameMode;
	if (mode != 0 || g_lastManStanding == 0) // @0x66004: modes 2/3, non-LMS, or any non-0 mode
	{
		pPlayer->m_flTSRespawnTime = gpGlobals->time + (float)sec;
	}
	else
	{
		float maxRespawns = respawnnumber.value;
		if (maxRespawns < 1.0f)
			maxRespawns = 1.0f;
		if (maxRespawns <= (float)pPlayer->m_iRespawnsUsed)
		{
			pPlayer->m_flTSRespawnTime = -1.0f; // out of respawns this round (@0x660dc)
		}
		else
		{
			const float roundLeft = (pRules ? pRules->m_flRoundEnd : 0.0f) - gpGlobals->time;
			float wait = (float)sec;
			if (roundLeft < wait)
				wait = roundLeft;
			pPlayer->m_flTSRespawnTime = gpGlobals->time + wait;
			float hold = roundLeft;
			if (hold > 10.0f)
				hold = 10.0f;
			pPlayer->m_flRespawnHoldTime = gpGlobals->time + hold;
		}
	}
	pPlayer->m_iAward = (pPlayer->m_flTSRespawnTime < 0.0f) ? 1 : 11;
}

// @0x667ac DecideGamePlay — returns the TEAMPLAY decision (1 = team-style rules, 0 = not) and
// sets the mode flags (disasm-exact: the jumptable cases resolve 1->0, 2->1, 3->LMS+0,
// 4->TheOne+1, out-of-range->0; the cvar path prioritizes mp_theonemode > lastmanstanding >
// teamplay > the caller default). The 3-char mapname copy is DEAD in the original — kept.
int DecideGamePlay(char *mapname, int def)
{
	g_theOneMode = 0;
	g_lastManStanding = 0;
	int ret = def;
	if (g_gamemode >= 1)
	{
		switch (g_gamemode)
		{
		case 1: ret = 0; break;                        // deathmatch
		case 2: ret = 1; break;                        // teamplay
		case 3: g_lastManStanding = 1; ret = 0; break; // last man standing
		case 4: g_theOneMode = 1; ret = 1; break;      // The One
		default: ret = 0; break;                       // out of range
		}
	}
	else
	{
		char buf[68]; // the original's dead mapname prefix copy (source parity)
		buf[0] = 0;
		strncpy(buf, mapname, 3);
		buf[3] = 0;
		if (mp_theonemode.value != 0.0f)
		{
			g_theOneMode = 1;
			ret = 1;
		}
		else if (lastmanstanding.value != 0.0f)
		{
			g_lastManStanding = 1;
			ret = 0;
		}
		else if (teamplay.value != 0.0f) // mp_teamplay (@0x668a4)
			ret = 1;
		// else: keep the caller's default
	}
	g_gamemode = 0; // the override is one-shot
	return ret;
}

int WeaponRestrictionByID(int id)
{
	switch (id)
	{
	case 1: return 0x0;
	case 2: return 0x1;
	case 3: return 0x3;
	case 4: return 0x13;
	default: return InitWeaponRestriction();
	}
}

// ---- melee deflect gates (RE dossier-melee.md §1.3 + analysis_ff_gamerules.md §6-8) ----
// Each returns true = the swing was BLOCKED (the caller aborts its damage). Getting blocked
// drains the ATTACKER 33 stamina (UpdateMeleeBar handles depletion + the KFuPower send) and
// stamps asymmetric next-attack locks. The block windows are the PreThink-decayed VALUES on
// CBasePlayer (+0x7a8 kungfu / +0x7ac knife / +0x7b0 katana).
// The block sounds pass EMIT_SOUND_DYN's default slow-motion weight, 0.5 (@0x66d7d, @0x66ee8).

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
