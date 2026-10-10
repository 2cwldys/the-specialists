/***
*  ts_bullets.cpp — the client bullet simulation, client.dll's manager object at 0x100eb678.
*
*  256 rounds of 0x5c bytes, then a count (+0x5c00). Recovered:
*    queue          FUN_10035c20 (from the fire core FUN_100450c0's realbullet branch, 0x10045a8b)
*    per frame      FUN_10034e60 -> FUN_10035b50 (last in HUD_DrawTransparentTriangles, 0x10056e59)
*    step           FUN_10035d40 (trace, impact, pierce), free FUN_10035d10
*    draw           FUN_10035010 (slow motion only), view cull FUN_100570b0, trail FUN_10046240
*    reset          FUN_10034eb0 -> FUN_10034f60 (the FX root's first frame, 0x10056de3)
*
*  The object also holds 32 cl_entity_t (+0x5c08) for drawing rounds as models: FUN_10035010 fills
*  one instead of drawing the sprite when the byte at +0x1d34d is set, and FUN_10034fa0 (from
*  HUD_CreateEntities through FUN_10034e80) hands them to the engine. Nothing in client.dll ever
*  sets that byte (FUN_10034f90 only clears it) or the model indices at +0x1d30c..+0x1d334, so
*  rounds are always sprites and that half is not reconstructed.
***/
#include "hud.h"
#include "cl_util.h"
#include "const.h"
#include "entity_state.h"
#include "cl_entity.h"
#include "triangleapi.h"
#include "com_model.h"
#include "event_api.h"
#include "pm_defs.h"
#include "ts_weapon_info.h" // get_ammo_info, get_range_multipler (shared with the server)
#include "ts_shared.h"      // GetBulletVelocityMult, FixedSlowMul
#include "ts_particles.h"   // the pool's sprite table, the bullet trail
#include "ts_bullets.h"

#include <string.h>
#include <math.h>

extern Vector v_origin, v_angles; // view.cpp: the view origin and angles of this frame
extern float v_frametime;         // view.cpp: pparams->frametime (client.dll 0x10191478)
extern float g_flClientSlow;      // ts_powerup_hud.cpp: the slow-motion scale (client.dll 0x100963e8)
extern bool g_bBulletTime;        // ts_powerup_hud.cpp: TSBTime (client.dll 0x1018d178)
extern int g_iTSRealBullet;       // ts_status.cpp: SrvSett id 0 (client.dll 0x1018d16c)

struct TSClientBullet
{
	int owner;       // +0x00 the shooter's entity index, 0 = free slot
	Vector pos;      // +0x04 where the simulation has the round
	Vector start;    // +0x10 where this leg began (range falloff)
	Vector dir;      // +0x1c
	Vector drawPos;  // +0x28 where it is drawn (moved per frame, only while drawn)
	Vector trailPos; // +0x34 the end of the last trail piece
	float damage;    // +0x40
	int caliber;     // +0x44 get_ammo_info index
	int unused48;    // +0x48 never written
	float nextTrail; // +0x4c
	float nextStep;  // +0x50
	int mode;        // +0x54 the shot mode (wi+0x20); 2 = a pierce leg
	int unused58;    // +0x58 cleared on queue, never read
};

#define TSB_MAX_BULLETS 256

static TSClientBullet s_bullets[TSB_MAX_BULLETS];
static int s_count; // +0x5c00: rounds seen by the last update plus those queued since

bool TSBullets_RealBullet()
{
	return g_iTSRealBullet != 0 || g_flClientSlow < 0.99f;
}

void TSBullets_Reset()
{
	memset(s_bullets, 0, sizeof(s_bullets));
	s_count = 0;
}

// FUN_10035c20: the first free slot takes the round; with none free it is dropped.
void TSBullets_Fire(int owner, const float* src, const float* dir, float damage, int caliber, int mode)
{
	for (int i = 0; i < TSB_MAX_BULLETS; i++)
	{
		TSClientBullet* b = &s_bullets[i];
		if (b->owner != 0)
			continue;
		b->owner = owner;
		b->dir = Vector(dir[0], dir[1], dir[2]);
		b->damage = damage;
		b->caliber = caliber;
		b->start = Vector(src[0], src[1], src[2]);
		b->pos = b->start;
		b->drawPos = b->start;
		b->trailPos = b->start;
		b->nextTrail = 0.0f;
		b->nextStep = 0.0f;
		b->mode = mode;
		b->unused58 = 0;
		s_count++;
		return;
	}
}

