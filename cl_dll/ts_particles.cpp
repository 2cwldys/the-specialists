/***
*  ts_particles.cpp — The Specialists CLIENT sprite-particle pool. See ts_particles.h.
*  A reconstruction of client.dll's sprite pool (one global pool of 300-byte particles, drawn per
*  frame via gEngfuncs.pTriAPI), its cl_fx configuration and the impact-effect spawners.
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
#include "pm_defs.h"
#include "event_api.h"
#include "r_studioint.h"
#include "r_efx.h"
#include "pmtrace.h"
#include "dlight.h"
#include "com_weapons.h" // UTIL_SharedRandomFloat/Long (client.dll FUN_1001aea0/FUN_1001ae60)
#include "ts_particles.h"

#include <stdio.h>
#include <string.h>
#include <math.h>

// The view basis (cl_dll/view.cpp): the refdef origin/angles and the frame time V_CalcRefdef saw.
extern Vector v_origin, v_angles;
extern float v_frametime;
extern float g_flClientSlow;      // client.dll DAT_100963e8, the eased slow-motion scale (ts_powerup_hud.cpp)
extern float g_flClientGravity;   // client.dll DAT_100963ec, HUD_TempEntUpdate's cl_gravity (entity.cpp)
extern engine_studio_api_t IEngineStudio;
extern Vector g_tspLocalSimVel;   // client.dll DAT_10191d20, ref_params simvel (view.cpp)

Vector g_tspViewForward, g_tspViewRight, g_tspViewUp;
TSFxConfig g_tsFx;

//-----------------------------------------------------------------------------
// The pool (client.dll DAT_1018bde8)
//-----------------------------------------------------------------------------
#define TSP_MAX_PARTICLES 0x1000 // FUN_100507e0 holds the size to 0x100..0x1000
#define TSP_MAX_SPRITES   64

struct TSSprite
{
	char name[64];   // +0xc  the "sprites/foo.spr" key
	HSPRITE handle;  // +0x2c SPR_Load result
	short rendermode;// +0x30 kRenderMode (the second registration key)
	short frames;    // +0x32 SPR_Frames(handle)
};

struct TSPool
{
	int max;         // +0x04 slots allocated (0 = not yet: DAT_1018bdf0 is the array pointer)
	TSParticle particles[TSP_MAX_PARTICLES];
	TSSprite   sprites[TSP_MAX_SPRITES];
	float      throttle[0x80]; // +0xa0c per-kind spawn timers (FUN_10050ad0/FUN_10050b00)
	int        freeCursor;     // +0xc0c lowest known-free slot
	int        highWater;      // +0xc10 highest active slot (loop bound)
	int        nSprites;
};

static TSPool g_tsPool;
static TSParticle g_scratch; // the spawner scratch particle (original DAT_1018a720)
static float g_fxCachedValue; // DAT_1018cac4: the cl_fx value the configuration was built for
static bool g_fxConfigured;   // DAT_1018d16a: the FX root's first frame is done

static cvar_t* TSP_ClFx()
{
	static cvar_t* s_cl_fx = nullptr;
	if (s_cl_fx == nullptr)
		s_cl_fx = gEngfuncs.pfnGetCvarPointer("cl_fx");
	return s_cl_fx;
}

static float TSP_Now()
{
	return gEngfuncs.GetClientTime();
}

// FUN_10056fa0: an interval stretched by the slow motion (x / max(slow, 0.01)).
static float TSP_SlowScaled(float interval)
{
	const float slow = (g_flClientSlow < 0.01f) ? 0.01f : g_flClientSlow;
	return interval * (1.0f / slow);
}

//-----------------------------------------------------------------------------
// cl_fx configuration (FUN_10057d50) and its cl_fx 1 retune (FUN_10057a70)
//-----------------------------------------------------------------------------
static void TSP_FxConfigure(float value)
{
	memset(&g_tsFx, 0, sizeof(g_tsFx));
	if (value != 2.0f && value != 1.0f)
	{
		g_tsFx.low = true;
		g_tsFx.maxParticles = 0x800;
		g_tsFx.scale = 0.35f;
		g_tsFx.rate1 = 0.2f;
		g_tsFx.rate2 = 0.3f;
		g_tsFx.rate3 = 0.2f;
		g_tsFx.high = false;
		g_tsFx.countScale = 0.7f;
		g_tsFx.detail = 5.0f;
		g_tsFx.interact = false;
		g_tsFx.divisor = 8;
	}
	else
	{
		g_tsFx.low = false;
		g_tsFx.maxParticles = 0x1000;
		g_tsFx.scale = 1.0f;
		g_tsFx.rate1 = 0.05f;
		g_tsFx.rate2 = 0.05f;
		g_tsFx.rate3 = 0.05f;
		g_tsFx.high = true;
		g_tsFx.countScale = 1.5f;
		g_tsFx.detail = 30.0f;
		g_tsFx.interact = true;
		g_tsFx.divisor = 1;
	}
	g_fxCachedValue = value;
}

// FUN_10057a70, run each frame while cl_fx is exactly 1: every 0.1 s keep the frame time x 100 in a
// ring of 20, every second turn the average (held to 1.11..3.33, about 90 down to 30 fps) into a
// quality q = 1 .. 0 and set the levels from it. q == 0 leaves the levels as they were.
static void TSP_FxRetune()
{
	const float now = TSP_Now();
	if (now < g_tsFx.nextSample)
		return;
	g_tsFx.nextSample = now + 0.1f;
	g_tsFx.samples[g_tsFx.sampleIndex] = v_frametime * 100.0f;
	if (++g_tsFx.sampleIndex > 19)
		g_tsFx.sampleIndex = 0;

	if (now < g_tsFx.nextTune)
		return;
	g_tsFx.nextTune = now + 1.0f;
	float avg = 0.0f;
	for (int i = 0; i < 20; i++)
		avg += g_tsFx.samples[i];
	avg *= 0.05f;
	if (avg >= 3.33)
		avg = 3.33f;
	else if (avg < 1.11)
		avg = 1.11f;
	const float q = 1.0f - (avg - 1.11f) * 0.45045f;
	if (q == 0.0f)
		return;

	g_tsFx.high = (q >= 0.3);
	g_tsFx.low = !g_tsFx.high;
	g_tsFx.scale = (q <= 0.25) ? 0.25f : q;
	float r = 0.05f / q;
	if (r >= 0.3)
		r = 0.3f;
	g_tsFx.rate1 = r;
	g_tsFx.rate2 = r;
	g_tsFx.rate3 = r;
	g_tsFx.countScale = q + q;
	if (g_tsFx.countScale <= 0.1)
		g_tsFx.countScale = 0.1f;
	g_tsFx.detail = q * 30.0f;
	if (g_tsFx.detail <= 2.0f)
		g_tsFx.detail = 2.0f;
	if (q < 0.374)
		g_tsFx.divisor = 6;
	else if (q < 0.599)
		g_tsFx.divisor = 4;
	else if (q < 0.749)
		g_tsFx.divisor = 2;
	else
		g_tsFx.divisor = 1;
	g_tsFx.interact = !(q <= 0.374);
}

//-----------------------------------------------------------------------------
// Manager (FUN_100507e0 alloc / FUN_1004ffe0 scratch / FUN_100509d0 register / FUN_100509a0 randframe /
//          FUN_10050b60 commit / FUN_10050b20 free)
//-----------------------------------------------------------------------------

// FUN_100507e0: size the pool (0x100..0x1000) and clear particles, sprites and throttles.
static void TSP_Alloc(int count)
{
	if (count < 0x100)
		count = 0x100;
	else if (count > 0x1000)
		count = 0x1000;
	memset(&g_tsPool, 0, sizeof(g_tsPool));
	g_tsPool.max = count;
	g_tsFx.maxParticles = count;
}

bool TSP_PoolReady()
{
	return g_tsPool.max != 0;
}

// FUN_1004ffe0 — zero the scratch particle and return it.
TSParticle* TSP_Scratch()
{
	memset(&g_scratch, 0, sizeof(g_scratch));
	return &g_scratch;
}

// FUN_100509d0 — find/SPR_Load a sprite keyed on (name, rendermode); return its table index, -1 on fail.
int TSP_RegisterSprite(const char* name, short rendermode)
{
	for (int i = 0; i < g_tsPool.nSprites; i++)
	{
		if (g_tsPool.sprites[i].rendermode == rendermode &&
			stricmp(g_tsPool.sprites[i].name, name) == 0)
			return i;
	}
	if (g_tsPool.nSprites >= TSP_MAX_SPRITES)
		return -1;

	HSPRITE h = SPR_Load(name);
	if (h == 0)
	{
		char msg[120];
		snprintf(msg, sizeof(msg), "WARNING: cannot found %s", name);
		gHUD.m_SayText.SayTextPrint(msg, strlen(msg) + 1);
		return -1;
	}

	TSSprite* s = &g_tsPool.sprites[g_tsPool.nSprites];
	strncpy(s->name, name, sizeof(s->name) - 1);
	s->name[sizeof(s->name) - 1] = 0;
	s->handle = h;
	s->rendermode = rendermode;
	s->frames = (short)SPR_Frames(h);
	return g_tsPool.nSprites++;
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

// FUN_100509a0 — a random start frame for sprite `idx`.
static float TSP_RandomFrame(int idx)
{
	if (idx < 0)
		return 0;
	return (float)gEngfuncs.pfnRandomLong(0, g_tsPool.sprites[idx].frames - 1);
}

// FUN_10050b20 — free slot `i`.
void TSP_Free(int i)
{
	if (!TSP_PoolReady() || i < 0 || i >= g_tsPool.max)
		return;
	g_tsPool.particles[i].active = false;
	g_tsPool.particles[i].life = 0;
	if (i < g_tsPool.freeCursor)
		g_tsPool.freeCursor = i;
}

// FUN_10050b60 — copy the scratch particle into the first free slot at or above the free cursor.
// Returns the slot, or -1 when none is free above it (the cursor only moves down, in TSP_Free and
// the update, so a full upper range fails a commit until the update finds a lower free slot).
int TSP_Commit(TSParticle* scratch, TSParticleThink think, TSParticleCollide onCollide)
{
	if (!TSP_PoolReady())
		return -1;
	// A kRenderTransColor sprite needs the hardware renderer.
	if (scratch->spriteIdx >= 0 && g_tsPool.sprites[scratch->spriteIdx].rendermode == kRenderTransColor &&
		0 == IEngineStudio.IsHardware())
		return -1;

	int slot = -1;
	for (int i = g_tsPool.freeCursor; i < g_tsPool.max; i++)
	{
		if (!g_tsPool.particles[i].active)
		{
			slot = i;
			break;
		}
	}
	if (slot < 0)
		return -1;

	TSParticle* p = &g_tsPool.particles[slot];
	*p = *scratch;
	p->slot = (short)slot;
	p->savedPos = p->pos;
	p->randVel = Vector(0, 0, 0);
	p->settled = false;
	p->nextCollide = 0;
	p->spawnTime = TSP_Now();
	if (p->drag < 0.1f) // _DAT_1007a84c
		p->drag = 0.1f;
	p->think = think;
	p->onCollide = onCollide;
	p->nextPerturb = 0;
	if (slot > g_tsPool.highWater)
		g_tsPool.highWater = slot;
	return slot;
}

//-----------------------------------------------------------------------------
// Per-frame motion (FUN_10050ce0 and the passes it calls)
//-----------------------------------------------------------------------------
static const float TSP_EPS = 0.001f; // _DAT_1007a884

// FUN_10057270: the camera the culls measure from (client.dll: the third-person camera origin when
// in third person, else the view origin; reTS's refdef origin is the camera in both cases).
static const Vector& TSP_CameraOrigin()
{
	return v_origin;
}

// FUN_100570b0: within 300 units always seen; farther, not behind the view and not too far off its
// centre (the off-centre allowance shrinks with distance).
static bool TSP_InView(const Vector& at)
{
	Vector d = at - TSP_CameraOrigin();
	const float dist = d.Length();
	if (!(dist > 300.0f))
		return true;
	d = d * (1.0f / dist);
	if (DotProduct(g_tspViewForward, d) < 0.0f)
		return false;
	const float r = fabs(DotProduct(g_tspViewRight, d));
	const float u = fabs(DotProduct(g_tspViewUp, d));
	const float lod = (float)((dist * 0.00012207f * 0.2 + 1.0) * (r + u * 0.5));
	return !(lod > 0.9f);
}

// FUN_100572a0: a clear line from the camera to the point (a world trace that ignores glass and the
// local player, hull 2, run to the end).
static bool TSP_LineClear(const Vector& at)
{
	cl_entity_t* lp = gEngfuncs.GetLocalPlayer();
	pmtrace_t tr;
	gEngfuncs.pEventAPI->EV_SetTraceHull(2);
	gEngfuncs.pEventAPI->EV_PlayerTrace((float*)&TSP_CameraOrigin(), (float*)&at, PM_GLASS_IGNORE, lp ? lp->index : -1, &tr);
	return tr.fraction == 1.0f;
}

// The impact spawners' first test: in view (FUN_100570b0) and in sight (FUN_100572a0).
static bool TSP_Seen(const Vector& at)
{
	return TSP_InView(at) && TSP_LineClear(at);
}

// FUN_10050ad0 / FUN_10050b00: a spawner kind may spawn again once its timer has passed; the timer is
// then set the given interval ahead.
static bool TSP_Due(int kind)
{
	return g_tsPool.throttle[kind] <= TSP_Now();
}

static void TSP_Hold(int kind, float interval)
{
	g_tsPool.throttle[kind] = TSP_Now() + interval;
}

// FUN_10051d10: the particle lands and stops.
static void TSP_Settle(TSParticle* p)
{
	p->settled = true;
	p->vel = Vector(0, 0, 0);
	p->avel.z = 0;
	p->angles.z = 0;
}

// FUN_10051d50: trace this frame's move against the world. The trace is throttled by distance (every
// 0.016 s up close, rarer far away; zoomed in counts half the distance); a hit puts the particle on the
// surface, bounces it (keeps 0.3 of the speed, less far away), lands it when it barely moved, and runs
// its onCollide. A start in solid kills a young particle and lands an older one.
static void TSP_Collide(TSParticle* p)
{
	if (p->collideMode == 0)
	{
		p->savedPos = p->pos;
		return;
	}
	const Vector moved = p->savedPos - p->pos;
	if (DotProduct(moved, moved) < TSP_EPS || !p->drawn)
		return;
	p->drawn = TSP_InView(p->pos);
	if (!p->drawn)
		return;
	const float now = TSP_Now();
	if (now < p->nextCollide)
		return;

	const double scale = (gHUD.m_iFOV > 39) ? 1.0 : 0.5;
	const double dist = (p->pos - TSP_CameraOrigin()).Length() * scale;
	float k = (float)(dist * 0.00012207f);
	if (1024.0f < dist)
		k += k;
	p->nextCollide = ((0.016f < k) ? k : 0.016f) + now;

	pmtrace_t tr;
	gEngfuncs.pEventAPI->EV_SetTraceHull(2);
	gEngfuncs.pEventAPI->EV_PlayerTrace(p->savedPos, p->pos, PM_STUDIO_BOX, -1, &tr);
	if (tr.fraction == 0.0f)
	{
		if (now >= p->spawnTime + 0.5f)
			TSP_Settle(p);
		else
			TSP_Free(p->slot);
	}
	else if (tr.fraction != 1.0f)
	{
		if (gEngfuncs.PM_PointContents(p->savedPos, NULL) == CONTENTS_SOLID)
			TSP_Free(p->slot);
		p->pos = tr.endpos;
		const float movedLen = (p->pos - p->savedPos).Length();
		float keep = 1.0f - k * 7.0f;
		if (keep < 0.0f)
			keep = 0.0f;
		p->vel = p->vel * (0.3f * keep);
		const Vector n = tr.plane.normal;
		p->vel = p->vel + n * (DotProduct(n, p->vel) * -2.0f);
		if (g_flClientSlow * movedLen < p->drag * 3.0f)
			TSP_Settle(p);
		if (p->onCollide)
			p->onCollide(p, &tr);
	}
	p->savedPos = p->pos;
}

// FUN_10051270: the step that runs every divisor/30 s (at least a frame) per particle: the random
// wander toward a new velocity in randVelLow..High, at most randomDelta per step; passing players
// pushing it away (cl_fx interact, within 128 units); the map's wind. The wander and the push only
// run within 512 units of the camera.
static void TSP_Perturb(TSParticle* p, float dt)
{
	const double scale = (gHUD.m_iFOV > 39) ? 1.0 : 0.5;
	const float dist = (float)((p->pos - TSP_CameraOrigin()).Length() * scale);
	float interval = (float)g_tsFx.divisor * 0.0333333f;
	if (interval < dt)
		interval = dt;

	if (p->randomDelta != 0.0f && dist < 512.0f)
	{
		Vector want;
		want.x = gEngfuncs.pfnRandomFloat(p->randVelHigh.x, p->randVelLow.x);
		want.y = gEngfuncs.pfnRandomFloat(p->randVelHigh.y, p->randVelLow.y);
		want.z = gEngfuncs.pfnRandomFloat(p->randVelHigh.z, p->randVelLow.z);
		Vector step = want - p->randVel;
		if (p->randomDelta < step.Length())
			step = step.Normalize() * p->randomDelta;
		p->randVel = p->randVel + step;
		p->vel = p->vel + p->randVel * interval;
	}

	if (g_tsFx.interact)
	{
		if (p->react != 0.0f && !p->settled && dist < 512.0f)
		{
			cl_entity_t* local = gEngfuncs.GetLocalPlayer();
			for (int i = 0; i <= gEngfuncs.GetMaxClients(); i++)
			{
				cl_entity_t* ent = gEngfuncs.GetEntityByIndex(i);
				if (ent == NULL || ent->player != 1 || ent->curstate.solid == 0)
					continue;
				const Vector d = p->pos - ent->origin;
				const float len = d.Length();
				if (len > 128.0f)
					continue;
				Vector v;
				if (local != NULL && local->index == ent->index)
				{
					v = Vector(g_tspLocalSimVel.x, g_tspLocalSimVel.y, 0);
				}
				else
				{
					const Vector moveDelta = ent->curstate.origin - ent->prevstate.origin;
					const float dt2 = ent->curstate.msg_time - ent->prevstate.msg_time;
					v = Vector(moveDelta.x / dt2, moveDelta.y / dt2, 0);
				}
				const float speed = v.Length();
				if (!(speed > 0.0f))
					continue;
				const Vector vn = v.Normalize();
				const Vector dn = (len == 0.0f) ? Vector(0, 0, 1) : d * (1.0f / len);
				const float along = dn.y * vn.y + dn.x * vn.x + 0.0f;
				// The away push; the binary's sideways term multiplies (vn.x - vn.x) and is always 0.
				const float a = (128.0f - len) * speed * along * 0.0078125f;
				const Vector push(dn.x * a, dn.y * a, 0);
				p->pos = p->pos + push * (interval * p->react);
			}
		}

		if (g_tsFx.interact && p->wind != 0.0f && !p->settled)
		{
			const float now = TSP_Now();
			TSWindState& w = g_tsWindState;
			if (now > w.changeTime + w.changeDuration)
			{
				w.strengthPrev = w.strengthTarget;
				w.dirPrev = w.dirTarget;
				w.dirBase.x = UTIL_SharedRandomFloat(w.seed, 0.0f, w.dirBase.x);
				w.dirBase.y = UTIL_SharedRandomFloat(w.seed, 0.0f, w.dirBase.y);
				w.dirBase.z = UTIL_SharedRandomFloat(w.seed, 0.0f, w.dirBase.z);
				w.strengthTarget = UTIL_SharedRandomFloat(w.seed, 0.0f, w.strengthBase);
				w.changeTime = TSP_Now();
				w.changeDuration = UTIL_SharedRandomFloat(w.seed, 0.001f, 1.0f);
				w.seed = UTIL_SharedRandomLong(w.seed, 0, 30000);
			}
			// FUN_10050760: cosine ease, f = (1 - cos(t pi)) / 2.
			const float t = (TSP_Now() - w.changeTime) / w.changeDuration;
			const float f = (float)((1.0 - cos(t * 3.14159265358979)) * 0.5);
			Vector dir(f * w.dirTarget.x + (1.0f - f) * w.dirPrev.x,
				f * w.dirTarget.y + (1.0f - f) * w.dirPrev.y,
				f * w.dirTarget.z + (1.0f - f) * w.dirPrev.z);
			const float strength = f * w.strengthTarget + (1.0f - f) * w.strengthPrev;
			if (strength > 0.0f)
			{
				const float dl = dir.Length();
				const Vector dn = (dl == 0.0f) ? Vector(0, 0, 1) : dir * (1.0f / dl);
				p->vel = p->vel + dn * (strength * interval * p->wind);
			}
		}
	}

	p->nextPerturb = TSP_Now() + interval;
}

// FUN_10050080: the four corners of the quad (12 floats) and their s/t (8 floats).
static void TSP_BuildQuad(float* out, const Vector& pos, const Vector& ang, float size, bool viewFacing,
	const Vector* axis, float stretch, const Vector& vel)
{
	Vector c[4];
	bool built = false;
	if (stretch > 0.01f)
	{
		Vector vd = vel;
		const float len = VectorNormalize(vd) * stretch;
		if (size + size < len)
		{
			const Vector half = vd * (len * 0.5f);
			Vector toView = v_origin - pos;
			const float tl = toView.Length();
			toView = (tl == 0.0f) ? Vector(0, 0, 1) : toView * (1.0f / tl);
			const Vector side = CrossProduct(toView, vd) * size;
			c[0] = pos + half - side;
			c[1] = pos + half + side;
			c[2] = pos - half + side;
			c[3] = pos - half - side;
			built = true;
		}
	}
	if (!built)
	{
		Vector up, right;
		if (axis != NULL)
		{
			up = *axis * size;
			Vector toView = v_origin - pos;
			const float tl = toView.Length();
			toView = (tl == 0.0f) ? Vector(0, 0, 1) : toView * (1.0f / tl);
			right = CrossProduct(toView, *axis);
		}
		else if (viewFacing)
		{
			up = g_tspViewUp * size;
			right = g_tspViewRight;
		}
		else
		{
			gEngfuncs.pfnAngleVectors(ang, NULL, right, up);
			up = up * size;
		}
		right = right * size;
		c[0] = pos + up - right;
		c[1] = pos + up + right;
		c[2] = pos - up + right;
		c[3] = pos - up - right;
	}
	for (int i = 0; i < 4; i++)
	{
		out[i * 3 + 0] = c[i].x;
		out[i * 3 + 1] = c[i].y;
		out[i * 3 + 2] = c[i].z;
	}
	out[12] = 0; out[16] = 0;
	out[13] = 1; out[17] = 0;
	out[14] = 1; out[18] = 1;
	out[15] = 0; out[19] = 1;
}

// FUN_10050ce0: move the particle (gravity, drag with less vertical drag the heavier it is, the
// perturb step, vel * dt * 10) or ride its entity, trace it, then build its quad by orient.
static void TSP_Move(TSParticle* p, float* quad, float dt)
{
	if (p->imode == 0)
	{
		if (!p->settled)
		{
			const float step = dt * 10.0f;
			if (p->gravity != 0.0f)
				p->vel.z += (float)(dt * p->gravity * 0.1) * -g_flClientGravity;
			float g = fabs(p->gravity);
			const float zDrag = (1.0 - g <= 0.0) ? 0.0f : (float)(1.0 - g);
			float d = p->drag * dt;
			if (1.0 <= d)
				d = 1.0f;
			p->vel = p->vel - Vector(p->vel.x * d, p->vel.y * d, zDrag * p->vel.z * d);
			if (p->nextPerturb < TSP_Now())
				TSP_Perturb(p, dt);
			p->pos = p->pos + p->vel * step;
		}
	}
	else
	{
		cl_entity_t* ent = gEngfuncs.GetEntityByIndex(p->imode - 1);
		if (ent != NULL)
		{
			if (p->attachment == 0)
				p->pos = p->attachOffset + ent->origin;
			else
				p->pos = p->attachOffset + ent->attachment[p->attachment];
		}
	}

	if (p->collide == 3 && !p->settled)
		TSP_Collide(p);
	if (p->collide == 4)
	{
		memcpy(quad, p->quad, sizeof(p->quad));
		return;
	}

	float stretch = p->stretch;
	const Vector* axis = NULL;
	bool viewFacing = true;
	Vector ang(0, 0, 0);
	if (p->orient != 0 && !p->settled)
	{
		if (p->orient == 1)
		{
			viewFacing = false;
			p->angles.z = (float)(p->avel.z * dt * 10.0 + p->angles.z);
			ang = Vector(v_angles.x, v_angles.y, p->angles.z);
		}
		else if (p->orient == 2)
		{
			viewFacing = false;
			p->angles = p->angles + p->avel * (dt * 10.0f);
			ang = p->angles;
			stretch = 0.0f;
		}
		else if (p->orient == 3)
		{
			axis = &p->followAxis;
			stretch = 0.0f;
		}
	}
	TSP_BuildQuad(quad, p->pos, ang, p->size, viewFacing, axis, stretch, p->vel);
}

//-----------------------------------------------------------------------------
// Per-frame UPDATE + RENDER (the FX root FUN_10056dd0's pool part: the cl_fx configuration, the view
// vectors, FUN_100520d0, then the cl_fx 1 retune)
//-----------------------------------------------------------------------------

// Drop all cached sprite handles + active particles. MUST be called on a map change (HUD VidInit):
// SPR_Load handles are indices into the per-map client sprite list, so a handle cached on the old map
// is stale after a load. Keeps the pool's size (client.dll allocates it once).
void TSParticles_Reset()
{
	const int max = g_tsPool.max;
	memset(&g_tsPool, 0, sizeof(g_tsPool));
	g_tsPool.max = max;
}

void TSParticles_Update()
{
	cvar_t* clfx = TSP_ClFx();
	const float fx = clfx ? clfx->value : 1.0f;
	if (!g_fxConfigured)
	{
		TSP_FxConfigure(fx);
		TSP_Alloc(g_tsFx.maxParticles); // FUN_10056bc0
		g_fxConfigured = true;
	}
	else if (fx != g_fxCachedValue)
	{
		TSP_FxConfigure(fx);
	}

	// FUN_10056f60
	gEngfuncs.pfnAngleVectors(v_angles, g_tspViewForward, g_tspViewRight, g_tspViewUp);

	const float dt = g_flClientSlow * v_frametime; // FUN_100520c0
	const float now = TSP_Now();
	triangleapi_t* tri = gEngfuncs.pTriAPI;
	int newHigh = 0;
	float quad[20];

	for (int i = 0; i <= g_tsPool.highWater; i++)
	{
		TSParticle* p = &g_tsPool.particles[i];
		if (!p->active)
		{
			if (i < g_tsPool.freeCursor)
				g_tsPool.freeCursor = i;
			continue;
		}
		newHigh = i;

		const TSSprite* spr = &g_tsPool.sprites[(p->spriteIdx < 0) ? 0 : p->spriteIdx];

		// Keep while life is left, the animation runs, it is fading, or it is shrinking.
		const bool keepLife = (TSP_EPS <= p->life);
		const bool animDone = (p->spriteIdx < 0) || (p->frameRate <= TSP_EPS) || (spr->frames <= (int)p->frame);
		const bool fadeDone = (p->fadeRate <= TSP_EPS) || (p->bright < TSP_EPS);
		const bool shrinkDone = (-TSP_EPS <= p->accelRate) || (p->size < TSP_EPS);
		if (!keepLife && animDone && fadeDone && shrinkDone)
		{
			TSP_Free(i);
			continue;
		}

		if (p->fadeRate != 0.0f && p->life <= 0.0f)
		{
			float b = p->bright - dt * p->fadeRate * 100.0f; // _DAT_1007a910 = 100
			if (b <= 0.0f)
				b = 0.0f;
			p->bright = b;
			if (1.0f < b)
				p->bright = 1.0f;
		}
		if (p->accelRate != 0.0f)
			p->size = dt * p->accelRate * 100.0f + p->size;

		p->drawn = (p->spriteIdx >= 0);
		TSP_Move(p, quad, dt);
		if (p->think)
			p->think(p);

		if (p->drawn)
		{
			int frame = (int)p->frame;
			if (spr->frames - 1 < frame)
				frame = spr->frames - 1;
			float alpha = 1.0f;
			if (0.0f < p->fadeIn && now < p->spawnTime + p->fadeIn)
				alpha = (now - p->spawnTime) / p->fadeIn;

			model_t* model = (model_t*)gEngfuncs.GetSpritePointer(spr->handle);
			if (model && tri->SpriteTexture(model, frame))
			{
				tri->RenderMode(spr->rendermode);
				tri->CullFace(TRI_NONE);
				tri->Begin(TRI_QUADS);
				tri->Color4f(p->bright, p->bright, p->bright, alpha);
				// FUN_10050000
				for (int v = 0; v < 4; v++)
				{
					tri->TexCoord2f(quad[12 + v], quad[16 + v]);
					tri->Vertex3fv(&quad[v * 3]);
				}
				tri->End();
				tri->RenderMode(kRenderNormal);
			}
		}

		if (p->frameRate != 0.0f && p->life < TSP_EPS)
			p->frame = dt * p->frameRate + p->frame;
		p->life -= dt;
	}
	g_tsPool.highWater = newHigh;

	if (clfx && clfx->value == 1.0f)
		TSP_FxRetune();
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
		if (TSP_Commit(p, NULL, NULL) == -1)
			return;
	}
}

// SPARK-SHOWER — FUN_100492b0. Hardcodes sprites/sparks.spr (rendermode 5). The visible spray.
static void TSP_SparkShower(const float* o, const float* n, int count, float spreadMul, float speed)
{
	// FUN_100492b0's gates: within 512 units of the camera (1024 zoomed under 40, FUN_10057910) and in
	// view. All sparks start at the point (the template's size is still 0 there).
	const Vector at(o[0], o[1], o[2]);
	const float near = (gHUD.m_iFOV < 40) ? 1024.0f : 512.0f;
	if (!((at - TSP_CameraOrigin()).Length() < near) || !TSP_InView(at))
		return;
	int idx = TSP_RegisterSprite("sprites/sparks.spr", 5 /*kRenderTransAdd*/);
	if (idx < 0)
		return;
	// the count: RandomLong(count / 2, count) x cl_fx's count scale, at most 1
	const float cs = (g_tsFx.countScale < 1.0f) ? g_tsFx.countScale : 1.0f;
	int cnt = (int)(gEngfuncs.pfnRandomLong(count / 2, count) * cs);
	for (int i = 0; i < cnt; i++)
	{
		TSParticle* p = TSP_Scratch();
		p->spriteIdx = (short)idx;
		p->active = true;
		p->orient = 0;             // +0x05 = 0 (0x1004933a): view-facing, so the stretch applies
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
		// traced against the world only when it lives past 0.7 s and the count scale is above 0.9
		p->collide = (p->life <= 0.7f || g_tsFx.countScale <= 0.9f) ? 0 : 3;
		p->collideMode = 2;        // +0x120 = 2 (0x10049396)
		p->pos = at;
		p->frame = TSP_RandomFrame(idx);
		if (TSP_Commit(p, NULL, NULL) == -1)
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
		if (TSP_Commit(p, NULL, NULL) == -1)
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
		p->react = 0.3f;           // +0x100
		p->size = size;            // +0x108
		p->collide = bounce ? 3 : 0;
		p->collideMode = 2;        // +0x120 = 2 (0x10046d73)
		// velocity = normal*16 + wide scatter (z half-spread).
		p->vel.x = n[0] * 16.0f + gEngfuncs.pfnRandomLong(-spread, spread);
		p->vel.y = n[1] * 16.0f + gEngfuncs.pfnRandomLong(-spread, spread);
		p->vel.z = n[2] * 16.0f + gEngfuncs.pfnRandomLong(-spread / 2, spread / 2);
		float ang = (float)gEngfuncs.pfnRandomLong(-180, 180);
		p->life = ang * 0.001f + life;  // _DAT_1007a868 = 0.001 (the spin value doubles as life jitter)
		p->avel.z = ang;                // +0x4c — roll rate; orient-1 tumble (angles.z += avel.z*dt*10)
		p->angles.z = 0.0f;             // +0x28 — starts level, accumulates
		p->pos = Vector(o[0] + n[0] * 2, o[1] + n[1] * 2, o[2] + n[2] * 2);
		p->frame = TSP_RandomFrame(idx);
		if (TSP_Commit(p, NULL, NULL) == -1)
			return;
	}
}

