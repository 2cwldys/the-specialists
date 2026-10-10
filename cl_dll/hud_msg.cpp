/***
*
*	Copyright (c) 1999, Valve LLC. All rights reserved.
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
//  hud_msg.cpp
//

#include "hud.h"
#include "cl_util.h"
#include "parsemsg.h"
#include "screenfade.h"
#include "shake.h" // FFADE_* flags (shared with the server ScreenFade)
#include "r_efx.h"

#include "particleman.h"
extern IParticleMan* g_pParticleMan;

extern BEAM* pBeam;
extern BEAM* pBeam2;


/// USER-DEFINED SERVER MESSAGE HANDLERS

bool CHud::MsgFunc_ResetHUD(const char* pszName, int iSize, void* pbuf)
{
	ASSERT(iSize == 0);

	// clear all hud data
	HUDLIST* pList = m_pHudList;

	while (pList)
	{
		if (pList->p)
			pList->p->Reset();
		pList = pList->pNext;
	}

	//Reset weapon bits.
	m_iWeaponBits = 0ULL;

	// reset sensitivity
	m_flMouseSensitivity = 0;

	// reset concussion effect
	m_iConcussionEffect = 0;

	return true;
}

void CHud::MsgFunc_InitHUD(const char* pszName, int iSize, void* pbuf)
{
	// prepare all hud data
	HUDLIST* pList = m_pHudList;

	while (pList)
	{
		if (pList->p)
			pList->p->InitHUDData();
		pList = pList->pNext;
	}


	//TODO: needs to be called on every map change, not just when starting a new game
	if (g_pParticleMan)
		g_pParticleMan->ResetParticles();

	//Probably not a good place to put this.
	pBeam = pBeam2 = NULL;
}


bool CHud::MsgFunc_GameMode(const char* pszName, int iSize, void* pbuf)
{
	BEGIN_READ(pbuf, iSize);
	// client.dll keeps the byte itself (CHud +0x5c, 0x10021a10) and tests it as 0 / not 0 and above 1.
	m_iTeamplay = READ_BYTE();
	m_Teamplay = m_iTeamplay != 0;

#ifdef STEAM_RICH_PRESENCE
	if (m_Teamplay)
		gEngfuncs.pfnClientCmd("richpresence_gamemode Teamplay\n");
	else
		gEngfuncs.pfnClientCmd("richpresence_gamemode\n"); // reset

	gEngfuncs.pfnClientCmd("richpresence_update\n");
#endif

	return true;
}


bool CHud::MsgFunc_Damage(const char* pszName, int iSize, void* pbuf)
{
	int armor, blood;
	Vector from;
	int i;
	float count;

	BEGIN_READ(pbuf, iSize);
	armor = READ_BYTE();
	blood = READ_BYTE();

	for (i = 0; i < 3; i++)
		from[i] = READ_COORD();

	count = (blood * 0.5) + (armor * 0.5);

	if (count < 10)
		count = 10;

	// TODO: kick viewangles,  show damage visually

	return true;
}

bool CHud::MsgFunc_Concuss(const char* pszName, int iSize, void* pbuf)
{
	BEGIN_READ(pbuf, iSize);
	m_iConcussionEffect = READ_BYTE();
	if (0 != m_iConcussionEffect)
	{
		int r, g, b;
		UnpackRGB(r, g, b, RGB_YELLOWISH);
		this->m_StatusIcons.EnableIcon("dmg_concuss", r, g, b);
	}
	else
		this->m_StatusIcons.DisableIcon("dmg_concuss");
	return true;
}

// (MsgFunc_Weapons removed — the HLU 64-bit weapon-bits message is not part of the original
// TS wire; registration-set parity, dossier-gmsg U0. m_iWeaponBits stays 0: the stock ammo
// HUD it gated is suppressed in TS anyway, and the TS HUD rides WeaponInfo/WStatus.)

// "TSFade" (client.dll 0x1004c1e0): short duration, short hold, short flags, byte r, g, b, a, kept
// with the time it arrived. It sets no engine fade itself: V_CalcRefdef composes the screen fade from
// it and the slow motion tint every frame (view.cpp).
TSFade_t g_tsFade;

bool CHud::MsgFunc_TSFade(const char* pszName, int iSize, void* pbuf)
{
	BEGIN_READ(pbuf, iSize);
	g_tsFade.duration = (unsigned short)READ_SHORT();
	g_tsFade.hold = (unsigned short)READ_SHORT();
	g_tsFade.flags = READ_SHORT();
	g_tsFade.r = READ_BYTE();
	g_tsFade.g = READ_BYTE();
	g_tsFade.b = READ_BYTE();
	g_tsFade.a = READ_BYTE();
	g_tsFade.start = m_flTime;
	return true;
}
