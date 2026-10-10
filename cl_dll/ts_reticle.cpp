/***
*  ts_reticle.cpp — the crosshair, the dive arc, the laser dots, the bomb wires and the aim names,
*  ported from client.dll's weapon bar (0x100d0ca0): the laser dots 0x1004e6f0, the crosshair
*  0x1004e9f0 with the dive arc 0x1004ef50 and the laser range number, the wires 0x1004d550, and the
*  aim trace with the aimed player's name 0x1004acc0 (run from the status draw). Sprites from the
*  weapon bar's load block at 0x1004cb9c: cross_e +0x14c, cross2 +0x150, cross_t +0x154, divestept,
*  divestepc, divestepb +0x158..0x160, the wires +0x36e4..0x36f8.
***/
#include "hud.h"
#include "cl_util.h"
#include "parsemsg.h"   // NextStunt (the stunt-meter byte)
#include "cl_entity.h"  // cl_entity_t (GetLocalPlayer)
#include "pmtrace.h"
#include "pm_defs.h"    // PM_STUDIO_BOX
#include "triangleapi.h" // pTriAPI->WorldToScreen (3rd-person reticle projection)
#include "ts_hud.h"
#include "event_api.h"
#include "ts_attachments.h" // the local laser pass's dots and range
#include "ts_reticle.h"
#include "player_info.h" // g_PlayerExtraInfo, g_PlayerInfoList
#include "com_model.h"
#include "r_studioint.h" // IEngineStudio.IsHardware

#include <math.h>

extern "C" int CL_IsThirdPerson(); // in_camera.cpp (engine export, C linkage)

DECLARE_MESSAGE(m_TSReticle, NextStunt)

extern Vector v_origin, v_angles;
extern Vector v_aimeye;

// 0 nothing aimed, 1 a teammate, 2 an enemy (client.dll 0x1018a60c). Read by the crosshair and by
// V_CalcNormalRefdef's third-person fade.
int g_iTSAimTarget = 0;
// The aim trace's end (weapon bar +0x38): the third-person crosshair and laser dot sit on it.
static Vector s_vecAimPoint;

// The eye the aim trace and the laser range use (0x10057270): the refdef eye in first person, the
// player's origin plus the view height in third person (0x10068b2b).
Vector TS_AimEye()
{
	if (0 != CL_IsThirdPerson())
	{
		cl_entity_t* lp = gEngfuncs.GetLocalPlayer();
		Vector vh;
		gEngfuncs.pEventAPI->EV_LocalPlayerViewheight(vh);
		if (lp)
			return lp->origin + vh;
	}
	return v_aimeye;
}

// World point to HUD pixels (0x100579c0): false behind the view.
static bool TS_ToScreen(const Vector& point, int& x, int& y)
{
	float screen[3];
	if (0 != gEngfuncs.pTriAPI->WorldToScreen((float*)&point, screen))
		return false;
	x = (int)((1.0f + screen[0]) * ScreenWidth / 2);
	y = (int)((1.0f - screen[1]) * ScreenHeight / 2);
	return true;
}

