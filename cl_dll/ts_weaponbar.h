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
	void Reset() override;
	bool MsgFunc_WStatus(const char* pszName, int iSize, void* pbuf);

private:
	static const int TSW_COUNT = 38;
	int m_iClip[TSW_COUNT]; // loaded rounds if owned, else -1
};
