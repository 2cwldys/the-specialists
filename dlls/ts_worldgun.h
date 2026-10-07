/***
*  ts_worldgun.h — CTSWorldGun + CTSAmmoPack (original TS_worldgun.cpp).
*  CTSWorldGun : CTSPhysicObject  — a player-dropped weapon (bounce physics + the carried
*  s_weapon_status), picked up by USE (or on touch for the throwable type 0x1d).
*  CTSAmmoPack : CBaseEntity      — a dropped consumable ammo+armor pack.
*  Both are spawned IN CODE by their static factories (CreateWeapon / CreateAmmoPack), not
*  placed in maps; the runtime classnames are "WorldGun" / "AmmoPack".
***/
#ifndef TS_WORLDGUN_H
#define TS_WORLDGUN_H

#include "ts_physicobject.h" // CTSPhysicObject (slow-motion physics base)
#include "ts_gun.h"          // TSItem (= the original's s_weapon_status arsenal slot)

// CTSWorldGun (@0xa07ec..) — dropped weapon. Derives the slow-motion physics base, carries the
// full weapon status (the dropped item), and hands it back on pickup.
class CTSWorldGun : public CTSPhysicObject
{
public:
	void Spawn() override;            // @0xa0954
	void Precache() override {}       // @0xa0b08 — empty (world models precached at weapon-table init)
	void EXPORT BounceTouch(CBaseEntity *pOther);                                    // @0xa0b0c
	void EXPORT Fall();                                                              // @0xa0d84
	void EXPORT WaitThink();                                                         // @0xa104c
	void OnGround();                                                                 // @0xa10a4
	void Use(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value) override; // @0xa1158
	void GetWeapon(CBaseEntity *pOther);                                             // @0xa1198
	void EXPORT WaitTouch(CBaseEntity *pOther);                                      // @0xa1250
	virtual bool ShouldCollide(CBaseEntity *pOther);                                // @0xa1290 (TS virtual)
	int  ObjectCaps() override { return FCAP_CONTINUOUS_USE; }                        // @0xa3404 = mov $0x10 = FCAP_CONTINUOUS_USE (cont.91: was IMPULSE 0x8 -> near-edge pickup flaky)
	virtual int CanUsedTroughWalls() { return 0; }                                  // @0xa340c -> 0 (TS virtual)

	// @0x907ec CreateWeapon — spawn a dropped weapon carrying `status` (the dropping gun's item slot)
	// from `owner` (NULL = no dropper). Spawns it but sets NO origin/velocity — the caller (the player's
	// ThrowWeapon) positions + tosses it. Sets owner + appearance + the flag's glow. NULL on failure.
	static CTSWorldGun *CreateWeapon(CBasePlayer *owner, int type, const TSItem &status);

	int    m_weaponType = TSW_BERETTA; // +0x88 — the dropped weapon type (== 0x1d : throwable/auto-pickup)
	TSItem m_status;                   // +0x8c — the FULL carried weapon state (s_weapon_status, 0x38 bytes)
	float  m_flExpire = 0;             // +0xc4 — lifetime deadline (UTIL_Remove past it)
	bool   m_bDropSnd = false;         // +0xc8 — debounces the bounce drop sound
	float  m_flUseCooldown = 0;        // +0xcc — earliest USE-pickup time (stops the dropper re-grabbing)
	edict_t *m_pDropper = nullptr;     // +0xd0 — the player who dropped it with the drop key or was disarmed
	                                   // (DropWeapon @0x84ed9/0x84fad/0x85265); AddToFullPack draws the gun
	                                   // with a glow shell for that player only. NULL = glows for nobody.
	bool   m_bTaken = false;           // +0xd4 — already picked up
};

// CTSAmmoPack (@0xa1334..) — a dropped consumable pack: per-caliber rounds + an armor value.
// Base is CBaseEntity (NOT CTSPhysicObject — its ammo[16] reuses the 0x64 byte range, no slow-mo).
class CTSAmmoPack : public CBaseEntity
{
public:
	void Spawn() override;            // @0xa13f4
	void Precache() override {}       // @0xa1540 — empty (ammopack.mdl precached at init)
	void EXPORT Fall();                                  // @0xa1544
	void EXPORT WaitThink();                             // @0xa15c0
	void OnGround();                                     // @0xa1610
	void EXPORT WaitTouch(CBaseEntity *pOther);          // @0xa16c0

	// @0xa1334 CreateAmmoPack — spawn a pack carrying `counts[16]` rounds + `armor` armour from
	// `pevOwner`, tossed from `origin` along `velocity`. Returns NULL if there was nothing to drop.
	static CTSAmmoPack *CreateAmmoPack(entvars_t *pevOwner, const int *counts, int armor,
		const Vector &origin, const Vector &velocity);

	int   m_ammo[TS_NUM_CALIBERS] = {}; // +0x64 — per-caliber rounds carried (consumed on pickup)
	float m_flExpire = 0;               // +0xa4 — lifetime deadline
	// carried ARMOR lives in pev->armorvalue; the pack removes itself once both ammo and armor are gone.
};

#endif // TS_WORLDGUN_H
