/***
*  ts_func_break.h — CInfoBreakable (TS_func_break.cpp), the breakable RESPAWN retry-helper.
*  Created + cross-linked by CBreakable::Spawn (when the breakable has a "respawntime"); its
*  RespawnThink (armed by CBreakable::Die) clears its think and calls the held breakable's
*  BreakableRespawn(false) — the box-clear retry. The helper PERSISTS (reused each death).
***/
#ifndef TS_FUNC_BREAK_H
#define TS_FUNC_BREAK_H

class CInfoBreakable : public CBaseDelay
{
public:
	void EXPORT RespawnThink(); // @0x65da0 — delegate to the breakable's BreakableRespawn(false)

	EHANDLE m_hBreakable; // +0x64 — the breakable this helper respawns
};

#endif // TS_FUNC_BREAK_H
