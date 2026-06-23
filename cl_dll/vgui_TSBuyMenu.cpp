/***
*  vgui_TSBuyMenu.cpp — The Specialists client-side buy/config menu (cont.69e/f).
*  See vgui_TSBuyMenu.h + reference/MENUS.md. vgui::Panel with toggled button pages
*  (config + buy-category), selected by mouse-click OR bound number key.
***/
#include "VGUI_Panel.h"
#include "VGUI_App.h"

#include "hud.h"
#include "cl_util.h"
#include "vgui_TeamFortressViewport.h"
#include "vgui_TSBuyMenu.h"

using namespace vgui;

// A TS-styled menu button. RE'd from client.dll: the TS menu entries are PLAIN vgui::Button
// (FUN_10066b40 — the vtable is all imported vgui::Button methods, NO paint override), so the
// TFC orange box/scheme comes only from the SDK's CommandButton. We keep CommandButton for its
// bound-key + action-signal plumbing but suppress that orange and draw plain text. Per-item COLOUR
// is per-entry RGB (FUN_10010440: e.g. "0 Cancel" = (100,100,205); availability green/red for
// weapon entries). The static config text is drawn white pending the runtime inspection of the
// +0x2d84 config panel (which resists static RE — reference/MENUS.md).
class CTSMenuButton : public CommandButton
{
public:
	CTSMenuButton(const char* text, int x, int y, int wide, int tall, int r, int g, int b)
		: CommandButton(text, x, y, wide, tall, /*bNoHighlight*/ true), m_r(r), m_g(g), m_b(b)
	{
		setContentAlignment(vgui::Label::a_west);
	}
	void paint() override
	{
		setFgColor(m_r, m_g, m_b, 0); // vgui alpha: 0 = opaque
		vgui::Button::paint();
	}
	void paintBackground() override {} // plain vgui::Button has no box
	void SetColor(int r, int g, int b) { m_r = r; m_g = g; m_b = b; } // for rebuilt (weapon-list) slots

private:
	int m_r, m_g, m_b;
};

// A page-switch action: clicking the button switches the menu to another page (the original
// navigates by toggling sub-panels; FUN_10011230 / the action thunks).
class CTSMenuHandler_Page : public ActionSignal
{
public:
	CTSMenuHandler_Page(CTSBuyMenuPanel* menu, CTSBuyMenuPanel::Page page) : m_pMenu(menu), m_page(page) {}
	void actionPerformed(Panel* panel) override { m_pMenu->ShowPage(m_page); }

private:
	CTSBuyMenuPanel* m_pMenu;
	CTSBuyMenuPanel::Page m_page;
};

// A buy-category action: clicking a category entry rebuilds + shows that category's weapon list
// (the original navigates to the +0x2d8c weapon-list panel rebuilt for the category — FUN_100108b0).
class CTSMenuHandler_Category : public ActionSignal
{
public:
	CTSMenuHandler_Category(CTSBuyMenuPanel* menu, int cat) : m_pMenu(menu), m_cat(cat) {}
	void actionPerformed(Panel* panel) override { m_pMenu->ShowCategory(m_cat); }

private:
	CTSBuyMenuPanel* m_pMenu;
	int m_cat;
};

// A weapon-list slot action: the slot's bound to a fixed index; on click the panel queues the
// weapon currently in that slot (or navigates back if it's the Cancel row). The weapon-list slots
// are reused across categories, so the action reads the live per-slot weapon id (m_weaponId[]).
class CTSMenuHandler_BuyWeapon : public ActionSignal
{
public:
	CTSMenuHandler_BuyWeapon(CTSBuyMenuPanel* menu, int slot) : m_pMenu(menu), m_slot(slot) {}
	void actionPerformed(Panel* panel) override { m_pMenu->OnWeaponSlot(m_slot); }

private:
	CTSBuyMenuPanel* m_pMenu;
	int m_slot;
};

