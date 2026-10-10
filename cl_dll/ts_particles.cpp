/***
*  ts_particles.cpp — The Specialists CLIENT impact-effect particle subsystem.
*  See ts_particles.h. Faithful reconstruction of the original client.dll sprite pool
*  (one global pool of 300-byte particles, drawn per frame via gEngfuncs.pTriAPI).
*
*  Recovered originals (cont.69x — full RE in scratchpad/impact-effects-re.md):
*    update/render FUN_100520d0 · billboard FUN_10050ce0/FUN_10050080 · quad FUN_10050000
*    manager FUN_1004ffe0/100509d0/100509a0/10050b60 · spawners 0x46680/46bd0/46f50/492b0/46a70
*    dispatcher FUN_10048f60.
*  Internal ABI is not a target (CLAUDE.md) — clean fields, recovered offsets are comments.
***/
#include "hud.h"
#include "cl_util.h"
#include "parsemsg.h" // TSPSys handler
#include "const.h"
#include "entity_state.h"
#include "cl_entity.h"
#include "triangleapi.h"
#include "com_model.h"
#include "ts_particles.h"

#include <string.h>
#include <math.h>

// The view basis (cl_dll/view.cpp) — used to build camera-facing billboards.
extern Vector v_origin, v_angles;

//-----------------------------------------------------------------------------
// The pool
//-----------------------------------------------------------------------------
#define TSP_MAX_PARTICLES 2048
#define TSP_MAX_SPRITES   64

struct TSSprite
{
	char name[64];   // +0xc  the "sprites/foo.spr" key (must hold the full path — a too-small
	                 //       buffer truncated the name, broke the cache strcmp, and filled the
	                 //       64-entry table within seconds, killing all effects permanently)
	HSPRITE handle;  // +0x2c SPR_Load result
	short rendermode;// +0x30 kRenderMode (the second registration key)
	short frames;    // +0x32 SPR_Frames(handle)
};

struct TSPool
{
	TSParticle particles[TSP_MAX_PARTICLES];
	TSSprite   sprites[TSP_MAX_SPRITES];
	int        freeCursor;  // +0xc0c lowest known-free slot
	int        highWater;   // +0xc10 highest active slot (loop bound)
	int        nSprites;
	float      lastTime;    // for our own frametime
};

static TSPool g_tsPool;
static TSParticle g_scratch; // the spawner scratch particle (original DAT_1018a720)

//-----------------------------------------------------------------------------
// Manager (FUN_1004ffe0 alloc / FUN_100509d0 register / FUN_100509a0 randframe /
//          FUN_10050b60 commit / FUN_10050b20 free)
//-----------------------------------------------------------------------------

// FUN_1004ffe0 — zero the scratch particle and return it.
static TSParticle* TSP_Scratch()
{
	memset(&g_scratch, 0, sizeof(g_scratch));
	return &g_scratch;
}

// FUN_100509d0 — find/SPR_Load a sprite keyed on (name, rendermode); return its table index, -1 on fail.
static int TSP_RegisterSprite(const char* name, short rendermode)
{
	for (int i = 0; i < g_tsPool.nSprites; i++)
	{
		if (g_tsPool.sprites[i].rendermode == rendermode &&
			strcmp(g_tsPool.sprites[i].name, name) == 0)
			return i;
	}
	if (g_tsPool.nSprites >= TSP_MAX_SPRITES)
		return -1;

	int idx = g_tsPool.nSprites;
	TSSprite& s = g_tsPool.sprites[idx];
	s.handle = SPR_Load(name);
	if (!s.handle)
		return -1;
	s.frames = (short)SPR_Frames(s.handle);
	s.rendermode = rendermode;
	strncpy(s.name, name, sizeof(s.name) - 1);
	s.name[sizeof(s.name) - 1] = 0;
	g_tsPool.nSprites++;
	return idx;
}

int TSParticles_Sprite(const char* name, short rendermode)
{
	return TSP_RegisterSprite(name, rendermode);
}

HSPRITE TSParticles_SpriteHandle(int idx)
{
	if (idx < 0 || idx >= g_tsPool.nSprites)
		return 0;
	return g_tsPool.sprites[idx].handle;
}

// FUN_100509a0 — a random frame in [0, frames-1] for sprite table index `idx`.
static float TSP_RandomFrame(int idx)
{
	if (idx < 0)
		return 0;
	return (float)gEngfuncs.pfnRandomLong(0, g_tsPool.sprites[idx].frames - 1);
}

// FUN_10050b20 — free slot `i`.
static void TSP_Free(int i)
{
	g_tsPool.particles[i].active = false;
	g_tsPool.particles[i].life = 0;
	if (i < g_tsPool.freeCursor)
		g_tsPool.freeCursor = i;
}

