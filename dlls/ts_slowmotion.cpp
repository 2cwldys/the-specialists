/***
*  ts_slowmotion.cpp — RECONSTRUCTED CTSSlowMotion from ts_i386.so + ts_fgd.fgd.
*  Functional/source-faithful goal (see CLAUDE.md). recovered/server/CTSSlowMotion.c.
*
*  @SolidClass base(Targetname, Target) = ts_slowmotion : "TS SlowMotion"
*    duration (string) "Duration(sec)" def 5   -> m_flDuration   (this+0xf8)
*    slowamt  (string) "SlowMotion ammount"def 0.5 -> m_flSlowAmount (this+0xfc)
*    message  (string) -> pev->message (shown to all on Use)
*    spawnflags: 1 Start ON, 2 All map (-> m_iAllMap, global slow), 4 Slow bullets
*
*  A brush slow-motion zone (11 placements across the stock maps). When active, a
*  player touching it is GoSlow()'d to m_flSlowAmount; CheckPlayersDistance drops
*  players who leave; the "All map" variant drives CTSGameRules::GlobalSlowMotion2
*  for every player. duration>0 = timed; duration<=0 = Use-toggled.
*
*  SLICE: the actual time-dilation MOVEMENT is client-side (GoSlow only sets the
*  player's slow-motion fields server-side — see CBasePlayer::GoSlow/InSlow).
***/
#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "player.h"
#include "gamerules.h"
#include "ts_gamerules.h"
#include "ts_utils.h" // TSHudMessageAll

#define SF_SLOMO_START_ON 1
#define SF_SLOMO_ALL_MAP 2
#define SF_SLOMO_SLOW_BULLETS 4

#define TS_SLOMO_MAX_PLAYERS 33 // 0x21 — gpGlobals->maxClients + 1, indexed by ENTINDEX

class CTSSlowMotion : public CBaseEntity
{
public:
	void Spawn() override;
	bool KeyValue(KeyValueData *pkvd) override;
	void Use(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value) override;
	int ObjectCaps() override { return 0; } // @ ObjectCaps__13CTSSlowMotion returns 0
	bool Save(CSave &save) override;
	bool Restore(CRestore &restore) override;
	static TYPEDESCRIPTION m_SaveData[];

	void EXPORT WaitThink();
	void EXPORT WaitTouch(CBaseEntity *pOther);
	void ActivateSlow();
	void DeactivateSlow();
	void CheckPlayersDistance();

	int m_iToggledOn = 0;     // 0x64 — toggle state (duration<=0 mode)
	float m_flStopTime = 0;   // 0x68 — time the zone shuts off (0 = inactive)
	EHANDLE m_hPlayers[TS_SLOMO_MAX_PLAYERS]; // 0x70 — players currently slowed
	float m_flRadius = 0;     // 0xf4 — drop-range for CheckPlayersDistance
	float m_flDuration = 5;   // 0xf8 — "duration"
	float m_flSlowAmount = 0.5f; // 0xfc — "slowamt" (GoSlow scale)
	int m_iAllMap = 0;        // 0x100 — SF_ALL_MAP: global slow-motion
	int m_bSlowBullets = 0;   // 0x104 — SF_SLOW_BULLETS
};

LINK_ENTITY_TO_CLASS(ts_slowmotion, CTSSlowMotion);

TYPEDESCRIPTION CTSSlowMotion::m_SaveData[] = {
	DEFINE_FIELD(CTSSlowMotion, m_flDuration, FIELD_FLOAT),
	DEFINE_FIELD(CTSSlowMotion, m_flSlowAmount, FIELD_FLOAT),
};
IMPLEMENT_SAVERESTORE(CTSSlowMotion, CBaseEntity);

bool CTSSlowMotion::KeyValue(KeyValueData *pkvd)
{
	if (FStrEq(pkvd->szKeyName, "duration"))
	{
		m_flDuration = atof(pkvd->szValue);
		return true;
	}
	if (FStrEq(pkvd->szKeyName, "slowamt"))
	{
		m_flSlowAmount = atof(pkvd->szValue);
		return true;
	}
	return CBaseEntity::KeyValue(pkvd);
}

void CTSSlowMotion::Spawn()
{
	pev->classname = MAKE_STRING("ts_slowmotion");
	SET_MODEL(ENT(pev), STRING(pev->model)); // brush model
	UTIL_SetSize(pev, pev->mins, pev->maxs);

	// The original's Spawn is minimal (classname/model/size); a brush trigger needs
	// the following to detect touches + poll. // VERIFY against engine defaults.
	pev->solid = SOLID_TRIGGER;
	pev->movetype = MOVETYPE_NONE;
	pev->effects |= EF_NODRAW;

	m_iAllMap = (pev->spawnflags & SF_SLOMO_ALL_MAP) ? 1 : 0;
	m_bSlowBullets = (pev->spawnflags & SF_SLOMO_SLOW_BULLETS) ? 1 : 0;
	m_flRadius = pev->size.Length() * 0.5f; // VERIFY: drop-range from the brush bounds

	SetThink(&CTSSlowMotion::WaitThink);
	pev->nextthink = gpGlobals->time + 0.5f;

	if (pev->spawnflags & SF_SLOMO_START_ON)
		ActivateSlow();
}

