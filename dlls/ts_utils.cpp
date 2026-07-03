/***
*  ts_utils.cpp — shared helpers for the reconstructed TS code.
*  RECONSTRUCTED from ts_i386.so (functional/source-faithful — see CLAUDE.md).
***/
#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "ts_utils.h"

int gmsgTSMessage = 0;

// @ TSHudMessageAll__FUcUcUciiPc  (@ 0x769d4)
//   if (!text[0]) return;
//   strncpy(buf,text,0x80); buf[127]=0;
//   MESSAGE_BEGIN(MSG_ALL=2, gmsgTSMessage); WRITE_BYTE x5; WRITE_STRING(buf); MESSAGE_END();
void TSHudMessageAll(byte r, byte g, byte b, int effect, int holdtime, const char *text)
{
	if (!text || !text[0])
		return;

	char buf[128];
	strncpy(buf, text, sizeof(buf));
	buf[sizeof(buf) - 1] = '\0';

	MESSAGE_BEGIN(MSG_ALL, gmsgTSMessage); // registered centrally in LinkUserMessages ("TSMessage", -1)
		WRITE_BYTE(r);
		WRITE_BYTE(g);
		WRITE_BYTE(b);
		WRITE_BYTE(effect);
		WRITE_BYTE(holdtime);
		WRITE_STRING(buf);
	MESSAGE_END();
}

// @ TSHudMessageOne__FUcUcUciiPcP9entvars_s — MSG_ONE variant of the above.
void TSHudMessageOne(byte r, byte g, byte b, int effect, int holdtime, const char *text, entvars_t *pPlayer)
{
	if (!text || !text[0] || !pPlayer)
		return;

	char buf[128];
	strncpy(buf, text, sizeof(buf));
	buf[sizeof(buf) - 1] = '\0';


	MESSAGE_BEGIN(MSG_ONE, gmsgTSMessage, NULL, ENT(pPlayer));
		WRITE_BYTE(r);
		WRITE_BYTE(g);
		WRITE_BYTE(b);
		WRITE_BYTE(effect);
		WRITE_BYTE(holdtime);
		WRITE_STRING(buf);
	MESSAGE_END();
}
