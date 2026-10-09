/***
*  ts_worldgun.cpp — RECONSTRUCTED CTSWorldGun + CTSAmmoPack (original TS_worldgun.cpp).
*  Functional/source-faithful (see CLAUDE.md). Recovered from ts_i386.so.
*
*  CTSWorldGun (@0xa07ec CreateWeapon / @0xa0954 Spawn / @0xa0b0c BounceTouch / @0xa0d84 Fall /
*    @0xa104c WaitThink / @0xa10a4 OnGround / @0xa1158 Use / @0xa1198 GetWeapon / @0xa1250 WaitTouch /
*    @0xa1290 ShouldCollide / @0xa3404 ObjectCaps / @0xa340c CanUsedTroughWalls)
*  — a player-dropped weapon. Derives CTSPhysicObject (slow-motion bounce physics), is MOVETYPE_BOUNCE,
*  carries the full s_weapon_status (our TSItem) of the dropped weapon, and hands it back on pickup
*  (USE, or on touch for the throwable type 0x1d). A thrown gun damages what it flies into (Fall's
*  swept trace). Its ShouldCollide decides what a move or trace that touches it hits (the global hook
*  asks the touched entity only, @0x9b7b4): guns and knives pass through it, everything else does not.
*
*  CTSAmmoPack (@0xa1334 CreateAmmoPack / @0xa13f4 Spawn / @0xa1544 Fall / @0xa15c0 WaitThink /
*    @0xa1610 OnGround / @0xa16c0 WaitTouch) — a dropped consumable pack carrying per-caliber rounds
*  AND an armor value; hands each to a TS-gun-holding toucher (ammo via CTSGun::AddAmmo, armor via
*  CBasePlayer::TakeArmor) and removes itself once empty. Base is CBaseEntity (no slow-mo physics).
***/
#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "player.h"
#include "weapons.h"
#include "ts_weapons.h"
#include "ts_utils.h"   // RemoveEntityFromSlowedArray (the pre-remove scrub)   // get_weapon_info, GiveTSWeapon
#include "ts_physicobject.h"
#include "ts_worldgun.h"
#include "ts_player.h"       // TSThrowSlow (the dropper's slow-motion rate)
#include "ts_gamerules.h"    // CTSGroundWeapon respawn rules
#include "ts_groundweapon.h"
#include "ts_groundammo.h"

//============================================================================
// CTSWorldGun — player-dropped weapon (MOVETYPE_BOUNCE, derives CTSPhysicObject).
//============================================================================
LINK_ENTITY_TO_CLASS(TSWorldGun, CTSWorldGun);

// @0xa07ec CreateWeapon — spawn a dropped weapon carrying the dropping gun's item state, tossed
// from `origin` along `velocity`. The original seeds velocity from the dropper's aim internally;
// our drop sites (throw / death-scatter) compute it and pass it in. classname = "WorldGun".
CTSWorldGun *CTSWorldGun::CreateWeapon(CBasePlayer *owner, int type, const TSItem &status)
{
	CTSWorldGun *pGun = GetClassPtr((CTSWorldGun *)NULL);
	if (!pGun)
	{
		// @0x907ec — UTIL_ClientPrintAll(3, "WARNING:cannot create weapon!\n")
		UTIL_ClientPrintAll(HUD_PRINTCONSOLE, "WARNING:cannot create weapon!\n");
		return NULL;
	}
	pGun->pev->classname = MAKE_STRING("WorldGun");
	pGun->m_weaponType = type;
	pGun->m_status = status;          // copy the full s_weapon_status (clip/mode/attach/barrels)
	pGun->Spawn();                    // sets NO origin/velocity — that's the caller's (ThrowWeapon) job

	if (!owner)
	{
		pGun->pev->owner = NULL;
		pGun->pev->fuser1 = 1.0f;     // gun+0x254 default (not slowed)
	}
	else
	{
		// @0x90881..0x908e3: the dropper becomes pev->owner and the gun takes his slow-motion rate
		// (GoSlow, vtable +0x14). No skin is set: the world model keeps skin 0.
		pGun->pev->owner = owner->edict();
		pGun->GoSlow(TSThrowSlow(owner), 0);
	}

	// The white glow shell, for the CTF flag (type 0x1d) only: both original binaries gate these
	// writes on it (ts_i386.so @0x90902, mp.dll @0x742a6). Every other dropped gun glows only for the
	// player who dropped it, through the entity state AddToFullPack sends him (m_pDropper).
	if (type == TSW_FLAG)
	{
		pGun->pev->renderamt = 5;
		pGun->pev->renderfx = kRenderFxGlowShell;
		pGun->pev->rendercolor = Vector(200, 200, 200);
	}
	return pGun;
}

