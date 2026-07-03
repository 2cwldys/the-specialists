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
#include "ts_hack.h"       // CTSHack
#include "ts_wingiver.h"   // CTSWinGiver / CTSScoreGiver
#include "explode.h"       // ExplosionCreate (CTSBomb)
#include "ts_bomb.h"
#include "ts_escape.h"
#include "ts_flag.h"       // s_flag_status (must precede ts_briefcase.h) + CTSFlag::CreateFlag
#include "ts_briefcase.h"
#include "UserMessages.h"  // gmsgScoreInfo, gmsgTeamScore (CTSBriefcase/CTSDMHill scoring)

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

// @0x8c310 Freeze — enable/disable the objective + re-render it on the HUD.
void CTSObjective::Freeze(bool bFreeze)
{
	m_bActive = !bFreeze;
	if (TSGameRules())
		TSGameRules()->ShowObjectiveOnHud(this, !bFreeze);
}

//============================================================================
// CTSObjectiveManager — point entity that holds the map's objective-pointer targets +
// the per-slot objective messages; fires them on Reset(). @0x8b7bc.
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

void CTSObjectiveManager::Spawn()
{
	pev->movetype = MOVETYPE_NONE;
	pev->solid = SOLID_NOT;
	pev->effects |= EF_NODRAW;
}

bool CTSObjectiveManager::KeyValue(KeyValueData *pkvd)
{
	// objptargeta..h -> targets[0..7]
	if (!strncmp(pkvd->szKeyName, "objptarget", 10))
	{
		int idx = pkvd->szKeyName[10] - 'a';
		if (idx >= 0 && idx < TS_OBJ_TARGETS)
		{
			targets[idx] = ALLOC_STRING(pkvd->szValue);
			return true;
		}
	}
	// messagea..d -> messages[0..3]
	if (!strncmp(pkvd->szKeyName, "message", 7) && pkvd->szKeyName[7] >= 'a' && pkvd->szKeyName[7] <= 'd')
	{
		messages[pkvd->szKeyName[7] - 'a'] = ALLOC_STRING(pkvd->szValue);
		return true;
	}
	if (FStrEq(pkvd->szKeyName, "targetontimeleft")) { end_round_target = ALLOC_STRING(pkvd->szValue); return true; }
	if (FStrEq(pkvd->szKeyName, "targetonspecdead")) { m_targetSpecDead = ALLOC_STRING(pkvd->szValue); return true; }
	if (FStrEq(pkvd->szKeyName, "targetonmercdead")) { m_targetMercDead = ALLOC_STRING(pkvd->szValue); return true; }
	return CBaseEntity::KeyValue(pkvd);
}

// @Reset — announce each non-empty message + fire each objective-pointer target.
void CTSObjectiveManager::Reset()
{
	for (int i = 0; i < TS_OBJ_MESSAGES; i++)
		if (!FStringNull(messages[i]))
			TSHudMessageAll(0, 0xff, 0, 8, 30, STRING(messages[i]));

	for (int i = 0; i < TS_OBJ_TARGETS; i++)
		if (!FStringNull(targets[i]))
			FireTargets(STRING(targets[i]), this, this, USE_TOGGLE, 0);
}

// Fire the objective manager's `targetontimeleft` when the round timer expires (called from
// CTSGameRules::RoundTimeLeft). No-op if there's no manager / target.
void TSFireTimeLeftTarget()
{
	CBaseEntity *pEnt = UTIL_FindEntityByClassname(nullptr, "ts_objective_manager");
	if (!pEnt)
		return;
	CTSObjectiveManager *mgr = (CTSObjectiveManager *)pEnt;
	if (!FStringNull(mgr->end_round_target))
		FireTargets(STRING(mgr->end_round_target), mgr, mgr, USE_TOGGLE, 0);
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
	void Use(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value) override; // @0x8bbd8

	string_t m_objMsg[5] = {}; // objmsgspec/merc/team3/team4/all (per-team pointer label)
};

LINK_ENTITY_TO_CLASS(ts_objective_ptr, CTSObjectivePointer);

void CTSObjectivePointer::Spawn()
{
	pev->movetype = MOVETYPE_NONE;
	pev->solid = SOLID_NOT;
	pev->effects |= EF_NODRAW;
}

