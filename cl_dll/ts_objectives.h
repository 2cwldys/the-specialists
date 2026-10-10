/***
*  ts_objectives.h — The Specialists objective/flag HUD (TSFlag / Objective / ShowObj /
*  TSHVoice user messages).
*
*  RE (reference/dossiers/cont94/client_message_hooks.md §4-7): the original hooks all four
*  on ONE statically-allocated hud element (the TS master element @ client.dll+0xd43cc,
*  "CHudTS") and draws both HUDs from its Draw (flag rows FUN_1004b0e0, objective markers
*  FUN_10049fc0). We mirror that: one element owning the 16-entry flag table (@+0x146c,
*  stride 0xc) and the 32-entry objective table (@+0xa30, stride 0x50).
***/
#pragma once

class CHudTSObjectives : public CHudBase
{
public:
	bool Init() override;
	bool VidInit() override;
	bool Draw(float flTime) override;
	void Reset() override;
	bool MsgFunc_TSFlag(const char* pszName, int iSize, void* pbuf);    // 4 bytes: action, flagID, team+1, carrier
	bool MsgFunc_Objective(const char* pszName, int iSize, void* pbuf); // 3 COORDs + BYTE idx + STRING label
	bool MsgFunc_ShowObj(const char* pszName, int iSize, void* pbuf);   // BYTE idx, BYTE show
	bool MsgFunc_TSHVoice(const char* pszName, int iSize, void* pbuf);  // BYTE 0 successful / 1 failed

private:
	// Flag table (original @element+0x146c, 16 entries stride 0xc), indexed by flagID.
	struct flagentry_t
	{
		int action;  // 0 at base / 1 picked up / 2 dropped
		int carrier; // carrier entindex (only while action == 1, else 0)
		int team;    // the wire team+1 value (1..4)
	};
	flagentry_t m_Flags[16];
	float m_flBlinkFast = 0, m_flBlinkSlow = 0; // +0x1530, +0x1534: the flag icons' blink clocks
	bool m_bBlinkFast = false, m_bBlinkSlow = false; // +0x152c, +0x152d

	// Objective table (original @element+0xa30, 32 entries stride 0x50), indexed by idx.
	struct objentry_t
	{
		bool visible;      // +0x00
		float x, y, z;     // +0x04
		char label[0x40];  // +0x10
	};
	objentry_t m_Objectives[32];
	int m_iMaxObjIdx; // +0x1430 — highest idx received (draw bound)

	HSPRITE m_hFlag;      // sprites/player/flag.spr  (+0x6c)
	HSPRITE m_hCompass;   // sprites/compass.spr      (+0x1438)
	HSPRITE m_hUpArrow;   // sprites/uparrow.spr      (+0x143c)
	HSPRITE m_hDownArrow; // sprites/downarrow.spr    (+0x1440)
	Rect m_rcFlag, m_rcCompass, m_rcUpArrow, m_rcDownArrow;
};
