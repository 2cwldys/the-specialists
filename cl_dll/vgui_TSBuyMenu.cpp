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
		: CommandButton(text, x, y, wide, tall, /*bNoHighlight*/ false), m_r(r), m_g(g), m_b(b)
	{
		setContentAlignment(vgui::Label::a_west);
		setFont(vgui::Scheme::sf_primary3); // the compact command-menu font (matches the original)
	}
	void paint() override
	{
		// Hovered/selected entry = bright green text on a green bar (the original's selection
		// highlight, reference image); otherwise the per-entry colour. vgui alpha: 0 = opaque.
		if (isArmed())
			setFgColor(80, 230, 80, 0);
		else
			setFgColor(m_r, m_g, m_b, 0);
		vgui::Button::paint();
	}
	void paintBackground() override
	{
		if (isArmed()) // the green selection bar
		{
			drawSetColor(30, 110, 30, 80);
			drawFilledRect(0, 0, _size[0], _size[1]);
		}
		// non-hovered: no box (the panel's faint bg shows through)
	}
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

// A config action that CLOSES the menu: sends the server command (menuselect N) then hides the
// whole buy menu. The original closes the menu on "Use New Config" (1), "Buy Random" (7) and
// "Cancel" (0) — all confirm/cancel the loadout (server TSMenuSelect cases 1/7/10 -> TSCloseMenu).
// The other config items (Remove/Reset/Save/Load) keep the menu up.
class CTSMenuHandler_SelectClose : public ActionSignal
{
public:
	CTSMenuHandler_SelectClose(const char* cmd) { strncpy(m_cmd, cmd, sizeof(m_cmd) - 1); m_cmd[sizeof(m_cmd) - 1] = 0; }
	void actionPerformed(Panel* panel) override
	{
		gEngfuncs.pfnClientCmd(m_cmd);
		if (gViewPort)
			gViewPort->ShowTSBuyMenu(false); // hide the panel + release the cursor (UpdateCursorState)
	}

private:
	char m_cmd[32];
};