// PELLETS — FUN_10046a70. A single shotgun-pellet mark sprite at the impact.
static void TSP_Pellets(const float* o, const float* n, const char* spr, short rendermode,
	int framecnt, float life, float bright, float fade, float accel)
{
	if (!TSP_InView(Vector(o[0], o[1], o[2]))) // FUN_10046a70's view cull
		return;
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
	TSP_Commit(p, NULL, NULL);
}

// cl_fx gun-smoke gate (the gun_smoke_add spawner FUN_10047130 @0x4716c): the smoke spawn is
// skipped entirely when cl_fx is 0 (float != 0.0). The cl_fx levels themselves (g_tsFx) are built
// in TSParticles_Update; the impact spawners' own reads of them (the spawn throttles and count
// scales at 0x10046680/0x10046bd0/0x100492b0/0x10048f60) are not in these spawners.

//-----------------------------------------------------------------------------
// Visual dispatcher  (FUN_10048f60) — per-surface texture-char switch + roll gating.
//-----------------------------------------------------------------------------
void TS_ImpactEffect(const float* origin, const float* normal, int mode, char texChar)
{
	// FUN_10048f60's gates: at most once per cl_fx rate1 of slowed time (kind 7), except modes 1 and 2,
	// which go through regardless (without rearming it); then in view and in sight.
	if (TSP_Due(7))
		TSP_Hold(7, TSP_SlowScaled(g_tsFx.rate1));
	else if (mode != 1 && mode != 2)
		return;
	const Vector at(origin[0], origin[1], origin[2]);
	if (!TSP_Seen(at))
		return;
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
		TSP_Smoke(origin, normal, "sprites/gun_smoke_add.spr", 5, smokeN, 16, 50.0f);
		break;

	case 'P': // computer
	case 'Y': // glass
		TSP_Scintille(origin, normal, "sprites/scintille_metal.spr", 5, 48);
		if (roll > 50)
			TSP_SparkShower(origin, normal, 3, 1.15f, 25.0f);
		TSP_Smoke(origin, normal, "sprites/gun_smoke_add.spr", 5, smokeN, 16, 50.0f);
		break;

	default: // concrete / tile / snow / etc. (RE: count=RandomLong(5/2,5)=2-5, spread=32)
		// debris_concrete3 is INDEXALPHA — the original's rendermode 1 (TransColor) renders as an
		// unmasked square in our TriAPI; kRenderTransAlpha(4) masks it to white chips. (engine deviation)
		TSP_Debris(origin, normal, "sprites/debris_concrete3.spr", 4, 5, 4.0f, 32, 1.0f, 1.0f, 1);
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

	// FUN_1004bfd0 sets the seed, the target and base direction, the target and base strength and
	// the change time; the previous values and the change interval stay as they were.
	g_tsWindState.seed = seed;
	g_tsWindState.dirTarget = fwd;
	g_tsWindState.dirBase = fwd;
	g_tsWindState.strengthBase = strength;
	g_tsWindState.strengthTarget = strength;
	g_tsWindState.changeTime = gEngfuncs.GetClientTime();
	return 1;
}

