/***
*  ts_weapon_info.h — SHARED TS weapon data types (tsweapon_t + s_weapon_info).
*
*  The pure-data half of the TS weapon layer: the weapon roster enum, the
*  s_weapon_info struct, and get_weapon_info(). Built into BOTH the server
*  (dlls/) and the client (cl_dll/) so the predicted .sc event handlers read the
*  SAME table the server uses (the original keeps a client copy at DAT_100aa050,
*  built by a client compile_weapons_info mirror — we share one TU instead).
*
*  No engine/SDK dependencies (no extdll.h/cbase.h) so it compiles cleanly on the
*  client side. The server-only weapon helpers (Precache/Give/drop/select/…) live
*  in ts_weapons.h, which includes this.
*
*  tsweapon_t order recovered from compile_weapons_info() @0x6fe28 (the order in
*  which it calls Write_<weapon>, == the get_weapon_info() table index).
*  get_weapon_info(w) @0x6f190: returns &table[w] for w in [0,0x25], else table[0].
***/
#ifndef TS_WEAPON_INFO_H
#define TS_WEAPON_INFO_H

// The Specialists weapon roster. Indices are the real get_weapon_info() table
// indices (= each Write_<weapon>'s table slot in compile_weapons_info @0x6fe28),
// which equal the FGD tsweaponid choices (ts_groundweapon "Weapon"). NOTE these
// are NOT the order the Write_ functions are *called* in.
enum tsweapon_t
{
	TSW_MELEE = 0,
	TSW_GLOCK18 = 1,
	TSW_BERETTA = 2,         // vertical-slice reference weapon
	TSW_UZI = 3,            // "Mini Uzi"
	TSW_BENELLIM3 = 4,
	TSW_M4 = 5,
	TSW_MP5SD = 6,
	TSW_MP5K = 7,
	TSW_BERETTA_AKIMBO = 8,
	TSW_MK23 = 9,
	TSW_MK23_AKIMBO = 10,
	TSW_USAS12 = 11,
	TSW_DESERT = 12,
	TSW_AK47 = 13,
	TSW_FNH = 14,           // "FiveSeveN"
	TSW_AUG = 15,
	TSW_UZI_AKIMBO = 16,
	TSW_SKORPION = 17,
	TSW_BARRETT = 18,
	TSW_PDW = 19,
	TSW_SPAS12 = 20,
	TSW_GOLD = 21,         // "Akimbo colts"
	TSW_GLOCK22 = 22,      // "Glock 20"
	TSW_MAC10 = 23,        // "HK UMP"
	TSW_M61 = 24,          // grenade
	TSW_KNIFE = 25,
	TSW_MOSSBERG = 26,
	TSW_M16 = 27,
	TSW_RUGER = 28,
	TSW_FLAG = 29,         // CTF flag — not in compile_weapons_info (separate)
	TSW_FNH_AKIMBO = 30,
	TSW_BULL = 31,
	TSW_M60 = 32,
	TSW_SAWEDOFF = 33,
	TSW_KATANA = 34,
	TSW_SEALKNIFE = 35,
	TSW_CONTENDER = 36,
	TSW_SKORPION_AKIMBO = 37,
	TSW_COUNT = 38
};