// A per-weapon attachment-submenu slot action (the +0x2d90 panel, FUN_10010bd0): toggle an
// attachment, buy the weapon (plain/akimbo/full), or go back. Reads the live slot from m_detailAction[].
class CTSMenuHandler_Detail : public ActionSignal
{
public:
	CTSMenuHandler_Detail(CTSBuyMenuPanel* menu, int slot) : m_pMenu(menu), m_slot(slot) {}
	void actionPerformed(Panel* panel) override { m_pMenu->OnDetailSlot(m_slot); }

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
	{'7', "Random", "menuselect 7"}, // "7 #Buy_Random" localizes to "Random" (not "Buy Random")
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

// --- Per-weapon attachment data (RE'd from the weapon table DAT_100aa050, reference/MENUS.md). ---
// The per-weapon attachment submenu (FUN_10010bd0) reads these weapon-struct fields:
//   0xE0 support mask | 0xE4 exclude mask (bit0 Silencer, bit1 Lasersight, bit2 Flashlight, bit3 Scope);
//   0xC8 akimbo type (akimbo variant id, -1 = none); 0xDC full-load flag (throwables).
// `attach` below is the EFFECTIVE available mask (support & ~exclude) — e.g. MP5SD/Ruger/AUG/Barrett
// exclude their integrated silencer/scope. The SAME fields drive the HUD's attached-icon row.
struct TSWeaponAttach { int id; int attach; int akimbo; int full; };
static const TSWeaponAttach s_weaponAttach[] = {
	{1, 0x7, -1, 0}, {2, 0x7, 8, 0}, {3, 0x1, 16, 0}, {4, 0xE, -1, 0}, {5, 0xF, -1, 0}, {6, 0xE, -1, 0},
	{7, 0xF, -1, 0}, {8, 0x7, -1, 0}, {9, 0x7, 10, 0}, {10, 0x7, -1, 0}, {11, 0x6, -1, 0}, {12, 0xB, -1, 0},
	{13, 0x8, -1, 0}, {14, 0x7, 30, 0}, {15, 0x3, -1, 0}, {16, 0x1, -1, 0}, {17, 0x0, 37, 0}, {18, 0x2, -1, 0},
	{19, 0xB, -1, 0}, {20, 0x6, -1, 0}, {21, 0x0, -1, 0}, {22, 0x7, -1, 0}, {23, 0xF, -1, 0}, {24, 0x0, -1, 1},
	{25, 0x0, -1, 1}, {26, 0x6, -1, 0}, {27, 0xE, -1, 0}, {28, 0x2, -1, 0}, {30, 0x7, -1, 0}, {31, 0xA, -1, 0},
	{32, 0x0, -1, 0}, {33, 0x2, -1, 0}, {34, 0x0, -1, 0}, {35, 0x0, -1, 1}, {36, 0xA, -1, 0}, {37, 0x0, -1, 0},
};
static const TSWeaponAttach* TSWeaponAttachData(int id)
{
	for (const auto& w : s_weaponAttach)
		if (w.id == id)
			return &w;
	return nullptr;
}
// Attachment names by bit index (RE'd menu strings: "%i Silencer %i", "%i Lasersight %i", ...).
static const char* const s_attachNames[4] = {"Silencer", "Lasersight", "Flashlight", "Scope"};

CTSBuyMenuPanel::CTSBuyMenuPanel(int x, int y, int wide, int tall)
	: Panel(x, y, wide, tall) // full-screen; the menu box + entries sit at the menu origin within it
{
	setBgColor(0, 0, 0, 255);  // the full-screen panel itself is transparent (alpha 255 = transparent)
	m_mx = XRES(40);           // menu origin (was the panel pos; now an offset inside the full-screen panel)
	m_my = YRES(120);
	m_wide = XRES(128);        // RE'd box/entry width 0x80 (FUN_100667e0)
	m_rowH = YRES(15);         // compact rows (the original packs the entries tightly — reference image)

	// The faint menu box (the shared menu/HUD look), behind the entry buttons; resized per page.
	m_pBg = new Panel(m_mx, m_my, m_wide, m_rowH * 8);
	m_pBg->setParent(this);
	m_pBg->setBgColor(0, 0, 0, 100);

	BuildConfig(m_wide);
	BuildCategory(m_wide);
	BuildAccessory(m_wide);
	BuildWeapons(m_wide);
	BuildDetail(m_wide);
	setVisible(false);
	ShowPage(PAGE_CONFIG);
}

void CTSBuyMenuPanel::BuildConfig(int wide)
{
	m_nConfig = sizeof(s_config) / sizeof(s_config[0]);
	for (int i = 0; i < m_nConfig; i++)
	{
		// static config actions: white (the exact colour awaits the +0x2d84 runtime inspect).
		m_pConfig[i] = new CTSMenuButton(s_config[i].label, m_mx, m_my + m_rowH * i, wide, m_rowH, 255, 255, 255);
		m_pConfig[i]->setParent(this);
		m_pConfig[i]->setBoundKey(s_config[i].key);
		char key = s_config[i].key;
		if (!s_config[i].cmd) // "2 Buy" -> the category page (client-side navigation)
			m_pConfig[i]->addActionSignal(new CTSMenuHandler_Page(this, PAGE_CATEGORY));
		else if (key == '1' || key == '7' || key == '0') // confirm/cancel -> send + close the menu
			m_pConfig[i]->addActionSignal(new CTSMenuHandler_SelectClose(s_config[i].cmd));
		else // Remove/Reset/Save/Load -> send, keep the menu up
			m_pConfig[i]->addActionSignal(new CMenuHandler_StringCommand(s_config[i].cmd, false));
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
		m_pCategory[i] = new CTSMenuButton(s_category[i].label, m_mx, m_my + m_rowH * i, wide, m_rowH, r, g, b);
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
		m_pWeapon[i] = new CTSMenuButton("", m_mx, m_my + m_rowH * i, wide, m_rowH, 45, 195, 45);
		m_pWeapon[i]->setParent(this);
		m_pWeapon[i]->addActionSignal(new CTSMenuHandler_BuyWeapon(this, i));
		m_pWeapon[i]->setVisible(false);
		m_weaponId[i] = -1;
	}
	m_nWeaponShown = 0;
}

// The per-weapon attachment submenu (the +0x2d90 panel, FUN_10010bd0). Reusable slots, rebuilt per
// weapon by ShowWeaponDetail(); each slot's action reads the live m_detailAction[].
void CTSBuyMenuPanel::BuildDetail(int wide)
{
	for (int i = 0; i < TS_MAX_BUTTONS; i++)
	{
		m_pDetail[i] = new CTSMenuButton("", m_mx, m_my + m_rowH * i, wide, m_rowH, 45, 195, 45);
		m_pDetail[i]->setParent(this);
		m_pDetail[i]->addActionSignal(new CTSMenuHandler_Detail(this, i));
		m_pDetail[i]->setVisible(false);
		m_detailAction[i] = -1;
	}
	m_nDetailShown = 0;
}

void CTSBuyMenuPanel::BuildAccessory(int wide)
{
	m_nAccessory = sizeof(s_accessory) / sizeof(s_accessory[0]);
	for (int i = 0; i < m_nAccessory; i++)
	{
		int r = 255, g = 255, b = 255;
		if (s_accessory[i].key == '0') { r = 100; g = 100; b = 205; } // Cancel (RE'd colour)
		m_pAccessory[i] = new CTSMenuButton(s_accessory[i].label, m_mx, m_my + m_rowH * i, wide, m_rowH, r, g, b);
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
	m_currentCategory = cat; // remembered so the detail submenu's Cancel returns to this list
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
		b->setPos(m_mx, m_my + m_rowH * shown);
		b->setVisible(true);
		m_weaponId[shown] = id;
		shown++;
	}
	// final "0 Cancel" row -> back to the category page
	CTSMenuButton* c = static_cast<CTSMenuButton*>(m_pWeapon[shown]);
	c->setText("Cancel"); // bound-key '0' column drawn by CommandButton (matches the other pages)
	c->setBoundKey('0');
	c->SetColor(100, 100, 205); // RE'd Cancel colour
	c->setPos(m_mx, m_my + m_rowH * shown);
	c->setVisible(true);
	m_weaponId[shown] = -1;
	shown++;
	for (int i = shown; i < TS_MAX_BUTTONS; i++)
		m_pWeapon[i]->setVisible(false);
	m_nWeaponShown = shown;
	ShowPage(PAGE_WEAPONS);
}

// A weapon-list slot was chosen (click or bound key). Cancel row (id<0) goes back to the category;
// otherwise open that weapon's attachment submenu (the original navigates to the +0x2d90 panel).
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
	m_selectedAttach = 0; // fresh attachment selection for this weapon
	ShowWeaponDetail(id);
}

