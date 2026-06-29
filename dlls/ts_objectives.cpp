/***
*  ts_objectives.cpp — RECONSTRUCTED TS objective subsystem (team-mode maps).
*  Functional/source-faithful (see CLAUDE.md). Recovered from ts_i386.so:
*  CTSObjective (base @0x8c2b0), CTSObjectiveManager (@0x8b7bc), CTSObjectivePointer
*  (@0x8bb44), CTSHack (@0x8cccc), CTSWinGiver (@0x8bf10).
*
*  These are the entities the `tm_` (team-mode) maps place (tm_avana: ts_hack,
*  ts_objective_manager, ts_objective_ptr, ts_wingiver). The FAITHFUL engine drives
*  objective interaction + scoring through CTSGameRules (TeamWin / ShowObjectiveOnHud /
*  AddObjectiveInList — the team round/score system, not yet built). This is a
*  FUNCTIONAL reconstruction: each entity spawns, parses its keyvalues, and runs its own
*  standalone behaviour (hack countdown, win trigger, target firing, HUD). // SLICE: the
*  CTSGameRules team-win scoring + the gamerules-driven "who is interacting" loop + the
*  client objective HUD pointer are deferred to the team subsystem / cl_dll.
***/
#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "player.h"
#include "ts_utils.h"     // TSHudMessageAll
#include "gamerules.h"
#include "ts_gamerules.h" // TSGameRules()->TeamWin
#include "ts_objectives.h" // CTSObjective (the cluster base, shared with ts_bomb/ts_escape)

#define SF_TS_REMOVE_ON_FIRE 1

TYPEDESCRIPTION CTSObjective::m_SaveData[] = {
	DEFINE_FIELD(CTSObjective, attacker_team, FIELD_INTEGER),
};
IMPLEMENT_SAVERESTORE(CTSObjective, CBaseEntity);

// @KeyValue__12CTSObjective — "attackerteam".
bool CTSObjective::KeyValue(KeyValueData *pkvd)
{
	if (FStrEq(pkvd->szKeyName, "attackerteam"))
	{
		attacker_team = atoi(pkvd->szValue);
		return true;
	}
	return CBaseEntity::KeyValue(pkvd);
}

// @0x8c38c CanInteract — only the attacker team (or anyone if -1) may act on this objective.
bool CTSObjective::CanInteract(int team)
{
	if (attacker_team == TS_TEAM_ANY)
		return true;
	return attacker_team == team;
}

// @0x8c344 CanInteract — resolve a player's team index, then gate.
bool CTSObjective::CanInteract(CBasePlayer *pPlayer)
{
	return CanInteract(TSGameRules()->GetTeamIndex(pPlayer->TeamID()));
}

// @0x8c310 Freeze — enable/disable the objective. The original ALSO re-renders it on the HUD via
// CTSGameRules::ShowObjectiveOnHud(this, !bFreeze) — NOT yet reconstructed (deferred with the
// CTSGameRules objective methods); SLICE: we only toggle the active flag here.
void CTSObjective::Freeze(bool bFreeze)
{
	m_bActive = !bFreeze;
}

//============================================================================
// CTSObjectiveManager — point entity that holds the map's objective-pointer targets +
// the per-slot objective messages; fires them on Reset(). @0x8b7bc.
//============================================================================
#define TS_OBJ_TARGETS 8
#define TS_OBJ_MESSAGES 4

//============================================================================
// CTSTrigger — a TS brush trigger: fires its target when a valid player (right attacker
// team, optionally the flag carrier) touches it, on a recheck delay, with a HUD message.
// @0x709b8 Spawn / @0x70ad8 WaitTouch / @0x70c68 WaitThink. Derives the team gate from
// CTSObjective (the FGD `tsobjective` base).
//============================================================================
#define SF_TS_TRIGGER_FLAGCARRIER 8