// BULLET TRAIL — FUN_10046240 (from the client bullet simulation's cl_wavedetail gate, 0x10035a66).
// Collide 4: the corners are fixed at spawn from the view up of that frame, last - up*size,
// last + up*size, pos + up*size, pos - up*size, with the pool's usual texture coordinates. Life 0,
// so it only fades: bright = alpha * 0.75 (_DAT_1007d474), fade rate 0.1 (bright -= dt * 10), no
// motion.
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
	p->collide = 4;
	const Vector c[4] = {last - up * size, last + up * size, pos + up * size, pos - up * size};
	for (int i = 0; i < 4; i++)
	{
		p->quad[i * 3 + 0] = c[i].x;
		p->quad[i * 3 + 1] = c[i].y;
		p->quad[i * 3 + 2] = c[i].z;
	}
	const float s[4] = {0, 1, 1, 0}, t[4] = {0, 0, 1, 1};
	for (int i = 0; i < 4; i++)
	{
		p->quad[12 + i] = s[i];
		p->quad[16 + i] = t[i];
	}
	TSP_Commit(p, NULL, NULL);
}

void TSParticles_Init()
{
	memset(&g_tsWindState, 0, sizeof(g_tsWindState));
	gEngfuncs.pfnHookUserMsg("TSPSys", __MsgFunc_TSPSys);
}

