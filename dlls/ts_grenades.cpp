/***
*  ts_grenade.cpp — RECONSTRUCTED CM61Grenade (the thrown M61 grenade).
*  Functional/source-faithful goal (see CLAUDE.md). recovered/server/CM61Grenade.c.
*
*  CM61Grenade derives CTSGrenade, which derives CTSPhysicObject (the slow-motion physics base, at
*  the top of this file), as in the original. Its WaitThink runs the fuse in slowed time and traces
*  64 units ahead to go off on a near impact; the throw's presentation is the grenade event
*  (events/grenade.sc, CTSGun::GrenadeFire).
***/
#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "monsters.h"
#include "weapons.h"
#include "player.h"
#include "ts_weapons.h"
#include "ts_utils.h"   // RemoveEntityFromSlowedArray (the pre-remove scrub)
#include "ts_player.h" // KungFu() — the thrown-katana pickup gate (CTSKnife::Use)
#include "ts_physicobject.h"
#include "ts_slowmotionpoint.h" // the spectacular-explosion slow bubble
#include "ts_gamerules.h"       // DoSpectacularNess
#include "game.h"               // g_flWeaponCheat (sv_cheats as the map started)
#include "ts_worldgun.h"         // CTSWorldGun (a blast throws dropped guns)

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

LINK_ENTITY_TO_CLASS(shrapnel, CTSShrapnel); // orig classname "shrapnel"

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
	pev->fuser1 = 1.0f; // TS timescale init (RE @0x6c186) — feeds the EMIT_SOUND_DYN slow-pitch
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
	RemoveEntityFromSlowedArray(this); // WaitThink__11CTSShrapnel @0x6c81e — scrub the packed slowed array before removal
	UTIL_Remove(this);
}

// @Create__11CTSShrapnel — spawn one fragment owned by `pevOwner`.
CTSShrapnel *CTSShrapnel::Create(entvars_t *pevOwner, const Vector &vecOrigin, const Vector &vecVelocity)
{
	CTSShrapnel *pShrap = GetClassPtr((CTSShrapnel *)NULL);
	pShrap->pev->classname = MAKE_STRING("shrapnel");
	UTIL_SetOrigin(pShrap->pev, vecOrigin);
	pShrap->Spawn();
	pShrap->pev->velocity = vecVelocity;
	pShrap->pev->owner = ENT(pevOwner);
	return pShrap;
}

//============================================================================
// @0x6d01c TSExplosion(origin, inflictor, attacker, damage, bits, inner, outer) — an explosion. It's a
// FUNCTION, not a spawned entity. The caller's origin is raised 1 unit. A screen shake (amplitude
// `damage`, radius 2 x outer) and the TS client explosion (TSCreateDebris material 1, its size the
// damage truncated). Everything in the outer sphere but the inflictor that takes damage and is not
// dead: a living player gets a pale flash; full damage out to `inner`, then down to 0 at `outer`; a
// clear trace from the blast to its centre deals it; a blocked one deals what is left after the
// distance from the blocker to where a trace back from the target leaves the wall, so a wall's
// thickness absorbs damage. A dropped gun ("WorldGun") in the sphere is thrown away from a point 50
// units above the blast, harder the closer and the lighter it is. Last the spectacular roll's
// one-second slow-motion bubble.
//============================================================================
extern int gmsgTSCreateDebris; // func_break.cpp (registered "TSCreateDebris")

