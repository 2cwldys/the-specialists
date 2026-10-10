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
// ts_hud.h
//
// Shared helpers for the custom TS HUD: the player-chosen HUD colour (the
// hud_r/hud_g/hud_b cvars — defaults 150/175/255, recovered from client.dll's
// RegisterVariable calls) and the translucent dark panel the TS HUD draws behind
// each cluster (pfnFillRGBA, the same call the original uses).
//

#pragma once

// Register the TS HUD cvars (hud_r/g/b/color, cl_realhud) with their faithful
// defaults. Idempotent; call from HUD init.
void TSHud_RegisterCvars();

// cl_realhud: true when the cvar is not 0. client.dll tests `cl_realhud->value == 0.0f` at the top of
// three draws (the status panel 0x1004a3e0, the weapon panel 0x1004d7e0, the crosshair 0x1004e9f0)
// and draws none of them otherwise.
bool TSHud_RealHud();

// nohud: true when the cvar is not 0. client.dll has no global HUD gate for it: it tests
// `nohud->value == 0.0f` in the death notices (0x1001b020), the status draw (0x10049f06: round timer,
// messages, flags, team dots, aim names, status panel, powerup label, objectives), the weapon bar's
// Draw after the laser dot (0x1004d3f9: buy menu, slots, weapon panel, crosshair) and the spectator
// panel (0x1006389d, 0x10063912, 0x10064984). Chat, menus, the scoreboard and text messages stay.
bool TSHud_NoHud();

// True while client.dll's status draw (0x10049ea0) skips the player's own parts: dead for the HUD
// (CHudTSStatus::IsDead) or spectating.
bool TSHud_PlayerPartsHidden();

// The HUD colour: hud_r/hud_g/hud_b truncated to int, as client.dll reads them (no clamp). 150/175/255
// before the cvars exist.
void TSHud_Color(int& r, int& g, int& b);

// Draw the translucent dark panel behind a HUD cluster (x,y,w,h in pixels).
void TSHud_Panel(int x, int y, int w, int h);

// Draw a coloured string clipped to maxX, per-character — a faithful port of the
// original FUN_10021ce0 (pfnDrawCharacter advancing by SCREENINFO charWidths, stops
// before any glyph that would cross maxX). Returns the ending x.
int TSHud_DrawString(int x, int y, int maxX, const char* str, int r, int g, int b);

// The HUD number (client.dll 0x1004c290) in the hud.txt number sprites; flags below. x,y top-left;
// returns the x past the last digit.
#define TSHN_DRAWZERO 0x01 // draw "0" for a value of 0 or less (else nothing)
#define TSHN_2DIGITS 0x02  // reserve the width of two digits
#define TSHN_3DIGITS 0x04
#define TSHN_4DIGITS 0x08
#define TSHN_5DIGITS 0x10
#define TSHN_ZEROPAD 0x20  // draw the reserved leading digits as zeros
int TSHud_DrawNumber(int x, int y, int flags, int value, int r, int g, int b);

// Pixel width of `value` drawn by TSHud_DrawNumber with no padding flags.
int TSHud_NumberWidth(int value);

// Height of a number sprite (hud.txt number_0..9 are 16 tall), for vertical centring against text.
#define TS_NUMBER_H 16

// The melee bar (FUN_1004d5f0): a 0x80(128)-wide frame at (x,y) at alpha 0x1e and a fill from
// the bar value, at the bottom of the weapon/ammo box (ts_hud.cpp has the colours).
void TSHud_MeleeBar(int x, int y, float bar);

// Last health value received (TSHealth). Set by CHudTSStatus, read by the ammo HUD so
// the health bar can be drawn stacked above the ammo box exactly as the original does.
extern int g_iTSHealth;

// Active weapon tsweapon_t (from WeaponInfo). Set by the ammo HUD, read by the weapon-
// select bar to highlight the held weapon's slot.
extern int g_iTSActiveWeapon;