class CTSTrigger : public CTSObjective
{
public:
	void Spawn() override;
	bool KeyValue(KeyValueData *pkvd) override;
	void EXPORT WaitTouch(CBaseEntity *pOther);
	void EXPORT WaitThink();                  // @0x70c68 — keep the trigger alive (reschedule)
	int ObjectCaps() override { return 0; }   // @0x73ad0 — no caps (not save/use-able as an objective)

	float m_flNextCheck = 0;   // nextcheck (recheck delay)
	float m_flNextAllowed = 0; // earliest time it can fire again
};

LINK_ENTITY_TO_CLASS(ts_trigger, CTSTrigger);

void CTSTrigger::Spawn()
{
	if (!FStringNull(pev->model))
		SET_MODEL(ENT(pev), STRING(pev->model)); // the brush
	pev->movetype = MOVETYPE_NONE;
	pev->solid = SOLID_TRIGGER;
	UTIL_SetOrigin(pev, pev->origin);
	SetTouch(&CTSTrigger::WaitTouch);
	// keep the trigger alive (the original arms a recheck nextthink + the first-allowed time)
	float wait = (m_flNextCheck < 0.5f) ? 0.5f : m_flNextCheck;
	SetThink(&CTSTrigger::WaitThink);
	pev->nextthink = gpGlobals->time + wait;
	m_flNextAllowed = gpGlobals->time + wait;
}

// @WaitThink__10CTSTrigger — reschedule so the trigger keeps ticking (recheck delay >= 0.5s).
void CTSTrigger::WaitThink()
{
	float wait = (m_flNextCheck < 0.5f) ? 0.5f : m_flNextCheck;
	pev->nextthink = gpGlobals->time + wait;
}

bool CTSTrigger::KeyValue(KeyValueData *pkvd)
{
	if (FStrEq(pkvd->szKeyName, "nextcheck"))
	{
		m_flNextCheck = atof(pkvd->szValue);
		return true;
	}
	return CTSObjective::KeyValue(pkvd); // attackerteam
}

// @WaitTouch — fire on a valid touch, then re-arm after `nextcheck` seconds.
void CTSTrigger::WaitTouch(CBaseEntity *pOther)
{
	if (gpGlobals->time < m_flNextAllowed)
		return;
	if (!pOther->IsPlayer() || !pOther->IsAlive())
		return;
	if (!CanInteract(pOther->pev->team)) // attacker-team gate (-1 = any)
		return;
	// SLICE: the "Flag Carrier Only" spawnflag (8) needs the CTF flag system (not built);
	// here it's ignored (the trigger fires for anyone on the right team).

	if (!FStringNull(pev->target))
		FireTargets((char *)STRING(pev->target), this, this, USE_TOGGLE, 0);
	if (!FStringNull(pev->message))
		TSHudMessageAll(0, 0xff, 0, 8, 30, STRING(pev->message));

	float delay = m_flNextCheck < 0.5f ? 0.5f : m_flNextCheck;
	m_flNextAllowed = gpGlobals->time + delay;
}

class CTSObjectiveManager : public CBaseEntity
{
public:
	void Spawn() override;
	bool KeyValue(KeyValueData *pkvd) override;
	void Reset(); // HUD the messages + fire the objective-pointer targets (@Reset 0x8b850)

	string_t m_objTargets[TS_OBJ_TARGETS] = {}; // objptargeta..h
	string_t m_messages[TS_OBJ_MESSAGES] = {};  // messagea..d
	string_t m_targetTimeLeft = iStringNull;    // targetontimeleft
	string_t m_targetSpecDead = iStringNull;    // targetonspecdead (Team 1 dead)
	string_t m_targetMercDead = iStringNull;    // targetonmercdead (Team 2 dead)
};

LINK_ENTITY_TO_CLASS(ts_objective_manager, CTSObjectiveManager);

void CTSObjectiveManager::Spawn()
{
	pev->movetype = MOVETYPE_NONE;
	pev->solid = SOLID_NOT;
	pev->effects |= EF_NODRAW;
}

