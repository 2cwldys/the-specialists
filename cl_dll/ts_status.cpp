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
#include "ts_weapon_info.h" // Set_ammo_count (the SrvSett id-2 shared call)
#include "ts_hud.h"
#include "demo_api.h"
#include "player_info.h" // g_PlayerExtraInfo (team name)
#include "vgui_TeamFortressViewport.h" // iTeamColors / iNumberOfTeamColors (team box color)

#include <stdio.h>

DECLARE_MESSAGE(m_TSStatus, TSHealth)
DECLARE_MESSAGE(m_TSStatus, TSArmor)
DECLARE_MESSAGE(m_TSStatus, TSCash)
DECLARE_MESSAGE(m_TSStatus, TSState)
DECLARE_MESSAGE(m_TSStatus, TSSpace)
DECLARE_MESSAGE(m_TSStatus, TSPAward)
DECLARE_MESSAGE(m_TSStatus, SrvSett)

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
	HOOK_MESSAGE(TSPAward); // per-player award/status byte (MSG_ALL broadcast)
	HOOK_MESSAGE(SrvSett); // server settings: realbullet/usecash/ammocount/weaponrestriction

	TSHud_RegisterCvars();

	gHUD.AddHudElem(this);

	return true;
}

// Load one sprites/player/<name>.spr and its frame size.
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

// client.dll's status Reset (0x10049e70) clears the health, armour and posture bytes (and the powerup
// and round-timer state, CHudTSPowerUp's and CHudRoundTime's here); the server resends them.
void CHudTSStatus::Reset()
{
	m_iHealth = 0;
	m_iArmor = 0;
	m_iState = 0;
	m_iFlags |= HUD_ACTIVE;
}

