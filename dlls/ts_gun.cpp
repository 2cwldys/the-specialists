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

#define TSGUN_WEAPON_ID 16 // CTSGun::GetItemInfo @0x85dd0 sets iId = 0x10
#define TSGUN_WEIGHT 15
#define TS_MAX_ARSENAL 12 // owned-weapon slots (the original items[] is unbounded-ish)

// The single weapon_tsgun (id 16) IS the player's whole TS arsenal. The original
// keeps a `s_weapon_status` items[] array (0x38-byte entries) with m_pActiveItem
// (@0xcc) pointing at the wielded one + weapon/ammo-group masks + akimbo pairing.
// This is a clean MODERN reconstruction of that behaviour (not the byte layout):
// a parallel array of owned weapon types, each with its own clip + reserve, and an
// active slot. m_weaponType tracks the active slot so the fire-type dispatch +
// Deploy keep working. SLICE: no akimbo pairing (a single active slot), no
// weapon/ammo groups, no fire-mode/burst state, no buy menu yet.
class CTSGun : public CBasePlayerWeapon
{
public:
	void Spawn() override;
	void Precache() override;
	bool GetItemInfo(ItemInfo *p) override;
	int iItemSlot() override { return 1; } // @ iItemSlot__6CTSGun returns slot 1
	bool Deploy() override;
	bool UpdateClientData(CBasePlayer *pPlayer) override; // sends the TS WeaponInfo HUD message
	void SendWeaponStatus(CBasePlayer *pPlayer);          // sends the WStatus weapon-bar messages
	void ItemPostFrame() override;
	void PrimaryAttack() override;
	void Reload() override;
	void WeaponIdle() override;

	void Fire();                                 // dispatch one shot by fire-type
	void StandardFire(const s_weapon_info *wi);  // bullet/pellet fire (StandardFire/ShotgunFire)
	void MeleeAttack(const s_weapon_info *wi);   // knife/katana fire-types
	void GrenadeThrow(const s_weapon_info *wi);  // grenade fire-type
	void AkimboFire(const s_weapon_info *wi);    // dual-wield fire-types (4/5)
	void SwitchFireMode();                       // cycle the held weapon's fire-mode

	// arsenal (the items-array). @ AddItem/SelectWeapon/TakeWeapon__6CTSGun.
	int FindSlot(tsweapon_t type);   // owned-slot index, or -1
	void AddWeapon(tsweapon_t type); // add to the arsenal (or resupply) + wield it
	void SelectSlot(int slot);       // wield an owned slot (saves/loads its clip)
	bool RemoveActiveWeapon(int &type, int &clip); // @DropWeapon — drop the active slot
	bool MultiplyClips();            // infinite-clip powerup: multiply the held reserve
	bool AddAmmo(int caliber, int count); // @AddAmmo__6CTSGun — add reserve rounds (capped)

	int m_weaponType = TSW_BERETTA; // ACTIVE tsweapon_t (= m_arsenalType[m_active])
	int m_arsenalType[TS_MAX_ARSENAL] = {};
	int m_arsenalClip[TS_MAX_ARSENAL] = {};    // saved loaded rounds per slot
	int m_arsenalMode[TS_MAX_ARSENAL] = {};    // saved fire-mode per slot
	int m_ammoReserve[TS_NUM_CALIBERS] = {};   // spare rounds PER CALIBER (gun+0xd4 in the .so)
	int m_arsenalCount = 0;
	int m_active = 0;
	int m_akimboSide = 0; // which barrel fired last (alternates muzzle L/R for akimbo)
	int m_fireMode = TSFM_SEMI; // ACTIVE fire-mode (= m_arsenalMode[m_active])
	int m_burstCount = 0;       // rounds left in the current burst (m_pActiveItem+8)

	// Cached "last sent" WeaponInfo fields (the TS ammo HUD). The original sends
	// WeaponInfo on change from CTSGun::UpdateClientData @0x758c8; we mirror that.
	int m_iClientWeaponType = -1;
	int m_iClientClip = -1;
	int m_iClientReserve = -1;
	int m_iClientFireMode = -1;

	// Cached per-weapon-type WStatus (the weapon-select bar): clip if owned, else -1.
	// The original sends WStatus per arsenal slot on change; we diff against this.
	int m_iClientWStatus[TSW_COUNT];
	bool m_bWStatusInit = false;
};

