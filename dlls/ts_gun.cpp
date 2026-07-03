/***
*  ts_gun.cpp — RECONSTRUCTED CTSGun (functional-minimal, all weapon types).
*  Functional/source-faithful goal (see CLAUDE.md). recovered/server/CTSGun.c.
*
*  CTSGun is the original's single unified weapon (entity `weapon_tsgun`, id 16)
*  that morphs by tsweapon_t. The FAITHFUL engine is a dual-wield "active items"
*  array (m_pActiveItem@0xcc / m_pActiveInfo@0xd0 / items[]@0x45), client-side
*  weapon-param networking, CTSAnimation, akimbo/fire-mode/zoom, dispatching by
*  the fire-type byte (s_weapon_info+0x348).
*
*  THIS SLICE is a functional-minimal stand-in: one CTSGun per weapon (shared id
*  16, so a player holds one TS weapon at a time), driven by m_weaponType against
*  the recovered s_weapon_info table — deploys the right view/world model and
*  semi-auto-fires with the weapon's real damage/clip. The faithful items-array /
*  akimbo / fire-mode engine replaces the // SLICE bodies incrementally.
***/
#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "player.h"
#include "weapons.h"
#include "ts_weapons.h"
#include "ts_bullet.h"
#include "ts_player.h" // DoubleFireRate (powerup accessor)
#ifndef CLIENT_DLL
#include "ts_gamerules.h" // VictimKatanaHold/VictimKnifeHold (the melee block gates)
#endif
#include "ts_worldgun.h" // CTSWorldGun::CreateWeapon / CTSAmmoPack::CreateAmmoPack (drop side)
#include "ts_shared.h"   // FixedSlowMul (DropKickback)

// WEAPON TIMING: the build defines CLIENT_WEAPONS (both targets), so UTIL_WeaponTimeBase() == 0 — a
// 0-based RELATIVE clock that gets DECREMENTED each frame by gpGlobals->frametime, on the CLIENT by
// the prediction engine and on the SERVER by CBasePlayer::ItemPostFrame's decrement loop (player.cpp,
// `#if defined(CLIENT_WEAPONS)` + `gun->UseDecrement()`). CTSGun is now CLIENT-PREDICTED (UseDecrement()
// == true), so its attack/idle/reload timers use UTIL_WeaponTimeBase() like every predicted weapon —
// set/compared against that base, decremented on both sides, consistent for prediction.
// (cont.69q earlier used absolute gpGlobals->time as a workaround for "fire-once-stuck"; that was only
// needed because UseDecrement() was false → the server decrement loop above skipped CTSGun. With
// UseDecrement()==true the server decrements it, so the 0-base elapses correctly. See EVENTS.md.)
// NOTE: real-time DELTAS (m_lastCCTime combo window, melee-bar regen now in PreThink) keep gpGlobals->time —
// they measure wall-clock elapsed, not a decremented weapon timer.

// The single weapon_tsgun (id 16) IS the player's whole TS arsenal. The original
// keeps an `s_weapon_status items[]` array INDEXED BY tsweapon_t (0x38-byte entries
// at gun+type*0x38+0x114), with m_pActiveItem(@0xcc)/m_pActiveInfo(@0xd0) pointing at
// the wielded one + per-item akimbo barrel clips. This is a clean MODERN reconstruction
// of that layout (recovered from TakeWeapon@0x80a04 / DropWeapon@0x84900 /
// SwitchItems@0x81564 / GetBetterWeapon@0x80f40 — see reference/WEAPON_SELECT.md):
// one TSItem per tsweapon_t. The faithful akimbo OWNERSHIP model — a single and its
// akimbo are SEPARATE owned items (combine keeps both), the akimbo carries the two
// barrel clips, and a single can be wielded/dropped independently of its akimbo pair.

#include "ts_gun.h" // the CTSGun class (extracted so the client prediction harness can instantiate it)
#ifdef CLIENT_DLL
#include "../cl_dll/ts_recoil.h" // TS_TriggerMeleeShake — the per-swing melee "bounce-back" (client only)
#endif

// TS weapon-info HUD message (the ammo readout) — registered centrally in LinkUserMessages
// (UserMessages.cpp): "WeaponInfo", 6. GLOBALS like the original's (TS_gun.cpp home).
int gmsgWeaponInfo = 0;
// The weapon-select bar message: "WStatus", size 3 = BYTE type, BYTE owned, BYTE clip.
int gmsgTSWeaponStatus = 0;
int gmsgTSActiveItems = 0; // per-player attach broadcast (laser/flashlight visible to all clients)
// gmsgKFuPower moved to ts_player.cpp — sent ONLY from CBasePlayer::UpdateMeleeBar (extern via ts_player.h)

// `damagemult` cvar (default 0.0 = off → 1x). A value > 0 scales all TS weapon damage.
static float TSDamageMult()
{
	const float m = CVAR_GET_FLOAT("damagemult");
	return (m > 0.0f) ? m : 1.0f;
}

// The world slow the player PERCEIVES (drives sound pitch): their applied dilation pev->fuser1, or the
// others-scale ramp while they're the slow-motion activator. 1.0 normal, ~0.35 slow-mo, ~0.225 slow-pause.
static float TSPerceivedSlow(CBasePlayer *p)
{
	float s = p->pev->fuser1;
	if ((p->m_iPowerups & 9) && p->m_flSlowMotionRamp > 0.0f && p->m_flSlowMotionRamp < s)
		s = p->m_flSlowMotionRamp; // slowmo 0.35 / slowpause 0.225
	return s;
}

// NORMAL-recorded combat sounds (close-combat/knife/katana, thrown-knife impacts) sit at 100 normally and
// are SLOWED (lower pitch) in slow-motion — never sped up. pitch = 100 * perceivedSlow, capped at 100.
int TSSlowPitch(CBasePlayer *p)
{
	int pitch = (int)(100.0f * TSPerceivedSlow(p));
	return pitch < 1 ? 1 : (pitch > 100 ? 100 : pitch);
}

// RECORDED-SLOW sounds (the gun-fire WAVs + bullet-hit ric are recorded already slowed) must be pitched UP
// for normal play and only deepen toward natural in slow-motion. pitch = 100 + 300 * perceivedSlow (the
// same formula the client uses for gunshots in ev_ts.cpp): normal ~400→clamp 255, slow-mo ~205. 1..255.
int TSRecordedSlowPitch(CBasePlayer *p)
{
	int pitch = (int)(100.0f + 300.0f * TSPerceivedSlow(p));
	return pitch < 1 ? 1 : (pitch > 255 ? 255 : pitch);
}

LINK_ENTITY_TO_CLASS(weapon_tsgun, CTSGun);

// @ GetWeaponType__F10tsweapon_t @0x7705c — the weapon's restriction CATEGORY code:
// the fire type, with the special cases 34 katana -> 3 (knife cat), 33 sawed-off -> 1
// (shotgun), 16/37 akimbo SMGs -> 9, 10/30/8/21 akimbo pistols -> 8.
char GetWeaponType(tsweapon_t w)
{
	switch (w)
	{
	case TSW_KATANA: return 3;
	case TSW_SAWEDOFF: return 1;
	case TSW_UZI_AKIMBO:
	case TSW_SKORPION_AKIMBO: return 9;
	case TSW_MK23_AKIMBO:
	case TSW_FNH_AKIMBO:
	case TSW_BERETTA_AKIMBO:
	case TSW_GOLD: return 8;
	default: return (char)get_weapon_info(w)->iFireType;
	}
}

// @ IsRestrictedWeapon__F10tsweapon_ti @0x770c8 — true = the weapon is NOT purchasable
// under the allowed-categories mask (mask 0 = everything restricted; categories:
// knives 0x1, pistols 0x2, SMGs 0x4, rifles 0x8, shotguns 0x10; anything uncategorized
// is never restricted).
bool IsRestrictedWeapon(tsweapon_t w, int mask)
{
	if (w < 1 || w > 37)
		return false;
	if (mask == 0)
		return true;
	switch (GetWeaponType(w))
	{
	case 1: return (mask & 0x10) == 0; // shotguns
	case 3: return (mask & 0x1) == 0;  // knives
	case 6:
	case 7: return (mask & 0x8) == 0;  // rifles
	case 8: return (mask & 0x2) == 0;  // pistols
	case 9: return (mask & 0x4) == 0;  // SMGs
	default: return false;
	}
}

// @ IsWeaponAkimbo__Fi @0x7019c — fire-type 4 (the combinable akimbo pairs).
bool IsWeaponAkimbo(int type)
{
	return get_weapon_info((tsweapon_t)type)->iFireType == TSFT_AKIMBO;
}

// @ IsOnlyAkimbo__Fi @0x701d4 — fire-type 5 (akimbo-only, e.g. the golden colts).
bool IsOnlyAkimbo(int type)
{
	return get_weapon_info((tsweapon_t)type)->iFireType == TSFT_AKIMBO_ONLY;
}

// @ CalcSpread__FRfP11CBasePlayerf @0x7020c — velocity-scaled spread multiplier:
// spread *= clamp(|vel| / (100/f), 1, 3f). DEAD server-side in the original (ScaleSpread
// superseded it); kept compiled-and-uncalled for source parity (VERIFY the client copy).
void CalcSpread(float &spread, CBasePlayer *pPlayer, float f)
{
	float mul = pPlayer->pev->velocity.Length() / (100.0f / f);
	if (mul < 1.0f)
		mul = 1.0f;
	if (mul > 3.0f * f)
		mul = 3.0f * f;
	spread *= mul;
}

// @ DropKickback__FPfT0ff @0x76ea0 — 2-axis kick decay toward the align vector, step
// 15*amt/FixedSlowMul(slowFactor). DEAD server-side (the producer VKickback is live but
// never calls it in the .so); source-parity only.
void DropKickback(float *kick, float *align, float amt, float slowFactor)
{
	if (amt < 0.0f)
		return;
	const float klen = sqrtf(kick[0] * kick[0] + kick[1] * kick[1]);
	if (klen <= 0.0f)
		return;
	const float d0 = kick[0] - align[0];
	const float d1 = kick[1] - align[1];
	const float dlen = sqrtf(d0 * d0 + d1 * d1);
	const float step = 15.0f * (amt / FixedSlowMul(slowFactor));
	// literal transcription: the align axes advance by d*step when the step fits inside the
	// distance, else by d*dlen (the original's own asymmetric else-branch — kept as-is).
	align[0] += (step <= dlen) ? d0 * step : d0 * dlen;
	align[1] += (step <= dlen) ? d1 * step : d1 * dlen;
	kick[0] /= klen;
	kick[1] /= klen;
	float rem = klen - step;
	if (rem < 0.0f)
		rem = 0.0f;
	kick[0] *= rem;
	kick[1] *= rem;
}

// @ GetCloseCombatVectors__FiR6VectorN31 @0x76e2c — out = src + 32*forward (first + last
// args unused). DEAD in the original (the math re-inlined in KnifeFire); source parity.
void GetCloseCombatVectors(int a0, Vector &vecSrc, Vector &vecOut, Vector &vecForward, Vector &a4)
{
	(void)a0;
	(void)a4;
	vecOut = vecSrc + vecForward * 32.0f;
}



// @ g_iWeaponPriority (filled in compile_weapons_info@0x6fe28) — the auto-select order
// GetBetterWeapon walks after a drop (highest priority first). Last entry (kung-fu, 0)
// is the always-owned fallback. Recovered verbatim from the .so.
static const int g_iWeaponPriority[TSW_COUNT] = {
	18, 32, 13, 5, 15, 27, 11, 20, 4, 26, 33, 16, 37, 6, 23, 7, 3, 17, 19,
	10, 8, 21, 30, 31, 12, 36, 22, 9, 2, 14, 28, 1, 34, 25, 35, 24, 29, 0,
};

// The one weapon_tsgun in a player's inventory (the whole TS arsenal), or NULL.
CTSGun *GetPlayerTSGun(CBasePlayer *pPlayer)
{
	for (int i = 0; i < MAX_ITEM_TYPES; i++)
		for (CBasePlayerItem *p = pPlayer->m_rgpPlayerItems[i]; p; p = p->m_pNext)
			if (FClassnameIs(p->pev, "weapon_tsgun"))
				return (CTSGun *)p;
	return NULL;
}

// @ GetSpeedBySlots__FP11CBasePlayer (@0x66bd4) — slot-based base movement speed. The player has 81
// inventory slots (CBasePlayer+0x548 init 0x51); every CARRIED weapon deducts its slotCost, and the
// remaining capacity sets the base maxspeed = 210 + (120/81)*remaining → empty ~330, fully loaded ~210.
// More/heavier guns = slower. The original recomputes this after every TakeWeapon/Drop/Select; we derive
// it from the owned-items set each PreThink (player.cpp) and slow-mo (fuser1) scales it on top.
float GetSpeedBySlots(CBasePlayer *pPlayer)
{
	CTSGun *pGun = GetPlayerTSGun(pPlayer);
	if (!pGun)
		return 320.0f; // no TS arsenal yet → engine default
	int remaining = 81 - pGun->SlotsUsed();
	if (pPlayer->m_flagStatus.active)
		remaining -= 20; // carrying the CTF flag costs 20 slot-weight (CBasePlayer::TakeFlag -0x14)
	if (remaining < 0) remaining = 0;
	if (remaining > 81) remaining = 81;
	return 210.0f + (120.0f / 81.0f) * (float)remaining;
}

// Remaining inventory-slot capacity (CBasePlayer+0x548, init 81): lighter loadout =
// more free slots. Used by CTSStunt::CanDive* (ts_stunt.cpp) to gate dive height.
int TSPlayerFreeSlots(CBasePlayer *pPlayer)
{
	CTSGun *pGun = GetPlayerTSGun(pPlayer);
	int remaining = pGun ? (81 - pGun->SlotsUsed()) : 81;
	if (pPlayer->m_flagStatus.active)
		remaining -= 20; // carrying the CTF flag costs 20 slot-weight (CBasePlayer::TakeFlag -0x14)
	if (remaining < 0) remaining = 0;
	if (remaining > 81) remaining = 81;
	return remaining;
}

// @0x705e0 Reset — wipe the WHOLE TS arsenal + active state back to bare kung-fu. The original is a
// bulk zero of items[]/m_ammoReserve + the active pointers + the combo/timer/change-detect block.
// Callers: Spawn (clean init) and CBasePlayer::StartObserver (round-restart / death→spectate wipe).
void CTSGun::Reset()
{
	for (int t = 0; t < TSW_COUNT; t++)
		m_items[t] = TSItem();
	for (int c = 0; c < TS_NUM_CALIBERS; c++)
		m_ammoReserve[c] = 0;

	m_weaponType = TSW_MELEE;
	m_lastType = TSW_MELEE;
	m_pActiveItem = nullptr;
	m_pActiveInfo = nullptr;
	m_iClip = 0;
	m_fireMode = TSFM_SEMI;
	m_bHoldPosition = false;

	// combo / melee state
	m_comboStep = 0;
	m_kickCombo = 0;
	m_lastMeleeAnim = 0;
	m_lastCCTime = 0.0f;

	// per-fire transient timers
	m_burstOrPhase = 0;      // item+0x8 (burst/pump-phase/cook)
	m_fInSpecialReload = 0;  // item pumpToggle (rack step / akimbo side)
	m_flNextAlternateAttack = 0;
	m_flLastShotTime = 0.0f;

	// change-detect cache → force a fresh HUD resend next frame
	m_iClientWeaponType = -1;
	m_iClientClip = -1;
	m_iClientReserve = -1;
	m_iClientFireMode = -1;
	m_iClientAttach = -1;
	m_iClientEquip = -1;
	m_bWStatusInit = false;

	UpdateActivePointers();
}

// @ Spawn__6CTSGun — classname weapon_tsgun, id 16, fall-init as a pickup.
void CTSGun::Spawn()
{
	pev->classname = MAKE_STRING("weapon_tsgun");
	Precache();
	m_iId = TSGUN_WEAPON_ID;

	// @0x70555 — the original Spawn wipes the arsenal clean (Reset). Preserve the placed/initial
	// weapon type across the wipe (our SLICE seeds it loaded below; the original leaves bare kung-fu).
	const int initialType = m_weaponType;
	Reset();
	m_weaponType = initialType;

	const s_weapon_info *wi = get_weapon_info((tsweapon_t)m_weaponType);

	// Seed the arsenal with the placed/initial weapon type.
	TSItem &it = m_items[m_weaponType];
	it.owned = true;
	it.clip = wi->iClip;
	it.mode = TSDefaultFireMode(wi->iFireModes);
	AddAmmo(wi->iCaliber, wi->iClip * 3); // seed the caliber's reserve (SLICE: buy menu sets this)
	m_fireMode = it.mode;
	m_iClip = wi->iClip; // SLICE: deploy loaded

	const char *world = wi->pszWorldModel ? wi->pszWorldModel : "models/w_weaponbox.mdl";
	SET_MODEL(ENT(pev), world);

	m_iDefaultAmmo = wi->iClip; // SLICE: stock ammo; TS runs its own ammo system
	UpdateActivePointers();
	FallInit();
}

// Number of owned weapon types in the arsenal.
int CTSGun::OwnedCount() const
{
	int n = 0;
	for (int t = 0; t < TSW_COUNT; t++)
		if (m_items[t].owned)
			n++;
	return n;
}

// Park the live active clip/mode back into its item (HolsterWeapon side).
void CTSGun::SyncActiveToItem()
{
	if (m_weaponType < 0 || m_weaponType >= TSW_COUNT)
		return;
	TSItem &it = m_items[m_weaponType];
	it.clip = m_iClip;
	it.mode = m_fireMode;
	it.holdPosition = m_bHoldPosition;
	it.pumpToggle = m_fInSpecialReload; // the rack/barrel step SURVIVES a switch (holster clears
	                                    // reloading+burstOrPhase but NOT this byte, @0x85698)
}

// Load the active item's clip/mode into the live state (Deploy side).
void CTSGun::LoadActiveFromItem()
{
	if (m_weaponType < 0 || m_weaponType >= TSW_COUNT)
		return;
	const TSItem &it = m_items[m_weaponType];
	m_iClip = it.clip;
	m_fireMode = it.mode;
	m_bHoldPosition = it.holdPosition;
	m_burstOrPhase = 0; // holster cleared the item's burst/pump-phase slot (@0x85698)
	m_fInSpecialReload = it.pumpToggle; // restore the parked rack/barrel step
}

// Refresh the cached active-weapon pointers (the original's gun+0xcc/0xd0) from m_weaponType. Call
// after any m_weaponType change. m_pActiveItem is NULL for kung-fu (TSW_MELEE) — matching the
// original's `m_pActiveItem != NULL` "a real weapon is wielded" guard.
void CTSGun::UpdateActivePointers()
{
	m_pActiveInfo = get_weapon_info((tsweapon_t)m_weaponType);
	m_pActiveItem = (m_weaponType >= 0 && m_weaponType < TSW_COUNT && m_weaponType != TSW_MELEE)
		? &m_items[m_weaponType] : nullptr;
}

// @ TakeWeapon__6CTSGun (@0x80a04) — add a weapon to the arsenal (fresh world pickup / buy),
// then wield it. The faithful akimbo OWNERSHIP model (reference/WEAPON_SELECT.md):
//  - own the single + pick up another of the same single -> fill the AKIMBO item (the combine):
//    items[akimbo].leftClip = items[single].clip, rightClip = weapon_info[single].clip, the
//    single item is NOT cleared (both stay owned: scroll skips the single, slot1-5 still picks it).
//  - a fresh akimbo (fire-type 4) reads its single base; a gold-colts only-akimbo (fire-type 5)
//    splits its clip into two barrels; a normal single/weapon just loads its full clip.
// Sum the inventory slots the carried weapons occupy (the original tracks remaining = CBasePlayer+0x548;
// we derive used from the owned set). Drives encumbrance: maxspeed + the pickup capacity gate.
int CTSGun::SlotsUsed() const
{
	int used = 0;
	for (int t = 0; t < TSW_COUNT; t++)
		if (m_items[t].owned)
		{
			const s_weapon_info *wi = get_weapon_info((tsweapon_t)t);
			// STACKABLES (knife/sealknife/m61) charge slotCost PER UNIT — the original's
			// PickUpWeapon multiplies the deduction by the stack's clip (@0x8314c) and
			// MultiplyClips funds added units from free slots (@0x71308).
			int units = 1;
			if (wi->iStackable)
			{
				const int count = (t == m_weaponType) ? m_iClip : m_items[t].clip;
				units = (count > 1) ? count : 1;
			}
			used += wi->slotCost * units;
		}
	return used;
}

