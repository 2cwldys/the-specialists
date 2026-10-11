/***
*  ts_buymenu.cpp — The Specialists REAL buy/config menu (cont.69l).
*  See ts_buymenu.h + reference/MENUS.md "★★ THE REAL BUY MENU". A HUD-drawn cascade
*  (NOT vgui), a port of client.dll's buy menu functions at 0x10036000-0x10039fff:
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
#include "ts_weapon_info.h" // get_weapon_info — CustomWP live prices + tunables
#include "parsemsg.h"
#include "vgui_TeamFortressViewport.h" // GetNumberOfTeams() + UpdateCursorState() (cursor while up)

#include <stdio.h>
#include <string.h>

void IN_GetMousePos(int* mx, int* my);

static const int TS_BUDGET = 30000; // FUN_10036640 (CashBased off)
static const int TS_SLOTCAP = 0x51; // FUN_10036660 = 81

static const int s_attachCost[4] = {900, 800, 500, 1800};
// The detail column's attachment lines (0x10037cf3..0x10037d62): key, the titles.txt name, price.
static const char* const s_attachLine[4] = {"%i #HUD_Silencer  %i", "%i #HUD_Lasersight  %i",
	"%i #HUD_Flashlight  %i", "%i #HUD_Scope  %i"};

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

// Per-weapon ammo caliber (weapon_info byte +0x08), read from the live weapon table. 0 = melee/no
// ammo. reference/MENUS.md. Indexed by tsweapon_t id.
static const signed char s_weaponCaliber[38] = {
	/*0*/ 0, 1, 1, 1, 3, 4, 1, 1, 1, 2, 2, 3, 5, 6, 7, 4, 1, 12, 8, 7,
	/*20*/ 3, 2, 9, 2, 0, 0, 3, 4, 10, -1, 7, 11, 4, 3, 0, 0, 13, 12};
static int WeaponCaliber(int id) { return (id >= 0 && id < 38) ? s_weaponCaliber[id] : 0; }

// The per-caliber ammo table DAT_1009ffc0 (stride 0xe ints), RUNTIME-captured from the live game
// (the max field [3] resisted static RE). {cost per unit [1], max rounds [3], rounds per unit [4]}.
// FUN_10039380 fills a weapon's caliber pool 1 unit at a time up to max; FUN_100392b0 distributes the
// post-weapon budget across calibers. Calibers 0/14/15 = melee (no ammo).
struct TSCaliber { int cost, maxRounds, unit; };
static const TSCaliber s_caliber[16] = {
	{0, 0, 0},      // 0 (melee)
	{50, 210, 30},  // 1  9mm
	{60, 175, 12},  // 2  .45
	{120, 60, 8},   // 3  12ga
	{80, 120, 30},  // 4  5.56
	{70, 70, 10},   // 5  .50AE
	{100, 90, 30},  // 6  7.62
	{120, 160, 20}, // 7  5.7
	{500, 30, 5},   // 8  .50BMG
	{70, 90, 15},   // 9  10mm
	{100, 150, 17}, // 10 .22
	{150, 50, 5},   // 11 .454
	{30, 300, 20},  // 12 .32
	{150, 35, 5},   // 13 .45-70
	{0, 0, 0}, {0, 0, 0}};

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
// The category names, titles.txt tokens (FUN_10036200 copies them to obj+0x2e5c).
static const char* const s_catName[TS_CATS] = {"#Buy_Handguns", "#Buy_SMGs", "#Buy_Rifles", "#Buy_Shotguns",
	"#Buy_SpecialPurpose"};

// A row's key character (0x10037a95): '1'..'9' for rows 0..8, then 'A' on.
static char RowKey(int row) { return (char)(row < 9 ? row + '1' : row + '8'); }

// The name the menus show for a weapon: the weapon table's (wi+0x44).
static const char* TableName(int id)
{
	const s_weapon_info* wi = get_weapon_info((tsweapon_t)id);
	return (wi && wi->pszName) ? wi->pszName : "";
}

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

// "CustomWP" — one ASCII line "%i %f %f %f %f %i" (id accuracy viewkick accuracykick damage
// cost) per customized weapon. Writes straight into the SHARED g_weaponInfo table so
// prediction picks the new tunables up for free; prices re-resolve live through iPrice.
// The original's no-clamp quirk kept: a bad id mutates slot 0 via get_weapon_info.
static int MsgFunc_CustomWP(const char* pszName, int iSize, void* pbuf)
{
	BEGIN_READ(pbuf, iSize);
	const char* line = READ_STRING();
	int id = 0, cost = 0;
	float acc = 0, vkick = 0, akick = 0, dmg = 0;
	if (sscanf(line, "%i %f %f %f %f %i", &id, &acc, &vkick, &akick, &dmg, &cost) == 6)
	{
		s_weapon_info* wi = (s_weapon_info*)get_weapon_info((tsweapon_t)id);
		wi->flSpread = acc;
		wi->flViewKick = vkick;
		wi->flRecoil = akick;
		wi->flDamage = dmg;
		wi->iPrice = cost;
	}
	return 1;
}