// FUN_10035d40: move the round one step. The step is the round's speed (caliber speed unit x 40,
// times the slow-motion bullet multiplier) over the update interval (the frame time when that is
// under 0.001). A hit lays the impact; a round with more than 5 damage left after the range
// falloff and the caliber's factor goes on through up to 48 units of anything that is not solid
// there, as the server's bullets do, else it is freed.
static void TSB_Step(TSClientBullet* b, pmtrace_t* tr)
{
	const s_ammo_info* ai = get_ammo_info(b->caliber);
	const float velMult = GetBulletVelocityMult(g_flClientSlow, g_bBulletTime ? 1 : 0);
	float dt = b->nextStep - gEngfuncs.GetClientTime();
	if (dt < 0.001f)
		dt = v_frametime;
	const Vector vel = b->dir * ((float)ai->iSpeedUnit * 40.0f) * dt * velMult;

	event_api_t* ev = gEngfuncs.pEventAPI;
	gEngfuncs.PM_PointContents(b->pos, nullptr); // client.dll makes this call and discards the result
	ev->EV_SetUpPlayerPrediction(0, 1);
	ev->EV_PushPMStates();
	ev->EV_SetSolidPlayers(b->owner - 1);
	ev->EV_SetTraceHull(2);
	Vector end = b->pos + vel;
	ev->EV_PlayerTrace(b->pos, end, PM_STUDIO_BOX, -1, tr);

	bool done = false;
	if (tr->fraction == 1.0f)
	{
		b->pos = b->pos + vel;
	}
	else
	{
		done = true;
		b->pos = tr->endpos;
		const float dist = (b->start - b->pos).Length();
		// a pierce leg falls off over 300 units, any other shot over its caliber's range unit
		const float idx = (b->mode == 2) ? dist * (1.0f / 300.0f) * 10.0f
		                                 : dist / ((float)ai->iRangeUnit * 10.0f) * 10.0f;
		float damage = get_range_multipler((int)idx) * b->damage;
		Vector hitEnd = b->pos + vel;
		EV_TSBulletHit(b->owner, b->pos, hitEnd, *tr, b->mode);
		damage = damage * ai->flFactor2;

		if (b->mode != 2 && b->mode != 1 && damage > 5.0f)
		{
			const float depth = (damage < 48.0f) ? damage : 48.0f;
			Vector exitPos = Vector(tr->endpos) + b->dir * depth;
			if (gEngfuncs.PM_PointContents(exitPos, nullptr) != CONTENTS_SOLID)
			{
				Vector face = tr->endpos;
				b->pos = exitPos;
				b->start = exitPos;
				b->damage = damage;
				b->mode = 2;
				// the far face, traced back from where the round comes out
				ev->EV_SetTraceHull(2);
				ev->EV_PlayerTrace(exitPos, face, PM_NORMAL, -1, tr);
				if (tr->fraction != 1.0f)
					EV_TSBulletHit(b->owner, exitPos, face, *tr, 2);
				done = false;
			}
		}
	}
	ev->EV_PopPMStates();

	if (done)
		memset(b, 0, sizeof(*b));
}

// FUN_100570b0: on screen enough to draw. Within 300 units always; further out only in front of
// the view and inside a cone that widens a little with distance.
static bool TSB_InView(const Vector& p, const Vector& fwd, const Vector& right, const Vector& up)
{
	Vector d = p - v_origin; // FUN_10057270: the view origin (the camera in third person)
	const float dist = d.Length();
	if (dist <= 300.0f)
		return true;
	d = d * (1.0f / dist);
	if (DotProduct(fwd, d) < 0.0f)
		return false;
	const float side = fabs(DotProduct(right, d));
	const float height = fabs(DotProduct(up, d));
	const float spread = (dist * (1.0f / 8192.0f) * 0.2f + 1.0f) * (side + height * 0.5f);
	return spread <= 0.9f;
}

static void TSB_Quad(triangleapi_t* tri, Vector a, Vector b, Vector c, Vector d)
{
	tri->TexCoord2f(0, 0);
	tri->Vertex3fv(a);
	tri->TexCoord2f(1, 0);
	tri->Vertex3fv(b);
	tri->TexCoord2f(1, 1);
	tri->Vertex3fv(c);
	tri->TexCoord2f(0, 1);
	tri->Vertex3fv(d);
}

