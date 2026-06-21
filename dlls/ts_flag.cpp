/***
*  ts_flag.cpp — RECONSTRUCTED CTSFlag (cont.38), the CTF flag/briefcase.
*  Functional/source-faithful (see CLAUDE.md). recovered/flag/CTSFlag.c:
*    CreateFlag @0x9d874, Spawn @0x9d978, PlayerTakeFlag @0x9e330 (Use/WaitTouch),
*    CanInteract @0x9e2d4, Fall @0x9df10, OnGround @0x9e21c, RespawnFlag @0x9de58,
*    Destroy @0x9de24; + CBasePlayer::TakeFlag @0x93830 / DropFlag @0x9334c.
*
*  The flag is a runtime-created `CTSFlag : CTSPhysicObject` (`ts_flag`, model
*  briefcase.mdl), NOT map-placed (no FGD entity, 0 BSP placements) — it is created by
*  CreateFlag from a player's carried-flag state, and the map-placed CTF objectives
*  (CTSBriefcase, not built) seed the bases. The .so couples it to CTSPhysicObject physics
*  + the player's s_flag_status / GetSpeedBySlots speed system.
*
*  FUNCTIONAL reconstruction onto the stock SDK (no CTSPhysicObject, no speed-by-slots):
*  a team-owned flag with the classic CTF loop — an enemy/neutral player touching it carries
*  it (the flag follows the carrier); a carrier dies → it drops where they fell and auto-
*  returns after a timeout; a carrier touching their OWN team's flag at base captures (scores
*  + returns the carried flag). Bases are seeded here by a `tsspawnflag` console command
*  (stand-in for the CTSBriefcase map objective). // SLICE: carry doesn't slow the carrier;
*  no CTSPhysicObject bounce; capture scoring is via TeamWin's team score.
***/
#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "player.h"
#include "gamerules.h"
#include "ts_gamerules.h"
#include "ts_utils.h"   // TSHudMessageAll

#define FLAG_MODEL "models/briefcase.mdl"
#define FLAG_RETURN_TIME 30.0f   // a dropped flag auto-returns to base after this

enum tsflag_state_t { FLAG_AT_BASE = 0, FLAG_CARRIED = 1, FLAG_DROPPED = 2 };

class CTSFlag : public CBaseEntity
{
public:
	void Spawn() override;
	void Precache() override;
	int  ObjectCaps() override { return FCAP_CONTINUOUS_USE; } // @ObjectCaps __7CTSFlag returns 0x10

	void EXPORT FlagTouch(CBaseEntity *pOther); // @WaitTouch/PlayerTakeFlag — pickup/capture
	void EXPORT CarryThink();                   // follow the carrier; drop if they die
	void EXPORT ReturnThink();                  // auto-return a dropped flag

	void AttachTo(CBasePlayer *pPlayer);
	void DropAt(const Vector &vecOrigin);
	void ReturnToBase();

	// @CanInteract — can a player of this flag's team act on it (the flag's own team, or any
	// if neutral). The .so gates the same-team capture/return branch on this.
	bool CanInteract(CBasePlayer *pPlayer);

	static CTSFlag *Create(int team, const Vector &vecBase);

	int     m_iFlagTeam = -1;   // +0x9c — the team this flag belongs to (-1 = neutral)
	Vector  m_vecBase;          // home position (returns here)
	EHANDLE m_hCarrier;         // +0xa4 — the carrying player while FLAG_CARRIED
	float   m_flReturnTime = 0; // +0x88 — when a dropped flag auto-returns
	int     m_state = FLAG_AT_BASE;
};

LINK_ENTITY_TO_CLASS(ts_flag, CTSFlag)

void CTSFlag::Precache()
{
	PRECACHE_MODEL(FLAG_MODEL);
	PRECACHE_SOUND("items/gunpickup2.wav"); // TakeFlag pickup sound (the .so)
	PRECACHE_SOUND("items/weapondrop1.wav"); // drop/bounce sound
}

