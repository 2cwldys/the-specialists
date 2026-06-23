/***
*  ts_buymenu.cpp — The Specialists REAL buy/config menu (cont.69l).
*  See ts_buymenu.h + reference/MENUS.md "★★ THE REAL BUY MENU". A HUD-drawn cascade
*  (NOT vgui), a faithful port of client.dll's 0x10037xxx cluster:
*    - draw   : FUN_10036750 orchestrator + FUN_10036af0 header/preview + the column builders,
*               each entry via FUN_1004ff10 (-> TSHud_DrawString + the TS HUD font).
*    - input  : FUN_10038840 (number-key + mouse cascade navigation, via Dispatch()).
*    - mouse  : FUN_10038360 / FUN_10038410 (left=select / right=back / click-out=apply).
*    - queue  : FUN_10038f60 (+ cost FUN_10012ad0 / slot FUN_10038dc0 / budget 30000 / cap 81).
*    - apply  : FUN_10039400 -> rst_cfg + tkw (the swap contract, reference/BUY.md).
***/

#include "hud.h"
#include "cl_util.h"
#include "ts_hud.h"
#include "ts_buymenu.h"
#include "vgui_TeamFortressViewport.h" // GetNumberOfTeams() + UpdateCursorState() (cursor while up)

#include <stdio.h>
#include <string.h>

void IN_GetMousePos(int* mx, int* my);

static const int TS_BUDGET = 30000; // FUN_10036640 (CashBased off)
static const int TS_SLOTCAP = 0x51; // FUN_10036660 = 81

static const int s_attachCost[4] = {900, 800, 500, 1800};
static const char* const s_attachName[4] = {"Silencer", "Lasersight", "Flashlight", "Scope"};

#define TS_ACC_KEVLAR 0x10
#define TS_ACC_STEALTH 0x20

struct TSWeapon
{
	int id;
	const char* name;
	int price;
	int slot;
	int fullcnt;
	bool full;
	int support;
	int akimbo;
	int hudCol;        // FUN_10036af0 preview column 1..5 (s_weapon_info+0x84)
	const char* sprite; // sprites/weapons/<sprite>.spr (the bought-weapon icon)
};
// price/slot/fullcnt extracted from the init funcs; support/akimbo from the attachment RE;
// hudCol/sprite from the weapon-select bar (s_weaponColumn / s_spriteName). reference/MENUS.md.
static const TSWeapon s_weapons[] = {
	{1, "Glock-18", 850, 10, 1, false, 0x7, -1, 2, "glock18"},
	{2, "Beretta 92F", 900, 10, 1, false, 0x7, 8, 2, "beretta"},
	{3, "Mini-Uzi", 1300, 20, 1, false, 0x1, 16, 3, "uzi"},
	{4, "Benelli M3", 3000, 40, 8, false, 0xE, -1, 4, "m3"},
	{5, "M4A1", 4500, 40, 1, false, 0xF, -1, 4, "m4"},
	{6, "MP5SD", 2500, 25, 1, false, 0xE, -1, 3, "mp5sd"},
	{7, "MP5K", 1500, 20, 1, false, 0xF, -1, 3, "mp5k"},
	{8, "Akimbo Berettas", 900, 10, 1, false, 0x7, -1, 5, "berettas"},
	{9, "SOCOM-MK23", 700, 10, 1, false, 0x7, 10, 2, "mk23"},
	{10, "Akimbo MK23", 700, 10, 1, false, 0x7, -1, 5, "mk23_akimbo"},
	{11, "USAS-12", 5100, 40, 8, false, 0x6, -1, 4, "usas12"},
	{12, "Desert Eagle", 1050, 15, 1, false, 0xB, -1, 2, "desert"},
	{13, "AK47", 3000, 40, 1, false, 0x8, -1, 4, "ak47"},
	{14, "Five-seveN", 1250, 10, 1, false, 0x7, 30, 2, "fnh"},
	{15, "STEYR-AUG", 5000, 30, 1, false, 0x3, -1, 4, "aug"},
	{16, "Akimbo Mini-Uzi", 1300, 20, 1, false, 0x1, -1, 5, "uzi_akimbo"},
	{17, "Skorpion", 1100, 15, 1, false, 0x0, 37, 3, "skorpion"},
	{18, "Barrett M82A1", 9000, 70, 1, false, 0x2, -1, 4, "m82"},
	{19, "MP7-PDW", 2800, 25, 1, false, 0xB, -1, 3, "pdw"},
	{20, "SPAS-12", 2100, 30, 8, false, 0x6, -1, 4, "spas12"},
	{21, "Golden Colts", 2600, 20, 1, false, 0x0, -1, 5, "gold"},
	{22, "Glock-20C", 1300, 10, 1, false, 0x7, -1, 2, "glock22"},
	{23, "UMP", 2300, 30, 1, false, 0xF, -1, 3, "ump"},
	{24, "M61 Grenade", 1000, 7, 2, true, 0x0, -1, 1, "m61"},
	{25, "Combat Knife", 100, 1, 5, true, 0x0, -1, 1, "knife"},
	{26, "Mossberg 500", 2500, 40, 8, false, 0x6, -1, 4, "mossberg"},
	{27, "M16A4", 2500, 40, 1, false, 0xE, -1, 4, "m16"},
	{28, "Ruger-MK1", 1500, 5, 1, false, 0x2, -1, 2, "ruger"},
	{30, "Akimbo Five-seveN", 1250, 10, 1, false, 0x7, -1, 5, "fnh_akimbo"},
	{31, "Raging Bull", 2100, 15, 1, false, 0xA, -1, 2, "bull"},
	{32, "M60E3", 8000, 70, 1, false, 0x0, -1, 4, "m60"},
	{33, "Sawed-off", 1900, 20, 8, false, 0x2, -1, 4, "sawedoff"},
	{34, "Katana", 2900, 10, 0, false, 0x0, -1, 1, "katana"},
	{35, "Seal Knife", 100, 1, 10, true, 0x0, -1, 1, "sealknife"},
	{36, "Contender G2", 1500, 15, 1, false, 0xA, -1, 2, "contender"},
	{37, "Akimbo Skorpions", 1100, 15, 1, false, 0x0, -1, 5, "skorpion_akimbo"},
};
static const TSWeapon* WeaponInfo(int id)
{
	for (const auto& w : s_weapons)
		if (w.id == id)
			return &w;
	return nullptr;
}

