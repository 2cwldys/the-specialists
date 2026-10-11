/***
*  ts_objectives.cpp — RECONSTRUCTED TS objective subsystem (team-mode maps) from ts_i386.so:
*  CTSObjective (base @0x7c2b0), CTSObjectiveManager (@0x7b7bc), CTSObjectivePointer (@0x7bb44),
*  CTSHack (@0x7cccc), CTSWinGiver/CTSScoreGiver (@0x7bf10/0x7c0b8), CTSBomb (@0x7c470),
*  CTSTeamEscape (@0x7d390), CVipEscape, CTSBriefcase.
*
*  These are the entities the `tm_` (team-mode) maps place (tm_avana: ts_hack,
*  ts_objective_manager, ts_objective_ptr, ts_wingiver). CTSGameRules drives them: SetObjective
*  picks a manager whose Reset fires its pointers, each pointer enables its objectives and lists
*  them on the HUD (AddObjectiveInList, ShowObjectiveOnHud), and the givers call TeamWin. The hack,
*  bomb and escape spawn inactive and nothing but a pointer enables them, which happens only in
*  the objective modes (2/3) that the shipped game never reaches; the givers and the briefcase work
*  in any mode.
***/
#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "player.h"
#include "ts_utils.h"     // TSHudMessageAll
#include "gamerules.h"
#include "ts_gamerules.h" // TSGameRules->TeamWin
#include "ts_objectives.h" // CTSObjective (the cluster base, shared with ts_bomb/ts_escape)
#include "ts_hack.h"       // CTSHack
#include "ts_wingiver.h"   // CTSWinGiver / CTSScoreGiver
#include "explode.h"       // ExplosionCreate (CTSBomb)
#include "ts_bomb.h"
#include "ts_escape.h"
#include "ts_flag.h"       // s_flag_status (must precede ts_briefcase.h) + CTSFlag::CreateFlag
#include "ts_briefcase.h"
#include "UserMessages.h"  // gmsgScoreInfo, gmsgTeamScore (CTSBriefcase/CTSDMHill scoring)

TYPEDESCRIPTION CTSObjective::m_SaveData[] = {
	DEFINE_FIELD(CTSObjective, attacker_team, FIELD_INTEGER),
};
IMPLEMENT_SAVERESTORE(CTSObjective, CBaseEntity);

// @0x7c2b0 KeyValue — "attackerteam" only; it does not chain to CBaseEntity::KeyValue.
bool CTSObjective::KeyValue(KeyValueData *pkvd)
{
	if (FStrEq(pkvd->szKeyName, "attackerteam"))
	{
		attacker_team = atoi(pkvd->szValue);
		return true;
	}
	return false;
}

// @0x7c38c CanInteract — only the attacker team (or anyone if -1) may act on this objective.
bool CTSObjective::CanInteract(int team)
{
	if (attacker_team == TS_TEAM_ANY)
		return true;
	return attacker_team == team;
}

// @0x7c344 CanInteract — resolve a player's team index, then gate.
bool CTSObjective::CanInteract(CBasePlayer *pPlayer)
{
	return CanInteract(TSGameRules->GetTeamIndex(pPlayer->TeamID()));
}

// @0x7c310 Freeze — enable/disable the objective + re-render it on the HUD.
void CTSObjective::Freeze(bool bFreeze)
{
	m_bActive = !bFreeze;
	TSGameRules->ShowObjectiveOnHud(this, !bFreeze);
}

//============================================================================
// CTSObjectiveManager — point entity that holds the map's objective-pointer targets +
// the per-slot objective messages; fires them on Reset(). @0x7b7bc.
//============================================================================

// CTSObjectiveManager is declared in ts_objectives.h (shared with ts_gamerules.cpp's SetObjective).
LINK_ENTITY_TO_CLASS(ts_objective_manager, CTSObjectiveManager);

// 13 saved string fields (the original's WriteFields count of 0xd): 8 targets + 4 messages + end_round_target.
TYPEDESCRIPTION CTSObjectiveManager::m_SaveData[] = {
	DEFINE_ARRAY(CTSObjectiveManager, targets, FIELD_STRING, TS_OBJ_TARGETS),
	DEFINE_ARRAY(CTSObjectiveManager, messages, FIELD_STRING, TS_OBJ_MESSAGES),
	DEFINE_FIELD(CTSObjectiveManager, end_round_target, FIELD_STRING),
};
IMPLEMENT_SAVERESTORE(CTSObjectiveManager, CBaseEntity);