// Weapon stats — a clean modern subset of the original 988-byte s_weapon_info,
// holding the fields CTSGun actually uses. Values recovered from each
// Write_<weapon> (scripts/extract_weapons.py): field offsets 0x44 name,
// 0x132 v_model, 0x172 p_model, 0x1f2 w_model, 0x232 anim-ext, 0x2c6 fire sound,
// 0x04 clip, 0x1c damage, 0x348 fire-type.
struct s_weapon_info
{
	const char *pszName;        // display name        "Beretta 92F"
	const char *pszViewModel;   // models/v_*.mdl
	const char *pszPlayerModel;    // models/p_*.mdl
	const char *pszPlayerModelSil; // +0x1b2 — the silenced p_ variant (Deploy picks it iff attach&1)
	const char *pszWorldModel;  // models/w_*.mdl
	const char *pszAnimExt;     // DefaultDeploy anim ext: onehanded/twohanded/akimbo
	const char *pszFireSound;   // weapons/*/fire.wav
	int   iClip;       // rounds per clip
	// ⚠⚠ SPARTA TWO-STAGE WARNING — the next four fields are OVERWRITTEN by Sparta() after every
	// Write_<weapon> (SetSpartaWeapon @0x6fa34 == client FUN_10007a50; memory
	// [[sparta-overrides-write-weapon]]). The Write_ disasm values for +0x1c/+0x28/+0x30/+0x34
	// are a DEAD EARLIER STAGE — never extract these four columns from Write_ bodies (that bit
	// us: skorpion viewkick, bull, mossberg, ...). EVERY OTHER column (clips, calibers, caps,
	// prices, anim blocks, fire types, ...) is Write_-stage truth — Sparta touches nothing else.
	float flDamage;    // +0x1c ⚠ SPARTA-OWNED — per-bullet damage (the Sparta() table is live)
	int   iFireType;   // s_weapon_info+0x348 — see TSFT_* below
	float flFireRate;  // seconds between shots (s_weapon_info+0x18) — Write_ truth (NOT Sparta)
	float flSpread;    // +0x28 ⚠ SPARTA-OWNED — base bullet spread cone
	float flRecoil;    // +0x34 ⚠ SPARTA-OWNED — per-shot spread INCREMENT (CTSGun::ScaleSpread)
	// The TS dynamic-accuracy fields (RE'd from CTSGun::StandardFire @0x824a4 + ScaleSpread @0x802a8):
	float flViewKick;  // +0x30 ⚠ SPARTA-OWNED — VKickback view-punch per shot (bull 13 NOT the
	                   // Write_ 18; barrett 10; autos 4-5 — always read the Sparta() table)
	int   bUsesSpread; // +0x2c — if set, StandardFire runs ScaleSpread (dynamic recoil); 0 = static (shotgun/melee)
	// NB: ScaleSpread accumulates spread into CBasePlayer+0x778 and the effective spread = flSpread + that
	// accumulator; the accumulator's recovery path is still being RE'd, so the accumulation isn't wired yet.
	// Akimbo pairing (recovered from Write_<weapon>): s_weapon_info+0xc8 = the akimbo
	// variant this single upgrades into when a 2nd identical gun is picked up (-1 if
	// none); +0xcc = the single-pistol base of an akimbo (= self for non-akimbo, -1
	// for the only-akimbo gold colts). Set in compile_weapons_info (see ts_weapon_info.cpp).
	int   iAkimboType; // +0xc8 — single -> akimbo upgrade target, or TSW_MELEE-sentinel -1
	int   iSingleType; // +0xcc — akimbo -> single base
	// Cold-cock (rifle-butt / pistol-whip) damage (s_weapon_info+0x10). 0 = the weapon can't
	// coldcock (the AlternateAttack gate). Per-weapon values RE'd from CTSGun::ColdCock @0x81dd0.
	int   iColdCockDamage; // +0x10 — coldcock hit damage (0 = no coldcock)
	// Thrown-blade type code (s_weapon_info+0x20): knife 6 / sealknife 7 / katana 8; 0 = not a
	// blade. Passed to CreateKnife (drives the projectile's model/gravity/pickup identity).
	int   iKnifeType;      // +0x20 — CreateKnife type code (6/7/8, 0 = none)
	// Kung-fu idle viewmodel sequence (s_weapon_info+0x252; 0 in the shipped data — kept as a
	// table read because CTSGun::CCombatIdle reads it from the TSW_MELEE row).
	int   iIdleAnim;       // +0x252 — bare-hands idle anim (0 in shipped data)
	// Blade swing viewmodel sequences — the anim block entries [15..17] (shorts at wi+0x270/
	// 0x272/0x274, disasm-read from Write_knife/sealknife/katana): the logical swing ids the
	// knife.sc/katana.sc events carry, remapped client-side to v_*.mdl sequences.
	int   iSwingAnims[3];  // +0x270/0x272/0x274 — swing ids 15/16/17 -> viewmodel sequence
	// Buy price (s_weapon_info+0xd8), disasm-read from every Write_<weapon>. The CustomWP
	// pipeline (SendWeaponParams/adjust_weapon_value) reads+writes it; the buy menu prices
	// resolve through it so a customized server re-prices live.
	int   iPrice;          // +0xd8 — buy price (cash)
	// Attachment viewmodel-body offsets (shorts at wi+0x128..0x130, disasm-read from every
	// Write_<weapon>): per-bit adds for silencer/laser/flashlight/scope + the laser+flash
	// combined body (-1 = none). GetBodyByItems composes the viewmodel body from these.
	int   iBodySilencer;   // +0x128 — mask bit 1
	int   iBodyLaser;      // +0x12a — mask bit 2
	int   iBodyFlash;      // +0x12c — mask bit 4
	int   iBodyScope;      // +0x12e — mask bit 8
	int   iBodyLaserFlash; // +0x130 — bits 2|4 together (-1 = use the separate adds)
	// Fire-mode mask (s_weapon_info+0xc): bitset of the trigger modes this weapon offers,
	// cycled by CTSGun::SwitchFireMode. Bit N set => mode N (tsfiremode_mode_t) available.
	int   iFireModes;  // +0xc — available fire-modes bitmask
	// Ammo caliber index (s_weapon_info+0x8): which of the 16 per-caliber reserves this
	// weapon draws from. 0 = none (melee/grenade). Reserves are shared across all weapons
	// of the same caliber (so akimbo berettas + a single beretta share the 9mm pool).
	int   iCaliber;    // +0x8 — ammo caliber index (0..15)
	// Bullets fired per shot (s_weapon_info+0x24): 1 for most guns, 8 for the shotguns
	// (benelli/usas/spas/mossberg/sawed-off). Drives the FireBulletsPlayer pellet count.
	int   iPellets;    // +0x24 — bullets/pellets per shot
	// Viewmodel fire-animation sequence index (the "shoot*" sequence in v_*.mdl, read
	// from the models). -1 = no standard fire anim (melee/grenade/akimbo use their own
	// paths). The original predicts it client-side through events/fire.sc (CTSGun
	// precaches m_usFire@0x964); the client event handler reads this from the shared table.
	int   iFireAnim;
	// Akimbo weapons fire one barrel at a time: iFireAnim = right-gun shoot sequence,
	// iFireAnimL = left-gun shoot sequence (both read from the v_*_akimbo.mdl). -1 for
	// non-akimbo weapons (they only use iFireAnim).
	int   iFireAnimL;
	// Viewmodel reload-animation sequence index (the "reload" sequence in v_*.mdl).
	// -1 = no reload anim (melee/grenade).
	int   iReloadAnim;
	// Hold-position (alternate stance) toggle — RE: CTSGun::SecondaryAttack@0x717a8. On a
	// non-akimbo, non-scoped weapon, right-click toggles m_pActiveItem+0x2c and re-points the
	// active anim block at the hold set (wi+0x28c) / normal set (wi+0x252), playing the model's
	// change/rechange transition; the fire anim then comes from the alternate stance's shoot seq.
	// Indices read straight from each v_*.mdl (the "b"/"c"/"2" sequence set). iHoldChange < 0 =>
	// the weapon has no hold stance (right-click does nothing for it).
	int   iHoldChange;    // normal -> hold transition anim (-1 = no hold stance)
	int   iHoldRechange;  // hold -> normal transition anim
	int   iHoldFireAnim;  // fire anim played while in the hold stance
	// Reload anim while in the hold stance (the "b" reload variant, e.g. mk23 reloadb / mossberg
	// reload1b). RE Reload__6CTSGun reads the reload seq from the active stance's anim block (item+0x28
	// +2), so reloading in the hold stance plays the held-grip reload and ENDS back in the hold pose.
	// -1 => use iReloadAnim in both stances. For pump shotguns this is the hold-stance reload1 base
	// (insert/close = +1/+2, same as iReloadAnim).
	int   iHoldReloadAnim;
	// Scope ZOOM levels (s_weapon_info+0x3c / +0x40) — the FOV values cycled by CTSGun::SecondaryAttack's
	// scoped branch (attach & 8): default FOV 0 (no zoom) -> zoomFov1 -> zoomFov2 -> back to 0. 0 = the
	// level doesn't exist (most weapons have only one zoom; ak47/m16/barrett have a second). Recovered
	// from each Write_<weapon>. Zoom hides the viewmodel when zoomFov1 < 40 (high-magnification scopes).
	int  zoomFov1;    // +0x3c — first zoom FOV (0 = no scope zoom)
	int  zoomFov2;    // +0x40 — second zoom FOV (0 = no second level)
	// Attachment masks (s_weapon_info+0xe0 / +0xe4), bits 1/2/4/8 = silencer/laser/flashlight/scope.
	// attachSupported = every attachment this weapon CAN mount; attachBuiltin = attachments it ships WITH
	// (integral — can't be bought again, and present from the moment you pick the gun up). The buyable set
	// the menu offers = attachSupported & ~attachBuiltin. Built-in examples: AUG/Barrett integral scope (8),
	// MP5SD/Ruger integral silencer (1). Recovered from each Write_<weapon> (+0xe0/+0xe4).
	int  attachSupported; // +0xe0 — attachments this weapon can mount
	int  attachBuiltin;   // +0xe4 — attachments it ships with (item.attach defaults to this on give)
	// Inventory SLOT cost (s_weapon_info+0xd0, a byte). The player has 81 slots (CBasePlayer+0x548 init
	// 0x51); each carried weapon's slotCost is deducted, and remaining capacity drives movement speed
	// (GetSpeedBySlots: maxspeed = 210 + (120/81)*remaining) — more/heavier weapons => slower. Also gates
	// floor pickups (can't pick up what won't fit). Recovered from each Write_<weapon> (+0xd0).
	int   slotCost;
	// Reload duration in seconds (s_weapon_info+0x14, disasm-read from every Write_<weapon>):
	// CTSGun::Reload passes it to ReloadWeapon as the whole-clip delay; for the pump shotguns
	// (0.3) it's the per-SHELL insert period in ShotgunReload phase 1.
	float flReloadTime;
	// STACKABLE flag (s_weapon_info+0xdc): the clip is a COUNT of carried units (knife/
	// sealknife/m61 grenade = 1). Slot cost is charged PER UNIT (PickUpWeapon multiplies
	// slotCost by the world item's clip), and MultiplyClips adds units funded by free slots.
	int   iStackable;
	// Hold-stance idle anim — the HOLD anim block's entry [0] (short at wi+0x28c); -1 = the
	// weapon has no hold stance block (65535 in the disasm). Normal-stance idle = iIdleAnim.
	int   iIdleAnimHold;
	// Shell-retained reload anim — anim block entry [8] (short at wi+0x262 / hold +0x29c):
	// Reload uses it ONLY for the sawed-off with a shell still chambered (@0x74d5c weapon 33,
	// clip > 0). -1 = none.
	int   iReloadKeepAnim;
	int   iReloadKeepAnimHold;
	// COLDCOCK viewmodel anim — anim block entry [28] (short at wi+0x28a / hold +0x2c4,
	// disasm-read; exactly the iColdCockDamage set carries one). The original plays it via the
	// coldcock.sc client handler; our stand-in SendWeaponAnims it until the ev swap lands. -1 = none.
	int   iColdCockAnim;
	int   iColdCockAnimHold;
	// The FULL viewmodel ANIM BLOCKS (29 shorts per stance: normal wi+0x252, hold wi+0x28c) —
	// the client event handlers' anim source: they play animBlock[logical index]. [0] idle,
	// [1] reload, [2] draw, [3..6] shoots, [8..14] pump-reload family, [15..18] melee swings +
	// the armed block [0x12], [19..21] grenade pin/overhand/underhand, [20] knife throw,
	// [22..27] akimbo barrels, [28] coldcock. -1 = unset. The item's live block pointer
	// (item+0x28) re-points between the two per the hold stance (WeaponIdle).
	short animBlock[29];
	short animBlockHold[29];
	// The ALT SOUND (wi+0x306, disasm-adjudicated per Write_): a triple-duty sample —
	// the -sil FIRE for silencer-capable guns, the PUMP RACK for the pump shotguns
	// (benelli AND mossberg both use m3-pump.wav; spas12 its own), and the akimbo LINKED
	// both-barrel bang (the -sil sample reused; Write_gold and Write_skorpion_akimbo store
	// none). NULL = none.
	const char *pszAltSound;   // +0x306
	// The ejected SHELL model (wi+0xe8) + the eject-port setback along fwd (wi+0x34c, int
	// units) — pump.sc / the fire core eject from attachment − fwd*setback.
	const char *pszShellModel; // +0xe8
	int iShellSetback;         // +0x34c
};