// FUN_10050b60 — copy the scratch particle into a free pool slot. Returns slot, or -1 if full.
// Round-robin from freeCursor, WRAPPING and bounded — the cursor must never index past the array
// (an earlier non-wrapping cursor walked off the end and corrupted the pool permanently).
static int TSP_Commit(TSParticle* scratch)
{
	int slot = -1;
	for (int n = 0; n < TSP_MAX_PARTICLES; n++)
	{
		int i = g_tsPool.freeCursor + n;
		if (i >= TSP_MAX_PARTICLES)
			i -= TSP_MAX_PARTICLES;
		if (!g_tsPool.particles[i].active)
		{
			slot = i;
			break;
		}
	}
	if (slot < 0)
		return -1; // pool genuinely full

	TSParticle* p = &g_tsPool.particles[slot];
	*p = *scratch;
	p->active = true;
	p->savedPos = p->pos;            // commit copies pos -> savedPos
	p->spawnTime = gEngfuncs.GetClientTime();
	if (p->drag < 0.1f)              // commit clamps drag >= 0.1 (_DAT_1007a84c)
		p->drag = 0.1f;

	g_tsPool.freeCursor = (slot + 1 >= TSP_MAX_PARTICLES) ? 0 : slot + 1;
	if (slot > g_tsPool.highWater)
		g_tsPool.highWater = slot;
	return slot;
}

//-----------------------------------------------------------------------------
// Per-frame UPDATE + RENDER  (FUN_100520d0)
//-----------------------------------------------------------------------------
static const float TSP_EPS = 0.001f; // _DAT_1007a884

// Drop all cached sprite handles + active particles. MUST be called on a map change (HUD VidInit):
// SPR_Load handles are indices into the per-map client sprite list, so a handle cached on the old map
// is STALE after a load — drawing with it renders garbage / corrupts TriAPI (the bullet/melee impact
// sprites vanished after changing map). Zeroing nSprites forces each sprite to re-SPR_Load a fresh
// handle on next use; clearing the particles avoids carrying dead slots across the load.
void TSParticles_Reset()
{
	memset(&g_tsPool, 0, sizeof(g_tsPool));
}

