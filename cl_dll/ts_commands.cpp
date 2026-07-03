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
// ts_commands.cpp
//
// The Specialists client console commands. The original client registers ~12 TS
// commands (reference/API_SURFACE.md). Registering them stops the "Unknown command"
// spam and lets the player's TS keybinds work. The input/action commands forward to
// the server (where the gameplay handler lives / will live); the pure-client UI
// commands are stubs for now (the menus are a VGUI cl_dll feature still to build).
//

#include "hud.h"
#include "cl_util.h"

// +alt2 (coldcock) is a BUTTON, not a server command: the original client's +alt2 sets the
// IN_CANCEL bit (0x40) in the usercmd, and the SERVER's base ItemPostFrame dispatches
// AlternateAttack off `pev->button & IN_CANCEL` (@0x903d4 — the original server has no
// "+alt2" ClientCommand at all; ours was removed for parity). input.cpp already maps the
// `in_cancel` state to the IN_CANCEL bit in CL_ButtonBits.
extern bool in_cancel; // cl_dll/input.cpp
static void IN_Alt2Down() { in_cancel = true; }
static void IN_Alt2Up() { in_cancel = false; }

// `buy` (bound to `b` in the original) opens/closes the buy/config menu — the REAL HUD-drawn cascade
// (CHudTSBuyMenu), driven by number keys. Only in the spectate-wait (buy) phase, so it can't be
// opened during live play. (The server `buy` handler is a no-op; the menu is client-side, cont.69l.)
static void TS_Buy()
{
	if (gHUD.m_TSBuyMenu.IsUp())
		gHUD.m_TSBuyMenu.Close();
	else if (gHUD.m_Spectate.IsActive())
		gHUD.m_TSBuyMenu.Open();
}

// Pure-client UI command stub (the server browser is a VGUI feature still to build).
static void TS_NoOp() {}

void TSRegisterCommands()
{
	// NOTE: xash3d-fwgs provides +alt1/-alt1, +break/-break, +commandmenu/-commandmenu,
	// ForceCloseCommandMenu and `special` as engine built-ins (re-registering them errors,
	// and they already forward to the server, which accepts them in dlls/client.cpp). On
	// stock GoldSrc the client.dll registers them; here we register only the TS commands
	// the engine lacks. See reference/API_SURFACE.md.
	gEngfuncs.pfnAddCommand("+alt2", IN_Alt2Down);
	gEngfuncs.pfnAddCommand("-alt2", IN_Alt2Up);
	gEngfuncs.pfnAddCommand("buy", TS_Buy);
	gEngfuncs.pfnAddCommand("togglebrowser", TS_NoOp);
}
