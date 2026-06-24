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
#include "ts_player.h" // DoubleFireRate (powerup accessor)

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

// Lazily-registered TS weapon-info HUD message (the ammo readout). Name + fixed size
// are byte-faithful to LinkUserMessages (reference/HUD_MESSAGES.md): "WeaponInfo", 6.
static int gmsgWeaponInfo = 0;
// The weapon-select bar message: "WStatus", size 3 = BYTE type, BYTE owned, BYTE clip.
static int gmsgWStatus = 0;

// `damagemult` cvar (default 0.0 = off → 1x). A value > 0 scales all TS weapon damage.
static float TSDamageMult()
{
	const float m = CVAR_GET_FLOAT("damagemult");
	return (m > 0.0f) ? m : 1.0f;
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
static CTSGun *GetPlayerTSGun(CBasePlayer *pPlayer)
{
	for (int i = 0; i < MAX_ITEM_TYPES; i++)
		for (CBasePlayerItem *p = pPlayer->m_rgpPlayerItems[i]; p; p = p->m_pNext)
			if (FClassnameIs(p->pev, "weapon_tsgun"))
				return (CTSGun *)p;
	return NULL;
}

// @ Spawn__6CTSGun — classname weapon_tsgun, id 16, fall-init as a pickup.
void CTSGun::Spawn()
{
	pev->classname = MAKE_STRING("weapon_tsgun");
	Precache();
	m_iId = TSGUN_WEAPON_ID;

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
}

// Load the active item's clip/mode into the live state (Deploy side).
void CTSGun::LoadActiveFromItem()
{
	if (m_weaponType < 0 || m_weaponType >= TSW_COUNT)
		return;
	const TSItem &it = m_items[m_weaponType];
	m_iClip = it.clip;
	m_fireMode = it.mode;
	m_burstCount = 0;
}

// @ TakeWeapon__6CTSGun (@0x80a04) — add a weapon to the arsenal (fresh world pickup / buy),
// then wield it. The faithful akimbo OWNERSHIP model (reference/WEAPON_SELECT.md):
//  - own the single + pick up another of the same single -> fill the AKIMBO item (the combine):
//    items[akimbo].leftClip = items[single].clip, rightClip = weapon_info[single].clip, the
//    single item is NOT cleared (both stay owned: scroll skips the single, slot1-5 still picks it).
//  - a fresh akimbo (fire-type 4) reads its single base; a gold-colts only-akimbo (fire-type 5)
//    splits its clip into two barrels; a normal single/weapon just loads its full clip.
void CTSGun::AddWeapon(tsweapon_t type)
{
	if (type < 0 || type >= TSW_COUNT)
		return;
	const s_weapon_info *wi = get_weapon_info(type);
	if (!wi->pszViewModel)
		return; // invalid weapon (e.g. the unused flag slot 29)

	// The active item's live clip lives in m_iClip; park it so the combine reads the current
	// rounds when the single being upgraded is the one we're holding.
	SyncActiveToItem();

	int fill = type; // the item slot we will fill
	if (m_items[type].owned)
	{
		// Already own this type. If it upgrades to an akimbo we don't own yet, fill the
		// akimbo (the combine). Otherwise just top the caliber reserve (resupply).
		if (wi->iAkimboType >= 0 && wi->iAkimboType < TSW_COUNT &&
			get_weapon_info((tsweapon_t)wi->iAkimboType)->pszViewModel && !m_items[wi->iAkimboType].owned)
		{
			fill = wi->iAkimboType;
		}
		else
		{
			AddAmmo(wi->iCaliber, wi->iClip * 3); // resupply the shared caliber pool
			return;
		}
	}

	// --- fill items[fill] (give path; world-pickup carrying a saved status is the SLICE case) ---
	const s_weapon_info *fwi = get_weapon_info((tsweapon_t)fill);
	TSItem &it = m_items[fill];
	it.owned = true;
	it.mode = TSDefaultFireMode(fwi->iFireModes);
	it.attach = 0;
	it.barrelMask = 0;

	if (fwi->iFireType == TSFT_AKIMBO && fwi->iSingleType >= 0 && fwi->iSingleType < TSW_COUNT)
	{
		// combinable akimbo: left barrel = the single you already hold, right barrel = a fresh single.
		const int single = fwi->iSingleType;
		it.leftClip = m_items[single].owned ? m_items[single].clip : 0;
		it.rightClip = get_weapon_info((tsweapon_t)single)->iClip;
		it.clip = it.leftClip + it.rightClip;
		AddAmmo(get_weapon_info((tsweapon_t)single)->iCaliber, get_weapon_info((tsweapon_t)single)->iClip);
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

	SelectWeaponType(fill);
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

// @ MultiplyClips__6CTSGun — the infinite-clip powerup. Tops the active weapon's caliber
// reserve toward 2x a full load (capped at the caliber max). Returns true if anything added.
bool CTSGun::MultiplyClips()
{
	const s_weapon_info *wi = get_weapon_info((tsweapon_t)m_weaponType);
	int caliber = wi->iCaliber;
	if (caliber <= 0 || caliber >= TS_NUM_CALIBERS)
		return false;
	int before = m_ammoReserve[caliber];
	int want = before * 2;
	if (want < wi->iClip * 3)
		want = wi->iClip * 3; // at least bring it back to a full resupply
	int max = TSAmmoMax(caliber);
	if (want > max)
		want = max;
	if (want <= before)
		return false;
	m_ammoReserve[caliber] = want;
	return true;
}

// @ SelectWeapon__6CTSGun (@0x80fb8) + the ClientCommand commit (@0xae433): holster the
// current weapon, set the new active type, then deploy. Faithful flow = holster -> deploy
// (the original's SelectWeapon calls HolsterWeapon then the caller Deploy()s if it changed);
// switching therefore cancels a reload and re-plays the draw animation/delay. Type 0 (kung-fu)
// is always selectable; any other type must be owned (the .so gates on items[type].owned).
void CTSGun::SelectWeaponType(int type)
{
	if (type < 0 || type >= TSW_COUNT || type == m_weaponType)
		return;
	if (type != TSW_MELEE && !m_items[type].owned)
		return;

	// --- HolsterWeapon (@0x85698): park the outgoing weapon's per-item state. ---
	SyncActiveToItem();                // save the wielded weapon's clip + fire-mode
	m_fInReload = false;               // a switch cancels an in-progress reload
	m_burstCount = 0;
	m_lastType = m_weaponType;         // remember the outgoing type (for `lastinv`)

	m_weaponType = type;
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
// (priority-last, always owned) is the fallback. Holster->deploy via SelectWeaponType.
void CTSGun::GetBetterWeapon(int exclude)
{
	for (int i = 0; i < TSW_COUNT; i++)
	{
		int t = g_iWeaponPriority[i];
		if (t == exclude || !IsOwned(t))
			continue;
		SelectWeaponType(t);
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
}

void CTSGun::SetWeaponData(const weapon_data_t &data)
{
	m_weaponType = data.iuser1;
	m_fireMode = data.iuser3;
	const s_weapon_info *wi = get_weapon_info((tsweapon_t)m_weaponType);
	const int caliber = wi->iCaliber;
	if (caliber > 0 && caliber < TS_NUM_CALIBERS)
		m_ammoReserve[caliber] = data.iuser2;
}

bool CTSGun::Deploy()
{
	const s_weapon_info *wi = get_weapon_info((tsweapon_t)m_weaponType);
	if (!wi->pszViewModel)
		return false;
	// SLICE: faithful Deploy uses the items array + GetBodyByItems + a weapon-param
	// message; here we just show the view/player model via DefaultDeploy.
	// Kung-fu (TSW_MELEE) has NO 3rd-person weapon model — it's bare hands, so the player must
	// hold NOTHING (empty weaponmodel) and just play the kung-fu body animations. Falling back to
	// the VIEW model here put v_melee on the player's back/hands (a phantom held weapon). For any
	// real weapon with no p_model we likewise show nothing rather than the viewmodel.
	const char *pmodel = wi->pszPlayerModel ? wi->pszPlayerModel : "";
	bool ok = DefaultDeploy((char *)wi->pszViewModel, (char *)pmodel, 0,
		(char *)(wi->pszAnimExt ? wi->pszAnimExt : "onehanded"));
	m_pPlayer->m_TSAnim.m_bKungFu = (m_weaponType == TSW_MELEE); // kung-fu uses the combat_idle stance
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
		CloseCombatAttack(false);
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
	switch (m_fireMode)
	{
	case TSFM_AUTO:
	case TSFM_PUMP:
	case TSFM_AKIMBO_BOTH:
		fire = held; // full-auto / pump / akimbo-both: fire while held
		break;
	case TSFM_SEMI:
	case TSFM_AKIMBO_ALT:
		// one round per pull; the double-firerate powerup lets a semi gun fire held.
		fire = pressed || (DoubleFireRate(m_pPlayer) && held);
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

	if (m_iClip <= 0)
	{
		PlayEmptySound();
		m_flNextPrimaryAttack = (UTIL_WeaponTimeBase() + 0.2f);
		m_burstCount = 0;
		return;
	}

	if (wi->iFireType == TSFT_AKIMBO || wi->iFireType == TSFT_AKIMBO_ONLY)
		AkimboFire(wi);
	else
		StandardFire(wi);
}

// @ StandardFire/ShotgunFire — a single bullet (or a pump-shotgun pellet spread), with the
// weapon's real damage/spread/rate. Burst rounds fire fast then slow on the last (the .so
// uses 0.03 / 0.2; StandardFire @0x824a4).
void CTSGun::StandardFire(const s_weapon_info *wi)
{
	const int cPellets = wi->iPellets > 0 ? wi->iPellets : 1; // 8 for shotguns (s_weapon_info+0x24)
	const float flSpread = wi->flSpread > 0 ? wi->flSpread : 0.01f;

	m_iClip--;
	m_pPlayer->pev->effects |= EF_MUZZLEFLASH; // 3rd-person muzzleflash on the player model
	m_pPlayer->SetAnimation(PLAYER_ATTACK1);

	// Viewmodel "shoot" animation — now CLIENT-PREDICTED (CTSGun is a UseDecrement weapon, cont.69s
	// 3d): SendWeaponAnim runs in the client's predicted ItemPostFrame, so the anim plays instantly.
	if (wi->iFireAnim >= 0)
		SendWeaponAnim(wi->iFireAnim);

	// events/fire.sc → EV_TSFire: predicted fire sound + brass shell + viewmodel muzzleflash. Played
	// with FEV_NOTHOST so the predicting host plays its OWN predicted copy (and isn't sent the server
	// copy too — otherwise the muzzle/shell would double); other clients get the server copy.
	PLAYBACK_EVENT_FULL(FEV_NOTHOST, m_pPlayer->edict(), m_usFire, 0.0, g_vecZero, g_vecZero,
		flSpread, flSpread, m_weaponType, 0, (m_iClip == 0) ? 1 : 0, 0);

	Vector vecSrc = m_pPlayer->GetGunPosition();
	Vector vecAiming = gpGlobals->v_forward;
	m_pPlayer->FireBulletsPlayer(cPellets, vecSrc, vecAiming,
		Vector(flSpread, flSpread, flSpread), 8192, BULLET_PLAYER_9MM,
		0, (int)(wi->flDamage * TSDamageMult()), m_pPlayer->pev, m_pPlayer->random_seed);

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
		if (DoubleFireRate(m_pPlayer)) // double-firerate powerup -> twice as fast
			flRate *= 0.5f;
	}
	m_flNextPrimaryAttack = (UTIL_WeaponTimeBase() + flRate);
	// Short idle during a burst so WeaponIdle drives the remaining rounds; else normal.
	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() +
		((m_fireMode == TSFM_BURST && m_burstCount > 0) ? flRate : 5.0f);
}

// @ KnifeFire/KatanaFire — a short forward hull trace; on a hit, melee damage.
// SLICE: no swing/hit sounds (the TS knife sounds aren't precached here) and no
// faithful katana throw/charge — just the close-range slash.
void CTSGun::MeleeAttack(const s_weapon_info *wi)
{
	if (!(m_pPlayer->m_afButtonPressed & IN_ATTACK))
		return; // one slash per press

	UTIL_MakeVectors(m_pPlayer->pev->v_angle);
	Vector vecSrc = m_pPlayer->GetGunPosition();
	const float flRange = (wi->iFireType == TSFT_KATANA) ? 64.0f : 48.0f;
	Vector vecEnd = vecSrc + gpGlobals->v_forward * flRange;

	TraceResult tr;
	UTIL_TraceLine(vecSrc, vecEnd, dont_ignore_monsters, ENT(m_pPlayer->pev), &tr);

	// 1st-person viewmodel slash — now CLIENT-PREDICTED (CTSGun is a UseDecrement weapon). The melee
	// viewmodels carry slash sequences (v_knife slash1=3; v_katana/v_sealknife slash1=2). A fixed slash
	// keeps the predicted anim in lockstep with the server (random per-swing would snap on reconcile).
	const bool isKatana = (wi->iFireType == TSFT_KATANA);
	SendWeaponAnim((m_weaponType == TSW_KNIFE) ? 3 : 2);
	// Swing whoosh (per attack) — server-driven EMIT_SOUND (a client no-op during prediction).
	EMIT_SOUND(ENT(m_pPlayer->pev), CHAN_WEAPON, isKatana ? "weapons/katana/miss.wav" : "weapons/knife/knife_miss.wav", VOL_NORM, ATTN_NORM);
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
		EMIT_SOUND(ENT(m_pPlayer->pev), CHAN_ITEM, hitSnd, VOL_NORM, ATTN_NORM);
		// events/knife.sc → EV_TSMelee: blade-on-surface spark + decal (the original FUN_10043c90).
		// Only on hard surfaces (not flesh) — the impact point rides in the event origin.
		if (!flesh && m_usMelee)
			PLAYBACK_EVENT_FULL(FEV_NOTHOST, m_pPlayer->edict(), m_usMelee, 0.0,
				tr.vecEndPos, g_vecZero, 0.0, 0.0, 0, 0, 0, 0);
		ClearMultiDamage();
		pHit->TraceAttack(m_pPlayer->pev, wi->flDamage * TSDamageMult(), gpGlobals->v_forward, &tr,
			DMG_CLUB | DMG_NEVERGIB);
		ApplyMultiDamage(m_pPlayer->pev, m_pPlayer->pev);
	}
}

// @ CCombatAttack__6CTSGun (@0x86078) — kung-fu (bare hands) contextual close-combat. The original
// picks the attack from the player's movement buttons + flags + velocity (it has no active weapon
// item, gun+0xcc==0). Recovered dispatch (pev->button bits + FL_ONGROUND/FL_DUCKING, attack ids
// 4-11 → v_melee.mdl sequences 0x0b-0x19, damage 0x23/0x2d/0x37 = 35/45/55):
//   right-click (+IN_ATTACK2):  duck+ground -> handstand/sideways kick (anim 0x14, 45)
//                               else        -> back-kick               (anim 0x11, 55)
//   fire (+IN_ATTACK):  airborne+moving+duck -> dropkick forward        (anim 0x10, 45)
//                       airborne+moving      -> forward/flying punch     (anim 0x12, 35)
//                       ground+duck          -> uppercut                 (anim 0x16, 45)
//                       ground               -> punch combo 0x0d/0e/0f   (35)
// SLICE: the original's exact knockback (CloseCombatAttack / gmsgTSKickAlign) + per-attack reach
// constants (DAT_00107f7c…) are approximated; the selection + anims + damage are RE-faithful.
void CTSGun::CloseCombatAttack(bool fSecondary)
{
	if (m_flNextPrimaryAttack > UTIL_WeaponTimeBase())
		return;

	const int btn = m_pPlayer->pev->button;
	const bool onGround = (m_pPlayer->pev->flags & FL_ONGROUND) != 0;
	const bool ducking = (btn & IN_DUCK) != 0 || (m_pPlayer->pev->flags & FL_DUCKING) != 0;
	const bool back = (btn & IN_BACK) != 0;
	const bool secondary = fSecondary || (btn & IN_ATTACK2) != 0;

	// v_melee.mdl sequences (this model, 11 seqs): 2 punch_left, 3 punch_right, 4 block_low, 5 kick,
	// 6 sidekick, 7 uppercut, 8 spinkick, 9 backkick, 10 punch_right2.
	enum { ANIM_PUNCH_L = 2, ANIM_PUNCH_R = 3, ANIM_KICK = 5, ANIM_SIDEKICK = 6, ANIM_UPPERCUT = 7,
	       ANIM_SPINKICK = 8, ANIM_BACKKICK = 9, ANIM_PUNCH_R2 = 10 };
	int anim;
	float dmg, range, delay;
	if (secondary)
	{
		// RIGHT-CLICK = KICKS only (kicking only ever happens on +attack2).
		if (!onGround && ducking) { anim = ANIM_SPINKICK; dmg = 45; range = 72; delay = 0.6f; }  // jump+duck dropkick/spin
		else if (ducking)         { anim = ANIM_SIDEKICK; dmg = 45; range = 64; delay = 0.5f; }  // duck sideways/handstand kick
		else if (back)            { anim = ANIM_BACKKICK; dmg = 55; range = 72; delay = 0.6f; }  // backward → back-kick
		else                      { anim = ANIM_KICK;     dmg = 50; range = 64; delay = 0.55f; } // plain kick
	}
	else
	{
		// FIRE = PUNCHES only (NEVER kicks — not even with jump). Duck → uppercut. The left/right/
		// forward punch SEQUENCE only runs while attacking rapidly/held; a lone tap is a single punch.
		if (ducking)
		{
			anim = ANIM_UPPERCUT; dmg = 45; range = 48; delay = 0.45f;
		}
		else
		{
			static const int combo[3] = {ANIM_PUNCH_L, ANIM_PUNCH_R, ANIM_PUNCH_R2};
			const bool chaining = (gpGlobals->time - m_lastCCTime) < 0.5f; // held / quick succession
			if (chaining)
			{
				m_comboStep++;
				anim = combo[m_comboStep % 3];
			}
			else
			{
				m_comboStep = 0; // fresh tap → a single regular punch (not the sequence)
				anim = RANDOM_LONG(0, 1) ? ANIM_PUNCH_L : ANIM_PUNCH_R;
			}
			dmg = 35; range = 48; delay = 0.32f;
		}
	}
	m_lastCCTime = gpGlobals->time;

	UTIL_MakeVectors(m_pPlayer->pev->v_angle);
	Vector vecSrc = m_pPlayer->GetGunPosition();
	Vector vecEnd = vecSrc + gpGlobals->v_forward * range;
	TraceResult tr;
	UTIL_TraceLine(vecSrc, vecEnd, dont_ignore_monsters, ENT(m_pPlayer->pev), &tr);

	SendWeaponAnim(anim);                       // the v_melee.mdl punch/kick (1st-person fists)
	// 3rd-person player BODY animation (TS player models have these by name): map the attack to its
	// kung-fu sequence so onlookers see the punch/kick instead of an aim pose.
	const char *bodySeq = "punchright";
	switch (anim)
	{
	case ANIM_PUNCH_L:  bodySeq = "punchleft";  break;
	case ANIM_PUNCH_R:  bodySeq = "punchright"; break;
	case ANIM_PUNCH_R2: bodySeq = "punchair";   break;
	case ANIM_UPPERCUT: bodySeq = "uppercut";   break;
	case ANIM_KICK:     bodySeq = "circkick";   break;
	case ANIM_SIDEKICK: bodySeq = "sidekick";   break;
	case ANIM_SPINKICK: bodySeq = "circspin";   break;
	case ANIM_BACKKICK: bodySeq = "backkick";   break;
	}
	m_pPlayer->m_TSAnim.PlayKungFu(m_pPlayer, bodySeq, delay);
	m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + delay;
	m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + delay;
	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 5.0f;

	// Kung-fu hit sounds (RE: the .so close-combat path + player/* sounds): a PLAYER/monster hit is
	// the wet "kungfuhit.wav" punch (+ damage); a WORLD/surface hit thuds (a footstep-material sound
	// pl_step*) under the swing; a clean miss (air swing) is just the swing whoosh (closecombat.wav).
	CBaseEntity *pHit = (tr.flFraction < 1.0f) ? CBaseEntity::Instance(tr.pHit) : nullptr;
	const bool flesh = (pHit && pHit->pev->takedamage != DAMAGE_NO);
	const bool surface = (tr.flFraction < 1.0f && !flesh);

	if (flesh)
	{
		EMIT_SOUND(ENT(m_pPlayer->pev), CHAN_WEAPON, "player/kungfuhit.wav", VOL_NORM, ATTN_NORM);
		ClearMultiDamage();
		pHit->TraceAttack(m_pPlayer->pev, dmg * TSDamageMult(), gpGlobals->v_forward, &tr, DMG_CLUB | DMG_NEVERGIB);
		ApplyMultiDamage(m_pPlayer->pev, m_pPlayer->pev);
	}
	else
	{
		// miss OR surface: the swing whoosh.
		EMIT_SOUND(ENT(m_pPlayer->pev), CHAN_WEAPON, "player/closecombat.wav", VOL_NORM, ATTN_NORM);
		if (surface)
		{
			// + the footstep-style surface thud (the material the fist struck).
			char step[24];
			sprintf(step, "player/pl_step%d.wav", RANDOM_LONG(1, 4));
			EMIT_SOUND(ENT(m_pPlayer->pev), CHAN_ITEM, step, VOL_NORM, ATTN_NORM);
		}
	}
}

// @ CCombatBlock__6CTSGun (@0x85ebc) — kung-fu "reload" = the block taunt. Plays the "hooah"
// (player/block.wav) + the block_low stance, and briefly gates attacks (sets the next-attack times).
// It costs stamina (the .so's melee bar, UpdateMeleeBar(33) → ~3 blocks); spamming it past empty
// plays the exhausted breathing (player/breathe2.wav) instead and does not block. The bar regens.
void CTSGun::CloseCombatBlock()
{
	if (m_flNextPrimaryAttack > UTIL_WeaponTimeBase())
		return; // mid-attack / already blocking

	// Regen the block bar since it was last touched (~25/sec → full in ~4s), then update the stamp.
	m_blockBar += (gpGlobals->time - m_blockBarTime) * 25.0f;
	if (m_blockBar > 100.0f)
		m_blockBar = 100.0f;
	m_blockBarTime = gpGlobals->time;

	const float kCost = 33.0f; // UpdateMeleeBar(33) — ~3 blocks before empty
	if (m_blockBar < kCost)
	{
		// exhausted — the Valve breathing puff, no block, short lockout.
		EMIT_SOUND(ENT(m_pPlayer->pev), CHAN_VOICE, "player/breathe2.wav", VOL_NORM, ATTN_NORM);
		m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 0.5f;
		m_flNextSecondaryAttack = m_flNextPrimaryAttack;
		return;
	}

	m_blockBar -= kCost;
	EMIT_SOUND(ENT(m_pPlayer->pev), CHAN_VOICE, "player/block.wav", VOL_NORM, ATTN_NORM); // "hooah"
	SendWeaponAnim(4); // block_low stance (v_melee.mdl seq 4, 1st person)
	m_pPlayer->m_TSAnim.PlayKungFu(m_pPlayer, "kungfu_block1", 0.8f); // 3rd-person block pose
	m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 0.8f;   // the block window — gates attacks
	m_flNextSecondaryAttack = m_flNextPrimaryAttack;
	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 1.0f;
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

	EMIT_SOUND(ENT(m_pPlayer->pev), CHAN_WEAPON, "player/kungfuhit.wav", VOL_NORM, ATTN_NORM);
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
	// w_m61 table damage is the per-pickup value; the explosion is area damage.
	float dmg = ((wi->flDamage > 0) ? wi->flDamage * 4.0f : 120.0f) * TSDamageMult();
	TSThrowM61(m_pPlayer->pev, vecSrc, vecVel, 4.0f, dmg);

	SendWeaponAnim(3); // 1st-person "throw" (v_m61.mdl seq 3) — client-predicted
	m_pPlayer->m_TSAnim.Animate(m_pPlayer, TSA_GRENADE); // 3rd-person "grenade" throw
	m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 1.0f;
	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 5.0f;
}

// @ AkimboFire__6CTSGun — the dual-wield fire-types (4 combinable, 5 gold-only). The
// faithful engine splits the clip into a left/right barrel (m_pActiveItem+0x1c/+0x20)
// and, in alternating fire-mode, fires one barrel per trigger (IN_ATTACK vs IN_ATTACK2)
// from a ±5u muzzle offset. FUNCTIONAL reconstruction on our single-clip arsenal: two
// guns fire continuously while held, alternating the muzzle side each shot.
void CTSGun::AkimboFire(const s_weapon_info *wi)
{
	TSItem &it = m_items[m_weaponType];

	// Pick the firing barrel. useitems (SwitchItems) sets it.barrelMask: 2 = left only,
	// 4 = right only, 0/6 = both -> alternate. Skip an empty barrel (fall to the other).
	int fireLeft;
	if ((it.barrelMask & 6) == 2)
		fireLeft = 1;                 // left only
	else if ((it.barrelMask & 6) == 4)
		fireLeft = 0;                 // right only
	else
		fireLeft = (m_akimboSide ^ 1) & 1; // both: alternate
	if (fireLeft && it.leftClip <= 0 && it.rightClip > 0)
		fireLeft = 0;
	else if (!fireLeft && it.rightClip <= 0 && it.leftClip > 0)
		fireLeft = 1;

	if (it.leftClip + it.rightClip <= 0)
	{
		m_iClip = 0;
		PlayEmptySound();
		m_flNextPrimaryAttack = (UTIL_WeaponTimeBase() + 0.2f);
		return;
	}

	if (fireLeft)
		it.leftClip--;
	else
		it.rightClip--;
	m_akimboSide = fireLeft ? 1 : 0;
	m_iClip = it.leftClip + it.rightClip; // total clip mirrors the two barrels

	m_pPlayer->pev->effects |= EF_MUZZLEFLASH;
	m_pPlayer->SetAnimation(PLAYER_ATTACK1);

	// Fire the matching barrel's viewmodel animation (right gun = iFireAnim, left =
	// iFireAnimL; read from the v_*_akimbo.mdl). SLICE: server-driven SendWeaponAnim.
	int akAnim = m_akimboSide ? wi->iFireAnim : wi->iFireAnimL;
	if (akAnim >= 0)
		SendWeaponAnim(akAnim);

	UTIL_MakeVectors(m_pPlayer->pev->v_angle);
	// Muzzle offset to the firing gun's side (+5u right / -5u left), like the original.
	const float side = m_akimboSide ? 5.0f : -5.0f;
	Vector vecSrc = m_pPlayer->GetGunPosition() + gpGlobals->v_right * side;
	Vector vecAiming = gpGlobals->v_forward;
	const float flSpread = wi->flSpread > 0 ? wi->flSpread : 0.02f;

	// events/fire.sc → EV_TSFire: fire sound + shell + the muzzle-flash flare at THIS barrel's
	// attachment (iparam2 = m_akimboSide: 0 = right/attachment 0, 1 = left/attachment 1).
	PLAYBACK_EVENT_FULL(FEV_NOTHOST, m_pPlayer->edict(), m_usFire, 0.0, g_vecZero, g_vecZero,
		flSpread, flSpread, m_weaponType, m_akimboSide, (m_iClip == 0) ? 1 : 0, 0);

	m_pPlayer->FireBulletsPlayer(1, vecSrc, vecAiming,
		Vector(flSpread, flSpread, flSpread), 8192, BULLET_PLAYER_9MM,
		0, (int)(wi->flDamage * TSDamageMult()), m_pPlayer->pev, m_pPlayer->random_seed);

	float flRate = wi->flFireRate > 0 ? wi->flFireRate : 0.1f;
	if (DoubleFireRate(m_pPlayer))
		flRate *= 0.5f;
	m_flNextPrimaryAttack = (UTIL_WeaponTimeBase() + flRate);
	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 5.0f;
}

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

	if (m_weaponType == m_iClientWeaponType && m_iClip == m_iClientClip &&
		reserve == m_iClientReserve && m_fireMode == m_iClientFireMode)
		return false; // nothing changed

	m_iClientWeaponType = m_weaponType;
	m_iClientClip = m_iClip;
	m_iClientReserve = reserve;
	m_iClientFireMode = m_fireMode;

	MESSAGE_BEGIN(MSG_ONE, gmsgWeaponInfo, NULL, pPlayer->pev);
	WRITE_BYTE(m_weaponType);     // active tsweapon_t
	WRITE_BYTE(m_iClip);          // rounds currently loaded
	WRITE_SHORT(reserve);         // spare rounds for this caliber
	WRITE_BYTE(m_fireMode);       // 0=auto 1=semi 2=burst 3=pump
	WRITE_BYTE(0);                // SLICE: weapon capability flags
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

	// Finish a reload once its delay elapses.
	if (m_fInReload && m_flNextPrimaryAttack <= UTIL_WeaponTimeBase())
	{
		if (pumpShotgun)
		{
			// @ ShotgunReload__6CTSGun (@0x84aac) — pump shotguns reload ONE shell at a time
			// (a per-shell state machine, interruptible by firing). Insert a shell, then keep the
			// reload going for the next shell until the tube is full or the reserve is empty.
			if (caliber > 0 && caliber < TS_NUM_CALIBERS && m_ammoReserve[caliber] > 0 && m_iClip < wi->iClip)
			{
				m_iClip++;
				m_ammoReserve[caliber]--;
				EMIT_SOUND(ENT(m_pPlayer->pev), CHAN_ITEM, "weapons/scock1.wav", VOL_NORM, ATTN_NORM);
			}
			if (m_iClip < wi->iClip && caliber > 0 && caliber < TS_NUM_CALIBERS && m_ammoReserve[caliber] > 0)
			{
				// more to load — arm the next shell (and replay the insert anim).
				if (wi->iReloadAnim >= 0)
					SendWeaponAnim(wi->iReloadAnim);
				m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 0.5f;
				m_flNextSecondaryAttack = m_flNextPrimaryAttack;
				m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 0.5f;
			}
			else
			{
				m_fInReload = false; // tube full / out of shells — pump finished
			}
		}
		else
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
	}

	if ((m_pPlayer->pev->button & IN_ATTACK2) && m_flNextPrimaryAttack <= UTIL_WeaponTimeBase())
	{
		// Kung-fu's right-click attacks (back-kick / handstand kick / dropkick) — CCombatAttack
		// reads IN_ATTACK2 too. Other weapons have no secondary here (zoom/coldcock are separate).
		if (m_weaponType == TSW_MELEE)
			CloseCombatAttack(true);
	}
	else if ((m_pPlayer->pev->button & IN_ATTACK) && m_flNextPrimaryAttack <= UTIL_WeaponTimeBase())
	{
		if (m_fInReload && pumpShotgun && m_iClip > 0)
			m_fInReload = false; // a pump shot interrupts the shell-by-shell reload
		PrimaryAttack();
	}
	else if ((m_pPlayer->pev->button & IN_RELOAD) && m_weaponType == TSW_MELEE)
	{
		CloseCombatBlock(); // kung-fu has no clip — reload = the "hooah" block taunt
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

	// Viewmodel reload anim — now CLIENT-PREDICTED (CTSGun is a UseDecrement weapon, cont.69s 3d):
	// SendWeaponAnim runs in the client's predicted ItemPostFrame, so the reload anim plays instantly
	// (the model's embedded clipout/clipin sound events play with it). Verified in-engine.
	if (wi->iReloadAnim >= 0)
		SendWeaponAnim(wi->iReloadAnim);

	m_pPlayer->m_TSAnim.Animate(m_pPlayer, TSA_RELOAD); // 3rd-person relo_<grip>

	m_fInReload = true;
	m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + TS_RELOAD_TIME;
	m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + TS_RELOAD_TIME;
	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + TS_RELOAD_TIME + 0.5f;
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
		pHeld->AddWeapon(type);
		return pHeld;
	}

	edict_t *pent = CREATE_NAMED_ENTITY(MAKE_STRING("weapon_tsgun"));
	if (FNullEnt(pent))
		return NULL;

	CTSGun *pGun = (CTSGun *)CBaseEntity::Instance(pent);
	pGun->m_weaponType = type;
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
		pGun->SelectWeaponType(type);
}

// @ the ClientCommand `lastinv` block (@0xae433, the SelectWeapon+Deploy path): toggle back
// to the previously-wielded type (player+0xa1c). SelectWeaponType itself records m_lastType.
void TSLastInv(CBasePlayer *pPlayer)
{
	CTSGun *pGun = GetPlayerTSGun(pPlayer);
	if (!pGun || pGun->OwnedCount() <= 1)
		return;
	if (pGun->IsOwned(pGun->m_lastType) || pGun->m_lastType == TSW_MELEE)
		pGun->SelectWeaponType(pGun->m_lastType);
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

// @ SwitchItems__6CTSGun = FUN_00081564 (the `useitems` command — default.cfg binds g): the
// akimbo dual-wield "fire one barrel / fire both" swap. Cycles the active item's barrel mask
// (item+0x128, bits 2/4 = left/right): both(0) -> left(2) -> right(4) -> both(6) -> both(0).
// AkimboFire reads the mask to gate which barrel(s) fire. Plays a swap sound.
void CTSGun::SwitchItems()
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

// Swap the held akimbo's active item (the `useitems` command -> CTSGun::SwitchItems).
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
void CTSGun::DropType(int type, bool fThrow)
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

// Spawn a thrown world gun in front of the player (the ThrowWeapon side of DropWeapon).
static void TSThrowWorldGun(CBasePlayer *pPlayer, int type, int clip, float side)
{
	UTIL_MakeVectors(pPlayer->pev->v_angle);
	Vector vecSrc = pPlayer->GetGunPosition() + gpGlobals->v_forward * 16 + gpGlobals->v_right * side;
	Vector vecVel = gpGlobals->v_forward * 250 + pPlayer->pev->velocity;
	TSCreateWorldGun(type, clip, vecSrc, vecVel);
}

// @ CBasePlayer::DropWeapon (@0x84900) — drop the player's ACTIVE TS weapon into the world.
// Faithful akimbo branches (reference/WEAPON_SELECT.md):
//  - dropping a combinable AKIMBO (fire-type 4) throws TWO of its base single (clip split in
//    half, tossed ±4) and removes BOTH items[akimbo] and items[single];
//  - dropping a SINGLE while you also own its akimbo throws ONE single, then keeps the single
//    owned (its clip becomes the akimbo's right barrel) and removes the akimbo item;
//  - any other weapon throws one and is removed.
// Returns true if anything was dropped.
bool TSDropWeapon(CBasePlayer *pPlayer)
{
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
		TSThrowWorldGun(pPlayer, single, half, 4.0f);
		TSThrowWorldGun(pPlayer, single, total - half, -4.0f);
		pGun->DropType(type, true);   // remove the akimbo (auto-selects next)
		pGun->DropType(single, true); // and the single base, if it was also owned
		EMIT_SOUND(ENT(pPlayer->pev), CHAN_ITEM, "items/weapondrop1.wav", VOL_NORM, ATTN_NORM);
		return true;
	}

	// 2) single whose akimbo we ALSO own -> throw one, keep the other (clip = akimbo right barrel).
	if (wi->iAkimboType >= 1 && wi->iAkimboType < TSW_COUNT && pGun->m_items[wi->iAkimboType].owned)
	{
		const int akimbo = wi->iAkimboType;
		const int thrownClip = pGun->m_iClip;                 // the dropped gun keeps the live clip
		const int keptClip = pGun->m_items[akimbo].rightClip; // the kept single takes the other barrel
		TSThrowWorldGun(pPlayer, type, thrownClip, 0.0f);
		pGun->m_items[type].clip = keptClip;
		pGun->DropType(akimbo, false);     // remove the akimbo item (type stays owned + active)
		pGun->m_iClip = keptClip;          // refresh the live clip of the kept single
		EMIT_SOUND(ENT(pPlayer->pev), CHAN_ITEM, "items/weapondrop1.wav", VOL_NORM, ATTN_NORM);
		return true;
	}

	// 3) plain weapon -> throw one, remove it.
	TSThrowWorldGun(pPlayer, type, pGun->m_iClip, 0.0f);
	pGun->DropType(type, true);
	EMIT_SOUND(ENT(pPlayer->pev), CHAN_ITEM, "items/weapondrop1.wav", VOL_NORM, ATTN_NORM);
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
		Vector vecVel(RANDOM_FLOAT(-60, 60), RANDOM_FLOAT(-60, 60), RANDOM_FLOAT(80, 140));
		TSCreateWorldGun(type, clip, pPlayer->pev->origin + Vector(0, 0, 20), vecVel);
	}
}

// On death, drop the player's per-caliber ammo reserve as a single CTSAmmoPack.
void TSDropAmmoOnDeath(CBasePlayer *pPlayer)
{
	CTSGun *pGun = GetPlayerTSGun(pPlayer);
	if (!pGun)
		return;
	Vector vecVel(RANDOM_FLOAT(-40, 40), RANDOM_FLOAT(-40, 40), RANDOM_FLOAT(80, 120));
	TSCreateAmmoPack(pPlayer->pev, pGun->m_ammoReserve, pPlayer->pev->origin + Vector(0, 0, 20), vecVel);
}

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