// client.dll 0x1004acc0, run by the status draw while nohud is 0 and the player is alive and playing:
// a player trace (studio boxes, the local player left out) 8192 units along the view from the eye.
// Its end is the third-person aim point. A player hit is an enemy when the GameMode byte is 0 or his
// team differs; his name is drawn at (ScreenWidth / 2, ScreenHeight / 2 - 40), red for an enemy, green
// for a teammate.
void TS_AimNames()
{
	cl_entity_t* lp = gEngfuncs.GetLocalPlayer();
	if (!lp)
		return;
	const int local = lp->index;
	gEngfuncs.pEventAPI->EV_PushPMStates();
	gEngfuncs.pEventAPI->EV_SetUpPlayerPrediction(0, 1);
	gEngfuncs.pEventAPI->EV_SetSolidPlayers(local - 1);
	gEngfuncs.pEventAPI->EV_SetTraceHull(2);
	Vector fwd;
	gEngfuncs.pfnAngleVectors(v_angles, fwd, NULL, NULL);
	const Vector eye = TS_AimEye();
	const Vector end = eye + fwd * 8192.0f;
	pmtrace_t tr;
	gEngfuncs.pEventAPI->EV_PlayerTrace((float*)&eye, (float*)&end, PM_STUDIO_BOX, local, &tr);
	s_vecAimPoint = tr.endpos;
	cl_entity_t* hit = gEngfuncs.GetEntityByIndex(gEngfuncs.pEventAPI->EV_IndexFromTrace(&tr));
	g_iTSAimTarget = 0;
	if (hit != nullptr && 0 != hit->player)
	{
		const int idx = hit->index;
		const char* name = g_PlayerInfoList[idx].name;
		const bool enemy = gHUD.m_iTeamplay == 0 || g_PlayerExtraInfo[idx].teamnumber != g_PlayerExtraInfo[local].teamnumber;
		if (name != nullptr)
		{
			if (enemy)
				TSHud_DrawString(ScreenWidth / 2, ScreenHeight / 2 - 0x28, ScreenWidth, name, 0xc3, 0x2d, 0x2d);
			else
				TSHud_DrawString(ScreenWidth / 2, ScreenHeight / 2 - 0x28, ScreenWidth, name, 0x2d, 0xc3, 0x2d);
			g_iTSAimTarget = enemy ? 2 : 1;
		}
	}
	gEngfuncs.pEventAPI->EV_PopPMStates();
}

bool CHudTSReticle::Init()
{
	m_flStuntTime = 0;
	// client.dll hooks NextStunt on the weapon bar (0x1004fac0); its consumer, the dive arc, is here.
	HOOK_MESSAGE(NextStunt);
	gHUD.AddHudElem(this);
	m_iFlags |= HUD_ACTIVE;
	return true;
}

// NextStunt (0x1004fac0): the byte x 0.1 s.
bool CHudTSReticle::MsgFunc_NextStunt(const char* pszName, int iSize, void* pbuf)
{
	BEGIN_READ(pbuf, iSize);
	m_flStuntTime = READ_BYTE() * 0.1f;
	m_iFlags |= HUD_ACTIVE;
	return true;
}

// The weapon bar's think (0x1004ce36) runs the stunt time down by the frame time times the slow
// motion factor (at least 0.01), to 0.
void CHudTSReticle::Think()
{
	extern float g_flClientSlow;
	const float slow = g_flClientSlow < 0.01f ? 0.01f : g_flClientSlow;
	m_flStuntTime -= (float)(gHUD.m_flTimeDelta * slow);
	if (m_flStuntTime <= 0.0f)
		m_flStuntTime = 0;
}

bool CHudTSReticle::VidInit()
{
	m_hCrossE = SPR_Load("sprites/cross_e.spr");
	m_hCross2 = SPR_Load("sprites/cross2.spr");
	m_hCrossT = SPR_Load("sprites/cross_t.spr");
	m_hDive[0] = SPR_Load("sprites/player/divestept.spr");
	m_hDive[1] = SPR_Load("sprites/player/divestepc.spr");
	m_hDive[2] = SPR_Load("sprites/player/divestepb.spr");
	m_hWire[0] = SPR_Load("sprites/wires/rwire.spr");
	m_hWire[1] = SPR_Load("sprites/wires/gwire.spr");
	m_hWire[2] = SPR_Load("sprites/wires/bwire.spr");
	m_hWireCut[0] = SPR_Load("sprites/wires/rwirecutted.spr");
	m_hWireCut[1] = SPR_Load("sprites/wires/gwirecutted.spr");
	m_hWireCut[2] = SPR_Load("sprites/wires/bwirecutted.spr");
	m_flStuntTime = 0;
	m_iLaserRange = 0;
	extern void TS_ScopeVidInit();
	TS_ScopeVidInit();
	return true;
}

