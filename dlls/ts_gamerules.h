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
#include "ts_weapon_info.h" // tsweapon_t (the CustomWP pipeline decls below)

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
#define TS_TEAMMODELS    3  // models per team (the original's +0x38/+0x48/+0x58 in each team record)
#define TS_TEAMMODEL_LEN 16
#define TS_TEAMSPOT_LEN  32 // the team's spawn spot class and its fallback (+0x68/+0x88)

// R1 phase 2: CTSGameRules is a STANDALONE class — base:null, NO vtable, all methods
// non-virtual, exactly like the original (nm has no __vt_12CTSGameRules; the 22.7KB static
// object @0x1681a0 is reached via the GOT pointer PTR_TSGameRules = our `TSGameRules`).
// It COEXISTS with g_pGameRules (a stock TS-patched CHalfLifeMultiplay/CHalfLifeTeamplay):
// MP::Think/Teamplay::Think call TSGameRules->Think() (@0xbf302/@0xd439a), and
// CBasePlayer::UpdateClientData's InitHUD block calls TSGameRules->InitHUD() (@0x81d00).
class CTSGameRules
{
public:

	// Pick a random permitted powerup bit out of `mask`, or 0 if none qualify.
	int GetRandomPowerup(int mask);

	// rules+0 (the original's first byte): 0/1 = non-team (DecideGamePlay return), 2 = team,
	// 3 = team WITH objectives (InitObjectives promotes 2/3; the Wires system gates on 3).
	int m_iGameMode = 0;

	// rules+1 — the dynamic powerups: -1 undecided (Init), 0 off (a map's own powerup spawned first,
	// CTSPowerUp::Spawn; item_longjump puts it back to -1), 1 on (the first Think in mode 0/1).
	signed char m_iRandomPowerups = -1;

	// --- SrvSett server-settings sync (star_SrvSett.md; dossier-gmsg U3) ---
	int m_iSrvRealBullet = 0;        // 0x578 — cached realbullet.value, chopped
	byte m_bSrvSuperJump = 0;        // 0x5885 — map-global SUPERJUMP mode (ts_mapglobals SF & 0x40):
	                                 // TSInit gives every spawn low-gravity + the "mj" physinfo key
	int m_iSrvUseCash = 0;           // 0x558 — cached usecash.value, chopped (orig var dmcashbased)
	int m_iSrvAmmoCount = 0;         // 0x55c — cached ammocount.value, chopped
	int m_iWeaponRestriction = 0x1f; // 0x5870 — ALLOWED-categories bitmask (knife 1/pistol 2/smg 4/rifle 8/shotgun 0x10)
	int m_iAllowedCategories = 0x1f; // 0x586c — the map's allowed mask (cmd args are AND-masked by it)
	int m_bWeaponParamsCustom = 0;   // 0x5884 — set once a customweapons file loads; InitHUD gates on it
	float m_flHealthAbsorb = 0.1f;   // 0x5888 — the customweapons health_absorb value (Init: 0.1; nothing reads it)

	// --- Team subsystem ---
	// The original's team gate is the global `g_teamplay != 0` at each site; the team-play virtual is
	// stock CHalfLifeTeamplay::IsTeamplay.
	// GetGameDescription is NOT overridden here — the original defines it on CHalfLifeMultiplay
	// (theOne->LMS->DM, no teamplay branch); CTSGameRules inherits it.

	// Team roster (the original's 4-slot name table). InitTeams seeds defaults; SetTeamList
	// tokenizes the ";"-list directly into the slots.
	void  InitTeams();
	int   GetTeamIndex(const char *pTeamName);              // @0x685xx non-virtual (original has it)
	// Thin slot accessor (name by index, "" if out of range). The original has no
	// CTSGameRules::GetIndexedTeamName — it inlines the slot read (name@+0x28+i*0x8c) at each of
	// ~8 sites; KEPT as one internal helper rather than duplicating the bounds check 8x (internal,
	// not part of the external interface — CLAUDE.md). audit.py flags it; accepted. [R1 item-9]
	const char *GetIndexedTeamName(int teamIndex);
	int   GetTeamsNum();                                     // @0x685a8 — number of active teams
	void  SetTeamList(const char *pList);                   // @0x680dc — parse the ";"-team list into the slots
	void  SetTeamModelList(const char *pList);             // @0x68258 — parse the "|"-model list into the slots
	void  SetTeamModelList(int team, const char *pModels); // @0x68438 — one team's ";"-list of up to 3 models
	int   GetTeamCash(CBasePlayer *pPlayer);               // @0x69aac — the player's team cash (0 outside modes 2/3)
	const char *GetPlayerModel(CBasePlayer *pPlayer);      // @0x69a08 — one of the player's team models, at random
	void  TogglePlayerFromSlow(CBasePlayer *pPlayer);      // @0x696c8 — remove a player from the slow registry
	void  InitTeamScore(int *pScores);                     // @0x69130 — (no-op in the original)
	void  UpdateTeamScores(int a, int b);                  // @0x67908 — (no-op in the original)

