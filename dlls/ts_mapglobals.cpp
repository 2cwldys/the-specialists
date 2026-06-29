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
#include "ts_mapglobals.h"

LINK_ENTITY_TO_CLASS(ts_mapglobals, CTSMapGlobals);

// @0x62488/0x624d8 — standard save/restore of the two vestigial fields (WriteFields count 2).
TYPEDESCRIPTION CTSMapGlobals::m_SaveData[] = {
	DEFINE_FIELD(CTSMapGlobals, player_at_a_time_num, FIELD_INTEGER),
	DEFINE_FIELD(CTSMapGlobals, player_respawn_num, FIELD_INTEGER),
};
IMPLEMENT_SAVERESTORE(CTSMapGlobals, CBaseEntity);

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
	UTIL_SetSize(pev, g_vecZero, g_vecZero);

	player_at_a_time_num = 0;
	player_respawn_num = 0;
	m_field14 = 0;
	m_field18 = 0;

	SetThink(&CTSMapGlobals::WaitThink);
	pev->nextthink = gpGlobals->time + 10.0f;

	// SLICE: the original Spawn also pushes the spawnflags into CTSGameRules — round-timer enable
	// (!(SF & 0x20)), superjump (SF & 0x40), and the weapon-restriction mask (get_weapon_restriction()
	// stored into the rules' restriction fields @+0x586c/0x5870). Those CTSGameRules members aren't
	// reconstructed yet, so the wiring is deferred (the entity itself is now method/member-faithful).
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
