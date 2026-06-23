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

// Per-weapon clip capacity (max rounds per clip), indexed by tsweapon_t — the iClip column of the
// server weapon table (dlls/ts_weapons.cpp; the client has the same data in its own weapon table).
// Drawn as "curclip/maxclip" for the highlighted weapon. 0 = none (flag).
static const int s_weaponMaxClip[] = {
	12, 17, 15, 32, 7, 30, 30, 30, 30, 12, 24, 20, 7, 30, 20, 30, 64, 40, 5, 40,
	8, 16, 15, 25, 25, 18, 8, 18, 17, 0, 40, 5, 100, 2, 1, 1, 1, 40};

// Per-weapon akimbo variant (weapon_info +0x32), indexed by tsweapon_t; -1 = no akimbo. Used by the
// mouse-scroll cursor to SKIP a single weapon whose akimbo is owned (FUN_1004f500). RE'd from the
// per-weapon init data (reference/MENUS.md): Beretta->AkBerettas, Mini-Uzi->AkUzi, MK23->AkMK23,
// Five-seveN->AkFiveSeven, Skorpion->AkSkorpions.
static const int s_weaponAkimbo[] = {
	-1, -1, 8, 16, -1, -1, -1, -1, -1, 10, -1, -1, -1, -1, 30, -1, -1, 37, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1};

bool CHudTSWeaponBar::Init()
{
	for (int i = 0; i < TSW_COUNT; i++)
		m_iClip[i] = -1;

	HOOK_MESSAGE(WStatus);
	TSHud_RegisterCvars();
	gHUD.AddHudElem(this);
	m_iFlags |= HUD_ACTIVE; // always active so Think() runs (FIRE-commit); Draw() self-gates on m_selecting
	return true;
}

