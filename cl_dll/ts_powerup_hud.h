/***
*  ts_powerup_hud.h — The Specialists held-powerup HUD: the green top-left timer that
*  shows the powerup you're holding + the seconds available, and the live countdown once
*  you double-tap +use to activate it.
*
*  Wire (dlls/ts_player.cpp): `PwUp` (3) = WriteShort(held type) + WriteByte(duration secs),
*  sent from UpdateClientData on change. `PwUpTime` (1) = WriteByte(seconds remaining), sent from
*  UsePowerUp on activation. client.dll keeps this state in its status element (0x100d43cc).
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
	bool MsgFunc_TSBTime(const char* pszName, int iSize, void* pbuf); // bullet-time variant flag

	int HeldFlags() const { return m_iHeld; } // the held powerup bits (client.dll status +0x14)

private:
	int m_iHeld = 0;              // +0x14 the held powerup bits from PwUp (0 = none)
	int m_iSeconds = 0;           // +0x18 the held seconds from PwUp, 0 once activated
	float m_flCountdownEnd = 0;   // +0x1c the countdown's end (PwUpTime), 0 when not counting
	char m_szLabel[64] = {};      // the label buffer (0x1018a5e0)
	float m_flSlowAmt;     // applied slow amount (TSSlowMo): 100 = normal, <100 = slowed
	bool m_bKungFu = false;    // +0x29 kung fu picked up (PwUp bit 4), cleared by Reset
	bool m_bSuperJump = false; // +0x2a super jump picked up (PwUp bit 0x100), cleared by Reset
	HSPRITE m_hKungFu = 0, m_hSuperJump = 0; // sprites/player/kungfu.spr, movement.spr
	// slow-mo INITIATION sound (RE client FUN_1004bef0): goslow.wav on the normal->slow transition.
	bool m_bSlowActive;       // currently in slow-mo (edge-detect the entry)
};

#endif // TS_POWERUP_HUD_H