// The REAL roster (obj+0x2d6c, FUN_10036200) — Beretta 92F (#5) + Contender G2 (#10) in Handguns.
#define TS_CATS 5
#define TS_ROWS 12
static const int s_roster[TS_CATS][TS_ROWS] = {
	{1, 9, 12, 14, 2, 21, 22, 28, 31, 36, -1, -1}, // 0 Handguns
	{3, 6, 7, 17, 19, 23, -1, -1, -1, -1, -1, -1}, // 1 SMGs
	{5, 13, 15, 27, 18, -1, -1, -1, -1, -1, -1, -1}, // 2 Rifles
	{4, 11, 20, 26, 33, -1, -1, -1, -1, -1, -1, -1}, // 3 Shotguns
	{24, 25, 32, 34, 35, -1, -1, -1, -1, -1, -1, -1}, // 4 Special Purpose
};
static const char* const s_catName[TS_CATS] = {"Handguns", "SMGs", "Rifles", "Shotguns", "Special Purpose"};

#define TS_X_CONFIG 8
#define TS_X_CATEGORY 0x88
#define TS_X_WEAPONS 0x108
#define TS_X_DETAIL 0x188
#define TS_ROWH 0x14
#define TS_COLW 0x80

static void ColDim(int& r, int& g, int& b) { TSHud_Color(r, g, b); r /= 2; g /= 2; b /= 2; }
#define COL_RED r = 0xc3, g = 0x2d, b = 0x2d
#define COL_GREEN r = 0x2d, g = 0xc3, b = 0x2d
#define COL_BLUE r = 100, g = 100, b = 0xcd

static int ScreenMidY() { return gHUD.m_scrinfo.iHeight / 2; }

//-----------------------------------------------------------------------------

bool CHudTSBuyMenu::Init()
{
	gHUD.AddHudElem(this);
	m_iFlags = 0;
	Reset();
	return true;
}

bool CHudTSBuyMenu::VidInit()
{
	// Load the bought-weapon preview icons (same sprites the weapon-select bar uses).
	char path[64];
	memset(m_hIcon, 0, sizeof(m_hIcon));
	for (const auto& w : s_weapons)
	{
		if (w.id < 0 || w.id >= (int)(sizeof(m_hIcon) / sizeof(m_hIcon[0])) || !w.sprite[0])
			continue;
		sprintf(path, "sprites/weapons/%s.spr", w.sprite);
		HSPRITE h = SPR_Load(path);
		if (h)
		{
			m_hIcon[w.id] = h;
			m_rcIcon[w.id].left = 0;
			m_rcIcon[w.id].top = 0;
			m_rcIcon[w.id].right = SPR_Width(h, 0);
			m_rcIcon[w.id].bottom = SPR_Height(h, 0);
		}
	}
	return true;
}

void CHudTSBuyMenu::Reset()
{
	m_active = false;
	m_level = LVL_CONFIG;
	m_cat = 0;
	m_attachMask = 0;
	m_weapon = -1;
	m_spent = 0;
	m_slotsUsed = 0;
	m_useNewConfig = true;
	m_prevButtons = 0;
	memset(m_slots, 0, sizeof(m_slots));
	m_iFlags &= ~HUD_ACTIVE;
}

void CHudTSBuyMenu::Open()
{
	m_active = true;
	m_level = LVL_CONFIG;
	m_weapon = -1;
	m_attachMask = 0;
	m_prevButtons = 0;
	m_iFlags |= HUD_ACTIVE;
	if (gViewPort)
		gViewPort->UpdateCursorState(); // free + show the cursor for clicking
}

