/***
*
*	Copyright (c) 1996-2002, Valve LLC. All rights reserved.
*
*	This product contains software technology licensed from Id
*	Software, Inc. ("Id Technology").  Id Technology (c) 1996 Id Software, Inc.
*	All Rights Reserved.
*
*   Use, distribution, and modification of this source code and/or resulting
*   object code is restricted to non-commercial enhancements to products from
*   Valve LLC.  All other use, distribution, or modification is prohibited
*   without written permission from Valve LLC.
*
****/
//
// ts_ammo.cpp
//
// The Specialists ammo HUD (the bottom-right weapon box). Reads the server's
// "WeaponInfo" message — { BYTE weaponType, BYTE clip, SHORT reserve, BYTE fireMode,
// BYTE flags } — byte-faithful to CTSGun::UpdateClientData (see
// reference/HUD_MESSAGES.md). Draws the original's stacked box: weapon name, caliber,
// fire-mode, then "clip / reserve" (matching the shipped HUD's Five-seveN / MP5K
// readout). TS has no stock CurWeapon/Ammo HUD, so this is the custom replacement.
//

#include "hud.h"
#include "cl_util.h"
#include "parsemsg.h"
#include "ts_hud.h"

#include <stdio.h>

DECLARE_MESSAGE(m_TSAmmo, WeaponInfo)

// Display name per tsweapon_t (the table index = the wire weaponType). Index 0 is the
// unarmed / kung-fu "weapon"; 29 is the CTF flag (no HUD). Names follow the shipped
// HUD ("Five-seveN", "MP5K", "Combat Knife", "Kung-Fu").
static const char* const s_weaponNames[] = {
	/* 0*/ "Kung-Fu",        /* 1*/ "Glock-18",       /* 2*/ "Beretta 92F",
	/* 3*/ "Mini-Uzi",       /* 4*/ "Benelli M3",     /* 5*/ "M4",
	/* 6*/ "MP5SD",          /* 7*/ "MP5K",           /* 8*/ "Akimbo Berettas",
	/* 9*/ "SOCOM-MK23",     /*10*/ "Akimbo MK23",    /*11*/ "USAS-12",
	/*12*/ "Desert Eagle",   /*13*/ "AK-47",          /*14*/ "Five-seveN",
	/*15*/ "Steyr AUG",      /*16*/ "Akimbo Mini-Uzi",/*17*/ "Skorpion",
	/*18*/ "Barrett M82A1",  /*19*/ "PDW",            /*20*/ "SPAS-12",
	/*21*/ "Golden Colts",   /*22*/ "Glock-20C",      /*23*/ "UMP",
	/*24*/ "M61 Grenade",    /*25*/ "Combat Knife",   /*26*/ "Mossberg 500",
	/*27*/ "M16",            /*28*/ "Ruger-MK1",      /*29*/ "",
	/*30*/ "Akimbo Five-seveN",/*31*/ "Raging Bull",  /*32*/ "M60",
	/*33*/ "Sawed-off",      /*34*/ "Katana",         /*35*/ "SEAL Knife",
	/*36*/ "Contender G2",   /*37*/ "Akimbo Skorpions",
};
static const int s_numWeapons = sizeof(s_weaponNames) / sizeof(s_weaponNames[0]);

// Per-weapon caliber index (s_weapon_info+0x8, recovered server-side; mirrored here for
// the cartridge label). 0 = no cartridge (melee/grenade).
static const int s_weaponCaliber[] = {
	/* 0*/ 0, /* 1*/ 1, /* 2*/ 1, /* 3*/ 1, /* 4*/ 3, /* 5*/ 4, /* 6*/ 1, /* 7*/ 1,
	/* 8*/ 1, /* 9*/ 2, /*10*/ 2, /*11*/ 3, /*12*/ 5, /*13*/ 6, /*14*/ 7, /*15*/ 4,
	/*16*/ 1, /*17*/12, /*18*/ 8, /*19*/ 7, /*20*/ 3, /*21*/ 2, /*22*/ 9, /*23*/ 2,
	/*24*/ 0, /*25*/ 0, /*26*/ 3, /*27*/ 4, /*28*/10, /*29*/ 0, /*30*/ 7, /*31*/11,
	/*32*/ 4, /*33*/ 3, /*34*/ 0, /*35*/ 0, /*36*/13, /*37*/12,
};

// Cartridge name per caliber index. 1/7 confirmed against the shipped HUD (9 x 19mm /
// 5.7 x 28); the strings come from the .so's ammo registry. The rarer ones (8/10/13)
// are best-effort // VERIFY against the original.
static const char* const s_caliberNames[] = {
	/* 0*/ "",            /* 1*/ "9 x 19mm",    /* 2*/ ".45 ACP",
	/* 3*/ "12 Gauge",    /* 4*/ "5.56 Nato",   /* 5*/ ".50 AE",
	/* 6*/ "7.62 x 39mm", /* 7*/ "5.7 x 28",    /* 8*/ "7.62 x 51mm",
	/* 9*/ "10mm AUTO",   /*10*/ ".22 LR",      /*11*/ ".454 Casull",
	/*12*/ ".32 ACP",     /*13*/ ".45-70",
};
static const int s_numCalibers = sizeof(s_caliberNames) / sizeof(s_caliberNames[0]);

