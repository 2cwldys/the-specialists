/***
*  ts_timer.cpp — RECONSTRUCTED CTSTimer from ts_i386.so + ts_fgd.fgd.
*  Functional/source-faithful (see CLAUDE.md). Raw decompile in
*  recovered/server/CTSTimer.c; prototypes in reference/signatures-gcc2.txt.
*
*  @PointClass base(Target, Targetname) = ts_timer
*    spawnflags: 1 "Start OFF", 2 "Loop"
*    "duration" keyvalue -> m_flDuration
*
*  A relay timer: every m_flDuration seconds it fires pev->target and shows
*  pev->message, until toggled off. Use toggles it on/off.
***/
#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "ts_utils.h"

#define SF_TIMER_START_OFF 1   // FGD bit 1
#define SF_TIMER_LOOP      2   // FGD bit 2

class CTSTimer : public CBaseEntity
{
public:
	void Spawn(void);
	bool KeyValue(KeyValueData *pkvd);
	void Use(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value);
	void EXPORT WaitThink(void);
	void ToggleTimer(void);

	float m_flDuration;   // this+0x64 — "duration" keyvalue (seconds)
	int   m_iActive;      // this+0x68 — running toggle state
};

LINK_ENTITY_TO_CLASS(ts_timer, CTSTimer);

bool CTSTimer::KeyValue(KeyValueData *pkvd)
{
	if (FStrEq(pkvd->szKeyName, "duration"))
	{
		m_flDuration = atof(pkvd->szValue);
		return true;
	}
	return CBaseEntity::KeyValue(pkvd);
}

void CTSTimer::Spawn(void)
{
	pev->classname = MAKE_STRING("ts_timer");
	pev->effects |= EF_NODRAW;
	// Original writes two pev fields (0x108=5, 0x10c=0); a timer neither moves
	// nor collides, so the behaviourally-equivalent point-entity setup: (VERIFY)
	pev->solid = SOLID_NOT;
	pev->movetype = MOVETYPE_NONE;

	m_iActive = 0;
	SetThink(NULL);

	if (!(pev->spawnflags & SF_TIMER_START_OFF))
		ToggleTimer();
	else
		SetThink(NULL);
}

// @ ToggleTimer__8CTSTimer
void CTSTimer::ToggleTimer(void)
{
	if (m_flDuration < 1.0f)
	{
		SetThink(NULL);
		m_iActive = 0;
		return;
	}

	m_iActive ^= 1;
	if (!m_iActive)
	{
		SetThink(NULL);
	}
	else
	{
		SetThink(&CTSTimer::WaitThink);
		pev->nextthink = gpGlobals->time + m_flDuration;
	}
}

// @ WaitThink__8CTSTimer
void CTSTimer::WaitThink(void)
{
	if (pev->target)
		FireTargets(STRING(pev->target), this, this, USE_TOGGLE, 0);

	if (pev->message)
		TSHudMessageAll(0, 255, 0, 1, 30, STRING(pev->message));

	// Original: if (pev->spawnflags & 1) clear think, then reschedule.
	// (FGD bit 1 = "Start OFF"; reproduced faithfully — VERIFY vs "Loop".)
	if (pev->spawnflags & SF_TIMER_START_OFF)
		SetThink(NULL);

	pev->nextthink = gpGlobals->time + m_flDuration;
}

void CTSTimer::Use(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value)
{
	ToggleTimer();
}