// @0xa0954 Spawn — show the world model, become a bouncing physics object, fall toward the floor.
void CTSWorldGun::Spawn()
{
	CTSPhysicObject::Spawn();         // init the slow-motion physics base (saved vel/grav, fuser1=1)
	Precache();
	const s_weapon_info *wi = get_weapon_info((tsweapon_t)m_weaponType);
	SET_MODEL(ENT(pev), wi->pszWorldModel ? wi->pszWorldModel : "models/w_weaponbox.mdl");

	pev->movetype = MOVETYPE_BOUNCE;  // @+0x108 = 10 — bounces + rolls (NOT TOSS)
	pev->solid = SOLID_BBOX;
	pev->gravity = 0.7f;
	pev->friction = 0.8f;

	// Falls through players: a ZERO-size bbox while in flight (it still drops + lands FL_ONGROUND, but
	// the player walks through it — damage comes from Fall's swept trace, not collision). @0xa0954.
	UTIL_SetSize(pev, g_vecZero, g_vecZero);
	UTIL_SetOrigin(pev, pev->origin);

	// Lifetime: throwables (0x1d) live 30s; everything else lives weaponstay (clamped 5..60s). A
	// drop-key or disarm drop then doubles it (DropWeapon's TSMarkDropKeyGun, clamped 10..120s).
	if (m_weaponType == 0x1d)
		m_flExpire = gpGlobals->time + 30.0f;
	else
	{
		float stay = CVAR_GET_FLOAT("weaponstay");
		if (stay < 5.0f)  stay = 5.0f;
		if (stay > 60.0f) stay = 60.0f;
		m_flExpire = gpGlobals->time + stay;
	}

	SetTouch(&CTSWorldGun::BounceTouch);
	SetThink(&CTSWorldGun::Fall);
	pev->nextthink = gpGlobals->time + 0.1f;
	m_bDropSnd = true;
	m_flUseCooldown = gpGlobals->time + 0.5f; // dropper can't instantly re-grab his own toss
	m_bTaken = false;
}

// @0xa0b0c BounceTouch — MOVETYPE_BOUNCE contact handler: damp the fall, play the drop sound on a
// solid floor hit, and apply restitution so the gun rolls/settles.
void CTSWorldGun::BounceTouch(CBaseEntity *pOther)
{
	pev->velocity.z *= 0.3f; // damp the vertical bounce

	if (!(pev->flags & FL_ONGROUND))
	{
		// In the air: on a fast enough impact, play the drop sound once (debounced by m_bDropSnd).
		// VERIFY: the original also gates on the contact plane being roughly flat (trace.plane.z > 0.98),
		// which a touch handler doesn't expose cheaply — omitted; behaviour is the drop sound on impact.
		float speed = pev->velocity.Length();
		if (m_bDropSnd && speed > 18.0f)
		{
			EMIT_SOUND_DYN(ENT(pev), CHAN_BODY, "items/weapondrop1.wav", VOL_NORM, ATTN_NORM,
				0, 80 + RANDOM_LONG(0, 10) + 20); // pitch ~0.8 (+random), per @0xa0b0c
			m_bDropSnd = false;
		}
	}
	else
	{
		// On the ground: bleed horizontal momentum on EVERY contact so the gun settles near the
		// landing point (RE @0xa0b0c: horiz <= 40 -> x0.7, else -> x0.9; the SAME scalar applies to
		// BOTH velocity and avelocity). We previously damped only the <=40 case (no else) and set
		// avelocity = velocity, so a real throw (speed 60-180) hit the un-damped path and NEVER shed
		// horizontal speed -> slid forever (endless friction) and skated off floor edges into the void
		// (the steep-drop sink / near-edge disappear were downstream of this). Constants verified in
		// .rodata: 40.0@0xf6d9c, 0.7@0xf7f50, 0.9@0xf7f44. cont.91.
		// VERIFY (slow-mo only, LOW): the original threshold is 40 * GiveSlowMul(); plain 40 is exact
		// at normal time scale.
		float horiz = Vector(pev->velocity.x, pev->velocity.y, 0).Length();
		const float k = (horiz <= 40.0f) ? 0.7f : 0.9f;
		pev->velocity = pev->velocity * k;
		pev->avelocity = pev->avelocity * k;
		pev->angles.z = 0;
	}
}

