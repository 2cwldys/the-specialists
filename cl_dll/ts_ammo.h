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
// ts_ammo.h
//
// The Specialists ammo HUD: current clip / reserve / fire-mode for the held TS
// weapon. Reads the server's "WeaponInfo" user message (6 bytes, byte-faithful to
// the original CTSGun::UpdateClientData — see reference/HUD_MESSAGES.md). TS has no
// stock CurWeapon/Ammo HUD; this is the custom replacement.
//

#pragma once

class CHudTSAmmo : public CHudBase
{
public:
	bool Init() override;
	bool VidInit() override;
	bool Draw(float flTime) override;
	void Reset() override;
	void Think() override; // the melee bar's own regeneration (client.dll @0x1004ce10)
	bool MsgFunc_WeaponInfo(const char* pszName, int iSize, void* pbuf);
	bool MsgFunc_KFuPower(const char* pszName, int iSize, void* pbuf);
	// The predicted gun's clip, fire mode and attachments (CTSGun::UpdateClientData, client side).
	void GunUpdate(int clip, int fireMode, int equip);

private:
	int m_iWeaponType; // active tsweapon_t
	float m_flMeleeBar; // melee block stamina -100..100 (this+0x3700): KFuPower sets it, Think regenerates it
	int m_iClip;       // rounds currently loaded
	int m_iReserve;    // spare rounds for this weapon's caliber
	int m_iFireMode;   // 0=auto 1=semi 2=burst 3=pump
	int m_iEquip;      // EQUIPPED attachment mask (WeaponInfo flags byte): silencer 1/laser 2/flash 4/scope 8
	// The attachment-list dot, far-right of each row: item_installed.spr when the attachment is ACTIVE,
	// item_not_installed.spr when equipped-but-off (the original's on/off indicator).
	HSPRITE m_hInstalled;
	HSPRITE m_hNotInstalled;
	Rect m_rcInstalled;
	Rect m_rcNotInstalled;
};
