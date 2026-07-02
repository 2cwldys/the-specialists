/***
*  TS_entities.cpp — RECONSTRUCTED original TU (funcfile.tsv): the small map-entity classes,
*  in the binary's address order: CTSTrigger (lands here in C13), CTSSlowMotion,
*  CTSSlowMotionPoint, CTSDMHill, CTSModel, CTSMapGlobals, CTSSellZone, CTSPlayerAmount,
*  CTSTimer, CTSCustomTarget. Merged from the former per-class files (see git history).
***/
#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "player.h"
#include "gamerules.h"
#include "ts_gamerules.h"
#include "ts_utils.h"        // TSHudMessageAll
#include "UserMessages.h"    // gmsgScoreInfo (CTSDMHill)
#include "ts_objectives.h"   // CTSTrigger cross-deps (C13)
#include "ts_slowmotion.h"
#include "ts_slowmotionpoint.h"
#include "ts_dmhill.h"
#include "ts_mapglobals.h"
#include "ts_playeramount.h"

// CTSTrigger lands here (C13) — first in the original address order.

// ===================== ts_slowmotion.cpp (merged) =====================
#define SF_SLOMO_START_ON 1
#define SF_SLOMO_ALL_MAP 2
#define SF_SLOMO_SLOW_BULLETS 4

LINK_ENTITY_TO_CLASS(ts_slowmotion, CTSSlowMotion);

TYPEDESCRIPTION CTSSlowMotion::m_SaveData[] = {
	DEFINE_FIELD(CTSSlowMotion, duration, FIELD_FLOAT),
	DEFINE_FIELD(CTSSlowMotion, slow_ammount, FIELD_FLOAT),
};
IMPLEMENT_SAVERESTORE(CTSSlowMotion, CBaseEntity);

bool CTSSlowMotion::KeyValue(KeyValueData *pkvd)
{
	if (FStrEq(pkvd->szKeyName, "duration"))
	{
		duration = atof(pkvd->szValue);
		return true;
	}
	if (FStrEq(pkvd->szKeyName, "slowamt"))
	{
		slow_ammount = atof(pkvd->szValue);
		return true;
	}
	return CBaseEntity::KeyValue(pkvd);
}

void CTSSlowMotion::Spawn()
{
	pev->classname = MAKE_STRING("ts_slowmotion");
	SET_MODEL(ENT(pev), STRING(pev->model)); // brush model
	UTIL_SetSize(pev, pev->mins, pev->maxs);

	// The original's Spawn is minimal (classname/model/size); a brush trigger needs
	// the following to detect touches + poll. // VERIFY against engine defaults.
	pev->solid = SOLID_TRIGGER;
	pev->movetype = MOVETYPE_NONE;
	pev->effects |= EF_NODRAW;

	m_iAllMap = (pev->spawnflags & SF_SLOMO_ALL_MAP) ? 1 : 0;
	m_bSlowBullets = (pev->spawnflags & SF_SLOMO_SLOW_BULLETS) ? 1 : 0;
	m_flRadius = pev->size.Length() * 0.5f; // VERIFY: drop-range from the brush bounds

	SetThink(&CTSSlowMotion::WaitThink);
	pev->nextthink = gpGlobals->time + 0.5f;

	if (pev->spawnflags & SF_SLOMO_START_ON)
		ActivateSlow();
}

// @ ActivateSlow__13CTSSlowMotion — arm the zone (timed or toggle), set the touch.
void CTSSlowMotion::ActivateSlow()
{
	if (duration > 0.0f)
	{
		m_flStopTime = gpGlobals->time + duration;
	}
	else
	{
		m_flStopTime = 0;
		m_iToggledOn ^= 1;
		if (m_iToggledOn == 0)
			DeactivateSlow();
	}

	if (!m_iAllMap)
		SetTouch(&CTSSlowMotion::WaitTouch);
}

// @ DeactivateSlow__13CTSSlowMotion — restore everyone the zone slowed, clear it.
void CTSSlowMotion::DeactivateSlow()
{
	m_flStopTime = 0;

	if (!m_iAllMap)
	{
		for (int i = 0; i < TS_SLOMO_MAX_PLAYERS; i++)
		{
			CBaseEntity *pEnt = m_hPlayers[i];
			if (pEnt && pEnt->IsPlayer())
				((CBasePlayer *)pEnt)->GoSlow(1.0f, 0);
			m_hPlayers[i] = NULL;
		}
	}
	else
	{
		TSGameRules()->GlobalSlowMotion2(0, 0);
		for (int i = 0; i < TS_SLOMO_MAX_PLAYERS; i++)
			m_hPlayers[i] = NULL;
	}
}

