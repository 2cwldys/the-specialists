/***
*  ts_slowmotionpoint.h — CTSSlowMotionPoint (original TS_entities.cpp), the runtime slow-motion
*  bubble (player/powerup-created). Declared in a header so the audit sees its members.
*  See ts_slowmotionpoint.cpp for behaviour.
***/
#ifndef TS_SLOWMOTIONPOINT_H
#define TS_SLOWMOTIONPOINT_H

class CTSSlowMotionPoint : public CBaseEntity
{
public:
	void Spawn() override;                                                  // @0x61564
	void EXPORT WaitThink();                                                // @0x61728
	void ActivateSlow(float scale, float duration, float value, bool slowBullets); // @0x618ac
	void DeactivateSlow();                                                  // @0x6166c — restore everyone slowed
	// (no ObjectCaps override — the original uses base CBaseEntity::ObjectCaps; runtime-created, not map-placed.)

	// @0x614d8 — a bubble at `origin`: Spawn, then ActivateSlow with the rest of the arguments
	static CTSSlowMotionPoint *CreateSlowMotionPoint(const Vector &origin, float scale, float duration,
		float value, bool slowBullets);

	EHANDLE m_hPlayers[33];  // +0x64 — the players this bubble slowed, by ENTINDEX
	float m_flEndTime = 0;   // +0xe8 — when the bubble expires
	float m_flScale = 0;     // +0xec — the slow factor applied
	float m_flValue = 0;     // +0xf0 — ActivateSlow's third value (written, never read)
	bool  m_bSlowBullets = false; // +0xf4 — the slowed players' bullets slow down too
};

#endif // TS_SLOWMOTIONPOINT_H
