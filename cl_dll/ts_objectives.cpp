/***
*  ts_objectives.cpp — The Specialists objective/flag HUD. See ts_objectives.h.
*
*  RE (reference/dossiers/cont94/client_message_hooks.md §4-7), all recovered from
*  reference/client.dll:
*    TSFlag   handler FUN_1004ba50 · draw FUN_1004b0e0  (per-team flag rows, right edge)
*    Objective handler FUN_1004bb70 · draw FUN_10049fc0 (compass.spr world markers + labels)
*    ShowObj  handler FUN_1004bc80  (visible bit on the objective table)
*    TSHVoice handler FUN_1004c180  (misc/successful.wav / misc/failed.wav)
*  The original draws flag + objective HUDs from ONE element (the TS master "CHudTS"
*  @0x100d43cc); this element is our split of that object.
***/
#include "hud.h"
#include "cl_util.h"
#include "parsemsg.h"
#include "cl_entity.h"   // cl_entity_t (GetLocalPlayer)
#include "triangleapi.h" // pTriAPI->WorldToScreen (objective marker projection)
#include "ts_hud.h"      // TSHud_Color / TSHud_DrawString
#include "player_info.h" // g_PlayerInfoList (carrier name)
#include "vgui_TeamFortressViewport.h" // iTeamColors
#include "ts_objectives.h"

#include <string.h>

DECLARE_MESSAGE(m_TSObjectives, TSFlag)
DECLARE_MESSAGE(m_TSObjectives, Objective)
DECLARE_MESSAGE(m_TSObjectives, ShowObj)
DECLARE_MESSAGE(m_TSObjectives, TSHVoice)

bool CHudTSObjectives::Init()
{
	memset(m_Flags, 0, sizeof(m_Flags));
	memset(m_Objectives, 0, sizeof(m_Objectives));
	m_iMaxObjIdx = 0;

	HOOK_MESSAGE(TSFlag);
	HOOK_MESSAGE(Objective);
	HOOK_MESSAGE(ShowObj);
	HOOK_MESSAGE(TSHVoice);

	gHUD.AddHudElem(this);
	return true;
}

// Load one sprite and capture its full-frame rect; 0 if absent (Draw skips it).
static HSPRITE LoadObjSprite(const char* path, Rect& rc)
{
	HSPRITE h = SPR_Load(path);
	rc = {0, 0, 0, 0};
	if (h != 0)
	{
		rc.right = SPR_Width(h, 0);
		rc.bottom = SPR_Height(h, 0);
	}
	return h;
}

bool CHudTSObjectives::VidInit()
{
	// The original loads these in the TS element's VidInit (@0x49d40-0x49dd3):
	// +0x6c player/flag.spr, +0x1438 compass.spr, +0x143c uparrow.spr, +0x1440 downarrow.spr.
	m_hFlag = LoadObjSprite("sprites/player/flag.spr", m_rcFlag);
	m_hCompass = LoadObjSprite("sprites/compass.spr", m_rcCompass);
	m_hUpArrow = LoadObjSprite("sprites/uparrow.spr", m_rcUpArrow);
	m_hDownArrow = LoadObjSprite("sprites/downarrow.spr", m_rcDownArrow);
	// The tables persist across the reload — the server re-sends the objective list on join,
	// and Objective idx 0 doubles as the whole-table reset (the original clears nothing here).
	return true;
}

void CHudTSObjectives::Reset()
{
	m_iFlags |= HUD_ACTIVE;
}

// TSFlag (FUN_1004ba50): BYTE action (0 base-spawn / 1 pickup / 2 drop), BYTE flagID,
// BYTE team+1, BYTE carrier. Guard 0 < flagID < 16 (the correct && form).
bool CHudTSObjectives::MsgFunc_TSFlag(const char* pszName, int iSize, void* pbuf)
{
	BEGIN_READ(pbuf, iSize);
	const int action = READ_BYTE();
	const int flagID = READ_BYTE();
	const int teamPlus1 = READ_BYTE();
	const int carrier = READ_BYTE();

	if (flagID > 0 && flagID < 16)
	{
		m_Flags[flagID].action = action;
		m_Flags[flagID].carrier = (action == 1) ? carrier : 0; // carrier only while picked up
		m_Flags[flagID].team = teamPlus1;
	}
	m_iFlags |= HUD_ACTIVE;
	return true;
}