// Lazily-registered TS weapon-info HUD message (the ammo readout). Name + fixed size
// are byte-faithful to LinkUserMessages (reference/HUD_MESSAGES.md): "WeaponInfo", 6.
static int gmsgWeaponInfo = 0;
// The weapon-select bar message: "WStatus", size 3 = BYTE type, BYTE owned, BYTE clip.
static int gmsgWStatus = 0;

LINK_ENTITY_TO_CLASS(weapon_tsgun, CTSGun);

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
	m_arsenalCount = 1;
	m_active = 0;
	m_arsenalType[0] = m_weaponType;
	m_arsenalClip[0] = wi->iClip;
	AddAmmo(wi->iCaliber, wi->iClip * 3); // seed the caliber's reserve (SLICE: buy menu sets this)
	m_arsenalMode[0] = TSDefaultFireMode(wi->iFireModes);
	m_fireMode = m_arsenalMode[0];
	m_iClip = wi->iClip; // SLICE: deploy loaded

	const char *world = wi->pszWorldModel ? wi->pszWorldModel : "models/w_weaponbox.mdl";
	SET_MODEL(ENT(pev), world);

	m_iDefaultAmmo = wi->iClip; // SLICE: stock ammo; TS runs its own ammo system
	FallInit();
}

// @ FindSlot — owned-weapon slot index for `type`, or -1.
int CTSGun::FindSlot(tsweapon_t type)
{
	for (int i = 0; i < m_arsenalCount; i++)
		if (m_arsenalType[i] == (int)type)
			return i;
	return -1;
}