void TSParticles_Update()
{
	float now = gEngfuncs.GetClientTime();
	float dt = now - g_tsPool.lastTime;
	g_tsPool.lastTime = now;
	if (dt <= 0.0f || dt > 0.2f)
		dt = 0.0f; // map change / first frame: integrate nothing this tick

	// Camera basis for the billboards.
	Vector fwd, right, up;
	AngleVectors(v_angles, fwd, right, up);

	triangleapi_t* tri = gEngfuncs.pTriAPI;
	int newHigh = -1;

	for (int i = 0; i <= g_tsPool.highWater; i++)
	{
		TSParticle* p = &g_tsPool.particles[i];
		if (!p->active)
		{
			if (i < g_tsPool.freeCursor)
				g_tsPool.freeCursor = i;
			continue;
		}

		const int frames = (p->spriteIdx >= 0) ? g_tsPool.sprites[p->spriteIdx].frames : 0;

		// Expiry (FUN_100520d0): keep while life left OR animating OR fading OR shrinking.
		const bool keepLife = (p->life >= TSP_EPS);
		const bool animBusy = (p->spriteIdx >= 0) && (p->frameRate > TSP_EPS) && ((int)p->frame < frames);
		const bool fadeBusy = (p->fadeRate > TSP_EPS) && (p->bright >= TSP_EPS);
		const bool shrinkBusy = (p->accelRate < -TSP_EPS) && (p->size >= TSP_EPS);
		if (!(keepLife || animBusy || fadeBusy || shrinkBusy))
		{
			TSP_Free(i);
			continue;
		}

		// Fade-OUT: once life is spent, bleed the brightness (additive sprite -> invisible).
		if (p->fadeRate != 0.0f && p->life <= 0.0f)
		{
			p->bright -= dt * p->fadeRate * 100.0f; // _DAT_1007a910 = 100
			if (p->bright < 0.0f)
				p->bright = 0.0f;
			if (p->bright > 1.0f)
				p->bright = 1.0f;
		}
		// Size growth/shrink.
		if (p->accelRate != 0.0f)
			p->size += dt * p->accelRate * 100.0f;

		// Integrate motion (gravity + drag), faithful to FUN_10050ce0 imode 0.
		// gravity accel = gravity * _DAT_1007a540(0.1) * _DAT_100963ec(sv_gravity=800) = gravity*80.
		if (dt > 0.0f && p->imode == 0)
		{
			p->vel.z -= dt * p->gravity * 80.0f;
			float d = dt * p->drag;
			if (d > 1.0f)
				d = 1.0f;
			p->vel = p->vel - p->vel * d;
		}
		// orient-1 (debris) tumble: roll angle accumulates at the spin rate (FUN_10050ce0 case 1:
		// angle += spin * frametime * _DAT_1007a538(10)).
		if (dt > 0.0f && p->orient == 1)
			p->angle += p->spin * dt * 10.0f;
		p->savedPos = p->pos;
		// pos += vel * frametime * _DAT_10079e88(10) — velocity is in deci-units/s, the ×10 makes
		// sparks move fast AND turns the velocity-space gravity (×80) into the real 800 u/s² fall.
		p->pos = p->pos + p->vel * (dt * 10.0f);

		// Draw (only particles that carry a sprite).
		if (p->spriteIdx >= 0)
		{
			int frame = (int)p->frame;
			if (frame > frames - 1)
				frame = frames - 1;
			if (frame < 0)
				frame = 0;

			// fade-IN alpha ramp at birth.
			float alpha = 1.0f;
			if (p->fadeIn > 0.0f && now < p->spawnTime + p->fadeIn)
				alpha = (now - p->spawnTime) / p->fadeIn;

			model_t* model = (model_t*)gEngfuncs.GetSpritePointer(g_tsPool.sprites[p->spriteIdx].handle);
			if (model && tri->SpriteTexture(model, frame))
			{
				// Billboard corners. Velocity-stretch (tracer) when fast + stretch set.
				Vector c[4];
				const float half = (p->size > 0.0f) ? p->size : 1.0f;
				float speed = p->vel.Length();
				if (p->shape != 4 && p->stretch > 0.0f && speed * p->stretch > 2.0f * half)
				{
					Vector vdir = p->vel * (1.0f / speed);
					Vector wide = CrossProduct(vdir, fwd);
					float wl = wide.Length();
					wide = (wl > 0.0001f) ? wide * (half / wl) : right * half;
					Vector tail = p->pos;
					Vector head = p->pos + vdir * (speed * p->stretch);
					c[0] = tail - wide;
					c[1] = head - wide;
					c[2] = head + wide;
					c[3] = tail + wide;
				}
				else if (p->shape == 4)
				{
					// FUN_10050ce0: shape 4 copies the quad stored at spawn (the bullet trail).
					for (int k = 0; k < 4; k++)
						c[k] = p->quad[k];
				}
				else
				{
					// Camera-facing quad, rolled by the sprite's angle (debris tumble).
					Vector ax = right, ay = up;
					if (p->angle != 0.0f)
					{
						float rad = p->angle * (3.14159265f / 180.0f);
						float cs = cosf(rad), sn = sinf(rad);
						ax = right * cs + up * sn;
						ay = up * cs - right * sn;
					}
					Vector r = ax * half;
					Vector u = ay * half;
					c[0] = p->pos - r + u;
					c[1] = p->pos + r + u;
					c[2] = p->pos + r - u;
					c[3] = p->pos - r - u;
				}

				tri->RenderMode(g_tsPool.sprites[p->spriteIdx].rendermode);
				tri->CullFace(TRI_NONE);
				tri->Begin(TRI_QUADS);
				tri->Color4f(p->bright, p->bright, p->bright, alpha);
				tri->TexCoord2f(0, 0); tri->Vertex3fv(c[0]);
				tri->TexCoord2f(1, 0); tri->Vertex3fv(c[1]);
				tri->TexCoord2f(1, 1); tri->Vertex3fv(c[2]);
				tri->TexCoord2f(0, 1); tri->Vertex3fv(c[3]);
				tri->End();
				tri->RenderMode(kRenderNormal);
			}
		}

		// Frame advance (after life is spent, like the original).
		if (p->frameRate != 0.0f && p->life < TSP_EPS)
			p->frame += dt * p->frameRate;

		p->life -= dt;
		newHigh = i;
	}

	g_tsPool.highWater = newHigh;
}

//-----------------------------------------------------------------------------
// Spawners
//-----------------------------------------------------------------------------
static inline void VScaleAdd(Vector& out, const Vector& base, const Vector& v, float s)
{
	out = base + v * s;
}