// A cvar-toggle action: clicking the entry flips a client cvar (the accessory selection state;
// RE: the Player Accessory page toggles cl_buykevlar/cl_laser, shown via the selected-bit).
class CTSMenuHandler_ToggleCvar : public ActionSignal
{
public:
	CTSMenuHandler_ToggleCvar(const char* cvar) { strncpy(m_cvar, cvar, sizeof(m_cvar) - 1); m_cvar[sizeof(m_cvar) - 1] = 0; }
	void actionPerformed(Panel* panel) override
	{
		float v = CVAR_GET_FLOAT(m_cvar);
		gEngfuncs.Cvar_SetValue(m_cvar, v != 0.0f ? 0.0f : 1.0f);
	}

private:
	char m_cvar[32];
};

// --- recovered entries (reference/MENUS.md, string table @0x10092808). ---
struct TSEntry { char key; const char* label; const char* cmd; };
// Config page: each fires the server menuselect handler (CBasePlayer::TSMenuSelect).
static const TSEntry s_config[] = {
	{'1', "Use New Config", "menuselect 1"},
	{'2', "Buy", nullptr},          // navigates to the category page (client-side)
	{'3', "Remove Item", "menuselect 3"},
	{'4', "Reset Config", "menuselect 4"},
	{'5', "Save Config", "menuselect 5"},
	{'6', "Load Config", "menuselect 6"},
	{'7', "Buy Random", "menuselect 7"},
	{'0', "Cancel", "menuselect 10"},
};
// Buy-category page (RE'd from FUN_10010440: the 0x10092824 table loop + Cancel).
static const TSEntry s_category[] = {
	{'1', "Handguns", nullptr},
	{'2', "SMGs", nullptr},
	{'3', "Rifles", nullptr},
	{'4', "Shotguns", nullptr},
	{'5', "Other", nullptr},
	{'6', "Player Accessory", nullptr}, // -> the accessory page
	{'0', "Cancel", nullptr},       // back to the config page
};
// Player Accessory page (RE'd from the accessory builder @0x10010dd0: Kevlar + the priced
// Silencer/Lasersight/Flashlight/Scope, "%i <name>  %i"). Each toggles its client cvar selection;
// the loadout applies the selected accessories. Kevlar -> cl_buykevlar (ties to the kevlar gameplay).
static const TSEntry s_accessory[] = {
	{'1', "Kevlar", "cl_buykevlar"},
	{'2', "Silencer", "cl_silencer"},
	{'3', "Lasersight", "cl_laser"},
	{'4', "Flashlight", "cl_flashlight"},
	{'5', "Scope", "cl_scope"},
	{'0', "Cancel", nullptr},       // back to the category page
};

// --- Per-category weapon membership + display names (RE'd, reference/MENUS.md). ---
// Category->weapon-id rows are DAT_1007a950[cat][9] in client.dll (FUN_100108b0 reads them);
// display names are weaponstruct+0x44 (each from its per-weapon init func, e.g. FUN_10010080).
// -1 terminates a category. The category ORDER matches s_category (Handguns/SMGs/Rifles/Shotguns/Other).
struct TSWeaponName { int id; const char* name; };
static const TSWeaponName s_weaponNames[] = {
	{1, "Glock-18"}, {2, "Beretta 92F"}, {3, "Mini-Uzi"}, {4, "Benelli M3"}, {5, "M4A1"},
	{6, "MP5SD"}, {7, "MP5K"}, {8, "Akimbo Berettas"}, {9, "SOCOM-MK23"}, {10, "Akimbo MK23"},
	{11, "USAS-12"}, {12, "Desert Eagle"}, {13, "AK47"}, {14, "Five-seveN"}, {15, "STEYR-AUG"},
	{16, "Akimbo Mini-Uzi"}, {17, "Skorpion"}, {18, "Barrett M82A1"}, {19, "MP7-PDW"}, {20, "SPAS-12"},
	{21, "Golden Colts"}, {22, "Glock-20C"}, {23, "UMP"}, {24, "M61 Grenade"}, {25, "Combat Knife"},
	{26, "Mossberg 500"}, {27, "M16A4"}, {28, "Ruger-MK1"}, {30, "Akimbo Five-seveN"}, {31, "Raging Bull"},
	{32, "M60E3"}, {33, "Sawed-off"}, {34, "Katana"}, {35, "Seal Knife"}, {36, "Contender G2"},
	{37, "Akimbo Skorpions"},
};
static const char* TSWeaponDisplayName(int id)
{
	for (const auto& w : s_weaponNames)
		if (w.id == id)
			return w.name;
	return "?";
}
// DAT_1007a950: 5 categories x up to 9 weapon ids (-1 = empty). EXACT from client.dll.
static const int s_categoryWeapons[5][9] = {
	{1, 9, 12, 14, 8, 21, 22, 28, 31}, // Handguns
	{3, 6, 7, 17, 19, 23, -1, -1, -1}, // SMGs
	{5, 13, 15, 27, 18, -1, -1, -1, -1}, // Rifles
	{4, 11, 20, 26, 33, -1, -1, -1, -1}, // Shotguns
	{24, 32, 34, 25, 35, -1, -1, -1, -1}, // Other
};
#define TS_M61_WEAPON_ID 24 // M61 Grenade: shown only in team mode (FUN_100108b0: iVar5 != 0x18 gate)

