/***
*  ts_func_break.cpp — RECONSTRUCTED CInfoBreakable (TS_func_break.cpp), the breakable RESPAWN
*  retry-helper. A TS func_breakable with a "respawntime" creates this helper in CBreakable::Spawn
*  (cross-linked by EHANDLE). When the breakable is destroyed, CBreakable::Die arms the helper's
*  RespawnThink (@0x65da0); it fires after the delay and delegates to the breakable's
*  CBreakable::BreakableRespawn(false) — the box-clear retry that re-spawns it (or re-arms this helper
*  in 5s if the spot is blocked). The helper PERSISTS for the breakable's life (reused each death).
***/
#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "func_break.h"     // CBreakable::BreakableRespawn
#include "ts_func_break.h"  // CInfoBreakable

LINK_ENTITY_TO_CLASS(info_breakable, CInfoBreakable);

// @ RespawnThink__7CInfoBreakable (@0x65da0) — clear our think and ask the held breakable to respawn.
// The box-clear check + restore + retry all live in CBreakable::BreakableRespawn; this helper is NOT
// removed (it stays armed for the breakable's next death).
void CInfoBreakable::RespawnThink()
{
	SetThink(NULL);
	CBaseEntity *pBreak = m_hBreakable;
	if (pBreak)
		pBreak->BreakableRespawn(false); // VIRTUAL (@0x65da0 calls vtbl slot +8) — the held
		                                 // entity is a CBreakable OR a CBaseDoor/CRotDoor
}