// SCINTILLE — FUN_10046f50. A 1-2 sprite bright HIT-GLINT offset along the surface normal.
// loopBound = (count<=24)?2:1; pos = origin + normal*((i+1)*count*0.5); the spray is separate.
static void TSP_Scintille(const float* o, const float* n, const char* spr, short rendermode, short count)
{
	int idx = TSP_RegisterSprite(spr, rendermode);
	if (idx < 0)
		return;
	const int loops = (count <= 24) ? 2 : 1;
	for (int i = 0; i < loops; i++)
	{
		TSParticle* p = TSP_Scratch();
		p->spriteIdx = (short)idx;
		p->active = true;
		p->orient = 0;
		p->vel = Vector(n[0] * 2, n[1] * 2, n[2] * 2);
		p->fadeRate = 0.05f;
		p->bright = 0.7f;
		p->frameRate = 0;
		p->life = 0;
		p->size = (float)count;
		p->accelRate = (i < 1) ? 0.0f : 1.0f;
		float step = (float)(i + 1) * ((float)count * 0.5f);
		p->pos = Vector(o[0] + n[0] * step, o[1] + n[1] * step, o[2] + n[2] * step);
		p->frame = TSP_RandomFrame(idx);
		if (TSP_Commit(p) == -1)
			return;
	}
}

// SPARK-SHOWER — FUN_100492b0. Hardcodes sprites/sparks.spr (rendermode 5). The visible spray.
static void TSP_SparkShower(const float* o, const float* n, int count, float spreadMul, float speed)
{
	int idx = TSP_RegisterSprite("sprites/sparks.spr", 5 /*kRenderTransAdd*/);
	if (idx < 0)
		return;
	int cnt = gEngfuncs.pfnRandomLong(count / 2, count);
	for (int i = 0; i < cnt; i++)
	{
		TSParticle* p = TSP_Scratch();
		p->spriteIdx = (short)idx;
		p->active = true;
		p->orient = 2;
		p->gravity = 1.0f;
		p->fadeRate = 0.05f;
		p->bright = 0.9f;
		p->frameRate = 0;
		p->stretch = 0.35f;        // +0x128 — sparks streak along motion
		p->drag = 0.1f;            // +0xf4
		p->size = gEngfuncs.pfnRandomFloat(0.5f, 1.15f) * spreadMul; // +0x108
		// velocity = normal*17 + scatter (z biased outward)
		p->vel.x = n[0] * 17.0f + gEngfuncs.pfnRandomFloat(-speed, speed);
		p->vel.y = n[1] * 17.0f + gEngfuncs.pfnRandomFloat(-speed, speed);
		p->vel.z = n[2] * 17.0f + gEngfuncs.pfnRandomFloat(0, speed);
		p->life = gEngfuncs.pfnRandomFloat(0.1f, 1.0f);
		p->collide = (p->life > 0.7f) ? 3 : 0;
		p->pos = Vector(o[0] + n[0] * p->size, o[1] + n[1] * p->size, o[2] + n[2] * p->size);
		p->frame = TSP_RandomFrame(idx);
		if (TSP_Commit(p) == -1)
			return;
	}
}

// SMOKE — FUN_10046680. A CONIC FLARE of additive puffs (gun_smoke_add / wood_smoke_add): the
// puffs are spawned in a CHAIN marching out along the surface normal (pos = origin + normal*size*0.8*i)
// with the size shrinking *0.8 per step, each puff velocity-STRETCHED along its outward velocity
// (stretch 7.0) and GROWING as it ages (accelRate 0.1). Plays its frame animation once (fadeRate 0,
// life 0). `count` = mode-derived puff count; frameRate 50 metal/glass / 20 concrete / 30 dirt-wood.
static void TSP_Smoke(const float* o, const float* n, const char* spr, short rendermode,
	int count, float baseSize, float frameRate)
{
	int idx = TSP_RegisterSprite(spr, rendermode);
	if (idx < 0)
		return;
	int cnt = count < 1 ? 1 : count;
	float size = baseSize;
	// puff 0 sits a touch off the surface (origin + normal*size*0.5).
	Vector pos(o[0] + n[0] * baseSize * 0.5f, o[1] + n[1] * baseSize * 0.5f, o[2] + n[2] * baseSize * 0.5f);
	for (int i = 0; i < cnt; i++)
	{
		TSParticle* p = TSP_Scratch();
		p->spriteIdx = (short)idx;
		p->active = true;
		p->orient = 0;
		p->gravity = 0.1f;        // +0xd0
		p->drag = 3.3f;           // +0xf4 (heavy drag)
		p->bright = 0.5f;         // +0xdc
		p->fadeRate = 0.0f;       // ends by frame-animation, not brightness fade
		p->life = 0.0f;
		p->accelRate = 0.1f;      // +0xe4 — the puff grows as it ages
		p->stretch = 7.0f;        // +0x128 — velocity-stretch -> the conic flare
		// frameRate = base + RandomLong(±base/2) — live-traced range is ~10-30 for base 20, i.e. each
		// puff animates at a noticeably different speed (a big chunk of the per-puff variation).
		int fj = (int)(frameRate * 0.5f);
		p->frameRate = frameRate + (float)gEngfuncs.pfnRandomLong(-fj, fj);
		p->size = size;           // +0x108
		// vel = normal * RandFloat(8,24) / (count - i) + a small (-2,2) x/y scatter. The divisor
		// DECREMENTS each puff (count,…,1 — FUN_10046680's param_11--), so the chain-tip puffs eject
		// progressively faster/farther — that's the conic flare lunging out (live-traced cont.69x).
		float sp = gEngfuncs.pfnRandomFloat(8.0f, 24.0f) / (float)(cnt - i);
		p->vel = Vector(n[0] * sp, n[1] * sp, n[2] * sp);
		p->vel.x += gEngfuncs.pfnRandomFloat(-2.0f, 2.0f);
		p->vel.y += gEngfuncs.pfnRandomFloat(-2.0f, 2.0f);
		p->pos = pos;
		p->frame = 0;             // start at frame 0 -> full billow
		if (TSP_Commit(p) == -1)
			return;
		// next puff: march out along the normal, size shrinks (the cone tip).
		float oldSize = size;
		size *= 0.8f;             // _DAT_1007d570
		pos = Vector(o[0] + n[0] * oldSize * 0.8f * (float)i,
			o[1] + n[1] * oldSize * 0.8f * (float)i,
			o[2] + n[2] * oldSize * 0.8f * (float)i);
	}
}