bool CTSGun::TakeWeapon(tsweapon_t type, const TSItem *pStatus, int iAttach)
{
	if (type < 0 || type >= TSW_COUNT)
		return false;
	const s_weapon_info *wi = get_weapon_info(type);
	if (!wi->pszViewModel)
		return false; // invalid weapon (e.g. the unused flag slot 29)

	// NB (@0x70a04, decompile-exact): the third arg is an ATTACHMENT MASK (BuyWeapon passes the
	// bought silencer/laser/flash/scope bits; the ground weapon its spawnflag attachments) —
	// and TakeWeapon grants NO reserve ammo AT ALL. The reserve comes ONLY from BuyAmmo (the
	// loadout, fill-to-cap in free mode), ammo packs, ground ammo, and the ground weapon's
	// empty-reserve top-up. Our old iClip*3 grant here was an invention (the "18/58 vs the
	// original 18/120" mismatch).

	// The active item's live clip lives in m_iClip; park it so the combine/merge reads the
	// current rounds when the affected slot is the one we're holding.
	SyncActiveToItem();

	int fill = type; // the item slot we will fill
	if (m_items[type].owned)
	{
		// The STACKABLE merge (knife/sealknife/m61): an owned stack gains exactly ONE unit per
		// take (@0x70b9b -> +1), gated on the carry cap — wi->iPellets (+0x24: knife 5,
		// sealknife 10, m61 2) (@0x70a8e). Slot cost charges per unit through SlotsUsed.
		if (wi->iStackable)
		{
			TSItem &it = m_items[type];
			if (it.clip >= wi->iPellets)
				return false; // the stack is at its carry cap
			it.clip += 1;
			if (type == m_weaponType)
				m_iClip = it.clip;
			return true;
		}
		// If it upgrades to an akimbo we don't own yet, fill the akimbo (the combine — the ONE
		// other case picking up an owned weapon is allowed). Otherwise it's a duplicate: refuse
		// (leave the world weapon on the floor).
		if (wi->iAkimboType >= 0 && wi->iAkimboType < TSW_COUNT &&
			get_weapon_info((tsweapon_t)wi->iAkimboType)->pszViewModel && !m_items[wi->iAkimboType].owned)
		{
			fill = wi->iAkimboType;
		}
		else
		{
			return false;
		}
	}
	else
	{
		// A fresh single REFUSES while its akimbo is already owned (@0x70a41 — no third copy).
		if (wi->iAkimboType >= 0 && wi->iAkimboType < TSW_COUNT && m_items[wi->iAkimboType].owned)
			return false;
		// The encumbrance gate: refuse it if it won't fit the 81 slots (GetSpeedBySlots
		// capacity). Akimbo COMBINES above bypass this (you already carry the single).
		if (SlotsUsed() + wi->slotCost > 81)
			return false;
	}

	// --- fill items[fill] ---
	const s_weapon_info *fwi = get_weapon_info((tsweapon_t)fill);
	TSItem &it = m_items[fill];

	if (pStatus && fill == type)
	{
		// a world pickup carrying its full parked state (clip/mode/attach/barrels/hold)
		it = *pStatus;
		it.owned = true;
		it.attach &= fwi->attachSupported; // @0x70d43 — clamp to what the gun can mount
		return true;
	}

	it.owned = true;
	it.mode = TSDefaultFireMode(fwi->iFireModes);
	it.attach = iAttach | fwi->attachBuiltin; // bought bits + the integral ones (@0x70c41)
	it.active = 0;
	it.barrelMask = 0;

	if (fwi->iFireType == TSFT_AKIMBO && fwi->iSingleType >= 0 && fwi->iSingleType < TSW_COUNT)
	{
		// combinable akimbo: right barrel = a fresh single's full clip; left barrel = the single
		// you already hold (its current clip), or a full clip when bought directly.
		const int single = fwi->iSingleType;
		const int singleClip = get_weapon_info((tsweapon_t)single)->iClip;
		it.leftClip = m_items[single].owned ? m_items[single].clip : singleClip;
		it.rightClip = singleClip;
		it.clip = it.leftClip + it.rightClip;
		// the combine UNIONS the attach/active masks across BOTH slots (@0x70c52-0x70c89)
		const int attachUnion = iAttach | it.attach | m_items[single].attach | fwi->attachBuiltin;
		it.attach = attachUnion;
		m_items[single].attach = attachUnion;
		const int activeUnion = it.active | m_items[single].active;
		it.active = activeUnion;
		m_items[single].active = activeUnion;
	}
	else if (fwi->iFireType == TSFT_AKIMBO_ONLY)
	{
		// gold colts: two barrels, each half the full clip.
		it.leftClip = fwi->iClip / 2;
		it.rightClip = fwi->iClip / 2;
		it.clip = fwi->iClip;
	}
	else
	{
		it.clip = fwi->iClip; // a fresh gun comes with one full clip — nothing else
	}

	it.attach &= fwi->attachSupported; // @0x70d43 — clamp to the mountable set
	// attached laser/flash start ACTIVE (@0x70d55-0x70d76)
	if (it.attach & 2)
		it.active |= 2;
	if (it.attach & 4)
		it.active |= 4;

	// NO auto-wield here — PickUpWeapon selects per its bSelect arg (@0x83318), so e.g.
	// recovering a thrown knife does NOT switch you off your gun.
	return true;
}

// @ AddAmmo__6CTSGun (@0x808ac) — add `count` rounds to caliber's reserve, capped at the
// caliber max. Reserves are per-caliber (gun+0xd4) and shared across same-caliber weapons.
bool CTSGun::AddAmmo(int caliber, int count)
{
	if (caliber <= 0 || caliber >= TS_NUM_CALIBERS || count <= 0)
		return false;
	int max = TSAmmoMax(caliber);
	if (m_ammoReserve[caliber] >= max)
		return false;
	m_ammoReserve[caliber] += count;
	if (m_ammoReserve[caliber] > max)
		m_ammoReserve[caliber] = max;
	return true;
}

// @ MultiplyClips__6CTSGun (@0x71308, decompile-exact) — the ammo (infinite-clip) powerup.
// Gates: a wielded item that isn't the m61. Interrupts any reload (StopReload), then:
//  - STACKABLE (knife/sealknife — wi+0xdc): ADD units up to 2x the default carry (wi->iPellets),
//    FUNDED by the player's free inventory slots (slotCost per unit; speed recomputed);
//  - normal: clip = 2x the max; an AKIMBO splits it across the barrels and mirrors HALF into its
//    single base's parked clip; a SINGLE with an akimbo variant adds half into the akimbo's parked
//    clip (leftClip = the single's clip).
bool CTSGun::MultiplyClips()
{
	if (!m_pActiveItem || !m_pActiveInfo)
		return false;
	if (m_weaponType == TSW_M61)
		return false;
	StopReload();
	const s_weapon_info *wi = m_pActiveInfo;
	TSItem &it = *m_pActiveItem;
	if (wi->iStackable)
	{
		const int wantSlots = (wi->iPellets * 2 - m_iClip) * wi->slotCost;
		const int freeSlots = TSPlayerFreeSlots(m_pPlayer);
		const int take = (wantSlots < freeSlots) ? wantSlots : freeSlots;
		if (wi->slotCost > 0 && take > 0)
		{
			m_iClip += take / wi->slotCost; // the free-slot deduction is implicit: stackables
			it.clip = m_iClip;              // charge slotCost PER UNIT in SlotsUsed
			m_pPlayer->SetPlayerSpeed(GetSpeedBySlots(m_pPlayer));
		}
	}
	else
	{
		it.clip = wi->iClip * 2;
		if (wi->iAkimboType == -1)
		{
			if (wi->iFireType == TSFT_AKIMBO || wi->iFireType == TSFT_AKIMBO_ONLY)
			{
				it.leftClip = (int)(0.5f * it.clip + 0.5f);
				it.rightClip = (int)(0.5f * it.clip + 0.5f);
				if (wi->iSingleType != -1 && wi->iSingleType != m_weaponType)
					m_items[wi->iSingleType].clip = (int)(0.5f * it.clip + 0.5f);
			}
		}
		else
		{
			m_items[wi->iAkimboType].clip += (int)(0.5f * it.clip + 0.5f);
			m_items[wi->iAkimboType].leftClip = it.clip;
		}
		m_iClip = it.clip;
	}
	return true;
}

// Per-tsweapon_t draw sequence in the v_*.mdl (the anim block entry [2], baked from the model
// sequence names — the original looks it up at init into the weapon anim set, item+0x28 +4).
static const int s_drawSeq[TSW_COUNT] = {
	1, 2, 4, 2, 5, 2, 4, 2, 2, 4, 5, 2, 2, 2, 2, 1, 2, 1, 2, 2,
	5, 2, 2, 1, 1, 2, 7, 2, 3, 2, 1, 3, 1, 1, 1, 1, 1, 1};

// @ HolsterWeapon__6CTSGun (@0x85698, decompile-exact: dossiers/cont81/holster.c) — holster the
// wielded weapon TO BARE HANDS: strip the view/weapon models, leave any zoom, drop the blade
// 3rd-person idle override, stamp the switch delay (m_flNextAttack +0.5) + idle +3.0, KILL any
// armed knife/katana whiff-lunge (the whiff-clear-on-switch), park the active item's transient
// state (grenade -> GrenadeIdle(1) put-away; else the burst/phase + reload bytes), reset to
// kung-fu (type 0, active pointers cleared), present the kung-fu viewmodel (draw anim + draw.sc
// FEV_NOTHOST iparam1=0), and zero the kung-fu combo counters. SelectWeapon calls this, then the
// caller Deploy()s the incoming weapon (so every switch carries the kungfu draw on the wire,
// exactly like the original).
void CTSGun::HolsterWeapon()
{
	SyncActiveToItem(); // park the live clip/fire-mode (the original mutates the item in place)
	m_pPlayer->pev->viewmodel = 0;
	m_pPlayer->pev->weaponmodel = 0;
#ifndef CLIENT_DLL
	Unzoom();
#endif
	m_pPlayer->m_szAnimOverride[0] = 0; // player+0x478 (+0xb58 twin byte) — drop the blade idle override
	m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 0.5f; // player+0x12c — the switch delay
	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 3.0f;

	// an armed whiff-lunge dies with the switch (only the knife/katana flavors, +0x7a4 != 0).
	if (m_pPlayer->m_flWhiffLungeTime != 0.0f && m_pPlayer->m_iWhiffLungeType != 0)
	{
		m_pPlayer->m_flWhiffLungeTime = 0;
		m_pPlayer->m_flWhiffLungeForce = 0;
		m_pPlayer->m_iWhiffLungeType = 0;
	}

	if (m_pPlayer && !m_pPlayer->IsAlive())
		return;

	if (m_pActiveItem && m_pActiveInfo)
	{
		if (m_pActiveInfo->iFireType == TSFT_GRENADE)
		{
			GrenadeIdle(1); // a cooked grenade is THROWN by the switch (the put-away force path)
		}
		else
		{
			m_fInReload = false;  // item+0xc
			m_burstOrPhase = 0;   // item+0x8 (burst / pump-phase / cook — one slot)
		}
	}

	m_weaponType = TSW_MELEE; // bare hands
	UpdateActivePointers();
	LoadActiveFromItem();
	const s_weapon_info *kf = get_weapon_info(TSW_MELEE);
	m_pPlayer->pev->viewmodel = MAKE_STRING(kf->pszViewModel); // v_melee
	SendWeaponAnim(s_drawSeq[TSW_MELEE]);                      // kungfu anim block [2] (draw)
	PLAYBACK_EVENT_FULL(FEV_NOTHOST, m_pPlayer->edict(), m_usDraw, 0.0, g_vecZero, g_vecZero,
		0.0, 0.0, m_weaponType, 0, 0, 0);
	m_kickCombo = 0;
	m_comboStep = 0;
	m_lastMeleeAnim = 0;
}

// @ SelectWeapon__6CTSGun (@0x80fb8) + the ClientCommand commit (@0xae433): holster the
// current weapon (HolsterWeapon — resets to bare hands), set the new active type, then deploy.
// Switching therefore cancels a reload and re-plays the draw animation/delay. Type 0 (kung-fu)
// is always selectable; any other type must be owned (the .so gates on items[type].owned).
void CTSGun::SelectWeapon(int type)
{
	if (type < 0 || type >= TSW_COUNT || type == m_weaponType)
		return;
	if (type != TSW_MELEE && !m_items[type].owned)
		return;

	const int outgoing = m_weaponType;
	HolsterWeapon();           // @0x85698 — park state + reset to bare hands (kungfu draw on the wire)
	m_lastType = outgoing;     // remember the outgoing type (for `lastinv`)

	m_weaponType = type;
	UpdateActivePointers();
	LoadActiveFromItem();

	// --- Deploy (@0x81030): re-deploy the new active weapon (draw anim + deploy delay). ---
	// The tsgun is the player's ONE weapon entity, so it is (or should be) the active item; if
	// nothing is active yet (e.g. the bare kung-fu spawn the engine's auto-deploy skips), claim
	// it so the viewmodel shows + ItemPostFrame runs (firing). Otherwise only redeploy if active.
	if (m_pPlayer && (m_pPlayer->m_pActiveItem == (CBasePlayerItem *)this || !m_pPlayer->m_pActiveItem))
	{
		m_pPlayer->m_pActiveItem = (CBasePlayerItem *)this;
		Deploy();
	}
}

// @ GetBetterWeapon__6CTSGun (@0x80f40) — after a drop, auto-select the highest-priority
// OWNED weapon (g_iWeaponPriority order), skipping `exclude` (the just-dropped type). Kung-fu
// (priority-last, always owned) is the fallback. Holster->deploy via SelectWeapon.
void CTSGun::GetBetterWeapon(int exclude)
{
	for (int i = 0; i < TSW_COUNT; i++)
	{
		int t = g_iWeaponPriority[i];
		if (t == exclude || !IsOwned(t))
			continue;
		SelectWeapon(t);
		return;
	}
}

// @ Precache__6CTSGun — faithful precaches 11 event scripts. Models for the whole
// roster are precached at map load (CWorld::Precache -> PrecacheAllTSWeapons), so
// a runtime give of any weapon type finds them cached (no precache-in-spawn error).
void CTSGun::Precache()
{
	// roster models/sounds/events are precached at map load (CWorld::Precache);
	// this re-call just resolves the cached indices for a runtime give.
	PrecacheAllTSWeapons();
	m_usFire = PRECACHE_EVENT(1, "events/fire.sc"); // predicted fire (sound/flash/anim + shell)
	m_usDraw = PRECACHE_EVENT(1, "events/draw.sc"); // deploy: seed the client active-weapon param cache
	m_usMelee = PRECACHE_EVENT(1, "events/knife.sc"); // melee impact spark/decal on a hard surface
	m_usKatana = PRECACHE_EVENT(1, "events/katana.sc"); // katana swing presentation (gun+0x974)
	m_usGrenade = PRECACHE_EVENT(1, "events/grenade.sc"); // pin/throw presentation (gun+0x978)
	m_usColdCock = PRECACHE_EVENT(1, "events/coldcock.sc"); // the bash presentation (gun+0x966)
	m_usCloseCombat = PRECACHE_EVENT(1, "events/closecombat.sc"); // kung-fu/block presentation (gun+0x96c)
	m_usEmpty = PRECACHE_EVENT(1, "events/empty.sc");   // dry-fire click (gun+0x968)
	m_usPump = PRECACHE_EVENT(1, "events/pump.sc");     // the rack half (gun+0x96a)
	m_usFireLinked = PRECACHE_EVENT(1, "events/firelinked.sc"); // akimbo both-barrels (gun+0x96e)
	m_usReload = PRECACHE_EVENT(1, "events/reload.sc"); // reload presentation (gun+0x970)
	PRECACHE_SOUND("player/kungfuhit.wav");   // kung-fu flesh hit (also the coldcock bash)
	PRECACHE_SOUND("player/closecombat.wav"); // kung-fu swing whoosh (miss / surface)
	PRECACHE_SOUND("player/block.wav");       // kung-fu block "hooah" (reload)
	PRECACHE_SOUND("player/breathe2.wav");    // exhausted breathing (block spam, stamina empty)
}

// @ GetItemInfo__6CTSGun — recovered: slot 1, pos 2, id 16, weight 1.
bool CTSGun::GetItemInfo(ItemInfo *p)
{
	const s_weapon_info *wi = get_weapon_info((tsweapon_t)m_weaponType);
	p->pszName = STRING(pev->classname);
	p->pszAmmo1 = "9mm"; // SLICE
	p->iMaxAmmo1 = 200;  // SLICE
	p->pszAmmo2 = NULL;
	p->iMaxAmmo2 = -1;
	p->iMaxClip = wi->iClip;
	p->iSlot = 1;
	p->iPosition = 2;
	p->iFlags = 0;
	p->iId = m_iId = TSGUN_WEAPON_ID;
	p->iWeight = TSGUN_WEIGHT;
	return true;
}

// Client-prediction state shipping (step 3c). The generic GetWeaponData loop (client.cpp /
// hl_weapons.cpp) already mirrors m_iClip + the timers + m_fInReload; here we add the TS-specific
// active state the prediction needs: the active weapon TYPE (drives the anim/fire-type the client
// runs) + the active caliber's spare RESERVE (drives the reload gate) + the fire-mode. Packed into
// the spare iuser fields (CTSGun doesn't use the base m_chargeReady/m_fInAttack/m_fireState those
// normally carry). Server packs in GetWeaponData; client unpacks in SetWeaponData before ItemPostFrame.
void CTSGun::GetWeaponData(weapon_data_t &data)
{
	const s_weapon_info *wi = get_weapon_info((tsweapon_t)m_weaponType);
	const int caliber = wi->iCaliber;
	data.iuser1 = m_weaponType;
	data.iuser2 = (caliber > 0 && caliber < TS_NUM_CALIBERS) ? m_ammoReserve[caliber] : 0;
	data.iuser3 = m_fireMode;
	data.iuser4 = m_bHoldPosition ? 1 : 0; // hold-position stance, so the client predicts the held-stance fire anim
	data.fuser4 = m_flNextAlternateAttack; // gun+0xa4 — the +alt2 gate rides the spare fuser4
	// The REAL per-barrel clips ride the spare fuser2/3 (CTSGun never uses the stock
	// m_flStartThrow/m_flReleaseThrow they normally carry; our GetWeaponData runs AFTER the
	// generic loop, so these win). Without them the client rebuilt the barrels BALANCED from
	// the combined clip every frame — so the predicted gun never saw a dry barrel: free-mode
	// fire kept flaring/banging client-side while the server refused, the predicted dry-click
	// never fired, and the plain-semi alternation desynced (the round-6 playtest bug).
	const TSItem &it = m_items[m_weaponType];
	data.fuser2 = (float)it.leftClip;
	data.fuser3 = (float)it.rightClip;
	// The ATTACH MASK rides the spare m_iWeaponState (unused by CTSGun):
	// the PREDICTED weapon's anims carry the attachment BODY and its predicted fire event
	// carries iparam2 — without this the client instance ran attach 0 (round-7i: the mask
	// landed server-side, yet no submodels and no silenced sound — both were the HOST's
	// predicted copies).
	data.m_iWeaponState = it.attach;
}

void CTSGun::SetWeaponData(const weapon_data_t &data)
{
	m_weaponType = data.iuser1;
	UpdateActivePointers();
	m_fireMode = data.iuser3;
	m_bHoldPosition = (data.iuser4 != 0);
	m_flNextAlternateAttack = data.fuser4;
	const s_weapon_info *wi = get_weapon_info((tsweapon_t)m_weaponType);
	const int caliber = wi->iCaliber;
	if (caliber > 0 && caliber < TS_NUM_CALIBERS)
		m_ammoReserve[caliber] = data.iuser2;

#ifdef CLIENT_DLL
	// The per-item ARSENAL state (m_items[]) is server-only and NOT in the prediction data — only
	// type/reserve/mode/holdpos travel (iuser1..4) + the base m_iClip. Anything a PREDICTED code
	// path gates on must be reconstructed here or the host's predicted events silently vanish
	// (FEV_NOTHOST means the server copy skips the host — the "coldcock went silent" bug: its
	// playback gates on item->owned, which stayed false client-side).
	// The wielded item is by definition OWNED:
	if (m_pActiveItem)
		m_pActiveItem->owned = true;
	// The REAL per-barrel clips (fuser2/3 — packed by our GetWeaponData). The old balanced
	// reconstruction from m_iClip is GONE: it fed the prediction half-full barrels every frame,
	// which is exactly why free-mode fire never dry-clicked and the alternation desynced.
	{
		TSItem &it = m_items[m_weaponType];
		it.leftClip = (int)data.fuser2;
		it.rightClip = (int)data.fuser3;
		it.attach = data.m_iWeaponState; // the wielded item's attachment mask (see GetWeaponData)
	}
#endif
}

// Per-tsweapon_t viewmodel DRAW (unholster) sequence index — recovered by parsing each viewmodel's
// sequence names (the "draw"/"drawb" sequence). The original looks this up at init into the weapon
// anim set (item+0x28 +4); we bake the result. Passed to DefaultDeploy so switching to a weapon plays
// its unholster animation. Index = tsweapon_t.
// (s_drawSeq moved above HolsterWeapon — the anim block entry [2] per weapon.)

