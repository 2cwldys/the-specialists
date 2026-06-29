/***
*  ts_escape.cpp — RECONSTRUCTED CTSTeamEscape (cont.42), the team escape/escort zone.
*  Functional/source-faithful (see CLAUDE.md). recovered/escape/CTSTeamEscape.c:
*    Spawn @0x8d390, EnableObjective @0x8d4a0, WaitTouch @0x8d4ec, WaitThink @0x8d6b0.
*
*  CTSTeamEscape is a map-placed (`@SolidClass`, brush, base tsobjective) escape zone: when
*  enough of the attacker team is simultaneously inside, the objective completes (fire target +
*  HUD message). `escapepercentage` sets the threshold (one member / 1/3 / 2/3 / all). The zone
*  counts distinct attacker-team players touching within a 1s window — WaitThink clears the set
*  each second, WaitTouch re-accumulates it. FGD-backed; 0 stock placements → synthetic-verified.
*
*  FUNCTIONAL reconstruction onto the stock SDK (the attacker-team gate inlined like
*  ts_objectives.cpp; team size = live players on the attacker team). // SLICE: enable is faithfully
*  gamerules-driven (InitObjectives) — here it arms on Spawn; no Save/Restore.
***/
#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "player.h"
#include "gamerules.h"
#include "ts_gamerules.h"
#include "ts_utils.h"   // TSHudMessageAll
#include "ts_objectives.h" // CTSObjective (the base CTSTeamEscape derives)

#include "ts_escape.h"

LINK_ENTITY_TO_CLASS(ts_teamescape, CTSTeamEscape)

TYPEDESCRIPTION CTSTeamEscape::m_SaveData[] = {
	DEFINE_FIELD(CTSTeamEscape, escape_percentage, FIELD_INTEGER),
};
IMPLEMENT_SAVERESTORE(CTSTeamEscape, CTSObjective);

bool CTSTeamEscape::KeyValue(KeyValueData *pkvd)
{
	if (FStrEq(pkvd->szKeyName, "escapepercentage")) { escape_percentage = atoi(pkvd->szValue); return true; }
	return CTSObjective::KeyValue(pkvd); // handles "attackerteam"
}

// @ Spawn — brush trigger; arm it and start the 1s reset tick.
void CTSTeamEscape::Spawn()
{
	if (!FStringNull(pev->model))
		SET_MODEL(edict(), STRING(pev->model)); // brush model
	UTIL_SetOrigin(pev, pev->origin);
	EnableObjective(true);
}

// @ EnableObjective — arm/disarm: (re)set the per-window tracking and the trigger solidity.
void CTSTeamEscape::EnableObjective(bool enable)
{
	m_bActive = enable;
	for (int i = 0; i < 33; i++)
		m_present[i] = NULL;
	m_count = 0;

	if (!enable)
	{
		pev->solid = SOLID_NOT;
		SetTouch(NULL);
		SetThink(NULL);
		pev->nextthink = 0;
		return;
	}

	pev->solid = SOLID_TRIGGER;
	SetTouch(&CTSTeamEscape::WaitTouch);
	SetThink(&CTSTeamEscape::WaitThink);
	pev->nextthink = gpGlobals->time + 1.0f;
}

// @ WaitTouch — an attacker-team player is inside: track them this window; complete when the
// distinct count meets the escapepercentage threshold (then freeze the objective).
void CTSTeamEscape::WaitTouch(CBaseEntity *pOther)
{
	if (!m_bActive || !pOther || !pOther->IsPlayer())
		return;
	CBasePlayer *pPlayer = (CBasePlayer *)pOther;
	if (!CanInteract(pPlayer)) // inherited CTSObjective gate (resolves the player's team index)
		return;

	// live players on the attacker team (the escape target population) — the original inlines this
	// (TSCountPlayers on the attacker-team roster) rather than a separate helper.
	int teamSize = 0;
	if (attacker_team >= 0)
		for (int i = 1; i <= gpGlobals->maxClients; i++)
		{
			CBaseEntity *p = UTIL_PlayerByIndex(i);
			if (p && p->IsAlive() && TSGameRules()->GetTeamIndex(p->TeamID()) == attacker_team)
				teamSize++;
		}

	int idx = pPlayer->entindex();
	if (idx >= 0 && idx < 33)
	{
		if ((CBaseEntity *)m_present[idx] == NULL)
			m_count++;          // newly seen this window
		m_present[idx] = pPlayer;
	}

	int threshold;
	switch (escape_percentage)
	{
	case 0:  threshold = 1; break;                 // one member
	case 1:  threshold = teamSize / 3; break;      // a third
	case 2:  threshold = (teamSize / 3) * 2; break;// two thirds
	case 3:  threshold = teamSize; break;          // all
	default: threshold = 1; break;
	}

	if (m_count >= threshold)
	{
		if (!FStringNull(pev->target))
			FireTargets(STRING(pev->target), this, this, USE_TOGGLE, 0);
		if (!FStringNull(pev->message))
			TSHudMessageAll(0, 0xff, 0, 8, 30, (char *)STRING(pev->message));
		Freeze(true); // the original freezes the objective on completion (m_bActive=false)
	}
}

// @ WaitThink — every second, forget who was inside (so the count reflects simultaneous presence).
void CTSTeamEscape::WaitThink()
{
	for (int i = 0; i < 33; i++)
		m_present[i] = NULL;
	m_count = 0;
	pev->nextthink = gpGlobals->time + 1.0f;
}

