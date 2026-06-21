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

	int m_weaponType = TSW_BERETTA; // tsweapon_t; map-placed weapon_tsgun defaults to beretta
};

LINK_ENTITY_TO_CLASS(weapon_tsgun, CTSGun);

// @ Spawn__6CTSGun — classname weapon_tsgun, id 16, fall-init as a pickup.
void CTSGun::Spawn()
{
	pev->classname = MAKE_STRING("weapon_tsgun");
	Precache();
	m_iId = TSGUN_WEAPON_ID;

	const s_weapon_info *wi = get_weapon_info((tsweapon_t)m_weaponType);
	const char *world = wi->pszWorldModel ? wi->pszWorldModel : "models/w_weaponbox.mdl";
	SET_MODEL(ENT(pev), world);

	m_iDefaultAmmo = wi->iClip; // SLICE: stock ammo; TS runs its own ammo system
	m_iClip = wi->iClip;        // SLICE: deploy loaded
	FallInit();
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

void CTSGun::Reload()
{
	const s_weapon_info *wi = get_weapon_info((tsweapon_t)m_weaponType);
	DefaultReload(wi->iClip, 0, 1.5f);
}

void CTSGun::WeaponIdle()
{
	ResetEmptySound();
}

// Create a CTSGun of `type` and hand it to the player (mirrors GiveNamedItem but
// sets m_weaponType before Spawn so the weapon configures to the right type).
CBaseEntity *GiveTSWeapon(CBasePlayer *pPlayer, tsweapon_t type)
{
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
