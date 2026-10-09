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
	int  attach = 0;       // item+0x10 — EQUIPPED attachment mask (silencer 1 / laser 2 / flash 4 / scope 8)
	int  active = 0;       // item+0x14 — ACTIVE attachments (laser 2 / flash 4) toggled by `useitems` (SwitchItems)
	int  barrelMask = 0;
	bool holdPosition = false; // item+0x2c — alternate viewmodel stance (right-click toggle, see CTSGun::SecondaryAttack)
	// item+0x24 — the per-item ALT-MODE byte: Primary/SecondaryAttack write 0/1 before calling the shared
	// fire function (grenade toss mode, knife slash-vs-throw). The pump-shotgun's fire/rack use of the SAME
	// original byte lives in m_fInSpecialReload instead (predicted — see the ShotgunFire note below).
	int  altMode = 0;
	// item+0x8 for the blades — BLOODY-BLADE state, set on a player hit: drives the gun's pev->body = 1
	// (bloody w_/p_ submodel) and rides the knife.sc/katana.sc events as iparam2 (bloody viewmodel).
	int  bloody = 0;
	// item pumpToggle byte — the pump fire/rack alternation AND the akimbo barrel side (one byte in
	// the original). PARKED per item: HolsterWeapon clears reloading+burstOrPhase but NOT this
	// (@0x85698), so the rack/barrel step survives a weapon switch. Live mirror = m_fInSpecialReload.
	int  pumpToggle = 0;
	// item+0x30 — the recorded OWNER (an EHANDLE): CBasePlayer::PickUpWeapon writes the picker
	// into it (and into the akimbo pair's slot) ONLY on the bRecordOwner path — the BUY path
	// (@0x846ca is the sole true caller). Consumer VERIFY (likely the sell-zone's yours-check).
	EHANDLE hOwner;
};

class CTSGun : public CBasePlayerWeapon
{
public:
	void Spawn() override;
	void Precache() override;
	bool GetItemInfo(ItemInfo *p) override;
	int iItemSlot() override { return 1; } // @ iItemSlot__6CTSGun returns slot 1
	// (CanDeploy override deleted, fold-plan Y3: the original CTSGun does NOT override it —
	// VTABLES.md slot 0x124 = CBasePlayerWeapon::CanDeploy. With GetItemInfo's original values
	// (pszAmmo1 == NULL, @0x75dd0) the stock CanDeploy returns true unconditionally.)
	bool Deploy() override;
	bool UpdateClientData(CBasePlayer *pPlayer) override; // sends the TS WeaponInfo HUD message
	// NB: the per-frame driver is the BASE CBasePlayerWeapon::ItemPostFrame (weapons_shared.cpp) —
	// the original TS rewrote the base method for its single arsenal weapon (@0x903d4, m_iId==0x10
	// gate); CTSGun does NOT override it.
	bool ShouldWeaponIdle() override { return true; } // @0x87274 — always allow idle (the original returns 1)
	void PrimaryAttack() override;
	void SecondaryAttack() override;             // @SecondaryAttack__6CTSGun @0x717a8 — the right-click dispatcher
	void Reload() override;
	int  ReloadWeapon(int iClipSize, int iAnim, float flDuration); // @ ReloadWeapon__6CTSGuniif @0x74a14 — arm a whole-clip reload
	void StopReload();    // @ StopReload__6CTSGun @0x71288 — abort an in-progress reload (ColdCock/MultiplyClips)
	void EmptySound();    // @ EmptySound__6CTSGun @0x71be4 — dry-fire click via events/empty.sc + 0.1s re-arm
	void ShotgunReload(); // @ ShotgunReload__6CTSGun @0x74aac — pump shell-by-shell 3-phase machine
	void WeaponIdle() override;
	// (SendWeaponAnim(int) shadow deleted, fold-plan Y4: the original CTSGun uses the base
	// CBasePlayerWeapon::SendWeaponAnim(iAnim, body) — slot 0x174 @0x900d0 — with callers
	// passing pev->body (Deploy stores GetBodyByItems there) or an explicit 0 in HolsterWeapon.)
	int  AddItem(int bits); // @ AddItem__6CTSGuni @0x80794 — attach a gun item (silencer/laser/.. bits 1/2/4/8) to the active weapon
#ifndef CLIENT_DLL
	void Zoom(int fov);     // @ Zoom__6CTSGuni @0x71710 — scope: set pev->fov (+ hide viewmodel for high-mag scopes)
	void Unzoom();          // @ Unzoom__6CTSGun @0x71748 — scope: clear pev->fov + restore the viewmodel
#endif

