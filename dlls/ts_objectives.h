/***
*  ts_objectives.h — CTSObjective, the base for the TS objective subsystem (hackable/win/bomb
*  objectives). Declared in a header so the per-objective classes that live in their own .cpp
*  (CTSBomb in ts_bomb.cpp, CTSTeamEscape in ts_escape.cpp, …) can DERIVE it. The method bodies +
*  save table live in ts_objectives.cpp.
***/
#ifndef TS_OBJECTIVES_H
#define TS_OBJECTIVES_H

// Team indices (FGD winnerteam/attackerteam choices): 0 Team1(spec), 1 Team2(merc), 2 draw…
#define TS_TEAM_ANY (-1)

// CTSObjective — base for hackable/win objectives. @0x8c2b0 (KeyValue/CanInteract/Freeze).
class CTSObjective : public CBaseEntity
{
public:
	bool KeyValue(KeyValueData *pkvd) override;
	bool Save(CSave &save) override;
	bool Restore(CRestore &restore) override;

	virtual bool CanInteract(CBasePlayer *pPlayer); // @0x8c344 — resolve the player's team, then gate
	virtual bool CanInteract(int team);             // @0x8c38c — attacker_team gate (-1 = any)
	virtual void Freeze(bool bFreeze);              // @0x8c310 — active = !freeze (+ ShowObjectiveOnHud)
	virtual bool IsObjective() { return true; }     // @0x8e6e0 — return 1 (this IS a TS objective)
	virtual void EnableObjective(bool bEnable) { m_bActive = bEnable; } // @0x8e6e4 — set the active flag

	static TYPEDESCRIPTION m_SaveData[];

	bool m_bActive = true;            // +0x64 — objective live (frozen/enabled flag)
	int  attacker_team = TS_TEAM_ANY; // +0x68 — keyvalue "attackerteam": team allowed to interact (-1 = any)
};

#endif // TS_OBJECTIVES_H