//-----------------------------------------------------------------------------
// The armour hit puff (FUN_100464f0) and the PTakeDam blood (FUN_10048840 with its landing callback
// FUN_10048710). Called from CHudHealth::MsgFunc_PTakeDam (cl_dll/health.cpp).
//-----------------------------------------------------------------------------

// FUN_100464f0: one puff, at most once per cl_fx rate2 (kind 0, slowed by the slow motion), when the
// point is in view and in sight: sprite `spr` in `rendermode`, size `size`, `frameRate` frames a
// second, at point + dir x size / 2, sinking at 2 units a second, growing (0.5), drag 3.3, stretch 7.
void TSP_Puff(const Vector& point, const Vector& dir, const char* spr, short rendermode, int size, int frameRate)
{
	if (!TSP_Due(0))
		return;
	TSP_Hold(0, TSP_SlowScaled(g_tsFx.rate2));
	if (!TSP_Seen(point))
		return;
	const int idx = TSP_RegisterSprite(spr, rendermode);
	if (idx < 0)
		return;
	TSParticle* p = TSP_Scratch();
	p->orient = 0;
	p->active = true;
	p->fadeRate = 0.0f;
	p->bright = 1.0f;
	p->frameRate = (float)frameRate;
	p->spriteIdx = (short)idx;
	p->life = 0.0f;
	p->frame = 0;
	p->size = (float)size;
	const float half = (float)size * 0.5f;
	p->pos = point + dir * half;
	p->collide = 0;
	p->vel = Vector(0, 0, -2.0f);
	p->accelRate = 0.5f;
	p->drag = 3.3f;
	p->stretch = 7.0f;
	TSP_Commit(p, NULL, NULL);
}

