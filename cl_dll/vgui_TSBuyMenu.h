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
	// PAGE_WEAPONS is the per-category weapon list (the original's +0x2d8c panel, FUN_100108b0).
	// PAGE_WEAPON_DETAIL is the per-weapon attachment submenu (the +0x2d90 panel, FUN_10010bd0):
	// Buy + the weapon's available attachments + Buy Akimbo / Full Load + Cancel.
	enum Page { PAGE_CONFIG = 0, PAGE_CATEGORY, PAGE_ACCESSORY, PAGE_WEAPONS, PAGE_WEAPON_DETAIL, PAGE_COUNT };
	void ShowPage(Page page); // switch which page's buttons are visible

	// Rebuild + show the weapon list for a buy category (0..4 = Handguns/SMGs/Rifles/Shotguns/Other).
	// RE'd from FUN_100108b0: iterates the DAT_1007a950[cat][9] weapon-id table, drops -1 / restricted
	// / (M61 in non-team mode), labels each "%i %s" (index+1, display name), ends with "0 Cancel".
	void ShowCategory(int cat);
	// A weapon-list slot was selected (mouse or key): open its attachment submenu, or (Cancel) go back.
	void OnWeaponSlot(int slot);
	// Rebuild + show the per-weapon attachment submenu for a weapon (RE'd from FUN_10010bd0): Buy +
	// the weapon's available attachments (support 0xE0 & ~exclude 0xE4, minus already-selected) +
	// Buy Akimbo (0xC8 != -1) or Full Load (0xDC) + Cancel.
	void ShowWeaponDetail(int weaponId);
	// A detail-submenu slot was selected: toggle an attachment, buy (weapon/akimbo/full), or go back.
	void OnDetailSlot(int slot);

	bool IsUp() const { return m_up; }
	// Fire the visible page's button bound to this number key ('0'-'9'). Returns true if consumed.
	// The original menu accepts BOTH mouse-click and the bound number key (reference/MENUS.md);
	// the viewport only routes keys to its own command menu, so this panel handles its own.
	bool HandleKey(int key);

private:
	void BuildConfig(int wide);
	void BuildCategory(int wide);
	void BuildAccessory(int wide);
	void BuildWeapons(int wide);
	void BuildDetail(int wide);
	CommandButton** CurrentPage(int& count);

	enum { TS_MAX_BUTTONS = 10 };
	CommandButton* m_pConfig[TS_MAX_BUTTONS];
	CommandButton* m_pCategory[TS_MAX_BUTTONS];
	CommandButton* m_pAccessory[TS_MAX_BUTTONS];
	CommandButton* m_pWeapon[TS_MAX_BUTTONS]; // reusable slots, rebuilt per category
	CommandButton* m_pDetail[TS_MAX_BUTTONS]; // reusable slots, rebuilt per weapon (attachment submenu)
	int m_weaponId[TS_MAX_BUTTONS];           // tsweapon_t for each slot (-1 = the Cancel/back row)
	int m_detailAction[TS_MAX_BUTTONS];       // per detail slot: -1 Cancel, -2 Buy, -3 Akimbo, -4 Full, 0-3 toggle attach bit
	vgui::Panel* m_pBg = nullptr;             // the faint menu box behind the entries (resized per page)
	int m_nConfig = 0, m_nCategory = 0, m_nAccessory = 0, m_nWeaponShown = 0, m_nDetailShown = 0;
	int m_currentCategory = 0;                // category of the active weapon list (for back-nav)
	int m_detailWeapon = -1, m_selectedAttach = 0; // the weapon being configured + its selected attachment mask
	int m_rowH = 0, m_wide = 0, m_mx = 0, m_my = 0; // box width + menu origin (offset in the full-screen panel)
	Page m_page = PAGE_CONFIG;
	bool m_up = false;
};
