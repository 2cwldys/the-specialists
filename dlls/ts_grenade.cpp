/***
*  ts_grenade.cpp — RECONSTRUCTED CM61Grenade (the thrown M61 grenade).
*  Functional/source-faithful goal (see CLAUDE.md). recovered/server/CM61Grenade.c.
*
*  The faithful CM61Grenade is `CM61Grenade : CTSGrenade : CTSPhysicObject` — TS's own
*  bouncing-physics + grenade base classes (not reconstructed) with a custom WaitThink
*  (traces 64u ahead to detonate on a near-impact) and event-driven creation. This is a
*  FUNCTIONAL reconstruction onto the stock SDK `CGrenade` (which already does the
*  arc/bounce/timed-detonate the original's bases provide): a thrown grenade that flies,
*  bounces, and explodes after a fuse with area damage — using the TS w_m61.mdl model.
*  SLICE: no CTSPhysicObject physics, no impact-trace early detonation, no throw event.
***/
#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "monsters.h"
#include "weapons.h"
#include "ts_weapons.h"

class CM61Grenade : public CGrenade
{
public:
	// @ CreateGrenade__11CM61Grenade — toss an M61 from `vecStart` with `vecVelocity`,
	// detonating after `fuse` seconds for `damage`.
	static CM61Grenade *Throw(entvars_t *pevOwner, Vector vecStart, Vector vecVelocity,
		float fuse, float damage);
};

CM61Grenade *CM61Grenade::Throw(entvars_t *pevOwner, Vector vecStart, Vector vecVelocity,
	float fuse, float damage)
{
	CM61Grenade *pGrenade = GetClassPtr((CM61Grenade *)NULL);

	// (Replicates CGrenade::ShootTimed's setup, but with the TS model so we don't
	//  depend on models/grenade.mdl being precached.)
	pGrenade->pev->movetype = MOVETYPE_BOUNCE;
	pGrenade->pev->classname = MAKE_STRING("grenade");
	pGrenade->pev->solid = SOLID_BBOX;
	SET_MODEL(ENT(pGrenade->pev), "models/w_m61.mdl"); // precached via PrecacheAllTSWeapons
	UTIL_SetSize(pGrenade->pev, Vector(0, 0, 0), Vector(0, 0, 0));
	pGrenade->m_fRegisteredSound = false;

	UTIL_SetOrigin(pGrenade->pev, vecStart);
	pGrenade->pev->velocity = vecVelocity;
	pGrenade->pev->angles = UTIL_VecToAngles(vecVelocity);
	pGrenade->pev->owner = ENT(pevOwner);

	pGrenade->SetTouch(&CGrenade::BounceTouch); // bounce off surfaces

	pGrenade->pev->dmgtime = gpGlobals->time + fuse;
	pGrenade->SetThink(&CGrenade::TumbleThink); // tumbles, then detonates at dmgtime
	pGrenade->pev->nextthink = gpGlobals->time + 0.1f;

	pGrenade->pev->sequence = 0;
	pGrenade->pev->framerate = 1.0f;
	pGrenade->pev->gravity = 0.5f;
	pGrenade->pev->friction = 0.8f;
	pGrenade->pev->dmg = damage;

	return pGrenade;
}

// Thrown by CTSGun::PrimaryAttack for the grenade fire-type (declared in ts_weapons.h).
CBaseEntity *TSThrowM61(entvars_t *pevOwner, const Vector &vecStart, const Vector &vecVelocity,
	float fuse, float damage)
{
	return CM61Grenade::Throw(pevOwner, vecStart, vecVelocity, fuse, damage);
}