void CHudTSBuyMenu::Close()
{
	m_active = false;
	m_level = LVL_CONFIG;
	m_weapon = -1;
	m_attachMask = 0;
	m_iFlags &= ~HUD_ACTIVE;
	if (gViewPort)
		gViewPort->UpdateCursorState();
}

//----------------------------------- accounting -----------------------------------

int CHudTSBuyMenu::TeamCount() const { return gViewPort ? gViewPort->GetNumberOfTeams() : 0; }

bool CHudTSBuyMenu::Restricted(int id) const { return id == 24 && TeamCount() < 2; }

bool CHudTSBuyMenu::AttachAvailable(int id, int bit) const
{
	if (m_attachMask & bit)
		return false;
	const TSWeapon* w = WeaponInfo(id);
	return w && (w->support & bit) != 0;
}

int CHudTSBuyMenu::WeaponCost(int id, int attachMask, bool akimboOrFull) const
{
	const TSWeapon* w = WeaponInfo(id);
	if (!w)
		return 0;
	if (w->full)
		return w->price * (akimboOrFull ? w->fullcnt : 1);
	int cost = w->price * (akimboOrFull ? 2 : 1);
	for (int bit = 0; bit < 4; bit++)
		if (attachMask & (1 << bit))
			cost += s_attachCost[bit];
	return cost;
}

int CHudTSBuyMenu::WeaponSlotCost(int id, bool akimboOrFull) const
{
	const TSWeapon* w = WeaponInfo(id);
	if (!w)
		return 0;
	if (!akimboOrFull)
		return w->slot;
	return w->full ? w->slot * w->fullcnt : w->slot * 2;
}

void CHudTSBuyMenu::Recount()
{
	m_spent = 0;
	m_slotsUsed = 0;
	for (const auto& s : m_slots)
	{
		if (!s.used)
			continue;
		m_spent += s.cost;
		if (s.accessory)
			m_slotsUsed += (s.weaponId == TS_ACC_KEVLAR) ? 10 : 0;
		else
			m_slotsUsed += WeaponSlotCost(s.weaponId, s.akimbo || s.count > 1);
	}
}

bool CHudTSBuyMenu::QueueWeapon(int id, int attachMask, bool akimboOrFull, bool accessory)
{
	if (!accessory && Restricted(id))
		return false;

	const TSWeapon* w = accessory ? nullptr : WeaponInfo(id);
	bool isFull = w && w->full;
	if (!accessory && !isFull)
		akimboOrFull = (w && w->akimbo >= 0) ? akimboOrFull : false;

	int cost = accessory ? (id == TS_ACC_KEVLAR ? 1900 : 700) : WeaponCost(id, attachMask, akimboOrFull);
	if (m_spent + cost > TS_BUDGET)
		return false;
	int slotCost = accessory ? (id == TS_ACC_KEVLAR ? 10 : 0) : WeaponSlotCost(id, akimboOrFull);
	if (m_slotsUsed + slotCost > TS_SLOTCAP)
		return false;

	for (int i = 0; i < MAX_SLOTS; i++)
	{
		if (!m_slots[i].used || m_slots[i].weaponId != id || m_slots[i].accessory != accessory)
			continue;
		if (accessory)
			return false;
		if (isFull)
		{
			if (m_slots[i].count >= w->fullcnt)
				return false;
			m_slots[i].count++;
			m_slots[i].cost += w->price;
			Recount();
			return true;
		}
		RemoveSlot(i); // re-buy toggles off
		return true;
	}

	for (int i = 0; i < MAX_SLOTS; i++)
	{
		if (m_slots[i].used)
			continue;
		m_slots[i].used = true;
		m_slots[i].accessory = accessory;
		m_slots[i].weaponId = id;
		m_slots[i].attachMask = accessory ? 0 : attachMask;
		m_slots[i].akimbo = (!accessory && akimboOrFull && !isFull);
		m_slots[i].count = isFull ? (akimboOrFull ? w->fullcnt : 1) : 1;
		m_slots[i].cost = cost;
		Recount();
		m_useNewConfig = true;
		return true;
	}
	return false;
}

void CHudTSBuyMenu::RemoveSlot(int i)
{
	if (i < 0 || i >= MAX_SLOTS || !m_slots[i].used)
		return;
	m_slots[i] = Slot{};
	Recount();
	m_useNewConfig = true;
}

void CHudTSBuyMenu::ResetConfig()
{
	memset(m_slots, 0, sizeof(m_slots));
	m_spent = 0;
	m_slotsUsed = 0;
	m_useNewConfig = true;
}

