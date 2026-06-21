/***
*  ts_utils.h — shared helpers for the reconstructed TS (The Specialists) code.
*  RECONSTRUCTED from ts_i386.so. See CLAUDE.md (functional/source-faithful).
***/
#ifndef TS_UTILS_H
#define TS_UTILS_H

// Registered user message "TSMessage" — a centred HUD text shown to all players.
extern int gmsgTSMessage;

// @ TSHudMessageAll__FUcUcUciiPc — broadcast a coloured HUD message to everyone.
// params (r,g,b) = colour; effect/holdtime are written as bytes (VERIFY exact
// client-side semantics); text is capped to 127 chars by the original.
void TSHudMessageAll(byte r, byte g, byte b, int effect, int holdtime, const char *text);

// @ TSHudMessageOne__FUcUcUciiPcP9entvars_s — same payload as TSHudMessageAll but
// sent MSG_ONE to a single player (pPlayer = that player's pev).
void TSHudMessageOne(byte r, byte g, byte b, int effect, int holdtime, const char *text, entvars_t *pPlayer);

#endif // TS_UTILS_H