// @Use__19CTSObjectivePointer — activate the objective(s) this pointer targets: enable each, add it
// to the HUD objective list carrying this pointer's per-team labels, and send it to every client.
void CTSObjectivePointer::Use(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value)
{
	if (FStringNull(pev->target) || !TSGameRules())
		return;
	CBaseEntity *pEnt = nullptr;
	while ((pEnt = UTIL_FindEntityByTargetname(pEnt, STRING(pev->target))) != nullptr)
	{
		if (!pEnt->IsObjective())
			continue;
		CTSObjective *pObj = (CTSObjective *)pEnt;
		pObj->EnableObjective(true);
		// the 4 per-team labels (spec/merc/team3/team4) — the 5th (objmsgall) isn't part of the list entry.
		int labels[4] = { (int)m_objMsg[0], (int)m_objMsg[1], (int)m_objMsg[2], (int)m_objMsg[3] };
		int idx = TSGameRules()->AddObjectiveInList(pObj, labels);
		if (idx == -1)
			continue;
		for (int i = 1; i <= gpGlobals->maxClients; i++)
		{
			CBasePlayer *p = (CBasePlayer *)UTIL_PlayerByIndex(i);
			if (p)
				TSGameRules()->SendObjectiveToHud(idx, p);
		}
	}
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
// CTSHack is declared in ts_hack.h (extracted so the audit sees its FGD member `hacksec`).

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
// FAITHFUL: Use -> the gamerules TeamWin (team, cash, …) + HUD. // SLICE (team subsystem
// not built): announce the win + fire the message-as-target; no cash/round restart.
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
IMPLEMENT_SAVERESTORE(CTSWinGiver, CTSObjective);

// @EnableObjective — the original ignores the arg and just RESETs (clears the active flag + the call
// state), regardless of enable. VERIFY: it zeroes field_0x64 (m_bActive) + a byte at field_0x68.
void CTSWinGiver::EnableObjective(bool enable)
{
	m_bActive = false;
	m_calls = 0;
	m_bDone = false;
}

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
	if (FStrEq(pkvd->szKeyName, "winnerteam"))  { winnerteam = atoi(pkvd->szValue); return true; }
	if (FStrEq(pkvd->szKeyName, "victorycash")) { cash_for_winner = atoi(pkvd->szValue); return true; }
	if (FStrEq(pkvd->szKeyName, "defeatcash"))  { cash_for_loser = atoi(pkvd->szValue); return true; }
	if (FStrEq(pkvd->szKeyName, "callsnumber")) { numberofcall = atoi(pkvd->szValue); return true; }
	return CTSObjective::KeyValue(pkvd);
}

// @Use__11CTSWinGiver — count the calls; on the last, declare the winner.
void CTSWinGiver::Use(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value)
{
	if (m_bDone)
		return;
	if (++m_calls < numberofcall)
		return;

	m_bDone = true;
	if (!FStringNull(pev->message))
		TSHudMessageAll(0, 0xff, 0, 8, 50, STRING(pev->message));

	// Declare the win through the game rules: scores the team + announces + restarts the round.
	if (TSGameRules())
		TSGameRules()->TeamWin(winnerteam);
	// SLICE: faithful TeamWin also awards victory/defeat cash (cash_for_winner/cash_for_loser).
}

//============================================================================
// CTSScoreGiver — like CTSWinGiver, but REPEATABLE: each time it's Use'd `callsnumber`
// times it scores the team again (resets the call count instead of latching done). @Spawn
// 0x8c0b8 / Use 0x8c158. A "Respawn Players" spawnflag respawns everyone on score (here that
// is subsumed by TeamWin's round restart — SLICE).
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
	pev->effects |= EF_NODRAW;
	pev->solid = SOLID_NOT;
	pev->movetype = MOVETYPE_NONE;
	pev->nextthink = gpGlobals->time + 10.0f;
}

