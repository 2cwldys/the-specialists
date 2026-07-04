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
	// read them later. (Stock-HL / engine cvars like cl_bob, cl_thirdperson, gl_fog*,
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

void TSHud_HealthBar(int x, int y, int health)
{
	int r, g, b;
	TSHud_Color(r, g, b);

	// Faint bar background (the original uses alpha 0x1e).
	FillRGBA(x, y, 0x80, 0x13, r, g, b, 0x1e);

	if (health < 0)
		health = 0;
	if (health > 100)
		health = 100;

	// Fill width is proportional to the value across the inset bar (0x80 - 4 px).
	const int fillW = (health * (0x80 - 4)) / 100;
	if (fillW > 0)
	{
		// Colour FADES from the HUD colour (full) toward red (empty) — redder the closer to 0, not a
		// hard switch at a threshold (the original lerps the colour across the bar's progress).
		const float frac = health / 100.0f; // 1 = full, 0 = empty
		const int fr = (int)(0xc3 + (r - 0xc3) * frac);
		const int fg = (int)(0x2d + (g - 0x2d) * frac);
		const int fb = (int)(0x2d + (b - 0x2d) * frac);
		FillRGBA(x + 2, y + 2, fillW, 0x10, fr, fg, fb, 0xb4);
	}
}

int TSHud_DrawString(int x, int y, int maxX, const char* str, int r, int g, int b)
{
	// Faithful port of FUN_10021ce0: draw each glyph with pfnDrawCharacter, advance by
	// the engine's per-character width, and stop before any glyph that would cross maxX.
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
