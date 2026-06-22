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

	// Defaults recovered from client.dll RegisterVariable: 150 / 175 / 255.
	s_hud_r = CVAR_CREATE("hud_r", "150", FCVAR_ARCHIVE);
	s_hud_g = CVAR_CREATE("hud_g", "175", FCVAR_ARCHIVE);
	s_hud_b = CVAR_CREATE("hud_b", "255", FCVAR_ARCHIVE);
	CVAR_CREATE("hud_color", "150,175,255", FCVAR_ARCHIVE);
	CVAR_CREATE("cl_realhud", "0", FCVAR_ARCHIVE);
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

	// Fill width is proportional to health across the inset bar (0x80 - 4 px).
	const int fillW = (health * (0x80 - 4)) / 100;
	if (fillW > 0)
	{
		int fr = r, fg = g, fb = b;
		if (health <= 20) // low-health flash, like the original's red branch
		{
			fr = 0xc3;
			fg = 0x2d;
			fb = 0x2d;
		}
		FillRGBA(x + 2, y + 2, fillW, 0x10, fr, fg, fb, 0xb4);
	}
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
