/***
*  ts_gamerules.h — RECONSTRUCTED CTSGameRules (skeleton) from ts_i386.so.
*  Functional/source-faithful (see CLAUDE.md).
*
*  The original CTSGameRules is large (41 funcs) and deeply entangled with TS
*  additions to CBasePlayer (slow-motion, powerups, teams, objectives, "the one"
*  / last-man-standing modes). This is the foundation stone: a real, installable
*  rules object that the rest of the TS code hangs off. It currently derives all
*  round/team/objective behaviour from CHalfLifeMultiplay; TS-specific methods are
*  filled in incrementally (each verified) as their dependencies land.
***/
#ifndef TS_GAMERULES_H
#define TS_GAMERULES_H

#include "gamerules.h"

class CTSObjective;        // dlls/ts_objectives.h (objective HUD-list element type)
class CTSObjectiveManager; // dlls/ts_objectives.h (round objective managers)

// TS powerup type bits — used both as ts_powerup spawnflags and as the mask for
// GetRandomPowerup (cf. ts_fgd.fgd ts_powerup spawnflags).
#define TS_PWUP_SLOWMO    (1 << 0) // 1
#define TS_PWUP_INFCLIP   (1 << 1) // 2
#define TS_PWUP_KUNGFU    (1 << 2) // 4
#define TS_PWUP_SLOWPAUSE (1 << 3) // 8
#define TS_PWUP_DBLFIRE   (1 << 4) // 16
#define TS_PWUP_GRENADE   (1 << 5) // 32
#define TS_PWUP_HEALTH    (1 << 6) // 64
// (1 << 7) == 0x80 is an unused slot — never selectable.
#define TS_PWUP_LOWGRAV   (1 << 8) // 0x100
#define TS_PWUP_ALL       0x1ff

// --- Team subsystem (cont.35) ---
// TS supports up to 4 teams; team mode uses 2 ("Specialists;Mercenaries"). Team
// indices are 0-based to match the FGD attackerteam/winnerteam convention the
// objective entities already gate on (0 Team1/spec, 1 Team2/merc — see ts_objectives.cpp).
#define TS_MAX_TEAMS    4
#define TS_TEAMNAME_LEN 32

class CTSGameRules : public CHalfLifeMultiplay
{
public:
	// Public passthrough so the `motd` console command can re-send the MOTD (the base
	// CHalfLifeMultiplay::SendMOTDToClient is protected). @ ClientCommand "motd".
	void ResendMOTD(edict_t *client) { SendMOTDToClient(client); }

	// Pick a random permitted powerup bit out of `mask`, or 0 if none qualify.
	int GetRandomPowerup(int mask);

	// --- Team subsystem (cont.35) ---
	// The map is a team-mode map. Faithful gate is the TS game-mode member (@+0 == 2/3),
	// not reconstructed; functional proxy = a ts_objective_manager is present. Lazily
	// resolved + cached (worldspawn installs the rules before objective ents spawn).
	bool IsTeamMode();
	bool IsTeamplay() override;          // teamplay iff IsTeamMode()
	const char* GetGameDescription() override; // "The Specialists (...)" per game mode

	// Team roster (mirrors CHalfLifeTeamplay). InitTeams seeds the TS default list when
	// team mode is first detected; RecountTeams tokenizes it into m_teamNames/m_numTeams.
	void  InitTeams();
	void  RecountTeams();
	int   GetTeamIndex(const char *pTeamName) override;     // 0-based, -1 = no match
	const char *GetIndexedTeamName(int teamIndex) override; // "" if out of range
	const char *GetTeamID(CBaseEntity *pEntity) override;   // the entity's team name string
	const char *TeamWithFewestPlayers();                    // auto-balance pick

	// Put `pPlayer` on `pTeamName`: pev->team (0-based) + m_szTeamName + model/team
	// infokeys, and broadcast the HUD team/score info. The server-side stand-in for the
	// faithful client class-selection menu (cl_dll, not built).
	void  AssignToTeam(CBasePlayer *pPlayer, const char *pTeamName);

	// Auto-balance assignment is driven from spawn (override of the MP spawn).
	void  PlayerSpawn(CBasePlayer *pPlayer) override;
	void  InitHUD(CBasePlayer *pPlayer) override;           // send the team roster to a client

	char  m_szTeamList[256] = {};
	char  m_teamNames[TS_MAX_TEAMS][TS_TEAMNAME_LEN] = {};
	char  m_teamModels[TS_MAX_TEAMS][TS_TEAMNAME_LEN] = {};
	int   m_numTeams = 0;
	int   m_iTeamMode = -1;              // tri-state cache: -1 unknown, 0 DM, 1 team

	// Per-frame: drives the dynamic deathmatch powerup spawner (PutPowerUp) on top of the
	// inherited CHalfLifeMultiplay round/spawn logic.
	void Think() override;

	// On death: the MP scoring, plus the TS death effects (drop the arsenal as world guns +
	// leave a fading corpse).
	void PlayerKilled(CBasePlayer *pVictim, entvars_t *pKiller, entvars_t *pInflictor) override;