// FUN_10048710: a blood drop that hits the world leaves {blood1..8} on a brush (solid BSP or
// MOVETYPE_PUSHSTEP) while r_decals is not 0, one in ten (RandomLong(0, 100) above 90) plays
// player/splat.wav there (volume 1, attenuation 0.99, pitch 25 + 75 x slow +-5), and the drop is gone.
static void TSP_BloodLanded(TSParticle* p, struct pmtrace_s* tr)
{
	char name[32];
	sprintf(name, "{blood%i", gEngfuncs.pfnRandomLong(1, 8));
	physent_t* pe = gEngfuncs.pEventAPI->EV_GetPhysent(tr->ent);
	if (name[0] != '\0' && pe != nullptr && (pe->solid == SOLID_BSP || pe->movetype == MOVETYPE_PUSHSTEP) &&
		gEngfuncs.pfnGetCvarFloat("r_decals") != 0.0f)
	{
		gEngfuncs.pEfxAPI->R_DecalShoot(gEngfuncs.pEfxAPI->Draw_DecalIndex(gEngfuncs.pEfxAPI->Draw_DecalIndexFromName(name)),
			gEngfuncs.pEventAPI->EV_IndexFromTrace(tr), 0, tr->endpos, 0);
	}
	if (gEngfuncs.pfnRandomLong(0, 100) > 90)
	{
		const int pitch = gEngfuncs.pfnRandomLong(-5, 5) + 25 - (int)(g_flClientSlow * -75.0f);
		gEngfuncs.pEventAPI->EV_PlaySound(-1, tr->endpos, CHAN_AUTO, "player/splat.wav", 1.0f, 0.99f, 0, pitch);
	}
	p->active = false;
}

