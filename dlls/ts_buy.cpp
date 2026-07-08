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
#include "ts_gun.h"
#include "ts_weapon_info.h"       // CTSGun + TSGUN_WEAPON_ID — attachment routing (AddItem)

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

// @ AddItemToArmament — queue a weapon or item into the loadout at the first free slot
// (cap 32). isItem==true marks an item bitmask (kevlar/grenade/attachment) so BuyArmament
// applies it via AddItem instead of BuyWeapon.
void CBasePlayer::AddItemToArmament(int type, int count, bool isItem)
{
	for (int i = 0; i < 32; i++)
	{
		if (m_armament[i].type == 0)
		{
			m_armament[i].type = type;
			m_armament[i].count = count;
			m_armament[i].isItem = isItem;
			return;
		}
	}
}

// Item bitmask for AddItem / the loadout (the original packs these in one int; bits
// recovered from AddItem @0x8440c). Attachments (silencer/laser/...) are 1/2/4/8.
#define TS_ITEM_KEVLAR  0x10
#define TS_ITEM_GRENADE 0x20

// @ TakeArmor — grant (amount>=0) or remove (amount<0) kevlar. Kevlar is a BINARY flag
// (m_bHasKevlar); the armorvalue it sets is only a "have armor" marker (TakeDamage applies a
// flat armor_absorb reduction, it does NOT deplete — see CBasePlayer::TakeDamage). Recovered
// from TakeArmor @0x83da4. SLICE: the original also costs/refunds 10 "free slots" and recomputes
// move speed via GetSpeedBySlots/SetPlayerSpeed — the free-slots encumbrance subsystem is
// deferred (its speed factors are runtime-init and unrecoverable statically).
void CBasePlayer::TakeArmor(int amount)
{
	if (amount >= 0)
	{
		if (!m_bHasKevlar)
		{
			m_bHasKevlar = true;
			EMIT_SOUND(ENT(pev), CHAN_ITEM, "weapons/items.wav", VOL_NORM, ATTN_NORM);
		}
		float v = (float)amount + pev->armorvalue;
		if (v > 100.0f) // armor cap
			v = 100.0f;
		pev->armorvalue = v;
	}
	else if (m_bHasKevlar)
	{
		m_bHasKevlar = false;
		pev->armorvalue = 0;
	}
}

