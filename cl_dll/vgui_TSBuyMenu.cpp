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
	{'6', "Player Accessory", nullptr},
	{'0', "Cancel", nullptr},       // back to the config page
};

CTSBuyMenuPanel::CTSBuyMenuPanel(int x, int y, int wide, int tall)
	: Panel(x, y, wide, tall)
{
	setBgColor(0, 0, 0, 100); // faint background panel (the shared menu/HUD look)
	m_rowH = YRES(20);        // RE'd row height 0x14 (FUN_100667e0)
	BuildConfig(wide);
	BuildCategory(wide);
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
		// (the per-category weapon-list navigation is the next slice — left inert for now)
	}
}

void CTSBuyMenuPanel::ShowPage(Page page)
{
	for (int i = 0; i < m_nConfig; i++)
		m_pConfig[i]->setVisible(page == PAGE_CONFIG);
	for (int i = 0; i < m_nCategory; i++)
		m_pCategory[i]->setVisible(page == PAGE_CATEGORY);
}

void CTSBuyMenuPanel::Open()
{
	setVisible(true);
	ShowPage(PAGE_CONFIG);
}

void CTSBuyMenuPanel::Close()
{
	setVisible(false);
}
