/***
*  ts_weapon_predict.cpp — the CLIENT active-weapon param cache + the draw (deploy)
*  event handler. See reference/EVENTS.md and ts_weapon_predict.h.
*
*  This is step 2 of the predicted-weapon-event subsystem: the active-weapon state the
*  predicted handlers (fire/reload/…) read. It is refreshed on every TS weapon event
*  (mirroring the original, where each handler caches its args at entry), and seeded on
*  deploy by the draw.sc event (the original FUN_100425b0).
***/
#include "hud.h"
#include "cl_util.h"
#include "const.h"
#include "entity_state.h"
#include "cl_entity.h"
#include "event_api.h"
#include "event_args.h"
#include "eventscripts.h"
#include "ts_weapon_predict.h"

// Advance the client-predicted active weapon to the deployed type (defined in hl_weapons.cpp,
// where the CTSGun g_TSGun + its server-side headers are already in scope). Used by EV_TSDraw so
// the predicted viewmodel body is right on the deploy frame. cont.89 akimbo dossier §3.
extern void TS_SetPredictedWeaponType(int type);

TSActiveWeapon g_tsActive;

void TSActiveWeapon::UpdateFromEvent(event_args_t* a)
{
	type = a->iparam1;
	info = get_weapon_info((tsweapon_t)type);
	barrelMask = a->iparam2;
	entindex = a->entindex;
	VectorCopy(a->origin, origin);
	VectorCopy(a->angles, angles);
	VectorCopy(a->velocity, velocity);
	isLocal = EV_IsLocal(entindex);
}

// events/draw.sc → EV_TSDraw — the original FUN_100425b0. On deploy the server plays this
// with iparam1 = the tsweapon_t + iparam2 = the active barrel mask; the client seeds the
// active-weapon cache so the predicted fire/reload handlers have the right weapon state
// before the first trigger pull. (The viewmodel/player model + the deploy anim are shown by
// the server-side DefaultDeploy; this seeds the predicted-event state only.)
void EV_TSDraw(event_args_t* args)
{
	g_tsActive.UpdateFromEvent(args);

	// Mirror the original draw.sc handler (client.dll FUN_100425b0): on the deploy frame, advance the
	// PREDICTED active weapon type to the deployed weapon so view.cpp's per-frame
	// TS_PredictedViewmodelBody asserts the akimbo/attachment BODY AT ONCE — it otherwise lags a
	// stale m_weaponType until the vuser3.x reconcile (the single->akimbo body flicker). cont.89
	// akimbo dossier §3. Local player only (the viewmodel is the local player's).
	if (EV_IsLocal(args->entindex))
		TS_SetPredictedWeaponType(args->iparam1);
}