// @ WaitTouch__13CTSSlowMotion — slow the touching player (if not already slowed).
void CTSSlowMotion::WaitTouch(CBaseEntity *pOther)
{
	if (m_flStopTime == 0.0f && m_iToggledOn == 0)
		return; // zone not active

	if (!pOther->IsPlayer())
		return;

	CBasePlayer *pPlayer = (CBasePlayer *)pOther;
	if (pPlayer->InSlow())
		return; // already slowed (by this or another source)

	pPlayer->GoSlow(slow_ammount, 0);
	pPlayer->m_bSlowBullets = m_bSlowBullets != 0;

	int idx = ENTINDEX(pOther->edict());
	if (idx >= 0 && idx < TS_SLOMO_MAX_PLAYERS)
		m_hPlayers[idx] = pOther;
}

// @ WaitThink__13CTSSlowMotion — poll: keep the slow applied / time it out.
void CTSSlowMotion::WaitThink()
{
	pev->nextthink = gpGlobals->time + 0.5f;

	if (m_iToggledOn)
	{
		if (!m_iAllMap)
			CheckPlayersDistance();
		else
			TSGameRules()->GlobalSlowMotion2(slow_ammount, m_bSlowBullets);
	}

	if (m_flStopTime != 0.0f)
	{
		if (m_flStopTime < gpGlobals->time)
		{
			SetTouch(NULL);
			DeactivateSlow();
		}
		else if (!m_iAllMap)
		{
			CheckPlayersDistance();
		}
		else
		{
			TSGameRules()->GlobalSlowMotion2(slow_ammount, m_bSlowBullets);
		}
	}
}

// @ CheckPlayersDistance__13CTSSlowMotion — drop tracked players who left the zone
// (restore them to normal speed). Reconstructed from intent (the decompile is heavy
// stack arithmetic): distance from the zone centre, beyond m_flRadius -> released.
void CTSSlowMotion::CheckPlayersDistance()
{
	Vector center = Center();
	for (int i = 0; i < TS_SLOMO_MAX_PLAYERS; i++)
	{
		CBaseEntity *pEnt = m_hPlayers[i];
		if (!pEnt || !pEnt->IsPlayer())
			continue;

		CBasePlayer *pPlayer = (CBasePlayer *)pEnt;
		if (!pPlayer->InSlow())
			continue;

		float dist = (pPlayer->pev->origin - center).Length();
		if (dist > m_flRadius)
		{
			pPlayer->GoSlow(1.0f, 0);
			m_hPlayers[i] = NULL;
		}
	}
}

// @ Use__13CTSSlowMotion — trigger the zone (button/relay), announce, fire targets.
void CTSSlowMotion::Use(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value)
{
	ActivateSlow();

	if (!m_iAllMap)
		SetTouch(&CTSSlowMotion::WaitTouch);

	if (!FStringNull(pev->message))
		TSHudMessageAll(0, 255, 0, 8, 30, STRING(pev->message));

	if (!FStringNull(pev->target))
		FireTargets(STRING(pev->target), this, this, USE_TOGGLE, 0);
}

// ===================== ts_slowmotionpoint.cpp (merged) =====================

LINK_ENTITY_TO_CLASS(ts_slowmotionpoint, CTSSlowMotionPoint);

void CTSSlowMotionPoint::Spawn()
{
	pev->movetype = MOVETYPE_NONE;
	pev->solid = SOLID_NOT;
	pev->effects |= EF_NODRAW;
	UTIL_SetSize(pev, g_vecZero, g_vecZero);
	UTIL_SetOrigin(pev, pev->origin);
}

// @ActivateSlow — arm the bubble: slow factor + lifetime, then start polling.
void CTSSlowMotionPoint::ActivateSlow(float scale, float duration, int slowBullets)
{
	m_flScale = scale;
	m_flEndTime = gpGlobals->time + duration;
	m_iSlowBullets = slowBullets;
	SetThink(&CTSSlowMotionPoint::WaitThink);
	pev->nextthink = gpGlobals->time + 0.1f;
}

