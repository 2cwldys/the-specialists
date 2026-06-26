/***
*  ts_attachments.cpp — client render of the VISIBLE TS gun attachments: the lasersight and the
*  flashlight. Drawn every frame from each player's gun muzzle, gated by the per-player attach mask
*  the server broadcasts (the `ActItems` user message). See ts_attachments.h for the RE map.
*
*  Faithful-BEHAVIOUR reconstruction (the standard used for the particle/impact subsystem):
*   - LASER (RE FUN_1003d600): trace from the muzzle along the gun aim; draw a thin laserbeam.spr
*     quad muzzle->hit + a laserdot.spr dot at the hit, in the player's laser colour
*     (FUN_100582f0: 1=green, 2=blue, else=red). The local player's colour comes from cl_lasercolor.
*   - FLASHLIGHT (RE FUN_1003b750): trace forward; a white dynamic light at the surface it lands on
*     + a faint flash beam from the muzzle.
*  The gun direction is the model's two muzzle attachments (attachment[1]-attachment[0]) — the same
*  the original reads (FUN_10056be0 idx,0/1) — with a view/origin fallback when a model hasn't set them.
***/
#include "hud.h"
#include "cl_util.h"
#include "const.h"
#include "entity_state.h"
#include "cl_entity.h"
#include "event_api.h"
#include "pm_defs.h"
#include "triangleapi.h"
#include "com_model.h"
#include "parsemsg.h" // BEGIN_READ / READ_BYTE
#include "ts_attachments.h"

#include <string.h>
#include <math.h>

#define TS_ATT_LASER 2 // attach bit 1 (value 2)
#define TS_ATT_FLASH 4 // attach bit 2 (value 4)

#define TS_MAXPLAYERS 33 // 1..32 players (index 0 unused) — matches the original's 0..0x20 loop

// Per-player attach mask, set by the ActItems message (BYTE playerIndex, BYTE attachMask). Mirrors
// the original's DAT_1018d128 array (one byte per player; the original packs a colour byte too which
// it networks separately — we colour by cl_lasercolor, see GetLaserColor).
static unsigned char g_playerAttach[TS_MAXPLAYERS];

static int s_laserBeamSpr = 0; // SPR_Load handles (lazy)
static int s_laserDotSpr = 0;

// ActItems — the server broadcast of a player's active-weapon attach mask. Client handler FUN_1004fa70:
// READ_BYTE playerIndex, READ_BYTE attachMask -> per-player array.
int MsgFunc_ActItems(const char* pszName, int iSize, void* pbuf)
{
	BEGIN_READ(pbuf, iSize);
	const int idx = READ_BYTE();
	const int attach = READ_BYTE();
	if (idx >= 0 && idx < TS_MAXPLAYERS)
		g_playerAttach[idx] = (unsigned char)attach;
	return 1;
}

void TS_Attachments_Init()
{
	memset(g_playerAttach, 0, sizeof(g_playerAttach));
	gEngfuncs.pfnHookUserMsg("ActItems", MsgFunc_ActItems);
}

// Map change (HUD VidInit): the cached SPR_Load handles index the per-map sprite list, so they go
// STALE after a load — drop them so they re-load fresh on next draw. (The per-player attach state is
// re-sent by the server's ActItems on respawn; clear it too so it doesn't linger from the old map.)
void TS_Attachments_VidInit()
{
	s_laserBeamSpr = 0;
	s_laserDotSpr = 0;
	memset(g_playerAttach, 0, sizeof(g_playerAttach));
}

// Laser colour (RE FUN_100582f0): 1 -> green, 2 -> blue, everything else -> red.
static void GetLaserColor(int index, float& r, float& g, float& b)
{
	if (index == 1) { r = 0; g = 205 / 255.0f; b = 0; }
	else if (index == 2) { r = 0; g = 120 / 255.0f; b = 255 / 255.0f; }
	else { r = 1.0f; g = 25 / 255.0f; b = 25 / 255.0f; }
}