bool CHudTSStatus::MsgFunc_TSHealth(const char* pszName, int iSize, void* pbuf)
{
	BEGIN_READ(pbuf, iSize);
	m_iHealth = READ_BYTE();
	g_iTSHealth = m_iHealth; // the ammo HUD draws the health bar stacked above the ammo box
	// The player's health for the dead tests (CL_IsDead, the death camera, the view): the original
	// client keeps it from TSHealth (its handler @0x1004b748 stores the byte that @0x10026860, the
	// "health < 1" test, reads), since the server never sends the stock Health message.
	gHUD.m_Health.m_iHealth = m_iHealth;
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

// TSPAward — every player's broadcast award/status byte (0 clear / 1 out-of-round / 2-4 kill
// tiers / 10 The One / 11 respawning), keyed by entindex. Stored per-player (the original keeps
// a stride-8 per-player table); the popup/scoreboard presentation is a client-RE VERIFY.
int g_iTSPlayerAward[MAX_PLAYERS + 1] = {};

bool CHudTSStatus::MsgFunc_TSPAward(const char* pszName, int iSize, void* pbuf)
{
	BEGIN_READ(pbuf, iSize);
	const int idx = READ_BYTE();
	const int val = READ_BYTE();
	if (idx >= 1 && idx <= MAX_PLAYERS)
		g_iTSPlayerAward[idx] = val;
	m_iFlags |= HUD_ACTIVE;
	return true;
}

// SrvSett — the server-settings sync (BYTE id + SHORT value; star_SrvSett.md). Case 2 runs
// the SHARED Set_ammo_count so the client's purchase/ammo bookkeeping matches the server's;
// the other ids land in globals (cash-HUD hide / buy-menu filter consumers are a VERIFY tail).
int g_iTSRealBullet = 0;         // id 0 — bullets-are-projectiles mode
int g_iTSUseCash = 0;            // id 1 — the cash economy toggle (bool on the wire)
int g_iTSWeaponRestriction = 0x1f; // id 3 — the allowed-categories bitmask

bool CHudTSStatus::MsgFunc_SrvSett(const char* pszName, int iSize, void* pbuf)
{
	BEGIN_READ(pbuf, iSize);
	const int id = READ_BYTE();
	const int val = READ_SHORT();
	switch (id)
	{
	case 0: g_iTSRealBullet = val; break;
	case 1: g_iTSUseCash = val; break;
	case 2: Set_ammo_count(val); break; // the shared max-carry column (both libs, like the original)
	case 3: g_iTSWeaponRestriction = val; break;
	default: break; // unknown ids tolerated (the original's switch falls through)
	}
	m_iFlags |= HUD_ACTIVE;
	return true;
}

// client.dll 0x1004c250: dead for the HUD = the last TSHealth is 0, unless a demo is playing or
// being recorded. The status draw (0x10049ea0) shows the status panel, the powerup label and the
// team and objective parts only while not dead and not spectating (0x1004c280).
bool CHudTSStatus::IsDead() const
{
	if (0 != gEngfuncs.pDemoAPI->IsPlayingback() || 0 != gEngfuncs.pDemoAPI->IsRecording())
		return false;
	return m_iHealth == 0;
}

bool TSHud_PlayerPartsHidden()
{
	return gHUD.m_TSStatus.IsDead() || gHUD.m_Spectate.IsActive();
}

bool CHudTSStatus::Draw(float flTime)
{
	if ((m_iFlags & HUD_ACTIVE) == 0)
		return true;

	if (gHUD.m_iHideHUDDisplay & (HIDEHUD_HEALTH | HIDEHUD_ALL))
		return true;

	// The status draw (0x10049ea0) reaches the panel only while nohud is 0, not dead and not spectating;
	// the panel (0x1004a3e0) returns at once unless cl_realhud is 0. The kung-fu and super-jump figures
	// it also draws are CHudTSPowerUp's here (ts_powerup_hud.cpp).
	if (TSHud_NoHud() || TSHud_PlayerPartsHidden() || TSHud_RealHud())
		return true;

	// client.dll 0x1004a3e0, fixed pixels (only y follows the screen height). The boxes use the full HUD
	// colour at alpha 0x0f, their contents half of it, (195, 45, 45) when low.
	int hr, hg, hb;
	TSHud_Color(hr, hg, hb);
	const int halfR = hr / 2, halfG = hg / 2, halfB = hb / 2;
	const int wR = 195, wG = 45, wB = 45;

	const int SW = ScreenWidth;
	const int SH = ScreenHeight;
	const int BOXW = 0x7f;

	// Posture figure (server TSState byte: 0 stand / 1 crouch / 2 prone).
	HSPRITE hFig = m_hStand;
	Rect* rcF = &m_rcStand;
	if (m_iState == 2) { hFig = m_hProne; rcF = &m_rcProne; }
	else if (m_iState == 1) { hFig = m_hCrouch; rcF = &m_rcCrouch; }

	// Health box at (8, SH-48).
	const int hbY = SH - 0x30;
	FillRGBA(8, hbY, BOXW, 0x27, hr, hg, hb, 0x0f);

	// The figure is a health gauge: the missing top part in (195, 45, 45), the rest in half colour.
	{
		const int figH = rcF->bottom - rcF->top;
		const int cap = (m_iHealth < 100) ? m_iHealth : 100;
		const int missing = figH - (cap * figH) / 100;
		const int figX = 8, figY = hbY - 4;
		if (m_iHealth < 100)
		{
			Rect rt = {0, rcF->right, 0, missing};
			SPR_Set(hFig, wR, wG, wB);
			SPR_DrawAdditive(0, figX, figY, &rt);
		}
		Rect rb = {0, rcF->right, missing, figH};
		SPR_Set(hFig, halfR, halfG, halfB);
		SPR_DrawAdditive(0, figX, figY + missing, &rb);
	}

	// Health number at (72, SH-44), three digits wide; heart at (104, SH-48); both red below 50.
	{
		const bool lo = m_iHealth < 50;
		const int nr = lo ? wR : halfR, ng = lo ? wG : halfG, nb = lo ? wB : halfB;
		TSHud_DrawNumber(0x48, hbY + 4, TSHN_DRAWZERO | TSHN_3DIGITS, m_iHealth, nr, ng, nb);
		SPR_Set(m_hHeart, nr, ng, nb);
		SPR_DrawAdditive(0, 0x68, hbY, nullptr);
	}
	// Kevlar at (72, SH-28) while armoured, red below 50.
	if (m_iArmor != 0)
	{
		const bool lo = m_iArmor < 50;
		SPR_Set(m_hKevlar, lo ? wR : halfR, lo ? wG : halfG, lo ? wB : halfB);
		SPR_DrawAdditive(0, 0x48, hbY + 0x14, nullptr);
	}

	// Team box above it, whenever the GameMode byte is not 0: the local player's team name in his team
	// colour, the box in that colour at alpha 0x23.
	if (gHUD.m_iTeamplay != 0)
	{
		cl_entity_t* lp = gEngfuncs.GetLocalPlayer();
		const int idx = lp ? lp->index : 0;
		int tn = g_PlayerExtraInfo[idx].teamnumber;
		if (tn < 0 || tn >= iNumberOfTeamColors)
			tn = 0;
		const int tr = iTeamColors[tn][0], tg = iTeamColors[tn][1], tb = iTeamColors[tn][2];
		const int tbY = hbY - 0x14;
		FillRGBA(8, tbY, BOXW, 0x13, tr, tg, tb, 0x23);
		TSHud_DrawString(10, tbY, SW, g_PlayerExtraInfo[idx].teamname, tr, tg, tb);
	}

	// Free slots box at (136, SH-28): the count two digits wide, then "#HUD_Free_Slots"; red at 0.
	int boxY = SH - 0x1c;
	{
		FillRGBA(0x88, boxY, BOXW, 0x13, hr, hg, hb, 0x0f);
		const bool lo = m_iSpace == 0;
		const int nr = lo ? wR : halfR, ng = lo ? wG : halfG, nb = lo ? wB : halfB;
		const int xEnd = TSHud_DrawNumber(0x90, boxY + 2, TSHN_DRAWZERO | TSHN_2DIGITS, m_iSpace, nr, ng, nb);
		TSHud_DrawString(xEnd + 8, boxY, SW, CHudTextMessage::BufferedLocaliseTextString("#HUD_FreeSlots"), nr, ng, nb);
	}

	// Credits box above it when the GameMode byte is above 1 or the server runs the cash economy
	// (SrvSett id 1): the cash five digits wide, then "#HUD_Credits"; red below 1000.
	if (gHUD.m_iTeamplay > 1 || g_iTSUseCash != 0)
	{
		boxY -= 0x14;
		FillRGBA(0x88, boxY, BOXW, 0x13, hr, hg, hb, 0x0f);
		const bool lo = m_iCash < 1000;
		const int nr = lo ? wR : halfR, ng = lo ? wG : halfG, nb = lo ? wB : halfB;
		const int xEnd = TSHud_DrawNumber(0x88, boxY + 2, TSHN_DRAWZERO | TSHN_5DIGITS, m_iCash, nr, ng, nb);
		TSHud_DrawString(xEnd + 8, boxY, SW, CHudTextMessage::BufferedLocaliseTextString("#HUD_Credits"), nr, ng, nb);
	}

	return true;
}
