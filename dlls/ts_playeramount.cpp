/***
*  ts_playeramount.cpp — RECONSTRUCTED CTSPlayerAmount (cont.44), the player-count relay.
*  Functional/source-faithful (see CLAUDE.md). recovered/miscCTSPlayerAmount/CTSPlayerAmount.c:
*    Spawn @0x72a78, KeyValue @0x729c0, ClientConnect @0x72aec, ClientDisconnect.
*
*  CTSPlayerAmount is a map-placed point relay (`@PointClass`, ts_playeramount): when the
*  player count rises to `clients` it fires `connecttarget`; when it falls back to `clients` it
*  fires `disconnecttarget`. The .so drives it off the engine connect/disconnect events
*  (CTSPlayerAmount::ClientConnect/Disconnect bump a counter and fire on `== clients`); we use a
*  poll-based equivalent (a Think that watches the live count and fires on the same edges) so it
*  needs no engine hook. FGD-backed; 0 stock placements → synthetic-verified.
***/
#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "player.h"

class CTSPlayerAmount : public CBaseEntity
{
public:
	bool KeyValue(KeyValueData *pkvd) override;
	void Spawn() override;
	void EXPORT CountThink();

	int CountConnectedPlayers() const;

	int      m_iClients = 0;          // +0x70 — fire when the count reaches this
	string_t m_iszConnectTarget = 0;  // +0x74 — fired when the count rises to m_iClients
	string_t m_iszDisconnectTarget = 0; // +0xd8 — fired when the count falls to m_iClients
	int      m_iPrevCount = 0;        // last polled count (edge detection)
};

LINK_ENTITY_TO_CLASS(ts_playeramount, CTSPlayerAmount)

bool CTSPlayerAmount::KeyValue(KeyValueData *pkvd)
{
	if (FStrEq(pkvd->szKeyName, "clients"))          { m_iClients = atoi(pkvd->szValue); return true; }
	if (FStrEq(pkvd->szKeyName, "connecttarget"))    { m_iszConnectTarget = ALLOC_STRING(pkvd->szValue); return true; }
	if (FStrEq(pkvd->szKeyName, "disconnecttarget")) { m_iszDisconnectTarget = ALLOC_STRING(pkvd->szValue); return true; }
	return CBaseEntity::KeyValue(pkvd);
}

void CTSPlayerAmount::Spawn()
{
	pev->movetype = MOVETYPE_NONE;
	pev->solid = SOLID_NOT;
	pev->effects |= EF_NODRAW;

	m_iPrevCount = CountConnectedPlayers();
	SetThink(&CTSPlayerAmount::CountThink);
	pev->nextthink = gpGlobals->time + 0.5f;
}

int CTSPlayerAmount::CountConnectedPlayers() const
{
	int n = 0;
	for (int i = 1; i <= gpGlobals->maxClients; i++)
		if (UTIL_PlayerByIndex(i) != nullptr)
			n++;
	return n;
}

// @ ClientConnect/ClientDisconnect — poll equivalent: fire connecttarget when the count rises to
// `clients`, disconnecttarget when it falls back to `clients` (the .so fires on `count == clients`
// after each connect/disconnect).
void CTSPlayerAmount::CountThink()
{
	pev->nextthink = gpGlobals->time + 0.5f;

	int count = CountConnectedPlayers();
	if (count != m_iPrevCount)
	{
		if (count > m_iPrevCount && count == m_iClients && !FStringNull(m_iszConnectTarget))
			FireTargets(STRING(m_iszConnectTarget), this, this, USE_TOGGLE, 0);
		else if (count < m_iPrevCount && count == m_iClients && !FStringNull(m_iszDisconnectTarget))
			FireTargets(STRING(m_iszDisconnectTarget), this, this, USE_TOGGLE, 0);
		m_iPrevCount = count;
	}
}