bool CTSObjectiveManager::KeyValue(KeyValueData *pkvd)
{
	// objptargeta..h -> m_objTargets[0..7]
	if (!strncmp(pkvd->szKeyName, "objptarget", 10))
	{
		int idx = pkvd->szKeyName[10] - 'a';
		if (idx >= 0 && idx < TS_OBJ_TARGETS)
		{
			m_objTargets[idx] = ALLOC_STRING(pkvd->szValue);
			return true;
		}
	}
	// messagea..d -> m_messages[0..3]
	if (!strncmp(pkvd->szKeyName, "message", 7) && pkvd->szKeyName[7] >= 'a' && pkvd->szKeyName[7] <= 'd')
	{
		m_messages[pkvd->szKeyName[7] - 'a'] = ALLOC_STRING(pkvd->szValue);
		return true;
	}
	if (FStrEq(pkvd->szKeyName, "targetontimeleft")) { m_targetTimeLeft = ALLOC_STRING(pkvd->szValue); return true; }
	if (FStrEq(pkvd->szKeyName, "targetonspecdead")) { m_targetSpecDead = ALLOC_STRING(pkvd->szValue); return true; }
	if (FStrEq(pkvd->szKeyName, "targetonmercdead")) { m_targetMercDead = ALLOC_STRING(pkvd->szValue); return true; }
	return CBaseEntity::KeyValue(pkvd);
}

// @Reset — announce each non-empty message + fire each objective-pointer target.
void CTSObjectiveManager::Reset()
{
	for (int i = 0; i < TS_OBJ_MESSAGES; i++)
		if (!FStringNull(m_messages[i]))
			TSHudMessageAll(0, 0xff, 0, 8, 30, STRING(m_messages[i]));

	for (int i = 0; i < TS_OBJ_TARGETS; i++)
		if (!FStringNull(m_objTargets[i]))
			FireTargets(STRING(m_objTargets[i]), this, this, USE_TOGGLE, 0);
}

// Fire the objective manager's `targetontimeleft` when the round timer expires (called from
// CTSGameRules::RoundTimeLeft). No-op if there's no manager / target.
void TSFireTimeLeftTarget()
{
	CBaseEntity *pEnt = UTIL_FindEntityByClassname(nullptr, "ts_objective_manager");
	if (!pEnt)
		return;
	CTSObjectiveManager *mgr = (CTSObjectiveManager *)pEnt;
	if (!FStringNull(mgr->m_targetTimeLeft))
		FireTargets(STRING(mgr->m_targetTimeLeft), mgr, mgr, USE_TOGGLE, 0);
}

//============================================================================
// CTSObjectivePointer — a per-team HUD pointer to an objective. The visible arrow is
// client-side; server-side this just spawns + stores the per-team messages. @0x8bb44.
//============================================================================
class CTSObjectivePointer : public CBaseEntity
{
public:
	void Spawn() override;
	bool KeyValue(KeyValueData *pkvd) override;

	string_t m_objMsg[5] = {}; // objmsgspec/merc/team3/team4/all (per-team pointer label)
};

LINK_ENTITY_TO_CLASS(ts_objective_ptr, CTSObjectivePointer);

void CTSObjectivePointer::Spawn()
{
	pev->movetype = MOVETYPE_NONE;
	pev->solid = SOLID_NOT;
	pev->effects |= EF_NODRAW;
}

bool CTSObjectivePointer::KeyValue(KeyValueData *pkvd)
{
	static const char *const keys[5] = { "objmsgspec", "objmsgmerc", "objmsgteam3", "objmsgteam4", "objmsgall" };
	for (int i = 0; i < 5; i++)
		if (FStrEq(pkvd->szKeyName, keys[i])) { m_objMsg[i] = ALLOC_STRING(pkvd->szValue); return true; }
	return CBaseEntity::KeyValue(pkvd);
}

