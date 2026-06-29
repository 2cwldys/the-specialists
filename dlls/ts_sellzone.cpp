/***
*  ts_sellzone.cpp — RECONSTRUCTED CTSSellZone (cont.41), the weapon sell zone.
*  Functional/source-faithful (see CLAUDE.md). recovered/sellzone/CTSSellZone.c:
*    KeyValue @0x726c0 (attackerteam), Spawn @0x72720, SellTouch @0x727e8.
*
*  CTSSellZone is a map-placed (`@SolidClass`, brush, base tsobjective) zone that sells dropped
*  weapons (a touching CTSWorldGun) for cash. In the shipped binary SellTouch is a ~61-byte
*  near-stub (just the "WorldGun" classname check) — consistent with cont.36's finding that
*  CashBased() is OFF, which makes the sell economy vestigial. FGD-backed (ts_dust2 places one).
*
*  FUNCTIONAL reconstruction: a brush trigger that consumes a touching dropped weapon
*  (ts_worldgun) and credits the nearest living player its sale value (the cont.36 cash) — the
*  one place cash actually flows. // SLICE: flat sale value (the per-weapon price s_weapon_info+
*  0xd8 isn't extracted, and is dead while CashBased is off); the dropped gun has no recorded
*  seller (no pev->owner), so the nearest player in the zone is credited.
***/
#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "player.h"
#include "gamerules.h"
#include "ts_gamerules.h"

#define SELLZONE_VALUE 500   // flat cash per weapon sold (price field is dead while CashBased off)

class CTSSellZone : public CBaseEntity
{
public:
	bool KeyValue(KeyValueData *pkvd) override;
	void Spawn() override;
	void EXPORT SellTouch(CBaseEntity *pOther);

	int m_attackerTeam = -1; // tsobjective attackerteam (-1 = any)
};

LINK_ENTITY_TO_CLASS(ts_sellzone, CTSSellZone)

bool CTSSellZone::KeyValue(KeyValueData *pkvd)
{
	if (FStrEq(pkvd->szKeyName, "attackerteam"))
	{
		m_attackerTeam = atoi(pkvd->szValue);
		return true;
	}
	return CBaseEntity::KeyValue(pkvd);
}

// @ Spawn — a brush trigger that watches for dropped weapons sliding in.
void CTSSellZone::Spawn()
{
	pev->solid = SOLID_TRIGGER;
	pev->movetype = MOVETYPE_NONE;
	if (!FStringNull(pev->model))
		SET_MODEL(edict(), STRING(pev->model)); // brush model
	UTIL_SetOrigin(pev, pev->origin);
	SetTouch(&CTSSellZone::SellTouch);
}

// @ SellTouch — a dropped weapon (classname "WorldGun") entered the zone: sell it. Credit the nearest
// living player (on the zone's team, if set) its value, then consume the gun.
void CTSSellZone::SellTouch(CBaseEntity *pOther)
{
	if (!pOther || !FClassnameIs(pOther->pev, "WorldGun"))
		return;

	// find the nearest living player to the sold gun (the seller stand-in)
	CBasePlayer *pBest = nullptr;
	float flBest = 256.0f * 256.0f;
	for (int i = 1; i <= gpGlobals->maxClients; i++)
	{
		CBaseEntity *pEnt = UTIL_PlayerByIndex(i);
		if (!pEnt || !pEnt->IsAlive())
			continue;
		if (m_attackerTeam != -1 && TSGameRules()->GetTeamIndex(pEnt->TeamID()) != m_attackerTeam)
			continue;
		float d = (pEnt->pev->origin - pOther->pev->origin).LengthSquared();
		if (d < flBest)
		{
			flBest = d;
			pBest = (CBasePlayer *)pEnt;
		}
	}

	if (pBest)
	{
		pBest->GiveCash(SELLZONE_VALUE);
		ClientPrint(pBest->pev, HUD_PRINTCENTER,
			UTIL_VarArgs("Sold weapon (+$%d)", SELLZONE_VALUE));
	}

	UTIL_Remove(pOther); // the gun is consumed by the sale
}

