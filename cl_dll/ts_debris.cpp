/***
*  ts_debris.cpp — The Specialists glass-shatter sprite shower (client side). See ts_debris.h.
*
*  Reconstructs client.dll FUN_10047cb0: an 81-fragment (9x9) grid of sprites/glassfrag.spr laid
*  across the broken pane's FACE (the two largest dims; the smallest dim = the pane normal is
*  dropped), each fragment given a velocity in the shatter direction with per-component spread,
*  gravity and a 2-5s fade life. Constants RE'd from client.dll (file = VA-0x10000000): grid
*  centre 0.5, step 1/9, velocity spread "+15", gravity 0.05, life RANDOM(2,5).
*
*  Payload ("TSFrags" = gmsgTSCreateDebris, sent by CBreakable::Die for matGlass): origin[3], size[3],
*  material byte(0=glass), shatterDir[3], refPoint[3], bitsDamageType(long), velMag(coord).
***/
#include "hud.h"
#include "cl_util.h"
#include "parsemsg.h"
#include "ts_debris.h"

#include "event_api.h"
#include "r_efx.h"
#include "com_model.h"

DECLARE_MESSAGE(m_TSDebris, TSFrags)

bool CHudTSDebris::Init()
{
	HOOK_MESSAGE(TSFrags); // the original wire name is "TSFrags" (star_LinkUserMessages.md #62)
	gHUD.AddHudElem(this);
	m_iFlags |= HUD_ACTIVE; // no Draw of its own; the temp-sprites it spawns render via the engine
	return true;
}

bool CHudTSDebris::VidInit()
{
	return true;
}

// cont.96 v4: xash implements neither FTENT_SLOWGRAVITY nor per-tempent gravity, so the shard
// motion is fully self-integrated (FTENT_CLIENTCUSTOM = the engine skips its own motion): fly on
// the scatter/cluster velocity, fall at half-g (VERIFY: the original integrator time-scales its
// 0.05 coefficient — slow-mo goes floaty; match exactly in a future RE of FUN_10050b60).
static void TSGlassShardThink(struct tempent_s* ent, float frametime, float)
{
	ent->entity.baseline.origin[2] -= 400.0f * frametime; // half sv_gravity
	ent->entity.origin = ent->entity.origin + ent->entity.baseline.origin * frametime;
	ent->entity.angles[2] += ent->entity.baseline.angles[2] * frametime; // keep the tumble
}

// Reconstruct FUN_10047cb0: spray the 9x9 glass-fragment grid across the pane face.
static void SpawnGlassShatter(const Vector& origin, const Vector& size, const Vector& dir,
	const Vector& refPoint, int spriteModel, float velMag, int dmgType)
{
	if (spriteModel <= 0)
		return;


	// Drop the smallest dimension (the pane normal): lay the grid on the two largest (the face).
	// Mirrors the original's "zero the smallest of param_4/5/6" branch.
	int normal;
	if (size.x <= size.y)
		normal = (size.x <= size.z) ? 0 : 2;
	else
		normal = (size.y <= size.z) ? 1 : 2;
	int ax0 = (normal == 0) ? 1 : 0;
	int ax1 = (normal == 2) ? 1 : 2;

	const float* o = &origin.x;
	const float* s = &size.x;
	const float* d = &dir.x;

	// fragment scale ∝ pane size but SMALL (little shards). The sprite is SPR_ADDITIVE, so it draws
	// with kRenderTransAdd below (black keys out — kRenderTransAlpha showed the black background).
	float scale = (s[ax0] + s[ax1]) * (1.0f / 180.0f);
	if (scale < 0.08f) scale = 0.08f;
	if (scale > 0.3f) scale = 0.3f;

	for (int i = 0; i < 9; i++)
	{
		for (int j = 0; j < 9; j++)
		{
			Vector pos = origin;
			// centre the grid on the face: corner = origin - size*0.5, step = size/9
			(&pos.x)[ax0] = o[ax0] - s[ax0] * 0.5f + s[ax0] * (1.0f / 9.0f) * (float)i;
			(&pos.x)[ax1] = o[ax1] - s[ax1] * 0.5f + s[ax1] * (1.0f / 9.0f) * (float)j;

			// @0x10047cb0 — TWO velocity formulas, selected by (dmgType & 0x8000002):
			Vector vel;
			if (dmgType & 0x8000002)
			{
				// SCATTER branch (bullets: DMG_BULLET 0x2). Base = a small CENTERED random (some shards go
				// BACKWARDS / straight down) + a directional bias weighted by 1/(distance from the fragment
				// to the impact point): fragments near the hit fly forward hard, far ones mostly scatter.
				vel.x = gEngfuncs.pfnRandomFloat(-9.0f, 9.0f);
				vel.y = gEngfuncs.pfnRandomFloat(-9.0f, 9.0f);
				vel.z = (float)gEngfuncs.pfnRandomLong(0, 10);
				const float dist = (pos - refPoint).Length();
				const float f = velMag / (dist + 1.0f) * 10.0f; // ε=1, ×10 (_DAT_10079e88)
				vel.x += d[0] * f; vel.y += d[1] * f; vel.z += d[2] * f;
			}
			else
			{
				// CLUSTER branch: per-component RANDOM_FLOAT(dir[c]*velMag*0.5, dir[c]*velMag+15) — tight
				// forward burst. (C1=0.5 _DAT_1007a870, C2=15 _DAT_1007a890.)
				for (int c = 0; c < 3; c++)
					(&vel.x)[c] = gEngfuncs.pfnRandomFloat(d[c] * velMag * 0.5f, d[c] * velMag + 15.0f);
			}

			// cont.96 v3: the original's custom shard integrator (@0x47cb0 tempent+0x38 = 0.05)
			// scales gravity by the TIME FACTOR (user: slow-mo = moon-grav, normal = real fall).
			// FTENT_GRAVITY (full g) buried the spread; SLOWGRAVITY (half g) lets the scatter
			// velocity carry while the shards still land. // VERIFY the exact curve vs the
			// original integrator (FUN_10050b60 side) + slow-mo scaling — future RE.
			TEMPENTITY* t = gEngfuncs.pEfxAPI->R_TempSprite(pos, vel, scale, spriteModel,
				kRenderTransAdd, kRenderFxNone, 1.0f, // glassfrag.spr is SPR_ADDITIVE
				gEngfuncs.pfnRandomFloat(2.0f, 5.0f),
				FTENT_FADEOUT | FTENT_CLIENTCUSTOM | FTENT_COLLIDEWORLD | FTENT_ROTATE);
			if (t)
			{
				// pick one of the 9 shard SHAPES (frames) at random — the original varies the frame
				// per fragment (mine all showed frame 0 = identical shapes). Plus a random initial
				// roll + a HIGH spin rate (FTENT_ROTATE applies baseline.angles as angular velocity;
				// the shards visibly tumble as they fall).
				t->entity.curstate.frame = (float)gEngfuncs.pfnRandomLong(0, 8);
				t->entity.angles[2] = gEngfuncs.pfnRandomFloat(0.0f, 360.0f);
				t->entity.baseline.angles[2] = gEngfuncs.pfnRandomFloat(-500.0f, 500.0f);
				t->callback = TSGlassShardThink; // v4: full self-integration (see above)
			}
		}
	}
}

