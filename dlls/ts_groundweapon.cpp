/***
*  ts_groundweapon.cpp — RECONSTRUCTED CTSGroundWeapon from ts_i386.so (TS_worldgun.cpp) + FGD.
*  Source-faithful method layout (see CLAUDE.md). The map weapon pickup: a settle-then-pickup
*  state machine (Spawn point-bbox falls → Fall waits for the floor → OnGround arms a SOLID_TRIGGER
*  grab box → WaitTouch/Use hands the weapon over via GetWeapon).
*
*  @PointClass base(Target, tsweaponlist, tsobjective) = ts_groundweapon
*    tsweaponid -> tsweaponid (0xa4) · wduration -> respawntime (0xa0) · wextraclip -> extraclip (0xa8)
*    attackerteam -> attackerteam (0xac) · buyable -> buyable (0xb0)
*    spawnflags: 1 Silencer, 2 LaserSight, 4 Flashlight, 8 Scope, 16 Lie-on-wall
*
*  SLICE: GetWeapon hands the weapon over via our GiveTSWeapon (the dual-wield arsenal); the original
*  routes through CBasePlayer::PickUpWeapon (not yet reconstructed) + the cash/buy-menu price logic
*  (deferred — VERIFY). The settle physics, USE/0x1d-touch gating, respawn schedule + Save table are faithful.
***/
#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "player.h"
#include "ts_weapons.h"
#include "ts_gamerules.h"
#include "ts_groundweapon.h"

#define SF_GW_ON_WALL 16 // "Lie on wall"

LINK_ENTITY_TO_CLASS(ts_groundweapon, CTSGroundWeapon);

TYPEDESCRIPTION CTSGroundWeapon::m_SaveData[] = {
	DEFINE_FIELD(CTSGroundWeapon, respawntime, FIELD_INTEGER),
	DEFINE_FIELD(CTSGroundWeapon, tsweaponid, FIELD_INTEGER),
	DEFINE_FIELD(CTSGroundWeapon, extraclip, FIELD_INTEGER),
};
IMPLEMENT_SAVERESTORE(CTSGroundWeapon, CBaseEntity);

bool CTSGroundWeapon::KeyValue(KeyValueData *pkvd)
{
	if (FStrEq(pkvd->szKeyName, "tsweaponid"))   { tsweaponid = atoi(pkvd->szValue); return true; }
	if (FStrEq(pkvd->szKeyName, "wduration"))    { respawntime = atoi(pkvd->szValue); return true; }
	if (FStrEq(pkvd->szKeyName, "wextraclip"))   { extraclip = atoi(pkvd->szValue); return true; }
	if (FStrEq(pkvd->szKeyName, "attackerteam")) { attackerteam = atoi(pkvd->szValue); return true; }
	if (FStrEq(pkvd->szKeyName, "buyable"))      { buyable = atoi(pkvd->szValue) != 0; return true; }
	return CBaseEntity::KeyValue(pkvd);
}

// NB the original's Precache is EMPTY (the weapon models are precached by the weapon-table at game
// init). We have no global TS-weapon precache pass, so precache the roster here so SET_MODEL below
// (and a runtime hand-out) is legal.
void CTSGroundWeapon::Precache()
{
	PrecacheAllTSWeapons();
}

void CTSGroundWeapon::Spawn()
{
	Precache();

	const s_weapon_info *wi = get_weapon_info((tsweapon_t)tsweaponid);
	if (!wi || !wi->pszName)
	{
		// unfilled weapon row -> self-delete (orig: SetThink(SUB_Remove) + nextthink 0.1).
		SetThink(&CTSGroundWeapon::SUB_Remove);
		pev->nextthink = gpGlobals->time + 0.1f;
		return;
	}
	// akimbo single -> upgrade target when the row marks it (wi+0xcc != -1 && firetype 4).
	if (wi->iSingleType != -1 && wi->iFireType == 4)
		tsweaponid = wi->iSingleType;

	const char *world = wi->pszWorldModel ? wi->pszWorldModel : "models/w_weaponbox.mdl";
	PRECACHE_MODEL((char *)world);
	SET_MODEL(ENT(pev), world);

	pev->movetype = (pev->spawnflags & SF_GW_ON_WALL) ? MOVETYPE_FLY : MOVETYPE_TOSS;
	pev->solid = SOLID_BBOX;
	UTIL_SetOrigin(pev, pev->origin);
	UTIL_SetSize(pev, g_vecZero, g_vecZero); // point bbox while falling (players pass through)

	SetTouch(NULL);
	SetThink(&CTSGroundWeapon::Fall);
	pev->nextthink = gpGlobals->time + 0.1f;
	pev->iuser4 = 0;
	pev->iuser4 |= 2;
	m_bPlaced = false;
}

// 8-ray 45° circle out of the origin; pin to the nearest wall and lie flat (spawnflag 0x10).
void CTSGroundWeapon::PutOnWall()
{
	TraceResult best;
	best.flFraction = 1.0f;
	Vector bestDir = g_vecZero;
	bool found = false;
	for (int i = 0; i < 8; i++)
	{
		float ang = (360.0f / 8.0f) * i;
		Vector dir(cos(ang * M_PI / 180.0f), sin(ang * M_PI / 180.0f), 0);
		TraceResult tr;
		UTIL_TraceLine(pev->origin, pev->origin + dir * 1024.0f, ignore_monsters, ENT(pev), &tr);
		if (tr.flFraction < 1.0f && tr.flFraction < best.flFraction)
		{
			best = tr;
			bestDir = dir;
			found = true;
		}
	}
	if (found)
	{
		UTIL_SetOrigin(pev, best.vecEndPos + best.vecPlaneNormal * 2.0f);
		pev->angles = UTIL_VecToAngles(bestDir);
		pev->angles.x += 90.0f; // lay flat against the wall
	}
	else
	{
		pev->angles.z = 90.0f; // no wall -> on its side
	}
}