// @0xa0d84 Fall — per-frame: once resting, settle (OnGround). While alive, sweep-trace ahead and
// damage anything we fly into (a thrown gun hurts). Past the expire deadline, remove. Runs the
// CTSPhysicObject slow-motion upkeep each frame.
void CTSWorldGun::Fall()
{
	pev->nextthink = gpGlobals->time + 0.1f;

	// @0x90de9: settles below GiveSlowMul() x 3.0 (.rodata 0xf7f78), so a slowed gun settles slower
	if ((pev->flags & FL_ONGROUND) && pev->velocity.Length() < GiveSlowMul() * 3.0f)
	{
		EMIT_SOUND_DYN(ENT(pev), CHAN_BODY, "items/weapondrop1.wav", VOL_NORM, 0.8f, 0, 100);
		pev->angles.x = 0;
		pev->angles.z = 0;
		OnGround();
		// no return: @0x90e4e falls through to the lifetime check, so the settling think still runs
		// the swept trace and the slow-motion upkeep below (or removes an expired gun)
	}

	if (gpGlobals->time <= m_flExpire)
	{
		// Swept impact: trace from here to where we'll be next frame; a fast enough hit damages.
		// VERIFY: the original gates on (speed * weapon_damage_byte@0xd0 >= victim->health) and deals
		// speed * damage_byte * 0.001; we use flDamage as that byte (exact byte/constants unconfirmed).
		Vector dir = pev->velocity * 0.1f;
		TraceResult tr;
		UTIL_TraceLine(pev->origin, pev->origin + dir, dont_ignore_monsters, ENT(pev), &tr);
		if (tr.flFraction < 1.0f && tr.pHit)
		{
			CBaseEntity *pHit = CBaseEntity::Instance(tr.pHit);
			const s_weapon_info *wi = get_weapon_info((tsweapon_t)m_weaponType);
			float speed = pev->velocity.Length();
			if (pHit && pHit->pev->takedamage != DAMAGE_NO &&
				speed * wi->flDamage >= pHit->pev->health)
			{
				ClearMultiDamage();
				Vector vdir = dir.Normalize();
				pHit->TraceAttack(pev, speed * wi->flDamage * 0.001f, vdir, &tr,
					DMG_CLUB | DMG_NEVERGIB); // VERIFY: orig damage-type bits 0x1080
				ApplyMultiDamage(pev, pev);
			}
		}
		CTSPhysicObject::WaitThink(); // slow-motion physics upkeep
	}
	else
	{
		RemoveEntityFromSlowedArray(this); // Fall__11CTSWorldGun @0x90e6a — scrub the packed slowed array before removal
		UTIL_Remove(this);
	}
}

// @0xa104c WaitThink — idle keep-alive after settling: remove once the lifetime expires.
void CTSWorldGun::WaitThink()
{
	pev->nextthink = gpGlobals->time + 0.1f;
	// RE @0x9104c: BOTH calls are gated by (m_flExpire < time) — the disasm `jae` skips both. The
	// source had dropped the braces, so UTIL_Remove(this) ran UNCONDITIONALLY every tick → every
	// dropped/thrown/death-scattered WorldGun was removed ~0.1s after it settled (should live
	// weaponstay 5-60s). This was masked while the endless-friction bug (BounceTouch) kept guns
	// sliding forever so they never handed off to WaitThink; with that fixed, settled guns hit this
	// path. The rapid create→settle→free edict churn under bot combat is also the prime suspect for
	// the rcbot FindEntityByString crash (cached pickup edict freed + slot-reused mid linear-scan).
	// cont.91 (findentity_crash.md #1).
	if (m_flExpire < gpGlobals->time)
	{
		RemoveEntityFromSlowedArray(this); // scrub the packed slowed array before removal
		UTIL_Remove(this);
	}
}

// @0xa10a4 OnGround — settled: become a USE trigger with the real grab box. Stays MOVETYPE_BOUNCE.
void CTSWorldGun::OnGround()
{
	pev->owner = NULL;
	pev->solid = SOLID_TRIGGER;
	UTIL_SetOrigin(pev, pev->origin);
	UTIL_SetSize(pev, Vector(-4, -4, 0), Vector(4, 4, 4)); // the real grab box
	SetTouch(&CTSWorldGun::WaitTouch);
	SetThink(&CTSWorldGun::WaitThink);
	pev->nextthink = gpGlobals->time + 0.1f;
}

// @0xa1158 Use — USE pickup, cooldown-gated so the dropper can't instantly re-grab his own toss.
void CTSWorldGun::Use(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value)
{
	if (m_flUseCooldown < gpGlobals->time)
		GetWeapon(pActivator);
}

// @0xa1198 GetWeapon — hand the dropped weapon (with its full carried status) to the toucher, then
// remove the world gun. The original calls CBasePlayer::PickUpWeapon(type, &m_status, …); we restore
// the carried item into the picker's arsenal (clip/mode/attach/barrels) the same way.
void CTSWorldGun::GetWeapon(CBaseEntity *pOther)
{
	if (m_bTaken || !pOther || !pOther->IsPlayer() || !pOther->IsAlive())
		return;
	if ((pev->effects & EF_NODRAW) || (pOther->pev->effects & EF_NODRAW))
		return;

	CBasePlayer *pPlayer = (CBasePlayer *)pOther;
	// @0x91224: PickUpWeapon(type, &m_status, 0, type != FLAG (the briefcase never auto-wields),
	// no owner record). A refusal (full slots / owned duplicate) leaves the gun in the world.
	if (!pPlayer->PickUpWeapon(m_weaponType, &m_status, 0, m_weaponType != TSW_FLAG, false))
		return;

	m_bTaken = true;
	RemoveEntityFromSlowedArray(this); // GetWeapon__11CTSWorldGun @0x9123b — scrub the packed slowed array before removal
	UTIL_Remove(this);
}