// DEBRIS — FUN_10046bd0. Tumbling fragment chips (woodfrag / debris_concrete3), gravity + bounce.
static void TSP_Debris(const float* o, const float* n, const char* spr, short rendermode,
	int framecnt, float size, int spread, float life, float gravity, int bounce)
{
	int idx = TSP_RegisterSprite(spr, rendermode);
	if (idx < 0)
		return;
	int cnt = gEngfuncs.pfnRandomLong(framecnt / 2, framecnt);
	for (int i = 0; i < cnt; i++)
	{
		TSParticle* p = TSP_Scratch();
		p->spriteIdx = (short)idx;
		p->active = true;
		p->orient = 1;
		p->gravity = gravity;      // +0xd0
		p->bright = 1.0f;          // +0xdc
		p->fadeRate = (rendermode == 5) ? 0.05f : 0.0f;
		p->frameRate = 0;
		p->renderEnable = 0.3f;    // +0x100
		p->size = size;            // +0x108
		p->collide = bounce ? 3 : 0;
		// velocity = normal*16 + wide scatter (z half-spread).
		p->vel.x = n[0] * 16.0f + gEngfuncs.pfnRandomLong(-spread, spread);
		p->vel.y = n[1] * 16.0f + gEngfuncs.pfnRandomLong(-spread, spread);
		p->vel.z = n[2] * 16.0f + gEngfuncs.pfnRandomLong(-spread / 2, spread / 2);
		float ang = (float)gEngfuncs.pfnRandomLong(-180, 180);
		p->life = ang * 0.001f + life;  // _DAT_1007a868 = 0.001 (the spin value doubles as life jitter)
		p->spin = ang;                  // +0x4c — roll rate (deg/s); orient-1 tumble
		p->angle = 0.0f;                // +0x28 — starts level, accumulates
		p->pos = Vector(o[0] + n[0] * 2, o[1] + n[1] * 2, o[2] + n[2] * 2);
		p->frame = TSP_RandomFrame(idx);
		if (TSP_Commit(p) == -1)
			return;
	}
}

// PELLETS — FUN_10046a70. A single shotgun-pellet mark sprite at the impact.
static void TSP_Pellets(const float* o, const float* n, const char* spr, short rendermode,
	int framecnt, float life, float bright, float fade, float accel)
{
	int idx = TSP_RegisterSprite(spr, rendermode);
	if (idx < 0)
		return;
	TSParticle* p = TSP_Scratch();
	p->spriteIdx = (short)idx;
	p->active = true;
	p->orient = 2;
	p->fadeRate = (rendermode == 5) ? fade : 0.0f;
	p->bright = bright;
	p->frameRate = 0;
	p->size = (float)framecnt;
	p->accelRate = accel;
	p->life = life;
	p->pos = Vector(o[0] + n[0] * 0.1f, o[1] + n[1] * 0.1f, o[2] + n[2] * 0.1f);
	p->savedPos = Vector(n[0], n[1], n[2]);
	p->frame = TSP_RandomFrame(idx);
	TSP_Commit(p);
}

