/***
*  ts_attachments.cpp — the ActItems / PlInfo attachment state, the lasersights and flashlights drawn in
*  the world, and the player blob shadows. The laser and flashlight passes are client.dll's, run from its
*  FX root in this order: the local laser 0x1003bb00 and the others' lasers 0x1003d600 (both via
*  0x1003de00), the local flashlight 0x1003b250 and the others' flashlights 0x1003b750 (via 0x1003bad0).
*  The local laser pass also leaves the weapon bar the projected beam ends, their distances and the
*  laser range number (+0x18 .. +0x30) that the HUD's laser dots and range read (ts_reticle.cpp).
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
#include "ts_gunanchor.h" // TS_PlayerGunAtt — the renderer-fed gun-anchor cache
#include "ts_particles.h" // the pool sprite table the passes register their sprites in
#include "player_info.h" // g_PlayerInfoList
#include <stdio.h>
extern "C" int CL_IsThirdPerson(); // in_camera.cpp

#include <string.h>
#include <math.h>

extern Vector v_origin, v_angles; // view origin/angles (cl_dll/view.cpp): the passes take their view vectors from v_angles

#define TS_ATT_LASER 2 // attach bit 1 (value 2)
#define TS_ATT_FLASH 4 // attach bit 2 (value 4)

#define TS_MAXPLAYERS 33 // 1..32 players (index 0 unused) — matches the original's 0..0x20 loop

// Per-player attach mask, set by the ActItems message (BYTE playerIndex, BYTE attachMask). Mirrors
// the original's DAT_1018d128 array (one byte per player; the original packs a colour byte too,
// networked separately via PlInfo — g_playerLaserColor below, the ONLY render colour source).
static unsigned char g_playerAttach[TS_MAXPLAYERS];
// PlInfo type 0 — each player's announced laser color byte (0/def red, 1 green, 2 blue).
static unsigned char g_playerLaserColor[TS_MAXPLAYERS];


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

// client.dll's Spectator handler (0x1006530c) clears the player's ActItems byte on every Spectator
// message for him.
void TS_ClearPlayerAttach(int idx)
{
	if (idx >= 0 && idx < TS_MAXPLAYERS)
		g_playerAttach[idx] = 0;
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
// last-sent mask in CBasePlayer::m_iClientActiveItems, player+0x720, cleared by TSInit and
// ForceClientDllUpdate, so a zero mask is not sent again). client.so is not reloaded across a map
// change either, so this static array stays in sync with the server's cache as long as we leave it
// alone. Clearing it here desynced the two → the laser/flashlight (and the dive-reticle
// distance) vanished after a map change. The original keeps its array likewise (a static global
// never cleared on level change), which is why it doesn't have this bug. See [[map-change-state-reset]].
static int s_shadowSpr = 0; // sprites/ombra.spr, the player shadow (TS_Shadows_Draw)

void TS_Attachments_VidInit()
{
	// Sprite handles change on every map load (HUD VidInit reloads hud.txt since 753f738, as
	// client.dll), so a handle cached from the previous map draws another sprite. client.dll looks
	// ombra.spr up again on every draw (0x10055801 -> 0x100509d0).
	s_shadowSpr = 0;
}

// Laser colour (FUN_100582f0): 1 green, 2 blue, anything else red.
static void LaserColor(int index, int& r, int& g, int& b)
{
	if (index == 1) { r = 0; g = 0xcd; b = 0; }
	else if (index == 2) { r = 0; g = 0x78; b = 0xff; }
	else { r = 0xff; g = 0x19; b = 0x19; }
}

// Players HUD_AddEntity let through this frame (client.dll 0x1018bd44, set at 0x1004485a, cleared by
// the FX root after the attachment passes, 0x10056f80).
static bool s_bAdded[TS_MAXPLAYERS];

void TS_AttachPlayerAdded(int index)
{
	if (index >= 0 && index < TS_MAXPLAYERS)
		s_bAdded[index] = true;
}

// FUN_100579a0: the flag for players 0..32, false for any other index.
bool TS_AttachPlayerWasAdded(int index)
{
	return index >= 0 && index < TS_MAXPLAYERS && s_bAdded[index];
}

// The view vectors the passes use (client.dll 0x1018bd90 / 0x1018bd9c / 0x1018bda8).
static Vector s_vForward, s_vRight, s_vUp;

// A view-facing quad of half size `size` at `pos` (FUN_10050080 with its view-facing flag, then
// FUN_10050000), inside the caller's Begin(TRI_QUADS).
static void EmitViewQuad(const Vector& pos, float size)
{
	const Vector up = s_vUp * size, right = s_vRight * size;
	const Vector c[4] = {pos + up - right, pos + up + right, pos - up + right, pos - up - right};
	static const float s_s[4] = {0, 1, 1, 0}, s_t[4] = {0, 0, 1, 1};
	for (int i = 0; i < 4; i++)
	{
		gEngfuncs.pTriAPI->TexCoord2f(s_s[i], s_t[i]);
		gEngfuncs.pTriAPI->Vertex3fv((float*)&c[i]);
	}
}

// A beam from `start` to `end`, `halfWidth` along the view's up (FUN_1003d280 and the local beam's
// quad), with the texture already bound.
static void EmitBeam(const Vector& start, const Vector& end, float halfWidth, float r, float g, float b)
{
	triangleapi_t* tri = gEngfuncs.pTriAPI;
	tri->RenderMode(kRenderTransAdd);
	tri->CullFace(TRI_NONE);
	tri->Begin(TRI_QUADS);
	tri->Color4f(r, g, b, 1.0f);
	const Vector w = s_vUp * halfWidth;
	const Vector c0 = end - w, c1 = end + w, c2 = start + w, c3 = start - w;
	tri->TexCoord2f(0, 0); tri->Vertex3fv((float*)&c0);
	tri->TexCoord2f(1, 0); tri->Vertex3fv((float*)&c1);
	tri->TexCoord2f(1, 1); tri->Vertex3fv((float*)&c2);
	tri->TexCoord2f(0, 1); tri->Vertex3fv((float*)&c3);
	tri->End();
	tri->RenderMode(kRenderNormal);
}

// Binds frame `frame` of pool sprite `idx`; false when it has none.
static bool BindPoolSprite(int idx, int frame)
{
	model_t* model = (model_t*)gEngfuncs.GetSpritePointer(TSParticles_SpriteHandle(idx));
	return model != nullptr && 0 != gEngfuncs.pTriAPI->SpriteTexture(model, frame);
}

// A player trace (studio boxes, every player solid but `shooter`, which it also ignores), as each pass
// sets it up.
static void PlayerTrace(int shooter, const Vector& start, const Vector& end, pmtrace_t& tr)
{
	gEngfuncs.pEventAPI->EV_PushPMStates();
	gEngfuncs.pEventAPI->EV_SetUpPlayerPrediction(0, 1);
	gEngfuncs.pEventAPI->EV_SetSolidPlayers(shooter - 1);
	gEngfuncs.pEventAPI->EV_SetTraceHull(2);
	gEngfuncs.pEventAPI->EV_PlayerTrace((float*)&start, (float*)&end, PM_STUDIO_BOX, shooter, &tr);
	gEngfuncs.pEventAPI->EV_PopPMStates();
}

// Another player's eye and view angles (FUN_10068e20): his origin, his angles with the pitch x -pi;
// not solid (dead), the roll set to 80 and 8 lower; else, unless stunt flag 0x400000 is set, 16 up
// when ducked (usehull 1) and 28 up otherwise. Without a player name it leaves both as they were.
void TS_PlayerEye(int idx, Vector& pos, Vector& ang)
{
	cl_entity_t* ent = gEngfuncs.GetEntityByIndex(idx);
	if (ent == nullptr || 0 == ent->player || g_PlayerInfoList[idx].name == nullptr)
		return;
	pos = ent->origin;
	ang = ent->angles;
	ang[0] = (float)(ang[0] * -3.141592653589793);
	if (ent->curstate.solid == 0)
	{
		ang[2] = 80.0f;
		pos[2] -= 8.0f;
		return;
	}
	if ((ent->curstate.iuser4 & 0x400000) != 0)
		return;
	pos[2] += ent->curstate.usehull == 1 ? 16.0f : 28.0f;
}

// The local player's laser as the weapon bar keeps it (+0x1c..+0x30, +0x18): each beam's projected end
// (x, y), -1 when off screen, its distance, and the range number.
static float s_laserDot[2][2] = {{-1, -1}, {-1, -1}};
static float s_laserDist[2];
static int s_laserRange;
static float s_beamFrame; // 0x101899b4

// The local viewmodel the laser and flashlight start from (client.dll 0x101914a0): first person with
// a field of view of 40 or more, else none.
static cl_entity_t* LocalGunModel()
{
	if (0 != CL_IsThirdPerson() || gHUD.m_iFOV < 40)
		return nullptr;
	cl_entity_t* vm = gEngfuncs.GetViewModel();
	return (vm != nullptr && vm->model != nullptr) ? vm : nullptr;
}

// A local beam's direction (0x1003bb6f): the view forward without a viewmodel or with cl_laser exactly
// 1, else the viewmodel's attachment a -> b direction (straight up when they coincide) plus a jitter of
// jx, jy x 0.003 on x and y.
static Vector LocalLaserDir(cl_entity_t* vm, int a, int b, float jx, float jy)
{
	static cvar_t* s_cl_laser = nullptr;
	if (s_cl_laser == nullptr)
		s_cl_laser = gEngfuncs.pfnGetCvarPointer("cl_laser");
	if (vm == nullptr || (s_cl_laser != nullptr && s_cl_laser->value == 1.0f))
		return s_vForward;
	Vector d = Vector(vm->attachment[b]) - Vector(vm->attachment[a]);
	const float len = d.Length();
	d = len == 0.0f ? Vector(0, 0, 1) : d * (1.0f / len);
	return Vector(d.x + jx * 0.003f, d.y + jy * 0.003f, d.z);
}

static void ProjectDot(const Vector& end, int which)
{
	const Vector p = end - s_vForward * 2.0f;
	float screen[3];
	if (0 != gEngfuncs.pTriAPI->WorldToScreen((float*)&p, screen))
	{
		s_laserDot[which][0] = s_laserDot[which][1] = -1.0f;
		return;
	}
	s_laserDot[which][0] = (float)(int)((1.0f + screen[0]) * ScreenWidth / 2);
	s_laserDot[which][1] = (float)(int)((1.0f - screen[1]) * ScreenHeight / 2);
}

// client.dll 0x1003bb00, with the local player's laser on (ActItems bit 2). One beam, or two with the
// akimbo bit 0x40, each traced 8192 units from the eye along its direction (the akimbo pair from 4 to
// the right and 4 to the left of it). Each end, 2 units back along the view, is projected for the HUD
// dot. With the viewmodel the beams are drawn: laserbeamanim.spr, its frame advancing 20 a second of
// slowed time, additive, from the attachment (0, or 2 for the second gun) one unit below along the view
// up to the end 2 units short, 0.5 x w wide, in the laser colour x w, where w = 1 - max(128 - d, 0) /
// 128 for the first beam's distance d. Last the range number, (int)(d x 0.025).
static void DrawLocalLaser(cl_entity_t* lp)
{
	const int idx = lp->index;
	const unsigned char attach = g_playerAttach[idx];
	if ((attach & TS_ATT_LASER) == 0)
		return;
	const float jx = gEngfuncs.pfnRandomFloat(-1.0f, 1.0f);
	const float jy = gEngfuncs.pfnRandomFloat(-1.0f, 1.0f);
	cl_entity_t* vm = LocalGunModel();
	extern Vector TS_AimEye();
	const Vector eye = TS_AimEye();
	const bool akimbo = (attach & 0x40) != 0;
	const Vector dir1 = LocalLaserDir(vm, 0, 1, jx, jy);
	Vector dir2;
	pmtrace_t tr1, tr2;
	if (!akimbo)
	{
		PlayerTrace(idx, eye, eye + dir1 * 8192.0f, tr1);
		s_laserDist[0] = (Vector(tr1.endpos) - eye).Length();
		s_laserDist[1] = 0;
	}
	else
	{
		dir2 = LocalLaserDir(vm, 2, 3, jy, jx);
		PlayerTrace(idx, eye + s_vRight * 4.0f, eye + dir1 * 8192.0f + s_vRight * 4.0f, tr1);
		PlayerTrace(idx, eye + s_vRight * -4.0f, eye + dir2 * 8192.0f + s_vRight * -4.0f, tr2);
		s_laserDist[0] = (Vector(tr1.endpos) - eye).Length();
		s_laserDist[1] = (Vector(tr2.endpos) - eye).Length();
	}
	ProjectDot(tr1.endpos, 0);
	if (akimbo)
		ProjectDot(tr2.endpos, 1);

	if (vm != nullptr)
	{
		const int spr = TSParticles_Sprite("sprites/laserbeamanim.spr", kRenderTransAdd);
		if (spr == -1)
			return;
		extern float g_flClientSlow;
		s_beamFrame += g_flClientSlow * (float)gHUD.m_flTimeDelta * 20.0f;
		const int frames = SPR_Frames(TSParticles_SpriteHandle(spr));
		if ((int)s_beamFrame < 0 || frames - 1 < (int)s_beamFrame)
			s_beamFrame = 0;
		if (!BindPoolSprite(spr, (int)s_beamFrame))
			return;
		float w = 128.0f - s_laserDist[0];
		if (w < 0.0f)
			w = 0.0f;
		w = 1.0f - w * (1.0f / 128.0f);
		int r, g, b;
		LaserColor(g_playerLaserColor[idx], r, g, b);
		const float k = w * (1.0f / 255.0f);
		EmitBeam(Vector(vm->attachment[0]) - s_vUp, Vector(tr1.endpos) - dir1 * 2.0f, 0.5f * w, r * k, g * k, b * k);
		if (akimbo)
			EmitBeam(Vector(vm->attachment[2]) - s_vUp, Vector(tr2.endpos) - dir2 * 2.0f, 0.5f * w, r * k, g * k, b * k);
	}
	s_laserRange = (int)(s_laserDist[0] * 0.025f);
}

// client.dll 0x1003d600: every other player with his laser on, drawn this frame. His eye (PlayerEye)
// with the view angles jittered by -0.05..0.05 degrees (one pair of draws a frame for all) gives a
// forward, replaced by his gun's direction (gun anchors 0 -> 1) when they are more than about 45
// degrees apart (dot product under 0.7). His smoothed direction eases to it by min(frame time x 15, 1)
// and is normalised; the trace runs 8192 units from the eye. At the end, 2 units short, a laserdot.spr
// quad of half size 4 facing the view; then a laserbeam.spr beam 0.5 wide from 16 units out of his gun
// anchor 0 to 16 units short of the end, both additive in his laser colour.
static Vector s_otherDir[TS_MAXPLAYERS]; // 0x10189638
static void DrawOtherLasers(int local)
{
	const int dotSpr = TSParticles_Sprite("sprites/laserdot.spr", kRenderTransAdd);
	if (dotSpr == -1)
		return;
	const int beamSpr = TSParticles_Sprite("sprites/laserbeam.spr", kRenderTransAdd);
	if (beamSpr == -1)
		return;
	const float jx = gEngfuncs.pfnRandomFloat(-1.0f, 1.0f);
	const float jy = gEngfuncs.pfnRandomFloat(-1.0f, 1.0f);
	static Vector s_pos, s_ang, s_fwd; // 0x10189960, 0x10189980, 0x10189620
	const int maxClients = gEngfuncs.GetMaxClients();
	for (int i = 0; i <= maxClients && i < TS_MAXPLAYERS; i++)
	{
		if (i == local || (g_playerAttach[i] & TS_ATT_LASER) == 0)
			continue;
		cl_entity_t* ent = gEngfuncs.GetEntityByIndex(i);
		if (ent == nullptr || 0 == ent->player || !s_bAdded[i])
			continue;
		if (!BindPoolSprite(dotSpr, 0))
			continue;
		TS_PlayerEye(i, s_pos, s_ang);
		s_ang[0] += jx * 0.05f;
		s_ang[1] += jy * 0.05f;
		gEngfuncs.pfnAngleVectors(s_ang, s_fwd, nullptr, nullptr);
		const float* g0 = TS_PlayerGunAtt(i, 0);
		const float* g1 = TS_PlayerGunAtt(i, 1);
		const Vector a0(g0[0], g0[1], g0[2]), a1(g1[0], g1[1], g1[2]);
		Vector gun = a1 - a0;
		VectorNormalize(gun);
		if (DotProduct(s_fwd, gun) < 0.7f)
			s_fwd = gun;
		float k = (float)gHUD.m_flTimeDelta * 15.0f;
		if (1.0f < k)
			k = 1.0f;
		s_otherDir[i] = s_otherDir[i] + (s_fwd - s_otherDir[i]) * k;
		VectorNormalize(s_otherDir[i]);
		pmtrace_t tr;
		PlayerTrace(ent->index, s_pos, s_pos + s_otherDir[i] * 8192.0f, tr);
		const Vector end = Vector(tr.endpos) - s_otherDir[i] * 2.0f;
		int r, g, b;
		LaserColor(g_playerLaserColor[i], r, g, b);
		triangleapi_t* tri = gEngfuncs.pTriAPI;
		tri->RenderMode(kRenderTransAdd);
		tri->CullFace(TRI_NONE);
		tri->Begin(TRI_QUADS);
		tri->Color4f(r / 255.0f, g / 255.0f, b / 255.0f, 1.0f);
		EmitViewQuad(end, 4.0f);
		tri->End();
		tri->RenderMode(kRenderNormal);
		if (BindPoolSprite(beamSpr, 0))
			EmitBeam(a0 + s_otherDir[i] * 16.0f, end - s_otherDir[i] * 16.0f, 0.5f, r / 255.0f, g / 255.0f, b / 255.0f);
	}
}

// A flashlight's light (0x1003b33b / 0x1003b80b): where its 1024 unit trace stops short, a white dynamic
// light keyed `key`, radius 1024 less the distance from the local eye, at most 120, for 0.1 s.
static void FlashlightLight(int key, const pmtrace_t& tr)
{
	if (tr.fraction == 1.0f)
		return;
	extern Vector TS_AimEye();
	const float dist = (Vector(tr.endpos) - TS_AimEye()).Length();
	dlight_t* dl = gEngfuncs.pEfxAPI->CL_AllocDlight(key);
	if (dl == nullptr)
		return;
	VectorCopy(tr.endpos, dl->origin);
	float radius = 1024.0f - dist;
	if (120.0f <= radius)
		radius = 120.0f;
	dl->radius = radius;
	dl->color.r = 255;
	dl->color.g = 255;
	dl->color.b = 255;
	dl->die = (float)(gEngfuncs.GetClientTime() + 0.1);
}

// client.dll 0x1003b250, the local flashlight (ActItems bit 4): the light (key 2) from a trace 1024
// units along the view from the eye; with the viewmodel a flash.spr quad of half size 6 facing the view,
// one unit below attachment 0 along the view up, grey 0.7 additive, and with the akimbo bit a half size
// 4 one at attachment 2.
static void DrawLocalFlashlight(cl_entity_t* lp)
{
	const unsigned char attach = g_playerAttach[lp->index];
	if ((attach & TS_ATT_FLASH) == 0)
		return;
	extern Vector TS_AimEye();
	const Vector eye = TS_AimEye();
	pmtrace_t tr;
	PlayerTrace(lp->index, eye, eye + s_vForward * 1024.0f, tr);
	FlashlightLight(2, tr);
	cl_entity_t* vm = LocalGunModel();
	if (vm == nullptr)
		return;
	const int spr = TSParticles_Sprite("sprites/flash.spr", kRenderTransAdd);
	if (spr == -1 || !BindPoolSprite(spr, 0))
		return;
	triangleapi_t* tri = gEngfuncs.pTriAPI;
	tri->RenderMode(kRenderTransAdd);
	tri->CullFace(TRI_NONE);
	tri->Begin(TRI_QUADS);
	tri->Color4f(0.7f, 0.7f, 0.7f, 1.0f);
	EmitViewQuad(Vector(vm->attachment[0]) - s_vUp, 6.0f);
	if ((attach & 0x40) != 0)
		EmitViewQuad(Vector(vm->attachment[2]) - s_vUp, 4.0f);
	tri->End();
	tri->RenderMode(kRenderNormal);
}

// client.dll 0x1003b750: every other player with his flashlight on, drawn this frame: the light (key
// his index + 3) from a trace 1024 units along his eye's view angles. No sprite on his gun.
static void DrawOtherFlashlights(int local)
{
	static Vector s_pos, s_ang; // 0x10189578, 0x10189568
	const int maxClients = gEngfuncs.GetMaxClients();
	for (int i = 0; i <= maxClients && i < TS_MAXPLAYERS; i++)
	{
		if (i == local || (g_playerAttach[i] & TS_ATT_FLASH) == 0)
			continue;
		cl_entity_t* ent = gEngfuncs.GetEntityByIndex(i);
		if (ent == nullptr || 0 == ent->player || !s_bAdded[i])
			continue;
		TS_PlayerEye(i, s_pos, s_ang);
		Vector fwd;
		gEngfuncs.pfnAngleVectors(s_ang, fwd, nullptr, nullptr);
		pmtrace_t tr;
		PlayerTrace(ent->index, s_pos, s_pos + fwd * 1024.0f, tr);
		FlashlightLight(ent->index + 3, tr);
	}
}

// ---- player blob shadow (RE client.dll: the ombra.spr ground blob under each player) ---------------
// The original draws a flat "shadow" sprite (sprites/ombra.spr — ombra = shadow) on the ground under
// every drawn player, gated by the `cl_shadow` cvar (0 off; non-zero = the blob for every player; ==1
// ALSO the local player's own blob in first-person). Per player: trace straight down to the ground,
// dist = |origin - hit| - 12; skip if dist >= 200; alpha fades 1.0 - dist*0.005; a flat quad at the hit.
// Pure client presentation — no wire change. (RE HUD_DrawTransparentTriangles @0x34e50 -> the shadow pass.)
static cvar_t* s_cl_shadow = nullptr;

void TS_Shadows_Draw()
{
	if (s_cl_shadow == nullptr)
		s_cl_shadow = gEngfuncs.pfnGetCvarPointer("cl_shadow");
	if (s_cl_shadow != nullptr && s_cl_shadow->value == 0.0f)
		return; // shadows disabled
	const bool localInFirstPerson = (0 == CL_IsThirdPerson());
	const bool drawLocal = (s_cl_shadow != nullptr && s_cl_shadow->value == 1.0f) || !localInFirstPerson;

	if (s_shadowSpr == 0)
		s_shadowSpr = SPR_Load("sprites/ombra.spr");
	if (s_shadowSpr == 0)
		return;
	model_t* model = (model_t*)gEngfuncs.GetSpritePointer((HSPRITE)s_shadowSpr);
	if (model == nullptr)
		return;

	triangleapi_t* tri = gEngfuncs.pTriAPI;
	if (!tri->SpriteTexture(model, 0))
		return;
	tri->RenderMode(kRenderTransTexture); // alpha-blended blob (not additive — a dark shadow)
	tri->CullFace(TRI_NONE);

	cl_entity_t* local = gEngfuncs.GetLocalPlayer();
	const int maxClients = gEngfuncs.GetMaxClients();
	for (int i = 1; i <= maxClients && i < TS_MAXPLAYERS; i++)
	{
		cl_entity_t* p = gEngfuncs.GetEntityByIndex(i);
		if (p == nullptr || !p->player || p->model == nullptr)
			continue;
		if ((p->curstate.effects & EF_NODRAW) != 0)
			continue;
		const bool isLocal = (local != nullptr && local->index == i);
		if (isLocal && !drawLocal)
			continue;
		// client.dll's loop (0x10055760) draws a player's blob only when HUD_AddEntity let that player
		// through this frame (FUN_100579a0, the 0x1018bd44 flags), so a player who has left, whose
		// cl_entity keeps its last model and origin, gets none. The local player's first-person blob is
		// the original's separate block before the loop, which this loop stands in for.
		if (!isLocal && !TS_AttachPlayerWasAdded(i))
			continue;

		Vector origin(p->origin[0], p->origin[1], p->origin[2]);
		Vector end(origin.x, origin.y, origin.z - 8192.0f);
		pmtrace_t* tr = gEngfuncs.PM_TraceLine((float*)&origin, (float*)&end, PM_TRACELINE_PHYSENTSONLY, 2, -1);
		if (tr == nullptr || tr->fraction >= 1.0f)
			continue; // no ground below
		Vector hit(tr->endpos[0], tr->endpos[1], tr->endpos[2]);
		float dist = (origin - hit).Length() - 12.0f;
		if (dist >= 200.0f)
			continue; // too high above the ground -> no shadow
		if (dist < 0.0f)
			dist = 0.0f;
		const float alpha = 1.0f - dist * 0.005f;

		const float s = 24.0f; // the ground blob half-extent (RE'd; tune vs the original if needed)
		const float z = hit.z + 1.0f; // lift 1u to avoid z-fighting the floor
		Vector c0(hit.x - s, hit.y - s, z);
		Vector c1(hit.x + s, hit.y - s, z);
		Vector c2(hit.x + s, hit.y + s, z);
		Vector c3(hit.x - s, hit.y + s, z);
		tri->Begin(TRI_QUADS);
		tri->Color4f(1.0f, 1.0f, 1.0f, alpha);
		tri->TexCoord2f(0, 0); tri->Vertex3fv((float*)&c0);
		tri->TexCoord2f(1, 0); tri->Vertex3fv((float*)&c1);
		tri->TexCoord2f(1, 1); tri->Vertex3fv((float*)&c2);
		tri->TexCoord2f(0, 1); tri->Vertex3fv((float*)&c3);
		tri->End();
	}
	tri->RenderMode(kRenderNormal);
}

void TS_Attachments_Draw()
{
	// The lasclr_ announcer: on a cl_lasercolor change client.dll sends the server command
	// lasclr_<n> (its weapon bar think, 0x1004ce10); the server rebroadcasts it as PlInfo.
	static int s_lastLaserColor = -1;
	const int lc = (int)gEngfuncs.pfnGetCvarFloat("cl_lasercolor");
	if (lc != s_lastLaserColor)
	{
		s_lastLaserColor = lc;
		char cmd[32];
		sprintf(cmd, "lasclr_%i", lc);
		gEngfuncs.pfnServerCmd(cmd);
	}

	gEngfuncs.pfnAngleVectors(v_angles, s_vForward, s_vRight, s_vUp);
	cl_entity_t* local = gEngfuncs.GetLocalPlayer();
	if (local != nullptr)
	{
		DrawLocalLaser(local);
		DrawOtherLasers(local->index);
		DrawLocalFlashlight(local);
		DrawOtherFlashlights(local->index);
	}
	memset(s_bAdded, 0, sizeof(s_bAdded)); // 0x10056f80
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

// The local laser's projected beam end `which` (0, or 1 for the akimbo second beam) as the weapon bar
// keeps it, -1 when off screen, and the local laser colour. False without the laser on.
bool TS_LocalLaserDot(int which, float& x, float& y, int& r, int& g, int& b)
{
	cl_entity_t* lp = gEngfuncs.GetLocalPlayer();
	if (lp == nullptr || lp->index >= TS_MAXPLAYERS || (g_playerAttach[lp->index] & TS_ATT_LASER) == 0)
		return false;
	x = s_laserDot[which & 1][0];
	y = s_laserDot[which & 1][1];
	LaserColor(g_playerLaserColor[lp->index], r, g, b);
	return true;
}

int TS_LocalLaserRange()
{
	return s_laserRange;
}
