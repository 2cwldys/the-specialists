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
#include "ts_physicobject.h"

//============================================================================
// CTSPhysicObject (@0x7ba54) — slow-motion-aware physics base for CTSGrenade + CTSWorldGun.
// In slow-motion (pev->fuser1 < 1) the velocity/avelocity/gravity are scaled down (originals
// saved); RestorePhysics undoes it. WaitThink toggles each frame. (Original InSlow() is a
// CBaseEntity virtual we don't have; fuser1<1 is the equivalent slow check for a physics object.)
//============================================================================
void CTSPhysicObject::Spawn()
{
	pev->fuser1 = 1.0f;
	m_lastSlow = 1.0f;
	m_savedVelocity = g_vecZero;
	m_savedAVelocity = g_vecZero;
	m_savedGravity = 0;
	m_field80 = 0;
}

void CTSPhysicObject::SavePhysics()
{
	const float slow = pev->fuser1;
	if (m_savedVelocity == g_vecZero)
	{
		// first entry into slow: capture the real physics, then scale down
		m_savedVelocity = pev->velocity;
		m_savedAVelocity = pev->avelocity;
		m_savedGravity = pev->gravity;
	}
	else if (m_lastSlow == slow)
	{
		return; // already scaled for this slow factor
	}
	pev->velocity = m_savedVelocity * slow;
	pev->avelocity = m_savedVelocity * slow; // (orig scales avel from the saved VELOCITY)
	pev->gravity = m_savedGravity * slow * slow;
	m_lastSlow = slow;
}

void CTSPhysicObject::RestorePhysycs()
{
	if (m_savedVelocity == g_vecZero)
		return; // nothing saved
	pev->velocity = m_savedVelocity;
	pev->avelocity = m_savedAVelocity;
	pev->gravity = m_savedGravity;
	m_savedVelocity = g_vecZero;
	m_savedAVelocity = g_vecZero;
	m_savedGravity = 0;
	m_lastSlow = 0;
}

void CTSPhysicObject::WaitThink()
{
	if (pev->fuser1 < 1.0f) // InSlow()
		SavePhysics();
	else
		RestorePhysycs();
}

//============================================================================
// CTSGrenade (@0x7bdd8) — a thrown physics grenade (base CTSPhysicObject): bounce sounds + settle.
//============================================================================
class CTSGrenade : public CTSPhysicObject
{
public:
	void Precache() override;
	void EXPORT BounceTouch(CBaseEntity *pOther);
};

void CTSGrenade::Precache()
{
	PRECACHE_SOUND("weapons/grenbounce1.wav");
	PRECACHE_SOUND("weapons/grenbounce2.wav");
	PRECACHE_SOUND("weapons/grenbounce3.wav");
	PRECACHE_SOUND("weapons/grenbounce4.wav");
}

void CTSGrenade::BounceTouch(CBaseEntity *pOther)
{
	if (!(pev->flags & FL_ONGROUND))
	{
		// airborne bounce (hit a wall) -> a random clatter
		static const char *const snd[4] = {
			"weapons/grenbounce1.wav", "weapons/grenbounce2.wav",
			"weapons/grenbounce3.wav", "weapons/grenbounce4.wav" };
		EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, snd[RANDOM_LONG(0, 3)], 0.25f, 0.8f, 0, 100);
	}
	else
	{
		// on the ground: damp the velocity; once slow enough, come to rest
		pev->velocity = pev->velocity * 0.8f;
		if (pev->velocity.Length() < 32.0f)
		{
			pev->angles.x = 0;
			pev->angles.z = 0;
			pev->velocity = g_vecZero;
		}
	}
}

// Keep CTSGrenade in the binary until the grenade-throw hierarchy is rewired to construct it (the
// original's thrown grenades). CTSPhysicObject is now kept by CTSWorldGun:CTSPhysicObject (ts_worldgun.cpp);
// this anchor remains only for CTSGrenade, which nothing constructs yet. Never called.
CBaseEntity *ts_grenades_keep_physic()
{
	static CTSGrenade g;
	return (CBaseEntity *)&g;
}

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
	void EXPORT WaitThink();
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
	SetThink(&CTSShrapnel::WaitThink);
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

void CTSShrapnel::WaitThink()
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