// @Use__13CTSScoreGiver — count the calls; on the last, score the team and re-arm (repeatable).
void CTSScoreGiver::Use(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value)
{
	if (++m_calls < numberofcall)
		return;
	m_calls = 0; // repeatable (CTSWinGiver latches m_bDone; the score giver re-arms)

	// Score the team directly (no round restart — a score giver gives points without ending the
	// round, unlike the win giver's TeamWin). // SLICE: the .so routes through TeamWin with the
	// team-subsystem cash; the "Respawn Players" spawnflag isn't applied.
	CTSGameRules *gr = TSGameRules();
	if (gr && winnerteam >= 0 && winnerteam < 4)
		gr->m_teamScore[winnerteam]++;
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
	void Spawn() override;                          // @0x8d23c
	void Precache() override;                       // @0x8d238 — empty
	bool KeyValue(KeyValueData *pkvd) override;     // @0x8d22c — handles no keys
};

LINK_ENTITY_TO_CLASS(func_vip_safetyzone, CVipEscape);

// @0x8d22c — CVipEscape ignores all keyvalues (the original just clears fHandled and returns;
// it does NOT chain to CBaseEntity::KeyValue).
bool CVipEscape::KeyValue(KeyValueData *pkvd)
{
	pkvd->fHandled = false;
	return false;
}

// @0x8d238 — empty.
void CVipEscape::Precache()
{
}

// @0x8d23c — passive non-solid brush marker (EF_NODRAW, SOLID_NOT, MOVETYPE_NONE).
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
#define BOMB_TEAM_ANY (-1)
#define BOMB_COUNTDOWN 45.0f   // VERIFY: the .so reads a bomb-time global (DAT_001781a8)

// wire states (the .so's +0x7c..0x7e): 1 defuse, 2 boobytrap, 3 neutral, 0 already cut
enum tsbomb_wire_t { WIRE_CUT = 0, WIRE_DEFUSE = 1, WIRE_BOOBYTRAP = 2, WIRE_NEUTRAL = 3 };

LINK_ENTITY_TO_CLASS(ts_bomb, CTSBomb)

TYPEDESCRIPTION CTSBomb::m_SaveData[] = {
	DEFINE_FIELD(CTSBomb, defusesec, FIELD_INTEGER),
	DEFINE_FIELD(CTSBomb, cancutwire, FIELD_INTEGER),
};
IMPLEMENT_SAVERESTORE(CTSBomb, CTSObjective);

void CTSBomb::Precache()
{
	PRECACHE_SOUND("misc/beep2.wav");
}

// @ KeyValue — defusetime / cutwires (+ attackerteam from the CTSObjective base).
bool CTSBomb::KeyValue(KeyValueData *pkvd)
{
	if (FStrEq(pkvd->szKeyName, "defusetime")) { defusesec = atoi(pkvd->szValue); return true; }
	if (FStrEq(pkvd->szKeyName, "cutwires"))   { cancutwire = atoi(pkvd->szValue); return true; }
	return CTSObjective::KeyValue(pkvd); // handles "attackerteam"
}

// @ Spawn — set up the brush and arm the bomb. (Faithful enable is gamerules-driven; a
// map-placed bomb arms itself here — SLICE.)
void CTSBomb::Spawn()
{
	// misc/beep2.wav is precached at map load (PrecacheAllTSWeapons) — the bomb can be armed at
	// runtime (gamerules / synthetic), where a PRECACHE_SOUND would Host_Error.
	if (!FStringNull(pev->model))
		SET_MODEL(edict(), STRING(pev->model)); // brush model
	UTIL_SetOrigin(pev, pev->origin);
	EnableObjective(true);
}

// @ SetRandomWires — 3 wires: one defuse (1), one boobytrap (2), the rest neutral (3).
void CTSBomb::SetRandomWires()
{
	m_wires[0] = m_wires[1] = m_wires[2] = WIRE_NEUTRAL;
	int defuse = RANDOM_LONG(0, 2);
	m_wires[defuse] = WIRE_DEFUSE;
	int trap;
	do { trap = RANDOM_LONG(0, 2); } while (m_wires[trap] == WIRE_DEFUSE);
	m_wires[trap] = WIRE_BOOBYTRAP;
}

// @ EnableObjective — arm (true) or safe (false) the bomb.
void CTSBomb::EnableObjective(bool enable)
{
	m_bActive = enable;
	m_bFast = false;

	if (!enable)
	{
		pev->solid = SOLID_NOT;
		m_flTimer = 0;
		m_flUseProgress = 0;
		m_flNextBeep = 0;
		SetThink(NULL);
		pev->nextthink = 0;
		return;
	}

	pev->solid = SOLID_BSP;
	m_flTimer = BOMB_COUNTDOWN;
	m_flUseProgress = 0;
	m_flLastUse = 0;
	m_flNextBeep = 0;
	SetRandomWires();
	SetThink(&CTSBomb::WaitThink);
	pev->nextthink = gpGlobals->time + 1.0f;
}

