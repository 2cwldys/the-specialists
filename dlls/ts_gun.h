/***
*  ts_gun.h — the CTSGun weapon class (the whole TS arsenal in one weapon_tsgun).
*
*  Extracted from ts_gun.cpp so the class is visible to the CLIENT prediction harness
*  (cl_dll/hl/hl_weapons.cpp instantiates a client g_TSGun and runs its ItemPostFrame each
*  prediction frame, like the HL SDK weapons) — step 3b of making CTSGun client-predicted
*  so predicted viewmodel anims stick. See reference/EVENTS.md (cont.69s).
***/
#ifndef TS_GUN_H
#define TS_GUN_H

#include "weapons.h"     // CBasePlayerWeapon, ItemInfo, weapon_data_t
#include "ts_weapons.h"  // tsweapon_t, s_weapon_info, TSW_*, TSFM_*, TS_NUM_CALIBERS

#define TSGUN_WEAPON_ID 16 // CTSGun::GetItemInfo @0x85dd0 sets iId = 0x10
#define TSGUN_WEIGHT 15

// One arsenal slot, indexed by tsweapon_t (the original's s_weapon_status entry):
//   owned      gun+type*0x38+0x114  — in the arsenal
//   clip       +0x118               — total loaded rounds (akimbo: leftClip+rightClip)
//   leftClip   +0x130 / rightClip +0x134 — the akimbo's two barrel clips
//   mode       +0x116               — saved fire-mode (TSFM_*)
//   attach     +0x124               — attachment mask (silencer/laser/...; future buy wiring)
//   barrelMask +0x128               — akimbo active barrels (bit1=left/2, bit2=right/4; useitems)
struct TSItem
{
	bool owned = false;
	int  clip = 0;
	int  leftClip = 0;
	int  rightClip = 0;
	int  mode = TSFM_SEMI;
	int  attach = 0;
	int  barrelMask = 0;
};

class CTSGun : public CBasePlayerWeapon
{
public:
	void Spawn() override;
	void Precache() override;
	bool GetItemInfo(ItemInfo *p) override;
	int iItemSlot() override { return 1; } // @ iItemSlot__6CTSGun returns slot 1
	// TS weapons keep their ammo in our own per-caliber reserve (m_ammoReserve), NOT the stock
	// m_rgAmmo that CBasePlayerWeapon::CanDeploy() checks. So the stock CanDeploy would refuse to
	// deploy any empty-clip TS weapon — notably kung-fu (always 0 clip), which then kept the
	// PREVIOUS weapon's viewmodel. The tsgun is always deployable.
	bool CanDeploy() override { return true; }
	bool Deploy() override;
	bool UpdateClientData(CBasePlayer *pPlayer) override; // sends the TS WeaponInfo HUD message
	void SendWeaponStatus(CBasePlayer *pPlayer);          // sends the WStatus weapon-bar messages
	void ItemPostFrame() override;
	void PrimaryAttack() override;
	void Reload() override;
	void WeaponIdle() override;

	// Client prediction (step 3b/3c) — SCAFFOLDED but DISABLED. Returning true here (with the
	// client registration in hl_weapons.cpp) makes the server pack CTSGun into weapon_data and the
	// client run CTSGun::ItemPostFrame each prediction frame. That ran, but the predicted viewmodel
	// anim still didn't stick + fire effects doubled — prediction-integration bugs to debug (3c/3d).
	// Flip to the CLIENT_WEAPONS form below (and uncomment the hl_weapons.cpp registration) once the
	// time-base fix lands. DISABLED for now: enabling it makes the engine decrement CTSGun's
	// m_flNextPrimaryAttack as a relative countdown, but CTSGun uses ABSOLUTE gpGlobals->time (the
	// cont.69q workaround for CLIENT_WEAPONS being defined globally) → the timers corrupt and the
	// predicted attack gate misfires, so SendWeaponAnim never drives the anim. THE FIX (cont.69s 3d):
	// (1) scope CLIENT_WEAPONS to the CLIENT build only (CMakeLists) so UTIL_WeaponTimeBase() is
	// gpGlobals->time server-side / 0 client-side; (2) revert CTSGun's timers from gpGlobals->time to
	// UTIL_WeaponTimeBase(). Then return true here.
	bool UseDecrement() override { return false; }
	void GetWeaponData(weapon_data_t &data) override; // server: pack the TS-specific active state (3c)
	void SetWeaponData(const weapon_data_t &data) override; // client: unpack it before ItemPostFrame (3c)

