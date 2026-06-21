/***
*  ts_gamerules.cpp — RECONSTRUCTED CTSGameRules (skeleton + GetRandomPowerup).
*  Functional/source-faithful (see CLAUDE.md). recovered/server/CTSGameRules.c.
***/
#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "player.h"
#include "gamerules.h"
#include "ts_gamerules.h"

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
