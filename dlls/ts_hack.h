/***
*  ts_hack.h — CTSHack (original TS_objectives.cpp), the held-USE hack objective.
*  Derives CTSObjective. Declared in a header so the audit sees its FGD member (hacksec).
*  See ts_objectives.cpp for behaviour.
***/
#ifndef TS_HACK_H
#define TS_HACK_H

#include "ts_objectives.h" // CTSObjective (the cluster base CTSHack derives)

class CTSHack : public CTSObjective
{
public:
	void Spawn() override;
	void Precache() override;
	bool KeyValue(KeyValueData *pkvd) override;
	bool Save(CSave &save) override;
	bool Restore(CRestore &restore) override;
	int  ObjectCaps() override { return FCAP_CONTINUOUS_USE; } // @0x8e788 -> 0x10
	void Use(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value) override; // @0x8cf14
	void EnableObjective(bool enable) override; // @0x8cd6? arm/disarm + seed the objective deadline
	void EXPORT WaitThink();                    // @0x8ceb0 — objective deadline countdown -> Freeze

	static TYPEDESCRIPTION m_SaveData[];

	int   hacksec = 10;         // +0x7c — keyvalue "hackingtime": seconds of HELD USE to complete
	float m_flObjTimer = 0;     // +0x6c — objective deadline countdown (round seconds left)
	float m_flHackProgress = 0; // +0x70 — hack progress remaining (held USE counts it down; <0 = done)
	float m_flLastUse = 0;      // +0x74 — last USE time (held-USE continuity)
	float m_flNextSound = 0;    // +0x78 — next "hacking" sound time
};

#endif // TS_HACK_H