	void AlternateAttack();                      // @AlternateAttack — +alt2: coldcock if a round is loaded
	void ColdCock();                             // @ColdCock — a quick pistol-whip melee bash
	void Fire();                                 // dispatch one shot by fire-type
	void StandardFire(const s_weapon_info *wi);  // bullet/pellet fire (StandardFire/ShotgunFire)
	void MeleeAttack(const s_weapon_info *wi);   // knife/katana fire-types
	void CloseCombatAttack(bool fSecondary);     // @CCombatAttack — kung-fu contextual punch/kick
	void CloseCombatBlock();                     // @CCombatBlock — kung-fu reload: "hooah" block (stamina)
	void GrenadeThrow(const s_weapon_info *wi);  // grenade fire-type
	void AkimboFire(const s_weapon_info *wi);    // dual-wield fire-types (4/5)
	void SwitchFireMode();                       // cycle the held weapon's fire-mode
	void SwitchItems();                          // @SwitchItems (useitems) — akimbo dual-wield swap

	// arsenal (the items-by-tsweapon_t array). @ TakeWeapon/SelectWeapon/DropWeapon__6CTSGun.
	bool IsOwned(int type) const { return type >= 0 && type < TSW_COUNT && m_items[type].owned; }
	int  OwnedCount() const;          // number of owned weapon types
	void AddWeapon(tsweapon_t type);  // @TakeWeapon — add/combine into the arsenal + wield it
	void SelectWeaponType(int type);  // @SelectWeapon — wield an owned type (holster->deploy)
	void GetBetterWeapon(int exclude);// @GetBetterWeapon — auto-select next owned by priority
	void DropType(int type, bool fThrow); // @DropWeapon__6CTSGun — drop an owned item from the arsenal
	bool MultiplyClips();             // infinite-clip powerup: multiply the held reserve
	bool AddAmmo(int caliber, int count); // @AddAmmo__6CTSGun — add reserve rounds (capped)
	void SyncActiveToItem();          // park the live active clip/mode back into its item
	void LoadActiveFromItem();        // load the active item's clip/mode into the live state

	int m_weaponType = TSW_MELEE; // ACTIVE tsweapon_t (gun+0xc8). Default = kung-fu: a bare
	                              // weapon_tsgun (GiveNamedItem) is just fists; the loadout
	                              // (BuyArmament) adds the real guns. Placed/given weapons set it.
	TSItem m_items[TSW_COUNT] = {};            // the arsenal, indexed by tsweapon_t
	int m_ammoReserve[TS_NUM_CALIBERS] = {};   // spare rounds PER CALIBER (gun+0xd4 in the .so)
	int m_lastType = TSW_MELEE; // previously-wielded type (for the `lastinv` toggle, player+0xa1c)
	int m_akimboSide = 0; // which barrel fired last (alternates muzzle L/R for akimbo)
	int m_fireMode = TSFM_SEMI; // ACTIVE fire-mode (= m_arsenalMode[m_active])
	int m_burstCount = 0;       // rounds left in the current burst (m_pActiveItem+8)
	int m_comboStep = 0;        // kung-fu ground-punch combo cycle (this+0xa00 in the .so)
	float m_lastCCTime = 0.0f;  // last close-combat attack time (combo only continues if rapid/held)
	float m_blockBar = 100.0f;  // kung-fu block stamina (UpdateMeleeBar): each block costs, regens
	float m_blockBarTime = 0.0f;// last time the block bar was updated (for regen)
	unsigned short m_usFire = 0; // events/fire.sc handle (predicted fire: sound/flash/anim)
	unsigned short m_usDraw = 0; // events/draw.sc handle (deploy: seed the client param cache)

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

#endif // TS_GUN_H
