/***
*  ts_spectate.cpp — The Specialists free-look spectate HUD (cont.65).
*  See ts_spectate.h. The "Spectator" message is hooked in hud.cpp and routed here.
***/
#include "hud.h"
#include "cl_util.h"
#include "parsemsg.h"
#include "ts_spectate.h"

#include <stdio.h>

// gmsgSpectator byte-1 states — must match dlls/player.h.
#define TSSPEC_OFF 0
#define TSSPEC_COUNTDOWN 1
#define TSSPEC_READY 2

bool CHudSpectate::Init()
{
	m_iState = TSSPEC_OFF;
	m_iSeconds = 0;
	// NB: the "Spectator" message is hooked in CHud::Init (routes to this element);
	// we do not HOOK_MESSAGE here to avoid double-registering the callback.
	gHUD.AddHudElem(this);
	return true;
}

bool CHudSpectate::VidInit()
{
	return true;
}

void CHudSpectate::Reset()
{
	m_iState = TSSPEC_OFF;
	m_iSeconds = 0;
}

int CHudSpectate::MsgFunc_Spectator(const char* pszName, int iSize, void* pbuf)
{
	BEGIN_READ(pbuf, iSize);
	m_iState = READ_BYTE();
	m_iSeconds = READ_BYTE();

	if (m_iState != TSSPEC_OFF)
		m_iFlags |= HUD_ACTIVE;
	else
		m_iFlags &= ~HUD_ACTIVE;

	// Open the TS buy/config menu (the HUD-drawn CHudTSBuyMenu) during the spectate-wait (join/buy)
	// phase, hide it on spawn. The original opens its buy menu here (reference/MENUS.md).
	if (m_iState != TSSPEC_OFF)
		gHUD.m_TSBuyMenu.Open();
	else
		gHUD.m_TSBuyMenu.Close();

	return 1;
}

// Centre a console-font string horizontally at the given y.
static void DrawCentered(int y, const char* str, int r, int g, int b)
{
	int w, h;
	GetConsoleStringSize(str, &w, &h);
	gHUD.DrawHudString((ScreenWidth - w) / 2, y, ScreenWidth, str, r, g, b);
}

bool CHudSpectate::Draw(float flTime)
{
	if (m_iState == TSSPEC_OFF)
		return true;

	int r, g, b;
	UnpackRGB(r, g, b, RGB_YELLOWISH);
	ScaleColors(r, g, b, 255);

	// Centre overlay: the spawn countdown, then the "press fire to play" prompt.
	if (m_iState == TSSPEC_COUNTDOWN && m_iSeconds > 0)
	{
		char szNum[8];
		sprintf(szNum, "%d", m_iSeconds);
		DrawCentered(ScreenHeight * 2 / 5, szNum, r, g, b);
	}
	else // TSSPEC_READY (or countdown hit 0)
	{
		DrawCentered(ScreenHeight * 2 / 5, "Press fire to play", r, g, b);
	}

	// Free-look control help, just above the bottom (under the engine's "Free-Look").
	DrawCentered(ScreenHeight - ScreenHeight / 12,
		"JUMP - Change Camera modes, MOVEMENT KEYS - move around, MOUSE - rotate view",
		r, g, b);

	return true;
}
