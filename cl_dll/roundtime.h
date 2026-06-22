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
// message; see CTSGameRules::StartRoundTimer / RoundTimeLeft on the server).
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
	float m_flEndTime; // client-time at which the round runs out (0 = no active timer)
};
