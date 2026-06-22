/***
*  ts_bomb.cpp — RECONSTRUCTED CTSBomb (cont.40), the defuse/wire-cut objective.
*  Functional/source-faithful (see CLAUDE.md). recovered/bomb/CTSBomb.c:
*    Spawn @0x8c470, Precache @0x8c570, SetRandomWires @0x8c598, EnableObjective @0x8c608,
*    CutWire @0x8c70c, WaitThink @0x8c860, Use @0x8c9a4, KeyValue @0x8cb68, ObjectCaps 0x10.
*
*  CTSBomb is a map-placed (`@SolidClass`, brush, base `tsobjective`) planted bomb: once
*  enabled it counts down (WaitThink, beeping faster as it nears zero) and EXPLODES unless
*  defused. Two defuse routes: hold USE for `defusetime` seconds (FCAP_CONTINUOUS_USE), or — if
*  `cutwires` — cut the right one of three wires (SetRandomWires: 1 = defuse, 2 = boobytrap →
*  the countdown goes fast, 3 = neutral → instant detonate). A defuse fires the target + HUD
*  message; detonation is a big radius blast.
*
*  FUNCTIONAL reconstruction onto the stock SDK (ExplosionCreate for the blast; the attacker-
*  team gate inlined like ts_objectives.cpp). // SLICE: the faithful enable is gamerules-driven
*  (InitObjectives) — here a map-placed bomb arms on Spawn; the wire-cut UI is cl_dll, so wires
*  are cut by the faithful `cutwire_<n>` prefix command; bomb countdown uses a fixed default.
***/
#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "player.h"
#include "gamerules.h"
#include "ts_gamerules.h"
#include "ts_utils.h"   // TSHudMessageAll
#include "explode.h"    // ExplosionCreate

#define BOMB_TEAM_ANY (-1)
#define BOMB_COUNTDOWN 45.0f   // VERIFY: the .so reads a bomb-time global (DAT_001781a8)

// wire states (the .so's +0x7c..0x7e): 1 defuse, 2 boobytrap, 3 neutral, 0 already cut
enum tsbomb_wire_t { WIRE_CUT = 0, WIRE_DEFUSE = 1, WIRE_BOOBYTRAP = 2, WIRE_NEUTRAL = 3 };

class CTSBomb : public CBaseEntity
{
public:
	bool KeyValue(KeyValueData *pkvd) override;
	void Spawn() override;
	void Precache() override;
	int  ObjectCaps() override { return FCAP_CONTINUOUS_USE; } // @ObjectCaps __7CTSBomb == 0x10
	void Use(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value) override;

	void EXPORT WaitThink();          // the bomb tick: countdown + beep, or explode at zero
	void EnableObjective(bool enable);
	void SetRandomWires();
	void CutWire(int wire, CBasePlayer *pPlayer);
	void Explode();

	bool CanInteract(int team) const { return m_attackerTeam == BOMB_TEAM_ANY || m_attackerTeam == team; }

	bool   m_bEnabled = false;     // +0x64 — the bomb is armed + ticking
	float  m_flTimer = 0;          // +0x6c — countdown remaining (seconds)
	float  m_flDefuseTime = 10;    // +0x80 — USE-hold seconds to defuse (FGD defusetime)
	int    m_bCutWires = 1;        // FGD cutwires
	int    m_attackerTeam = BOMB_TEAM_ANY; // tsobjective attackerteam (-1 = any)

	int    m_wires[3] = {};        // +0x7c..0x7e
	bool   m_bFast = false;        // +0x7f — boobytrap sped the countdown up
	float  m_flUseProgress = 0;    // +0x74 — remaining USE-defuse time
	float  m_flLastUse = 0;        // +0x78 — last USE tick (continuity check)
	float  m_flNextBeep = 0;       // +0x70
};

LINK_ENTITY_TO_CLASS(ts_bomb, CTSBomb)

void CTSBomb::Precache()
{
	PRECACHE_SOUND("misc/beep2.wav");
}

// @ KeyValue — defusetime / cutwires / target / message (+ attackerteam from tsobjective).
bool CTSBomb::KeyValue(KeyValueData *pkvd)
{
	if (FStrEq(pkvd->szKeyName, "defusetime")) { m_flDefuseTime = atof(pkvd->szValue); return true; }
	if (FStrEq(pkvd->szKeyName, "cutwires"))   { m_bCutWires = atoi(pkvd->szValue); return true; }
	if (FStrEq(pkvd->szKeyName, "attackerteam")) { m_attackerTeam = atoi(pkvd->szValue); return true; }
	return CBaseEntity::KeyValue(pkvd);
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
	m_bEnabled = enable;
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
	if (!m_bEnabled || !m_bCutWires || m_flTimer <= 0.0f || wire < 0 || wire > 2)
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
	if (!m_bEnabled || !pActivator || !pActivator->IsPlayer() || !pActivator->IsAlive())
		return;
	CBasePlayer *pPlayer = (CBasePlayer *)pActivator;
	if (!CanInteract(pPlayer->pev->team))
		return;

	// continuity: if USE lapsed >0.5s the hold restarts
	if (m_flLastUse == 0.0f || gpGlobals->time - m_flLastUse >= 0.5f)
		m_flUseProgress = m_flDefuseTime;
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

	if (!m_bEnabled)
		return;

	m_flTimer -= 1.0f;
	if (m_flTimer <= 0.0f)
	{
		Explode();
		return;
	}

	if (gpGlobals->time >= m_flNextBeep)
	{
		m_flNextBeep = gpGlobals->time + interval;
		EMIT_SOUND_DYN(edict(), CHAN_VOICE, "misc/beep2.wav", 1.0, ATTN_NORM, 0, 98 + RANDOM_LONG(0, 3));
	}
}

// @ detonation — a big radius blast (the .so's TSExplosion), then the bomb is spent.
void CTSBomb::Explode()
{
	// ExplosionCreate spawns the env_explosion sprite/sound + does the radius blast damage.
	ExplosionCreate(Center(), pev->angles, edict(), 300, true);
	EnableObjective(false);
}

// Free helper for the wire-cut console command (the cl_dll wire UI isn't built).
void TSCutWire(CBasePlayer *pPlayer, int wire)
{
	CBaseEntity *pEnt = nullptr;
	while ((pEnt = UTIL_FindEntityByClassname(pEnt, "ts_bomb")) != nullptr)
		((CTSBomb *)pEnt)->CutWire(wire, pPlayer);
}

