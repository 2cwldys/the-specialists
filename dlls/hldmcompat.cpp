/***
*  hldmcompat.cpp — RECONSTRUCTED Half-Life-DM-compatibility weapon spawners (TS_ original
*  hldmcompat.cpp). These remap STOCK HL deathmatch weapon entity names onto TS weapons so a
*  stock valve DM map is playable in TS. Each derives CTSGroundWeapon and either picks a RANDOM
*  weapon of its category (RandomX) or a FIXED one, then runs the base pickup physics.
*
*  Entity-name -> class map (the swap contract, RE'd from the .so factory exports):
*    weapon_shotgun -> CTSShotgunWeapon       ammo_buckshot -> CTSShotgunAmmo
*    weapon_gauss + weapon_egon -> CTSRifleWeapon
*    weapon_rpg -> CTSSMGWeapon               weapon_9MMAR -> CTSBigPistolWeapon
*    ammo_ARgrenades + weapon_satchel -> CTSKnifeWeapon
*    weapon_crossbow -> CTSSniperWeapon       weapon_hornetgun -> CTSMGWeapon
*    weapon_snark -> CTSKatanaWeapon          weapon_handgrenade -> CTSGrenadeWeapon
*  CTSPistolWeapon is COMPILED but UNLINKED in the original (no LINK_ENTITY_TO_CLASS) — same here.
***/
#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "player.h"
#include "ts_weapons.h"
#include "ts_gamerules.h" // TS_PWUP_* (CTSSuperJump)
#include "ts_powerup.h"   // CTSPowerUp base (CTSSuperJump)
#include "ts_groundweapon.h"
#include "ts_groundammo.h"

// On respawn the base GetWeapon hides the pickup (EF_NODRAW) before re-arming; a random spawner
// re-rolls its weapon + model each time so the respawned pickup is a fresh random gun.
static void RerollModel(CTSGroundWeapon *gw, int id)
{
	gw->tsweaponid = id;
	const s_weapon_info *wi = get_weapon_info((tsweapon_t)id);
	const char *world = (wi && wi->pszWorldModel) ? wi->pszWorldModel : "models/w_weaponbox.mdl";
	PRECACHE_MODEL((char *)world);
	SET_MODEL(ENT(gw->pev), world);
}

// ---- random-category weapon spawners (Spawn + GetWeapon + RandomX) --------------

class CTSPistolWeapon : public CTSGroundWeapon
{
public:
	void Spawn() override { tsweaponid = RandomPistol(); CTSGroundWeapon::Spawn(); }
	void GetWeapon(CBaseEntity *pOther) override
	{ CTSGroundWeapon::GetWeapon(pOther); if (pev->effects & EF_NODRAW) RerollModel(this, RandomPistol()); }
	int RandomPistol()
	{
		static const int s[7] = { TSW_GLOCK18, TSW_BERETTA_AKIMBO, TSW_MK23, TSW_FNH, TSW_GOLD, TSW_GLOCK22, TSW_RUGER };
		return s[RANDOM_LONG(0, 6)];
	}
};
// (UNLINKED in the original — no LINK_ENTITY_TO_CLASS for weapon_9mmhandgun/weapon_357.
//  CTSPistolWeapon/CTSPistolAmmo are still COMPILED INTO the original binary though; a static
//  instance of each (below) keeps our linker from DCE'ing them so they're present as in the original.)

class CTSBigPistolWeapon : public CTSGroundWeapon
{
public:
	void Spawn() override { tsweaponid = RandomBigPistol(); CTSGroundWeapon::Spawn(); }
	void GetWeapon(CBaseEntity *pOther) override
	{ CTSGroundWeapon::GetWeapon(pOther); if (pev->effects & EF_NODRAW) RerollModel(this, RandomBigPistol()); }
	int RandomBigPistol()
	{
		static const int s[3] = { TSW_DESERT, TSW_BULL, TSW_CONTENDER };
		return s[RANDOM_LONG(0, 2)];
	}
};
LINK_ENTITY_TO_CLASS(weapon_9mmAR, CTSBigPistolWeapon);

