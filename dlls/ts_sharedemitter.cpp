/***
*  ts_sharedemitter.cpp — RECONSTRUCTED CTSSharedEmitter, the TS particle/emitter manager
*  (TS_sharedemitter.cpp). Global singleton s_Emitter. Method set 1:1 with the original.
*
*  FIDELITY NOTE: in the shipped binary most of the pipeline is DEAD — AddEmitter,
*  SetEmitterParticle, SV_UpdateClient, KillEmitter have ZERO callers; only AddParticle
*  (CTSParticle::Spawn), KillAll (ServerDeactivate) and the two KeyValue parsers run live.
*  Reproduced faithfully (present-but-inert), per the RE.
***/
#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "ts_sharedemitter.h"
#include <string.h>

CTSSharedEmitter s_Emitter;          // .bss singleton (orig _16CTSSharedEmitter.s_Emitter)
int gmsgTSParticleSystem = 0;        // "TSPSys" (size -1) — registered centrally in LinkUserMessages

int CTSSharedEmitter::NextEmitter()
{
	for (int i = 0; i < TS_EMITTER_MAX; i++)
		if (emitters[i] == NULL)
			return i;
	return -1;
}

int CTSSharedEmitter::NextParticle()
{
	for (int i = 0; i < TS_EMITTER_MAX; i++)
		if (particles[i] == NULL)
			return i;
	return -1;
}

// DEAD in the shipped binary (no callers) — kept for parity.
int CTSSharedEmitter::AddEmitter(sharedemitter_s *e)
{
	int i = NextEmitter();
	if (i == -1)
		return -1;
	emitters[i] = e;
	e->active = 1;
	e->id = i;
	return i;
}

// LIVE — CTSParticle::Spawn registers here and the particle persists.
int CTSSharedEmitter::AddParticle(sharedparticle_s *p)
{
	int i = NextParticle();
	if (i == -1)
		return -1;
	particles[i] = p;
	p->active = 1;
	p->refcount = 0;
	p->id = i;
	return i;
}

// DEAD — links emitter e to particle p (adjusts the particle refcounts).
void CTSSharedEmitter::SetEmitterParticle(int e, int p)
{
	if (e < 0 || e >= TS_EMITTER_MAX || !emitters[e])
		return;
	int cur = emitters[e]->id; // (orig stores the linked-particle index on the emitter)
	if (cur >= 0 && cur < TS_EMITTER_MAX && particles[cur])
		particles[cur]->refcount--;
	emitters[e]->id = p;
	if (p >= 0 && p < TS_EMITTER_MAX && particles[p])
		particles[p]->refcount++;
}

// DEAD — per-client particle replay (the cut per-client initial-state send).
void CTSSharedEmitter::SV_UpdateClient(int clientIdx)
{
	edict_t *pClient = INDEXENT(clientIdx);
	if (!pClient)
		return;
	for (int i = 0; i < TS_EMITTER_MAX; i++)
	{
		if (particles[i] && particles[i]->active)
		{
			MESSAGE_BEGIN(MSG_ONE, gmsgTSParticleSystem, NULL, pClient);
			WRITE_BYTE(1); // subtype 1 = particle ping (no payload)
			MESSAGE_END();
		}
	}
}

// LIVE — ServerDeactivate clears every emitter + particle on map shutdown.
void CTSSharedEmitter::KillAll()
{
	for (int i = 0; i < TS_EMITTER_MAX; i++)
	{
		emitters[i] = NULL;
		particles[i] = NULL;
	}
}

// DEAD — free one emitter slot.
void CTSSharedEmitter::KillEmitter(int i)
{
	if (i >= 0 && i < TS_EMITTER_MAX)
		emitters[i] = NULL;
}

static bool ParseVec(const char *val, Vector &out)
{
	float x, y, z;
	if (sscanf(val, "%f %f %f", &x, &y, &z) == 3)
	{
		out = Vector(x, y, z);
		return true;
	}
	return false;
}

int CTSSharedEmitter::EmitterKeyValue(sharedemitter_s *e, const char *key, const char *val)
{
	if (FStrEq(key, "particle"))      { strncpy(e->particle, val, sizeof(e->particle) - 1); return 1; }
	if (FStrEq(key, "epshigh"))       { e->epshigh = atof(val); return 1; }
	if (FStrEq(key, "epslow"))        { e->epslow = atof(val); return 1; }
	if (FStrEq(key, "ppehigh"))       { e->ppehigh = atoi(val); return 1; }
	if (FStrEq(key, "ppelow"))        { e->ppelow = atoi(val); return 1; }
	if (FStrEq(key, "cameradensity")) { e->cameradensity = atof(val); return 1; }
	if (FStrEq(key, "cameraradius"))  { e->cameraradius = atof(val); return 1; }
	if (FStrEq(key, "origin"))        { ParseVec(val, e->origin); return 1; }
	if (FStrEq(key, "variancehigh"))
	{
		if (!ParseVec(val, e->variancehigh)) ALERT(at_console, "variancehigh has bad value. Should be \"x y z\"\n");
		return 1;
	}
	if (FStrEq(key, "variancelow"))
	{
		if (!ParseVec(val, e->variancelow)) ALERT(at_console, "variancelow has bad value. Should be \"x y z\"\n");
		return 1;
	}
	return 0;
}