// FUN_10035010: draw one round (called only in slow motion) and move its drawn position on.
// The sprite is sprites/shotgun_pellets.spr, additive, half-size 1 - slow^2 (at least 0.15):
// two crossed streaks 5..25 units either side of it along its direction (slow x 37.5; none for
// caliber 3) and a camera-facing quad. Within about 3 units of the view (squared distance under
// 10) it fades. cl_wavedetail above 0 lays a trail piece from the last one every 0.01 s of slowed
// time (0x10035a66).
static void TSB_Draw(TSClientBullet* b, int sprite, const Vector& fwd, const Vector& right, const Vector& up)
{
	if (!TSB_InView(b->drawPos, fwd, right, up))
		return;

	float size = 1.0f - g_flClientSlow * g_flClientSlow;
	if (size < 0.15f)
		size = 0.15f;
	// client.dll measures this from its first-person view origin (0x10190b28); v_origin is that
	// point in first person
	const Vector toView = v_origin - b->drawPos;
	float alpha = DotProduct(toView, toView);
	alpha = (alpha < 10.0f) ? alpha * 0.1f : 1.0f;

	if (sprite >= 0)
	{
		triangleapi_t* tri = gEngfuncs.pTriAPI;
		model_t* model = (model_t*)gEngfuncs.GetSpritePointer(TSParticles_SpriteHandle(sprite));
		if (model != nullptr && 0 != tri->SpriteTexture(model, 0))
		{
			tri->RenderMode(kRenderTransAdd);
			tri->CullFace(TRI_NONE);
			tri->Begin(TRI_QUADS);
			tri->Color4f(1.0f, 1.0f, 1.0f, alpha);
			if (b->caliber != 3)
			{
				float len = g_flClientSlow * 37.5f;
				if (len < 5.0f)
					len = 5.0f;
				else if (len > 25.0f)
					len = 25.0f;
				const Vector head = b->drawPos + b->dir * len;
				const Vector tail = b->drawPos - b->dir * len;
				const Vector u = up * size;
				const Vector r = right * size;
				TSB_Quad(tri, head - u, head + u, tail + u, tail - u);
				TSB_Quad(tri, head - r, head + r, tail + r, tail - r);
			}
			// FUN_10050080 with no velocity: the camera-facing quad
			const Vector u = up * size;
			const Vector r = right * size;
			TSB_Quad(tri, b->drawPos + u - r, b->drawPos + u + r, b->drawPos - u + r, b->drawPos - u - r);
			tri->End();
			tri->RenderMode(kRenderNormal);
		}
	}

	const float velMult = GetBulletVelocityMult(g_flClientSlow, g_bBulletTime ? 1 : 0);
	const float speed = (float)get_ammo_info(b->caliber)->iSpeedUnit * 40.0f * velMult * v_frametime;
	b->drawPos = b->drawPos + b->dir * speed;

	static cvar_t* s_waveDetail = nullptr;
	if (s_waveDetail == nullptr)
		s_waveDetail = gEngfuncs.pfnGetCvarPointer("cl_wavedetail");
	if (s_waveDetail != nullptr && s_waveDetail->value > 0.0f && gEngfuncs.GetClientTime() > b->nextTrail)
	{
		const float wait = FixedSlowMul(g_flClientSlow) * 0.01f; // FUN_10056fa0(0.01)
		b->nextTrail = gEngfuncs.GetClientTime() + wait;
		TSP_BulletTrail(b->drawPos, b->trailPos, size, alpha);
		b->trailPos = b->drawPos;
	}
}

// FUN_10034e60 / FUN_10035b50. A round is stepped when its time comes: every 0.1 s with realbullet
// 0, every 0.05 s with realbullet 1, every frame otherwise. In slow motion each is then drawn,
// including one freed by its step this frame (client.dll draws the cleared slot too: at most one
// frame of a tiny pellet sprite at the world origin, kept as the original does it).
void TSBullets_Update()
{
	if (s_count == 0)
		return;

	const int sprite = TSParticles_Sprite("sprites/shotgun_pellets.spr", kRenderTransAdd);
	Vector fwd, right, up;
	AngleVectors(v_angles, fwd, right, up); // the FX root's AngleVectors (0x10056f60)

	s_count = 0;
	pmtrace_t tr;
	for (int i = 0; i < TSB_MAX_BULLETS; i++)
	{
		TSClientBullet* b = &s_bullets[i];
		if (b->owner == 0)
			continue;
		if (b->nextStep < gEngfuncs.GetClientTime())
		{
			const float now = gEngfuncs.GetClientTime();
			b->nextStep = now;
			if (g_iTSRealBullet >= 0 && g_iTSRealBullet < 2)
				b->nextStep = 0.1f / (float)(g_iTSRealBullet + 1) + now;
			TSB_Step(b, &tr);
		}
		if (g_flClientSlow < 0.99f)
			TSB_Draw(b, sprite, fwd, right, up);
		s_count++;
	}
}