// @0xa1250 WaitTouch — throwables (type 0x1d) auto-pick on touch; everything else needs USE held.
void CTSWorldGun::WaitTouch(CBaseEntity *pOther)
{
	if (m_weaponType == 0x1d || (pOther && (pOther->pev->button & IN_USE)))
		GetWeapon(pOther);
}

// @0xa1290 ShouldCollide — a dropped gun collides with everything except other dropped guns and
// thrown knives ("WorldGun", "knife"). Dispatched by the global ShouldCollide hook when the gun is the
// touched entity. The original reads pOther->pev without a check, so a trace with no skip entity
// that reaches a flying gun crashes it; here NULL (no entity, e.g. the world's trace) collides.
bool CTSWorldGun::ShouldCollide(CBaseEntity *pOther)
{
	if (!pOther)
		return true;
	return !FClassnameIs(pOther->pev, "WorldGun") && !FClassnameIs(pOther->pev, "knife");
}

//============================================================================
// CTSAmmoPack — dropped consumable ammo+armor pack (MOVETYPE_TOSS, base CBaseEntity).
//============================================================================
LINK_ENTITY_TO_CLASS(TSAmmoPack, CTSAmmoPack);

// @0xa1334 CreateAmmoPack — spawn a pack carrying `counts[16]` rounds + `armor` armour from
// `pevOwner`, tossed from `origin` along `velocity`. classname = "AmmoPack". NULL if nothing to drop.
CTSAmmoPack *CTSAmmoPack::CreateAmmoPack(entvars_t *pevOwner, const int *counts, int armor,
	const Vector &origin, const Vector &velocity)
{
	bool any = (armor > 0);
	for (int c = 0; c < TS_NUM_CALIBERS && !any; c++)
		if (counts && counts[c] > 0) any = true;
	if (!any)
		return NULL;

	CTSAmmoPack *pPack = GetClassPtr((CTSAmmoPack *)NULL);
	if (!pPack)
	{
		UTIL_ClientPrintAll(HUD_PRINTCONSOLE, "WARNING:cannot create ammopack!\n");
		return NULL;
	}
	pPack->pev->classname = MAKE_STRING("AmmoPack");
	for (int c = 0; c < TS_NUM_CALIBERS; c++)
		pPack->m_ammo[c] = counts ? counts[c] : 0;
	UTIL_SetOrigin(pPack->pev, origin);
	pPack->Spawn();
	pPack->pev->velocity = velocity;
	pPack->pev->armorvalue = (float)armor;        // carried armor
	pPack->pev->owner = ENT(pevOwner);
	return pPack;
}

// @0xa13f4 Spawn — MOVETYPE_TOSS + SOLID_BBOX, zero-size while falling; settle then become a trigger.
void CTSAmmoPack::Spawn()
{
	Precache();
	SET_MODEL(ENT(pev), "models/ammopack.mdl");
	pev->movetype = MOVETYPE_TOSS;
	pev->solid = SOLID_BBOX;
	UTIL_SetSize(pev, g_vecZero, g_vecZero);      // point bbox while falling
	UTIL_SetOrigin(pev, pev->origin);

	float stay = CVAR_GET_FLOAT("weaponstay");
	if (stay < 5.0f)  stay = 5.0f;
	if (stay > 60.0f) stay = 60.0f;
	m_flExpire = gpGlobals->time + stay;

	SetThink(&CTSAmmoPack::Fall);
	pev->nextthink = gpGlobals->time + 0.1f;
	pev->armorvalue = 0;
	pev->owner = NULL;
}

// @0xa1544 Fall — settle on the ground (OnGround), or remove past the lifetime.
void CTSAmmoPack::Fall()
{
	pev->nextthink = gpGlobals->time + 0.1f;
	if (pev->flags & FL_ONGROUND)
	{
		pev->angles.x = 0;
		pev->angles.z = 0;
		OnGround();
		return;
	}
	if (m_flExpire < gpGlobals->time)
		UTIL_Remove(this);
}

// @0xa15c0 WaitThink — idle keep-alive after settling: remove once the lifetime expires.
void CTSAmmoPack::WaitThink()
{
	pev->nextthink = gpGlobals->time + 0.1f;
	if (m_flExpire < gpGlobals->time)
		UTIL_Remove(this);
}