bool CTSGun::Deploy()
{
	const s_weapon_info *wi = get_weapon_info((tsweapon_t)m_weaponType);
	if (!wi->pszViewModel)
		return false;
	// NB: do NOT reset m_iClientAttach here to force a re-broadcast — it re-fired on every deploy and
	// broke the fresh-buy laser. The map-change loss was the CLIENT clearing its g_playerAttach array on
	// VidInit; that clear was removed (cl_dll/ts_attachments.cpp) so the array stays in sync with this
	// change-detect cache across a level change. See [[map-change-state-reset]].
	// Faithful Deploy: view/player model via DefaultDeploy + the attachment BODY via
	// GetBodyByItems below (the weapon-param message ride-along is WeaponInfo, sent by
	// UpdateClientData's change-detect).
	// Kung-fu (TSW_MELEE) has NO 3rd-person weapon model — it's bare hands, so the player must
	// hold NOTHING (empty weaponmodel) and just play the kung-fu body animations. Falling back to
	// the VIEW model here put v_melee on the player's back/hands (a phantom held weapon). For any
	// real weapon with no p_model we likewise show nothing rather than the viewmodel.
	// @0x71030: the p_ model = the SILENCED variant iff the silencer is attached (wi+0x1b2).
	const char *pmodel = wi->pszPlayerModel ? wi->pszPlayerModel : "";
	if ((m_items[m_weaponType].attach & 1) != 0 && wi->pszPlayerModelSil)
		pmodel = wi->pszPlayerModelSil;
	const int drawSeq = (m_weaponType >= 0 && m_weaponType < TSW_COUNT) ? s_drawSeq[m_weaponType] : 0;
	// Kung-fu (TSW_MELEE) uses an EMPTY anim-ext (grip) — the original keys the bare-hands
	// combat_idle stance + the close-combat attack set off m_szAnimExtention being empty
	// (no invented m_bKungFu bool). Real weapons carry onehanded/twohanded/akimbo.
	const char *animExt = (m_weaponType == TSW_MELEE) ? "" : (wi->pszAnimExt ? wi->pszAnimExt : "onehanded");
	// @0x710c4: the draw anim CARRIES the attachment body — DefaultDeploy's 5th arg reaches the
	// viewmodel via the weaponanim message (pev->body alone never does; the missing-silencer/
	// scope-submodel bug).
	bool ok = DefaultDeploy((char *)wi->pszViewModel, (char *)pmodel, drawSeq, (char *)animExt,
		GetBodyByItems(m_weaponType, m_items[m_weaponType].attach));
	// @0x710c4: the deployed viewmodel BODY comes from the EQUIPPED attachment mask through
	// GetBodyByItems (silencer/laser/flash/scope submodels; the client draw.sc handler runs
	// the same math via FUN_10007110).
	pev->body = GetBodyByItems(m_weaponType, m_items[m_weaponType].attach);
	// player+0x478 3rd-person body-anim OVERRIDE (RE DefaultDeploy@0x90178, weapon_tsgun switch on m_weaponType):
	// the dedicated melee idles. CTSAnimation::Animate plays this directly when non-empty (else idle_/aim_<grip>).
	const char *ov = (m_weaponType == TSW_KNIFE || m_weaponType == TSW_SEALKNIFE) ? "knife_idle"
		: (m_weaponType == TSW_KATANA) ? "katana_idle"
		: (m_weaponType == TSW_FLAG) ? "briefcase_idle" : "";
	strncpy(m_pPlayer->m_szAnimOverride, ov, sizeof(m_pPlayer->m_szAnimOverride) - 1);
	m_pPlayer->m_szAnimOverride[sizeof(m_pPlayer->m_szAnimOverride) - 1] = 0;
	if (ok)
		m_pPlayer->m_TSAnim.Animate(m_pPlayer, TSA_DRAW); // 3rd-person draw_<grip>

	// events/draw.sc → EV_TSDraw: seed the CLIENT active-weapon param cache for the predicted
	// fire/reload handlers (iparam1 = type, iparam2 = active barrel mask: akimbo 2/4/6, else 1).
	// Additive — the deploy view/anim above stays server-driven; this only primes client state.
	if (ok && m_usDraw)
	{
		const int barrelMask = (m_items[m_weaponType].barrelMask & 6) ? (m_items[m_weaponType].barrelMask & 6) : 1;
		PLAYBACK_EVENT_FULL(0, m_pPlayer->edict(), m_usDraw, 0.0, g_vecZero, g_vecZero,
			0.0, 0.0, m_weaponType, barrelMask, 0, 0);
	}
	return ok;
}

// @ SecondaryAttack__6CTSGun @0x717a8 — the right-click dispatcher (decompile-exact). Bare hands ->
// kung-fu secondary; grenade/knife write the item's alt-mode byte (item+0x24 = 1) then share the
// primary's fire function; the katana has its own KatanaThrow; the flag (fire-type 11) does nothing.
// The akimbo fire-modes route right-click to Fire() — the OTHER hand shoots (mode 4 alternating needs
// a fresh press unless the double-fire powerup); everything else cycles the scope when one is equipped
// (attach & 8), otherwise toggles the hold-position stance, then re-arms SECONDARY +0.3 and idle +1.0
// (the original leaves m_flNextPrimaryAttack alone here — our old inline block had the two swapped).
void CTSGun::SecondaryAttack()
{
	if (m_weaponType == TSW_MELEE) // original: m_pActiveItem == NULL (bare-hands kung-fu)
	{
		CCombatAttack(true);
		return;
	}
	const s_weapon_info *wi = get_weapon_info((tsweapon_t)m_weaponType);
	TSItem &it = m_items[m_weaponType];

	if (wi->iFireType == TSFT_GRENADE)
	{
		it.altMode = 1; // item+0x24 = 1 -> the UNDERHAND roll (GrenadeIdle's release branch)
		GrenadeFire();
		return;
	}
	if (wi->iFireType == TSFT_KNIFE)
	{
		it.altMode = 1; // item+0x24 = 1 -> KnifeFire's THROW branch (@0x717d3)
		KnifeFire();
		return;
	}
	if (wi->iFireType == TSFT_KATANA)
	{
		KatanaThrow();
		return;
	}
	if (wi->iFireType == TSFT_FLAG)
		return; // @0x717e6: fire-type 11 has no secondary

	if (m_fireMode == TSFM_AKIMBO_ALT) // "Free Semi": right click = the RIGHT barrel, per press
	{
		if (!(m_pPlayer->m_afButtonPressed & IN_ATTACK2) && !DoubleFireRate(m_pPlayer))
			return;
	}
	else if (m_fireMode != TSFM_AKIMBO_BOTH) // plain modes: an akimbo right-click is the HOLD
	                                          // toggle below (guns oriented facing each other),
	                                          // exactly like the singles
	{
		if (it.attach & 8)
		{
			// scope cycle: 0 (none) -> zoomFov1 -> zoomFov2 -> back to 0. The view FOV + viewmodel-hide
			// are player pev fields the server networks to the client (server-authoritative).
#ifndef CLIENT_DLL
			const float fov = m_pPlayer->pev->fov;
			if (fov == 0.0f)
			{
				if (wi->zoomFov1 != 0)
					Zoom(wi->zoomFov1);
			}
			else if (wi->zoomFov2 == 0 || (float)wi->zoomFov2 == fov)
				Unzoom();
			else
				Zoom(wi->zoomFov2);
#endif
		}
		else if (wi->iHoldChange >= 0)
		{
			// hold-position stance toggle (wi+0x28c != -1): play the change/rechange transition and
			// re-point the active anim set (the original swaps item+0x28 between info+0x252/+0x28c).
			m_bHoldPosition = !m_bHoldPosition;
			it.holdPosition = m_bHoldPosition;
			SendWeaponAnim(m_bHoldPosition ? wi->iHoldChange : wi->iHoldRechange);
		}
		m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + 0.3f; // dc_0_3 @LAB_000819f3
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 1.0f;      // dc_1
		return;
	}
	// the FREE modes (4 "Free Semi" / 5 "Free Full"): right click fires the RIGHT barrel
	m_items[m_weaponType].altMode = 1;
	Fire();
}

// @ PrimaryAttack__6CTSGun -> Fire -> {Standard,Shotgun,Knife,Katana,Grenade,Akimbo}Fire.
// Faithful dispatch by the weapon's fire-type, using the recovered per-weapon
// rate/spread (StandardFire reads s_weapon_info+0x18 rate / +0x28 spread / +0x348
// fire-type). SLICE: the faithful path dispatches on the dual-wield items-array
// (m_pActiveItem/m_pActiveInfo) — here we dispatch on the single held m_weaponType.
// Melee/grenade/akimbo split out to MeleeAttack/GrenadeFire/AkimboFire.
void CTSGun::PrimaryAttack()
{
	const s_weapon_info *wi = get_weapon_info((tsweapon_t)m_weaponType);

	// Kung-fu (TSW_MELEE) = bare hands. The original has NO active weapon item here (gun+0xcc==0)
	// and routes PrimaryAttack -> CCombatAttack (@0x86078): a contextual punch/kick chosen from the
	// movement buttons + flags. Knife/katana are real weapon items (their own fire-types below).
	if (m_weaponType == TSW_MELEE)
	{
		CCombatAttack(false);
		return;
	}

	// Fire-types dispatched directly (outside the fire-mode trigger system, like the
	// original PrimaryAttack @0x81a20): melee hull-traces and the grenade throw.
	if (wi->iFireType == TSFT_KNIFE)
	{
		m_items[m_weaponType].altMode = 0; // @0x71a89: item+0x24 = 0 (slash) before KnifeFire
		KnifeFire();
		return;
	}
	if (wi->iFireType == TSFT_KATANA)
	{
		KatanaFire(); // @0x71aaf: fire-type 10 has its OWN fire fn (never through KnifeFire)
		return;
	}
	if (wi->iFireType == TSFT_GRENADE)
	{
		m_items[m_weaponType].altMode = 0; // @0x71a61: item+0x24 = 0 (overhand) before GrenadeFire
		GrenadeFire();
		return;
	}

	// Bullet/akimbo/pellet weapons: the current FIRE-MODE gates the trigger (the fire
	// *function* is still chosen by fire-type inside Fire()). m_pActiveItem+2 in the .so.
	const bool held = (m_pPlayer->pev->button & IN_ATTACK) != 0;
	const bool pressed = (m_pPlayer->m_afButtonPressed & IN_ATTACK) != 0;
	bool fire = false;

	// DOUBLE-FIRE powerup: EVERY weapon goes full-auto — fire while the trigger is held, regardless of
	// the weapon's fire-mode (so a semi pistol or a burst SMG hoses while held). Overrides the per-mode
	// trigger logic below entirely.
	if (DoubleFireRate(m_pPlayer))
	{
		fire = held;
	}
	else
	switch (m_fireMode)
	{
	case TSFM_AUTO:        // full-auto (an akimbo in plain AUTO fires BOTH barrels at once — the uzis)
	case TSFM_PUMP:        // pump shotgun: fire while HELD (orig PrimaryAttack mode 3 = `button & 1`) — but
	                       // ShotgunFire ALTERNATES shot/rack per held frame, so you don't full-auto bullets.
	case TSFM_AKIMBO_BOTH: // "Free Full" — primary = the LEFT barrel, full-auto while held
		fire = held;
		break;
	case TSFM_SEMI:
		// one round per pull (semi; an akimbo in plain SEMI ALTERNATES barrels per pull).
		fire = pressed;
		break;
	case TSFM_AKIMBO_ALT: // "Free Semi" — primary = the LEFT barrel, one round per pull
		fire = pressed;
		break;
	case TSFM_BURST:
		// 3-round burst (2 for the MAC-10): start on a fresh pull, then run the queue
		// down (here + in WeaponIdle) so a tap fires the whole burst.
		if (m_burstOrPhase > 0)
			fire = true;
		else if (pressed)
		{
			m_burstOrPhase = (m_weaponType == TSW_MAC10) ? 2 : 3;
			fire = true;
		}
		break;
	}

	if (fire)
	{
		// the FREE akimbo modes route the trigger to a BARREL: primary (left click) = the LEFT
		// barrel (item+0x24 = 0); SecondaryAttack sets 1 for the right (user-confirmed model).
		if (wi->iFireType == TSFT_AKIMBO || wi->iFireType == TSFT_AKIMBO_ONLY)
			m_items[m_weaponType].altMode = 0;
		Fire();
	}
}

// @ Fire__6CTSGun (@0x71c78, decompile-exact) — fire ONE shot, dispatching by fire-type (the
// trigger gating happens in PrimaryAttack/WeaponIdle). Fully underwater = the dry click; an empty
// clip clicks too UNLESS the InfiniteClips powerup or the pump's non-firing RACK half (toggle set,
// pump mode, pump gun — it must run even empty, to rack on the last shell). 3rd-person body pose:
// the AKIMBO plays 8+(barrel side) and only if it actually fired; pump/standard play TSA_FIRE(0).
// (cont.83: the 8/9 pose was on the PUMP here and TSA_FIRE on the akimbo — the binary has it the
// other way around.)
void CTSGun::Fire()
{
	if (!m_pActiveItem)
		return;
	if (m_pPlayer->pev->waterlevel == 3)
	{
		EmptySound(); // @0x71caf — guns don't fire underwater
		return;
	}
	const s_weapon_info *wi = m_pActiveInfo;
	const bool pumpRackStep = (m_fInSpecialReload != 0 && m_fireMode == TSFM_PUMP &&
		wi->iFireType == TSFT_PUMP);
	if (!InfiniteClips(m_pPlayer) && m_iClip < 1 && !pumpRackStep)
	{
		EmptySound(); // @0x71cfc — the 0.1s re-arm happens inside
		return;
	}

	int shootanim = -1;
	UTIL_MakeVectors(m_pPlayer->pev->v_angle);
	if (wi->iFireType == TSFT_AKIMBO || wi->iFireType == TSFT_AKIMBO_ONLY)
	{
		if (AkimboFire(wi))
			shootanim = 8 + (m_fInSpecialReload != 0 ? 1 : 0); // the fired barrel's shoot pose
	}
	else
	{
		if (wi->iFireType == TSFT_PUMP)
			ShotgunFire(wi);
		else
			StandardFire(wi);
		shootanim = TSA_FIRE;
	}
	if (shootanim >= 0)
		m_pPlayer->m_TSAnim.Animate(m_pPlayer, shootanim); // direct Animate (SetAnimation is a TS stub)
}

// @ StandardFire/ShotgunFire — a single bullet (or a pump-shotgun pellet spread), with the
// weapon's real damage/spread/rate. Burst rounds fire fast then slow on the last (the .so
// uses 0.03 / 0.2; StandardFire @0x824a4).
void CTSGun::StandardFire(const s_weapon_info *wi)
{
	const int cPellets = wi->iPellets > 0 ? wi->iPellets : 1; // 8 for shotguns (s_weapon_info+0x24)
	float flSpread = wi->flSpread > 0 ? wi->flSpread : 0.01f;
	// TS dynamic accuracy (RE StandardFire @0x824a4): for weapons flagged bUsesSpread (s_weapon_info+0x2c —
	// the firearms; shotguns/melee fire at the static base cone), grow the per-player spread accumulator and
	// fold it into this shot's cone, so sustained fire walks the spread out (it recovers in PreThink).
	// silenced = attach bit0 present and NOT integral (an MP5SD's built-in silencer doesn't count)
	const bool sfSilenced = (m_items[m_weaponType].attach & 1) != 0 && (wi->attachBuiltin & 1) == 0;
	if (wi->bUsesSpread)
	{
		// @0x82638/0x82674: the recoil increment is x5 while MOVING faster than 175 u/s
		const float recoil = (m_pPlayer->pev->velocity.Length() > 175.0f) ? 5.0f * wi->flRecoil
		                                                                  : wi->flRecoil;
		ScaleSpread(flSpread, recoil, sfSilenced);
	}

	m_iClip--;
	m_pPlayer->pev->effects |= EF_MUZZLEFLASH; // 3rd-person muzzleflash on the player model
	// (the 3rd-person shoot_ body pose is set by CTSGun::Fire's direct Animate call — RE Fire@0x71c78 — not
	// here; TS's SetAnimation is an empty stub, so we don't route through it.)

	// Viewmodel "shoot" animation — now CLIENT-PREDICTED (CTSGun is a UseDecrement weapon, cont.69s
	// 3d): SendWeaponAnim runs in the client's predicted ItemPostFrame, so the anim plays instantly.
	// In the hold-position stance the fire anim comes from the held shoot seq (the "b"/"c"/"2" set).
	const int fireAnim = (m_bHoldPosition && wi->iHoldFireAnim >= 0) ? wi->iHoldFireAnim : wi->iFireAnim;
	if (fireAnim >= 0)
		SendWeaponAnim(fireAnim);

	// events/fire.sc → EV_TSFire: predicted fire sound + brass shell + viewmodel muzzleflash. Played
	// with FEV_NOTHOST so the predicting host plays its OWN predicted copy (and isn't sent the server
	// copy too — otherwise the muzzle/shell would double); other clients get the server copy.
	// Arg packing is byte-faithful to the original StandardFire@0x824a4 pfnPlaybackEvent:
	//   iparam1 = weaponType, iparam2 = ATTACH MASK (active item+0x10; bit0 = silencer),
	//   bparam1 = (clip == 0).  The silencer bit drives the client's fire-sil sound + muzzle
	//   suppression (EV_TSFire). Barrel side is always 0 here (non-akimbo); we carry it in bparam2
	//   for the merged akimbo path (see AkimboFire) — our extension, not read by the original client.
	// Recoil magnitude — computed HERE (predicted-side, where the fire-mode is known) and carried to the
	// client aim-recoil via the event's fparam2 (EV_TSFire calls TS_AddViewRecoil with it). The REAL player's
	// recoil is the client-side aim kick, NOT the server VKickback (which the original gates to bots) — so the
	// pump-shotgun 0.35x reduction has to ride along to the client. Pump shotguns kick LESS: the original
	// ShotgunFire@0x82108 scales view-kick to 0.35x when the weapon has an auto/semi mode bit (iFireModes & 3)
	// AND it's not in semi mode — so the dual-mode benelli/spas (mask 0xa) get 0.35x IN PUMP MODE (full in
	// semi), while the pump-ONLY mossberg (mask 0x8, no &3 bit) keeps the full kick.
	float flViewKick = wi->flViewKick;
	if (wi->iFireType == TSFT_PUMP && (wi->iFireModes & 3) != 0 && m_fireMode != TSFM_SEMI)
		flViewKick *= 0.35f;
	// bparam1 = (clip==0) for the standard classes (the akimbo classes read it as the barrel);
	// bparam2 = the FireBullets SEED (the original client's pellet pattern reads it).
	PLAYBACK_EVENT_FULL(FEV_NOTHOST, m_pPlayer->edict(), m_usFire, 0.0, g_vecZero, g_vecZero,
		flSpread, flViewKick, m_weaponType, m_items[m_weaponType].attach, (m_iClip == 0) ? 1 : 0,
		m_pPlayer->random_seed);

	Vector vecSrc = m_pPlayer->GetGunPosition();
	Vector vecAiming = gpGlobals->v_forward;
#ifndef CLIENT_DLL
	// @0x72759 (the StandardFire call site): (iPellets, src, aim, (s,s,s), flDamage,
	// wi->iKnifeType (0 for firearms), iCaliber, random_seed). damagemult applies INSIDE
	// (cheat-gated) — the old outer TSDamageMult was double-applying it.
	// @0x8256c: a (non-integral) silencer costs damage — x0.775
	m_pPlayer->ShootBulletsPlayer((unsigned char)cPellets, vecSrc, vecAiming,
		Vector(flSpread, flSpread, flSpread),
		sfSilenced ? wi->flDamage * 0.775f : wi->flDamage, wi->iKnifeType,
		wi->iCaliber, m_pPlayer->random_seed);
#else
	// Client prediction: spread/recoil only (damage + decals are server-authoritative).
	m_pPlayer->FireBulletsPlayer(cPellets, vecSrc, vecAiming,
		Vector(flSpread, flSpread, flSpread), 8192, BULLET_PLAYER_9MM,
		0, (int)(wi->flDamage * TSDamageMult()), m_pPlayer->pev, m_pPlayer->random_seed);
#endif

	// Server-side view-punch (RE StandardFire: VKickback when weaponinfo+0x38 != 0 — all firearms). Uses the
	// same flViewKick computed above (pump 0.35x folded in). NB: VKickback's punchangle only affects BOTS; real
	// players get the client aim-recoil via the event fparam2 above.
	VKickback(flViewKick, wi->iFireType);

	float flRate;
	if (m_fireMode == TSFM_BURST)
	{
		// one burst round consumed; fast cadence between rounds, slow after the last.
		if (m_burstOrPhase > 0)
			m_burstOrPhase--;
		flRate = (m_burstOrPhase <= 0) ? 0.2f : 0.03f;
	}
	else
	{
		flRate = wi->flFireRate > 0 ? wi->flFireRate : 0.15f;
		// Double-firerate powerup is really 1.5x, NOT 2x (RE PostThink @0x81610: when player+0x724 & 0x10,
		// the weapon-timer decrement is *= 1.5 — so timers drain 1.5x faster). We apply it to flRate here
		// (Fire is client-predicted, so this stays in sync). It also makes semis fire-while-held (above).
		if (DoubleFireRate(m_pPlayer))
			flRate /= 1.5f;
	}
	m_flNextPrimaryAttack = (UTIL_WeaponTimeBase() + flRate);
	// Short idle during a burst so WeaponIdle drives the remaining rounds; else normal.
	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() +
		((m_fireMode == TSFM_BURST && m_burstOrPhase > 0) ? flRate : 5.0f);
}

