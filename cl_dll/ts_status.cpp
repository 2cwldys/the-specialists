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
#include "ts_hud.h"

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

	TSHud_RegisterCvars();

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
	g_iTSHealth = m_iHealth; // the ammo HUD draws the health bar stacked above the ammo box
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

	// Health is drawn as the bottom-right bar by the ammo HUD (FUN_1004d5f0, stacked
	// above the ammo box) — see CHudTSAmmo. Here we draw the remaining values (armor,
	// cash) as text bottom-left. (Their exact faithful placement is still TODO — the
	// default TS HUD's armor/cash draw hasn't been traced yet; this is functional.)
	const int lineHeight = gHUD.m_scrinfo.iCharHeight + 2;
	const int pad = lineHeight / 2;
	const int margin = ScreenWidth / 40;
	const bool showCash = (m_iCash > 0);
	const int numLines = showCash ? 2 : 1;

	int hr, hg, hb;
	TSHud_Color(hr, hg, hb);

	const int panelW = ScreenWidth / 7;
	const int panelH = lineHeight * numLines + pad * 2;
	const int panelX = margin;
	const int panelY = ScreenHeight - panelH - ScreenHeight / 40;
	TSHud_Panel(panelX, panelY, panelW, panelH);

	const int x = panelX + pad;
	int y = panelY + pad;

	char buf[32];

	if (showCash)
	{
		sprintf(buf, "$%d", m_iCash);
		gHUD.DrawHudString(x, y, ScreenWidth, buf, hr, hg, hb);
		y += lineHeight;
	}

	sprintf(buf, "Armor: %d", m_iArmor);
	gHUD.DrawHudString(x, y, ScreenWidth, buf, hr, hg, hb);

	return true;
}
