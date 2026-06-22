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

// Forward a bare command to the server (the gameplay handler is server-side).
static void Fwd(const char* cmd)
{
	char buf[64];
	sprintf(buf, "%s\n", cmd);
	ServerCmd(buf);
}

// TS alt-fire / action inputs — forwarded to the server.
static void IN_Alt1Down() { Fwd("+alt1"); }
static void IN_Alt1Up() { Fwd("-alt1"); }
static void IN_Alt2Down() { Fwd("+alt2"); }
static void IN_Alt2Up() { Fwd("-alt2"); }
static void IN_BreakDown() { Fwd("+break"); }
static void IN_BreakUp() { Fwd("-break"); }
static void TS_Special() { Fwd("special"); }
static void TS_Buy() { Fwd("buy"); }

// Pure-client UI commands (no server side) — stubs so the binds don't error. The
// command/class menu + server browser are VGUI features still to reconstruct.
static void TS_NoOp() {}

void TSRegisterCommands()
{
	gEngfuncs.pfnAddCommand("+alt1", IN_Alt1Down);
	gEngfuncs.pfnAddCommand("-alt1", IN_Alt1Up);
	gEngfuncs.pfnAddCommand("+alt2", IN_Alt2Down);
	gEngfuncs.pfnAddCommand("-alt2", IN_Alt2Up);
	gEngfuncs.pfnAddCommand("+break", IN_BreakDown);
	gEngfuncs.pfnAddCommand("-break", IN_BreakUp);
	gEngfuncs.pfnAddCommand("special", TS_Special);
	gEngfuncs.pfnAddCommand("buy", TS_Buy);
	gEngfuncs.pfnAddCommand("+commandmenu", TS_NoOp);
	gEngfuncs.pfnAddCommand("-commandmenu", TS_NoOp);
	gEngfuncs.pfnAddCommand("ForceCloseCommandMenu", TS_NoOp);
	gEngfuncs.pfnAddCommand("togglebrowser", TS_NoOp);
}