// @0x7b7bc Spawn: a hidden point entity, nextthink 10 s out with no think set.
void CTSObjectiveManager::Spawn()
{
	pev->classname = MAKE_STRING("ts_objective_manager");
	pev->effects = EF_NODRAW;
	pev->solid = SOLID_NOT;
	pev->flags = 0;
	pev->movetype = MOVETYPE_NONE;
	SetTouch(NULL);
	SetThink(NULL);
	pev->nextthink = gpGlobals->time + 10.0f;
}

// @0x7b46c KeyValue — the eight objptarget<a..h>, the four message<a..d>, targetontimeleft,
// targetonspecdead and targetonmercdead; nothing else, and no chain to CBaseEntity::KeyValue.
bool CTSObjectiveManager::KeyValue(KeyValueData *pkvd)
{
	// objptargeta..h -> targets[0..7]
	if (!strncmp(pkvd->szKeyName, "objptarget", 10) && pkvd->szKeyName[10] >= 'a' && pkvd->szKeyName[10] <= 'h' && !pkvd->szKeyName[11])
	{
		targets[pkvd->szKeyName[10] - 'a'] = ALLOC_STRING(pkvd->szValue);
		return true;
	}
	// messagea..d -> messages[0..3]
	if (!strncmp(pkvd->szKeyName, "message", 7) && pkvd->szKeyName[7] >= 'a' && pkvd->szKeyName[7] <= 'd' && !pkvd->szKeyName[8])
	{
		messages[pkvd->szKeyName[7] - 'a'] = ALLOC_STRING(pkvd->szValue);
		return true;
	}
	if (FStrEq(pkvd->szKeyName, "targetontimeleft")) { end_round_target = ALLOC_STRING(pkvd->szValue); return true; }
	if (FStrEq(pkvd->szKeyName, "targetonspecdead")) { m_targetSpecDead = ALLOC_STRING(pkvd->szValue); return true; }
	if (FStrEq(pkvd->szKeyName, "targetonmercdead")) { m_targetMercDead = ALLOC_STRING(pkvd->szValue); return true; }
	return false;
}

// @0x7b850 Reset — announce each non-empty message + fire each objective-pointer target.
void CTSObjectiveManager::Reset()
{
	for (int i = 0; i < TS_OBJ_MESSAGES; i++)
		if (!FStringNull(messages[i]))
			TSHudMessageAll(0, 0xff, 0, 8, 30, STRING(messages[i]));

	for (int i = 0; i < TS_OBJ_TARGETS; i++)
		if (!FStringNull(targets[i]))
			FireTargets(STRING(targets[i]), this, this, USE_TOGGLE, 0);
}

//============================================================================
// CTSObjectivePointer — a per-team HUD pointer to an objective. The visible arrow is
// client-side; server-side this just spawns + stores the per-team messages. @0x7bb44.
//============================================================================
class CTSObjectivePointer : public CBaseEntity
{
public:
	void Spawn() override;
	bool KeyValue(KeyValueData *pkvd) override;
	void Use(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value) override; // @0x7bbd8

	string_t m_objMsg[5] = {}; // objmsgspec/merc/team3/team4/all (per-team pointer label)
};

LINK_ENTITY_TO_CLASS(ts_objective_ptr, CTSObjectivePointer);

// @0x7bb44 Spawn: a hidden point entity, nextthink 10 s out with no think set.
void CTSObjectivePointer::Spawn()
{
	pev->classname = MAKE_STRING("ts_objective_ptr");
	pev->effects = EF_NODRAW;
	pev->solid = SOLID_NOT;
	pev->flags = 0;
	pev->movetype = MOVETYPE_NONE;
	SetTouch(NULL);
	SetThink(NULL);
	pev->nextthink = gpGlobals->time + 10.0f;
}

// @0x7bbd8 Use — activate the objective(s) this pointer targets: enable each, add it
// to the HUD objective list carrying this pointer's per-team labels, and send it to every client.
void CTSObjectivePointer::Use(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value)
{
	if (FStringNull(pev->target))
		return;
	CBaseEntity *pEnt = nullptr;
	while ((pEnt = UTIL_FindEntityByTargetname(pEnt, STRING(pev->target))) != nullptr)
	{
		if (!pEnt->IsObjective())
			continue;
		pEnt->EnableObjective(true);
		// the 4 per-team labels (spec/merc/team3/team4) — the 5th (objmsgall) isn't part of the list entry.
		int labels[4] = { (int)m_objMsg[0], (int)m_objMsg[1], (int)m_objMsg[2], (int)m_objMsg[3] };
		int idx = TSGameRules->AddObjectiveInList((CTSObjective *)pEnt, labels);
		if (idx == -1)
			continue;
		for (int i = 1; i <= gpGlobals->maxClients; i++)
		{
			CBasePlayer *p = (CBasePlayer *)UTIL_PlayerByIndex(i);
			if (p)
				TSGameRules->SendObjectiveToHud(idx, p);
		}
	}
}

