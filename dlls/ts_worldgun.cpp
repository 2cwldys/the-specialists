/***
*  ts_worldgun.cpp — RECONSTRUCTED CTSWorldGun (player-dropped weapon in the world).
*  Functional/source-faithful (see CLAUDE.md). Recovered from ts_i386.so:
*  CTSWorldGun (@0xa07ec CreateWeapon / @0xa0954 Spawn / @0xa1198 GetWeapon /
*  @0xa1250 WaitTouch).
*
*  When a player drops/throws a weapon (CBasePlayer::DropWeapon -> CTSGun::DropWeapon),
*  the weapon's state becomes a CTSWorldGun: a physics object that falls, settles, and is
*  picked up by another player pressing USE on it (GetWeapon -> PickUpWeapon). The FAITHFUL
*  base is `CTSWorldGun : CTSPhysicObject` (TS bounce physics + the carried s_weapon_status)
*  — like CM61Grenade, this is a FUNCTIONAL reconstruction on the stock SDK (MOVETYPE_TOSS)
*  carrying just the weapon TYPE + loaded CLIP. // SLICE: no CTSPhysicObject bounce, no
*  full s_weapon_status (caliber reserves stay with the dropper), flag(0x1d) auto-pickup.
***/
#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "player.h"
#include "weapons.h"
#include "ts_weapons.h" // get_weapon_info, TSPickUpWeapon

class CTSWorldGun : public CBaseEntity
{
public:
	void Spawn() override;
	void Precache() override;
	void EXPORT FallThink();          // settle on the ground, then become a trigger
	void EXPORT WaitTouch(CBaseEntity *pOther);
	int ObjectCaps() override { return CBaseEntity::ObjectCaps() & ~FCAP_ACROSS_TRANSITION; }

	int  m_weaponType = TSW_BERETTA; // +0x88 — the dropped weapon type
	int  m_clip = 0;                 // carried loaded rounds
	bool m_bTaken = false;           // +0x35 — already picked up
};

LINK_ENTITY_TO_CLASS(ts_worldgun, CTSWorldGun);

void CTSWorldGun::Precache()
{
	const s_weapon_info *wi = get_weapon_info((tsweapon_t)m_weaponType);
	if (wi->pszWorldModel)
		PRECACHE_MODEL((char *)wi->pszWorldModel);
}

// @Spawn — show the weapon's world model, toss it, fall toward the ground.
void CTSWorldGun::Spawn()
{
	Precache();
	const s_weapon_info *wi = get_weapon_info((tsweapon_t)m_weaponType);
	SET_MODEL(ENT(pev), wi->pszWorldModel ? wi->pszWorldModel : "models/w_weaponbox.mdl");

	pev->movetype = MOVETYPE_TOSS;
	pev->solid = SOLID_BBOX;
	// While falling the world gun must NOT block the player. The original (CTSWorldGun::Spawn
	// @0xa0954) gives the falling gun a ZERO-size bbox (UTIL_SetSize with DAT_00106a08 = 0.0),
	// so the SOLID_BBOX point still drops + lands (FL_ONGROUND) yet the player walks through it
	// (the thrown gun damages via the Fall trace, not collision). It only takes a real (trigger)
	// size on landing (OnGround). reference/WEAPON_SELECT.md / @0xa0954.
	UTIL_SetSize(pev, g_vecZero, g_vecZero);
	UTIL_SetOrigin(pev, pev->origin);

	SetThink(&CTSWorldGun::FallThink);
	pev->nextthink = gpGlobals->time + 0.1f;
}

// @Fall/OnGround — once resting on the ground, become a USE trigger.
void CTSWorldGun::FallThink()
{
	pev->nextthink = gpGlobals->time + 0.1f;
	if (!(pev->flags & FL_ONGROUND))
		return;

	pev->movetype = MOVETYPE_FLY; // stop sliding
	pev->velocity = g_vecZero;
	pev->solid = SOLID_TRIGGER;
	UTIL_SetSize(pev, Vector(-16, -16, -4), Vector(16, 16, 4));
	SetThink(nullptr);
	SetTouch(&CTSWorldGun::WaitTouch);
}

// @WaitTouch — a living player pressing USE over the gun picks it up. @GetWeapon does the
// hand-off + removes the world gun.
void CTSWorldGun::WaitTouch(CBaseEntity *pOther)
{
	if (m_bTaken || !pOther->IsPlayer() || !pOther->IsAlive())
		return;
	if (!(pOther->pev->button & IN_USE)) // press USE to pick up (matches the .so)
		return;

	CBasePlayer *pPlayer = (CBasePlayer *)pOther;
	TSPickUpWeapon(pPlayer, m_weaponType, m_clip);
	EMIT_SOUND(ENT(pPlayer->pev), CHAN_ITEM, "items/gunpickup2.wav", VOL_NORM, ATTN_NORM);
	m_bTaken = true;
	SetTouch(nullptr);
	UTIL_Remove(this);
}

// Spawn a dropped weapon in the world (the drop side of CTSWorldGun::CreateWeapon): create
// the entity carrying `type`/`clip`, place it at `origin`, and toss it along `velocity`.
CBaseEntity *TSCreateWorldGun(int type, int clip, const Vector &origin, const Vector &velocity)
{
	CTSWorldGun *pGun = GetClassPtr((CTSWorldGun *)NULL);
	if (!pGun)
		return NULL;
	pGun->pev->classname = MAKE_STRING("ts_worldgun");
	pGun->m_weaponType = type;
	pGun->m_clip = clip;
	UTIL_SetOrigin(pGun->pev, origin);
	pGun->Spawn();
	pGun->pev->velocity = velocity;
	return pGun;
}
