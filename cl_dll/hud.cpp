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
// hud.cpp
//
// implementation of CHud class
//

#include "hud.h"
#include "cl_util.h"
#include <string.h>
#include <stdio.h>
#include "parsemsg.h"
#include "ts_attachments.h" // TS_Attachments_Init — the per-player laser/flashlight attach state
#include "ts_hud.h"			// TSHud_RegisterCvars — registers `nohud` (the original's HUD-off cvar)
#include "ts_particles.h"   // TSParticles_Reset — drop stale per-map sprite handles on VidInit
#include "ts_motionblur.h"  // TS_MotionBlur_Init — the TSMBlur ghost-trail hook
#include "vgui_int.h"
#include "vgui_TeamFortressViewport.h"

#include "demo.h"
#include "demo_api.h"
#include "vgui_ScorePanel.h"

hud_player_info_t g_PlayerInfoList[MAX_PLAYERS_HUD + 1];	// player info from the engine
extra_player_info_t g_PlayerExtraInfo[MAX_PLAYERS_HUD + 1]; // additional player info sent directly to the client dll

class CHLVoiceStatusHelper : public IVoiceStatusHelper
{
public:
	void GetPlayerTextColor(int entindex, int color[3]) override
	{
		color[0] = color[1] = color[2] = 255;

		if (entindex >= 0 && entindex < sizeof(g_PlayerExtraInfo) / sizeof(g_PlayerExtraInfo[0]))
		{
			int iTeam = g_PlayerExtraInfo[entindex].teamnumber;

			if (iTeam < 0)
			{
				iTeam = 0;
			}

			iTeam = iTeam % iNumberOfTeamColors;

			color[0] = iTeamColors[iTeam][0];
			color[1] = iTeamColors[iTeam][1];
			color[2] = iTeamColors[iTeam][2];
		}
	}

	void UpdateCursorState() override
	{
		gViewPort->UpdateCursorState();
	}

	int GetAckIconHeight() override
	{
		return ScreenHeight - gHUD.m_iFontHeight * 3 - 6;
	}

	bool CanShowSpeakerLabels() override
	{
		if (gViewPort && gViewPort->m_pScoreBoard)
			return !gViewPort->m_pScoreBoard->isVisible();
		else
			return false;
	}
};
static CHLVoiceStatusHelper g_VoiceStatusHelper;


extern client_sprite_t* GetSpriteList(client_sprite_t* pList, const char* psz, int iRes, int iCount);

extern cvar_t* sensitivity; // inputw32.cpp; MsgFunc_SetFOV reads its value as client.dll does
cvar_t* cl_lw = NULL;
cvar_t* r_decals = nullptr;

void ShutdownInput();

//DECLARE_MESSAGE(m_Logo, Logo)
int __MsgFunc_Logo(const char* pszName, int iSize, void* pbuf)
{
	return static_cast<int>(gHUD.MsgFunc_Logo(pszName, iSize, pbuf));
}

//DECLARE_MESSAGE(m_Logo, Logo)
int __MsgFunc_ResetHUD(const char* pszName, int iSize, void* pbuf)
{
	return static_cast<int>(gHUD.MsgFunc_ResetHUD(pszName, iSize, pbuf));
}

int __MsgFunc_InitHUD(const char* pszName, int iSize, void* pbuf)
{
	gHUD.MsgFunc_InitHUD(pszName, iSize, pbuf);
	return 1;
}

int __MsgFunc_SetFOV(const char* pszName, int iSize, void* pbuf)
{
	return static_cast<int>(gHUD.MsgFunc_SetFOV(pszName, iSize, pbuf));
}

int __MsgFunc_Concuss(const char* pszName, int iSize, void* pbuf)
{
	return static_cast<int>(gHUD.MsgFunc_Concuss(pszName, iSize, pbuf));
}

int __MsgFunc_TSFade(const char* pszName, int iSize, void* pbuf)
{
	return static_cast<int>(gHUD.MsgFunc_TSFade(pszName, iSize, pbuf));
}

