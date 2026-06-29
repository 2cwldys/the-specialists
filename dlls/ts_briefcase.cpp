/***
*  ts_briefcase.cpp — RECONSTRUCTED CTSBriefcase (original TS_objectives.cpp), the CTF base +
*  flag registry. Functional/source-faithful (see CLAUDE.md). Recovered addresses (nm):
*    ctor @0x7d788, KeyValue @0x7d7f4, Spawn @0x7d88c, FlagFall @0x7da00,
*    WaitThink @0x7db14, CanInteract @0x7db48, CaptureTouch @0x7db9c.
*
*  CTSBriefcase is the map-placed brush objective — one per team. It OWNS the flag: it stamps a
*  template s_flag_status (whose EHANDLE points back to itself) and seeds its team's loose CTSFlag
*  at its centre (FlagFall). It is the capture zone: a player carrying the enemy flag who reaches
*  their OWN briefcase captures it (CaptureTouch → +10 frags + the captured flag respawns at its
*  own home base via that flag's status EHANDLE).
***/
#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "player.h"
#include "gamerules.h"
#include "ts_gamerules.h"
#include "ts_flag.h"        // CTSFlag::CreateFlag, HaveFlag
#include "ts_briefcase.h"
#include "ts_utils.h"        // TSHudMessageAll
#include "UserMessages.h"    // gmsgScoreInfo, gmsgTeamScore

// @0x1779a0 — cycling flag-id source (1..15), bumped per briefcase in the ctor.
int CTSBriefcase::s_nextFlagID = 0;

// gmsgTSHaveFlag (the base-spawn notify type 0) is defined in ts_flag.cpp.
extern int gmsgTSHaveFlag;

// @ gmsgTSVoiceMessage — MSG_ONE(1): a per-player capture taunt (1 = an enemy captured, 0 = yours).
static int gmsgTSVoiceMessage = 0;

LINK_ENTITY_TO_CLASS(ts_briefcase, CTSBriefcase)

// @0x7d788 — each briefcase (and so each flag it seeds) gets a unique cycling id 1..15.
CTSBriefcase::CTSBriefcase()
{
	if (++s_nextFlagID > 15)
		s_nextFlagID = 1;
	m_iFlagID = s_nextFlagID;
}

// @0x7d7f4 KeyValue — ownerteam (1-based in the FGD; stored 0-based), contents (a char[32] label).
// message/target flow through the base CBaseEntity::KeyValue / pev.
bool CTSBriefcase::KeyValue(KeyValueData *pkvd)
{
	if (FStrEq(pkvd->szKeyName, "ownerteam"))
	{
		m_iOwnerTeam = atoi(pkvd->szValue) - 1;
		return true;
	}
	if (FStrEq(pkvd->szKeyName, "contents"))
	{
		strncpy(m_szContents, pkvd->szValue, sizeof(m_szContents) - 1);
		m_szContents[sizeof(m_szContents) - 1] = '\0';
		return true;
	}
	return CBaseEntity::KeyValue(pkvd);
}

// @0x7d88c Spawn — the (invisible) brush capture trigger; stamp the template status (with the
// EHANDLE pointing back at this briefcase), and schedule the first FlagFall.
void CTSBriefcase::Spawn()
{
	pev->classname = MAKE_STRING("ts_briefcase");
	SET_MODEL(ENT(pev), STRING(pev->model)); // brush model
	UTIL_SetOrigin(pev, pev->origin);
	pev->effects |= EF_NODRAW;
	pev->solid = SOLID_BBOX;
	pev->movetype = MOVETYPE_PUSH;

	if (m_szContents[0] == '\0')
		strcpy(m_szContents, "a flag");

	m_bFlagOut = false;

	// Stamp the template that every seeded flag copies.
	m_flagStatus.active = 0;
	m_flagStatus.contents = MAKE_STRING(m_szContents);
	m_flagStatus.flagID = m_iFlagID;
	m_flagStatus.team = m_iOwnerTeam;
	m_flagStatus.lastCarrier = 0;
	m_flagStatus.hBriefcase = this; // the EHANDLE rides inside the status (respawn target)

	SetTouch(&CTSBriefcase::CaptureTouch);
	SetThink(&CTSBriefcase::FlagFall);
	pev->nextthink = gpGlobals->time + 1.0f;
}

// @0x7da00 FlagFall — seed this team's loose flag at the briefcase centre, then idle.
void CTSBriefcase::FlagFall()
{
	CTSFlag *pFlag = CTSFlag::CreateFlag(NULL, &m_flagStatus);
	if (pFlag)
	{
		pFlag->m_flExpireTime = 0; // a base flag never auto-returns
		UTIL_SetOrigin(pFlag->pev, Center());
	}
	m_bFlagOut = true;

	if (gmsgTSHaveFlag == 0)
		gmsgTSHaveFlag = REG_USER_MSG("TSHaveFlag", 4);
	MESSAGE_BEGIN(MSG_ALL, gmsgTSHaveFlag);
	WRITE_BYTE(0); // type 0 = base spawn
	WRITE_BYTE(m_flagStatus.flagID);
	WRITE_BYTE(m_flagStatus.team + 1);
	WRITE_BYTE(0);
	MESSAGE_END();

	SetThink(&CTSBriefcase::WaitThink);
	pev->nextthink = gpGlobals->time + 1.0f;
}