// cl_fx gun-smoke gate (dossier cont94 §5 — the gun_smoke_add spawner FUN_10047130 @0x4716c):
// the smoke spawn is skipped entirely when cl_fx is 0. Exact original test: float != 0.0. Only
// the gun_smoke_add spawns are gated here — the other cl_fx consumers (blur frame cap, the
// FX-root/particle-config reads @0x56dea/0x57d5f) belong to un-reconstructed subsystems.
static bool TSP_FxSmokeOn()
{
	static cvar_t* s_cl_fx = nullptr;
	if (s_cl_fx == nullptr)
		s_cl_fx = gEngfuncs.pfnGetCvarPointer("cl_fx");
	return s_cl_fx == nullptr || s_cl_fx->value != 0.0f;
}

//-----------------------------------------------------------------------------
// Visual dispatcher  (FUN_10048f60) — per-surface texture-char switch + roll gating.
//-----------------------------------------------------------------------------
void TS_ImpactEffect(const float* origin, const float* normal, int mode, char texChar)
{
	const int roll = gEngfuncs.pfnRandomLong(0, 100);
	// smoke puff count = mode-derived (FUN_10048f60: 1 if mode==1, 2 if mode==2, else 4).
	const int smokeN = (mode == 1) ? 1 : (mode == 2) ? 2 : 4;

	switch (texChar)
	{
	case 'D': // dirt — smoke only
		TSP_Smoke(origin, normal, "sprites/wood_smoke_add.spr", 5, smokeN, 16, 30.0f);
		break;

	case 'W': // wood — fragments + smoke (RE FUN_10046bd0: count=RandomLong(5/2,5)=2-5, spread=32)
		TSP_Debris(origin, normal, "sprites/woodfrag.spr", 5, 5, 5.0f, 32, 1.0f, 0.75f, 1);
		TSP_Smoke(origin, normal, "sprites/wood_smoke_add.spr", 5, smokeN, 16, 30.0f);
		break;

	case 'F': // flesh
	case 'S': // slosh/water — nothing (blood/splash elsewhere)
		break;

	case 'G': // grate
	case 'M': // metal
	case 'V': // vent
		if (roll > 33)
			TSP_Scintille(origin, normal, "sprites/scintille_metal.spr", 5, 48);
		TSP_SparkShower(origin, normal, 23, 1.15f, 25.0f);
		if (roll > 70)
			TSP_Pellets(origin, normal, "sprites/shotgun_pellets.spr", 5, 5, 0.0f, 0.75f, 0.015f, 0.25f);
		if (TSP_FxSmokeOn())
			TSP_Smoke(origin, normal, "sprites/gun_smoke_add.spr", 5, smokeN, 16, 50.0f);
		break;

	case 'P': // computer
	case 'Y': // glass
		TSP_Scintille(origin, normal, "sprites/scintille_metal.spr", 5, 48);
		if (roll > 50)
			TSP_SparkShower(origin, normal, 3, 1.15f, 25.0f);
		if (TSP_FxSmokeOn())
			TSP_Smoke(origin, normal, "sprites/gun_smoke_add.spr", 5, smokeN, 16, 50.0f);
		break;

	default: // concrete / tile / snow / etc. (RE: count=RandomLong(5/2,5)=2-5, spread=32)
		// debris_concrete3 is INDEXALPHA — the original's rendermode 1 (TransColor) renders as an
		// unmasked square in our TriAPI; kRenderTransAlpha(4) masks it to white chips. (engine deviation)
		TSP_Debris(origin, normal, "sprites/debris_concrete3.spr", 4, 5, 4.0f, 32, 1.0f, 1.0f, 1);
		if (TSP_FxSmokeOn())
			TSP_Smoke(origin, normal, "sprites/gun_smoke_add.spr", 5, smokeN, 16, 20.0f);
		if (roll > 50)
		{
			TSP_Scintille(origin, normal, "sprites/scintille_metal.spr", 5, 3);
			TSP_SparkShower(origin, normal, 5, 1.15f, 25.0f);
		}
		break;
	}
}

void TS_SurfaceImpact(const float* origin, const float* normal, char texChar, int mode)
{
	TS_ImpactEffect(origin, normal, mode, texChar);
}

// MELEE impact — original client blade handlers (knife LAB_10043c90 / katana LAB_10044230).
// Just the scintille_metal glint (knife 6 / katana 12) + sparks.spr shower (count 7); NO smoke,
// NO debris. Skips dirt/wood/flesh (the original switch breaks with no spark on 'D'/'W'/'F' /
// untextured); everything else sparks.
void TS_MeleeImpact(const float* origin, const float* normal, char texChar, int glints)
{
	if (texChar == 'D' || texChar == 'W' || texChar == 'F' || texChar == 0)
		return;
	TSP_Scintille(origin, normal, "sprites/scintille_metal.spr", 5, glints);
	TSP_SparkShower(origin, normal, 7, 1.15f, 25.0f);
}

