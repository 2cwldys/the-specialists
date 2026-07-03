/***
*
*	Copyright (c) 1996-2001, Valve LLC. All rights reserved.
*
*	This product contains software technology licensed from Id
*	Software, Inc. ("Id Technology").  Id Technology (c) 1996 Id Software, Inc.
*	All Rights Reserved.
*
*   Use, distribution, and modification of this source code and/or resulting
*   object code is restricted to non-commercial enhancements to products from
*   Valve LLC.  All other use, distribution, or modification is prohibited
*   without written permission from Valve LLC.
*
****/
#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "player.h"
#include "weapons.h"
#include "gamerules.h"
#include "ts_gun.h" // TS: the base ItemPostFrame IS the tsgun's per-frame driver (@0x903d4)
#include "in_buttons.h"

// Precaches the ammo and queues the ammo info for sending to clients
void AddAmmoNameToAmmoRegistry(const char* szAmmoname, const char* weaponName)
{
	// make sure it's not already in the registry
	for (int i = 0; i < MAX_AMMO_SLOTS; i++)
	{
		if (!CBasePlayerItem::AmmoInfoArray[i].pszName)
			continue;

		if (stricmp(CBasePlayerItem::AmmoInfoArray[i].pszName, szAmmoname) == 0)
			return; // ammo already in registry, just quite
	}


	giAmmoIndex++;
	ASSERT(giAmmoIndex < MAX_AMMO_SLOTS);
	if (giAmmoIndex >= MAX_AMMO_SLOTS)
		giAmmoIndex = 0;

	auto& ammoType = CBasePlayerItem::AmmoInfoArray[giAmmoIndex];

	ammoType.pszName = szAmmoname;
	ammoType.iId = giAmmoIndex; // yes, this info is redundant
	ammoType.WeaponName = weaponName;
}

bool CBasePlayerWeapon::CanDeploy()
{
	bool bHasAmmo = false;

	if (!pszAmmo1())
	{
		// this weapon doesn't use ammo, can always deploy.
		return true;
	}

	if (pszAmmo1())
	{
		bHasAmmo |= (m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] != 0);
	}
	if (pszAmmo2())
	{
		bHasAmmo |= (m_pPlayer->m_rgAmmo[m_iSecondaryAmmoType] != 0);
	}
	if (m_iClip > 0)
	{
		bHasAmmo |= true;
	}
	if (!bHasAmmo)
	{
		return false;
	}

	return true;
}

bool CBasePlayerWeapon::DefaultReload(int iClipSize, int iAnim, float fDelay, int body)
{
	if (m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] <= 0)
		return false;

	int j = V_min(iClipSize - m_iClip, m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType]);

	if (j == 0)
		return false;

	m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + fDelay;

	//!!UNDONE -- reload sound goes here !!!
	SendWeaponAnim(iAnim, body);

	m_fInReload = true;

	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 3;
	return true;
}

void CBasePlayerWeapon::ResetEmptySound()
{
	m_iPlayEmptySound = true;
}

bool CanAttack(float attack_time, float curtime, bool isPredicted)
{
#if defined(CLIENT_WEAPONS)
	if (!isPredicted)
#else
	if (1)
#endif
	{
		return (attack_time <= curtime) ? true : false;
	}
	else
	{
		return ((static_cast<int>(std::floor(attack_time * 1000.0)) * 1000.0) <= 0.0) ? true : false;
	}
}