// @ ShotgunFire__6CTSGun (@0x72108) — the pump shotgun's primary attack. Unlike StandardFire (one shot per
// trigger event), this ALTERNATES per trigger-frame: a FIRE-step (a real pellet shot) then a RACK-step (the
// pump animation + sound, no bullet), each gated by the fire rate. So holding the trigger gives
// shot→rack→shot→rack at half the raw rate — and you visibly pump between shots. The original keys the
// alternation off m_pActiveItem+0x24; we keep it in m_fInSpecialReload (predicted — see ts_gun.h) so the
// client and server stay in step (a non-predicted toggle desyncs → the host fires every frame = infinite-fire).
//
// The fire-step delegates to StandardFire (identical pellet/spread/muzzle/event/recoil/clip/rate logic). The
// rack-step plays the viewmodel "pump" sequence (parsed from the models: v_m3/v_spas12 "pump"=8,
// v_mossberg "pump"=2) + the per-weapon rack sound, and re-arms the fire timer at the same rate.
void CTSGun::ShotgunFire(const s_weapon_info *wi)
{
	// A shell-by-shell reload in progress: a fire press interrupts it — close the pump (phase 2), no shot.
	// (orig: `if (item+8 != 0) { item+8 = 2; return; }`.) ItemPostFrame's ShotgunReload then plays the close.
	if (m_burstOrPhase != 0)
	{
		m_burstOrPhase = 2;
		return;
	}

	// RACK half of the cycle: pump mode (3) with the toggle set. No bullet, no shell consumed.
	// The WHOLE presentation rides events/pump.sc (EV_TSPump: the block[13/14] anim, the ejected
	// shell, and the rack sound — wi+0x306, played at ent -1 so it never channel-cuts the still-
	// playing gunshot; that's the original's trick our CHAN_ITEM hack approximated). RE: all 3
	// pump shotguns DEFAULT to PUMP mode (TakeWeapon: AUTO>PUMP>BURST>SEMI) — held = auto
	// fire/rack alternation, a tap = one step.
	if (m_fireMode == TSFM_PUMP && m_fInSpecialReload != 0)
	{
		float flRate = wi->flFireRate > 0 ? wi->flFireRate : 0.15f;
		if (DoubleFireRate(m_pPlayer))
			flRate /= 1.5f;
		m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + flRate;
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 2.0f;

		PLAYBACK_EVENT_FULL(FEV_NOTHOST, m_pPlayer->edict(), m_usPump, 0.0, g_vecZero, g_vecZero,
			0.0, 0.0, m_weaponType, m_pActiveItem ? m_pActiveItem->attach : 0, 0, 0);
		m_fInSpecialReload = 0; // next trigger-frame is a FIRE-step
		return;
	}

	// FIRE half of the cycle: a normal shotgun shot (pellets/muzzle/event/recoil/clip + the rate gate).
	StandardFire(wi);
	// Arm the rack for the next trigger-frame — but only in PUMP mode (in SEMI mode a benelli just fires per
	// pull, no auto-rack). Matches the original toggling item+0x24 each ShotgunFire.
	m_fInSpecialReload = (m_fireMode == TSFM_PUMP) ? 1 : 0;
}

// @ ScaleSpread__6CTSGun (@0x802a8) — TS dynamic accuracy. Folds the per-player spread accumulator
// (CBasePlayer::m_flSpreadAccum, the .so's player+0x778) into THIS shot's cone, then grows the accumulator
// for the next shot — so sustained fire walks the spread out. The accumulator recovers over time in
// CBasePlayer::PreThink. `increment` = the weapon's flRecoil (s_weapon_info+0x34) — x5 while MOVING
// faster than 175 u/s, x2 for the akimbo LINKED shot (the callers fold those in).
// ⚠ ARG IDENTITY (round-6 RE): the 4th arg is the SILENCED flag (attach bit0 present and NOT
// integral — both original call sites @0x82674/@0x82ad4 compute exactly that), NOT ducking as we
// had mislabeled — a SILENCER tightens the base cone x0.875 and the growth x0.775.
void CTSGun::ScaleSpread(float &spread, float increment, bool silenced)
{
	if (silenced)
		spread *= 0.875f;                 // silenced -> tighter base cone
	if (increment <= 0.0f)
		return;                           // weapon has no dynamic recoil
	float factor = 2.0f;
	if (DoubleFireRate(m_pPlayer)) factor *= 0.5f;    // bullet-time fires softer
	if (silenced)                  factor *= 0.775f;
	const float dt = gpGlobals->time - m_flLastShotTime; // time since this gun last fired
	if (dt >= 0.001f)
	{
		spread += m_pPlayer->m_flSpreadAccum;            // effective spread = base + accumulator
		float recovery = m_pPlayer->m_flSlowMotionGo * dt; // GiveSlowMul(player) * dt
		if (recovery >= 1.0f) recovery = 0.5f;           // RE: cap the per-shot recovery at 0.5
		float accum = (1.0f - recovery) * increment * factor + m_pPlayer->m_flSpreadAccum;
		if (accum > 0.1f) accum = 0.1f;                  // RE: capped at 0.1
		m_pPlayer->m_flSpreadAccum = accum;
	}
	m_flLastShotTime = gpGlobals->time;
}

// @ VKickback__6CTSGun (@0x70414) — the firearm recoil view-punch. Kicks pev->punchangle, which the engine
// renders onto the view and decays back over time — so the aim jolts up like a mouse nudge and the player
// pulls down to compensate (spraying walks the aim). Vertical = 0.85*magnitude (magnitude = wi->flViewKick,
// s_weapon_info+0x30); the horizontal sign/size is per fire-type. Ducking and bullet-time (DoubleFireRate)
// damp it. (The original gates the SERVER apply on FL_FAKECLIENT — real players predict it client-side; we
// apply it directly so the kick shows on the listenserver. Client-prediction of the punch is a TODO.)
void CTSGun::VKickback(float magnitude, int firetype)
{
	float vert = 0.85f * magnitude;
	float horiz;
	switch (firetype)
	{
	case 8:         horiz = vert *  0.35f; break; // pistol (semi)
	case 4: case 5: horiz = vert *  0.15f; break; // akimbo / akimbo-only
	case 1:         horiz = vert * -0.25f; break; // pump shotgun
	case 6: case 7: horiz = vert * -0.30f; break; // autoshotgun / sniper
	case 9:         horiz = vert * -0.35f; break; // auto SMG
	default:        horiz = 0.0f;          break;
	}
	if (m_pPlayer->pev->flags & FL_DUCKING) { vert *= 0.775f;  horiz *= 0.775f; }  // ducked → tighter
	if (DoubleFireRate(m_pPlayer))          { vert *= 0.6667f; horiz *= 0.6667f; } // bullet-time → softer
	// RE @0x70414: the punchangle is applied ONLY to FL_FAKECLIENT (bots). Real players' recoil is the
	// angle-based aim kick (client-side), NOT punchangle — so this is effectively a no-op for human players.
	if (m_pPlayer->pev->flags & FL_FAKECLIENT)
	{
		m_pPlayer->pev->punchangle.x -= vert;  // kick the (bot's) view UP
		m_pPlayer->pev->punchangle.y += horiz; // and sideways per the recoil pattern
	}
}

// @ KnifeFire__6CTSGun @0x74044 — the knife/sealknife SLASH and THROW in one method, split on the
// item's alt-mode byte (item+0x24: PrimaryAttack writes 0 = slash, SecondaryAttack 1 = throw).
// No button/timer gates here (ItemPostFrame + the dispatcher gate); both attack timers re-arm at
// flFireRate (x1.5 stunting) so holding either button auto-repeats. SLASH: 3rd-person anim 29-31
// random, dmg x(2/3 stunting) x GetMeleeMultiplier, CheckHull hull-3 at 35u, VictimKnifeHold
// deflect, DMG_SLASH|DMG_NEVERGIB, player hit bloodies the blade + victim-edict knife_hitbody.wav,
// miss arms the whiff-lunge; knife.sc plays EVERY slash (FEV_NOTHOST, iparam1=weaponType,
// iparam2=bloody — ALL swing presentation is client-side, no server whoosh, no SendWeaponAnim).
// THROW: wall-clipped eye trace spawn, CreateKnife @1900, roll 90 / spin -1300, anim 32, server
// knife_miss.wav whoosh, consume gated !InfiniteClips; 0 knives -> DropWeapon(type, true), else a
// model-derived redraw + draw.sc + idle 2.0.
void CTSGun::KnifeFire()
{
	if (m_weaponType == TSW_MELEE) // original gate: m_pActiveItem == NULL
		return;
	entvars_t *ppev = m_pPlayer->pev;
	TSItem &it = m_items[m_weaponType];
	const s_weapon_info *wi = get_weapon_info((tsweapon_t)m_weaponType);

	const bool stunting = (ppev->iuser4 & 0x70c30) != 0;
	float rate = wi->flFireRate;
	if (stunting)
		rate *= 1.5f;
	m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + rate;
	m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + rate;
	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 3.0f;
	m_pPlayer->m_flLastMeleeRate = rate; // player+0x4f8
	UTIL_MakeVectors(ppev->v_angle);

	bool consumed = false;

	if (it.altMode == 0)
	{
		// ---- SLASH ----
		const int r = RANDOM_LONG(0, 2);
		if (!stunting && (ppev->flags & FL_ONGROUND))
			m_pPlayer->m_TSAnim.Animate(m_pPlayer, TSA_CC_KNIFE1 + r); // body anims 29-31

		float dmg = wi->flDamage;
		if (stunting)
			dmg *= 2.0f / 3.0f;

#ifndef CLIENT_DLL // hit resolution is server-authoritative (house pattern)
		dmg *= m_pPlayer->GetMeleeMultiplier();
		Vector vecSrc = m_pPlayer->GetGunPosition();
		Vector vecEnd = vecSrc + gpGlobals->v_forward * 35.0f; // slash reach 35, hull-3 sweep
		TraceResult tr;
		m_pPlayer->CheckHull(vecSrc, vecEnd, tr, 3);

		bool whiff = true;
		if (tr.flFraction < 1.0f)
		{
			CBaseEntity *pEnt = tr.pHit ? CBaseEntity::Instance(tr.pHit) : nullptr;
			if (pEnt && pEnt->IsAlive() && !(pEnt->pev->effects & EF_NODRAW))
			{
				// world/props take NO slash damage — only living, drawn victims resolve.
				whiff = false;
				if (!VictimKnifeHold(m_pPlayer, pEnt, dmg))
				{
					ClearMultiDamage();
					pEnt->TraceAttack(ppev, dmg, gpGlobals->v_forward, &tr, DMG_SLASH | DMG_NEVERGIB); // 0x1004
					ApplyMultiDamage(ppev, ppev);
					if (pEnt->IsPlayer())
					{
						it.bloody = 1; // item+0x8 — bloody knife (rides knife.sc as iparam2)
						pev->body = 1; // bloody w_/p_ submodel
						EMIT_SOUND_DYN(ENT(pEnt->pev), CHAN_BODY, "weapons/knife/knife_hitbody.wav",
							VOL_NORM, 0.8f, 0, 95 + RANDOM_LONG(0, 31)); // from the VICTIM's edict
					}
				}
			}
		}
		if (whiff)
		{
			// miss (or a non-living hit): arm the whiff-lunge — knife flavor.
			m_pPlayer->m_flWhiffLungeTime = 0.25f * wi->flFireRate;
			m_pPlayer->m_flWhiffLungeForce = dmg;
			m_pPlayer->m_iWhiffLungeType = 1;
		}
#endif
		// knife.sc EVERY slash — the whoosh/impact/viewmodel presentation is client-side.
		PLAYBACK_EVENT_FULL(FEV_NOTHOST, m_pPlayer->edict(), m_usMelee, 0.0, g_vecZero, g_vecZero,
			0.0, 0.0, m_weaponType, it.bloody, 0, 0);
		// the slash never touches the clip (consumed stays false).
	}
	else
	{
		// ---- THROW ----
		Vector eyes = ppev->origin + ppev->view_ofs;
		TraceResult tr;
		UTIL_TraceLine(eyes + gpGlobals->v_right * 1.0f,
			eyes + gpGlobals->v_forward * 32.0f + gpGlobals->v_right * 1.0f,
			dont_ignore_monsters, ENT(ppev), &tr);

		CBaseEntity *pKnife = TSThrowKnife(m_pPlayer, tr.vecEndPos, gpGlobals->v_forward * 1900.0f,
			wi->iKnifeType); // server-only (client stub returns NULL)
		if (pKnife)
		{
			Vector dir = pKnife->pev->velocity;
			dir = (dir.Length() > 0.0f) ? dir.Normalize() : g_vecZero;
			pKnife->pev->angles = UTIL_VecToAngles(dir);
			pKnife->pev->angles.z = 90.0f;       // blade rolled edge-on
			pKnife->pev->avelocity.x = -1300.0f; // end-over-end spin (KatanaThrow uses -700)
		}
		consumed = true; // even if CreateKnife failed (the original's merge point @0x74865)

		if (!(ppev->iuser4 & 0x30c30) && (ppev->flags & FL_ONGROUND))
			m_pPlayer->m_TSAnim.Animate(m_pPlayer, TSA_CC_KNIFELAUNCH); // throw anim 32

		EMIT_SOUND_DYN(ENT(ppev), CHAN_BODY, "weapons/knife/knife_miss.wav",
			VOL_NORM, 0.8f, 0, 95 + RANDOM_LONG(0, 31)); // the throw whoosh — always
	}

	if (consumed)
	{
		if (!InfiniteClips(m_pPlayer))
		{
			// the thrown knife frees its carry slots + the move speed recomputes. (Original:
			// player+0x548 += slotCost by hand; our slots derive from the owned items, so the
			// recompute only shifts on the LAST knife's DropWeapon below — VERIFY per-knife cost.)
			m_iClip--;
			m_pPlayer->SetPlayerSpeed(GetSpeedBySlots(m_pPlayer));
		}
		if (m_iClip <= 0)
		{
			DropWeapon(m_weaponType, true); // threw the last one — leaves the arsenal, next-best deploys
		}
		else
		{
			// redraw the next knife: model-derived draw seq + draw.sc + a short idle.
			SendWeaponAnim(s_drawSeq[m_weaponType]);
			PLAYBACK_EVENT_FULL(FEV_NOTHOST, m_pPlayer->edict(), m_usDraw, 0.0, g_vecZero, g_vecZero,
				0.0, 0.0, m_weaponType, it.attach, 0, 0);
			m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 2.0f;
		}
	}
}

