/***
*  ts_gamerules.cpp — RECONSTRUCTED CTSGameRules (skeleton + GetRandomPowerup).
*  Functional/source-faithful (see CLAUDE.md). recovered/server/CTSGameRules.c.
***/
#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "player.h"
#include "gamerules.h"
#include "client.h"     // respawn()
#include "ts_gamerules.h"
#include "ts_utils.h"   // TSHudMessageAll

// DAT_0017da25 in the original: a cached flag that, when nonzero, removes the
// low-gravity (0x100) powerup from the random pool. Zero-init in the binary, so
// low-gravity is permitted by default. VERIFY: which cvar feeds this.
static byte g_iNoLowGravPowerup = 0;

// @ GetRandomPowerup__12CTSGameRulesi — collect the permitted powerup bits from
// `mask` and return a random one (0 if the pool is empty). Faithful to the .so:
// the 0x80 slot is always skipped, and 0x100 (low gravity) is gated.
int CTSGameRules::GetRandomPowerup(int mask)
{
	int list[9];
	int count = 0;

	for (int bit = 1; bit <= TS_PWUP_LOWGRAV; bit <<= 1)
	{
		if ((mask & bit) == 0)
			continue;
		if (bit == 0x80) // unused powerup slot
			continue;
		if (bit == TS_PWUP_LOWGRAV && g_iNoLowGravPowerup != 0)
			continue;
		list[count++] = bit;
	}

	if (count == 0)
		return 0;

	return list[RANDOM_LONG(0, count - 1)];
}

// Defined in ts_powerup.cpp — create a temporary powerup of `type` at `vecOrigin`.
CBaseEntity *TSCreatePowerUpAt(int type, const Vector &vecOrigin);
// Defined in ts_corpse.cpp / ts_gun.cpp — the TS death effects.
CBaseEntity *TSCreateDeadCorpse(CBasePlayer *pPlayer, float lifetime);
void TSDropWeaponsOnDeath(CBasePlayer *pPlayer);
void TSDropAmmoOnDeath(CBasePlayer *pPlayer);

// @ PlayerKilled — run the inherited MP scoring/death-notice, then the TS death effects:
// scatter the dead player's weapons into the world + leave a fading corpse.
void CTSGameRules::PlayerKilled(CBasePlayer *pVictim, entvars_t *pKiller, entvars_t *pInflictor)
{
	CHalfLifeMultiplay::PlayerKilled(pVictim, pKiller, pInflictor);
	TSDropWeaponsOnDeath(pVictim);
	TSDropAmmoOnDeath(pVictim);
	TSCreateDeadCorpse(pVictim, 15.0f);
}

// @ TeamWin — record the win + announce it, then schedule a round restart 3s out (the .so
// uses param_1+0xc = time+3). // SLICE: faithful TeamWin also awards victory/defeat cash to
// each team + drives the round-time HUD; here it's the score + restart.
void CTSGameRules::TeamWin(int team)
{
	if (team >= 0 && team < 4)
		m_teamScore[team]++;

	const char *who = (team == 0) ? "Team 1" : (team == 1) ? "Team 2"
		: (team == 3) ? "Team 3" : (team == 4) ? "Team 4" : "Nobody";
	TSHudMessageAll(0, 0xff, 0, 8, 50,
		UTIL_VarArgs("%s wins the round!  (%d - %d)", who, m_teamScore[0], m_teamScore[1]));

	m_flRoundRestart = gpGlobals->time + 3.0f;
}

// Re-spawn everyone + re-arm the objective entities for a fresh round.
static void TSRestartRound()
{
	for (int i = 1; i <= gpGlobals->maxClients; i++)
	{
		CBasePlayer *p = (CBasePlayer *)UTIL_PlayerByIndex(i);
		if (p)
			respawn(p->pev, false);
	}
	CBaseEntity *e = nullptr;
	while ((e = UTIL_FindEntityByClassname(e, "ts_wingiver")) != nullptr)
		e->Spawn();
	e = nullptr;
	while ((e = UTIL_FindEntityByClassname(e, "ts_hack")) != nullptr)
		e->Spawn();
	TSHudMessageAll(0, 0xff, 0, 8, 30, "Round restart!");
}

