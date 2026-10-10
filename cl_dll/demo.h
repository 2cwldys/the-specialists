//========= Copyright © 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose:
//
// $NoKeywords: $
//=============================================================================

#pragma once

// The demo buffer types client.dll writes and reads (Demo_ReadBuffer 0x1001b8c0), one 4-byte value
// each: 0 the field of view HUD_GetFOV returns, 1 the hold-position stance, 2 the third-person state.
enum
{
	TYPE_ZOOM = 0,
	TYPE_HOLD,
	TYPE_THIRDPERSON
};

void Demo_WriteBuffer(int type, int size, unsigned char* buffer);

extern float g_demozoom;          // 0x100d6074
extern int g_iTSDemoHold;         // 0x100d6078
extern int g_iTSDemoThirdPerson;  // 0x100d607c
