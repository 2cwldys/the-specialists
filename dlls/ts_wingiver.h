/***
*  ts_wingiver.h — CTSWinGiver + CTSScoreGiver (original TS_objectives.cpp), the Use-fired
*  team-win / score giver objectives. Declared in a header so the audit sees the FGD members
*  (winnerteam/victorycash/defeatcash/callsnumber). See ts_objectives.cpp for behaviour.
***/
#ifndef TS_WINGIVER_H
#define TS_WINGIVER_H

#include "ts_objectives.h"

// The base is CBaseEntity (the original's typeinfo, @0x7e604), not CTSObjective: IsObjective and
// EnableObjective are CBaseEntity's TS virtuals, overridden here.
class CTSWinGiver : public CBaseEntity
{
public:
	void Spawn() override;
	bool KeyValue(KeyValueData *pkvd) override;
	bool Save(CSave &save) override;
	bool Restore(CRestore &restore) override;
	bool IsObjective() override { return true; }  // @0x7e64c — returns 1 (explicit override)
	void EnableObjective(bool enable) override;   // @0x7bf00 — clears the call count and the done byte
	void EXPORT Use(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value) override;

	static TYPEDESCRIPTION m_SaveData[];

	int winnerteam = 0;         // keyvalue "winnerteam" (0 Team1, 1 Team2, 2 draw, …)
	int cash_for_winner = 0;    // +0x6c keyvalue "victorycash" (saved, read nowhere)
	int cash_for_loser = 0;     // +0x70 keyvalue "defeatcash" (saved, read nowhere)
	int numberofcall = 0;       // +0x78 keyvalue "callsnumber": Use calls needed to fire
	int m_calls = 0;            // calls so far (transient)
	bool m_bDone = false;       // already fired (transient)
};

class CTSScoreGiver : public CTSWinGiver
{
public:
	void Spawn() override;                                  // @0x8c0b8
	bool KeyValue(KeyValueData *pkvd) override;             // @0x8c090 — delegates to CTSWinGiver
	void EXPORT Use(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value) override;
};

#endif // TS_WINGIVER_H