	// (AssignToTeam DELETED — R1 phase 3: teams are assigned by the TS-patched
	// CHalfLifeTeamplay::SetDefaultPlayerTeam/ChangePlayerTeam @0xd44ac/@0xd488c.)

	// (PlayerSpawn override DELETED — R1/Y9: the virtual is the TS-patched
	// CHalfLifeMultiplay::PlayerSpawn @0x790ac; equip core + tagged interims live there.)
	void  InitHUD(CBasePlayer *pPlayer);                    // @0x69134 non-virtual — called from
	                                                        // CBasePlayer::UpdateClientData's InitHUD block

	char  m_teamNames[TS_MAX_TEAMS][TS_TEAMNAME_LEN] = {};
	char  m_teamModels[TS_MAX_TEAMS][TS_TEAMMODELS][TS_TEAMMODEL_LEN] = {};
	char  m_teamSpot[TS_MAX_TEAMS][TS_TEAMSPOT_LEN] = {};         // spawn spot class (InitTeams; EntSelectSpawnPoint)
	char  m_teamSpotFallback[TS_MAX_TEAMS][TS_TEAMSPOT_LEN] = {}; // its fallback class
	int   m_teamCash[TS_MAX_TEAMS] = {}; // per-team cash pool (GetTeamCash; the cash economy is dead in the shipped game)
	int   m_numTeams = 0;

	// Per-frame: drives the dynamic deathmatch powerup spawner (PutPowerUp) on top of the
	// inherited CHalfLifeMultiplay round/spawn logic.
	void Think(); // @0x67204 non-virtual — called from the TS-patched MP/Teamplay::Think

	// On death: the MP scoring, plus the TS death effects (drop the arsenal as world guns +
	// leave a fading corpse).
	// (PlayerKilled override DELETED — R1/Y8: the virtual is the TS-patched
	// CHalfLifeMultiplay::PlayerKilled @0x795e4; scatter lives in CBasePlayer::Killed @0x7ed5c.)

	// @ TeamWin — declare `team` the round winner: bump its score, announce, and schedule a
	// round restart (respawn everyone + re-arm objectives). Called by CTSWinGiver.
	void TeamWin(int team, int cash, bool bRestart, bool bScore); // @0x68a08

	int   m_teamScore[4] = {};    // per-team round wins
	float m_flRoundRestart = 0;   // >0 = a restart is pending at this time (the .so's +0xc)

	// @ CheckDeads / CallCheckDeads — the round's end condition. A player death schedules
	// CheckDeads ~1s later (CallCheckDeads). Last man standing: one player left alive ends the
	// round; objective modes (2/3): a wiped team loses. Plain teamplay (mode 1) has no rounds.
	void  CheckDeads();
	void  CallCheckDeads();
	float m_flCheckDeads = 0;     // >0 = a deads-check is pending at this time (the .so's +0x5880)

	// @ round timer — team rounds time out. RestartRound arms it INLINE (roundtime clamped
	// [30,180]) + broadcasts the remaining seconds (gmsgTSRoundTime); InitHUD catches a joiner
	// up inline; when it expires, RoundTimeLeft fires the objective manager's `targetontimeleft`
	// and declares a draw (TeamWin(2)). (StartRoundTimer/SendRoundTime helpers deleted — no
	// original counterparts; fold-plan G3/G4.)
	void  RoundTimeLeft();
	float m_flRoundEnd = 0;       // >0 = the round ends at this time (the .so's +0x8); 0 = not armed
	bool  m_bRoundTimerEnabled = true; // the .so's field_0x10 — gates the RoundTime HUD message

