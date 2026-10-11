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

// Throw a knife/seal-knife/katana projectile (CTSGun::ThrowKnife/KatanaThrow — right-click).
// knifeType = the s_weapon_info+0x20 code (knife 6 / sealknife 7 / katana 8). Defined in ts_grenades.cpp.
class CBasePlayer;
class CBaseEntity;
CBaseEntity *TSThrowKnife(CBasePlayer *pOwner, const Vector &vecStart, const Vector &vecVel, int knifeType);
// The thrown blade's kill-feed name (CTSKnife::GetDescription bridge; "Throwing" fallback).
const char *TSKnifeGetDescription(CBaseEntity *pEnt);
// The thrower of a thrown blade (CTSKnife +0x8c), NULL for any other entity (AddToFullPack's glow).
edict_t *TSKnifeThrower(CBaseEntity *pEnt);

// Give a player a CTSGun configured for `type` (used by pickups / future buy menu).
class CBaseEntity;
class CBasePlayer;
CBaseEntity *GiveTSWeapon(CBasePlayer *pPlayer, tsweapon_t type);
bool CashBased(); // ts_buy.cpp — the cash economy is compiled OUT in the shipped game (returns false)

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

// Slow-world sound pitch (1..100 = 100*perceivedSlow); pitches NORMAL-recorded combat sounds DOWN in
// slow-motion. Defined in ts_gun.cpp; shared by ts_knife.cpp / ts_bullet.cpp. See its definition.
int TSSlowPitch(CBasePlayer *p);
// Pitch for RECORDED-SLOW sounds (gun-fire / bullet-hit ric): 1..255 = 100 + 300*perceivedSlow — high at
// normal play (the slowed recording sped up to sound natural), deepening in slow-motion. See ts_gun.cpp.
int TSRecordedSlowPitch(CBasePlayer *p);

// @ GetSpeedBySlots — slot-based base movement speed (210..330) from the player's carried weapon slots;
// more/heavier weapons => slower. Slow-mo (fuser1) scales it on top. Defined in ts_gun.cpp.
float GetSpeedBySlots(CBasePlayer *pPlayer);
int TSPlayerFreeSlots(CBasePlayer *pPlayer);

// (TSCheckRunThrough removed cont.96 — duplicate of CTSStunt::CheckForBreakables @0x8bd9c.)

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
// (TSDropWeapon is now the method CBasePlayer::DropWeapon — see player.h / ts_gun.cpp.)
// (TSPickUpWeapon is now the method CBasePlayer::PickUpWeapon — see player.h / ts_player.cpp.)
void TSDropWeaponsOnDeath(CBasePlayer *pPlayer); // scatter the arsenal on death
// The drop-into-world factories are the static methods CTSWorldGun::CreateWeapon /
// CTSAmmoPack::CreateAmmoPack (dlls/ts_worldgun.h), matching the original's TS_worldgun.cpp.

// Infinite-clip powerup: multiply the held weapon's reserve. Returns true if applied.
bool TSMultiplyClips(CBasePlayer *pPlayer);

// Toss an M61 grenade (the grenade fire-type / grenade powerup). Defined in ts_grenade.cpp.
struct entvars_s;
CBaseEntity *TSThrowM61(struct entvars_s *pevOwner, const Vector &vecStart,
	const Vector &vecVelocity, float fuse);

#endif // TS_WEAPONS_H