// FUN_10048840: the blood of a PTakeDam, when the point is in view and in sight. The byte is half the
// damage, plus 128 for an armoured hit. The spray runs back along the hit direction; an armoured hit
// jitters it (x and y by +-0.3, z by +-0.1), puffs gun_smoke_add (5, size 4, 20 fps) 12 units out and
// halves the damage. bloodfrag2.spr (client.dll registers it TransColor; reTS draws it TransAlpha,
// which its triangle code needs for the indexalpha sprite), size damage x 0.075 held to 1..5. Drops:
// damage x 0.2 held to 5..15, halved when armoured, times cl_gore's factor (1: 0.5, 2: 1, 3: 2.5,
// 4: 5, 5..10: 10, above 10: the value, up to 33, 0 or below: none). The spray direction then takes
// +0.5..1 upward and is normalised. The first drop flies along the hit at 32; the rest along the spray
// at 8..24 plus -1..1 in x and y, each started where the streak stood before it stepped on (size x 0.3
// along the spray, sinking by its growing droop over the count). Each drop lives 1..2 s, grows by
// 0.075, falls at half gravity and is traced against the world, landing in TSP_BloodLanded. A drop
// inside solid ends the spray.
void TSP_BloodSpray(const float* point, const float* dir, int encByte)
{
	const Vector at(point[0], point[1], point[2]);
	if (!TSP_Seen(at))
		return;
	Vector spray(dir[0] * -1.0f, dir[1] * -1.0f, dir[2] * -1.0f);
	const bool armored = encByte >= 0x80;
	int dmg = (armored ? encByte - 0x80 : encByte) * 2;
	if (armored)
	{
		spray.x += gEngfuncs.pfnRandomFloat(-0.3f, 0.3f);
		spray.y += gEngfuncs.pfnRandomFloat(-0.3f, 0.3f);
		spray.z += gEngfuncs.pfnRandomFloat(-0.1f, 0.1f);
		spray = spray.Normalize();
		TSP_Puff(at, spray * 6.0f, "sprites/gun_smoke_add.spr", 5, 4, 0x14);
		dmg = (int)(dmg * 0.5f);
	}

	const int idx = TSP_RegisterSprite("sprites/bloodfrag2.spr", 4 /*kRenderTransAlpha*/);
	if (idx < 0)
		return;
	TSParticle* p = TSP_Scratch();
	p->spriteIdx = (short)idx;
	p->active = true;
	p->gravity = 0.5f;
	p->orient = 0;
	p->fadeRate = 0.0f;
	p->bright = 1.0f;
	p->frameRate = 0.0f;
	float size = dmg * 0.075f;
	if (size <= 1.0f)
		size = 1.0f;
	else if (size >= 5.0f)
		size = 5.0f;
	p->size = size;
	p->stretch = 2.0f;
	p->collide = 3;
	p->collideMode = 2;

	double c = dmg * 0.2;
	if (c <= 5.0)
		c = 5.0;
	else if (c >= 15.0)
		c = 15.0;
	int count = (int)c;
	if (armored)
		count /= 2;
	static cvar_t* s_cl_gore = nullptr;
	if (s_cl_gore == nullptr)
		s_cl_gore = gEngfuncs.pfnGetCvarPointer("cl_gore");
	const float gore = s_cl_gore != nullptr ? s_cl_gore->value : 2.0f;
	if (gore > 10.0f)
		count = (int)(count * (gore < 33.0f ? gore : 33.0f));
	else
	{
		switch ((int)(gore < 5.0f ? gore : 5.0f))
		{
		case 1: count = (int)(count * 0.5f); break;
		case 2: count = (int)(float)count; break;
		case 3: count = (int)(count * 2.5f); break;
		case 4: count = (int)(count * 5.0f); break;
		case 5: count = (int)(count * 10.0f); break;
		default: count = 0; break;
		}
	}
	spray.z += gEngfuncs.pfnRandomFloat(0.5f, 1.0f);
	spray = spray.Normalize();

	Vector streak = at;
	float droop = 0.0f;
	for (int i = 0; i < count; i++)
	{
		p->frame = TSP_RandomFrame(idx);
		p->accelRate = 0.075f;
		p->life = gEngfuncs.pfnRandomFloat(1.0f, 2.0f);
		if (i < 1)
		{
			p->vel = Vector(dir[0], dir[1], dir[2]) * 32.0f;
			p->pos = at;
		}
		else
		{
			p->vel = spray * gEngfuncs.pfnRandomFloat(8.0f, 24.0f);
			p->vel.x += gEngfuncs.pfnRandomFloat(-1.0f, 1.0f);
			p->vel.y += gEngfuncs.pfnRandomFloat(-1.0f, 1.0f);
			p->pos = streak;
			streak = streak + spray * (p->size * 0.3f);
			droop += p->size * 0.1f;
			streak.z -= droop / (float)count;
		}
		if (gEngfuncs.PM_PointContents((float*)&p->pos, NULL) == CONTENTS_SOLID)
			return;
		if (TSP_Commit(p, NULL, TSP_BloodLanded) == -1)
			return;
	}
}