// Objective (FUN_1004bb70): COORD x, COORD y, COORD z, BYTE idx, STRING label (buffered to 64).
// idx == 0 clears the whole table (the server always sends the list in order, so entry 0
// doubles as the reset).
bool CHudTSObjectives::MsgFunc_Objective(const char* pszName, int iSize, void* pbuf)
{
	BEGIN_READ(pbuf, iSize);
	const float x = READ_COORD();
	const float y = READ_COORD();
	const float z = READ_COORD();
	const int idx = READ_BYTE();
	const char* label = READ_STRING();

	if (idx == 0)
	{
		memset(m_Objectives, 0, sizeof(m_Objectives));
		m_iMaxObjIdx = 0;
	}
	// The original's bounds check is `(idx > -1) || (idx < 0x20)` — always true (an original
	// `||` bug). Written as && here: drop-in identical for well-formed servers.
	if (idx > -1 && idx < 32)
	{
		objentry_t& e = m_Objectives[idx];
		e.visible = true;
		e.x = x;
		e.y = y;
		e.z = z;
		strncpy(e.label, label, sizeof(e.label) - 1);
		e.label[sizeof(e.label) - 1] = 0;
		if (idx > m_iMaxObjIdx)
			m_iMaxObjIdx = idx;
	}
	m_iFlags |= HUD_ACTIVE;
	return true;
}

// ShowObj (FUN_1004bc80): BYTE idx, BYTE show — the visible bit on the objective table.
// (Same always-true `||` bounds quirk in the original; && here.)
bool CHudTSObjectives::MsgFunc_ShowObj(const char* pszName, int iSize, void* pbuf)
{
	BEGIN_READ(pbuf, iSize);
	const int idx = READ_BYTE();
	const int show = READ_BYTE();
	if (idx > -1 && idx < 32)
		m_Objectives[idx].visible = (show != 0);
	m_iFlags |= HUD_ACTIVE;
	return true;
}

// TSHVoice (FUN_1004c180): BYTE 0 -> misc/successful.wav, 1 -> misc/failed.wav, vol 1.0;
// anything else -> nothing (still sets HUD_ACTIVE).
bool CHudTSObjectives::MsgFunc_TSHVoice(const char* pszName, int iSize, void* pbuf)
{
	BEGIN_READ(pbuf, iSize);
	const int v = READ_BYTE();
	if (v == 0)
		gEngfuncs.pfnPlaySoundByName("misc/successful.wav", 1.0f);
	else if (v == 1)
		gEngfuncs.pfnPlaySoundByName("misc/failed.wav", 1.0f);
	m_iFlags |= HUD_ACTIVE;
	return true;
}

// Pixel width of a HUD string (charWidths sum, up to a newline).
static int ObjStringWidth(const char* str)
{
	int w = 0;
	for (const char* p = str; *p != 0 && *p != '\n'; p++)
		w += gHUD.m_scrinfo.charWidths[(unsigned char)*p];
	return w;
}

