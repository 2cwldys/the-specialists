/***
*
*	Copyright (c) 1996-2002, Valve LLC. All rights reserved.
*
*	This product contains software technology licensed from Id
*	Software, Inc. ("Id Technology").  Id Technology (c) 1996 Id Software, Inc.
*	All Rights Reserved.
*
*   Use, distribution, and modification of this source code and/or resulting
*   object code is restricted to non-commercial enhancements to products from
*   Valve LLC.  All other use, distribution, or modification is prohibited
*   without written permission from Valve LLC.
*
****/
//
// ts_weaponbar.h
//
// The Specialists top weapon-select bar (reference Image #1). Reads the server's
// "WStatus" message ({type, owned, clip}) to track which arsenal weapons the player
// holds, and draws them in five columns by category. Geometry + column/row mapping
// recovered from FUN_1004e030 + each weapon's s_weapon_info +0x84 (column) / +0x85
// (row) bytes — see reference/HUD_DRAWCHAIN.md.
//

#pragma once

class CHudTSWeaponBar : public CHudBase
{
public:
	bool Init() override;
	bool VidInit() override;
	bool Draw(float flTime) override;
	void Think() override; // FIRE-commits the highlighted weapon while the select menu is open
	void Reset() override;
	bool MsgFunc_WStatus(const char* pszName, int iSize, void* pbuf);
	bool MsgFunc_ClipInfo(const char* pszName, int iSize, void* pbuf); // 1 byte: active weapon's clip cell
	bool MsgFunc_Wires(const char* pszName, int iSize, void* pbuf); // 3 bytes: nearby-bomb wire roles

	unsigned char m_wires[3] = {}; // +0x36fc-0x36fe — the wire ROLE codes (0 = no bomb nearby)

	// The weapon-select menu (the original's hud_fastswitch-0 model — reference/WEAPON_SELECT.md):
	// the bar is HIDDEN until the player presses invnext/invprev or slot1-5, which OPEN it and move
	// a highlight cursor through the owned arsenal; pressing FIRE commits the highlighted weapon
	// (`weapon_<N>` -> server holster->deploy). It times out / cancels if no key is pressed.
	void Step(bool fNext);     // invnext/invprev: open + move the cursor to the next/prev owned weapon
	void SelectColumn(int col); // slot1-5: open + jump to (or advance within) HUD column 1..5
	void Commit();              // FIRE: send `weapon_<N>` for the cursor, close the menu
	void CloseSelect();
	bool IsSelecting() const { return m_selecting; }

private:
	void OpenSelect();          // seed the cursor from the active weapon, mark selecting
	int FindOwned(int from, bool fNext, int colOnly) const; // next/prev owned in display order (colOnly<0 = any)

	static const int TSW_COUNT = 38;
	int m_iClip[TSW_COUNT];          // loaded rounds if owned, else -1
	HSPRITE m_hIcon[TSW_COUNT];      // sprites/weapons/<name>.spr handle (0 if none)
	Rect m_rcIcon[TSW_COUNT];        // full-sprite rect

	bool m_selecting = false;        // the select menu is open (this+0x73c)
	int m_selType = -1;              // highlighted tsweapon_t (the cursor)
	float m_selTime = 0;             // last input time (for the auto-close timeout)
	bool m_swallowAttack = false;    // after a FIRE-commit, ignore +attack until released (no stray shot)
};