void CHudTSBuyMenu::BuyRandom()
{
	ResetConfig();
	for (int cat = 0; cat < TS_CATS; cat++)
	{
		int ids[TS_ROWS], n = 0;
		for (int row = 0; row < TS_ROWS; row++)
		{
			int id = s_roster[cat][row];
			if (id < 0)
				break;
			if (!Restricted(id))
				ids[n++] = id;
		}
		if (n > 0)
			QueueWeapon(ids[gEngfuncs.pfnRandomLong(0, n - 1)], 0, false, false);
	}
}

void CHudTSBuyMenu::Apply()
{
	if (m_spent <= 0)
		return;

	gEngfuncs.pfnClientCmd("rst_cfg\n");

	char cmd[256] = "tkw";
	bool any = false, kevlar = false;
	for (const auto& s : m_slots)
	{
		if (!s.used)
			continue;
		if (s.accessory)
		{
			if (s.weaponId == TS_ACC_KEVLAR)
				kevlar = true;
			continue;
		}
		char buf[32];
		sprintf(buf, " %d_%d", s.weaponId, s.count < 1 ? 1 : s.count);
		strncat(cmd, buf, sizeof(cmd) - strlen(cmd) - 1);
		any = true;
	}
	if (any)
	{
		strncat(cmd, "\n", sizeof(cmd) - strlen(cmd) - 1);
		gEngfuncs.pfnClientCmd(cmd);
	}
	gEngfuncs.Cvar_SetValue("cl_buykevlar", kevlar ? 1.0f : 0.0f);

	m_useNewConfig = false;
	Close();
}

//----------------------------------- input -----------------------------------

bool CHudTSBuyMenu::HandleKey(int ascii)
{
	if (!m_active)
		return false;
	int key = (ascii == '0') ? 10 : (ascii - '0');
	if (key < 1 || key > 10)
		return false;
	Dispatch(m_level, key); // keyboard always acts on the current column
	return true;
}

// FUN_10038840 — run `key` at `level`. The mouse can target any visible column's level.
void CHudTSBuyMenu::Dispatch(int level, int key)
{
	if (key < 1 || key > 11)
		return;

	switch (level)
	{
	case LVL_CONFIG:
		if (key == 1) Apply();
		else if (key == 2) m_level = LVL_CATEGORY;
		else if (key == 3) m_level = LVL_REMOVE;
		else if (key == 4) ResetConfig();
		else if (key == 5) m_level = LVL_SAVE;
		else if (key == 6) m_level = LVL_LOAD;
		else if (key == 7) BuyRandom();
		else if (key == 10) Close();
		break;

	case LVL_CATEGORY:
		if (key == 6) m_level = LVL_ACCESSORY;
		else if (key == 10) m_level = LVL_CONFIG;
		else if (key >= 1 && key <= 5 && s_roster[key - 1][0] != -1)
		{
			m_cat = key - 1;
			m_level = LVL_WEAPONS;
		}
		break;

	case LVL_WEAPONS:
	{
		if (key == 10) { m_level = LVL_CATEGORY; break; }
		int row = (key >= 10) ? key - 2 : key - 1; // key 11 -> the 10th weapon (original quirk)
		if (row < 0 || row >= TS_ROWS)
			break;
		int id = s_roster[m_cat][row];
		if (id < 0 || Restricted(id))
			break;
		m_weapon = id;
		m_attachMask = 0;
		m_level = LVL_DETAIL;
		break;
	}

	case LVL_DETAIL:
		if (key == 1 || key == 6)
		{
			QueueWeapon(m_weapon, m_attachMask, key == 6, false);
			m_level = LVL_CATEGORY;
			m_weapon = -1;
			m_attachMask = 0;
		}
		else if (key >= 2 && key <= 5)
		{
			int bit = 1 << (key - 2);
			if (AttachAvailable(m_weapon, bit))
				m_attachMask |= bit;
		}
		else if (key == 10)
		{
			m_level = LVL_WEAPONS;
			m_weapon = -1;
			m_attachMask = 0;
		}
		break;

	case LVL_ACCESSORY:
		if (key == 1) { QueueWeapon(TS_ACC_KEVLAR, 0, false, true); m_level = LVL_CATEGORY; }
		else if (key == 2 && TeamCount() >= 2) { QueueWeapon(TS_ACC_STEALTH, 0, false, true); m_level = LVL_CATEGORY; }
		else if (key == 10) m_level = LVL_CATEGORY;
		break;

	case LVL_REMOVE:
	{
		if (key == 10) { Close(); break; }
		int seen = 0;
		for (int i = 0; i < MAX_SLOTS; i++)
		{
			if (!m_slots[i].used)
				continue;
			if (++seen == key)
			{
				RemoveSlot(i);
				break;
			}
		}
		break;
	}

	case LVL_SAVE:
		if (key == 10) { Close(); break; }
		if (key >= 1 && key <= SAVE_SLOTS)
		{
			memcpy(m_saved[key - 1], m_slots, sizeof(m_slots));
			m_savedFull[key - 1] = (m_spent > 0);
			m_level = LVL_CONFIG;
		}
		break;

	case LVL_LOAD:
		if (key == 10) { Close(); break; }
		if (key >= 1 && key <= SAVE_SLOTS && m_savedFull[key - 1])
		{
			memcpy(m_slots, m_saved[key - 1], sizeof(m_slots));
			Recount();
			m_useNewConfig = true;
			m_level = LVL_CONFIG;
		}
		break;

	case LVL_CHANGETEAM:
		if (key == 1 || key == 2) { char c[32]; sprintf(c, "chgteam %d\n", key - 1); gEngfuncs.pfnClientCmd(c); Close(); }
		else if (key == 10) Close();
		break;
	}
}