int __MsgFunc_GameMode(const char* pszName, int iSize, void* pbuf)
{
	return static_cast<int>(gHUD.MsgFunc_GameMode(pszName, iSize, pbuf));
}

// TFFree Command Menu
void __CmdFunc_OpenCommandMenu()
{
	if (gViewPort)
	{
		gViewPort->ShowCommandMenu(gViewPort->m_StandardMenu);
	}
}

// TFC "special" command
void __CmdFunc_InputPlayerSpecial()
{
	if (gViewPort)
	{
		gViewPort->InputPlayerSpecial();
	}
}

void __CmdFunc_CloseCommandMenu()
{
	if (gViewPort)
	{
		gViewPort->InputSignalHideCommandMenu();
	}
}

void __CmdFunc_ForceCloseCommandMenu()
{
	if (gViewPort)
	{
		gViewPort->HideCommandMenu();
	}
}

// TFFree Command Menu Message Handlers
int __MsgFunc_ValClass(const char* pszName, int iSize, void* pbuf)
{
	if (gViewPort)
		return static_cast<int>(gViewPort->MsgFunc_ValClass(pszName, iSize, pbuf));
	return 0;
}

int __MsgFunc_TeamNames(const char* pszName, int iSize, void* pbuf)
{
	if (gViewPort)
		return static_cast<int>(gViewPort->MsgFunc_TeamNames(pszName, iSize, pbuf));
	return 0;
}

int __MsgFunc_Feign(const char* pszName, int iSize, void* pbuf)
{
	if (gViewPort)
		return static_cast<int>(gViewPort->MsgFunc_Feign(pszName, iSize, pbuf));
	return 0;
}

int __MsgFunc_Detpack(const char* pszName, int iSize, void* pbuf)
{
	if (gViewPort)
		return static_cast<int>(gViewPort->MsgFunc_Detpack(pszName, iSize, pbuf));
	return 0;
}

int __MsgFunc_VGUIMenu(const char* pszName, int iSize, void* pbuf)
{
	if (gViewPort)
		return static_cast<int>(gViewPort->MsgFunc_VGUIMenu(pszName, iSize, pbuf));
	return 0;
}

int __MsgFunc_MOTD(const char* pszName, int iSize, void* pbuf)
{
	if (gViewPort)
		return static_cast<int>(gViewPort->MsgFunc_MOTD(pszName, iSize, pbuf));
	return 0;
}

int __MsgFunc_BuildSt(const char* pszName, int iSize, void* pbuf)
{
	if (gViewPort)
		return static_cast<int>(gViewPort->MsgFunc_BuildSt(pszName, iSize, pbuf));
	return 0;
}

int __MsgFunc_RandomPC(const char* pszName, int iSize, void* pbuf)
{
	if (gViewPort)
		return static_cast<int>(gViewPort->MsgFunc_RandomPC(pszName, iSize, pbuf));
	return 0;
}

int __MsgFunc_ServerName(const char* pszName, int iSize, void* pbuf)
{
	if (gViewPort)
		return static_cast<int>(gViewPort->MsgFunc_ServerName(pszName, iSize, pbuf));
	return 0;
}

int __MsgFunc_ScoreInfo(const char* pszName, int iSize, void* pbuf)
{
	if (gViewPort)
		return static_cast<int>(gViewPort->MsgFunc_ScoreInfo(pszName, iSize, pbuf));
	return 0;
}

int __MsgFunc_TeamScore(const char* pszName, int iSize, void* pbuf)
{
	if (gViewPort)
		return static_cast<int>(gViewPort->MsgFunc_TeamScore(pszName, iSize, pbuf));
	return 0;
}

int __MsgFunc_TeamInfo(const char* pszName, int iSize, void* pbuf)
{
	if (gViewPort)
		return static_cast<int>(gViewPort->MsgFunc_TeamInfo(pszName, iSize, pbuf));
	return 0;
}

