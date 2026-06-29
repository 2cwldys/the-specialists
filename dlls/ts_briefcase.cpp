/***
*  ts_briefcase.cpp — RECONSTRUCTED CTSBriefcase (cont.39), the CTF base + capture zone.
*  Functional/source-faithful (see CLAUDE.md). recovered/briefcase/CTSBriefcase.c:
*    ctor @0x8d788, KeyValue @0x8d7f4, Spawn @0x8d88c, FlagFall @0x8da00,
*    WaitThink @0x8db14, CanInteract @0x8db48, CaptureTouch @0x8db9c.
*
*  CTSBriefcase is the map-placed (`@SolidClass`, brush) CTF objective — one per team
*  (ownerteam 1..4). It seeds its team's carriable flag at its centre (FlagFall → the
*  cont.38 CTSFlag) and is the capture zone: an enemy carrying a flag who reaches THEIR own
*  briefcase captures it. It's the map-placed creator the cont.38 CTSFlag deferred to (it
*  replaces the `tsspawnflag` console stand-in). Verified live on ctb_deadheat.
*
*  FUNCTIONAL reconstruction onto the stock SDK: a brush trigger that creates its flag on a
*  short delay (so the model is precached) and captures a carrier of the enemy flag on touch.
*  // SLICE: the s_flag_status / per-flag network id (gmsgTSHaveFlag HUD) is server-only; the
*  faithful capture awards cash + a HUD, here it scores via the gamerules team score.
***/
#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "player.h"
#include "gamerules.h"
#include "ts_gamerules.h"
#include "ts_utils.h"   // TSHudMessageAll

// Defined in ts_flag.cpp.
CBaseEntity *TSCreateFlag(int team, const Vector &vecBase);
bool TSPlayerHasFlag(CBasePlayer *pPlayer);
void TSReturnCarriedFlag(CBasePlayer *pPlayer);

class CTSBriefcase : public CBaseEntity
{
public:
	bool KeyValue(KeyValueData *pkvd) override;
	void Spawn() override;
	// (no ObjectCaps override — the original CTSBriefcase uses base CBaseEntity::ObjectCaps; the
	// save/transition difference is inert in multiplayer, where there is no mid-game save/load.)

	void EXPORT FlagFall();                       // create this team's flag at the base
	void EXPORT WaitThink();                      // idle keepalive (matches the .so)
	void EXPORT CaptureTouch(CBaseEntity *pOther); // capture a carrier of the enemy flag

	// @CanInteract — can a player of this briefcase's team act on it (the owner team, or any
	// if unset). The .so gates the capture on this.
	bool CanInteract(CBasePlayer *pPlayer);

	int     m_iOwnerTeam = -1;   // +0x68 — owning team, 0-based (FGD ownerteam 1..4 minus 1)
	string_t m_iszContents = 0;  // FGD "contents"
	EHANDLE m_hFlag;             // the flag this briefcase seeded
};

LINK_ENTITY_TO_CLASS(ts_briefcase, CTSBriefcase)

// @ KeyValue — message (capture announce, via the Targetname base), contents, ownerteam.
// ownerteam is 1-based in the FGD; store it 0-based to match pev->team / the objective gating.
bool CTSBriefcase::KeyValue(KeyValueData *pkvd)
{
	if (FStrEq(pkvd->szKeyName, "ownerteam"))
	{
		m_iOwnerTeam = atoi(pkvd->szValue) - 1;
		return true;
	}
	if (FStrEq(pkvd->szKeyName, "contents"))
	{
		m_iszContents = ALLOC_STRING(pkvd->szValue);
		return true;
	}
	if (FStrEq(pkvd->szKeyName, "message"))
	{
		pev->message = ALLOC_STRING(pkvd->szValue);
		return true;
	}
	return CBaseEntity::KeyValue(pkvd);
}

// @ Spawn — set up the brush (capture trigger) and schedule the flag drop. FlagFall runs on a
// short delay so the briefcase-centre is final and the flag model (precached at map load) is
// SET_MODEL'd after the load pass.
void CTSBriefcase::Spawn()
{
	pev->solid = SOLID_TRIGGER;
	pev->movetype = MOVETYPE_NONE;
	SET_MODEL(edict(), STRING(pev->model)); // brush model
	UTIL_SetOrigin(pev, pev->origin);

	SetTouch(&CTSBriefcase::CaptureTouch);
	SetThink(&CTSBriefcase::FlagFall);
	pev->nextthink = gpGlobals->time + 0.5f;
}

// @ FlagFall — create this team's carriable flag at the briefcase centre, then idle.
void CTSBriefcase::FlagFall()
{
	m_hFlag = TSCreateFlag(m_iOwnerTeam, Center());

	SetThink(&CTSBriefcase::WaitThink);
	pev->nextthink = gpGlobals->time + 1.0f;
}

void CTSBriefcase::WaitThink()
{
	pev->nextthink = gpGlobals->time + 1.0f;
}

bool CTSBriefcase::CanInteract(CBasePlayer *pPlayer)
{
	if (m_iOwnerTeam == -1)
		return true;
	return m_iOwnerTeam == TSGameRules()->GetTeamIndex(pPlayer->TeamID());
}

// @ CaptureTouch — a living player carrying the enemy flag who reaches their OWN briefcase
// captures it: the carried flag returns to its base and the capturing team scores.
void CTSBriefcase::CaptureTouch(CBaseEntity *pOther)
{
	if (!pOther || !pOther->IsPlayer() || !pOther->IsAlive())
		return;

	CBasePlayer *pPlayer = (CBasePlayer *)pOther;
	if (!TSPlayerHasFlag(pPlayer))     // must be carrying a flag
		return;
	if (!CanInteract(pPlayer))         // must be this player's own briefcase
		return;

	int team = TSGameRules()->GetTeamIndex(pPlayer->TeamID());
	TSReturnCarriedFlag(pPlayer);
	TSGameRules()->TeamWin(team);      // SLICE: faithful capture awards cash + a HUD, not a round win

	const char *who = TSGameRules()->GetIndexedTeamName(team);
	const char *msg = !FStringNull(pev->message) ? STRING(pev->message) : nullptr;
	TSHudMessageAll(0, 0xff, 0, 4, 30, msg ? (char *)msg :
		UTIL_VarArgs("%s captured the briefcase!", (who && who[0]) ? who : "A team"));
}
