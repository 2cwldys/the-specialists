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
#include "player_info.h" // g_PlayerExtraInfo (team name)

#include <stdio.h>

DECLARE_MESSAGE(m_TSStatus, TSHealth)
DECLARE_MESSAGE(m_TSStatus, TSArmor)
DECLARE_MESSAGE(m_TSStatus, TSCash)
DECLARE_MESSAGE(m_TSStatus, TSState)
DECLARE_MESSAGE(m_TSStatus, TSSpace)

bool CHudTSStatus::Init()
{
	m_iHealth = 0;
	m_iArmor = 0;
	m_iCash = 0;

	HOOK_MESSAGE(TSHealth);
	HOOK_MESSAGE(TSArmor);
	HOOK_MESSAGE(TSCash);
	HOOK_MESSAGE(TSState); // posture byte (0 stand / 1 duck / 2 stunt-prone) — server-authoritative
	HOOK_MESSAGE(TSSpace); // free carry slots

	TSHud_RegisterCvars();

	gHUD.AddHudElem(this);

	return true;
}

// Load one sprites/player/<name>.spr and capture its full-frame rect. Returns 0 if
// the sprite is absent (Draw then falls back to text), so a missing file never crashes.
static HSPRITE LoadStatusSprite(const char* name, Rect& rc)
{
	char path[64];
	sprintf(path, "sprites/player/%s.spr", name);
	HSPRITE h = SPR_Load(path);
	rc = {0, 0, 0, 0};
	if (h != 0)
	{
		rc.right = SPR_Width(h, 0);
		rc.bottom = SPR_Height(h, 0);
	}
	return h;
}

bool CHudTSStatus::VidInit()
{
	// TS draws a custom bottom-left status panel instead of the stock HEV health/battery:
	// a posture figure + the health number with a heart + a Kevlar (armor) indicator.
	// Sprite set RE'd from client.dll's load block @0x10049d1f.
	m_hStand = LoadStatusSprite("stand", m_rcStand);
	m_hCrouch = LoadStatusSprite("crouched", m_rcCrouch);
	m_hProne = LoadStatusSprite("prone", m_rcProne);
	m_hHeart = LoadStatusSprite("health", m_rcHeart);
	m_hKevlar = LoadStatusSprite("kevlar", m_rcKevlar);
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

bool CHudTSStatus::MsgFunc_TSState(const char* pszName, int iSize, void* pbuf)
{
	BEGIN_READ(pbuf, iSize);
	m_iState = READ_BYTE();
	m_iFlags |= HUD_ACTIVE;
	return true;
}

bool CHudTSStatus::MsgFunc_TSSpace(const char* pszName, int iSize, void* pbuf)
{
	BEGIN_READ(pbuf, iSize);
	m_iSpace = READ_BYTE(); // free carry slots (panel styling VERIFY — stored, drawn later)
	m_iFlags |= HUD_ACTIVE;
	return true;
}

bool CHudTSStatus::Draw(float flTime)
{
	if ((m_iFlags & HUD_ACTIVE) == 0)
		return true;

	if (gHUD.m_iHideHUDDisplay & (HIDEHUD_HEALTH | HIDEHUD_ALL))
		return true;

	// Hide the status panel while dead/spectating (the original shows no health area then) — the
	// spectate/buy phase owns the screen. cont.69l.
	if (gHUD.m_Spectate.IsActive())
		return true;

	// The TS status panel (bottom-left): team name on top, the posture figure on the
	// left, the health number + heart to its right, and a "Kevlar" label below when
	// armored. RE'd from client.dll (sprite slots @0x10049d1f, "%i Kevlar  %i" @0x1003808e)
	// and matched to the in-game reference. Armor is binary here (kevlar-or-not), not the
	// numeric battery the stock HUD shows.
	int hr, hg, hb;
	TSHud_Color(hr, hg, hb);

	const int lineHeight = gHUD.m_scrinfo.iCharHeight + 2;

	// Posture figure = the server's TSState byte (0 stand / 1 duck / 2 stunt-prone) — the
	// invented local derivation (g_iTSProne + curstate.usehull) is deleted; the wire drives it.
	HSPRITE hFig = m_hStand;
	Rect* rcFig = &m_rcStand;
	const int posture = m_iState;
	if (posture == 2 && m_hProne != 0) { hFig = m_hProne; rcFig = &m_rcProne; }
	else if (posture == 1 && m_hCrouch != 0) { hFig = m_hCrouch; rcFig = &m_rcCrouch; }

	const int figW = (hFig != 0) ? (rcFig->right - rcFig->left) : 0;
	const int figH = (hFig != 0) ? (rcFig->bottom - rcFig->top) : 0;

	// Panel geometry, bottom-left. Sized to the figure (with room for the team line
	// above and the kevlar line below).
	const int margin = ScreenWidth / 40;
	const int pad = lineHeight / 2;
	const int contentH = (figH > lineHeight * 2 ? figH : lineHeight * 2);
	const int panelW = (figW > 0 ? figW : 24) + ScreenWidth / 9;
	const int panelH = lineHeight + contentH + lineHeight + pad * 2;
	const int panelX = margin;
	const int panelY = ScreenHeight - panelH - ScreenHeight / 40;
	TSHud_Panel(panelX, panelY, panelW, panelH);

	const int left = panelX + pad;
	int y = panelY + pad;

	// Team name ("humans"/"specialists") from the local player's networked info.
	cl_entity_t* lp = gEngfuncs.GetLocalPlayer();
	if (lp != nullptr)
	{
		const char* team = g_PlayerExtraInfo[lp->index].teamname;
		if (team != nullptr && team[0] != '\0')
			TSHud_DrawString(left, y, ScreenWidth, team, hr, hg, hb);
	}
	y += lineHeight;

	// Posture figure on the left.
	const int figY = y + (contentH - figH) / 2;
	if (hFig != 0)
	{
		SPR_Set(hFig, hr, hg, hb);
		SPR_DrawAdditive(0, left, figY, rcFig);
	}

	// Health number + heart, to the right of the figure.
	const int col = left + (figW > 0 ? figW + pad : 28);
	const int midY = y + (contentH - lineHeight) / 2;
	// Health as the numbers.spr digit font (the original's HUD number), vertically centred on the line.
	const int numY = midY + (lineHeight - TS_NUMBER_H) / 2;
	int nx = TSHud_DrawNumber(col, numY, m_iHealth, hr, hg, hb);
	if (m_hHeart != 0)
	{
		const int heartH = m_rcHeart.bottom - m_rcHeart.top;
		SPR_Set(m_hHeart, hr, hg, hb);
		SPR_DrawAdditive(0, nx + pad, midY + (lineHeight - heartH) / 2, &m_rcHeart);
	}

	// Kevlar indicator below — drawn only when the player has armor (binary), with the
	// kevlar sprite if present, otherwise the "Kevlar" label.
	if (m_iArmor > 0)
	{
		const int ky = y + contentH;
		if (m_hKevlar != 0)
		{
			SPR_Set(m_hKevlar, hr, hg, hb);
			SPR_DrawAdditive(0, col, ky, &m_rcKevlar);
		}
		else
		{
			TSHud_DrawString(col, ky, ScreenWidth, "Kevlar", hr, hg, hb);
		}
	}

	return true;
}