// @ PutPowerUp__12CTSGameRulesi — spawn a powerup at a random info_player_deathmatch
// spawn point. Skips the spot if a ts_powerup is already nearby (so dynamically-spawned
// powerups don't stack). Returns true if one was created.
bool CTSGameRules::PutPowerUp(int type)
{
	// Collect the deathmatch spawn points (the .so caps at 32).
	CBaseEntity *spawns[32];
	int count = 0;
	CBaseEntity *pSpot = nullptr;
	while (count < 32 &&
		(pSpot = UTIL_FindEntityByClassname(pSpot, "info_player_deathmatch")) != nullptr)
		spawns[count++] = pSpot;

	if (count == 0)
		return false;

	Vector vecOrigin = spawns[RANDOM_LONG(0, count - 1)]->pev->origin + Vector(0, 0, 16);

	// Don't pile powerups onto a spot that already has one.
	CBaseEntity *pNear = nullptr;
	while ((pNear = UTIL_FindEntityInSphere(pNear, vecOrigin, 128.0f)) != nullptr)
		if (FClassnameIs(pNear->pev, "ts_powerup"))
			return false;

	return TSCreatePowerUpAt(type, vecOrigin) != nullptr;
}

// @ CTSGameRules::Think — run the inherited MP think, then (in non-team deathmatch) drive
// the dynamic powerup spawner: roll a random powerup every ~20s and PutPowerUp it (retry in
// 1s if no spawn point was free). Mirrors the .so's Think powerup block.
void CTSGameRules::Think()
{
	CHalfLifeMultiplay::Think();

	// A scheduled round restart (from TeamWin) fires once its delay elapses.
	if (m_flRoundRestart != 0 && gpGlobals->time >= m_flRoundRestart)
	{
		m_flRoundRestart = 0;
		TSRestartRound();
	}

	if (gpGlobals->time < m_flNextPowerupSpawn)
		return;

	// Only deathmatch maps get dynamic powerups. IsTeamplay() now resolves to the team-mode
	// proxy (a ts_objective_manager present — see CTSGameRules::IsTeamMode), so the old
	// explicit objective-manager scan is folded into it.
	if (IsTeamplay())
	{
		m_flNextPowerupSpawn = gpGlobals->time + 20.0f;
		return;
	}

	int type = GetRandomPowerup(TS_PWUP_ALL);
	bool spawned = type != 0 && PutPowerUp(type);
	m_flNextPowerupSpawn = gpGlobals->time + (spawned ? 20.0f : 1.0f);
}

// @ GlobalSlowMotion2__12CTSGameRulesfb — apply (or clear, scale 0) a global slow-
// motion to every player. Throttled to ~0.5s except when turning off.
// SLICE: the original also slows live "grenade"/"WorldGun" projectiles; deferred
// (the per-entity GoSlow virtual on CBaseEntity isn't built — only CBasePlayer has it).
void CTSGameRules::GlobalSlowMotion2(float scale, int slowBullets)
{
	if (m_flNextGlobalSlow > gpGlobals->time && scale != 0.0f)
		return;
	m_flNextGlobalSlow = gpGlobals->time + 0.5f;

	for (int i = 1; i <= gpGlobals->maxClients; i++)
	{
		CBaseEntity *pEnt = UTIL_PlayerByIndex(i);
		if (!pEnt || !pEnt->IsPlayer())
			continue;
		CBasePlayer *pPlayer = (CBasePlayer *)pEnt;
		if (!pPlayer->InSlow())
		{
			pPlayer->GoSlow(scale, 0);
			pPlayer->m_bSlowBullets = slowBullets != 0;
		}
		if (scale == 0.0f)
			pPlayer->GoSlow(1.0f, 0); // restore
	}
}

// @ SetGlobalSlowMotion__12CTSGameRulesff — start/stop a timed global slow-motion.
void CTSGameRules::SetGlobalSlowMotion(float scale, float duration)
{
	if (scale == m_flGlobalSlowScale)
		return;

	if (scale == 0.0f)
	{
		m_flGlobalSlowActive = 0;
		return;
	}

	GlobalSlowMotion2(0, 0); // reset everyone first
	m_flNextGlobalSlow = gpGlobals->time;
	m_flGlobalSlowScale = scale;
	m_flGlobalSlowActive = 1.0f;
	m_flGlobalSlowEnd = (duration == 0.0f) ? 0.0f : duration + gpGlobals->time;
}
