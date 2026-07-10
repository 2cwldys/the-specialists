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
// This file was the stock Half-Life deathmatch weapon event handlers (glock,
// shotgun, mp5, python, gauss, crossbow, rpg, egon, hornetgun, tripmine, snark,
// crowbar). None of those weapons are playable in The Specialists: their events
// are never hooked (cl_dll/hl/hl_events.cpp registers only the 11 TS events), so
// every handler here was dead compiled code — and each pulled in stock Half-Life
// sound/model/sprite assets the mod does not ship. All of it has been removed.
//
// Three symbols remain because live client code still links against them:
//   pBeam / pBeam2     — beam state read by hl_objects.cpp and hud_msg.cpp
//   EV_TFC_IsAllyTeam  — team check called from vgui_ScorePanel.cpp (TS has no
//                        team allies, so it is always false)
#include "hud.h"
#include "cl_util.h"
#include "com_model.h"
#include "cl_entity.h"
#include "r_efx.h"

BEAM* pBeam;
BEAM* pBeam2;

bool EV_TFC_IsAllyTeam(int iTeam1, int iTeam2)
{
	return false;
}
