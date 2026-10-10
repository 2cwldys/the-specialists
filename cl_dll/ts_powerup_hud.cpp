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
#include "cl_entity.h"  // cl_entity_t (GetLocalPlayer)
#include "event_api.h"  // pEventAPI->EV_PlaySound (goslow.wav, the original's mechanism)
#include "ts_powerup_hud.h"

#include <stdio.h>

DECLARE_MESSAGE(m_TSPowerUp, PwUp)
DECLARE_MESSAGE(m_TSPowerUp, PwUpTime)
DECLARE_MESSAGE(m_TSPowerUp, TSSlowMo)
DECLARE_MESSAGE(m_TSPowerUp, TSBTime)

// held powerup type bits (dlls/ts_gamerules.h TS_PWUP_*)
#define PWUP_SLOWMO    0x01
#define PWUP_SLOWPAUSE 0x08
#define PWUP_DBLFIRE   0x10
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
	return true;
}

void CHudTSPowerUp::Reset()
{
	m_iHeldType = 0;
	m_iSecondsAvail = 0;
	m_bActive = false;
	m_bKungFu = false;    // per-life independent powerups clear on respawn (ResetHUD)
	m_bSuperJump = false;
	m_bSlowActive = false;
	m_flCountdownEnd = 0;
	m_flSlowAmt = 100.0f;
}

// PwUp — the held powerup state (type + seconds available). type 0 = nothing held.
bool CHudTSPowerUp::MsgFunc_PwUp(const char* pszName, int iSize, void* pbuf)
{
	BEGIN_READ(pbuf, iSize);
	int type = READ_SHORT();
	int dur = READ_BYTE();
	// The INDEPENDENT persistent powerups (kung-fu = bit 4, super-jump/low-grav = bit 0x100) come through
	// PwUp with dur 0 — they're NOT the held timed slot; latch a bottom-center indicator that persists
	// until respawn (Reset). They coexist with a held slow/double powerup.
	if (type == TS_PWUP_KUNGFU_BIT)  { m_bKungFu = true; return true; }
	if (type == TS_PWUP_SUPERJUMP_BIT) { m_bSuperJump = true; return true; }

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

// Draw one independent-powerup indicator centred horizontally at row `y`: the named sprite (additive,
// drawn at its natural colour) if it loads, otherwise a whitish text label. It's a binary "have it /
// don't" indicator — NOT tinted green like the timed slow-powerup HUD. The original shows
// sprites/weapons/<n>.spr here; that file isn't in this (partial) game-data extraction, so the by-name
// SPR_Load returns 0 and we fall back to a label. (When the .spr is present it draws as the sprite.)
static void DrawIndependentPowerup(const char* spriteName, const char* label, int x, int y)
{
	const int wr = 200, wg = 200, wb = 200; // soft white
	HSPRITE spr = SPR_Load(spriteName);
	if (spr)
	{
		SPR_Set(spr, wr, wg, wb);
		SPR_DrawAdditive(0, x, y, NULL); // NULL rect = the whole sprite frame; x = sprite top-left
	}
	else
	{
		int tw, th;
		GetConsoleStringSize(label, &tw, &th);
		TSHud_DrawString(x, y, ScreenWidth, label, wr, wg, wb);
	}
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

	// INDEPENDENT persistent powerups (kung-fu / super-jump): two bottom-center FIGURE indicators, shown
	// the whole life once picked up and coexisting with any held slow/double powerup. RE (status-HUD
	// dossier): kung-fu = sprites/player/kungfu.spr @ (SW/2-48, SH-56); super-jump = the "movement" figure
	// sprites/player/movement.spr @ (SW/2, SH-56) — side by side. (The old sprites/weapons/kungfu.spr was
	// the weapon-bar icon and sprites/weapons/superjump.spr doesn't exist — SPR_Load returned a stale
	// handle, so it drew the last weapon/numbers sprite.)
	// client.dll draws these two inside the status draw (0x1004a3e0, flags +0x29/+0x2a), after its
	// cl_realhud test, so they go with the status panel.
	if (!TSHud_RealHud() && !TSHud_NoHud())
	{
		const int py = ScreenHeight - 56;
		if (m_bKungFu)    DrawIndependentPowerup("sprites/player/kungfu.spr", "Kung-Fu", ScreenWidth / 2 - 48, py);
		if (m_bSuperJump) DrawIndependentPowerup("sprites/player/movement.spr", "Super Jump", ScreenWidth / 2, py);
	}

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

	// the label is client.dll's 0x1004a2e0, drawn from its status draw after the nohud test (0x10049f06)
	if (TSHud_NoHud())
		return true;

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
	// seconds available/remaining in the numbers.spr digit font (centred on the text row).
	TSHud_DrawNumber(112, y + (gHUD.m_scrinfo.iCharHeight - TS_NUMBER_H) / 2, secs, 96, 255, 96);
	return true;
}
