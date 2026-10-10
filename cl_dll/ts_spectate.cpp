/***
*  ts_spectate.cpp — The Specialists free-look spectate HUD (cont.65).
*  See ts_spectate.h. The "Spectator" message is hooked in hud.cpp and routed here.
***/
#include "hud.h"
#include "cl_util.h"
#include "parsemsg.h"
#include "ts_spectate.h"

#include <stdio.h>
#include "Exports.h" // CL_IsThirdPerson

// in_camera.cpp — the cl_thirdperson camera edges (below)
void CAM_ToThirdPerson();
void CAM_ToFirstPerson();

// Per-player spectator flags from the (idx, flag) broadcast — the ORIGINAL wire
// (client.dll FUN_10065250 parses exactly BYTE idx + BYTE flag into a per-player array).
static bool s_bPlayerSpectating[MAX_PLAYERS + 1];

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

// Is the LOCAL player spectating? The ORIGINAL architecture: the stock g_iUser1 (the observer
// mode from the networked entity state, entity.cpp HUD_TxferLocalOverrides) — NOT the Spectator
// user message. The message only maintains the per-player table (the stock g_IsSpectator shape);
// the local player's own state always rides iuser1. (Round-7c/7d: wire-latched local state was
// unreliable during connect; g_iUser1 cannot be.)
extern int g_iUser1;

static bool TS_LocalSpectating()
{
	return g_iUser1 != 0;
}

// The Spectator table entry for a player (client.dll 0x101900ec).
bool TS_PlayerSpectating(int idx)
{
	return idx >= 1 && idx <= MAX_PLAYERS && s_bPlayerSpectating[idx];
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

// (Fire-to-play is not here: the original sends "respawn" from the +attack press itself, through the
// TS body of CHudSpectator::HandleButtonsDown, hud_spectator.cpp. reTS polled +attack here once a frame
// behind a function-static 0.2 s clock that no map change reset and a buy-menu test the original has
// not got; see the ledger, 2026-10-10.)

bool CHudSpectate::Draw(float flTime)
{
	// Per-frame local-state derivation + edge detection (parse time is too early — see above).
	// The original applies the camera logic at Spectator parse time (client.dll FUN_10065250,
	// which re-runs on every local Spectator message); applying it on these SAME edges is
	// behaviorally equivalent.
	const bool spec = TS_LocalSpectating();
	if (spec && m_iState == 0)
	{
		// entering spectate-wait: always force FIRST person…
		if (0 != CL_IsThirdPerson())
			CAM_ToFirstPerson();
		// VERIFY: the original also calls FUN_10036670/FUN_10004600 here (unidentified —
		// camera/interp-save resets?). Not reconstructed; trace live if spectate entry glitches.
		gHUD.m_TSBuyMenu.Open(); // …and the buy/config menu comes up once
	}
	else if (!spec && m_iState != 0)
	{
		// entering play (spawning): cl_thirdperson = the preferred camera. BIT-exact tests
		// (client.dll 0x652c4/0x652db): only exactly 1.0 forces third person, only exactly
		// 0.0 forces first; any other value keeps the current camera.
		static cvar_t* s_cl_thirdperson = nullptr;
		if (!s_cl_thirdperson)
			s_cl_thirdperson = gEngfuncs.pfnGetCvarPointer("cl_thirdperson");
		if (s_cl_thirdperson)
		{
			const float v = s_cl_thirdperson->value;
			if (v == 1.0f)
			{
				if (0 == CL_IsThirdPerson())
					CAM_ToThirdPerson();
			}
			else if (v == 0.0f)
			{
				if (0 != CL_IsThirdPerson())
					CAM_ToFirstPerson();
			}
		}
		gHUD.m_TSBuyMenu.Close();
	}
	m_iState = spec ? 1 : 0;
	if (!spec)
		return true;

	// (The respawn countdown number + "Press Fire To Play!" arrive as server TSMessage lines
	// now — the original mechanism; nothing to draw here for them.)

	// (The bottom bar is the STOCK SpectatorPanel — the original re-skins the stock spectator
	// rather than drawing its own; our old custom orange bar was the invention. cont.84)

	return true;
}
