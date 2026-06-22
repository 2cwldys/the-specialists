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
// ts_weaponbar.cpp
//
// The Specialists top weapon-select bar. Reads "WStatus" ({type, owned, clip}) to
// track the held arsenal, and draws the held weapons in five category columns. The
// column for each weapon = its s_weapon_info +0x84 byte (recovered from the .so
// Write_<weapon> functions): 1=melee/grenade, 2=pistols, 3=SMGs, 4=rifles/shotguns/
// heavy, 5=akimbo. Geometry from FUN_1004e030 (slot panel x = slot*0x80 - 0x78, y = 8,
// 0x80 wide, 0x14 tall). See reference/HUD_DRAWCHAIN.md.
//

#include "hud.h"
#include "cl_util.h"
#include "parsemsg.h"
#include "ts_hud.h"

#include <stdio.h>

DECLARE_MESSAGE(m_TSWeaponBar, WStatus)

// Per-tsweapon_t select-bar column (1..5), recovered from each weapon's s_weapon_info
// +0x84 byte. Index = tsweapon_t.
static const int s_weaponColumn[] = {
	/* 0 melee     */ 1, /* 1 glock18   */ 2, /* 2 beretta   */ 2, /* 3 uzi       */ 3,
	/* 4 benellim3 */ 4, /* 5 m4        */ 4, /* 6 mp5sd     */ 3, /* 7 mp5k      */ 3,
	/* 8 beretta_ak*/ 5, /* 9 mk23      */ 2, /*10 mk23_ak   */ 5, /*11 usas12    */ 4,
	/*12 desert    */ 2, /*13 ak47      */ 4, /*14 fnh       */ 2, /*15 aug       */ 4,
	/*16 uzi_ak    */ 5, /*17 skorpion  */ 3, /*18 barrett   */ 4, /*19 pdw       */ 3,
	/*20 spas12    */ 4, /*21 gold      */ 5, /*22 glock22   */ 2, /*23 mac10/ump */ 3,
	/*24 m61       */ 1, /*25 knife     */ 1, /*26 mossberg  */ 4, /*27 m16       */ 4,
	/*28 ruger     */ 2, /*29 flag      */ 1, /*30 fnh_ak    */ 5, /*31 bull      */ 2,
	/*32 m60       */ 4, /*33 sawedoff  */ 4, /*34 katana    */ 1, /*35 sealknife */ 1,
	/*36 contender */ 2, /*37 skorpion_ak*/5,
};

// Short display names for the bar (compact forms of the ammo HUD's names).
static const char* const s_shortName[] = {
	"Kung-Fu", "Glock-18", "Beretta", "Mini-Uzi", "Benelli M3", "M4", "MP5SD", "MP5K",
	"Akimbo Beretta", "MK23", "Akimbo MK23", "USAS-12", "Desert Eagle", "AK-47",
	"Five-seveN", "AUG", "Akimbo Uzi", "Skorpion", "Barrett", "PDW", "SPAS-12",
	"Golden Colts", "Glock-20C", "UMP", "M61", "Knife", "Mossberg", "M16", "Ruger",
	"", "Akimbo 5-7", "Raging Bull", "M60", "Sawed-off", "Katana", "SEAL Knife",
	"Contender", "Akimbo Skorpion",
};
// Per-tsweapon_t icon sprite base name → sprites/weapons/<name>.spr (the files the
// original loads; recovered from the client.dll string table). "" = no icon.
static const char* const s_spriteName[] = {
	"kungfu", "glock18", "beretta", "uzi", "m3", "m4", "mp5sd", "mp5k", "berettas",
	"mk23", "mk23_akimbo", "usas12", "desert", "ak47", "fnh", "aug", "uzi_akimbo",
	"skorpion", "m82", "pdw", "spas12", "gold", "glock22", "ump", "m61", "knife",
	"mossberg", "m16", "ruger", "", "fnh_akimbo", "bull", "m60", "sawedoff", "katana",
	"sealknife", "contender", "skorpion_akimbo",
};
static const int TSW_COUNT_LOCAL = sizeof(s_weaponColumn) / sizeof(s_weaponColumn[0]);

bool CHudTSWeaponBar::Init()
{
	for (int i = 0; i < TSW_COUNT; i++)
		m_iClip[i] = -1;

	HOOK_MESSAGE(WStatus);
	TSHud_RegisterCvars();
	gHUD.AddHudElem(this);
	return true;
}