// @0x7ba1c KeyValue — the five objmsg* keys; nothing else, and no chain to CBaseEntity::KeyValue.
bool CTSObjectivePointer::KeyValue(KeyValueData *pkvd)
{
	static const char *const keys[5] = { "objmsgspec", "objmsgmerc", "objmsgteam3", "objmsgteam4", "objmsgall" };
	for (int i = 0; i < 5; i++)
		if (FStrEq(pkvd->szKeyName, keys[i])) { m_objMsg[i] = ALLOC_STRING(pkvd->szValue); return true; }
	return false;
}

//============================================================================
// CTSHack — a hackable brush objective: an attacker-team player holding USE on it counts
// `hackingtime` down; on completion it fires its target + announces. @0x7cccc. It spawns inactive;
// an objective pointer enables it (EnableObjective), which only happens in the objective modes.
//============================================================================
// CTSHack is declared in ts_hack.h (extracted so the audit sees its FGD member `hacksec`).

LINK_ENTITY_TO_CLASS(ts_hack, CTSHack);

TYPEDESCRIPTION CTSHack::m_SaveData[] = {
	DEFINE_FIELD(CTSHack, hacksec, FIELD_INTEGER),
};
IMPLEMENT_SAVERESTORE(CTSHack, CTSObjective);

// @0x7cdb4: empty (the hack sounds are in TSPrecache's list)
void CTSHack::Precache()
{
}

// @0x7cccc Spawn — hidden, not solid, inactive, thinking every second.
void CTSHack::Spawn()
{
	pev->classname = MAKE_STRING("ts_hack");
	SET_MODEL(ENT(pev), STRING(pev->model));
	UTIL_SetOrigin(pev, pev->origin);
	pev->effects = EF_NODRAW;
	pev->solid = SOLID_NOT;
	pev->flags = 0;
	pev->movetype = MOVETYPE_NONE;
	m_bActive = false;
	SetTouch(NULL);
	SetThink(&CTSHack::WaitThink);
	pev->nextthink = pev->ltime + 1.0f;
	m_flObjTimer = 0;
	m_flLastUse = 0;
	m_flHackProgress = 0;
}

// @0x7d1c8 KeyValue — "hackingtime", else the objective's.
bool CTSHack::KeyValue(KeyValueData *pkvd)
{
	if (FStrEq(pkvd->szKeyName, "hackingtime"))
	{
		hacksec = atoi(pkvd->szValue);
		return true;
	}
	return CTSObjective::KeyValue(pkvd);
}

// @0x7cdb8 EnableObjective — arm/disarm. Arming shows it as a USE-able brush and seeds the objective
// deadline with the round's time left (negative if no round runs); the hack progress only starts on
// the first USE.
void CTSHack::EnableObjective(bool enable)
{
	m_bActive = enable;
	if (enable)
	{
		pev->effects &= ~EF_NODRAW;
		pev->solid = SOLID_BSP;
		pev->movetype = MOVETYPE_PUSH;
		pev->flags |= FL_WORLDBRUSH;
		SET_MODEL(ENT(pev), STRING(pev->model));
		m_flObjTimer = TSGameRules->m_flRoundEnd - gpGlobals->time;
	}
	else
	{
		pev->effects = EF_NODRAW;
		pev->solid = SOLID_NOT;
		pev->flags = 0;
		pev->movetype = MOVETYPE_NONE;
		m_flObjTimer = 0;
	}
	m_flLastUse = 0;
	m_flHackProgress = 0;
	m_flNextSound = 0;
}

// @0x7ceb0 WaitThink — the objective deadline: while armed, count it down a second at a time and
// freeze the objective when it runs out.
void CTSHack::WaitThink()
{
	pev->nextthink = pev->ltime + 1.0f;
	if (m_bActive)
	{
		m_flObjTimer -= 1.0f;
		if (m_flObjTimer <= 0.0f)
			Freeze(true);
	}
}