// @0xa1610 OnGround — settled: become a pickup trigger with the real box.
void CTSAmmoPack::OnGround()
{
	pev->owner = NULL;
	pev->solid = SOLID_TRIGGER;
	UTIL_SetOrigin(pev, pev->origin);
	UTIL_SetSize(pev, Vector(-16, -16, 0), Vector(16, 16, 16));
	SetTouch(&CTSAmmoPack::WaitTouch);
	SetThink(&CTSAmmoPack::WaitThink);
	pev->nextthink = gpGlobals->time + 0.1f;
}

// @0xa16c0 WaitTouch — give each carried caliber to a TS-gun-holding player (via CTSGun::AddAmmo),
// then any carried armor (via CBasePlayer::TakeArmor); remove once BOTH ammo and armor are consumed.
void CTSAmmoPack::WaitTouch(CBaseEntity *pOther)
{
	if (!pOther || !pOther->IsPlayer() || !pOther->IsAlive())
		return;
	CBasePlayer *pPlayer = (CBasePlayer *)pOther;
	CTSGun *pGun = GetPlayerTSGun(pPlayer); // the toucher must hold a CTSGun (orig: m_pActiveItem type@0x88==0x10)
	if (!pGun)
		return;

	bool gaveAny = false;
	for (int c = 0; c < TS_NUM_CALIBERS; c++)
	{
		if (m_ammo[c] <= 0)
			continue;
		if (pGun->AddAmmo(c, m_ammo[c]))
		{
			m_ammo[c] = 0; // consumed from the pack
			gaveAny = true;
		}
	}
	if (gaveAny)
		EMIT_SOUND_DYN(ENT(pPlayer->pev), CHAN_ITEM, "items/9mmclip1.wav", VOL_NORM, 0.8f, 0, 100);

	// Armor: give when USE is held or the player still has some armor capacity, then zero it.
	if (pev->armorvalue > 0 && ((pPlayer->pev->button & IN_USE) || pPlayer->pev->armorvalue != 0))
	{
		pPlayer->TakeArmor((int)(pev->armorvalue + 0.5f));
		pev->armorvalue = 0;
	}

	if (pev->armorvalue == 0) // fully consumed (armor gone — matches the original's removal gate)
	{
		SetTouch(NULL);
		UTIL_Remove(this);
	}
}

//============================================================================
// Global ShouldCollide hook (NEW_DLL_FUNCTIONS::pfnShouldCollide, @0x9b7b4): the touched entity's own
// ShouldCollide virtual decides, unless it is being removed (FL_KILLME); either edict may be NULL (the
// engine passes a trace's NULL passedict), and then it collides.
//============================================================================
extern "C" int ShouldCollide(edict_t *pentTouched, edict_t *pentOther)
{
	CBaseEntity *pTouched = pentTouched ? CBaseEntity::Instance(pentTouched) : NULL;
	CBaseEntity *pOther = pentOther ? CBaseEntity::Instance(pentOther) : NULL;
	if (pTouched && !(pTouched->pev->flags & FL_KILLME))
		return pTouched->ShouldCollide(pOther) ? 1 : 0;
	return 1;
}

// ===================== CTSGroundWeapon (merged from ts_groundweapon.cpp) =====================

#define SF_GW_ON_WALL 16 // "Lie on wall"

LINK_ENTITY_TO_CLASS(ts_groundweapon, CTSGroundWeapon);

TYPEDESCRIPTION CTSGroundWeapon::m_SaveData[] = {
	DEFINE_FIELD(CTSGroundWeapon, respawntime, FIELD_INTEGER),
	DEFINE_FIELD(CTSGroundWeapon, tsweaponid, FIELD_INTEGER),
	DEFINE_FIELD(CTSGroundWeapon, extraclip, FIELD_INTEGER),
};
IMPLEMENT_SAVERESTORE(CTSGroundWeapon, CBaseEntity);

bool CTSGroundWeapon::KeyValue(KeyValueData *pkvd)
{
	if (FStrEq(pkvd->szKeyName, "tsweaponid"))   { tsweaponid = atoi(pkvd->szValue); return true; }
	if (FStrEq(pkvd->szKeyName, "wduration"))    { respawntime = atoi(pkvd->szValue); return true; }
	if (FStrEq(pkvd->szKeyName, "wextraclip"))   { extraclip = atoi(pkvd->szValue); return true; }
	if (FStrEq(pkvd->szKeyName, "attackerteam")) { attackerteam = atoi(pkvd->szValue); return true; }
	if (FStrEq(pkvd->szKeyName, "buyable"))      { buyable = atoi(pkvd->szValue) != 0; return true; }
	return CBaseEntity::KeyValue(pkvd);
}

// NB the original's Precache is EMPTY (the weapon models are precached by the weapon-table at game
// init). We have no global TS-weapon precache pass, so precache the roster here so SET_MODEL below
// (and a runtime hand-out) is legal.
void CTSGroundWeapon::Precache()
{
	PrecacheAllTSWeapons();
}

