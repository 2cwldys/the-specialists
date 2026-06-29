/***
*  ts_func_break.cpp — RECONSTRUCTED CInfoBreakable (TS_func_break.cpp), the breakable RESPAWN
*  retry-helper. When a TS func_breakable with a respawn time is destroyed, CBreakable::Die spawns a
*  CInfoBreakable holding an EHANDLE back to the breakable; its RespawnThink (RE @0x66ec0) fires after
*  the delay and asks the breakable to respawn — re-arming itself (e.g. +5s) if the spot is still blocked.
*
*  FIDELITY NOTE: the full feature is a cluster — CBreakable::BreakableRespawn (@0x74e84, the box-clear
*  check + reset+Spawn) and CBreakable::Die creating + arming this helper. Those touch the
*  playtest-confirmed stock CBreakable/glass-debris path, so they are a deliberate FOLLOW-UP (not wired
*  here). This file reconstructs CInfoBreakable itself (1:1 method) + its core respawn behaviour; it's
*  kept in the binary by the keepalive below until Die is wired to create it. VERIFY in-engine.
***/
#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "func_break.h"

class CInfoBreakable : public CBaseDelay
{
public:
	void EXPORT RespawnThink(); // @0x66ec0

	EHANDLE m_hBreakable; // +0x64 — the breakable this helper respawns
};

// @ RespawnThink__7CInfoBreakable — clear our think, then re-spawn the held breakable.
void CInfoBreakable::RespawnThink()
{
	SetThink(NULL);
	CBaseEntity *pBreak = m_hBreakable;
	if (pBreak)
	{
		// The original calls the breakable's respawn (vtable dispatch). Restore it to life and
		// re-run its Spawn so it reappears. (Full box-clear retry lives in CBreakable::BreakableRespawn.)
		pBreak->pev->deadflag = DEAD_NO;
		pBreak->pev->effects &= ~EF_NODRAW;
		pBreak->pev->takedamage = DAMAGE_YES;
		pBreak->Spawn();
	}
	UTIL_Remove(this);
}

// Keep CInfoBreakable in the binary (the original compiles it; ours has no caller until
// CBreakable::Die is wired to create it — the breakable-respawn FOLLOW-UP). Never called.
CBaseEntity *ts_func_break_keep()
{
	static CInfoBreakable c;
	return (CBaseEntity *)&c;
}