class CTSShotgunWeapon : public CTSGroundWeapon
{
public:
	void Spawn() override { tsweaponid = RandomShotgun(); CTSGroundWeapon::Spawn(); }
	void GetWeapon(CBaseEntity *pOther) override
	{ CTSGroundWeapon::GetWeapon(pOther); if (pev->effects & EF_NODRAW) RerollModel(this, RandomShotgun()); }
	int RandomShotgun()
	{
		static const int s[5] = { TSW_BENELLIM3, TSW_MOSSBERG, TSW_SAWEDOFF, TSW_SPAS12, TSW_USAS12 };
		return s[RANDOM_LONG(0, 4)];
	}
};
LINK_ENTITY_TO_CLASS(weapon_shotgun, CTSShotgunWeapon);

class CTSRifleWeapon : public CTSGroundWeapon
{
public:
	void Spawn() override { tsweaponid = RandomRifle(); CTSGroundWeapon::Spawn(); }
	void GetWeapon(CBaseEntity *pOther) override
	{ CTSGroundWeapon::GetWeapon(pOther); if (pev->effects & EF_NODRAW) RerollModel(this, RandomRifle()); }
	int RandomRifle()
	{
		static const int s[4] = { TSW_AK47, TSW_M4, TSW_M16, TSW_AUG };
		return s[RANDOM_LONG(0, 3)];
	}
};
LINK_ENTITY_TO_CLASS(weapon_gauss, CTSRifleWeapon);
LINK_ENTITY_TO_CLASS(weapon_egon, CTSRifleWeapon);

class CTSSMGWeapon : public CTSGroundWeapon
{
public:
	void Spawn() override { tsweaponid = RandomSMG(); CTSGroundWeapon::Spawn(); }
	void GetWeapon(CBaseEntity *pOther) override
	{ CTSGroundWeapon::GetWeapon(pOther); if (pev->effects & EF_NODRAW) RerollModel(this, RandomSMG()); }
	int RandomSMG()
	{
		static const int s[6] = { TSW_MAC10, TSW_MP5K, TSW_MP5SD, TSW_PDW, TSW_SKORPION, TSW_UZI };
		return s[RANDOM_LONG(0, 5)];
	}
};
LINK_ENTITY_TO_CLASS(weapon_rpg, CTSSMGWeapon);

class CTSKnifeWeapon : public CTSGroundWeapon
{
public:
	void Spawn() override { tsweaponid = RandomKnife(); CTSGroundWeapon::Spawn(); }
	void GetWeapon(CBaseEntity *pOther) override
	{ CTSGroundWeapon::GetWeapon(pOther); if (pev->effects & EF_NODRAW) RerollModel(this, RandomKnife()); }
	int RandomKnife()
	{
		static const int s[2] = { TSW_KNIFE, TSW_SEALKNIFE };
		return s[RANDOM_LONG(0, 1)];
	}
};
LINK_ENTITY_TO_CLASS(ammo_ARgrenades, CTSKnifeWeapon);
LINK_ENTITY_TO_CLASS(weapon_satchel, CTSKnifeWeapon);

// ---- fixed-weapon spawners (Spawn only; inherit the base GetWeapon) -------------

class CTSSniperWeapon : public CTSGroundWeapon
{
public:
	void Spawn() override { tsweaponid = TSW_BARRETT; CTSGroundWeapon::Spawn(); }
};
LINK_ENTITY_TO_CLASS(weapon_crossbow, CTSSniperWeapon);

class CTSMGWeapon : public CTSGroundWeapon
{
public:
	void Spawn() override { tsweaponid = TSW_M60; extraclip = 100; CTSGroundWeapon::Spawn(); }
};
LINK_ENTITY_TO_CLASS(weapon_hornetgun, CTSMGWeapon);

class CTSKatanaWeapon : public CTSGroundWeapon
{
public:
	void Spawn() override { tsweaponid = TSW_KATANA; CTSGroundWeapon::Spawn(); }
};
LINK_ENTITY_TO_CLASS(weapon_snark, CTSKatanaWeapon);

class CTSGrenadeWeapon : public CTSGroundWeapon
{
public:
	void Spawn() override { tsweaponid = TSW_M61; CTSGroundWeapon::Spawn(); }
};
LINK_ENTITY_TO_CLASS(weapon_handgrenade, CTSGrenadeWeapon);