// The live buy price: the shared table's iPrice (CustomWP-adjustable), falling back to the
// static menu row for anything the table doesn't price.
static int LivePrice(const TSWeapon* w)
{
	const int p = get_weapon_info((tsweapon_t)w->id)->iPrice;
	return p > 0 ? p : w->price;
}

bool CHudTSBuyMenu::Init()
{
	gHUD.AddHudElem(this);
	m_iFlags = 0;
	gEngfuncs.pfnHookUserMsg("CustomWP", MsgFunc_CustomWP);
	Reset();
	return true;
}

bool CHudTSBuyMenu::VidInit()
{
	m_firstOpen = true; // first open on a fresh map randomizes (FUN_10036200 sets obj+0x2f10)

	// cont.96: CHANGELEVEL state reset ([[map-change-state-reset]]) — if the menu was logically
	// up when the level changed, m_active stayed true on the new map with nothing drawn, and
	// IN_MouseEvent then gave it every click, so "Press Fire To Play" never spawned you — sticky
	// for the whole session. Close resets m_active + the cursor state.
	Close();

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

// FUN_10036670 — open: load the last-applied config; on the FIRST open of the map, randomize the
// loadout instead (so it comes up as a ready "Use New Config"). Subsequent opens reload the last
// config ("Use Last Config").
void CHudTSBuyMenu::Open()
{
	if (m_active)
		return; // already open — don't reload/re-randomize (FUN_10036670 guards on obj[0]==0). The
	            // server resends the spectate message each countdown tick, so Open() is called repeatedly.

	m_active = true;
	m_level = LVL_CONFIG;
	m_weapon = -1;
	m_attachMask = 0;
	m_prevButtons = 0;

	// load the last config (obj+0x41c -> obj+0x14)
	if (m_lastConfigValid)
		memcpy(m_slots, m_lastConfig, sizeof(m_slots));
	else
		memset(m_slots, 0, sizeof(m_slots));
	m_useNewConfig = false;

	if (m_firstOpen)
	{
		m_firstOpen = false;
		BuyRandom(); // FUN_100398e0 — fills a random loadout + flips to "Use New Config"
	}
	Recount();

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

// client.dll 0x1000b670: the restriction category of a weapon, its fire type (weapon info +0x348)
// except the katana (3), the sawed-off (1), the akimbo SMGs (9) and the akimbo pistols (8).
static int RestrictionCategory(int id)
{
	switch (id)
	{
	case 0x22: return 3;
	case 0x21: return 1;
	case 0x10: case 0x25: return 9;
	case 10: case 0x1e: case 8: case 0x15: return 8;
	default: return get_weapon_info((tsweapon_t)id)->iFireType;
	}
}

// client.dll 0x1000b6d0: a weapon (1..37) the server's restriction mask leaves out. A mask of 0 leaves
// out every weapon; otherwise category 1 needs bit 0x10, 3 bit 1, 6 and 7 bit 8, 8 bit 2, 9 bit 4, and
// the other categories are never left out.
static bool WeaponRestricted(int id, int mask)
{
	if (id <= 0 || id >= 0x26)
		return false;
	if (mask == 0)
		return true;
	switch (RestrictionCategory(id))
	{
	case 1: return (mask & 0x10) == 0;
	case 3: return (mask & 1) == 0;
	case 6: case 7: return (mask & 8) == 0;
	case 8: return (mask & 2) == 0;
	case 9: return (mask & 4) == 0;
	default: return false;
	}
}

// client.dll 0x100365e0: a weapon is left out when the restriction mask (SrvSett id 3) leaves it out,
// and the M61 while the GameMode byte is below 2; an accessory with bit 0x20 (the stealth shoes) while
// the GameMode byte is below 2.
bool CHudTSBuyMenu::Restricted(int id, bool accessory) const
{
	extern int g_iTSWeaponRestriction; // ts_status.cpp
	if (!accessory)
		return WeaponRestricted(id, g_iTSWeaponRestriction) || (gHUD.m_iTeamplay < 2 && id == 24);
	return gHUD.m_iTeamplay < 2 && (id & 0x20) != 0;
}

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
		return LivePrice(w) * (akimboOrFull ? w->fullcnt : 1);
	int cost = LivePrice(w) * (akimboOrFull ? 2 : 1);
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
	memset(m_ammoRounds, 0, sizeof(m_ammoRounds));
	memset(m_ammoUnits, 0, sizeof(m_ammoUnits));
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

	// Auto-ammo (FUN_100392b0): distribute the remaining budget across the loadout's calibers, one
	// unit per weapon per pass, until nothing more can be bought; the ammo cost adds to the Total.
	int remaining = TS_BUDGET - m_spent;
	bool progress = true;
	while (progress && remaining > 0)
	{
		progress = false;
		for (const auto& s : m_slots)
		{
			if (!s.used || s.accessory)
				continue;
			int cal = WeaponCaliber(s.weaponId);
			if (cal <= 0 || cal >= 16)
				continue;
			const TSCaliber& ci = s_caliber[cal];
			if (ci.maxRounds <= 0 || m_ammoRounds[cal] >= ci.maxRounds || remaining < ci.cost)
				continue;
			m_ammoRounds[cal] += ci.unit;
			if (m_ammoRounds[cal] > ci.maxRounds)
				m_ammoRounds[cal] = ci.maxRounds;
			m_ammoUnits[cal]++;
			remaining -= ci.cost;
			m_spent += ci.cost; // the Total includes auto-ammo (Glock 350 -> 3400; verified)
			progress = true;
		}
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
			m_slots[i].cost += LivePrice(w);
			Recount();
			return true;
		}
		// 2nd buy of an akimbo-capable single -> COMBINE into akimbo (the server gives the akimbo
		// variant). Tentatively upgrade, then revert if it busts the budget / slot cap. Re-buying an
		// already-akimbo (or non-akimbo) weapon toggles it off.
		if (w && w->akimbo >= 0 && !m_slots[i].akimbo)
		{
			const int prevCost = m_slots[i].cost;
			m_slots[i].akimbo = true;
			m_slots[i].cost = WeaponCost(id, m_slots[i].attachMask, true);
			Recount();
			if (m_spent > TS_BUDGET || m_slotsUsed > TS_SLOTCAP)
			{
				m_slots[i].akimbo = false;
				m_slots[i].cost = prevCost;
				Recount();
				return false;
			}
			return true;
		}
		RemoveSlot(i); // re-buy steps down: akimbo -> single (RemoveSlot), single -> off
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
	// An akimbo slot steps DOWN to its single first (removing the dual-wield keeps the base weapon);
	// a second remove deletes it. Non-akimbo slots delete outright.
	if (m_slots[i].akimbo)
	{
		m_slots[i].akimbo = false;
		m_slots[i].cost = WeaponCost(m_slots[i].weaponId, m_slots[i].attachMask, false);
		Recount();
		m_useNewConfig = true;
		return;
	}
	m_slots[i] = Slot{};
	CompactSlots();
	Recount();
	m_useNewConfig = true;
}

// The used slots close up in order, as client.dll's 0x10038e40 keeps them (no gaps).
void CHudTSBuyMenu::CompactSlots()
{
	int n = 0;
	for (int k = 0; k < MAX_SLOTS; k++)
		if (m_slots[k].used)
			m_slots[n++] = m_slots[k];
	for (; n < MAX_SLOTS; n++)
		m_slots[n] = Slot{};
}

// Remove Item (client.dll 0x10038e40, then the recount 0x100392b0): the whole slot goes, an akimbo pair
// with it, and the used slots close up so they stay in order with no gaps.
void CHudTSBuyMenu::RemoveItem(int i)
{
	if (i < 0 || i >= MAX_SLOTS || !m_slots[i].used)
		return;
	m_slots[i] = Slot{};
	CompactSlots();
	Recount();
	m_useNewConfig = true;
}

// client.dll 0x10038800: an emptied config starts with kevlar while cl_buykevlar is not 0 (its only
// reader; client.dll never writes the cvar).
void CHudTSBuyMenu::ResetConfig()
{
	memset(m_slots, 0, sizeof(m_slots));
	m_spent = 0;
	m_slotsUsed = 0;
	m_useNewConfig = true;
	if (gEngfuncs.pfnGetCvarFloat("cl_buykevlar") != 0.0f)
		QueueWeapon(TS_ACC_KEVLAR, 0, false, true);
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
	// FUN_10039400's gates: an empty config only says so (red) and is applied anyway, so the menu
	// still closes and the player gets a spawn with nothing bought; over the budget it says so and
	// stays open; over the slot cap it stays open silently.
	if (m_spent < 1)
		gHUD.m_TSMessage.Add("#Buy_ConfigEmpty", 195, 45, 45, 20, 0);
	if (TS_BUDGET < m_spent)
	{
		gHUD.m_TSMessage.Add("#Buy_NoCash", 195, 45, 45, 20, 0);
		return;
	}
	if (TS_SLOTCAP < m_slotsUsed)
		return;

	gEngfuncs.pfnClientCmd("rst_cfg\n");

	// THE ORIGINAL GRAMMAR (FUN_10039400, dossier-attachments §2): ONE batched `tkw` where each
	// gun token is `<type>_<attachMask>` (the attach bits ride INSIDE the tkw token — there is
	// no per-attachment tki); a stackable (knife/m61) token is `<type>_<count>`; a bought-double
	// (akimbo) appends an EXTRA `<type>_0` token (the pair carries no attach bits — the server's
	// AddItem/TakeWeapon mirrors them). `tki` carries only EQUIPMENT items (kevlar). Send order:
	// rst_cfg -> tkw -> tki -> tka.
	bool kevlar = false;
	char tkw[512] = "tkw";
	bool anyWeapon = false;
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
		const TSWeapon* sw = WeaponInfo(s.weaponId);
		const s_weapon_info* wi = get_weapon_info((tsweapon_t)s.weaponId);
		char buf[32];
		if (wi && wi->iStackable)
		{
			sprintf(buf, " %d_%d", s.weaponId, s.count < 1 ? 1 : s.count);
			strncat(tkw, buf, sizeof(tkw) - strlen(tkw) - 1);
		}
		else
		{
			sprintf(buf, " %d_%d", s.weaponId, s.attachMask & 0xF);
			strncat(tkw, buf, sizeof(tkw) - strlen(tkw) - 1);
			if (s.akimbo && sw && sw->akimbo >= 0)
			{
				// the bought-double: a second bare token; the two singles COMBINE server-side
				sprintf(buf, " %d_0", s.weaponId);
				strncat(tkw, buf, sizeof(tkw) - strlen(tkw) - 1);
			}
		}
		anyWeapon = true;
	}
	if (anyWeapon)
	{
		strncat(tkw, "\n", sizeof(tkw) - strlen(tkw) - 1);
		gEngfuncs.pfnClientCmd(tkw);
	}

	// the auto-bought ammo: one `tka <caliber>_<units>` batch (FUN_10039400's tka loop).
	char tka[256] = "tka";
	bool anyAmmo = false;
	for (int cal = 1; cal < 16; cal++)
	{
		if (m_ammoUnits[cal] <= 0)
			continue;
		char buf[24];
		sprintf(buf, " %d_%d", cal, m_ammoUnits[cal]);
		strncat(tka, buf, sizeof(tka) - strlen(tka) - 1);
		anyAmmo = true;
	}
	if (anyAmmo)
	{
		strncat(tka, "\n", sizeof(tka) - strlen(tka) - 1);
		gEngfuncs.pfnClientCmd(tka);
	}
	if (kevlar)
		gEngfuncs.pfnClientCmd("tki 16\n"); // equipment items ride tki (kevlar = 0x10)

	// the centred notice (FUN_10039400's "Using new/last config." via the TSMessage queue) + save
	// this loadout as the "last config" (obj+0x14 -> obj+0x41c) so the next open reuses it.
	gHUD.m_TSMessage.Add(m_useNewConfig ? "#Buy_UsingNewConfig" : "#Buy_UsingLastConfig", 0, 0, 0, 20, 0);
	memcpy(m_lastConfig, m_slots, sizeof(m_lastConfig));
	m_lastConfigValid = true;

	m_useNewConfig = false;
	Close();
}

