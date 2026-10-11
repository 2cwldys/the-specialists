//=========== (C) Copyright 1999 Valve, L.L.C. All rights reserved. ===========
//
// The copyright to the contents herein is the property of Valve, L.L.C.
// The contents may be used and/or copied only with the written permission of
// Valve, L.L.C., or in accordance with the terms and conditions stipulated in
// the agreement/contract under which the contents have been supplied.
//
// Purpose: Functionality for the observer chase camera
//
// $Workfile:     $
// $Date:         $
//
//-----------------------------------------------------------------------------
// $Log: $
//
// $NoKeywords: $
//=============================================================================
#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "player.h"
#include "weapons.h"
#include "pm_shared.h"
#include "UserMessages.h"
#include "gamerules.h"
#include "ts_gamerules.h"

// @0x89b0c Observer_FindNextPlayer — the next (or, reversed, the previous) client to watch: alive, in
// play (iuser1 0) and drawn. In the objective modes (2/3) and in last man standing a spectator whose
// respawn time is not below 0 (still in the round) watches only his teammates. The target is put in
// iuser2 and the spectator moved to him in every mode; with nobody to watch he roams.
void CBasePlayer::Observer_FindNextPlayer(bool bReverse)
{
	int iStart;
	if (m_hObserverTarget)
		iStart = ENTINDEX(m_hObserverTarget->edict());
	else
		iStart = ENTINDEX(edict());
	int iCurrent = iStart;
	m_hObserverTarget = NULL;
	int iDir = bReverse ? -1 : 1;

	do
	{
		iCurrent += iDir;

		// Loop through the clients
		if (iCurrent > gpGlobals->maxClients)
			iCurrent = 1;
		if (iCurrent < 1)
			iCurrent = gpGlobals->maxClients;

		CBaseEntity* pEnt = UTIL_PlayerByIndex(iCurrent);
		if (!pEnt)
			continue;
		if (pEnt == this)
			continue;
		if (pEnt->pev->iuser1 != 0 || (pEnt->pev->effects & EF_NODRAW) != 0)
			continue;
		if (!pEnt->IsAlive())
			continue;
		if (((TSGameRules && (unsigned char)(TSGameRules->m_iGameMode - 2) < 2) || g_lastManStanding != 0) &&
			m_flTSRespawnTime >= 0.0f && g_pGameRules->PlayerRelationship(this, pEnt) != GR_TEAMMATE)
			continue;

		m_hObserverTarget = pEnt;
		break;

	} while (iCurrent != iStart);

	if (!m_hObserverTarget)
	{
		Observer_SetMode(OBS_ROAMING);
	}
	else
	{
		pev->iuser2 = ENTINDEX(m_hObserverTarget->edict());
		UTIL_SetOrigin(pev, m_hObserverTarget->pev->origin);
	}
}

// @0x89ca4 Observer_Idle — on its own 0.5 s clock (m_flNextObserverIdle), a spectator who is not
// roaming moves on when his target has died or is spectating himself.
void CBasePlayer::Observer_Idle()
{
	if (m_flNextObserverIdle > gpGlobals->time)
		return;
	m_flNextObserverIdle = gpGlobals->time + 0.5f;

	if (pev->iuser1 == OBS_ROAMING)
		return;
	if (!m_hObserverTarget)
		return;
	if (!m_hObserverTarget->IsAlive() || m_hObserverTarget->pev->iuser1 != 0)
		Observer_FindNextPlayer(false);
}

// @0x89d34 Observer_HandleButtons — read off the held buttons every 0.2 s: jump cycles the mode
// (roaming -> chase -> locked chase -> first person -> roaming), forward and back step to the next and the
// previous player when not roaming.
void CBasePlayer::Observer_HandleButtons()
{
	if (m_flNextObserverInput > gpGlobals->time)
		return;

	if ((pev->button & IN_JUMP) != 0)
	{
		if (pev->iuser1 == OBS_ROAMING)
			Observer_SetMode(OBS_CHASE_FREE);
		else if (pev->iuser1 == OBS_CHASE_FREE)
			Observer_SetMode(OBS_CHASE_LOCKED);
		else if (pev->iuser1 == OBS_CHASE_LOCKED)
			Observer_SetMode(OBS_IN_EYE);
		else
			Observer_SetMode(OBS_ROAMING);

		m_flNextObserverInput = gpGlobals->time + 0.2;
	}

	if ((pev->button & IN_FORWARD) != 0 && pev->iuser1 != OBS_ROAMING)
	{
		Observer_FindNextPlayer(false);

		m_flNextObserverInput = gpGlobals->time + 0.2;
	}

	if ((pev->button & IN_BACK) != 0 && pev->iuser1 != OBS_ROAMING)
	{
		Observer_FindNextPlayer(true);

		m_flNextObserverInput = gpGlobals->time + 0.2;
	}
}

// @0x89e44 Observer_SetMode — roaming clears the target and sets the client's max speed to 420; the
// chase, locked chase and first person modes need a target (found if there is none) and fall back to roaming
// without one. Other modes change nothing past the view reset. Nothing is printed and the last mode
// is not stored here (StopObserver keeps it).
void CBasePlayer::Observer_SetMode(int iMode)
{
	// Just abort if we're changing to the mode we're already in
	if (iMode == pev->iuser1)
		return;

	pev->view_ofs = g_vecZero;
	pev->viewmodel = 0;

	if (iMode == OBS_ROAMING)
	{
		pev->iuser1 = OBS_ROAMING;
		pev->iuser2 = 0;
		g_engfuncs.pfnSetClientMaxspeed(edict(), 420.0f);
		return;
	}

	if (iMode != OBS_IN_EYE && iMode != OBS_CHASE_LOCKED && iMode != OBS_CHASE_FREE)
		return;

	if (m_hObserverTarget == NULL)
		Observer_FindNextPlayer(false);
	if (m_hObserverTarget == NULL)
	{
		Observer_SetMode(OBS_ROAMING);
		return;
	}

	pev->iuser1 = iMode;
	pev->iuser2 = ENTINDEX(m_hObserverTarget->edict());
}