// Recover a player's gun muzzle (world) + forward aim. The faithful source is the two muzzle
// attachments of the active model (dir = attachment[1] - attachment[0]); for the LOCAL player that's
// the viewmodel, for others the player (p_) model. Falls back to the entity origin + view/body angles
// when a model hasn't populated its attachments yet (returns false if nothing usable).
static bool GetMuzzleAim(int idx, Vector& muzzle, Vector& forward)
{
	cl_entity_t* local = gEngfuncs.GetLocalPlayer();
	const bool isLocal = local && local->index == idx;

	cl_entity_t* model = nullptr;
	if (isLocal)
		model = gEngfuncs.GetViewModel();
	if (!model)
		model = gEngfuncs.GetEntityByIndex(idx);
	if (!model)
		return false;

	const float* a0 = model->attachment[0];
	const float* a1 = model->attachment[1];
	const bool haveAttach = (a0[0] || a0[1] || a0[2]) && (a1[0] || a1[1] || a1[2]) &&
		(a0[0] != a1[0] || a0[1] != a1[1] || a0[2] != a1[2]);

	if (haveAttach)
	{
		muzzle = Vector(a0[0], a0[1], a0[2]);
		forward = Vector(a1[0] - a0[0], a1[1] - a0[1], a1[2] - a0[2]);
		const float len = forward.Length();
		if (len < 0.01f)
			return false;
		forward = forward * (1.0f / len);
		return true;
	}

	// Fallback: aim from the (view for local / body for others) angles, muzzle at the eye.
	Vector angles;
	if (isLocal)
		gEngfuncs.GetViewAngles((float*)angles);
	else
	{
		cl_entity_t* p = gEngfuncs.GetEntityByIndex(idx);
		if (!p)
			return false;
		angles = p->angles;
		angles[0] = -angles[0]; // player model pitch is stored negated
	}
	Vector right, up;
	AngleVectors(angles, forward, right, up);
	cl_entity_t* p = isLocal ? local : gEngfuncs.GetEntityByIndex(idx);
	muzzle = p->origin;
	muzzle[2] += 20.0f; // approx gun height above the origin
	return true;
}

// Trace from muzzle along forward to the first solid; returns the hit point. Uses PM_TraceLine
// (render-SAFE) — NOT the EV_PushPMStates/EV_PlayerTrace dance, which mutates the shared physent
// prediction state and, when run from the render pass (HUD_DrawTransparentTriangles), corrupted the
// state the TriAPI impact-particle pool relies on (the bullet/melee sparks vanished). PM_TraceLine
// doesn't touch that state, so it's safe to call every frame during rendering.
static void TraceAim(int idx, const Vector& muzzle, const Vector& forward, Vector& hit)
{
	Vector end = muzzle + forward * 8192.0f;
	pmtrace_t* tr = gEngfuncs.PM_TraceLine((float*)&muzzle, (float*)&end, PM_TRACELINE_PHYSENTSONLY, 2, -1);
	if (tr)
		hit = Vector(tr->endpos[0], tr->endpos[1], tr->endpos[2]);
	else
		hit = end;
}

// Draw a camera-facing beam quad (start->end) of the given half-width, textured by sprite `spr`,
// additive, in colour (r,g,b). The width vector is perpendicular to both the beam and the view dir
// so the thin quad always faces the camera (the classic beam billboard).
static void DrawBeam(const Vector& start, const Vector& end, int spr, float halfWidth,
	float r, float g, float b)
{
	model_t* model = (model_t*)gEngfuncs.GetSpritePointer((HSPRITE)spr);
	if (!model)
		return;

	Vector vAngles, vF, vR, vU;
	gEngfuncs.GetViewAngles((float*)vAngles);
	AngleVectors(vAngles, vF, vR, vU);

	Vector beam = end - start;
	Vector side = CrossProduct(beam, vF);
	const float len = side.Length();
	if (len < 0.0001f)
		return;
	side = side * (halfWidth / len);

	triangleapi_t* tri = gEngfuncs.pTriAPI;
	if (!tri->SpriteTexture(model, 0))
		return;
	tri->RenderMode(kRenderTransAdd);
	tri->CullFace(TRI_NONE);
	tri->Begin(TRI_QUADS);
	tri->Color4f(r, g, b, 1.0f);
	Vector c0 = start - side, c1 = start + side, c2 = end + side, c3 = end - side;
	tri->TexCoord2f(0, 0); tri->Vertex3fv((float*)&c0);
	tri->TexCoord2f(0, 1); tri->Vertex3fv((float*)&c1);
	tri->TexCoord2f(1, 1); tri->Vertex3fv((float*)&c2);
	tri->TexCoord2f(1, 0); tri->Vertex3fv((float*)&c3);
	tri->End();
	tri->RenderMode(kRenderNormal);
}