	// Client prediction (step 3b/3c) — SCAFFOLDED but DISABLED. Returning true here (with the
	// client registration in hl_weapons.cpp) makes the server pack CTSGun into weapon_data and the
	// client run CTSGun::ItemPostFrame each prediction frame. That ran, but the predicted viewmodel
	// anim still didn't stick + fire effects doubled — prediction-integration bugs to debug (3c/3d).
	// ENABLED (cont.69s 3d): CTSGun is client-predicted so its viewmodel anims stick. UseDecrement()
	// == true makes (a) the server pack CTSGun into weapon_data (GetWeaponData) + decrement its 0-based
	// timers each frame (CBasePlayer::ItemPostFrame), and (b) the client run CTSGun::ItemPostFrame each
	// prediction frame. Requires CTSGun's timers on UTIL_WeaponTimeBase() (done) — NOT absolute time.
	bool UseDecrement() override
	{
#if defined(CLIENT_WEAPONS)
		return true;
#else
		return false;
#endif
	}
	// (cont.89: GetWeaponData/SetWeaponData overrides removed — TS predicted weapon state rides the
	// clientdata vuser block, not weapon_data; see dlls/client.cpp + cl_dll/hl/hl_weapons.cpp.)

	void AlternateAttack() override;             // @AlternateAttack @0x71ba8 — +alt2 (IN_CANCEL): ColdCock if the weapon can
	void ColdCock();                             // @ColdCock — a quick pistol-whip melee bash
	void Fire();                                 // dispatch one shot by fire-type
	void StandardFire(const s_weapon_info *wi);  // bullet/pellet fire (StandardFire)
	void ShotgunFire(const s_weapon_info *wi);   // @ ShotgunFire__6CTSGun @0x72108 — pump shotgun: ALTERNATES
	                                             // fire-step / pump(rack)-step per trigger-frame (see .cpp)
	void VKickback(float magnitude, int firetype); // @VKickback @0x70414 — recoil view-punch (punchangle)
	void ScaleSpread(float &spread, float increment, bool silenced); // @ScaleSpread @0x702a8 — dynamic accuracy (4th arg = the SILENCED flag, round-6 RE; sil tightens x0.875/x0.775)
	void KnifeFire();                            // @KnifeFire__6CTSGun @0x74044 — knife slash AND throw (item+0x24)
	void CCombatAttack();                    // @CCombatAttack — kung-fu contextual punch/kick (reads IN_ATTACK/IN_ATTACK2)
	void CCombatBlock();                     // @CCombatBlock — RELOAD melee block (all melee): "hooah" + stamina
	void CCombatIdle();                      // @0x85e54 — bare-hands viewmodel idle (WeaponIdle no-item branch)
	void GrenadeFire();                          // @GrenadeFire @0x732f0 — PULL THE PIN (cook; no fuse yet)
	void GrenadeIdle(int fForce);                // @GrenadeIdle @0x74f18 — the cook machine: throw on release
	                                             // (fForce=1 = holster put-away throw), post-throw consume
	void KatanaFire();                           // @KatanaFire__6CTSGun @0x733f0 — the 5-stance katana slash
	void KatanaThrow();                          // @KatanaThrow__6CTSGun @0x73b68 — right-click HURLS the katana
	bool AkimboFire(const s_weapon_info *wi);    // dual-wield fire-types (4/5); false = dry (Fire skips the anim)
	void SwitchFireMode();                       // cycle the held weapon's fire-mode
	void SwitchItems();                          // @SwitchItems__6CTSGun (useitems) — toggle ACTIVE attachments (laser/flash)
	int  GetActiveItems() const;                 // @GetActiveItems__6CTSGun — active attach mask (+0x40 if akimbo)

	void Reset();                     // @0x705e0 — wipe the whole arsenal + active state to bare kung-fu