// @0x7cf14 Use — a player HOLDS USE on the panel: progress the hack (from `hacksec`), beeping each
// second; when the progress runs out the objective completes (fire target + HUD + "hacked" sound + freeze).
void CTSHack::Use(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value)
{
	if (!m_bActive || !pActivator || !pActivator->IsPlayer())
		return;
	CBasePlayer *pPlayer = (CBasePlayer *)pActivator;
	if (!CanInteract(pPlayer))
		return;

	if (m_flLastUse == 0.0f)
		m_flLastUse = gpGlobals->time;

	// held continuously (gap < 0.5s)? progress the hack.
	if (gpGlobals->time - m_flLastUse < 0.5f)
	{
		if (m_flHackProgress == 0.0f) // init the progress timer: hacksec, capped 30, floored 1
			m_flHackProgress = (hacksec < 30) ? (float)(hacksec < 1 ? 1 : hacksec) : 30.0f;
		else
			m_flHackProgress -= (gpGlobals->time - m_flLastUse); // count down by the elapsed time
	}

	if (m_flNextSound < gpGlobals->time) // the "hacking" sound, once a second
	{
		m_flNextSound = gpGlobals->time + 1.0f;
		// @0x7d052: static channel, attenuation 1.25; the pitch is drawn before the volume
		const int pitch = 98 + RANDOM_LONG(0, 3);
		EMIT_SOUND_DYN(edict(), CHAN_STATIC, "misc/hacking.wav", RANDOM_FLOAT(0.92f, 1.0f), 1.25f, 0, pitch);
	}

	ClientPrint(pPlayer->pev, HUD_PRINTCENTER, UTIL_VarArgs("%i seconds left", (int)m_flHackProgress));
	m_flLastUse = gpGlobals->time;

	if (m_flHackProgress < 0.0f) // hack complete
	{
		if (!FStringNull(pev->target))
			FireTargets(STRING(pev->target), this, this, USE_TOGGLE, 0);
		if (!FStringNull(pev->message))
			TSHudMessageAll(0, 0xff, 0, 8, 30, STRING(pev->message));
		m_flObjTimer = 0;
		Freeze(true);
		const int pitch = 98 + RANDOM_LONG(0, 3);
		EMIT_SOUND_DYN(edict(), CHAN_STATIC, "misc/hacked.wav", RANDOM_FLOAT(0.92f, 1.0f), 1.25f, 0, pitch);
	}
}

//============================================================================
// CTSWinGiver — when Use'd `callsnumber` times, declares a team the winner, once: TeamWin(team, the
// use value as the winners' cash, restart the round) and the message. @0x7bf10. Its base is
// CBaseEntity, not CTSObjective (the original's typeinfo).
//============================================================================
// CTSWinGiver is declared in ts_wingiver.h (extracted so the audit sees its FGD members
// winnerteam/victorycash/defeatcash/callsnumber).

LINK_ENTITY_TO_CLASS(ts_wingiver, CTSWinGiver);

// 4 saved fields (the original's WriteFields count of 4).
TYPEDESCRIPTION CTSWinGiver::m_SaveData[] = {
	DEFINE_FIELD(CTSWinGiver, winnerteam, FIELD_INTEGER),
	DEFINE_FIELD(CTSWinGiver, cash_for_winner, FIELD_INTEGER),
	DEFINE_FIELD(CTSWinGiver, cash_for_loser, FIELD_INTEGER),
	DEFINE_FIELD(CTSWinGiver, numberofcall, FIELD_INTEGER),
};
IMPLEMENT_SAVERESTORE(CTSWinGiver, CBaseEntity);

// @0x7bf00 EnableObjective — whatever the argument: the call count (+0x64) and the done byte (+0x68)
// go back to 0, so the giver can fire again.
void CTSWinGiver::EnableObjective(bool enable)
{
	m_calls = 0;
	m_bDone = false;
}

// @0x7bf10: a hidden point entity, nextthink 10 s out with no think set, then Activate (CBaseEntity's).
void CTSWinGiver::Spawn()
{
	pev->classname = MAKE_STRING("ts_wingiver");
	pev->effects = EF_NODRAW;
	pev->solid = SOLID_NOT;
	pev->flags = 0;
	pev->movetype = MOVETYPE_NONE;
	SetTouch(NULL);
	SetThink(NULL);
	pev->nextthink = gpGlobals->time + 10.0f;
	Activate();
}

bool CTSWinGiver::KeyValue(KeyValueData *pkvd)
{
	// @0x7bdfc: these four only; no chain to a base KeyValue.
	// victorycash/defeatcash are kept and saved, and nothing reads them.
	if (FStrEq(pkvd->szKeyName, "victorycash")) { cash_for_winner = atoi(pkvd->szValue); return true; }
	if (FStrEq(pkvd->szKeyName, "defeatcash"))  { cash_for_loser = atoi(pkvd->szValue); return true; }
	if (FStrEq(pkvd->szKeyName, "winnerteam"))  { winnerteam = atoi(pkvd->szValue); return true; }
	if (FStrEq(pkvd->szKeyName, "callsnumber")) { numberofcall = atoi(pkvd->szValue); return true; }
	return false;
}

// @0x7bfb0 Use — count the calls; on the last, declare the winner, then the message, once.
void CTSWinGiver::Use(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value)
{
	if (m_bDone)
		return;
	if (++m_calls < numberofcall)
		return;

	TSGameRules->TeamWin(winnerteam, (int)value, true, false);
	if (!FStringNull(pev->message))
		TSHudMessageAll(0, 0xff, 0, 8, 30, STRING(pev->message));
	m_calls = 0;
	m_bDone = true;
}