void CTSGroundWeapon::Spawn()
{
	Precache();

	const s_weapon_info *wi = get_weapon_info((tsweapon_t)tsweaponid);
	if (!wi || !wi->pszName)
	{
		// unfilled weapon row -> self-delete (orig: SetThink(SUB_Remove) + nextthink 0.1).
		SetThink(&CTSGroundWeapon::SUB_Remove);
		pev->nextthink = gpGlobals->time + 0.1f;
		return;
	}
	// akimbo single -> upgrade target when the row marks it (wi+0xcc != -1 && firetype 4).
	if (wi->iSingleType != -1 && wi->iFireType == 4)
		tsweaponid = wi->iSingleType;

	const char *world = wi->pszWorldModel ? wi->pszWorldModel : "models/w_weaponbox.mdl";
	PRECACHE_MODEL((char *)world);
	SET_MODEL(ENT(pev), world);

	pev->movetype = (pev->spawnflags & SF_GW_ON_WALL) ? MOVETYPE_FLY : MOVETYPE_TOSS;
	pev->solid = SOLID_BBOX;
	UTIL_SetOrigin(pev, pev->origin);
	UTIL_SetSize(pev, g_vecZero, g_vecZero); // point bbox while falling (players pass through)

	SetTouch(NULL);
	SetThink(&CTSGroundWeapon::Fall);
	pev->nextthink = gpGlobals->time + 0.1f;
	pev->iuser4 = 0;
	pev->iuser4 |= 2;
	m_bPlaced = false;
}

// 8-ray 45° circle out of the origin; pin to the nearest wall and lie flat (spawnflag 0x10).
void CTSGroundWeapon::PutOnWall()
{
	TraceResult best;
	best.flFraction = 1.0f;
	Vector bestDir = g_vecZero;
	bool found = false;
	for (int i = 0; i < 8; i++)
	{
		float ang = (360.0f / 8.0f) * i;
		Vector dir(cos(ang * M_PI / 180.0f), sin(ang * M_PI / 180.0f), 0);
		TraceResult tr;
		UTIL_TraceLine(pev->origin, pev->origin + dir * 1024.0f, ignore_monsters, ENT(pev), &tr);
		if (tr.flFraction < 1.0f && tr.flFraction < best.flFraction)
		{
			best = tr;
			bestDir = dir;
			found = true;
		}
	}
	if (found)
	{
		UTIL_SetOrigin(pev, best.vecEndPos + best.vecPlaneNormal * 2.0f);
		pev->angles = UTIL_VecToAngles(bestDir);
		pev->angles.x += 90.0f; // lay flat against the wall
	}
	else
	{
		pev->angles.z = 90.0f; // no wall -> on its side
	}
}

// think (+0.5s): wait for the floor (or the wall), play the drop sound, flatten, settle.
void CTSGroundWeapon::Fall()
{
	pev->nextthink = gpGlobals->time + 0.5f;

	if (pev->spawnflags & SF_GW_ON_WALL)
	{
		if (m_bPlaced)
			return;
		PutOnWall();
	}
	else
	{
		if (!(pev->flags & FL_ONGROUND) && !m_bPlaced)
			return; // keep falling
		pev->angles.x = 0;
		pev->angles.z = 0;
	}

	EMIT_SOUND_DYN(ENT(pev), CHAN_BODY, "items/weapondrop1.wav", VOL_NORM, 0.8f, 0, 100);
	OnGround();
	m_bPlaced = true;
}

void CTSGroundWeapon::OnGround()
{
	pev->solid = SOLID_TRIGGER;
	pev->effects &= ~EF_NODRAW;
	UTIL_SetOrigin(pev, pev->origin);
	UTIL_SetSize(pev, Vector(-4, -4, -4), Vector(4, 4, 4)); // the real grab box
	SetTouch(&CTSGroundWeapon::WaitTouch);
	SetThink(NULL);
}

void CTSGroundWeapon::WaitThink()
{
	pev->nextthink = gpGlobals->time + 1.0f;
}

// USE = pick up (dispatches to the subclass's virtual GetWeapon).
void CTSGroundWeapon::Use(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value)
{
	GetWeapon(pActivator);
}

// touch: only the 0x1d (throwable) type auto-picks on touch; everything else must be USE'd.
void CTSGroundWeapon::WaitTouch(CBaseEntity *pOther)
{
	if (tsweaponid == 0x1d)
		GetWeapon(pOther);
}

