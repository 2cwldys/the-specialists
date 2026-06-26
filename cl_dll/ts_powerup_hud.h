/***
*  ts_powerup_hud.h — The Specialists held-powerup HUD: the green top-left timer that
*  shows the powerup you're holding + the seconds available, and the live countdown once
*  you double-tap +use to activate it.
*
*  Wire (dlls/ts_player.cpp): `PwUp` (3) = WriteShort(held type) + WriteByte(duration secs),
*  sent from UpdateClientData on change — the held icon + available seconds. `PwUpTime` (1) =
*  WriteByte(seconds remaining), sent from UsePowerUp on activation — starts the local countdown.
***/
#ifndef TS_POWERUP_HUD_H
#define TS_POWERUP_HUD_H

class CHudTSPowerUp : public CHudBase
{
public:
	bool Init() override;
	bool VidInit() override;
	void Reset() override;
	bool Draw(float flTime) override;
	bool MsgFunc_PwUp(const char* pszName, int iSize, void* pbuf);
	bool MsgFunc_PwUpTime(const char* pszName, int iSize, void* pbuf);
	bool MsgFunc_TSSlowMo(const char* pszName, int iSize, void* pbuf);

private:
	int m_iHeldType;       // held powerup type bits (0 = none held)
	int m_iSecondsAvail;   // seconds available (from PwUp) before activation
	bool m_bActive;        // activated (counting down)
	float m_flCountdownEnd; // local end time while active
	float m_flSlowAmt;     // applied slow amount (TSSlowMo): 100 = normal, <100 = slowed → green overlay
};

#endif // TS_POWERUP_HUD_H