//============================================================================
// CTSScoreGiver — like CTSWinGiver, but REPEATABLE: each time it's Use'd `callsnumber`
// times it scores the team again (resets the call count instead of latching done). @Spawn
// 0x7c0b8 / Use 0x7c158. Its spawnflag 1 ("Respawn Players") makes the score restart the round.
//============================================================================
#define SF_SCOREGIVER_RESPAWN 1

// CTSScoreGiver is declared in ts_wingiver.h (derives CTSWinGiver).

LINK_ENTITY_TO_CLASS(ts_scoregiver, CTSScoreGiver);

// @KeyValue__13CTSScoreGiver — same keys as the win giver (winnerteam/callsnumber/...).
bool CTSScoreGiver::KeyValue(KeyValueData *pkvd)
{
	return CTSWinGiver::KeyValue(pkvd);
}

// @Spawn__13CTSScoreGiver — a hidden point relay; polls (nextthink) and registers as an objective.
void CTSScoreGiver::Spawn()
{
	pev->classname = MAKE_STRING("ts_scoregiver");
	pev->effects = EF_NODRAW;
	pev->solid = SOLID_NOT;
	pev->flags = 0;
	pev->movetype = MOVETYPE_NONE;
	SetTouch(NULL);
	SetThink(NULL);
	pev->nextthink = gpGlobals->time + 10.0f;
	Activate();
}

// @0x7c158 Use — count the calls; on the last, TeamWin as a score (the use value as the winners'
// cash, a round restart only with spawnflag 1), the message, and count again.
void CTSScoreGiver::Use(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value)
{
	if (++m_calls < numberofcall)
		return;

	TSGameRules->TeamWin(winnerteam, (int)value, (pev->spawnflags & SF_SCOREGIVER_RESPAWN) != 0, true);
	if (!FStringNull(pev->message))
		TSHudMessageAll(0, 0xff, 0, 8, 30, STRING(pev->message));
	m_calls = 0;
}

//============================================================================
// CVipEscape — the VIP safety zone class (`func_vip_safetyzone`, brush): a passive marker with no
// touch or use of its own. @Spawn 0x7d23c. Nothing in the original reads it.
//============================================================================
class CVipEscape : public CBaseEntity
{
public:
	void Spawn() override;                          // @0x7d23c
	void Precache() override;                       // @0x7d238 — empty
	bool KeyValue(KeyValueData *pkvd) override;     // @0x7d22c — handles no keys
};

// func_vip_safetyzone is not registered in the original: CVipEscape has no GetClassPtr and no
// LINK factory, so it is never constructed (its Spawn, which sets the classname, never runs). The
// FGD's team-escape entity is ts_teamescape -> CTSTeamEscape. The class is kept, unlinked.
// LINK_ENTITY_TO_CLASS(func_vip_safetyzone, CVipEscape);

// @0x7d22c — CVipEscape ignores all keyvalues (the original just clears fHandled and returns;
// it does NOT chain to CBaseEntity::KeyValue).
bool CVipEscape::KeyValue(KeyValueData *pkvd)
{
	pkvd->fHandled = false;
	return false;
}

// @0x7d238 — empty.
void CVipEscape::Precache()
{
}

// @0x7d23c — passive non-solid brush marker (EF_NODRAW, SOLID_NOT, MOVETYPE_NONE).
void CVipEscape::Spawn()
{
	pev->effects = EF_NODRAW;
	pev->solid = SOLID_NOT;
	pev->flags = 0;
	pev->movetype = MOVETYPE_NONE;
	pev->classname = MAKE_STRING("func_vip_safetyzone");
	Precache();
	SET_MODEL(ENT(pev), STRING(pev->model));
}


// ===================== CTSBomb (merged from ts_bomb.cpp) =====================
// A bomb objective: it spawns inactive and hidden; an objective pointer arms it (EnableObjective),
// which only happens in the objective modes. Armed, it counts the round's time left down a second at
// a time (a tenth after the booby-trap wire), beeping, and explodes at zero; holding USE for
// `defusetime` or cutting the defuse wire defuses it.

// wire states (the .so's +0x7c..0x7e): 1 defuse, 2 boobytrap, 3 neutral, 0 already cut
enum tsbomb_wire_t { WIRE_CUT = 0, WIRE_DEFUSE = 1, WIRE_BOOBYTRAP = 2, WIRE_NEUTRAL = 3 };

LINK_ENTITY_TO_CLASS(ts_bomb, CTSBomb)