int __MsgFunc_Spectator(const char* pszName, int iSize, void* pbuf)
{
	// TS (cont.65): the "Spectator" message drives the TS free-look spectate HUD
	// (countdown + "Press fire to play"), not the TFC spectator panel.
	return gHUD.m_Spectate.MsgFunc_Spectator(pszName, iSize, pbuf);
}

int __MsgFunc_AllowSpec(const char* pszName, int iSize, void* pbuf)
{
	if (gViewPort)
		return static_cast<int>(gViewPort->MsgFunc_AllowSpec(pszName, iSize, pbuf));
	return 0;
}

// This is called every time the DLL is loaded
void CHud::Init()
{
	HOOK_MESSAGE(Logo);
	HOOK_MESSAGE(ResetHUD);
	HOOK_MESSAGE(GameMode);
	HOOK_MESSAGE(InitHUD);
	// (ViewMode / SpecFade / ResetFade hooks removed — not in the original client's
	// pfnHookUserMsg set (HL spectator leftovers); the viewport MsgFunc_*Fade methods
	// stay for the spectator code's internal calls.)
	HOOK_MESSAGE(SetFOV);
	HOOK_MESSAGE(Concuss);
	HOOK_MESSAGE(TSFade); // the TS screen fade (slow-time-stretched ScreenFade)
	// (Weapons hook removed — not in the original client's pfnHookUserMsg set; U0 parity.)

	// TFFree CommandMenu
	HOOK_COMMAND("+commandmenu", OpenCommandMenu);
	HOOK_COMMAND("-commandmenu", CloseCommandMenu);
	HOOK_COMMAND("ForceCloseCommandMenu", ForceCloseCommandMenu);
	HOOK_COMMAND("special", InputPlayerSpecial);

	HOOK_MESSAGE(ValClass);
	HOOK_MESSAGE(TeamNames);
	HOOK_MESSAGE(Feign);
	HOOK_MESSAGE(Detpack);
	HOOK_MESSAGE(MOTD);
	HOOK_MESSAGE(BuildSt);
	HOOK_MESSAGE(RandomPC);
	HOOK_MESSAGE(ServerName);
	HOOK_MESSAGE(ScoreInfo);
	HOOK_MESSAGE(TeamScore);
	HOOK_MESSAGE(TeamInfo);

	HOOK_MESSAGE(Spectator);
	HOOK_MESSAGE(AllowSpec);

	// VGUI Menus
	HOOK_MESSAGE(VGUIMenu);

	CVAR_CREATE("hud_classautokill", "1", FCVAR_ARCHIVE | FCVAR_USERINFO); // controls whether or not to suicide immediately on TF class switch
	CVAR_CREATE("hud_takesshots", "0", FCVAR_ARCHIVE);					   // controls whether or not to automatically take screenshots at the end of a round


	m_iLogo = 0;
	m_iFOV = 0;

	CVAR_CREATE("zoom_sensitivity_ratio", "1.2", 0); // flags 0, as client.dll (0x10020364)
	// TS fog cvars — the original registers all six HERE, right after zoom_sensitivity_ratio
	// (client.dll CHud::Init, sites 0x20372–0x203d4); only gl_fog is ARCHIVE, and every read is
	// by-name (pfnGetCvarFloat), so the returned pointers are unused. Semantics: gl_fog 0
	// (default) = fog OFF even on fog maps; 1 = honor the map's ts_fog; 2 = (single-player only)
	// force user fog from gl_fog_red/green/blue/start/end. Consumer seam: the un-reconstructed
	// client fog pass (FUN_10056c20, called from the V_CalcRefdef tail) + the ts_fog BSP
	// entity-lump parse — see reference/dossiers/cont94/client_cvar_parity.md §A1.
	CVAR_CREATE("gl_fog", "0", FCVAR_ARCHIVE);
	gEngfuncs.pfnRegisterVariable("gl_fog_start", "0", 0);
	gEngfuncs.pfnRegisterVariable("gl_fog_end", "2048", 0);
	gEngfuncs.pfnRegisterVariable("gl_fog_red", ".5", 0);
	gEngfuncs.pfnRegisterVariable("gl_fog_green", ".5", 0);
	gEngfuncs.pfnRegisterVariable("gl_fog_blue", ".5", 0);
	default_fov = CVAR_CREATE("default_fov", "80", 0); // TS default FOV is 80 (user-confirmed vs the original); flags 0 as client.dll (0x100203fe)
	m_pCvarStealMouse = CVAR_CREATE("hud_capturemouse", "1", FCVAR_ARCHIVE);
	// TS parity: no `hud_draw` in the original client — its HUD-off switch is the `nohud` cvar
	// (registered in TSHud_RegisterCvars); the draw gates test nohud == 0.
	TSHud_RegisterCvars();
	m_pCvarNoHud = gEngfuncs.pfnGetCvarPointer("nohud");
	cl_lw = gEngfuncs.pfnGetCvarPointer("cl_lw");
	r_decals = gEngfuncs.pfnGetCvarPointer("r_decals");

	m_pSpriteList = NULL;

	// Clear any old HUD list
	if (m_pHudList)
	{
		HUDLIST* pList;
		while (m_pHudList)
		{
			pList = m_pHudList;
			m_pHudList = m_pHudList->pNext;
			free(pList);
		}
		m_pHudList = NULL;
	}

	// In case we get messages before the first update -- time will be valid
	m_flTime = 1.0;

	m_Ammo.Init();
	m_Health.Init();
	m_SayText.Init();
	m_Spectator.Init();
	m_Geiger.Init();
	m_Train.Init();
	m_Battery.Init();
	m_Flash.Init();
	m_Message.Init();
	m_StatusBar.Init();
	m_DeathNotice.Init();
	m_AmmoSecondary.Init();
	m_TextMessage.Init();
	m_StatusIcons.Init();
	m_RoundTime.Init();
	m_TSStatus.Init();
	m_TSAmmo.Init();
	m_TSWeaponBar.Init();
	m_Spectate.Init();
	m_TSBuyMenu.Init();
	m_TSMessage.Init();
	m_TSPowerUp.Init();
	m_TSReticle.Init();
	m_TSDebris.Init();
	m_TSObjectives.Init(); // TSFlag / Objective / ShowObj / TSHVoice (the original's one CHudTS element)
	TS_Attachments_Init(); // hook the ActItems message (per-player laser/flashlight attach state)
	TS_MotionBlur_Init();  // hook the TSMBlur message (slow-motion ghost trails)
	TSParticles_Init();    // hook the TSPSys message (particle-system triggers; subtype 3 = wind)
	GetClientVoiceMgr()->Init(&g_VoiceStatusHelper, (vgui::Panel**)&gViewPort);

	m_Menu.Init();

	MsgFunc_ResetHUD(0, 0, NULL);

#ifdef STEAM_RICH_PRESENCE
	gEngfuncs.pfnClientCmd("richpresence_gamemode\n"); // reset
	gEngfuncs.pfnClientCmd("richpresence_update\n");
#endif
}

