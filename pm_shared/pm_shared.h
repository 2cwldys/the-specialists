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
// pm_shared.h
//

#pragma once

#include "Platform.h"

struct playermove_s;

void PM_Init(playermove_s* ppmove);
void PM_Move(playermove_s* ppmove, qboolean server);
char PM_FindTextureType(const char* name);

/**
*	@brief Engine calls this to enumerate player collision hulls, for prediction. Return false if the hullnumber doesn't exist.
*/
bool PM_GetHullBounds(int hullnumber, float* mins, float* maxs);

// Spectator Movement modes (stored in pev->iuser1, so the physics code can get at them).
// The Specialists numbers them 1, 2, 3 and 5: its server's Observer_SetMode (0x89e44) takes 1, 2, 3
// and 5, and client.dll draws 5 as the first person view. No TS mode is 4, and TS has no overview map
// modes; the two below keep the SDK client's map code compiling with numbers TS never sends.
#define OBS_NONE 0
#define OBS_CHASE_LOCKED 1
#define OBS_CHASE_FREE 2
#define OBS_ROAMING 3
#define OBS_IN_EYE 5
#define OBS_MAP_FREE 6
#define OBS_MAP_CHASE 7

extern playermove_s* pmove;

inline bool g_CheckForPlayerStuck = false;
