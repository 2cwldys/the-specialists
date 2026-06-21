/***
*  ts_buy.cpp — RECONSTRUCTED TS cash + buy/armament system (cont.36).
*  Functional/source-faithful (see CLAUDE.md). recovered/buy/CBasePlayer.c:
*    GiveCash @0x9310c, ResetArmament @0x93e88, AddItemToArmament @0x93ea4,
*    AddAmmoToArmament @0x93ef4, BuyArmament @0x93f10, BuyAmmo @0x941e8,
*    BuyWeapon @0x945bc; CashBased @0x66abc; IsRestrictedWeapon @0x770c8.
*
*  IMPORTANT — CashBased() is hardcoded `return 0` in the shipped .so: the cash
*  economy is COMPILED OUT, so every `if (CashBased())` cost-deduction branch is dead
*  and the buy system gives weapons/ammo FREE. The real mechanic is therefore a free
*  ARMAMENT (loadout) selector: the client class/buy menu queues weapons+ammo
*  (AddItemToArmament/AddAmmoToArmament) and BuyArmament applies them on spawn. We keep
*  the cash member + GiveCash (tracked for the cash-for-win/lose cvars + a future sell
*  zone) but, faithfully, never charge for weapons.
*
*  The faithful UI is the cl_dll buy menu (menuselect); not built. The server entry
*  points are exposed here and driven by console commands in client.cpp (// SLICE: menu).
***/
#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "player.h"
#include "gamerules.h"
#include "ts_gamerules.h"
#include "ts_weapons.h"   // tsweapon_t, get_weapon_info, GiveTSWeapon, TSAddAmmo

// M61 grenade weapon index (tsweapon_t) — special-cased in BuyWeapon (team-mode only, cap 2).
#define TSW_M61 24

// @ CashBased__Fv — the shipped binary returns 0 unconditionally (cash charging disabled).
bool CashBased()
{
	return false;
}

// @ GiveCash — add `amount` to the player's cash, clamped to [0, 30000].
int CBasePlayer::GiveCash(int amount)
{
	m_iCash += amount;
	if (m_iCash < 0)
		m_iCash = 0;
	if (m_iCash > 30000)
		m_iCash = 30000;
	return m_iCash;
}

// @ ResetArmament — clear the pending loadout (the .so zeroes 0x70 ints from +0x554).
void CBasePlayer::ResetArmament()
{
	for (int i = 0; i < 16; i++)
		m_armamentAmmo[i] = 0;
	for (int i = 0; i < 32; i++)
		m_armament[i] = s_armament_item();
}

// @ AddItemToArmament — queue a weapon into the loadout at the first free slot (cap 32).
void CBasePlayer::AddItemToArmament(int type, int count, bool gift)
{
	for (int i = 0; i < 32; i++)
	{
		if (m_armament[i].type == 0)
		{
			m_armament[i].type = type;
			m_armament[i].count = count;
			m_armament[i].gift = gift;
			return;
		}
	}
}

// @ AddAmmoToArmament — queue ammo (rounds) for a caliber into the loadout.
void CBasePlayer::AddAmmoToArmament(int caliber, int rounds)
{
	if (caliber >= 0 && caliber < 16)
		m_armamentAmmo[caliber] = rounds;
}

// @ BuyWeapon — give the player a weapon. Free (CashBased off). Gated on the weapon
// restriction (// SLICE: the category-by-bitmask IsRestrictedWeapon@0x770c8 over the
// CTSMapGlobals restriction isn't wired — the default restriction 0x1f allows every
// category, so an allow-all stub is faithful for stock maps) and the M61 special rule:
// grenades are team-mode only and capped at 2 per life. Returns true if given.
bool CBasePlayer::BuyWeapon(int type)
{
	if (type == TSW_M61)
	{
		if (!TSGameRules()->IsTeamMode()) // grenades are team-mode only
			return false;
		if (m_iGrenadeBuys >= 2) // cap 2 per life
			return false;
		m_iGrenadeBuys++;
	}

	// Free in the shipped game (CashBased() == false). If a cash economy is ever turned
	// on, this is where the price (get_weapon_info(type)->price, s_weapon_info+0xd8) would
	// be checked against m_iCash and deducted.
	return GiveTSWeapon(this, (tsweapon_t)type) != nullptr;
}

// @ BuyAmmo — give the player ammo for a caliber (free). Tops up the shared per-caliber
// reserve via TSAddAmmo (capped at the caliber max).
bool CBasePlayer::BuyAmmo(int caliber, int rounds)
{
	if (caliber <= 0 || caliber >= 16 || rounds <= 0)
		return false;
	return TSAddAmmo(this, caliber, rounds);
}

// @ BuyArmament — apply the pending loadout: buy (or free-give) each queued weapon, then
// top up each queued caliber. Mirrors the .so's BuyArmament (gift slots bypass the buy,
// everything is free anyway), then ResetArmament so it isn't re-applied.
void CBasePlayer::BuyArmament()
{
	for (int i = 0; i < 32; i++)
	{
		if (m_armament[i].type != 0)
			BuyWeapon(m_armament[i].type);
	}
	for (int cal = 1; cal < 16; cal++)
	{
		if (m_armamentAmmo[cal] > 0)
			BuyAmmo(cal, m_armamentAmmo[cal]);
	}
	ResetArmament();
}