// Hand the weapon to the toucher and schedule the respawn (map pickups persist).
void CTSGroundWeapon::GetWeapon(CBaseEntity *pOther)
{
	if (!pOther || !pOther->IsPlayer() || !pOther->IsAlive())
		return;
	if ((pev->effects & EF_NODRAW) || (pOther->pev->effects & EF_NODRAW))
		return;

	if (extraclip < 0)
		extraclip = 0;

	CBasePlayer *pPlayer = (CBasePlayer *)pOther;

	// Buy gate (the original's, gated on CashBased()). CashBased() is hard-OFF in the shipped game so
	// this is DEAD — but present for 1:1 fidelity, and it's what consumes `buyable`/`attackerteam`/
	// `extraclip`. In a round mode (2/3) only `attackerteam` may buy; then the weapon costs cash
	// (base price + per-attachment surcharge + a magazine's price per extra clip, @0x9242c), taken
	// once the pickup succeeds (@0x92518).
	int price = 0;
	if (CashBased() && buyable)
	{
		CTSGameRules *gr = TSGameRules;
		if (gr && (unsigned char)(gr->m_iGameMode - 2) < 2 && attackerteam != -1 &&
			attackerteam != gr->GetTeamIndex(pPlayer->TeamID()))
		{
			TSHudMessageOne(0, 0, 0, 16, 30, "Your team cannot buy this weapon.", pPlayer->pev);
			return;
		}
		const s_weapon_info *bwi = get_weapon_info((tsweapon_t)tsweaponid);
		price = bwi->iPrice;
		if (pev->spawnflags & 1) price += 900;  // silencer
		if (pev->spawnflags & 2) price += 800;  // lasersight
		if (pev->spawnflags & 4) price += 500;  // flashlight
		if (pev->spawnflags & 8) price += 1800; // scope
		price += get_ammo_info(bwi->iCaliber)->iPrice * extraclip;
		if (pPlayer->m_iCash < price)
		{
			TSHudMessageOne(0, 0, 0, 16, 30, "You don't have enough money for that weapon.", pPlayer->pev);
			return;
		}
	}

	// @0x924ff: PickUpWeapon(type, NULL, spawnflags, type != FLAG, no owner record) — the pickup
	// sound + speed recompute live inside. The original passes the WHOLE spawnflags as the attach
	// mask (silencer 1 / lasersight 2 / flashlight 4 / scope 8 are the priced bits; TakeWeapon
	// clamps the new item to what the gun can mount).
	if (!pPlayer->PickUpWeapon(tsweaponid, NULL, pev->spawnflags, tsweaponid != TSW_FLAG, false))
		return; // won't fit -> leave it on the floor
	pPlayer->GiveCash(-price);

	// @0x9251d respawn schedule: <1 -> 60s, <2 -> 1s, else min(respawntime,180) — except that a
	// weapon with no wduration (< 1) does not come back in a round mode (2/3) (@0x9253a).
	SetTouch(NULL);
	pev->effects |= EF_NODRAW;
	pev->solid = SOLID_NOT;
	m_bPlaced = false;
	if (respawntime < 1 && TSGameRules && (unsigned char)(TSGameRules->m_iGameMode - 2) < 2)
	{
		SetThink(NULL);
	}
	else
	{
		float delay = (respawntime < 1) ? 60.0f : (respawntime < 2) ? 1.0f
			: (respawntime < 180 ? (float)respawntime : 180.0f);
		SetThink(&CTSGroundWeapon::Fall);
		pev->nextthink = gpGlobals->time + delay;
	}

	// @0x925c4: the entity's extra clips (wextraclip, in rounds) go to an EMPTY caliber reserve,
	// capped by AddAmmo; a non-empty reserve, or no extra clips, gets nothing.
	if (extraclip != 0)
	{
		const s_weapon_info *gwi = get_weapon_info((tsweapon_t)tsweaponid);
		CTSGun *pGun = GetPlayerTSGun(pPlayer);
		if (pGun && gwi->iCaliber >= 0 && gwi->iCaliber < TS_NUM_CALIBERS &&
			pGun->m_ammoReserve[gwi->iCaliber] < 1 && pGun->AddAmmo(gwi->iCaliber, extraclip))
			EMIT_SOUND_DYN(ENT(pPlayer->pev), CHAN_ITEM, "items/9mmclip1.wav", VOL_NORM, 0.8f, 0, PITCH_NORM);
	}

	// @0x92689: the map's pickup text goes out as a TSMessage (effect 8, hold 30).
	if (!FStringNull(pev->message))
		TSHudMessageOne(0, 0, 0, 8, 30, STRING(pev->message), pPlayer->pev);
	if (pev->target)
		FireTargets(STRING(pev->target), this, this, USE_TOGGLE, 0);
}

// ===================== CTSGroundAmmo (merged from ts_groundammo.cpp) =====================

#define AMMOPACK_MODEL "models/ammopack.mdl"
#define SF_AMMO_ON_WALL 16

LINK_ENTITY_TO_CLASS(ts_groundammo, CTSGroundAmmo);

