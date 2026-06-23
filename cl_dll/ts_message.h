/***
*  ts_message.h — The Specialists centred HUD notification ("Using last config.", kill/award
*  notices, etc.). RE'd from client.dll: the server's "TSMessage" user message
*  (R,G,B,flags,holdtime,string — dlls/ts_utils.cpp) is parsed by FUN_1004c0a0 and queued into the
*  notify ring FUN_1004b520, drawn centred just below screen centre (flags 0). The buy menu's
*  config-apply notices ("Using new/last config.") go through the same path. reference/MENUS.md.
***/
#pragma once

class CHudTSMessage : public CHudBase
{
public:
	bool Init() override;
	bool VidInit() override;
	bool Draw(float flTime) override;
	void Reset() override;
	bool MsgFunc_TSMessage(const char* pszName, int iSize, void* pbuf);

	// Queue a notification (used by the buy menu for the config-apply notices). flags bit1=top,
	// bit2=left, bit3=right, bit4=bottom (FUN_1004b520); 0 = centred. r/g/b 0 => default light text.
	void Add(const char* text, int r, int g, int b, int holdtime, int flags);

private:
	enum { MAX_MSG = 8 };
	struct Msg
	{
		char text[128];
		int r, g, b, flags;
		float die; // client time at which it expires (0 = free)
	};
	Msg m_msgs[MAX_MSG] = {};
};