void TSExplosion(Vector &origin, entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage,
	int bitsDamageType, float flInner, float flOuter)
{
	origin.z += 1.0f; // @0x6d037: the caller's vector
	Vector high = origin;
	high.z += 50.0f;

	UTIL_ScreenShake(origin, flDamage, 80.0f, 1.0f, flOuter * 2.0f);

	MESSAGE_BEGIN(MSG_ALL, gmsgTSCreateDebris);
	WRITE_COORD(origin.x);
	WRITE_COORD(origin.y);
	WRITE_COORD(origin.z);
	WRITE_COORD((float)(int)flDamage); // @0x6d11d: truncated
	WRITE_COORD(0);
	WRITE_COORD(0);
	WRITE_BYTE(1);
	MESSAGE_END();

	const float falloff = flDamage / (flOuter - flInner);
	if (!pevAttacker)
		pevAttacker = pevInflictor;

	CBaseEntity *pEntity = NULL;
	while ((pEntity = UTIL_FindEntityInSphere(pEntity, origin, flOuter)) != NULL)
	{
		if (pEntity->pev == pevInflictor)
			continue;

		if (pEntity->pev->takedamage != DAMAGE_NO && pEntity->pev->deadflag != DEAD_DEAD)
		{
			if (pEntity->IsAlive() && pEntity->IsPlayer())
				UTIL_ScreenFade(pEntity, Vector(222, 227, 189), 0.1f, 0.05f, 75, 0); // @0x6d25c

			Vector target = pEntity->Center();
			target.z += 1.0f;
			TraceResult tr;
			UTIL_TraceLine(origin, target, dont_ignore_monsters, ENT(pevInflictor), &tr);

			const float dist = (origin - target).Length();
			float dmg = flDamage;
			if (dist > flInner)
			{
				dmg = flDamage - (dist - flInner) * falloff;
				if (dmg < 0.0f)
					dmg = 0.0f;
			}

			if (tr.flFraction == 1.0f || tr.pHit == pEntity->edict())
			{
				ClearMultiDamage();
				Vector dir = target - origin;
				const float len = dir.Length();
				dir = (len == 0.0f) ? Vector(0, 0, 1) : dir * (1.0f / len);
				pEntity->TraceAttack(pevAttacker, dmg, dir, &tr, bitsDamageType);
				ApplyMultiDamage(pevInflictor, pevAttacker);
			}
			else
			{
				// @0x6d47c: the wall's thickness, from the trace back out of the target
				const Vector wallIn = tr.fStartSolid ? origin : tr.vecEndPos;
				UTIL_TraceLine(target, origin, dont_ignore_monsters, pEntity->edict(), &tr);
				if (tr.flFraction != 1.0f)
				{
					const float thickness = (tr.vecEndPos - wallIn).Length();
					if (thickness < dmg)
					{
						ClearMultiDamage();
						Vector dir = target - origin;
						const float len = dir.Length();
						dir = (len == 0.0f) ? Vector(0, 0, 1) : dir * (1.0f / len);
						pEntity->TraceAttack(pevAttacker, dmg - thickness, dir, &tr, bitsDamageType);
						ApplyMultiDamage(pevInflictor, pevAttacker);
					}
				}
			}
		}

		// @0x6d651: a dropped gun flies
		if (!strcmp(STRING(pEntity->pev->classname), "WorldGun"))
		{
			CTSWorldGun *pGun = (CTSWorldGun *)pEntity;
			UTIL_SetOrigin(pGun->pev, pGun->pev->origin);
			UTIL_SetSize(pGun->pev, g_vecZero, g_vecZero);
			pGun->SetTouch(&CTSWorldGun::BounceTouch);
			pGun->SetThink(&CTSWorldGun::Fall);
			pGun->pev->nextthink = gpGlobals->time + 0.1f;
			pGun->pev->movetype = MOVETYPE_BOUNCE;
			pGun->pev->solid = SOLID_BBOX;
			pGun->pev->gravity = 0.7f;
			pGun->pev->friction = 0.8f;
			Vector away = pGun->pev->origin - high;
			const float d = away.Length();
			const Vector dir = (d == 0.0f) ? Vector(0, 0, 1) : away * (1.0f / d);
			const float cost = (float)(unsigned char)get_weapon_info((tsweapon_t)pGun->m_weaponType)->slotCost;
			pGun->pev->velocity = (dir * (flOuter - d)) / cost * 10.0f;
			if (pGun->pev->flags & FL_ONGROUND)
			{
				pGun->pev->flags &= ~FL_ONGROUND;
				pGun->pev->velocity.z = fabs(pGun->pev->velocity.z) + 50.0f;
			}
			pGun->pev->avelocity.y = RANDOM_LONG(-300, 300);
			pGun->pev->avelocity.z = RANDOM_LONG(-300, 300);
		}
	}

	// @0x6d964: the "spectacular" roll — DoSpectacularNess (RANDOM_LONG(0,100) <= round(`spectacularness`
	// cvar), default 1 -> ~2%) drops a one-second slow-motion bubble (scale 0.25) at the blast.
	if (DoSpectacularNess())
		CTSSlowMotionPoint::CreateSlowMotionPoint(origin, 0.25f, 1.0f, 0.0f, false);
}

//============================================================================
// CM61Grenade (@0x7c874 CreateGrenade / @0x7c974 Spawn / @0x7cacc WaitThink / @0x7ca90 BounceTouch /
// @0x7ce64 Use / @0x7effc ObjectCaps) — the thrown M61. Base chain CM61Grenade : CTSGrenade :
// CTSPhysicObject. A MOVETYPE_BOUNCE physics object whose WaitThink (every 0.1s) (1) advances a fuse
// counter in SLOWED time (slow-mo lengthens the fuse), (2) does a 64u swept break ahead of itself —
// shattering anything it can outright kill (health <= 0.1*speed) so it flies THROUGH (e.g. glass),
// exactly like the thrown knife — and (3) detonates via TSExplosion when the fuse runs out. BounceTouch
// bounces off whatever the sweep didn't break (skipping the owner). Use re-throws it (+USE).
//============================================================================
class CM61Grenade : public CTSGrenade
{
public:
	void Spawn() override;                          // @0x7c974
	void EXPORT WaitThink();                         // @0x7cacc — fuse + swept break-through + detonate
	void EXPORT BounceTouch(CBaseEntity *pOther);    // @0x7ca90 — owner-skip wrapper -> CTSGrenade::BounceTouch
	void Use(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value) override; // @0x7ce64
	int  ObjectCaps() override { return FCAP_IMPULSE_USE; } // @0x7effc -> 0x10 (so +USE fires Use)