//-----------------------------------------------------------------------------
// TSFrags (client.dll's handler 0x1004bd50, ts_debris.cpp): the glass shower FUN_10047cb0, the
// explosion FUN_100481b0 and the metal hit of material 2.
//-----------------------------------------------------------------------------

// FUN_10047cb0: glassfrag.spr (additive) in a 9 x 9 grid over the pane's face (the smallest size
// component is the pane's normal and is left out), from origin - size / 2 in steps of size / 9. Each
// shard: size (the two face sizes) / 18 x 0.7, at most 20; view-facing and rolled (orient 1) at
// -90..90 a second; full gravity, bright 0.7 fading at 0.05 once its 2..5 s are over; traced against
// the world; a passing player pushes it by 0.5. Its velocity: for a bullet or blast hit (damage bits
// 0x8000002) -9..9, -9..9, 0..10 plus the direction x velMag / (its distance to the hit + 1) x 10, else
// each component RandomFloat(dir x velMag x 0.5, dir x velMag + 15).
void TSP_GlassShatter(const Vector& origin, const Vector& size, const Vector& dir, const Vector& hit, int dmgBits, float velMag)
{
	Vector face = size;
	if (size.x <= size.y)
	{
		if (size.x <= size.z)
			face.x = 0;
		else
			face.z = 0;
	}
	else if (size.y <= size.z)
		face.y = 0;
	else
		face.z = 0;

	const int idx = TSP_RegisterSprite("sprites/glassfrag.spr", 5);
	if (idx == -1)
		return;
	TSParticle* p = TSP_Scratch();
	p->gravity = 1.0f;
	p->attachment = 0;
	p->imode = 0;
	p->orient = 1;
	p->active = true;
	p->fadeRate = 0.05f;
	p->bright = 0.7f;
	p->spriteIdx = (short)idx;
	p->frameRate = 0;
	p->collide = 3;
	p->collideMode = 2;
	p->react = 0.5f;
	p->stretch = 0;
	float s = (face.z + face.y + face.x) * 0.0555555556f * 0.7f;
	if (20.0f <= s)
		s = 20.0f;
	p->size = s;
	p->avel.x = 0;
	p->avel.y = 0;

	const Vector corner(origin.x - size.x * 0.5f, origin.y - size.y * 0.5f, origin.z - size.z * 0.5f);
	Vector pos = origin;
	if (face.x != 0.0f)
		pos.x = corner.x;
	if (face.y != 0.0f)
		pos.y = corner.y;
	if (face.z != 0.0f)
		pos.z = corner.z;
	const bool scatter = (dmgBits & 0x8000002) != 0;
	for (int i = 0; i < 9; i++)
	{
		if (face.x == 0.0f)
			pos.y = size.y * 0.111111112f * i + corner.y;
		else
			pos.x = size.x * 0.111111112f * i + corner.x;
		for (int j = 0; j < 9; j++)
		{
			if (face.x == 0.0f || face.y == 0.0f)
				pos.z = size.z * 0.111111112f * j + corner.z;
			else
				pos.y = size.y * 0.111111112f * j + corner.y;
			p->frame = TSP_RandomFrame(idx);
			p->pos = pos;
			if (!scatter)
			{
				p->vel.x = gEngfuncs.pfnRandomFloat(dir.x * velMag * 0.5f, dir.x * velMag + 15.0f);
				p->vel.y = gEngfuncs.pfnRandomFloat(dir.y * velMag * 0.5f, dir.y * velMag + 15.0f);
				p->vel.z = gEngfuncs.pfnRandomFloat(dir.z * velMag * 0.5f, dir.z * velMag + 15.0f);
			}
			else
			{
				p->vel.x = gEngfuncs.pfnRandomFloat(-9.0f, 9.0f);
				p->vel.y = gEngfuncs.pfnRandomFloat(-9.0f, 9.0f);
				p->vel.z = (float)gEngfuncs.pfnRandomLong(0, 10);
				const float k = velMag / ((p->pos - hit).Length() + 1.0f);
				p->vel = p->vel + dir * (k * 10.0f);
			}
			p->avel.z = (float)gEngfuncs.pfnRandomLong(-90, 90);
			p->life = gEngfuncs.pfnRandomFloat(2.0f, 5.0f);
			if (TSP_Commit(p, NULL, NULL) == -1)
				return;
		}
	}
}