//============================================================================
// TSExplosion (@0x6d01c) — the grenade detonation logic. It's a FUNCTION, not a spawned entity:
// screen-shake + the TS client explosion visual (gmsgTSCreateDebris material 1) + linear-falloff
// radius damage with a white screen-flash on players caught in the blast. (SLICE: the original also
// drops a brief CTSSlowMotionPoint on a "spectacular" kill via DoSpectacularNess — deferred.)
//============================================================================
extern int gmsgTSCreateDebris; // func_break.cpp (registered "TSCreateDebris")

#define M61_BLAST_RADIUS 400.0f // damage falloff radius (tuned; the original's exact value was obscured by the struct-Vector ABI)
#define M61_SHAKE_MAG    110.0f // screen-shake amplitude + debris size (RE @0x42dc0000 = 110)

void TSExplosion(const Vector &origin, entvars_t *pevInflictor, entvars_t *pevAttacker,
	float magnitude, float baseDamage, float radius)
{
	Vector pos = origin;
	pos.z += 1.0f; // RE: nudge the blast centre 1u off the surface

	UTIL_ScreenShake(pos, magnitude, 80.0f, 1.0f, radius * 2.0f);

	// Explosion visual: a TE_EXPLOSION (dynamic light + fireball sprite + the engine's explode sound).
	// SLICE: the original sends gmsgTSCreateDebris material 1 for its own client fireball-debris effect;
	// that client handler isn't reconstructed yet, so we use the standard TE fireball as a faithful-enough
	// boom (the screen-shake / radius damage / screen-flash below ARE the faithful TSExplosion gameplay).
	MESSAGE_BEGIN(MSG_PAS, SVC_TEMPENTITY, pos);
	WRITE_BYTE(TE_EXPLOSION);
	WRITE_COORD(pos.x);
	WRITE_COORD(pos.y);
	WRITE_COORD(pos.z);
	WRITE_SHORT(g_sModelIndexFireball);
	WRITE_BYTE(25);              // sprite scale (×10)
	WRITE_BYTE(15);              // framerate
	WRITE_BYTE(TE_EXPLFLAG_NONE);
	MESSAGE_END();

	// Radius damage: full baseDamage at the centre, falling off linearly to 0 at `radius`. A wall in the
	// way spares the target; players caught in the blast get a brief white screen-flash (RE UTIL_ScreenFade).
	const float falloff = (radius > 0.0f) ? baseDamage / radius : baseDamage;
	CBaseEntity *pEnt = NULL;
	while ((pEnt = UTIL_FindEntityInSphere(pEnt, pos, radius)) != NULL)
	{
		if (pEnt->pev->takedamage == DAMAGE_NO)
			continue;
		Vector vecTgt = pEnt->BodyTarget(pos);
		float dmg = baseDamage - (vecTgt - pos).Length() * falloff;
		if (dmg <= 0.0f)
			continue;
		TraceResult tr;
		UTIL_TraceLine(pos, vecTgt, dont_ignore_monsters, pevInflictor ? ENT(pevInflictor) : NULL, &tr);
		if (tr.flFraction < 1.0f && tr.pHit != pEnt->edict())
			continue; // a wall is in the way
		if (pEnt->IsPlayer())
			UTIL_ScreenFade(pEnt, Vector(255, 255, 255), 0.1f, 0.05f, 75, 0);
		ClearMultiDamage();
		pEnt->TraceAttack(pevAttacker, dmg, (vecTgt - pos).Normalize(), &tr, DMG_BLAST);
		ApplyMultiDamage(pevInflictor, pevAttacker);
	}
}

//============================================================================
// CM61Grenade (@0x7c974 Spawn / @0x7cacc WaitThink / @0x7ca90 BounceTouch) — the thrown M61. Faithful:
// a MOVETYPE_BOUNCE physics object whose WaitThink (every 0.1s) (1) advances a fuse counter in SLOWED
// time (slow-mo lengthens the fuse), (2) does a 64u swept break ahead of itself — shattering anything
// it can outright kill (health <= 0.1*speed) so it flies THROUGH (e.g. glass), exactly like the thrown
// knife — and (3) detonates via TSExplosion when the fuse runs out. BounceTouch just bounces off
// whatever the sweep didn't break, so break-through and bounce stay mutually exclusive.
//============================================================================
class CM61Grenade : public CGrenade
{
public:
	static CM61Grenade *Throw(entvars_t *pevOwner, Vector vecStart, Vector vecVelocity,
		float fuse, float damage);
	void EXPORT M61WaitThink(); // fuse + swept break-through + detonate (@0x7cacc)

