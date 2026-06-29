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
#include "ts_bulletmgr.h"
#include "ts_player.h" // DoubleFireRate (powerup accessor)
#include "ts_worldgun.h" // CTSWorldGun::CreateWeapon / CTSAmmoPack::CreateAmmoPack (drop side)

// WEAPON TIMING: the build defines CLIENT_WEAPONS (both targets), so UTIL_WeaponTimeBase() == 0 — a
// 0-based RELATIVE clock that gets DECREMENTED each frame by gpGlobals->frametime, on the CLIENT by
// the prediction engine and on the SERVER by CBasePlayer::ItemPostFrame's decrement loop (player.cpp,
// `#if defined(CLIENT_WEAPONS)` + `gun->UseDecrement()`). CTSGun is now CLIENT-PREDICTED (UseDecrement()
// == true), so its attack/idle/reload timers use UTIL_WeaponTimeBase() like every predicted weapon —
// set/compared against that base, decremented on both sides, consistent for prediction.
// (cont.69q earlier used absolute gpGlobals->time as a workaround for "fire-once-stuck"; that was only
// needed because UseDecrement() was false → the server decrement loop above skipped CTSGun. With
// UseDecrement()==true the server decrements it, so the 0-base elapses correctly. See EVENTS.md.)
// NOTE: real-time DELTAS (m_lastCCTime combo window, m_blockBar stamina regen) keep gpGlobals->time —
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

// Lazily-registered TS weapon-info HUD message (the ammo readout). Name + fixed size
// are byte-faithful to LinkUserMessages (reference/HUD_MESSAGES.md): "WeaponInfo", 6.
static int gmsgWeaponInfo = 0;
// The weapon-select bar message: "WStatus", size 3 = BYTE type, BYTE owned, BYTE clip.
static int gmsgWStatus = 0;
static int gmsgActItems = 0; // per-player attach broadcast (laser/flashlight visible to all clients)
static int gmsgKFuPower = 0; // melee block bar (stamina 0..100, CHAR) — owner-only, gmsgKFuPower @UpdateMeleeBar

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
	m_slashStep = 0;
	m_lastCCTime = 0.0f;
	m_blockBar = 100.0f;
	m_blockBarTime = 0.0f;
	m_blockCooldown = 0.0f;

	// per-fire transient timers
	m_burstCount = 0;
	m_shotgunPhase = 0;
	m_akimboSide = 0;
	m_flLastShotTime = 0.0f;

	// change-detect cache → force a fresh HUD resend next frame
	m_iClientWeaponType = -1;
	m_iClientClip = -1;
	m_iClientReserve = -1;
	m_iClientFireMode = -1;
	m_iClientAttach = -1;
	m_iClientEquip = -1;
	m_iClientBlockBar = -1;
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
	m_burstCount = 0;
	m_shotgunPhase = 0; // a weapon switch cancels any in-progress pump reload
	m_fInSpecialReload = 0; // start on a FIRE-step (the pump fire/rack toggle; see ShotgunFire)
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
			used += get_weapon_info((tsweapon_t)t)->slotCost;
	return used;
}

