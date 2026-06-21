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

//============================================================================
// CTSShrapnel — a bouncing fragment thrown out by an exploding grenade. @0x7bf9c Create /
// @0x7c0a0 Spawn / @0x7c200 BounceTouch / @0x7c678 WaitThink. The faithful base is
// CTSPhysicObject (MOVETYPE_BOUNCE @+0x108=10); FUNCTIONAL reconstruction on the stock SDK.
//============================================================================
#define SHRAPNEL_MODEL "models/shrapnel.mdl" // valve fallback model
#define SHRAPNEL_DMG 14                       // per-fragment hit damage

class CTSShrapnel : public CBaseEntity
{
public:
	void Spawn() override;
	void Precache() override;
	void EXPORT BounceTouch(CBaseEntity *pOther);
	void EXPORT FadeThink();
	int ObjectCaps() override { return CBaseEntity::ObjectCaps() & ~FCAP_ACROSS_TRANSITION; }

	static CTSShrapnel *Create(entvars_t *pevOwner, const Vector &vecOrigin, const Vector &vecVelocity);

	int m_iBounces = 0;
};

LINK_ENTITY_TO_CLASS(ts_shrapnel, CTSShrapnel);

void CTSShrapnel::Precache()
{
	PRECACHE_MODEL(SHRAPNEL_MODEL);
}

// @Spawn__11CTSShrapnel — a small bouncing solid fragment.
void CTSShrapnel::Spawn()
{
	Precache();
	pev->movetype = MOVETYPE_BOUNCE;
	pev->solid = SOLID_BBOX;
	pev->gravity = 1.0f;
	pev->friction = 1.0f;
	SET_MODEL(ENT(pev), SHRAPNEL_MODEL);
	UTIL_SetSize(pev, Vector(0, 0, 0), Vector(0, 0, 0));

	SetTouch(&CTSShrapnel::BounceTouch);
	SetThink(&CTSShrapnel::FadeThink);
	pev->nextthink = gpGlobals->time + 1.5f; // short-lived
}

// @BounceTouch — deal damage to a damageable thing it strikes, else bounce a few times.
void CTSShrapnel::BounceTouch(CBaseEntity *pOther)
{
	if (pOther->pev->takedamage != DAMAGE_NO)
	{
		TraceResult tr = UTIL_GetGlobalTrace();
		ClearMultiDamage();
		entvars_t *pevOwner = pev->owner ? VARS(pev->owner) : pev;
		pOther->TraceAttack(pevOwner, SHRAPNEL_DMG, pev->velocity.Normalize(), &tr,
			DMG_BULLET | DMG_NEVERGIB);
		ApplyMultiDamage(pev, pevOwner);
		SetTouch(nullptr);
		UTIL_Remove(this);
		return;
	}
	// bounce off the world; expire after a couple of bounces
	if (++m_iBounces > 2)
	{
		SetTouch(nullptr);
		UTIL_Remove(this);
	}
}

void CTSShrapnel::FadeThink()
{
	UTIL_Remove(this);
}

// @Create__11CTSShrapnel — spawn one fragment owned by `pevOwner`.
CTSShrapnel *CTSShrapnel::Create(entvars_t *pevOwner, const Vector &vecOrigin, const Vector &vecVelocity)
{
	CTSShrapnel *pShrap = GetClassPtr((CTSShrapnel *)NULL);
	pShrap->pev->classname = MAKE_STRING("ts_shrapnel");
	UTIL_SetOrigin(pShrap->pev, vecOrigin);
	pShrap->Spawn();
	pShrap->pev->velocity = vecVelocity;
	pShrap->pev->owner = ENT(pevOwner);
	return pShrap;
}

// Spawn a burst of shrapnel from a grenade blast at `vecOrigin`.
static void TSSpawnShrapnel(entvars_t *pevOwner, const Vector &vecOrigin, int count)
{
	for (int i = 0; i < count; i++)
	{
		Vector vecDir(RANDOM_FLOAT(-1, 1), RANDOM_FLOAT(-1, 1), RANDOM_FLOAT(0.2f, 1));
		Vector vecVel = vecDir.Normalize() * RANDOM_FLOAT(400, 700);
		CTSShrapnel::Create(pevOwner, vecOrigin + Vector(0, 0, 8), vecVel);
	}
}

class CM61Grenade : public CGrenade
{
public:
	// @ CreateGrenade__11CM61Grenade — toss an M61 from `vecStart` with `vecVelocity`,
	// detonating after `fuse` seconds for `damage`.
	static CM61Grenade *Throw(entvars_t *pevOwner, Vector vecStart, Vector vecVelocity,
		float fuse, float damage);

	void EXPORT M61Tumble();   // tumble + detonate at dmgtime (a CTSShrapnel-spawning detonate)
	void EXPORT M61Detonate(); // shrapnel burst + the area-damage explosion
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
	pGrenade->SetThink(&CM61Grenade::M61Tumble); // tumbles, then detonates (with shrapnel)
	pGrenade->pev->nextthink = gpGlobals->time + 0.1f;

	pGrenade->pev->sequence = 0;
	pGrenade->pev->framerate = 1.0f;
	pGrenade->pev->gravity = 0.5f;
	pGrenade->pev->friction = 0.8f;
	pGrenade->pev->dmg = damage;

	return pGrenade;
}

// @M61Tumble — spin in flight; at the fuse time, detonate (the .so's TumbleThink + dmgtime).
void CM61Grenade::M61Tumble()
{
	pev->nextthink = gpGlobals->time + 0.1f;
	if (pev->dmgtime <= gpGlobals->time)
	{
		M61Detonate();
		return;
	}
	pev->avelocity = Vector(400, 200, 0); // tumble
}

// @M61Detonate — throw a shrapnel burst, then the stock area-damage explosion (+ remove).
void CM61Grenade::M61Detonate()
{
	entvars_t *pevOwner = pev->owner ? VARS(pev->owner) : pev;
	TSSpawnShrapnel(pevOwner, pev->origin, 10);
	Detonate(); // CGrenade::Detonate — RadiusDamage explosion + UTIL_Remove
}

// Thrown by CTSGun::PrimaryAttack for the grenade fire-type (declared in ts_weapons.h).
CBaseEntity *TSThrowM61(entvars_t *pevOwner, const Vector &vecStart, const Vector &vecVelocity,
	float fuse, float damage)
{
	return CM61Grenade::Throw(pevOwner, vecStart, vecVelocity, fuse, damage);
}
