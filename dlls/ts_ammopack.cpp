/***
*  ts_ammopack.cpp — RECONSTRUCTED CTSAmmoPack (a dropped, consumable ammo pack).
*  Functional/source-faithful (see CLAUDE.md). Recovered from ts_i386.so:
*  CTSAmmoPack (CreateAmmoPack @0xa1334 / Spawn @0xa13f4 / WaitTouch @0xa16c0).
*
*  Unlike the map-placed CTSGroundAmmo refill STATION (cont.17, persistent), this is a
*  CONSUMABLE pack carrying per-caliber round counts that drop from a dead player (or are
*  thrown). WaitTouch hands each caliber to a TS-gun-holding toucher via CTSGun::AddAmmo and
*  ZEROES the given entries (matches the .so); when emptied the pack removes itself.
*  The faithful base is CTSPhysicObject — FUNCTIONAL reconstruction on the stock SDK
*  (MOVETYPE_TOSS), like CTSWorldGun/CM61Grenade.
***/
#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "player.h"
#include "weapons.h"
#include "ts_weapons.h" // TSAddAmmo, TS_NUM_CALIBERS

#define AMMOPACK_MODEL "models/ammopack.mdl"

class CTSAmmoPack : public CBaseEntity
{
public:
	void Spawn() override;
	void Precache() override;
	void EXPORT FallThink();
	void EXPORT WaitTouch(CBaseEntity *pOther);
	int ObjectCaps() override { return CBaseEntity::ObjectCaps() & ~FCAP_ACROSS_TRANSITION; }

	int m_ammo[TS_NUM_CALIBERS] = {}; // per-caliber rounds carried (consumed on pickup)
};

LINK_ENTITY_TO_CLASS(ts_ammopack, CTSAmmoPack);

void CTSAmmoPack::Precache()
{
	PRECACHE_MODEL(AMMOPACK_MODEL);
}

// @Spawn__11CTSAmmoPack — MOVETYPE_TOSS (@+0x108=6) + SOLID_BBOX (@+0x10c=2); falls/settles.
void CTSAmmoPack::Spawn()
{
	Precache();
	SET_MODEL(ENT(pev), AMMOPACK_MODEL);
	pev->movetype = MOVETYPE_TOSS;
	pev->solid = SOLID_BBOX;
	UTIL_SetSize(pev, Vector(-12, -12, -4), Vector(12, 12, 4));
	UTIL_SetOrigin(pev, pev->origin);

	SetThink(&CTSAmmoPack::FallThink);
	pev->nextthink = gpGlobals->time + 0.1f;
}

// Settle on the ground, then become a pickup trigger.
void CTSAmmoPack::FallThink()
{
	pev->nextthink = gpGlobals->time + 0.1f;
	if (!(pev->flags & FL_ONGROUND))
		return;

	pev->movetype = MOVETYPE_FLY;
	pev->velocity = g_vecZero;
	pev->solid = SOLID_TRIGGER;
	UTIL_SetSize(pev, Vector(-16, -16, -4), Vector(16, 16, 4));
	SetThink(nullptr);
	SetTouch(&CTSAmmoPack::WaitTouch);
}

// @WaitTouch — give each carried caliber to a TS-gun-holding player, zeroing what was taken;
// remove the pack once it is empty.
void CTSAmmoPack::WaitTouch(CBaseEntity *pOther)
{
	if (!pOther->IsPlayer() || !pOther->IsAlive())
		return;
	CBasePlayer *pPlayer = (CBasePlayer *)pOther;
	if (!pPlayer->HasNamedPlayerItem("weapon_tsgun"))
		return;

	bool tookAny = false, anyLeft = false;
	for (int c = 0; c < TS_NUM_CALIBERS; c++)
	{
		if (m_ammo[c] <= 0)
			continue;
		if (TSAddAmmo(pPlayer, c, m_ammo[c]))
		{
			m_ammo[c] = 0; // consumed from the pack
			tookAny = true;
		}
		else
			anyLeft = true; // player was full on this caliber
	}

	if (tookAny)
		EMIT_SOUND(ENT(pPlayer->pev), CHAN_ITEM, "items/9mmclip1.wav", VOL_NORM, ATTN_NORM);
	if (!anyLeft) // fully consumed
	{
		SetTouch(nullptr);
		UTIL_Remove(this);
	}
}

// Spawn a dropped ammo pack carrying `counts[16]` (the drop side of CTSAmmoPack::CreateAmmoPack).
// Returns NULL if there was no ammo to drop.
CBaseEntity *TSCreateAmmoPack(entvars_t *pevOwner, const int *counts, const Vector &origin,
	const Vector &velocity)
{
	bool any = false;
	for (int c = 0; c < TS_NUM_CALIBERS; c++)
		if (counts[c] > 0) { any = true; break; }
	if (!any)
		return NULL;

	CTSAmmoPack *pPack = GetClassPtr((CTSAmmoPack *)NULL);
	pPack->pev->classname = MAKE_STRING("ts_ammopack");
	for (int c = 0; c < TS_NUM_CALIBERS; c++)
		pPack->m_ammo[c] = counts[c];
	UTIL_SetOrigin(pPack->pev, origin);
	pPack->Spawn();
	pPack->pev->velocity = velocity;
	pPack->pev->owner = ENT(pevOwner);
	return pPack;
}
