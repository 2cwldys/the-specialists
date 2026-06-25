/***
*  ts_particles.h — The Specialists CLIENT impact-effect particle subsystem.
*
*  A faithful reconstruction of the original client.dll custom sprite-particle pool
*  (NOT engine tempents). The original is one global pool object (DAT_1018bde8) of
*  300-byte particles, updated + rendered per frame from HUD_DrawTransparentTriangles
*  via the engine triangle API (gEngfuncs.pTriAPI). Recovered:
*    - per-frame update/render : FUN_100520d0(pool)    (the crux, cont.69x)
*    - billboard corner build  : FUN_10050ce0/FUN_10050080
*    - quad emit               : FUN_10050000
*    - manager (alloc/register/random-frame/commit) : FUN_1004ffe0/100509d0/100509a0/10050b60
*    - 5 spawners              : smoke 0x46680 / debris 0x46bd0 / scintille 0x46f50 /
*                                spark-shower 0x492b0 / pellets 0x46a70
*    - visual dispatcher       : FUN_10048f60 (per-surface texture-char switch + roll gating)
*
*  Per CLAUDE.md the internal C++ ABI is NOT a reconstruction target — we mirror the
*  BEHAVIOUR/look with clean named fields; the original byte offsets are provenance
*  comments only. Full RE in scratchpad/impact-effects-re.md.
***/
#pragma once

#include "cl_dll.h"
#include "com_model.h"

// One particle. Original = 300 bytes; offsets in comments are the recovered layout.
struct TSParticle
{
	bool active;          // +0x00  slot in use
	unsigned char collide;// +0x01  0 = none, 3 = bounce off world
	unsigned char orient; // +0x05  0 = camera-facing, 1/2 = motion-oriented, 3 = trail
	unsigned char imode;  // +0x06  integration mode (0 = velocity + gravity + drag)

	Vector savedPos;      // +0x08  previous position (trail / motion blur)
	Vector pos;           // +0x14  current world position
	Vector auxPos;        // +0x20  orient-2 motion accumulator
	Vector vel;           // +0x38  velocity (units/s)
	Vector dir;           // +0x44  oriented-billboard direction

	float gravity;        // +0xd0  downward accel coefficient
	float spawnTime;      // +0xd4  birth time (engine clock)
	float life;           // +0xd8  lifetime countdown (-= dt); <0 starts frame-advance
	float bright;         // +0xdc  mono brightness 0..1 (Color4f rgb)
	float fadeRate;       // +0xe0  brightness fade speed (bright -= dt*fadeRate*100)
	float accelRate;      // +0xe4  size growth/shrink rate (size += dt*accelRate*100)
	float fadeIn;         // +0xf0  fade-IN ramp duration after spawn
	float drag;           // +0xf4  air drag (vel -= vel*clamp(dt*drag,1)); min 0.1
	float frame;          // +0xf8  current sprite frame (float)
	float frameRate;      // +0xfc  frames/sec (0 = static)
	float renderEnable;   // +0x100 >0 => eligible to draw this frame
	float size;           // +0x108 quad HALF-SIZE (animated by accelRate)
	float stretch;        // +0x128 velocity-stretch length (0 = pure billboard)
	short spriteIdx;      // +0x114 index into the pool sprite table (<0 = none)
};

// Public client API (the byte-port front-end). Origin/normal are world-space.

// Per-frame update + render — call once from HUD_DrawTransparentTriangles.
void TSParticles_Update();

// The per-surface visual dispatcher (original FUN_10048f60). texChar is a CHAR_TEX_*
// material class (pm_materials.h). mode: 0 = normal, 1/2 = forced (skip throttle).
void TS_ImpactEffect(const float* origin, const float* normal, int mode, char texChar);

// Convenience: trace the surface at `pos` and run TS_ImpactEffect with its material.
// (original wrapper FUN_10044bf0, minus the sound/decal which our server path owns.)
void TS_SurfaceImpact(const float* origin, const float* normal, char texChar, int mode);