// The laser dots (0x1004e6f0), drawn by the weapon bar before any other test. With the local player's
// laser on (ActItems bit 2), 2 x 2 dots in his laser colour, one per beam end the laser pass projected
// (none for an end off screen). The pass's dot sits at the beam end, (x - 1, y - 1); while the predicted
// gun's active attachments are set and it may attack (g_iTSLaserSettled) and cl_laser is below 1 it
// sits at the crosshair centre instead (the screen centre, or the projected aim point in third person),
// jittered by jx, jy of -1..1 pixel: at (cx - 1 + jx, cy - 1 + jy), or with the akimbo bit at
// (cx + 3 + jx, cy - 1 + jy) for the first gun and (cx - 5 + jy, cy - 1 + jx) for the second.
void CHudTSReticle::DrawLaserDots()
{
	int r, g, b;
	float x0, y0, x1, y1;
	if (!TS_LocalLaserDot(0, x0, y0, r, g, b))
		return;
	TS_LocalLaserDot(1, x1, y1, r, g, b);
	const int jx = gEngfuncs.pfnRandomLong(-10, 10) / 10;
	const int jy = gEngfuncs.pfnRandomLong(-10, 10) / 10;
	int cx = ScreenWidth / 2, cy = ScreenHeight / 2;
	if (0 != CL_IsThirdPerson() && !TS_ToScreen(s_vecAimPoint, cx, cy))
		cx = cy = -1;
	static cvar_t* s_cl_laser = nullptr;
	if (s_cl_laser == nullptr)
		s_cl_laser = gEngfuncs.pfnGetCvarPointer("cl_laser");
	extern int g_iTSLaserSettled;
	const bool centred = g_iTSLaserSettled != 0 && s_cl_laser != nullptr && s_cl_laser->value < 1.0f;
	if ((TS_LocalActiveAttach() & 0x40) == 0)
	{
		if (x0 == -1.0f || y0 == -1.0f)
			return;
		if (centred)
			FillRGBA(cx - 1 + jx, cy - 1 + jy, 2, 2, r, g, b, 0xff);
		else
			FillRGBA((int)(x0 - 1.0f), (int)(y0 - 1.0f), 2, 2, r, g, b, 0xff);
		return;
	}
	if (x0 != -1.0f && y0 != -1.0f)
	{
		if (centred)
			FillRGBA(cx + 3 + jx, cy - 1 + jy, 2, 2, r, g, b, 0xff);
		else
			FillRGBA((int)(x0 - 1.0f), (int)(y0 - 1.0f), 2, 2, r, g, b, 0xff);
	}
	if (x1 == -1.0f || y1 == -1.0f)
		return;
	if (centred)
		FillRGBA(cx - 5 + jy, cy - 1 + jx, 2, 2, r, g, b, 0xff);
	else
		FillRGBA((int)(x1 - 1.0f), (int)(y1 - 1.0f), 2, 2, r, g, b, 0xff);
}

// The bomb wires (0x1004d550), in place of the crosshair while any Wires byte is set: three sprites
// from (ScreenWidth / 2 - 48, ScreenHeight / 2 - 128), 32 apart, white, the whole wire for a set byte
// and the cut wire for 0.
void CHudTSReticle::DrawWires(const unsigned char* wires)
{
	int x = ScreenWidth / 2 - 0x30;
	const int y = ScreenHeight / 2 - 0x80;
	for (int i = 0; i < 3; i++, x += 0x20)
	{
		const HSPRITE h = wires[i] != 0 ? m_hWire[i] : m_hWireCut[i];
		SPR_Set(h, 0xff, 0xff, 0xff);
		SPR_DrawHoles(0, x, y, nullptr);
	}
}

