/***
*  rets_api.h — the interface reTS exports for AMX Mod X's TSX module (reTSX).
*
*  Plugins written for the original TS reach player state by member offset. reTS's member layout
*  matches the original nowhere, differs between i386 and 64-bit, and some original fields are not
*  stored at all (free slots are computed, the item bits are separate members), so TSX reads and
*  writes reTS's state through these functions instead. The game library exports
*
*      extern "C" const rets_interface_t *ReTS_GetInterface(int version);
*
*  which returns NULL for any version but RETS_INTERFACE_VERSION. Any change to this struct bumps the
*  version. An entry is NULL when reTS has no such state; the caller then refuses.
*  Every function but GetAmmoMaxCarry takes the player's edict and does nothing (returns 0) for a
*  non-player.
*
*  reTS (dlls/rets_api.h) and AMX Mod X (modules/ts/tsx/rets_api.h) keep identical copies. Change
*  both together.
***/
#ifndef RETS_API_H
#define RETS_API_H

#define RETS_INTERFACE_VERSION 3
#define RETS_GETINTERFACE_NAME "ReTS_GetInterface"

struct edict_s;

typedef struct rets_interface_s
{
	int version; // RETS_INTERFACE_VERSION

	int (*GetCash)(struct edict_s *player);
	void (*SetCash)(struct edict_s *player, int cash);

	// The original keeps free slots as a counter (0x548, 81 on spawn) that pickups, drops, the flag
	// and kevlar move. reTS derives them from the arsenal; SetFreeSlots stores the difference to the
	// derived count, which then moves with the loadout as the counter would, until the next spawn.
	// Neither one changes the move speed: that is GetSpeedBySlots and SetPlayerSpeed below.
	int (*GetFreeSlots)(struct edict_s *player);
	void (*SetFreeSlots)(struct edict_s *player, int slots);

	int (*GetKevlar)(struct edict_s *player); // the original's item bit 0x10
	void (*SetKevlar)(struct edict_s *player, int kevlar);

	float (*GetSlowMotionGo)(struct edict_s *player);
	void (*SetSlowMotionGo)(struct edict_s *player, float value);
	float (*GetClientSlowAmt)(struct edict_s *player);
	void (*SetClientSlowAmt)(struct edict_s *player, float value);
	float (*GetSlowMotionTarget)(struct edict_s *player);
	void (*SetSlowMotionTarget)(struct edict_s *player, float value);
	float (*GetNextSlowFlow)(struct edict_s *player);
	void (*SetNextSlowFlow)(struct edict_s *player, float value);
	float (*GetSlowMotionScale)(struct edict_s *player);
	void (*SetSlowMotionScale)(struct edict_s *player, float value);
	float (*GetSlowMotionRamp)(struct edict_s *player);
	void (*SetSlowMotionRamp)(struct edict_s *player, float value);

	int (*GetPowerups)(struct edict_s *player);
	void (*SetPowerups)(struct edict_s *player, int powerups);
	int (*GetHeldPowerup)(struct edict_s *player);
	void (*SetHeldPowerup)(struct edict_s *player, int powerup);
	float (*GetPowerupExpireTime)(struct edict_s *player);
	void (*SetPowerupExpireTime)(struct edict_s *player, float time);
	float (*GetPowerupTime)(struct edict_s *player);
	void (*SetPowerupTime)(struct edict_s *player, float time);

	int (*GetAward)(struct edict_s *player); // the award/status byte
	void (*SetAward)(struct edict_s *player, int award);

	float (*GetMeleeBar)(struct edict_s *player);
	void (*SetMeleeBar)(struct edict_s *player, float value);
	float (*GetLungeTime)(struct edict_s *player);
	void (*SetLungeTime)(struct edict_s *player, float value);
	float (*GetLungeForce)(struct edict_s *player);
	void (*SetLungeForce)(struct edict_s *player, float value);

	int (*GetAmmoReserve)(struct edict_s *player, int caliber); // the player's weapon_tsgun, 0..15
	void (*SetAmmoReserve)(struct edict_s *player, int caliber, int rounds);

	// The original's s_weapon_status[weapon] (weapon_tsgun + 0x114, weapon 0..37). The grenade and
	// the knives count in their clip; reTS computes free slots from those counts.
	int (*GetWeaponOwned)(struct edict_s *player, int weapon);
	void (*SetWeaponOwned)(struct edict_s *player, int weapon, int owned);
	int (*GetWeaponClip)(struct edict_s *player, int weapon);
	void (*SetWeaponClip)(struct edict_s *player, int weapon, int clip);

	int (*GetSlowBullets)(struct edict_s *player); // orig 0x175, the bullet-time flag sent as TSBTime
	void (*SetSlowBullets)(struct edict_s *player, int slow);
	int (*GetKungFu)(struct edict_s *player); // orig 0x74e, the kung fu powerup
	void (*SetKungFu)(struct edict_s *player, int kungfu);
	int (*GetLowGravity)(struct edict_s *player); // orig 0x74f, the superjump powerup
	void (*SetLowGravity)(struct edict_s *player, int lowgravity);

	// The game's own GetSpeedBySlots (the move speed for the free slots) and CBasePlayer::SetPlayerSpeed
	// (clamps to 0..330 and sends it to the client), which the game runs after every slot change.
	float (*GetSpeedBySlots)(struct edict_s *player);
	void (*SetPlayerSpeed)(struct edict_s *player, float speed);

	// s_ammo_info[caliber].iMaxCarry, the most CTSGun::AddAmmo lets a caliber's reserve hold; it
	// follows the ammocount cvar. 0 for a caliber out of range.
	int (*GetAmmoMaxCarry)(int caliber);
} rets_interface_t;

typedef const rets_interface_t *(*RETS_GETINTERFACE_FN)(int version);

#endif // RETS_API_H
