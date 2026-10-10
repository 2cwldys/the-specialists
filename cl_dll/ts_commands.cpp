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
// The TS client commands that have no stock home: +alt2/-alt2, buy and togglebrowser
// (client.dll's command list: reference/API_SURFACE.md). The stock ones TS keeps (+alt1, +break,
// +commandmenu, ForceCloseCommandMenu, special, specmode) are registered where the SDK has them.
//

#include "hud.h"
#include "cl_util.h"
#include "kbutton.h"

// +alt2 (coldcock) is a BUTTON, not a server command: the original client's +alt2 sets the
// IN_CANCEL bit (0x40) in the usercmd, and the SERVER's base ItemPostFrame dispatches
// AlternateAttack off `pev->button & IN_CANCEL` (@0x903d4 — the original server has no
// "+alt2" ClientCommand at all; ours was removed for parity). input.cpp already maps the
// `in_cancel` state to the IN_CANCEL bit in CL_ButtonBits.
// With cl_attackswitch on, +alt2 drives attack2 instead (TS_AttackSwitched, input.cpp).
extern bool in_cancel; // cl_dll/input.cpp
extern kbutton_t in_attack2;
bool TS_AttackSwitched();
void KeyDown(kbutton_t* b);
void KeyUp(kbutton_t* b);
extern bool g_bTSKeyBound; // input.cpp: the + commands press only from a key bound to them
static void IN_Alt2Down()
{
	if (!g_bTSKeyBound)
		return;
	if (TS_AttackSwitched())
		KeyDown(&in_attack2);
	else
		in_cancel = true;
}
static void IN_Alt2Up()
{
	if (TS_AttackSwitched())
		KeyUp(&in_attack2);
	else
		in_cancel = false;
}

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

// togglebrowser: client.dll's (0x1001fec0 -> 0x10063590) shows or hides the SDK 2.3 in-game server
// browser panel, which halflife-updated no longer has; here it does nothing.
static void TS_NoOp() {}

void TSRegisterCommands()
{
	gEngfuncs.pfnAddCommand("+alt2", IN_Alt2Down);
	gEngfuncs.pfnAddCommand("-alt2", IN_Alt2Up);
	gEngfuncs.pfnAddCommand("buy", TS_Buy);
	gEngfuncs.pfnAddCommand("togglebrowser", TS_NoOp);
}
