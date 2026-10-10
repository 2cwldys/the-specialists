/***
*  ts_powerup_hud.cpp — The Specialists held-powerup HUD (green top-left timer). See ts_powerup_hud.h.
*
*  Powerups are picked up by RUNNING OVER them (touch). A timed powerup (slow-motion / slow-pause /
*  double-firerate) is then HELD: this HUD shows its name + the seconds you have available to use.
*  Double-tapping +use (CHudTSAmmo::Think, gated by cl_usepowerup) activates it; the server then sends
*  PwUpTime and this HUD counts the remaining seconds down.
***/

#include "hud.h"
#include "cl_util.h"
#include "parsemsg.h"
#include "ts_hud.h"
#include "cl_entity.h"  // cl_entity_t (GetLocalPlayer)
#include "event_api.h"  // pEventAPI->EV_PlaySound (goslow.wav, the original's mechanism)
#include "ts_powerup_hud.h"

#include <stdio.h>
#include <string.h>

DECLARE_MESSAGE(m_TSPowerUp, PwUp)
DECLARE_MESSAGE(m_TSPowerUp, PwUpTime)
DECLARE_MESSAGE(m_TSPowerUp, TSSlowMo)
DECLARE_MESSAGE(m_TSPowerUp, TSBTime)

// independent persistent powerups (bottom-center indicators)
#define TS_PWUP_KUNGFU_BIT    0x04
#define TS_PWUP_SUPERJUMP_BIT 0x100

bool CHudTSPowerUp::Init()
{
	HOOK_MESSAGE(PwUp);
	HOOK_MESSAGE(PwUpTime);
	HOOK_MESSAGE(TSSlowMo);
	HOOK_MESSAGE(TSBTime); // bullet-time variant flag (recoil x0.66667 consumer, ev_ts.cpp)
	gHUD.AddHudElem(this);
	m_iFlags |= HUD_ACTIVE; // self-gates on a held powerup
	Reset();
	return true;
}

bool CHudTSPowerUp::VidInit()
{
	// Reset the slow-motion state on connect / level-change — the original CHud::VidInit (@0x100206c0)
	// resets the slow CURRENT (DAT_100963e8) to 1.0 there. Our smoothed current g_flClientSlow is only
	// eased in Draw(), which does NOT run while spectating (press-fire-to-play), so without this a
	// stale <1 value from the previous session persists → the view.cpp slow-mo tint stays up on EVERY
	// reconnect (even with no players on), clearing only once you spawn and the HUD draws again.
	extern float g_flClientSlow;
	g_flClientSlow = 1.0f; // current (matches the original's VidInit reset)
	m_flSlowAmt = 100.0f;  // target  (ours isn't otherwise reset on connect — belt-and-suspenders)
	// The figures' sprites, loaded with the status sprites (0x10049d1f: +100 and +0x68).
	m_hKungFu = SPR_Load("sprites/player/kungfu.spr");
	m_hSuperJump = SPR_Load("sprites/player/movement.spr");
	return true;
}

// client.dll's status Reset (0x10049e70) clears the held powerup bits, the countdown end and the
// kung-fu and super-jump flags; the held seconds stay (nothing draws them while no bit is held).
void CHudTSPowerUp::Reset()
{
	m_iHeld = 0;
	m_flCountdownEnd = 0;
	m_bKungFu = false;
	m_bSuperJump = false;
	m_bSlowActive = false;
	m_flSlowAmt = 100.0f;
}

// PwUp (client.dll 0x1004b9e0): short type, byte seconds. A type with the kung-fu (4) or super-jump
// (0x100) bit only raises that figure's flag; any other type, 0 included, replaces the held bits and
// seconds.
bool CHudTSPowerUp::MsgFunc_PwUp(const char* pszName, int iSize, void* pbuf)
{
	BEGIN_READ(pbuf, iSize);
	const int type = READ_SHORT();
	const int secs = READ_BYTE();
	if ((type & (TS_PWUP_KUNGFU_BIT | TS_PWUP_SUPERJUMP_BIT)) == 0)
	{
		m_iHeld = type;
		m_iSeconds = secs;
	}
	else if ((type & TS_PWUP_KUNGFU_BIT) != 0)
		m_bKungFu = true;
	else
		m_bSuperJump = true;
	return true;
}

