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
	void Deflect(const Vector &pos, const Vector &normal, const Vector &flyDir);
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

// Too-glancing (steep-angle) hit? RE CTSKnife::BounceTouch: the knife only sticks/breaks-through when
// it hits flat enough — deflect when the blade↔surface alignment -(normal·flyDir) < 0.4 AND it's not a
// near-flat floor (normal.z <= 0.98). Applied at EVERY surface (each pane in a chain, and walls).
static bool KnifeGlances(const Vector &normal, const Vector &flyDir)
{
	const float align = -DotProduct(normal, flyDir); // 1 = head-on, ~0 = glancing
	return (align < 0.4f && normal.z <= 0.98f);
}

// Deflect off a too-steeply-hit surface: reposition at the surface with a reflected, dampened velocity
// and keep flying (the physics carries it on). Does NOT break/stick.
void CTSKnife::Deflect(const Vector &pos, const Vector &normal, const Vector &flyDir)
{
	const float speed = pev->velocity.Length();
	Vector refl = flyDir - normal * (2.0f * DotProduct(flyDir, normal)); // mirror off the surface
	UTIL_SetOrigin(pev, pos - flyDir * 4.0f);
	pev->velocity = refl * (speed * 0.5f); // lose half the speed on the glance
	pev->angles = UTIL_VecToAngles(refl);
	if (UTIL_PointContents(pos) != CONTENTS_SKY)
		UTIL_Sparks(pos);
	EMIT_SOUND(ENT(pev), CHAN_WEAPON, HitSound(), VOL_NORM, ATTN_NORM);
}

void CTSKnife::KnifeTouch(CBaseEntity *pOther)
{
	if (!pOther || pOther->edict() == pev->owner)
		return; // never collide with the thrower or nothing
	if (FClassnameIs(pOther->pev, "ts_knife"))
		return; // ignore other thrown knives in flight

	entvars_t *pevOwner = pev->owner ? VARS(pev->owner) : pev;
	const Vector flyDir = pev->velocity.Normalize();
	// Walk the knife's flight path one surface at a time: ALWAYS damage a breakable it hits (so damage
	// ACCUMULATES across throws — weak glass breaks after a few, strong/high-health glass survives any
	// number); on a head-on break it PUNCHES THROUGH to the next pane (the whole chain breaks); a
	// glancing hit, or a pane that survives, DEFLECTS it off. Walls stick head-on / deflect glancing.
	// A trace-loop (not physics) because MOVETYPE_TOSS clips the velocity flat against a pane (slides).
	// The knife LOSES ENERGY per pane it breaks (dmg halves), so it usually breaks one pane and drops
	// at the next — it almost never chains through more than ~two, matching the original.
	TraceResult tr = UTIL_GetGlobalTrace();
	CBaseEntity *pSurf = pOther;
	float dmg = m_flDamage;
	for (int layer = 0; layer < 8; layer++)
	{
		const Vector normal = tr.vecPlaneNormal;
		const bool glancing = KnifeGlances(normal, flyDir);
		const bool surfFlesh = pSurf && (pSurf->IsPlayer() || (pSurf->pev->flags & FL_MONSTER) != 0);
		const bool surfDamageable = pSurf && pSurf->pev->takedamage != DAMAGE_NO;

		if (surfFlesh)
		{
			// Stab a body — damage + drop a recoverable knife where it struck.
			ClearMultiDamage();
			pSurf->TraceAttack(pevOwner, m_flDamage, flyDir, &tr, DMG_CLUB | DMG_NEVERGIB);
			ApplyMultiDamage(pev, pevOwner);
			EMIT_SOUND(ENT(pev), CHAN_WEAPON, BodySound(), VOL_NORM, ATTN_NORM);
			TSCreateWorldGun(m_knifeType, 1, tr.vecEndPos, g_vecZero);
			SetTouch(nullptr);
			UTIL_Remove(this);
			return;
		}

		if (surfDamageable)
		{
			// ANGLE-SCALED damage: a head-on throw does full damage (breaks weak glass), a glancing
			// hit does proportionally less (just chips it). Damage ACCUMULATES across throws, so a
			// pane chips down over several hits; a strong / high-health pane survives any number.
			// (RE caveat: the binary's knife→glass break path isn't statically located — both
			// BreakTouch & BounceTouch gate on IsPlayer — so this models the OBSERVED behaviour with
			// the RE-grounded angle (-(normal·flyDir)) + standard CBreakable health.)
			float align = -DotProduct(normal, flyDir); // 1 = head-on, ~0 = glancing
			if (align < 0.0f)
				align = 0.0f;
			ClearMultiDamage();
			pSurf->TraceAttack(pevOwner, dmg * align, flyDir, &tr, DMG_CLUB | DMG_NEVERGIB);
			ApplyMultiDamage(pev, pevOwner);
			const bool broke = (pSurf->pev->solid == SOLID_NOT) || (pSurf->pev->health <= 0);

			if (broke)
			{
				// Broke it — punch THROUGH to the next surface, but with HALF the energy, so the next
				// pane usually survives and the knife drops there (it rarely chains past ~two panes).
				dmg *= 0.5f;
				Vector start = tr.vecEndPos + flyDir * 2.0f;
				UTIL_TraceLine(start, start + flyDir * 4096.0f, dont_ignore_monsters, ENT(pev), &tr);
				if (tr.flFraction == 1.0f)
				{
					StickAt(tr.vecEndPos, flyDir); // open air ahead — settle at the end
					return;
				}
				pSurf = tr.pHit ? CBaseEntity::Instance(tr.pHit) : nullptr;
				continue; // process the next pane / wall
			}

			// Couldn't break it (too strong, or too glancing to do enough) — the knife FALLS STRAIGHT
			// DOWN. It NEVER sticks in glass; it keeps the damage it dealt (so later throws chip it
			// down). Kill the horizontal velocity + back off the pane so it doesn't re-touch, and let
			// gravity drop it (it becomes a floor pickup when it lands).
			EMIT_SOUND(ENT(pev), CHAN_WEAPON, HitSound(), VOL_NORM, ATTN_NORM);
			if (UTIL_PointContents(tr.vecEndPos) != CONTENTS_SKY)
				UTIL_Sparks(tr.vecEndPos);
			pev->velocity.x = 0;
			pev->velocity.y = 0;
			if (pev->velocity.z > 0)
				pev->velocity.z = 0; // drop, don't pop up
			UTIL_SetOrigin(pev, pev->origin - flyDir * 6.0f);
			return;
		}

		// A non-breakable solid wall — STICK head-on, DEFLECT if glancing (RE thresholds 0.4 / 0.98).
		if (glancing)
			Deflect(tr.vecEndPos, normal, flyDir);
		else
			StickAt(tr.vecEndPos, flyDir);
		return;
	}
	StickAt(pev->origin, flyDir); // safety: ran out of layers
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
