/***
*  ts_gamerules.cpp — RECONSTRUCTED CTSGameRules (skeleton + GetRandomPowerup).
*  Functional/source-faithful (see CLAUDE.md). recovered/server/CTSGameRules.c.
***/
#include "extdll.h"
#include "util.h"
#include "cbase.h"
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