CTSBuyMenuPanel::CTSBuyMenuPanel(int x, int y, int wide, int tall)
	: Panel(x, y, wide, tall)
{
	setBgColor(0, 0, 0, 100); // faint background panel (the shared menu/HUD look)
	m_wide = wide;
	m_rowH = YRES(20);        // RE'd row height 0x14 (FUN_100667e0)
	BuildConfig(wide);
	BuildCategory(wide);
	BuildAccessory(wide);
	BuildWeapons(wide);
	setVisible(false);
	ShowPage(PAGE_CONFIG);
}

void CTSBuyMenuPanel::BuildConfig(int wide)
{
	m_nConfig = sizeof(s_config) / sizeof(s_config[0]);
	for (int i = 0; i < m_nConfig; i++)
	{
		// static config actions: white (the exact colour awaits the +0x2d84 runtime inspect).
		m_pConfig[i] = new CTSMenuButton(s_config[i].label, 0, m_rowH * i, wide, m_rowH, 255, 255, 255);
		m_pConfig[i]->setParent(this);
		m_pConfig[i]->setBoundKey(s_config[i].key);
		if (s_config[i].cmd)
			m_pConfig[i]->addActionSignal(new CMenuHandler_StringCommand(s_config[i].cmd, true));
		else // "2 Buy" -> the category page (client-side navigation)
			m_pConfig[i]->addActionSignal(new CTSMenuHandler_Page(this, PAGE_CATEGORY));
	}
}

void CTSBuyMenuPanel::BuildCategory(int wide)
{
	m_nCategory = sizeof(s_category) / sizeof(s_category[0]);
	for (int i = 0; i < m_nCategory; i++)
	{
		// RE'd colours (FUN_10010440): "0 Cancel" = (100,100,205); category items are computed
		// per-entry (the exact float-derived RGB + the per-category weapon lists are the next
		// slice). Drawn white for now where not yet pinned.
		int r = 255, g = 255, b = 255;
		if (s_category[i].key == '0') { r = 100; g = 100; b = 205; } // Cancel (RE'd)
		m_pCategory[i] = new CTSMenuButton(s_category[i].label, 0, m_rowH * i, wide, m_rowH, r, g, b);
		m_pCategory[i]->setParent(this);
		m_pCategory[i]->setBoundKey(s_category[i].key);
		if (s_category[i].key == '0') // Cancel -> back to config
			m_pCategory[i]->addActionSignal(new CTSMenuHandler_Page(this, PAGE_CONFIG));
		else if (s_category[i].key == '6') // Player Accessory -> the accessory page
			m_pCategory[i]->addActionSignal(new CTSMenuHandler_Page(this, PAGE_ACCESSORY));
		else // Handguns/SMGs/Rifles/Shotguns/Other (keys 1-5) -> that category's weapon list
			m_pCategory[i]->addActionSignal(new CTSMenuHandler_Category(this, i)); // i == cat index 0..4
	}
}

