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

// The current HUD text colour (hud_r/g/b, clamped 0..255). Defaults to the TS steel
// blue 150/175/255 if the cvars are missing.
void TSHud_Color(int& r, int& g, int& b);

// Draw the translucent dark panel behind a HUD cluster (x,y,w,h in pixels).
void TSHud_Panel(int x, int y, int w, int h);

// Draw a coloured string clipped to maxX, per-character — a faithful port of the
// original FUN_10021ce0 (pfnDrawCharacter advancing by SCREENINFO charWidths, stops
// before any glyph that would cross maxX). Returns the ending x.
int TSHud_DrawString(int x, int y, int maxX, const char* str, int r, int g, int b);

// The TS health bar (FUN_1004d5f0): a 0x80(128)-wide bar at (x,y) — faint background
// (alpha 0x1e) + a fill proportional to health, in the HUD colour (red when low). The
// original draws it bottom-right, stacked with the weapon/ammo box.
void TSHud_HealthBar(int x, int y, int health);

// Last health value received (TSHealth). Set by CHudTSStatus, read by the ammo HUD so
// the health bar can be drawn stacked above the ammo box exactly as the original does.
extern int g_iTSHealth;

// Active weapon tsweapon_t (from WeaponInfo). Set by the ammo HUD, read by the weapon-
// select bar to highlight the held weapon's slot.
extern int g_iTSActiveWeapon;