// Per-caliber ammo: the 16 calibers, the mag/magazine size, and the max reserve (rounds),
// from compile_ammo_info (@0x6f51c). Indices match the weapon iCaliber + the FGD ts_groundammo
// magsN keyvalues (magN -> caliber N).
#define TS_NUM_CALIBERS 16
// s_ammo_info — the per-caliber ammo table (original 0x38-byte struct; offsets are
// provenance). Built by compile_ammo_info (chained from compile_weapons_info).
struct s_ammo_info
{
	float flFactor;      // +0x00 — per-caliber factor; nothing in ts_i386.so reads it
	int   iPrice;        // +0x04 — cash per magazine (BuyAmmo)
	int   iKind;         // +0x08 — 1 pistol / 2 rifle-shotgun (falloff form selector)
	int   iMaxCarry;     // +0x0c — reserve cap; the ONLY Set_ammo_count-scaled column
	int   iRoundsPerMag; // +0x10 — the buy/refill unit
	int   iMaxTravel;    // +0x14 — 8192 for all calibers: ShootBulletsPlayer's trace length (@0x5fc2c)
	float flFactor2;     // +0x18 — the pierce decay (ShootBulletsPlayer)
	float flMult;        // +0x1c — 1.0 for all entries
	int   iRangeUnit;    // +0x20 — full-damage range unit (falloff form B)
	int   iSpeedUnit;    // +0x24 — slow-mo bullet flight speed unit (x40)
	char  szName[16];    // +0x28 — caliber display name
};
extern s_ammo_info global_ammo_info[16];
void compile_ammo_info(void);            // @0x6f51c
s_ammo_info *get_ammo_info(int cal);     // @0x6f1c4 — out-of-range/0 -> the zero entry
void Set_ammo_count(int mode);           // @0x6f2cc — the ammocount-mode max-carry column (SHARED)
void compile_range_damage(void);         // @0x6f1f4 — the falloff LUT
float get_range_multipler(int idx);      // @0x6f290 — clamped LUT read

