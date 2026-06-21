/***
*  ts_gamerules.h — RECONSTRUCTED CTSGameRules (skeleton) from ts_i386.so.
*  Functional/source-faithful (see CLAUDE.md).
*
*  The original CTSGameRules is large (41 funcs) and deeply entangled with TS
*  additions to CBasePlayer (slow-motion, powerups, teams, objectives, "the one"
*  / last-man-standing modes). This is the foundation stone: a real, installable
*  rules object that the rest of the TS code hangs off. It currently derives all
*  round/team/objective behaviour from CHalfLifeMultiplay; TS-specific methods are
*  filled in incrementally (each verified) as their dependencies land.
***/
#ifndef TS_GAMERULES_H
#define TS_GAMERULES_H

#include "gamerules.h"

// TS powerup type bits — used both as ts_powerup spawnflags and as the mask for
// GetRandomPowerup (cf. ts_fgd.fgd ts_powerup spawnflags).
#define TS_PWUP_SLOWMO    (1 << 0) // 1
#define TS_PWUP_INFCLIP   (1 << 1) // 2
#define TS_PWUP_KUNGFU    (1 << 2) // 4
#define TS_PWUP_SLOWPAUSE (1 << 3) // 8
#define TS_PWUP_DBLFIRE   (1 << 4) // 16
#define TS_PWUP_GRENADE   (1 << 5) // 32
#define TS_PWUP_HEALTH    (1 << 6) // 64
// (1 << 7) == 0x80 is an unused slot — never selectable.
#define TS_PWUP_LOWGRAV   (1 << 8) // 0x100
#define TS_PWUP_ALL       0x1ff

class CTSGameRules : public CHalfLifeMultiplay
{
public:
	// Pick a random permitted powerup bit out of `mask`, or 0 if none qualify.
	int GetRandomPowerup(int mask);

	// Per-frame: drives the dynamic deathmatch powerup spawner (PutPowerUp) on top of the
	// inherited CHalfLifeMultiplay round/spawn logic.
	void Think() override;

	// @ PutPowerUp(type) — spawn a powerup of `type` at a random info_player_deathmatch
	// (skipped if a powerup is already nearby). Returns true if one was spawned.
	bool PutPowerUp(int type);

	float m_flNextPowerupSpawn = 0; // next dynamic-powerup time (DM mode; member @+4 in .so)

	// Global ("all map") slow-motion. GlobalSlowMotion2 applies `scale` to every
	// player each call (throttled ~0.5s; scale 0 = restore). SetGlobalSlowMotion is
	// the timed wrapper (cmd_globalslow / the ts_slowmotion "All map" mode).
	void GlobalSlowMotion2(float scale, int slowBullets);
	void SetGlobalSlowMotion(float scale, float duration);

	float m_flNextGlobalSlow = 0;   // 0x56c — GlobalSlowMotion2 throttle
	float m_flGlobalSlowActive = 0; // 0x560 — SetGlobalSlowMotion: active flag
	float m_flGlobalSlowScale = 0;  // 0x564 — current global slow scale
	float m_flGlobalSlowEnd = 0;    // 0x568 — global slow end time (0 = no limit)
};

// In this mod the active rules object is always a CTSGameRules (see
// InstallGameRules). Mirrors the original's global `TSGameRules` instance.
inline CTSGameRules* TSGameRules()
{
	return static_cast<CTSGameRules*>(g_pGameRules);
}

#endif // TS_GAMERULES_H