// FUN_10038360 — mouse: left-click an entry -> its key (miss -> key 1), right-click -> Go Back (10).
void CHudTSBuyMenu::HandleMouse(int buttons)
{
	if (!m_active)
		return;
	int released = (buttons ^ m_prevButtons) & ~buttons; // newly released
	m_prevButtons = buttons;

	if (released & 1) // left
	{
		int mx, my;
		IN_GetMousePos(&mx, &my);
		int level = -1, key = 0;
		MouseToEntry(mx, my, level, key);
		if (level == -1)
			Dispatch(m_level, 1); // click outside any entry = key 1 (apply / Buy on this level)
		else
			Dispatch(level, key);
	}
	else if (released & 2) // right
	{
		Dispatch(m_level, 10);
	}
}

// FUN_10038410 — hit-test a cursor pos to (column-level, on-screen key), skipping hidden entries.
void CHudTSBuyMenu::MouseToEntry(int mx, int my, int& level, int& key)
{
	level = -1;
	if (mx < 8 || my < ScreenMidY())
		return;
	int row = (my - ScreenMidY()) / TS_ROWH;
	int col = (mx - 8) / TS_COLW; // 0 config, 1 category, 2 weapons, 3 detail

	if (col == 0) // config column
	{
		if (row >= 0 && row <= 7) { level = LVL_CONFIG; key = (row == 7) ? 10 : row + 1; }
		return;
	}
	if (col == 1) // category column (or accessory/save/load share this geometry per level)
	{
		if (m_level == LVL_ACCESSORY || m_level == LVL_SAVE || m_level == LVL_LOAD || m_level == LVL_REMOVE)
		{
			level = m_level;
			key = row + 1; // the active sub-column owns column 1; keys are 1.. then 0 Go Back as last
			return;
		}
		int cats = 0;
		for (int c = 0; c < TS_CATS; c++)
			if (s_roster[c][0] != -1)
				cats++;
		if (row < cats) { level = LVL_CATEGORY; key = row + 1; }
		else if (row == cats) { level = LVL_CATEGORY; key = 6; }      // Player Accessory
		else if (row == cats + 1) { level = LVL_CATEGORY; key = 10; } // Go Back
		return;
	}
	if (col == 2 && (m_level == LVL_WEAPONS || m_level == LVL_DETAIL)) // weapons column
	{
		int shown = 0;
		for (int r = 0; r < TS_ROWS; r++)
		{
			int id = s_roster[m_cat][r];
			if (id < 0)
				break;
			if (Restricted(id))
				continue;
			if (shown == row) { level = LVL_WEAPONS; key = (r >= 9) ? 11 : r + 1; return; }
			shown++;
		}
		if (row == shown) { level = LVL_WEAPONS; key = 10; } // Go Back
		return;
	}
	if (col == 3 && m_level == LVL_DETAIL) // detail column
	{
		if (row == 0) { level = LVL_DETAIL; key = 1; return; } // Buy
		int line = 1;
		for (int bit = 0; bit < 4; bit++)
		{
			if (!AttachAvailable(m_weapon, bit))
				continue;
			if (row == line) { level = LVL_DETAIL; key = bit + 2; return; }
			line++;
		}
		const TSWeapon* w = WeaponInfo(m_weapon);
		if (w && (w->akimbo >= 0 || w->full))
		{
			if (row == line) { level = LVL_DETAIL; key = 6; return; }
			line++;
		}
		if (row == line) { level = LVL_DETAIL; key = 10; } // Go Back
		return;
	}
}

//----------------------------------- rendering -----------------------------------

void CHudTSBuyMenu::SetCursor(Cursor& c, int x, int y, int rowH, int width)
{
	c.x = x;
	c.y = y;
	c.rowH = rowH;
	c.width = width;
}

void CHudTSBuyMenu::DrawEntry(Cursor& c, const char* text, int r, int g, int b)
{
	FillRGBA(c.x, c.y, c.width - 1, c.rowH - 1, r, g, b, 0x32);
	TSHud_DrawString(c.x, c.y, ScreenWidth, text, r, g, b);
	c.y += c.rowH;
}