int CTSSharedEmitter::ParticleKeyValue(sharedparticle_s *p, const char *key, const char *val)
{
	// scalar high/low pairs
	if (FStrEq(key, "lifehigh"))         { p->lifehigh = atof(val); return 1; }
	if (FStrEq(key, "lifelow"))          { p->lifelow = atof(val); return 1; }
	if (FStrEq(key, "gravityhigh"))      { p->gravityhigh = atof(val); return 1; }
	if (FStrEq(key, "gravitylow"))       { p->gravitylow = atof(val); return 1; }
	if (FStrEq(key, "colorhigh"))        { p->colorhigh = atof(val); return 1; }
	if (FStrEq(key, "colorlow"))         { p->colorlow = atof(val); return 1; }
	if (FStrEq(key, "fadespeedhigh"))    { p->fadespeedhigh = atof(val); return 1; }
	if (FStrEq(key, "fadespeedlow"))     { p->fadespeedlow = atof(val); return 1; }
	if (FStrEq(key, "p_radiushigh"))     { p->p_radiushigh = atof(val); return 1; }
	if (FStrEq(key, "p_radiuslow"))      { p->p_radiuslow = atof(val); return 1; }
	if (FStrEq(key, "scalespeedhigh"))   { p->scalespeedhigh = atof(val); return 1; }
	if (FStrEq(key, "scalespeedlow"))    { p->scalespeedlow = atof(val); return 1; }
	if (FStrEq(key, "current_framehigh")){ p->current_framehigh = atoi(val); return 1; }
	if (FStrEq(key, "current_framelow")) { p->current_framelow = atoi(val); return 1; }
	if (FStrEq(key, "frame_ratehigh"))   { p->frame_ratehigh = atof(val); return 1; }
	if (FStrEq(key, "frame_ratelow"))    { p->frame_ratelow = atof(val); return 1; }
	if (FStrEq(key, "randomdeltahigh"))  { p->randomdeltahigh = atof(val); return 1; }
	if (FStrEq(key, "randomdeltalow"))   { p->randomdeltalow = atof(val); return 1; }
	if (FStrEq(key, "reactamounthigh"))  { p->reactamounthigh = atof(val); return 1; }
	if (FStrEq(key, "reactamountlow"))   { p->reactamountlow = atof(val); return 1; }
	if (FStrEq(key, "windamounthigh"))   { p->windamounthigh = atof(val); return 1; }
	if (FStrEq(key, "windamountlow"))    { p->windamountlow = atof(val); return 1; }
	if (FStrEq(key, "termvelocityhigh")) { p->termvelocityhigh = atof(val); return 1; }
	if (FStrEq(key, "termvelocitylow"))  { p->termvelocitylow = atof(val); return 1; }
	if (FStrEq(key, "fadeintimehigh"))   { p->fadeintimehigh = atof(val); return 1; }
	if (FStrEq(key, "fadeintimelow"))    { p->fadeintimelow = atof(val); return 1; }
	if (FStrEq(key, "stretchspeed"))     { p->stretchspeed = atof(val); return 1; }
	// string fields
	if (FStrEq(key, "sprite_name"))      { strncpy(p->sprite_name, val, sizeof(p->sprite_name) - 1); return 1; }
	if (FStrEq(key, "soundonemit"))      { strncpy(p->soundonemit, val, sizeof(p->soundonemit) - 1); return 1; }
	// int flags
	if (FStrEq(key, "sprite_trans"))     { p->sprite_trans = atoi(val); return 1; }
	if (FStrEq(key, "follow_view"))      { p->follow_view = atoi(val); return 1; }
	if (FStrEq(key, "collide"))          { p->collide = atoi(val); return 1; }
	if (FStrEq(key, "oncollide"))        { p->oncollide = atoi(val); return 1; }
	// vec3 fields (sscanf, warn on malformed)
	struct { const char *k; Vector *v; } vecs[] = {
		{ "velocityhigh", &p->velocityhigh },   { "velocitylow", &p->velocitylow },
		{ "a_velocityhigh", &p->a_velocityhigh },{ "a_velocitylow", &p->a_velocitylow },
		{ "randomvelhigh", &p->randomvelhigh },  { "randomvellow", &p->randomvellow },
		{ "followaxis", &p->followaxis },
	};
	for (unsigned i = 0; i < sizeof(vecs) / sizeof(vecs[0]); i++)
		if (FStrEq(key, vecs[i].k))
		{
			if (!ParseVec(val, *vecs[i].v))
				ALERT(at_console, "%s has bad value. Should be \"x y z\"\n", key);
			return 1;
		}
	return 0;
}