TYPEDESCRIPTION CTSBomb::m_SaveData[] = {
	DEFINE_FIELD(CTSBomb, defusesec, FIELD_INTEGER),
	DEFINE_FIELD(CTSBomb, cancutwire, FIELD_INTEGER),
};
IMPLEMENT_SAVERESTORE(CTSBomb, CTSObjective);

// @0x7c570
void CTSBomb::Precache()
{
	PRECACHE_SOUND("misc/beep2.wav");
}

// @0x7cb68 KeyValue — defusetime / cutwires, else the objective's (attackerteam).
bool CTSBomb::KeyValue(KeyValueData *pkvd)
{
	if (FStrEq(pkvd->szKeyName, "defusetime")) { defusesec = atoi(pkvd->szValue); return true; }
	if (FStrEq(pkvd->szKeyName, "cutwires"))   { cancutwire = atoi(pkvd->szValue); return true; }
	return CTSObjective::KeyValue(pkvd);
}

// @0x7c470 Spawn — hidden, not solid, inactive, thinking every second; no wires yet.
void CTSBomb::Spawn()
{
	pev->classname = MAKE_STRING("ts_bomb");
	SET_MODEL(edict(), STRING(pev->model));
	UTIL_SetOrigin(pev, pev->origin);
	pev->effects = EF_NODRAW;
	pev->solid = SOLID_NOT;
	pev->flags = 0;
	pev->movetype = MOVETYPE_NONE;
	m_bActive = false;
	SetTouch(NULL);
	SetThink(&CTSBomb::WaitThink);
	pev->nextthink = pev->ltime + 1.0f;
	m_flTimer = 0;
	m_flNextBeep = 0;
	m_flLastUse = 0;
	m_flUseProgress = 0;
	m_wires[0] = m_wires[1] = m_wires[2] = WIRE_CUT;
	m_bFast = false;
}

// @0x7c598 SetRandomWires — 3 wires: one defuse (1), one boobytrap (2), the rest neutral (3).
void CTSBomb::SetRandomWires()
{
	m_wires[0] = m_wires[1] = m_wires[2] = WIRE_NEUTRAL;
	int defuse = RANDOM_LONG(0, 2);
	m_wires[defuse] = WIRE_DEFUSE;
	int trap;
	do { trap = RANDOM_LONG(0, 2); } while (m_wires[trap] == WIRE_DEFUSE);
	m_wires[trap] = WIRE_BOOBYTRAP;
}

// @0x7c608 EnableObjective — arm: shown as a USE-able brush, the countdown the round's time left
// (negative if no round runs), new wires; disarm: hidden and stopped.
void CTSBomb::EnableObjective(bool enable)
{
	m_bActive = enable;
	m_bFast = false;
	if (enable)
	{
		pev->effects &= ~EF_NODRAW;
		pev->solid = SOLID_BSP;
		pev->movetype = MOVETYPE_PUSH;
		pev->flags |= FL_WORLDBRUSH;
		SET_MODEL(edict(), STRING(pev->model));
		m_flTimer = TSGameRules->m_flRoundEnd - gpGlobals->time;
		m_flNextBeep = 0;
		m_flLastUse = 0;
		m_flUseProgress = 0;
		SetRandomWires();
	}
	else
	{
		pev->effects = EF_NODRAW;
		pev->solid = SOLID_NOT;
		pev->flags = 0;
		pev->movetype = MOVETYPE_NONE;
		m_flTimer = 0;
		m_flNextBeep = 0;
		m_flLastUse = 0;
		m_flUseProgress = 0;
	}
}

// @0x7c70c CutWire — with wire cutting on and time left: a neutral wire ends the countdown (it
// explodes on the next tick), the booby-trap speeds it up tenfold and hands the cutter the wire
// states, the defuse wire fires the target, announces and freezes the bomb.
void CTSBomb::CutWire(int wire, CBasePlayer *pPlayer)
{
	if (!m_bActive || !cancutwire || m_flTimer <= 0.0f || (unsigned int)wire >= 3)
		return;
	if (m_wires[wire] == WIRE_CUT || pPlayer == nullptr)
		return;

	if (m_wires[wire] == WIRE_NEUTRAL)
	{
		m_wires[wire] = WIRE_CUT;
		m_flTimer = 0;
	}
	else if (m_wires[wire] == WIRE_BOOBYTRAP)
	{
		m_bFast = true;
		m_wires[wire] = WIRE_CUT;
		pPlayer->m_iWires[0] = (byte)m_wires[0];
		pPlayer->m_iWires[1] = (byte)m_wires[1];
		pPlayer->m_iWires[2] = (byte)m_wires[2];
	}
	else if (m_wires[wire] == WIRE_DEFUSE)
	{
		m_wires[wire] = WIRE_CUT;
		if (!FStringNull(pev->target))
			FireTargets(STRING(pev->target), this, this, USE_TOGGLE, 0);
		if (!FStringNull(pev->message))
			TSHudMessageAll(0, 0xff, 0, 8, 30, (char *)STRING(pev->message));
		m_flTimer = 0;
		m_flNextBeep = 0;
		Freeze(true);
	}
}