// FUN_10036af0 — the header bar (Total/FreeSlots) + the bought-loadout preview columns.
void CHudTSBuyMenu::DrawHeader()
{
	int dr, dg, db;
	ColDim(dr, dg, db);
	FillRGBA(8, 8, 0x280, 0x13, dr, dg, db, 0x32);

	char buf[64];
	int r, g, b;
	sprintf(buf, "(Total: %d)", m_spent);
	if (m_spent > TS_BUDGET) { COL_RED; } else { r = dr; g = dg; b = db; }
	TSHud_DrawString(8, 8, ScreenWidth, buf, r, g, b);

	// "Free Slots" shows the REMAINING slots (cap - used), red when overspent — confirmed by the
	// reference images (Glock slot 10 -> 81-10 = 71; FUN_10036af0).
	int freeSlots = TS_SLOTCAP - m_slotsUsed;
	sprintf(buf, "Free Slots : %d", freeSlots);
	if (freeSlots < 0) { COL_RED; } else { r = dr; g = dg; b = db; }
	TSHud_DrawString(0x108, 8, ScreenWidth, buf, r, g, b);

	DrawLoadoutPreview();
}

// FUN_10036af0's preview: the bought items drawn as icons in 5 columns (by HUD column 1..5), in the
// ORDER they were queued (confirmed by the reference images — Kevlar interleaves with the col-1
// melee items in add-order). Each item shows its count to the LEFT of the icon (Seal Knife 10 /
// Combat Knife 5 = full-load counts) and its attachments below as "(Sil ,Las ,Fla ,Sco )".
// NOTE: for ammo weapons the original shows the auto-bought AMMO count here (e.g. Glock 210); the
// per-caliber max that caps it resisted static RE (see reference/MENUS.md), so guns show no number
// yet — the auto-ammo (and its share of Total/the tka command) is the one remaining slice.
void CHudTSBuyMenu::DrawLoadoutPreview()
{
	int hr, hg, hb;
	TSHud_Color(hr, hg, hb);
	static const char* const ab[4] = {"Sil ", "Las ", "Fla ", "Sco "};

	for (int col = 1; col <= 5; col++)
	{
		const int x = col * TS_COLW - 0x78;
		// column header box + the slot number (1..5), centered like the weapon-select bar.
		FillRGBA(x, 0x1c, TS_COLW, TS_ROWH, hr / 2, hg / 2, hb / 2, 0x32);
		char num[2] = {(char)('0' + col), '\0'};
		int nw, nh;
		GetConsoleStringSize(num, &nw, &nh);
		gHUD.DrawHudString(x + (TS_COLW - nw) / 2, 0x1c + 2, x + TS_COLW, num, hr, hg, hb);

		int y = 0x1c + TS_ROWH + 4;
		for (const auto& s : m_slots)
		{
			if (!s.used)
				continue;
			if (s.accessory)
			{
				if (col == 1) // accessories live in column 1, in add-order with the melee items
				{
					const char* nm = (s.weaponId == TS_ACC_KEVLAR) ? "Kevlar" : "Stealth Shoes";
					TSHud_DrawString(x, y + 4, ScreenWidth, nm, hr, hg, hb);
					y += 0x30;
				}
				continue;
			}
			const TSWeapon* w = WeaponInfo(s.weaponId);
			if (!w || w->hudCol != col)
				continue;

			// count to the LEFT of the icon (only when >1 — full-load throwables; FUN_10036af0).
			int iconX = x;
			if (s.count > 1)
			{
				char cnt[8];
				sprintf(cnt, "%d", s.count);
				TSHud_DrawString(x, y + 4, ScreenWidth, cnt, hr, hg, hb);
				iconX = x + 0x18;
			}
			if (m_hIcon[s.weaponId])
			{
				SPR_Set(m_hIcon[s.weaponId], hr, hg, hb);
				SPR_DrawAdditive(0, iconX, y, &m_rcIcon[s.weaponId]);
			}
			else
			{
				TSHud_DrawString(iconX, y, ScreenWidth, w->name, hr, hg, hb);
			}
			// attachments under the icon, as "(Sil ,Las ,Fla ,Sco )".
			if (s.attachMask)
			{
				char at[40] = "(";
				for (int bit = 0; bit < 4; bit++)
					if (s.attachMask & (1 << bit))
						strncat(at, ab[bit], sizeof(at) - strlen(at) - 1);
				strncat(at, ")", sizeof(at) - strlen(at) - 1);
				TSHud_DrawString(x, y + 0x14, ScreenWidth, at, hr / 2, hg / 2, hb / 2);
			}
			y += 0x30;
		}
	}
}