// Rebuild the per-weapon attachment submenu (the +0x2d90 panel, FUN_10010bd0): "Buy", then each
// available attachment (effective mask minus already-selected), then "Buy Akimbo" (0xC8 != -1) or
// "Full Load" (0xDC), then "Cancel". Selecting an attachment removes it from the list (matches the
// original's (bit & selected)==0 gate) and rebuilds.
void CTSBuyMenuPanel::ShowWeaponDetail(int weaponId)
{
	m_detailWeapon = weaponId;
	const TSWeaponAttach* wa = TSWeaponAttachData(weaponId);
	int shown = 0;

	// "Buy" — queue the weapon (with the selected attachments) into the loadout.
	CTSMenuButton* b = static_cast<CTSMenuButton*>(m_pDetail[shown]);
	b->setText("Buy");
	b->setBoundKey((char)('1' + shown));
	b->SetColor(45, 195, 45);
	b->setPos(m_mx, m_my + m_rowH * shown);
	b->setVisible(true);
	m_detailAction[shown] = -2; // Buy
	shown++;

	// available attachments (effective mask, minus the already-selected ones)
	int avail = wa ? (wa->attach & ~m_selectedAttach) : 0;
	for (int bit = 0; bit < 4 && shown < TS_MAX_BUTTONS - 1; bit++)
	{
		if ((avail & (1 << bit)) == 0)
			continue;
		CTSMenuButton* a = static_cast<CTSMenuButton*>(m_pDetail[shown]);
		a->setText(s_attachNames[bit]);
		a->setBoundKey((char)('1' + shown));
		a->SetColor(45, 195, 45);
		a->setPos(m_mx, m_my + m_rowH * shown);
		a->setVisible(true);
		m_detailAction[shown] = bit; // toggle attachment bit
		shown++;
	}

	// "Buy Akimbo" (weapon has an akimbo variant) OR "Full Load" (throwables) — mutually exclusive.
	if (wa && wa->akimbo >= 0 && shown < TS_MAX_BUTTONS - 1)
	{
		CTSMenuButton* k = static_cast<CTSMenuButton*>(m_pDetail[shown]);
		k->setText("Buy Akimbo");
		k->setBoundKey((char)('1' + shown));
		k->SetColor(45, 195, 45);
		k->setPos(m_mx, m_my + m_rowH * shown);
		k->setVisible(true);
		m_detailAction[shown] = -3; // Buy Akimbo
		shown++;
	}
	else if (wa && wa->full && shown < TS_MAX_BUTTONS - 1)
	{
		CTSMenuButton* f = static_cast<CTSMenuButton*>(m_pDetail[shown]);
		f->setText("Full Load");
		f->setBoundKey((char)('1' + shown));
		f->SetColor(45, 195, 45);
		f->setPos(m_mx, m_my + m_rowH * shown);
		f->setVisible(true);
		m_detailAction[shown] = -4; // Full Load
		shown++;
	}

	// "Cancel" -> back to the weapon list
	CTSMenuButton* c = static_cast<CTSMenuButton*>(m_pDetail[shown]);
	c->setText("Cancel");
	c->setBoundKey('0');
	c->SetColor(100, 100, 205);
	c->setPos(m_mx, m_my + m_rowH * shown);
	c->setVisible(true);
	m_detailAction[shown] = -1; // back to weapon list
	shown++;

	for (int i = shown; i < TS_MAX_BUTTONS; i++)
		m_pDetail[i]->setVisible(false);
	m_nDetailShown = shown;
	ShowPage(PAGE_WEAPON_DETAIL);
}

