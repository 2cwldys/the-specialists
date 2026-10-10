/***
*  ts_particles.h — The Specialists CLIENT sprite-particle pool.
*
*  A reconstruction of client.dll's custom sprite-particle pool (NOT engine tempents). The original is
*  one global pool object (DAT_1018bde8) of 300-byte particles, updated + rendered per frame from
*  HUD_DrawTransparentTriangles via the engine triangle API (gEngfuncs.pTriAPI). It carries both the
*  impact effects (sparks/smoke/debris/blood) and the map's ts_particle/ts_emitter effects (rain,
*  snow; cl_dll/ts_emitter.cpp). Recovered:
*    - per-frame update/render : FUN_100520d0(pool)
*    - motion + quad build     : FUN_10050ce0 (integrate, attach, orient) -> FUN_10050080 (corners)
*    - quad emit               : FUN_10050000
*    - wander / players / wind : FUN_10051270
*    - world collision         : FUN_10051d50 (+ FUN_10051d10 settle, FUN_100570b0 view cull)
*    - manager (alloc/register/random-frame/commit/free) : FUN_100507e0/1004ffe0/100509d0/100509a0/
*                                10050b60/10050b20
*    - cl_fx configuration     : FUN_10057d50 (fixed levels) + FUN_10057a70 (cl_fx 1, frame-rate driven)
*    - 5 spawners              : smoke 0x46680 / debris 0x46bd0 / scintille 0x46f50 /
*                                spark-shower 0x492b0 / pellets 0x46a70
*    - visual dispatcher       : FUN_10048f60 (per-surface texture-char switch + roll gating)
*
*  Per CLAUDE.md the internal C++ ABI is NOT a reconstruction target — clean named fields; the
*  original byte offsets are provenance comments.
***/
#pragma once

#include "cl_dll.h"
#include "com_model.h"

struct TSParticle;
struct pmtrace_s;

typedef void (*TSParticleThink)(TSParticle* p);                       // +0x118, run every frame
typedef void (*TSParticleCollide)(TSParticle* p, struct pmtrace_s* tr); // +0x11c, run on a world hit

// One particle. Original = 300 bytes; offsets in comments are its layout.
struct TSParticle
{
	bool active;              // +0x00 slot in use
	bool settled;             // +0x01 landed (FUN_10051d10): no more motion, view-facing
	bool drawn;               // +0x02 set every frame when it has a sprite; the collision pass clears
	                          //       it for a particle out of view
	unsigned char flags;      // +0x03 ts_particle spawnflags bit 1 (client.dll writes it, never reads it)
	unsigned char collide;    // +0x04 3 = trace the motion against the world, 4 = draw quad[] as is
	unsigned char orient;     // +0x05 0 = view-facing, 1 = view-facing rolled by angles.z, 2 = own angles,
	                          //       3 = stands along followAxis turned to the view
	unsigned char imode;      // +0x06 0 = free motion, n = rides entity n-1
	unsigned char attachment; // +0x07 that entity's attachment (0 = its origin)

	Vector savedPos;          // +0x08 position before this frame's move (the collision trace start)
	Vector pos;               // +0x14 current world position
	Vector angles;            // +0x20 orient 2 angles; angles.z is orient 1's roll
	Vector attachOffset;      // +0x2c offset from the ridden entity
	Vector vel;               // +0x38 velocity (moves vel * frametime * 10)
	Vector avel;              // +0x44 angular velocity (orient 1 rolls by avel.z, orient 2 turns by all)
	Vector randVelLow;        // +0x50 the random wander's target range ...
	Vector randVelHigh;       // +0x5c
	Vector randVel;           // +0x68 ... and its current value
	Vector followAxis;        // +0x74 orient 3's axis
	float quad[20];           // +0x80 collide 4: corners (12) + s (4) + t (4)

	float gravity;            // +0xd0 fall rate (x 0.1 x cl_gravity)
	float spawnTime;          // +0xd4 client time at commit
	float life;               // +0xd8 lifetime countdown; after it the fade and frame advance run
	float bright;             // +0xdc mono brightness 0..1 (Color4f rgb)
	float fadeRate;           // +0xe0 bright -= dt * fadeRate * 100 once life is spent
	float accelRate;          // +0xe4 size += dt * accelRate * 100
	float randomDelta;        // +0xe8 the wander's largest step
	float nextCollide;        // +0xec next collision trace (throttled by distance)
	float fadeIn;             // +0xf0 fade-in time after spawn
	float drag;               // +0xf4 air drag (commit holds it at 0.1 or more)
	float frame;              // +0xf8 current sprite frame
	float frameRate;          // +0xfc frames/s once life is spent
	float react;              // +0x100 how far a passing player pushes it
	float wind;               // +0x104 how much the map's ts_wind blows it
	float size;               // +0x108 quad half size
	float nextPerturb;        // +0x110 next wander / player / wind step
	short spriteIdx;          // +0x114 index into the pool sprite table (<0 = none)
	short slot;               // +0x116 its own pool slot (set by commit)
	TSParticleThink think;    // +0x118
	TSParticleCollide onCollide; // +0x11c
	int collideMode;          // +0x120 0 = no collision trace; set = trace (spawners use 2)
	int target;               // +0x124 ts_particle def spawned by onCollide (cl_dll/ts_emitter.cpp)
	float stretch;            // +0x128 velocity-stretch factor (orient 0 and 1)
};

