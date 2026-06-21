/***
*  ts_mapglobals.cpp — RECONSTRUCTED CTSMapGlobals from ts_i386.so + ts_fgd.fgd.
*  Functional/source-faithful (see CLAUDE.md). recovered/server/CTSMapGlobals.c.
*
*  @PointClass = ts_mapglobals : "TS map globals"
*    spawnflags: 1 No Knives, 2 No Pistols, 4 No SMGs, 8 No Rifles,
*                16 No Shotguns, 32 No Round Timer, 64 Superjump
*
*  An invisible per-map config entity. It carries no keyvalues of its own — its
*  whole configuration is the spawnflags, which the game rules read back via
*  get_weapon_restriction(). Appears in 9 of the 60 stock TS maps.
***/
#include "extdll.h"
#include "util.h"
#include "cbase.h"

class CTSMapGlobals : public CBaseEntity
{
public:
	void Spawn(void) override;
	void Precache(void) override;
	bool KeyValue(KeyValueData *pkvd) override;
	void EXPORT WaitThink(void);
	int get_weapon_restriction(void);
};

LINK_ENTITY_TO_CLASS(ts_mapglobals, CTSMapGlobals);

bool CTSMapGlobals::KeyValue(KeyValueData *pkvd)
{
	// Original marks every key unhandled — the entity has no keyvalues beyond the
	// engine-applied spawnflags/origin/angles.
	return false;
}

void CTSMapGlobals::Precache(void)
{
}

void CTSMapGlobals::Spawn(void)
{
	Precache();
	pev->movetype = MOVETYPE_NONE; // 0x108 = 0
	pev->solid = SOLID_NOT;        // 0x10c = 0
	UTIL_SetOrigin(pev, pev->origin);
}

// Bitmask of permitted weapon types: 0x1f (all five) XOR the "No <weapon>"
// spawnflags. (The original's InitWeaponRestriction() simply returns 0x1f.)
// Consumed by the game rules — kept here for faithfulness; nothing calls it
// until CTSGameRules is reconstructed.
int CTSMapGlobals::get_weapon_restriction(void)
{
	return 0x1f ^ pev->spawnflags;
}

// Present in the original but never scheduled by Spawn (an inert 10s
// self-reschedule); kept for completeness.
void CTSMapGlobals::WaitThink(void)
{
	pev->nextthink = gpGlobals->time + 10.0f;
}