//============================================================================
// CTSHack — a hackable brush objective: a player of the attacker team standing in it
// counts down `hackingtime`; on completion it fires its target + announces. @0x8cccc.
// SLICE: the faithful "is a player hacking" decision is gamerules-driven; here a touch
// from a valid player drives the countdown.
//============================================================================
class CTSHack : public CTSObjective
{
public:
	void Spawn() override;
	void Precache() override;
	bool KeyValue(KeyValueData *pkvd) override;
	bool Save(CSave &save) override;
	bool Restore(CRestore &restore) override;
	int  ObjectCaps() override { return FCAP_CONTINUOUS_USE; } // @0x8e788 -> 0x10
	void Use(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value) override; // @0x8cf14
	void EnableObjective(bool enable) override; // @0x8cd6? arm/disarm + seed the objective deadline
	void EXPORT WaitThink();                    // @0x8ceb0 — objective deadline countdown -> Freeze

	static TYPEDESCRIPTION m_SaveData[];

	int   hacksec = 10;         // +0x7c — keyvalue "hackingtime": seconds of HELD USE to complete
	float m_flObjTimer = 0;     // +0x6c — objective deadline countdown (round seconds left)
	float m_flHackProgress = 0; // +0x70 — hack progress remaining (held USE counts it down; <0 = done)
	float m_flLastUse = 0;      // +0x74 — last USE time (held-USE continuity)
	float m_flNextSound = 0;    // +0x78 — next "hacking" sound time
};

LINK_ENTITY_TO_CLASS(ts_hack, CTSHack);

TYPEDESCRIPTION CTSHack::m_SaveData[] = {
	DEFINE_FIELD(CTSHack, hacksec, FIELD_INTEGER),
};
IMPLEMENT_SAVERESTORE(CTSHack, CTSObjective);

// The original Precache is empty (the hack sounds are precached globally); we precache them here for
// runtime safety (the model is precached by SET_MODEL).
void CTSHack::Precache()
{
	PRECACHE_SOUND("misc/hacking.wav");
	PRECACHE_SOUND("misc/hacked.wav");
}

// @Spawn__7CTSHack — the original spawns it INACTIVE (NODRAW, non-solid) + just thinks; the gamerules
// InitObjectives arms it via EnableObjective. That isn't built, so we arm it here (SLICE) to keep the
// hack functional.
void CTSHack::Spawn()
{
	Precache();
	pev->classname = MAKE_STRING("ts_hack");
	if (!FStringNull(pev->model))
		SET_MODEL(ENT(pev), STRING(pev->model));
	UTIL_SetOrigin(pev, pev->origin);
	SetThink(&CTSHack::WaitThink);
	pev->nextthink = pev->ltime + 1.0f;
	EnableObjective(true); // SLICE: the original defers arming to CTSGameRules::InitObjectives
}

bool CTSHack::KeyValue(KeyValueData *pkvd)
{
	if (FStrEq(pkvd->szKeyName, "hackingtime"))
	{
		hacksec = atoi(pkvd->szValue);
		return true;
	}
	return CTSObjective::KeyValue(pkvd);
}

// @EnableObjective — arm/disarm. Arming makes it a USE-able brush + seeds the objective deadline from
// the round timer; the hack progress (m_flHackProgress) only starts on the first USE.
void CTSHack::EnableObjective(bool enable)
{
	m_bActive = enable;
	if (enable)
	{
		pev->effects &= ~EF_NODRAW;
		pev->solid = SOLID_BBOX;       // RE solid=4; a USE-able solid
		pev->movetype = MOVETYPE_PUSH; // RE movetype=7
		pev->flags |= FL_WORLDBRUSH;   // RE flags |= 0x2000000
		if (!FStringNull(pev->model))
			SET_MODEL(ENT(pev), STRING(pev->model));
		CTSGameRules *gr = TSGameRules();
		m_flObjTimer = (gr && gr->m_flRoundEnd > 0) ? (gr->m_flRoundEnd - gpGlobals->time) : 0; // round seconds left
		m_flHackProgress = 0;
		m_flLastUse = 0;
		m_flNextSound = 0;
	}
	else
	{
		pev->effects = EF_NODRAW;
		pev->solid = SOLID_NOT;
		pev->flags = 0;
		pev->movetype = MOVETYPE_NONE;
		m_flObjTimer = 0;
		m_flHackProgress = 0;
		m_flLastUse = 0;
	}
}

