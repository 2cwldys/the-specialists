/***
*  ts_shared.cpp — RECONSTRUCTED TS slow-motion math (original TS_shared.cpp). Recovered (nm):
*    FixedSlowAmt @0x890bc, FixedSlowMul @0x890e4, InterpolateSlowMotion @0x8910c,
*    GetBulletVelocityMult @0x89160.
*
*  These were already implemented in our build but SCATTERED (GetBulletVelocityMult was a static in
*  ts_bullet.cpp; InterpolateSlowMotion was inlined in CBasePlayer::PreThink; FixedSlowAmt is the
*  max(s,0.01) clamp inlined in CBaseEntity::GiveSlowMul). Consolidated here as the original's free
*  functions so the layout matches; the call sites now route through these. Behaviour is unchanged.
***/
#include "extdll.h"
#include "ts_shared.h"

// @0x890bc — clamp the slow factor so it never reaches 0 (a 0 would stop time entirely / divide-by-0).
float FixedSlowAmt(float s)
{
	return s < 0.01f ? 0.01f : s;
}

// @0x890e4 — the inverse of the clamped slow factor (the "scale up" multiplier).
float FixedSlowMul(float s)
{
	return 1.0f / FixedSlowAmt(s);
}

// @0x8910c — ease *cur toward target by min(scale*7, 1) per call; snap when already within 0.05.
void InterpolateSlowMotion(float scale, float *cur, float target)
{
	float d = target - *cur;
	if ((d < 0.0f ? -d : d) > 0.05f)
	{
		float f = scale * 7.0f;
		if (f > 1.0f)
			f = 1.0f;
		d *= f;
	}
	*cur += d;
}

// @0x89160 — a slow-mo bullet's speed as a fraction of full speed, from the shooter's slow factor +
// the slow-PAUSE flag. Slow-mo (pause=0) → ~slow (a fast tracer); slow-pause (pause=1, the ×10) →
// much slower (dodgeable). Not slowed (≥0.99) → 1.0.
float GetBulletVelocityMult(float slow, int pause)
{
	if (slow >= 0.99f)
		return 1.0f;
	float denom = (1.0f - slow * slow) * (pause ? 10.0f : 1.0f);
	if (denom < 1.0f)
		denom = 1.0f;
	return slow / denom;
}
