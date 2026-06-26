/***
*  ts_powerup_hud.cpp — The Specialists held-powerup HUD (green top-left timer). See ts_powerup_hud.h.
*
*  Powerups are picked up by RUNNING OVER them (touch). A timed powerup (slow-motion / slow-pause /
*  double-firerate) is then HELD: this HUD shows its icon-name + the seconds you have available to use.
*  Double-tapping +use (cl_dll/input.cpp, gated by cl_usepowerup) activates it; the server then sends
*  PwUpTime and this HUD counts the remaining seconds down. Mutually-exclusive by design — you only ever
*  hold one (picking up a different slow powerup replaces it).
***/

#include "hud.h"
#include "cl_util.h"
#include "parsemsg.h"
#include "ts_hud.h"
#include "ts_powerup_hud.h"

#include <stdio.h>

DECLARE_MESSAGE(m_TSPowerUp, PwUp)
DECLARE_MESSAGE(m_TSPowerUp, PwUpTime)
DECLARE_MESSAGE(m_TSPowerUp, TSSlowMo)

// held powerup type bits (dlls/ts_gamerules.h TS_PWUP_*)
#define PWUP_SLOWMO    0x01
#define PWUP_SLOWPAUSE 0x08
#define PWUP_DBLFIRE   0x10

bool CHudTSPowerUp::Init()
{
	HOOK_MESSAGE(PwUp);
	HOOK_MESSAGE(PwUpTime);
	HOOK_MESSAGE(TSSlowMo);
	gHUD.AddHudElem(this);
	m_iFlags |= HUD_ACTIVE; // self-gates on a held powerup
	Reset();
	return true;
}

bool CHudTSPowerUp::VidInit()
{
	return true;
}

void CHudTSPowerUp::Reset()
{
	m_iHeldType = 0;
	m_iSecondsAvail = 0;
	m_bActive = false;
	m_flCountdownEnd = 0;
	m_flSlowAmt = 100.0f;
}

// PwUp — the held powerup state (type + seconds available). type 0 = nothing held.
bool CHudTSPowerUp::MsgFunc_PwUp(const char* pszName, int iSize, void* pbuf)
{
	BEGIN_READ(pbuf, iSize);
	int type = READ_SHORT();
	int dur = READ_BYTE();
	// the one-shot grant notifies (kung-fu / low-grav) come through PwUp too, with dur 0 and a
	// non-held type — ignore those here (they aren't a held timed powerup).
	if (type == PWUP_SLOWMO || type == PWUP_SLOWPAUSE || type == PWUP_DBLFIRE)
	{
		if (type != m_iHeldType)
			m_bActive = false; // a freshly-held (or swapped) powerup is not yet activated
		m_iHeldType = type;
		m_iSecondsAvail = dur;
	}
	else
	{
		m_iHeldType = 0;
		m_bActive = false;
	}
	return true;
}

// PwUpTime — sent on activation: start (or refresh) the local countdown.
bool CHudTSPowerUp::MsgFunc_PwUpTime(const char* pszName, int iSize, void* pbuf)
{
	BEGIN_READ(pbuf, iSize);
	int secs = READ_BYTE();
	m_bActive = true;
	m_flCountdownEnd = gHUD.m_flTime + secs;
	return true;
}

// TSSlowMo — the TARGET slow amount (100 = normal speed). The smoothed g_flClientSlow eases toward
// it (in Draw). Sent for the local player whether they slowed themselves OR are a LOS victim of
// someone else's slow-motion, so the gunshot pitch deepens for victims too. RE: a 0 coord = normal.
bool CHudTSPowerUp::MsgFunc_TSSlowMo(const char* pszName, int iSize, void* pbuf)
{
	BEGIN_READ(pbuf, iSize);
	float amt = READ_COORD();
	m_flSlowAmt = (amt == 0.0f) ? 100.0f : amt;
	return true;
}

// Client CURRENT smoothed slow factor (0..1, init 1.0 = normal), eased toward the TSSlowMo target.
// RE: client DAT_100963e8 — consumed by the weapon-fire events to pitch the gunshot sounds (the more
// time is dilated, the deeper they sound). See ev_ts.cpp.
float g_flClientSlow = 1.0f;

bool CHudTSPowerUp::Draw(float flTime)
{
	// Ease the smoothed slow factor toward the TSSlowMo target (RE client InterpolateSlowMotion:
	// snap when within 0.05, else step by min(frametime*7, 1)). This feeds the gunshot sound pitch.
	{
		float target = m_flSlowAmt * 0.01f;
		float d = target - g_flClientSlow;
		float ad = d < 0.0f ? -d : d;
		if (ad > 0.05f)
		{
			float s = gHUD.m_flTimeDelta * 7.0f;
			if (s > 1.0f) s = 1.0f;
			d *= s;
		}
		g_flClientSlow += d;
	}

	// The slow-motion screen tint is drawn in V_CalcRefdef (cl_dll/view.cpp) via the engine screen-
	// fade — the faithful teal RGBA(0,100,125) at alpha (1-slow)*50, NOT a green fill here. (That
	// drives off g_flClientSlow, which this element eases above.)

	// The held-powerup label + seconds, at the TOP-LEFT, on the weapon-bar header row (raw
	// coords: the bar's header is at y=8 with slot 1 at x=8, so the label overlays its left edge —
	// matches the original). While HELD it shows the seconds available; once activated it counts down.
	if (m_iHeldType == 0)
		return true;

	int secs;
	if (m_bActive)
	{
		secs = (int)(m_flCountdownEnd - flTime + 0.999f); // ceil
		if (secs <= 0)
		{
			m_iHeldType = 0; // expired (the server also clears it via PwUp(0))
			m_bActive = false;
			return true;
		}
	}
	else
	{
		secs = m_iSecondsAvail;
	}

	const char* name;
	switch (m_iHeldType)
	{
	case PWUP_SLOWMO:    name = "Slow Motion"; break;
	case PWUP_SLOWPAUSE: name = "Slow Pause";  break;
	case PWUP_DBLFIRE:   name = "Double Fire"; break;
	default:             name = "Power-Up";    break;
	}

	// Label in green; the seconds a touch brighter and a fixed distance over (so the number sits in
	// the slot-1 area like the reference). Raw pixel coords (the TS bar isn't resolution-scaled).
	const int y = 10; // weapon-bar header text row (HDRY + 2)
	TSHud_DrawString(2, y, ScreenWidth, name, 0, 200, 0);
	char num[16];
	snprintf(num, sizeof(num), "%d", secs);
	num[sizeof(num) - 1] = '\0';
	TSHud_DrawString(112, y, ScreenWidth, num, 96, 255, 96);
	return true;
}
