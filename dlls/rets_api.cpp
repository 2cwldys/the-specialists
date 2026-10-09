/***
*  rets_api.cpp — ReTS_GetInterface, the player-state interface AMX Mod X's TSX module reads
*  (reTSX). NOT in the original: the original's plugins poke member offsets, which reTS cannot
*  match (see rets_api.h). Each entry maps one original field (reference/coverage/offset_map.tsv)
*  to the reTS member or computation that holds it.
***/
#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "player.h"
#include "ts_gun.h"
#include "ts_weapon_info.h"
#include "ts_weapons.h" // TSKnifeThrower
#include "rets_api.h"

static CBasePlayer *RetsPlayer(edict_t *pEdict)
{
	if (FNullEnt(pEdict))
		return nullptr;

	CBaseEntity *pEntity = CBaseEntity::Instance(pEdict);
	return (pEntity && pEntity->IsPlayer()) ? static_cast<CBasePlayer *>(pEntity) : nullptr;
}

#define RETS_PLAYER_FIELD(name, type, member)                                      \
	static type Get##name(edict_t *pEdict)                                         \
	{                                                                              \
		CBasePlayer *pPlayer = RetsPlayer(pEdict);                                 \
		return pPlayer ? (type)pPlayer->member : 0;                                \
	}                                                                              \
	static void Set##name(edict_t *pEdict, type value)                             \
	{                                                                              \
		CBasePlayer *pPlayer = RetsPlayer(pEdict);                                 \
		if (pPlayer)                                                               \
			pPlayer->member = (decltype(pPlayer->member))value;                    \
	}

RETS_PLAYER_FIELD(Cash, int, m_iCash)                             // orig 0x540
RETS_PLAYER_FIELD(Kevlar, int, m_bHasKevlar)                      // orig 0x718 bit 0x10
RETS_PLAYER_FIELD(SlowMotionGo, float, m_flSlowMotionGo)          // orig 0x168
RETS_PLAYER_FIELD(ClientSlowAmt, float, m_flClientSlowAmt)        // orig 0x16c
RETS_PLAYER_FIELD(SlowMotionTarget, float, m_flSlowMotionTarget)  // orig 0x170
RETS_PLAYER_FIELD(NextSlowFlow, float, m_flNextSlowFlow)          // orig 0x178
RETS_PLAYER_FIELD(SlowMotionScale, float, m_flSlowMotionScale)    // orig 0x17c
RETS_PLAYER_FIELD(SlowMotionRamp, float, m_flSlowMotionRamp)      // orig 0x180
RETS_PLAYER_FIELD(Powerups, int, m_iPowerups)                     // orig 0x724
RETS_PLAYER_FIELD(HeldPowerup, int, m_iHeldPowerup)               // orig 0x728
RETS_PLAYER_FIELD(PowerupExpireTime, float, m_flPowerupExpireTime) // orig 0x72c
RETS_PLAYER_FIELD(PowerupTime, int, m_iPowerupTime)              // orig 0x730, an int
RETS_PLAYER_FIELD(Award, int, m_iAward)                           // orig 0x74c (byte)
RETS_PLAYER_FIELD(MeleeBar, float, m_flMeleeBar)                  // orig 0x774
RETS_PLAYER_FIELD(LungeTime, float, m_flWhiffLungeTime)           // orig 0x79c
RETS_PLAYER_FIELD(LungeForce, float, m_flWhiffLungeForce)         // orig 0x7a0
RETS_PLAYER_FIELD(SlowBullets, int, m_bSlowBullets)               // orig 0x175
RETS_PLAYER_FIELD(KungFu, int, m_iKungFu)                         // orig 0x74e
RETS_PLAYER_FIELD(LowGravity, int, m_bLowGravity)                 // orig 0x74f