	// @0x7c874 CreateGrenade — static factory: spawn an M61 from `pevOwner` at `origin` along `velocity`
	// with a `fuse`-second timer; carries the thrower's slow-motion factor onto the grenade.
	static CM61Grenade *CreateGrenade(entvars_t *pevOwner, float fuse, const Vector &origin,
		const Vector &velocity);

	float m_flFuseEnd = 4.0f; // +0x88 — fuse LIMIT (detonate once m_flFuse reaches it)
	float m_flFuse = 0.0f;    // +0x8c — fuse ELAPSED counter, advanced in slowed time
	// the throwing player's edict is the original's member @0x90; we model it as pev->owner.
};

// @0x7c874 CreateGrenade — build + place + launch (the original sets the fuse + owner, calls Spawn,
// then origin/velocity/owner + the slow-motion propagation).
// @0x6c850 — the original EXPORTS an m61_grenade entity factory (engine dlsym + the
// TSPrecache PrecacheOther). CreateGrenade's runtime classname stays "grenade" (V6-gated).
LINK_ENTITY_TO_CLASS(m61_grenade, CM61Grenade);

CM61Grenade *CM61Grenade::CreateGrenade(entvars_t *pevOwner, float fuse, const Vector &origin,
	const Vector &velocity)
{
	CM61Grenade *pGrenade = GetClassPtr((CM61Grenade *)NULL);
	if (!pGrenade)
		return NULL;
	pGrenade->pev->classname = MAKE_STRING("grenade");
	pGrenade->m_flFuseEnd = fuse;       // member 0x88 set before Spawn (per @0x7c874)
	pGrenade->Spawn();
	UTIL_SetOrigin(pGrenade->pev, origin);
	pGrenade->pev->velocity = velocity;
	pGrenade->pev->owner = ENT(pevOwner);

	// @0x6c91b: the grenade takes the thrower's slow-motion rate (TSThrowSlow) through GoSlow, which
	// stores it in fuser1.
	CBaseEntity *pOwner = CBaseEntity::Instance(pevOwner);
	if (pOwner && pOwner->IsPlayer())
		pGrenade->GoSlow(TSThrowSlow((CBasePlayer *)pOwner), 0);
	return pGrenade;
}

// @0x7c974 Spawn — become the bouncing M61 physics object and start the fuse think THIS frame.
void CM61Grenade::Spawn()
{
	CTSPhysicObject::Spawn();   // slow-motion physics base init
	CTSGrenade::Precache();     // grenbounce{1..4}.wav
	SET_MODEL(ENT(pev), "models/w_m61.mdl"); // precached via PrecacheAllTSWeapons
	pev->movetype = MOVETYPE_BOUNCE;
	pev->solid = SOLID_BBOX;
	pev->gravity = 0.75f;
	pev->friction = 0.8f;
	UTIL_SetSize(pev, g_vecZero, g_vecZero); // point bbox

	SetThink(&CM61Grenade::WaitThink);
	SetTouch(&CM61Grenade::BounceTouch);
	pev->nextthink = gpGlobals->time;        // first fuse/sweep THIS frame
	m_flFuse = 0.0f;
	pev->fuser1 = 1.0f;                       // slow factor (1 = normal)
	pev->iuser4 = 0;
	pev->iuser4 |= 2;                         // the grenade/usable flag Use checks
}

// @0x7cacc WaitThink — advance the fuse (in slowed time), sweep-break ahead, detonate on expiry.
void CM61Grenade::WaitThink()
{
	pev->nextthink = gpGlobals->time + 0.1f; // RE: 0.1s think interval

	if (m_flFuse < m_flFuseEnd)
	{
		// Fuse advances in SLOWED time: += GiveSlowMul()*0.1 = max(fuser1,0.01)*0.1.
		const float slow = (pev->fuser1 < 0.01f) ? 0.01f : pev->fuser1;
		m_flFuse += slow * 0.1f;

		// Swept break: trace 64u ahead and shatter anything we can outright kill (health <= 0.1*speed),
		// so the grenade flies THROUGH (e.g. glass) — the same model as the thrown knife. (RE has only
		// the health gate, no !IsPlayer() guard.)
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
				if (pHit && pHit->pev->health <= dmg)
				{
					ClearMultiDamage();
					pHit->TraceAttack(pev, dmg, dir, &tr, DMG_CLUB | DMG_NEVERGIB);
					ApplyMultiDamage(pev, pev);
				}
			}
		}
		CTSPhysicObject::WaitThink(); // slow-motion physics Save/Restore upkeep
	}
	else
	{
		if (IsInWorld())
		{
			// @0x6cb5d: a copy of the origin; 110 damage (and shake), DMG_BLAST | DMG_NEVERGIB, full
			// out to 300 units, nothing past 1024
			Vector vecOrigin = pev->origin;
			TSExplosion(vecOrigin, pev, pev->owner ? VARS(pev->owner) : NULL, 110.0f, DMG_BLAST | DMG_NEVERGIB,
				300.0f, 1024.0f);
		}
		RemoveEntityFromSlowedArray(this); // WaitThink__11CM61Grenade @0x6cb9f — scrub the packed slowed array before removal
		UTIL_Remove(this);
	}
}

