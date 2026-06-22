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

// A TS-styled menu button. RE'd from client.dll: the TS menu entries are PLAIN vgui::Button
// objects (FUN_10066b40 — the vtable is all imported vgui::Button methods, NO paint override), so
// the TFC orange box comes only from the SDK's CommandButton::paintBackground. We keep
// CommandButton for its bound-key + action-signal plumbing but suppress that orange box and use the
// original's colours: text is GREEN (45,195,45) when the entry is available, RED (195,45,45) when
// not (FUN_10011160 computes this per-entry from affordability, stored at button+0xfc/0x100). The
// faint background is the menu panel itself (the same look as the status/kevlar HUD).
class CTSMenuButton : public CommandButton
{
public:
	CTSMenuButton(const char* text, int x, int y, int wide, int tall, bool available = true)
		: CommandButton(text, x, y, wide, tall, /*bNoHighlight*/ true)
	{
		// TS colours (FUN_10011160): available = green, unavailable = red.
		m_r = available ? 45 : 195;
		m_g = available ? 195 : 45;
		m_b = 45;
		setContentAlignment(vgui::Label::a_west);
	}

	// CommandButton::paint() forces the TFC orange scheme every frame; override to apply the TS
	// green/red and draw via the base vgui::Button (brighter when the cursor is over it).
	void paint() override
	{
		if (isArmed())
			setFgColor(V_min(255, m_r + 60), V_min(255, m_g + 60), V_min(255, m_b + 60), 0);
		else
			setFgColor(m_r, m_g, m_b, 0);
		vgui::Button::paint();
	}

	// Plain vgui::Button has no box — suppress CommandButton's orange paintBackground entirely.
	void paintBackground() override {}

private:
	int m_r, m_g, m_b;
};

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
		m_pButtons[i] = new CTSMenuButton(s_config[i].label, 0, yPos, wide, buttonH);
		m_pButtons[i]->setParent(this);
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
