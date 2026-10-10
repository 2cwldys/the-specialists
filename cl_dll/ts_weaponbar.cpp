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
#include "demo_api.h"
#include "../dlls/ts_weapon_info.h" // get_weapon_info: names, clip sizes, carry caps

#include <stdio.h>

DECLARE_MESSAGE(m_TSWeaponBar, WStatus)
DECLARE_MESSAGE(m_TSWeaponBar, ClipInfo)
DECLARE_MESSAGE(m_TSWeaponBar, Wires)

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

// Per-tsweapon_t select-grid row (0..16), each weapon's s_weapon_info +0x85 byte, read from every
// Write_<weapon> in ts_i386.so (`movb $row,0x85(...)`). client.dll keeps the owned weapons in a 17 x 11
// grid at this+0x164 (cell (row * 11 + column) * 8: the type, -1 when empty; WStatus 0x1004f990 and
// WeaponInfo 0x1004f800 fill it), and the slot keys walk a column of it by row.
static const int s_weaponRow[] = {
	/* 0 melee     */ 6, /* 1 glock18   */ 1, /* 2 beretta   */ 9, /* 3 uzi       */ 1,
	/* 4 benellim3 */ 1, /* 5 m4        */ 2, /* 6 mp5sd     */ 2, /* 7 mp5k      */ 3,
	/* 8 beretta_ak*/ 1, /* 9 mk23      */ 2, /*10 mk23_ak   */ 2, /*11 usas12    */ 11,
	/*12 desert    */ 3, /*13 ak47      */ 3, /*14 fnh       */ 4, /*15 aug       */ 4,
	/*16 uzi_ak    */ 3, /*17 skorpion  */ 4, /*18 barrett   */ 5, /*19 pdw       */ 5,
	/*20 spas12    */ 6, /*21 gold      */ 4, /*22 glock22   */ 5, /*23 mac10/ump */ 6,
	/*24 m61       */ 1, /*25 knife     */ 3, /*26 mossberg  */ 7, /*27 m16       */ 8,
	/*28 ruger     */ 6, /*29 flag      */ 5, /*30 fnh_ak    */ 5, /*31 bull      */ 7,
	/*32 m60       */ 9, /*33 sawedoff  */ 10, /*34 katana    */ 2, /*35 sealknife */ 4,
	/*36 contender */ 8, /*37 skorpion_ak*/6,
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

// "Only guns have the crosshair" — the reticle's aiming crosshair (donut sight + NSEW ticks)
// is suppressed for melee/grenade/flag (weapon column 1); guns are column >= 2 (pistols/SMGs/
// rifles/akimbo). Reads the active weaponType tracked by the WeaponInfo message.
bool TS_ActiveWeaponIsGun()
{
	const int t = g_iTSActiveWeapon;
	if (t < 0 || t >= TSW_COUNT_LOCAL)
		return false; // unarmed / unknown -> no crosshair
	return s_weaponColumn[t] >= 2;
}

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
	HOOK_MESSAGE(ClipInfo); // 1 byte: the ACTIVE weapon's clip cell (original client hooks it)
	HOOK_MESSAGE(Wires); // 3 bytes: the nearby-bomb wire ROLE codes (0 = away from any bomb)
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
	// same for +attack2 after a right-click drop (no stray secondary fire once the menu closes).
	if (m_swallowAttack2)
	{
		if ((gHUD.m_iKeyBits & IN_ATTACK2) != 0)
			gHUD.m_iKeyBits &= ~IN_ATTACK2;
		else
			m_swallowAttack2 = false;
	}

	if (!m_selecting)
		return;

	if ((gHUD.m_iKeyBits & IN_ATTACK) != 0)
	{
		Commit();
		gHUD.m_iKeyBits &= ~IN_ATTACK;
		m_swallowAttack = true;
	}
	else if ((gHUD.m_iKeyBits & IN_ATTACK2) != 0)
	{
		// RIGHT-CLICK during select = DROP the highlighted weapon (original client.dll sends
		// "drop_<type>", mirroring the left-click "weapon_<type>"). Then close the menu and swallow
		// the held +attack2 so it doesn't leak into secondary fire on the weapon we keep.
		if (m_selType >= 0)
		{
			char cmd[32];
			sprintf(cmd, "drop_%d\n", m_selType);
			ServerCmd(cmd);
		}
		m_selecting = false;
		gHUD.m_iKeyBits &= ~IN_ATTACK2;
		m_swallowAttack2 = true;
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
	}
	return true;
}

