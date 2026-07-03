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

extern Vector v_origin, v_angles; // view origin/angles (cl_dll/view.cpp) — for the horizontal centering

#define TS_ATT_LASER 2 // attach bit 1 (value 2)
#define TS_ATT_FLASH 4 // attach bit 2 (value 4)

#define TS_MAXPLAYERS 33 // 1..32 players (index 0 unused) — matches the original's 0..0x20 loop

// Per-player attach mask, set by the ActItems message (BYTE playerIndex, BYTE attachMask). Mirrors
// the original's DAT_1018d128 array (one byte per player; the original packs a colour byte too which
// it networks separately — we colour by cl_lasercolor, see GetLaserColor).
static unsigned char g_playerAttach[TS_MAXPLAYERS];
// PlInfo type 0 — each player's announced laser color byte (0/def red, 1 green, 2 blue).
static unsigned char g_playerLaserColor[TS_MAXPLAYERS];

static int s_laserBeamSpr = 0; // SPR_Load handles (lazy)
static int s_laserDotSpr = 0;
static int s_flashSpr = 0;     // sprites/flash.spr — the flashlight glow on the gun (active flashlight)

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

// PlInfo — the per-player info broadcast: BYTE playerIndex, BYTE type, BYTE value.
// Type 0 = laser color (the only type the original sends). Raw byte, no clamp.
int MsgFunc_PlInfo(const char* pszName, int iSize, void* pbuf)
{
	BEGIN_READ(pbuf, iSize);
	const int idx = READ_BYTE();
	const int type = READ_BYTE();
	const int val = READ_BYTE();
	if (idx >= 1 && idx < TS_MAXPLAYERS && type == 0)
		g_playerLaserColor[idx] = (unsigned char)val;
	return 1;
}

void TS_Attachments_Init()
{
	memset(g_playerAttach, 0, sizeof(g_playerAttach));
	memset(g_playerLaserColor, 0, sizeof(g_playerLaserColor));
	gEngfuncs.pfnHookUserMsg("ActItems", MsgFunc_ActItems);
	gEngfuncs.pfnHookUserMsg("PlInfo", MsgFunc_PlInfo);
}

// Map change (HUD VidInit): the cached SPR_Load handles index the per-map sprite list, so they go
// STALE after a load — drop them so they re-load fresh on next draw.
//
// Do NOT clear g_playerAttach here. The server broadcasts ActItems only on CHANGE (it caches the
// last-sent mask in CTSGun::m_iClientAttach), and that cache SURVIVES a changelevel (the gun object
// persists) — so it never re-broadcasts on the new map. client.so is not reloaded across a map
// change either, so this static array stays in sync with the server's surviving cache as long as we
// leave it alone. Clearing it here desynced the two → the laser/flashlight (and the dive-reticle
// distance) vanished after a map change. The original keeps its array likewise (a static global
// never cleared on level change), which is why it doesn't have this bug. See [[map-change-state-reset]].
void TS_Attachments_VidInit()
{
	s_laserBeamSpr = 0;
	s_laserDotSpr = 0;
	s_flashSpr = 0;
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
	float r, float g, float b, int frame = 0)
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
	if (!tri->SpriteTexture(model, frame))
		return;
	tri->RenderMode(kRenderTransAdd);
	tri->CullFace(TRI_NONE);
	tri->Begin(TRI_QUADS);
	tri->Color4f(r, g, b, 1.0f);
	Vector c0 = start - side, c1 = start + side, c2 = end + side, c3 = end - side;
	// laserbeamanim.spr is a VERTICAL strip (32x128): run its LONG axis (V) along the beam length and its
	// width (U) across the beam, else the beam texture is laid out sideways.
	tri->TexCoord2f(0, 0); tri->Vertex3fv((float*)&c0);
	tri->TexCoord2f(1, 0); tri->Vertex3fv((float*)&c1);
	tri->TexCoord2f(1, 1); tri->Vertex3fv((float*)&c2);
	tri->TexCoord2f(0, 1); tri->Vertex3fv((float*)&c3);
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