// @0x7c9a4 Use — an attacker-team player HOLDS USE on it: the defuse counts `defusetime` down; when
// it runs out the bomb is defused (target, message, frozen).
void CTSBomb::Use(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value)
{
	if (!m_bActive || !pActivator || !pActivator->IsPlayer())
		return;
	CBasePlayer *pPlayer = (CBasePlayer *)pActivator;
	if (!CanInteract(pPlayer))
		return;

	if (m_flLastUse == 0.0f)
		m_flLastUse = gpGlobals->time;
	if (gpGlobals->time - m_flLastUse < 0.5f)
	{
		if (m_flUseProgress == 0.0f)
			m_flUseProgress = (float)defusesec;
		else
			m_flUseProgress -= (gpGlobals->time - m_flLastUse);
	}

	ClientPrint(pPlayer->pev, HUD_PRINTCENTER, UTIL_VarArgs("%i seconds left", (int)m_flUseProgress));
	m_flLastUse = gpGlobals->time;

	if (m_flUseProgress < 0.0f)
	{
		if (!FStringNull(pev->target))
			FireTargets(STRING(pev->target), this, this, USE_TOGGLE, 0);
		if (!FStringNull(pev->message))
			TSHudMessageAll(0, 0xff, 0, 8, 30, (char *)STRING(pev->message));
		m_flTimer = 0;
		m_flNextBeep = 0;
		Freeze(true);
	}
}

// @0x7c860 WaitThink — the bomb tick. Armed: count down one each tick; with time left, the beep
// (each second, each tenth after the booby-trap); at zero the round's time is up (RoundTimeLeft),
// the bomb freezes and explodes: TSExplosion from its centre, 300 damage, full out to 2048 units,
// nothing past 4096.
void CTSBomb::WaitThink()
{
	pev->nextthink = pev->ltime + (m_bFast ? 0.1f : 1.0f);
	if (!m_bActive)
		return;

	m_flTimer -= 1.0f;
	if (m_flTimer > 0.0f)
	{
		if (m_flNextBeep < gpGlobals->time)
		{
			m_flNextBeep = gpGlobals->time + (m_bFast ? 0.1f : 1.0f);
			const int pitch = 98 + RANDOM_LONG(0, 3);
			EMIT_SOUND_DYN(edict(), CHAN_STATIC, "misc/beep2.wav", RANDOM_FLOAT(0.92f, 1.0f), 1.25f, 0, pitch);
		}
	}
	else
	{
		TSGameRules->RoundTimeLeft();
		Freeze(true);
		Vector vecCenter = Center();
		TSExplosion(vecCenter, pev, pev, 300.0f, DMG_BLAST | DMG_NEVERGIB, 2048.0f, 4096.0f);
	}
}



// ===================== CTSTeamEscape (merged from ts_escape.cpp) =====================
// An escape zone objective: a hidden trigger, inactive until an objective pointer enables it (the
// objective modes only). Once the share of the attacker team set by escapepercentage has stood in it
// within the same second, it fires its target, announces and freezes.

LINK_ENTITY_TO_CLASS(ts_teamescape, CTSTeamEscape)

TYPEDESCRIPTION CTSTeamEscape::m_SaveData[] = {
	DEFINE_FIELD(CTSTeamEscape, escape_percentage, FIELD_INTEGER),
};
IMPLEMENT_SAVERESTORE(CTSTeamEscape, CTSObjective);

// @0x7d700 KeyValue — escapepercentage, else the objective's (attackerteam).
bool CTSTeamEscape::KeyValue(KeyValueData *pkvd)
{
	if (FStrEq(pkvd->szKeyName, "escapepercentage")) { escape_percentage = atoi(pkvd->szValue); return true; }
	return CTSObjective::KeyValue(pkvd);
}

// @0x7d390 Spawn — a hidden trigger brush (solid even while inactive), touching and thinking every
// second, nobody counted.
void CTSTeamEscape::Spawn()
{
	pev->classname = MAKE_STRING("ts_teamescape");
	SET_MODEL(edict(), STRING(pev->model));
	UTIL_SetSize(pev, pev->mins, pev->maxs);
	UTIL_SetOrigin(pev, pev->origin);
	pev->effects = EF_NODRAW;
	pev->solid = SOLID_TRIGGER;
	pev->flags = 0;
	pev->movetype = MOVETYPE_FLY;
	m_bActive = false;
	SetTouch(&CTSTeamEscape::WaitTouch);
	SetThink(&CTSTeamEscape::WaitThink);
	pev->nextthink = gpGlobals->time + 1.0f;
	for (int i = 0; i < 33; i++)
		m_present[i] = NULL;
	m_count = 0;
}