// Fire-mode labels, indexed by the WeaponInfo fireMode byte (tsfiremode_mode_t).
static const char* const s_fireModeNames[] = {"Full Auto", "Semi Auto", "Burst", "Pump Action", "Akimbo", "Akimbo"};
static const int s_numFireModes = sizeof(s_fireModeNames) / sizeof(s_fireModeNames[0]);

// Melee/grenade weapons (no magazine) show just the name — no caliber/clip/reserve.
static bool TSWeaponHasAmmo(int type)
{
	return !(type == 0 || type == 24 || type == 25 || type == 34 || type == 35);
}

bool CHudTSAmmo::Init()
{
	m_iWeaponType = -1;
	m_iClip = 0;
	m_iReserve = 0;
	m_iFireMode = 1;

	HOOK_MESSAGE(WeaponInfo);

	TSHud_RegisterCvars();

	gHUD.AddHudElem(this);

	return true;
}

bool CHudTSAmmo::VidInit()
{
	return true;
}

void CHudTSAmmo::Reset()
{
	// Keep drawing across respawns; the server resends WeaponInfo on weapon deploy.
	m_iFlags |= HUD_ACTIVE;
}

bool CHudTSAmmo::MsgFunc_WeaponInfo(const char* pszName, int iSize, void* pbuf)
{
	BEGIN_READ(pbuf, iSize);
	m_iWeaponType = READ_BYTE();
	m_iClip = READ_BYTE();
	m_iReserve = READ_SHORT();
	m_iFireMode = READ_BYTE();
	READ_BYTE(); // weapon flags (attachments — Lasersight/Silencer; unused so far)

	g_iTSActiveWeapon = m_iWeaponType; // the weapon-select bar highlights this slot

	m_iFlags |= HUD_ACTIVE;
	return true;
}

bool CHudTSAmmo::Draw(float flTime)
{
	if ((m_iFlags & HUD_ACTIVE) == 0 || m_iWeaponType < 0 || m_iWeaponType >= s_numWeapons)
		return true;

	if (gHUD.m_fPlayerDead || (gHUD.m_iHideHUDDisplay & (HIDEHUD_WEAPONS | HIDEHUD_ALL)))
		return true;

	const char* szName = s_weaponNames[m_iWeaponType];
	if (!szName || !szName[0])
		return true;

	// Exact geometry recovered from FUN_1004d7e0 (reference/HUD_DRAWCHAIN.md): a
	// right-anchored box, 0x80 (128px) wide, x = ScreenWidth - 0x88 (136); lines are
	// 0x14 (20px) tall and stack UPWARD from y = ScreenHeight - 8 (each line drops y
	// by 20 before drawing). Each strip is a FillRGBA of the HUD colour at alpha 0x0f.
	// Text is inset +2px; descriptive lines (caliber/fire-mode) draw at half
	// brightness, the name + ammo numbers at full.
	const int BOXW = 0x80;          // 128
	const int LINEH = 0x14;         // 20
	const int x = ScreenWidth - 0x88;
	const bool hasAmmo = TSWeaponHasAmmo(m_iWeaponType);

	int hr, hg, hb;
	TSHud_Color(hr, hg, hb);
	const int dr = hr / 2, dg = hg / 2, db = hb / 2; // dimmed (label) colour

	// Lines bottom→top as the original stacks them: ammo (bottom), fire-mode, caliber,
	// name (top). We draw each as a faint panel strip + its text.
	struct Line { const char* s; int r, g, b; };
	Line lines[4];
	int n = 0;

	char szAmmo[32];
	if (hasAmmo)
	{
		sprintf(szAmmo, "%d / %d", m_iClip, m_iReserve);
		int ar = hr, ag = hg, ab = hb;
		if (m_iClip == 0)
		{
			UnpackRGB(ar, ag, ab, RGB_REDISH);
			ScaleColors(ar, ag, ab, 255);
		}
		lines[n++] = {szAmmo, ar, ag, ab}; // bottom

		const int mode = (m_iFireMode >= 0 && m_iFireMode < s_numFireModes) ? m_iFireMode : 1;
		lines[n++] = {s_fireModeNames[mode], dr, dg, db};

		const int cal = s_weaponCaliber[m_iWeaponType];
		if (cal > 0 && cal < s_numCalibers && s_caliberNames[cal][0])
			lines[n++] = {s_caliberNames[cal], dr, dg, db};
	}
	lines[n++] = {szName, hr, hg, hb}; // top (full brightness)

	int y = ScreenHeight - 8;
	for (int i = 0; i < n; i++)
	{
		y -= LINEH;
		TSHud_Panel(x, y, BOXW, LINEH - 1);
		TSHud_DrawString(x + 2, y, x + BOXW, lines[i].s, lines[i].r, lines[i].g, lines[i].b);
	}

	// Health bar stacked above the box — the original draws FUN_1004d5f0 here, after the
	// weapon/ammo lines, at the running y (reference/HUD_DRAWCHAIN.md).
	y -= LINEH;
	TSHud_HealthBar(x, y, g_iTSHealth);

	return true;
}
