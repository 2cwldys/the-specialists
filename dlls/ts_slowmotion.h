/***
*  ts_slowmotion.h — CTSSlowMotion (original TS_entities.cpp), the map slow-motion trigger zone.
*  Declared in a header so the audit sees its FGD members (duration/slowamt). See ts_slowmotion.cpp.
***/
#ifndef TS_SLOWMOTION_H
#define TS_SLOWMOTION_H

#define TS_SLOMO_MAX_PLAYERS 33 // 0x21 — gpGlobals->maxClients + 1, indexed by ENTINDEX

class CTSSlowMotion : public CBaseEntity
{
public:
	void Spawn() override;
	bool KeyValue(KeyValueData *pkvd) override;
	void Use(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value) override;
	int ObjectCaps() override { return 0; } // @ ObjectCaps__13CTSSlowMotion returns 0
	bool Save(CSave &save) override;
	bool Restore(CRestore &restore) override;
	static TYPEDESCRIPTION m_SaveData[];

	void EXPORT WaitThink();
	void EXPORT WaitTouch(CBaseEntity *pOther);
	void ActivateSlow();
	void DeactivateSlow();
	void CheckPlayersDistance();

	int m_iToggledOn = 0;     // 0x64 — toggle state (duration<=0 mode)
	float m_flStopTime = 0;   // 0x68 — time the zone shuts off (0 = inactive)
	EHANDLE m_hPlayers[TS_SLOMO_MAX_PLAYERS]; // 0x70 — players currently slowed
	float m_flRadius = 0;     // 0xf4 — drop-range for CheckPlayersDistance
	float duration = 5;   // 0xf8 — "duration"
	float slow_ammount = 0.5f; // 0xfc — "slowamt" (GoSlow scale)
	int m_iAllMap = 0;        // 0x100 — SF_ALL_MAP: global slow-motion
	int m_bSlowBullets = 0;   // 0x104 — SF_SLOW_BULLETS
};

#endif // TS_SLOWMOTION_H