// @WaitThink — every 0.5s, GoSlow each living player (not yet slowed) that has line of sight
// to the point; when the lifetime ends, restore them all and remove.
void CTSSlowMotionPoint::WaitThink()
{
	pev->nextthink = gpGlobals->time + 0.5f;

	if (gpGlobals->time > m_flEndTime)
	{
		DeactivateSlow();
		UTIL_Remove(this);
		return;
	}

	for (int i = 1; i <= gpGlobals->maxClients; i++)
	{
		CBasePlayer *p = (CBasePlayer *)UTIL_PlayerByIndex(i);
		if (!p || !p->IsAlive() || p->edict() == pev->owner)
			continue;
		if (p->InSlow()) // already in slow-motion (by us or another source)
			continue;

		TraceResult tr;
		UTIL_TraceLine(pev->origin, p->pev->origin + p->pev->view_ofs,
			ignore_monsters, ignore_glass, ENT(pev), &tr);
		if (tr.flFraction < 1.0f)
			continue; // no line of sight

		p->GoSlow(m_flScale, m_iSlowBullets);
		if (m_nSlowed < MAX_SLOWED)
			m_hSlowed[m_nSlowed++] = p;
	}
}

// @DeactivateSlow — restore everyone this bubble slowed to normal speed.
void CTSSlowMotionPoint::DeactivateSlow()
{
	for (int i = 0; i < m_nSlowed; i++)
	{
		CBasePlayer *p = (CBasePlayer *)(CBaseEntity *)m_hSlowed[i];
		if (p)
			p->GoSlow(1.0f, 0);
	}
	m_nSlowed = 0;
}

// @CreateSlowMotionPoint — spawn a slow-motion bubble at `origin`.
CTSSlowMotionPoint *CTSSlowMotionPoint::CreateSlowMotionPoint(const Vector &origin, float scale, float duration,
	int slowBullets, CBaseEntity *pOwner)
{
	CTSSlowMotionPoint *pPoint = GetClassPtr((CTSSlowMotionPoint *)NULL);
	pPoint->pev->classname = MAKE_STRING("ts_slowmotionpoint");
	UTIL_SetOrigin(pPoint->pev, origin);
	pPoint->Spawn();
	if (pOwner)
		pPoint->pev->owner = pOwner->edict();
	pPoint->ActivateSlow(scale, duration, slowBullets);
	return pPoint;
}

// Spawn a slow-motion bubble (used by the slow-motion powerup activation).
CBaseEntity *TSCreateSlowMotionPoint(const Vector &origin, float scale, float duration,
	int slowBullets, CBaseEntity *pOwner)
{
	return CTSSlowMotionPoint::CreateSlowMotionPoint(origin, scale, duration, slowBullets, pOwner);
}

// ===================== ts_dmhill.cpp (merged) =====================

LINK_ENTITY_TO_CLASS(ts_dmhill, CTSDMHill);

TYPEDESCRIPTION CTSDMHill::m_SaveData[] = {
	DEFINE_FIELD(CTSDMHill, nextcheck, FIELD_FLOAT),
	DEFINE_FIELD(CTSDMHill, fragseverycheck, FIELD_INTEGER),
	DEFINE_FIELD(CTSDMHill, fragseverycheckteam, FIELD_INTEGER),
};
IMPLEMENT_SAVERESTORE(CTSDMHill, CBaseEntity);

bool CTSDMHill::KeyValue(KeyValueData *pkvd)
{
	if (FStrEq(pkvd->szKeyName, "nextcheck"))
	{
		nextcheck = atof(pkvd->szValue);
		return true;
	}
	if (FStrEq(pkvd->szKeyName, "fragamt"))     { fragseverycheck = atoi(pkvd->szValue); return true; }
	if (FStrEq(pkvd->szKeyName, "fragamtteam")) { fragseverycheckteam = atoi(pkvd->szValue); return true; }
	return CBaseEntity::KeyValue(pkvd);
}

// @Spawn__9CTSDMHill — a (usually brush) trigger volume. Only arms the touch handler if the relevant
// frag amount is configured for the active mode (DM: fragamt>0; team: fragamtteam>0).
void CTSDMHill::Spawn()
{
	if (!FStringNull(pev->model))
		SET_MODEL(ENT(pev), STRING(pev->model)); // the brush, from the map
	else
		UTIL_SetSize(pev, Vector(-48, -48, -36), Vector(48, 48, 36)); // synthetic/point fallback
	pev->effects |= EF_NODRAW;
	pev->solid = SOLID_TRIGGER; // non-blocking trigger volume
	pev->movetype = MOVETYPE_NONE;
	UTIL_SetOrigin(pev, pev->origin);

	const bool team = TSGameRules() && TSGameRules()->IsTeamMode();
	if ((!team || fragseverycheckteam > 0) && (!team || fragseverycheck > 0))
		SetTouch(&CTSDMHill::WaitTouch);
	else
		SetTouch(NULL);

	SetThink(&CTSDMHill::WaitThink);
	float check = (nextcheck < 0.5f) ? 0.5f : nextcheck;
	pev->nextthink = gpGlobals->time + check;
	m_flStartGate = gpGlobals->time + 1.0f; // 1s startup grace
	m_hKing = NULL;
}

