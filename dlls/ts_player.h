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

// Kill-flavor globals (original .data @0x104480/81/82) — melee/coldcock/headshot kill blow
// markers driving the death-notice weapon string + #TS_Coldcock HUD broadcast. ts_player.cpp.
extern bool glast_closecombat;
extern bool glast_coldcock;
extern bool glast_headshot;

// The TS close-combat damage marker bit riding TraceAttack's bitsDamageType alongside
// DMG_CLUB|DMG_NEVERGIB (RE CloseCombatAttack @0x8646c: bits 0x4001080). Shared by
// CloseCombatAttack (ts_player.cpp) and the run-through resolver (ts_stunt.cpp).
#define DMG_TSCLOSECOMBAT 0x4000000

// "KFuPower" (1, CHAR 0..100) — the melee stamina bar; sent ONLY from CBasePlayer::UpdateMeleeBar.
extern int gmsgKFuPower;

// Slow-motion "bullets as models" — see CBulletManager in ts_bullet.h (g_BulletManager).
// The fire path calls g_BulletManager.FireBullet; gamerules Think calls g_BulletManager.Think.

// @ ClientCommand `powerup <N>` apply (RE: ts_i386.so ClientCommand 0x9db8a..0x9dd42).
// Grant a powerup directly (the TS debug cheat). Instant bits apply at once; timed bits are
// held with a `duration`-second timer (extended if the same type is already held), guarded by
// no powerup currently active. Mirrors PlayerAddPowerUp's per-bit dispatch (ts_powerup.cpp).
bool TSGrantPowerup(CBasePlayer *pPlayer, int type, int duration);

// TS powerup HUD/effect user messages (registered lazily, like gmsgTSMessage).
extern int gmsgTSPowerUpTime; // remaining-seconds bar for the active powerup
extern int gmsgTSMotionBlur;  // slow-motion motion-blur effect (per player)

#endif // TS_PLAYER_H