// @0x7ca90 BounceTouch — bounce off everything EXCEPT the owner (delegates to CTSGrenade::BounceTouch).
void CM61Grenade::BounceTouch(CBaseEntity *pOther)
{
	if (pOther && pOther->edict() == pev->owner)
		return; // don't bounce/clatter against the thrower
	CTSGrenade::BounceTouch(pOther);
}

// @0x7ce64 Use — a player +USE re-throws the grenade forward+up at 625 u/s (gated on the iuser4 flag).
void CM61Grenade::Use(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value)
{
	if (!(pev->iuser4 & 2))
		return;
	if (pCaller && pCaller->IsPlayer() && pCaller->IsAlive())
	{
		UTIL_MakeVectors(pCaller->pev->angles); // RE reads the caller's angles (entvars+0x78)
		Vector dir = (gpGlobals->v_forward + Vector(0, 0, 0.75f)).Normalize();
		pev->velocity = pCaller->pev->velocity + dir * 625.0f;
	}
}

// Thrown by CTSGun::PrimaryAttack for the grenade fire-type (declared in ts_weapons.h).
CBaseEntity *TSThrowM61(entvars_t *pevOwner, const Vector &vecStart, const Vector &vecVelocity,
	float fuse)
{
	return CM61Grenade::CreateGrenade(pevOwner, fuse, vecStart, vecVelocity);
}

/***
*  ts_knife.cpp — CTSKnife, the thrown knife/seal-knife/katana projectile (right-click throw).
*
*  ORIGINAL SOURCE: the CTSKnife class lives in the original's **TS_grenades.cpp** (grouped with all
*  the throwables — CM61Grenade/CTSShrapnel/CTSGrenade/CTSPhysicObject); the original's TS_knife.cpp
*  holds only Write_knife (the s_weapon_info table entry). Original method set (keep these names so the
*  name-matched coverage diff stays accurate — see scripts/coverage/{filemap,reconstruction}.py):
*    Spawn, WaitThink, BounceTouch, WaitTouch, Use, ShouldCollide, ObjectCaps, GetDescription, CreateKnife.
*
*  RE: CTSGun::SecondaryAttack (@0x817a8) routes a knife (fire-type 3) to KnifeFire (@0x74044), which
*  spawns the projectile (CreateKnife @0x6d9e4) at speed 1900 along v_forward. It is MOVETYPE_BOUNCE
*  (Spawn @0x6db54). Breaking is done ONLY by the swept WaitThink (@0x6ea30); BounceTouch (@0x6decc)
*  stabs players / sticks in worldspawn / bounces off everything else — never breaks. So break (pass
*  through) and bounce are mutually exclusive. CTSKnife is a CTSGrenade (a CTSPhysicObject): the engine
*  bounces it (MOVETYPE_BOUNCE) and WaitThink ends in the slow-motion physics upkeep, as the original's.
***/
#include "ts_gun.h"

class CTSKnife : public CTSGrenade // the original's base (hierarchy.tsv)
{
public:
	void Spawn() override;
	void EXPORT BounceTouch(CBaseEntity *pOther); // @CTSKnife::BounceTouch @0x6decc — contact handler
	void EXPORT WaitTouch(CBaseEntity *pOther);   // @CTSKnife::WaitTouch @0x6dce8 — post-settle ground damping
	void EXPORT WaitThink();                     // @CTSKnife::WaitThink @0x6ea30 — in-flight swept break + expire
	void Use(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value) override; // @0x6dda4 — +use pickup
	char *GetDescription();                       // @0x6db04 — the thrown-knife name by type
	int  ObjectCaps() override { return FCAP_CONTINUOUS_USE; } // @0x6f04c -> 0x10
	virtual bool ShouldCollide(CBaseEntity *pOther) { return true; } // @0x6ed54 -> 1 (TS virtual)
	// @0x6d9e4 CreateKnife(CBasePlayer*, Vector pos, Vector vel, int knifeType) — knifeType is the
	// s_weapon_info+0x20 code (knife 6 / sealknife 7 / katana 8; stored at knife+0xa0 in the original).
	// NO damage parameter — the stab damage lives inside the class (StabDamage: BounceTouch @0x6dfcd
	// gives code 8 150, code 6 100, anything else 50).
	static CTSKnife *CreateKnife(CBasePlayer *pOwner, const Vector &vecStart, const Vector &vecVel, int knifeType);

