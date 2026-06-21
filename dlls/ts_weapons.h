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
};

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

// Cycle to the next weapon in the player's TS arsenal (the `tsnextweapon` command).
void TSCycleWeapon(CBasePlayer *pPlayer);

// Infinite-clip powerup: multiply the held weapon's reserve. Returns true if applied.
bool TSMultiplyClips(CBasePlayer *pPlayer);

// Toss an M61 grenade (the grenade fire-type / grenade powerup). Defined in ts_grenade.cpp.
struct entvars_s;
CBaseEntity *TSThrowM61(struct entvars_s *pevOwner, const Vector &vecStart,
	const Vector &vecVelocity, float fuse, float damage);

#endif // TS_WEAPONS_H
