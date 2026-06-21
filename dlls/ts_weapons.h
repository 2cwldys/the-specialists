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

// The Specialists weapon roster (38 weapons, indices 0..37).
enum tsweapon_t
{
	TSW_MELEE = 0,
	TSW_GLOCK18,
	TSW_UZI,
	TSW_UZI_AKIMBO,
	TSW_BENELLIM3,
	TSW_M4,
	TSW_MP5SD,
	TSW_MP5K,
	TSW_BERETTA_AKIMBO,
	TSW_BERETTA,        // 9 — vertical-slice reference weapon
	TSW_MK23,
	TSW_MK23_AKIMBO,
	TSW_USAS12,
	TSW_DESERT,
	TSW_AK47,
	TSW_FNH,
	TSW_AUG,
	TSW_SKORPION,
	TSW_BARRETT,
	TSW_PDW,
	TSW_SPAS12,
	TSW_GOLD,
	TSW_GLOCK22,
	TSW_MAC10,
	TSW_M61,
	TSW_KNIFE,          // 25
	TSW_MOSSBERG,
	TSW_M16,
	TSW_RUGER,
	TSW_FNH_AKIMBO,
	TSW_BULL,
	TSW_M60,
	TSW_SAWEDOFF,
	TSW_KATANA,
	TSW_SEALKNIFE,
	TSW_CONTENDER,
	TSW_SKORPION_AKIMBO,
	TSW_FLAG,           // 37
	TSW_COUNT
};

// Weapon stats. A clean modern subset of the original 988-byte s_weapon_info;
// fields/values recovered from Write_beretta @0x5e4a8 (VERIFY numeric semantics
// against more weapons as the roster is filled in).
struct s_weapon_info
{
	const char *pszName;       // display name        e.g. "Beretta 92F"
	const char *pszHudSprite;  // sprites/weapons/*.spr
	const char *pszViewModel;  // models/v_*.mdl
	const char *pszPlayerModel; // models/p_*.mdl
	const char *pszPlayerModelSil; // models/p_*_sil.mdl (silenced variant)
	const char *pszWorldModel; // models/w_*.mdl
	const char *pszFireSound;  // weapons/*/fire.wav
	const char *pszFireSoundSil; // weapons/*/fire-sil.wav

	int   iClip;      // rounds per clip      (beretta: 15)
	float flDamage;   // per-bullet damage    (beretta: 29.0)
	float flSpread;   // base spread          (beretta: 0.015)
	float flRecoil;   // kickback             (beretta: 3.0)
	int   iFireRate;  // rounds/min-ish       (beretta: 900) VERIFY
	int   iAmmoType;  // ammo class           VERIFY
	int   iFireModes; // available fire modes VERIFY
};

// @ get_weapon_info__F10tsweapon_t — stats for a weapon (table[0] if out of range).
const s_weapon_info *get_weapon_info(tsweapon_t w);

#endif // TS_WEAPONS_H