// @WaitThink — keep the entity thinking (the holder logic is all in WaitTouch).
void CTSDMHill::WaitThink()
{
	float check = (nextcheck < 0.5f) ? 0.5f : nextcheck;
	pev->nextthink = gpGlobals->time + check;
}

// @WaitTouch — a living player overlapping the volume. Track the holder; pay out once they've held
// past the award time. A same-team toucher keeps the hold; an enemy toucher drops it.
void CTSDMHill::WaitTouch(CBaseEntity *pOther)
{
	if (gpGlobals->time < m_flStartGate)
		return;
	if (!pOther || !pOther->IsPlayer() || !pOther->IsAlive())
		return;
	CBasePlayer *pPlayer = (CBasePlayer *)pOther;
	const float check = (nextcheck < 0.5f) ? 0.5f : nextcheck;

	// No holder yet — this toucher claims it and starts the hold timer.
	if (!((CBaseEntity *)m_hKing))
	{
		m_hKing = pPlayer;
		m_flStartGate = 0;
		m_flAwardTime = gpGlobals->time + check;
		m_flLastTouch = gpGlobals->time + check;
		return;
	}

	if ((CBaseEntity *)m_hKing == pPlayer)
	{
		// the holder is still here — drop them only if their touches lapsed past a full check window
		if (check < gpGlobals->time - m_flLastTouch)
		{
			m_hKing = NULL;
			m_flLastTouch = gpGlobals->time + check;
			return;
		}
	}
	else
	{
		// a DIFFERENT player touched — a teammate keeps the hold, an enemy steals/resets it
		CBaseEntity *pKing = m_hKing;
		bool sameTeam = false;
		if (pKing && TSGameRules() && TSGameRules()->IsTeamMode())
			sameTeam = TSGameRules()->GetTeamIndex(((CBasePlayer *)pKing)->TeamID()) ==
				TSGameRules()->GetTeamIndex(pPlayer->TeamID());
		if (!sameTeam)
		{
			m_hKing = NULL;
			m_flLastTouch = gpGlobals->time + check;
			return;
		}
	}

	if (gpGlobals->time <= m_flAwardTime)
		return; // held, but not long enough yet

	// --- PAYOUT ---
	const bool team = TSGameRules() && TSGameRules()->IsTeamMode();
	char kingMsg[128] = {0};
	if (!FStringNull(pev->message))
	{
		if (team)
		{
			strcpy(kingMsg, "Team ");
			strcat(kingMsg, pPlayer->TeamID());
		}
		else
		{
			strncpy(kingMsg, STRING(pPlayer->pev->netname), sizeof(kingMsg) - 1);
		}
		strcat(kingMsg, " ");
		strncat(kingMsg, STRING(pev->message), sizeof(kingMsg) - strlen(kingMsg) - 1);
	}

	if (!team)
	{
		pPlayer->pev->frags += fragseverycheck;
		MESSAGE_BEGIN(MSG_ALL, gmsgScoreInfo);
		WRITE_BYTE(pPlayer->entindex());
		WRITE_SHORT((int)pPlayer->pev->frags);
		WRITE_SHORT(pPlayer->m_iDeaths);
		WRITE_SHORT(0);
		WRITE_SHORT(g_pGameRules->GetTeamIndex(pPlayer->m_szTeamName) + 1);
		MESSAGE_END();
		if (!FStringNull(pev->message))
			strncat(kingMsg, UTIL_VarArgs(" ,%i frags for him", fragseverycheck),
				sizeof(kingMsg) - strlen(kingMsg) - 1);
	}
	else
	{
		// award every player on the holder's team
		CBaseEntity *pEnt = NULL;
		while ((pEnt = UTIL_FindEntityByClassname(pEnt, "player")) != NULL)
		{
			if (!pEnt->IsPlayer())
				continue;
			CBasePlayer *p = (CBasePlayer *)pEnt;
			if (strcmp(p->TeamID(), pPlayer->TeamID()) != 0)
				continue;
			p->pev->frags += fragseverycheckteam;
			MESSAGE_BEGIN(MSG_ALL, gmsgScoreInfo);
			WRITE_BYTE(p->entindex());
			WRITE_SHORT((int)p->pev->frags);
			WRITE_SHORT(p->m_iDeaths);
			WRITE_SHORT(0);
			WRITE_SHORT(g_pGameRules->GetTeamIndex(p->m_szTeamName) + 1);
			MESSAGE_END();
		}
		if (!FStringNull(pev->message))
			strncat(kingMsg, UTIL_VarArgs(" ,%i frags for they", fragseverycheckteam),
				sizeof(kingMsg) - strlen(kingMsg) - 1);
	}

	if (!FStringNull(pev->message))
	{
		float dur = check * 10.0f;
		if (dur > 30.0f) dur = 30.0f;
		TSHudMessageAll(0, 0xff, 0, 8, (int)(dur + 0.5f), kingMsg);
	}

	m_flAwardTime = gpGlobals->time + check;
	if (!FStringNull(pev->target))
		FireTargets(STRING(pev->target), this, this, USE_TOGGLE, 0);
	m_flLastTouch = gpGlobals->time;
}

