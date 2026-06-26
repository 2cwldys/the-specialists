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

// Slow-motion "bullets as models" (ts_bulletmgr.cpp / CBulletManager). When the shooter is slowed
// (slow factor < 0.99) the fire path registers each pellet as a flying bullet; TSUpdateSlowBullets
// (per frame from gamerules Think) flies + traces + damages them; they vanish when slow-mo ends.
void TSFireSlowBullet(CBasePlayer *owner, const Vector &src, const Vector &dir, float damage, int caliber);
void TSUpdateSlowBullets();
void TSResetSlowBullets();

// @ ClientCommand `powerup <N>` apply (RE: ts_i386.so ClientCommand 0x9db8a..0x9dd42).
// Grant a powerup directly (the TS debug cheat). Instant bits apply at once; timed bits are
// held with a `duration`-second timer (extended if the same type is already held), guarded by
// no powerup currently active. Mirrors PlayerAddPowerUp's per-bit dispatch (ts_powerup.cpp).
bool TSGrantPowerup(CBasePlayer *pPlayer, int type, int duration);

// TS powerup HUD/effect user messages (registered lazily, like gmsgTSMessage).
extern int gmsgTSPowerUpTime; // remaining-seconds bar for the active powerup
extern int gmsgTSMotionBlur;  // slow-motion motion-blur effect (per player)

#endif // TS_PLAYER_H
