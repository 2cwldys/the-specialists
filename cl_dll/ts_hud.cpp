/***
*
*	Copyright (c) 1996-2002, Valve LLC. All rights reserved.
*
*	This product contains software technology licensed from Id
*	Software, Inc. ("Id Technology").  Id Technology (c) 1996 Id Software, Inc.
*	All Rights Reserved.
*
*   Use, distribution, and modification of this source code and/or resulting
*   object code is restricted to non-commercial enhancements to products from
*   Valve LLC.  All other use, distribution, or modification is prohibited
*   without written permission from Valve LLC.
*
****/
//
// ts_hud.cpp
//
// Shared TS HUD chrome. The original client draws its HUD text in the player's
// hud_r/hud_g/hud_b colour (RegisterVariable defaults "150"/"175"/"255" — i.e. the
// steel blue, recovered from client.dll) over a dark translucent panel drawn with
// pfnFillRGBA. We register the same cvars and mirror the look.
//

#include "hud.h"
#include "cl_util.h"
#include "ts_hud.h"

static cvar_t* s_hud_r = nullptr;
static cvar_t* s_hud_g = nullptr;
static cvar_t* s_hud_b = nullptr;
static cvar_t* s_realhud = nullptr;
static cvar_t* s_nohud = nullptr;

void TSHud_RegisterCvars()
{
	if (s_hud_r)
		return; // already registered

	// HUD colour — defaults recovered from client.dll RegisterVariable: 150 / 175 / 255.
	s_hud_r = CVAR_CREATE("hud_r", "150", FCVAR_ARCHIVE);
	s_hud_g = CVAR_CREATE("hud_g", "175", FCVAR_ARCHIVE);
	s_hud_b = CVAR_CREATE("hud_b", "255", FCVAR_ARCHIVE);
	CVAR_CREATE("hud_color", "150,175,255", FCVAR_ARCHIVE);

	// The remaining TS-specific client cvars, with client.dll's defaults and flags (its InitInput
	// block 0x10026fdd..0x10027211; the full audit is reference/API_SURFACE.md section 3).
	// The rest of that block is registered where the stock code has it: cl_vsmoothing in input.cpp,
	// cl_camtype and cam_back/right/up in in_camera.cpp, hud_classautokill, hud_takesshots
	// and gl_fog* in CHud::Init, cl_bob and cl_slowfade in V_Init.
	s_realhud = CVAR_CREATE("cl_realhud", "0", FCVAR_ARCHIVE);
	CVAR_CREATE("cl_motionblur", "1", FCVAR_ARCHIVE);
	CVAR_CREATE("cl_gore", "2", FCVAR_ARCHIVE);
	CVAR_CREATE("cl_laser", "0", FCVAR_ARCHIVE);
	CVAR_CREATE("cl_lasercolor", "10", FCVAR_ARCHIVE);
	CVAR_CREATE("cl_solidcross", "2", FCVAR_ARCHIVE);
	CVAR_CREATE("cl_usepowerup", "1", FCVAR_ARCHIVE);
	CVAR_CREATE("cl_buykevlar", "1", FCVAR_ARCHIVE);
	CVAR_CREATE("cl_showequipment", "1", FCVAR_ARCHIVE);
	CVAR_CREATE("cl_shadow", "2", FCVAR_ARCHIVE);
	CVAR_CREATE("cl_thirdtransparency", "1", FCVAR_ARCHIVE);
	CVAR_CREATE("cl_attackswitch", "0", FCVAR_ARCHIVE);
	CVAR_CREATE("cl_thirdpersonswitch", "0", FCVAR_ARCHIVE);
	// Preferred camera on (re)spawn/unspectate — 1 forces third person, 0 (default) forces
	// first person, anything else keeps the current camera; applied on the spectate edges in
	// ts_spectate.cpp. (Original provenance: input.cpp InitInput @0x2719a, "0" ARCHIVE.)
	CVAR_CREATE("cl_thirdperson", "0", FCVAR_ARCHIVE);
	CVAR_CREATE("cl_wavedetail", "0", FCVAR_ARCHIVE);
	CVAR_CREATE("cl_fx", "1", FCVAR_ARCHIVE);
	s_nohud = CVAR_CREATE("nohud", "0", 0); // flags 0, as client.dll (0x100270e0)
	CVAR_CREATE("r_paintball", "0", FCVAR_ARCHIVE);
}

void TSHud_Color(int& r, int& g, int& b)
{
	r = s_hud_r ? (int)s_hud_r->value : 150;
	g = s_hud_g ? (int)s_hud_g->value : 175;
	b = s_hud_b ? (int)s_hud_b->value : 255;
}

bool TSHud_RealHud()
{
	return s_realhud != nullptr && s_realhud->value != 0.0f;
}

bool TSHud_NoHud()
{
	return s_nohud != nullptr && s_nohud->value != 0.0f;
}

int g_iTSHealth = 100;
int g_iTSActiveWeapon = -1;

