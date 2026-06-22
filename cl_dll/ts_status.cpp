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
// ts_status.cpp
//
// The Specialists player-status HUD (health / armor / cash). Reads the server's
// TSHealth (byte), TSArmor (byte) and TSCash (short) user messages — the names +
// wire sizes are byte-faithful to the original LinkUserMessages
// (reference/HUD_MESSAGES.md). We draw the values as text in the bottom-left,
// since TS replaces the stock sprite-based health/battery HUD with a custom one.
//

#include "hud.h"
#include "cl_util.h"
#include "parsemsg.h"

#include <stdio.h>

DECLARE_MESSAGE(m_TSStatus, TSHealth)
DECLARE_MESSAGE(m_TSStatus, TSArmor)
DECLARE_MESSAGE(m_TSStatus, TSCash)

bool CHudTSStatus::Init()
{
	m_iHealth = 0;
	m_iArmor = 0;
	m_iCash = 0;

	HOOK_MESSAGE(TSHealth);
	HOOK_MESSAGE(TSArmor);
	HOOK_MESSAGE(TSCash);

	gHUD.AddHudElem(this);

	return true;
}

bool CHudTSStatus::VidInit()
{
	return true;
}

void CHudTSStatus::Reset()
{
	// Keep drawing across respawns; the server resends the values on HUD init.
	m_iFlags |= HUD_ACTIVE;
}

bool CHudTSStatus::MsgFunc_TSHealth(const char* pszName, int iSize, void* pbuf)
{
	BEGIN_READ(pbuf, iSize);
	m_iHealth = READ_BYTE();
	m_iFlags |= HUD_ACTIVE;
	return true;
}

bool CHudTSStatus::MsgFunc_TSArmor(const char* pszName, int iSize, void* pbuf)
{
	BEGIN_READ(pbuf, iSize);
	m_iArmor = READ_BYTE();
	m_iFlags |= HUD_ACTIVE;
	return true;
}

bool CHudTSStatus::MsgFunc_TSCash(const char* pszName, int iSize, void* pbuf)
{
	BEGIN_READ(pbuf, iSize);
	m_iCash = READ_SHORT();
	m_iFlags |= HUD_ACTIVE;
	return true;
}

bool CHudTSStatus::Draw(float flTime)
{
	if ((m_iFlags & HUD_ACTIVE) == 0)
		return true;

	if (gHUD.m_iHideHUDDisplay & (HIDEHUD_HEALTH | HIDEHUD_ALL))
		return true;

	// Bottom-left stack. Lines are drawn upward from the bottom margin.
	const int x = ScreenWidth / 40;
	const int lineHeight = gHUD.m_scrinfo.iCharHeight + 2;
	int y = ScreenHeight - lineHeight * 2 - ScreenHeight / 40;

	char buf[32];
	int r, g, b;

	// Health — gold normally, flashing red when low.
	sprintf(buf, "Health: %d", m_iHealth);
	UnpackRGB(r, g, b, m_iHealth <= 25 ? RGB_REDISH : RGB_YELLOWISH);
	ScaleColors(r, g, b, 255);
	gHUD.DrawHudString(x, y, ScreenWidth, buf, r, g, b);
	y += lineHeight;

	// Armor.
	sprintf(buf, "Armor: %d", m_iArmor);
	UnpackRGB(r, g, b, RGB_YELLOWISH);
	ScaleColors(r, g, b, 255);
	gHUD.DrawHudString(x, y, ScreenWidth, buf, r, g, b);

	// Cash — only when the player has any (DM has CashBased() off, so it stays 0).
	if (m_iCash > 0)
	{
		sprintf(buf, "$%d", m_iCash);
		UnpackRGB(r, g, b, RGB_YELLOWISH);
		ScaleColors(r, g, b, 255);
		gHUD.DrawHudString(x, ScreenHeight - lineHeight * 3 - ScreenHeight / 40, ScreenWidth, buf, r, g, b);
	}

	return true;
}