// The cl_fx configuration (client.dll DAT_1018ca40, 0x22 dwords). FUN_10057d50 sets one of two fixed
// levels (cl_fx 1 or 2: high, anything else: low); with cl_fx exactly 1 FUN_10057a70 retunes it every
// second from the frame rate. maxParticles is read once, when the pool is first allocated.
struct TSFxConfig
{
	float scale;          // 0x1018ca40  high 1.0,  low 0.35
	float rate1;          // 0x1018ca44  high 0.05, low 0.2   (spawn throttles of the impact spawners)
	float rate2;          // 0x1018ca48  high 0.05, low 0.3
	float rate3;          // 0x1018ca4c  high 0.05, low 0.2
	float countScale;     // 0x1018ca50  high 1.5,  low 0.7
	float detail;         // 0x1018ca54  high 30,   low 5
	bool high;            // 0x1018ca58  high 1, low 0
	bool low;             // 0x1018ca59  high 0, low 1
	bool interact;        // 0x1018ca5a  players and wind move particles (high 1, low 0)
	int maxParticles;     // 0x1018ca5c  high 0x1000, low 0x800
	int divisor;          // 0x1018ca60  emitter count/interval divisor (high 1, low 8)
	float nextSample;     // 0x1018ca64  FUN_10057a70's 0.1 s frame-time sample
	float nextTune;       // 0x1018ca68  ... and its 1 s retune
	float samples[20];    // 0x1018ca6c  frame time x 100
	int sampleIndex;      // 0x1018cabc
};
extern TSFxConfig g_tsFx;

// Public client API. Origin/normal are world-space.

// Hook the TSPSys user message (call once from CHud::Init). Subtype 3 = wind: the state lands in
// g_tsWindState below; every other subtype is read-and-ignored (like the original).
void TSParticles_Init();

// Per-frame update + render — call once from HUD_DrawTransparentTriangles.
void TSParticles_Update();
void TSParticles_Reset();  // drop cached sprite handles + particles on a map change (call from VidInit)

// The view the pool last rendered with (client.dll's FX root FUN_10056f60 keeps fwd/right/up at
// DAT_1018bd90/9c/a8 for the next frame's emitters and culls).
extern Vector g_tspViewForward, g_tspViewRight, g_tspViewUp;

// The pool's sprite table (FUN_100509d0): find or SPR_Load (name, rendermode), -1 on failure. The
// handle of an index is valid until the next TSParticles_Reset.
int TSParticles_Sprite(const char* name, short rendermode);
HSPRITE TSParticles_SpriteHandle(int idx);

// The bullet trail (FUN_10046240): one sprites/bullet_trail.spr quad from `last` to `pos`, `size`
// either side along the view up, brightness alpha x 0.75, fading at 0.1. Drawn by the client bullet
// simulation (ts_bullets.cpp) while cl_wavedetail is above 0.
void TSP_BulletTrail(const Vector& pos, const Vector& last, float size, float alpha);

// The pool manager, shared with the map emitters (cl_dll/ts_emitter.cpp).
bool TSP_PoolReady();                                        // DAT_1018bdf0 != 0: allocated
TSParticle* TSP_Scratch();                                   // FUN_1004ffe0
int TSP_RegisterSprite(const char* name, short rendermode);  // FUN_100509d0
int TSP_Commit(TSParticle* scratch, TSParticleThink think, TSParticleCollide onCollide); // FUN_10050b60
void TSP_Free(int slot);                                     // FUN_10050b20

// TSPSys subtype-3 WIND state (handler FUN_1004bfd0; consumed per particle by FUN_10051270). The
// message sets the seed, both directions and both strengths and the change time; every time the
// change interval runs out the consumer moves target to previous, draws a new target strength
// between 0 and the base with the seeded random, and a new interval of 0.001..1 s, and eases between
// them with a cosine. The direction never changes after the message (the consumer re-draws dirBase,
// which nothing reads).
struct TSWindState
{
	int seed;               // 0x10189438
	float strengthPrev;     // 0x1018943c
	float strengthTarget;   // 0x10189440
	float strengthBase;     // 0x10189444 (the message's strength)
	Vector dirPrev;         // 0x10189448
	Vector dirTarget;       // 0x10189454 (the message's direction)
	Vector dirBase;         // 0x10189460
	float changeTime;       // 0x1018946c
	float changeDuration;   // 0x10189470
};
extern TSWindState g_tsWindState;

// PTakeDam blood tail (RE dossier cont94 §8, original FUN_10048840): the bloodfrag2.spr spray at the
// hit point. point/dir = the wire's hit point + hit direction; encByte = the encoded damage byte
// (bit7 = armored hit -> dust puff variant, low 7 bits = dmg*0.5).
void TSP_BloodSpray(const float* point, const float* dir, int encByte);

// The per-surface visual dispatcher (original FUN_10048f60). texChar is a CHAR_TEX_* material class
// (pm_materials.h). mode: 0 = normal, 1/2 = forced (skip throttle).
void TS_ImpactEffect(const float* origin, const float* normal, int mode, char texChar);

// The MELEE impact effect (original client blade handlers). NOT the bullet dispatcher — just the
// scintille_metal glint (knife 6 / katana 12) + sparks.spr shower (no smoke, no debris), and only on
// hard surfaces (skips dirt/wood/flesh).
void TS_MeleeImpact(const float* origin, const float* normal, char texChar, int glints = 6);

// Convenience: trace the surface at `pos` and run TS_ImpactEffect with its material.
// (original wrapper FUN_10044bf0, minus the sound/decal which our server path owns.)
void TS_SurfaceImpact(const float* origin, const float* normal, char texChar, int mode);
