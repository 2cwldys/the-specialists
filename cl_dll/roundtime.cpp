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
// The Specialists round-time countdown HUD.
//
// The server (CTSGameRules::RestartRound (inline round-timer arm)) registers a user
// message named "RoundTime" (fixed size 2) and writes two bytes: the seconds
// remaining in the round, then a flag byte (0 at start/restart, 2 at timeout —
// matches the original CTSGameRules wire format). We turn the remaining
// seconds into an absolute client-time expiry and count it down locally so the
// clock keeps ticking between server updates, then draw it as MM:SS near the top
// of the screen.
//

#include "hud.h"
#include "cl_util.h"
#include "parsemsg.h"
#include "ts_hud.h"

#include <stdio.h>

DECLARE_MESSAGE(m_RoundTime, RoundTime)

bool CHudRoundTime::Init()
{
	m_flEndTime = 0;
	m_iFlags = 0;

	HOOK_MESSAGE(RoundTime);

	gHUD.AddHudElem(this);

	return true;
}

bool CHudRoundTime::VidInit()
{
	return true;
}

void CHudRoundTime::Reset()
{
	m_flEndTime = 0;
	m_iFlags &= ~HUD_ACTIVE;
}

bool CHudRoundTime::MsgFunc_RoundTime(const char* pszName, int iSize, void* pbuf)
{
	BEGIN_READ(pbuf, iSize);
	int seconds = READ_BYTE();
	READ_BYTE(); // flag byte — 0 at round start/restart, 2 at timeout (original RoundTime is 2 bytes)

	if (seconds <= 0)
	{
		// the round timer was cleared / ran out — stop drawing the clock
		m_flEndTime = 0;
		m_iFlags &= ~HUD_ACTIVE;
	}
	else
	{
		m_flEndTime = gHUD.m_flTime + seconds;
		m_iFlags |= HUD_ACTIVE;
	}

	return true;
}

bool CHudRoundTime::Draw(float flTime)
{
	if ((m_iFlags & HUD_ACTIVE) == 0 || m_flEndTime == 0)
		return true;
	// client.dll draws the timer (0x1004a130) from its status draw, after its nohud test (0x10049f06)
	if (TSHud_NoHud())
		return true;

	int remaining = (int)(m_flEndTime - gHUD.m_flTime);
	if (remaining < 0)
		remaining = 0;

	char szTime[16];
	sprintf(szTime, "%d:%02d", remaining / 60, remaining % 60);

	int r, g, b;
	// flash red in the last ten seconds, otherwise the standard yellowish HUD tone
	UnpackRGB(r, g, b, remaining <= 10 ? RGB_REDISH : RGB_YELLOWISH);
	ScaleColors(r, g, b, 255);

	// centred just below the top of the screen
	int width, height;
	GetConsoleStringSize(szTime, &width, &height);
	int x = (ScreenWidth - width) / 2;
	int y = ScreenHeight / 40;

	gHUD.DrawHudString(x, y, ScreenWidth, szTime, r, g, b);

	return true;
}
