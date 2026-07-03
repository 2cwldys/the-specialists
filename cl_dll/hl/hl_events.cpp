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
#include "../hud.h"
#include "../cl_util.h"
#include "event_api.h"
#include "pmtrace.h"
#include "ev_hldm.h"

/*
======================
Game_HookEvents

Associate script file name with callback functions. Note that the format is
 always the same.  Of course, a clever mod team could actually embed parameters, behavior
 into the actual .sc files and create a .sc file parser and hook their functionality through
 that.. i.e., a scripting system.

That was what we were going to do, but we ran out of time...oh well.
======================
*/
void EV_TSFire(struct event_args_s* args); // The Specialists predicted fire (ev_ts.cpp)
void EV_TSDraw(struct event_args_s* args); // The Specialists deploy: seed the param cache (ts_weapon_predict.cpp)
void EV_TSKnife(struct event_args_s* args); // The Specialists knife slash presentation (ev_ts.cpp)
void EV_TSKatana(struct event_args_s* args); // The Specialists katana swing presentation (ev_ts.cpp)

void Game_HookEvents()
{
	gEngfuncs.pfnHookEvent("events/fire.sc", EV_TSFire); // TS: weapon_tsgun StandardFire
	gEngfuncs.pfnHookEvent("events/draw.sc", EV_TSDraw); // TS: weapon_tsgun Deploy (param-cache seed)
	gEngfuncs.pfnHookEvent("events/knife.sc", EV_TSKnife); // TS: knife slash (whoosh/impact/vm anim)
	gEngfuncs.pfnHookEvent("events/katana.sc", EV_TSKatana); // TS: katana swing (whoosh/impact/vm anim)
	// The stock-HL weapon event hooks (glock/shotgun/mp5/python/gauss/crowbar/crossbow/rpg/egon/
	// hornet/tripmine/snark) are DELETED — the original TS client hooks only its own 11 events
	// (registration parity; the stock weapons aren't playable in TS). train.sc/vehicle.sc are
	// engine-func pitch adjust, also absent from the original's hook set.
}