// ===================== ts_model.cpp (merged) =====================

class CTSModel : public CBaseAnimating
{
public:
	void Spawn(void) override;
	void Precache(void) override;
};

LINK_ENTITY_TO_CLASS(ts_model, CTSModel);

// Original Precache is empty (the model is precached in Spawn); kept because
// Spawn calls it through the vtable.
void CTSModel::Precache(void)
{
}

void CTSModel::Spawn(void)
{
	Precache();
	pev->classname = MAKE_STRING("ts_model");

	if (FStringNull(pev->model))
	{
		// No model assigned — nothing to display, so remove ourselves.
		REMOVE_ENTITY(ENT(pev));
		return;
	}

	PRECACHE_MODEL((char *)STRING(pev->model));
	SET_MODEL(ENT(pev), STRING(pev->model));
	UTIL_SetSize(pev, Vector(0, 0, 0), Vector(0, 0, 0));
	UTIL_SetOrigin(pev, pev->origin);

	InitBoneControllers();

	// Cycler-style decorative prop: non-moving, animates client-side from its
	// sequence. (Unlike CCycler it is not a damageable target and is point-sized.)
	pev->movetype = MOVETYPE_NONE;
	pev->solid = SOLID_SLIDEBOX;
	pev->effects = 0;
	pev->framerate = 1.0f;
	pev->yaw_speed = 5;
	pev->ideal_yaw = pev->angles.y;

	m_flFrameRate = 75;
	m_flGroundSpeed = 0;

	pev->nextthink += 1.0;

	ResetSequenceInfo();
}

// ===================== ts_mapglobals.cpp (merged) =====================

LINK_ENTITY_TO_CLASS(ts_mapglobals, CTSMapGlobals);

// @0x62488/0x624d8 — standard save/restore of the two vestigial fields (WriteFields count 2).
TYPEDESCRIPTION CTSMapGlobals::m_SaveData[] = {
	DEFINE_FIELD(CTSMapGlobals, player_at_a_time_num, FIELD_INTEGER),
	DEFINE_FIELD(CTSMapGlobals, player_respawn_num, FIELD_INTEGER),
};
IMPLEMENT_SAVERESTORE(CTSMapGlobals, CBaseEntity);

bool CTSMapGlobals::KeyValue(KeyValueData *pkvd)
{
	// Original marks every key unhandled — the entity has no keyvalues beyond the
	// engine-applied spawnflags/origin/angles.
	return false;
}

void CTSMapGlobals::Precache(void)
{
}

void CTSMapGlobals::Spawn(void)
{
	Precache();
	pev->movetype = MOVETYPE_NONE; // 0x108 = 0
	pev->solid = SOLID_NOT;        // 0x10c = 0
	UTIL_SetOrigin(pev, pev->origin);
	UTIL_SetSize(pev, g_vecZero, g_vecZero);

	player_at_a_time_num = 0;
	player_respawn_num = 0;
	m_field14 = 0;
	m_field18 = 0;

	SetThink(&CTSMapGlobals::WaitThink);
	pev->nextthink = gpGlobals->time + 10.0f;

	// SLICE: the original Spawn also pushes the spawnflags into CTSGameRules — round-timer enable
	// (!(SF & 0x20)), superjump (SF & 0x40), and the weapon-restriction mask (get_weapon_restriction()
	// stored into the rules' restriction fields @+0x586c/0x5870). Those CTSGameRules members aren't
	// reconstructed yet, so the wiring is deferred (the entity itself is now method/member-faithful).
}