// @ Spawn — set up the idle flag at its base (briefcase model, touchable trigger). The model +
// sounds are precached at map load (ts_weapons.cpp PrecacheAllTSWeapons) since the flag is created
// at runtime — calling Precache here would PRECACHE_MODEL after spawn and Host_Error.
void CTSFlag::Spawn()
{
	SET_MODEL(edict(), FLAG_MODEL);
	UTIL_SetSize(pev, Vector(-16, -16, 0), Vector(16, 16, 24));

	pev->movetype = MOVETYPE_TOSS;
	pev->solid = SOLID_TRIGGER;
	if (m_vecBase == g_vecZero)
		m_vecBase = pev->origin;
	UTIL_SetOrigin(pev, m_vecBase);

	m_state = FLAG_AT_BASE;
	m_hCarrier = NULL;
	SetTouch(&CTSFlag::FlagTouch);
	SetThink(NULL);
	pev->nextthink = 0;
}

bool CTSFlag::CanInteract(CBasePlayer *pPlayer)
{
	if (m_iFlagTeam == -1)
		return true;
	return m_iFlagTeam == TSGameRules()->GetTeamIndex(pPlayer->TeamID());
}

// @ PlayerTakeFlag — a player touched the flag. If it's an enemy/neutral flag and they're
// not already carrying one, they pick it up. If it's their OWN team's flag (at base) and
// they're carrying the enemy flag, that's a capture.
void CTSFlag::FlagTouch(CBaseEntity *pOther)
{
	if (!pOther || !pOther->IsPlayer() || !pOther->IsAlive())
		return;
	if (m_state == FLAG_CARRIED)
		return;

	CBasePlayer *pPlayer = (CBasePlayer *)pOther;
	int playerTeam = TSGameRules()->GetTeamIndex(pPlayer->TeamID());

	bool ownFlag = (m_iFlagTeam != -1 && m_iFlagTeam == playerTeam);

	if (ownFlag)
	{
		// Capture: this is our flag (at base) and we're carrying the enemy flag.
		if (m_state == FLAG_AT_BASE && (CBaseEntity *)pPlayer->m_hCarriedFlag != NULL)
		{
			CTSFlag *pCarried = (CTSFlag *)(CBaseEntity *)pPlayer->m_hCarriedFlag;
			pPlayer->m_hCarriedFlag = NULL;
			pCarried->ReturnToBase();

			TSGameRules()->TeamWin(playerTeam); // SLICE: faithful capture scores cash + a HUD, not a round win
			const char *who = TSGameRules()->GetIndexedTeamName(playerTeam);
			TSHudMessageAll(0, 0xff, 0, 4, 30,
				UTIL_VarArgs("%s captured the flag!", (who && who[0]) ? who : "A team"));
		}
		return;
	}

	// Enemy/neutral flag: pick it up if we aren't already carrying one.
	if ((CBaseEntity *)pPlayer->m_hCarriedFlag != NULL)
		return;

	AttachTo(pPlayer);
}

// Attach the flag to a carrier: it follows them and can't be re-touched until dropped.
void CTSFlag::AttachTo(CBasePlayer *pPlayer)
{
	m_hCarrier = pPlayer;
	pPlayer->m_hCarriedFlag = this;
	m_state = FLAG_CARRIED;

	pev->solid = SOLID_NOT;
	SetTouch(NULL);
	EMIT_SOUND(edict(), CHAN_ITEM, "items/gunpickup2.wav", 1.0, ATTN_NORM);

	SetThink(&CTSFlag::CarryThink);
	pev->nextthink = gpGlobals->time + 0.1f;

	const char *who = TSGameRules()->GetIndexedTeamName(TSGameRules()->GetTeamIndex(pPlayer->TeamID()));
	TSHudMessageAll(0, 0xff, 0, 4, 20,
		UTIL_VarArgs("%s has the flag!", (who && who[0]) ? who : "Someone"));
}

