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
*  material byte(0=glass), shatterDir[3], refPoint[3], spriteModelIndex(short), velMag(coord).
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

// Reconstruct FUN_10047cb0: spray the 9x9 glass-fragment grid across the pane face.
static void SpawnGlassShatter(const Vector& origin, const Vector& size, const Vector& dir,
	int spriteModel, float velMag)
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

			// velocity: a forward bias along the shatter dir + a LARGE random scatter on every axis
			// so the shards burst outward and land spread out (not clumped along one direction).
			Vector vel;
			for (int c = 0; c < 3; c++)
				(&vel.x)[c] = d[c] * velMag * 0.4f +
					gEngfuncs.pfnRandomFloat(-velMag * 0.6f, velMag * 0.6f);

			TEMPENTITY* t = gEngfuncs.pEfxAPI->R_TempSprite(pos, vel, scale, spriteModel,
				kRenderTransAdd, kRenderFxNone, 1.0f, // glassfrag.spr is SPR_ADDITIVE
				gEngfuncs.pfnRandomFloat(2.0f, 5.0f),
				FTENT_FADEOUT | FTENT_GRAVITY | FTENT_COLLIDEWORLD | FTENT_ROTATE);
			if (t)
			{
				// pick one of the 9 shard SHAPES (frames) at random — the original varies the frame
				// per fragment (mine all showed frame 0 = identical shapes). Plus a random initial
				// roll + a HIGH spin rate (FTENT_ROTATE applies baseline.angles as angular velocity;
				// the shards visibly tumble as they fall).
				t->entity.curstate.frame = (float)gEngfuncs.pfnRandomLong(0, 8);
				t->entity.angles[2] = gEngfuncs.pfnRandomFloat(0.0f, 360.0f);
				t->entity.baseline.angles[2] = gEngfuncs.pfnRandomFloat(-500.0f, 500.0f);
			}
		}
	}
}

bool CHudTSDebris::MsgFunc_TSFrags(const char* pszName, int iSize, void* pbuf)
{
	BEGIN_READ(pbuf, iSize);

	Vector origin, size, dir, ref;
	origin.x = READ_COORD(); origin.y = READ_COORD(); origin.z = READ_COORD();
	size.x = READ_COORD();   size.y = READ_COORD();   size.z = READ_COORD();
	int material = READ_BYTE();

	if (material == 0) // glass → the sprite shower
	{
		dir.x = READ_COORD(); dir.y = READ_COORD(); dir.z = READ_COORD();
		ref.x = READ_COORD(); ref.y = READ_COORD(); ref.z = READ_COORD();
		int spriteModel = READ_SHORT();
		float velMag = READ_COORD();
		SpawnGlassShatter(origin, size, dir, spriteModel, velMag);
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