// FIRE-commits the highlighted weapon while the select menu is open (the original's hud_fastswitch-0
// model). Mirrors CHudAmmo::Think: read gHUD.m_iKeyBits & IN_ATTACK and CLEAR it so the shot is
// consumed by the selection instead of firing. Times out (cancels) after a short idle.
void CHudTSWeaponBar::Think()
{
	// after a commit, swallow the still-held +attack until released (no stray shot on the new weapon).
	if (m_swallowAttack)
	{
		if ((gHUD.m_iKeyBits & IN_ATTACK) != 0)
			gHUD.m_iKeyBits &= ~IN_ATTACK;
		else
			m_swallowAttack = false;
	}

	if (!m_selecting)
		return;

	if ((gHUD.m_iKeyBits & IN_ATTACK) != 0)
	{
		Commit();
		gHUD.m_iKeyBits &= ~IN_ATTACK;
		m_swallowAttack = true;
	}
	// No idle auto-close: the original closes the select only on commit (FIRE) or cancel
	// (key 0 / cancelselect) — there is no timer-based close (FUN_1004f220 / FUN_1004d2cf).
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

// Next/prev OWNED weapon relative to `from`, in the bar's display order (column-major: column
// 1..5 outer, tsweapon_t ascending within — the same order Draw() lays them out). colOnly>=1
// restricts to that HUD column (slot select); colOnly<0 = any. Returns the type, or -1 if none.
int CHudTSWeaponBar::FindOwned(int from, bool fNext, int colOnly) const
{
	int order[TSW_COUNT];
	int n = 0, cur = -1;
	for (int col = 1; col <= 5; col++)
	{
		if (colOnly >= 1 && col != colOnly)
			continue;
		for (int t = 0; t < TSW_COUNT_LOCAL && t < TSW_COUNT; t++)
		{
			if (m_iClip[t] < 0 || s_weaponColumn[t] != col)
				continue;
			// Mouse-scroll (colOnly<0) SKIPS kung-fu (type 0) and a single weapon whose akimbo variant
			// is owned — for smooth weapon cycling (FUN_1004f500: the `0 < type` gate + the akimbo-owned
			// skip). Both stay reachable via slot1-5 (colOnly>=1 does NOT skip). reference/WEAPON_SELECT.md.
			if (colOnly < 0)
			{
				if (t == 0)
					continue; // kung-fu — only via slot1
				int ak = (t < (int)(sizeof(s_weaponAkimbo) / sizeof(s_weaponAkimbo[0]))) ? s_weaponAkimbo[t] : -1;
				if (ak >= 0 && ak < TSW_COUNT && m_iClip[ak] >= 0)
					continue; // own the akimbo -> skip the single base
			}
			if (t == from)
				cur = n;
			order[n++] = t;
		}
	}
	if (n == 0)
		return -1;
	if (cur < 0)
		return order[fNext ? 0 : n - 1]; // cursor not in this set -> first/last
	int next = fNext ? (cur + 1) % n : (cur + n - 1) % n;
	return order[next];
}

// Open the select menu, seeding the cursor from the active weapon (FUN_1004f500 this+0x73c/0x740).
void CHudTSWeaponBar::OpenSelect()
{
	if (!m_selecting)
	{
		m_selecting = true;
		m_selType = g_iTSActiveWeapon;
	}
	m_selTime = gHUD.m_flTime;
}

// invnext/invprev: open the menu + move the cursor to the next/prev owned weapon (no commit yet —
// the original's hud_fastswitch-0 model; FIRE commits).
void CHudTSWeaponBar::Step(bool fNext)
{
	if (gHUD.m_fPlayerDead)
		return; // no weapon select while dead/spectating (the buy menu owns the screen)
	OpenSelect();
	int t = FindOwned(m_selType, fNext, -1);
	if (t >= 0)
		m_selType = t;
}

// slot1-5: open the menu + jump to HUD column `col` (or advance within it if already there).
void CHudTSWeaponBar::SelectColumn(int col)
{
	if (col < 1 || col > 5)
		return;
	if (gHUD.m_fPlayerDead)
		return; // no weapon select while dead/spectating
	OpenSelect();
	int from = (m_selType >= 0 && m_selType < TSW_COUNT_LOCAL && s_weaponColumn[m_selType] == col) ? m_selType : -1;
	int t = FindOwned(from, true, col);
	if (t >= 0)
		m_selType = t;
}

// FIRE: commit the highlighted weapon to the server via `weapon_<N>` (-> CTSGun holster->deploy
// with the draw anim, server side), then close. Re-selecting the active weapon just closes.
void CHudTSWeaponBar::Commit()
{
	if (m_selType >= 0 && m_selType != g_iTSActiveWeapon)
	{
		char cmd[24];
		sprintf(cmd, "weapon_%d\n", m_selType);
		ServerCmd(cmd);
		PlaySound("weapons/switch.wav", 1); // RE'd commit sound (FUN_1004d2cf, s_weapons_switch_wav)
	}
	CloseSelect();
}

void CHudTSWeaponBar::CloseSelect()
{
	m_selecting = false;
}

bool CHudTSWeaponBar::Draw(float flTime)
{
	// The bar is ALWAYS drawn (it's the in-game weapon HUD) but very transparent; opening the select
	// (invnext/invprev/slot1-5) brightens it, and the cursor weapon is the most opaque (FUN_1004e030
	// + the reference images). The highlight is OPACITY, not a box. Only drawn while alive.
	if (gHUD.m_fPlayerDead || gHUD.m_Spectate.IsActive())
		return true;
	if (gHUD.m_iHideHUDDisplay & (HIDEHUD_WEAPONS | HIDEHUD_ALL))
		return true;

	int hr, hg, hb;
	TSHud_Color(hr, hg, hb);

	// Geometry recovered from FUN_1004e030: five 0x80-wide columns at x = col*0x80-0x78.
	// Each column has a header panel (y=8, h=0x14) carrying the slot NUMBER, then its
	// weapons stacked below from y=0x1c (28) at 0x30 (48) per row.
	const int SLOTW = 0x80;   // 128
	const int HDRY = 8, HDRH = 0x14;
	const int ROW0 = 0x1c, ROWH = 0x30; // first weapon row y, row stride

	// The cursor (m_selType) column gets the bright header; FIRE will commit it.
	const int selCol = (m_selType >= 0 && m_selType < TSW_COUNT_LOCAL)
		? s_weaponColumn[m_selType]
		: -1;

	for (int col = 1; col <= 5; col++)
	{
		const int x = col * SLOTW - 0x78;

		// The column header bar + slot number only show while the select menu is open (the bright
		// header = the cursor's column, alpha 100 vs 50 — FUN_1004e030).
		if (m_selecting)
		{
			gEngfuncs.pfnFillRGBA(x, HDRY, SLOTW, HDRH, hr, hg, hb, (col == selCol) ? 100 : 50);
			char num[2] = {(char)('0' + col), '\0'};
			int nw, nh;
			GetConsoleStringSize(num, &nw, &nh);
			gHUD.DrawHudString(x + (SLOTW - nw) / 2, HDRY + 2, x + SLOTW, num, hr, hg, hb);
		}

		// Weapons in this column, stacked below the header.
		int y = ROW0;
		for (int t = 0; t < TSW_COUNT_LOCAL && t < TSW_COUNT; t++)
		{
			if (m_iClip[t] < 0 || s_weaponColumn[t] != col)
				continue;

			const bool cursor = m_selecting && (t == m_selType); // the highlighted weapon

			// Three opacity levels (the highlight is brightness, not a box): NOT selecting -> very
			// transparent; selecting non-cursor -> medium; cursor -> full. Additive draw, so scaling
			// the colour down = more transparent.
			int num = 1, den = 6; // not selecting -> ~1/6 (very faint)
			if (m_selecting)
			{
				num = 1;
				den = cursor ? 1 : 2; // cursor full, others ~half
			}
			// no-ammo weapons tint red (the original's empty-clip colour), scaled by the opacity.
			const bool noAmmo = (m_iClip[t] == 0);
			const int ir = (noAmmo ? 0xc3 : hr) * num / den;
			const int ig = (noAmmo ? 0x2d : hg) * num / den;
			const int ib = (noAmmo ? 0x2d : hb) * num / den;

			if (m_hIcon[t] != 0)
			{
				SPR_Set(m_hIcon[t], ir, ig, ib);
				SPR_DrawAdditive(0, x, y, &m_rcIcon[t]);
			}
			else if (s_shortName[t][0])
			{
				TSHud_DrawString(x + 2, y, x + SLOTW, s_shortName[t], ir, ig, ib);
			}

			// The cursor weapon (while selecting): its name + curclip/maxclip, at full opacity.
			if (cursor)
			{
				int nw, nh;
				GetConsoleStringSize("0", &nw, &nh);
				const char* nm = (t < TSW_COUNT_LOCAL && s_shortName[t][0]) ? s_shortName[t] : "";
				gHUD.DrawHudString(x + SLOTW + 4, y, ScreenWidth, nm, hr, hg, hb);
				if (m_iClip[t] >= 0)
				{
					// "curclip/maxclip" (e.g. 8/8) — current loaded rounds over the clip capacity.
					char ammo[16];
					int maxc = (t < (int)(sizeof(s_weaponMaxClip) / sizeof(s_weaponMaxClip[0]))) ? s_weaponMaxClip[t] : 0;
					sprintf(ammo, "%d/%d", m_iClip[t], maxc);
					gHUD.DrawHudString(x + SLOTW + 4, y + nh + 2, ScreenWidth, ammo, hr, hg, hb);
				}
			}
			y += ROWH;
		}
	}

	return true;
}