	float StabDamage() const // RE BounceTouch: per-type player-stab damage (katana 150 / knife 100 / seal 50)
	{
		return (m_knifeType == TSW_KATANA) ? 150.0f : (m_knifeType == TSW_SEALKNIFE) ? 50.0f : 100.0f;
	}
	float Gravity() const // RE Spawn: pev->gravity by knife type
	{
		return (m_knifeType == TSW_KATANA) ? 0.85f : (m_knifeType == TSW_SEALKNIFE) ? 0.3f : 0.75f;
	}

	int m_knifeType = TSW_KNIFE; // the TSW_* identity (the original stores the raw 6/7/8 code at +0xa0)
	float m_flExpire = 0;        // orig +0x88 — auto-remove time (Spawn +30s; settle tail +10s; Use gate)
	bool m_bInFlight = false;    // orig +0x90 — set in Spawn; cleared on a sub-700 u/s touch + in the settle
	                             // tail; Use refuses pickup while set
	int m_iKatanaFlag = 0;       // orig +0x91 — re-derived EVERY touch: 1 iff a katana touched a non-player;
	                             // Use gates a flagged blade on KungFu(player)
	edict_t *m_pThrower = nullptr; // orig +0x8c — the thrower (CreateKnife @0x6da4f). The original keeps him
	                             // here and never sets pev->owner: BounceTouch skips and credits him through
	                             // it, AddToFullPack draws the knife with a glow shell for him alone, and
	                             // TSX reads it (TSKNIFE_OFFSET 35 on the original, rets_api GetKnifeThrower
	                             // on reTS). TSKnifeThrower is the bridge.
};

LINK_ENTITY_TO_CLASS(knife, CTSKnife); // orig classname "knife" (CTSWorldGun::ShouldCollide checks "knife")

void CTSKnife::Spawn()
{
	// RE CTSKnife::Spawn @0x6db54: the knife is a MOVETYPE_BOUNCE physics object with per-type gravity
	// and friction 0.8 — the engine bounces it; breaking is done ONLY by the swept WaitThink (below).
	CTSPhysicObject::Spawn(); // @0x6db6d — the slow-motion physics state
	pev->movetype = MOVETYPE_BOUNCE;
	pev->solid = SOLID_BBOX;
	pev->gravity = Gravity();   // 0.75 knife / 0.3 sealknife / 0.85 katana
	pev->friction = 0.8f;       // RE pev+0x120 = 0x3f4ccccd
	pev->fuser1 = 1.0f;         // TS timescale init (RE @0x6dca4) — feeds the EMIT_SOUND_DYN slow-pitch
	const char *mdl = (m_knifeType == TSW_KATANA) ? "models/w_katana.mdl"
		: (m_knifeType == TSW_SEALKNIFE) ? "models/w_sealknife.mdl"
		: "models/w_knife.mdl";
	SET_MODEL(ENT(pev), (char *)mdl); // precached at map load (PrecacheAllTSWeapons)
	UTIL_SetSize(pev, Vector(0, 0, 0), Vector(0, 0, 0)); // a point hull (RE SetSize zero,zero)
	// (angles/roll-90/avelocity spin are set by the THROWER after CreateKnife — KnifeFire -1300,
	// KatanaThrow -700 — exactly like the original; Spawn does not orient the blade.)
	SetTouch(&CTSKnife::BounceTouch);
	SetThink(&CTSKnife::WaitThink);
	// RE Spawn @0x6db54 line 130: nextthink = gpGlobals->time (NOT +0.1) — the first sweep fires THIS
	// frame, at the spawn point right in front of the thrower, so close glass breaks instantly. (Our old
	// +0.1 let the knife fly ~190u and hit the pane via engine collision before the first sweep ran.)
	pev->nextthink = gpGlobals->time;
	m_flExpire = gpGlobals->time + 30.0f; // RE DAT_00107148 = 30s vanish-if-it-never-lands
	m_bInFlight = true; // orig @0x6dcbe: this+0x90 = 1 — no +USE pickup while flying
}

// @ CTSKnife::WaitTouch (@0x6dce8) — the post-settle touch handler (installed by BounceTouch's settle
// tail; pOther is ignored). Pure ground damping: each on-ground contact halves the velocity; below
// 32 u/s the knife lies flat (pitch/roll zeroed) and stops. No pickup, no sound, no state change —
// the knife stays a MOVETYPE_BOUNCE point object; pickup is exclusively the +USE path (Use below).
void CTSKnife::WaitTouch(CBaseEntity *pOther)
{
	if (!(pev->flags & FL_ONGROUND))
		return;
	pev->velocity = pev->velocity * 0.5f;
	if (pev->velocity.Length() < 32.0f)
	{
		pev->angles.x = 0;
		pev->angles.z = 0;
		pev->velocity = g_vecZero;
	}
}

