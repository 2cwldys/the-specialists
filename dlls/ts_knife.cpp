/***
*  ts_knife.cpp — CTSKnife, the thrown knife/seal-knife projectile (right-click throw).
*
*  RE: CTSGun::SecondaryAttack (@0x817a8) routes a knife (fire-type 3) to KnifeFire's THROW branch,
*  which spawns a CTSKnife via CreateKnife (@0x6d9e4) flying forward + tumbling. Collision (RE:
*  CTSKnife::BounceTouch @0x6decc): it embeds in a non-breakable surface (worldspawn → stick), bounces
*  off solids it strikes at a glance / can't damage, and damages what it hits. The faithful BEHAVIOUR
*  the user asked for: STICK into non-breakable non-flesh surfaces; BOUNCE off a breakable it can't
*  break; if it CAN break it, the breakable shatters and the knife punches THROUGH to stick in the
*  wall beyond. CTSKnife is a CTSPhysicObject in the original; this is a clean modern projectile on
*  the stock SDK.
***/
#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "player.h"
#include "ts_weapons.h"
#include "ts_gun.h"

class CTSKnife : public CBaseEntity
{
public:
	void Spawn() override;
	int Classify() override { return CLASS_NONE; }
	void EXPORT KnifeTouch(CBaseEntity *pOther);
	void EXPORT PickupTouch(CBaseEntity *pOther);
	void StickAt(const Vector &pos, const Vector &flyDir);
	static CTSKnife *Throw(CBasePlayer *pOwner, int type, float damage, const Vector &vecStart, const Vector &vecVel);

	int m_knifeType = TSW_KNIFE;
	float m_flDamage = 40.0f;

	const char *HitSound() const { return (m_knifeType == TSW_KATANA) ? "weapons/katana/hit.wav" : "weapons/knife/knife_hit.wav"; }
	const char *BodySound() const { return (m_knifeType == TSW_KATANA) ? "weapons/katana/hit-body.wav" : "weapons/knife/knife_hitbody.wav"; }
};

LINK_ENTITY_TO_CLASS(ts_knife, CTSKnife);

void CTSKnife::Spawn()
{
	pev->movetype = MOVETYPE_TOSS; // a gravity arc (the original's CTSPhysicObject bounces; TOSS is close)
	pev->solid = SOLID_BBOX;
	pev->gravity = 0.5f;
	const char *mdl = (m_knifeType == TSW_KATANA) ? "models/w_katana.mdl"
		: (m_knifeType == TSW_SEALKNIFE) ? "models/w_sealknife.mdl"
		: "models/w_knife.mdl";
	SET_MODEL(ENT(pev), (char *)mdl); // precached at map load (PrecacheAllTSWeapons)
	UTIL_SetSize(pev, Vector(0, 0, 0), Vector(0, 0, 0));
	pev->angles = UTIL_VecToAngles(pev->velocity);
	pev->avelocity = Vector(-1300, 0, 0); // tumble end-over-end (RE: ~-1300 pitch spin)
	SetTouch(&CTSKnife::KnifeTouch);
	SetThink(&CBaseEntity::SUB_Remove); // clean up if it never strikes anything
	pev->nextthink = gpGlobals->time + 15.0f;
}

// Embed the knife at `pos`, pointing the way it flew, as a recoverable pickup (walk into it).
void CTSKnife::StickAt(const Vector &pos, const Vector &flyDir)
{
	pev->velocity = g_vecZero;
	pev->avelocity = g_vecZero;
	pev->movetype = MOVETYPE_NONE;
	pev->angles = UTIL_VecToAngles(flyDir);
	// Become a recoverable trigger. Set the solid + a real (small) trigger size, THEN relink via
	// UTIL_SetSize/SetOrigin — changing pev->solid without relinking trips "trigger in clipping list".
	pev->solid = SOLID_TRIGGER;
	UTIL_SetSize(pev, Vector(-16, -16, -16), Vector(16, 16, 16)); // a USE-pickup radius (relinks the edict)
	UTIL_SetOrigin(pev, pos - flyDir * 4.0f); // pull back a touch so the blade sits IN the surface
	if (UTIL_PointContents(pos) != CONTENTS_SKY)
		UTIL_Sparks(pos);
	EMIT_SOUND(ENT(pev), CHAN_WEAPON, HitSound(), VOL_NORM, ATTN_NORM);
	// WHITE glow shell so the stuck knife reads as a pickup (every TS drop glows — see CTSWorldGun).
	pev->renderfx = kRenderFxGlowShell;
	pev->rendercolor = Vector(255, 255, 255);
	pev->renderamt = 4;
	SetTouch(&CTSKnife::PickupTouch);
	SetThink(&CBaseEntity::SUB_Remove);
	pev->nextthink = gpGlobals->time + 30.0f; // linger as a pickup, then vanish
}

