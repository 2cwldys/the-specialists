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
#include "pm_shared.h"   // OBS_IN_EYE
#include "ts_hud.h"      // TSHud_Color / TSHud_DrawString
#include "player_info.h" // g_PlayerInfoList (carrier name)
#include "ts_objectives.h"

#include <string.h>

extern Vector v_origin; // view origin (cl_dll/view.cpp) — the local view z for the up/down cue

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

	// The original master Draw gates everything on the nohud-ish cvar (0x100df50c) == 0.
	{
		static cvar_t* s_nohud = nullptr;
		if (s_nohud == nullptr)
			s_nohud = gEngfuncs.pfnGetCvarPointer("nohud");
		if (s_nohud != nullptr && s_nohud->value != 0.0f)
			return true;
	}

	int hr, hg, hb;
	TSHud_Color(hr, hg, hb);

	cl_entity_t* lp = gEngfuncs.GetLocalPlayer();
	const int localIdx = (lp != nullptr) ? lp->index : 0;

	// ---- Flag HUD (FUN_1004b0e0): one row per team at the right edge, only for flags
	// AWAY from base (action != 0). ----
	for (int id = 1; id < 16; id++)
	{
		const flagentry_t& f = m_Flags[id];
		if (f.action == 0 || f.team < 1 || f.team > 4)
			continue;

		const int iconX = ScreenWidth - 0x38;
		const int iconY = f.team * 0x30 + 0x28;
		if (m_hFlag != 0)
		{
			SPR_Set(m_hFlag, hr, hg, hb);
			SPR_DrawAdditive(0, iconX, iconY, &m_rcFlag);
		}

		// Carrier name (client player-info array, strncpy 24) left of the icon.
		if (f.carrier > 0 && f.carrier <= MAX_PLAYERS_HUD)
		{
			gEngfuncs.pfnGetPlayerInfo(f.carrier, &g_PlayerInfoList[f.carrier]);
			if (g_PlayerInfoList[f.carrier].name != nullptr)
			{
				char name[25];
				strncpy(name, g_PlayerInfoList[f.carrier].name, 24);
				name[24] = 0;
				// x = iconX - stringwidth - 0x20 (the original's exact gap was decompiler-elided
				// — "0x20-ish" in the dossier). VERIFY at playtest.
				TSHud_DrawString(iconX - ObjStringWidth(name) - 0x20, iconY, iconX, name, hr, hg, hb);
			}

			// The local player carries this flag: the centered carry prompt at y = 0x140.
			if (f.carrier == localIdx)
			{
				const char* prompt = "Take the Briefcase to the base!";
				TSHud_DrawString((ScreenWidth - ObjStringWidth(prompt)) / 2, 0x140,
					ScreenWidth, prompt, hr, hg, hb);
			}
		}
		// Blink state: the original updates two countdowns here (+0x1530/+0x1534, reset
		// 0.1/0.5 s, toggling bools +0x152c/+0x152d, decrement m_flTimeDelta/timescale);
		// their exact visual coupling was decompiler-elided (likely icon/text blink).
		// VERIFY at playtest — not reconstructed yet.
	}

	// ---- Objective HUD (FUN_10049fc0): drawn only from a first-person view (the original
	// gates it behind FirstPersonState/spectator checks @0x1004c250/0x1004c280). ----
	if (g_iUser1 == 0 || g_iUser1 == OBS_IN_EYE)
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

			// Vertical cue vs the local view z ± 72: entry higher -> uparrow, lower -> downarrow.
			const float zdelta = e.z - v_origin.z;
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
