/***
*  ts_weapons.cpp — RECONSTRUCTED TS weapon data table (get_weapon_info).
*  Functional/source-faithful (see CLAUDE.md).
*
*  The original builds a 38-entry table via compile_weapons_info() (@0x6fe28)
*  calling Write_<weapon>(&table[i]). We do the same with a clean s_weapon_info.
*  VERTICAL SLICE: only the beretta (TSW_BERETTA) is filled for now (recovered
*  from Write_beretta @0x5e4a8); the rest land as the roster is reconstructed.
***/
#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "ts_weapons.h"

static s_weapon_info g_weaponInfo[TSW_COUNT];
static bool g_weaponInfoCompiled = false;

// @ Write_beretta__FP13s_weapon_info
static void Write_beretta(s_weapon_info *w)
{
	w->pszName = "Beretta 92F";
	w->pszHudSprite = "sprites/weapons/beretta.spr";
	w->pszViewModel = "models/v_beretta.mdl";
	w->pszPlayerModel = "models/p_beretta.mdl";
	w->pszPlayerModelSil = "models/p_beretta_sil.mdl";
	w->pszWorldModel = "models/w_berettas.mdl";
	w->pszFireSound = "weapons/beretta/fire.wav";
	w->pszFireSoundSil = "weapons/beretta/fire-sil.wav";

	w->iClip = 15;        // 0x04
	w->flDamage = 29.0f;  // 0x1c
	w->flSpread = 0.015f; // 0x28
	w->flRecoil = 3.0f;   // 0x30
	w->iFireRate = 900;   // 0xd8  VERIFY
	w->iAmmoType = 0;     // VERIFY
	w->iFireModes = 0;    // VERIFY
}

// @ compile_weapons_info__Fv — populate the weapon table (enum order).
static void compile_weapons_info()
{
	Write_beretta(&g_weaponInfo[TSW_BERETTA]);
	// TODO: remaining 37 weapons (Write_melee, Write_glock18, ... Write_flag).
	g_weaponInfoCompiled = true;
}

// @ get_weapon_info__F10tsweapon_t
const s_weapon_info *get_weapon_info(tsweapon_t w)
{
	if (!g_weaponInfoCompiled)
		compile_weapons_info();

	if (w < 0 || w >= TSW_COUNT)
		w = TSW_MELEE; // original clamps out-of-range to table[0]

	return &g_weaponInfo[w];
}