bool CHudTSWeaponBar::VidInit()
{
	// Load each weapon's HUD icon (the original loads sprites/weapons/<name>.spr).
	char path[64];
	for (int t = 0; t < TSW_COUNT_LOCAL && t < TSW_COUNT; t++)
	{
		m_hIcon[t] = 0;
		if (!s_spriteName[t][0])
			continue;
		sprintf(path, "sprites/weapons/%s.spr", s_spriteName[t]);
		m_hIcon[t] = SPR_Load(path);
		if (m_hIcon[t] != 0)
		{
			m_rcIcon[t].left = 0;
			m_rcIcon[t].top = 0;
			m_rcIcon[t].right = SPR_Width(m_hIcon[t], 0);
			m_rcIcon[t].bottom = SPR_Height(m_hIcon[t], 0);
		}
	}
	return true;
}

void CHudTSWeaponBar::Reset()
{
	for (int i = 0; i < TSW_COUNT; i++)
		m_iClip[i] = -1;
}

bool CHudTSWeaponBar::MsgFunc_WStatus(const char* pszName, int iSize, void* pbuf)
{
	BEGIN_READ(pbuf, iSize);
	const int type = READ_BYTE();
	const int owned = READ_BYTE();
	const int clip = READ_BYTE();
	if (type >= 0 && type < TSW_COUNT)
	{
		m_iClip[type] = owned ? clip : -1;
		if (owned)
			m_iFlags |= HUD_ACTIVE;
	}
	return true;
}

bool CHudTSWeaponBar::Draw(float flTime)
{
	if ((m_iFlags & HUD_ACTIVE) == 0)
		return true;
	if (gHUD.m_iHideHUDDisplay & (HIDEHUD_WEAPONS | HIDEHUD_ALL))
		return true;

	int hr, hg, hb;
	TSHud_Color(hr, hg, hb);

	// Geometry recovered from FUN_1004e030: five 0x80-wide columns at x = col*0x80-0x78.
	// Each column has a header panel (y=8, h=0x14) carrying the slot NUMBER, then its
	// weapons stacked below from y=0x1c (28) at 0x30 (48) per row. The active column's
	// header is alpha 100, the rest alpha 50. A weapon with no ammo draws red.
	const int SLOTW = 0x80;   // 128
	const int HDRY = 8, HDRH = 0x14;
	const int ROW0 = 0x1c, ROWH = 0x30; // first weapon row y, row stride

	const int activeCol = (g_iTSActiveWeapon >= 0 && g_iTSActiveWeapon < TSW_COUNT_LOCAL)
		? s_weaponColumn[g_iTSActiveWeapon]
		: -1;

	for (int col = 1; col <= 5; col++)
	{
		const int x = col * SLOTW - 0x78;

		// Column header: faint panel + the slot number (always shown, like the original).
		gEngfuncs.pfnFillRGBA(x, HDRY, SLOTW, HDRH, hr, hg, hb, (col == activeCol) ? 100 : 50);
		char num[2] = {(char)('0' + col), '\0'};
		int nw, nh;
		GetConsoleStringSize(num, &nw, &nh);
		gHUD.DrawHudString(x + (SLOTW - nw) / 2, HDRY + 2, x + SLOTW, num, hr, hg, hb);

		// Weapons in this column, stacked below the header.
		int y = ROW0;
		for (int t = 0; t < TSW_COUNT_LOCAL && t < TSW_COUNT; t++)
		{
			if (m_iClip[t] < 0 || s_weaponColumn[t] != col)
				continue;

			const bool active = (t == g_iTSActiveWeapon);
			// no-ammo weapons draw red (0xc3,0x2d,0x2d) — the original's empty-clip tint.
			const bool noAmmo = (m_iClip[t] == 0);
			const int ir = noAmmo ? 0xc3 : hr;
			const int ig = noAmmo ? 0x2d : hg;
			const int ib = noAmmo ? 0x2d : hb;

			if (m_hIcon[t] != 0)
			{
				SPR_Set(m_hIcon[t], ir, ig, ib);
				SPR_DrawAdditive(0, x, y, &m_rcIcon[t]);
			}
			else if (s_shortName[t][0])
			{
				TSHud_DrawString(x + 2, y, x + SLOTW, s_shortName[t], ir, ig, ib);
			}

			// The active weapon: its name + clip, drawn to the right of its column.
			if (active)
			{
				const char* nm = (t < TSW_COUNT_LOCAL && s_shortName[t][0]) ? s_shortName[t] : "";
				gHUD.DrawHudString(x + SLOTW + 4, y, ScreenWidth, nm, hr, hg, hb);
				if (m_iClip[t] >= 0)
				{
					char ammo[16];
					sprintf(ammo, "%d", m_iClip[t]);
					gHUD.DrawHudString(x + SLOTW + 4, y + nh + 2, ScreenWidth, ammo, hr, hg, hb);
				}
			}
			y += ROWH;
		}
	}

	return true;
}
