/***
*  ts_gun.cpp — RECONSTRUCTED CTSGun (vertical slice: beretta deploy + fire).
*  Functional/source-faithful goal (see CLAUDE.md). recovered/server/CTSGun.c.
*
*  CTSGun is the original's single unified weapon (entity `weapon_tsgun`, id 16)
*  that morphs by tsweapon_t. The FAITHFUL engine is large and tightly coupled:
*  a dual-wield "active items" array (m_pActiveItem @0xcc / m_pActiveInfo @0xd0 /
*  items[] @0x45), client-side weapon-param networking, CTSAnimation, and the
*  988-byte s_weapon_info table. Deploy/Fire dispatch by the weapon's fire-type
*  byte (s_weapon_info+0x348; beretta=8 "standard").
*
*  THIS SLICE is intentionally a *functional-minimal* stand-in for that engine:
*  it deploys and semi-auto-fires the beretta using stock CBasePlayerWeapon
*  mechanics so we can prove a TS weapon is holdable + fireable in-engine. The
*  faithful items-array / akimbo / fire-mode / networking reconstruction replaces
*  the // SLICE-marked bodies below, incrementally and verified.
***/
#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "player.h"
#include "weapons.h"
#include "ts_weapons.h"

#define TSGUN_WEAPON_ID 16 // CTSGun::GetItemInfo @0x85dd0 sets iId = 0x10
#define TSGUN_WEIGHT 15

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

	tsweapon_t m_weaponType = TSW_BERETTA; // SLICE: fixed to the beretta
};

LINK_ENTITY_TO_CLASS(weapon_tsgun, CTSGun);

// @ Spawn__6CTSGun — classname weapon_tsgun, id 16, fall-init as a pickup.
void CTSGun::Spawn()
{
	pev->classname = MAKE_STRING("weapon_tsgun");
	Precache();
	m_iId = TSGUN_WEAPON_ID;

	const s_weapon_info *wi = get_weapon_info(m_weaponType);
	SET_MODEL(ENT(pev), wi->pszWorldModel);

	m_iDefaultAmmo = wi->iClip; // SLICE: stock ammo model; TS uses its own system
	m_iClip = wi->iClip;        // SLICE: deploy loaded (TS manages its own ammo)
	FallInit();
}

// @ Precache__6CTSGun — the original precaches 11 event scripts; here we also
// precache the slice weapon's models/sounds (the faithful version precaches the
// whole roster via the weapon table).
void CTSGun::Precache()
{
	const s_weapon_info *wi = get_weapon_info(m_weaponType);
	PRECACHE_MODEL((char *)wi->pszViewModel);
	PRECACHE_MODEL((char *)wi->pszPlayerModel);
	PRECACHE_MODEL((char *)wi->pszWorldModel);
	PRECACHE_SOUND((char *)wi->pszFireSound);

	// faithful event scripts (CTSGun::Precache @0x80660)
	PRECACHE_EVENT(1, "events/fire.sc");
	PRECACHE_EVENT(1, "events/reload.sc");
	PRECACHE_EVENT(1, "events/draw.sc");
}

// @ GetItemInfo__6CTSGun — recovered: slot 1, pos 2, id 16, weight 1. (The
// original reports no clip/ammo and runs its own ammo system; the slice gives a
// stock clip so the demo fires.)
bool CTSGun::GetItemInfo(ItemInfo *p)
{
	const s_weapon_info *wi = get_weapon_info(m_weaponType);
	p->pszName = STRING(pev->classname);
	p->pszAmmo1 = "9mm"; // SLICE
	p->iMaxAmmo1 = 200;  // SLICE
	p->pszAmmo2 = NULL;
	p->iMaxAmmo2 = -1;
	p->iMaxClip = wi->iClip; // beretta 15
	p->iSlot = 1;
	p->iPosition = 2;
	p->iFlags = 0;
	p->iId = m_iId = TSGUN_WEAPON_ID;
	p->iWeight = TSGUN_WEIGHT;
	return true;
}

bool CTSGun::Deploy()
{
	const s_weapon_info *wi = get_weapon_info(m_weaponType);
	// SLICE: faithful Deploy uses the items array + GetBodyByItems + a weapon-param
	// message; here we just show the view/player model via DefaultDeploy.
	return DefaultDeploy((char *)wi->pszViewModel, (char *)wi->pszPlayerModel, 0, "onehanded");
}

// @ PrimaryAttack__6CTSGun -> Fire -> StandardFire (faithful path dispatches by
// fire-type and runs ShootBulletsPlayer + VKickback). SLICE: stock semi-auto.
void CTSGun::PrimaryAttack()
{
	if (m_iClip <= 0)
	{
		PlayEmptySound();
		m_flNextPrimaryAttack = GetNextAttackDelay(0.2);
		return;
	}

	const s_weapon_info *wi = get_weapon_info(m_weaponType);
	m_iClip--;

	m_pPlayer->pev->effects |= EF_MUZZLEFLASH;
	m_pPlayer->SetAnimation(PLAYER_ATTACK1);

	EMIT_SOUND(ENT(m_pPlayer->pev), CHAN_WEAPON, wi->pszFireSound, VOL_NORM, ATTN_NORM);

	Vector vecSrc = m_pPlayer->GetGunPosition();
	Vector vecAiming = gpGlobals->v_forward;
	m_pPlayer->FireBulletsPlayer(1, vecSrc, vecAiming,
		Vector(wi->flSpread, wi->flSpread, wi->flSpread), 8192, BULLET_PLAYER_9MM,
		0, (int)wi->flDamage, m_pPlayer->pev, m_pPlayer->random_seed);

	m_flNextPrimaryAttack = GetNextAttackDelay(0.15);
	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 5.0f;
}

void CTSGun::Reload()
{
	const s_weapon_info *wi = get_weapon_info(m_weaponType);
	DefaultReload(wi->iClip, 0, 1.5f);
}

void CTSGun::WeaponIdle()
{
	ResetEmptySound();
}
