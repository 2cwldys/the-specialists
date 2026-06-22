/***
*  ts_weapons.h — RECONSTRUCTED TS weapon data layer (tsweapon_t + s_weapon_info).
*  Functional/source-faithful (see CLAUDE.md). NOT byte-exact: the original
*  s_weapon_info is a 988-byte struct; we keep a clean modern struct with the
*  fields CTSGun actually uses, recovered from the Write_<weapon> initializers.
*
*  tsweapon_t order recovered from compile_weapons_info() @0x6fe28 (the order in
*  which it calls Write_<weapon>, which == the get_weapon_info() table index).
*  get_weapon_info(w) @0x6f190: returns &table[w] for w in [0,0x25], else table[0].
***/
#ifndef TS_WEAPONS_H
#define TS_WEAPONS_H

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
	const char *pszPlayerModel; // models/p_*.mdl
	const char *pszWorldModel;  // models/w_*.mdl
	const char *pszAnimExt;     // DefaultDeploy anim ext: onehanded/twohanded/akimbo
	const char *pszFireSound;   // weapons/*/fire.wav
	int   iClip;       // rounds per clip
	float flDamage;    // per-bullet damage
	int   iFireType;   // s_weapon_info+0x348 — see TSFT_* below
	float flFireRate;  // seconds between shots (s_weapon_info+0x18)
	float flSpread;    // base bullet spread cone (s_weapon_info+0x28)
	float flRecoil;    // recoil / spread-scale factor (s_weapon_info+0x34)
	// Akimbo pairing (recovered from Write_<weapon>): s_weapon_info+0xc8 = the akimbo
	// variant this single upgrades into when a 2nd identical gun is picked up (-1 if
	// none); +0xcc = the single-pistol base of an akimbo (= self for non-akimbo, -1
	// for the only-akimbo gold colts). Set in compile_weapons_info (see ts_weapons.cpp).
	int   iAkimboType; // +0xc8 — single -> akimbo upgrade target, or TSW_MELEE-sentinel -1
	int   iSingleType; // +0xcc — akimbo -> single base
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
	// paths). SLICE: we drive it server-side via SendWeaponAnim; the original predicts
	// it client-side through the events/fire.sc event (CTSGun precaches m_usFire@0x964).
	int   iFireAnim;
	// Akimbo weapons fire one barrel at a time: iFireAnim = right-gun shoot sequence,
	// iFireAnimL = left-gun shoot sequence (both read from the v_*_akimbo.mdl). -1 for
	// non-akimbo weapons (they only use iFireAnim).
	int   iFireAnimL;
	// Viewmodel reload-animation sequence index (the "reload" sequence in v_*.mdl).
	// -1 = no reload anim (melee/grenade).
	int   iReloadAnim;
};

// Reload duration (seconds) — // VERIFY against the .so per-weapon reload time (the
// original reads s_weapon_info+0x14); a single value for now.
#define TS_RELOAD_TIME 2.0f

// Per-caliber ammo: the 16 calibers, the mag/magazine size, and the max reserve (rounds),
// from compile_ammo_info (@0x6f51c). Indices match the weapon iCaliber + the FGD ts_groundammo
// magsN keyvalues (magN -> caliber N).
#define TS_NUM_CALIBERS 16
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
const s_weapon_info *get_weapon_info(tsweapon_t w);

// Precache every weapon's models + sound (call during the map's precache window).
void PrecacheAllTSWeapons();

// Give a player a CTSGun configured for `type` (used by pickups / future buy menu).
class CBaseEntity;
class CBasePlayer;
CBaseEntity *GiveTSWeapon(CBasePlayer *pPlayer, tsweapon_t type);

// Weapon-select (the faithful protocol — see reference/WEAPON_SELECT.md). The client's
// invnext/invprev select cursor emits the `weapon_<N>` command (numeric tsweapon_t);
// TSSelectWeaponType wields that owned type (holster->deploy). `lastinv` toggles back to
// the previously-wielded weapon.
void TSSelectWeaponType(CBasePlayer *pPlayer, int type);
void TSLastInv(CBasePlayer *pPlayer);

// Trigger the held TS gun's alternate attack (+alt2 → coldcock pistol-whip).
void TSAlternateAttack(CBasePlayer *pPlayer);

// Cycle the held weapon's fire-mode (the `firemode` command -> CTSGun::SwitchFireMode).
void TSSwitchFireMode(CBasePlayer *pPlayer);

// Add `count` rounds of `caliber` to the player's TS gun reserve (capped). Returns true if
// any were added. Used by CTSGroundAmmo (the per-caliber refill station).
bool TSAddAmmo(CBasePlayer *pPlayer, int caliber, int count);

// Drop the player's active TS weapon (out: its type + loaded clip); false if it can't drop
// (held <2 weapons). Pick up a dropped weapon of `type`/`clip`. (CTSWorldGun + `drop` cmd.)
bool TSDropActiveWeapon(CBasePlayer *pPlayer, int &type, int &clip);
void TSPickUpWeapon(CBasePlayer *pPlayer, int type, int clip);
void TSDropWeaponsOnDeath(CBasePlayer *pPlayer); // scatter the arsenal on death
void TSDropAmmoOnDeath(CBasePlayer *pPlayer);    // drop the per-caliber reserve as a pack

// Spawn a dropped ammo pack carrying per-caliber `counts[16]`. Defined in ts_ammopack.cpp.
CBaseEntity *TSCreateAmmoPack(struct entvars_s *pevOwner, const int *counts, const Vector &origin,
	const Vector &velocity);

// Spawn a dropped weapon entity (CTSWorldGun) carrying `type`/`clip`. Defined in ts_worldgun.cpp.
CBaseEntity *TSCreateWorldGun(int type, int clip, const Vector &origin, const Vector &velocity);

// Infinite-clip powerup: multiply the held weapon's reserve. Returns true if applied.
bool TSMultiplyClips(CBasePlayer *pPlayer);

// Toss an M61 grenade (the grenade fire-type / grenade powerup). Defined in ts_grenade.cpp.
struct entvars_s;
CBaseEntity *TSThrowM61(struct entvars_s *pevOwner, const Vector &vecStart,
	const Vector &vecVelocity, float fuse, float damage);

#endif // TS_WEAPONS_H
