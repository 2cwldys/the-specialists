/***
*  ts_player.h — RECONSTRUCTED TS player extensions (operate on CBasePlayer).
*  Functional/source-faithful (see CLAUDE.md).
*
*  TS has no CTSPlayer class — it bolts its state + behaviour onto the stock
*  CBasePlayer (see the mod-ts member block in player.h) plus free functions like
*  these. The accessors below mirror the originals (SlowMotion__FP11CBasePlayer
*  etc.), each a one-line test of the m_iPowerups bitfield / kung-fu byte.
***/
#ifndef TS_PLAYER_H
#define TS_PLAYER_H

class CBasePlayer;

// Powerup-state accessors (bits match TS_PWUP_* in ts_gamerules.h).
bool SlowMotion(CBasePlayer *pPlayer);     // @ 0x65fa8 — m_iPowerups & 1
bool InfiniteClips(CBasePlayer *pPlayer);  // @ 0x65fc8 — m_iPowerups & 2
bool SlowPause(CBasePlayer *pPlayer);      // @ 0x65fb8 — m_iPowerups & 8
bool DoubleFireRate(CBasePlayer *pPlayer); // @ 0x65fe4 — m_iPowerups & 0x10
int  KungFu(CBasePlayer *pPlayer);         // @ 0x65fd8 — m_iKungFu

#endif // TS_PLAYER_H
