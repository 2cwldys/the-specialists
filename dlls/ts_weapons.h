/***
*  ts_weapons.h — RECONSTRUCTED TS weapon layer (server-side helpers).
*  Functional/source-faithful (see CLAUDE.md).
*
*  The pure-data half (tsweapon_t + s_weapon_info + get_weapon_info + the per-caliber
*  ammo/fire-mode helpers) lives in the SHARED ts_weapon_info.h, built into both the
*  server (dlls/) and the client (cl_dll/) — included below. This header adds the
*  server-only weapon helpers (precache/give/select/drop/ammo/…) that need the SDK.
***/
#ifndef TS_WEAPONS_H
#define TS_WEAPONS_H

#include "ts_weapon_info.h" // shared: tsweapon_t, s_weapon_info, get_weapon_info, ammo/mode helpers

// Precache every weapon's models + sound (call during the map's precache window).
void PrecacheAllTSWeapons();

// Give a player a CTSGun configured for `type` (used by pickups / future buy menu).
class CBaseEntity;
class CBasePlayer;
CBaseEntity *GiveTSWeapon(CBasePlayer *pPlayer, tsweapon_t type);

// Weapon-select (the faithful protocol — see reference/WEAPON_SELECT.md). The client's
// invnext/invprev select cursor emits the `weapon_<N>` command (numeric tsweapon_t);
// TSSelectWeaponType wields that owned type (holster->deploy). `lastinv` toggles back to
// the previously-wielded weapon.
void TSSelectWeaponType(CBasePlayer *pPlayer, int type);
void TSLastInv(CBasePlayer *pPlayer);

// Force the player's TS arsenal active + deploy (call at the end of PlayerSpawn). The engine's
// auto-deploy skips kung-fu (melee, no clip), so without this a kung-fu-only loadout would leave
// no active weapon — no viewmodel + ItemPostFrame (firing) never runs. See dlls/ts_gun.cpp.
void TSDeployArsenal(CBasePlayer *pPlayer);

// Trigger the held TS gun's alternate attack (+alt2 → coldcock pistol-whip).
void TSAlternateAttack(CBasePlayer *pPlayer);

// Cycle the held weapon's fire-mode (the `firemode` command -> CTSGun::SwitchFireMode).
void TSSwitchFireMode(CBasePlayer *pPlayer);

// Swap the held akimbo's active item (the `useitems` command -> CTSGun::SwitchItems).
void TSSwitchItems(CBasePlayer *pPlayer);

// Add `count` rounds of `caliber` to the player's TS gun reserve (capped). Returns true if
// any were added. Used by CTSGroundAmmo (the per-caliber refill station).
bool TSAddAmmo(CBasePlayer *pPlayer, int caliber, int count);

// Drop the player's active TS weapon into the world (the `drop` cmd / CBasePlayer::DropWeapon):
// spawns the CTSWorldGun(s) + does the items[] bookkeeping (akimbo splits into two singles; a
// single dropped while its akimbo is owned leaves the other). Returns false if it can't drop
// (melee/no world model). Pick up a dropped weapon of `type`/`clip` (CTSWorldGun pickup).
bool TSDropWeapon(CBasePlayer *pPlayer);
void TSPickUpWeapon(CBasePlayer *pPlayer, int type, int clip);
void TSDropWeaponsOnDeath(CBasePlayer *pPlayer); // scatter the arsenal on death
void TSDropAmmoOnDeath(CBasePlayer *pPlayer);    // drop the per-caliber reserve as a pack

// Spawn a dropped ammo pack carrying per-caliber `counts[16]`. Defined in ts_ammopack.cpp.
CBaseEntity *TSCreateAmmoPack(struct entvars_s *pevOwner, const int *counts, const Vector &origin,
	const Vector &velocity);

// Spawn a dropped weapon entity (CTSWorldGun) carrying `type`/`clip`. Defined in ts_worldgun.cpp.
CBaseEntity *TSCreateWorldGun(int type, int clip, const Vector &origin, const Vector &velocity);

// Infinite-clip powerup: multiply the held weapon's reserve. Returns true if applied.
bool TSMultiplyClips(CBasePlayer *pPlayer);

// Toss an M61 grenade (the grenade fire-type / grenade powerup). Defined in ts_grenade.cpp.
struct entvars_s;
CBaseEntity *TSThrowM61(struct entvars_s *pevOwner, const Vector &vecStart,
	const Vector &vecVelocity, float fuse, float damage);

#endif // TS_WEAPONS_H
