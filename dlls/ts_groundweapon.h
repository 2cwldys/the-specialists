/***
*  ts_groundweapon.h — CTSGroundWeapon (the map-placed weapon pickup, ts_groundweapon).
*  Declared in a header so the hldmcompat spawner subclasses (CTSPistolWeapon, CTSShotgunWeapon,
*  …) can derive it and override the virtual GetWeapon (the original's vtable slot 0x118).
*
*  Method set + member offsets recovered 1:1 from ts_i386.so (TS_worldgun.cpp). Members carry the
*  original FGD/save-table NAMES (tsweaponid/respawntime/extraclip/attackerteam/buyable).
***/
#ifndef TS_GROUNDWEAPON_H
#define TS_GROUNDWEAPON_H

#include "ts_weapon_info.h" // tsweapon_t / TSW_* (the default member type below)

class CTSGroundWeapon : public CBaseEntity
{
public:
	void Spawn() override;
	void Precache() override;
	bool KeyValue(KeyValueData *pkvd) override;
	bool Save(CSave &save) override;
	bool Restore(CRestore &restore) override;
	static TYPEDESCRIPTION m_SaveData[];

	// settle-then-pickup physics (shared with CTSGroundAmmo)
	void EXPORT Fall();                 // think: wait for the floor (or PutOnWall), then OnGround
	void PutOnWall();                   // spawnflag 0x10 "lie on wall": 8-ray circle, lay flat
	virtual void OnGround();            // landed: become a SOLID_TRIGGER grab pickup
	void EXPORT WaitThink();            // idle keep-alive (post-respawn)
	void EXPORT WaitTouch(CBaseEntity *pOther); // touch: auto-pickup only for tsweaponid 0x1d
	void EXPORT Use(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value); // USE = pick up
	virtual void GetWeapon(CBaseEntity *pOther); // hand the weapon over (subclasses re-roll)

	int ObjectCaps() override { return FCAP_CONTINUOUS_USE; } // @orig: bare 0x10 (USE-to-pick-up)
	virtual int CanUsedTroughWalls() { return 0; }            // TS virtual (slot 0xc): false here

	int respawntime = 60; // +0xa0 wduration                 (SAVED)
	int tsweaponid = TSW_GLOCK18; // +0xa4 weapon type        (SAVED)
	int extraclip = 0;    // +0xa8 wextraclip                 (SAVED)
	int attackerteam = -1;// +0xac (team that may buy; -1 any)
	bool buyable = false; // +0xb0
	bool m_bPlaced = false; // +0x64 settled flag
};

#endif // TS_GROUNDWEAPON_H