// CHud destructor
// cleans up memory allocated for m_rg* arrays
CHud::~CHud()
{
	delete[] m_rghSprites;
	delete[] m_rgrcRects;
	delete[] m_rgszSpriteNames;

	if (m_pHudList)
	{
		HUDLIST* pList;
		while (m_pHudList)
		{
			pList = m_pHudList;
			m_pHudList = m_pHudList->pNext;
			free(pList);
		}
		m_pHudList = NULL;
	}
}

// GetSpriteIndex()
// searches through the sprite list loaded from hud.txt for a name matching SpriteName
// returns an index into the gHUD.m_rghSprites[] array
// returns 0 if sprite not found
int CHud::GetSpriteIndex(const char* SpriteName)
{
	// look through the loaded sprite name list for SpriteName
	for (int i = 0; i < m_iSpriteCount; i++)
	{
		if (strncmp(SpriteName, m_rgszSpriteNames + (i * MAX_SPRITE_NAME_LENGTH), MAX_SPRITE_NAME_LENGTH) == 0)
			return i;
	}

	return -1; // invalid sprite
}

void CHud::VidInit()
{
	m_scrinfo.iSize = sizeof(m_scrinfo);
	GetScreenInfo(&m_scrinfo);

	// ----------
	// Load Sprites
	// ---------
	//	m_hsprFont = LoadSprite("sprites/%d_font.spr");

	m_hsprLogo = 0;
	m_hsprCursor = 0;

	// 320 or 640 on every call, as client.dll (@0x100206e1); TS's hud.txt has nothing else. HL25's
	// 1280/2560 matched no entry after the first call, so no sprite was reloaded at a map change and
	// the engine gave their handles to other sprites (the death notice's skull became crouched.spr).
	if (ScreenWidth < 640)
		m_iRes = 320;
	else
		m_iRes = 640;

	// Only load this once
	if (!m_pSpriteList)
	{
		// we need to load the hud.txt, and all sprites within
		m_pSpriteList = SPR_GetList("sprites/hud.txt", &m_iSpriteCountAllRes);

		if (m_pSpriteList)
		{
			// count the number of sprites of the appropriate res
			m_iSpriteCount = 0;
			client_sprite_t* p = m_pSpriteList;
			int j;
			for (j = 0; j < m_iSpriteCountAllRes; j++)
			{
				if (p->iRes == m_iRes)
					m_iSpriteCount++;
				p++;
			}

			// allocated memory for sprite handle arrays
			m_rghSprites = new HSPRITE[m_iSpriteCount];
			m_rgrcRects = new Rect[m_iSpriteCount];
			m_rgszSpriteNames = new char[m_iSpriteCount * MAX_SPRITE_NAME_LENGTH];

			p = m_pSpriteList;
			int index = 0;
			for (j = 0; j < m_iSpriteCountAllRes; j++)
			{
				if (p->iRes == m_iRes)
				{
					char sz[256];
					sprintf(sz, "sprites/%s.spr", p->szSprite);
					m_rghSprites[index] = SPR_Load(sz);
					m_rgrcRects[index] = p->rc;
					strncpy(&m_rgszSpriteNames[index * MAX_SPRITE_NAME_LENGTH], p->szName, MAX_SPRITE_NAME_LENGTH);

					index++;
				}

				p++;
			}
		}
	}
	else
	{
		// we have already have loaded the sprite reference from hud.txt, but
		// we need to make sure all the sprites have been loaded (we've gone through a transition, or loaded a save game)
		client_sprite_t* p = m_pSpriteList;
		int index = 0;
		for (int j = 0; j < m_iSpriteCountAllRes; j++)
		{
			if (p->iRes == m_iRes)
			{
				char sz[256];
				sprintf(sz, "sprites/%s.spr", p->szSprite);
				m_rghSprites[index] = SPR_Load(sz);
				index++;
			}

			p++;
		}
	}

	// assumption: number_1, number_2, etc, are all listed and loaded sequentially
	m_HUD_number_0 = GetSpriteIndex("number_0");

	m_iFontHeight = m_rgrcRects[m_HUD_number_0].bottom - m_rgrcRects[m_HUD_number_0].top;

	m_Ammo.VidInit();
	m_Health.VidInit();
	m_Spectator.VidInit();
	m_Geiger.VidInit();
	m_Train.VidInit();
	m_Battery.VidInit();
	m_Flash.VidInit();
	m_Message.VidInit();
	m_StatusBar.VidInit();
	m_DeathNotice.VidInit();
	m_SayText.VidInit();
	m_Menu.VidInit();
	m_AmmoSecondary.VidInit();
	m_TextMessage.VidInit();
	m_StatusIcons.VidInit();
	m_RoundTime.VidInit();
	m_TSStatus.VidInit();
	m_TSAmmo.VidInit();
	m_TSWeaponBar.VidInit();
	m_Spectate.VidInit();
	m_TSBuyMenu.VidInit();
	m_TSMessage.VidInit();
	m_TSPowerUp.VidInit();
	m_TSReticle.VidInit();
	m_TSDebris.VidInit();
	m_TSObjectives.VidInit();
	TSParticles_Reset();     // drop stale per-map sprite handles (impact particle pool)
	TS_Attachments_VidInit(); // drop stale per-map laser/flashlight sprite handles
	GetClientVoiceMgr()->VidInit();
}

