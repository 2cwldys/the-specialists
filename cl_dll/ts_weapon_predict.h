/***
*  ts_weapon_predict.h — the CLIENT active-weapon param cache for the predicted TS
*  weapon events (.sc). See reference/EVENTS.md.
*
*  In the original, every TS weapon-event handler (draw/fire/reload/…) caches its
*  event args into a block of DAT_10189xxx globals at entry (active type, the
*  get_weapon_info() ptr, the barrel mask, the entindex, the origin/angles/velocity)
*  and then reproduces the predicted viewmodel anim / sound / effects from them. The
*  ~70 globals are the client's INTERNAL prediction state — NOT part of the protocol.
*  Per CLAUDE.md (behaviour + interface fidelity, not byte/offset) we model that state
*  as one clean struct refreshed by each handler at entry. The PROTOCOL we DO match is
*  the event arg packing (iparam1 = tsweapon_t, iparam2 = barrel mask, fparam1 = stage).
***/
#ifndef TS_WEAPON_PREDICT_H
#define TS_WEAPON_PREDICT_H

#include "ts_weapon_info.h"

struct event_args_s;

// Mirrors the original per-handler cache-fill (FUN_100425b0 draw / FUN_10042690 reload):
//   type     = iparam1            (DAT_10189d0c / DAT_10189ca0)
//   info     = get_weapon_info(type)  (DAT_10189ae4 / DAT_10189e3c)
//   barrelMask = iparam2          (DAT_10189ff4 / DAT_10189c98)
//   entindex = args->entindex     (DAT_10189f40 / _DAT_10189eb4)
//   origin/angles/velocity        (cached from args+8/+0x14/+0x20)
//   isLocal  = EV_IsLocal(entindex)
struct TSActiveWeapon
{
	int type = TSW_MELEE;
	const s_weapon_info* info = nullptr;
	int barrelMask = 0;
	int entindex = 0;
	float origin[3] = {0, 0, 0};
	float angles[3] = {0, 0, 0};
	float velocity[3] = {0, 0, 0};
	bool isLocal = false;

	// Refresh from a TS weapon event's args (call at the top of every handler).
	void UpdateFromEvent(struct event_args_s* a);
};

// The single client-side active-weapon cache, shared by every TS event handler.
extern TSActiveWeapon g_tsActive;

#endif // TS_WEAPON_PREDICT_H