// @0x7d4a0 EnableObjective — the active flag, the count cleared, a trigger or not solid.
void CTSTeamEscape::EnableObjective(bool enable)
{
	m_bActive = enable;
	for (int i = 0; i < 33; i++)
		m_present[i] = NULL;
	m_count = 0;
	pev->solid = enable ? SOLID_TRIGGER : SOLID_NOT;
}

// @0x7d4ec WaitTouch — an attacker-team player inside, counted once a second; the threshold is one
// player, a third, two thirds or all of the attacker team's players in the round (alive or about to
// respawn, TSCountPlayers), and 0 for any other escapepercentage.
void CTSTeamEscape::WaitTouch(CBaseEntity *pOther)
{
	if (!m_bActive || !pOther || !pOther->IsPlayer())
		return;
	CBasePlayer *pPlayer = (CBasePlayer *)pOther;
	if (!CanInteract(pPlayer))
		return;

	int teamSize = 0;
	if (attacker_team >= 0)
		teamSize = TSCountPlayers(TSGameRules->m_teamNames[attacker_team], true);

	const int idx = pPlayer->entindex();
	if ((unsigned int)idx < 33)
	{
		if ((CBaseEntity *)m_present[idx] == NULL)
			m_count++;
		m_present[idx] = pPlayer;
	}

	int threshold = 0;
	switch (escape_percentage)
	{
	case 0: threshold = 1; break;
	case 1: threshold = teamSize / 3; break;
	case 2: threshold = (teamSize / 3) * 2; break;
	case 3: threshold = teamSize; break;
	}

	if (m_count >= threshold)
	{
		if (!FStringNull(pev->target))
			FireTargets(STRING(pev->target), this, this, USE_TOGGLE, 0);
		if (!FStringNull(pev->message))
			TSHudMessageAll(0, 0xff, 0, 8, 30, (char *)STRING(pev->message));
		Freeze(true);
	}
}

// @0x7d6b0 WaitThink — every second, forget who was inside (so the count reflects simultaneous presence).
void CTSTeamEscape::WaitThink()
{
	pev->nextthink = gpGlobals->time + 1.0f;
	for (int i = 0; i < 33; i++)
		m_present[i] = NULL;
	m_count = 0;
}


// ===================== CTSBriefcase (merged from ts_briefcase.cpp, last class) =====================

// @0x1779a0 — cycling flag-id source (1..15), bumped per briefcase in the ctor.
int CTSBriefcase::s_nextFlagID = 0;

// gmsgTSHaveFlag (the base-spawn notify type 0) is defined in ts_flag.cpp.
extern int gmsgTSHaveFlag;

// @ gmsgTSVoiceMessage — MSG_ONE(1): a per-player capture taunt (1 = an enemy captured, 0 = yours).
// Wire name "TSHVoice" (star_LinkUserMessages.md #54); defined in TS_gamerulesclass.cpp (the
// original's home), registered centrally in LinkUserMessages.
extern int gmsgTSVoiceMessage;

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
	return m_iOwnerTeam == TSGameRules->GetTeamIndex(pPlayer->TeamID());
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

	int team = TSGameRules->GetTeamIndex(pPlayer->TeamID());

	if (TSGameRules->CountPlayers() < 2)
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
		TSGameRules->m_teamScore[team]++;
	if (gmsgTeamScore)
	{
		for (int t = 0; t < TSGameRules->GetTeamsNum(); t++)
		{
			MESSAGE_BEGIN(MSG_ALL, gmsgTeamScore);
			WRITE_STRING(TSGameRules->GetIndexedTeamName(t));
			WRITE_SHORT(TSGameRules->m_teamScore[t]);
			WRITE_SHORT(0);
			MESSAGE_END();
		}
	}

	// Per-player capture taunt (1 = an enemy team captured, 0 = your team).
	for (int i = 1; i <= gpGlobals->maxClients; i++)
	{
		CBaseEntity *p = UTIL_PlayerByIndex(i);
		if (!p || !p->IsPlayer() || !p->IsAlive())
			continue;
		int pteam = TSGameRules->GetTeamIndex(((CBasePlayer *)p)->TeamID());
		MESSAGE_BEGIN(MSG_ONE, gmsgTSVoiceMessage, NULL, p->edict());
		WRITE_BYTE((pteam != team) ? 1 : 0);
		MESSAGE_END();
	}
}