void CHudTSWeaponBar::Reset()
{
	for (int i = 0; i < TSW_COUNT; i++)
		m_iClip[i] = -1;
	// client.dll Reset (@0x1004cd60-0x1004cdc7): kung fu goes in its own cell, clip 1. The server
	// never sends a WStatus for it (its kung fu slot is not owned).
	m_iClip[0] = 1;
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

void CHudTSWeaponBar::SetClip(int type, int clip, bool bOwn)
{
	if (type < 0 || type >= TSW_COUNT)
		return;
	if (bOwn || m_iClip[type] >= 0)
		m_iClip[type] = clip;
}

// "Wires" (3 bytes) — the nearby armed bomb's wire ROLE codes (0 cut / 1 defuse / 2 boobytrap /
// 3 intact; all-zero = walked away). Stored at the original's provenance (+0x36fc..fe on the
// weapon-bar object); the wires-UI crosshair swap (FUN_1004d550) is a VERIFY draw tail.
bool CHudTSWeaponBar::MsgFunc_Wires(const char* pszName, int iSize, void* pbuf)
{
	BEGIN_READ(pbuf, iSize);
	m_wires[0] = (unsigned char)READ_BYTE();
	m_wires[1] = (unsigned char)READ_BYTE();
	m_wires[2] = (unsigned char)READ_BYTE();
	m_iFlags |= HUD_ACTIVE;
	return true;
}

// "ClipInfo" (1 byte) — the ACTIVE weapon's clip cell in the select grid. The original client
// hooks it (registration parity); its server registers-but-never-sends it, so this only fires
// against a server that actually uses it. Ours keeps the grid fresh through WStatus.
bool CHudTSWeaponBar::MsgFunc_ClipInfo(const char* pszName, int iSize, void* pbuf)
{
	BEGIN_READ(pbuf, iSize);
	const int clip = READ_BYTE();
	if (g_iTSActiveWeapon >= 0 && g_iTSActiveWeapon < TSW_COUNT)
	{
		m_iClip[g_iTSActiveWeapon] = clip;
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

// The grid cell (row, column): the owned weapon there, or -1 (client.dll this+0x164).
int CHudTSWeaponBar::CellType(int row, int col) const
{
	if (row < 0 || row > 16 || col < 0 || col > 10)
		return -1;
	for (int t = 0; t < TSW_COUNT_LOCAL && t < TSW_COUNT; t++)
	{
		if (m_iClip[t] >= 0 && s_weaponRow[t] == row && s_weaponColumn[t] == col)
			return t;
	}
	return -1;
}

// Point the cursor (this+0x740 row, +0x744 column) at weapon t's cell.
void CHudTSWeaponBar::SetCursor(int t)
{
	m_selType = t;
	if (t >= 0 && t < TSW_COUNT_LOCAL)
	{
		m_selRow = s_weaponRow[t];
		m_selCol = s_weaponColumn[t];
	}
}

// Open the select menu, seeding the cursor from the active weapon (FUN_1004f500 this+0x73c/0x740).
void CHudTSWeaponBar::OpenSelect()
{
	if (!m_selecting)
	{
		m_selecting = true;
		SetCursor(g_iTSActiveWeapon);
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
		SetCursor(t);
}

// slot1..slot10 once no text menu took the key (client.dll 0x1004f220; the menu test is in ammo.cpp).
// The buy menu, when up, gets the slot at its current level. Then, alive and not spectating:
// - near a bomb (any Wires byte set), slots 1..3 send "cutwire_<slot - 1>";
// - slot 10 sends "weapon_0" (kung fu) and puts the cursor at (0, 0) unless the select is open, and
//   closes it;
// - slots 1..5 with hud_fastswitch 0 open the select at that column: the same column again moves
//   the cursor down to the next owned row (rows 1..16, wrapping to 1), a new column starts from its
//   first owned row (row 1 when it has none); FIRE then commits (Think);
// - slots 1..5 with hud_fastswitch on send "weapon_<type>" for the column's next owned row after the
//   cursor's (rows 0..16, wrapping), at once, and close the select; the cursor's column stays;
// - slots 6..9 do nothing.
void CHudTSWeaponBar::SelectSlot(int slot)
{
	if (gHUD.m_TSBuyMenu.IsUp())
	{
		gHUD.m_TSBuyMenu.HandleSlot(slot);
		return;
	}
	if (gHUD.m_fPlayerDead || gHUD.m_Spectate.IsActive())
		return;

	char cmd[32];
	if (m_wires[0] != 0 || m_wires[1] != 0 || m_wires[2] != 0)
	{
		if (slot > 0 && slot < 4)
		{
			sprintf(cmd, "cutwire_%i", slot - 1);
			ServerCmd(cmd);
		}
		return;
	}

	if (slot == 10)
	{
		if (!m_selecting)
		{
			ServerCmd("weapon_0");
			m_selRow = 0;
			m_selCol = 0;
			m_selType = CellType(0, 0);
		}
		m_selecting = false;
		return;
	}
	if (slot >= 6)
		return;

	if (CVAR_GET_FLOAT("hud_fastswitch") == 0.0f)
	{
		if (!m_selecting)
			m_selecting = true;
		m_selTime = gHUD.m_flTime;
		if (slot == m_selCol)
		{
			bool found = false;
			int row = m_selRow + 1;
			if (row > 16)
				row = 1;
			for (; row < 17 && !found; row++)
			{
				if (CellType(row, m_selCol) != -1)
				{
					m_selRow = row;
					found = true;
				}
			}
			if (!found)
			{
				m_selRow = 1;
				if (CellType(1, slot) == -1)
				{
					while (m_selRow < 17)
					{
						m_selRow++;
						if (CellType(m_selRow, slot) != -1)
							break;
					}
				}
			}
		}
		else
		{
			m_selRow = 0;
			if (CellType(0, slot) == -1)
			{
				bool found = false;
				while (!found && ++m_selRow <= 16)
					found = CellType(m_selRow, slot) != -1;
				if (!found)
					m_selRow = 1;
			}
		}
		m_selCol = slot;
		m_selType = CellType(m_selRow, m_selCol);
		return;
	}

	int row = m_selRow + 1;
	if (row > 16)
		row = 0;
	m_selRow = row;
	do
	{
		if (CellType(row, slot) != -1)
			break;
		if (++row > 16)
			row = 0;
	} while (m_selRow != row);
	m_selRow = row;
	const int t = CellType(row, slot);
	if (t >= 0)
	{
		sprintf(cmd, "weapon_%i", t);
		ServerCmd(cmd);
	}
	m_selType = t;
	m_selecting = false;
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
		// (no weapon-switch sound — the original plays none on commit; this was fabricated)
	}
	CloseSelect();
}

void CHudTSWeaponBar::CloseSelect()
{
	m_selecting = false;
}

bool CHudTSWeaponBar::Draw(float flTime)
{
	// client.dll's weapon bar Draw (0x1004d3f0) draws the bar after the laser dot, only while nohud is 0
	// and the player is neither dead for the HUD nor spectating; with the select open (and no demo
	// playing) in full, else passively.
	if (gHUD.m_iHideHUDDisplay & (HIDEHUD_WEAPONS | HIDEHUD_ALL))
		return true;
	if (TSHud_NoHud() || TSHud_PlayerPartsHidden())
		return true;
	DrawBar(!m_selecting || 0 != gEngfuncs.pDemoAPI->IsPlayingback());
	return true;
}

// client.dll 0x1004e030. Passively the bar draws only for cl_showequipment 1, 2 or 3, every colour
// scaled by 0.25, 0.5 or 1. Five 128 wide columns at x = column * 128 - 120. With the select open each
// column has a 128 x 20 header at y 8 (alpha 100 on the cursor's column, 50 elsewhere) and its number
// two digits wide, in the HUD colour scaled by 200/255 on the cursor's column and 100/255 elsewhere.
// Below, from y 28 in steps of 48, the owned weapons in grid row order: the icon in the HUD colour,
// (195, 45, 45) with an empty clip, scaled by 255/255 for the cursor's cell and 100/255 for the rest;
// right of it a 5 wide gauge of the clip against the clip size (the carry cap for the knives and the
// M61), alpha 100 filled and 50 empty, none for kung fu, the katana and the flag. The cursor's cell
// also shows the weapon's name and its count ("%d" for the knives and the M61, "%d/%d" with the clip
// size for the guns) to the right of the column. Passively nothing marks a cursor and kung fu is left
// out.
void CHudTSWeaponBar::DrawBar(bool passive)
{
	float scale = 1.0f;
	if (passive)
	{
		static cvar_t* s_showequip = nullptr;
		if (s_showequip == nullptr)
			s_showequip = gEngfuncs.pfnGetCvarPointer("cl_showequipment");
		switch (s_showequip != nullptr ? (int)s_showequip->value : 1)
		{
		case 1: scale = 0.25f; break;
		case 2: scale = 0.5f; break;
		case 3: scale = 1.0f; break;
		default: return;
		}
	}
	auto scaled = [](int& r, int& g, int& b, int a) { // ScaleColors (0x10021c60): x a x (float)(1/255)
		const double k = (double)a * (double)(1.0f / 255.0f);
		r = (int)(r * k);
		g = (int)(g * k);
		b = (int)(b * k);
	};

	for (int col = 1; col <= 5; col++)
	{
		int r, g, b;
		TSHud_Color(r, g, b);
		const int x = col * 0x80 - 0x78;
		if (!passive)
		{
			const bool hlCol = col == m_selCol;
			FillRGBA(x, 8, 0x80, 0x14, (int)(r * scale), (int)(g * scale), (int)(b * scale), hlCol ? 100 : 50);
			scaled(r, g, b, hlCol ? 200 : 100);
			TSHud_DrawNumber(x, 8, TSHN_2DIGITS, col, (int)(r * scale), (int)(g * scale), (int)(b * scale));
		}

		int y = 0x1c;
		for (int row = 0; row < 17; row++)
		{
			const int t = CellType(row, col);
			if (t < 0)
				continue;
			bool hl = false;
			if (passive)
			{
				if (t == 0)
				{
					y += 0x30;
					continue;
				}
			}
			else if (row == m_selRow && col == m_selCol)
			{
				hl = true;
				const s_weapon_info* wi = get_weapon_info((tsweapon_t)t);
				TSHud_Color(r, g, b);
				scaled(r, g, b, 200);
				const int tr = (int)(r * scale), tg = (int)(g * scale), tb = (int)(b * scale);
				TSHud_DrawString(x + 0x80, y, ScreenWidth, wi->pszName, tr, tg, tb);
				char buf[32];
				buf[0] = '\0';
				if (t == 25 || t == 35 || t == 24) // knife, seal knife, M61
					sprintf(buf, "%d", m_iClip[t]);
				else if (t != 34 && t != 0 && t != 29) // not the katana, kung fu, the flag
					sprintf(buf, "%d/%d", m_iClip[t], wi->iClip);
				if (buf[0] != '\0')
					TSHud_DrawString(x + 0x80, y + 0x10, ScreenWidth, buf, tr, tg, tb);
			}

			if (m_iClip[t] <= 0)
			{
				r = 0xc3;
				g = 0x2d;
				b = 0x2d;
			}
			else
				TSHud_Color(r, g, b);
			scaled(r, g, b, hl ? 255 : 100);
			const int ir = (int)(r * scale), ig = (int)(g * scale), ib = (int)(b * scale);
			if (m_hIcon[t] != 0) // the flag has no icon (client.dll sets handle 0, which draws nothing)
			{
				SPR_Set(m_hIcon[t], ir, ig, ib);
				SPR_DrawAdditive(0, x, y, nullptr);
			}

			if (t != 34 && t != 0 && t != 29)
			{
				const s_weapon_info* wi = get_weapon_info((tsweapon_t)t);
				const float have = (float)m_iClip[t];
				const float full = (float)(wi->iStackable != 0 ? wi->iPellets : wi->iClip);
				int h = full != 0.0f ? (int)(have * 44.0f / full) : 0;
				if (h < 0)
					h = 0;
				if (full > 0.0f)
					FillRGBA(x + 0x7b, y - h + 0x2e, 5, h, ir, ig, ib, 100);
				if (have < full)
					FillRGBA(x + 0x7b, y + 2, 5, 0x2c - h, ir, ig, ib, 0x32);
			}
			y += 0x30;
		}
	}
}
