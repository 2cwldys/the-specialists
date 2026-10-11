/***
*  ts_groundammo.h — CTSGroundAmmo (the map-placed ammo pack, ts_groundammo).
*  Declared in a header so the hldmcompat ammo subclasses (CTSPistolAmmo, CTSRifleAmmo, …) can
*  derive it and seed the per-caliber array in their Spawn. Method/member layout 1:1 vs the
*  original (TS_worldgun.cpp): the ammo array is at +0x68, indexed directly by caliber.
***/
#ifndef TS_GROUNDAMMO_H
#define TS_GROUNDAMMO_H

#define TS_AMMO_CALIBERS 16

class CTSGroundAmmo : public CBaseEntity
{
public:
	void Spawn() override;
	void Precache() override;
	bool KeyValue(KeyValueData *pkvd) override;

	void PutOnWall();                   // SF 0x10: 8-ray circle, settle against a wall
	void EXPORT Fall();                 // think: settle to the floor (or wall)
	virtual void OnGround();            // landed: a SOLID_TRIGGER pickup
	void EXPORT WaitThink();            // idle keep-alive
	void Use(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value) {} // @orig: empty
	void EXPORT WaitTouch(CBaseEntity *pOther); // the TS gun in hand takes the rounds; the pack goes until it falls again

	int ObjectCaps() override { return FCAP_CONTINUOUS_USE; } // @orig: bare 0x10
	virtual int CanUsedTroughWalls() { return 0; }            // TS virtual, false here

	int respawntime = 0;                  // +0xa8 wduration (0 unless the map sets it)
	int m_ammo[TS_AMMO_CALIBERS] = {0};   // +0x68 per-caliber ROUNDS (magsN -> idx N, calibers 0..15)
	bool m_bPlaced = false;               // +0x64 settled flag
};

#endif // TS_GROUNDAMMO_H