// orig 0x548 is a stored counter (TakeArmor adds/subtracts 10); reTS derives it from the arsenal,
// the flag and kevlar
static int GetFreeSlots(edict_t *pEdict)
{
	CBasePlayer *pPlayer = RetsPlayer(pEdict);
	return pPlayer ? TSPlayerFreeSlots(pPlayer) : 0;
}

// The original's ts_setuserslots writes the counter; later pickups and drops move it from there.
// reTS keeps the difference to the derived count, which TSPlayerFreeSlots adds back.
static void SetFreeSlots(edict_t *pEdict, int slots)
{
	CBasePlayer *pPlayer = RetsPlayer(pEdict);
	if (!pPlayer)
		return;
	pPlayer->m_iFreeSlotsAdjust = 0;
	pPlayer->m_iFreeSlotsAdjust = slots - TSPlayerFreeSlots(pPlayer);
}

static float RetsGetSpeedBySlots(edict_t *pEdict)
{
	CBasePlayer *pPlayer = RetsPlayer(pEdict);
	return pPlayer ? GetSpeedBySlots(pPlayer) : 0;
}

static void RetsSetPlayerSpeed(edict_t *pEdict, float speed)
{
	CBasePlayer *pPlayer = RetsPlayer(pEdict);
	if (pPlayer)
		pPlayer->SetPlayerSpeed(speed);
}

static int GetAmmoMaxCarry(int caliber)
{
	if (caliber < 0 || caliber >= TS_NUM_CALIBERS)
		return 0;
	return get_ammo_info(caliber)->iMaxCarry;
}

// orig knife + 0x8c, the thrower's edict (CTSKnife::m_pThrower)
static edict_t *GetKnifeThrower(edict_t *pEdict)
{
	if (FNullEnt(pEdict))
		return nullptr;
	return TSKnifeThrower(CBaseEntity::Instance(pEdict));
}

// orig weapon_tsgun + 0xd4, int[16] per caliber
static int *AmmoReserve(edict_t *pEdict, int caliber)
{
	CBasePlayer *pPlayer = RetsPlayer(pEdict);
	CTSGun *pGun = pPlayer ? GetPlayerTSGun(pPlayer) : nullptr;
	if (!pGun || caliber < 0 || caliber >= TS_NUM_CALIBERS)
		return nullptr;
	return &pGun->m_ammoReserve[caliber];
}

static int GetAmmoReserve(edict_t *pEdict, int caliber)
{
	int *pReserve = AmmoReserve(pEdict, caliber);
	return pReserve ? *pReserve : 0;
}

static void SetAmmoReserve(edict_t *pEdict, int caliber, int rounds)
{
	int *pReserve = AmmoReserve(pEdict, caliber);
	if (pReserve)
		*pReserve = rounds;
}

// orig weapon_tsgun + 0x114, s_weapon_status[38]: bOwned +0, iClip +4. The wielded weapon's clip
// lives in the gun's m_iClip while it is out (CTSGun::SyncActiveToItem parks it on holster).
static TSItem *WeaponStatus(edict_t *pEdict, int weapon, CTSGun **ppGun)
{
	CBasePlayer *pPlayer = RetsPlayer(pEdict);
	CTSGun *pGun = pPlayer ? GetPlayerTSGun(pPlayer) : nullptr;
	if (!pGun || weapon < 0 || weapon >= TSW_COUNT)
		return nullptr;
	*ppGun = pGun;
	return &pGun->m_items[weapon];
}

static int GetWeaponOwned(edict_t *pEdict, int weapon)
{
	CTSGun *pGun;
	TSItem *pItem = WeaponStatus(pEdict, weapon, &pGun);
	return pItem ? pItem->owned : 0;
}

static void SetWeaponOwned(edict_t *pEdict, int weapon, int owned)
{
	CTSGun *pGun;
	TSItem *pItem = WeaponStatus(pEdict, weapon, &pGun);
	if (pItem)
		pItem->owned = owned != 0;
}

