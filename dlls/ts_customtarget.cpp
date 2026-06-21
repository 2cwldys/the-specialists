/***
*  ts_customtarget.cpp — RECONSTRUCTED CTSCustomTarget from ts_i386.so + ts_fgd.fgd.
*  Functional/source-faithful (see CLAUDE.md). Raw decompile in
*  recovered/server/CTSCustomTarget.c; prototypes in reference/signatures-gcc2.txt.
*
*  @PointClass base(Targetname) = ts_customtarget : "TS custom target"
*    "targets"       — target list, ';' separated     -> m_szTargets[]
*    "accumulate"    — # of Use calls before it fires  -> m_iAccumulate (default 1)
*    "triggermethod" — 0 all / 1 random / 2 sequential -> m_iTriggerMethod
*    "message"       — HUD message shown on fire       -> pev->message (base KeyValue)
*
*  A relay that, after being Used "accumulate" times, broadcasts an optional HUD
*  message and fires its stored targets according to triggermethod.
***/
#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "ts_utils.h"

// Original storage: a 0x800-byte block at this+0x64, 0x20 bytes/slot -> 64 slots
// of 32 chars (recovered offsets are provenance, not targets).
#define TS_MAX_TARGETS 64
#define TS_TARGET_LEN  32

// triggermethod values (FGD "fire method" choices)
#define TS_FIRE_ALL    0
#define TS_FIRE_RANDOM 1
#define TS_FIRE_SEQ    2

class CTSCustomTarget : public CBaseEntity
{
public:
	void Spawn(void);
	bool KeyValue(KeyValueData *pkvd);
	void Use(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value);

	void GetTargets(const char *pTargets);
	void FireTarget(int index);

	char m_szTargets[TS_MAX_TARGETS][TS_TARGET_LEN]; // this+0x64 — parsed target names
	int  m_iNumTargets;                              // this+0x864 — parsed count
	int  m_iTriggerMethod;                           // this+0x868 — 0/1/2
	int  m_iAccumulate;                              // this+0x86c — calls before fire
	int  m_iCurCount;                               // this+0x870 — Use counter
	int  m_iSeqIndex;                               // this+0x874 — sequential cursor
};

LINK_ENTITY_TO_CLASS(ts_customtarget, CTSCustomTarget);

// @ GetTargets__15CTSCustomTargetPc — split a ';'-separated list into m_szTargets.
void CTSCustomTarget::GetTargets(const char *pTargets)
{
	memset(m_szTargets, 0, sizeof(m_szTargets));
	m_iNumTargets = 0;

	if (!pTargets)
		return;

	const char *p = pTargets;
	while (*p && m_iNumTargets < TS_MAX_TARGETS)
	{
		int n = 0;
		while (*p && *p != ';' && n < TS_TARGET_LEN - 1)
			m_szTargets[m_iNumTargets][n++] = *p++;
		m_szTargets[m_iNumTargets][n] = '\0';
		m_iNumTargets++;

		// discard any overflow of an over-long token, then the separator
		while (*p && *p != ';')
			p++;
		if (*p == ';')
			p++;
	}
}

bool CTSCustomTarget::KeyValue(KeyValueData *pkvd)
{
	if (FStrEq(pkvd->szKeyName, "targets"))
	{
		GetTargets(pkvd->szValue);
		return true;
	}
	if (FStrEq(pkvd->szKeyName, "accumulate"))
	{
		m_iAccumulate = atoi(pkvd->szValue);
		return true;
	}
	if (FStrEq(pkvd->szKeyName, "triggermethod"))
	{
		m_iTriggerMethod = atoi(pkvd->szValue);
		return true;
	}
	// "message" -> pev->message, plus targetname/etc.
	return CBaseEntity::KeyValue(pkvd);
}

void CTSCustomTarget::Spawn(void)
{
	pev->classname = MAKE_STRING("ts_customtarget");
	pev->effects |= EF_NODRAW;
	// Original writes pev 0x108=5, 0x10c=0; like ts_timer this point relay neither
	// moves nor collides, so the behaviourally-equivalent setup: (VERIFY)
	pev->solid = SOLID_NOT;
	pev->movetype = MOVETYPE_NONE;

	m_iCurCount = 0;
	m_iSeqIndex = 0;
	// m_iAccumulate is zero-initialised by the engine; the FGD always emits
	// "accumulate" (default 1). With 0, Use still fires every call (== 1).
}

// @ FireTarget__15CTSCustomTargeti — index<0 or out of range fires ALL targets.
void CTSCustomTarget::FireTarget(int index)
{
	if (index < 0 || index >= m_iNumTargets)
	{
		for (int i = 0; i < m_iNumTargets; i++)
			FireTargets(m_szTargets[i], this, this, USE_TOGGLE, 0);
	}
	else
	{
		FireTargets(m_szTargets[index], this, this, USE_TOGGLE, 0);
	}
}

void CTSCustomTarget::Use(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value)
{
	if (++m_iCurCount < m_iAccumulate)
		return;

	m_iCurCount = 0;

	if (pev->message)
		TSHudMessageAll(0, 255, 0, 1, 30, STRING(pev->message));

	switch (m_iTriggerMethod)
	{
	case TS_FIRE_RANDOM:
		FireTarget(RANDOM_LONG(0, m_iNumTargets - 1));
		break;

	case TS_FIRE_SEQ:
	{
		int idx = m_iSeqIndex++;
		FireTarget(idx);
		if (m_iSeqIndex >= m_iNumTargets)
			m_iSeqIndex = 0;
		break;
	}

	case TS_FIRE_ALL:
	default:
		FireTarget(-1);
		break;
	}
}