// A detail-submenu slot was chosen. Toggle (0-3) flips the attachment + rebuilds; Buy/Akimbo/Full
// queue the weapon via `tkw <id>_1` (reference/BUY.md) then return to the weapon list; Cancel backs out.
// NOTE: the chosen attachments are tracked client-side; the server-side attachment encoding (the
// FUN_10012050 apply path) + server application + the HUD attached-icon row are a documented next slice.
void CTSBuyMenuPanel::OnDetailSlot(int slot)
{
	if (slot < 0 || slot >= m_nDetailShown)
		return;
	int act = m_detailAction[slot];
	if (act >= 0) // toggle an attachment bit, rebuild
	{
		m_selectedAttach ^= (1 << act);
		ShowWeaponDetail(m_detailWeapon);
		return;
	}
	if (act == -1) // Cancel -> back to the weapon list
	{
		ShowCategory(m_currentCategory);
		return;
	}
	int buyId = m_detailWeapon;
	if (act == -3) // Buy Akimbo -> the akimbo variant id
	{
		const TSWeaponAttach* wa = TSWeaponAttachData(m_detailWeapon);
		if (wa && wa->akimbo >= 0)
			buyId = wa->akimbo;
	}
	char cmd[32];
	sprintf(cmd, "tkw %d_1\n", buyId); // -2 Buy / -3 Akimbo / -4 Full Load all queue the weapon
	gEngfuncs.pfnClientCmd(cmd);
	ShowCategory(m_currentCategory); // back to the weapon list to buy more
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
	{
		m_pWeapon[i]->setVisible(page == PAGE_WEAPONS && i < m_nWeaponShown);
		m_pDetail[i]->setVisible(page == PAGE_WEAPON_DETAIL && i < m_nDetailShown);
	}

	// Hug the bg box to the active page's row count (the original sizes each sub-panel to its
	// own entry count via FUN_100667e0), so the box isn't taller than the visible entries.
	int rows;
	switch (page)
	{
	case PAGE_CATEGORY:      rows = m_nCategory;    break;
	case PAGE_ACCESSORY:     rows = m_nAccessory;   break;
	case PAGE_WEAPONS:       rows = m_nWeaponShown; break;
	case PAGE_WEAPON_DETAIL: rows = m_nDetailShown; break;
	default:                 rows = m_nConfig;      break;
	}
	if (m_pBg)
		m_pBg->setSize(m_wide, m_rowH * rows); // hug the box to the active page (panel stays full-screen)
}

CommandButton** CTSBuyMenuPanel::CurrentPage(int& count)
{
	switch (m_page)
	{
	case PAGE_CATEGORY:      count = m_nCategory;    return m_pCategory;
	case PAGE_ACCESSORY:     count = m_nAccessory;   return m_pAccessory;
	case PAGE_WEAPONS:       count = m_nWeaponShown; return m_pWeapon;
	case PAGE_WEAPON_DETAIL: count = m_nDetailShown; return m_pDetail;
	default:                 count = m_nConfig;      return m_pConfig;
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
