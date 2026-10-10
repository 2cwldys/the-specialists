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
// roundtime.cpp
//
// The Specialists round timer. client.dll keeps it in its status element (0x100d43cc): RoundTime
// (0x1004bb20) stores the seconds and a mode byte, and the status draw (0x10049ea0) runs the timer
// draw (0x1004a130) whenever the seconds are not 0, dead or not.
//

#include "hud.h"
#include "cl_util.h"
#include "parsemsg.h"
#include "ts_hud.h"

#include <stdio.h>

DECLARE_MESSAGE(m_RoundTime, RoundTime)

bool CHudRoundTime::Init()
{
	m_flSeconds = 0;
	m_flNextTick = 0;
	m_iMode = 0;
	m_iFlags = 0;

	HOOK_MESSAGE(RoundTime);

	gHUD.AddHudElem(this);

	return true;
}

bool CHudRoundTime::VidInit()
{
	m_flSeconds = 0;
	m_flNextTick = 0;
	m_iMode = 0;
	return true;
}

// client.dll's status Reset (0x10049e70) clears the timer too.
void CHudRoundTime::Reset()
{
	m_flSeconds = 0;
	m_flNextTick = 0;
	m_iMode = 0;
}

// RoundTime: byte seconds, byte mode (0 counting, 1 counting ten times as fast, 2 held). The next
// tick is due at once.
bool CHudRoundTime::MsgFunc_RoundTime(const char* pszName, int iSize, void* pbuf)
{
	BEGIN_READ(pbuf, iSize);
	m_flSeconds = (float)READ_BYTE();
	m_iMode = READ_BYTE();
	m_flNextTick = 0;
	m_iFlags |= HUD_ACTIVE;
	return true;
}

// client.dll 0x1004a130. Each tick (every second, every 0.1 s in mode 1) takes a second off unless the
// mode is 2, down to 0. The minutes are drawn two digits wide at (ScreenWidth - 72, 12), then ":", then
// the seconds two digits with a leading zero, all in the HUD colour, (195, 45, 45) under a minute.
bool CHudRoundTime::Draw(float flTime)
{
	if ((m_iFlags & HUD_ACTIVE) == 0 || m_flSeconds == 0.0f)
		return true;
	// the status draw runs it after its nohud test (0x10049f06)
	if (TSHud_NoHud())
		return true;

	if (m_flNextTick < gHUD.m_flTime)
	{
		if (m_iMode != 2)
			m_flSeconds -= 1.0f;
		if (m_flSeconds <= 0.0f)
			m_flSeconds = 0;
		m_flNextTick = gHUD.m_flTime + (m_iMode == 1 ? 0.1f : 1.0f);
	}

	const int secs = (int)m_flSeconds;
	int r, g, b;
	if (secs < 60)
	{
		r = 195;
		g = 45;
		b = 45;
	}
	else
		TSHud_Color(r, g, b);

	int x = TSHud_DrawNumber(ScreenWidth - 0x48, 0xc, TSHN_DRAWZERO | TSHN_2DIGITS, secs / 60, r, g, b);
	x = TSHud_DrawString(x, 0xc, ScreenWidth, ":", r, g, b);
	TSHud_DrawNumber(x, 0xc, TSHN_DRAWZERO | TSHN_2DIGITS | TSHN_ZEROPAD, secs % 60, r, g, b);
	return true;
}