// FUN_100524c0 with no extra direction: with cl_fx's interaction on, every particle within `radius`
// is pushed away from the point by power x (radius - distance) / radius; a landed one lifts off (a
// falling speed is turned upward and scaled by half that fraction, then 10 x the fraction added up),
// and a rolled one (orient 1) takes -90..90 x the fraction more roll.
static void TSP_BlastPush(const Vector& at, float radius, float power)
{
	if (!g_tsFx.interact)
		return;
	for (int i = 0; i <= g_tsPool.highWater && i < g_tsPool.max; i++)
	{
		TSParticle* p = &g_tsPool.particles[i];
		if (!p->active)
			continue;
		Vector d = p->pos - at;
		const float len = d.Length();
		if (len != 0.0f)
			d = d * (1.0f / len);
		if (!(len <= radius))
			continue;
		const float f = (radius - len) / radius;
		p->vel = p->vel + d * (power * f);
		if (p->settled)
		{
			if (p->vel.z < 0.0f)
				p->vel.z = fabsf(p->vel.z) * f * 0.5f;
			p->settled = false;
			p->vel.z = f * 10.0f + p->vel.z;
		}
		if (p->orient == 1)
			p->avel.z = gEngfuncs.pfnRandomFloat(-90.0f, 90.0f) * f + p->avel.z;
	}
}

// FUN_10048e60: with the high cl_fx level, the explosion's light, at most once per 0.1 s of slowed
// time: radius 192, (240, 221, 172), for 0.06 s of slowed time.
static void TSP_BlastLight(const Vector& at)
{
	static float s_last = 0;
	if (!g_tsFx.high)
		return;
	const float now = TSP_Now();
	const float since = now - s_last;
	if (since < 0.0f)
	{
		s_last = now;
		return;
	}
	if (TSP_SlowScaled(0.1f) <= since)
	{
		s_last = now;
		dlight_t* dl = gEngfuncs.pEfxAPI->CL_AllocDlight(0);
		if (dl != nullptr)
		{
			dl->origin = at;
			dl->radius = 192.0f;
			dl->color.r = 0xf0;
			dl->color.g = 0xdd;
			dl->color.b = 0xac;
			dl->die = TSP_SlowScaled(0.06f) + TSP_Now();
		}
	}
}

// FUN_100481b0, material 1 of TSFrags with `mag` = the size's x (truncated): one of explode.wav,
// explode1..4.wav on channel 6 (volume 0.92..1, attenuation 0.2, pitch 100 + 300 x slow); a blast push
// of 512 units at mag x 10; grenade_asplode.spr puffs (cl_fx scale x 24 of them, size mag / 2,
// 70 out, 25..30 fps) and gun_smoke_add.spr puffs (scale x 48, size mag x 0.7, 54 out, 2..10 fps),
// both raised by half their size, barely falling (0.01), bright 0.3, growing at 0.1, drag 3.3; the
// light 32 units up; one scintille_metal.spr flash of size mag x 2 growing at 2 and fading at 0.05.
void TSP_Explosion(const Vector& origin, int mag)
{
	static const char* const s_boom[5] = {"explo/explode.wav", "explo/explode1.wav", "explo/explode2.wav",
		"explo/explode3.wav", "explo/explode4.wav"};
	const int k = gEngfuncs.pfnRandomLong(0, 4);
	if (k >= 0 && k <= 4)
	{
		const int pitch = 100 - (int)(g_flClientSlow * -300.0f);
		gEngfuncs.pEventAPI->EV_PlaySound(0, (float*)&origin, CHAN_STATIC, s_boom[k],
			gEngfuncs.pfnRandomFloat(0.92f, 1.0f), 0.2f, 0, pitch);
	}
	TSP_BlastPush(origin, 512.0f, (float)(mag * 10));

	int idx = TSP_RegisterSprite("sprites/grenade_asplode.spr", 5);
	if (idx == -1)
		return;
	TSParticle* p = TSP_Scratch();
	p->spriteIdx = (short)idx;
	p->gravity = 0.01f;
	p->attachment = 0;
	p->imode = 0;
	p->orient = 0;
	p->size = (float)(mag * 0.5);
	p->active = true;
	p->fadeRate = 0;
	p->bright = 0.3f;
	p->life = 0;
	p->frame = 0;
	p->pos = origin;
	p->pos.z = p->size * 0.5f + p->pos.z;
	p->collide = 0;
	p->accelRate = 0.1f;
	p->drag = 3.3f;
	p->stretch = 0;
	int count = (int)(g_tsFx.scale * 24.0f);
	for (int i = 0; i < count; i++)
	{
		p->vel.x = gEngfuncs.pfnRandomFloat(-70.0f, 70.0f);
		p->vel.y = gEngfuncs.pfnRandomFloat(-70.0f, 70.0f);
		p->vel.z = gEngfuncs.pfnRandomFloat(0.0f, 70.0f);
		p->frameRate = (float)gEngfuncs.pfnRandomLong(0x19, 0x1e);
		if (TSP_Commit(p, NULL, NULL) == -1)
			return;
	}

	idx = TSP_RegisterSprite("sprites/gun_smoke_add.spr", 5);
	if (idx == -1)
		return;
	p->gravity = 0.01f;
	p->attachment = 0;
	p->imode = 0;
	p->orient = 0;
	p->active = true;
	p->size = (float)(mag * 0.7);
	p->fadeRate = 0;
	p->bright = 0.3f;
	p->spriteIdx = (short)idx;
	p->life = 0;
	p->frame = 0;
	p->pos = origin;
	p->pos.z = p->size * 0.5f + p->pos.z;
	p->collide = 0;
	p->accelRate = 0.1f;
	p->drag = 3.3f;
	p->stretch = 0;
	count = (int)(g_tsFx.scale * 48.0f);
	for (int i = 0; i < count; i++)
	{
		p->vel.x = gEngfuncs.pfnRandomFloat(-54.0f, 54.0f);
		p->vel.y = gEngfuncs.pfnRandomFloat(-54.0f, 54.0f);
		p->vel.z = gEngfuncs.pfnRandomFloat(0.0f, 54.0f);
		p->frameRate = (float)gEngfuncs.pfnRandomLong(2, 10);
		if (TSP_Commit(p, NULL, NULL) == -1)
			return;
	}

	TSP_BlastLight(origin + Vector(0, 0, 32));

	idx = TSP_RegisterSprite("sprites/scintille_metal.spr", 5);
	if (idx == -1)
		return;
	p->gravity = 0;
	p->attachment = 0;
	p->imode = 0;
	p->orient = 0;
	p->active = true;
	p->fadeRate = 0.05f;
	p->vel = Vector(0, 0, 0);
	p->spriteIdx = (short)idx;
	p->bright = 1.0f;
	p->frameRate = 0;
	p->life = 0;
	p->size = (float)(mag * 2);
	p->pos = origin;
	p->pos.z = p->size * 0.5f + p->pos.z;
	p->collide = 0;
	p->frame = TSP_RandomFrame(idx);
	p->accelRate = 2.0f;
	TSP_Commit(p, NULL, NULL);
}

// TSFrags material 2 (0x1004be6b): a scintille_metal.spr glint (count 9) and a spark shower (7, 1.15,
// 25) at the point. client.dll passes a normal it never fills (uninitialised stack); reTS passes 0.
void TSP_MetalHit(const Vector& origin)
{
	const float n[3] = {0, 0, 0};
	TSP_Scintille(&origin.x, n, "sprites/scintille_metal.spr", 5, 9);
	TSP_SparkShower(&origin.x, n, 7, 1.15f, 25.0f);
}
