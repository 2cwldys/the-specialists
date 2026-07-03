/***
*  ts_spectate.h — The Specialists free-look spectate HUD (cont.65).
*
*  Reads the server's "Spectator" user message (gmsgSpectator, 2 bytes: state,
*  secondsLeft — see dlls/player.cpp TSSpectateThink) and draws the join/respawn
*  free-look overlay: a centre-screen countdown (5..1) that becomes "Press fire to
*  play" once the spawn gate elapses, plus the free-look control help line. The
*  player is held in free-look by the server until they pick a loadout + press fire.
***/
#pragma once

class CHudSpectate : public CHudBase
{
public:
	bool Init() override;
	bool VidInit() override;
	bool Draw(float flTime) override;
	void Reset() override;
	int MsgFunc_Spectator(const char* pszName, int iSize, void* pbuf);

	bool IsActive() const; // computed from the (idx,flag) table + the LIVE local index (lazy —
	                       // the first Spectator broadcast can arrive before GetLocalPlayer works)

private:
	int m_iState;   // TSSPEC_OFF/COUNTDOWN/READY (matches dlls/player.h)
	int m_iSeconds; // countdown seconds remaining (state COUNTDOWN)
};