// @ CTSKnife::WaitThink (@0x6ea30) — the in-flight physics think, the SOLE breaker. Every 0.1s, if
// moving (speed > 64), trace a 64u forward SWEEP along the velocity and break any NON-PLAYER entity in
// front whose health the throw can kill outright (health <= 0.1*speed). The pane shatters BEFORE the
// knife's physics move reaches it, so the knife flies cleanly through (no collision, no bounce).
// (RE: attacker = the knife's own pev; damage = 0.1*speed; DMG bits 0x8001000; NO takedamage gate —
// TraceAttack is a no-op on a non-damageable hit.) The contact handler (BounceTouch) never breaks.
void CTSKnife::WaitThink()
{
	if (m_flExpire != 0 && gpGlobals->time > m_flExpire)
	{
		RemoveEntityFromSlowedArray(this); // WaitThink__8CTSKnife @0x6ea8a — scrub the packed slowed array before removal
		UTIL_Remove(this);
		return;
	}
	pev->nextthink = gpGlobals->time + 0.1f; // RE: DAT_00107138 think interval

	const float speed = pev->velocity.Length();
	if (speed > 64.0f) // RE: DAT_00107ea0 = 64 speed threshold + sweep distance
	{
		Vector dir = pev->velocity * (1.0f / speed);
		Vector end = pev->origin + dir * 64.0f;
		TraceResult tr;
		UTIL_TraceLine(pev->origin, end, dont_ignore_monsters, ENT(pev), &tr);
		if (tr.flFraction < 1.0f)
		{
			CBaseEntity *pHit = tr.pHit ? CBaseEntity::Instance(tr.pHit) : nullptr;
			const float dmg = 0.1f * speed; // RE: DAT_00107138(0.1) * speed
			if (pHit && !pHit->IsPlayer() && pHit->pev->health <= dmg)
			{
				ClearMultiDamage();
				pHit->TraceAttack(pev, dmg, dir * 64.0f, &tr, DMG_CLUB | DMG_NEVERGIB);
				ApplyMultiDamage(pev, pev);
			}
		}
	}
	CTSPhysicObject::WaitThink(); // @0x6ed41 — slow-motion physics upkeep (GoSlow sets fuser1)
}

// The knife's hit sounds deepen with the KNIFE ENTITY's own pev->fuser1 (init 1.0 at Spawn
// @0x6dca4, slowed via CBaseEntity::GoSlow), applied inside EMIT_SOUND_DYN with the default 0.5
// weight.