// ---- ammo spawners (derive CTSGroundAmmo; Spawn seeds the per-caliber array) ----
// Each seeds a starting amount in its category's caliber(s) then runs the base ammo-station
// spawn. orig: m_ammo[cal] = get_ammo_info(cal)->default/2; we use TSRoundsPerMag (a magazine) as
// the proxy — VERIFY the exact half-default (the +0xc ammo-info field isn't in our table yet).

class CTSPistolAmmo : public CTSGroundAmmo
{
public:
	void Spawn() override
	{
		CTSGroundAmmo::Spawn();
		int c[5] = { 1, 2, 7, 9, 10 };
		for (int i = 0; i < 5; i++) m_ammo[c[i]] = TSRoundsPerMag(c[i]);
	}
};
// (UNLINKED in the original — ammo_9mmclip/ammo_357 are string-table only)

class CTSBigPistolAmmo : public CTSGroundAmmo
{
public:
	void Spawn() override
	{
		CTSGroundAmmo::Spawn();
		m_ammo[5] = TSRoundsPerMag(5);
		m_ammo[11] = TSRoundsPerMag(11);
	}
};
LINK_ENTITY_TO_CLASS(ammo_9mmAR, CTSBigPistolAmmo);

class CTSRifleAmmo : public CTSGroundAmmo
{
public:
	void Spawn() override
	{
		CTSGroundAmmo::Spawn();
		m_ammo[4] = TSRoundsPerMag(4);
		m_ammo[6] = TSRoundsPerMag(6);
	}
};
LINK_ENTITY_TO_CLASS(ammo_gaussclip, CTSRifleAmmo);

class CTSSMGAmmo : public CTSGroundAmmo
{
public:
	void Spawn() override
	{
		CTSGroundAmmo::Spawn();
		int c[4] = { 1, 2, 7, 12 };
		for (int i = 0; i < 4; i++) m_ammo[c[i]] = TSRoundsPerMag(c[i]);
	}
};
LINK_ENTITY_TO_CLASS(ammo_rpgclip, CTSSMGAmmo);

class CTSShotgunAmmo : public CTSGroundAmmo
{
public:
	void Spawn() override
	{
		CTSGroundAmmo::Spawn();
		m_ammo[3] = TSRoundsPerMag(3);
	}
};
LINK_ENTITY_TO_CLASS(ammo_buckshot, CTSShotgunAmmo);

class CTSSniperAmmo : public CTSGroundAmmo
{
public:
	void Spawn() override
	{
		CTSGroundAmmo::Spawn();
		m_ammo[8] = TSRoundsPerMag(8);
		m_ammo[13] = TSRoundsPerMag(13);
	}
};
LINK_ENTITY_TO_CLASS(ammo_crossbow, CTSSniperAmmo);

//============================================================================
// CTSSuperJump — a map-placed powerup pre-set to the low-gravity / super-jump type.
// The .so's CTSSuperJump::Spawn (@0xc9f44) sets poweruptype = 0x100 (TS_PWUP_LOWGRAV) then
// chains CTSPowerUp::Spawn, so it always gives the super-jump (low-gravity) effect.
//============================================================================
class CTSSuperJump : public CTSPowerUp
{
public:
	void Spawn() override
	{
		poweruptype = TS_PWUP_LOWGRAV; // force the super-jump (low-gravity) powerup
		CTSPowerUp::Spawn();
		TSGameRules->m_iRandomPowerups = -1; // @0xb9f71: a super jump does not turn the dynamic powerups off
	}
};

// @0xb9484 — the original registers CTSSuperJump under the stock HL entity name `item_longjump`
// (TS remaps the longjump item to its super-jump powerup, same pattern as the weapon remaps above).
// Our earlier `ts_superjump` name was invented drift — a map-placed `item_longjump` powerup spawned
// nothing in our build, and it broke drop-in with the original client/maps.
LINK_ENTITY_TO_CLASS(item_longjump, CTSSuperJump);

// Keep the two UNLINKED pistol classes in the binary (compiled-but-unlinked in the original;
// weapon_9mmhandgun/357 + ammo_9mmclip/357 are string-table-only there). This exported anchor
// references both so --gc-sections can't drop their vtables/methods. Never called.
CBaseEntity *hldmcompat_keep_unlinked()
{
	static CTSPistolWeapon w;
	static CTSPistolAmmo a;
	return RANDOM_LONG(0, 1) ? (CBaseEntity *)&w : (CBaseEntity *)&a;
}
