/***
*  ts_weapons.cpp — server-only TS weapon helpers.
*  Functional/source-faithful (see CLAUDE.md).
*
*  The pure weapon DATA table (s_weapon_info + get_weapon_info + the per-caliber
*  ammo tables) moved to the SHARED ts_weapon_info.cpp (built into both hl.so and
*  client.so so the predicted .sc event handlers read the same table). What stays
*  here needs the engine (PRECACHE_*): the map-time precache of every weapon's
*  models/sounds/events.
***/
#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "ts_weapons.h"

void PrecacheAllTSWeapons()
{
	for (int i = 0; i < TSW_COUNT; i++)
	{
		const s_weapon_info *wi = get_weapon_info((tsweapon_t)i);
		if (wi->pszViewModel)   PRECACHE_MODEL((char *)wi->pszViewModel);
		if (wi->pszPlayerModel) PRECACHE_MODEL((char *)wi->pszPlayerModel);
		if (wi->pszWorldModel)  PRECACHE_MODEL((char *)wi->pszWorldModel);
		if (wi->pszFireSound)   PRECACHE_SOUND((char *)wi->pszFireSound);
	}

	// the grenade's shrapnel fragments (CM61Grenade detonation) + the death-dropped ammo pack
	// + the CTF flag (CTSFlag, runtime-created) — precached here so a mid-game spawn
	// (detonation / death / flag create) doesn't precache after spawn.
	PRECACHE_MODEL("models/shrapnel.mdl");
	PRECACHE_MODEL("models/ammopack.mdl");
	PRECACHE_MODEL("models/briefcase.mdl");
	PRECACHE_SOUND("items/gunpickup2.wav");
	PRECACHE_SOUND("items/weapondrop1.wav");
	PRECACHE_SOUND("misc/beep2.wav"); // CTSBomb countdown beep (runtime-armed bomb)
	// the dynamic deathmatch powerup spawner (CTSGameRules::PutPowerUp) creates ts_powerup at
	// runtime — precache its model here so a DM map with no ts_powerup placements doesn't
	// PRECACHE_MODEL after spawn (Host_Error, seen on ts_dust2).
	PRECACHE_MODEL("models/powerup.mdl");

	// TS muzzle-flash "flare" sprites — drawn at the gun muzzle attachment by the fire event
	// (client EV_TSFire). ts_muzzleflash1h = one-handed (pistols), ts_muzzleflash6 = two-handed.
	// (RE: client.dll FUN_100474b0 loads these two; akimbo draws one per barrel attachment.)
	PRECACHE_MODEL("sprites/ts_muzzleflash1h.spr");
	PRECACHE_MODEL("sprites/ts_muzzleflash6.spr");

	// Bullet-impact smoke puffs (client EV_TSBulletImpact): gun_smoke for metal/concrete/glass,
	// wood_smoke for wood/dirt. The texture/flesh impact SOUND + decal + blood are server-side.
	PRECACHE_MODEL("sprites/gun_smoke_add.spr");
	PRECACHE_MODEL("sprites/wood_smoke_add.spr");

	PRECACHE_SOUND("weapons/scock1.wav"); // pump-shotgun per-shell insert (ShotgunReload)

	// Melee slash sounds (CTSGun::MeleeAttack — knife/katana/seal-knife): swing (miss) + surface/flesh hit.
	PRECACHE_SOUND("weapons/knife/knife_miss.wav");
	PRECACHE_SOUND("weapons/knife/knife_hit.wav");
	PRECACHE_SOUND("weapons/knife/knife_hitbody.wav");
	PRECACHE_SOUND("weapons/katana/miss.wav");
	PRECACHE_SOUND("weapons/katana/hit.wav");
	PRECACHE_SOUND("weapons/katana/hit-body.wav");

	// weapon event scripts (CTSGun::Precache @0x80660) — must be precached at map
	// load so a runtime weapon give doesn't precache an event mid-game.
	PRECACHE_EVENT(1, "events/fire.sc");
	PRECACHE_EVENT(1, "events/reload.sc");
	PRECACHE_EVENT(1, "events/draw.sc");
}