//-----------------------------------------------------------------------------
// TSPSys — the particle-system trigger message (RE dossier cont94 §3, handler
// FUN_1004bfd0). Only subtype 3 (wind) exists client-side; every other subtype
// is ignored by the original client. Wire: BYTE 3, LONG seed, LONG yawDeg,
// COORD strength (byte-for-byte our dlls/ts_emitter.cpp CTSWind::BlowThink).
//-----------------------------------------------------------------------------
TSWindState g_tsWindState = {};

static int __MsgFunc_TSPSys(const char* pszName, int iSize, void* pbuf)
{
	BEGIN_READ(pbuf, iSize);
	const int subtype = READ_BYTE();
	if (subtype != 3)
		return 1; // read-and-ignore (the original returns without parsing further)

	const int seed = READ_LONG();
	const int yawDeg = READ_LONG();
	const float strength = READ_COORD();

	Vector angles(0.0f, (float)yawDeg, 0.0f);
	Vector fwd;
	gEngfuncs.pfnAngleVectors(angles, fwd, NULL, NULL);

	// Current + target set EQUAL on receive (the per-frame wind pass lerps them apart later).
	g_tsWindState.seed = seed;
	g_tsWindState.dirCurrent = fwd;
	g_tsWindState.dirTarget = fwd;
	g_tsWindState.strengthCurrent = strength;
	g_tsWindState.strengthTarget = strength;
	g_tsWindState.changeTime = gEngfuncs.GetClientTime();
	// CONSUMER DEFERRED: the TSP wind pass (~FUN@0x10051aa0) that applies this to the
	// particles is un-reconstructed — g_tsWindState is the seam it will read.
	return 1;
}

// BULLET TRAIL — FUN_10046240 (from the client bullet simulation's cl_wavedetail gate, 0x10035a66).
// Shape 4: the corners are fixed at spawn from the view up of that frame, last - up*size,
// last + up*size, pos + up*size, pos - up*size. Life 0, so it only fades: bright = alpha * 0.75
// (_DAT_1007d474), fade rate 0.1 (bright -= dt * 10), no motion.
void TSP_BulletTrail(const Vector& pos, const Vector& last, float size, float alpha)
{
	const int idx = TSP_RegisterSprite("sprites/bullet_trail.spr", kRenderTransAdd);
	if (idx == -1)
		return;
	Vector fwd, right, up;
	AngleVectors(v_angles, fwd, right, up);

	TSParticle* p = TSP_Scratch();
	p->orient = 0;
	p->active = true;
	p->bright = alpha * 0.75f;
	p->fadeRate = 0.1f;
	p->spriteIdx = (short)idx;
	p->size = size;
	p->pos = pos;
	p->shape = 4;
	p->quad[0] = last - up * size;
	p->quad[1] = last + up * size;
	p->quad[2] = pos + up * size;
	p->quad[3] = pos - up * size;
	TSP_Commit(p);
}

void TSParticles_Init()
{
	memset(&g_tsWindState, 0, sizeof(g_tsWindState));
	gEngfuncs.pfnHookUserMsg("TSPSys", __MsgFunc_TSPSys);
}

