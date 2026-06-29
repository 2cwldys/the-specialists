/***
*  ts_sharedemitter.h — CTSSharedEmitter (the TS particle/emitter MANAGER, TS_sharedemitter.cpp).
*  A non-polymorphic global singleton (s_Emitter) holding two 512-slot arrays of emitter/particle
*  structs, plus the keyvalue parsers the ts_emitter/ts_particle entities delegate to. Most of the
*  emitter-linkage + per-client replay path is DEAD in the shipped binary (see the .cpp notes); only
*  AddParticle, KillAll and the two KeyValue parsers are exercised live.
***/
#ifndef TS_SHAREDEMITTER_H
#define TS_SHAREDEMITTER_H

#define TS_EMITTER_MAX 512

// A ts_emitter's parsed properties (embedded on the entity; passed to EmitterKeyValue).
struct sharedemitter_s
{
	int    id;            // +0x00 slot index (-1 = unregistered)
	Vector origin;        // +0x04 "origin"
	int    active;        // +0x10
	char   particle[100]; // +0x18 "particle" target name
	float  epshigh, epslow;       // +0x7c/0x80 emits per second
	int    ppehigh, ppelow;       // +0x84/0x88 particles per emit
	Vector variancehigh, variancelow; // +0x8c / +0x98
	float  cameradensity;         // +0xa4
	float  cameraradius;          // +0xa8
};

// A ts_particle's parsed properties (embedded on the entity; passed to ParticleKeyValue).
struct sharedparticle_s
{
	int   id;                 // +0x00 slot index
	int   active;             // +0x04
	int   refcount;           // +0x08 emitters linked to it
	float lifehigh, lifelow;             // +0x0c/0x10
	float gravityhigh, gravitylow;       // +0x14/0x18
	float colorhigh, colorlow;           // +0x1c/0x20
	float fadespeedhigh, fadespeedlow;   // +0x24/0x28
	float p_radiushigh, p_radiuslow;     // +0x2c/0x30
	float scalespeedhigh, scalespeedlow; // +0x34/0x38
	int   current_framehigh, current_framelow; // +0x3c/0x40
	float frame_ratehigh, frame_ratelow; // +0x44/0x48
	float randomdeltahigh, randomdeltalow; // +0x4c/0x50
	float reactamounthigh, reactamountlow; // +0x54/0x58
	float windamounthigh, windamountlow;   // +0x5c/0x60
	float termvelocityhigh, termvelocitylow; // +0x64/0x68
	float fadeintimehigh, fadeintimelow;   // +0x6c/0x70
	Vector velocityhigh, velocitylow;      // +0x74 / +0x80
	Vector a_velocityhigh, a_velocitylow;  // +0x8c / +0x98
	Vector randomvelhigh, randomvellow;    // +0xbc / +0xc8
	Vector followaxis;        // +0xd4
	float  stretchspeed;      // +0xe0
	char   sprite_name[100];  // +0xe4
	int    sprite_trans;      // +0x148
	int    follow_view;       // +0x14c
	int    collide;           // +0x150
	int    oncollide;         // +0x154
	char   soundonemit[64];   // +0x158
};

class CTSSharedEmitter
{
public:
	int  NextEmitter();                       // first free emitters[] slot, else -1
	int  NextParticle();                      // first free particles[] slot, else -1
	int  AddEmitter(sharedemitter_s *e);      // register an emitter (DEAD: no callers)
	int  AddParticle(sharedparticle_s *p);    // register a particle (LIVE: CTSParticle::Spawn)
	void SetEmitterParticle(int e, int p);    // link emitter->particle (DEAD)
	void SV_UpdateClient(int clientIdx);      // per-client particle replay (DEAD)
	void KillAll();                           // clear both arrays (LIVE: ServerDeactivate)
	void KillEmitter(int i);                  // free one emitter slot (DEAD)
	int  EmitterKeyValue(sharedemitter_s *e, const char *key, const char *val);
	int  ParticleKeyValue(sharedparticle_s *p, const char *key, const char *val);

	sharedemitter_s  *emitters[TS_EMITTER_MAX];  // +0x000
	sharedparticle_s *particles[TS_EMITTER_MAX]; // +0x800
};

extern CTSSharedEmitter s_Emitter;
extern int gmsgTSParticleSystem;

#endif // TS_SHAREDEMITTER_H
