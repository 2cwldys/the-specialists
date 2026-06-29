/***
*  ts_mapglobals.h — CTSMapGlobals (original TS_entities.cpp), the invisible per-map config entity.
*  Declared in a header so the audit sees its saved members. See ts_mapglobals.cpp for behaviour.
***/
#ifndef TS_MAPGLOBALS_H
#define TS_MAPGLOBALS_H

class CTSMapGlobals : public CBaseEntity
{
public:
	void Spawn(void) override;
	void Precache(void) override;
	bool KeyValue(KeyValueData *pkvd) override;
	bool Save(CSave &save) override;       // @0x62488 — WriteFields(2) over m_SaveData
	bool Restore(CRestore &restore) override; // @0x624d8 — ReadFields(2)
	void EXPORT WaitThink(void);           // @0x62668 — inert 10s self-reschedule
	int get_weapon_restriction(void);      // @0x6263c — InitWeaponRestriction() ^ spawnflags

	static TYPEDESCRIPTION m_SaveData[];

	// The two SAVED fields (the original's WriteFields count of 2). Vestigial on this entity —
	// the original's KeyValue handles no keys and Spawn only zeroes them; the live player-amount
	// logic lives in ts_playeramount. Kept for save-table fidelity.
	int player_at_a_time_num = 0; // +0xc
	int player_respawn_num = 0;   // +0x10
	int m_field14 = 0;            // +0x14 — transient (zeroed in Spawn)
	int m_field18 = 0;            // +0x18 — transient (zeroed in Spawn)
};

#endif // TS_MAPGLOBALS_H