// @ ActivateSlow__13CTSSlowMotion — arm the zone (timed or toggle), set the touch.
void CTSSlowMotion::ActivateSlow()
{
	if (m_flDuration > 0.0f)
	{
		m_flStopTime = gpGlobals->time + m_flDuration;
	}
	else
	{
		m_flStopTime = 0;
		m_iToggledOn ^= 1;
		if (m_iToggledOn == 0)
			DeactivateSlow();
	}

	if (!m_iAllMap)
		SetTouch(&CTSSlowMotion::WaitTouch);
}

// @ DeactivateSlow__13CTSSlowMotion — restore everyone the zone slowed, clear it.
void CTSSlowMotion::DeactivateSlow()
{
	m_flStopTime = 0;

	if (!m_iAllMap)
	{
		for (int i = 0; i < TS_SLOMO_MAX_PLAYERS; i++)
		{
			CBaseEntity *pEnt = m_hPlayers[i];
			if (pEnt && pEnt->IsPlayer())
				((CBasePlayer *)pEnt)->GoSlow(1.0f, 0);
			m_hPlayers[i] = NULL;
		}
	}
	else
	{
		TSGameRules()->GlobalSlowMotion2(0, 0);
		for (int i = 0; i < TS_SLOMO_MAX_PLAYERS; i++)
			m_hPlayers[i] = NULL;
	}
}

// @ WaitTouch__13CTSSlowMotion — slow the touching player (if not already slowed).
void CTSSlowMotion::WaitTouch(CBaseEntity *pOther)
{
	if (m_flStopTime == 0.0f && m_iToggledOn == 0)
		return; // zone not active

	if (!pOther->IsPlayer())
		return;

	CBasePlayer *pPlayer = (CBasePlayer *)pOther;
	if (pPlayer->InSlow())
		return; // already slowed (by this or another source)

	pPlayer->GoSlow(m_flSlowAmount, 0);
	pPlayer->m_bSlowBullets = m_bSlowBullets != 0;

	int idx = ENTINDEX(pOther->edict());
	if (idx >= 0 && idx < TS_SLOMO_MAX_PLAYERS)
		m_hPlayers[idx] = pOther;
}

// @ WaitThink__13CTSSlowMotion — poll: keep the slow applied / time it out.
void CTSSlowMotion::WaitThink()
{
	pev->nextthink = gpGlobals->time + 0.5f;

	if (m_iToggledOn)
	{
		if (!m_iAllMap)
			CheckPlayersDistance();
		else
			TSGameRules()->GlobalSlowMotion2(m_flSlowAmount, m_bSlowBullets);
	}

	if (m_flStopTime != 0.0f)
	{
		if (m_flStopTime < gpGlobals->time)
		{
			SetTouch(NULL);
			DeactivateSlow();
		}
		else if (!m_iAllMap)
		{
			CheckPlayersDistance();
		}
		else
		{
			TSGameRules()->GlobalSlowMotion2(m_flSlowAmount, m_bSlowBullets);
		}
	}
}

// @ CheckPlayersDistance__13CTSSlowMotion — drop tracked players who left the zone
// (restore them to normal speed). Reconstructed from intent (the decompile is heavy
// stack arithmetic): distance from the zone centre, beyond m_flRadius -> released.
void CTSSlowMotion::CheckPlayersDistance()
{
	Vector center = Center();
	for (int i = 0; i < TS_SLOMO_MAX_PLAYERS; i++)
	{
		CBaseEntity *pEnt = m_hPlayers[i];
		if (!pEnt || !pEnt->IsPlayer())
			continue;

		CBasePlayer *pPlayer = (CBasePlayer *)pEnt;
		if (!pPlayer->InSlow())
			continue;

		float dist = (pPlayer->pev->origin - center).Length();
		if (dist > m_flRadius)
		{
			pPlayer->GoSlow(1.0f, 0);
			m_hPlayers[i] = NULL;
		}
	}
}

// @ Use__13CTSSlowMotion — trigger the zone (button/relay), announce, fire targets.
void CTSSlowMotion::Use(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value)
{
	ActivateSlow();

	if (!m_iAllMap)
		SetTouch(&CTSSlowMotion::WaitTouch);

	if (!FStringNull(pev->message))
		TSHudMessageAll(0, 255, 0, 8, 30, STRING(pev->message));

	if (!FStringNull(pev->target))
		FireTargets(STRING(pev->target), this, this, USE_TOGGLE, 0);
}
