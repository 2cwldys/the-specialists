/***
*  ts_playeramount.cpp — RECONSTRUCTED CTSPlayerAmount, the player-count relay (TS_entities.cpp).
*  Functional/source-faithful (see CLAUDE.md). ts_i386.so:
*    Spawn @0x72a78, KeyValue @0x729c0, ClientConnect @0x72aec, ClientDisconnect @0x72c08.
*
*  A map-placed point relay (`@PointClass`, ts_playeramount): the engine's client connect/
*  disconnect interface hooks (client.cpp) call CTSPlayerAmount::ClientConnect/ClientDisconnect on
*  every placed relay, which bump a per-entity counter; when it reaches `clients` the relay fires
*  connecttarget (on a rise) or disconnecttarget (on a fall). `delay` (CBaseDelay) + a "remove after
*  firing" spawnflag (bit 1) come from the original. FGD-backed; 0 stock placements.
***/
#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "player.h"
#include "ts_playeramount.h"

LINK_ENTITY_TO_CLASS(ts_playeramount, CTSPlayerAmount)

bool CTSPlayerAmount::KeyValue(KeyValueData *pkvd)
{
	if (FStrEq(pkvd->szKeyName, "clients"))          { m_iClients = atoi(pkvd->szValue); return true; }
	if (FStrEq(pkvd->szKeyName, "connecttarget"))    { m_iszConnectTarget = ALLOC_STRING(pkvd->szValue); return true; }
	if (FStrEq(pkvd->szKeyName, "disconnecttarget")) { m_iszDisconnectTarget = ALLOC_STRING(pkvd->szValue); return true; }
	return CBaseDelay::KeyValue(pkvd); // "delay" (m_flDelay) etc. handled by the base
}

void CTSPlayerAmount::Spawn()
{
	pev->classname = MAKE_STRING("ts_playeramount");
	pev->effects |= EF_NODRAW;
	pev->solid = SOLID_NOT;
	pev->movetype = MOVETYPE_FLY; // (original sets movetype 5; a non-colliding point relay)
	m_iCount = 0;
}

// @0x72aec ClientConnect — a client connected: count up; on reaching `clients`, fire connecttarget.
void CTSPlayerAmount::ClientConnect()
{
	if (++m_iCount != m_iClients)
		return;
	if (!FStringNull(m_iszConnectTarget))
	{
		// delay==0 fires now; the original schedules a DelayedUse for m_flDelay>0 (deferred refinement).
		FireTargets(STRING(m_iszConnectTarget), this, this, USE_TOGGLE, 0);
		if (pev->spawnflags & 1) // bit 1 = remove the relay after it fires
			UTIL_Remove(this);
	}
}

// @0x72c08 ClientDisconnect — a client left: count down; on reaching `clients`, fire disconnecttarget.
void CTSPlayerAmount::ClientDisconnect()
{
	if (--m_iCount != m_iClients)
		return;
	if (!FStringNull(m_iszDisconnectTarget))
	{
		FireTargets(STRING(m_iszDisconnectTarget), this, this, USE_TOGGLE, 0);
		if (pev->spawnflags & 1)
			UTIL_Remove(this);
	}
}