// @ CCombatAttack__6CTSGun (@0x86078) — kung-fu (bare hands) contextual close-combat. The original
// reads the player's movement BUTTONS + flags + speed (it has no active weapon item, gun+0xcc==0) and
// dispatches one of ~13 attack ids, each with its own anim / damage / refire / forward-lunge. This is the
// RE-faithful port (entvars decoded: button@0x180 → IN_ATTACK 1 / IN_DUCK 4 / IN_BACK 0x10 / IN_ATTACK2
// 0x800; flags byte@0x1a5 → FL_ONGROUND 0x200 / FL_DUCKING 0x4000; iuser4@0x250 = TS lock-state mask
// 0x50c30). Two PERSISTENT combo counters drive the chains: PUNCH (m_comboStep / orig this+0x9fc) = 3 jabs
// → uppercut; KICK (m_kickCombo / orig this+0xa00) = 3 kicks → spin finisher. Both reset only on a finisher/
// dive (NOT on a time gap — the original has no chaining window). The last-attack id (m_lastMeleeAnim /
// this+0xa04) makes each chain cycle without repeating. Damage (local_114): punches 35, kicks 45, back-kick
// 55. Kung-fu is FULL-AUTO (the standard ItemPostFrame holds fire → repeats at the refire cap). Refire =
// local_108 (NOT local_10c — Ghidra mis-attributed the load; the boundary-confused frame analysis shifted
// it 4 bytes): normal punch 0.25 (= DAT_00107e98, live-confirmed ~4/sec full-auto cap), uppercut 0.65,
// air-punch 0.5; basic kick 0.3, spin finisher 0.5, duckkick/sidekick-dive 1.0, back-kick 1.35 (1.0 on
// ground), flying kick 0.75. local_10c (0.1-0.35) is the VICTIM block-stun (BlockPlayer(player, *0.75)).
//
// The original kung-fu viewmodel carries ~26 sequences (ids 0xa..0x19); our v_melee.mdl has 11, so each
// original id is remapped to the nearest v_melee seq + the player-model 3rd-person kung-fu seq. The
// DISPATCH / DAMAGE / TIMING are byte-faithful; the anim is a remap (see the ledger note on the 11-seq model).
//
// SLICE (documented, not yet ported): the forward WHIFF-LUNGE (orig local_110 → player+0x79c, a decaying
// PreThink velocity push applied on a miss, scaled by GetMeleeMultiplier + doubled by the KungFu powerup)
// is a separate PreThink subsystem; the per-move magnitude is kept as a comment. The iuser4 lock-state
// (0x50c30 → forced light attack) and the player+0x814/0x815 state bits (special-punch 0x17 / forced kick
// finisher) default to "clear" — the common path. v_melee anims: 2 punch_left, 3 punch_right, 4 block_low,
// 5 kick, 6 sidekick, 7 uppercut, 8 spinkick, 9 backkick, 10 punch_right2.
void CTSGun::CCombatAttack(bool fSecondary)
{
	if (m_flNextPrimaryAttack > UTIL_WeaponTimeBase())
		return;

	entvars_t *ppev = m_pPlayer->pev;
	const int btn = ppev->button;
	const bool onGround = (ppev->flags & FL_ONGROUND) != 0;
	const bool ducking  = (ppev->flags & FL_DUCKING) != 0;  // fully crouched (flags byte 0x1a5 & 0x40)
	const bool duckBtn  = (btn & IN_DUCK) != 0;            // duck key held (button 0x180 & 4)
	const float speed   = ppev->velocity.Length();

	// forward/back of the current movement (orig local_118 = back?3:2). Slow → the +back key; fast →
	// compare velocity to facing (dot <= -0.5 = running backward; > 0.5 = running forward).
	bool movingBack, movingFwd = false;
	if (speed <= 100.0f)
	{
		movingBack = (btn & IN_BACK) != 0;
	}
	else
	{
		UTIL_MakeVectors(Vector(0.0f, ppev->v_angle.y, ppev->v_angle.z)); // orig builds from (0, yaw, roll)
		float fwdDot = DotProduct(gpGlobals->v_forward, ppev->velocity * (1.0f / speed));
		movingBack = fwdDot <= -0.5f;
		movingFwd  = fwdDot >  0.5f;
	}

	// original kung-fu viewmodel anim ids (0xa..0x19), remapped to v_melee seqs below.
	enum { A_SPIN_A=0x0a, A_GKICK_A=0x0b, A_GKICK_B=0x0c, A_SPIN_D=0x0d, A_SPIN_E=0x0e, A_SPIN_F=0x0f,
	       A_DROPKICK=0x10, A_BACKKICK=0x11, A_FLY_12=0x12, A_FLY_13=0x13, A_HANDSTAND=0x14,
	       A_PUNCH=0x16, A_PUNCH2=0x17, A_AIRPUNCH=0x18, A_UPPERCUT=0x19 };

	int oa;             // original attack id (drives m_lastMeleeAnim cycling)
	int jabIdx = -1;    // for the normal-punch chain, which of the 3 jabs (0/1/2)
	float dmg, range, refire;

	if (fSecondary)
	{
		// ===== KICKS (orig IN_ATTACK2 branch). First match wins. =====
		if (onGround && (duckBtn || ducking))
		{
			// duck-kick (HandleCloseCombat 0x14 "duckkick") — duck on the ground
			oa = A_HANDSTAND; dmg = 45; range = 64; refire = 1.0f; /*local_108; block-stun local_10c 0.35*/
		}
		else if (movingBack)
		{
			// back-kick — moving backward (the +back key when slow, velocity when fast)
			oa = A_BACKKICK; dmg = 55; range = 72; refire = onGround ? 1.0f : 1.35f; /*block-stun 0.3*/
		}
		else if (!onGround && speed > 270.0f && duckBtn && movingFwd)
		{
			// side-kick dive (0x10 "sidekick") — a fast forward dive + duck in the air
			oa = A_DROPKICK; dmg = 45; range = 72; refire = 1.0f; /*block-stun 0.25*/
			m_kickCombo = 0;
		}
		else if (onGround || speed <= 200.0f)
		{
			// ground kick combo: 3 basic kicks → a spin finisher (orig this+0xa00)
			if (m_kickCombo > 2) // (orig also forces this on player+0x815&8 — defaulted clear)
			{
				// spin finisher — cycle 0xa/0xe/0xd/0xf without repeating the last
				switch (m_lastMeleeAnim)
				{
				case A_SPIN_F: oa = A_SPIN_A; break;
				case A_SPIN_A: oa = A_SPIN_E; break;
				case A_SPIN_E: oa = A_SPIN_D; break;
				case A_SPIN_D: oa = A_SPIN_F; break;
				default: { int r = RANDOM_LONG(0, 3); oa = (r==0)?A_SPIN_E : (r==1)?A_SPIN_D : (r==2)?A_SPIN_A : A_SPIN_F; }
				}
				dmg = 45; range = 64; refire = 0.5f; /*block-stun 0.3*/
				m_kickCombo = 0;
			}
			else
			{
				// basic ground kick — alternate 0xb/0xc
				oa = (m_lastMeleeAnim == A_GKICK_A) ? A_GKICK_B
				   : (m_lastMeleeAnim == A_GKICK_B || RANDOM_LONG(0, 1) == 0) ? A_GKICK_A : A_GKICK_B;
				dmg = 45; range = 64; refire = 0.3f; /*block-stun 0.2*/
				m_kickCombo++;
			}
		}
		else
		{
			// flying / jump kick — fast + airborne, not the above. cycle 0x13/0x12/0xf
			switch (m_lastMeleeAnim)
			{
			case A_SPIN_F: oa = A_FLY_13; break;
			case A_FLY_13: oa = A_FLY_12; break;
			case A_FLY_12: oa = A_SPIN_F; break;
			default: { int r = RANDOM_LONG(0, 2); oa = (r==1)?A_FLY_12 : (r==0)?A_FLY_13 : A_SPIN_F; }
			}
			dmg = 45; range = 72; refire = 0.75f; /*block-stun 0.25*/
			m_kickCombo = 0;
		}
	}
	else
	{
		// ===== PUNCHES (orig IN_ATTACK branch). All deal 35. =====
		if (!onGround)
		{
			// air punch
			oa = A_AIRPUNCH; refire = 0.5f; /*block-stun 0.15*/
			m_comboStep = 0;
		}
		else if (m_comboStep > 2 || (duckBtn && !ducking))
		{
			// uppercut — combo finisher (3 punches thrown) OR a duck-TAP (key down, not yet crouched)
			oa = A_UPPERCUT; refire = 0.65f; /*block-stun 0.15*/
			m_comboStep = 0;
		}
		else
		{
			// normal punch combo — orig swaps to a "special punch" 0x17 on player+0x814&1 (defaulted
			// clear → always the normal chain). The single orig id 0x16 is shown as 3 cycling jabs.
			oa = A_PUNCH; refire = 0.25f; /*block-stun 0.1*/
			jabIdx = m_comboStep;
			m_comboStep++;
		}
		dmg = 35; range = 48;
	}

	m_lastMeleeAnim = oa;
	m_lastCCTime = gpGlobals->time;

	// The closecombat.sc wire args per original anim id (disasm slot-tracking @0x76078:
	// [esp+0x28] = the MOVE id iparam1, [esp+0x8] = the ANIM-BLOCK index iparam2 — the client
	// plays animBlock[idx] raw; block[3]=2, [4]=10, [5]=3, [6]=5, [7]=9, [8]=6, [9]=5, [10]=8,
	// [11]=7 on v_melee.mdl). The 3rd-person ACTIVITY code stays server-side (the [esp+0x18]
	// writes: punch 0x23, kicks 0x2d, backkick 0x37 families -> our TSA_CC_* codes).
	// NB the original sends anim 3 for EVERY basic punch — the 3-jab cycling was our invention.
	int ccMove, ccAnimIdx, bodyAct;
	switch (oa)
	{
	case A_PUNCH:     ccMove = 1; ccAnimIdx = 3;  bodyAct = TSA_CC_PUNCHLEFT; break; // @0x763e9
	case A_PUNCH2:    ccMove = 1; ccAnimIdx = 4;  bodyAct = TSA_CC_PUNCHAIR;  break; // @0x76436
	case A_AIRPUNCH:  ccMove = 1; ccAnimIdx = 5;  bodyAct = TSA_CC_PUNCHAIR;  break; // @0x76344
	case A_UPPERCUT:  ccMove = 1; ccAnimIdx = 11; bodyAct = TSA_CC_UPPERCUT;  break; // @0x76393
	case A_HANDSTAND: ccMove = 2; ccAnimIdx = 6;  bodyAct = TSA_CC_SIDEKICK;  break; // @0x764a3
	case A_BACKKICK:  ccMove = 3; ccAnimIdx = 7;  bodyAct = TSA_CC_BACKKICK;  break; // @0x76565
	case A_DROPKICK:  ccMove = 2; ccAnimIdx = 8;  bodyAct = TSA_CC_CIRCSPIN;  break; // @0x766b5
	case A_SPIN_A: case A_SPIN_D: case A_SPIN_E:
	                  ccMove = 2; ccAnimIdx = 10; bodyAct = TSA_CC_CIRCSPIN;  break; // @0x76912
	case A_GKICK_A: case A_GKICK_B:
	                  ccMove = 2; ccAnimIdx = 9;  bodyAct = TSA_CC_CIRCKICK;  break; // @0x76997
	case A_SPIN_F: case A_FLY_12: case A_FLY_13:
	default:          ccMove = 2; ccAnimIdx = 10; bodyAct = TSA_CC_CIRCSPIN;  break; // VERIFY: the
	                  // fly family's [esp+0x8] write is register-carried (not an immediate) —
	                  // 10 (the spin-kick anim) pending a focused pass
	}

	UTIL_MakeVectors(ppev->v_angle);
	Vector vecSrc = m_pPlayer->GetGunPosition();
	Vector vecEnd = vecSrc + gpGlobals->v_forward * range;

	// events/closecombat.sc — the client owns the 1st-person anim (animBlock[iparam2]), the
	// ALWAYS-whoosh, and the miss/surface presentation (texture note + smoke puff).
	PLAYBACK_EVENT_FULL(FEV_NOTHOST, m_pPlayer->edict(), m_usCloseCombat, 0.0, g_vecZero,
		g_vecZero, (float)m_weaponType, 0.0, ccMove, ccAnimIdx, 0, 0);
#ifdef CLIENT_DLL
	// Kung-fu "bounce-back" screen-shake, per punch/kick — ALWAYS fires on attack (RE client FUN_1000af10
	// @0xb5e5: Shake_Setup(amp = min(power, 45) * 0.35, 0.2, 10)). The original derives `power` from the
	// attack's velocity (a forward LUNGE the move imparts, *0.15385=1/6.5), so it's non-zero even standing
	// still. Live-verified amps: punch~7.0 (power~20), kick~8.75 (power~25). Transient render-only.
	// We don't model the per-move lunge → use the live-measured per-category power. fSecondary = the kicks.
	{
		float kfPower = fSecondary ? 25.0f : 20.0f;
		TS_TriggerViewShake(kfPower * 0.35f); // 0x1007a8a8 = 0.35 -> punch 7.0, kick 8.75
	}
#endif
	m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + refire;   // orig: both timers = base + local_10c
	m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + refire;
	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 5.0f;

#ifndef CLIENT_DLL
	// @0x76cf6: route the hit through the shared resolver CBasePlayer::CloseCombatAttack —
	// it does the 3rd-person anim, the TSMBlur roll, the CheckHull trace, validity/teammate
	// gates, the VictimKungfuHold deflect (incl. the counter role-swap), disarm-pressure,
	// >30-dmg knockback and the glast-marked TraceAttack. Hull: the backkick family (attack
	// class 3) uses the human hull (1), everything else the head hull (3).
	const int ccHull = (oa == A_BACKKICK) ? 1 : 3;
	CBaseEntity *pHit = m_pPlayer->CloseCombatAttack(vecSrc, vecEnd, gpGlobals->v_forward,
		ccHull, dmg * TSDamageMult(), bodyAct, true, false, true, false);

	// tail per @0x76c8c: attacking closes your own block window; the last melee refire is stored.
	// DEFERRED (VERIFY L7): the per-move whiff-lunge arm on miss (+0x79c/0x7a0, negated for the
	// backkick) and the rootFlag BlockPlayer(refire*0.75) self-lock — the per-move table is
	// un-RE'd; the lunge consumer is stunt-cluster scope. closecombat.sc playback lands in S7b.
	m_pPlayer->m_flKungFuBlock = 0.0f;
	m_pPlayer->m_flLastMeleeRate = refire;

	// (the miss/surface presentation — whoosh + texture note + smoke puff — now rides
	// events/closecombat.sc; EV_TSCloseCombat owns it)
#endif
}

// @ CCombatBlock__6CTSGun (@0x85ebc) — kung-fu "reload" = the block taunt. Plays the "hooah"
// @ CCombatBlock__6CTSGun — RELOAD with ANY melee weapon (kung-fu/knife/katana/seal-knife) =
// the block taunt: opens the per-weapon 0.5s deflect window, drains 33 stamina via
// CBasePlayer::UpdateMeleeBar (which handles the depleted 1.5s lockout + the KFuPower send),
// and gates attacks. RE-faithful gates: not mid-attack, not stunting (iuser4 & 0x70c30), bar
// must be non-empty (>= 0.01 — a LOW bar still blocks, then empties + locks out).
// The window is a VALUE 0.5 counted down in PreThink (addendum L2), floor 0.
// @ CCombatBlock__6CTSGun (@0x75ebc, decompile-exact) — the melee BLOCK. Gates: the player's
// m_flNextAttack AND all three weapon timers (primary/secondary/ALTERNATE) clear, not stunting
// (iuser4 & 0x70c30), melee bar >= 0.01. Opens the fire-type-matched block window (+0x7a8 bare /
// +0x7ac knife / +0x7b0 katana = 0.5s), then the CLIENT owns the presentation via
// events/closecombat.sc — iparam1 = 0 (BLOCK: the handler plays player/block.wav and returns),
// iparam2 = the anim-block index (1 bare hands / 0x12 armed), fparam1 = (float)weaponType.
// 3rd-person Animate(TSA_CC_BLOCK) ALWAYS (armed too); UpdateMeleeBar(33); primary+secondary
// +0.5, idle +3.0.
void CTSGun::CCombatBlock()
{
	if (m_pPlayer->m_flNextAttack > UTIL_WeaponTimeBase())
		return; // global attack gate (incl. the depleted-bar 1.5s lockout)
	if (m_flNextPrimaryAttack > UTIL_WeaponTimeBase() ||
		m_flNextSecondaryAttack > UTIL_WeaponTimeBase() ||
		m_flNextAlternateAttack > UTIL_WeaponTimeBase())
		return; // mid-attack / already blocking (all THREE gates, @0x75ee4)
	if ((m_pPlayer->pev->iuser4 & 0x70c30) != 0)
		return; // stunting (dive/roll/wall-jump family) — no block
	if (m_pPlayer->m_flMeleeBar < 0.01f)
		return; // empty bar — nothing to block with

	// Open the fire-type-matched BLOCK window + pick the event's anim index.
	int blockIdx;
	if (!m_pActiveItem)
	{
		m_pPlayer->m_flKungFuBlock = 0.5f;
		blockIdx = 1; // the bare-hands block anim slot
	}
	else
	{
		if (m_pActiveInfo && m_pActiveInfo->iFireType == TSFT_KATANA)
			m_pPlayer->m_flKatanaBlock = 0.5f;
		else if (m_pActiveInfo && m_pActiveInfo->iFireType == TSFT_KNIFE)
			m_pPlayer->m_flKnifeBlock = 0.5f;
		blockIdx = 0x12; // the armed block anim slot
	}

	// events/closecombat.sc — the block presentation (the client plays block.wav + block[idx]).
	PLAYBACK_EVENT_FULL(FEV_NOTHOST, m_pPlayer->edict(), m_usCloseCombat, 0.0, g_vecZero,
		g_vecZero, (float)m_weaponType, 0.0, 0, blockIdx, 0, 0);

	m_pPlayer->m_TSAnim.Animate(m_pPlayer, TSA_CC_BLOCK); // 3rd-person block pose (armed too)
#ifndef CLIENT_DLL
	m_pPlayer->UpdateMeleeBar(33.0f); // drain (handles depletion lockout + the KFuPower send; server-only)
#endif
	m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + 0.5f;
	m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 0.5f;
	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 3.0f;
}

// @ AlternateAttack__6CTSGun (@0x71ba8, decompile-exact) — the +alt2 (IN_CANCEL) input:
// `if (m_pActiveItem && iColdCockDamage > 0) ColdCock();`. NO timer gate here — the base
// ItemPostFrame dispatches it through CanAttack(m_flNextAlternateAttack) (gun+0xa4), which
// ColdCock re-arms (+0.725). (The old kept-VERIFY primary-timer gate is resolved: L9 closed.)
void CTSGun::AlternateAttack()
{
	if (!m_pActiveItem)
		return;
	const s_weapon_info *wi = get_weapon_info((tsweapon_t)m_weaponType);
	if (!wi || wi->iColdCockDamage <= 0)
		return;
	ColdCock();
}

// @ ColdCock__6CTSGun @0x71dd0 — a quick melee bash with the held weapon (CBasePlayer::
// ColdCockAttack@0x86610 in the original). Short hull-trace in front; on hit deals club
// damage. Reuses the knife melee sounds (no dedicated coldcock sound ships). SLICE: the
// faithful coldcock viewmodel anim rides events/coldcock.sc (not yet reconstructed); we
// drive the 3rd-person attack anim + sound server-side.
// @0x81dd0 ColdCock — the pistol-whip/rifle-butt wrapper: per-weapon damage from the table
// (iColdCockDamage, x2/3 while stunting), range 35, hull 3, routed through the
// ColdCockAttack twin with all four behavior bools FALSE. Cadence 0.725 (x1.5 stunting).
// Interrupts zoom/reload/grenade-idle. Attacker confirm on a player hit = kevlarhit.wav
// pitch 60-91. coldcock.sc fires even on a miss in the original — the event swap lands
// with the S7b protocol step (presentation kept server-side meanwhile).
void CTSGun::ColdCock()
{
	const s_weapon_info *wi = get_weapon_info((tsweapon_t)m_weaponType);
	if (!wi || wi->iColdCockDamage <= 0)
		return; // this weapon can't coldcock

	const bool stunting = (m_pPlayer->pev->iuser4 & 0x70c30) != 0;

	UTIL_MakeVectors(m_pPlayer->pev->v_angle);

	// @0x81e90: the bash INTERRUPTS what the weapon was doing — a cooked grenade is force-thrown
	// (the put-away path), any in-progress reload aborts.
	if (m_pActiveInfo)
	{
		if (m_pActiveInfo->iFireType == TSFT_GRENADE)
			GrenadeIdle(1);
		StopReload();
	}

	const float cadence = stunting ? 0.725f * 1.5f : 0.725f;
#ifndef CLIENT_DLL
	Unzoom();
#endif
	// ALL FOUR gates re-arm to the cadence (alternate/secondary/primary + the player's
	// m_flNextAttack), idle +2.0 (@0x81f0c).
	m_flNextAlternateAttack = UTIL_WeaponTimeBase() + cadence;
	m_flNextSecondaryAttack = m_flNextAlternateAttack;
	m_flNextPrimaryAttack = m_flNextAlternateAttack;
	m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + cadence;
	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 2.0f;

	float dmg = (float)wi->iColdCockDamage;
	if (stunting)
		dmg *= 2.0f / 3.0f;

	// events/coldcock.sc (@0x81f80, gated on the item being owned): the CLIENT owns the whole
	// presentation — the block[28] viewmodel anim, the ALWAYS-whoosh, the texture note + smoke
	// puff on a hit (EV_TSColdCock). iparam1=type, iparam2=attach; fparam1/2 carry the aim dir
	// x/y (unconsumed by the handler — kept for byte-faithfulness).
	if (m_pActiveItem && m_pActiveItem->owned)
		PLAYBACK_EVENT_FULL(FEV_NOTHOST, m_pPlayer->edict(), m_usColdCock, 0.0, g_vecZero,
			g_vecZero, gpGlobals->v_forward.x, gpGlobals->v_forward.y,
			m_weaponType, m_pActiveItem->attach, 0, 0);

	Vector vecSrc = m_pPlayer->GetGunPosition();
	Vector vecEnd = vecSrc + gpGlobals->v_forward * 35.0f;

#ifndef CLIENT_DLL
	CBaseEntity *pHit = m_pPlayer->ColdCockAttack(vecSrc, vecEnd, gpGlobals->v_forward,
		3, dmg * TSDamageMult(), stunting ? -1 : 39 /*anim 39, none while stunting*/,
		false, false, false, false);
	if (pHit && pHit->IsPlayer())
	{
		// attacker confirm — the kevlar "clonk" on CHAN_BODY, attn 0.8 (RE: pitch 60 + rand(0,31))
		EMIT_SOUND_DYN(ENT(m_pPlayer->pev), CHAN_BODY, "player/kevlarhit.wav", VOL_NORM,
			0.8f, 0, 60 + RANDOM_LONG(0, 31));
	}
	// (the miss whoosh now rides events/coldcock.sc — the client plays closecombat.wav on
	// EVERY bash, hit or miss)
#endif
}

#ifndef CLIENT_DLL

// @ CTSStunt::CheckForBreakables (@0x9bd9c) — the dive/run-THROUGH-breakables barge, called every
// PreThink. When moving fast (speed ≥ 64), trace a hull forward ALONG THE VELOCITY direction (64u)
// and damage whatever is there (damage = speed), so flying/running into glass shatters it (a clean
// dive-through, NO kick anim — distinct from the jump-kick) and into a person shoves them. RE'd
// constants: speed/reach 64 (DAT_00107ea0), grounded damage ×0.1 vs full airborne (DAT_00107138).
void TSCheckRunThrough(CBasePlayer *pPlayer)
{
	// AIRBORNE only — the original (CheckForBreakables @0x9bd9c, line ~46) returns early when the player
	// is ON GROUND, so only a mid-air dive barges through breakables. (This was the "walking up to glass
	// breaks it" bug — walking is grounded.)
	if ((pPlayer->pev->flags & FL_ONGROUND) != 0)
	{
		pPlayer->m_hRunThroughLast = NULL;
		return;
	}

	const float speed = pPlayer->pev->velocity.Length();
	if (speed < 64.0f) // must be moving fast enough (orig DAT_00107ea0)
	{
		pPlayer->m_hRunThroughLast = NULL;
		return;
	}

	// trace forward along the VELOCITY direction (orig velocity-mode), reach 64u, head hull.
	Vector dir = pPlayer->pev->velocity.Normalize();
	Vector src = pPlayer->pev->origin;
	Vector end = src + dir * 64.0f;

	TraceResult tr;
	UTIL_TraceHull(src, end, dont_ignore_monsters, head_hull, ENT(pPlayer->pev), &tr);
	if (tr.flFraction >= 1.0f)
		return;

	CBaseEntity *pHit = CBaseEntity::Instance(tr.pHit);
	if (!pHit || pHit == pPlayer || pHit->pev->takedamage == DAMAGE_NO || !pHit->IsAlive())
		return;
	if ((CBaseEntity *)pPlayer->m_hRunThroughLast == pHit)
		return; // already barged through this one (don't re-hit every frame)

	// damage = speed; reduced when grounded (running), full when airborne (diving) — orig 0.1 factor.
	const float dmg = (pPlayer->pev->flags & FL_ONGROUND) ? speed * 0.1f : speed;

	ClearMultiDamage();
	gMultiDamage.type = DMG_CLUB | DMG_NEVERGIB;
	pHit->TraceAttack(pPlayer->pev, dmg, dir, &tr, DMG_CLUB | DMG_NEVERGIB);
	ApplyMultiDamage(pPlayer->pev, pPlayer->pev);

	pPlayer->m_hRunThroughLast = pHit;
}
#endif // !CLIENT_DLL

// Reload the active weapon from its own per-slot reserve (the original draws from
// the per-caliber armament; here each arsenal slot carries its own spare rounds).
// @ GrenadeFire__6CTSGun @0x732f0 — PULL THE PIN (decompile-exact): gated on an idle phase
// (item+0x8 == 0) + a loaded grenade; plays the pin-pull viewmodel anim (anim block [19]) +
// grenade.sc iparam1=19, arms the COOK phase (item+0x8 = 1), re-arms both attacks + idle at
// +0.7, and stamps the cook member (gun+0xa0c = time+5; consumer VERIFY). NO fuse starts here
// — the grenade is thrown (fuse 3.0 FROM THE THROW) by GrenadeIdle when the button releases.
void CTSGun::GrenadeFire()
{
	if (m_weaponType == TSW_MELEE) // original: m_pActiveItem == NULL
		return;
	if (m_burstOrPhase != 0 || m_iClip <= 0) // item+0x8 phase busy / no grenade
		return;

	SendWeaponAnim(2); // m61 anim block [19] = seq 2 (pin pull; record+0x26)
	PLAYBACK_EVENT_FULL(FEV_NOTHOST, m_pPlayer->edict(), m_usGrenade, 0.0, g_vecZero, g_vecZero,
		0.0, 0.0, 19, 0, 0, 0);
	m_burstOrPhase = 1; // item+0x8 = COOKED (the pin is out)
	m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 0.7f;
	m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + 0.7f;
	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 0.7f;
	m_flGrenadeCook = gpGlobals->time + 5.0f; // gun+0xa0c (consumer VERIFY)
}