// @ CTSKnife::BounceTouch (@0x6decc, decompile-exact) — the contact handler. It NEVER breaks anything
// (the swept WaitThink is the sole breaker). Branches: stab PLAYERS (per-type damage, -0.25 bounce-off),
// worldspawn fast+glancing → deflect (no settle), worldspawn otherwise → EMBED (SOLID_NOT +
// MOVETYPE_FLY, back off 6u, roll 90 — fast OR slow), other thrown knives → ignore, any other solid →
// -0.25 BOUNCE. Every non-glance path funnels into the common settle tail @LAB_0007e985: hit sound,
// SetTouch(WaitTouch) + SetThink(WaitThink), in-flight cleared, gravity 0.75, expire = time + 10.
void CTSKnife::BounceTouch(CBaseEntity *pOther)
{
	pev->nextthink = gpGlobals->time + 0.1f; // RE BounceTouch resets nextthink
	if (!pOther || pOther->edict() == m_pThrower)
		return; // never touch the thrower (orig @0x6df18: knife+0x8c)
	if (pOther->pev->effects & EF_NODRAW)
		return; // RE @0x6decc: ((char)effects < 0) → skip EF_NODRAW (triggers/invisible brushes)

	const float speed = pev->velocity.Length();
	if (speed < 700.0f)
		m_bInFlight = false; // RE fc_700: slowed below throw speed → becomes +USE-recoverable

	const Vector flyDir = (speed > 0) ? pev->velocity * (1.0f / speed) : Vector(0, 0, 1);
	TraceResult tr = UTIL_GetGlobalTrace();

	// dmgreport (@0x6dffe): with cheats on (as the map started), every touch that gets this far
	// prints the blade's stab damage to everyone, whatever it touched.
	if (g_flWeaponCheat != 0 && CVAR_GET_FLOAT("dmgreport") > 1.0f)
		UTIL_ClientPrintAll(HUD_PRINTTALK, UTIL_VarArgs("(debug)%f\n", StabDamage()));

	m_iKatanaFlag = 0; // +0x91 — re-derived every touch

	// the common settle tail (@LAB_0007e985): sound + WaitTouch/WaitThink re-arm + settle state.
	const char *tailSnd = "weapons/knife/knife_hit.wav";
	bool tailSparks = true; // the stab path skips the sparks (jumps straight to the sound)

	// PLAYER stab (RE: takedamage != 0 && IsPlayer()) — damage, then bounce off (knife does NOT stick
	// in bodies in TS). Attacker credit goes to the thrower.
	if (pOther->pev->takedamage != DAMAGE_NO && pOther->IsPlayer())
	{
		entvars_t *pevOwner = m_pThrower ? VARS(m_pThrower) : pev; // orig @0x6e0b3: knife+0x8c
		ClearMultiDamage();
		pOther->TraceAttack(pevOwner, StabDamage(), flyDir, &tr, DMG_CLUB | DMG_NEVERGIB);
		ApplyMultiDamage(pev, pevOwner);
		pev->velocity = pev->velocity * -0.25f; // RE DAT_00107e98 bounce-back
		tailSnd = "weapons/knife/knife_hitbody.wav"; // ALL blade types use the knife wavs here
		tailSparks = false;
	}
	else
	{
		if (m_knifeType == TSW_KATANA)
			m_iKatanaFlag = 1; // a settled katana is +USE-gated on KungFu()

		if (FClassnameIs(pOther->pev, "worldspawn"))
		{
			// embed in the world: back off 6u along the flight, point the way it flew, roll 90,
			// SOLID_NOT + MOVETYPE_FLY (no gravity — it stays in the wall), full stop.
			auto embed = [&]()
			{
				Vector back = flyDir * 6.0f;
				UTIL_SetOrigin(pev, pev->origin - back);
				pev->angles = UTIL_VecToAngles(back);
				pev->angles.z = 90.0f;
				pev->solid = SOLID_NOT;
				pev->movetype = MOVETYPE_FLY;
				pev->velocity = g_vecZero;
				pev->avelocity = g_vecZero;
			};
			if (speed >= 100.0f)
			{
				// fast + too glancing (blade↔surface alignment < 0.4, not a near-flat floor) →
				// DEFLECT: spark + a high hit note, halve the speed, keep flying. NO settle tail.
				if (-DotProduct(tr.vecPlaneNormal, flyDir) < 0.4f && tr.vecPlaneNormal.z <= 0.98f)
				{
					if (UTIL_PointContents(pev->origin) != CONTENTS_SKY)
						UTIL_Sparks(pev->origin);
					EMIT_SOUND_DYN(ENT(pev), CHAN_BODY, "weapons/knife/knife_hit.wav", VOL_NORM, 0.8f, 0,
						(int)(100 + RANDOM_FLOAT(0, 10))); // orig fp-computed pitch @0x6e4f1; slow-deepening = the overload + our fuser1
					pev->velocity = pev->velocity * 0.5f; // RE: /2; engine clip already slid it
					return;
				}
				embed(); // fast head-on
			}
			else
			{
				// slow world touch: embed too, with an extra softer hit note before the tail.
				embed();
				EMIT_SOUND_DYN(ENT(pev), CHAN_BODY, "weapons/knife/knife_hit.wav", VOL_NORM, 0.8f, 0,
					(int)(100 + RANDOM_FLOAT(-10, 0))); // orig fp-computed pitch @0x6e6a2; slow-deepening = the overload + our fuser1
			}
		}
		else
		{
			if (FClassnameIs(pOther->pev, "knife"))
				return; // ignore other thrown knives (orig classname "knife")
			pev->velocity = pev->velocity * -0.25f; // bounce off any other solid (breakable/func_wall/door…)
		}
	}

	// --- the common settle tail (@LAB_0007e985) ---
	if (tailSparks && UTIL_PointContents(pev->origin) != CONTENTS_SKY)
		UTIL_Sparks(pev->origin);
	EMIT_SOUND_DYN(ENT(pev), CHAN_BODY, tailSnd, VOL_NORM, 0.8f, 0, 100); // orig @0x6e9a4: vol 1.0, attn 0.8, pitch 100, w 0.5 (default)
	SetTouch(&CTSKnife::WaitTouch);
	SetThink(&CTSKnife::WaitThink); // the swept-break/expire think keeps running after settle
	m_bInFlight = false;
	pev->nextthink = gpGlobals->time + 0.1f;
	pev->gravity = 0.75f;
	m_flExpire = gpGlobals->time + 10.0f; // a settled knife lives ~10 s unless picked up

	// The white PICKUPABLE glow shell on a settled blade — user-directed (ledger, playtest round 3),
	// kept. The original writes no render fields in the knife code: the shell its players see is
	// AddToFullPack's, a grey glow on a thrown knife for its thrower alone, in flight and settled
	// (client.cpp, m_pThrower). The KATANA is the exception here: NO shell (its recovery is
	// KungFu-gated, the +0x91 flag — playtest-confirmed).
	if (m_knifeType != TSW_KATANA)
	{
		pev->renderamt = 5;
		pev->renderfx = kRenderFxGlowShell;
		pev->rendercolor = Vector(200, 200, 200);
	}
}

