/***
*  vgui_TSBuyMenu.cpp — The Specialists client-side buy/config menu (cont.69e).
*  See vgui_TSBuyMenu.h + reference/MENUS.md. First slice: the CONFIG menu (vgui::Panel of
*  CommandButtons with bound keys + StringCommand action signals -> the server menuselect handler).
***/
#include "VGUI_Panel.h"
#include "VGUI_App.h"

#include "hud.h"
#include "cl_util.h"
#include "vgui_TeamFortressViewport.h"
#include "vgui_TSBuyMenu.h"

using namespace vgui;

// Recovered config-menu entries (reference/MENUS.md, string table @0x10092808). Each fires the
// server menuselect handler (CBasePlayer::TSMenuSelect), which already maps these slots.
struct TSConfigEntry
{
	char key;          // bound number key / menuselect slot
	const char* label; // recovered label
	const char* cmd;   // server command fired on click/key
};
static const TSConfigEntry s_config[8] = {
	{'1', "Use New Config", "menuselect 1"},
	{'2', "Buy", "menuselect 2"},
	{'3', "Remove Item", "menuselect 3"},
	{'4', "Reset Config", "menuselect 4"},
	{'5', "Save Config", "menuselect 5"},
	{'6', "Load Config", "menuselect 6"},
	{'7', "Buy Random", "menuselect 7"},
	{'0', "Cancel", "menuselect 10"},
};

CTSBuyMenuPanel::CTSBuyMenuPanel(int x, int y, int wide, int tall)
	: Panel(x, y, wide, tall)
{
	setBgColor(0, 0, 0, 100); // faint background panel (the shared menu/HUD look)

	const int buttonH = YRES(24);
	const int spacer = YRES(2);
	for (int i = 0; i < NUM_CONFIG_BUTTONS; i++)
	{
		const int yPos = (buttonH + spacer) * i;
		m_pButtons[i] = new CommandButton(s_config[i].label, 0, yPos, wide, buttonH);
		m_pButtons[i]->setParent(this);
		m_pButtons[i]->setContentAlignment(vgui::Label::a_west);
		m_pButtons[i]->setBoundKey(s_config[i].key);
		m_pButtons[i]->addActionSignal(new CMenuHandler_StringCommand(s_config[i].cmd, true));
	}

	setVisible(false);
}

void CTSBuyMenuPanel::Open()
{
	setVisible(true);
}

void CTSBuyMenuPanel::Close()
{
	setVisible(false);
}