// @ GrenadeIdle__6CTSGuni @0x74f18 — the cook machine (decompile-exact). Phase 1 (cooked):
// while ATTACK or ATTACK2 is held (and not forced) keep cooking; on release THROW — pitch-shaped
// force (90 - shapedPitch)*7 capped 750; overhand (altMode 0) from the eyes at full force with
// avelocity.x=200; underhand (altMode 1) from the FEET at 2/3 force, friction 0.4,
// avelocity=(-100,600); fuse 3.0 FROM THE THROW; throw anim (block [20]/[21]) + grenade.sc
// iparam1 20/21 + body anim 0x35; timers +0.5; the last grenade strips the weaponmodel;
// phase -> 2 (or a forced throw consumes immediately). Phase 2: the post-throw tick consumes —
// slot refund + speed recompute, DropWeapon at 0 left, else redraw (block [2]) + idle 2.0.
// Phase 0: idle +3.0.
void CTSGun::GrenadeIdle(int fForce)
{
	if (m_weaponType == TSW_MELEE) // original: m_pActiveItem == NULL
		return;
	const s_weapon_info *wi = get_weapon_info((tsweapon_t)m_weaponType);
	TSItem &it = m_items[m_weaponType];
	bool consume = false;

	if (m_burstOrPhase == 1)
	{
		if ((m_pPlayer->pev->button & (IN_ATTACK | IN_ATTACK2)) && fForce == 0)
			return; // still cooking

		// pitch-shaped throw force: shaped = pitch*(up 1.111 / down 0.888) - 10;
		// force = (90 - shaped) * 7, capped 750 (aim up = harder throw).
		Vector ang = m_pPlayer->pev->v_angle;
		const float px = ang.x;
		ang.x = px * ((px < 0.0f) ? (float)(10.0 / 9.0) : (float)(8.0 / 9.0)) - 10.0f;
		float force = (90.0f - ang.x) * 7.0f;
		if (force > 750.0f)
			force = 750.0f;
		UTIL_MakeVectors(ang);

		Vector vecSrc, vecVel;
		if (it.altMode == 0)
		{
			// OVERHAND: from the eyes, full force + the thrower's velocity.
			vecSrc = m_pPlayer->pev->origin + m_pPlayer->pev->view_ofs + gpGlobals->v_forward * 16.0f;
			vecVel = gpGlobals->v_forward * force + m_pPlayer->pev->velocity;
		}
		else
		{
			// UNDERHAND roll: from the FEET, 2/3 force.
			vecSrc = m_pPlayer->pev->origin + gpGlobals->v_forward * 16.0f;
			vecVel = gpGlobals->v_forward * (force * (2.0f / 3.0f)) + m_pPlayer->pev->velocity;
		}

		if (wi->iKnifeType == 4) // info+0x20 == 4 — the m61 projectile-type code
		{
			CBaseEntity *pGren = TSThrowM61(m_pPlayer->pev, vecSrc, vecVel, 3.0f); // FUSE 3.0 from the throw
			if (pGren)
			{
				if (it.altMode == 0)
				{
					pGren->pev->avelocity.x = 200.0f; // overhand tumble
				}
				else
				{
					pGren->pev->friction = 0.4f;      // the roll
					pGren->pev->avelocity.y = 600.0f;
					pGren->pev->avelocity.x = -100.0f;
				}
			}
		}

		SendWeaponAnim(it.altMode == 0 ? 3 : 4); // m61 block [20]/[21] (record+0x28/+0x2a)
		PLAYBACK_EVENT_FULL(FEV_NOTHOST, m_pPlayer->edict(), m_usGrenade, 0.0, g_vecZero, g_vecZero,
			0.0, 0.0, (it.altMode != 0) + 0x14, 0, 0, 0); // iparam1 = 20 overhand / 21 underhand
		m_pPlayer->m_TSAnim.Animate(m_pPlayer, 0x35); // 3rd-person grenade throw

		m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 0.5f;
		m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + 0.5f;
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 0.5f;
		if (m_iClip == 1)
			m_pPlayer->pev->weaponmodel = 0; // the last grenade leaves the hand
		if (fForce == 0)
			m_burstOrPhase = 2; // the next idle tick consumes
		else
		{
			m_burstOrPhase = 0;
			consume = true; // forced (holster) throw consumes NOW
		}
		m_iClip--;
	}
	else if (m_burstOrPhase == 2)
	{
		m_burstOrPhase = 0;
		consume = true;
	}
	else
	{
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 3.0f;
	}

	if (consume)
	{
		// the thrown grenade frees its slots + the speed recomputes (our slots derive from the
		// arsenal, so the recompute shifts on the last one's DropWeapon).
		m_pPlayer->SetPlayerSpeed(GetSpeedBySlots(m_pPlayer));
		if (m_iClip < 1)
		{
			DropWeapon(m_weaponType, true);
		}
		else
		{
			SendWeaponAnim(s_drawSeq[m_weaponType]); // block [2] = 1 — redraw the next grenade
			m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 2.0f;
		}
	}
}

// @ KatanaFire__6CTSGun @0x733f0 — the katana slash, a 5-STANCE system (decompile + raw-disasm exact;
// reference/dossiers/cont81/analysis_katanafire.md). Stance -> {reach, SIGNED dmg, delay, viewmodel
// anim (the event's iparam1), 3rd-person anim, post-swing lock}:
//   STUNT (iuser4 & 0x70c30)      35 /  35 / 1.0 / 17 / -1 (keep the stunt anim)
//   BACK  (moving backward)       55 / -75 / 1.0 / 15 / 35  + lock — swings BEHIND the player
//   DUCK  (ground + duck)         50 /  65 / 0.9 / 17 / 37  + lock + PRONE|STOP + view_ofs 16 + aux 2.0
//   NORMAL (ground)               50 /  45 / 0.7 / 17|16 / 33|34 alternating (m_lastMeleeAnim 1<->2)
//   AIR                           50 / clamp(speed*2/13, 35, 55) / 1.1 / 17 / 36
// dmg x GetMeleeMultiplier; negative dmg traces BACKWARD (CheckHull hull 1, else 3); VictimKatanaHold
// deflects into the block presentation; a player hit bloodies the blade (item+0x8 + pev->body=1) and
// plays hit-body.wav on the VICTIM; a miss arms the whiff-lunge (0.25s window, SIGNED force, type 2).
// katana.sc plays EVERY swing (FEV_NOTHOST, iparam1=vmAnim, iparam2=bloody) — the whoosh/viewmodel
// presentation is CLIENT-side; the server emits no swing sound and sends no SendWeaponAnim.
void CTSGun::KatanaFire()
{
	if (m_weaponType == TSW_MELEE) // original gate: m_pActiveItem == NULL
		return;
	entvars_t *ppev = m_pPlayer->pev;
	TSItem &it = m_items[m_weaponType];

	UTIL_MakeVectors(ppev->v_angle);
	const bool stunting = (ppev->iuser4 & 0x70c30) != 0;
	const bool onGround = (ppev->flags & FL_ONGROUND) != 0;
	const float speed = ppev->velocity.Length();

	// moving backward: slow -> the +back key; fast -> velocity vs facing (dot <= -0.5).
	bool movingBack;
	if (speed <= 100.0f)
		movingBack = (ppev->button & IN_BACK) != 0;
	else
		movingBack = DotProduct(gpGlobals->v_forward, ppev->velocity * (1.0f / speed)) <= -0.5f;

	int reach, vmAnim, bodyAnim;
	float dmg, delay;
	bool lock = false;
	if (stunting)
	{
		reach = 35; dmg = 35.0f; delay = 1.0f; vmAnim = 17; bodyAnim = -1;
	}
	else if (movingBack)
	{
		reach = 55; dmg = -75.0f; delay = 1.0f; vmAnim = 15; bodyAnim = TSA_CC_KATANA3; // 35
		lock = true;
	}
	else if (onGround && ((ppev->button & IN_DUCK) || (pev->flags & FL_DUCKING)))
	{
		// NB: the FL_DUCKING test reads the GUN's own pev (original-source slip @0x73624 — a carried
		// item never has it set, so IN_DUCK is the live trigger). Replicated as-is for fidelity (L4).
		reach = 50; dmg = 65.0f; delay = 0.9f; vmAnim = 17; bodyAnim = TSA_CC_KATANADUCK; // 37
		lock = true;
		ppev->iuser4 |= TSSTUNT_PRONE | TSSTUNT_STOP; // drop into the ducking hull + damp velocity
		ppev->view_ofs = Vector(0, 0, 16);
		m_pPlayer->m_flDuckSlashAux = 2.0f; // player+0x4f4 (consumer VERIFY)
	}
	else if (onGround)
	{
		reach = 50; dmg = 45.0f; delay = 0.7f;
		// alternate the two normal slashes without repeating (this+0xa04 cycle 1<->2).
		if (m_lastMeleeAnim != 1 && m_lastMeleeAnim != 2)
			m_lastMeleeAnim = RANDOM_LONG(1, 2);
		if (m_lastMeleeAnim == 1)
		{
			vmAnim = 17; bodyAnim = TSA_CC_KATANA1; m_lastMeleeAnim = 2; // 33
		}
		else
		{
			vmAnim = 16; bodyAnim = TSA_CC_KATANA2; m_lastMeleeAnim = 1; // 34
		}
	}
	else
	{
		reach = 50; delay = 1.1f; vmAnim = 17; bodyAnim = TSA_CC_KATANAAIR; // 36
		dmg = speed * (2.0f / 13.0f); // air slash scales with speed
		if (dmg < 35.0f) dmg = 35.0f;
		if (dmg > 55.0f) dmg = 55.0f;
	}

	m_pPlayer->m_TSAnim.Animate(m_pPlayer, bodyAnim); // -1 (stunt) is a no-op

#ifndef CLIENT_DLL // hit resolution is server-authoritative (house pattern, see ColdCock/CCombatAttack)
	dmg *= m_pPlayer->GetMeleeMultiplier();

	// signed damage sets the trace direction: the back-slash sweeps BEHIND the player.
	Vector vecSrc = m_pPlayer->GetGunPosition();
	Vector vecEnd = vecSrc + gpGlobals->v_forward * ((dmg < 0) ? -(float)reach : (float)reach);
	TraceResult tr;
	m_pPlayer->CheckHull(vecSrc, vecEnd, tr, (dmg < 0) ? 1 : 3);

	bool whiff = true;
	if (tr.flFraction < 1.0f)
	{
		CBaseEntity *pEnt = tr.pHit ? CBaseEntity::Instance(tr.pHit) : nullptr;
		if (pEnt && pEnt->IsAlive() && !(pEnt->pev->effects & EF_NODRAW))
		{
			whiff = false;
			if (!VictimKatanaHold(m_pPlayer, pEnt, dmg))
			{
				ClearMultiDamage();
				pEnt->TraceAttack(ppev, fabsf(dmg), gpGlobals->v_forward, &tr, DMG_CLUB | DMG_NEVERGIB);
				ApplyMultiDamage(ppev, ppev);
				if (pEnt->IsPlayer())
				{
					it.bloody = 1;   // item+0x8 — bloody blade (rides the event as iparam2)
					pev->body = 1;   // bloody katana w_/p_ submodel
					EMIT_SOUND_DYN(ENT(pEnt->pev), CHAN_BODY, "weapons/katana/hit-body.wav",
						VOL_NORM, 0.8f, 0, 95 + RANDOM_LONG(0, 31)); // from the VICTIM's edict
				}
			}
		}
	}
	if (whiff)
	{
		// miss (or a dead/nodraw target): arm the whiff-lunge — SIGNED force, katana flavor.
		m_pPlayer->m_flWhiffLungeTime = 0.25f;
		m_pPlayer->m_flWhiffLungeForce = dmg;
		m_pPlayer->m_iWhiffLungeType = 2;
	}

	// post-swing root (back/duck slash): skipped when STOP is already up (the duck branch set it).
	if (lock && !(ppev->iuser4 & TSSTUNT_STOP))
	{
		m_pPlayer->m_flBlockTime = delay * 0.9f;
		ppev->iuser4 |= TSSTUNT_STOP;
	}
#endif

	PLAYBACK_EVENT_FULL(FEV_NOTHOST, m_pPlayer->edict(), m_usKatana, 0.0, g_vecZero, g_vecZero,
		0.0, 0.0, vmAnim, it.bloody, 0, 0);

	m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + delay;
	m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + delay;
	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 3.0f;
	m_pPlayer->m_flLastMeleeRate = delay; // player+0x4f8
}

// @ KatanaThrow__6CTSGun @0x73b68 — right-click with the katana HURLS it: a 1000 u/s CTSKnife
// (type code 8, s_weapon_info+0x20) spawned at a wall-clipped muzzle point, rolled edge-on with a
// -700 end-over-end spin. The weapon then LEAVES the arsenal (drop + next-best select + speed
// recompute) unless the infinite-clips powerup keeps it. No viewmodel anim, no client event —
// a server entity + one server sound; the deploy of the next weapon is the visible presentation.
void CTSGun::KatanaThrow()
{
	if (m_weaponType == TSW_MELEE) // original gate: m_pActiveItem == NULL
		return;
	const s_weapon_info *wi = get_weapon_info((tsweapon_t)m_weaponType);

	// both attack timers re-arm at info+0x28 (katana 0.7 — moot: the weapon is dropped below).
	m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + wi->flSpread;
	m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + wi->flSpread;
	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 3.0f;

	// spawn point = the first hit within eye + 32*forward (+1u right) — the blade can't start in a wall.
	UTIL_MakeVectors(m_pPlayer->pev->v_angle);
	Vector eyes = m_pPlayer->pev->origin + m_pPlayer->pev->view_ofs;
	TraceResult tr;
	UTIL_TraceLine(eyes + gpGlobals->v_right * 1.0f,
		eyes + gpGlobals->v_forward * 32.0f + gpGlobals->v_right * 1.0f,
		dont_ignore_monsters, ENT(m_pPlayer->pev), &tr);

	CBaseEntity *pKnife = TSThrowKnife(m_pPlayer, tr.vecEndPos, gpGlobals->v_forward * 1000.0f,
		wi->iKnifeType); // server-only (client stub returns NULL)
	if (pKnife)
	{
		// the thrower orients the projectile (the original does this here, not in CTSKnife::Spawn).
		Vector dir = pKnife->pev->velocity;
		dir = (dir.Length() > 0.0f) ? dir.Normalize() : g_vecZero;
		pKnife->pev->angles = UTIL_VecToAngles(dir);
		pKnife->pev->angles.z = 90.0f;      // blade rolled edge-on
		pKnife->pev->avelocity.x = -700.0f; // end-over-end spin (KnifeFire's knife uses -1300)
	}

	if (!(m_pPlayer->pev->iuser4 & 0x30c30) && (m_pPlayer->pev->flags & FL_ONGROUND))
		m_pPlayer->m_TSAnim.Animate(m_pPlayer, TSA_CC_KNIFELAUNCH); // 3rd-person launch gesture, not mid-stunt

	EMIT_SOUND_DYN(ENT(m_pPlayer->pev), CHAN_BODY, "weapons/katana/miss.wav",
		VOL_NORM, 0.8f, 0, 95 + RANDOM_LONG(0, 31)); // orig 8-arg EMIT_SOUND_DYN slowmo arg 0.5 (VERIFY L12)

	if (!InfiniteClips(m_pPlayer))
	{
		// thrown away for good: remove from the arsenal + auto-select next-best, then recompute the
		// move speed from the freed slots (the original refunds slotCost into player+0x548 by hand;
		// our slots derive from the owned items, so drop first, then recompute).
		DropWeapon(m_weaponType, true);
		m_pPlayer->SetPlayerSpeed(GetSpeedBySlots(m_pPlayer));
	}
}

// @ AkimboFire__6CTSGun — the dual-wield fire-types (4 combinable, 5 gold-only). The
// faithful engine splits the clip into a left/right barrel (m_pActiveItem+0x1c/+0x20)
// and, in alternating fire-mode, fires one barrel per trigger (IN_ATTACK vs IN_ATTACK2)
// from a ±5u muzzle offset. FUNCTIONAL reconstruction on our single-clip arsenal: two
// guns fire continuously while held, alternating the muzzle side each shot.
bool CTSGun::AkimboFire(const s_weapon_info *wi)
{
	TSItem &it = m_items[m_weaponType];

	// The USER-CONFIRMED akimbo trigger model:
	//  - plain AUTO (the uzis' default): BOTH barrels fire AT ONCE while held;
	//  - plain SEMI: ALTERNATE barrels per pull (skipping a dry one);
	//  - the FREE modes (4 "Free Semi" / 5 "Free Full"): the CLICKED barrel only — item+0x24
	//    (0 = left click = LEFT barrel, 1 = right click = RIGHT), each draining ITS OWN clip
	//    and dry-clicking when THAT barrel is empty even if the other still has rounds.
	// @0x829e5 (round-6 verify RE): in the FREE modes, BOTH buttons together = a LINKED
	// both-barrels shot — "Free Full" (5) checks the HELD buttons, "Free Semi" (4) the
	// PRESSED cache (player+0x814).
	const unsigned int ccBtns = (m_fireMode == TSFM_AKIMBO_BOTH)
		? (unsigned int)m_pPlayer->pev->button : (unsigned int)m_pPlayer->m_afButtonPressed;
	const bool freeLinked = (m_fireMode == TSFM_AKIMBO_ALT || m_fireMode == TSFM_AKIMBO_BOTH) &&
		(ccBtns & IN_ATTACK) != 0 && (ccBtns & IN_ATTACK2) != 0;

	int sides[2], nSides = 0; // 0 = right, 1 = left
	if (m_fireMode == TSFM_AUTO || freeLinked)
	{
		if (it.rightClip > 0) sides[nSides++] = 0;
		if (it.leftClip  > 0) sides[nSides++] = 1;
		if (nSides == 0)
		{
			m_iClip = 0;
			EmptySound(); // @0x72adc — the in-akimbo dry click (0.1s re-arm inside)
			return false;
		}
	}
	else if (m_fireMode == TSFM_AKIMBO_ALT || m_fireMode == TSFM_AKIMBO_BOTH)
	{
		const int fireLeft = it.altMode ? 0 : 1; // primary(left click)=LEFT, secondary=RIGHT
		const int barrelClip = fireLeft ? it.leftClip : it.rightClip;
		if (barrelClip <= 0)
		{
			EmptySound(); // THIS barrel is dry — the other may still be loaded
			return false;
		}
		sides[nSides++] = fireLeft;
		m_fInSpecialReload = fireLeft; // the last-fired side (the item's pumpToggle byte)
	}
	else // plain SEMI: alternate (the useitems barrelMask override honored)
	{
		if (it.leftClip + it.rightClip <= 0)
		{
			m_iClip = 0;
			EmptySound();
			return false;
		}
		int fireLeft;
		if ((it.barrelMask & 6) == 2)      fireLeft = 1;          // left only
		else if ((it.barrelMask & 6) == 4) fireLeft = 0;          // right only
		else                               fireLeft = (m_fInSpecialReload ^ 1) & 1; // alternate
		if (fireLeft && it.leftClip <= 0 && it.rightClip > 0) fireLeft = 0;
		else if (!fireLeft && it.rightClip <= 0 && it.leftClip > 0) fireLeft = 1;
		sides[nSides++] = fireLeft;
		m_fInSpecialReload = fireLeft; // the barrel side lives in the item's pumpToggle byte
	}

	m_pPlayer->pev->effects |= EF_MUZZLEFLASH;
	// (3rd-person shoot body set by CTSGun::Fire's direct Animate call, not via the empty TS SetAnimation.)

	UTIL_MakeVectors(m_pPlayer->pev->v_angle);
	Vector vecAiming = gpGlobals->v_forward;
	float flSpread = wi->flSpread > 0 ? wi->flSpread : 0.02f;
	// @0x82bf8 (round-6 verify RE): the akimbos run the dynamic-accuracy growth too — recoil
	// DOUBLED on a linked both-barrels shot; the (non-integral) silencer flag tightens.
	const bool akSilenced = (it.attach & 1) != 0 && (wi->attachBuiltin & 1) == 0;
	if (wi->bUsesSpread)
		ScaleSpread(flSpread, (nSides == 2) ? wi->flRecoil * 2.0f : wi->flRecoil, akSilenced);
	// @0x82c65: the silencer damage penalty (x0.775)
	const float akDamage = akSilenced ? wi->flDamage * 0.775f : wi->flDamage;

	for (int s = 0; s < nSides; s++)
	{
		const int leftSide = sides[s];
		if (leftSide) it.leftClip--; else it.rightClip--;
		// Muzzle offset to the firing gun's side (+5u right / -5u left), like the original.
		Vector vecSrc = m_pPlayer->GetGunPosition() + gpGlobals->v_right * (leftSide ? -5.0f : 5.0f);
#ifndef CLIENT_DLL
		// @0x72e62/0x72fff/0x73133: one pellet per fired barrel through the TS bullet path.
		m_pPlayer->ShootBulletsPlayer(1, vecSrc, vecAiming,
			Vector(flSpread, flSpread, flSpread), akDamage, wi->iKnifeType,
			wi->iCaliber, m_pPlayer->random_seed);
#else
		m_pPlayer->FireBulletsPlayer(1, vecSrc, vecAiming,
			Vector(flSpread, flSpread, flSpread), 8192, BULLET_PLAYER_9MM,
			0, (int)(wi->flDamage * TSDamageMult()), m_pPlayer->pev, m_pPlayer->random_seed);
#endif
	}
	m_iClip = it.leftClip + it.rightClip; // total clip mirrors the two barrels
	// @0x82c3a: a fire-type-4 akimbo mirrors one barrel into its SINGLE's parked clip after
	// every shot (the same mirror the reload completion keeps).
	if (wi->iFireType == TSFT_AKIMBO && wi->iSingleType >= 0 && wi->iSingleType < TSW_COUNT &&
		wi->iSingleType != m_weaponType)
		m_items[wi->iSingleType].clip = it.leftClip;

	// (the viewmodel anim rides the EVENT now — EV_TSFireLinked plays block[3+RL(0,2)] for the
	// both-barrels shot; EV_TSFire plays block[22/23] left / [24/25] right off bparam1)

	// The ORIGINAL wire (guntail dossier §5): the BOTH-barrel shot is its OWN EVENT —
	// events/firelinked.sc (one bang = wi+0x306, two flares/shells, two volleys) — while a
	// SINGLE akimbo shot rides fire.sc with bparam1 = the barrel (0 right / 1 left) and
	// bparam2 = the FireBullets SEED (our old bparam2=side extension collided with the seed;
	// fixed). fparam2 = the recoil magnitude (EV_TSFire/Linked's TS_AddViewRecoil reads it).
	if (nSides == 2)
		PLAYBACK_EVENT_FULL(FEV_NOTHOST, m_pPlayer->edict(), m_usFireLinked, 0.0, g_vecZero,
			g_vecZero, flSpread, wi->flViewKick, m_weaponType, it.attach, 0,
			m_pPlayer->random_seed);
	else
		PLAYBACK_EVENT_FULL(FEV_NOTHOST, m_pPlayer->edict(), m_usFire, 0.0, g_vecZero, g_vecZero,
			flSpread, wi->flViewKick, m_weaponType, it.attach, sides[0],
			m_pPlayer->random_seed);

	float flRate = wi->flFireRate > 0 ? wi->flFireRate : 0.1f;
	if (DoubleFireRate(m_pPlayer))
		flRate *= 0.5f;
	m_flNextPrimaryAttack = (UTIL_WeaponTimeBase() + flRate);
	m_flNextSecondaryAttack = m_flNextPrimaryAttack; // the right-barrel trigger shares the cadence
	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 5.0f;
	return true;
}

