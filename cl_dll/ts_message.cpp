/***
*  ts_message.cpp — The Specialists centred HUD notification. See ts_message.h.
*  Faithful to client.dll FUN_1004b520 (queue) / FUN_1004c0a0 (TSMessage parse) / the centred draw:
*  flags 0 => centred horizontally, anchored at screen centre (just below); messages stack down.
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

void CHudTSMessage::Add(const char* text, int r, int g, int b, int holdtime, int flags)
{
	if (!text || !text[0])
		return;
	// flags 0x10 (the spectate countdown class) = a SINGLE-SLOT message: a new one REPLACES the
	// live one in place instead of stacking (the 1 Hz "%i" lines otherwise pile up and bump the
	// queue down — round-7c). VERIFY vs FUN_1004b520's exact flag semantics when it gets RE'd.
	if ((flags & 0x10) != 0)
	{
		for (int i = 0; i < MAX_MSG; i++)
		{
			if (m_msgs[i].die > gHUD.m_flTime && (m_msgs[i].flags & 0x10) != 0)
			{
				Msg& mm = m_msgs[i];
				strncpy(mm.text, text, sizeof(mm.text) - 1);
				mm.text[sizeof(mm.text) - 1] = '\0';
				mm.r = r; mm.g = g; mm.b = b; mm.flags = flags;
				int h = holdtime < 0 ? -holdtime : holdtime;
				if (h <= 0 || h > 100) h = 30;
				mm.die = gHUD.m_flTime + h * 0.1f + 1.0f;
				return;
			}
		}
	}
	// find a free (or the oldest) slot
	int slot = -1;
	float oldest = 1e9f;
	for (int i = 0; i < MAX_MSG; i++)
	{
		if (m_msgs[i].die <= gHUD.m_flTime)
		{
			slot = i;
			break;
		}
		if (m_msgs[i].die < oldest)
		{
			oldest = m_msgs[i].die;
			slot = i;
		}
	}
	Msg& m = m_msgs[slot];
	strncpy(m.text, text, sizeof(m.text) - 1);
	m.text[sizeof(m.text) - 1] = '\0';
	m.r = r;
	m.g = g;
	m.b = b;
	m.flags = flags;
	// holdtime is the original's |dur|-clamped-to-100 field; show for a few seconds.
	int hold = holdtime < 0 ? -holdtime : holdtime;
	if (hold <= 0 || hold > 100)
		hold = 30;
	m.die = gHUD.m_flTime + hold * 0.1f + 1.0f;
}

bool CHudTSMessage::MsgFunc_TSMessage(const char* pszName, int iSize, void* pbuf)
{
	// wire: byte R, byte G, byte B, byte flags, byte holdtime, string text (dlls/ts_utils.cpp).
	BEGIN_READ(pbuf, iSize);
	int r = READ_BYTE();
	int g = READ_BYTE();
	int b = READ_BYTE();
	int flags = READ_BYTE();
	int hold = READ_BYTE();
	const char* text = READ_STRING();
	Add(text, r, g, b, hold, flags);
	return true;
}

bool CHudTSMessage::Draw(float flTime)
{
	int line = 0;
	for (auto& m : m_msgs)
	{
		if (m.die <= flTime)
			continue;

		// colour: a 0,0,0 message (the config-apply notices) draws in light text, like the original;
		// a server-coloured TSMessage uses its colour.
		int r = m.r, g = m.g, b = m.b;
		if ((r | g | b) == 0)
		{
			r = g = b = 230;
		}

		// Fade out as the message expires (the original's notify queue ramps the message down over its
		// remaining lifetime; +0x70 lifetime field). The TS HUD font draws additively, so scaling the
		// colour toward 0 fades the text to invisible. Ramp over the last second.
		float remaining = m.die - flTime;
		if (remaining < 1.0f)
		{
			float f = remaining < 0.0f ? 0.0f : remaining;
			r = (int)(r * f);
			g = (int)(g * f);
			b = (int)(b * f);
		}

		int w, h;
		GetConsoleStringSize(m.text, &w, &h);

		// position (FUN_1004b520): flags bit1=top(y=0x20), bit4=bottom; else centre. bit2=left(x=0x20),
		// bit3=right; else centred. The config notices use flags 0 -> centred, anchored at centre.
		int x, y;
		if (m.flags & 2)
			x = XRES(0x20);
		else if (m.flags & 4)
			x = ScreenWidth - w - XRES(0x20);
		else
			x = (ScreenWidth - w) / 2;
		if (m.flags & 1)
			y = YRES(0x20);
		else if (m.flags & 8)
			y = ScreenHeight - YRES(0x40);
		else
			y = ScreenHeight / 2;
		y += line * (h + 2); // stack multiple messages downward

		gHUD.DrawHudString(x, y, ScreenWidth, m.text, r, g, b);
		line++;
	}
	return true;
}
