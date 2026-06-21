/***
*  ts_dmhill.cpp — RECONSTRUCTED CTSDMHill (king-of-the-hill brush volume).
*  Functional/source-faithful (see CLAUDE.md). Recovered from ts_i386.so:
*  CTSDMHill (Spawn @0x71a8c / KeyValue @0x719b8 / WaitTouch @0x71be8 / WaitThink).
*
*  A brush a player must hold ALONE for `nextcheck` seconds to be crowned "king of the hill"
*  and awarded `fragamt` frags (`fragamtteam` in team modes), with a "<name> <message>"
*  broadcast. The original drives this off WaitTouch tracking the holder; this functional
*  reconstruction polls the volume each think (clean + contest-aware): exactly one living
*  player inside earns the award on the timer; 0 or 2+ inside resets the king.
*  // SLICE: the .so's team-contest nuance (same-team holders don't reset) is simplified to
*  "sole occupant"; no per-map test (ts_dmhill has 0 stock placements — verified synthetically).
***/
#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "player.h"
#include "gamerules.h"
#include "ts_gamerules.h"
#include "ts_utils.h"       // TSHudMessageAll
#include "UserMessages.h"   // gmsgScoreInfo

class CTSDMHill : public CBaseEntity
{
public:
	void Spawn() override;
	bool KeyValue(KeyValueData *pkvd) override;
	void EXPORT HillThink();
	int ObjectCaps() override { return CBaseEntity::ObjectCaps() & ~FCAP_ACROSS_TRANSITION; }

	void Award(CBasePlayer *pKing); // crown + frag the holder

	float m_flNextCheck = 10.0f; // nextcheck (seconds to hold)
	int   m_iFragAmt = 5;        // fragamt (DM)
	int   m_iFragAmtTeam = 1;    // fragamtteam (team DM)
	EHANDLE m_hKing;             // current sole holder
	float m_flAwardTime = 0;     // when the held hill next pays out
};

LINK_ENTITY_TO_CLASS(ts_dmhill, CTSDMHill);

bool CTSDMHill::KeyValue(KeyValueData *pkvd)
{
	if (FStrEq(pkvd->szKeyName, "nextcheck"))
	{
		m_flNextCheck = atof(pkvd->szValue);
		if (m_flNextCheck < 0.5f)
			m_flNextCheck = 0.5f;
		return true;
	}
	if (FStrEq(pkvd->szKeyName, "fragamt"))     { m_iFragAmt = atoi(pkvd->szValue); return true; }
	if (FStrEq(pkvd->szKeyName, "fragamtteam")) { m_iFragAmtTeam = atoi(pkvd->szValue); return true; }
	return CBaseEntity::KeyValue(pkvd);
}

// @Spawn__9CTSDMHill — a (usually brush) trigger volume; polls for a sole holder.
void CTSDMHill::Spawn()
{
	if (!FStringNull(pev->model))
		SET_MODEL(ENT(pev), STRING(pev->model)); // the brush, from the map
	else
		UTIL_SetSize(pev, Vector(-48, -48, -36), Vector(48, 48, 36)); // synthetic/point fallback
	pev->movetype = MOVETYPE_NONE;
	pev->solid = SOLID_TRIGGER; // non-blocking; keeps absmin/absmax for Intersects
	UTIL_SetOrigin(pev, pev->origin);

	SetThink(&CTSDMHill::HillThink);
	pev->nextthink = gpGlobals->time + 0.5f;
}

// @WaitThink/WaitTouch — exactly one living player in the volume holds it; once held past the
// timer, pay out + re-arm. 0 or 2+ inside (contested/empty) drops the king.
void CTSDMHill::HillThink()
{
	pev->nextthink = gpGlobals->time + 0.5f;

	CBasePlayer *pInside = nullptr;
	int count = 0;
	for (int i = 1; i <= gpGlobals->maxClients; i++)
	{
		CBasePlayer *p = (CBasePlayer *)UTIL_PlayerByIndex(i);
		if (!p || !p->IsAlive())
			continue;
		if (Intersects(p))
		{
			count++;
			pInside = p;
		}
	}

	if (count != 1)
	{
		m_hKing = nullptr; // empty or contested
		return;
	}

	if ((CBaseEntity *)m_hKing == pInside)
	{
		if (gpGlobals->time >= m_flAwardTime)
		{
			Award(pInside);
			m_flAwardTime = gpGlobals->time + m_flNextCheck;
		}
	}
	else // a new sole holder — start their hold timer
	{
		m_hKing = pInside;
		m_flAwardTime = gpGlobals->time + m_flNextCheck;
	}
}

// Crown the holder: add frags, update the scoreboard, announce.
void CTSDMHill::Award(CBasePlayer *pKing)
{
	const bool team = g_pGameRules && g_pGameRules->IsTeamplay();
	pKing->pev->frags += team ? m_iFragAmtTeam : m_iFragAmt;

	MESSAGE_BEGIN(MSG_ALL, gmsgScoreInfo);
	WRITE_BYTE(pKing->entindex());
	WRITE_SHORT((int)pKing->pev->frags);
	WRITE_SHORT(pKing->m_iDeaths);
	WRITE_SHORT(0);
	WRITE_SHORT(g_pGameRules->GetTeamIndex(pKing->m_szTeamName) + 1);
	MESSAGE_END();

	if (!FStringNull(pev->message))
		TSHudMessageAll(0, 0xff, 0, 8, 30,
			UTIL_VarArgs("%s %s", STRING(pKing->pev->netname), STRING(pev->message)));
}
