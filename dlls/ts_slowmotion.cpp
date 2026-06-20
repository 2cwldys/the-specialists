/***
*
*  ts_slowmotion.cpp — RECONSTRUCTED from ts_i386.so (The Specialists).
*
*  Status: FIRST-DRAFT reconstruction. Behaviour recovered from the decompiled
*  CTSSlowMotion / CTSSlowMotionPoint methods + ts_fgd.fgd. NOT yet in the build
*  (not listed in dlls/CMakeLists.txt) and NOT yet ABI-verified. Depends on
*  classes not reconstructed yet: CTSGameRules::GlobalSlowMotion2(), the
*  TS-extended player slow-motion virtuals, TSHudMessageAll(). Marked VERIFY
*  where the binary was ambiguous.
*
*  Source of truth: reference/ts_i386.so (GCC2, not stripped) — see
*  recovered/server/CTSSlowMotion.c for the raw decompile and
*  reference/signatures-gcc2.txt for the prototypes.
*
*  Recovered entity (ts_fgd.fgd):
*    @SolidClass base(Targetname,Target) = ts_slowmotion
*      duration(string) default "5"      -> m_flDuration
*      slowamt(string)  default "0.5"    -> m_flSlowAmount
*      spawnflags: 1 "Start ON", 2 "All map", 4 "Slow bullets"
*
****/

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "player.h"
#include "gamerules.h"

#define SF_SLOMO_START_ON     1   // spawnflag bit 1
#define SF_SLOMO_ALL_MAP      2   // spawnflag bit 2  -> m_iAllMap
#define SF_SLOMO_SLOW_BULLETS 4   // spawnflag bit 4  -> m_bSlowBullets

#define TS_SLOMO_MAX_PLAYERS  33  // 0x21 — gpGlobals->maxClients + 1

// Player slow-motion is applied through TS-extended CBasePlayer virtuals.
// In ts_i386.so these are called by vtable offset; exact names are VERIFY:
//   vtbl+0x14 (player)->SetSlowMotion(float scale, bool slowBullets)
//   vtbl+0x18 (player)->IsSlowMotion() -> bool        (skip if already slowed)
// 1.0f scale == restore to normal speed.

class CTSSlowMotion : public CBaseEntity
{
public:
	void Spawn(void);
	void KeyValue(KeyValueData *pkvd);
	void EXPORT WaitThink(void);
	void EXPORT WaitTouch(CBaseEntity *pOther);
	void Use(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value);
	int  ObjectCaps(void) { return 0; }   // @ ObjectCaps__13CTSSlowMotion returns 0

	void ActivateSlow(void);
	void DeactivateSlow(void);
	void CheckPlayersDistance(void);

	virtual int Save(CSave &save);
	virtual int Restore(CRestore &restore);
	static TYPEDESCRIPTION m_SaveData[2];

	// --- recovered members (offsets on `this`, from the decompile) ---
	int            m_iToggledOn;     // 0x64  toggle state (infinite-duration mode)
	float          m_flStopTime;     // 0x68  gpGlobals->time + duration; 0 == inactive
	CBaseEntity   *m_pPlayers[TS_SLOMO_MAX_PLAYERS]; // 0x70  players currently slowed
	float          m_flRadius;       // 0xf4  per-player range check (VERIFY: set in Spawn)
	float          m_flDuration;     // 0xf8  "duration" keyvalue
	float          m_flSlowAmount;   // 0xfc  "slowamt" keyvalue (speed scale)
	int            m_iAllMap;        // 0x100 SF_SLOMO_ALL_MAP -> global slowmo
	int            m_bSlowBullets;   // 0x104 SF_SLOMO_SLOW_BULLETS
};

LINK_ENTITY_TO_CLASS(ts_slowmotion, CTSSlowMotion);

TYPEDESCRIPTION CTSSlowMotion::m_SaveData[2] =
{
	// VERIFY: exact saved fields — the binary saves 2 fields via WriteFields.
	DEFINE_FIELD(CTSSlowMotion, m_flDuration, FIELD_FLOAT),
	DEFINE_FIELD(CTSSlowMotion, m_flSlowAmount, FIELD_FLOAT),
};
IMPLEMENT_SAVERESTORE(CTSSlowMotion, CBaseEntity);

