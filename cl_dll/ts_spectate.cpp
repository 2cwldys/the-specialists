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

// Spectator (client.dll 0x10065250): BYTE playerIndex (1..64), BYTE flag (1 = spectating, 0 =
// playing); the respawn countdown and "Press Fire To Play!" arrive as TSMessage texts. Parse stores
// the table and clears that player's ActItems byte, as client.dll does; the local player's camera,
// recoil and buy menu consequences run on the edges in Draw, where the local index is reliable.
int CHudSpectate::MsgFunc_Spectator(const char* pszName, int iSize, void* pbuf)
{
	BEGIN_READ(pbuf, iSize);
	const int idx = READ_BYTE();
	const int flag = READ_BYTE();
	if (idx >= 1 && idx <= MAX_PLAYERS)
		s_bPlayerSpectating[idx] = (flag != 0);
	extern void TS_ClearPlayerAttach(int idx);
	if (idx >= 1 && idx <= 0x40)
		TS_ClearPlayerAttach(idx);
	return 1;
}

// (Fire-to-play is not here: the original sends "respawn" from the +attack press itself, through the
// TS body of CHudSpectator::HandleButtonsDown, hud_spectator.cpp. reTS polled +attack here once a frame
// behind a function-static 0.2 s clock that no map change reset and a buy-menu test the original has
// not got; see the ledger, 2026-10-10.)

bool CHudSpectate::Draw(float flTime)
{
	// The local player's Spectator consequences (0x10065290..0x10065300), on the edges of his state:
	// entering spectate the camera leaves third person through the thirdperson toggle and the buy
	// menu opens (0x10036670); entering play cl_thirdperson exactly 1 turns third person on and exactly
	// 0 turns it off, any other value keeps the camera. Both reset the predicted recoil (0x10004600).
	const bool spec = TS_LocalSpectating();
	extern void HUD_TSResetRecoil();
	if (spec && m_iState == 0)
	{
		if (0 != CL_IsThirdPerson())
			CAM_ToThirdPerson();
		gHUD.m_TSBuyMenu.Open();
		HUD_TSResetRecoil();
	}
	else if (!spec && m_iState != 0)
	{
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
					CAM_ToThirdPerson();
			}
		}
		HUD_TSResetRecoil();
	}
	m_iState = spec ? 1 : 0;
	if (!spec)
		return true;

	// (The respawn countdown number + "Press Fire To Play!" arrive as server TSMessage lines
	// now — the original mechanism; nothing to draw here for them.)

	// (The bottom bar is the stock SpectatorPanel, reskinned, as client.dll's.)

	return true;
}