void CTSKnife::KnifeTouch(CBaseEntity *pOther)
{
	if (!pOther || pOther->edict() == pev->owner)
		return; // never collide with the thrower or nothing
	if (FClassnameIs(pOther->pev, "ts_knife"))
		return; // ignore other thrown knives in flight

	entvars_t *pevOwner = pev->owner ? VARS(pev->owner) : pev;
	const Vector flyDir = pev->velocity.Normalize();
	const bool flesh = pOther->IsPlayer() || (pOther->pev->flags & FL_MONSTER) != 0;
	const bool damageable = pOther->pev->takedamage != DAMAGE_NO;

	if (flesh)
	{
		// Stab a victim — damage, then drop a recoverable knife where it struck.
		TraceResult tr = UTIL_GetGlobalTrace();
		ClearMultiDamage();
		pOther->TraceAttack(pevOwner, m_flDamage, flyDir, &tr, DMG_CLUB | DMG_NEVERGIB);
		ApplyMultiDamage(pev, pevOwner);
		EMIT_SOUND(ENT(pev), CHAN_WEAPON, BodySound(), VOL_NORM, ATTN_NORM);
		TSCreateWorldGun(m_knifeType, 1, pev->origin, g_vecZero);
		SetTouch(nullptr);
		UTIL_Remove(this);
		return;
	}

	if (damageable)
	{
		// A breakable / prop. Can our throw break it (enough damage for its remaining health)?
		const bool canBreak = (pOther->pev->health <= m_flDamage);
		TraceResult tr = UTIL_GetGlobalTrace();
		ClearMultiDamage();
		pOther->TraceAttack(pevOwner, m_flDamage, flyDir, &tr, DMG_CLUB | DMG_NEVERGIB);
		ApplyMultiDamage(pev, pevOwner);

		if (canBreak)
		{
			// It shatters — punch THROUGH and stick in the wall beyond. Trace forward IGNORING glass
			// (transparent brushes) so we find the solid surface past the breakable it just destroyed.
			TraceResult fwd;
			UTIL_TraceLine(pev->origin, pev->origin + flyDir * 4096.0f, ignore_monsters, ignore_glass, ENT(pev), &fwd);
			StickAt(fwd.vecEndPos, flyDir);
			return;
		}

		// Couldn't break it — bounce off (the original reverses + dampens the velocity).
		EMIT_SOUND(ENT(pev), CHAN_WEAPON, HitSound(), VOL_NORM, ATTN_NORM);
		pev->velocity = pev->velocity * -0.4f;
		pev->avelocity = pev->avelocity * -1.0f;
		return;
	}

	// A non-breakable solid (wall / floor) — embed in place.
	StickAt(pev->origin, flyDir);
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

CTSKnife *CTSKnife::Throw(CBasePlayer *pOwner, int type, float damage, const Vector &vecStart, const Vector &vecVel)
{
	CTSKnife *p = GetClassPtr((CTSKnife *)NULL);
	p->pev->classname = MAKE_STRING("ts_knife");
	p->m_knifeType = type;
	p->m_flDamage = damage;
	UTIL_SetOrigin(p->pev, vecStart);
	p->pev->velocity = vecVel;
	p->pev->owner = pOwner->edict();
	p->Spawn();
	return p;
}

// Public helper (ts_weapons.h) — CTSGun::ThrowKnife spawns the projectile through this.
CBaseEntity *TSThrowKnife(CBasePlayer *pOwner, int type, float damage, const Vector &vecStart, const Vector &vecVel)
{
	return CTSKnife::Throw(pOwner, type, damage, vecStart, vecVel);
}
