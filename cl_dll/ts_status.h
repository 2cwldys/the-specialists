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
// ts_status.h
//
// The Specialists player-status HUD: health, armor and cash. TS ships a fully
// custom client HUD (its sprites/hud.txt has no stock health/battery layout), so
// the original networks its own TSHealth / TSArmor / TSCash user messages from
// CBasePlayer::UpdateClientData and draws them here rather than via the stock
// CHudHealth/CHudBattery. See reference/HUD_MESSAGES.md for the wire formats and
// dlls/ts_player.cpp (CBasePlayer::TSUpdateClientData) for the server side.
//

#pragma once

class CHudTSStatus : public CHudBase
{
public:
	bool Init() override;
	bool VidInit() override;
	bool Draw(float flTime) override;
	void Reset() override;
	bool MsgFunc_TSHealth(const char* pszName, int iSize, void* pbuf);
	bool MsgFunc_TSArmor(const char* pszName, int iSize, void* pbuf);
	bool MsgFunc_TSCash(const char* pszName, int iSize, void* pbuf);

private:
	int m_iHealth; // last TSHealth byte received
	int m_iArmor;  // last TSArmor byte received
	int m_iCash;   // last TSCash short received

	// Custom TS status-panel sprites (sprites/player/*.spr), loaded in VidInit and
	// drawn bottom-left in Draw — the posture figure (stand/crouched/prone), the
	// heart by the health number, and the Kevlar (armor) indicator. RE'd from
	// client.dll's sprite-load block @0x10049d1f (slots stand=0x38, crouched=0x3c,
	// prone=0x40, health=0x58, kevlar=0x5c off the HUD object).
	HSPRITE m_hStand, m_hCrouch, m_hProne, m_hHeart, m_hKevlar;
	Rect m_rcStand, m_rcCrouch, m_rcProne, m_rcHeart, m_rcKevlar;
};