// @ CutWire — cut wire `wire` (0..2): defuse / boobytrap (fast) / neutral (instant detonate).
void CTSBomb::CutWire(int wire, CBasePlayer *pPlayer)
{
	if (!m_bActive || !cancutwire || m_flTimer <= 0.0f || wire < 0 || wire > 2)
		return;
	if (m_wires[wire] == WIRE_CUT || pPlayer == nullptr)
		return;

	if (m_wires[wire] == WIRE_NEUTRAL)
	{
		// cut a dead wire → the timer runs out now
		m_wires[wire] = WIRE_CUT;
		m_flTimer = 0.01f;
	}
	else if (m_wires[wire] == WIRE_BOOBYTRAP)
	{
		// boobytrap → the countdown speeds up
		m_bFast = true;
		m_wires[wire] = WIRE_CUT;
	}
	else if (m_wires[wire] == WIRE_DEFUSE)
	{
		// the right wire → defused
		m_wires[wire] = WIRE_CUT;
		if (!FStringNull(pev->target))
			FireTargets(STRING(pev->target), this, this, USE_TOGGLE, 0);
		if (!FStringNull(pev->message))
			TSHudMessageAll(0, 0xff, 0, 8, 30, (char *)STRING(pev->message));
		EnableObjective(false);
	}
}

// @ Use — hold USE to defuse: accumulate continuous-use time; when it reaches defusetime, the
// bomb is defused (fire target + message). The engine calls this each frame while +USE is held
// (FCAP_CONTINUOUS_USE).
void CTSBomb::Use(CBaseEntity *pActivator, CBaseEntity *, USE_TYPE, float)
{
	if (!m_bActive || !pActivator || !pActivator->IsPlayer() || !pActivator->IsAlive())
		return;
	CBasePlayer *pPlayer = (CBasePlayer *)pActivator;
	if (!CanInteract(pPlayer->pev->team))
		return;

	// continuity: if USE lapsed >0.5s the hold restarts
	if (m_flLastUse == 0.0f || gpGlobals->time - m_flLastUse >= 0.5f)
		m_flUseProgress = defusesec;
	else
		m_flUseProgress -= (gpGlobals->time - m_flLastUse);
	m_flLastUse = gpGlobals->time;

	float left = m_flUseProgress < 0 ? 0 : m_flUseProgress;
	ClientPrint(pPlayer->pev, HUD_PRINTCENTER, UTIL_VarArgs("%i seconds left", (int)(left + 0.5f)));

	if (m_flUseProgress < 0.0f)
	{
		if (!FStringNull(pev->target))
			FireTargets(STRING(pev->target), this, this, USE_TOGGLE, 0);
		if (!FStringNull(pev->message))
			TSHudMessageAll(0, 0xff, 0, 8, 30, (char *)STRING(pev->message));
		EnableObjective(false);
	}
}

// @ WaitThink — the bomb tick. Counts down (fast = 0.1s ticks after a boobytrap), beeps with a
// rising pitch, and detonates at zero.
void CTSBomb::WaitThink()
{
	float interval = m_bFast ? 0.1f : 1.0f;
	pev->nextthink = gpGlobals->time + interval;

	if (!m_bActive)
		return;

	m_flTimer -= 1.0f;
	if (m_flTimer <= 0.0f)
	{
		// detonate inline (the original has no separate Explode method): a big radius blast, then spent.
		ExplosionCreate(Center(), pev->angles, edict(), 300, true);
		EnableObjective(false);
		return;
	}

	if (gpGlobals->time >= m_flNextBeep)
	{
		m_flNextBeep = gpGlobals->time + interval;
		EMIT_SOUND_DYN(edict(), CHAN_VOICE, "misc/beep2.wav", 1.0, ATTN_NORM, 0, 98 + RANDOM_LONG(0, 3));
	}
}



// ===================== CTSTeamEscape (merged from ts_escape.cpp) =====================

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
