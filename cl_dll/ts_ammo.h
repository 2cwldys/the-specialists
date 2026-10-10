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
	void DrawKungFu(); // client.dll 0x1004d700
	void Reset() override;
	void Think() override; // the melee bar's regeneration, the heartbeat and the breathing (client.dll @0x1004ce10)
	void PlayLoop(int channel, const char* sample, float volume, int flags); // heartbeat1.wav or breathe2.wav
	bool MsgFunc_WeaponInfo(const char* pszName, int iSize, void* pbuf);
	bool MsgFunc_KFuPower(const char* pszName, int iSize, void* pbuf);
	// The predicted gun's clip, fire mode and attachments (CTSGun::UpdateClientData, client side).
	void GunUpdate(int clip, int fireMode, int equip);

private:
	int m_iWeaponType; // active tsweapon_t
	float m_flMeleeBar; // melee block stamina -100..100 (this+0x3700): KFuPower sets it, Think regenerates it
	int m_iLastFireMode = 0;     // this+0x7bc — the fire mode the switch sound last played for
	int m_iLastKeyBits = 0;      // this+0x3724 — the HUD key bits at the last think (the +use edge)
	float m_flUseDeadline = 0;   // this+0x3720 — a +use press before this time sends usepowerup
	int m_iHeartState = 0;       // this+0x3708 — the heartbeat: 0 off, 2 playing, 1 fading out
	float m_flHeartVolume = 0;   // this+0x370c — its volume (1.0, down 0.01 a frame while fading)
	int m_iBreathState = 0;      // this+0x3710 — the breathing: 0 off, 2 playing, 1 fading out
	float m_flBreathVolume = 0;  // this+0x3714 — its volume (0.7, down 0.005 a frame while fading)
	int m_iClip;       // rounds currently loaded
	int m_iReserve;    // spare rounds for this weapon's caliber
	int m_iFireMode;   // 0=auto 1=semi 2=burst 3=pump
	int m_iEquip;      // EQUIPPED attachment mask (WeaponInfo flags byte): silencer 1/laser 2/flash 4/scope 8
	// item_installed.spr, each attachment row's dot (full colour when active, half when not)
	HSPRITE m_hInstalled;
};