// @ AddItem__6CTSGuni @0x80794 — attach a gun item (silencer/laser/flashlight/scope: bits 1/2/4/8) to
// the ACTIVE weapon. The original validates against the weapon's supported-attachment mask
// (s_weapon_info+0xe0) and a conflicting/silenced-base mask (+0xe4), refuses a duplicate, OR's the bits
// into the active item's attach (item+0x10) — and into the per-slot save array — then re-selects/deploys
// to refresh the attachment'd model. In our arsenal the active item IS m_items[m_weaponType], so one
// store covers both. SLICE: we don't carry the +0xe0/+0xe4 masks yet, so we accept any not-already-held
// bit — the client buy menu's AttachAvailable already gates which weapons can be sold a silencer, and
// EV_TSFire only swaps the SOUND for weapons that have a `-sil` variant, so an unsupported attach is inert.
// The attach mask travels to the client in the fire event (iparam2 — see StandardFire/AkimboFire).
void CTSGun::SendWeaponAnim(int iAnim)
{
	const int body = (m_weaponType >= 0 && m_weaponType < TSW_COUNT)
		? GetBodyByItems(m_weaponType, m_items[m_weaponType].attach) : 0;
	CBasePlayerWeapon::SendWeaponAnim(iAnim, body);
}

int CTSGun::AddItem(int bits)
{
	if (m_weaponType < 0 || m_weaponType >= TSW_COUNT)
		return 0;
	TSItem &it = m_items[m_weaponType];
	if (!it.owned)
		return 0;
	const s_weapon_info *wi = get_weapon_info((tsweapon_t)m_weaponType);
	// RE @0x80794: refuse if the weapon doesn't support the attachment (not in +0xe0), if it's a
	// built-in/conflicting attachment (+0xe4, e.g. a silencer on the silenced-base MP5SD), or if it's
	// already attached. Then OR the bits onto the active item's attach.
	if ((bits & wi->attachSupported) == 0)   return 0; // unsupported
	if ((bits & wi->attachBuiltin) != 0)     return 0; // built-in / can't re-add
	if ((bits & it.attach) != 0)             return 0; // already attached
	it.attach |= bits;
	// Laser/flashlight start ON (maximal active — the player toggles them OFF as they play;
	// user-verified vs the live original). In the original the spawn loadout carries the
	// attach mask INSIDE tkw -> TakeWeapon's fresh-attach path grants active there; until our
	// buy grammar moves onto that path, grant it here too so a tki-delivered attachment
	// behaves identically.
	it.active |= (bits & 6);

	// @0x80794: MIRROR the bits onto the akimbo pair — an akimbo shares its single's
	// attachments (buy a silencer for the mk23 and the akimbo pair has it too, both ways).
	if (wi->iFireType == TSFT_AKIMBO || wi->iFireType == TSFT_AKIMBO_ONLY)
	{
		if (wi->iSingleType >= 0 && wi->iSingleType < TSW_COUNT)
		{
			m_items[wi->iSingleType].attach |= bits;
			m_items[wi->iSingleType].active |= (bits & 6);
		}
	}
	else if (wi->iAkimboType != -1 && IsOwned(wi->iAkimboType))
	{
		m_items[wi->iAkimboType].attach |= bits;
		m_items[wi->iAkimboType].active |= (bits & 6);
	}

	// @0x80794 tail: the original zeroes m_weaponType and re-runs SelectWeapon + Deploy so the
	// viewmodel body/draw refresh with the new attachment. Our SelectWeapon early-outs on the
	// same type by design, so redeploy the active weapon directly — same effect (draw anim +
	// pev->body recomputed via GetBodyByItems with the new attach mask).
	if (m_pPlayer && m_pPlayer->m_pActiveItem == (CBasePlayerItem *)this)
		Deploy();
	return 1;
}

#ifndef CLIENT_DLL
// @ Zoom__6CTSGuni @0x71710 — enter a scope zoom level. Sets the player's networked view FOV; for a
// high-magnification scope (zoomFov1 < 40) it ALSO hides the viewmodel (pev->viewmodel = 0), matching
// the original (you look through the scope, not at the gun).
void CTSGun::Zoom(int fov)
{
	// BOTH carriers: pev->fov (the server-side cycle state) AND m_iFOV — the client's refdef
	// only honors gHUD.m_iFOV (HUD_UpdateClientData reports it back to the engine), which the
	// stock SetFOV message feeds from m_iFOV (round-7k: the zoom never changed the view).
	m_pPlayer->pev->fov = (float)fov;
#ifndef CLIENT_DLL
	m_pPlayer->m_iFOV = fov;
#endif
	const s_weapon_info *wi = get_weapon_info((tsweapon_t)m_weaponType);
	if (wi->zoomFov1 < 40)
		m_pPlayer->pev->viewmodel = 0;
}

// @ Unzoom__6CTSGun @0x71748 — leave the scope: clear the view FOV and, if the viewmodel was hidden,
// restore it to the weapon's v_model.
void CTSGun::Unzoom()
{
	m_pPlayer->pev->fov = 0;
#ifndef CLIENT_DLL
	m_pPlayer->m_iFOV = 0;
#endif
	if (m_pPlayer->pev->viewmodel == 0)
	{
		const s_weapon_info *wi = get_weapon_info((tsweapon_t)m_weaponType);
		if (wi->pszViewModel)
			m_pPlayer->pev->viewmodel = MAKE_STRING(wi->pszViewModel);
	}
}
#endif

// @ UpdateClientData__6CTSGun @0x758c8 — send the TS WeaponInfo HUD message (the ammo
// readout) on change. The original packs the active s_weapon_status slot:
//   BYTE weaponType, BYTE clip (slot+0x4), SHORT reserve (per-caliber), BYTE fireMode
//   (slot+0x2), BYTE flags (slot+0x10).  Our modern arsenal stores the same values
//   directly (m_weaponType / m_iClip / m_ammoReserve[caliber] / m_fireMode). The flags
//   byte (weapon capability bits the original masks from s_weapon_info) is a SLICE: we
//   send 0 — it doesn't affect the ammo numbers. See reference/HUD_MESSAGES.md.
bool CTSGun::UpdateClientData(CBasePlayer *pPlayer)
{

	const s_weapon_info *wi = get_weapon_info((tsweapon_t)m_weaponType);
	const int caliber = wi->iCaliber;
	const int reserve = (caliber > 0 && caliber < TS_NUM_CALIBERS) ? m_ammoReserve[caliber] : 0;

	// ActItems — broadcast this player's ACTIVE attachment mask to ALL clients (so everyone sees the
	// laser/flashlight only while it's toggled ON via `useitems`, not just because it's equipped). The
	// visible attachments (laser bit 2, flashlight bit 4) are rendered client-side from the muzzle
	// (cl_dll/ts_attachments.cpp), and the owner's dive-reticle distance gates on the active laser.
	// Wire: BYTE playerIndex, BYTE activeMask (client handler FUN_1004fa70). Broadcast separately from
	// the owner-only WeaponInfo below, and BEFORE its early-out, so a toggle always propagates.
	const int activeAttach = m_items[m_weaponType].active;
	if (activeAttach != m_iClientAttach)
	{
		m_iClientAttach = activeAttach;
		MESSAGE_BEGIN(MSG_ALL, gmsgTSActiveItems);
		WRITE_BYTE(ENTINDEX(pPlayer->edict())); // player entity index
		WRITE_BYTE(activeAttach);               // ACTIVE attach mask (bit 2 laser / 4 flash)
		MESSAGE_END();
	}

	const int equipped = m_items[m_weaponType].attach;
	// The WeaponInfo "clip" byte carries the MELEE BAR for melee weapons (RE UpdateClientData
	// @0x758c8 reads player+0x774) — this is how the client tracks the silent PreThink regen.
	const s_weapon_info *wiActive = get_weapon_info((tsweapon_t)m_weaponType);
	const bool meleeInfo = wiActive && (wiActive->iFireType == 3 || wiActive->iFireType == 10);
	const int clipByte = meleeInfo ? (int)(pPlayer->m_flMeleeBar + 0.5f) : m_iClip;
	if (m_weaponType == m_iClientWeaponType && clipByte == m_iClientClip &&
		reserve == m_iClientReserve && m_fireMode == m_iClientFireMode &&
		equipped == m_iClientEquip)
		return false; // nothing changed

	m_iClientWeaponType = m_weaponType;
	m_iClientClip = clipByte;
	m_iClientReserve = reserve;
	m_iClientFireMode = m_fireMode;
	m_iClientEquip = equipped;

	MESSAGE_BEGIN(MSG_ONE, gmsgWeaponInfo, NULL, pPlayer->pev);
	WRITE_BYTE(m_weaponType);     // active tsweapon_t
	WRITE_BYTE(clipByte);         // rounds loaded (melee: the stamina bar)
	WRITE_SHORT(reserve);         // spare rounds for this caliber
	WRITE_BYTE(m_fireMode);       // 0=auto 1=semi 2=burst 3=pump
	WRITE_BYTE(equipped);         // EQUIPPED attachment mask (silencer 1 / laser 2 / flash 4 / scope 8)
	MESSAGE_END();

	// @ the WStatus block of CTSGun::UpdateClientData @0x758c8 — the original owns the whole
	// weapon-bar loop INSIDE UpdateClientData (dossier-invented A2; our SendWeaponStatus split
	// was invented — merged back). One WStatus per arsenal slot on change: BYTE type,
	// BYTE owned(0/1), BYTE clip.

	if (!m_bWStatusInit)
	{
		for (int i = 0; i < TSW_COUNT; i++)
			m_iClientWStatus[i] = -2; // force a first send of every slot
		m_bWStatusInit = true;
	}

	for (int t = 0; t < TSW_COUNT; t++)
	{
		const int owned = m_items[t].owned ? 1 : 0;
		// the live active clip is in m_iClip, not yet synced into its item
		const int clip = owned ? ((t == m_weaponType) ? m_iClip : m_items[t].clip) : 0;
		const int state = owned ? clip : -1;
		if (state == m_iClientWStatus[t])
			continue;
		m_iClientWStatus[t] = state;
		MESSAGE_BEGIN(MSG_ONE, gmsgTSWeaponStatus, NULL, pPlayer->pev);
		WRITE_BYTE(t);     // tsweapon_t
		WRITE_BYTE(owned); // 1 = in the arsenal
		WRITE_BYTE(clip);  // loaded rounds
		MESSAGE_END();
	}

	return true;
}

// @ ReloadWeapon__6CTSGuniif (@0x74a14, decompile-exact) — ARM a whole-clip reload: gate on a
// wielded item with caliber reserve; take = min(iClipSize - clip, reserve); if any, set the
// item's reloading flag (item+0xc, our m_fInReload) and lock the player's m_flNextAttack for
// flDuration (the weapon's flReloadTime). The REFILL happens in the base ItemPostFrame when that
// lock elapses. iAnim is computed by the caller but DEAD in the original body — the reload
// viewmodel anim rides the events/reload.sc playback client-side.
int CTSGun::ReloadWeapon(int iClipSize, int iAnim, float flDuration)
{
	(void)iAnim;
	if (!m_pActiveItem || !m_pActiveInfo)
		return 0;
	const int caliber = m_pActiveInfo->iCaliber;
	if (caliber <= 0 || caliber >= TS_NUM_CALIBERS || m_ammoReserve[caliber] <= 0)
		return 0;
	const int need = iClipSize - m_iClip;
	const int take = (need < m_ammoReserve[caliber]) ? need : m_ammoReserve[caliber];
	if (take <= 0)
		return 0;
	m_fInReload = true; // item+0xc — the base ItemPostFrame completes the refill
	m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + flDuration; // player+0x12c
	return 1;
}

// @ StopReload__6CTSGun (@0x71288, decompile-exact) — abort an ARMED whole-clip reload (the
// item+0xc flag): clear it, and for a pump gun also reset the phase machine + the rack toggle;
// idle + the player's attack lock snap to NOW. Callers: ColdCock + MultiplyClips (the pump's
// shell-by-shell machine is phase-driven, never arms item+0xc, and is interrupted by
// ShotgunFire's phase->close instead).
void CTSGun::StopReload()
{
	if (m_pActiveItem && m_pActiveInfo && m_fInReload)
	{
		m_fInReload = false; // item+0xc
		if (m_pActiveInfo->iFireType == TSFT_PUMP)
		{
			m_burstOrPhase = 0;     // item+0x8 — the pump phase machine
			m_fInSpecialReload = 0; // item pumpToggle — back to a FIRE step
		}
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase();
		m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase();
	}
}

// @ EmptySound__6CTSGun (@0x71be4, decompile-exact) — the dry-fire click, via events/empty.sc
// (all params zero; the CLIENT handler owns the click sound — ev swap pending, see the gun-event
// tail task). The contender (weapon 0x24) is silent. Re-arms primary+secondary +0.1, idle +0.5.
void CTSGun::EmptySound()
{
	if (m_weaponType == TSW_CONTENDER)
		return;
	PLAYBACK_EVENT_FULL(FEV_NOTHOST, m_pPlayer->edict(), m_usEmpty, 0.0, g_vecZero, g_vecZero,
		0.0, 0.0, 0, 0, 0, 0);
	// (EV_TSEmpty owns the click now — weapons/pistol-empty.wav, universal; the old
	// 357_cock1 server stand-in was the wrong sample anyway.)
	m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + 0.1f;
	m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 0.1f;
	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 0.5f;
}

// @ Reload__6CTSGun (@0x74d5c, decompile-exact) — the +reload dispatch. Bare hands AND the
// blades (fire-types 3/10) route to the kung-fu BLOCK; the grenade (fire-type 2) refuses; a pump
// (fire-type 1) runs the shell-by-shell ShotgunReload machine. Otherwise gate on reserve + not
// mid-burst, pick the reload anim from the ACTIVE STANCE's anim block [1] (the SAWED-OFF with a
// shell still chambered uses block [8] — the shell-retained variant), arm it via ReloadWeapon
// (duration = the weapon's flReloadTime), and on success: idle +5.0, Unzoom, play
// events/reload.sc (iparam1=type, iparam2=equipped attach, fparam1=0), 3rd-person TSA_RELOAD.
void CTSGun::Reload()
{
	if (!m_pActiveItem)
	{
		CCombatBlock(); // bare hands: RELOAD = the kung-fu block
		return;
	}
	if (m_pActiveInfo->iFireType == TSFT_GRENADE)
		return;
	if (m_pActiveInfo->iFireType == TSFT_KNIFE || m_pActiveInfo->iFireType == TSFT_KATANA)
	{
		CCombatBlock(); // the blades block on RELOAD too
		return;
	}
	if (m_pActiveInfo->iFireType == TSFT_PUMP)
	{
		ShotgunReload();
		return;
	}
	const int caliber = m_pActiveInfo->iCaliber;
	if (caliber <= 0 || caliber >= TS_NUM_CALIBERS || m_ammoReserve[caliber] < 1)
		return;
	if (m_burstOrPhase != 0)
		return; // mid-burst: the queued rounds finish first (@0x84e18)

	// The vestigial anim arg: the original computes the stance block's [1] — or the sawed-off
	// shell-retained [8] — and passes it to ReloadWeapon, where it is DEAD; the client anim
	// rides reload.sc's fparam1 (0 -> block[1]). The sawed-off block[8] variant never actually
	// plays in the original (its Reload sends fparam1=0 too — the data is a leftover).
	const int anim = (m_bHoldPosition && m_pActiveInfo->iHoldReloadAnim >= 0)
		? m_pActiveInfo->iHoldReloadAnim : m_pActiveInfo->iReloadAnim;

	if (ReloadWeapon(m_pActiveInfo->iClip, anim, 1.0f * m_pActiveInfo->flReloadTime))
	{
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 5.0f; // dc_5
#ifndef CLIENT_DLL
		Unzoom();
#endif
		// events/reload.sc — EV_TSReload owns the whole presentation (anim block[1], the
		// non-local generic_reload + brass/mag toss).
		PLAYBACK_EVENT_FULL(FEV_NOTHOST, m_pPlayer->edict(), m_usReload, 0.0, g_vecZero, g_vecZero,
			0.0, 0.0, m_weaponType, m_pActiveItem->attach, 0, 0);
		m_pPlayer->m_TSAnim.Animate(m_pPlayer, TSA_RELOAD); // 0x33 — 3rd-person relo_<grip>
	}
}

// @ ShotgunReload__6CTSGun (@0x74aac, decompile-exact) — the pump shell-by-shell reload, a
// 3-phase machine in the item's burstOrPhase slot (item+0x8). NEVER arms item+0xc — the driver
// is WeaponIdle (fire-type 1 + phase != 0 -> Reload -> here, every frame), and a fire press
// interrupts via ShotgunFire's phase->2 (close). Each phase plays events/reload.sc with the
// phase discriminator in fparam1: 8.0 open / 9.0 insert / 10.0 close (iparam1=type,
// iparam2=attach).
//   phase 0 (start, from +reload): if a shell fits — Unzoom, restore a scope-hidden viewmodel,
//     phase=1, ALL FOUR locks +0.3 (primary/secondary/idle + the player's m_flNextAttack);
//   phase 1 (insert): on the idle tick — dry/full -> phase 2; else reserve--, clip++, next
//     shell after flReloadTime (0.3 for every pump);
//   phase 2 (close): phase=0, rack toggle reset (next trigger = FIRE step), player lock +0.3,
//     idle +3.0.
// reload1/2/3 are consecutive in every pump v_*.mdl (iReloadAnim = reload1), so the stand-in
// anims below are base/+1/+2 until the reload.sc client handler lands.
void CTSGun::ShotgunReload()
{
	const s_weapon_info *wi = m_pActiveInfo;
	const int caliber = wi->iCaliber;
	const bool haveShell = (caliber > 0 && caliber < TS_NUM_CALIBERS && m_ammoReserve[caliber] > 0);

	if (m_burstOrPhase == 0)
	{
		if (haveShell && m_iClip < wi->iClip)
		{
#ifndef CLIENT_DLL
			Unzoom();
			if (!m_pPlayer->pev->viewmodel && wi->pszViewModel)
				m_pPlayer->pev->viewmodel = MAKE_STRING(wi->pszViewModel); // scope-hidden restore
#endif
			m_burstOrPhase = 1;
			// EV_TSReload plays block[8] (the open) off fparam1=8
			PLAYBACK_EVENT_FULL(FEV_NOTHOST, m_pPlayer->edict(), m_usReload, 0.0, g_vecZero,
				g_vecZero, 8.0, 0.0, m_weaponType, m_pActiveItem->attach, 0, 0);
			m_pPlayer->m_TSAnim.Animate(m_pPlayer, TSA_RELOAD); // 3rd-person reload
			m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 0.3f; // dc_0_3
			m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 0.3f;
			m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 0.3f;
			m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + 0.3f;
		}
	}
	else if (m_burstOrPhase == 1)
	{
		if (m_flTimeWeaponIdle <= UTIL_WeaponTimeBase())
		{
			if (!haveShell || m_iClip >= wi->iClip)
			{
				m_burstOrPhase = 2;
			}
			else
			{
				m_ammoReserve[caliber]--;
				m_iClip++;
				// EV_TSReload plays block[9] + the insert-shell shuck (everyone) off fparam1=9
				PLAYBACK_EVENT_FULL(FEV_NOTHOST, m_pPlayer->edict(), m_usReload, 0.0, g_vecZero,
					g_vecZero, 9.0, 0.0, m_weaponType, m_pActiveItem->attach, 0, 0);
				m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + wi->flReloadTime; // wi+0x14 (0.3)
			}
		}
	}
	else if (m_burstOrPhase == 2)
	{
		m_burstOrPhase = 0;
		m_fInSpecialReload = 0; // item pumpToggle — next trigger is a FIRE step
		// EV_TSReload plays block[10] + the weapon's close/rack sound (non-local) off fparam1=10
		PLAYBACK_EVENT_FULL(FEV_NOTHOST, m_pPlayer->edict(), m_usReload, 0.0, g_vecZero,
			g_vecZero, 10.0, 0.0, m_weaponType, m_pActiveItem->attach, 0, 0);
		m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 0.3f;
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 3.0f; // dc_3
	}
}

