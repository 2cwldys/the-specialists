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
// roundtime.h
//
// The Specialists round-time countdown HUD (reads the server's "RoundTime"
// message; see CTSGameRules::RestartRound / RoundTimeLeft on the server).
//

#pragma once

//
//-----------------------------------------------------
//
class CHudRoundTime : public CHudBase
{
public:
	bool Init() override;
	bool VidInit() override;
	bool Draw(float flTime) override;
	void Reset() override;
	bool MsgFunc_RoundTime(const char* pszName, int iSize, void* pbuf);

private:
	float m_flSeconds; // status +0x2c, the seconds left (drawn while not 0)
	float m_flNextTick; // status +0x30, when the next second comes off
	int m_iMode;       // status +0x34, the RoundTime mode byte: 0 counting, 1 ten times as fast, 2 held
};