bool CTSGun::TakeWeapon(tsweapon_t type)
{
	if (type < 0 || type >= TSW_COUNT)
		return false;
	const s_weapon_info *wi = get_weapon_info(type);
	if (!wi->pszViewModel)
		return false; // invalid weapon (e.g. the unused flag slot 29)

	// The active item's live clip lives in m_iClip; park it so the combine reads the current
	// rounds when the single being upgraded is the one we're holding.
	SyncActiveToItem();

	int fill = type; // the item slot we will fill
	if (m_items[type].owned)
	{
		// Already own this type. If it upgrades to an akimbo we don't own yet, fill the
		// akimbo (the combine — the ONE case picking up an owned weapon is allowed). Otherwise it's a
		// duplicate of something we already carry: just top the caliber reserve (no second pickup).
		if (wi->iAkimboType >= 0 && wi->iAkimboType < TSW_COUNT &&
			get_weapon_info((tsweapon_t)wi->iAkimboType)->pszViewModel && !m_items[wi->iAkimboType].owned)
		{
			fill = wi->iAkimboType;
		}
		else
		{
			// We already own this weapon and it can't combine into a new akimbo — so DON'T pick it up
			// (leave the world weapon on the floor). User-directed: no re-picking-up owned weapons.
			return false;
		}
	}
	else
	{
		// A fresh (un-owned) weapon — the encumbrance gate: refuse it if it won't fit the 81 slots
		// (GetSpeedBySlots capacity). Akimbo COMBINES above bypass this (you already carry the single).
		if (SlotsUsed() + wi->slotCost > 81)
			return false;
	}

	// --- fill items[fill] (give path; world-pickup carrying a saved status is the SLICE case) ---
	const s_weapon_info *fwi = get_weapon_info((tsweapon_t)fill);
	TSItem &it = m_items[fill];
	it.owned = true;
	it.mode = TSDefaultFireMode(fwi->iFireModes);
	it.attach = fwi->attachBuiltin; // integral attachments ship with the gun (AUG/Barrett scope, MP5SD/Ruger silencer)
	it.barrelMask = 0;

	if (fwi->iFireType == TSFT_AKIMBO && fwi->iSingleType >= 0 && fwi->iSingleType < TSW_COUNT)
	{
		// combinable akimbo: right barrel = a fresh single; left barrel = the single you already hold
		// (its current clip), or — when bought directly as the akimbo (single not owned) — a full clip,
		// so a freshly-bought akimbo comes with BOTH barrels loaded (not one empty).
		const int single = fwi->iSingleType;
		const int singleClip = get_weapon_info((tsweapon_t)single)->iClip;
		it.leftClip = m_items[single].owned ? m_items[single].clip : singleClip;
		it.rightClip = singleClip;
		it.clip = it.leftClip + it.rightClip;
		AddAmmo(get_weapon_info((tsweapon_t)single)->iCaliber, singleClip);
	}
	else if (fwi->iFireType == TSFT_AKIMBO_ONLY)
	{
		// gold colts: two barrels, each half the full clip.
		it.leftClip = fwi->iClip / 2;
		it.rightClip = fwi->iClip - it.leftClip;
		it.clip = fwi->iClip;
		AddAmmo(fwi->iCaliber, fwi->iClip * 3);
	}
	else
	{
		it.clip = fwi->iClip;
		AddAmmo(fwi->iCaliber, fwi->iClip * 3); // give/refill the caliber's shared reserve
	}

	SelectWeapon(fill);
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

// @ MultiplyClips__6CTSGun — the ammo (infinite-clip) powerup. USER-DIRECTED: it instantly sets the
// active weapon's LOADED CLIP to DOUBLE the weapon's max clip (not double the current load), so you can
// fire 2x before reloading. Returns true if it raised the clip.
bool CTSGun::MultiplyClips()
{
	const s_weapon_info *wi = get_weapon_info((tsweapon_t)m_weaponType);
	const int want = wi->iClip * 2; // double the MAX clip
	TSItem &it = m_items[m_weaponType];
	if (wi->iFireType == TSFT_AKIMBO || wi->iFireType == TSFT_AKIMBO_ONLY) // akimbo: split across the two barrels
	{
		if (it.leftClip + it.rightClip >= want)
			return false;
		it.leftClip = wi->iClip;
		it.rightClip = wi->iClip;
		m_iClip = it.leftClip + it.rightClip;
		return true;
	}
	if (m_iClip >= want)
		return false;
	m_iClip = want;
	it.clip = want;
	return true;
}

// @ SelectWeapon__6CTSGun (@0x80fb8) + the ClientCommand commit (@0xae433): holster the
// current weapon, set the new active type, then deploy. Faithful flow = holster -> deploy
// (the original's SelectWeapon calls HolsterWeapon then the caller Deploy()s if it changed);
// switching therefore cancels a reload and re-plays the draw animation/delay. Type 0 (kung-fu)
// is always selectable; any other type must be owned (the .so gates on items[type].owned).
void CTSGun::SelectWeapon(int type)
{
	if (type < 0 || type >= TSW_COUNT || type == m_weaponType)
		return;
	if (type != TSW_MELEE && !m_items[type].owned)
		return;

	// --- HolsterWeapon (@0x85698): park the outgoing weapon's per-item state. ---
	SyncActiveToItem();                // save the wielded weapon's clip + fire-mode
#ifndef CLIENT_DLL
	if (m_pPlayer && m_pPlayer->pev->fov != 0.0f) // leave any scope zoom when switching away (HolsterWeapon unzooms)
		Unzoom();
#endif
	m_fInReload = false;               // a switch cancels an in-progress reload
	m_burstCount = 0;
	m_lastType = m_weaponType;         // remember the outgoing type (for `lastinv`)

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
}

void CTSGun::SetWeaponData(const weapon_data_t &data)
{
	m_weaponType = data.iuser1;
	UpdateActivePointers();
	m_fireMode = data.iuser3;
	m_bHoldPosition = (data.iuser4 != 0);
	const s_weapon_info *wi = get_weapon_info((tsweapon_t)m_weaponType);
	const int caliber = wi->iCaliber;
	if (caliber > 0 && caliber < TS_NUM_CALIBERS)
		m_ammoReserve[caliber] = data.iuser2;
}

// Per-tsweapon_t viewmodel DRAW (unholster) sequence index — recovered by parsing each viewmodel's
// sequence names (the "draw"/"drawb" sequence). The original looks this up at init into the weapon
// anim set (item+0x28 +4); we bake the result. Passed to DefaultDeploy so switching to a weapon plays
// its unholster animation. Index = tsweapon_t.
static const int s_drawSeq[TSW_COUNT] = {
	1, 2, 4, 2, 5, 2, 4, 2, 2, 4, 5, 2, 2, 2, 2, 1, 2, 1, 2, 2,
	5, 2, 2, 1, 1, 2, 7, 2, 3, 2, 1, 3, 1, 1, 1, 1, 1, 1};

bool CTSGun::Deploy()
{
	const s_weapon_info *wi = get_weapon_info((tsweapon_t)m_weaponType);
	if (!wi->pszViewModel)
		return false;
	// NB: do NOT reset m_iClientAttach here to force a re-broadcast — it re-fired on every deploy and
	// broke the fresh-buy laser. The map-change loss was the CLIENT clearing its g_playerAttach array on
	// VidInit; that clear was removed (cl_dll/ts_attachments.cpp) so the array stays in sync with this
	// change-detect cache across a level change. See [[map-change-state-reset]].
	// SLICE: faithful Deploy uses the items array + GetBodyByItems + a weapon-param
	// message; here we just show the view/player model via DefaultDeploy.
	// Kung-fu (TSW_MELEE) has NO 3rd-person weapon model — it's bare hands, so the player must
	// hold NOTHING (empty weaponmodel) and just play the kung-fu body animations. Falling back to
	// the VIEW model here put v_melee on the player's back/hands (a phantom held weapon). For any
	// real weapon with no p_model we likewise show nothing rather than the viewmodel.
	const char *pmodel = wi->pszPlayerModel ? wi->pszPlayerModel : "";
	const int drawSeq = (m_weaponType >= 0 && m_weaponType < TSW_COUNT) ? s_drawSeq[m_weaponType] : 0;
	// Kung-fu (TSW_MELEE) uses an EMPTY anim-ext (grip) — the original keys the bare-hands
	// combat_idle stance + the close-combat attack set off m_szAnimExtention being empty
	// (no invented m_bKungFu bool). Real weapons carry onehanded/twohanded/akimbo.
	const char *animExt = (m_weaponType == TSW_MELEE) ? "" : (wi->pszAnimExt ? wi->pszAnimExt : "onehanded");
	bool ok = DefaultDeploy((char *)wi->pszViewModel, (char *)pmodel, drawSeq, (char *)animExt);
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

// @ PrimaryAttack__6CTSGun -> Fire -> {Standard,Shotgun,Knife,Katana,Grenade,Akimbo}Fire.
// Faithful dispatch by the weapon's fire-type, using the recovered per-weapon
// rate/spread (StandardFire reads s_weapon_info+0x18 rate / +0x28 spread / +0x348
// fire-type). SLICE: the faithful path dispatches on the dual-wield items-array
// (m_pActiveItem/m_pActiveInfo) — here we dispatch on the single held m_weaponType.
// Melee/grenade/akimbo split out to MeleeAttack/GrenadeThrow/AkimboFire.
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
	if (wi->iFireType == TSFT_KNIFE || wi->iFireType == TSFT_KATANA)
	{
		MeleeAttack(wi);
		return;
	}
	if (wi->iFireType == TSFT_GRENADE)
	{
		GrenadeThrow(wi);
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
	case TSFM_AUTO:
	case TSFM_PUMP:        // pump shotgun: fire while HELD (orig PrimaryAttack mode 3 = `button & 1`) — but
	                       // ShotgunFire ALTERNATES shot/rack per held frame, so you don't full-auto bullets.
	case TSFM_AKIMBO_BOTH:
		fire = held; // full-auto / pump / akimbo-both: fire while held
		break;
	case TSFM_SEMI:
	case TSFM_AKIMBO_ALT:
		// one round per pull (semi).
		fire = pressed;
		break;
	case TSFM_BURST:
		// 3-round burst (2 for the MAC-10): start on a fresh pull, then run the queue
		// down (here + in WeaponIdle) so a tap fires the whole burst.
		if (m_burstCount > 0)
			fire = true;
		else if (pressed)
		{
			m_burstCount = (m_weaponType == TSW_MAC10) ? 2 : 3;
			fire = true;
		}
		break;
	}

	if (fire)
		Fire();
}

// @ Fire__6CTSGun — fire ONE shot, dispatching by fire-type (the trigger gating happens
// in PrimaryAttack/WeaponIdle). Akimbo alternates barrels; everything else fires bullets.
void CTSGun::Fire()
{
	const s_weapon_info *wi = get_weapon_info((tsweapon_t)m_weaponType);

	// Pump-shotgun RACK step (orig Fire@0x71c78): a pump shotgun in pump mode whose toggle is set is the
	// non-firing RACK half of the cycle — it must run even on an empty clip (to rack on the last shell).
	const bool pumpRackStep = (wi->iFireType == TSFT_PUMP && m_fireMode == TSFM_PUMP &&
		m_fInSpecialReload != 0);

	if (m_iClip <= 0 && !pumpRackStep)
	{
		PlayEmptySound();
		m_flNextPrimaryAttack = (UTIL_WeaponTimeBase() + 0.2f);
		m_burstCount = 0;
		return;
	}

	if (wi->iFireType == TSFT_AKIMBO || wi->iFireType == TSFT_AKIMBO_ONLY)
		AkimboFire(wi);
	else if (wi->iFireType == TSFT_PUMP)
		ShotgunFire(wi);
	else
		StandardFire(wi);
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
	if (wi->bUsesSpread)
		ScaleSpread(flSpread, wi->flRecoil, (m_pPlayer->pev->flags & FL_DUCKING) != 0);

	m_iClip--;
	m_pPlayer->pev->effects |= EF_MUZZLEFLASH; // 3rd-person muzzleflash on the player model
	m_pPlayer->SetAnimation(PLAYER_ATTACK1);

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
	PLAYBACK_EVENT_FULL(FEV_NOTHOST, m_pPlayer->edict(), m_usFire, 0.0, g_vecZero, g_vecZero,
		flSpread, flViewKick, m_weaponType, m_items[m_weaponType].attach, (m_iClip == 0) ? 1 : 0, 0);

	Vector vecSrc = m_pPlayer->GetGunPosition();
	Vector vecAiming = gpGlobals->v_forward;
#ifndef CLIENT_DLL
	// Server: penetrating fire (glass/thin walls let the round through to what's behind, decalling
	// both faces) — the original ShootBulletsPlayer path.
	TSFireBulletsPenetrate(m_pPlayer, cPellets, vecSrc, vecAiming,
		Vector(flSpread, flSpread, flSpread), 8192,
		(int)(wi->flDamage * TSDamageMult()), m_pPlayer->random_seed, wi->iCaliber);
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
		if (m_burstCount > 0)
			m_burstCount--;
		flRate = (m_burstCount <= 0) ? 0.2f : 0.03f;
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
		((m_fireMode == TSFM_BURST && m_burstCount > 0) ? flRate : 5.0f);
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
	if (m_shotgunPhase != 0)
	{
		m_shotgunPhase = 2;
		return;
	}

	// RACK half of the cycle: pump mode (3) with the toggle set. No bullet, no shell consumed.
	if (m_fireMode == TSFM_PUMP && m_fInSpecialReload != 0)
	{
		float flRate = wi->flFireRate > 0 ? wi->flFireRate : 0.15f;
		if (DoubleFireRate(m_pPlayer))
			flRate /= 1.5f;
		m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + flRate;
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 2.0f;

		int pumpSeq = 8; // v_m3 / v_spas12 "pump"
		const char *snd = "weapons/m3/m3-pump.wav";
		if (m_weaponType == TSW_MOSSBERG) { pumpSeq = 2; snd = "weapons/mossberg/scock.wav"; }
		else if (m_weaponType == TSW_SPAS12) { snd = "weapons/spas12/spas12-pump.wav"; }
		SendWeaponAnim(pumpSeq); // predicted on the host (runs in the predicted ItemPostFrame)
#ifndef CLIENT_DLL
		// Rack sound server-only (the host already predicted the anim; emitting the sound client-side too
		// would double it). All clients — incl. the host — hear it from the server. On CHAN_ITEM (NOT
		// CHAN_WEAPON) so the rack does NOT cut off the still-playing gunshot — the fire sound (fire.sc/
		// EV_TSFire) is on CHAN_WEAPON, so a same-channel rack 0.225s later truncates it. RE: all 3 pump
		// shotguns DEFAULT to PUMP mode (TakeWeapon: AUTO>PUMP>BURST>SEMI; masks benelli/spas=SEMI+PUMP,
		// mossberg=PUMP-only) — so held = auto fire/rack alternation, a tap = one step. Faithful.
		if (m_pPlayer)
			EMIT_SOUND_DYN(ENT(m_pPlayer->pev), CHAN_ITEM, snd, VOL_NORM, ATTN_NORM, 0, TSSlowPitch(m_pPlayer));
#endif
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
// CBasePlayer::PreThink. `increment` = the weapon's flRecoil (s_weapon_info+0x34). Growth is faster when
// firing rapidly (small gap → low recovery) and capped at 0.1; ducking + bullet-time tighten it.
void CTSGun::ScaleSpread(float &spread, float increment, bool ducking)
{
	if (ducking)
		spread *= 0.875f;                 // ducked → tighter base cone
	if (increment <= 0.0f)
		return;                           // weapon has no dynamic recoil
	float factor = 2.0f;
	if (DoubleFireRate(m_pPlayer)) factor *= 0.5f;    // bullet-time fires softer
	if (ducking)                   factor *= 0.775f;
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

// @ KnifeFire/KatanaFire — a short forward hull trace; on a hit, melee damage.
// SLICE: no swing/hit sounds (the TS knife sounds aren't precached here) and no
// faithful katana throw/charge — just the close-range slash.
void CTSGun::MeleeAttack(const s_weapon_info *wi)
{
	// RE KnifeFire melee branch: NOT one-per-press — holding ATTACK slashes continuously, gated only by
	// m_flNextPrimaryAttack (flFireRate), exactly like the kung-fu CCombatAttack path. (Was a press-only
	// gate, so the knife could only slash once per click.)
	UTIL_MakeVectors(m_pPlayer->pev->v_angle);
	Vector vecSrc = m_pPlayer->GetGunPosition();
	const float flRange = (wi->iFireType == TSFT_KATANA) ? 64.0f : 48.0f;
	Vector vecEnd = vecSrc + gpGlobals->v_forward * flRange;

	TraceResult tr;
	UTIL_TraceLine(vecSrc, vecEnd, dont_ignore_monsters, ENT(m_pPlayer->pev), &tr);

	// 1st-person viewmodel slash — CLIENT-PREDICTED (CTSGun is a UseDecrement weapon). The melee viewmodels
	// carry MULTIPLE slash sequences; the original cycles them (RE: 3 body anims 0x1d+RANDOM(0,2), viewmodel
	// via knife.sc). We cycle them DETERMINISTICALLY (m_slashStep, predicted in lockstep with the server) so
	// the held slash animates through its variants without snapping on reconcile. // VERIFY seq indices.
	const bool isKatana = (wi->iFireType == TSFT_KATANA);
	const int slashBase = (m_weaponType == TSW_KNIFE) ? 3 : 2; // v_knife slash1=3; v_katana/v_sealknife slash1=2
	const int nSlash = 3;                                       // RE: 3 slash variants (0x1d/1e/1f body anims)
	SendWeaponAnim(slashBase + (m_slashStep % nSlash));
	m_slashStep++;
#ifdef CLIENT_DLL
	// Knife/katana "bounce-back" screen-shake, per swing (RE client FUN_1000a270 @0xa345: Shake_Setup(amp =
	// flDamage*0.15, dur 0.2, freq 10) — knife dmg 30 -> amp 4.5, live-verified). Client-predicted path =
	// local player; transient render-only shake (TS_ApplyMeleeShake in V_CalcRefdef). The fire-rate gate
	// (m_flNextPrimaryAttack below) means MeleeAttack runs once per swing, so prediction replay won't re-fire.
	TS_TriggerViewShake(wi->flDamage * 0.15f);
#endif
	// Swing whoosh (per attack) — server-driven EMIT_SOUND (a client no-op during prediction).
	EMIT_SOUND_DYN(ENT(m_pPlayer->pev), CHAN_WEAPON, isKatana ? "weapons/katana/miss.wav" : "weapons/knife/knife_miss.wav", VOL_NORM, ATTN_NORM, 0, TSSlowPitch(m_pPlayer));
	m_pPlayer->SetAnimation(PLAYER_ATTACK1);
	const float flRate = wi->flFireRate > 0 ? wi->flFireRate : 0.25f;
	m_flNextPrimaryAttack = (UTIL_WeaponTimeBase() + flRate);
	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 5.0f;

	if (tr.flFraction >= 1.0f)
		return; // miss

	CBaseEntity *pHit = CBaseEntity::Instance(tr.pHit);
	if (pHit)
	{
		// Surface vs flesh hit sound (players/monsters → body; world/props → blade-on-surface).
		const bool flesh = pHit->IsPlayer() || (pHit->pev->flags & FL_MONSTER) != 0;
		const char *hitSnd = isKatana
			? (flesh ? "weapons/katana/hit-body.wav" : "weapons/katana/hit.wav")
			: (flesh ? "weapons/knife/knife_hitbody.wav" : "weapons/knife/knife_hit.wav");
		EMIT_SOUND_DYN(ENT(m_pPlayer->pev), CHAN_ITEM, hitSnd, VOL_NORM, ATTN_NORM, 0, TSSlowPitch(m_pPlayer));
		// events/knife.sc → EV_TSMelee: blade-on-surface spark + decal (the original FUN_10043c90).
		// SERVER-AUTHORITATIVE (not predicted): the client-predicted melee world-trace is unreliable
		// and usually misses, so playing it from prediction (FEV_NOTHOST) gave the local slasher no
		// spark. The server's trace reliably hits, so broadcast to EVERYONE incl. the host (flags 0).
		// Only on hard surfaces (not flesh) — impact point in the origin, player aim in the angles so
		// the client can trace the surface (material + normal).
#ifndef CLIENT_DLL
		if (!flesh && m_usMelee)
			PLAYBACK_EVENT_FULL(0, m_pPlayer->edict(), m_usMelee, 0.0,
				tr.vecEndPos, m_pPlayer->pev->v_angle, 0.0, 0.0, 0, 0, 0, 0);
#endif
		ClearMultiDamage();
		pHit->TraceAttack(m_pPlayer->pev, wi->flDamage * TSDamageMult(), gpGlobals->v_forward, &tr,
			DMG_CLUB | DMG_NEVERGIB);
		ApplyMultiDamage(m_pPlayer->pev, m_pPlayer->pev);
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

	// remap the original anim id → v_melee.mdl 1st-person seq + the 3rd-person close-combat
	// ACTIVITY CODE (CTSAnimation::HandleCloseCombat maps the code to the body sequence name).
	int vseq, bodyAct;
	switch (oa)
	{
	case A_PUNCH:     { static const int jv[3]={2,3,10}; static const int jb[3]={TSA_CC_PUNCHLEFT,TSA_CC_PUNCHRIGHT,TSA_CC_PUNCHAIR};
	                    int j = (jabIdx >= 0 && jabIdx < 3) ? jabIdx : 0; vseq = jv[j]; bodyAct = jb[j]; break; }
	case A_PUNCH2:    vseq = 10; bodyAct = TSA_CC_PUNCHAIR;  break; // special punch
	case A_AIRPUNCH:  vseq = 10; bodyAct = TSA_CC_PUNCHAIR;  break;
	case A_UPPERCUT:  vseq = 7;  bodyAct = TSA_CC_UPPERCUT;  break;
	case A_HANDSTAND: vseq = 6;  bodyAct = TSA_CC_SIDEKICK;  break;
	case A_BACKKICK:  vseq = 9;  bodyAct = TSA_CC_BACKKICK;  break;
	case A_DROPKICK:  vseq = 8;  bodyAct = TSA_CC_CIRCSPIN;  break;
	case A_SPIN_A: case A_SPIN_D: case A_SPIN_E: case A_SPIN_F:
	                  vseq = 8;  bodyAct = TSA_CC_CIRCSPIN;  break; // spin finisher
	case A_GKICK_A: case A_GKICK_B:
	case A_FLY_12: case A_FLY_13:
	default:          vseq = 5;  bodyAct = TSA_CC_CIRCKICK;  break; // ground / flying kick
	}

	UTIL_MakeVectors(ppev->v_angle);
	Vector vecSrc = m_pPlayer->GetGunPosition();
	Vector vecEnd = vecSrc + gpGlobals->v_forward * range;
	TraceResult tr;
	UTIL_TraceLine(vecSrc, vecEnd, dont_ignore_monsters, ENT(m_pPlayer->pev), &tr);

	SendWeaponAnim(vseq);                        // the v_melee.mdl punch/kick (1st-person fists)
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
	// 3rd-person close-combat body anim (HandleCloseCombat builds the seq + holds the torso 2.0s).
	m_pPlayer->m_TSAnim.Animate(m_pPlayer, bodyAct);
	m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + refire;   // orig: both timers = base + local_10c
	m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + refire;
	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 5.0f;

	// Kung-fu hit sounds (RE: the .so close-combat path + player/* sounds): a PLAYER/monster hit is
	// the wet "kungfuhit.wav" punch (+ damage); a WORLD/surface hit thuds (a footstep-material sound
	// pl_step*) under the swing; a clean miss (air swing) is just the swing whoosh (closecombat.wav).
	CBaseEntity *pHit = (tr.flFraction < 1.0f) ? CBaseEntity::Instance(tr.pHit) : nullptr;
	const bool flesh = (pHit && pHit->pev->takedamage != DAMAGE_NO);
	const bool surface = (tr.flFraction < 1.0f && !flesh);

	if (flesh)
	{
		// Kung-fu BLOCK (RE VictimKungfuHold@0x66f50): a player victim holding a block (their block window,
		// opened by CCombatBlock = IN_RELOAD while unarmed/melee) DEFLECTS this close-combat hit — the
		// "kungfuhit.wav" clack plays but NO damage lands, and the ATTACKER is stunned 0.5s while the blocker
		// recovers in 0.25s. So a kung-fu reload-block beats an incoming kick.
		CBasePlayer *pVictim = pHit->IsPlayer() ? (CBasePlayer *)pHit : nullptr;
		const bool blocked = pVictim && pVictim->m_flKungFuBlock > gpGlobals->time;
		EMIT_SOUND_DYN(ENT(m_pPlayer->pev), CHAN_WEAPON, "player/kungfuhit.wav", VOL_NORM, ATTN_NORM, 0, TSSlowPitch(m_pPlayer));
		if (blocked)
		{
			m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 0.5f;   // attacker stunned (VictimKungfuHold)
			m_flNextSecondaryAttack = m_flNextPrimaryAttack;
			pVictim->m_flKungFuBlock = gpGlobals->time + 0.25f;     // blocker holds/recovers 0.25 more
		}
		else
		{
			ClearMultiDamage();
			pHit->TraceAttack(m_pPlayer->pev, dmg * TSDamageMult(), gpGlobals->v_forward, &tr, DMG_CLUB | DMG_NEVERGIB);
			ApplyMultiDamage(m_pPlayer->pev, m_pPlayer->pev);
		}
	}
	else
	{
		// miss OR surface: the swing whoosh.
		EMIT_SOUND_DYN(ENT(m_pPlayer->pev), CHAN_WEAPON, "player/closecombat.wav", VOL_NORM, ATTN_NORM, 0, TSSlowPitch(m_pPlayer));
		if (surface)
		{
			// + the footstep-style surface thud (the material the fist struck).
			char step[24];
			sprintf(step, "player/pl_step%d.wav", RANDOM_LONG(1, 4));
			EMIT_SOUND(ENT(m_pPlayer->pev), CHAN_ITEM, step, VOL_NORM, ATTN_NORM);

			// A kung-fu wall hit puffs the same impact SMOKE as bullets — but ONLY the smoke: no decal,
			// no bullet hole, no sparks. A short-lived additive smoke sprite a touch off the surface.
			Vector pos = tr.vecEndPos + tr.vecPlaneNormal * 2.0f;
			MESSAGE_BEGIN(MSG_PVS, SVC_TEMPENTITY, pos);
			WRITE_BYTE(TE_SPRITE);
			WRITE_COORD(pos.x);
			WRITE_COORD(pos.y);
			WRITE_COORD(pos.z);
			WRITE_SHORT(MODEL_INDEX("sprites/gun_smoke_add.spr")); // precached in PrecacheAllTSWeapons
			WRITE_BYTE(4);   // scale 0.4 (matches the bullet-impact puff)
			WRITE_BYTE(150); // brightness
			MESSAGE_END();
		}
	}
}

// @ CCombatBlock__6CTSGun (@0x85ebc) — kung-fu "reload" = the block taunt. Plays the "hooah"
// (player/block.wav) + the block_low stance, and briefly gates attacks (sets the next-attack times).
// It costs stamina (the .so's melee bar, UpdateMeleeBar(33) → ~3 blocks); spamming it past empty
// plays the exhausted breathing (player/breathe2.wav) instead and does not block. The bar regens.
// Continuous regen of the melee block bar (m_blockBar -> 100). Idempotent: advances by the time since
// the last call. (RE: the original regens field_0x774 via UpdateMeleeBar with a negative amount.)
void CTSGun::RegenBlockBar()
{
	if (m_blockBarTime == 0.0f)
		m_blockBarTime = gpGlobals->time;
	// Regen is ALWAYS on — even while blocking (matches the original). The rate is low enough that 4
	// rapid blocks (0.5s apart, -25 each) still out-drain it to empty, while paced blocks sustain longer.
	m_blockBar += (gpGlobals->time - m_blockBarTime) * 25.0f; // ~25/sec continuous -> ~4s full refill (user-tuned)
	if (m_blockBar > 100.0f)
		m_blockBar = 100.0f;
	m_blockBarTime = gpGlobals->time;
}

// @ CCombatBlock__6CTSGun (@0x85ebc) — RELOAD with ANY melee weapon (kung-fu/knife/katana/seal-knife) =
// the block taunt. Plays the "hooah" (player/block.wav) + a block stance, opens a brief deflect window,
// and gates attacks. It costs stamina (the melee bar, UpdateMeleeBar(33) -> ~3 blocks); spamming it past
// empty plays the exhausted breathing (player/breathe2.wav), does NOT block, and starts a recovery
// lockout. The bar regens continuously (RegenBlockBar, networked via gmsgKFuPower).
void CTSGun::CCombatBlock()
{
	if (m_flNextPrimaryAttack > UTIL_WeaponTimeBase())
		return; // mid-attack / already blocking

	RegenBlockBar();

	const float kCost = 33.0f; // RE UpdateMeleeBar(33); with the ~25/sec regen the net visible step is ~25% and
	                           // 4 rapid blocks empty it (the regen tops up a little between presses)
	if (m_blockBar < kCost || gpGlobals->time < m_blockCooldown)
	{
		// exhausted — the Valve breathing puff, no block, short lockout (RE: field_0x12c recovery delay).
		EMIT_SOUND(ENT(m_pPlayer->pev), CHAN_VOICE, "player/breathe2.wav", VOL_NORM, ATTN_NORM);
		if (m_blockBar < kCost)
			m_blockCooldown = gpGlobals->time + 1.5f;
		m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 0.5f;
		m_flNextSecondaryAttack = m_flNextPrimaryAttack;
		return;
	}

	m_blockBar -= kCost;
	EMIT_SOUND(ENT(m_pPlayer->pev), CHAN_VOICE, "player/block.wav", VOL_NORM, ATTN_NORM); // "hooah"
	SendWeaponAnim(4); // block_low stance (seq 4, 1st person; VERIFY per knife/katana viewmodel)
	if (m_weaponType == TSW_MELEE)
		m_pPlayer->m_TSAnim.Animate(m_pPlayer, TSA_CC_BLOCK); // 3rd-person kungfu/knife/katana block pose
	// RE CCombatBlock: re-arm next primary/secondary at +0.5 (fc_0_5), idle at +3.0 (fc_3) — you can
	// re-block ~twice a second (faster than our old 0.8 lockout).
	m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 0.5f;
	m_flNextSecondaryAttack = m_flNextPrimaryAttack;
	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 3.0f;
	// Open the BLOCK window (RE CCombatBlock@0x85ebc sets player+0x7a8 = 0.5): for the next 0.5s an
	// incoming close-combat attack is deflected (CCombatAttack -> VictimKungfuHold checks this).
	m_pPlayer->m_flKungFuBlock = gpGlobals->time + 0.5f;
}

// @ AlternateAttack__6CTSGun @0x81ba8 — the +alt2 input: pistol-whip (ColdCock) when the
// active weapon has a round loaded (the .so gates on the active slot's clip > 0).
void CTSGun::AlternateAttack()
{
	if (m_flNextPrimaryAttack > UTIL_WeaponTimeBase())
		return; // mid-attack cooldown
	if (m_iClip <= 0)
		return; // .so: only coldcock with a loaded weapon
	ColdCock();
}

// @ ColdCock__6CTSGun @0x71dd0 — a quick melee bash with the held weapon (CBasePlayer::
// ColdCockAttack@0x86610 in the original). Short hull-trace in front; on hit deals club
// damage. Reuses the knife melee sounds (no dedicated coldcock sound ships). SLICE: the
// faithful coldcock viewmodel anim rides events/coldcock.sc (not yet reconstructed); we
// drive the 3rd-person attack anim + sound server-side.
void CTSGun::ColdCock()
{
	UTIL_MakeVectors(m_pPlayer->pev->v_angle);
	Vector vecSrc = m_pPlayer->GetGunPosition();
	Vector vecEnd = vecSrc + gpGlobals->v_forward * 40.0f;

	TraceResult tr;
	UTIL_TraceLine(vecSrc, vecEnd, dont_ignore_monsters, ENT(m_pPlayer->pev), &tr);

	m_pPlayer->SetAnimation(PLAYER_ATTACK1);
	m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 0.5f; // pistol-whip cadence
	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 5.0f;

	if (tr.flFraction >= 1.0f)
		return; // miss (swing is the visual; no sound)

	EMIT_SOUND_DYN(ENT(m_pPlayer->pev), CHAN_WEAPON, "player/kungfuhit.wav", VOL_NORM, ATTN_NORM, 0, TSSlowPitch(m_pPlayer));
	CBaseEntity *pHit = CBaseEntity::Instance(tr.pHit);
	if (pHit)
	{
		ClearMultiDamage();
		pHit->TraceAttack(m_pPlayer->pev, 25.0f * TSDamageMult(), gpGlobals->v_forward, &tr, DMG_CLUB | DMG_NEVERGIB);
		ApplyMultiDamage(m_pPlayer->pev, m_pPlayer->pev);
	}
}

// Trigger the active TS gun's alternate attack (+alt2 / coldcock) for a player.
void TSAlternateAttack(CBasePlayer *pPlayer)
{
	CTSGun *pGun = GetPlayerTSGun(pPlayer);
	if (pGun)
		pGun->AlternateAttack();
}

#ifndef CLIENT_DLL

// @ ShootBulletsPlayer (@0x6f69c) — the TS bullet path's PENETRATION (the original's pierce
// recursion). Fires cShots pellets (player-synced spread, mirroring FireBulletsPlayer), and on each
// brush/world hit lets the bullet PUNCH THROUGH a thin surface to damage what's behind: after a hit,
// step `min(damage*2, 48)` forward; if that point is NOT inside world solid (the slice is thinner
// than the bullet can pierce, OR it's an entity brush like glass), the bullet continues — decalling
// BOTH the entry face and the exit (back) face, and re-tracing onward. So firing at glass breaks it
// AND the round hits the wall beyond, with bullet-holes punched in each surface it crossed. Server
// only: the client's prediction FireBulletsPlayer (hl_weapons.cpp) just does the spread/recoil.
// (still inside the #ifndef CLIENT_DLL opened above — these are server-only)
void TSFireBulletsPenetrate(CBasePlayer *pPlayer, int cShots, Vector vecSrc, Vector vecDir,
	Vector vecSpread, float flDist, int iDamage, int seed, int caliber)
{
	entvars_t *pevAttacker = pPlayer->pev;
	const Vector vecRight = gpGlobals->v_right;
	const Vector vecUp = gpGlobals->v_up;

	// Per-pierce damage multiplier, RE-matched to ShootBulletsPlayer: damage_adjust = the `damagemult`
	// cvar VALUE (read raw as *(damagemult+0xc)), NOT a fixed 0.75. It defaults to 0, so a penetrating
	// round does ZERO damage to layers beyond the first — they get a bullet DECAL but do NOT break
	// (this is why shooting two glass panes breaks only the first and just decals the one behind it).
	const float damageAdjust = CVAR_GET_FLOAT("damagemult");

	// RE ShootBulletsPlayer `rbullet` gate: when the shooter is in slow-motion (slow factor < 0.99),
	// each pellet becomes a flying bullet (CBulletManager) instead of an instant hitscan — it crosses
	// the map and damages where it actually arrives (dodgeable in slow-pause). See ts_bulletmgr.cpp.
	float shooterSlow = pPlayer->pev->fuser1;
	if (shooterSlow < 0.01f)
		shooterSlow = 0.01f;
	const bool slowBullets = shooterSlow < 0.99f;

	for (int iShot = 1; iShot <= cShots; iShot++)
	{
		// player-synced gaussian spread (same formula as FireBulletsPlayer)
		float x = UTIL_SharedRandomFloat(seed + iShot, -0.5, 0.5) +
			UTIL_SharedRandomFloat(seed + (1 + iShot), -0.5, 0.5);
		float y = UTIL_SharedRandomFloat(seed + (2 + iShot), -0.5, 0.5) +
			UTIL_SharedRandomFloat(seed + (3 + iShot), -0.5, 0.5);
		Vector dir = vecDir + x * vecSpread.x * vecRight + y * vecSpread.y * vecUp;

		if (slowBullets)
		{
			g_BulletManager.FireBullet(pPlayer, vecSrc, dir, (float)iDamage, caliber); // flies as a tracked bullet
			continue;
		}

		Vector start = vecSrc;
		float dmg = (float)iDamage;

		// Penetration chain, RE-matched to CBasePlayer::ShootBulletsPlayer (@0x5f69c): one UNIFORM loop —
		// no glass-specific branch. Each layer takes the damage (ApplyMultiDamage shatters glass iff dmg >=
		// its health), then the round penetrates iff the surface is thinner than min(dmg*2, 48): we step
		// `beyond` that far along the ray and continue when UTIL_PointContents(beyond) != CONTENTS_SOLID.
		// Because PointContents only tests WORLD brushes, entity brushes (glass/doors) always read non-solid,
		// so the bullet always passes through them (leaving a hole) whether or not they shattered — exactly
		// the original. (cap = runaway backstop; the original recurses with mode=2.)
		for (int pen = 0; pen < 8; pen++)
		{
			TraceResult tr;
			UTIL_TraceLine(start, start + dir * flDist, dont_ignore_monsters, ENT(pevAttacker), &tr);
			if (tr.flFraction == 1.0f)
				break; // hit nothing

			CBaseEntity *pHit = CBaseEntity::Instance(tr.pHit);
			ClearMultiDamage();
			gMultiDamage.type = DMG_BULLET | DMG_NEVERGIB;
			pHit->TraceAttack(pevAttacker, dmg, dir, &tr, DMG_BULLET | DMG_NEVERGIB); // orig DMG bits 0x1002
			// NB: the bullet ric impact SOUND is played CLIENT-side now (cl_dll/ev_ts.cpp EV_TSBulletImpact),
			// not here. The old server EMIT_AMBIENT_SOUND on the WORLD entity spawned a PERSISTENT static
			// sound at the impact point, so far-away wall hits were heard "as you walked up to them".
			// NO server-side bullet decal — the original lays it CLIENT-side (cl_dll/ev_ts.cpp
			// EV_TSBulletImpact -> R_DecalShoot "{shot"); its server ShootBulletsPlayer lays none. Doing
			// it server-side broke drop-in compat (double decal vs the original client; none vs the
			// original server). The client decals only the FIRST hit (one trace), so there's no exit hole.
			ApplyMultiDamage(pevAttacker, pevAttacker); // shatters glass iff dmg >= pane health (20)

			if (dmg <= 5.0f) // orig: damage > 5.0 to keep piercing
				break;
			float penDepth = dmg * 2.0f; // orig: min(dmg*2, 48)
			if (penDepth > 48.0f)
				penDepth = 48.0f;
			Vector beyond = tr.vecEndPos + dir * penDepth;
			if (UTIL_PointContents(beyond) == CONTENTS_SOLID)
				break; // world surface thicker than the round can pierce — stop here

			// (No exit-hole decal — the original lays decals client-side on the first hit only, so there
			// is no back-face mark. Penetration still steps past this layer for the damage pass below.)
			start = beyond; // continue past this layer
			dmg *= damageAdjust; // orig damage_adjust = damagemult cvar value (default 0 → 0 dmg beyond layer 1)
		}
	}
}

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
// @ GrenadeFire__6CTSGun — toss an M61 grenade (one per trigger pull).
void CTSGun::GrenadeThrow(const s_weapon_info *wi)
{
	if (!(m_pPlayer->m_afButtonPressed & IN_ATTACK))
		return; // one throw per press

	if (m_iClip <= 0)
	{
		PlayEmptySound();
		m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 0.5f;
		return;
	}
	m_iClip--;

	UTIL_MakeVectors(m_pPlayer->pev->v_angle);
	Vector vecSrc = m_pPlayer->GetGunPosition() + gpGlobals->v_forward * 16;
	Vector vecVel = gpGlobals->v_forward * 800 + m_pPlayer->pev->velocity;
	// The M61's blast damage/radius are fixed in CM61Grenade::WaitThink (the original carries no
	// per-throw damage); just launch it with the 4s fuse.
	TSThrowM61(m_pPlayer->pev, vecSrc, vecVel, 4.0f);

	SendWeaponAnim(3); // 1st-person "throw" (v_m61.mdl seq 3) — client-predicted
	m_pPlayer->m_TSAnim.Animate(m_pPlayer, TSA_GRENADE); // 3rd-person "grenade" throw
	m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 1.0f;
	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 5.0f;

	// The grenade is have/don't-have (clip 1): throwing the last one drops the slot + auto-selects the
	// next-best weapon — it vanishes from the arsenal exactly like the thrown knife (USER-DIRECTED).
	if (m_iClip <= 0)
	{
		m_items[m_weaponType].owned = false;
		GetBetterWeapon(m_weaponType);
	}
}

// @ KnifeFire__6CTSGun (throw branch, reached via SecondaryAttack@0x817a8) — right-click hurls the
// active knife as a tumbling CTSKnife projectile. The arsenal carries a stack of throwables (m_iClip);
// each throw spends one, and running dry auto-selects the next-best weapon. The viewmodel plays the
// model's "throw" sequence (v_knife throw=7, v_sealknife throw=5) — predicted via SendWeaponAnim.
void CTSGun::ThrowKnife()
{
	const s_weapon_info *wi = get_weapon_info((tsweapon_t)m_weaponType);
	if (!(m_pPlayer->m_afButtonPressed & IN_ATTACK2))
		return; // one throw per press
	if (m_iClip <= 0)
	{
		PlayEmptySound();
		m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + 0.5f;
		return;
	}
	m_iClip--;

	UTIL_MakeVectors(m_pPlayer->pev->v_angle);
	Vector vecSrc = m_pPlayer->GetGunPosition() + gpGlobals->v_forward * 16;
	Vector vecVel = gpGlobals->v_forward * 1900; // RE KnifeFire: _UNK_00107f68 = 1900 (no player-vel term)
	const float dmg = (wi->flDamage > 0 ? wi->flDamage : 40.0f) * TSDamageMult();
	TSThrowKnife(m_pPlayer, m_weaponType, dmg, vecSrc, vecVel); // server-only (client stub no-ops)

	const bool isKatana = (wi->iFireType == TSFT_KATANA);
	SendWeaponAnim((m_weaponType == TSW_KNIFE) ? 7 : 5); // "throw" seq — client-predicted (VERIFY katana seq)
	EMIT_SOUND_DYN(ENT(m_pPlayer->pev), CHAN_WEAPON,
		isKatana ? "weapons/katana/miss.wav" : "weapons/knife/knife_miss.wav", VOL_NORM, ATTN_NORM, 0, TSSlowPitch(m_pPlayer));
	m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 0.5f;
	m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + 0.5f;
	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 5.0f;

	// Thrown the last one — drop the (now empty) knife slot + auto-select the next-best weapon.
	if (m_iClip <= 0)
	{
		m_items[m_weaponType].owned = false;
		GetBetterWeapon(m_weaponType);
	}
}

// @ AkimboFire__6CTSGun — the dual-wield fire-types (4 combinable, 5 gold-only). The
// faithful engine splits the clip into a left/right barrel (m_pActiveItem+0x1c/+0x20)
// and, in alternating fire-mode, fires one barrel per trigger (IN_ATTACK vs IN_ATTACK2)
// from a ±5u muzzle offset. FUNCTIONAL reconstruction on our single-clip arsenal: two
// guns fire continuously while held, alternating the muzzle side each shot.
void CTSGun::AkimboFire(const s_weapon_info *wi)
{
	TSItem &it = m_items[m_weaponType];

	if (it.leftClip + it.rightClip <= 0)
	{
		m_iClip = 0;
		PlayEmptySound();
		m_flNextPrimaryAttack = (UTIL_WeaponTimeBase() + 0.2f);
		return;
	}

	// RE AkimboFire (@0x729c0): `fireBoth = bothButtonsHeld || mode==AUTO`. In AUTO mode (the default
	// for the full-auto akimbos — uzi_akimbo's fire-mode mask is 0x21, AUTO bit set), BOTH guns fire
	// every shot (this is why our uzi looked like it "switched hands" — it was stuck alternating).
	// The SEMI/AKIMBO_ALT modes fire one alternating barrel per pull (pistol akimbos like the gold/
	// berettas default to that). SLICE: the mode-4/5 L-click-one/R-click-other split is the secondary-
	// attack work (task #15 part 2).
	const bool fireBoth = (m_fireMode == TSFM_AUTO || m_fireMode == TSFM_AKIMBO_BOTH)
		|| ((m_pPlayer->pev->button & (IN_ATTACK | IN_ATTACK2)) == (IN_ATTACK | IN_ATTACK2));

	// Which barrel(s) fire this shot (0 = right, 1 = left).
	int sides[2], nSides = 0;
	if (fireBoth)
	{
		if (it.rightClip > 0) sides[nSides++] = 0;
		if (it.leftClip  > 0) sides[nSides++] = 1;
	}
	else
	{
		int fireLeft;
		if ((it.barrelMask & 6) == 2)      fireLeft = 1;          // left only
		else if ((it.barrelMask & 6) == 4) fireLeft = 0;          // right only
		else                               fireLeft = (m_akimboSide ^ 1) & 1; // alternate
		if (fireLeft && it.leftClip <= 0 && it.rightClip > 0) fireLeft = 0;
		else if (!fireLeft && it.rightClip <= 0 && it.leftClip > 0) fireLeft = 1;
		sides[nSides++] = fireLeft;
		m_akimboSide = fireLeft;
	}

	m_pPlayer->pev->effects |= EF_MUZZLEFLASH;
	m_pPlayer->SetAnimation(PLAYER_ATTACK1);

	UTIL_MakeVectors(m_pPlayer->pev->v_angle);
	Vector vecAiming = gpGlobals->v_forward;
	const float flSpread = wi->flSpread > 0 ? wi->flSpread : 0.02f;

	for (int s = 0; s < nSides; s++)
	{
		const int leftSide = sides[s];
		if (leftSide) it.leftClip--; else it.rightClip--;
		// Muzzle offset to the firing gun's side (+5u right / -5u left), like the original.
		Vector vecSrc = m_pPlayer->GetGunPosition() + gpGlobals->v_right * (leftSide ? -5.0f : 5.0f);
#ifndef CLIENT_DLL
		TSFireBulletsPenetrate(m_pPlayer, 1, vecSrc, vecAiming,
			Vector(flSpread, flSpread, flSpread), 8192,
			(int)(wi->flDamage * TSDamageMult()), m_pPlayer->random_seed, wi->iCaliber);
#else
		m_pPlayer->FireBulletsPlayer(1, vecSrc, vecAiming,
			Vector(flSpread, flSpread, flSpread), 8192, BULLET_PLAYER_9MM,
			0, (int)(wi->flDamage * TSDamageMult()), m_pPlayer->pev, m_pPlayer->random_seed);
#endif
	}
	m_iClip = it.leftClip + it.rightClip; // total clip mirrors the two barrels

	// viewmodel fire anim. For the AUTO-both shot, play the model's "both fire" sequence (BOTH guns
	// recoil TOGETHER) — recovered from the akimbo viewmodels: uzi_akimbo akimbo_shoot1=12, beretta
	// linkedshoot=26, gold linkedshoot=7. (Fixes "switches hands"/"only one gun" — the single-gun fire
	// anims animate one gun at a time. Akimbos without a linked anim fall back to alternating.)
	int akAnim;
	if (nSides == 2)
	{
		int bothAnim = -1;
		switch (m_weaponType)
		{
		case 16: bothAnim = 12; break; // uzi_akimbo: akimbo_shoot1 (both guns)
		case 8:  bothAnim = 26; break; // beretta_akimbo: linkedshoot
		case 21: bothAnim = 7;  break; // gold: linkedshoot
		}
		if (bothAnim >= 0)
			akAnim = bothAnim;
		else { m_akimboSide ^= 1; akAnim = m_akimboSide ? wi->iFireAnim : wi->iFireAnimL; } // fallback
	}
	else
		akAnim = sides[0] ? wi->iFireAnimL : wi->iFireAnim;
	if (akAnim >= 0)
		SendWeaponAnim(akAnim);

	// events/fire.sc → EV_TSFire: sound + shell + muzzle flare(s). Faithful packing (StandardFire@0x824a4):
	// iparam2 = ATTACH MASK (bit0 = silencer), bparam1 = (clip==0). The barrel side (0=right, 1=left,
	// 2=BOTH — the client draws both flares for the AUTO-both shot) rides in bparam2: the original splits
	// akimbo into a separate handler/event (firelinked.sc), a SLICE pending #15; bparam2 is our merged-path
	// extension and is unread by the original client for non-akimbo.
	const int eventSide = (nSides == 2) ? 2 : sides[0];
	// fparam2 = the recoil magnitude (EV_TSFire's TS_AddViewRecoil reads it). Akimbo isn't a pump shotgun, so
	// no 0.35x reduction — just the weapon's flViewKick.
	PLAYBACK_EVENT_FULL(FEV_NOTHOST, m_pPlayer->edict(), m_usFire, 0.0, g_vecZero, g_vecZero,
		flSpread, wi->flViewKick, m_weaponType, it.attach, (m_iClip == 0) ? 1 : 0, eventSide);

	float flRate = wi->flFireRate > 0 ? wi->flFireRate : 0.1f;
	if (DoubleFireRate(m_pPlayer))
		flRate *= 0.5f;
	m_flNextPrimaryAttack = (UTIL_WeaponTimeBase() + flRate);
	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 5.0f;
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
	it.active |= (bits & 6); // newly-added laser/flashlight start ACTIVE (on) — useitems toggles them
	return 1;
}

#ifndef CLIENT_DLL
// @ Zoom__6CTSGuni @0x71710 — enter a scope zoom level. Sets the player's networked view FOV; for a
// high-magnification scope (zoomFov1 < 40) it ALSO hides the viewmodel (pev->viewmodel = 0), matching
// the original (you look through the scope, not at the gun).
void CTSGun::Zoom(int fov)
{
	m_pPlayer->pev->fov = (float)fov;
	const s_weapon_info *wi = get_weapon_info((tsweapon_t)m_weaponType);
	if (wi->zoomFov1 < 40)
		m_pPlayer->pev->viewmodel = 0;
}

// @ Unzoom__6CTSGun @0x71748 — leave the scope: clear the view FOV and, if the viewmodel was hidden,
// restore it to the weapon's v_model.
void CTSGun::Unzoom()
{
	m_pPlayer->pev->fov = 0;
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
	if (gmsgWeaponInfo == 0)
		gmsgWeaponInfo = REG_USER_MSG("WeaponInfo", 6);

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
		if (gmsgActItems == 0)
			gmsgActItems = REG_USER_MSG("ActItems", 2);
		m_iClientAttach = activeAttach;
		MESSAGE_BEGIN(MSG_ALL, gmsgActItems);
		WRITE_BYTE(ENTINDEX(pPlayer->edict())); // player entity index
		WRITE_BYTE(activeAttach);               // ACTIVE attach mask (bit 2 laser / 4 flash)
		MESSAGE_END();
	}

	// Melee block bar (gmsgKFuPower): regen the stamina continuously and network it (CHAR, 0..100) to the
	// owner on change. The client draws it as the bottom weapon-box bar, but ONLY while a melee weapon is
	// held (guns have no block bar). Sent before the WeaponInfo early-out so the bar always tracks regen.
	RegenBlockBar();
	{
		const int barv = (int)(m_blockBar + 0.5f);
		if (barv != m_iClientBlockBar)
		{
			if (gmsgKFuPower == 0)
				gmsgKFuPower = REG_USER_MSG("KFuPower", 1);
			m_iClientBlockBar = barv;
			MESSAGE_BEGIN(MSG_ONE, gmsgKFuPower, NULL, pPlayer->pev);
			WRITE_CHAR(barv);
			MESSAGE_END();
		}
	}

	const int equipped = m_items[m_weaponType].attach;
	if (m_weaponType == m_iClientWeaponType && m_iClip == m_iClientClip &&
		reserve == m_iClientReserve && m_fireMode == m_iClientFireMode &&
		equipped == m_iClientEquip)
		return false; // nothing changed

	m_iClientWeaponType = m_weaponType;
	m_iClientClip = m_iClip;
	m_iClientReserve = reserve;
	m_iClientFireMode = m_fireMode;
	m_iClientEquip = equipped;

	MESSAGE_BEGIN(MSG_ONE, gmsgWeaponInfo, NULL, pPlayer->pev);
	WRITE_BYTE(m_weaponType);     // active tsweapon_t
	WRITE_BYTE(m_iClip);          // rounds currently loaded
	WRITE_SHORT(reserve);         // spare rounds for this caliber
	WRITE_BYTE(m_fireMode);       // 0=auto 1=semi 2=burst 3=pump
	WRITE_BYTE(equipped);         // EQUIPPED attachment mask (silencer 1 / laser 2 / flash 4 / scope 8)
	MESSAGE_END();

	SendWeaponStatus(pPlayer);
	return true;
}

// @ the WStatus block of CTSGun::UpdateClientData — drive the weapon-select bar. The
// original sends one WStatus per arsenal slot on change; we diff each weapon type's
// owned/clip against the cached mirror and send only what changed. Wire (byte-faithful
// to the original handler): BYTE type, BYTE owned(0/1), BYTE clip.
void CTSGun::SendWeaponStatus(CBasePlayer *pPlayer)
{
	if (gmsgWStatus == 0)
		gmsgWStatus = REG_USER_MSG("WStatus", 3);

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
		MESSAGE_BEGIN(MSG_ONE, gmsgWStatus, NULL, pPlayer->pev);
		WRITE_BYTE(t);     // tsweapon_t
		WRITE_BYTE(owned); // 1 = in the arsenal
		WRITE_BYTE(clip);  // loaded rounds
		MESSAGE_END();
	}
}

// @ Reload/ReloadWeapon — refill the clip from the weapon's CALIBER reserve: take =
// min(clip-deficit, reserve[caliber]) (@0x84a14).
// @ ItemPostFrame__6CTSGun — drive the TS ammo system per frame. The stock base
// ItemPostFrame gates reload on the player's standard m_rgAmmo, which TS never uses
// (ammo lives in our per-caliber m_ammoReserve), so reload never triggered -> "fire
// til empty, no reload". This override reloads from m_ammoReserve instead: on +reload,
// or automatically when the clip runs dry.
void CTSGun::ItemPostFrame()
{
	const s_weapon_info *wi = get_weapon_info((tsweapon_t)m_weaponType);
	const int caliber = wi->iCaliber;
	const bool hasReserve = (caliber > 0 && caliber < TS_NUM_CALIBERS && m_ammoReserve[caliber] > 0);

	const bool pumpShotgun = (wi->iFireType == TSFT_PUMP);

	// Pump shotguns reload shell-by-shell via the 3-phase ShotgunReload state machine (open ->
	// insert per shell -> close). It self-gates on m_flNextSecondaryAttack, so drive it every frame.
	if (m_fInReload && pumpShotgun)
	{
		ShotgunReload();
	}
	// Every other weapon: finish a normal whole-clip reload once its delay elapses.
	else if (m_fInReload && m_flNextPrimaryAttack <= UTIL_WeaponTimeBase())
	{
		if (caliber > 0 && caliber < TS_NUM_CALIBERS)
		{
			int need = wi->iClip - m_iClip;
			int take = (need < m_ammoReserve[caliber]) ? need : m_ammoReserve[caliber];
			if (take > 0)
			{
				m_iClip += take;
				m_ammoReserve[caliber] -= take;
			}
		}
		// akimbo: redistribute the reloaded total across the two barrels.
		if (wi->iFireType == TSFT_AKIMBO || wi->iFireType == TSFT_AKIMBO_ONLY)
		{
			TSItem &it = m_items[m_weaponType];
			it.leftClip = m_iClip / 2;
			it.rightClip = m_iClip - it.leftClip;
		}
		m_fInReload = false;
	}

	if ((m_pPlayer->pev->button & IN_ATTACK2) && m_flNextSecondaryAttack <= UTIL_WeaponTimeBase())
	{
		// Kung-fu's right-click attacks (back-kick / handstand kick / dropkick) — CCombatAttack
		// reads IN_ATTACK2 too. Knives THROW on right-click (CTSGun::SecondaryAttack@0x817a8 routes
		// fire-type KNIFE -> KnifeFire's throw branch). Other weapons: zoom/coldcock are separate.
		if (m_weaponType == TSW_MELEE)
			CCombatAttack(true);
		else if (wi->iFireType == TSFT_KNIFE || wi->iFireType == TSFT_KATANA)
			ThrowKnife(); // right-click hurls the knife/seal-knife/katana (orig SecondaryAttack -> KnifeFire/KatanaThrow)
		else if ((m_items[m_weaponType].attach & 8) &&
			wi->iFireType != TSFT_AKIMBO && wi->iFireType != TSFT_AKIMBO_ONLY)
		{
			// @ CTSGun::SecondaryAttack@0x817a8 SCOPED branch (item attach & 8 — bought or built-in scope):
			// right-click cycles the scope FOV: 0 (none) -> zoomFov1 -> zoomFov2 -> back to 0. The view FOV +
			// viewmodel-hide are player pev fields the server networks to the client, so the zoom is server-
			// authoritative (no client prediction needed). Timers re-arm primary +0.3 / secondary +1.0 (the
			// recovered doubles @0xf7fb8 / @0xf6df8), the same as the hold-position toggle.
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
			m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 0.3f;
			m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + 1.0f;
		}
		else if (wi->iHoldChange >= 0 &&
			wi->iFireType != TSFT_AKIMBO && wi->iFireType != TSFT_AKIMBO_ONLY)
		{
			// @ CTSGun::SecondaryAttack@0x717a8 hold-position branch — on a non-akimbo gun whose hold
			// anim-block is set (wi+0x28c != -1, !scoped), right-click TOGGLES the alternate viewmodel
			// stance (item+0x2c): play the model's change/rechange transition and re-point the active
			// anim set so the fire anim comes from the held shoot seq. (A scoped weapon would Zoom
			// instead — the attach&8 branch — which we don't wire yet.) Re-arms both timers: the
			// recovered doubles set primary +0.3 (@0xf7fb8), secondary +1.0 (@0xf6df8).
			m_bHoldPosition = !m_bHoldPosition;
			m_items[m_weaponType].holdPosition = m_bHoldPosition;
			SendWeaponAnim(m_bHoldPosition ? wi->iHoldChange : wi->iHoldRechange);
			m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 0.3f;
			m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + 1.0f;
		}
		else if (wi->iFireType == TSFT_AKIMBO || wi->iFireType == TSFT_AKIMBO_ONLY)
		{
			// Akimbo dual-wield: right-click swaps which barrel(s) fire (both->left->right->both).
			// (useitems/SwitchItems is the ATTACHMENT toggle, not this — see SwitchBarrel.)
			SwitchBarrel();
			m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + 0.3f;
		}
		else if (m_pPlayer->m_iKungFu)
		{
			// KUNG-FU POWERUP: while held it lets you close-combat WHILE ARMED — right-click throws a
			// kung-fu KICK (a leg attack; the weapon stays in hand). This is the fallback AFTER the
			// weapon's own secondary (scope/hold-stance), so guns without one gain the kick. (Exact
			// in-game trigger is RE-approximate — flagged for refinement once live-confirmed.)
			CCombatAttack(true);
		}
	}
	else if ((m_pPlayer->pev->button & IN_ATTACK) && m_flNextPrimaryAttack <= UTIL_WeaponTimeBase())
	{
		// @ StopReload__6CTSGun — a pump shot interrupts the shell-by-shell reload (resets item+8).
		if (m_fInReload && pumpShotgun && m_iClip > 0)
		{
			m_fInReload = false;
			m_shotgunPhase = 0;
		}
		PrimaryAttack();
	}
	else if ((m_pPlayer->pev->button & IN_RELOAD) &&
		(m_weaponType == TSW_MELEE || wi->iFireType == TSFT_KNIFE || wi->iFireType == TSFT_KATANA))
	{
		CCombatBlock(); // ALL melee (kung-fu/knife/seal-knife/katana) block on RELOAD — RE CCombatBlock
	}
	else if ((m_pPlayer->pev->button & IN_RELOAD) && !m_fInReload && m_iClip < wi->iClip && hasReserve)
	{
		Reload();
	}
	else if (!(m_pPlayer->pev->button & (IN_ATTACK | IN_ATTACK2)))
	{
		// auto-reload a dry clip (melee/grenade have no caliber -> hasReserve is false)
		if (!m_fInReload && m_iClip <= 0 && hasReserve)
			Reload();
		else if (m_flTimeWeaponIdle <= UTIL_WeaponTimeBase())
			WeaponIdle();
	}
}

// @ Reload__6CTSGun — begin a reload: gate it, play the anim, and arm the delay; the
// actual clip refill from the per-caliber reserve happens in ItemPostFrame when the
// delay elapses (the original draws from the per-caliber armament reserve too).
void CTSGun::Reload()
{
	const s_weapon_info *wi = get_weapon_info((tsweapon_t)m_weaponType);
	int caliber = wi->iCaliber;
	if (caliber <= 0 || caliber >= TS_NUM_CALIBERS)
		return;
	if (m_iClip >= wi->iClip || m_ammoReserve[caliber] <= 0)
		return;

	// @ Reload__6CTSGun routes pump shotguns (fire-type 1) to the shell-by-shell ShotgunReload
	// machine. Begin it here (phase 0 = open); the ItemPostFrame driver advances it each frame.
	if (wi->iFireType == TSFT_PUMP)
	{
		m_fInReload = true;
		m_shotgunPhase = 0;
		ShotgunReload(); // do the open immediately (matches the original's Reload -> ShotgunReload)
		return;
	}

	// Viewmodel reload anim — now CLIENT-PREDICTED (CTSGun is a UseDecrement weapon, cont.69s 3d):
	// SendWeaponAnim runs in the client's predicted ItemPostFrame, so the reload anim plays instantly
	// (the model's embedded clipout/clipin sound events play with it). Verified in-engine.
	// In the hold-position stance, play the held-grip reload variant (RE Reload__6CTSGun reads the
	// reload seq from the active stance's anim block) so the gun ENDS back in the hold stance.
	const int reloadAnim = (m_bHoldPosition && wi->iHoldReloadAnim >= 0) ? wi->iHoldReloadAnim : wi->iReloadAnim;
	if (reloadAnim >= 0)
		SendWeaponAnim(reloadAnim);

	m_pPlayer->m_TSAnim.Animate(m_pPlayer, TSA_RELOAD); // 3rd-person relo_<grip>

	m_fInReload = true;
	m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + TS_RELOAD_TIME;
	m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + TS_RELOAD_TIME;
	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + TS_RELOAD_TIME + 0.5f;
}

// @ ShotgunReload__6CTSGun (@0x84aac) — the pump shotgun's shell-by-shell reload, a 3-phase
// state machine (m_shotgunPhase == item+8) advanced one step per ItemPostFrame while m_fInReload:
//   phase 0 = open the tube (play reload1, lock 0.3s) -> phase 1
//   phase 1 = insert one shell every 0.3s (wi+0x14), play reload2, until the tube is full or the
//             reserve is dry -> phase 2
//   phase 2 = close/pump (play reload3), end the reload, secondary cooldown +3.0 (dc_3)
// reload1/2/3 are consecutive in every pump v_*.mdl, and iReloadAnim already points at reload1, so
// the insert/close seqs are iReloadAnim+1 / iReloadAnim+2. Firing interrupts via the StopReload path.
void CTSGun::ShotgunReload()
{
	const s_weapon_info *wi = get_weapon_info((tsweapon_t)m_weaponType);
	const int caliber = wi->iCaliber;
	const bool haveShell = (caliber > 0 && caliber < TS_NUM_CALIBERS && m_ammoReserve[caliber] > 0);
	// In the hold stance, use the held-grip reload set (e.g. mossberg reload1b/2b/3b at 13/14/15).
	const int reloadBase = (m_bHoldPosition && wi->iHoldReloadAnim >= 0) ? wi->iHoldReloadAnim : wi->iReloadAnim;
	const int openSeq = reloadBase;                          // reload1
	const int insertSeq = (reloadBase >= 0) ? reloadBase + 1 : -1; // reload2
	const int closeSeq = (reloadBase >= 0) ? reloadBase + 2 : -1;  // reload3

	if (m_shotgunPhase == 0)
	{
		// Open the tube — only if there's a shell to chamber and room for it.
		if (haveShell && m_iClip < wi->iClip)
		{
			m_shotgunPhase = 1;
			if (openSeq >= 0)
				SendWeaponAnim(openSeq);
			m_pPlayer->m_TSAnim.Animate(m_pPlayer, TSA_RELOAD); // 3rd-person reload
			const float t = UTIL_WeaponTimeBase() + 0.3f; // dc_0_3
			m_flNextPrimaryAttack = t;   // can't fire until the tube is open (then a shot interrupts)
			m_flNextSecondaryAttack = t; // first shell goes in after the open completes
			m_flTimeWeaponIdle = t;
		}
		else
		{
			m_fInReload = false; // nothing to load
		}
	}
	else if (m_shotgunPhase == 1)
	{
		// Insert one shell per tick until the tube is full or the reserve is empty.
		if (m_flNextSecondaryAttack <= UTIL_WeaponTimeBase())
		{
			if (!haveShell || m_iClip >= wi->iClip)
			{
				m_shotgunPhase = 2;
			}
			else
			{
				m_ammoReserve[caliber]--;
				m_iClip++;
				if (insertSeq >= 0)
					SendWeaponAnim(insertSeq);
				// Individual racking sound per shell loaded (pitched DOWN in slow-motion like every
				// other combat sound). weapons/insert-shell.wav is the TS per-shell shuck.
				EMIT_SOUND_DYN(ENT(m_pPlayer->pev), CHAN_ITEM, "weapons/insert-shell.wav",
					VOL_NORM, ATTN_NORM, 0, TSSlowPitch(m_pPlayer));
				m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + 0.3f; // wi+0x14, per-shell cadence
			}
		}
	}
	else // phase 2 — close/pump and finish
	{
		m_shotgunPhase = 0;
		m_fInReload = false;
		if (closeSeq >= 0)
			SendWeaponAnim(closeSeq);
		m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 0.3f;   // ready to fire after the pump
		m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + 3.0f; // dc_3 — no immediate re-reload
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 0.3f;
	}
}

// @ WeaponIdle__6CTSGun (@0x854dc) — drives the remaining burst rounds: while a burst is
// queued and the idle timer has elapsed, fire the next round (this is how a tapped burst
// completes after the trigger is released).
void CTSGun::WeaponIdle()
{
	if (m_fireMode == TSFM_BURST && m_burstCount > 0 &&
		m_flTimeWeaponIdle <= UTIL_WeaponTimeBase())
	{
		Fire();
		return;
	}
	ResetEmptySound();
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
			m_burstCount = 0;
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

// Akimbo dual-wield barrel swap (right-click / SecondaryAttack on an akimbo weapon). Cycles the
// active item's barrel mask (bits 2/4 = left/right): both(0) -> left(2) -> right(4) -> both(6) ->
// both(0). AkimboFire reads the mask to gate which barrel(s) fire. (SLICE vs the original, whose
// SecondaryAttack@0x817a8 routes akimbo right-click to Fire() — fire-the-other-hand; deferred.)
void CTSGun::SwitchBarrel()
{
	const s_weapon_info *wi = get_weapon_info((tsweapon_t)m_weaponType);
	if (wi->iFireType != TSFT_AKIMBO && wi->iFireType != TSFT_AKIMBO_ONLY)
		return; // not dual-wielding — nothing to swap

	TSItem &it = m_items[m_weaponType];
	const int m = it.barrelMask & 6;
	if (m == 6)
		it.barrelMask = 0;        // both -> default (both)
	else if (m & 2)
		it.barrelMask = 4;        // left -> right
	else if (m & 4)
		it.barrelMask = 6;        // right -> both
	else
		it.barrelMask = 2;        // default -> left

	if (m_pPlayer)
		EMIT_SOUND(ENT(m_pPlayer->pev), CHAN_WEAPON, "weapons/switch.wav", VOL_NORM, ATTN_NORM);
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
	// VERIFY: the exact throw-velocity formula is NOT statically recovered (asm hints 0.9/0.33 scalars
	// + player velocity); needs a live trace. This forward*250 + player-velocity is a placeholder.
	pGun->pev->velocity = gpGlobals->v_forward * 250.0f + pev->velocity;
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
void TSPickUpWeapon(CBasePlayer *pPlayer, int type, int clip)
{
	CTSGun *pGun = GetPlayerTSGun(pPlayer);
	const bool ownedBefore = pGun && pGun->IsOwned(type);

	GiveTSWeapon(pPlayer, (tsweapon_t)type);
	pGun = GetPlayerTSGun(pPlayer);
	if (!pGun)
		return;

	// If we already owned the single and the pickup combined into the akimbo, leave the
	// combined barrel clips alone. Otherwise restore the carried clip on the picked-up type.
	if (!ownedBefore && pGun->IsOwned(type))
	{
		pGun->m_items[type].clip = clip;
		if (type == pGun->m_weaponType)
			pGun->m_iClip = clip;
	}
}

// Add per-caliber reserve to the player's TS gun (CTSGroundAmmo refill station).
bool TSAddAmmo(CBasePlayer *pPlayer, int caliber, int count)
{
	CTSGun *pGun = GetPlayerTSGun(pPlayer);
	return pGun ? pGun->AddAmmo(caliber, count) : false;
}