int GetBodyByItems(int type, int items); // @0x6f0a4 — attachment mask -> viewmodel body (Deploy)
int TSAmmoMax(int caliber); // max reserve (rounds) for a caliber, 0 if none
int TSRoundsPerMag(int caliber); // rounds per magazine for a caliber (CTSGroundAmmo magsN)

// Trigger fire-modes (m_pActiveItem+2): which the CURRENT mode is, cycled among the
// weapon's iFireModes mask. Orthogonal to iFireType (which picks the fire *function*):
// the mode only gates the trigger (auto/semi/burst/pump) + rate. From SwitchFireMode
// (@0x8169c) + the TakeWeapon default-mode selection (@0x80a04).
enum tsfiremode_mode_t
{
	TSFM_AUTO = 0,        // full-auto: fire while held
	TSFM_SEMI = 1,        // semi-auto: one round per trigger pull
	TSFM_BURST = 2,       // 3-round burst (2 for the MAC-10)
	TSFM_PUMP = 3,        // pump action: fire while held (shotguns)
	TSFM_AKIMBO_ALT = 4,  // akimbo, one alternating barrel per pull
	TSFM_AKIMBO_BOTH = 5, // akimbo, fire while held (both barrels)
};

// Default fire-mode for a weapon's mode mask (TakeWeapon: auto if available, else pump,
// else burst if available, else semi).
int TSDefaultFireMode(int modeMask);