// The dive arc (0x1004ef50) around (cx, cy): the three steps (top, centre, bottom) at (cx - 40, cy - 40),
// each in (195, 45, 45) when the player is too loaded for that dive height (more than 41, 51, 61 slots
// used), else in the base colour, full on the step his view pitch picks (below -10 the top, up to 10 the
// centre, else the bottom) and half on the others. The base colour is the HUD colour, (195, 45, 45)
// while a stunt is pending (stunt time 0.01 or more). Then a 2 x 2 dot in the base colour at alpha 100,
// 32 pixels from (cx, cy) at the angle three times the pitch, held to -90..90.
void CHudTSReticle::DrawArc(int cx, int cy)
{
	int r, g, b;
	if (m_flStuntTime >= 0.01f)
	{
		r = 0xc3;
		g = 0x2d;
		b = 0x2d;
	}
	else
		TSHud_Color(r, g, b);
	const float pitch = gHUD.m_vecAngles[0];
	const int aimStep = pitch < -10.0f ? 0 : (pitch <= 10.0f ? 1 : 2);
	const int used = 0x51 - gHUD.m_TSStatus.FreeSlots();
	static const int s_limit[3] = {0x2a, 0x34, 0x3e};
	for (int i = 0; i < 3; i++)
	{
		if (used >= s_limit[i])
			SPR_Set(m_hDive[i], 0xc3, 0x2d, 0x2d);
		else
		{
			const double k = (i == aimStep) ? 1.0 : 0.5;
			SPR_Set(m_hDive[i], (int)(r * k), (int)(g * k), (int)(b * k));
		}
		SPR_DrawAdditive(0, cx - 0x28, cy - 0x28, nullptr);
	}
	float a = pitch * 3.0f;
	if (a > 90.0f)
		a = 90.0f;
	if (a < -90.0f)
		a = -90.0f;
	const double rad = a * 0.017453292519943295;
	FillRGBA((int)(cx - cos(rad) * 32.0), (int)(sin(rad) * 32.0 + cy), 2, 2, r, g, b, 100);
}