bool CHudTSDebris::MsgFunc_TSFrags(const char* pszName, int iSize, void* pbuf)
{
	BEGIN_READ(pbuf, iSize);

	Vector origin, size, point, dir;
	origin.x = READ_COORD(); origin.y = READ_COORD(); origin.z = READ_COORD();
	size.x = READ_COORD();   size.y = READ_COORD();   size.z = READ_COORD();
	int material = READ_BYTE();

	if (material == 0) // glass → the sprite shower
	{
		// @0x752e0 wire order: triple 1 = impact POINT (field_0xf8), triple 2 = shatter DIRECTION
		// (field_0x104). The shards fly along the DIRECTION (triple 2). We used to use triple 1 for the
		// velocity, which held the server's CENTRE against the original server → static/non-directional.
		point.x = READ_COORD(); point.y = READ_COORD(); point.z = READ_COORD(); // triple 1 = impact point (refPoint)
		dir.x = READ_COORD();   dir.y = READ_COORD();   dir.z = READ_COORD();   // triple 2 = shatter direction
		// @0x752e0: this LONG is the killing hit's bitsDamageType (NOT a sprite index) — the client resolves
		// the glassfrag sprite by NAME and uses this only to select the velocity formula. Read as LONG (4
		// bytes); a short read here misaligned velMag to 0.
		int dmgType = READ_LONG();
		float velMag = READ_COORD();
		const int spriteModel = gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/glassfrag.spr");
		SpawnGlassShatter(origin, size, dir, point, spriteModel, velMag, dmgType);
	}
	else if (material == 1) // grenade explosion (TSExplosion): size.x = the blast magnitude
	{
		// the CLIENT owns the boom: the TS explosion sound (explo/explode1-4.wav — VERIFY
		// whether explode.wav joins the pool) + the fireball, NOSOUND so the engine's stock
		// explode never plays (the user-reported stock-HL-boom bug). Full debris VERIFY (#8).
		char snd[24];
		sprintf(snd, "explo/explode%d.wav", gEngfuncs.pfnRandomLong(1, 4));
		// the explode wavs are RECORDED SLOW (like the gunshots): pitch UP for normal play,
		// deepening toward natural in slow-mo — the same 100 + 300*slow formula as ev_ts.cpp.
		extern float g_flClientSlow;
		int pitch = 100 + (int)(300.0f * g_flClientSlow);
		if (pitch > 255) pitch = 255;
		if (pitch < 1) pitch = 1;
		gEngfuncs.pEventAPI->EV_PlaySound(0, origin, CHAN_STATIC, snd, VOL_NORM, 0.8f, 0, pitch);
		const int fireball = gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/zerogxplode.spr");
		if (fireball)
			gEngfuncs.pEfxAPI->R_Explosion(origin, fireball, size.x * (1.0f / 44.0f), 15.0f,
				TE_EXPLFLAG_NOSOUND); // scale ~ magnitude/44 -> the old TE's 2.5 at mag 110
	}

	return true;
}
