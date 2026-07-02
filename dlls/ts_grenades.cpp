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
#include "player.h" // CBasePlayer::m_flSlowMotionGo (slow-mo propagation in CreateGrenade)
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
	pShrap->pev->classname = MAKE_STRING("shrapnel");
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

#define M61_SHAKE_MAG    110.0f  // screen-shake amplitude + debris size (RE @0x42dc0000 = 110)
#define M61_BLAST_DAMAGE 300.0f  // VERIFY: recovered TSExplosion literal (was a dynamic per-throw value)
#define M61_BLAST_RADIUS 1024.0f // VERIFY: recovered TSExplosion literal @0x44800000 (was a tuned 400 guess)

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

	// Carry the thrower's slow-motion factor onto the grenade (the original's GoSlow propagation; our
	// slow-mo is per-entity via fuser1 rather than the original's slowed-entity array). VERIFY.
	CBaseEntity *pOwner = CBaseEntity::Instance(pevOwner);
	if (pOwner && pOwner->IsPlayer())
		pGrenade->pev->fuser1 = ((CBasePlayer *)pOwner)->m_flSlowMotionGo;
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
			TSExplosion(pev->origin, pev, pev->owner ? VARS(pev->owner) : pev,
				M61_SHAKE_MAG, M61_BLAST_DAMAGE, M61_BLAST_RADIUS);
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
*  through) and bounce are mutually exclusive. CTSKnife is a CTSPhysicObject; this is a clean modern
*  projectile on the stock SDK (engine MOVETYPE_BOUNCE replaces the original's CTSPhysicObject physics).
***/
#include "ts_gun.h"

class CTSKnife : public CBaseEntity
{
public:
	void Spawn() override;
	int Classify() override { return CLASS_NONE; }
	void EXPORT BounceTouch(CBaseEntity *pOther); // @CTSKnife::BounceTouch @0x6decc — contact handler
	void EXPORT WaitThink();                     // @CTSKnife::WaitThink @0x6ea30 — in-flight swept break
	void EXPORT PickupTouch(CBaseEntity *pOther);
	void EXPORT SettleThink();                    // after first contact: fall, poll FL_ONGROUND, become the pickup
	void StickAt(const Vector &pos, const Vector &flyDir);
	void StartSettle();                           // hand a bounced/stabbed knife off to SettleThink
	void BecomePickup();                          // turn the stationary knife into a recoverable glowing E-pickup
	void Use(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value) override; // @0x6dda4 — +use pickup
	char *GetDescription();                       // @0x6db04 — the thrown-knife name by type
	int  ObjectCaps() override { return FCAP_CONTINUOUS_USE; } // @0x6f04c -> 0x10
	virtual bool ShouldCollide(CBaseEntity *pOther) { return true; } // @0x6ed54 -> 1 (TS virtual)
	static CTSKnife *CreateKnife(CBasePlayer *pOwner, int type, float damage, const Vector &vecStart, const Vector &vecVel); // @0x6d9e4

	float StabDamage() const // RE BounceTouch: per-type player-stab damage (DAT_00107ec0/eac/0010761c)
	{
		return (m_knifeType == TSW_KATANA) ? 150.0f : (m_knifeType == TSW_SEALKNIFE) ? 50.0f : 100.0f;
	}
	float Gravity() const // RE Spawn: pev->gravity by knife type
	{
		return (m_knifeType == TSW_KATANA) ? 0.85f : (m_knifeType == TSW_SEALKNIFE) ? 0.3f : 0.75f;
	}

	int m_knifeType = TSW_KNIFE;
	float m_flDamage = 40.0f;
	float m_flExpire = 0; // auto-remove time if it never lands (orig this+0x88)

	const char *HitSound() const { return (m_knifeType == TSW_KATANA) ? "weapons/katana/hit.wav" : "weapons/knife/knife_hit.wav"; }
	const char *BodySound() const { return (m_knifeType == TSW_KATANA) ? "weapons/katana/hit-body.wav" : "weapons/knife/knife_hitbody.wav"; }
};

LINK_ENTITY_TO_CLASS(knife, CTSKnife); // orig classname "knife" (CTSWorldGun::ShouldCollide checks "knife")

void CTSKnife::Spawn()
{
	// RE CTSKnife::Spawn @0x6db54: the knife is a MOVETYPE_BOUNCE physics object with per-type gravity
	// and friction 0.8 — the engine bounces it; breaking is done ONLY by the swept WaitThink (below).
	pev->movetype = MOVETYPE_BOUNCE;
	pev->solid = SOLID_BBOX;
	pev->gravity = Gravity();   // 0.75 knife / 0.3 sealknife / 0.85 katana
	pev->friction = 0.8f;       // RE pev+0x120 = 0x3f4ccccd
	const char *mdl = (m_knifeType == TSW_KATANA) ? "models/w_katana.mdl"
		: (m_knifeType == TSW_SEALKNIFE) ? "models/w_sealknife.mdl"
		: "models/w_knife.mdl";
	SET_MODEL(ENT(pev), (char *)mdl); // precached at map load (PrecacheAllTSWeapons)
	UTIL_SetSize(pev, Vector(0, 0, 0), Vector(0, 0, 0)); // a point hull (RE SetSize zero,zero)
	pev->angles = UTIL_VecToAngles(pev->velocity);
	pev->avelocity = Vector(-1300, 0, 0); // tumble end-over-end (visual)
	SetTouch(&CTSKnife::BounceTouch);
	SetThink(&CTSKnife::WaitThink);
	// RE Spawn @0x6db54 line 130: nextthink = gpGlobals->time (NOT +0.1) — the first sweep fires THIS
	// frame, at the spawn point right in front of the thrower, so close glass breaks instantly. (Our old
	// +0.1 let the knife fly ~190u and hit the pane via engine collision before the first sweep ran.)
	pev->nextthink = gpGlobals->time;
	m_flExpire = gpGlobals->time + 30.0f; // RE DAT_00107148 = 30s vanish-if-it-never-lands
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
}

// Turn the (now-stationary) knife into a recoverable USE-pickup: a small trigger with the white drop
// glow, picked up on E. Our nicety — the original leaves a settled knife inert; we make every drop pickable.
void CTSKnife::BecomePickup()
{
	// A KATANA is a single weapon — throwing it LOSES it: a stuck/settled katana stays embedded as
	// inert decoration and is NOT pickup-removable (per the original; the user confirmed). Only the
	// stackable throwing knives (knife/seal-knife) become recoverable pickups.
	if (m_knifeType == TSW_KATANA)
	{
		pev->velocity = g_vecZero;
		pev->avelocity = g_vecZero;
		pev->movetype = MOVETYPE_NONE;
		pev->solid = SOLID_NOT;       // embedded blade — no pickup trigger, no collision
		SetTouch(nullptr);
		SetThink(&CBaseEntity::SUB_Remove);
		pev->nextthink = gpGlobals->time + 60.0f; // linger as decoration, then vanish (no edict leak)
		return;
	}

	pev->velocity = g_vecZero;
	pev->avelocity = g_vecZero;
	pev->movetype = MOVETYPE_NONE;
	// Become a recoverable trigger. Set the solid + a real (small) trigger size, THEN relink via
	// UTIL_SetSize/SetOrigin — changing pev->solid without relinking trips "trigger in clipping list".
	pev->solid = SOLID_TRIGGER;
	UTIL_SetSize(pev, Vector(-16, -16, -16), Vector(16, 16, 16)); // a USE-pickup radius (relinks the edict)
	UTIL_SetOrigin(pev, pev->origin);                             // relink at the rest position
	// WHITE glow shell so the dropped knife reads as a pickup (every TS drop glows — see CTSWorldGun).
	pev->renderfx = kRenderFxGlowShell;
	pev->rendercolor = Vector(255, 255, 255);
	pev->renderamt = 4;
	SetTouch(&CTSKnife::PickupTouch);
	SetThink(&CBaseEntity::SUB_Remove);
	pev->nextthink = gpGlobals->time + 30.0f; // linger as a pickup, then vanish
}

// Pitch a thrown-knife impact sound by the THROWER's slow factor (slow-motion deepens it, like every
// other normal-recorded combat sound — see TSSlowPitch). 100 (normal) if the thrower isn't a player.
static int KnifeHitPitch(CBaseEntity *knife)
{
	CBaseEntity *thrower = knife->pev->owner ? CBaseEntity::Instance(knife->pev->owner) : nullptr;
	if (thrower && thrower->IsPlayer())
		return TSSlowPitch((CBasePlayer *)thrower);
	return 100;
}

// Embed the knife at `pos`, pointing the way it flew, as a recoverable pickup (walk into it). Worldspawn only.
void CTSKnife::StickAt(const Vector &pos, const Vector &flyDir)
{
	pev->angles = UTIL_VecToAngles(flyDir);
	pev->angles.z = 90.0f; // RE worldspawn stick: roll the embedded blade 90° (orig sets angles.z = 90)
	UTIL_SetOrigin(pev, pos - flyDir * 4.0f); // pull back a touch so the blade sits IN the surface
	if (UTIL_PointContents(pos) != CONTENTS_SKY)
		UTIL_Sparks(pos);
	EMIT_SOUND_DYN(ENT(pev), CHAN_WEAPON, HitSound(), VOL_NORM, ATTN_NORM, 0, KnifeHitPitch(this));
	BecomePickup(); // white glow + E-pickup at the embed point
}

// Too-glancing (steep-angle) hit? RE CTSKnife::BounceTouch worldspawn path: the knife only sticks when
// it hits flat enough — it SLIDES/deflects when the blade↔surface alignment -(normal·flyDir) < 0.4 AND
// it's not a near-flat floor (normal.z <= 0.98). Thresholds DAT_00107ed8 (0.4) / DAT_00107ed0 (0.98).
static bool KnifeGlances(const Vector &normal, const Vector &flyDir)
{
	const float align = -DotProduct(normal, flyDir); // 1 = head-on, ~0 = glancing
	return (align < 0.4f && normal.z <= 0.98f);
}

// @ CTSKnife::BounceTouch (@0x6decc) — the contact handler. It NEVER breaks anything (the swept
// WaitThink is the sole breaker). It only: stabs PLAYERS (per-type damage, then bounces off), sticks in
// the WORLD head-on / slides off it glancing, and BOUNCES off every other solid (incl. breakables the
// sweep couldn't kill — they bounce, they don't break). Break (sweep, pre-contact → fly through) and
// bounce (here) are thus mutually exclusive, exactly like the original.
void CTSKnife::BounceTouch(CBaseEntity *pOther)
{
	pev->nextthink = gpGlobals->time + 0.1f; // RE BounceTouch resets nextthink
	if (!pOther || pOther->edict() == pev->owner)
		return; // never collide with the thrower
	if (pOther->pev->effects & EF_NODRAW)
		return; // RE BounceTouch @0x6decc: ((char)effects < 0) → skip EF_NODRAW (triggers/invisible brushes)

	const Vector flyDir = pev->velocity.Normalize();
	const float speed = pev->velocity.Length();
	TraceResult tr = UTIL_GetGlobalTrace();

	// PLAYER stab (RE: takedamage != 0 && IsPlayer()) — damage, then bounce off (knife does NOT stick in
	// bodies in TS). Attacker credit goes to the thrower.
	if (pOther->pev->takedamage != DAMAGE_NO && pOther->IsPlayer())
	{
		entvars_t *pevOwner = pev->owner ? VARS(pev->owner) : pev;
		ClearMultiDamage();
		pOther->TraceAttack(pevOwner, StabDamage(), flyDir, &tr, DMG_CLUB | DMG_NEVERGIB);
		ApplyMultiDamage(pev, pevOwner);
		EMIT_SOUND_DYN(ENT(pev), CHAN_WEAPON, BodySound(), VOL_NORM, ATTN_NORM, 0, KnifeHitPitch(this));
		pev->velocity = pev->velocity * -0.25f; // RE DAT_00107e98 bounce-back
		m_flExpire = gpGlobals->time + 10.0f;   // RE DAT_00107140 post-bounce expire
		StartSettle();                          // RE LAB_0007e985: hand off to the ground-settle handler
		return;
	}

	// WORLDSPAWN (the BSP world) — the ONLY thing the knife sticks in (RE: func_wall/breakables bounce).
	if (FClassnameIs(pOther->pev, "worldspawn"))
	{
		// Head-on (or slow) → embed as a recoverable pickup. Glancing & fast → slide off, dampened.
		if (speed >= 100.0f && KnifeGlances(tr.vecPlaneNormal, flyDir)) // RE: speed >= DAT_0010761c(100)
		{
			if (UTIL_PointContents(pev->origin) != CONTENTS_SKY)
				UTIL_Sparks(pev->origin);
			EMIT_SOUND_DYN(ENT(pev), CHAN_WEAPON, HitSound(), VOL_NORM, ATTN_NORM, 0, KnifeHitPitch(this));
			pev->velocity = pev->velocity * 0.5f; // RE: /= DAT_00107ec4(2); engine clip already slid it
			return;
		}
		StickAt(pev->origin - flyDir * 6.0f, flyDir); // RE: back off DAT_00107e9c(6) along flight, embed
		return;
	}

	// ANY OTHER non-player solid (breakable the sweep couldn't kill, func_wall, door…) → BOUNCE off.
	// RE: the original NEVER sticks here — it bounces (-0.25·vel) and settles flat on the FLOOR (so it never
	// ends up stuck in a glass pane); we then make the rested knife a glowing pickup like every other drop.
	if (FClassnameIs(pOther->pev, "knife"))
		return; // RE BounceTouch else-branch: ignore other thrown knives (orig classname "knife")
	if (UTIL_PointContents(pev->origin) != CONTENTS_SKY)
		UTIL_Sparks(pev->origin);
	EMIT_SOUND_DYN(ENT(pev), CHAN_WEAPON, HitSound(), VOL_NORM, ATTN_NORM, 0, KnifeHitPitch(this));
	pev->velocity = pev->velocity * -0.25f; // RE DAT_00107e98 bounce-back
	m_flExpire = gpGlobals->time + 10.0f;   // RE DAT_00107140 post-bounce expire
	StartSettle();                          // RE LAB_0007e985: hand off to the ground-settle handler
}

// After its first non-stick contact (stab or bounce) the knife is done flying — settle it like every other
// dropped weapon (mirrors CTSWorldGun: MOVETYPE_TOSS to the floor, poll FL_ONGROUND in a think, then become
// the glowing USE-pickup). We use a think (not the original's touch-based WaitTouch @0x6dce8) because a
// zero-hull MOVETYPE_BOUNCE knife rarely gets FL_ONGROUND from a touch, so most bounced knives never became
// pickups (and so never lit the glow). This is our pickup nicety; the original leaves a bounced knife inert.
void CTSKnife::StartSettle()
{
	pev->gravity = 0.75f;            // RE LAB_0007e985: gravity 0.75 after first contact
	pev->movetype = MOVETYPE_TOSS;  // fall + land reliably (FL_ONGROUND), exactly like CTSWorldGun
	SetTouch(nullptr);              // no more contact handling — the sweep already had its chance
	SetThink(&CTSKnife::SettleThink);
	pev->nextthink = gpGlobals->time + 0.1f;
}

void CTSKnife::SettleThink()
{
	pev->nextthink = gpGlobals->time + 0.1f;
	if (m_flExpire != 0 && gpGlobals->time > m_flExpire) // never reached the ground (fell in a pit, etc.)
	{
		UTIL_Remove(this);
		return;
	}
	if (!(pev->flags & FL_ONGROUND)) // same landing test as CTSWorldGun::FallThink
		return;
	pev->avelocity = g_vecZero;
	pev->angles.x = 0; // lie flat
	pev->angles.z = 0;
	BecomePickup();    // white glow + E-pickup, identical to every other TS ground weapon
}

void CTSKnife::PickupTouch(CBaseEntity *pOther)
{
	if (!pOther || !pOther->IsPlayer() || !pOther->IsAlive())
		return;
	if (!(pOther->pev->button & IN_USE)) // press USE (E) to pick up — like every TS dropped weapon
		return;
	TSPickUpWeapon((CBasePlayer *)pOther, m_knifeType, 1);
	EMIT_SOUND(ENT(pev), CHAN_ITEM, "items/gunpickup2.wav", VOL_NORM, ATTN_NORM);
	SetTouch(nullptr);
	UTIL_Remove(this);
}

CTSKnife *CTSKnife::CreateKnife(CBasePlayer *pOwner, int type, float damage, const Vector &vecStart, const Vector &vecVel)
{
	CTSKnife *p = GetClassPtr((CTSKnife *)NULL);
	p->pev->classname = MAKE_STRING("knife");
	p->m_knifeType = type;
	p->m_flDamage = damage;
	UTIL_SetOrigin(p->pev, vecStart);
	p->pev->velocity = vecVel;
	p->pev->owner = pOwner->edict();
	p->Spawn();
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

// @0x6dda4 Use — +use pickup (the impulse path; the same TSPickUpWeapon our touch path uses). The
// original gates on a settle timer (this+0x88) and removes the knife on a successful pickup.
void CTSKnife::Use(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value)
{
	if (!pActivator || !pActivator->IsPlayer() || !pActivator->IsAlive())
		return;
	if (pActivator->pev->effects & EF_NODRAW) // spectator/observer guard
		return;
	TSPickUpWeapon((CBasePlayer *)pActivator, m_knifeType, 1);
	EMIT_SOUND(ENT(pev), CHAN_ITEM, "items/gunpickup2.wav", VOL_NORM, ATTN_NORM);
	SetTouch(nullptr);
	UTIL_Remove(this);
}

// Public helper (ts_weapons.h) — CTSGun::ThrowKnife spawns the projectile through this.
CBaseEntity *TSThrowKnife(CBasePlayer *pOwner, int type, float damage, const Vector &vecStart, const Vector &vecVel)
{
	return CTSKnife::CreateKnife(pOwner, type, damage, vecStart, vecVel);
}