// @0x7db14 WaitThink — idle keepalive.
void CTSBriefcase::WaitThink()
{
	pev->nextthink = gpGlobals->time + 1.0f;
}

// @0x7db48 CanInteract — neutral briefcase: anyone; team briefcase: only its own team.
bool CTSBriefcase::CanInteract(CBasePlayer *pPlayer)
{
	if (m_iOwnerTeam == -1)
		return true;
	return m_iOwnerTeam == TSGameRules()->GetTeamIndex(pPlayer->TeamID());
}

// @0x7db9c CaptureTouch — a living player carrying the enemy flag who reaches their OWN briefcase
// captures it: the carried flag respawns at its own home base, the capturer scores +10, and the
// scoreboard / team score / taunt are broadcast.
void CTSBriefcase::CaptureTouch(CBaseEntity *pOther)
{
	if (!m_bFlagOut || !pOther || !pOther->IsPlayer() || !pOther->IsAlive())
		return;

	CBasePlayer *pPlayer = (CBasePlayer *)pOther;
	if (!HaveFlag(pPlayer) || !CanInteract(pPlayer))
		return;

	// Respawn the captured flag at ITS home base (via the carried status' briefcase handle).
	CTSBriefcase *pSource = (CTSBriefcase *)(CBaseEntity *)pPlayer->m_flagStatus.hBriefcase;
	if (pSource)
	{
		pSource->SetThink(&CTSBriefcase::FlagFall);
		pSource->pev->nextthink = gpGlobals->time + 0.25f;
	}

	pPlayer->DropFlag(true); // silent: clear the carrier's state, no loose flag

	if (!FStringNull(pev->target))
		FireTargets(STRING(pev->target), pPlayer, this, USE_TOGGLE, 0);

	int team = TSGameRules()->GetTeamIndex(pPlayer->TeamID());

	if (TSGameRules()->CountPlayers() < 2)
	{
		TSHudMessageAll(0xff, 0x80, 0, 4, 20, "Waiting for other players.");
		return; // no scoring solo
	}

	pPlayer->pev->frags += 10;

	const char *who = (pPlayer->pev->netname) ? STRING(pPlayer->pev->netname) : "Someone";
	if (!FStringNull(pev->message))
		TSHudMessageAll(0, 0xff, 0, 4, 30, (char *)STRING(pev->message));
	else
		TSHudMessageAll(0, 0xff, 0, 4, 30,
			UTIL_VarArgs("%s captured the briefcase! %i #TS_Frg", who, (int)pPlayer->pev->frags));

	if (gmsgScoreInfo)
	{
		MESSAGE_BEGIN(MSG_ALL, gmsgScoreInfo);
		WRITE_BYTE(ENTINDEX(pPlayer->edict()));
		WRITE_SHORT((int)pPlayer->pev->frags);
		WRITE_SHORT(pPlayer->m_iDeaths);
		WRITE_SHORT(0);
		WRITE_SHORT(team + 1);
		MESSAGE_END();
	}

	// Team score +1 and broadcast the team scoreboard.
	if (team >= 0 && team < 4)
		TSGameRules()->m_teamScore[team]++;
	if (gmsgTeamScore)
	{
		for (int t = 0; t < TSGameRules()->GetTeamsNum(); t++)
		{
			MESSAGE_BEGIN(MSG_ALL, gmsgTeamScore);
			WRITE_STRING(TSGameRules()->GetIndexedTeamName(t));
			WRITE_SHORT(TSGameRules()->m_teamScore[t]);
			WRITE_SHORT(0);
			MESSAGE_END();
		}
	}

	// Per-player capture taunt (1 = an enemy team captured, 0 = your team).
	if (gmsgTSVoiceMessage == 0)
		gmsgTSVoiceMessage = REG_USER_MSG("TSVoiceMessage", 1);
	for (int i = 1; i <= gpGlobals->maxClients; i++)
	{
		CBaseEntity *p = UTIL_PlayerByIndex(i);
		if (!p || !p->IsPlayer() || !p->IsAlive())
			continue;
		int pteam = TSGameRules()->GetTeamIndex(((CBasePlayer *)p)->TeamID());
		MESSAGE_BEGIN(MSG_ONE, gmsgTSVoiceMessage, NULL, p->edict());
		WRITE_BYTE((pteam != team) ? 1 : 0);
		MESSAGE_END();
	}
}