// @ ItemPostFrame__17CBasePlayerWeapon (@0x903d4, original TU TS_weapons.cpp, decompile-exact) —
// TS REWROTE the stock per-frame driver for its single arsenal weapon: anything that isn't the
// tsgun (m_iId != 0x10) returns immediately (the original mod ships no stock weapons).
//
// BARE HANDS (no active item): attack2 / attack through their CanAttack gates, +reload ->
// Reload() (which routes to the kung-fu block), then WeaponIdle() unconditionally.
//
// WIELDED ITEM:
//  1. RELOAD COMPLETION — the item's reloading flag (item+0xc, our m_fInReload) was armed by
//     ReloadWeapon with the player's m_flNextAttack as the clock; when it elapses: refill the
//     clip from the caliber reserve, split the left/right barrel mirrors, and sync the AKIMBO
//     pairing (reloading an akimbo mirrors half into its single's parked clip; reloading a
//     single tops up its akimbo's parked pool).
//  2. DISPATCH — attack2 -> SecondaryAttack; attack -> PrimaryAttack (the CONTENDER refuses on
//     an empty chamber); IN_CANCEL (the original client's +alt2 button bit, 0x40) ->
//     AlternateAttack through the m_flNextAlternateAttack gate; +reload (not mid-reload, not
//     the contender) -> Reload.
//  3. WeaponIdle() EVERY frame (ShouldWeaponIdle-gated after an action; direct when no attack
//     buttons are down) — WeaponIdle self-paces on its timers and drives the pump/burst/cook
//     machines.
void CBasePlayerWeapon::ItemPostFrame()
{
	if (m_iId != TSGUN_WEAPON_ID)
		return; // the original mod runs NO stock weapons through this
	CTSGun *gun = static_cast<CTSGun *>(this);

	if (!gun->m_pActiveItem)
	{
		// KUNG-FU (bare hands).
		if ((m_pPlayer->pev->button & IN_ATTACK2) != 0 &&
			CanAttack(m_flNextSecondaryAttack, gpGlobals->time, UseDecrement()))
			SecondaryAttack();
		else if ((m_pPlayer->pev->button & IN_ATTACK) != 0 &&
			CanAttack(m_flNextPrimaryAttack, gpGlobals->time, UseDecrement()))
			PrimaryAttack();
		else if ((m_pPlayer->pev->button & IN_RELOAD) != 0)
			Reload(); // -> CCombatBlock
		WeaponIdle();
		return;
	}

	// 1. RELOAD COMPLETION (@0xa0455).
	if (gun->m_fInReload && m_pPlayer->m_flNextAttack <= UTIL_WeaponTimeBase())
	{
		const s_weapon_info *wi = gun->m_pActiveInfo;
		TSItem &it = *gun->m_pActiveItem;
		const int caliber = wi->iCaliber;
		const int reserve = (caliber > 0 && caliber < TS_NUM_CALIBERS) ? gun->m_ammoReserve[caliber] : 0;
		const int need = wi->iClip - gun->m_iClip;
		const int take = (need < reserve) ? need : reserve;
		gun->m_iClip += take;
		it.clip = gun->m_iClip;
		it.leftClip = it.clip / 2; // the barrel split is unconditional in the original
		it.rightClip = it.clip - it.leftClip;
		if (wi->iFireType == TSFT_AKIMBO)
		{
			// reloading the AKIMBO mirrors one barrel into the single's parked clip
			if (wi->iSingleType >= 0 && wi->iSingleType < TSW_COUNT && wi->iSingleType != gun->m_weaponType)
				gun->m_items[wi->iSingleType].clip = it.leftClip;
		}
		else if (wi->iAkimboType != -1)
		{
			// reloading the SINGLE tops up its akimbo's parked pool
			gun->m_items[wi->iAkimboType].clip += take;
			gun->m_items[wi->iAkimboType].leftClip = it.clip;
		}
		if (caliber > 0 && caliber < TS_NUM_CALIBERS)
			gun->m_ammoReserve[caliber] -= take;
		gun->m_fInReload = false; // item+0xc
	}

	// 2. DISPATCH.
	bool idleDirect = false;
	if ((m_pPlayer->pev->button & IN_ATTACK2) != 0 &&
		CanAttack(m_flNextSecondaryAttack, gpGlobals->time, UseDecrement()))
	{
		SecondaryAttack();
	}
	else if ((m_pPlayer->pev->button & IN_ATTACK) != 0 &&
		CanAttack(m_flNextPrimaryAttack, gpGlobals->time, UseDecrement()) &&
		(gun->m_weaponType != TSW_CONTENDER || gun->m_iClip != 0))
	{
		PrimaryAttack();
	}
	else if ((m_pPlayer->pev->button & IN_CANCEL) != 0 &&
		CanAttack(gun->m_flNextAlternateAttack, gpGlobals->time, UseDecrement()))
	{
		gun->AlternateAttack(); // +alt2 = the IN_CANCEL bit (0x40); ColdCock re-arms the gate
	}
	else if ((m_pPlayer->pev->button & IN_RELOAD) != 0 && !gun->m_fInReload &&
		gun->m_weaponType != TSW_CONTENDER)
	{
		Reload();
	}
	else if ((m_pPlayer->pev->button & (IN_ATTACK | IN_ATTACK2)) == 0)
	{
		idleDirect = true; // no fire buttons: straight to WeaponIdle (skips ShouldWeaponIdle)
	}

	// 3. IDLE (every frame — WeaponIdle self-paces).
	if (!idleDirect && !ShouldWeaponIdle())
		return;
	WeaponIdle();
}

void CBasePlayer::SelectLastItem()
{
	if (!m_pLastItem)
	{
		return;
	}

	if (m_pActiveItem && !m_pActiveItem->CanHolster())
	{
		return;
	}

	ResetAutoaim();

	// FIX, this needs to queue them up and delay
	if (m_pActiveItem)
		m_pActiveItem->Holster();

	CBasePlayerItem* pTemp = m_pActiveItem;
	m_pActiveItem = m_pLastItem;
	m_pLastItem = pTemp;

	auto weapon = m_pActiveItem->GetWeaponPtr();

	if (weapon)
	{
		weapon->m_ForceSendAnimations = true;
	}

	m_pActiveItem->Deploy();

	if (weapon)
	{
		weapon->m_ForceSendAnimations = false;
	}

	m_pActiveItem->UpdateItemInfo();
}