// @0x85e54 CCombatIdle — the bare-hands (kung-fu) viewmodel idle: on the idle timer, re-arm
// idle +3.0 and play the TSW_MELEE row's idle sequence from the table (+0x252 — 0 in the
// shipped data; kept as a table read like the original). Sole caller: WeaponIdle's leading
// no-active-item branch.
void CTSGun::CCombatIdle()
{
	if (m_flTimeWeaponIdle > UTIL_WeaponTimeBase())
		return;
	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 3.0f;
	const s_weapon_info *wi = get_weapon_info(TSW_MELEE);
	SendWeaponAnim(wi ? wi->iIdleAnim : 0);
}

// @ WeaponIdle__6CTSGun (@0x754dc, decompile-exact) — runs EVERY frame (the base ItemPostFrame
// tail), not just on the idle timer. Bare hands -> CCombatIdle. Then, for a wielded item:
//  - the PUMP-RELOAD continuation: fire-type 1 with the phase machine armed -> Reload() (which
//    routes back into ShotgunReload) — this is the machine's per-frame driver;
//  - the BURST machine: fire-mode 2 with the primary timer clear — fire the next queued round
//    and decrement (this is how a tapped burst completes after release);
//  - on the idle timer: the grenade cook check (GrenadeIdle(0)), AUTO-RELOAD a dry clip (unless
//    InfiniteClips), or the idle anim — the stance block's [0] — with idle re-armed +3.0.
// (The original also mirrors item->clip into the client-clip cache and re-points item+0x28 at
// the stance's anim block here; our stance selection resolves at each use site instead.)
void CTSGun::WeaponIdle()
{
	if (!m_pActiveItem)
	{
		CCombatIdle();
		return;
	}
	if (m_pActiveInfo->iFireType == TSFT_PUMP && m_burstOrPhase != 0)
	{
		Reload(); // the shell-by-shell continuation (-> ShotgunReload)
		return;
	}
	if (m_fireMode == TSFM_BURST && m_flNextPrimaryAttack <= UTIL_WeaponTimeBase())
	{
		if (m_burstOrPhase < 1)
			m_burstOrPhase = 0;
		else
		{
			Fire();
			m_burstOrPhase--;
		}
	}
	if (m_flTimeWeaponIdle <= UTIL_WeaponTimeBase())
	{
		if (m_pActiveInfo->iFireType == TSFT_GRENADE)
		{
			GrenadeIdle(0); // the cook machine (the pin set idle +0.7, so this starts then)
		}
		else if (!InfiniteClips(m_pPlayer) && m_iClip < 1)
		{
			Reload(); // AUTO-RELOAD the dry clip
		}
		else if (m_iClip != 0)
		{
			m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 3.0f;
			const int idleAnim = (m_bHoldPosition && m_pActiveInfo->iIdleAnimHold >= 0)
				? m_pActiveInfo->iIdleAnimHold : m_pActiveInfo->iIdleAnim; // stance block [0]
			SendWeaponAnim(idleAnim); // the CTSGun shadow computes the attachment body itself
		}
	}
}

// @ SwitchFireMode__6CTSGun (@0x8169c) — cycle the held weapon's fire-mode to the next one
// offered in its iFireModes mask (modes 0..5, wrapping), starting just past the current.
void CTSGun::SwitchFireMode()
{
	const s_weapon_info *wi = get_weapon_info((tsweapon_t)m_weaponType);
	for (int i = 1; i <= 6; i++)
	{
		int mode = (m_fireMode + i) % 6;
		if (wi->iFireModes & (1 << mode))
		{
			m_fireMode = mode;
			m_items[m_weaponType].mode = mode;
			m_burstOrPhase = 0;
			return;
		}
	}
}

// Give a player a TS weapon of `type`. If they already hold the weapon_tsgun (the
// arsenal), add the type to it; otherwise create the weapon_tsgun and pick it up.
CBaseEntity *GiveTSWeapon(CBasePlayer *pPlayer, tsweapon_t type)
{
	CTSGun *pHeld = GetPlayerTSGun(pPlayer);
	if (pHeld)
	{
		if (!pHeld->TakeWeapon(type))
			return NULL; // won't fit the slot capacity (or invalid) — caller leaves it on the floor
		return pHeld;
	}

	edict_t *pent = CREATE_NAMED_ENTITY(MAKE_STRING("weapon_tsgun"));
	if (FNullEnt(pent))
		return NULL;

	CTSGun *pGun = (CTSGun *)CBaseEntity::Instance(pent);
	pGun->m_weaponType = type;
	pGun->UpdateActivePointers();
	pGun->pev->origin = pPlayer->pev->origin;
	pGun->pev->spawnflags |= SF_NORESPAWN;
	DispatchSpawn(pent);
	DispatchTouch(pent, pPlayer->edict());
	return pGun;
}

// Force the player's TS arsenal to be the active item + deploy its current weapon. Called at the
// end of PlayerSpawn: the tsgun is the player's single weapon entity, but the engine's auto-deploy
// (GetNextBestWeapon) skips kung-fu (melee, no clip), so a bare/kung-fu-only loadout would leave
// m_pActiveItem NULL → no viewmodel + ItemPostFrame (firing) never runs. This guarantees it deploys.
void TSDeployArsenal(CBasePlayer *pPlayer)
{
	CTSGun *pGun = GetPlayerTSGun(pPlayer);
	if (!pGun)
		return;
	pPlayer->m_pActiveItem = (CBasePlayerItem *)pGun;
	pGun->Deploy();
}

// @ the ClientCommand `weapon_<N>` commit (@0xae433): wield the owned arsenal slot holding
// tsweapon_t `type` (holster -> set active -> deploy via SelectSlot). This is the faithful
// weapon-select transport: the client's invnext/invprev select cursor (FUN_1004f500) emits
// "weapon_%i" with the numeric type; the server selects that type if owned.
void TSSelectWeaponType(CBasePlayer *pPlayer, int type)
{
	CTSGun *pGun = GetPlayerTSGun(pPlayer);
	if (!pGun || type < 0 || type >= TSW_COUNT)
		return;
	if (type == TSW_MELEE || pGun->IsOwned(type))
		pGun->SelectWeapon(type);
}

// @ the ClientCommand `lastinv` block (@0xae433, the SelectWeapon+Deploy path): toggle back
// to the previously-wielded type (player+0xa1c). SelectWeapon itself records m_lastType.
void TSLastInv(CBasePlayer *pPlayer)
{
	CTSGun *pGun = GetPlayerTSGun(pPlayer);
	if (!pGun || pGun->OwnedCount() <= 1)
		return;
	if (pGun->IsOwned(pGun->m_lastType) || pGun->m_lastType == TSW_MELEE)
		pGun->SelectWeapon(pGun->m_lastType);
}

// Infinite-clip powerup hook (PlayerAddPowerUp) — multiply the held weapon's reserve.
bool TSMultiplyClips(CBasePlayer *pPlayer)
{
	CTSGun *pGun = GetPlayerTSGun(pPlayer);
	return pGun ? pGun->MultiplyClips() : false;
}

// Cycle the held weapon's fire-mode (the `firemode` command — default.cfg binds f).
void TSSwitchFireMode(CBasePlayer *pPlayer)
{
	CTSGun *pGun = GetPlayerTSGun(pPlayer);
	if (pGun)
		pGun->SwitchFireMode();
}

// @ SwitchItems__6CTSGun = FUN_00081564 (the `useitems` command — default.cfg binds g): toggle the
// ACTIVE gun attachments (laser bit 2 / flashlight bit 4) of the held weapon. The active mask
// (TSItem::active, item+0x14) is a subset of the EQUIPPED mask (item+0x10); useitems cycles it.
// Faithful RE state machine (the laser/flashlight only RENDER while active, and AddItem starts them
// all-on). See [[useitems-is-attachment-toggle]].
//   both laser+flash equipped: active cycles  6(both) -> 0(off) -> 2(laser) -> 4(flash) -> 6 ...
//   only one equipped:         that bit toggles on/off
void CTSGun::SwitchItems()
{
	if (m_weaponType < 0 || m_weaponType >= TSW_COUNT)
		return;
	TSItem &it = m_items[m_weaponType];
	const int equipped = it.attach;

	if ((equipped & 6) == 6) // both laser + flashlight equipped
	{
		if ((it.active & 6) == 6)        // both on -> off
			it.active &= ~6;
		else if (it.active & 2)          // laser -> flashlight
			it.active = (it.active & ~2) | 4;
		else if (it.active & 4)          // flashlight -> both
			it.active |= 2;
		else                             // off -> laser
			it.active |= 2;
	}
	else if (equipped & 2)               // only laser equipped -> toggle laser
		it.active ^= 2;
	else if (equipped & 4)               // only flashlight equipped -> toggle flashlight
		it.active ^= 4;
	else
	{
		it.active &= ~6; // nothing toggleable equipped — clear, no sound
		return;
	}

	if (m_pPlayer)
		EMIT_SOUND(ENT(m_pPlayer->pev), CHAN_ITEM, "weapons/switch.wav", VOL_NORM, ATTN_NORM);
}

// @ GetActiveItems__6CTSGun (@0x71228): the active attachment mask of the held weapon (item+0x14),
// with bit 0x40 OR'd in for an akimbo/akimbo-only weapon (the original tags dual-wield here).
int CTSGun::GetActiveItems() const
{
	if (m_weaponType < 0 || m_weaponType >= TSW_COUNT)
		return 0;
	int active = m_items[m_weaponType].active;
	const s_weapon_info *wi = get_weapon_info((tsweapon_t)m_weaponType);
	if (wi->iFireType == TSFT_AKIMBO || wi->iFireType == TSFT_AKIMBO_ONLY)
		active |= 0x40;
	return active;
}

// Toggle the held weapon's active attachments (the `useitems` command -> CTSGun::SwitchItems).
void TSSwitchItems(CBasePlayer *pPlayer)
{
	CTSGun *pGun = GetPlayerTSGun(pPlayer);
	if (pGun)
		pGun->SwitchItems();
}

// @ DropWeapon__6CTSGun (@0x8094c) — remove an owned item from the arsenal. If it was the
// active weapon, auto-select the next-best owned weapon (fThrow → GetBetterWeapon) or just
// holster it. Does NOT spawn the world gun (CBasePlayer::DropWeapon does the throws); this is
// only the items[] bookkeeping side.
void CTSGun::DropWeapon(int type, bool fThrow)
{
	if (type < 1 || type >= TSW_COUNT || !m_items[type].owned)
		return;

	m_items[type] = TSItem(); // clear the item (owned=false, clips/mode/masks zeroed)

	if (type == m_weaponType)
	{
		if (fThrow)
			GetBetterWeapon(type);   // auto-select the next-best owned weapon (holster->deploy)
		else if (m_pPlayer)
			SyncActiveToItem();      // holster only (the active item is already cleared)
	}
}

#ifndef CLIENT_DLL // the drop-into-world helpers spawn server-only CTSWorldGun/CTSAmmoPack entities
// @0x8529c CBasePlayer::ThrowWeapon — the throw primitive (the sole caller is DropWeapon). Build the
// throw angles (alive = view + offset; dead = random), create the dropped gun via the 3-arg factory,
// then position it (eye + 16*forward + lateral), give it a random tumble, and toss it.
CTSWorldGun *CBasePlayer::ThrowWeapon(int type, const TSItem &status, const Vector *angleOffset,
	const Vector *velOffset)
{
	Vector ang;
	if (IsAlive())
	{
		ang = pev->v_angle;
		if (angleOffset)
			ang = ang + *angleOffset;
	}
	else
	{
		ang = Vector(RANDOM_FLOAT(-60, 60), RANDOM_FLOAT(-180, 180), 0);
	}
	UTIL_MakeVectors(ang);

	CTSWorldGun *pGun = CTSWorldGun::CreateWeapon(this, type, status);
	if (!pGun)
		return NULL;

	// origin = eye + 16*forward (+ velOffset: forward on x, lateral on right via y, vertical via z).
	Vector off(16, 0, 0);
	if (velOffset)
		off = Vector(16 + velOffset->x, velOffset->y, velOffset->z);
	Vector origin = (pev->origin + pev->view_ofs) + gpGlobals->v_forward * off.x + gpGlobals->v_right * off.y;
	origin.z += off.z;
	UTIL_SetOrigin(pGun->pev, origin);

	pGun->pev->angles = g_vecZero;
	pGun->pev->avelocity.y = RANDOM_FLOAT(-300, 300); // random tumble/spin (@0x8529c)
	pGun->pev->avelocity.z = RANDOM_FLOAT(-300, 300);

	// Throw speed S (RE'd from the @0x85359 / @0x853c1 x87 asm + the .rodata 0.9/0.33/90 constants):
	//   alive: S = (90 - look-pitch) * 2  (look level -> 180; look up -> harder; look down -> drops)
	//   dead : S = random 60..160
	// velocity = 0.9*playerVelocity (inherited momentum) + S*forward (toss) + 0.33*S upward (lob).
	float S = IsAlive() ? (90.0f - pev->v_angle.x) * 2.0f : RANDOM_FLOAT(60, 160);
	pGun->pev->velocity = pev->velocity * 0.9f + gpGlobals->v_forward * S;
	pGun->pev->velocity.z += S * 0.33f;
	return pGun;
}

// Thin glue for DropWeapon's branches: build the dropped item's status (clip override), then route
// through the real CBasePlayer::ThrowWeapon (velOffset.y carries the akimbo's ±lateral split).
static void TSThrowWorldGun(CBasePlayer *pPlayer, CTSGun *pGun, int type, int clip, float side)
{
	TSItem status = pGun->m_items[type]; // carry the dropped weapon's full state (mode/attach/barrels)
	status.owned = true;
	status.clip = clip;
	Vector lateral(0, side, 0);
	pPlayer->ThrowWeapon(type, status, NULL, &lateral);
}

// @ CBasePlayer::DropWeapon (@0x84900) — drop the player's ACTIVE TS weapon into the world.
// Faithful akimbo branches (reference/WEAPON_SELECT.md):
//  - dropping a combinable AKIMBO (fire-type 4) throws TWO of its base single (clip split in
//    half, tossed ±4) and removes BOTH items[akimbo] and items[single];
//  - dropping a SINGLE while you also own its akimbo throws ONE single, then keeps the single
//    owned (its clip becomes the akimbo's right barrel) and removes the akimbo item;
//  - any other weapon throws one and is removed.
// Returns true if anything was dropped.
// @0x84900 CBasePlayer::DropWeapon — drop the player's ACTIVE TS weapon into the world.
bool CBasePlayer::DropWeapon()
{
	CBasePlayer *pPlayer = this;
	pPlayer->DropFlag(false); // CTF: the drop key also drops a carried flag

	CTSGun *pGun = GetPlayerTSGun(pPlayer);
	if (!pGun)
		return false;

	const int type = pGun->m_weaponType;
	if (type < 1 || type >= TSW_COUNT || !pGun->m_items[type].owned)
		return false;
	const s_weapon_info *wi = get_weapon_info((tsweapon_t)type);
	if (!wi->pszWorldModel) // kung-fu / katana &c. have no droppable world model
		return false;

	// 1) combinable akimbo -> throw two base singles, remove both items.
	if (wi->iFireType == TSFT_AKIMBO && wi->iSingleType >= 1 && wi->iSingleType < TSW_COUNT)
	{
		const int single = wi->iSingleType;
		const int total = pGun->m_iClip; // live total (= leftClip+rightClip)
		const int half = total / 2;
		TSThrowWorldGun(pPlayer, pGun, single, half, 4.0f);
		TSThrowWorldGun(pPlayer, pGun, single, total - half, -4.0f);
		pGun->DropWeapon(type, true);   // remove the akimbo (auto-selects next)
		pGun->DropWeapon(single, true); // and the single base, if it was also owned
		EMIT_SOUND(ENT(pPlayer->pev), CHAN_ITEM, "weapons/weapondrop.wav", VOL_NORM, ATTN_NORM);
		return true;
	}

	// 2) single whose akimbo we ALSO own -> throw one, keep the other (clip = akimbo right barrel).
	if (wi->iAkimboType >= 1 && wi->iAkimboType < TSW_COUNT && pGun->m_items[wi->iAkimboType].owned)
	{
		const int akimbo = wi->iAkimboType;
		const int thrownClip = pGun->m_iClip;                 // the dropped gun keeps the live clip
		const int keptClip = pGun->m_items[akimbo].rightClip; // the kept single takes the other barrel
		TSThrowWorldGun(pPlayer, pGun, type, thrownClip, 0.0f);
		pGun->m_items[type].clip = keptClip;
		pGun->DropWeapon(akimbo, false);     // remove the akimbo item (type stays owned + active)
		pGun->m_iClip = keptClip;          // refresh the live clip of the kept single
		EMIT_SOUND(ENT(pPlayer->pev), CHAN_ITEM, "weapons/weapondrop.wav", VOL_NORM, ATTN_NORM);
		return true;
	}

	// 3) plain weapon -> throw one, remove it.
	TSThrowWorldGun(pPlayer, pGun, type, pGun->m_iClip, 0.0f);
	pGun->DropWeapon(type, true);
	EMIT_SOUND(ENT(pPlayer->pev), CHAN_ITEM, "weapons/weapondrop.wav", VOL_NORM, ATTN_NORM);
	return true;
}

// On death, scatter the player's whole arsenal into the world as CTSWorldGuns (the dead
// player's arsenal is discarded on respawn, so no removal bookkeeping is needed).
void TSDropWeaponsOnDeath(CBasePlayer *pPlayer)
{
	CTSGun *pGun = GetPlayerTSGun(pPlayer);
	if (!pGun)
		return;
	for (int type = 1; type < TSW_COUNT; type++)
	{
		if (!pGun->m_items[type].owned)
			continue;
		const s_weapon_info *wi = get_weapon_info((tsweapon_t)type);
		if (!wi->pszWorldModel) // skip melee/things without a droppable world model
			continue;
		int clip = (type == pGun->m_weaponType) ? pGun->m_iClip : pGun->m_items[type].clip;
		// Route through the same throw path as a live drop. The player is dead here, so ThrowWeapon
		// takes its dead branch (random angles). VERIFY: scatter direction/feel vs the original.
		TSThrowWorldGun(pPlayer, pGun, type, clip, 0.0f);
	}
}

// On death, drop the player's per-caliber ammo reserve as a single CTSAmmoPack.
void TSDropAmmoOnDeath(CBasePlayer *pPlayer)
{
	CTSGun *pGun = GetPlayerTSGun(pPlayer);
	if (!pGun)
		return;
	Vector vecVel(RANDOM_FLOAT(-40, 40), RANDOM_FLOAT(-40, 40), RANDOM_FLOAT(80, 120));
	// Drop the per-caliber reserve AND the player's armor (VERIFY: orig carries pev->armorvalue).
	int armor = (int)pPlayer->pev->armorvalue;
	CTSAmmoPack::CreateAmmoPack(pPlayer->pev, pGun->m_ammoReserve, armor,
		pPlayer->pev->origin + Vector(0, 0, 20), vecVel);
}
#endif // !CLIENT_DLL (drop-into-world helpers)

// Pick up a dropped weapon of `type` carrying `clip` loaded rounds (CTSWorldGun pickup):
// add it to the arsenal (combining if it triggers an akimbo upgrade), then restore the
// carried clip — unless the pickup combined into the akimbo (which sets its own barrels).
// Returns false when the pickup didn't land (won't fit / already-owned duplicate) so the
// caller can leave the world item on the floor (the original PickUpWeapon failure contract).
// Add per-caliber reserve to the player's TS gun (CTSGroundAmmo refill station).
bool TSAddAmmo(CBasePlayer *pPlayer, int caliber, int count)
{
	CTSGun *pGun = GetPlayerTSGun(pPlayer);
	return pGun ? pGun->AddAmmo(caliber, count) : false;
}