// @ AddItem__6CTSGun / TakeWeapon — add a weapon to the arsenal (or resupply if owned),
// wield it. Akimbo upgrade (TakeWeapon @0x80a04): picking up a 2nd of a single pistol you
// already hold combines the pair into its akimbo variant (s_weapon_info+0xc8, doubled clip).
void CTSGun::AddWeapon(tsweapon_t type)
{
	const s_weapon_info *wi = get_weapon_info(type);

	// Already hold the single version + it has an akimbo upgrade + don't hold the akimbo
	// yet -> upgrade the slot in place to the akimbo (combine the two pistols).
	int single = FindSlot(type);
	if (single >= 0 && wi->iAkimboType >= 0 &&
		FindSlot((tsweapon_t)wi->iAkimboType) < 0)
	{
		const s_weapon_info *aki = get_weapon_info((tsweapon_t)wi->iAkimboType);
		// Wield the single first so m_iClip reflects its live loaded rounds, then upgrade
		// the slot in place: the akimbo's loaded clip = the single's rounds + a fresh 2nd
		// pistol (capped at the doubled clip), reserve carries the 2nd pistol's spares.
		SelectSlot(single);
		int loaded = m_iClip + wi->iClip;
		if (loaded > aki->iClip)
			loaded = aki->iClip;
		m_arsenalType[single] = wi->iAkimboType;
		m_weaponType = wi->iAkimboType;
		m_arsenalClip[single] = loaded;
		m_iClip = loaded;
		AddAmmo(aki->iCaliber, wi->iClip); // the 2nd pistol's rounds (shared caliber pool)
		m_arsenalMode[single] = TSDefaultFireMode(aki->iFireModes);
		m_fireMode = m_arsenalMode[single];
		m_burstCount = 0;
		if (m_pPlayer && m_pPlayer->m_pActiveItem == (CBasePlayerItem *)this)
			Deploy(); // re-deploy with the akimbo view model
		return;
	}

	int slot = FindSlot(type);
	if (slot < 0)
	{
		if (m_arsenalCount >= TS_MAX_ARSENAL)
			return; // arsenal full
		slot = m_arsenalCount++;
		m_arsenalType[slot] = type;
		m_arsenalClip[slot] = wi->iClip;
		m_arsenalMode[slot] = TSDefaultFireMode(wi->iFireModes);
	}
	AddAmmo(wi->iCaliber, wi->iClip * 3); // give/refill the caliber's shared reserve
	SelectSlot(slot);
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

// @ SelectWeapon__6CTSGun — wield an owned slot (save the current clip, load the new).
void CTSGun::SelectSlot(int slot)
{
	if (slot < 0 || slot >= m_arsenalCount)
		return;

	m_arsenalClip[m_active] = m_iClip; // save the wielded weapon's clip
	m_arsenalMode[m_active] = m_fireMode; // and its fire-mode
	m_active = slot;
	m_weaponType = m_arsenalType[slot];
	m_iClip = m_arsenalClip[slot];
	m_fireMode = m_arsenalMode[slot];
	m_burstCount = 0;

	// Re-deploy the new active weapon if we're the player's held item.
	if (m_pPlayer && m_pPlayer->m_pActiveItem == (CBasePlayerItem *)this)
		Deploy();
}

// @ Precache__6CTSGun — faithful precaches 11 event scripts. Models for the whole
// roster are precached at map load (CWorld::Precache -> PrecacheAllTSWeapons), so
// a runtime give of any weapon type finds them cached (no precache-in-spawn error).
void CTSGun::Precache()
{
	// roster models/sounds/events are precached at map load (CWorld::Precache);
	// this re-call just resolves the cached indices for a runtime give.
	PrecacheAllTSWeapons();
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

bool CTSGun::Deploy()
{
	const s_weapon_info *wi = get_weapon_info((tsweapon_t)m_weaponType);
	if (!wi->pszViewModel)
		return false;
	// SLICE: faithful Deploy uses the items array + GetBodyByItems + a weapon-param
	// message; here we just show the view/player model via DefaultDeploy.
	const char *pmodel = wi->pszPlayerModel ? wi->pszPlayerModel : wi->pszViewModel;
	return DefaultDeploy((char *)wi->pszViewModel, (char *)pmodel, 0,
		(char *)(wi->pszAnimExt ? wi->pszAnimExt : "onehanded"));
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
		m_flNextPrimaryAttack = GetNextAttackDelay(0.2);
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
	m_pPlayer->pev->effects |= EF_MUZZLEFLASH;
	m_pPlayer->SetAnimation(PLAYER_ATTACK1);

	// Play the viewmodel's fire animation (the "shoot*" sequence read from v_*.mdl).
	// SLICE: server-driven SendWeaponAnim; the original predicts it client-side via the
	// events/fire.sc event. -1 = weapon has no standard fire anim (melee/grenade/akimbo).
	if (wi->iFireAnim >= 0)
		SendWeaponAnim(wi->iFireAnim);

	if (wi->pszFireSound)
		EMIT_SOUND(ENT(m_pPlayer->pev), CHAN_WEAPON, wi->pszFireSound, VOL_NORM, ATTN_NORM);

	Vector vecSrc = m_pPlayer->GetGunPosition();
	Vector vecAiming = gpGlobals->v_forward;
	m_pPlayer->FireBulletsPlayer(cPellets, vecSrc, vecAiming,
		Vector(flSpread, flSpread, flSpread), 8192, BULLET_PLAYER_9MM,
		0, (int)wi->flDamage, m_pPlayer->pev, m_pPlayer->random_seed);

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
	m_flNextPrimaryAttack = GetNextAttackDelay(flRate);
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

	m_pPlayer->SetAnimation(PLAYER_ATTACK1);
	const float flRate = wi->flFireRate > 0 ? wi->flFireRate : 0.25f;
	m_flNextPrimaryAttack = GetNextAttackDelay(flRate);
	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 5.0f;

	if (tr.flFraction >= 1.0f)
		return; // miss

	CBaseEntity *pHit = CBaseEntity::Instance(tr.pHit);
	if (pHit)
	{
		ClearMultiDamage();
		pHit->TraceAttack(m_pPlayer->pev, wi->flDamage, gpGlobals->v_forward, &tr,
			DMG_CLUB | DMG_NEVERGIB);
		ApplyMultiDamage(m_pPlayer->pev, m_pPlayer->pev);
	}
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
		m_flNextPrimaryAttack = GetNextAttackDelay(0.5);
		return;
	}
	m_iClip--;

	UTIL_MakeVectors(m_pPlayer->pev->v_angle);
	Vector vecSrc = m_pPlayer->GetGunPosition() + gpGlobals->v_forward * 16;
	Vector vecVel = gpGlobals->v_forward * 800 + m_pPlayer->pev->velocity;
	// w_m61 table damage is the per-pickup value; the explosion is area damage.
	float dmg = (wi->flDamage > 0) ? wi->flDamage * 4.0f : 120.0f;
	TSThrowM61(m_pPlayer->pev, vecSrc, vecVel, 4.0f, dmg);

	m_pPlayer->SetAnimation(PLAYER_ATTACK1);
	m_flNextPrimaryAttack = GetNextAttackDelay(1.0);
	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 5.0f;
}

// @ AkimboFire__6CTSGun — the dual-wield fire-types (4 combinable, 5 gold-only). The
// faithful engine splits the clip into a left/right barrel (m_pActiveItem+0x1c/+0x20)
// and, in alternating fire-mode, fires one barrel per trigger (IN_ATTACK vs IN_ATTACK2)
// from a ±5u muzzle offset. FUNCTIONAL reconstruction on our single-clip arsenal: two
// guns fire continuously while held, alternating the muzzle side each shot.
void CTSGun::AkimboFire(const s_weapon_info *wi)
{
	if (m_iClip <= 0)
	{
		PlayEmptySound();
		m_flNextPrimaryAttack = GetNextAttackDelay(0.2);
		return;
	}

	m_iClip--;
	m_akimboSide ^= 1; // alternate barrels (left/right gun)

	m_pPlayer->pev->effects |= EF_MUZZLEFLASH;
	m_pPlayer->SetAnimation(PLAYER_ATTACK1);

	// Fire the matching barrel's viewmodel animation (right gun = iFireAnim, left =
	// iFireAnimL; read from the v_*_akimbo.mdl). SLICE: server-driven SendWeaponAnim.
	int akAnim = m_akimboSide ? wi->iFireAnim : wi->iFireAnimL;
	if (akAnim >= 0)
		SendWeaponAnim(akAnim);

	if (wi->pszFireSound)
		EMIT_SOUND(ENT(m_pPlayer->pev), CHAN_WEAPON, wi->pszFireSound, VOL_NORM, ATTN_NORM);

	UTIL_MakeVectors(m_pPlayer->pev->v_angle);
	// Muzzle offset to the firing gun's side (+5u right / -5u left), like the original.
	const float side = m_akimboSide ? 5.0f : -5.0f;
	Vector vecSrc = m_pPlayer->GetGunPosition() + gpGlobals->v_right * side;
	Vector vecAiming = gpGlobals->v_forward;
	const float flSpread = wi->flSpread > 0 ? wi->flSpread : 0.02f;

	m_pPlayer->FireBulletsPlayer(1, vecSrc, vecAiming,
		Vector(flSpread, flSpread, flSpread), 8192, BULLET_PLAYER_9MM,
		0, (int)wi->flDamage, m_pPlayer->pev, m_pPlayer->random_seed);

	float flRate = wi->flFireRate > 0 ? wi->flFireRate : 0.1f;
	if (DoubleFireRate(m_pPlayer))
		flRate *= 0.5f;
	m_flNextPrimaryAttack = GetNextAttackDelay(flRate);
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
		const int slot = FindSlot((tsweapon_t)t);
		const int owned = (slot >= 0) ? 1 : 0;
		const int clip = (slot >= 0) ? m_arsenalClip[slot] : 0;
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

	// Finish a reload once its delay elapses: move rounds from the per-caliber reserve
	// into the clip (capped at clip size / available reserve).
	if (m_fInReload && m_flNextPrimaryAttack <= UTIL_WeaponTimeBase())
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
		m_fInReload = false;
	}

	if ((m_pPlayer->pev->button & IN_ATTACK2) && m_flNextSecondaryAttack <= UTIL_WeaponTimeBase())
	{
		SecondaryAttack();
	}
	else if ((m_pPlayer->pev->button & IN_ATTACK) && m_flNextPrimaryAttack <= UTIL_WeaponTimeBase())
	{
		PrimaryAttack();
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

	if (wi->iReloadAnim >= 0)
		SendWeaponAnim(wi->iReloadAnim);

	m_fInReload = true;
	m_flNextPrimaryAttack = GetNextAttackDelay(TS_RELOAD_TIME);
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
			m_arsenalMode[m_active] = mode;
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

// Cycle to the next weapon in the player's TS arsenal (the `tsnextweapon` command).
void TSCycleWeapon(CBasePlayer *pPlayer)
{
	CTSGun *pGun = GetPlayerTSGun(pPlayer);
	if (!pGun || pGun->m_arsenalCount <= 1)
		return;
	pGun->SelectSlot((pGun->m_active + 1) % pGun->m_arsenalCount);
}

// Infinite-clip powerup hook (PlayerAddPowerUp) — multiply the held weapon's reserve.
bool TSMultiplyClips(CBasePlayer *pPlayer)
{
	CTSGun *pGun = GetPlayerTSGun(pPlayer);
	return pGun ? pGun->MultiplyClips() : false;
}

// Cycle the held weapon's fire-mode (the `tsfiremode` command).
void TSSwitchFireMode(CBasePlayer *pPlayer)
{
	CTSGun *pGun = GetPlayerTSGun(pPlayer);
	if (pGun)
		pGun->SwitchFireMode();
}

// @ DropWeapon__6CTSGun — remove the active arsenal slot (for dropping it into the world),
// returning its type + loaded clip. Requires holding >= 2 weapons (no unarmed state — SLICE);
// the next slot is wielded.
bool CTSGun::RemoveActiveWeapon(int &type, int &clip)
{
	if (m_arsenalCount < 2)
		return false;

	type = m_arsenalType[m_active];
	clip = m_iClip;

	for (int i = m_active; i < m_arsenalCount - 1; i++)
	{
		m_arsenalType[i] = m_arsenalType[i + 1];
		m_arsenalClip[i] = m_arsenalClip[i + 1];
		m_arsenalMode[i] = m_arsenalMode[i + 1];
	}
	m_arsenalCount--;
	if (m_active >= m_arsenalCount)
		m_active = 0;

	m_weaponType = m_arsenalType[m_active];
	m_iClip = m_arsenalClip[m_active];
	m_fireMode = m_arsenalMode[m_active];
	m_burstCount = 0;
	if (m_pPlayer && m_pPlayer->m_pActiveItem == (CBasePlayerItem *)this)
		Deploy();
	return true;
}

// Drop the player's active TS weapon: pull it from the arsenal (out-params type/clip). The
// caller (the dropweapon command) spawns the CTSWorldGun. Returns false if it can't drop.
bool TSDropActiveWeapon(CBasePlayer *pPlayer, int &type, int &clip)
{
	CTSGun *pGun = GetPlayerTSGun(pPlayer);
	return pGun ? pGun->RemoveActiveWeapon(type, clip) : false;
}

// On death, scatter the player's whole arsenal into the world as CTSWorldGuns (the dead
// player's arsenal is discarded on respawn, so no removal bookkeeping is needed).
void TSDropWeaponsOnDeath(CBasePlayer *pPlayer)
{
	CTSGun *pGun = GetPlayerTSGun(pPlayer);
	if (!pGun)
		return;
	for (int i = 0; i < pGun->m_arsenalCount; i++)
	{
		int type = pGun->m_arsenalType[i];
		int clip = (i == pGun->m_active) ? pGun->m_iClip : pGun->m_arsenalClip[i];
		const s_weapon_info *wi = get_weapon_info((tsweapon_t)type);
		if (!wi->pszWorldModel) // skip melee/things without a droppable world model
			continue;
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
// add it to the arsenal, then restore the carried clip on that slot.
void TSPickUpWeapon(CBasePlayer *pPlayer, int type, int clip)
{
	GiveTSWeapon(pPlayer, (tsweapon_t)type);
	CTSGun *pGun = GetPlayerTSGun(pPlayer);
	if (!pGun)
		return;
	int slot = pGun->FindSlot((tsweapon_t)type);
	if (slot >= 0)
	{
		pGun->m_arsenalClip[slot] = clip;
		if (slot == pGun->m_active)
			pGun->m_iClip = clip;
	}
}

// Add per-caliber reserve to the player's TS gun (CTSGroundAmmo refill station).
bool TSAddAmmo(CBasePlayer *pPlayer, int caliber, int count)
{
	CTSGun *pGun = GetPlayerTSGun(pPlayer);
	return pGun ? pGun->AddAmmo(caliber, count) : false;
}
