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

// @0x8fae0 PlayerIsCheating — bots never cheat; a human cheats iff their cl_lw != "1"
// (TS_util.cpp; called from TSInit with the player's info key buffer).
class CBasePlayer;
bool PlayerIsCheating(CBasePlayer *pPlayer, char *infobuffer);

// @0x8e944 — linear find over a packed entity-pointer array; -1 if absent (TS_util.cpp).
class CBaseEntity;
int UTIL_EntInArray(CBaseEntity **pArray, int n, CBaseEntity *pEnt);
// @0x8fb4c — scrub a dying entity out of every player's packed slowed array (+0x208/+0x288).
// Call before UTIL_Remove on any slowable projectile (flag/worldgun/knife/grenade/shrapnel).
void RemoveEntityFromSlowedArray(CBaseEntity *pEnt);
// @0x8f94c — the leaver teardown (vote-block clear + edict park + CallCheckDeads); the sole
// caller is ClientDisconnected (TS_util.cpp).
void TSPlayerDisconnect(CBasePlayer *pPlayer);

#endif // TS_UTILS_H