// PwUpTime (client.dll 0x1004bad0), sent on activation: the countdown ends that many seconds from now
// and the held seconds go to 0.
bool CHudTSPowerUp::MsgFunc_PwUpTime(const char* pszName, int iSize, void* pbuf)
{
	BEGIN_READ(pbuf, iSize);
	const int secs = READ_BYTE();
	m_iSeconds = 0;
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

	// Slow-mo INITIATION (RE FUN_1004bef0): the normal->slow transition plays goslow.wav. We
	// edge-detect entry here: slowing means m_flSlowAmt drops below ~normal. (The heartbeat is the
	// ammo HUD's Think, on the eased slow factor, as client.dll's @0x1004ce10.)
	const bool slowNow = m_flSlowAmt < 99.0f;
	if (slowNow && !m_bSlowActive)
	{
		// goslow.wav — play it the way the ORIGINAL does (RE FUN_1004bef0): EV_PlaySound on the local
		// player, channel 0, vol 1.0, attn 0.8, pitch 100. (PlaySoundByName wasn't reliably audible.)
		cl_entity_t* lp = gEngfuncs.GetLocalPlayer();
		if (lp)
			gEngfuncs.pEventAPI->EV_PlaySound(lp->index, lp->origin, CHAN_AUTO, "goslow.wav",
				1.0f, ATTN_NORM, 0, 100);
	}
	m_bSlowActive = slowNow;
	return true;
}

// Client CURRENT smoothed slow factor (0..1, init 1.0 = normal), eased toward the TSSlowMo target.
// RE: client DAT_100963e8 — consumed by the weapon-fire events to pitch the gunshot sounds (the more
// time is dilated, the deeper they sound). See ev_ts.cpp.
float g_flClientSlow = 1.0f;

// TSBTime — the bullet-time variant flag (server byte 0x175). Consumed by the recoil
// reduction in ev_ts.cpp (x0.66667 while in bullet-time).
bool g_bBulletTime = false;

bool CHudTSPowerUp::MsgFunc_TSBTime(const char* pszName, int iSize, void* pbuf)
{
	BEGIN_READ(pbuf, iSize);
	g_bBulletTime = READ_BYTE() != 0;
	m_iFlags |= HUD_ACTIVE;
	return true;
}

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

	// The status draw (0x10049ea0) draws these only while nohud is 0, not dead and not spectating.
	if (TSHud_NoHud() || TSHud_PlayerPartsHidden())
		return true;

	// The kung-fu and super-jump figures at the bottom centre, in two thirds of the HUD colour. client.dll
	// draws them at the end of the status panel (0x1004a3e0, flags +0x29 and +0x2a), past its cl_realhud
	// test.
	if (!TSHud_RealHud())
	{
		int hr, hg, hb;
		TSHud_Color(hr, hg, hb);
		const int py = ScreenHeight - 0x38;
		if (m_bKungFu)
		{
			SPR_Set(m_hKungFu, hr * 2 / 3, hg * 2 / 3, hb * 2 / 3);
			SPR_DrawAdditive(0, ScreenWidth / 2 - 0x30, py, nullptr);
		}
		if (m_bSuperJump)
		{
			SPR_Set(m_hSuperJump, hr * 2 / 3, hg * 2 / 3, hb * 2 / 3);
			SPR_DrawAdditive(0, ScreenWidth / 2, py, nullptr);
		}
	}

	// The held powerup's label (client.dll 0x1004a2e0, reached only while a bit is held; with none the
	// status draw clears the countdown end): at (8, 8) in (45, 195, 45), the name by the first held bit
	// of slow motion (1), infinite ammo (2), slow pause (8), double fire rate (0x10). Then, four pixels
	// on, the seconds left while the countdown runs (truncated), else the held seconds when not 0, two
	// digits wide.
	if (m_iHeld == 0)
	{
		m_flCountdownEnd = 0;
		return true;
	}
	const char* name = nullptr;
	if ((m_iHeld & 0x01) != 0)
		name = "#Pwr_SlowMotion";
	else if ((m_iHeld & 0x02) != 0)
		name = "#Pwr_InfiniteAmmo";
	else if ((m_iHeld & 0x08) != 0)
		name = "#Pwr_SlowPause";
	else if ((m_iHeld & 0x10) != 0)
		name = "#Pwr_DoubleFireRate";
	if (name != nullptr) // client.dll keeps its last label in a static buffer when no known bit is set
		strncpy(m_szLabel, CHudTextMessage::BufferedLocaliseTextString(name), sizeof(m_szLabel) - 1);
	const int x = TSHud_DrawString(8, 8, ScreenWidth, m_szLabel, 45, 195, 45);
	int secs;
	if (m_flCountdownEnd > gHUD.m_flTime)
		secs = (int)(m_flCountdownEnd - gHUD.m_flTime);
	else if (m_iSeconds != 0)
		secs = m_iSeconds;
	else
		return true;
	TSHud_DrawNumber(x + 4, 8, TSHN_DRAWZERO | TSHN_2DIGITS, secs, 45, 195, 45);
	return true;
}