void CTSSlowMotion::KeyValue(KeyValueData *pkvd)
{
	if (FStrEq(pkvd->szKeyName, "duration"))
	{
		m_flDuration = atof(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "slowamt"))
	{
		m_flSlowAmount = atof(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else
	{
		CBaseEntity::KeyValue(pkvd);
	}
}

void CTSSlowMotion::Spawn(void)
{
	// Brush trigger: classname set, model from the brush, size from the brush.
	pev->classname = MAKE_STRING("ts_slowmotion");
	SET_MODEL(ENT(pev), STRING(pev->model));
	UTIL_SetSize(pev, pev->mins, pev->maxs);

	// VERIFY: the decompiler truncated after UTIL_SetSize; the following is the
	// standard brush-trigger setup + spawnflag latch inferred from the FGD and
	// the members the other methods read.
	pev->solid = SOLID_NOT;            // not solid; touch via SetTouch when armed
	pev->movetype = MOVETYPE_NONE;
	pev->effects |= EF_NODRAW;

	m_iAllMap     = (pev->spawnflags & SF_SLOMO_ALL_MAP) ? 1 : 0;
	m_bSlowBullets = (pev->spawnflags & SF_SLOMO_SLOW_BULLETS) ? 1 : 0;
	m_iToggledOn  = 0;
	m_flStopTime  = 0;

	if (pev->spawnflags & SF_SLOMO_START_ON)
		ActivateSlow();
}

// Use handler (button/trigger). @ Use__13CTSSlowMotion...USE_TYPEf
void CTSSlowMotion::Use(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value)
{
	ActivateSlow();

	if (!m_iAllMap)
	{
		SetTouch(&CTSSlowMotion::WaitTouch);   // this+0x14/0x18 = m_pfnTouch pmf
	}

	if (pev->message)   // pev+0x1d4 (VERIFY: pev->message)
		TSHudMessageAll(0, 255, 0, 8, 30, STRING(pev->message));

	if (pev->target)    // pev+0x1c8 (VERIFY: pev->target)
		FireTargets(STRING(pev->target), this, this, USE_TOGGLE, 0);
}

// @ ActivateSlow__13CTSSlowMotion (the 126-byte impl @ 0x71384)
void CTSSlowMotion::ActivateSlow(void)
{
	if (m_flDuration > 0.0f)
	{
		m_flStopTime = gpGlobals->time + m_flDuration;
	}
	else
	{
		// duration <= 0: toggle on/off
		m_flStopTime = 0;
		m_iToggledOn ^= 1;
		if (m_iToggledOn == 0)
			DeactivateSlow();
	}

	if (!m_iAllMap)
		SetTouch(&CTSSlowMotion::WaitTouch);
}

// @ DeactivateSlow__13CTSSlowMotion (the 271-byte impl @ 0x71078)
void CTSSlowMotion::DeactivateSlow(void)
{
	m_flStopTime = 0;

	if (!m_iAllMap)
	{
		// restore every tracked player's normal speed, then clear the table
		for (int i = 0; i < TS_SLOMO_MAX_PLAYERS; i++)
		{
			if (m_pPlayers[i])
			{
				// VERIFY: player vtbl+0x14 -> SetSlowMotion(1.0f /*normal*/, 0)
				((CBasePlayer *)m_pPlayers[i])->SetSlowMotion(1.0f, false);
				m_pPlayers[i] = NULL;
			}
		}
	}
	else
	{
		// global slow-motion off
		((CTSGameRules *)g_pGameRules)->GlobalSlowMotion2(0, false);
	}

	for (int i = 0; i < TS_SLOMO_MAX_PLAYERS; i++)
		m_pPlayers[i] = NULL;
}

// @ WaitTouch__13CTSSlowMotionP11CBaseEntity (@ 0x70fdc)
void CTSSlowMotion::WaitTouch(CBaseEntity *pOther)
{
	if (m_flStopTime == 0.0f && m_iToggledOn == 0)
		return;   // not active

	if (!pOther->IsPlayer())          // VERIFY: pOther vtbl+0xc8 -> IsPlayer()
		return;

	CBasePlayer *pPlayer = (CBasePlayer *)pOther;
	if (pPlayer->IsSlowMotion())      // VERIFY: vtbl+0x18 -> already slowed?
		return;

	pPlayer->SetSlowMotion(m_flSlowAmount, false);   // VERIFY: vtbl+0x14
	pPlayer->m_bSlowBullets = (m_bSlowBullets != 0);  // player+0x175 (VERIFY name)

	int idx = ENTINDEX(pOther->edict());
	if (idx < TS_SLOMO_MAX_PLAYERS)
		m_pPlayers[idx] = pOther;
}

// @ WaitThink__13CTSSlowMotion (@ 0x712a4)
void CTSSlowMotion::WaitThink(void)
{
	pev->nextthink = gpGlobals->time + 0.5f;   // VERIFY: pev+0x104, +0.5 interval

	if (m_iToggledOn)
	{
		if (!m_iAllMap)
			CheckPlayersDistance();
		else
			((CTSGameRules *)g_pGameRules)->GlobalSlowMotion2(m_flSlowAmount, m_bSlowBullets != 0);
	}

	if (m_flStopTime != 0.0f)
	{
		if (m_flStopTime < gpGlobals->time)
		{
			SetTouch(NULL);          // this+0x14/0x18 cleared
			DeactivateSlow();
		}
		else if (!m_iAllMap)
		{
			CheckPlayersDistance();
		}
		else
		{
			((CTSGameRules *)g_pGameRules)->GlobalSlowMotion2(m_flSlowAmount, m_bSlowBullets != 0);
		}
	}
}

// @ CheckPlayersDistance__13CTSSlowMotion (@ 0x7118c)
// Iterates the tracked players; any that moved beyond m_flRadius from this
// entity is restored to normal speed and dropped from the table.
// (Reconstructed from intent — the decompile is heavy stack-pointer arithmetic.)
void CTSSlowMotion::CheckPlayersDistance(void)
{
	for (int i = 0; i < TS_SLOMO_MAX_PLAYERS; i++)
	{
		CBaseEntity *pEnt = m_pPlayers[i];
		if (!pEnt)
			continue;

		if (!pEnt->IsPlayer())       // VERIFY: vtbl+0x18 in the loop
			continue;

		// distance from this entity's origin to the player (VERIFY: vtbl+0xf8
		// is a position/Center accessor on `this` used to build the delta)
		float dist = (pEnt->pev->origin - pev->origin).Length();
		if (dist > m_flRadius)
		{
			((CBasePlayer *)pEnt)->SetSlowMotion(1.0f, false);  // restore
			m_pPlayers[i] = NULL;
		}
	}
}