// Draw a camera-facing dot sprite at `pos`.
static void DrawDot(const Vector& pos, int spr, float size, float r, float g, float b)
{
	model_t* model = (model_t*)gEngfuncs.GetSpritePointer((HSPRITE)spr);
	if (!model)
		return;

	Vector vAngles, vF, vR, vU;
	gEngfuncs.GetViewAngles((float*)vAngles);
	AngleVectors(vAngles, vF, vR, vU);

	triangleapi_t* tri = gEngfuncs.pTriAPI;
	if (!tri->SpriteTexture(model, 0))
		return;
	tri->RenderMode(kRenderTransAdd);
	tri->CullFace(TRI_NONE);
	tri->Begin(TRI_QUADS);
	tri->Color4f(r, g, b, 1.0f);
	Vector ru = (vR + vU) * size, rd = (vR - vU) * size;
	Vector c0 = pos - ru, c1 = pos - rd, c2 = pos + ru, c3 = pos + rd;
	tri->TexCoord2f(0, 0); tri->Vertex3fv((float*)&c0);
	tri->TexCoord2f(0, 1); tri->Vertex3fv((float*)&c1);
	tri->TexCoord2f(1, 1); tri->Vertex3fv((float*)&c2);
	tri->TexCoord2f(1, 0); tri->Vertex3fv((float*)&c3);
	tri->End();
	tri->RenderMode(kRenderNormal);
}

void TS_Attachments_Draw()
{
	const int maxClients = gEngfuncs.GetMaxClients();
	cl_entity_t* local = gEngfuncs.GetLocalPlayer();

	if (s_laserBeamSpr == 0)
		s_laserBeamSpr = SPR_Load("sprites/laserbeam.spr");
	if (s_laserDotSpr == 0)
		s_laserDotSpr = SPR_Load("sprites/laserdot.spr");

	for (int i = 1; i <= maxClients && i < TS_MAXPLAYERS; i++)
	{
		const unsigned char attach = g_playerAttach[i];
		if ((attach & (TS_ATT_LASER | TS_ATT_FLASH)) == 0)
			continue;

		Vector muzzle, forward, hit;
		if (!GetMuzzleAim(i, muzzle, forward))
			continue;
		TraceAim(i, muzzle, forward, hit);

		if (attach & TS_ATT_LASER)
		{
			// local player's laser colour from cl_lasercolor; others default red (the per-player
			// colour byte the original networks separately is a SLICE — see ts_attachments.h).
			float r, g, b;
			const bool isLocal = local && local->index == i;
			GetLaserColor(isLocal ? (int)gEngfuncs.pfnGetCvarFloat("cl_lasercolor") : 0, r, g, b);
			DrawBeam(muzzle, hit, s_laserBeamSpr, 1.0f, r, g, b);
			DrawDot(hit, s_laserDotSpr, 3.0f, r, g, b);
		}

		if (attach & TS_ATT_FLASH)
		{
			// a white dynamic light at the surface the flashlight lands on (RE FUN_1003b750:
			// CL_AllocDlight at the hit, white, radius by distance) + a faint white beam.
			dlight_t* dl = gEngfuncs.pEfxAPI->CL_AllocDlight(i); // key on player index -> updated, not stacked
			if (dl)
			{
				VectorCopy((float*)&hit, dl->origin);
				dl->radius = 80.0f;
				dl->color.r = 255; dl->color.g = 255; dl->color.b = 255;
				dl->die = gEngfuncs.GetClientTime() + 0.1f;
			}
			DrawBeam(muzzle, hit, s_laserBeamSpr, 1.5f, 0.6f, 0.6f, 0.6f);
		}
	}
}
