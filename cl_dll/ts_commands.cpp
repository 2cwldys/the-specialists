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
#include "vgui_TeamFortressViewport.h" // gViewPort->ShowTSBuyMenu (reopen the buy menu)

// Forward a bare command to the server (the gameplay handler is server-side).
static void Fwd(const char* cmd)
{
	char buf[64];
	sprintf(buf, "%s\n", cmd);
	ServerCmd(buf);
}

// TS alt-fire / action inputs — forwarded to the server.
static void IN_Alt2Down() { Fwd("+alt2"); }
static void IN_Alt2Up() { Fwd("-alt2"); }

// `buy` (bound to `b` in the original) (re)opens the buy/config menu. The menu is the client vgui
// panel (CTSBuyMenuPanel), so reopen it directly — only while in the spectate-wait (buy) phase, so
// it can't be opened during live play. (The server `buy` handler is now a no-op; cont.69g.)
static void TS_Buy()
{
	if (gViewPort && gHUD.m_Spectate.IsActive())
		gViewPort->ShowTSBuyMenu(true);
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