// The crosshair (0x1004e9f0), after the weapon bar Draw's nohud, dead and spectating tests, the buy
// menu and the wires; it returns at once unless cl_realhud is 0. Zoomed means a field of view under 40.
// Centre: the screen centre in first person, where the arc is drawn unless zoomed; the projected aim
// point in third person, never zoomed there. Kung fu, the knives, the katana and the M61 stop after
// the arc. The colour: (45, 195, 45) on a teammate, (195, 45, 45) on an enemy, else the HUD colour.
// Not zoomed, or on a teammate: the gap is 22 (12 with the laser on, for weapons other than kung fu,
// the knife and the M61) plus 160 x the predicted spread; on a teammate cross_t in 0.7 of the colour,
// else four 5 x 2 bars at alpha 75 that far out, then with the hardware renderer and cl_solidcross
// above 0 cross2 with holes (in black above 1), otherwise cross_e additive in 0.75 of the colour, all
// 64 x 64 on the centre. With the laser on, the laser range one digit wide at (cx + 28, cy - half the
// number height, 8 higher when zoomed).
void CHudTSReticle::DrawCrosshair()
{
	if (TSHud_RealHud())
		return;
	bool zoomed = gHUD.m_iFOV < 40;
	int cx, cy;
	if (0 == CL_IsThirdPerson())
	{
		cx = ScreenWidth / 2;
		cy = ScreenHeight / 2;
		if (!zoomed)
			DrawArc(cx, cy);
	}
	else
	{
		if (!TS_ToScreen(s_vecAimPoint, cx, cy))
			cx = cy = -1;
		zoomed = false;
		DrawArc(cx, cy);
	}

	const int type = g_iTSActiveWeapon;
	if (type == 0 || type == 25 || type == 35 || type == 34 || type == 24)
		return;

	int r, g, b;
	if (g_iTSAimTarget == 1)
	{
		r = 0x2d;
		g = 0xc3;
		b = 0x2d;
	}
	else if (g_iTSAimTarget == 2)
	{
		r = 0xc3;
		g = 0x2d;
		b = 0x2d;
	}
	else
		TSHud_Color(r, g, b);

	const bool laser = (TS_LocalActiveAttach() & 2) != 0;
	if (!zoomed || g_iTSAimTarget == 1)
	{
		int gap = 0x16;
		if (type != 0 && type != 25 && type != 24)
		{
			extern float g_flTSSpreadAdd;
			gap = (int)(g_flTSSpreadAdd * 160.0f + (laser ? 0x0c : 0x16));
		}
		if (g_iTSAimTarget == 1)
		{
			SPR_Set(m_hCrossT, (int)(r * 0.7), (int)(g * 0.7), (int)(b * 0.7));
			SPR_DrawAdditive(0, cx - 0x20, cy - 0x20, nullptr);
		}
		else
		{
			FillRGBA(cx - gap, cy - 1, 5, 2, r, g, b, 0x4b);
			FillRGBA(cx + gap, cy - 1, -5, 2, r, g, b, 0x4b);
			FillRGBA(cx - 1, cy - gap, 2, 5, r, g, b, 0x4b);
			FillRGBA(cx - 1, cy + gap, 2, -5, r, g, b, 0x4b);
			static cvar_t* s_solidcross = nullptr;
			if (s_solidcross == nullptr)
				s_solidcross = gEngfuncs.pfnGetCvarPointer("cl_solidcross");
			extern engine_studio_api_t IEngineStudio;
			if (0 != IEngineStudio.IsHardware() && s_solidcross != nullptr && s_solidcross->value > 0.0f)
			{
				const bool black = s_solidcross->value > 1.0f;
				SPR_Set(m_hCross2, black ? 0 : r, black ? 0 : g, black ? 0 : b);
				SPR_DrawHoles(0, cx - 0x20, cy - 0x20, nullptr);
			}
			else
			{
				SPR_Set(m_hCrossE, (int)(r * 0.75), (int)(g * 0.75), (int)(b * 0.75));
				SPR_DrawAdditive(0, cx - 0x20, cy - 0x20, nullptr);
			}
		}
	}
	if (laser)
	{
		const int fontH = gHUD.GetSpriteRect(gHUD.m_HUD_number_0).bottom - gHUD.GetSpriteRect(gHUD.m_HUD_number_0).top;
		TSHud_DrawNumber(cx + 0x1c, cy - fontH / 2 - (zoomed ? 8 : 0), TSHN_DRAWZERO, m_iLaserRange, r, g, b);
	}
}

bool CHudTSReticle::Draw(float flTime)
{
	if (gHUD.m_iHideHUDDisplay & HIDEHUD_ALL)
		return true;

	// The scope: reTS draws quarterscope.spr in screen space with black side bars and a pinned dot
	// (Steven's sizing, 7m); client.dll draws it as a world quad in front of the eye (0x10054510).
	{
		extern bool TS_ScopedIn();
		extern void TS_DrawScopeOverlayHUD();
		if (TS_ScopedIn())
		{
			TS_DrawScopeOverlayHUD();
			float lx, ly;
			int lr2, lg2, lb2;
			if (TS_LocalLaserDot(0, lx, ly, lr2, lg2, lb2))
				FillRGBA(ScreenWidth / 2 - 1, ScreenHeight / 2 - 1, 3, 3, lr2, lg2, lb2, 235);
			return true;
		}
	}

	// The laser range (weapon bar +0x18), left by the laser pass (0x1003d170).
	m_iLaserRange = TS_LocalLaserRange();

	DrawLaserDots();
	if (TSHud_NoHud() || TSHud_PlayerPartsHidden())
		return true;
	if (gHUD.m_TSBuyMenu.IsUp())
		return true;
	const unsigned char* wires = gHUD.m_TSWeaponBar.m_wires;
	if (wires[0] != 0 || wires[1] != 0 || wires[2] != 0)
		DrawWires(wires);
	else
		DrawCrosshair();
	return true;
}
