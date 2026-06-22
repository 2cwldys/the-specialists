/***
*  vgui_TSBuyMenu.h — The Specialists client-side buy/config menu (cont.69e/f).
*
*  Faithful reproduction of the TS buy menu, RE'd from client.dll (reference/MENUS.md): a
*  vgui::Panel of clickable text vgui::Button entries (NOT the MOTD gmsgVGUIMenu), selected by
*  mouse-click OR the bound number key, with the hierarchy Config -> Buy (weapon categories) ->
*  Player Accessory -> Kevlar/Lasersight/Silencer. Modelled on the SDK's CTeamMenuPanel.
*
*  This panel holds the menu PAGES (config + buy-category) as toggled button sets, matching the
*  original's panel-toggle navigation (FUN_10011230 setVisible). The config actions reuse the
*  server menuselect handler (CBasePlayer::TSMenuSelect); the category page (FUN_10010440) +
*  Cancel navigate client-side. The per-category weapon LISTS + the accessory page are the next
*  slices (their builders decompile; reference/MENUS.md).
***/
#pragma once

#include "VGUI_Panel.h"

class CommandButton;

class CTSBuyMenuPanel : public vgui::Panel
{
public:
	CTSBuyMenuPanel(int x, int y, int wide, int tall);

	void Open();   // show the menu (at the config page)
	void Close();  // hide the menu

	// Menu pages (the original's separate sub-panels, here as toggled button sets).
	enum Page { PAGE_CONFIG = 0, PAGE_CATEGORY, PAGE_ACCESSORY, PAGE_COUNT };
	void ShowPage(Page page); // switch which page's buttons are visible

	bool IsUp() const { return m_up; }
	// Fire the visible page's button bound to this number key ('0'-'9'). Returns true if consumed.
	// The original menu accepts BOTH mouse-click and the bound number key (reference/MENUS.md);
	// the viewport only routes keys to its own command menu, so this panel handles its own.
	bool HandleKey(int key);

private:
	void BuildConfig(int wide);
	void BuildCategory(int wide);
	void BuildAccessory(int wide);
	CommandButton** CurrentPage(int& count);

	enum { TS_MAX_BUTTONS = 10 };
	CommandButton* m_pConfig[TS_MAX_BUTTONS];
	CommandButton* m_pCategory[TS_MAX_BUTTONS];
	CommandButton* m_pAccessory[TS_MAX_BUTTONS];
	int m_nConfig = 0, m_nCategory = 0, m_nAccessory = 0;
	int m_rowH = 0;
	Page m_page = PAGE_CONFIG;
	bool m_up = false;
};
