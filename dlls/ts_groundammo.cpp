/***
*  ts_groundammo.cpp — RECONSTRUCTED CTSGroundAmmo from ts_i386.so + FGD.
*  Functional/source-faithful goal (see CLAUDE.md). recovered/server/CTSGroundAmmo.c.
*
*  @PointClass base(Target) size(-8 -8 -8, 8 8 8) = ts_groundammo : "TS ground ammo"
*    wduration (int) "Respawn time" def 60 -> m_respawnTime
*    mags1..mags11 (int) per-caliber magazine counts -> m_mags[0..10]
*      (9x19 / .45ACP / SHELLS / 5.56 / .50AE / 7.62x39 / 5.7x28 / 50BMG / 10mm /
*       .22LR / .454)
*    spawnflags: 16 "Lie on wall?"
*
*  A map-placed ammo pack (16 placements). Lifecycle mirrors CTSPowerUp: drops as a
*  MOVETYPE_TOSS object (or MOVETYPE_FLY pinned to a wall), settles on the floor via
*  Fall -> OnGround (SOLID_TRIGGER + WaitTouch), then refills a touching player who
*  holds a TS gun. The pack PERSISTS (it tops players up whenever they have room),
*  it doesn't get consumed on pickup.
*
*  SLICE: the faithful WaitTouch calls CTSGun::AddAmmo(caliber, mags) per ammo type
*  (the per-caliber reserve is part of the not-yet-reconstructed weapon items-array);
*  here we hand the held weapon's 9mm reserve a refill instead.
***/
#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "player.h"
#include "weapons.h"
#include "ts_weapons.h" // TSAddAmmo / TSRoundsPerMag (per-caliber refill)
#include "ts_groundammo.h"

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