	// arsenal (the items-by-tsweapon_t array). @ TakeWeapon/SelectWeapon/DropWeapon__6CTSGun.
	bool IsOwned(int type) const { return type >= 0 && type < TSW_COUNT && m_items[type].owned; }
	// (OwnedCount deleted — no original counterpart; fold-plan Y1.)
	// @TakeWeapon @0x70a04 — install into the arsenal: akimbo combine (attach/active unions),
	// the STACKABLE merge (+1 per take, capped at wi->iPellets — knife 5 / sealknife 10), or a
	// fresh slot (pStatus = the dropped gun's full carried state). iAttach = the BOUGHT
	// attachment mask (ORed with the integral ones, clamped to the mountable set). NO ammo
	// grant and NO auto-wield — reserve comes from BuyAmmo/packs; PickUpWeapon selects.
	bool TakeWeapon(tsweapon_t type, const TSItem *pStatus = nullptr, int iAttach = 0);
	// (SlotsUsed deleted — demoted to the file-static TSSlotsUsed in ts_gun.cpp; the original
	// maintains a STORED counter at player+0x548 instead. Fold-plan Y2.)
	void Holster() override;      // @Holster @0x75674 — pure forwarder to HolsterWeapon() (0x21 bytes;
	                              //   without it we fell through to the stock CBasePlayerWeapon::Holster)
	void HolsterWeapon();         // @HolsterWeapon @0x85698 — park state + reset to bare hands (kungfu draw)
	void SelectWeapon(int type);  // @SelectWeapon — wield an owned type (HolsterWeapon->deploy)
	void GetBetterWeapon(int exclude);// @GetBetterWeapon — auto-select next owned by priority
	void DropWeapon(int type, bool fThrow); // @DropWeapon__6CTSGun — drop an owned item from the arsenal
	bool MultiplyClips();             // infinite-clip powerup: multiply the held reserve
	bool AddAmmo(int caliber, int count); // @AddAmmo__6CTSGun — add reserve rounds (capped)
	void SyncActiveToItem();          // park the live active clip/mode back into its item
	void LoadActiveFromItem();        // load the active item's clip/mode into the live state
	// (UpdateActivePointers deleted — no original helper; the m_pActiveItem/m_pActiveInfo
	// assignments live inline at each m_weaponType-change site, like Deploy @0x71030 /
	// HolsterWeapon @0x75698 do. Fold-plan G5.)