// Bitmask of permitted weapon types: InitWeaponRestriction() (all five) XOR the "No <weapon>"
// spawnflags. Consumed by the game rules — kept here for faithfulness.
int InitWeaponRestriction(void); // defined in TS_gamerules.cpp (its original home)

int CTSMapGlobals::get_weapon_restriction(void)
{
	return InitWeaponRestriction() ^ pev->spawnflags;
}

// Present in the original but never scheduled by Spawn (an inert 10s
// self-reschedule); kept for completeness.
void CTSMapGlobals::WaitThink(void)
{
	pev->nextthink = gpGlobals->time + 10.0f;
}

// ===================== ts_sellzone.cpp (merged) =====================

#define SELLZONE_VALUE 500   // flat cash per weapon sold (price field is dead while CashBased off)

class CTSSellZone : public CBaseEntity
{
public:
	bool KeyValue(KeyValueData *pkvd) override;
	void Spawn() override;
	void EXPORT SellTouch(CBaseEntity *pOther);

	int m_attackerTeam = -1; // tsobjective attackerteam (-1 = any)
};

LINK_ENTITY_TO_CLASS(ts_sellzone, CTSSellZone)

bool CTSSellZone::KeyValue(KeyValueData *pkvd)
{
	if (FStrEq(pkvd->szKeyName, "attackerteam"))
	{
		m_attackerTeam = atoi(pkvd->szValue);
		return true;
	}
	return CBaseEntity::KeyValue(pkvd);
}

// @ Spawn — a brush trigger that watches for dropped weapons sliding in.
void CTSSellZone::Spawn()
{
	pev->solid = SOLID_TRIGGER;
	pev->movetype = MOVETYPE_NONE;
	if (!FStringNull(pev->model))
		SET_MODEL(edict(), STRING(pev->model)); // brush model
	UTIL_SetOrigin(pev, pev->origin);
	SetTouch(&CTSSellZone::SellTouch);
}

// @ SellTouch — a dropped weapon (classname "WorldGun") entered the zone: sell it. Credit the nearest
// living player (on the zone's team, if set) its value, then consume the gun.
void CTSSellZone::SellTouch(CBaseEntity *pOther)
{
	if (!pOther || !FClassnameIs(pOther->pev, "WorldGun"))
		return;

	// find the nearest living player to the sold gun (the seller stand-in)
	CBasePlayer *pBest = nullptr;
	float flBest = 256.0f * 256.0f;
	for (int i = 1; i <= gpGlobals->maxClients; i++)
	{
		CBaseEntity *pEnt = UTIL_PlayerByIndex(i);
		if (!pEnt || !pEnt->IsAlive())
			continue;
		if (m_attackerTeam != -1 && TSGameRules()->GetTeamIndex(pEnt->TeamID()) != m_attackerTeam)
			continue;
		float d = (pEnt->pev->origin - pOther->pev->origin).LengthSquared();
		if (d < flBest)
		{
			flBest = d;
			pBest = (CBasePlayer *)pEnt;
		}
	}

	if (pBest)
	{
		pBest->GiveCash(SELLZONE_VALUE);
		ClientPrint(pBest->pev, HUD_PRINTCENTER,
			UTIL_VarArgs("Sold weapon (+$%d)", SELLZONE_VALUE));
	}

	UTIL_Remove(pOther); // the gun is consumed by the sale
}


// ===================== ts_playeramount.cpp (merged) =====================

LINK_ENTITY_TO_CLASS(ts_playeramount, CTSPlayerAmount)

bool CTSPlayerAmount::KeyValue(KeyValueData *pkvd)
{
	if (FStrEq(pkvd->szKeyName, "clients"))          { m_iClients = atoi(pkvd->szValue); return true; }
	if (FStrEq(pkvd->szKeyName, "connecttarget"))    { m_iszConnectTarget = ALLOC_STRING(pkvd->szValue); return true; }
	if (FStrEq(pkvd->szKeyName, "disconnecttarget")) { m_iszDisconnectTarget = ALLOC_STRING(pkvd->szValue); return true; }
	return CBaseDelay::KeyValue(pkvd); // "delay" (m_flDelay) etc. handled by the base
}