bool CHud::MsgFunc_Logo(const char* pszName, int iSize, void* pbuf)
{
	BEGIN_READ(pbuf, iSize);

	// update Train data
	m_iLogo = READ_BYTE();

	return true;
}

float g_lastFOV = 0.0;

/*
============
COM_FileBase
============
*/
// Extracts the base name of a file (no path, no extension, assumes '/' as path separator)
void COM_FileBase(const char* in, char* out)
{
	int len, start, end;

	len = strlen(in);

	// scan backward for '.'
	end = len - 1;
	while (0 != end && in[end] != '.' && in[end] != '/' && in[end] != '\\')
		end--;

	if (in[end] != '.') // no '.', copy to end
		end = len - 1;
	else
		end--; // Found ',', copy to left of '.'


	// Scan backward for '/'
	start = len - 1;
	while (start >= 0 && in[start] != '/' && in[start] != '\\')
		start--;

	if (in[start] != '/' && in[start] != '\\')
		start = 0;
	else
		start++;

	// Length of new sting
	len = end - start + 1;

	// Copy partial string
	strncpy(out, &in[start], len);
	// Terminate it
	out[len] = 0;
}

/*
=================
HUD_IsGame

=================
*/
bool HUD_IsGame(const char* game)
{
	const char* gamedir;
	char gd[1024];

	gamedir = gEngfuncs.pfnGetGameDirectory();
	if (gamedir && '\0' != gamedir[0])
	{
		COM_FileBase(gamedir, gd);
		if (!stricmp(gd, game))
			return true;
	}
	return false;
}