// The per-category weapon list (the original's +0x2d8c panel, FUN_100108b0). One reusable page of
// slots, rebuilt for the selected category by ShowCategory(). Each slot has a fixed bound key + a
// persistent action that reads the live weapon id in that slot (so categories can swap in place).
void CTSBuyMenuPanel::BuildWeapons(int wide)
{
	for (int i = 0; i < TS_MAX_BUTTONS; i++)
	{
		// available weapons draw green (45,195,45) — RE'd availability colour (FUN_10011160; the
		// red 195,45,45 unavailable/free-slots gate is the deferred client loadout-cost model).
		m_pWeapon[i] = new CTSMenuButton("", 0, m_rowH * i, wide, m_rowH, 45, 195, 45);
		m_pWeapon[i]->setParent(this);
		m_pWeapon[i]->addActionSignal(new CTSMenuHandler_BuyWeapon(this, i));
		m_pWeapon[i]->setVisible(false);
		m_weaponId[i] = -1;
	}
	m_nWeaponShown = 0;
}

void CTSBuyMenuPanel::BuildAccessory(int wide)
{
	m_nAccessory = sizeof(s_accessory) / sizeof(s_accessory[0]);
	for (int i = 0; i < m_nAccessory; i++)
	{
		int r = 255, g = 255, b = 255;
		if (s_accessory[i].key == '0') { r = 100; g = 100; b = 205; } // Cancel (RE'd colour)
		m_pAccessory[i] = new CTSMenuButton(s_accessory[i].label, 0, m_rowH * i, wide, m_rowH, r, g, b);
		m_pAccessory[i]->setParent(this);
		m_pAccessory[i]->setBoundKey(s_accessory[i].key);
		if (s_accessory[i].key == '0') // Cancel -> back to the category page
			m_pAccessory[i]->addActionSignal(new CTSMenuHandler_Page(this, PAGE_CATEGORY));
		else if (s_accessory[i].cmd) // toggle the accessory's client cvar (e.g. Kevlar -> cl_buykevlar)
			m_pAccessory[i]->addActionSignal(new CTSMenuHandler_ToggleCvar(s_accessory[i].cmd));
	}
}

// Rebuild the weapon-list slots for a category, then show the page. Mirrors FUN_100108b0:
// iterate the category's 9 weapon-id slots, drop -1 / (M61 outside team mode), number them
// "%i %s" (1-based on-screen index + display name), then a final "0 Cancel" (blue 100,100,205).
void CTSBuyMenuPanel::ShowCategory(int cat)
{
	if (cat < 0 || cat > 4)
		return;
	bool teamMode = gViewPort && gViewPort->GetNumberOfTeams() > 1;
	int shown = 0;
	for (int s = 0; s < 9 && shown < TS_MAX_BUTTONS - 1; s++)
	{
		int id = s_categoryWeapons[cat][s];
		if (id < 0)
			continue;
		if (id == TS_M61_WEAPON_ID && !teamMode) // M61: team mode only (FUN_100108b0 gate)
			continue;
		// RE'd label is "%i %s" (number + name); our SDK CommandButton already draws the bound-key
		// number as its left column (like the config/category pages, whose labels carry no number),
		// so we pass the name only — the rendered result is the same "<n> <name>".
		CTSMenuButton* b = static_cast<CTSMenuButton*>(m_pWeapon[shown]);
		b->setText(TSWeaponDisplayName(id));
		b->setBoundKey((char)('1' + shown));
		b->SetColor(45, 195, 45); // available = green
		b->setPos(0, m_rowH * shown);
		b->setVisible(true);
		m_weaponId[shown] = id;
		shown++;
	}
	// final "0 Cancel" row -> back to the category page
	CTSMenuButton* c = static_cast<CTSMenuButton*>(m_pWeapon[shown]);
	c->setText("Cancel"); // bound-key '0' column drawn by CommandButton (matches the other pages)
	c->setBoundKey('0');
	c->SetColor(100, 100, 205); // RE'd Cancel colour
	c->setPos(0, m_rowH * shown);
	c->setVisible(true);
	m_weaponId[shown] = -1;
	shown++;
	for (int i = shown; i < TS_MAX_BUTTONS; i++)
		m_pWeapon[i]->setVisible(false);
	m_nWeaponShown = shown;
	ShowPage(PAGE_WEAPONS);
}

