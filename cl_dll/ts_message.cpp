/***
*  ts_message.cpp — the TS notices. See ts_message.h.
***/

#include "hud.h"
#include "cl_util.h"
#include "parsemsg.h"
#include "ts_hud.h"
#include "ts_message.h"

#include <string.h>

DECLARE_MESSAGE(m_TSMessage, TSMessage)

bool CHudTSMessage::Init()
{
	HOOK_MESSAGE(TSMessage);
	gHUD.AddHudElem(this);
	m_iFlags |= HUD_ACTIVE; // always eligible to draw (it self-gates on queued messages)
	Reset();
	return true;
}

bool CHudTSMessage::VidInit()
{
	return true;
}

void CHudTSMessage::Reset()
{
	for (auto& m : m_msgs)
		m.die = 0;
}

// The TS font's width of a string up to maxX (client.dll 0x10021ca0).
static int TSStringWidth(int maxX, const char* str)
{
	int x = 0;
	for (const char* p = str; *p && *p != '\n'; p++)
	{
		const int w = gHUD.m_scrinfo.charWidths[(unsigned char)*p];
		if (x + w > maxX)
			break;
		x += w;
	}
	return x;
}

// client.dll 0x1004b520. The first free slot of the first fifteen, else the last when free, else
// the notice is dropped. The text is localised; a notice naming the local player is drawn grey
// (155, 155, 155). The place comes from the flags; then, against every live notice at the same place
// with the same flags, a 0x10 notice ends that one, any other moves 16 up (16 down at the top) and
// checks again, up to 16 times. It lasts hold x 0.1 s.
void CHudTSMessage::Add(const char* text, int r, int g, int b, int holdtime, int flags)
{
	int slot = 0;
	while (slot < MAX_MSG - 1 && !(m_msgs[slot].die < gHUD.m_flTime))
		slot++;
	if (gHUD.m_flTime < m_msgs[slot].die)
		return;
	Msg& m = m_msgs[slot];
	char buf[128];
	CHudTextMessage::LocaliseTextString(text, buf, sizeof(buf));
	strncpy(m.text, buf, sizeof(m.text) - 1);
	m.text[sizeof(m.text) - 1] = '\0';

	m.r = r;
	m.g = g;
	m.b = b;
	cl_entity_t* lp = gEngfuncs.GetLocalPlayer();
	if (lp != nullptr && lp->index >= 0 && g_PlayerInfoList[lp->index].name != nullptr &&
		strstr(m.text, g_PlayerInfoList[lp->index].name) != nullptr)
		m.r = m.g = m.b = 0x9b;

	if ((flags & 1) != 0)
		m.y = 0x20;
	else if ((flags & 8) != 0)
		m.y = ScreenHeight - 0x40;
	else
		m.y = ScreenHeight / 2;
	if ((flags & 2) != 0)
		m.x = 0x20;
	else if ((flags & 4) != 0)
		m.x = ScreenWidth - TSStringWidth(ScreenWidth, m.text) - 0x20;
	else
		m.x = ScreenWidth / 2 - TSStringWidth(ScreenWidth, m.text) / 2;
	m.flags = flags;

	for (int i = 0, moves = 0; i < MAX_MSG && moves <= 15; i++)
	{
		const Msg& o = m_msgs[i];
		if (!(gHUD.m_flTime < o.die) || o.y != m.y || o.flags != m.flags)
			continue;
		if ((flags & 0x10) != 0)
		{
			m_msgs[i].die = 0;
			break;
		}
		m.y += (m.flags & 1) != 0 ? 0x10 : -0x10;
		i = -1;
		moves++;
	}
	m.die = holdtime * 0.1f + gHUD.m_flTime;
}

// TSMessage (0x1004c0a0): byte r, g, b, flags, hold (its size held to 100), string.
bool CHudTSMessage::MsgFunc_TSMessage(const char* pszName, int iSize, void* pbuf)
{
	BEGIN_READ(pbuf, iSize);
	const int r = READ_BYTE();
	const int g = READ_BYTE();
	const int b = READ_BYTE();
	const int flags = READ_BYTE();
	int hold = READ_BYTE();
	if (hold < 1)
		hold = -hold;
	if (hold > 100)
		hold = 100;
	char text[128];
	strncpy(text, READ_STRING(), sizeof(text) - 1);
	text[sizeof(text) - 1] = '\0';
	Add(text, r, g, b, hold, flags);
	m_iFlags |= HUD_ACTIVE;
	return true;
}

// client.dll 0x1004af40, from the status draw after its nohud test: each live notice no further
// than 10 s from its end, in its colour (the HUD colour for 0, 0, 0) faded over its last second, in
// the TS font at its place.
bool CHudTSMessage::Draw(float flTime)
{
	if (TSHud_NoHud())
		return true;
	int hr, hg, hb;
	TSHud_Color(hr, hg, hb);
	for (auto& m : m_msgs)
	{
		if (!(gHUD.m_flTime < m.die))
			continue;
		if (m.die - gHUD.m_flTime > 10.0f)
			m.die = gHUD.m_flTime + 10.0f;
		const bool hud = m.r == 0 && m.g == 0 && m.b == 0;
		float k = m.die - gHUD.m_flTime;
		if (k >= 1.0f)
			k = 1.0f;
		const int r = (int)((hud ? hr : m.r) * k);
		const int g = (int)((hud ? hg : m.g) * k);
		const int b = (int)((hud ? hb : m.b) * k);
		TSHud_DrawString(m.x, m.y, ScreenWidth, m.text, r, g, b);
	}
	return true;
}