	// @ TeamWin — declare `team` the round winner: bump its score, announce, and schedule a
	// round restart (respawn everyone + re-arm objectives). Called by CTSWinGiver.
	void TeamWin(int team);

	int   m_teamScore[4] = {};    // per-team round wins
	float m_flRoundRestart = 0;   // >0 = a restart is pending at this time (the .so's +0xc)

	// @ CheckDeads / CallCheckDeads — the team-round elimination win condition. A player
	// death schedules CheckDeads ~1s later (CallCheckDeads); it counts live players per team
	// and, if only one team has survivors, declares that team the round winner via TeamWin
	// (nobody alive = a draw; two+ teams alive = the round continues).
	void  CheckDeads();
	void  CallCheckDeads();
	float m_flCheckDeads = 0;     // >0 = a deads-check is pending at this time (the .so's +0x5880)

	// @ round timer — team rounds time out. StartRoundTimer arms it (roundtime clamped [30,180]) +
	// sends the remaining seconds (gmsgTSRoundTime); when it expires, RoundTimeLeft fires the
	// objective manager's `targetontimeleft` and declares a draw (TeamWin(2)).
	void  StartRoundTimer();
	void  SendRoundTime(CBasePlayer *pPlayer); // catch a joining client up on the round clock
	void  RoundTimeLeft();
	float m_flRoundEnd = 0;       // >0 = the round ends at this time (the .so's +0x8); 0 = not armed

	// --- Objective subsystem (the HUD objective list + the round's objective managers). The cluster
	// entities (ts_objective_ptr/hack/wingiver/bomb) drive the HUD through these. ---
	// One HUD-list entry: the objective + a per-team label string (string-pool index by team).
	struct ObjectiveEntry { CTSObjective *obj = nullptr; int teamStr[4] = {}; };

	void ClearObjectiveList();                              // @0x6abdc — reset the HUD list
	int  FindIndexByObjective(CTSObjective *pObj);          // @0x6ae20 — slot of pObj, or -1
	int  AddObjectiveInList(CTSObjective *pObj, const int teamStr[4]); // @0x6ac00 — append (dedup), return index
	void SendObjectiveToHud(int idx, CBasePlayer *pPlayer); // @0x6ac80 — one entry -> one client (gmsgTSObjective)
	void SendObjectiveList(CBasePlayer *pPlayer);           // @0x6add4 — the whole list -> one client
	void ShowObjectiveOnHud(CTSObjective *pObj, bool show); // @0x6aebc — toggle visibility (gmsgTSShowObjective)
	void InitObjectives();                                  // @0x685d8 — scan managers + disable all objectives
	void SetObjective();                                    // @0x690b4 — pick + Reset() a random manager

	ObjectiveEntry m_objectiveList[32];        // the HUD list (field_0x2d4); count below
	int   m_iObjectiveCount = 0;               // field_0x2d0
	CTSObjectiveManager *m_managers[32] = {};  // every ts_objective_manager (field_0x250)
	int   m_iManagerCount = 0;                 // field_0x24c
	CTSObjectiveManager *m_pActiveManager = nullptr; // the chosen manager this round (field_0x554)

	// @ PutPowerUp(type) — spawn a powerup of `type` at a random info_player_deathmatch
	// (skipped if a powerup is already nearby). Returns true if one was spawned.
	bool PutPowerUp(int type);

	float m_flNextPowerupSpawn = 0; // next dynamic-powerup time (DM mode; member @+4 in .so)

	// Global ("all map") slow-motion. GlobalSlowMotion2 applies `scale` to every
	// player each call (throttled ~0.5s; scale 0 = restore). SetGlobalSlowMotion is
	// the timed wrapper (cmd_globalslow / the ts_slowmotion "All map" mode).
	void GlobalSlowMotion2(float scale, int slowBullets);
	void SetGlobalSlowMotion(float scale, float duration);

	float m_flNextGlobalSlow = 0;   // 0x56c — GlobalSlowMotion2 throttle
	float m_flGlobalSlowActive = 0; // 0x560 — SetGlobalSlowMotion: active flag
	bool m_bSlowMatchActive = false; // the `slowmatch` cvar < 1.0 runs the whole match slowed
	float m_flGlobalSlowScale = 0;  // 0x564 — current global slow scale
	float m_flGlobalSlowEnd = 0;    // 0x568 — global slow end time (0 = no limit)

	// Fraction of damage absorbed by kevlar (the `armor_absorb` config value, gamerules
	// member @+0x588c read in CBasePlayer::TakeDamage). A flat reduction applied while the
	// player has armor — kevlar does NOT deplete. The exact original default is config-driven
	// (parsed from the server config, runtime-init — not statically recoverable), so this is a
	// sensible, server-tunable default.
	float m_flArmorAbsorb = 0.5f;
};

// In this mod the active rules object is always a CTSGameRules (see
// InstallGameRules). Mirrors the original's global `TSGameRules` instance.
inline CTSGameRules* TSGameRules()
{
	return static_cast<CTSGameRules*>(g_pGameRules);
}

#endif // TS_GAMERULES_H
