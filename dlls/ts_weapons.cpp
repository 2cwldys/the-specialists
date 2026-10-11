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
		if (wi->pszPlayerModelSil) PRECACHE_MODEL((char *)wi->pszPlayerModelSil);
		if (wi->pszWorldModel)  PRECACHE_MODEL((char *)wi->pszWorldModel);
		if (wi->pszFireSound)   PRECACHE_SOUND((char *)wi->pszFireSound);
	}

	// the grenade's shrapnel fragments (CM61Grenade detonation) + the death-dropped ammo pack
	// + the CTF flag (CTSFlag, runtime-created) — precached here so a mid-game spawn
	// (detonation / death / flag create) doesn't precache after spawn.
	PRECACHE_MODEL("models/shell.mdl");        // brass shell casing ejected on fire (cl_dll/ev_ts.cpp EV_TSFire
	                                           // EV_FindModelIndex). Stock weapons precached it; TS uses
	                                           // weapon_tsgun, so without this the casing model index is 0 = no shell.
	PRECACHE_MODEL("models/shotgunshell.mdl"); // shotgun shell casing
	PRECACHE_MODEL("models/shrapnel.mdl");
	PRECACHE_MODEL("models/ammopack.mdl");
	// NB: w_weaponbox.mdl is intentionally NOT precached. RE-confirmed the original never
	// instantiates CWeaponBox — both weaponbox creators (PackDeadPlayerItems, DropPlayerItem) are
	// dead code (0 callers), so CWeaponBox::Precache never runs and w_weaponbox is never precached.
	// TS drops weapons via its own DropWeapon/CTSWorldGun path (TSDropWeaponsOnDeath). See the
	// removed stock PackDeadPlayerItems() call in CBasePlayer::PlayerDeathThink.
	PRECACHE_MODEL("models/briefcase.mdl");
	PRECACHE_SOUND("items/gunpickup2.wav");
	PRECACHE_SOUND("items/weapondrop1.wav");
	PRECACHE_SOUND("weapons/weapondrop.wav"); // CBasePlayer::DropWeapon throw sound
	PRECACHE_SOUND("misc/beep2.wav"); // CTSBomb countdown beep (runtime-armed bomb)
	PRECACHE_SOUND("weapons/insert-shell.wav"); // per-shell shotgun racking (ts_gun.cpp ShotgunReload)
	PRECACHE_SOUND("debris/ric1.wav"); // bullet impact ricochet (ts_gun.cpp / ts_bullet.cpp)
	PRECACHE_SOUND("debris/ric2.wav");
	PRECACHE_SOUND("debris/ric3.wav");
	PRECACHE_SOUND("debris/ric4.wav");
	PRECACHE_SOUND("debris/ric5.wav");
	// the dynamic deathmatch powerup spawner (CTSGameRules::PutPowerUp) creates ts_powerup at
	// runtime — precache its model here so a DM map with no ts_powerup placements doesn't
	// PRECACHE_MODEL after spawn (Host_Error, seen on ts_dust2).
	PRECACHE_MODEL("models/powerup.mdl");

	// TS muzzle-flash sprites — drawn at the gun muzzle attachment by the fire event (client EV_TSFire),
	// the SAME for all weapons: gun_muzzle.spr when silenced (dimmer), gun_muzzle2.spr otherwise.
	PRECACHE_MODEL("sprites/gun_muzzle.spr");
	PRECACHE_MODEL("sprites/gun_muzzle2.spr");

	// Bullet-impact smoke puffs (client EV_TSBulletImpact): gun_smoke for metal/concrete/glass,
	// wood_smoke for wood/dirt. The texture/flesh impact SOUND + decal + blood are server-side.
	PRECACHE_MODEL("sprites/gun_smoke_add.spr");
	PRECACHE_MODEL("sprites/wood_smoke_add.spr");
	// Falling impact debris chips (client EV_TSBulletImpact): woodfrag for wood/dirt,
	// debris_concrete3 for concrete/default. Fly out + drop under gravity.
	PRECACHE_MODEL("sprites/woodfrag.spr");
	PRECACHE_MODEL("sprites/debris_concrete3.spr");

	// Pump-action rack sounds (CTSGun::WeaponIdle — the pump between shots).
	PRECACHE_SOUND("weapons/m3/m3-pump.wav");
	PRECACHE_SOUND("weapons/spas12/spas12-pump.wav");

	// Lasersight sprites (client EV draws the beam+dot from the muzzle for any player with a laser —
	// cl_dll/ts_attachments.cpp). The flashlight uses a dynamic light (no sprite to precache).
	PRECACHE_MODEL("sprites/laserbeamanim.spr"); // 16-frame animated lasersight beam
	PRECACHE_MODEL("sprites/laserdot.spr");

	// Silenced fire sounds — played by the client fire event (EV_TSFire s_fireSilSound) when a weapon
	// has a SILENCER attached (attach bit0). Precached so a silenced shot doesn't precache mid-game;
	// these are the distinct `-sil` files referenced by the original (ts_i386.so/client.dll). The set
	// matches s_fireSilSound[] in cl_dll/ev_ts.cpp (akimbo variants reuse their single's wav).
	PRECACHE_SOUND("weapons/glock/glock-fire-sil.wav");
	PRECACHE_SOUND("weapons/beretta/fire-sil.wav");
	PRECACHE_SOUND("weapons/uzi/uzi-fire-sil.wav");
	PRECACHE_SOUND("weapons/m4/m4-fire-sil.wav");
	PRECACHE_SOUND("weapons/mp5k/mp5k-fire-sil.wav");
	PRECACHE_SOUND("weapons/mk23/mk23-fire-sil.wav");
	PRECACHE_SOUND("weapons/deagle/fire-sil.wav");
	PRECACHE_SOUND("weapons/fnh/fnh-fire-sil.wav");
	PRECACHE_SOUND("weapons/aug/aug-fire-sil.wav");
	PRECACHE_SOUND("weapons/tmp/fire-sil.wav");
	PRECACHE_SOUND("weapons/pdw/fire-sil.wav");
	PRECACHE_SOUND("weapons/glock22/glock22-fire-sil.wav");
	PRECACHE_SOUND("weapons/ump/ump-fire-sil.wav");

	// Melee slash sounds (CTSGun::MeleeAttack — knife/katana/seal-knife): swing (miss) + surface/flesh hit.
	PRECACHE_SOUND("weapons/knife/knife_miss.wav");
	PRECACHE_SOUND("weapons/knife/knife_hit.wav");
	PRECACHE_SOUND("weapons/knife/knife_hitbody.wav");
	PRECACHE_SOUND("weapons/katana/miss.wav");
	PRECACHE_SOUND("weapons/katana/hit.wav");
	PRECACHE_SOUND("weapons/katana/hit-body.wav");
	PRECACHE_SOUND("player/superjump-land.wav"); // super-jump powerup landing thud (player.cpp PreThink)
	PRECACHE_SOUND("player/airwheel.wav");       // double-jump wall-climb kick (player.cpp PreThink)
	PRECACHE_SOUND("player/land.wav");           // dive-roll landing (player.cpp PreThink)
	PRECACHE_SOUND("player/dive.wav");           // +alt1 dive (player.cpp PreThink)
	PRECACHE_SOUND("weapons/whizz1.wav"); // bullet flyby (cl_dll ev_ts)
	PRECACHE_SOUND("weapons/whizz2.wav");
	PRECACHE_SOUND("weapons/whizz3.wav");
	PRECACHE_SOUND("weapons/whizz4.wav");
	PRECACHE_SOUND("weapons/whizz5.wav");
	PRECACHE_SOUND("goslow.wav");                // slow-mo initiation sound (cl_dll powerup HUD)
	PRECACHE_SOUND("player/heartbeat1.wav");     // slow-mo init heartbeats (faded x4)

	// TS damage feedback (player.cpp TraceAttack body-impact + Pain voice): bullet/melee hit on flesh,
	// vest, head; the pain grunt.
	// The engine keeps the name pointer it is given, so a name must outlive the map: string literals only.
	PRECACHE_SOUND("player/bodyhit1.wav");
	PRECACHE_SOUND("player/bodyhit2.wav");
	PRECACHE_SOUND("player/bodyhit3.wav");
	PRECACHE_SOUND("player/bodyhit4.wav");
	PRECACHE_SOUND("player/pain1.wav");
	PRECACHE_SOUND("player/pain2.wav");
	PRECACHE_SOUND("player/pain3.wav");
	PRECACHE_SOUND("player/pain4.wav");
	PRECACHE_SOUND("player/headshot.wav");
	PRECACHE_SOUND("player/headshot2.wav"); // only headshot.wav + headshot2.wav exist (no headshot1.wav)
	PRECACHE_SOUND("player/kevlarhit.wav");

	// weapon event scripts (CTSGun::Precache @0x80660) — must be precached at map
	// load so a runtime weapon give doesn't precache an event mid-game.
	PRECACHE_EVENT(1, "events/fire.sc");
	PRECACHE_EVENT(1, "events/reload.sc");
	PRECACHE_EVENT(1, "events/draw.sc");
	PRECACHE_EVENT(1, "events/knife.sc"); // melee impact spark
}