void CTSPlayerAmount::Spawn()
{
	pev->classname = MAKE_STRING("ts_playeramount");
	pev->effects |= EF_NODRAW;
	pev->solid = SOLID_NOT;
	pev->movetype = MOVETYPE_FLY; // (original sets movetype 5; a non-colliding point relay)
	m_iCount = 0;
}

// @0x72aec ClientConnect — a client connected: count up; on reaching `clients`, fire connecttarget.
void CTSPlayerAmount::ClientConnect()
{
	if (++m_iCount != m_iClients)
		return;
	if (!FStringNull(m_iszConnectTarget))
	{
		// delay==0 fires now; the original schedules a DelayedUse for m_flDelay>0 (deferred refinement).
		FireTargets(STRING(m_iszConnectTarget), this, this, USE_TOGGLE, 0);
		if (pev->spawnflags & 1) // bit 1 = remove the relay after it fires
			UTIL_Remove(this);
	}
}

// @0x72c08 ClientDisconnect — a client left: count down; on reaching `clients`, fire disconnecttarget.
void CTSPlayerAmount::ClientDisconnect()
{
	if (--m_iCount != m_iClients)
		return;
	if (!FStringNull(m_iszDisconnectTarget))
	{
		FireTargets(STRING(m_iszDisconnectTarget), this, this, USE_TOGGLE, 0);
		if (pev->spawnflags & 1)
			UTIL_Remove(this);
	}
}

// ===================== ts_timer.cpp (merged) =====================

#define SF_TIMER_START_OFF 1   // FGD bit 1
#define SF_TIMER_LOOP      2   // FGD bit 2

class CTSTimer : public CBaseEntity
{
public:
	void Spawn(void);
	bool KeyValue(KeyValueData *pkvd);
	void Use(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value);
	void EXPORT WaitThink(void);
	void ToggleTimer(void);

	float m_flDuration;   // this+0x64 — "duration" keyvalue (seconds)
	int   m_iActive;      // this+0x68 — running toggle state
};

LINK_ENTITY_TO_CLASS(ts_timer, CTSTimer);

bool CTSTimer::KeyValue(KeyValueData *pkvd)
{
	if (FStrEq(pkvd->szKeyName, "duration"))
	{
		m_flDuration = atof(pkvd->szValue);
		return true;
	}
	return CBaseEntity::KeyValue(pkvd);
}

void CTSTimer::Spawn(void)
{
	pev->classname = MAKE_STRING("ts_timer");
	pev->effects |= EF_NODRAW;
	// Original writes two pev fields (0x108=5, 0x10c=0); a timer neither moves
	// nor collides, so the behaviourally-equivalent point-entity setup: (VERIFY)
	pev->solid = SOLID_NOT;
	pev->movetype = MOVETYPE_NONE;

	m_iActive = 0;
	SetThink(NULL);

	if (!(pev->spawnflags & SF_TIMER_START_OFF))
		ToggleTimer();
	else
		SetThink(NULL);
}

// @ ToggleTimer__8CTSTimer
void CTSTimer::ToggleTimer(void)
{
	if (m_flDuration < 1.0f)
	{
		SetThink(NULL);
		m_iActive = 0;
		return;
	}

	m_iActive ^= 1;
	if (!m_iActive)
	{
		SetThink(NULL);
	}
	else
	{
		SetThink(&CTSTimer::WaitThink);
		pev->nextthink = gpGlobals->time + m_flDuration;
	}
}

// @ WaitThink__8CTSTimer
void CTSTimer::WaitThink(void)
{
	if (pev->target)
		FireTargets(STRING(pev->target), this, this, USE_TOGGLE, 0);

	if (pev->message)
		TSHudMessageAll(0, 255, 0, 1, 30, STRING(pev->message));

	// Original: if (pev->spawnflags & 1) clear think, then reschedule.
	// (FGD bit 1 = "Start OFF"; reproduced faithfully — VERIFY vs "Loop".)
	if (pev->spawnflags & SF_TIMER_START_OFF)
		SetThink(NULL);

	pev->nextthink = gpGlobals->time + m_flDuration;
}

void CTSTimer::Use(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value)
{
	ToggleTimer();
}

// ===================== ts_customtarget.cpp (merged) =====================

// Original storage: a 0x800-byte block at this+0x64, 0x20 bytes/slot -> 64 slots
// of 32 chars (recovered offsets are provenance, not targets).
#define TS_MAX_TARGETS 64
#define TS_TARGET_LEN  32

