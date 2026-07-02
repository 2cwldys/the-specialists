/***
*  ts_emitter.cpp — RECONSTRUCTED CTSEmitter / CTSParticle / CTSWind (TS_emitter.cpp).
*  The three map entities of the TS particle system; they delegate their keyvalues to the global
*  CTSSharedEmitter (s_Emitter). Reconstructed 1:1 from ts_i386.so.
*
*  FIDELITY (RE): ts_emitter self-removes in Spawn and never registers (the emitter linkage is dead
*  code as shipped); ts_particle registers with s_Emitter and PERSISTS; ts_wind broadcasts the live
*  wind message (gmsgTSParticleSystem subtype 3) on a frequency timer. See ts_sharedemitter.cpp.
***/
#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "ts_sharedemitter.h"
#include <string.h>

static void RegisterParticleMsg()
{
	if (gmsgTSParticleSystem == 0)
		gmsgTSParticleSystem = REG_USER_MSG("TSPSys", -1);
}

//============================================================================
// CTSEmitter (ts_emitter) — validates its `particle` target, then self-removes (dead pipeline).
//============================================================================
class CTSEmitter : public CBaseEntity
{
public:
	void Spawn() override;
	bool KeyValue(KeyValueData *pkvd) override;
	void CheckValues();
	void EXPORT InitThink();

	sharedemitter_s m_data = {}; // +0x68 in the original
};

LINK_ENTITY_TO_CLASS(ts_emitter, CTSEmitter);

bool CTSEmitter::KeyValue(KeyValueData *pkvd)
{
	if (s_Emitter.EmitterKeyValue(&m_data, pkvd->szKeyName, pkvd->szValue))
		return true;
	return CBaseEntity::KeyValue(pkvd); // unknown (e.g. FGD "master")
}

void CTSEmitter::CheckValues()
{
	if (m_data.epslow <= 0 || m_data.epshigh <= 0)
		ALERT(at_console, "Emits per second <= 0! (%s)\n", STRING(pev->targetname));
	if (m_data.ppelow < 1 || m_data.ppehigh < 1)
		ALERT(at_console, "Particles per emit <= 0! (%s)\n", STRING(pev->targetname));
}

void CTSEmitter::Spawn()
{
	pev->classname = MAKE_STRING("ts_emitter");
	CheckValues();
	SetThink(&CTSEmitter::InitThink);
	pev->nextthink = gpGlobals->time + 0.01f;
	m_data.origin = pev->origin;
	// The shipped binary always removes the emitter at the end of Spawn (the emitter→particle
	// linkage was cut); reproduced literally. The InitThink validation below is effectively dead.
	UTIL_Remove(this);
}

void CTSEmitter::InitThink()
{
	CBaseEntity *p = UTIL_FindEntityByTargetname(NULL, m_data.particle);
	if (!p)
	{
		ALERT(at_console, "Particle '%s' does not exist!\n", m_data.particle);
		UTIL_Remove(this);
		return;
	}
	if (!FClassnameIs(p->pev, "ts_particle"))
	{
		ALERT(at_console, "'%s' is definitely not a ts_particle.\n", m_data.particle);
		UTIL_Remove(this);
		return;
	}
	// found a valid ts_particle — the original does nothing further (linkage cut).
}

//============================================================================
// CTSParticle (ts_particle) — registers with s_Emitter and PERSISTS (the live data holder).
//============================================================================
class CTSParticle : public CBaseEntity
{
public:
	void Spawn() override;
	bool KeyValue(KeyValueData *pkvd) override;
	void CheckValues();

	sharedparticle_s m_data = {}; // +0x68
};

LINK_ENTITY_TO_CLASS(ts_particle, CTSParticle);

bool CTSParticle::KeyValue(KeyValueData *pkvd)
{
	if (s_Emitter.ParticleKeyValue(&m_data, pkvd->szKeyName, pkvd->szValue))
		return true;
	return CBaseEntity::KeyValue(pkvd);
}

