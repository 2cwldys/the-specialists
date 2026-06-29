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

LINK_ENTITY_TO_CLASS(ts_knife, CTSKnife);

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
	if (FClassnameIs(pOther->pev, "ts_knife"))
		return; // RE BounceTouch else-branch: ignore other thrown knives (orig classname "knife", ours "ts_knife")
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
	p->pev->classname = MAKE_STRING("ts_knife");
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
