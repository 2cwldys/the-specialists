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
	const int oldState = m_iState;
	m_iState = READ_BYTE();
	m_iSeconds = READ_BYTE();

	if (m_iState != TSSPEC_OFF)
		m_iFlags |= HUD_ACTIVE;
	else
		m_iFlags &= ~HUD_ACTIVE;

	// "Press fire to play" is a one-shot prompt shown when the respawn countdown finishes (it fades),
	// NOT a permanent centre overlay — push it through the notification queue on the transition to READY.
	if (m_iState == TSSPEC_READY && oldState != TSSPEC_READY)
		gHUD.m_TSMessage.Add("Press fire to play", 0, 0, 0, 20, 0);

	// Open the TS buy/config menu ONCE, on entering the spectate-wait phase — NOT every countdown
	// tick. The server resends the spectate message each tick; reopening every tick made the menu
	// flicker (and reappear after the player applied a config). After the player applies (which closes
	// the menu), it stays closed; `buy` reopens it. Hide it on spawn (transition to OFF).
	if (m_iState != TSSPEC_OFF && oldState == TSSPEC_OFF)
		gHUD.m_TSBuyMenu.Open();
	else if (m_iState == TSSPEC_OFF)
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

	// Centre overlay: the respawn countdown number (light text, centred — the TS notification style).
	// "Press fire to play" is NOT drawn here; it's a one-shot fading notification pushed on the
	// transition to READY (see MsgFunc_Spectator), so it appears after the countdown then disappears.
	if (m_iState == TSSPEC_COUNTDOWN && m_iSeconds > 0)
	{
		char szNum[8];
		sprintf(szNum, "%d", m_iSeconds);
		DrawCentered(ScreenHeight / 2, szNum, 230, 230, 230);
	}

	// The TS spectate bottom bar (no top bar — verified vs the original): a dark band carrying the
	// camera-mode label + the free-look control help. Drawn in the HUD layer (so the help is NOT
	// hidden under the stock spectator overlay, which is now suppressed during TS spectate).
	int br, bg, bb;
	UnpackRGB(br, bg, bb, RGB_YELLOWISH); // the bottom bar (mode + help) is the orange spectator text
	ScaleColors(br, bg, bb, 255);
	// Geometry from the stock SpectatorPanel bottom border (the original's spectate bar IS that panel):
	// height = YRES_HD(PANEL_HEIGHT=64), anchored at the screen bottom; the mode label sits YRES_HD(6)
	// from the bar top (vgui_SpectatorPanel.cpp), the help line near the bottom.
	const int barH = (int)YRES_HD(64);
	const int barY = ScreenHeight - barH;
	const int lineH = gHUD.m_scrinfo.iCharHeight;
	// pfnFillRGBA is ADDITIVE (black adds nothing -> invisible); the dark bar needs the alpha-BLEND fill.
	gEngfuncs.pfnFillRGBABlend(0, barY, ScreenWidth, barH, 0, 0, 0, 180);
	DrawCentered(barY + (int)YRES_HD(6), "Free-Look", br, bg, bb);
	DrawCentered(barY + barH - lineH - (int)YRES_HD(6),
		"JUMP - Change Camera modes, MOVEMENT KEYS - move around, MOUSE - rotate view",
		br, bg, bb);

	return true;
}