static int GetWeaponClip(edict_t *pEdict, int weapon)
{
	CTSGun *pGun;
	TSItem *pItem = WeaponStatus(pEdict, weapon, &pGun);
	if (!pItem)
		return 0;
	return weapon == pGun->m_weaponType ? pGun->m_iClip : pItem->clip;
}

static void SetWeaponClip(edict_t *pEdict, int weapon, int clip)
{
	CTSGun *pGun;
	TSItem *pItem = WeaponStatus(pEdict, weapon, &pGun);
	if (!pItem)
		return;
	pItem->clip = clip;
	if (weapon == pGun->m_weaponType)
		pGun->m_iClip = clip;
}

static rets_interface_t BuildInterface()
{
	rets_interface_t i = {};
	i.version = RETS_INTERFACE_VERSION;
	i.GetCash = GetCash;
	i.SetCash = SetCash;
	i.GetFreeSlots = GetFreeSlots;
	i.SetFreeSlots = SetFreeSlots;
	i.GetKevlar = GetKevlar;
	i.SetKevlar = SetKevlar;
	i.GetSlowMotionGo = GetSlowMotionGo;
	i.SetSlowMotionGo = SetSlowMotionGo;
	i.GetClientSlowAmt = GetClientSlowAmt;
	i.SetClientSlowAmt = SetClientSlowAmt;
	i.GetSlowMotionTarget = GetSlowMotionTarget;
	i.SetSlowMotionTarget = SetSlowMotionTarget;
	i.GetNextSlowFlow = GetNextSlowFlow;
	i.SetNextSlowFlow = SetNextSlowFlow;
	i.GetSlowMotionScale = GetSlowMotionScale;
	i.SetSlowMotionScale = SetSlowMotionScale;
	i.GetSlowMotionRamp = GetSlowMotionRamp;
	i.SetSlowMotionRamp = SetSlowMotionRamp;
	i.GetPowerups = GetPowerups;
	i.SetPowerups = SetPowerups;
	i.GetHeldPowerup = GetHeldPowerup;
	i.SetHeldPowerup = SetHeldPowerup;
	i.GetPowerupExpireTime = GetPowerupExpireTime;
	i.SetPowerupExpireTime = SetPowerupExpireTime;
	i.GetPowerupTime = GetPowerupTime;
	i.SetPowerupTime = SetPowerupTime;
	i.GetAward = GetAward;
	i.SetAward = SetAward;
	i.GetMeleeBar = GetMeleeBar;
	i.SetMeleeBar = SetMeleeBar;
	i.GetLungeTime = GetLungeTime;
	i.SetLungeTime = SetLungeTime;
	i.GetLungeForce = GetLungeForce;
	i.SetLungeForce = SetLungeForce;
	i.GetAmmoReserve = GetAmmoReserve;
	i.SetAmmoReserve = SetAmmoReserve;
	i.GetWeaponOwned = GetWeaponOwned;
	i.SetWeaponOwned = SetWeaponOwned;
	i.GetWeaponClip = GetWeaponClip;
	i.SetWeaponClip = SetWeaponClip;
	i.GetSlowBullets = GetSlowBullets;
	i.SetSlowBullets = SetSlowBullets;
	i.GetKungFu = GetKungFu;
	i.SetKungFu = SetKungFu;
	i.GetLowGravity = GetLowGravity;
	i.SetLowGravity = SetLowGravity;
	i.GetSpeedBySlots = RetsGetSpeedBySlots;
	i.SetPlayerSpeed = RetsSetPlayerSpeed;
	i.GetAmmoMaxCarry = GetAmmoMaxCarry;
	i.GetKnifeThrower = GetKnifeThrower;
	return i;
}

static const rets_interface_t g_RetsInterface = BuildInterface();

extern "C" DLLEXPORT const rets_interface_t *ReTS_GetInterface(int version)
{
	return version == RETS_INTERFACE_VERSION ? &g_RetsInterface : nullptr;
}