// --- LOCAL laser dot/beam endpoint ------------------------------------------------------------------
// The dot/beam follow the gun MODEL's actual aim (which the engine viewmodel physically raises when
// moving on the ground — that's where the "aims higher" comes from, not a computed offset). We only
// strip the gun's static RIGHTWARD offset (it's held to the right) so the dot pins to the crosshair x;
// the vertical rides the real model aim. The same point feeds the beam end and the HUD dot so the beam
// runs straight to the dot. Returns false if no active local laser.
static bool LocalLaserEndpoint(Vector& muzzle, Vector& endpoint)
{
	cl_entity_t* lp = gEngfuncs.GetLocalPlayer();
	if (!lp || (g_playerAttach[lp->index] & TS_ATT_LASER) == 0)
		return false;
	Vector forward, hit, vr, vf, vu;
	if (!GetMuzzleAim(lp->index, muzzle, forward))
		return false;
	TraceAim(lp->index, muzzle, forward, hit);
	const float d = (hit - muzzle).Length(); // surface distance along the gun aim (beam length)
	AngleVectors(v_angles, vf, vr, vu);
	// SCREENSPACE -> gamespace: the dot is fixed in SCREEN space — the gun DIRECTION (horizontally
	// centred), which is distance-INDEPENDENT, so the dot stays put even up close (no near-parallax).
	// Every point on an EYE ray projects to one screen point, so place the endpoint on the eye ray
	// along that direction at the surface distance — the dot lands there and the beam from the gun
	// runs to it. The model's movespeed aim-up is already in `forward`, so the dot still rides up.
	Vector dir = forward;
	const float rc = dir.x * vr.x + dir.y * vr.y + dir.z * vr.z; // strip rightward -> pin x to centre
	dir = dir - vr * rc;
	const float dl = dir.Length();
	if (dl > 0.001f)
		dir = dir * (1.0f / dl);
	endpoint = v_origin + dir * d;
	return true;
}

void TS_Attachments_Draw()
{
	// The lasclr_ announcer (dossier-gmsg U2): on a cl_lasercolor change, tell the server via
	// the PREFIX COMMAND `lasclr_<n>` (pfnServerCmd — the original client sends a server cmd,
	// not a ClientCmd). The server rebroadcasts it as PlInfo to everyone. No late-joiner
	// resend — the original has none (a joiner sees red until the owner next changes it).
	static int s_lastLaserColor = -1;
	const int lc = (int)gEngfuncs.pfnGetCvarFloat("cl_lasercolor");
	if (lc != s_lastLaserColor)
	{
		s_lastLaserColor = lc;
		char cmd[32];
		sprintf(cmd, "lasclr_%i", lc);
		gEngfuncs.pfnServerCmd(cmd);
	}

	const int maxClients = gEngfuncs.GetMaxClients();
	cl_entity_t* local = gEngfuncs.GetLocalPlayer();

	if (s_laserBeamSpr == 0)
		s_laserBeamSpr = SPR_Load("sprites/laserbeamanim.spr"); // 16-frame animated beam (the original's)
	if (s_laserDotSpr == 0)
		s_laserDotSpr = SPR_Load("sprites/laserdot.spr");
	if (s_flashSpr == 0)
		s_flashSpr = SPR_Load("sprites/flash.spr");

	// animate the beam: cycle the 16 frames over time (~30fps)
	const int beamFrame = ((int)(gEngfuncs.GetClientTime() * 30.0f)) % 16;

	for (int i = 1; i <= maxClients && i < TS_MAXPLAYERS; i++)
	{
		const unsigned char attach = g_playerAttach[i];
		if ((attach & (TS_ATT_LASER | TS_ATT_FLASH)) == 0)
			continue;

		Vector muzzle, forward, hit;
		if (!GetMuzzleAim(i, muzzle, forward))
			continue;
		TraceAim(i, muzzle, forward, hit);

		const bool isLocal = local && local->index == i;
		if (attach & TS_ATT_LASER)
		{
			// local laser uses cl_lasercolor; others use their PlInfo-announced color byte.
			float r, g, b;
			GetLaserColor(isLocal ? (int)gEngfuncs.pfnGetCvarFloat("cl_lasercolor")
			                      : (int)g_playerLaserColor[i], r, g, b);
			// The lasersight mounts UNDER the barrel — start the beam below the muzzle (it was emitting
			// from the barrel top). Offset along the gun's down (perpendicular to the aim).
			Vector right = CrossProduct(forward, Vector(0, 0, 1));
			Vector up = CrossProduct(right, forward);
			const float ul = up.Length();
			if (ul > 0.001f)
				up = up * (1.0f / ul);
			const Vector lmuzzle = muzzle - up * 3.0f;
			// The local beam ends at the horizontally-CENTRED endpoint (so it lines up with the HUD dot);
			// others' beams go to the real surface hit. Thicker: RE FUN_1003d600 width 4.0 (FUN_10050080).
			Vector beamEnd = hit;
			if (isLocal)
			{
				Vector m2, ep;
				if (LocalLaserEndpoint(m2, ep))
					beamEnd = ep;
			}
			DrawBeam(lmuzzle, beamEnd, s_laserBeamSpr, 1.25f, r, g, b, beamFrame);
			// The SURFACE dot is OTHERS-only — the local player's dot is the HUD dot (ts_reticle.cpp /
			// TS_LocalLaserDot), since the original draws no world dot for the local player.
			if (!isLocal)
				DrawDot(hit, s_laserDotSpr, 3.0f, r, g, b);
		}

		if (attach & TS_ATT_FLASH)
		{
			// The flashlight is ONLY the white dynamic light at the surface it lands on (RE FUN_1003b750:
			// CL_AllocDlight at the hit, white, radius by distance). NO muzzle beam/trail — only the LASER
			// draws a beam. (The old faint grey beam here used the laser sprite, so with both active it
			// overlapped the laser beam and read as a much wider laser; removed.)
			dlight_t* dl = gEngfuncs.pEfxAPI->CL_AllocDlight(i); // key on player index -> updated, not stacked
			if (dl)
			{
				VectorCopy((float*)&hit, dl->origin);
				dl->radius = 120.0f; // 1.5x the prior 80 (user-tuned)
				dl->color.r = 255; dl->color.g = 255; dl->color.b = 255;
				dl->die = gEngfuncs.GetClientTime() + 0.1f;
			}
			// flash.spr glow ON the gun (the flashlight device), shown while the flashlight is active —
			// at the muzzle attachment for every player (local viewmodel + others' world models).
			if (s_flashSpr)
				DrawDot(muzzle, s_flashSpr, 4.0f, 1.0f, 1.0f, 1.0f);
		}
	}
}