// While carried, ride along on the carrier's back; drop if the carrier dies or leaves.
void CTSFlag::CarryThink()
{
	CBasePlayer *pCarrier = (CBasePlayer *)(CBaseEntity *)m_hCarrier;
	if (!pCarrier || !pCarrier->IsPlayer() || !pCarrier->IsAlive())
	{
		// carrier gone — drop where they were (or at our last spot)
		DropAt(pCarrier ? pCarrier->pev->origin : pev->origin);
		return;
	}

	UTIL_SetOrigin(pev, pCarrier->pev->origin + Vector(0, 0, 24));
	pev->angles.y = pCarrier->pev->angles.y;
	pev->nextthink = gpGlobals->time + 0.1f;
}

// @ DropFlag — the carrier lost the flag: it falls where they were and starts a return timer.
void CTSFlag::DropAt(const Vector &vecOrigin)
{
	CBasePlayer *pCarrier = (CBasePlayer *)(CBaseEntity *)m_hCarrier;
	if (pCarrier)
		pCarrier->m_hCarriedFlag = NULL;
	m_hCarrier = NULL;

	m_state = FLAG_DROPPED;
	pev->solid = SOLID_TRIGGER;
	UTIL_SetOrigin(pev, vecOrigin);
	EMIT_SOUND(edict(), CHAN_ITEM, "items/weapondrop1.wav", 1.0, ATTN_NORM);

	SetTouch(&CTSFlag::FlagTouch);
	SetThink(&CTSFlag::ReturnThink);
	m_flReturnTime = gpGlobals->time + FLAG_RETURN_TIME;
	pev->nextthink = gpGlobals->time + 0.5f;
}

void CTSFlag::ReturnThink()
{
	if (gpGlobals->time >= m_flReturnTime)
	{
		ReturnToBase();
		return;
	}
	pev->nextthink = gpGlobals->time + 0.5f;
}

// @ RespawnFlag — send the flag home, idle and touchable again.
void CTSFlag::ReturnToBase()
{
	CBasePlayer *pCarrier = (CBasePlayer *)(CBaseEntity *)m_hCarrier;
	if (pCarrier)
		pCarrier->m_hCarriedFlag = NULL;
	m_hCarrier = NULL;

	m_state = FLAG_AT_BASE;
	pev->solid = SOLID_TRIGGER;
	UTIL_SetOrigin(pev, m_vecBase);
	SetTouch(&CTSFlag::FlagTouch);
	SetThink(NULL);
	pev->nextthink = 0;
}

// @ CreateFlag — spawn a flag of `team` at `vecBase`.
CTSFlag *CTSFlag::Create(int team, const Vector &vecBase)
{
	CTSFlag *pFlag = GetClassPtr((CTSFlag *)NULL);
	pFlag->pev->classname = MAKE_STRING("ts_flag");
	pFlag->m_iFlagTeam = team;
	pFlag->m_vecBase = vecBase;
	pFlag->pev->origin = vecBase;
	pFlag->Spawn();
	return pFlag;
}

// Free helpers used by the gamerules / console driver.
CBaseEntity *TSCreateFlag(int team, const Vector &vecBase)
{
	return CTSFlag::Create(team, vecBase);
}

// @ DropFlag — drop the flag a player is carrying (on death / disconnect). No-op if none.
void TSDropFlag(CBasePlayer *pPlayer)
{
	CTSFlag *pFlag = (CTSFlag *)(CBaseEntity *)pPlayer->m_hCarriedFlag;
	if (pFlag)
		pFlag->DropAt(pPlayer->pev->origin);
}

// @ HaveFlag — is this player carrying a CTF flag? (Used by CTSBriefcase capture.)
bool TSPlayerHasFlag(CBasePlayer *pPlayer)
{
	return (CBaseEntity *)pPlayer->m_hCarriedFlag != NULL;
}

// Return the flag a player is carrying to its base (the capture side; the caller scores). No-op
// if none. Used by CTSBriefcase::CaptureTouch.
void TSReturnCarriedFlag(CBasePlayer *pPlayer)
{
	CTSFlag *pFlag = (CTSFlag *)(CBaseEntity *)pPlayer->m_hCarriedFlag;
	if (pFlag)
	{
		pPlayer->m_hCarriedFlag = NULL;
		pFlag->ReturnToBase();
	}
}