// Fire-type (s_weapon_info+0x348) — drives CTSGun::PrimaryAttack dispatch.
enum tsfiretype_t
{
	TSFT_SEMI = 0,       // semi-auto single bullet (also 8)
	TSFT_PUMP = 1,       // pump shotgun (pellets, one per trigger)
	TSFT_GRENADE = 2,    // thrown grenade
	TSFT_KNIFE = 3,      // melee slash
	TSFT_AKIMBO = 4,     // dual-wield (combinable: beretta/uzi/mk23/fnh/skorpion)
	TSFT_AKIMBO_ONLY = 5,// dual-wield only (gold colts — no single counterpart)
	TSFT_AUTOSHOTGUN = 6,// full-auto (incl. auto shotguns)
	TSFT_SNIPER = 7,     // bolt/zoom single shot
	TSFT_SEMI8 = 8,      // semi-auto (pistols)
	TSFT_AUTO = 9,       // full-auto (SMGs)
	TSFT_KATANA = 10,    // melee sword
	TSFT_FLAG = 11,      // CTF flag
};

// @ get_weapon_info__F10tsweapon_t — stats for a weapon (table[0] if out of range).
// Shared: built into both hl.so (server) and client.so (client).
void compile_weapons_info(); // @0x6fe28: (re)build the table; the client reruns it per map
const s_weapon_info *get_weapon_info(tsweapon_t w);

// The base SINGLE weapon for an akimbo variant (the reverse of iAkimboType / weapon_info+0xcc),
// or (tsweapon_t)-1 if `type` isn't an akimbo. The original's CBasePlayer::DropWeapon throws TWO
// of this single when an akimbo is dropped (reference/WEAPON_SELECT.md / @DropWeapon).
tsweapon_t TSAkimboBase(tsweapon_t type);

#endif // TS_WEAPON_INFO_H
