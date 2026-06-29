/***
*  ts_cvars.cpp — RECONSTRUCTED TS server game cvars.
*
*  The original registers a set of game cvars (mp_teammodels, roundtime, slowmatch,
*  the cash economy, the-one mode, …) whose names + defaults were recovered from the
*  cvar_t structs in reference/ts_i386.so (see reference/API_SURFACE.md). Registering
*  them stops "Unknown command" when a server config sets them, and exposes the knobs
*  for the game-rules code to read. Behaviour wiring (where not already hooked) is
*  follow-on; this is the registration surface.
***/
#include "extdll.h"
#include "util.h"
#include "cbase.h"

// name, default, flags — byte-faithful to the .so cvar_t structs.
static cvar_t ts_mp_teammodels = {"mp_teammodels", "", FCVAR_SERVER};
static cvar_t ts_initial_cash = {"initial_cash", "1200", FCVAR_SERVER};
static cvar_t ts_cashforwin = {"cashforwin", "10000", FCVAR_SERVER};
static cvar_t ts_cashforlose = {"cashforlose", "1200", FCVAR_SERVER};
static cvar_t ts_roundtime = {"roundtime", "150", FCVAR_SERVER};
static cvar_t ts_respawntime = {"respawntime", "3", FCVAR_SERVER};
// Dropped-weapon lifetime in seconds (CTSWorldGun::Spawn reads it, clamps [5,60]). The original registers
// "weaponstay" default 15 — distinct from stock "mp_weaponstay" (0). Ours read the unregistered name -> 0 ->
// the 5s floor, so dropped guns vanished early; default 15 restores the original 15s persistence.
static cvar_t ts_weaponstay = {"weaponstay", "15", FCVAR_SERVER};
static cvar_t ts_damagemult = {"damagemult", "0.0", FCVAR_SERVER};
static cvar_t ts_slowmatch = {"slowmatch", "1.0", FCVAR_SERVER};
static cvar_t ts_nopowerup = {"nopowerup", "0.0", FCVAR_SERVER};
static cvar_t ts_botlearn = {"botlearn", "0.0", FCVAR_SERVER};
static cvar_t ts_realbullet = {"realbullet", "0.0", FCVAR_SERVER};
static cvar_t ts_usecash = {"usecash", "0.0", FCVAR_SERVER};
static cvar_t ts_ammocount = {"ammocount", "0", FCVAR_SERVER};
static cvar_t ts_bottalk = {"bottalk", "1", FCVAR_SERVER};
static cvar_t ts_mp_theonemode = {"mp_theonemode", "0", FCVAR_SERVER};
static cvar_t ts_mp_theone = {"mp_theone", "gordon", FCVAR_SERVER};
static cvar_t ts_mp_theothers = {"mp_theothers", "seal", FCVAR_SERVER};
static cvar_t ts_lastmanstanding = {"lastmanstanding", "0", FCVAR_SERVER};
static cvar_t ts_spectacularness = {"spectacularness", "1", FCVAR_SERVER};
static cvar_t ts_respawnnumber = {"respawnnumber", "1", FCVAR_SERVER};
static cvar_t ts_customweapons = {"customweapons", "", FCVAR_SERVER};
static cvar_t ts_banfortk = {"banfortk", "0", FCVAR_SERVER};
static cvar_t ts_dmgreport = {"dmgreport", "0", FCVAR_SERVER};
static cvar_t ts_botarmor = {"botarmor", "0", FCVAR_SERVER};
static cvar_t ts_deadstomach = {"deadstomach", "deadtable", FCVAR_SERVER};

void TSRegisterCvars()
{
	CVAR_REGISTER(&ts_mp_teammodels);
	CVAR_REGISTER(&ts_initial_cash);
	CVAR_REGISTER(&ts_cashforwin);
	CVAR_REGISTER(&ts_cashforlose);
	CVAR_REGISTER(&ts_roundtime);
	CVAR_REGISTER(&ts_respawntime);
	CVAR_REGISTER(&ts_weaponstay);
	CVAR_REGISTER(&ts_damagemult);
	CVAR_REGISTER(&ts_slowmatch);
	CVAR_REGISTER(&ts_nopowerup);
	CVAR_REGISTER(&ts_botlearn);
	CVAR_REGISTER(&ts_realbullet);
	CVAR_REGISTER(&ts_usecash);
	CVAR_REGISTER(&ts_ammocount);
	CVAR_REGISTER(&ts_bottalk);
	CVAR_REGISTER(&ts_mp_theonemode);
	CVAR_REGISTER(&ts_mp_theone);
	CVAR_REGISTER(&ts_mp_theothers);
	CVAR_REGISTER(&ts_lastmanstanding);
	CVAR_REGISTER(&ts_spectacularness);
	CVAR_REGISTER(&ts_respawnnumber);
	CVAR_REGISTER(&ts_customweapons);
	CVAR_REGISTER(&ts_banfortk);
	CVAR_REGISTER(&ts_dmgreport);
	CVAR_REGISTER(&ts_botarmor);
	CVAR_REGISTER(&ts_deadstomach);
}