void CHudTSBuyMenu::DrawColumnConfig()
{
	Cursor c;
	int dr, dg, db, r, g, b;
	ColDim(dr, dg, db);
	SetCursor(c, TS_X_CONFIG, ScreenMidY(), TS_ROWH, TS_COLW);

	if (m_spent < 1)
	{
		COL_RED;
		DrawEntry(c, "1 Empty Config", r, g, b);
	}
	else
	{
		const char* lbl = m_useNewConfig ? "1 Use New Config" : "1 Use Last Config";
		if (m_spent > TS_BUDGET) { COL_RED; } else { r = dr; g = dg; b = db; }
		DrawEntry(c, lbl, r, g, b);
	}
	DrawEntry(c, "2 Buy", dr, dg, db);
	DrawEntry(c, "3 Remove Item", dr, dg, db);
	DrawEntry(c, "4 Reset Config", dr, dg, db);
	DrawEntry(c, "5 Save Config", dr, dg, db);
	DrawEntry(c, "6 Load Config", dr, dg, db);
	COL_GREEN;
	DrawEntry(c, "7 Random", r, g, b);
	if (m_level == LVL_CONFIG)
	{
		COL_BLUE;
		DrawEntry(c, "0 Cancel", r, g, b);
	}
}

void CHudTSBuyMenu::DrawColumnCategory()
{
	Cursor c;
	int dr, dg, db, r, g, b;
	ColDim(dr, dg, db);
	SetCursor(c, TS_X_CATEGORY, ScreenMidY(), TS_ROWH, TS_COLW);

	char buf[48];
	for (int cat = 0; cat < TS_CATS; cat++)
	{
		if (s_roster[cat][0] == -1)
			break;
		sprintf(buf, "%d %s", cat + 1, s_catName[cat]);
		DrawEntry(c, buf, dr, dg, db);
	}
	DrawEntry(c, "6 Player Accessory", dr, dg, db);
	if (m_level == LVL_CATEGORY)
	{
		COL_BLUE;
		DrawEntry(c, "0 Go Back", r, g, b);
	}
	DrawColumnConfig();
}

void CHudTSBuyMenu::DrawColumnWeapons()
{
	Cursor c;
	int dr, dg, db, r, g, b;
	ColDim(dr, dg, db);
	SetCursor(c, TS_X_WEAPONS, ScreenMidY(), TS_ROWH, TS_COLW);

	char buf[48];
	int shown = 0;
	for (int row = 0; row < TS_ROWS && shown < 0xc; row++)
	{
		int id = s_roster[m_cat][row];
		if (id < 0)
			break;
		if (Restricted(id))
			continue;
		const TSWeapon* w = WeaponInfo(id);
		bool afford = w && (m_spent + WeaponCost(id, 0, false) <= TS_BUDGET) &&
					  (m_slotsUsed + WeaponSlotCost(id, false) <= TS_SLOTCAP);
		if (afford) { r = dr; g = dg; b = db; } else { COL_RED; }
		sprintf(buf, "%d %s", shown + 1, w ? w->name : "?");
		DrawEntry(c, buf, r, g, b);
		if (id == m_weapon)
			DrawWeaponPreview(id);
		shown++;
	}
	if (m_level == LVL_WEAPONS)
	{
		COL_BLUE;
		DrawEntry(c, "0 Go Back", r, g, b);
	}
	DrawColumnCategory();
}

void CHudTSBuyMenu::DrawColumnDetail()
{
	Cursor c;
	int dr, dg, db, r, g, b;
	ColDim(dr, dg, db);
	SetCursor(c, TS_X_DETAIL, ScreenMidY(), TS_ROWH, TS_COLW);

	const TSWeapon* w = WeaponInfo(m_weapon);
	int baseCost = WeaponCost(m_weapon, m_attachMask, false);
	bool afford = m_spent + baseCost <= TS_BUDGET;
	if (afford) { r = dr; g = dg; b = db; } else { COL_RED; }
	DrawEntry(c, "1 Buy", r, g, b);

	char buf[48];
	int key = 2;
	for (int bit = 0; bit < 4; bit++)
	{
		if (!AttachAvailable(m_weapon, bit))
			continue;
		int cost = m_spent + baseCost + s_attachCost[bit];
		if (cost <= TS_BUDGET) { r = dr; g = dg; b = db; } else { COL_RED; }
		sprintf(buf, "%d %s  %d", key, s_attachName[bit], s_attachCost[bit]);
		DrawEntry(c, buf, r, g, b);
		key++;
	}
	if (w && w->akimbo >= 0)
	{
		bool ak = m_spent + WeaponCost(m_weapon, m_attachMask, true) <= TS_BUDGET;
		if (ak) { r = dr; g = dg; b = db; } else { COL_RED; }
		DrawEntry(c, "6 Buy Akimbo", r, g, b);
	}
	else if (w && w->full)
	{
		bool fl = m_spent + WeaponCost(m_weapon, m_attachMask, true) <= TS_BUDGET;
		if (fl) { r = dr; g = dg; b = db; } else { COL_RED; }
		DrawEntry(c, "6 Full Load", r, g, b);
	}
	if (m_level == LVL_DETAIL)
	{
		COL_BLUE;
		DrawEntry(c, "0 Go Back", r, g, b);
	}
	DrawColumnWeapons();
}

