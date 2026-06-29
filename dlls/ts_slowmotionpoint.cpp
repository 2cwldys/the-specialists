/***
*  ts_slowmotionpoint.cpp — RECONSTRUCTED CTSSlowMotionPoint (the slow-motion "bubble").
*  Functional/source-faithful (see CLAUDE.md). Recovered from ts_i386.so:
*  CTSSlowMotionPoint (CreateSlowMotionPoint @0x69c60 / ActivateSlow @0x663d0 /
*  WaitThink @0x71728 / DeactivateSlow).
*
*  A runtime-created point that slows every player with LINE OF SIGHT to it (GoSlow) for a
*  duration, then restores them. This is the deferred SlowMotionFlow link (cont.14/15): an
*  activated slow-motion powerup spawns one of these at the player, so nearby players actually
*  drop into slow-motion. Built on the cont.15 CBasePlayer::GoSlow/InSlow state.
*  // SLICE: the visible time-dilation MOVEMENT is still client-side (GoSlow sets server
*  fields only); we skip the owner (the activator stays "fast", the classic bullet-time feel).
***/
#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "player.h"

#include "ts_slowmotionpoint.h"

LINK_ENTITY_TO_CLASS(ts_slowmotionpoint, CTSSlowMotionPoint);

void CTSSlowMotionPoint::Spawn()
{
	pev->movetype = MOVETYPE_NONE;
	pev->solid = SOLID_NOT;
	pev->effects |= EF_NODRAW;
	UTIL_SetSize(pev, g_vecZero, g_vecZero);
	UTIL_SetOrigin(pev, pev->origin);
}

// @ActivateSlow — arm the bubble: slow factor + lifetime, then start polling.
void CTSSlowMotionPoint::ActivateSlow(float scale, float duration, int slowBullets)
{
	m_flScale = scale;
	m_flEndTime = gpGlobals->time + duration;
	m_iSlowBullets = slowBullets;
	SetThink(&CTSSlowMotionPoint::WaitThink);
	pev->nextthink = gpGlobals->time + 0.1f;
}

// @WaitThink — every 0.5s, GoSlow each living player (not yet slowed) that has line of sight
// to the point; when the lifetime ends, restore them all and remove.
void CTSSlowMotionPoint::WaitThink()
{
	pev->nextthink = gpGlobals->time + 0.5f;

	if (gpGlobals->time > m_flEndTime)
	{
		DeactivateSlow();
		UTIL_Remove(this);
		return;
	}

	for (int i = 1; i <= gpGlobals->maxClients; i++)
	{
		CBasePlayer *p = (CBasePlayer *)UTIL_PlayerByIndex(i);
		if (!p || !p->IsAlive() || p->edict() == pev->owner)
			continue;
		if (p->InSlow()) // already in slow-motion (by us or another source)
			continue;

		TraceResult tr;
		UTIL_TraceLine(pev->origin, p->pev->origin + p->pev->view_ofs,
			ignore_monsters, ignore_glass, ENT(pev), &tr);
		if (tr.flFraction < 1.0f)
			continue; // no line of sight

		p->GoSlow(m_flScale, m_iSlowBullets);
		if (m_nSlowed < MAX_SLOWED)
			m_hSlowed[m_nSlowed++] = p;
	}
}

// @DeactivateSlow — restore everyone this bubble slowed to normal speed.
void CTSSlowMotionPoint::DeactivateSlow()
{
	for (int i = 0; i < m_nSlowed; i++)
	{
		CBasePlayer *p = (CBasePlayer *)(CBaseEntity *)m_hSlowed[i];
		if (p)
			p->GoSlow(1.0f, 0);
	}
	m_nSlowed = 0;
}

// @CreateSlowMotionPoint — spawn a slow-motion bubble at `origin`.
CTSSlowMotionPoint *CTSSlowMotionPoint::CreateSlowMotionPoint(const Vector &origin, float scale, float duration,
	int slowBullets, CBaseEntity *pOwner)
{
	CTSSlowMotionPoint *pPoint = GetClassPtr((CTSSlowMotionPoint *)NULL);
	pPoint->pev->classname = MAKE_STRING("ts_slowmotionpoint");
	UTIL_SetOrigin(pPoint->pev, origin);
	pPoint->Spawn();
	if (pOwner)
		pPoint->pev->owner = pOwner->edict();
	pPoint->ActivateSlow(scale, duration, slowBullets);
	return pPoint;
}

// Spawn a slow-motion bubble (used by the slow-motion powerup activation).
CBaseEntity *TSCreateSlowMotionPoint(const Vector &origin, float scale, float duration,
	int slowBullets, CBaseEntity *pOwner)
{
	return CTSSlowMotionPoint::CreateSlowMotionPoint(origin, scale, duration, slowBullets, pOwner);
}
