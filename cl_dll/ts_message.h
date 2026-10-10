/***
*  ts_message.h — the TS notices: the server's "TSMessage" (R, G, B, flags, hold, string) parsed by
*  client.dll's 0x1004c0a0 into its sixteen-slot queue (0x1004b520) and drawn by 0x1004af40. The buy
*  menu's notices go through the same queue.
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

	// Queue a notice (0x1004b520): flags 1 top, 8 bottom, else the middle; 2 left, 4 right, else
	// centred; 0x10 replaces a live notice with the same place and flags instead of stacking. hold in
	// tenths of a second. r, g, b 0 draws in the HUD colour.
	void Add(const char* text, int r, int g, int b, int holdtime, int flags);

private:
	enum { MAX_MSG = 16 };
	struct Msg
	{
		float die;  // +0x70 when it goes (0 = free)
		int r, g, b; // +0x74
		char text[128]; // +0x80
		int x, y;   // +0x100, +0x104
		int flags;  // +0x108
	};
	Msg m_msgs[MAX_MSG] = {};
};