// FUN_10037fe0 — accessory column (sits at the weapons X). Kevlar + (team) Stealth Shoes.
void CHudTSBuyMenu::DrawColumnAccessory()
{
	Cursor c;
	int dr, dg, db, r, g, b;
	ColDim(dr, dg, db);
	SetCursor(c, TS_X_WEAPONS, ScreenMidY(), TS_ROWH, TS_COLW);
	DrawEntry(c, "1 Kevlar  1900", dr, dg, db);
	if (TeamCount() >= 2)
		DrawEntry(c, "2 Stealth Shoes  700", dr, dg, db);
	COL_BLUE;
	DrawEntry(c, "0 Go Back", r, g, b);
	DrawColumnCategory();
}

// FUN_10037260 (save) / the load column. Lists the 9 config slots.
void CHudTSBuyMenu::DrawColumnSaveLoad(bool save)
{
	Cursor c;
	int dr, dg, db, r, g, b;
	ColDim(dr, dg, db);
	SetCursor(c, TS_X_CATEGORY, ScreenMidY(), TS_ROWH, TS_COLW);
	COL_GREEN;
	DrawEntry(c, save ? "Save Equipment" : "Load Equipment", r, g, b);
	char buf[48];
	for (int i = 0; i < SAVE_SLOTS; i++)
	{
		bool full = m_savedFull[i];
		sprintf(buf, "%d Slot #%d (%s)", i + 1, i + 1, full ? "Available" : "Empty");
		if (!save && !full) { COL_RED; } else { r = dr; g = dg; b = db; }
		DrawEntry(c, buf, r, g, b);
	}
	COL_BLUE;
	DrawEntry(c, "0 Go Back", r, g, b);
	DrawColumnConfig();
}

// the Remove-Item column: lists the queued slots.
void CHudTSBuyMenu::DrawColumnRemove()
{
	Cursor c;
	int dr, dg, db, r, g, b;
	ColDim(dr, dg, db);
	SetCursor(c, TS_X_CATEGORY, ScreenMidY(), TS_ROWH, TS_COLW);
	char buf[48];
	int key = 1;
	for (const auto& s : m_slots)
	{
		if (!s.used)
			continue;
		if (s.accessory)
			sprintf(buf, "%d %s", key, s.weaponId == TS_ACC_KEVLAR ? "Kevlar" : "Stealth Shoes");
		else
		{
			const TSWeapon* w = WeaponInfo(s.weaponId);
			sprintf(buf, "%d %s", key, w ? w->name : "?");
		}
		DrawEntry(c, buf, dr, dg, db);
		key++;
	}
	COL_BLUE;
	DrawEntry(c, "0 Go Back", r, g, b);
	DrawColumnConfig();
}

void CHudTSBuyMenu::DrawWeaponPreview(int id)
{
	const TSWeapon* w = WeaponInfo(id);
	if (!w)
		return;
	int r, g, b;
	TSHud_Color(r, g, b);
	int y = ScreenMidY() - 0x30;
	FillRGBA(0x108, y, 0xff, 0x2f, r, g, b, 0x19);

	char buf[32];
	sprintf(buf, "%d Credits", w->price);
	TSHud_DrawString(0x108 + 0x80, y, ScreenWidth, buf, r / 2, g / 2, b / 2);
	sprintf(buf, "%d Slots", w->slot);
	TSHud_DrawString(0x108 + 0x80, y + 0x14, ScreenWidth, buf, r / 2, g / 2, b / 2);
}

void CHudTSBuyMenu::DrawColumnChangeTeam()
{
	Cursor c;
	int r, g, b, dr, dg, db;
	ColDim(dr, dg, db);
	SetCursor(c, TS_X_CONFIG, ScreenMidY(), TS_ROWH, TS_COLW);
	COL_GREEN;
	DrawEntry(c, "Change Team:", r, g, b);
	DrawEntry(c, "1 Join Specialists", dr, dg, db);
	DrawEntry(c, "2 Join Mercenaries", dr, dg, db);
	COL_BLUE;
	DrawEntry(c, "0 Cancel", r, g, b);
}

// FUN_10036750 — orchestrator: header/preview + the active cascade (builders draw their parents).
bool CHudTSBuyMenu::Draw(float flTime)
{
	if (!m_active)
		return true;

	if (m_level == LVL_CHANGETEAM)
	{
		DrawColumnChangeTeam();
		return true;
	}

	DrawHeader();
	switch (m_level)
	{
	case LVL_CATEGORY: DrawColumnCategory(); break;
	case LVL_WEAPONS: DrawColumnWeapons(); break;
	case LVL_DETAIL: DrawColumnDetail(); break;
	case LVL_ACCESSORY: DrawColumnAccessory(); break;
	case LVL_REMOVE: DrawColumnRemove(); break;
	case LVL_SAVE: DrawColumnSaveLoad(true); break;
	case LVL_LOAD: DrawColumnSaveLoad(false); break;
	default: DrawColumnConfig(); break;
	}
	return true;
}
