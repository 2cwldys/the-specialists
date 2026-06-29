/***
*  ts_bomb.h — CTSBomb (original TS_objectives.cpp), the plantable/defusable bomb objective.
*  Derives CTSObjective. Declared in a header so the audit sees its FGD members
*  (defusesec/cancutwire). See ts_bomb.cpp for behaviour.
***/
#ifndef TS_BOMB_H
#define TS_BOMB_H

#include "ts_objectives.h" // CTSObjective (the base CTSBomb derives)

class CTSBomb : public CTSObjective
{
public:
	bool KeyValue(KeyValueData *pkvd) override;
	bool Save(CSave &save) override;
	bool Restore(CRestore &restore) override;
	void Spawn() override;
	void Precache() override;
	int  ObjectCaps() override { return FCAP_CONTINUOUS_USE; } // @ObjectCaps __7CTSBomb == 0x10
	void Use(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value) override;

	void EXPORT WaitThink();          // the bomb tick: countdown + beep, or explode at zero
	void EnableObjective(bool enable) override; // arm/disarm (the bomb's own override of the base)
	void SetRandomWires();
	void CutWire(int wire, CBasePlayer *pPlayer);

	static TYPEDESCRIPTION m_SaveData[];

	// m_bActive (armed/ticking, +0x64) + attacker_team (+0x68) are inherited from CTSObjective.
	float  m_flTimer = 0;          // +0x6c — countdown remaining (seconds)
	int    defusesec = 10;    // +0x80 — keyvalue "defusetime": USE-hold seconds to defuse
	int    cancutwire = 1;        // keyvalue "cutwires": wire-cut defuse enabled

	int    m_wires[3] = {};        // +0x7c..0x7e
	bool   m_bFast = false;        // +0x7f — boobytrap sped the countdown up
	float  m_flUseProgress = 0;    // +0x74 — remaining USE-defuse time
	float  m_flLastUse = 0;        // +0x78 — last USE tick (continuity check)
	float  m_flNextBeep = 0;       // +0x70
};

#endif // TS_BOMB_H