// @ AddItem — grant a loadout item from its bitmask (kevlar 0x10, grenade 0x20, gun
// attachments 1/2/4/8). Recovered from AddItem @0x8440c. CashBased() is off so the cost
// branches (kevlar 1900 / grenade 700 / attachments) are dead. Kevlar gives 100 armor via
// TakeArmor; grenade is the team-mode M61; attachments route to the active CTSGun (SLICE).
void CBasePlayer::AddItem(int itemBits)
{
	if ((itemBits & TS_ITEM_KEVLAR) != 0)
	{
		if (pev->armorvalue < 100.0f) // not already maxed (orig: armorvalue < cap)
			TakeArmor(100);
	}
	else if ((itemBits & TS_ITEM_GRENADE) != 0)
	{
		if (g_teamplay) // grenades are team-mode only
			BuyWeapon(TSW_M61);
	}
	else
	{
		// gun attachments (bits 1/2/4/8: silencer/laser/flashlight/scope) -> the active CTSGun
		// (RE AddItem@0x8440c: requires m_pActiveItem with m_iId == TSGUN_WEAPON_ID(0x10), then
		// CTSGun::AddItem ORs the bits onto the active weapon). The silencer (bit 1) drives the
		// fire-sil sound + muzzle suppression via the fire event (iparam2). NB: applies to the
		// WIELDED weapon — buy a silencer while holding the gun you want it on (the spawn-time
		// loadout-apply order is a SLICE; see ts_buymenu.cpp Apply()).
		if (m_pActiveItem && m_pActiveItem->m_iId == TSGUN_WEAPON_ID)
			((CTSGun *)m_pActiveItem)->AddItem(itemBits);
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
bool CBasePlayer::BuyWeapon(int type, int iAttach)
{
	// the restriction gate (@0x84606): the category-bitmask filter over the server's
	// weaponrestriction mask (default 0x1f allows everything — stock behavior unchanged).
	extern bool IsRestrictedWeapon(tsweapon_t w, int mask);
	if (TSGameRules && IsRestrictedWeapon((tsweapon_t)type, TSGameRules->m_iWeaponRestriction))
		return false;

	if (type == TSW_M61)
	{
		if (!g_teamplay) // grenades are team-mode only
			return false;
		if (m_iGrenadeBuys >= 2) // cap 2 per life
			return false;
		m_iGrenadeBuys++;
	}

	// Free in the shipped game (CashBased() == false). If a cash economy is ever turned
	// on, this is where the price (get_weapon_info(type)->price, s_weapon_info+0xd8) would
	// be checked against m_iCash and deducted.
	// @0x846ca: the buy path selects the purchase AND records the buyer (the sole
	// bRecordOwner caller — item+0x30). iAttach = the tkw token's attach mask.
	return PickUpWeapon(type, NULL, iAttach, true, true);
}

// @0x841e8 BuyAmmo(caliber) (decompile-exact shape) — FILL the caliber's reserve to its cap,
// one magazine (roundsPerMag) at a time, charging the per-mag price each step when the cash
// economy is on (free in the shipped game -> fills straight to iMaxCarry — the original's
// "18/120 m16 on buy"). caliber == -2 fills EVERY owned weapon's caliber. Plays the clip
// sound once per call.
bool CBasePlayer::BuyAmmo(int caliber)
{
	CTSGun *pGun = GetPlayerTSGun(this);
	if (!pGun)
		return false;

	bool any = false;
	auto fill = [&](int cal)
	{
		if (cal <= 0 || cal >= 16)
			return;
		const s_ammo_info *ai = get_ammo_info(cal);
		while (pGun->AddAmmo(cal, ai->iRoundsPerMag))
		{
			if (CashBased())
			{
				m_iCash -= ai->iPrice; // per-mag charge (@0x84284)
				if (m_iCash - ai->iPrice < 0)
					break;
			}
			any = true;
		}
	};

	if (caliber == -2)
	{
		for (int t = 1; t < TSW_COUNT; t++)
			if (pGun->IsOwned(t))
				fill(get_weapon_info((tsweapon_t)t)->iCaliber);
	}
	else
	{
		if (CashBased() && m_iCash - get_ammo_info(caliber)->iPrice < 0)
			return false;
		fill(caliber);
	}

	if (any)
		EMIT_SOUND_DYN(ENT(pev), CHAN_ITEM, "items/9mmclip1.wav", VOL_NORM, 0.8f, 0, PITCH_NORM);
	return any;
}

// @ BuyArmament — apply the player's loadout: buy (or free-give) each queued weapon, then
// top up each queued caliber. Mirrors the .so's BuyArmament @0x83f10 — which does NOT reset
// the armament afterwards, so the chosen loadout PERSISTS and is re-applied on every spawn
// (respawn + round restart). The queue is only cleared by an explicit rst_cfg (rebuy).
void CBasePlayer::BuyArmament()
{
	// Kung-fu (TSW_MELEE) is the unarmed melee every player ALWAYS has, regardless of loadout — give
	// it first so the weapon-select bar always offers it even with an empty/minimal config.
	GiveTSWeapon(this, TSW_MELEE);

	for (int i = 0; i < 32; i++)
	{
		if (m_armament[i].type == 0)
			continue;
		if (m_armament[i].isItem) // kevlar/grenade/attachment bitmask -> AddItem
			AddItem(m_armament[i].type);
		else
		{
			// The queued second number is dual-meaning (dossier-attachments §2): for a
			// STACKABLE (knife/m61) it is the COUNT (one BuyWeapon per unit — TakeWeapon's
			// +1 merge, cap iPellets); for a GUN it is the ATTACH MASK, carried into
			// BuyWeapon -> PickUpWeapon -> TakeWeapon (which clamps to the supported set,
			// starts laser/flash ACTIVE, and mirrors onto an akimbo pair).
			const s_weapon_info *wi = get_weapon_info((tsweapon_t)m_armament[i].type);
			if (wi && wi->iStackable)
			{
				const int n = (m_armament[i].count > 1) ? m_armament[i].count : 1;
				for (int k = 0; k < n; k++)
					BuyWeapon(m_armament[i].type);
			}
			else
				BuyWeapon(m_armament[i].type, m_armament[i].count & 0xF);
		}
	}
	for (int cal = 1; cal < 16; cal++)
	{
		if (m_armamentAmmo[cal] > 0)
			BuyAmmo(cal); // @0x840e3 — per queued caliber; fills to the cap in free mode
	}
	// NB: NO ResetArmament here — the loadout persists across spawns (the original keeps it),
	// so a round restart / respawn re-equips the same config. rst_cfg clears it on a rebuy.
}
