/***
*  ts_knife.cpp — CTSKnife, the thrown knife/seal-knife projectile (right-click throw).
*
*  RE: CTSGun::SecondaryAttack (@0x817a8) routes a knife (fire-type 3) to KnifeFire's THROW branch,
*  which spawns a CTSKnife via CreateKnife (@0x6d9e4) flying forward + tumbling (avelocity spin),
*  with the per-type world model (w_knife / w_sealknife / w_katana). CTSKnife is a CTSPhysicObject
*  in the original; this is a clean modern projectile on the stock SDK — it deals melee damage to a
*  victim it strikes (or thunks into a surface) and drops a recoverable CTSWorldGun where it lands so
*  the knife can be picked back up.
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
	static CTSKnife *Throw(CBasePlayer *pOwner, int type, float damage, const Vector &vecStart, const Vector &vecVel);

	int m_knifeType = TSW_KNIFE;
	float m_flDamage = 40.0f;
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

void CTSKnife::KnifeTouch(CBaseEntity *pOther)
{
	if (pOther && pOther->edict() == pev->owner)
		return; // never hit the thrower

	entvars_t *pevOwner = pev->owner ? VARS(pev->owner) : pev;
	const bool isKatana = (m_knifeType == TSW_KATANA);
	if (pOther && pOther->pev->takedamage != DAMAGE_NO)
	{
		TraceResult tr = UTIL_GetGlobalTrace();
		ClearMultiDamage();
		pOther->TraceAttack(pevOwner, m_flDamage, pev->velocity.Normalize(), &tr, DMG_CLUB | DMG_NEVERGIB);
		ApplyMultiDamage(pev, pevOwner);
		EMIT_SOUND(ENT(pev), CHAN_WEAPON, isKatana ? "weapons/katana/hit-body.wav" : "weapons/knife/knife_hitbody.wav", VOL_NORM, ATTN_NORM);
	}
	else
	{
		EMIT_SOUND(ENT(pev), CHAN_WEAPON, isKatana ? "weapons/katana/hit.wav" : "weapons/knife/knife_hit.wav", VOL_NORM, ATTN_NORM);
	}

	// Drop a recoverable knife where it landed (walk over it to pick it back up).
	TSCreateWorldGun(m_knifeType, 1, pev->origin, g_vecZero);
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