// A weapon-list slot was chosen (click or bound key). Cancel row (id<0) navigates back; otherwise
// queue the weapon into the server-side armament via the real swap-contract command `tkw type_count`
// (reference/BUY.md; FUN_10012050's apply emits the same). The loadout commits on "Use New Config".
// NOTE: the original queues client-side then emits the full rst_cfg+tkw list on apply; queuing each
// pick immediately is a faithful-command bridge (same as the config page's menuselect bridge). The
// per-weapon detail submenu (6 Full Load / 6 Buy Akimbo, the +0x2d90 panel) is a further slice.
void CTSBuyMenuPanel::OnWeaponSlot(int slot)
{
	if (slot < 0 || slot >= m_nWeaponShown)
		return;
	int id = m_weaponId[slot];
	if (id < 0) // the Cancel row
	{
		ShowPage(PAGE_CATEGORY);
		return;
	}
	char cmd[32];
	sprintf(cmd, "tkw %d_1\n", id);
	gEngfuncs.pfnClientCmd(cmd);
}

void CTSBuyMenuPanel::ShowPage(Page page)
{
	m_page = page;
	for (int i = 0; i < m_nConfig; i++)
		m_pConfig[i]->setVisible(page == PAGE_CONFIG);
	for (int i = 0; i < m_nCategory; i++)
		m_pCategory[i]->setVisible(page == PAGE_CATEGORY);
	for (int i = 0; i < m_nAccessory; i++)
		m_pAccessory[i]->setVisible(page == PAGE_ACCESSORY);
	for (int i = 0; i < TS_MAX_BUTTONS; i++)
		if (page != PAGE_WEAPONS)
			m_pWeapon[i]->setVisible(false);
		else
			m_pWeapon[i]->setVisible(i < m_nWeaponShown);

	// Hug the bg box to the active page's row count (the original sizes each sub-panel to its
	// own entry count via FUN_100667e0), so the box isn't taller than the visible entries.
	int rows;
	switch (page)
	{
	case PAGE_CATEGORY:  rows = m_nCategory;    break;
	case PAGE_ACCESSORY: rows = m_nAccessory;   break;
	case PAGE_WEAPONS:   rows = m_nWeaponShown; break;
	default:             rows = m_nConfig;      break;
	}
	setSize(m_wide, m_rowH * rows);
}

CommandButton** CTSBuyMenuPanel::CurrentPage(int& count)
{
	switch (m_page)
	{
	case PAGE_CATEGORY:  count = m_nCategory;    return m_pCategory;
	case PAGE_ACCESSORY: count = m_nAccessory;   return m_pAccessory;
	case PAGE_WEAPONS:   count = m_nWeaponShown; return m_pWeapon;
	default:             count = m_nConfig;      return m_pConfig;
	}
}

bool CTSBuyMenuPanel::HandleKey(int key)
{
	if (!m_up)
		return false;
	int count = 0;
	CommandButton** page = CurrentPage(count);
	for (int i = 0; i < count; i++)
	{
		if (page[i]->getBoundKey() == (char)key)
		{
			page[i]->fireActionSignal();
			return true;
		}
	}
	return false;
}

void CTSBuyMenuPanel::Open()
{
	m_up = true;
	setVisible(true);
	ShowPage(PAGE_CONFIG);
}

void CTSBuyMenuPanel::Close()
{
	m_up = false;
	setVisible(false);
}