// The melee bar (client.dll @0x1004d5f0), from the bar value (this+0x3700, -100..100) and the raw
// hud_r/hud_g/hud_b values (truncated, unclamped). At 0.01 or below only the frame is drawn, in
// (0xc3, 0x2d, 0x2d); above, the fill is (int)(bar x 1.28) - 2 wide in (r x 128/255, w x g/255,
// w x b/255) at alpha 255 - w, then the 128 x 19 frame in the colour at alpha 0x1e.
void TSHud_MeleeBar(int x, int y, float bar)
{
	int r = (int)(s_hud_r ? s_hud_r->value : 150.0f);
	int g = (int)(s_hud_g ? s_hud_g->value : 175.0f);
	int b = (int)(s_hud_b ? s_hud_b->value : 255.0f);

	if (bar <= 0.01f)
	{
		r = 0xc3;
		g = 0x2d;
		b = 0x2d;
	}
	else
	{
		const int w = (int)(bar * 1.28f);
		FillRGBA(x + 2, y + 2, w - 2, 0x10, (r * 0x80) / 0xff, (w * g) / 0xff, (w * b) / 0xff, 0xff - w);
	}
	FillRGBA(x, y, 0x80, 0x13, r, g, b, 0x1e);
}

int TSHud_DrawString(int x, int y, int maxX, const char* str, int r, int g, int b)
{
	// Faithful port of FUN_10021ce0: a per-glyph loop calling the engine's DrawCharacter
	// (DAT_100d5b90), advancing by charWidths[c], stopping before any glyph crossing maxX.
	// (Confirmed by RE: the original uses DrawCharacter, NOT DrawConsoleString — the console
	// string path in xash draws a drop-shadow the original doesn't have. Any residual BOLDNESS
	// vs the original is the xash-vs-GoldSrc engine HUD font, not this code.)
	for (const char* p = str; *p && *p != '\n'; p++)
	{
		const unsigned char c = (unsigned char)*p;
		const int next = x + gHUD.m_scrinfo.charWidths[c];
		if (next > maxX)
			break;
		gEngfuncs.pfnDrawCharacter(x, y, c, r, g, b);
		x = next;
	}
	return x;
}

void TSHud_Panel(int x, int y, int w, int h)
{
	// The original (FUN_1004d7e0) backs each HUD strip with a FillRGBA of the HUD
	// colour itself at alpha 0x0f (~6%) — a very faint tint, not an opaque panel.
	// See reference/HUD_DRAWCHAIN.md.
	int r, g, b;
	TSHud_Color(r, g, b);
	FillRGBA(x, y, w, h, r, g, b, 0x0f);
}

// client.dll 0x1004c290: the stock CHud::DrawHudNumber widened to five digits, drawing the hud.txt
// number_0..number_9 sprites. A value of 0 or less draws a single 0 only with TSHN_DRAWZERO. Each
// TSHN_nDIGITS flag reserves the width of the leading digits a smaller value lacks (TSHN_5DIGITS
// pads ten thousands, thousands, hundreds and tens; TSHN_4DIGITS from thousands; and so on), and
// TSHN_ZEROPAD draws those reserved digits as zeros. Returns the x past the last digit.
int TSHud_DrawNumber(int x, int y, int flags, int value, int r, int g, int b)
{
	const int n0 = gHUD.m_HUD_number_0;
	if (n0 < 0)
		return x;
	const int w = gHUD.GetSpriteRect(n0).right - gHUD.GetSpriteRect(n0).left;
	auto digit = [&](int k) {
		SPR_Set(gHUD.GetSprite(n0 + k), r, g, b);
		SPR_DrawAdditive(0, x, y, &gHUD.GetSpriteRect(n0 + k));
		x += w;
	};
	auto pad = [&](int mask) {
		if ((flags & mask) == 0)
			return;
		if ((flags & TSHN_ZEROPAD) != 0)
			digit(0);
		else
			x += w;
	};

	if (value > 0)
	{
		if (value >= 10000)
			digit(value / 10000);
		else
			pad(TSHN_5DIGITS);
		if (value >= 1000)
			digit((value % 10000) / 1000);
		else
			pad(TSHN_5DIGITS | TSHN_4DIGITS);
		if (value >= 100)
			digit((value % 1000) / 100);
		else
			pad(TSHN_4DIGITS | TSHN_3DIGITS);
		if (value >= 10)
			digit((value % 100) / 10);
		else
			pad(TSHN_3DIGITS | TSHN_2DIGITS);
		digit(value % 10);
		return x;
	}
	if ((flags & TSHN_DRAWZERO) == 0)
		return x;
	pad(TSHN_5DIGITS);
	pad(TSHN_5DIGITS | TSHN_4DIGITS);
	pad(TSHN_4DIGITS | TSHN_3DIGITS);
	pad(TSHN_3DIGITS | TSHN_2DIGITS);
	digit(0);
	return x;
}

// Pixel width of `value` as TSHud_DrawNumber draws it without padding flags.
int TSHud_NumberWidth(int value)
{
	const int n0 = gHUD.m_HUD_number_0;
	if (n0 < 0)
		return 0;
	int n = value < 0 ? 0 : value, digits = 1;
	while (n >= 10) { n /= 10; digits++; }
	return digits * (gHUD.GetSpriteRect(n0).right - gHUD.GetSpriteRect(n0).left);
}
