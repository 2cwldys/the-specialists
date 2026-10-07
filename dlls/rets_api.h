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
*  Every function takes the player's edict and does nothing (returns 0) for a non-player.
*
*  AMX Mod X keeps an identical copy (modules/ts/tsx/rets_api.h). Change both together.
***/
#ifndef RETS_API_H
#define RETS_API_H

#define RETS_INTERFACE_VERSION 2
#define RETS_GETINTERFACE_NAME "ReTS_GetInterface"

struct edict_s;

typedef struct rets_interface_s
{
	int version; // RETS_INTERFACE_VERSION

	int (*GetCash)(struct edict_s *player);
	void (*SetCash)(struct edict_s *player, int cash);

	int (*GetFreeSlots)(struct edict_s *player);
	void (*SetFreeSlots)(struct edict_s *player, int slots); // NULL: reTS computes free slots

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
} rets_interface_t;

typedef const rets_interface_t *(*RETS_GETINTERFACE_FN)(int version);

#endif // RETS_API_H