/*
=====================
HUD_GetFOV

Returns last FOV
=====================
*/
float HUD_GetFOV()
{
	if (0 != gEngfuncs.pDemoAPI->IsRecording())
	{
		// Write it
		int i = 0;
		unsigned char buf[100];

		// Active
		*(float*)&buf[i] = g_lastFOV;
		i += sizeof(float);

		Demo_WriteBuffer(TYPE_ZOOM, i, buf);
	}

	if (0 != gEngfuncs.pDemoAPI->IsPlayingback())
	{
		g_lastFOV = g_demozoom;
	}
	return g_lastFOV;
}

bool CHud::MsgFunc_SetFOV(const char* pszName, int iSize, void* pbuf)
{
	BEGIN_READ(pbuf, iSize);

	int newfov = READ_BYTE();

	// client.dll 0x10049570: ignored while cl_lw is on (weapon prediction carries the zoom, and Think
	// eases toward it); else 80 for 0, and the sensitivity scales with newfov x 0.0125 (newfov / 80).
	if (cl_lw && cl_lw->value != 0.0f)
		return true;

	g_lastFOV = newfov;

	if (newfov == 0)
		m_iFOV = 80;
	else
		m_iFOV = newfov;

	if (m_iFOV == 80)
	{
		// reset to saved sensitivity
		m_flMouseSensitivity = 0;
		return true;
	}

	m_flMouseSensitivity = (float)newfov * 0.0125f * sensitivity->value * CVAR_GET_FLOAT("zoom_sensitivity_ratio");
	return true;
}


void CHud::AddHudElem(CHudBase* phudelem)
{
	HUDLIST *pdl, *ptemp;

	//phudelem->Think();

	if (!phudelem)
		return;

	pdl = (HUDLIST*)malloc(sizeof(HUDLIST));
	if (!pdl)
		return;

	memset(pdl, 0, sizeof(HUDLIST));
	pdl->p = phudelem;

	if (!m_pHudList)
	{
		m_pHudList = pdl;
		return;
	}

	ptemp = m_pHudList;

	while (ptemp->pNext)
		ptemp = ptemp->pNext;

	ptemp->pNext = pdl;
}

float CHud::GetSensitivity()
{
	return m_flMouseSensitivity;
}