// triggermethod values (FGD "fire method" choices)
#define TS_FIRE_ALL    0
#define TS_FIRE_RANDOM 1
#define TS_FIRE_SEQ    2

class CTSCustomTarget : public CBaseEntity
{
public:
	void Spawn(void);
	bool KeyValue(KeyValueData *pkvd);
	void Use(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value);

	void GetTargets(const char *pTargets);
	void FireTarget(int index);

	char m_szTargets[TS_MAX_TARGETS][TS_TARGET_LEN]; // this+0x64 — parsed target names
	int  m_iNumTargets;                              // this+0x864 — parsed count
	int  m_iTriggerMethod;                           // this+0x868 — 0/1/2
	int  m_iAccumulate;                              // this+0x86c — calls before fire
	int  m_iCurCount;                               // this+0x870 — Use counter
	int  m_iSeqIndex;                               // this+0x874 — sequential cursor
};

LINK_ENTITY_TO_CLASS(ts_customtarget, CTSCustomTarget);

// @ GetTargets__15CTSCustomTargetPc — split a ';'-separated list into m_szTargets.
void CTSCustomTarget::GetTargets(const char *pTargets)
{
	memset(m_szTargets, 0, sizeof(m_szTargets));
	m_iNumTargets = 0;

	if (!pTargets)
		return;

	const char *p = pTargets;
	while (*p && m_iNumTargets < TS_MAX_TARGETS)
	{
		int n = 0;
		while (*p && *p != ';' && n < TS_TARGET_LEN - 1)
			m_szTargets[m_iNumTargets][n++] = *p++;
		m_szTargets[m_iNumTargets][n] = '\0';
		m_iNumTargets++;

		// discard any overflow of an over-long token, then the separator
		while (*p && *p != ';')
			p++;
		if (*p == ';')
			p++;
	}
}

bool CTSCustomTarget::KeyValue(KeyValueData *pkvd)
{
	if (FStrEq(pkvd->szKeyName, "targets"))
	{
		GetTargets(pkvd->szValue);
		return true;
	}
	if (FStrEq(pkvd->szKeyName, "accumulate"))
	{
		m_iAccumulate = atoi(pkvd->szValue);
		return true;
	}
	if (FStrEq(pkvd->szKeyName, "triggermethod"))
	{
		m_iTriggerMethod = atoi(pkvd->szValue);
		return true;
	}
	// "message" -> pev->message, plus targetname/etc.
	return CBaseEntity::KeyValue(pkvd);
}

void CTSCustomTarget::Spawn(void)
{
	pev->classname = MAKE_STRING("ts_customtarget");
	pev->effects |= EF_NODRAW;
	// Original writes pev 0x108=5, 0x10c=0; like ts_timer this point relay neither
	// moves nor collides, so the behaviourally-equivalent setup: (VERIFY)
	pev->solid = SOLID_NOT;
	pev->movetype = MOVETYPE_NONE;

	m_iCurCount = 0;
	m_iSeqIndex = 0;
	// m_iAccumulate is zero-initialised by the engine; the FGD always emits
	// "accumulate" (default 1). With 0, Use still fires every call (== 1).
}

// @ FireTarget__15CTSCustomTargeti — index<0 or out of range fires ALL targets.
void CTSCustomTarget::FireTarget(int index)
{
	if (index < 0 || index >= m_iNumTargets)
	{
		for (int i = 0; i < m_iNumTargets; i++)
			FireTargets(m_szTargets[i], this, this, USE_TOGGLE, 0);
	}
	else
	{
		FireTargets(m_szTargets[index], this, this, USE_TOGGLE, 0);
	}
}

void CTSCustomTarget::Use(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value)
{
	if (++m_iCurCount < m_iAccumulate)
		return;

	m_iCurCount = 0;

	if (pev->message)
		TSHudMessageAll(0, 255, 0, 1, 30, STRING(pev->message));

	switch (m_iTriggerMethod)
	{
	case TS_FIRE_RANDOM:
		FireTarget(RANDOM_LONG(0, m_iNumTargets - 1));
		break;

	case TS_FIRE_SEQ:
	{
		int idx = m_iSeqIndex++;
		FireTarget(idx);
		if (m_iSeqIndex >= m_iNumTargets)
			m_iSeqIndex = 0;
		break;
	}

	case TS_FIRE_ALL:
	default:
		FireTarget(-1);
		break;
	}
}
