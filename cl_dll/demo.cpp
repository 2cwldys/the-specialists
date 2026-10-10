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
#include "hud.h"
#include "cl_util.h"
#include "demo.h"
#include "demo_api.h"
#include <memory.h>
#include "Exports.h"

float g_demozoom;
int g_iTSDemoHold;
int g_iTSDemoThirdPerson;

// FIXME:  There should be buffer helper functions to avoid all of the *(int *)& crap.

/*
=====================
Demo_WriteBuffer

Write some data to the demo stream
=====================
*/
void Demo_WriteBuffer(int type, int size, unsigned char* buffer)
{
	int pos = 0;
	unsigned char buf[32 * 1024];
	*(int*)&buf[pos] = type;
	pos += sizeof(int);

	memcpy(&buf[pos], buffer, size);

	// Write full buffer out
	gEngfuncs.pDemoAPI->WriteBuffer(size + sizeof(int), buf);
}

/*
=====================
Demo_ReadBuffer

Engine wants us to parse some data from the demo stream
=====================
*/
void DLLEXPORT Demo_ReadBuffer(int size, unsigned char* buffer)
{
	//	RecClReadDemoBuffer(size, buffer);

	int type;
	int i = 0;

	type = *(int*)buffer;
	i += sizeof(int);
	switch (type)
	{
	case TYPE_ZOOM:
		g_demozoom = *(float*)&buffer[i];
		break;
	case TYPE_HOLD:
		g_iTSDemoHold = *(int*)&buffer[i];
		break;
	case TYPE_THIRDPERSON:
		g_iTSDemoThirdPerson = *(int*)&buffer[i];
		break;
	default:
		gEngfuncs.Con_DPrintf("Unknown demo buffer type, skipping.\n");
		break;
	}
}