bool CHudTSObjectives::Draw(float flTime)
{
	if ((m_iFlags & HUD_ACTIVE) == 0)
		return true;

	// The status draw (0x10049ea0) runs the flags and the objectives only while nohud is 0.
	if (TSHud_NoHud())
		return true;

	int hr, hg, hb;
	TSHud_Color(hr, hg, hb);

	cl_entity_t* lp = gEngfuncs.GetLocalPlayer();
	const int localIdx = (lp != nullptr) ? lp->index : 0;

	// ---- Flag HUD (client.dll 0x1004b0e0), from the status draw whatever the player's state. Two
	// blink clocks run down by the frame time times the slow motion factor: every 0.1 s and every 0.5 s
	// each flips its state. For each flag id away from base (action not 0) of team 1..4: flag.spr at
	// (ScreenWidth - 56, team x 48 + 40) in the team's colour x 0.75, x 0.5 more while its clock's state
	// is off (the fast clock for a carried flag, action 1, the slow one otherwise). A carried flag also
	// shows its carrier's name (24 characters) ending at the icon, (40, 175, 40) for a carrier on another
	// team than the local player's and (175, 40, 40) for one on his, and to the local carrier "Take the
	// Briefcase to the base!" at (ScreenWidth - its width - 32, 320) in the HUD colour with red + 35. ----
	{
		extern float g_flClientSlow;
		const float dt = (float)gHUD.m_flTimeDelta * (g_flClientSlow < 0.01f ? 0.01f : g_flClientSlow);
		m_flBlinkFast -= dt;
		m_flBlinkSlow -= dt;
		if (m_flBlinkFast < 0.0f)
		{
			m_flBlinkFast = 0.1f;
			m_bBlinkFast = !m_bBlinkFast;
		}
		if (m_flBlinkSlow < 0.0f)
		{
			m_flBlinkSlow = 0.5f;
			m_bBlinkSlow = !m_bBlinkSlow;
		}
	}
	const int localTeam = g_PlayerExtraInfo[localIdx].teamnumber;
	for (int id = 1; id < 16; id++)
	{
		const flagentry_t& f = m_Flags[id];
		if (f.action == 0 || f.team < 1 || f.team > 4)
			continue;

		const int iconX = ScreenWidth - 0x38;
		const int iconY = f.team * 0x30 + 0x28;
		const bool lit = (f.action == 1) ? m_bBlinkFast : m_bBlinkSlow;
		const float k = (lit ? 1.0f : 0.5f) * 0.75f;
		const int tc = f.team < iNumberOfTeamColors ? f.team : 0;
		SPR_Set(m_hFlag, (int)(iTeamColors[tc][0] * k), (int)(iTeamColors[tc][1] * k), (int)(iTeamColors[tc][2] * k));
		SPR_DrawAdditive(0, iconX, iconY, nullptr);

		if (f.carrier > 0 && f.carrier < 0x41)
		{
			if (f.carrier == localIdx)
			{
				const char* prompt = "Take the Briefcase to the base!";
				const int pr = hr + 0x23 > 0xff ? 0xff : hr + 0x23;
				TSHud_DrawString(ScreenWidth - ObjStringWidth(prompt) - 0x20, 0x140, ScreenWidth, prompt, pr, hg, hb);
			}
			char name[25] = "";
			if (g_PlayerInfoList[f.carrier].name != nullptr)
				strncpy(name, g_PlayerInfoList[f.carrier].name, 24);
			name[24] = 0;
			const bool other = g_PlayerExtraInfo[f.carrier].teamnumber != localTeam;
			TSHud_DrawString(iconX - ObjStringWidth(name), iconY, ScreenWidth, name,
				(int)((other ? 0x2d : 0xc3) * 0.9f), (int)((other ? 0xc3 : 0x2d) * 0.9f), 0x28);
		}
	}

	// ---- Objective HUD (client.dll 0x10049fc0): the status draw runs it when the GameMode byte is
	// above 1 and the player is neither dead for the HUD nor spectating. ----
	if (gHUD.m_iTeamplay > 1 && !TSHud_PlayerPartsHidden())
	{
		const int mr = 100, mg = 100, mb = 0xcd; // the marker/label color (100,100,205)
		for (int i = 0; i <= m_iMaxObjIdx && i < 32; i++)
		{
			const objentry_t& e = m_Objectives[i];
			if (!e.visible || e.label[0] == 0)
				continue;

			Vector pos(e.x, e.y, e.z);
			float screen[3];
			if (0 != gEngfuncs.pTriAPI->WorldToScreen((float*)&pos, screen))
				continue; // behind the view -> offscreen (the original's -1)

			const int sx = (int)((1.0f + screen[0]) * 0.5f * (float)ScreenWidth);
			const int sy = (int)((1.0f - screen[1]) * 0.5f * (float)ScreenHeight);

			if (m_hCompass != 0)
			{
				SPR_Set(m_hCompass, mr, mg, mb);
				SPR_DrawAdditive(0, sx - 16, sy - 16, &m_rcCompass);
			}

			// Vertical cue against the player's origin z (CHud m_vecOrigin) +- 72: higher -> uparrow,
			// lower -> downarrow.
			const float zdelta = e.z - gHUD.m_vecOrigin[2];
			if (zdelta > 72.0f && m_hUpArrow != 0)
			{
				SPR_Set(m_hUpArrow, mr, mg, mb);
				SPR_DrawAdditive(0, sx - 16, sy - 0x30, &m_rcUpArrow);
			}
			else if (zdelta < -72.0f && m_hDownArrow != 0)
			{
				SPR_Set(m_hDownArrow, mr, mg, mb);
				SPR_DrawAdditive(0, sx - 16, sy + 0x10, &m_rcDownArrow);
			}

			TSHud_DrawString(sx + 0x10, sy - 8, ScreenWidth, e.label, mr, mg, mb);
		}
	}

	return true;
}
