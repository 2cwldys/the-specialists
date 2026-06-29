/***
*  ts_dmhill.cpp — RECONSTRUCTED CTSDMHill (king-of-the-hill brush volume).
*  Functional/source-faithful (see CLAUDE.md). Recovered from ts_i386.so:
*  CTSDMHill (Save @0x71918 / Restore / KeyValue @0x719b8 / Spawn @0x71a8c / WaitTouch @0x71be8 /
*  WaitThink @0x72280 / ObjectCaps @0x73b20).
*
*  A brush a player must HOLD to be crowned "king of the hill" and awarded `fragamt` frags (the whole
*  team gets `fragamtteam` in team modes) every `nextcheck` seconds, with a "<name> <message>" broadcast.
*  The original is TOUCH-driven (WaitTouch tracks the current holder): the first toucher claims it; a
*  same-team toucher keeps it; an enemy toucher resets it; the holder is paid out once they've held past
*  the award time. WaitThink just keeps the entity alive (reschedules).
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
	bool Save(CSave &save) override;
	bool Restore(CRestore &restore) override;
	int ObjectCaps() override { return 0; } // @0x73b20 — no caps (not save/use-able as an objective)
	void EXPORT WaitTouch(CBaseEntity *pOther); // @0x71be8 — holder tracking + payout
	void EXPORT WaitThink();                    // @0x72280 — idle keepalive (reschedule)

	static TYPEDESCRIPTION m_SaveData[];

	float nextcheck = 10.0f;        // +? — keyvalue "nextcheck": seconds to hold per payout (>=0.5)
	int   fragseverycheck = 5;      // keyvalue "fragamt": frags per payout (solo/DM)
	int   fragseverycheckteam = 1;  // keyvalue "fragamtteam": frags per payout to each teammate (team DM)
	EHANDLE m_hKing;                // +0x6c — the current holder
	float m_flAwardTime = 0;        // +0x7c — when the held hill next pays out
	float m_flLastTouch = 0;        // +0x68 — the holder's last-touch time (drop if it lapses)
	float m_flStartGate = 0;        // +0x64 — startup grace: ignore touches until this time
};

LINK_ENTITY_TO_CLASS(ts_dmhill, CTSDMHill);

TYPEDESCRIPTION CTSDMHill::m_SaveData[] = {
	DEFINE_FIELD(CTSDMHill, nextcheck, FIELD_FLOAT),
	DEFINE_FIELD(CTSDMHill, fragseverycheck, FIELD_INTEGER),
	DEFINE_FIELD(CTSDMHill, fragseverycheckteam, FIELD_INTEGER),
};
IMPLEMENT_SAVERESTORE(CTSDMHill, CBaseEntity);

bool CTSDMHill::KeyValue(KeyValueData *pkvd)
{
	if (FStrEq(pkvd->szKeyName, "nextcheck"))
	{
		nextcheck = atof(pkvd->szValue);
		return true;
	}
	if (FStrEq(pkvd->szKeyName, "fragamt"))     { fragseverycheck = atoi(pkvd->szValue); return true; }
	if (FStrEq(pkvd->szKeyName, "fragamtteam")) { fragseverycheckteam = atoi(pkvd->szValue); return true; }
	return CBaseEntity::KeyValue(pkvd);
}

// @Spawn__9CTSDMHill — a (usually brush) trigger volume. Only arms the touch handler if the relevant
// frag amount is configured for the active mode (DM: fragamt>0; team: fragamtteam>0).
void CTSDMHill::Spawn()
{
	if (!FStringNull(pev->model))
		SET_MODEL(ENT(pev), STRING(pev->model)); // the brush, from the map
	else
		UTIL_SetSize(pev, Vector(-48, -48, -36), Vector(48, 48, 36)); // synthetic/point fallback
	pev->effects |= EF_NODRAW;
	pev->solid = SOLID_TRIGGER; // non-blocking trigger volume
	pev->movetype = MOVETYPE_NONE;
	UTIL_SetOrigin(pev, pev->origin);

	const bool team = TSGameRules() && TSGameRules()->IsTeamMode();
	if ((!team || fragseverycheckteam > 0) && (!team || fragseverycheck > 0))
		SetTouch(&CTSDMHill::WaitTouch);
	else
		SetTouch(NULL);

	SetThink(&CTSDMHill::WaitThink);
	float check = (nextcheck < 0.5f) ? 0.5f : nextcheck;
	pev->nextthink = gpGlobals->time + check;
	m_flStartGate = gpGlobals->time + 1.0f; // 1s startup grace
	m_hKing = NULL;
}

// @WaitThink — keep the entity thinking (the holder logic is all in WaitTouch).
void CTSDMHill::WaitThink()
{
	float check = (nextcheck < 0.5f) ? 0.5f : nextcheck;
	pev->nextthink = gpGlobals->time + check;
}

// @WaitTouch — a living player overlapping the volume. Track the holder; pay out once they've held
// past the award time. A same-team toucher keeps the hold; an enemy toucher drops it.
void CTSDMHill::WaitTouch(CBaseEntity *pOther)
{
	if (gpGlobals->time < m_flStartGate)
		return;
	if (!pOther || !pOther->IsPlayer() || !pOther->IsAlive())
		return;
	CBasePlayer *pPlayer = (CBasePlayer *)pOther;
	const float check = (nextcheck < 0.5f) ? 0.5f : nextcheck;

	// No holder yet — this toucher claims it and starts the hold timer.
	if (!((CBaseEntity *)m_hKing))
	{
		m_hKing = pPlayer;
		m_flStartGate = 0;
		m_flAwardTime = gpGlobals->time + check;
		m_flLastTouch = gpGlobals->time + check;
		return;
	}

	if ((CBaseEntity *)m_hKing == pPlayer)
	{
		// the holder is still here — drop them only if their touches lapsed past a full check window
		if (check < gpGlobals->time - m_flLastTouch)
		{
			m_hKing = NULL;
			m_flLastTouch = gpGlobals->time + check;
			return;
		}
	}
	else
	{
		// a DIFFERENT player touched — a teammate keeps the hold, an enemy steals/resets it
		CBaseEntity *pKing = m_hKing;
		bool sameTeam = false;
		if (pKing && TSGameRules() && TSGameRules()->IsTeamMode())
			sameTeam = TSGameRules()->GetTeamIndex(((CBasePlayer *)pKing)->TeamID()) ==
				TSGameRules()->GetTeamIndex(pPlayer->TeamID());
		if (!sameTeam)
		{
			m_hKing = NULL;
			m_flLastTouch = gpGlobals->time + check;
			return;
		}
	}

	if (gpGlobals->time <= m_flAwardTime)
		return; // held, but not long enough yet

	// --- PAYOUT ---
	const bool team = TSGameRules() && TSGameRules()->IsTeamMode();
	char kingMsg[128] = {0};
	if (!FStringNull(pev->message))
	{
		if (team)
		{
			strcpy(kingMsg, "Team ");
			strcat(kingMsg, pPlayer->TeamID());
		}
		else
		{
			strncpy(kingMsg, STRING(pPlayer->pev->netname), sizeof(kingMsg) - 1);
		}
		strcat(kingMsg, " ");
		strncat(kingMsg, STRING(pev->message), sizeof(kingMsg) - strlen(kingMsg) - 1);
	}

	if (!team)
	{
		pPlayer->pev->frags += fragseverycheck;
		MESSAGE_BEGIN(MSG_ALL, gmsgScoreInfo);
		WRITE_BYTE(pPlayer->entindex());
		WRITE_SHORT((int)pPlayer->pev->frags);
		WRITE_SHORT(pPlayer->m_iDeaths);
		WRITE_SHORT(0);
		WRITE_SHORT(g_pGameRules->GetTeamIndex(pPlayer->m_szTeamName) + 1);
		MESSAGE_END();
		if (!FStringNull(pev->message))
			strncat(kingMsg, UTIL_VarArgs(" ,%i frags for him", fragseverycheck),
				sizeof(kingMsg) - strlen(kingMsg) - 1);
	}
	else
	{
		// award every player on the holder's team
		CBaseEntity *pEnt = NULL;
		while ((pEnt = UTIL_FindEntityByClassname(pEnt, "player")) != NULL)
		{
			if (!pEnt->IsPlayer())
				continue;
			CBasePlayer *p = (CBasePlayer *)pEnt;
			if (strcmp(p->TeamID(), pPlayer->TeamID()) != 0)
				continue;
			p->pev->frags += fragseverycheckteam;
			MESSAGE_BEGIN(MSG_ALL, gmsgScoreInfo);
			WRITE_BYTE(p->entindex());
			WRITE_SHORT((int)p->pev->frags);
			WRITE_SHORT(p->m_iDeaths);
			WRITE_SHORT(0);
			WRITE_SHORT(g_pGameRules->GetTeamIndex(p->m_szTeamName) + 1);
			MESSAGE_END();
		}
		if (!FStringNull(pev->message))
			strncat(kingMsg, UTIL_VarArgs(" ,%i frags for they", fragseverycheckteam),
				sizeof(kingMsg) - strlen(kingMsg) - 1);
	}

	if (!FStringNull(pev->message))
	{
		float dur = check * 10.0f;
		if (dur > 30.0f) dur = 30.0f;
		TSHudMessageAll(0, 0xff, 0, 8, (int)(dur + 0.5f), kingMsg);
	}

	m_flAwardTime = gpGlobals->time + check;
	if (!FStringNull(pev->target))
		FireTargets(STRING(pev->target), this, this, USE_TOGGLE, 0);
	m_flLastTouch = gpGlobals->time;
}
