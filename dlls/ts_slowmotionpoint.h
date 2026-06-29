/***
*  ts_slowmotionpoint.h — CTSSlowMotionPoint (original TS_entities.cpp), the runtime slow-motion
*  bubble (player/powerup-created). Declared in a header so the audit sees its members.
*  See ts_slowmotionpoint.cpp for behaviour.
***/
#ifndef TS_SLOWMOTIONPOINT_H
#define TS_SLOWMOTIONPOINT_H

#define MAX_SLOWED 32

class CTSSlowMotionPoint : public CBaseEntity
{
public:
	void Spawn() override;
	void EXPORT WaitThink();
	void ActivateSlow(float scale, float duration, int slowBullets); // @ActivateSlow
	void DeactivateSlow();                                           // restore everyone we slowed
	// (no ObjectCaps override — the original uses base CBaseEntity::ObjectCaps; runtime-created, not map-placed.)

	static CTSSlowMotionPoint *CreateSlowMotionPoint(const Vector &origin, float scale, float duration,
		int slowBullets, CBaseEntity *pOwner);

	float m_flEndTime = 0;   // +0xe8 — when the bubble expires
	float m_flScale = 1.0f;  // +0xec — the slow factor applied
	int   m_iSlowBullets = 0;// +0xf4
	EHANDLE m_hSlowed[MAX_SLOWED]; // players we put into slow-motion (to restore)
	int   m_nSlowed = 0;
};

#endif // TS_SLOWMOTIONPOINT_H