// Does the LOCAL player have a lasersight equipped? (the dive-reticle uses it for the distance
// readout + tighter crosshair ticks). Reads the per-player attach mask the server broadcasts.
bool TS_LocalHasLaser()
{
	cl_entity_t* lp = gEngfuncs.GetLocalPlayer();
	if (!lp)
		return false;
	int idx = lp->index;
	if (idx < 1 || idx >= TS_MAXPLAYERS)
		return false;
	return (g_playerAttach[idx] & TS_ATT_LASER) != 0;
}

// The LOCAL player's ACTIVE attachment mask (the ActItems broadcast carries the active state — laser
// bit 2 / flash bit 4 toggled by `useitems`). Used by the bottom weapon-box to highlight the active
// attachments. Returns 0 on error.
int TS_LocalActiveAttach()
{
	cl_entity_t* lp = gEngfuncs.GetLocalPlayer();
	if (!lp) return 0;
	int idx = lp->index;
	if (idx < 1 || idx >= TS_MAXPLAYERS) return 0;
	return g_playerAttach[idx];
}

// The LOCAL player's laser dot is a HUD dot, not a world sprite (RE: the 3D laser draw FUN_1003d600
// skips the local player). It marks where the gun MODEL is actually pointing — trace along the
// viewmodel's muzzle aim and project the hit to screen, so the dot sits near the reticle and shifts
// with the gun (which isn't exactly the view centre). Returns false if no active laser / behind view.
bool TS_LocalLaserDot(int& sx, int& sy, int& r, int& g, int& b)
{
	cl_entity_t* lp = gEngfuncs.GetLocalPlayer();
	if (!lp || (g_playerAttach[lp->index] & TS_ATT_LASER) == 0)
		return false;

	// The dot IS the beam's (horizontally-centred) endpoint — they coincide, so the beam runs straight to
	// the dot. Centred in x (the gun's rightward offset stripped), vertical rides the aim/bob.
	Vector muzzle, endpoint;
	if (!LocalLaserEndpoint(muzzle, endpoint))
		return false;

	float screen[3];
	if (gEngfuncs.pTriAPI->WorldToScreen((float*)&endpoint, screen)) // returns 1 if behind the view
		return false;
	sx = (int)((1.0f + screen[0]) * 0.5f * (float)ScreenWidth);
	sy = (int)((1.0f - screen[1]) * 0.5f * (float)ScreenHeight);

	float fr, fg, fb;
	GetLaserColor((int)gEngfuncs.pfnGetCvarFloat("cl_lasercolor"), fr, fg, fb);
	r = (int)(fr * 255.0f); g = (int)(fg * 255.0f); b = (int)(fb * 255.0f);
	return true;
}