//-----------------------------------------------------------------------------
// PTakeDam blood tail (FUN_10048840) — the bloodfrag2.spr spray on the TSP pool.
// Called from CHudHealth::MsgFunc_PTakeDam (cl_dll/health.cpp). The original does
// NOTHING but blood here (no pain-direction indicator).
//-----------------------------------------------------------------------------
void TSP_BloodSpray(const float* point, const float* dir, int encByte)
{
	// (The original first runs view-relative visibility culls — FUN_100570b0 +
	// FUN_10057270/FUN_100572a0 — and skips unseen sprays; our pool simply doesn't
	// render what's offscreen, so the cull is omitted.)

	Vector bloodvel(-dir[0], -dir[1], -dir[2]); // inverted hit dir (dir * -1.0)
	const bool armored = encByte > 127;         // bit7 = armored/upper-body hit
	int enc = encByte;
	if (armored)
		enc -= 128;
	float base = (float)enc * 2.0f; // server encodes dmg*0.5 -> base ≈ original damage

	if (armored)
	{
		// Armored: jitter bloodvel per component + normalize, and spawn a gun_smoke_add
		// dust puff INSTEAD of extra blood (FUN_100464f0, count 5, near point + dir*6 —
		// exact params decompiler-elided, VERIFY).
		Vector jit(bloodvel.x + gEngfuncs.pfnRandomFloat(-1.0f, 1.0f),
			bloodvel.y + gEngfuncs.pfnRandomFloat(-1.0f, 1.0f),
			bloodvel.z + gEngfuncs.pfnRandomFloat(-1.0f, 1.0f));
		jit = jit.Normalize();
		Vector puffAt(point[0] + dir[0] * 6.0f, point[1] + dir[1] * 6.0f, point[2] + dir[2] * 6.0f);
		TSP_Smoke(puffAt, jit, "sprites/gun_smoke_add.spr", 5, 5, 16, 50.0f);
		// base is further randomized + halved (decompiler-elided; VERIFY the jitter form).
		base *= 0.5f;
	}

	// Blood particle count from cl_gore (client_display_cvars §3 table, confirmed):
	// 0 -> 0 · 1 -> base*0.5 · 2 -> base · 3 -> base*2.5 · 4 -> base*5 · 5..10 -> base*10 ·
	// >10 -> base*min(v,33).
	static cvar_t* s_cl_gore = nullptr;
	if (s_cl_gore == nullptr)
		s_cl_gore = gEngfuncs.pfnGetCvarPointer("cl_gore");
	const int gore = (int)(s_cl_gore != nullptr ? s_cl_gore->value : 2.0f);
	float fcount;
	if (gore <= 0)
		return;
	else if (gore == 1)
		fcount = base * 0.5f;
	else if (gore == 2)
		fcount = base;
	else if (gore == 3)
		fcount = base * 2.5f;
	else if (gore == 4)
		fcount = base * 5.0f;
	else if (gore <= 10)
		fcount = base * 10.0f;
	else
		fcount = base * (float)(gore < 33 ? gore : 33);
	const int count = (int)fcount;
	if (count <= 0)
		return;

	// Template (FUN_1004ffe0 + FUN_100509d0, texture sprites/bloodfrag2.spr). Rendermode
	// VERIFY: bloodfrag2 is indexalpha — kRenderTransAlpha(4) masks it in our TriAPI (the
	// debris_concrete3 precedent); the template's unmapped dword [0x48]=2 may be the
	// original registration mode (kRenderTransTexture).
	const int idx = TSP_RegisterSprite("sprites/bloodfrag2.spr", 4 /*kRenderTransAlpha*/);
	if (idx < 0)
		return;

	// size = clamp(base * 0.075, 1..5)  (template dword [0x42])
	float size = base * 0.075f;
	if (size < 1.0f) size = 1.0f;
	if (size > 5.0f) size = 5.0f;

	Vector pos(point[0], point[1], point[2]);
	float droop = 0.0f; // the accumulating streak droop
	for (int i = 0; i < count; i++)
	{
		if (i > 0)
		{
			// the spawn pos WALKS a streak along bloodvel, drooping as it goes.
			pos = pos + bloodvel * (size * 0.3f);
			droop += size * 0.1f;
			pos.z -= droop / (float)count;
		}

		// skip particles inside solid (PM_PointContents == CONTENTS_SOLID(-2)).
		if (gEngfuncs.PM_PointContents((float*)&pos, NULL) == CONTENTS_SOLID)
			continue;

		TSParticle* p = TSP_Scratch();
		p->spriteIdx = (short)idx;
		p->active = true;   // byte@0 = 1
		p->collide = 3;     // byte@4 = 3 (VERIFY field mapping — collide/orient ambiguity)
		p->gravity = 0.5f;  // [0x34]
		p->bright = 1.0f;   // [0x37]
		p->fadeRate = 0.0f; // [0x38]
		p->frameRate = 0.0f;// [0x3f]
		p->size = size;     // [0x42]
		p->stretch = 2.0f;  // [0x4a]
		p->accelRate = 0.075f; // per-particle [0x39] — the droplet grows as it flies
		p->life = gEngfuncs.pfnRandomFloat(0.0f, 1.0f); // per-particle [0x36] = RandomFloat (range elided, VERIFY)
		if (i == 0)
		{
			// the main spurt: straight along the HIT direction.
			p->vel = Vector(dir[0], dir[1], dir[2]) * 32.0f;
			p->pos = Vector(point[0], point[1], point[2]);
		}
		else
		{
			p->vel = bloodvel * gEngfuncs.pfnRandomFloat(0.0f, 1.0f); // VERIFY range (elided)
			p->vel.x += gEngfuncs.pfnRandomFloat(-2.0f, 2.0f); // xy jitter (range elided, VERIFY)
			p->vel.y += gEngfuncs.pfnRandomFloat(-2.0f, 2.0f);
			p->pos = pos;
		}
		p->frame = TSP_RandomFrame(idx);
		if (TSP_Commit(p) == -1)
			return;
	}
}
