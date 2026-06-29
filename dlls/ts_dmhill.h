/***
*  ts_dmhill.h — CTSDMHill (original TS_entities.cpp), the touch-based king-of-the-hill.
*  Declared in a header so the audit sees its FGD members (nextcheck/fragamt/fragamtteam).
*  See ts_dmhill.cpp for behaviour.
***/
#ifndef TS_DMHILL_H
#define TS_DMHILL_H

class CTSDMHill : public CBaseEntity
{
public:
	void Spawn() override;
	bool KeyValue(KeyValueData *pkvd) override;
	bool Save(CSave &save) override;
	bool Restore(CRestore &restore) override;
	int ObjectCaps() override { return 0; } // @0x73b20 — no caps (not save/use-able as an objective)
	void EXPORT WaitTouch(CBaseEntity *pOther); // @0x71be8 — holder tracking + payout
	void EXPORT WaitThink();                    // @0x72280 — idle keepalive (reschedule)

	static TYPEDESCRIPTION m_SaveData[];

	float nextcheck = 10.0f;        // +? — keyvalue "nextcheck": seconds to hold per payout (>=0.5)
	int   fragseverycheck = 5;      // keyvalue "fragamt": frags per payout (solo/DM)
	int   fragseverycheckteam = 1;  // keyvalue "fragamtteam": frags per payout to each teammate (team DM)
	EHANDLE m_hKing;                // +0x6c — the current holder
	float m_flAwardTime = 0;        // +0x7c — when the held hill next pays out
	float m_flLastTouch = 0;        // +0x68 — the holder's last-touch time (drop if it lapses)
	float m_flStartGate = 0;        // +0x64 — startup grace: ignore touches until this time
};

#endif // TS_DMHILL_H
