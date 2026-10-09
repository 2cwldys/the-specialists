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

void TSHud_RegisterCvars()
{
	if (s_hud_r)
		return; // already registered

	// HUD colour — defaults recovered from client.dll RegisterVariable: 150 / 175 / 255.
	s_hud_r = CVAR_CREATE("hud_r", "150", FCVAR_ARCHIVE);
	s_hud_g = CVAR_CREATE("hud_g", "175", FCVAR_ARCHIVE);
	s_hud_b = CVAR_CREATE("hud_b", "255", FCVAR_ARCHIVE);
	CVAR_CREATE("hud_color", "150,175,255", FCVAR_ARCHIVE);

	// The remaining TS-specific client cvars, with the exact defaults recovered from
	// client.dll (reference/API_SURFACE.md). Registering them stops the "Unknown
	// command" spam when the player's TS config sets them, and lets the HUD/effects
	// read them later. (gl_fog* are MOD cvars registered in CHud::Init — the original's
	// slot; cl_thirdperson is registered below. Stock-HL / engine cvars like cl_bob,
	// hud_centerid, sensitivity are already registered by the engine/base client.)
	CVAR_CREATE("cl_realhud", "0", FCVAR_ARCHIVE);
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
	CVAR_CREATE("cl_camtype", "0", FCVAR_ARCHIVE);
	CVAR_CREATE("cl_attackswitch", "0", FCVAR_ARCHIVE);
	CVAR_CREATE("cl_thirdpersonswitch", "0", FCVAR_ARCHIVE);
	// Preferred camera on (re)spawn/unspectate — 1 forces third person, 0 (default) forces
	// first person, anything else keeps the current camera; applied on the spectate edges in
	// ts_spectate.cpp. (Original provenance: input.cpp InitInput @0x2719a, "0" ARCHIVE.)
	CVAR_CREATE("cl_thirdperson", "0", FCVAR_ARCHIVE);
	CVAR_CREATE("cl_wavedetail", "0", FCVAR_ARCHIVE);
	CVAR_CREATE("cl_fx", "1", FCVAR_ARCHIVE);
	CVAR_CREATE("cl_slowfade", "1", FCVAR_ARCHIVE);
	CVAR_CREATE("cl_vsmoothing", "0.05", FCVAR_ARCHIVE);
	CVAR_CREATE("hud_classautokill", "1", FCVAR_ARCHIVE);
	CVAR_CREATE("hud_takesshots", "0", FCVAR_ARCHIVE);
	CVAR_CREATE("nohud", "0", FCVAR_ARCHIVE);
	CVAR_CREATE("r_paintball", "0", FCVAR_ARCHIVE);
}

static int Clamp255(float v)
{
	if (v < 0)
		return 0;
	if (v > 255)
		return 255;
	return (int)v;
}

void TSHud_Color(int& r, int& g, int& b)
{
	r = s_hud_r ? Clamp255(s_hud_r->value) : 150;
	g = s_hud_g ? Clamp255(s_hud_g->value) : 175;
	b = s_hud_b ? Clamp255(s_hud_b->value) : 255;

	// A fully-zero colour would draw invisible text; fall back to the default.
	if ((r | g | b) == 0)
	{
		r = 150;
		g = 175;
		b = 255;
	}
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

// Draw an unsigned integer using sprites/numbers.spr — the TS digit imagemap (10 digits, each ~11px
// wide x 16 tall; rects from sprites/hud.txt number_0..number_9). x,y = top-left of the first digit;
// returns the x past the last digit. Additive, tinted (r,g,b). Used by the dive-reticle distance,
// and (TODO) the health / ammo / powerup counters.
int TSHud_DrawNumber(int x, int y, int value, int r, int g, int b)
{
	HSPRITE numbers = SPR_Load("sprites/numbers.spr");
	if (!numbers)
		return x;
	static const int lefts[10] = { 0, 11, 21, 32, 44, 55, 67, 79, 89, 101 };
	char buf[16];
	int n = value < 0 ? 0 : value;
	int len = 0;
	do { buf[len++] = (char)('0' + n % 10); n /= 10; } while (n && len < 15); // reversed digits
	SPR_Set(numbers, r, g, b);
	for (int i = len - 1; i >= 0; --i)
	{
		int d = buf[i] - '0';
		Rect rc;
		rc.left = lefts[d];
		rc.right = lefts[d] + 11;
		rc.top = 0;
		rc.bottom = 16;
		SPR_DrawAdditive(0, x, y, &rc);
		x += 11;
	}
	return x;
}

// Pixel width of `value` as drawn by TSHud_DrawNumber (digits * 11px).
int TSHud_NumberWidth(int value)
{
	int n = value < 0 ? 0 : value, digits = 1;
	while (n >= 10) { n /= 10; digits++; }
	return digits * 11;
}

// Draw "a / b" with the numbers.spr digit font and a slash between (the bottom weapon-box ammo
// readout — e.g. clip / reserve, "30 / 120"). numbers.spr is digits only, so the slash is a text
// glyph drawn at the digit baseline (the original's separate "divider" sprite isn't in this data
// extraction). Returns x past the last digit.
int TSHud_DrawNumberPair(int x0, int x1, int y, int a, int b_, int r, int g, int b)
{
	// Layout "[  x  /  x  ]": the '/' is fixed-centred in the [x0,x1] span, the clip (a) is
	// centred in the LEFT half, the reserve (b) centred in the RIGHT half — so the slash stays
	// put and each number is balanced within its own half (the original's centred readout).
	const int centerX = (x0 + x1) / 2;
	const int slashW = gHUD.m_scrinfo.charWidths[(unsigned char)'/'];
	const int slashX = centerX - slashW / 2;
	const int slashY = y + (TS_NUMBER_H - gHUD.m_scrinfo.iCharHeight) / 2; // slash on the digit baseline

	// slash, centred
	TSHud_DrawString(slashX, slashY, slashX + slashW + 2, "/", r, g, b);
	// clip: centred in the left half [x0, slashX]
	const int leftMid = (x0 + slashX) / 2;
	TSHud_DrawNumber(leftMid - TSHud_NumberWidth(a) / 2, y, a, r, g, b);
	// reserve: centred in the right half [slashX+slashW, x1]
	const int rightMid = (slashX + slashW + x1) / 2;
	return TSHud_DrawNumber(rightMid - TSHud_NumberWidth(b_) / 2, y, b_, r, g, b);
}