// think (+0.5s): wait for the floor (or the wall), play the drop sound, flatten, settle.
void CTSGroundWeapon::Fall()
{
	pev->nextthink = gpGlobals->time + 0.5f;

	if (pev->spawnflags & SF_GW_ON_WALL)
	{
		if (m_bPlaced)
			return;
		PutOnWall();
	}
	else
	{
		if (!(pev->flags & FL_ONGROUND) && !m_bPlaced)
			return; // keep falling
		pev->angles.x = 0;
		pev->angles.z = 0;
	}

	EMIT_SOUND_DYN(ENT(pev), CHAN_BODY, "items/weapondrop1.wav", VOL_NORM, 0.8f, 0, 100);
	OnGround();
	m_bPlaced = true;
}

void CTSGroundWeapon::OnGround()
{
	pev->solid = SOLID_TRIGGER;
	pev->effects &= ~EF_NODRAW;
	UTIL_SetOrigin(pev, pev->origin);
	UTIL_SetSize(pev, Vector(-4, -4, -4), Vector(4, 4, 4)); // the real grab box
	SetTouch(&CTSGroundWeapon::WaitTouch);
	SetThink(NULL);
}

void CTSGroundWeapon::WaitThink()
{
	pev->nextthink = gpGlobals->time + 1.0f;
}

// USE = pick up (dispatches to the subclass's virtual GetWeapon).
void CTSGroundWeapon::Use(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value)
{
	GetWeapon(pActivator);
}

// touch: only the 0x1d (throwable) type auto-picks on touch; everything else must be USE'd.
void CTSGroundWeapon::WaitTouch(CBaseEntity *pOther)
{
	if (tsweaponid == 0x1d)
		GetWeapon(pOther);
}

// Hand the weapon to the toucher and schedule the respawn (map pickups persist).
void CTSGroundWeapon::GetWeapon(CBaseEntity *pOther)
{
	if (!pOther || !pOther->IsPlayer() || !pOther->IsAlive())
		return;
	if ((pev->effects & EF_NODRAW) || (pOther->pev->effects & EF_NODRAW))
		return;

	if (extraclip < 0)
		extraclip = 0;

	CBasePlayer *pPlayer = (CBasePlayer *)pOther;

	// Buy gate (the original's, gated on CashBased()). CashBased() is hard-OFF in the shipped game so
	// this is DEAD — but present for 1:1 fidelity, and it's what consumes `buyable`/`attackerteam`/
	// `extraclip`. In a round/team mode only `attackerteam` may buy; then the weapon costs cash
	// (base price + per-attachment surcharge + extra clips), deducted from the player's wallet.
	if (CashBased() && buyable)
	{
		CTSGameRules *gr = TSGameRules();
		if (gr && gr->IsTeamMode() && attackerteam != -1 &&
			attackerteam != gr->GetTeamIndex(pPlayer->TeamID()))
		{
			ClientPrint(pPlayer->pev, HUD_PRINTCENTER, "Your team cannot buy this weapon.");
			return;
		}
		int price = 0; // base = get_weapon_info(tsweaponid)->price (the cl_dll buy table mirror)
		if (pev->spawnflags & 1) price += 900;  // silencer
		if (pev->spawnflags & 2) price += 800;  // lasersight
		if (pev->spawnflags & 4) price += 500;  // flashlight
		if (pev->spawnflags & 8) price += 1800; // scope
		price += extraclip * 100;               // extra clips (per-caliber clip cost)
		if (pPlayer->m_iCash < price)
		{
			ClientPrint(pPlayer->pev, HUD_PRINTCENTER, "You don't have enough money for that weapon.");
			return;
		}
		pPlayer->GiveCash(-price);
	}

	// SLICE: faithful is CBasePlayer::PickUpWeapon(tsweaponid, clip, spawnflags, type!=0x1d, …)
	// (not yet reconstructed). We give via the dual-wield arsenal.
	if (!GiveTSWeapon(pPlayer, (tsweapon_t)tsweaponid))
		return; // won't fit -> leave it on the floor
	EMIT_SOUND(ENT(pev), CHAN_ITEM, "items/gunpickup2.wav", VOL_NORM, ATTN_NORM);

	if (!FStringNull(pev->message))
		UTIL_ShowMessage(STRING(pev->message), pPlayer);
	if (pev->target)
		FireTargets(STRING(pev->target), this, this, USE_TOGGLE, 0);

	// respawn schedule: <1 -> 60s, <2 -> 1s, else min(respawntime,180).
	SetTouch(NULL);
	pev->effects |= EF_NODRAW;
	pev->solid = SOLID_NOT;
	float delay = (respawntime < 1) ? 60.0f : (respawntime < 2) ? 1.0f
		: (respawntime < 180 ? (float)respawntime : 180.0f);
	SetThink(&CTSGroundWeapon::Fall);
	pev->nextthink = gpGlobals->time + delay;
	m_bPlaced = false;
}