	// --- Round lifecycle (the .so's Init / RestartRound / ResetRestartRound / RespawnEntities). ---
	CTSGameRules();                    // @0x67d6c Init-equivalent: one-time setup (calls Init())
	void Init();                       // @0x67d6c — map setup: InitTeams + InitObjectives/SetObjective + arm 1st round
	void RestartRound();               // @0x67920 — the round reset (re-arm timer, objectives, refund + spectate, RespawnEntities)
	void RespawnEntities(bool bFirst); // @0x67764 — respawn the map's breakables/weapons/powerups/flags
	void ResetRestartRound(int depth); // @0x68750 — modes 2/3 or LMS: arm a 1s restart (depth>1 wipes team scores)
	int  CountPlayers();               // @0x6a154 — number of connected players

	// --- "The One" juggernaut mode (one buffed player; the killer inherits the role). ---
	bool IsCTB();                                          // @0x67f08 — is this capture-the-briefcase mode
	void SelectRandomTheOne();                            // @0x6af28 — pick a random player to be The One (throttled)
	void SetPlayerTheOne(CBasePlayer *pPlayer);           // @0x6b078 — buff `pPlayer` into The One + announce
	void TheOneIsDead(CBasePlayer *pDead, CBasePlayer *pKiller); // @0x6b258 — clear + hand the role to the killer
	bool  m_bTheOneActive = false;     // a One is currently designated
	float m_flNextTheOneSelect = 0;    // SelectRandomTheOne throttle (every ~10s)
	signed char m_iTheOneOthers = 0;   // rules+0x5876 — the other players, counted by Think's 10 s One
	                                   // check; the One's regeneration in PreThink scales with it
	bool  m_bCTBChecked = false;       // IsCTB cache validity
	bool  m_bHasBriefcase = false;     // IsCTB cache: a ts_briefcase exists

	// --- Voting (per-player map / slowmatch / teamplay / friendly-fire votes; tallied to a majority). ---
	int  PlayerVote(edict_t *pEntity, const char *pCmd); // @0x6a1c8 — a player's `vote ...` command
	void CheckVotes();                                   // @0x6a5c4 — tally votes (throttled), apply on majority
	// One block per ENTINDEX, 0x48 bytes from +0x684 as in the original: block 0 is never a player's
	// and player 32 has none (his vote is not a vote). Only "vote map" fills one in; the three
	// on/off bytes (1 on, 2 off) are counted and cleared but nothing in the game sets them.
	struct VoteBlock
	{
		char map[64];      // +0x00 (+0x684) the voted next map ("" = none)
		byte slowmatch;    // +0x40 (+0x6c4)
		byte teamplay;     // +0x41 (+0x6c5)
		byte friendlyfire; // +0x42 (+0x6c6)
		int  owner;        // +0x44 (+0x6c8) ENTINDEX + 1 once the block is reset (written, never read)
	};
	VoteBlock m_votes[32] = {};
	float m_flNextVoteCheck = 0;       // CheckVotes throttle (every ~10s) [field_0xf84]
	// The last count's tally (+0xf88..+0x1823), kept for "vote stats": CheckVotes clears it and fills
	// it in each time it counts; nothing else clears it, so a new map shows the old map's tally until
	// its first count.
	char m_voteMaps[32][64] = {};      // +0xf88 — the maps voted for, in the order first seen
	int  m_voteMapCount[32] = {};      // +0x1788 — votes for each
	int  m_voteSlowOn = 0, m_voteSlowOff = 0;   // +0x1808 / +0x180c
	int  m_voteTeamOn = 0, m_voteTeamOff = 0;   // +0x1810 / +0x1814
	int  m_voteFFOn = 0, m_voteFFOff = 0;       // +0x1818 / +0x181c
	int  m_iVotants = 0;               // +0x1820 — players counted
	char m_szEndMap[0x40] = {};        // @+0x1824 — the forced next map (a vote winner, or an objective
	                                   // set it); MP/Teamplay::Think end the map when byte 0 is set,
	                                   // MP::ChangeLevel @0xc0784 consumes it (unless it's "nextmap")

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

	float m_flNextPowerupSpawn = 0; // next dynamic-powerup time (modes 0/1; member @+4 in .so)

