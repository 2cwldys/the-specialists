/***
*  ts_escape.h — CTSTeamEscape (original TS_objectives.cpp), the team-escape zone objective.
*  Derives CTSObjective. Declared in a header so the audit sees its FGD member
*  (escape_percentage). See ts_escape.cpp for behaviour.
***/
#ifndef TS_ESCAPE_H
#define TS_ESCAPE_H

#include "ts_objectives.h" // CTSObjective (the base CTSTeamEscape derives)

class CTSTeamEscape : public CTSObjective
{
public:
	bool KeyValue(KeyValueData *pkvd) override;
	bool Save(CSave &save) override;
	bool Restore(CRestore &restore) override;
	void Spawn() override;
	void Precache() override {}                  // @0x8d4xx — empty
	int  ObjectCaps() override { return FCAP_CONTINUOUS_USE; } // -> 0x10
	void EXPORT WaitTouch(CBaseEntity *pOther);
	void EXPORT WaitThink();
	void EnableObjective(bool enable) override;  // arm/disarm (the escape zone's own override)

	static TYPEDESCRIPTION m_SaveData[];

	// m_bActive (armed, +0x64) + attacker_team (+0x68) are inherited from CTSObjective.
	int     escape_percentage = 0;  // +0xf4 — keyvalue "escapepercentage": 0 one / 1 third / 2 two-thirds / 3 all
	EHANDLE m_present[33];           // +0x6c.. — players counted this window (by entindex)
	int     m_count = 0;             // +0xf0 — distinct attacker-team players inside
};

#endif // TS_ESCAPE_H