bool CTSGroundAmmo::KeyValue(KeyValueData *pkvd)
{
	if (FStrEq(pkvd->szKeyName, "wduration"))
	{
		respawntime = atoi(pkvd->szValue);
		return true;
	}
	// magsN -> m_ammo[N] (caliber N), stored as ROUNDS = magcount * roundsPerMag(N) (orig @0x68 array).
	if (strncmp(pkvd->szKeyName, "mags", 4) == 0 && pkvd->szKeyName[4] != '\0')
	{
		int cal = atoi(pkvd->szKeyName + 4);
		if (cal >= 0 && cal < TS_AMMO_CALIBERS)
		{
			m_ammo[cal] = atoi(pkvd->szValue) * TSRoundsPerMag(cal);
			return true;
		}
	}
	return CBaseEntity::KeyValue(pkvd);
}

void CTSGroundAmmo::Precache()
{
	PRECACHE_MODEL(AMMOPACK_MODEL);
	PRECACHE_SOUND("items/9mmclip1.wav");
	PRECACHE_SOUND("items/weapondrop1.wav");
}

void CTSGroundAmmo::Spawn()
{
	Precache();
	pev->classname = MAKE_STRING("ts_groundammo");
	SET_MODEL(ENT(pev), AMMOPACK_MODEL);

	// Wall-mounted packs hang in place (FLY); floor packs fall (TOSS).
	pev->movetype = (pev->spawnflags & SF_AMMO_ON_WALL) ? MOVETYPE_FLY : MOVETYPE_TOSS;
	pev->solid = SOLID_BBOX;

	UTIL_SetOrigin(pev, pev->origin);
	UTIL_SetSize(pev, Vector(-8, -8, -8), Vector(8, 8, 8));

	SetTouch(NULL);
	SetThink(&CTSGroundAmmo::Fall);
	pev->nextthink = gpGlobals->time + 0.1f;
}

// @ Fall — settle to the floor (or pin to the wall), then become a touch-pickup.
void CTSGroundAmmo::Fall()
{
	pev->nextthink = gpGlobals->time + 0.5f;

	if (pev->spawnflags & SF_AMMO_ON_WALL)
	{
		if (!m_bPlaced)
		{
			PutOnWall();
			EMIT_SOUND(ENT(pev), CHAN_BODY, "items/weapondrop1.wav", VOL_NORM, ATTN_NORM);
		}
		return;
	}

	if (pev->flags & FL_ONGROUND)
	{
		OnGround();
		EMIT_SOUND(ENT(pev), CHAN_BODY, "items/weapondrop1.wav", VOL_NORM, ATTN_NORM);
	}
}

// @ OnGround — landed: become a SOLID_TRIGGER refill station (real tail via objdump:
// SetTouch(WaitTouch) + SetThink(WaitThink)).
void CTSGroundAmmo::OnGround()
{
	pev->movetype = MOVETYPE_FLY; // stop falling
	pev->solid = SOLID_TRIGGER;
	pev->effects &= ~EF_NODRAW;
	pev->effects |= EF_DIMLIGHT;
	m_bPlaced = true;

	UTIL_SetOrigin(pev, pev->origin);
	UTIL_SetSize(pev, Vector(-16, -16, -16), Vector(16, 16, 16));

	SetTouch(&CTSGroundAmmo::WaitTouch);
	SetThink(&CTSGroundAmmo::WaitThink);
	pev->nextthink = gpGlobals->time + 1.0f;
}

// @ PutOnWall — SLICE: the wall-mounted variant just settles in place.
void CTSGroundAmmo::PutOnWall()
{
	OnGround();
}

// @ WaitThink — idle (keeps the entity ticking).
void CTSGroundAmmo::WaitThink()
{
	pev->nextthink = gpGlobals->time + 1.0f;
}

// @ WaitTouch — refill a touching player who holds a TS gun.
void CTSGroundAmmo::WaitTouch(CBaseEntity *pOther)
{
	if (pev->effects & EF_NODRAW)
		return; // hidden

	if (!pOther->IsPlayer() || !pOther->IsAlive())
		return;

	CBasePlayer *pPlayer = (CBasePlayer *)pOther;
	if (!pPlayer->HasNamedPlayerItem("weapon_tsgun"))
		return; // must be holding a TS weapon (faithful: m_pActiveItem id == 16)

	// Faithful (@WaitTouch): add each caliber's ROUNDS (m_ammo[caliber], the @0x68 array) to the
	// TS gun's per-caliber reserve via TSAddAmmo. Persistent station: tops players up on every touch.
	bool added = false;
	for (int cal = 0; cal < TS_AMMO_CALIBERS; cal++)
	{
		if (m_ammo[cal] > 0 && TSAddAmmo(pPlayer, cal, m_ammo[cal]))
			added = true;
	}

	if (added)
		EMIT_SOUND(ENT(pPlayer->pev), CHAN_ITEM, "items/9mmclip1.wav", VOL_NORM, ATTN_NORM);
}