// @WaitThink__7CTSHack — the OBJECTIVE deadline: while armed, count the round-relative timer down and
// freeze when it runs out (the attackers failed to finish the hack in time).
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

// @Use__7CTSHack — a player HOLDS USE on the panel: progress the hack (from `hacksec`), beeping each
// second; when the progress runs out the objective completes (fire target + HUD + "hacked" sound + freeze).
void CTSHack::Use(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value)
{
	if (!m_bActive || !pCaller || !pCaller->IsPlayer())
		return;
	CBasePlayer *pPlayer = (CBasePlayer *)pCaller;
	if (!CanInteract(pPlayer))
		return;

	if (m_flLastUse == 0.0f)
		m_flLastUse = gpGlobals->time;

	// held continuously (gap < 0.5s)? progress the hack.
	if (gpGlobals->time - m_flLastUse < 0.5f)
	{
		if (m_flHackProgress == 0.0f) // init the progress timer (RE: hacksec, capped 30, floored 1)
			m_flHackProgress = (hacksec < 30) ? (float)(hacksec < 1 ? 1 : hacksec) : 30.0f;
		else
			m_flHackProgress -= (gpGlobals->time - m_flLastUse); // count down by the elapsed time
	}

	if (m_flNextSound < gpGlobals->time) // the "hacking" sound, once a second
	{
		m_flNextSound = gpGlobals->time + 1.0f;
		EMIT_SOUND_DYN(edict(), CHAN_BODY, "misc/hacking.wav", RANDOM_FLOAT(0.92f, 1.0f), ATTN_NORM, 0,
			98 + RANDOM_LONG(0, 3)); // VERIFY exact channel/attn
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
		EMIT_SOUND_DYN(edict(), CHAN_BODY, "misc/hacked.wav", RANDOM_FLOAT(0.92f, 1.0f), ATTN_NORM, 0,
			98 + RANDOM_LONG(0, 3));
	}
}

//============================================================================
// CTSWinGiver — when Use'd `callsnumber` times, declares a team the winner. @0x8bf10.
// FAITHFUL: Use -> CTSGameRules::TeamWin(team, cash, …) + HUD. // SLICE (team subsystem
// not built): announce the win + fire the message-as-target; no cash/round restart.
//============================================================================
class CTSWinGiver : public CTSObjective
{
public:
	void Spawn() override;
	bool KeyValue(KeyValueData *pkvd) override;
	void EXPORT Use(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value) override;

	int m_winnerTeam = 0;     // winnerteam (0 Team1, 1 Team2, 2 draw, …)
	int m_victoryCash = 2000; // victorycash
	int m_defeatCash = -1000; // defeatcash
	int m_callsNumber = 1;    // calls needed to activate
	int m_calls = 0;          // calls so far
	bool m_bDone = false;
};

LINK_ENTITY_TO_CLASS(ts_wingiver, CTSWinGiver);

void CTSWinGiver::Spawn()
{
	pev->movetype = MOVETYPE_NONE;
	pev->solid = SOLID_NOT;
	pev->effects |= EF_NODRAW;
	m_calls = 0;
	m_bDone = false;
}

