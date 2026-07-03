/***
*  ts_spectate.cpp — The Specialists free-look spectate HUD (cont.65).
*  See ts_spectate.h. The "Spectator" message is hooked in hud.cpp and routed here.
***/
#include "hud.h"
#include "cl_util.h"
#include "parsemsg.h"
#include "ts_spectate.h"

#include <stdio.h>
#include "kbutton.h" // in_attack state — the fire-to-play "respawn" sender

// Per-player spectator flags from the (idx, flag) broadcast — the ORIGINAL wire
// (client.dll FUN_10065250 parses exactly BYTE idx + BYTE flag into a per-player array).
static bool s_bPlayerSpectating[MAX_PLAYERS + 1];

extern kbutton_t in_attack; // cl_dll/input.cpp

bool CHudSpectate::Init()
{
	m_iState = 0;
	m_iSeconds = 0;
	memset(s_bPlayerSpectating, 0, sizeof(s_bPlayerSpectating));
	// NB: the "Spectator" message is hooked in CHud::Init (routes to this element);
	// we do not HOOK_MESSAGE here to avoid double-registering the callback.
	gHUD.AddHudElem(this);
	m_iFlags |= HUD_ACTIVE; // always eligible; Draw self-gates on the live local state
	return true;
}

bool CHudSpectate::VidInit()
{
	return true;
}

void CHudSpectate::Reset()
{
	m_iState = 0;
	m_iSeconds = 0;
	memset(s_bPlayerSpectating, 0, sizeof(s_bPlayerSpectating));
}

// Is the LOCAL player spectating per the (idx,flag) table? Computed lazily — during connect
// the first Spectator broadcast arrives BEFORE GetLocalPlayer() is valid, so latching local
// state at parse time misses it (the round-7c "buy menu gone" regression).
static bool TS_LocalSpectating()
{
	cl_entity_t* local = gEngfuncs.GetLocalPlayer();
	if (!local || local->index < 1 || local->index > MAX_PLAYERS)
		return false;
	return s_bPlayerSpectating[local->index];
}

bool CHudSpectate::IsActive() const
{
	return TS_LocalSpectating();
}

// The ORIGINAL wire: BYTE playerIndex (1..32), BYTE flag (1 = spectating, 0 = playing).
// (The old state+seconds payload was an invented wire — refuted by the cont.84 dossier;
// the respawn countdown/"Press Fire To Play!" lines arrive as TSMessage texts instead.)
// Parse ONLY stores the table — all local-state consequences (menu open/close, the HUD) are
// derived per-frame where the local index is reliable.
int CHudSpectate::MsgFunc_Spectator(const char* pszName, int iSize, void* pbuf)
{
	BEGIN_READ(pbuf, iSize);
	const int idx = READ_BYTE();
	const int flag = READ_BYTE();
	if (idx >= 1 && idx <= MAX_PLAYERS)
		s_bPlayerSpectating[idx] = (flag != 0);
	return 1;
}

// Fire-to-play: the ORIGINAL client sends the "respawn" console command on +attack while
// spectating (client.dll @0x10023450, 0.2 s ratelimit) — the server's respawn gate decides.
// Gated on the buy menu being closed: while the menu is up, a click belongs to the menu.
static void TS_SpectateFireCheck()
{
	static float s_flNextSend;
	const float now = gEngfuncs.GetClientTime();
	if ((in_attack.state & 1) == 0 || gHUD.m_TSBuyMenu.IsUp())
		return;
	if (now < s_flNextSend)
		return;
	s_flNextSend = now + 0.2f;
	gEngfuncs.pfnClientCmd("respawn");
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
	// Per-frame local-state derivation + edge detection (parse time is too early — see above).
	const bool spec = TS_LocalSpectating();
	if (spec && m_iState == 0)
		gHUD.m_TSBuyMenu.Open(); // entering spectate-wait: the buy/config menu comes up once
	else if (!spec && m_iState != 0)
		gHUD.m_TSBuyMenu.Close(); // entering play
	m_iState = spec ? 1 : 0;
	if (!spec)
		return true;

	TS_SpectateFireCheck(); // fire-to-play -> the "respawn" console command (original mechanism)

	// (The respawn countdown number + "Press Fire To Play!" arrive as server TSMessage lines
	// now — the original mechanism; nothing to draw here for them.)

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