CTSKnife *CTSKnife::CreateKnife(CBasePlayer *pOwner, const Vector &vecStart, const Vector &vecVel, int knifeType)
{
	CTSKnife *p = GetClassPtr((CTSKnife *)NULL);
	p->pev->classname = MAKE_STRING("knife");
	// the original stores the raw 6/7/8 code (knife+0xa0) and maps at each use point
	// (Use @0x6dda4: 8 -> katana, 6 -> knife, else -> sealknife); we keep the mapped TSW_* identity.
	p->m_knifeType = (knifeType == 8) ? TSW_KATANA : (knifeType == 6) ? TSW_KNIFE : TSW_SEALKNIFE;
	UTIL_SetOrigin(p->pev, vecStart);
	p->pev->velocity = vecVel;
	// @0x6da4f: the thrower goes to knife+0x8c; pev->owner stays NULL (no write in CreateKnife, Spawn,
	// KnifeFire or KatanaThrow), so the engine does not exempt the thrower from the knife's collisions.
	p->m_pThrower = pOwner->edict();
	p->Spawn();
	p->GoSlow(TSThrowSlow(pOwner), 0); // @0x6daa3: the thrower's slow-motion rate (Spawn set fuser1 1.0)
	return p;
}

// @0x6db04 GetDescription — the thrown-knife name shown in the pickup HUD, by knife type.
char *CTSKnife::GetDescription()
{
	switch (m_knifeType)
	{
	case TSW_KNIFE:     return (char *)"Throwing Combat Knife";
	case TSW_SEALKNIFE: return (char *)"Throwing Seal Knife";
	case TSW_KATANA:    return (char *)"Throwing Katana";
	default:            return (char *)"";
	}
}

// Header-visible bridge for AddToFullPack (CTSKnife lives in this TU): the thrower of a thrown blade
// (knife+0x8c), or NULL for any other entity.
edict_t *TSKnifeThrower(CBaseEntity *pEnt)
{
	if (pEnt && FClassnameIs(pEnt->pev, "knife"))
		return ((CTSKnife *)pEnt)->m_pThrower;
	return NULL;
}

// Header-visible bridge for TSGetPointsForFrag (CTSKnife lives in this TU): the thrown-blade
// kill-feed name, or "Throwing" for a non-knife entity (the original's NULL fallback).
const char *TSKnifeGetDescription(CBaseEntity *pEnt)
{
	if (pEnt && FClassnameIs(pEnt->pev, "knife"))
		return ((CTSKnife *)pEnt)->GetDescription();
	return "Throwing";
}

// @0x6dda4 Use — the ONLY pickup path (+USE via FCAP_CONTINUOUS_USE), decompile-exact gates:
// no pickup while in flight (+0x90); player/alive/!EF_NODRAW; a flagged katana (+0x91) only for a
// KungFu() player; an EXPIRED knife is silently cleaned up on use; a failed pickup (arsenal full /
// duplicate) leaves the knife on the ground. On success/expiry the knife is removed.
// (Original also calls RemoveEntityFromSlowedArray(this) — our slow-mo doesn't keep that array yet;
// lands with the TS_util freefunc group.)
void CTSKnife::Use(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value)
{
	if (m_bInFlight)
		return;
	if (!pActivator || !pActivator->IsPlayer() || !pActivator->IsAlive())
		return;
	if (pActivator->pev->effects & EF_NODRAW) // spectator/observer guard
		return;
	if (m_iKatanaFlag && !KungFu((CBasePlayer *)pActivator))
		return; // a thrown katana is recoverable ONLY by a kung-fu player (RE +0x91 gate; L13)

	if (m_flExpire > gpGlobals->time)
	{
		// @0x6de93: PickUpWeapon(type, NULL, 0, bSelect FALSE — recovering a thrown blade
		// does NOT switch you off your gun; an owned stack just gains the unit. The pickup
		// sound plays inside.
		if (!((CBasePlayer *)pActivator)->PickUpWeapon(m_knifeType, NULL, 0, false, false))
			return; // won't fit — the knife stays
	}
	SetTouch(nullptr);
	RemoveEntityFromSlowedArray(this); // Use__8CTSKnife @0x6deb5 — scrub the packed slowed array before removal
	UTIL_Remove(this);
}

// Public helper (ts_weapons.h) — CTSGun::ThrowKnife/KatanaThrow spawn the projectile through this
// (knifeType = the s_weapon_info+0x20 code, 6/7/8).
CBaseEntity *TSThrowKnife(CBasePlayer *pOwner, const Vector &vecStart, const Vector &vecVel, int knifeType)
{
	return CTSKnife::CreateKnife(pOwner, vecStart, vecVel, knifeType);
}