	float m_flFuse = 0.0f;    // fuse counter, advanced in slowed time (orig CM61Grenade fuse field)
	float m_flFuseEnd = 4.0f; // detonate once m_flFuse reaches this
};

CM61Grenade *CM61Grenade::Throw(entvars_t *pevOwner, Vector vecStart, Vector vecVelocity,
	float fuse, float damage)
{
	CM61Grenade *pGrenade = GetClassPtr((CM61Grenade *)NULL);
	pGrenade->pev->movetype = MOVETYPE_BOUNCE;
	pGrenade->pev->classname = MAKE_STRING("grenade");
	pGrenade->pev->solid = SOLID_BBOX;
	SET_MODEL(ENT(pGrenade->pev), "models/w_m61.mdl"); // precached via PrecacheAllTSWeapons
	UTIL_SetSize(pGrenade->pev, Vector(0, 0, 0), Vector(0, 0, 0));

	UTIL_SetOrigin(pGrenade->pev, vecStart);
	pGrenade->pev->velocity = vecVelocity;
	pGrenade->pev->angles = UTIL_VecToAngles(vecVelocity);
	pGrenade->pev->owner = ENT(pevOwner);
	pGrenade->pev->fuser1 = 1.0f; // slow factor (1 = normal); slow-mo lowers it so the fuse counts slower

	pGrenade->SetTouch(&CGrenade::BounceTouch);     // bounce off whatever the sweep didn't break
	pGrenade->SetThink(&CM61Grenade::M61WaitThink);
	pGrenade->pev->nextthink = gpGlobals->time;     // first sweep THIS frame (RE Spawn: nextthink = time)
	pGrenade->m_flFuse = 0.0f;
	pGrenade->m_flFuseEnd = fuse;

	pGrenade->pev->sequence = 0;
	pGrenade->pev->framerate = 1.0f;
	pGrenade->pev->gravity = 0.75f; // RE Spawn: gravity 0.75
	pGrenade->pev->friction = 0.8f;
	pGrenade->pev->dmg = damage;
	return pGrenade;
}

void CM61Grenade::M61WaitThink()
{
	pev->nextthink = gpGlobals->time + 0.1f; // RE: 0.1s think interval

	// Fuse advances in SLOWED time (RE WaitThink: += GiveSlowMul()*0.1) — slow-motion lengthens it.
	const float slow = (pev->fuser1 < 0.01f) ? 0.01f : pev->fuser1;
	m_flFuse += slow * 0.1f;
	if (m_flFuse >= m_flFuseEnd)
	{
		if (IsInWorld())
			TSExplosion(pev->origin, pev, pev->owner ? VARS(pev->owner) : pev,
				M61_SHAKE_MAG, pev->dmg, M61_BLAST_RADIUS);
		UTIL_Remove(this);
		return;
	}

	// Swept break (RE WaitThink): while moving, trace 64u ahead and shatter anything we can outright
	// kill (health <= 0.1*speed) — the pane breaks BEFORE the physics move reaches it, so the grenade
	// flies cleanly THROUGH instead of bouncing off (the same model as the thrown knife).
	const float speed = pev->velocity.Length();
	if (speed > 64.0f)
	{
		Vector dir = pev->velocity * (1.0f / speed);
		Vector end = pev->origin + dir * 64.0f;
		TraceResult tr;
		UTIL_TraceLine(pev->origin, end, dont_ignore_monsters, ENT(pev), &tr);
		if (tr.flFraction < 1.0f)
		{
			CBaseEntity *pHit = tr.pHit ? CBaseEntity::Instance(tr.pHit) : NULL;
			const float dmg = 0.1f * speed; // RE: 0.1 * speed
			if (pHit && !pHit->IsPlayer() && pHit->pev->health <= dmg)
			{
				ClearMultiDamage();
				pHit->TraceAttack(pev, dmg, dir, &tr, DMG_CLUB | DMG_NEVERGIB);
				ApplyMultiDamage(pev, pev);
			}
		}
	}
	pev->avelocity = Vector(400, 200, 0); // tumble in flight
}

// Thrown by CTSGun::PrimaryAttack for the grenade fire-type (declared in ts_weapons.h).
CBaseEntity *TSThrowM61(entvars_t *pevOwner, const Vector &vecStart, const Vector &vecVelocity,
	float fuse, float damage)
{
	return CM61Grenade::Throw(pevOwner, vecStart, vecVelocity, fuse, damage);
}