	int m_weaponType = TSW_MELEE; // ACTIVE tsweapon_t (gun+0xc8). Default = kung-fu: a bare
	                              // weapon_tsgun (GiveNamedItem) is just fists; the loadout
	                              // (BuyArmament) adds the real guns. Placed/given weapons set it.
	TSItem m_items[TSW_COUNT] = {};            // the arsenal, indexed by tsweapon_t
	// Cached active-weapon pointers (the original's gun+0xcc/0xd0). m_pActiveItem = &m_items[m_weaponType]
	// or NULL for kung-fu (TSW_MELEE) — the original's `m_pActiveItem != NULL` guard == our
	// `m_weaponType != TSW_MELEE`. m_pActiveInfo = get_weapon_info(m_weaponType). Transient
	// (reassigned inline wherever m_weaponType is assigned, not saved — fold-plan G5).
	TSItem *m_pActiveItem = nullptr;             // +0xcc — the active arsenal slot (NULL = kung-fu)
	const s_weapon_info *m_pActiveInfo = nullptr; // +0xd0 — the active weapon's static template
	int m_ammoReserve[TS_NUM_CALIBERS] = {};   // spare rounds PER CALIBER (gun+0xd4 in the .so)
	int m_lastType = TSW_MELEE; // previously-wielded type (for the `lastinv` toggle, player+0xa1c)
	int m_fireMode = TSFM_SEMI; // ACTIVE fire-mode (= m_arsenalMode[m_active])
	bool m_bHoldPosition = false; // ACTIVE weapon's hold-position stance (= m_pActiveItem+0x2c); see SecondaryAttack
	// item+0x8 burstOrPhase — ONE per-item slot in the original serving three machines by context:
	// burst rounds-left (WeaponIdle's burst continuation), the pump-reload phase (ShotgunReload
	// 0 open / 1 insert / 2 close), and the grenade cook phase (GrenadeFire/GrenadeIdle). Cleared
	// on holster (@0x85698). Live mirror of the active item's slot.
	int m_burstOrPhase = 0;
	// item pumpToggle — the pump fire/rack alternation AND the akimbo barrel side (one item byte in
	// the original; Fire's akimbo 3rd-person anim reads it as 8+(toggle!=0)). We keep the live copy in
	// the INHERITED CBasePlayerWeapon::m_fInSpecialReload (CTSGun doesn't otherwise use it) because that
	// member is already mirrored BOTH ways in the client-prediction code (cl_dll/hl/hl_weapons.cpp) — so
	// the toggle is predicted for free and the host stays in sync with the server (a non-predicted toggle
	// desyncs → the host fires every step = infinite-fire; see [[predicted-weapon-gotchas]]). Parked to
	// TSItem::pumpToggle on holster (it survives switches, unlike burstOrPhase/reloading).
	float m_flNextAlternateAttack = 0; // gun+0xa4 — the +alt2 (IN_CANCEL) gate, armed by ColdCock (+0.725)
	float m_flLastShotTime = 0; // gun+0x9e8 — last bullet-fire time, for ScaleSpread's per-shot recovery
	int m_comboStep = 0;        // kung-fu PUNCH combo counter (this+0x9fc in the .so): 3 jabs → uppercut, reset on finisher
	int m_kickCombo = 0;        // kung-fu KICK combo counter (this+0xa00 in the .so): 3 kicks → spin finisher, reset on finisher/dive
	int m_lastMeleeAnim = 0;    // last kung-fu attack id (this+0xa04): drives the no-repeat anim cycling per move
	float m_lastCCTime = 0.0f;  // last close-combat attack time (combo only continues if rapid/held)
	float m_flGrenadeCook = 0;  // gun+0xa0c — set to time+5 at pin-pull (consumer VERIFY: reads @0xbfe54/0xcb09d)
	unsigned short m_usFire = 0; // events/fire.sc handle (predicted fire: sound/flash/anim)
	unsigned short m_usDraw = 0; // events/draw.sc handle (deploy: seed the client param cache)
	unsigned short m_usMelee = 0; // events/knife.sc handle (gun+0x972): every knife slash, iparam1=weaponType, iparam2=bloody
	unsigned short m_usKatana = 0; // events/katana.sc handle (gun+0x974): every katana swing, iparam1=vmAnim, iparam2=bloody
	unsigned short m_usGrenade = 0; // events/grenade.sc handle (gun+0x978): iparam1 = 19 pin / 20 overhand / 21 underhand
	unsigned short m_usColdCock = 0; // events/coldcock.sc handle (gun+0x966): every bash, iparam1=type,
	                                 // iparam2=attach, fparam1/2 = the aim dir x/y (unconsumed client-side)
	unsigned short m_usCloseCombat = 0; // events/closecombat.sc handle (gun+0x96c): iparam1 = move id
	                                    // (0 block/1 punch/2 second/3 back), iparam2 = anim-block index,
	                                    // fparam1 = (float)weaponType
	unsigned short m_usEmpty = 0;  // events/empty.sc handle (gun+0x968): the dry-fire click (all params 0)
	unsigned short m_usPump = 0;   // events/pump.sc handle (gun+0x96a): the rack half — iparam1=type,
	                               // iparam2=attach (anim/shell/rack sound are all client-side)
	unsigned short m_usFireLinked = 0; // events/firelinked.sc handle (gun+0x96e): the akimbo BOTH-barrels
	                                   // shot — fparam1/2=spread, iparam1=type, iparam2=attach, bparam2=seed
	unsigned short m_usReload = 0; // events/reload.sc handle (gun+0x970): iparam1=type, iparam2=attach,
	                               // fparam1 = 0 whole-clip / 8 pump-open / 9 shell-insert / 10 pump-close

	// Cached "last sent" WeaponInfo fields (the TS ammo HUD). The original sends WeaponInfo from
	// CTSGun::UpdateClientData @0x758c8 when the type (gun+0x97a) or the reserve (gun+0x9b4) changes;
	// Reset clears both to 0. The client keeps the clip, fire mode and attachments itself.
	int m_iClientWeaponType = 0;
	int m_iClientReserve = 0;
	int m_iClientAttach = -1; // last-broadcast ACTIVE attach mask (ActItems change-detect)

	// Cached per-weapon-type WStatus (the weapon-select bar): clip if owned, else -1.
	// The original sends WStatus per arsenal slot on change; we diff against this.
	int m_iClientWStatus[TSW_COUNT];
	bool m_bWStatusInit = false;
	// gun+0x9e0 — UpdateClientData runs at most every 0.1 s (@0x758e7); Deploy clears it (@0x71064)
	float m_flNextClientUpdate = 0;
};

// Find the player's CTSGun item (the single arsenal weapon), or NULL. Defined in ts_gun.cpp.
CTSGun *GetPlayerTSGun(CBasePlayer *pPlayer);

#endif // TS_GUN_H
