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
	void PrimaryAttack() override;
	void Reload() override;
	void WeaponIdle() override;

	void MeleeAttack(const s_weapon_info *wi); // knife/katana fire-types

	// arsenal (the items-array). @ AddItem/SelectWeapon/TakeWeapon__6CTSGun.
	int FindSlot(tsweapon_t type);   // owned-slot index, or -1
	void AddWeapon(tsweapon_t type); // add to the arsenal (or resupply) + wield it
	void SelectSlot(int slot);       // wield an owned slot (saves/loads its clip)

	int m_weaponType = TSW_BERETTA; // ACTIVE tsweapon_t (= m_arsenalType[m_active])
	int m_arsenalType[TS_MAX_ARSENAL] = {};
	int m_arsenalClip[TS_MAX_ARSENAL] = {};    // saved loaded rounds per slot
	int m_arsenalReserve[TS_MAX_ARSENAL] = {}; // spare rounds per slot
	int m_arsenalCount = 0;
	int m_active = 0;
};

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
	m_arsenalReserve[0] = wi->iClip * 3;
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

// @ AddItem__6CTSGun — add a weapon to the arsenal (or resupply if owned), wield it.
void CTSGun::AddWeapon(tsweapon_t type)
{
	const s_weapon_info *wi = get_weapon_info(type);
	int slot = FindSlot(type);
	if (slot < 0)
	{
		if (m_arsenalCount >= TS_MAX_ARSENAL)
			return; // arsenal full
		slot = m_arsenalCount++;
		m_arsenalType[slot] = type;
		m_arsenalClip[slot] = wi->iClip;
		m_arsenalReserve[slot] = wi->iClip * 3;
	}
	else
	{
		m_arsenalReserve[slot] = wi->iClip * 3; // already owned -> resupply
	}
	SelectSlot(slot);
}

// @ SelectWeapon__6CTSGun — wield an owned slot (save the current clip, load the new).
void CTSGun::SelectSlot(int slot)
{
	if (slot < 0 || slot >= m_arsenalCount)
		return;

	m_arsenalClip[m_active] = m_iClip; // save the wielded weapon's clip
	m_active = slot;
	m_weaponType = m_arsenalType[slot];
	m_iClip = m_arsenalClip[slot];

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
// (m_pActiveItem/m_pActiveInfo) — here we dispatch on the single held m_weaponType;
// grenade/akimbo fall back to bullets (need the throw entity / dual-wield engine).
void CTSGun::PrimaryAttack()
{
	const s_weapon_info *wi = get_weapon_info((tsweapon_t)m_weaponType);

	// Melee fire-types do a hull trace, not bullets.
	if (wi->iFireType == TSFT_KNIFE || wi->iFireType == TSFT_KATANA)
	{
		MeleeAttack(wi);
		return;
	}

	if (m_iClip <= 0)
	{
		PlayEmptySound();
		m_flNextPrimaryAttack = GetNextAttackDelay(0.2);
		return;
	}

	// Semi-auto types fire one round per trigger pull (the pump shotgun + pistols +
	// sniper + SLICE akimbo/grenade); full-auto types (SMG/rifle/auto-shotgun) fire
	// while the trigger is held.
	const int ft = wi->iFireType;
	const bool fullAuto = (ft == TSFT_AUTO || ft == TSFT_AUTOSHOTGUN);
	if (!fullAuto && !(m_pPlayer->m_afButtonPressed & IN_ATTACK))
		return;

	// Pump shotguns fire a pellet spread; everything else a single bullet.
	const int cPellets = (ft == TSFT_PUMP) ? 6 : 1; // VERIFY: pellet count not yet mapped
	const float flSpread = wi->flSpread > 0 ? wi->flSpread : 0.01f;

	m_iClip--;
	m_pPlayer->pev->effects |= EF_MUZZLEFLASH;
	m_pPlayer->SetAnimation(PLAYER_ATTACK1);

	if (wi->pszFireSound)
		EMIT_SOUND(ENT(m_pPlayer->pev), CHAN_WEAPON, wi->pszFireSound, VOL_NORM, ATTN_NORM);

	Vector vecSrc = m_pPlayer->GetGunPosition();
	Vector vecAiming = gpGlobals->v_forward;
	m_pPlayer->FireBulletsPlayer(cPellets, vecSrc, vecAiming,
		Vector(flSpread, flSpread, flSpread), 8192, BULLET_PLAYER_9MM,
		0, (int)wi->flDamage, m_pPlayer->pev, m_pPlayer->random_seed);

	const float flRate = wi->flFireRate > 0 ? wi->flFireRate : 0.15f;
	m_flNextPrimaryAttack = GetNextAttackDelay(flRate);
	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 5.0f;
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
void CTSGun::Reload()
{
	const s_weapon_info *wi = get_weapon_info((tsweapon_t)m_weaponType);
	if (m_iClip >= wi->iClip || m_arsenalReserve[m_active] <= 0)
		return;

	int need = wi->iClip - m_iClip;
	int take = (need < m_arsenalReserve[m_active]) ? need : m_arsenalReserve[m_active];
	m_iClip += take;
	m_arsenalReserve[m_active] -= take;

	m_flNextPrimaryAttack = GetNextAttackDelay(1.5);
	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 2.0f;
	// SLICE: no reload animation/event.
}

void CTSGun::WeaponIdle()
{
	ResetEmptySound();
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
