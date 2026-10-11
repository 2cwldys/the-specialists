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

// CTSObjective — base for the hack, bomb and escape objectives. @0x7c2b0 (KeyValue/CanInteract/Freeze).
class CTSObjective : public CBaseEntity
{
public:
	bool KeyValue(KeyValueData *pkvd) override;
	bool Save(CSave &save) override;
	bool Restore(CRestore &restore) override;

	virtual bool CanInteract(CBasePlayer *pPlayer); // @0x7c344 — resolve the player's team, then gate
	virtual bool CanInteract(int team);             // @0x7c38c — attacker_team gate (-1 = any)
	virtual void Freeze(bool bFreeze);              // @0x7c310 — active = !freeze (+ ShowObjectiveOnHud)
	bool IsObjective() override { return true; }     // @0x7e6e0 — return 1 (this IS a TS objective)
	void EnableObjective(bool bEnable) override { m_bActive = bEnable; } // @0x7e6e4 — set the active flag

	static TYPEDESCRIPTION m_SaveData[];

	bool m_bActive = false;           // +0x64 — objective live (frozen/enabled flag); off until enabled
	int  attacker_team = 0;           // +0x68 — keyvalue "attackerteam": team allowed to interact (-1 = any); 0 without the key, as the original's zeroed entity
};

#define TS_OBJ_TARGETS 8
#define TS_OBJ_MESSAGES 4

// CTSObjectiveManager — point entity holding the round's objective-pointer targets + per-slot
// messages; its Reset() (driven by CTSGameRules::SetObjective) announces them + fires the targets.
class CTSObjectiveManager : public CBaseEntity
{
public:
	void Spawn() override;
	bool KeyValue(KeyValueData *pkvd) override;
	bool Save(CSave &save) override;
	bool Restore(CRestore &restore) override;
	void Reset(); // @0x7b850 — HUD the messages + fire the objective-pointer targets

	static TYPEDESCRIPTION m_SaveData[]; // the 13 saved string fields (targets/messages/end_round_target)

	string_t targets[TS_OBJ_TARGETS] = {};   // keyvalue objptargeta..h
	string_t messages[TS_OBJ_MESSAGES] = {};  // keyvalue messagea..d
	string_t end_round_target = iStringNull;  // keyvalue targetontimeleft
	string_t m_targetSpecDead = iStringNull;  // targetonspecdead (Team 1 dead) — not saved
	string_t m_targetMercDead = iStringNull;  // targetonmercdead (Team 2 dead) — not saved
};

#endif // TS_OBJECTIVES_H