//----------------------------------- input -----------------------------------

// 0x10038b80: back to the config column, the category, weapon and attachments cleared.
void CHudTSBuyMenu::BackToConfig()
{
	m_level = LVL_CONFIG;
	m_cat = 0;
	m_weapon = -1;
	m_attachMask = 0;
}

void CHudTSBuyMenu::HandleSlot(int slot)
{
	if (!m_active)
		return;
	Dispatch(m_level, slot); // the keys always act on the current column
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
		else if (key == 4)
		{
			ResetConfig();
			gHUD.m_TSMessage.Add("#Buy_ConfigReset", 0, 0, 0, 20, 0);
		}
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
		else if (key == 2 && !Restricted(TS_ACC_STEALTH, true)) { QueueWeapon(TS_ACC_STEALTH, 0, false, true); m_level = LVL_CATEGORY; }
		else if (key == 10) m_level = LVL_CATEGORY;
		break;

	case LVL_REMOVE:
	{
		// 0x10038a5d: the key is the slot's place among all sixteen (as the column numbers them); the
		// notice names it ("%s Removed") before it goes. Go Back returns to the config column.
		if (key == 10) { BackToConfig(); break; }
		const int i = key - 1;
		if (i < 0 || i >= MAX_SLOTS || !m_slots[i].used)
			break;
		const char* nm = m_slots[i].accessory
			? ((m_slots[i].weaponId & TS_ACC_KEVLAR) ? "Kevlar" : "Stealth Shoes")
			: TableName(m_slots[i].weaponId);
		char msg[64];
		snprintf(msg, sizeof(msg), CHudTextMessage::BufferedLocaliseTextString("#Buy_Removed"), nm);
		gHUD.m_TSMessage.Add(msg, 0, 0, 0, 20, 0);
		RemoveItem(i);
		break;
	}

	case LVL_SAVE:
		// 0x10038b2a: store, back to the config column, "Saved Config on slot #N".
		if (key == 10) { BackToConfig(); break; }
		if (key >= 1 && key <= SAVE_SLOTS)
		{
			memcpy(m_saved[key - 1], m_slots, sizeof(m_slots));
			m_savedSpent[key - 1] = m_spent;
			m_savedFull[key - 1] = (m_spent > 0);
			BackToConfig();
			char msg[64];
			snprintf(msg, sizeof(msg), CHudTextMessage::BufferedLocaliseTextString("#Buy_SavedConfig"), key);
			gHUD.m_TSMessage.Add(msg, 0, 0, 0, 20, 0);
		}
		break;

	case LVL_LOAD:
		// 0x10038bab: only a slot that cost something; "Config #N Loaded".
		if (key == 10) { BackToConfig(); break; }
		if (key >= 1 && key <= SAVE_SLOTS && m_savedFull[key - 1])
		{
			memcpy(m_slots, m_saved[key - 1], sizeof(m_slots));
			Recount();
			char msg[64];
			snprintf(msg, sizeof(msg), CHudTextMessage::BufferedLocaliseTextString("#Buy_ConfigLoaded"), key);
			gHUD.m_TSMessage.Add(msg, 0, 0, 0, 20, 0);
			m_useNewConfig = true;
			BackToConfig();
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
		// FUN_10038410 for the sub-columns that own column 1: Save and Load put their title on row 0, so a
		// row is its own key (rows 1..9, row 10 Go Back); Remove numbers the used slots from row 0, the row
		// after them Go Back.
		if (m_level == LVL_SAVE || m_level == LVL_LOAD)
		{
			if (row < 0xb) { level = m_level; key = row; }
			return;
		}
		if (m_level == LVL_REMOVE)
		{
			int used = 0;
			while (used < MAX_SLOTS && m_slots[used].used)
				used++;
			if (row < used) { level = LVL_REMOVE; key = row + 1; }
			else if (row == used) { level = LVL_REMOVE; key = 10; }
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
	if (col == 2 && m_level == LVL_ACCESSORY) // the accessory column sits at the weapons x
	{
		// FUN_10038410: rows 0 and 1 are Kevlar and Stealth Shoes, a restricted Kevlar moving the row
		// down one; the row after them is Go Back.
		if (row >= 3)
			return;
		int r2 = row;
		if (r2 < 2 && Restricted(r2 == 0 ? TS_ACC_KEVLAR : TS_ACC_STEALTH, true))
			r2++;
		level = LVL_ACCESSORY;
		key = (r2 == 2) ? 10 : r2 + 1;
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
			if (!AttachAvailable(m_weapon, 1 << bit)) // bit VALUE, matching the display loop (DrawColumnDetail)
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
	// 0x1004ff10 localises the entry (titles.txt tokens, 0x100349a0) before it draws it
	TSHud_DrawString(c.x, c.y, ScreenWidth, CHudTextMessage::BufferedLocaliseTextString(text), r, g, b);
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
	sprintf(buf, CHudTextMessage::BufferedLocaliseTextString("#Buy_Total"), m_spent);
	if (m_spent > TS_BUDGET) { COL_RED; } else { r = dr; g = dg; b = db; }
	TSHud_DrawString(8, 8, ScreenWidth, buf, r, g, b);

	// "Free Slots" shows the REMAINING slots (cap - used), red when overspent — confirmed by the
	// reference images (Glock slot 10 -> 81-10 = 71; FUN_10036af0).
	int freeSlots = TS_SLOTCAP - m_slotsUsed;
	sprintf(buf, CHudTextMessage::BufferedLocaliseTextString("#Buy_FreeSlots"), freeSlots);
	if (freeSlots < 0) { COL_RED; } else { r = dr; g = dg; b = db; }
	TSHud_DrawString(0x108, 8, ScreenWidth, buf, r, g, b);

	DrawLoadoutPreview();
}

// FUN_10036af0's preview: the bought items drawn as icons in 5 columns (by HUD column 1..5), in the
// ORDER they were queued (confirmed by the reference images — Kevlar interleaves with the col-1
// melee items in add-order). Each item shows its count to the LEFT of the icon (Seal Knife 10 /
// Combat Knife 5 = full-load counts) and its attachments below as "(Sil,Las,Fla,Sco)" from titles.txt.
// NOTE: for ammo weapons the original shows the auto-bought AMMO count here (e.g. Glock 210); the
// per-caliber max that caps it resisted static RE (see reference/MENUS.md), so guns show no number
// yet — the auto-ammo (and its share of Total/the tka command) is the one remaining slice.
void CHudTSBuyMenu::DrawLoadoutPreview()
{
	int hr, hg, hb;
	TSHud_Color(hr, hg, hb);
	static const char* const ab[4] = {"#Buy_Sil", "#Buy_Las", "#Buy_Fla", "#Buy_Sco"};

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
			// A slot shows its single in the single's column; an AKIMBO slot ALSO shows the akimbo
			// variant in column 5 (so the bar shows BOTH the base gun and the dual-wield). Figure out
			// which icon, if any, belongs in THIS column.
			const TSWeapon* base = WeaponInfo(s.weaponId);
			int dispId = -1;
			bool isAkimboIcon = false;
			if (base && base->hudCol == col)
				dispId = s.weaponId; // the single (always shown)
			else if (s.akimbo && base && base->akimbo >= 0)
			{
				const TSWeapon* ak = WeaponInfo(base->akimbo);
				if (ak && ak->hudCol == col) { dispId = base->akimbo; isAkimboIcon = true; }
			}
			if (dispId < 0)
				continue;
			const TSWeapon* w = WeaponInfo(dispId);

			// number to the LEFT of the icon (FUN_10036af0): for ammo weapons = the auto-bought rounds
			// (Glock 210 / M16 120), for full-load throwables = the count (Seal Knife 10 / Combat Knife 5).
			// The akimbo icon carries no separate count (its ammo shows on the single's row).
			int iconX = x;
			int cal = WeaponCaliber(s.weaponId);
			int lead = isAkimboIcon ? 0
					   : (cal > 0 && cal < 16 && m_ammoRounds[cal] > 0) ? m_ammoRounds[cal]
					   : (s.count > 1 ? s.count : 0);
			if (lead > 0)
			{
				char cnt[8];
				sprintf(cnt, "%d", lead);
				TSHud_DrawString(x, y + 4, ScreenWidth, cnt, hr, hg, hb);
				iconX = x + 0x18;
			}
			if (m_hIcon[dispId])
			{
				SPR_Set(m_hIcon[dispId], hr, hg, hb);
				SPR_DrawAdditive(0, iconX, y, &m_rcIcon[dispId]);
			}
			else
			{
				TSHud_DrawString(iconX, y, ScreenWidth, w->name, hr, hg, hb);
			}
			// attachments under the icon (0x100368f0): "(" then each one's localised abbreviation with a
			// "," between them, then ")"
			if (s.attachMask)
			{
				char at[64] = "(";
				bool any = false;
				for (int bit = 0; bit < 4; bit++)
				{
					if ((s.attachMask & (1 << bit)) == 0)
						continue;
					if (any)
						strncat(at, ",", sizeof(at) - strlen(at) - 1);
					strncat(at, CHudTextMessage::BufferedLocaliseTextString(ab[bit]), sizeof(at) - strlen(at) - 1);
					any = true;
				}
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
		DrawEntry(c, "1 #Buy_EmptyConfig", r, g, b);
	}
	else
	{
		const char* lbl = m_useNewConfig ? "1 #Buy_UseNewConfig" : "1 #Buy_UseLastConfig";
		if (m_spent > TS_BUDGET) { COL_RED; } else { r = dr; g = dg; b = db; }
		DrawEntry(c, lbl, r, g, b);
	}
	DrawEntry(c, "2 #Buy_Buy", dr, dg, db);
	DrawEntry(c, "3 #Buy_RemoveItem", dr, dg, db);
	DrawEntry(c, "4 #Buy_ResetConfig", dr, dg, db);
	DrawEntry(c, "5 #Buy_SaveConfig", dr, dg, db);
	DrawEntry(c, "6 #Buy_LoadConfig", dr, dg, db);
	COL_GREEN;
	DrawEntry(c, "7 #Buy_Random", r, g, b);
	if (m_level == LVL_CONFIG)
	{
		COL_BLUE;
		DrawEntry(c, "0 #Buy_Cancel", r, g, b);
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
		sprintf(buf, "%c %s", RowKey(cat), s_catName[cat]); // 0x10037871
		DrawEntry(c, buf, dr, dg, db);
	}
	DrawEntry(c, "6 #Buy_PlayerAccessory", dr, dg, db);
	if (m_level == LVL_CATEGORY)
	{
		COL_BLUE;
		DrawEntry(c, "0 #Buy_GoBack", r, g, b);
	}
	DrawColumnConfig();
}

void CHudTSBuyMenu::DrawColumnWeapons()
{
	Cursor c;
	int dr, dg, db, r, g, b;
	ColDim(dr, dg, db);
	SetCursor(c, TS_X_WEAPONS, ScreenMidY(), TS_ROWH, TS_COLW);

	// 0x100379c7: rows 0..11 of the category, an empty or restricted one skipped (not drawn, its key
	// unused), each labelled with its own row's key character.
	char buf[48];
	for (int row = 0; row < TS_ROWS; row++)
	{
		int id = s_roster[m_cat][row];
		if (id < 0)
			continue;
		if (Restricted(id))
			continue;
		const TSWeapon* w = WeaponInfo(id);
		bool afford = w && (m_spent + WeaponCost(id, 0, false) <= TS_BUDGET) &&
					  (m_slotsUsed + WeaponSlotCost(id, false) <= TS_SLOTCAP);
		if (afford) { r = dr; g = dg; b = db; } else { COL_RED; }
		sprintf(buf, "%c %s", RowKey(row), TableName(id)); // 0x10037abf
		DrawEntry(c, buf, r, g, b);
		if (id == m_weapon)
			DrawWeaponPreview(id);
	}
	if (m_level == LVL_WEAPONS)
	{
		COL_BLUE;
		DrawEntry(c, "0 #Buy_GoBack", r, g, b);
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
	if (afford) { COL_GREEN; } else { COL_RED; } // a confirm action — green like "7 Random"
	DrawEntry(c, "1 #Buy_Buy", r, g, b);

	char buf[48];
	// Attachments use FIXED keys 2..5 (silencer/laser/flashlight/scope = bits 1/2/4/8), matching the
	// key handler (`bit = 1 << (key-2)`). AttachAvailable takes the bit VALUE (1<<bit), not the index —
	// the s_attachLine/s_attachCost arrays are indexed by `bit`. Only supported, not-yet-bought ones show
	// (so the numbering can have gaps, e.g. the SOCOM has no Scope), but each attachment keeps its own key.
	for (int bit = 0; bit < 4; bit++)
	{
		if (!AttachAvailable(m_weapon, 1 << bit))
			continue;
		int cost = m_spent + baseCost + s_attachCost[bit];
		if (cost <= TS_BUDGET) { r = dr; g = dg; b = db; } else { COL_RED; }
		sprintf(buf, s_attachLine[bit], bit + 2, s_attachCost[bit]);
		DrawEntry(c, buf, r, g, b);
	}
	if (w && w->akimbo >= 0)
	{
		bool ak = m_spent + WeaponCost(m_weapon, m_attachMask, true) <= TS_BUDGET;
		if (ak) { COL_GREEN; } else { COL_RED; } // a confirm action — green like "7 Random"
		DrawEntry(c, "6 #Buy_Akimbo", r, g, b);
	}
	else if (w && w->full)
	{
		bool fl = m_spent + WeaponCost(m_weapon, m_attachMask, true) <= TS_BUDGET;
		if (fl) { COL_GREEN; } else { COL_RED; } // a confirm action — green like "7 Random"
		DrawEntry(c, "6 #Buy_FullLoad", r, g, b);
	}
	if (m_level == LVL_DETAIL)
	{
		COL_BLUE;
		DrawEntry(c, "0 #Buy_GoBack", r, g, b);
	}
	DrawColumnWeapons();
}

// FUN_10037fe0 — the accessory column (at the weapons x): Kevlar (1900, 10 slots) and Stealth Shoes
// (700, no slots), each unless restricted (0x100365e0), red when it would pass the budget or the slot
// cap, else dim.
void CHudTSBuyMenu::DrawColumnAccessory()
{
	Cursor c;
	int dr, dg, db, r, g, b;
	ColDim(dr, dg, db);
	SetCursor(c, TS_X_WEAPONS, ScreenMidY(), TS_ROWH, TS_COLW);
	if (!Restricted(TS_ACC_KEVLAR, true))
	{
		if (m_spent + 1900 > TS_BUDGET || m_slotsUsed + 10 > TS_SLOTCAP) { COL_RED; } else { r = dr; g = dg; b = db; }
		DrawEntry(c, "1 Kevlar  1900", r, g, b);
	}
	if (!Restricted(TS_ACC_STEALTH, true))
	{
		if (m_spent + 700 > TS_BUDGET || m_slotsUsed > TS_SLOTCAP) { COL_RED; } else { r = dr; g = dg; b = db; }
		DrawEntry(c, "2 Stealth Shoes  700", r, g, b);
	}
	COL_BLUE;
	DrawEntry(c, "0 #Buy_GoBack", r, g, b);
	DrawColumnCategory();
}

// FUN_10037260 (save) / the load column. Lists the 9 config slots.
void CHudTSBuyMenu::DrawColumnSaveLoad(bool save)
{
	Cursor c;
	int dr, dg, db, r, g, b;
	ColDim(dr, dg, db);
	SetCursor(c, TS_X_CATEGORY, ScreenMidY(), TS_ROWH, TS_COLW);
	// FUN_10037260: the title, then "Slot #N (Available)" for a slot that cost something, else
	// "Slot #N (Empty)", red when loading one that would pass the budget, else dim.
	COL_GREEN;
	DrawEntry(c, save ? "#Buy_SaveEquipment" : "#Buy_LoadEquipment", r, g, b);
	char buf[48];
	for (int i = 0; i < SAVE_SLOTS; i++)
	{
		sprintf(buf, CHudTextMessage::BufferedLocaliseTextString(
			m_savedSpent[i] > 0 ? "#Buy_SlotAvailable" : "#Buy_SlotEmpty"), i + 1);
		if (!save && TS_BUDGET < m_savedSpent[i]) { COL_RED; } else { r = dr; g = dg; b = db; }
		DrawEntry(c, buf, r, g, b);
	}
	COL_BLUE;
	DrawEntry(c, "0 #Buy_GoBack", r, g, b);
	DrawColumnConfig();
}

// the Remove-Item column: lists the queued slots.
void CHudTSBuyMenu::DrawColumnRemove()
{
	Cursor c;
	int dr, dg, db, r, g, b;
	ColDim(dr, dg, db);
	SetCursor(c, TS_X_CATEGORY, ScreenMidY(), TS_ROWH, TS_COLW);
	// FUN_100373c0: each used slot under its place among all sixteen, "%i %s" with the weapon table's
	// name, an accessory "%i " and Kevlar or Stealth Shoes.
	char buf[48];
	for (int i = 0; i < MAX_SLOTS; i++)
	{
		const Slot& s = m_slots[i];
		if (!s.used)
			continue;
		if (s.accessory)
			sprintf(buf, "%i %s", i + 1, (s.weaponId & TS_ACC_KEVLAR) ? "Kevlar" : "Stealth Shoes");
		else
			sprintf(buf, "%i %s", i + 1, TableName(s.weaponId));
		DrawEntry(c, buf, dr, dg, db);
	}
	COL_BLUE;
	DrawEntry(c, "0 #Buy_GoBack", r, g, b);
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
	sprintf(buf, CHudTextMessage::BufferedLocaliseTextString("#Buy_Credits"), LivePrice(w));
	TSHud_DrawString(0x108 + 0x80, y, ScreenWidth, buf, r / 2, g / 2, b / 2);
	sprintf(buf, CHudTextMessage::BufferedLocaliseTextString("#Buy_Slots"), w->slot);
	TSHud_DrawString(0x108 + 0x80, y + 0x14, ScreenWidth, buf, r / 2, g / 2, b / 2);
}

void CHudTSBuyMenu::DrawColumnChangeTeam()
{
	Cursor c;
	int r, g, b, dr, dg, db;
	ColDim(dr, dg, db);
	SetCursor(c, TS_X_CONFIG, ScreenMidY(), TS_ROWH, TS_COLW);
	COL_GREEN;
	DrawEntry(c, "#Chg_ChangeTeam", r, g, b); // 0x1003684c..0x100368df, titles.txt tokens
	DrawEntry(c, "1 #Chg_Join Specialists", dr, dg, db);
	DrawEntry(c, "2 #Chg_Join Mercenaries", dr, dg, db);
	COL_BLUE;
	DrawEntry(c, "0 #Chg_Cancel", r, g, b);
}

// FUN_10036750 — orchestrator: header/preview + the active cascade (builders draw their parents).
bool CHudTSBuyMenu::Draw(float flTime)
{
	if (!m_active)
		return true;
	// client.dll draws the buy menu from the weapon bar's Draw (0x1004d441), after its nohud test
	// (0x1004d3f9); its keys still work
	if (TSHud_NoHud())
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