bool CTSWinGiver::KeyValue(KeyValueData *pkvd)
{
	if (FStrEq(pkvd->szKeyName, "winnerteam"))  { m_winnerTeam = atoi(pkvd->szValue); return true; }
	if (FStrEq(pkvd->szKeyName, "victorycash")) { m_victoryCash = atoi(pkvd->szValue); return true; }
	if (FStrEq(pkvd->szKeyName, "defeatcash"))  { m_defeatCash = atoi(pkvd->szValue); return true; }
	if (FStrEq(pkvd->szKeyName, "callsnumber")) { m_callsNumber = atoi(pkvd->szValue); return true; }
	return CTSObjective::KeyValue(pkvd);
}

// @Use__11CTSWinGiver — count the calls; on the last, declare the winner.
void CTSWinGiver::Use(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value)
{
	if (m_bDone)
		return;
	if (++m_calls < m_callsNumber)
		return;

	m_bDone = true;
	if (!FStringNull(pev->message))
		TSHudMessageAll(0, 0xff, 0, 8, 50, STRING(pev->message));

	// Declare the win through the game rules: scores the team + announces + restarts the round.
	if (TSGameRules())
		TSGameRules()->TeamWin(m_winnerTeam);
	// SLICE: faithful TeamWin also awards victory/defeat cash (m_victoryCash/m_defeatCash).
}

//============================================================================
// CTSScoreGiver — like CTSWinGiver, but REPEATABLE: each time it's Use'd `callsnumber`
// times it scores the team again (resets the call count instead of latching done). @Spawn
// 0x8c0b8 / Use 0x8c158. A "Respawn Players" spawnflag respawns everyone on score (here that
// is subsumed by TeamWin's round restart — SLICE).
//============================================================================
#define SF_SCOREGIVER_RESPAWN 1

class CTSScoreGiver : public CTSWinGiver
{
public:
	void Spawn() override;                                  // @0x8c0b8
	bool KeyValue(KeyValueData *pkvd) override;             // @0x8c090 — delegates to CTSWinGiver
	void EXPORT Use(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value) override;
};

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
	pev->effects |= EF_NODRAW;
	pev->solid = SOLID_NOT;
	pev->movetype = MOVETYPE_NONE;
	pev->nextthink = gpGlobals->time + 10.0f;
}

// @Use__13CTSScoreGiver — count the calls; on the last, score the team and re-arm (repeatable).
void CTSScoreGiver::Use(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value)
{
	if (++m_calls < m_callsNumber)
		return;
	m_calls = 0; // repeatable (CTSWinGiver latches m_bDone; the score giver re-arms)

	// Score the team directly (no round restart — a score giver gives points without ending the
	// round, unlike the win giver's TeamWin). // SLICE: the .so routes through TeamWin with the
	// team-subsystem cash; the "Respawn Players" spawnflag isn't applied.
	CTSGameRules *gr = TSGameRules();
	if (gr && m_winnerTeam >= 0 && m_winnerTeam < 4)
		gr->m_teamScore[m_winnerTeam]++;
	if (!FStringNull(pev->message))
		TSHudMessageAll(0, 0xff, 0, 8, 50, STRING(pev->message));
}

//============================================================================
// CVipEscape — the VIP safety zone (`func_vip_safetyzone`, brush). In the .so this is a passive
// marker the (unbuilt) VIP-escort logic checks against; it has no touch/use of its own. @Spawn
// 0x8d23c. Build it so a map placing it doesn't Can't-init. // SLICE: the VIP-escape win
// detection (who is the VIP, did they reach the zone) lives in the team subsystem, not built.
//============================================================================
class CVipEscape : public CBaseEntity
{
public:
	void Spawn() override
	{
		pev->movetype = MOVETYPE_NONE;
		pev->solid = SOLID_NOT; // passive marker (the .so SET_MODELs the brush, non-solid)
		if (!FStringNull(pev->model))
			SET_MODEL(edict(), STRING(pev->model));
	}
};

LINK_ENTITY_TO_CLASS(func_vip_safetyzone, CVipEscape);

