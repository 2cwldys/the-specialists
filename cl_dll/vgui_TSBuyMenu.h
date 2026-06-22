/***
*  vgui_TSBuyMenu.h — The Specialists client-side buy/config menu (cont.69e).
*
*  Faithful reproduction of the TS buy menu, RE'd from client.dll (reference/MENUS.md): a
*  vgui::Panel of clickable text vgui::Button entries (NOT the MOTD gmsgVGUIMenu), selected by
*  mouse-click OR the bound number key, with the hierarchy Config -> Buy (weapon categories) ->
*  Player Accessory -> Kevlar/Lasersight/Silencer. Modelled on the SDK's CTeamMenuPanel.
*
*  Reproduction is incremental; this first slice is the CONFIG menu. Each entry is a CommandButton
*  with setBoundKey(number) + a CMenuHandler_StringCommand action signal — giving the original's
*  click-and-key selection. The config actions fire the existing server menuselect handler
*  (CBasePlayer::TSMenuSelect); the Buy/Accessory submenus + the apply (rst_cfg/tkw/tki/tka)
*  sequencer are the next slices.
***/
#pragma once

#include "VGUI_Panel.h"

class CommandButton;

// The config (top-level) buy menu panel. Owned + shown by the viewport during spectate-wait.
class CTSBuyMenuPanel : public vgui::Panel
{
public:
	CTSBuyMenuPanel(int x, int y, int wide, int tall);

	void Open();   // show the menu
	void Close();  // hide the menu

private:
	enum { NUM_CONFIG_BUTTONS = 8 };
	CommandButton* m_pButtons[NUM_CONFIG_BUTTONS];
};
