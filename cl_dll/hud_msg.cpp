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

void CAM_ToFirstPerson();

void CHud::MsgFunc_ViewMode(const char* pszName, int iSize, void* pbuf)
{
	CAM_ToFirstPerson();
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
	//Note: this user message could be updated to include multiple gamemodes, so make sure this checks for game mode 1
	//See CHalfLifeTeamplay::UpdateGameMode
	//TODO: define game mode constants
	m_Teamplay = READ_BYTE() == 1;

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

// "TSFade" — the TS screen fade (same layout as the stock ScreenFade; the server already
// stretched duration/holdTime by the recipient's slow factor). The engine never sees this
// message name, so we replicate its ScreenFade parse math exactly (xash CL_ParseScreenFade)
// and hand the result to pfnSetScreenFade. NB: while slow-mo is active, the view.cpp tint
// rewrites the engine fade each frame — coexistence is the documented VERIFY tail.
bool CHud::MsgFunc_TSFade(const char* pszName, int iSize, void* pbuf)
{
	BEGIN_READ(pbuf, iSize);
	const float duration = (float)READ_SHORT();
	const float holdTime = (float)READ_SHORT();
	const int flags = READ_SHORT();
	const int r = READ_BYTE(), g = READ_BYTE(), b = READ_BYTE(), a = READ_BYTE();

	const float flScale = (flags & FFADE_LONGFADE) ? (1.0f / 256.0f) : (1.0f / 4096.0f);
	const float now = gEngfuncs.GetClientTime();

	screenfade_t sf;
	memset(&sf, 0, sizeof(sf));
	sf.fadeFlags = flags;
	sf.fader = r;
	sf.fadeg = g;
	sf.fadeb = b;
	sf.fadealpha = a;
	sf.fadeSpeed = 0.0f;
	sf.fadeEnd = duration * flScale;
	sf.fadeReset = holdTime * flScale;
	if (duration > 0.0f)
	{
		if (flags & FFADE_OUT)
		{
			if (sf.fadeEnd != 0.0f)
				sf.fadeSpeed = -(float)sf.fadealpha / sf.fadeEnd;
			sf.fadeEnd += now;
			sf.fadeTotalEnd = sf.fadeEnd;
			sf.fadeReset += sf.fadeEnd;
		}
		else
		{
			if (sf.fadeEnd != 0.0f)
				sf.fadeSpeed = (float)sf.fadealpha / sf.fadeEnd;
			sf.fadeReset += now;
			sf.fadeEnd += sf.fadeReset;
		}
	}
	gEngfuncs.pfnSetScreenFade(&sf);
	return true;
}