	// Global ("all map") slow-motion. GlobalSlowMotion2 applies `scale` to every
	// player each call (throttled ~0.5s; scale 0 = restore). SetGlobalSlowMotion is
	// the timed wrapper (cmd_globalslow / the ts_slowmotion "All map" mode).
	void GlobalSlowMotion2(float scale, int slowBullets);
	void SetGlobalSlowMotion(float scale, float duration);

	float m_flNextGlobalSlow = 0;   // 0x56c — GlobalSlowMotion2 throttle
	float m_flGlobalSlowActive = 0; // 0x560 — SetGlobalSlowMotion: active flag
	float m_flGlobalSlowScale = 0;  // 0x564 — current global slow scale
	float m_flGlobalSlowEnd = 0;    // 0x568 — global slow end time (0 = no limit)

	// Fraction of damage absorbed by kevlar (the `armor_absorb` value of a loadweapons file,
	// gamerules member @+0x588c read in CBasePlayer::TakeDamage). A flat reduction applied while
	// the player has armor — kevlar does NOT deplete. Init sets it to 0.2125 on every map
	// (@0x67ee8), as it sets health_absorb to 0.1.
	float m_flArmorAbsorb = 0.2125f;
};

// The global TS rules pointer (set in InstallGameRules; defined in ts_player.cpp — the
// original's home file). INTERIM: the original `TSGameRules` is a static OBJECT @0x1681a0
// (22,676 bytes, standalone class — no vtable, base null) that COEXISTS with g_pGameRules;
// the object-ness lands with the rules split (fold-plan R1). Null outside the CTSGameRules
// branch (deathmatch 0).
extern CTSGameRules *TSGameRules;

// ---- melee deflect gates (free functions, TS_gamerules.cpp; RE dossier-melee §1.3) ----
// true = the swing was blocked (caller aborts damage); the attacker pays 33 stamina.
bool DoSpectacularNess(void); // @0x76b48 — the `spectacularness` random roll (slow-mo bubbles)
int DecideGamePlay(char *mapname, int def); // @0x667ac — teamplay decision + LMS/TheOne flags
extern int g_gamemode;        // one-shot mode override (consumed by DecideGamePlay)
extern int g_theOneMode;      // The One mode flag
extern int g_lastManStanding; // last-man-standing mode flag
// @.data — the mode-dependent server-browser game-name strings (GetGameDescription). g_szDMDescription
// @0x10d004 / g_szTDMDescription @0x10d019 / g_szTOMDescription @0x10d034 / g_szLMSDescription @0x10d04e.
extern char g_szDMDescription[];  // "The Specialists (DM)"       — Multiplay default
extern char g_szTDMDescription[]; // "The Specialists (Teamplay)" — Teamplay default
extern char g_szTOMDescription[]; // "The Specialists (The One)"
extern char g_szLMSDescription[]; // "The Specialists (LMS)"
// @0x65ff4 — every respawn-wait funnels here (TS_gamerules.cpp); the bool is dead in the original.
void SetPlayerRespawnTime(CBasePlayer *pPlayer, int sec, bool bUnused);
// @0x66244 — TS's spawn scorer (teammate-cover band / unseen band / nearest-distance;
// crowded-winner telefrag; sets g_pLastSpawn). The sole caller is EntSelectSpawnPoint.
edict_t *TSSelectSpawnSpot(const char *szClass, CBaseEntity *pPlayer, const char *szFallback);
int WeaponRestrictionByID(int id); // @0x66ad8 — 1->0x0, 2->0x1, 3->0x3, 4->0x13, else InitWeaponRestriction()
int InitWeaponRestriction(void);   // @TS_gamerules.cpp — 0x1f (all five categories)
extern int gmsgTSServerSetting;    // "SrvSett" 3 — TS_gamerulesclass.cpp
void SendWeaponParams(tsweapon_t id, CBaseEntity *pOnly);   // @0x8eb98 — TS_util.cpp (CustomWP)
void InformClientsOnWeaponsParams(CBaseEntity *pOnly);      // @0x8eb14 — banner + full 38-slot sync
bool VictimKatanaHold(CBasePlayer *pAttacker, CBaseEntity *pVictim, float flDamage);
bool VictimKnifeHold(CBasePlayer *pAttacker, CBaseEntity *pVictim, float flDamage);
bool VictimKungfuHold(CBasePlayer *pAttacker, CBaseEntity *pVictim, float flDamage, TraceResult *ptr);

#endif // TS_GAMERULES_H
