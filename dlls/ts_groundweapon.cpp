/***
*  ts_groundweapon.cpp — RECONSTRUCTED CTSGroundWeapon from ts_i386.so + FGD.
*  Functional/source-faithful goal (see CLAUDE.md). recovered/server/CTSGroundWeapon.c.
*
*  @PointClass base(Target, tsweaponlist, tsobjective) = ts_groundweapon
*    tsweaponid (FGD "Weapon", == tsweapon_t index) -> m_weaponType (this+0xa4)
*    wduration "Respawn time" -> m_respawnTime (this+0xa0)
*    wextraclip "Extra clips" -> m_extraClip   (this+0xa8)
*    buyable    -> m_buyable                    (this+0xb0)
*    spawnflags: 1 Silencer, 2 LaserSight, 4 Flashlight, 8 Scope, 16 Lie-on-wall
*
*  A map-placed weapon pickup (65 placements across the stock maps). The faithful
*  GetWeapon hands the weapon to the toucher via CBasePlayer::PickUpWeapon (the
*  dual-wield armament engine, not yet reconstructed). THIS SLICE gives the stock
*  CTSGun (weapon_tsgun) instead — enough to make weapons reachable in play, clear
*  the engine's "Can't init ts_groundweapon", and (crucially) precache the weapon
*  at MAP LOAD so a runtime give doesn't hit the precache-only-in-spawn error.
***/
#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "player.h"
#include "ts_weapons.h"

class CTSGroundWeapon : public CBaseEntity
{
public:
	void Spawn() override;
	void Precache() override;
	bool KeyValue(KeyValueData *pkvd) override;
	void EXPORT GetWeapon(CBaseEntity *pOther); // touch
	void EXPORT MaterializeThink();
	int ObjectCaps() override { return CBaseEntity::ObjectCaps() & ~FCAP_ACROSS_TRANSITION; }

	int m_weaponType = TSW_GLOCK18; // tsweaponid default 1 (FGD)
	int m_respawnTime = 60;
	int m_extraClip = 0;
	int m_buyable = 0;
};

LINK_ENTITY_TO_CLASS(ts_groundweapon, CTSGroundWeapon);

bool CTSGroundWeapon::KeyValue(KeyValueData *pkvd)
{
	if (FStrEq(pkvd->szKeyName, "tsweaponid"))
	{
		m_weaponType = atoi(pkvd->szValue);
		return true;
	}
	if (FStrEq(pkvd->szKeyName, "wduration"))
	{
		m_respawnTime = atoi(pkvd->szValue);
		return true;
	}
	if (FStrEq(pkvd->szKeyName, "wextraclip"))
	{
		m_extraClip = atoi(pkvd->szValue);
		return true;
	}
	if (FStrEq(pkvd->szKeyName, "buyable"))
	{
		m_buyable = atoi(pkvd->szValue);
		return true;
	}
	return CBaseEntity::KeyValue(pkvd);
}

void CTSGroundWeapon::Precache()
{
	// Precache the given weapon at map load so the runtime hand-out below is legal
	// (PRECACHE_MODEL is only allowed during spawn). SLICE: we hand out the stock
	// CTSGun (beretta); the faithful path precaches the whole roster.
	UTIL_PrecacheOther("weapon_tsgun");
}

void CTSGroundWeapon::Spawn()
{
	Precache();
	pev->classname = MAKE_STRING("ts_groundweapon");

	const s_weapon_info *wi = get_weapon_info((tsweapon_t)m_weaponType);
	// Most weapon types have no stats filled yet -> fall back to a stock model.
	const char *world = (wi && wi->pszWorldModel) ? wi->pszWorldModel : "models/w_weaponbox.mdl";
	PRECACHE_MODEL((char *)world);
	SET_MODEL(ENT(pev), world);

	pev->movetype = MOVETYPE_TOSS;
	pev->solid = SOLID_TRIGGER;
	UTIL_SetSize(pev, Vector(-8, -8, -8), Vector(8, 8, 8));
	UTIL_SetOrigin(pev, pev->origin);

	SetTouch(&CTSGroundWeapon::GetWeapon);

	// settle onto the floor
	if (!(pev->spawnflags & 16)) // 16 = "Lie on wall" -> keep placed position
		pev->movetype = MOVETYPE_TOSS;
}

// @ GetWeapon — touch handler. Faithful: CBasePlayer::PickUpWeapon(type,...).
// SLICE: hand the toucher a CTSGun and respawn after wduration.
void CTSGroundWeapon::GetWeapon(CBaseEntity *pOther)
{
	if (!pOther || !pOther->IsPlayer() || !pOther->IsAlive())
		return;

	CBasePlayer *pPlayer = (CBasePlayer *)pOther;
	if (pPlayer->HasNamedPlayerItem("weapon_tsgun"))
		return;

	pPlayer->GiveNamedItem("weapon_tsgun");
	EMIT_SOUND(ENT(pev), CHAN_ITEM, "items/gunpickup2.wav", VOL_NORM, ATTN_NORM);

	if (pev->target)
		FireTargets(STRING(pev->target), this, this, USE_TOGGLE, 0);

	// hide + respawn after wduration (0 or teamplay-round handling deferred)
	SetTouch(NULL);
	pev->effects |= EF_NODRAW;
	pev->solid = SOLID_NOT;
	if (m_respawnTime > 0)
	{
		SetThink(&CTSGroundWeapon::MaterializeThink);
		pev->nextthink = gpGlobals->time + m_respawnTime;
	}
	else
	{
		UTIL_Remove(this);
	}
}

void CTSGroundWeapon::MaterializeThink()
{
	pev->effects &= ~EF_NODRAW;
	pev->solid = SOLID_TRIGGER;
	UTIL_SetOrigin(pev, pev->origin);
	SetTouch(&CTSGroundWeapon::GetWeapon);
	SetThink(NULL);
}