void CTSParticle::CheckValues()
{
	if (m_data.lifehigh < 0) m_data.lifehigh = 0;
	if (m_data.lifelow < 0) m_data.lifelow = 0;
	if (m_data.colorhigh < 0) m_data.colorhigh = 0; else if (m_data.colorhigh > 1) m_data.colorhigh = 1;
	if (m_data.colorlow < 0) m_data.colorlow = 0; else if (m_data.colorlow > 1) m_data.colorlow = 1;
	if (m_data.p_radiushigh < 0) m_data.p_radiushigh = 0;
	if (m_data.p_radiuslow < 0) m_data.p_radiuslow = 0;
	if (m_data.current_framehigh < 0) m_data.current_framehigh = 0;
	if (m_data.current_framelow < 0) m_data.current_framelow = 0;
}

void CTSParticle::Spawn()
{
	pev->classname = MAKE_STRING("ts_particle");
	CheckValues();
	m_data.id = s_Emitter.AddParticle(&m_data);
	if (m_data.id == -1)
	{
		ALERT(at_console, "Too many particles! Particle limit is %d particles.\n", TS_EMITTER_MAX);
		UTIL_Remove(this);
		return;
	}
	if (m_data.soundonemit[0])
		PRECACHE_SOUND(m_data.soundonemit);
	// persists — held in s_Emitter.particles[]
}

//============================================================================
// CTSWind (ts_wind) — broadcasts the live wind message on a frequency timer (no s_Emitter use).
//============================================================================
class CTSWind : public CBaseEntity
{
public:
	void Spawn() override;
	bool KeyValue(KeyValueData *pkvd) override;
	void EXPORT BlowThink();

	float m_base = 0;      // +0x64
	float m_variance = 0;  // +0x68
	float m_frequency = 0; // +0x6c
};

LINK_ENTITY_TO_CLASS(ts_wind, CTSWind);

bool CTSWind::KeyValue(KeyValueData *pkvd)
{
	if (FStrEq(pkvd->szKeyName, "base"))      { m_base = atof(pkvd->szValue); return true; }
	if (FStrEq(pkvd->szKeyName, "variance"))  { m_variance = atof(pkvd->szValue); return true; }
	if (FStrEq(pkvd->szKeyName, "frequency")) { m_frequency = atof(pkvd->szValue); return true; }
	return CBaseEntity::KeyValue(pkvd);
}

void CTSWind::Spawn()
{
	pev->classname = MAKE_STRING("ts_wind");
	RegisterParticleMsg();
	SetThink(&CTSWind::BlowThink);
	pev->nextthink = gpGlobals->time + 0.01f;
}

void CTSWind::BlowThink()
{
	MESSAGE_BEGIN(MSG_ALL, gmsgTSParticleSystem);
	WRITE_BYTE(3); // subtype 3 = wind
	WRITE_LONG(RANDOM_LONG(0, 30000));
	WRITE_LONG((int)(pev->angles.y + RANDOM_LONG(-45, 45) + 0.5f)); // wind yaw, ±45° of entity angle
	WRITE_COORD(m_base + RANDOM_FLOAT(-m_variance, m_variance));     // wind strength
	MESSAGE_END();

	pev->nextthink = gpGlobals->time + m_frequency + RANDOM_FLOAT(-m_frequency * 0.5f, m_frequency * 0.5f);
}

/***
*  ts_fog.cpp — RECONSTRUCTED CTSFog from ts_i386.so + ts_fgd.fgd.
*  Functional/source-faithful (see CLAUDE.md). recovered/server/CTSFog.c.
*
*  @PointClass = ts_fog : "TS Fog"  — keyvalues near/far/color.
*
*  The original Spawn does exactly one thing: UTIL_Remove(this). The fog is a
*  client-side effect read from the map's entity data; the server entity has no
*  runtime role and deletes itself on spawn.
***/

class CTSFog : public CBaseEntity
{
public:
	void Spawn(void) override;
};

LINK_ENTITY_TO_CLASS(ts_fog, CTSFog);

void CTSFog::Spawn(void)
{
	UTIL_Remove(this);
}
