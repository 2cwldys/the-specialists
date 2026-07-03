/***
*  ts_cvars.cpp — RECONSTRUCTED TS server game cvars.
*
*  The original registers a set of game cvars (mp_teammodels, roundtime, slowmatch,
*  the cash economy, the-one mode, …) whose names + defaults were recovered from the
*  cvar_t structs in reference/ts_i386.so (see reference/API_SURFACE.md). Registering
*  them stops "Unknown command" when a server config sets them, and exposes the knobs
*  for the game-rules code to read. Behaviour wiring (where not already hooked) is
*  follow-on; this is the registration surface.
*
*  Struct names + linkage match the original's .data globals (nm ts_i386.so @0x108e48..):
*  initial_cash/cashforwin/.../botarmor, plus the odd ones out: teammodels ("mp_teammodels"),
*  dmcashbased ("usecash"), cv_ammocount ("ammocount"), weaponstay_time ("weaponstay").
*  ("deadstomach" was an extraction FALSE POSITIVE — the string is a CDeadHEV/CDeadHGrunt
*  pose name, not a cvar; its lone .data pointer is inside CDeadHEV::m_szPoses.)
***/
#include "extdll.h"
#include "util.h"
#include "cbase.h"

// name, default, flags — byte-faithful to the .so cvar_t structs (defaults verified @0x108d80..0x109028).
cvar_t teammodels = {"mp_teammodels", "", FCVAR_SERVER};
cvar_t initial_cash = {"initial_cash", "1200", FCVAR_SERVER};
cvar_t cashforwin = {"cashforwin", "10000", FCVAR_SERVER};
cvar_t cashforlose = {"cashforlose", "1200", FCVAR_SERVER};
cvar_t roundtime = {"roundtime", "150", FCVAR_SERVER};
cvar_t respawntime = {"respawntime", "3", FCVAR_SERVER};
// Dropped-weapon lifetime in seconds (CTSWorldGun::Spawn reads it, clamps [5,60]). The original registers
// "weaponstay" default 15 — distinct from stock "mp_weaponstay" (0). Ours read the unregistered name -> 0 ->
// the 5s floor, so dropped guns vanished early; default 15 restores the original 15s persistence.
cvar_t weaponstay_time = {"weaponstay", "15", FCVAR_SERVER};
cvar_t damagemult = {"damagemult", "0.0", FCVAR_SERVER};
cvar_t slowmatch = {"slowmatch", "1.0", FCVAR_SERVER};
cvar_t nopowerup = {"nopowerup", "0.0", FCVAR_SERVER};
cvar_t botlearn = {"botlearn", "0.0", FCVAR_SERVER};
cvar_t realbullet = {"realbullet", "0.0", FCVAR_SERVER};
cvar_t dmcashbased = {"usecash", "0.0", FCVAR_SERVER};
cvar_t cv_ammocount = {"ammocount", "0", FCVAR_SERVER};
cvar_t bottalk = {"bottalk", "1", FCVAR_SERVER};
cvar_t mp_theonemode = {"mp_theonemode", "0", FCVAR_SERVER};
cvar_t mp_theone = {"mp_theone", "gordon", FCVAR_SERVER};
cvar_t mp_theothers = {"mp_theothers", "seal", FCVAR_SERVER};
cvar_t lastmanstanding = {"lastmanstanding", "0", FCVAR_SERVER};
cvar_t spectacularness = {"spectacularness", "1", FCVAR_SERVER};
cvar_t respawnnumber = {"respawnnumber", "1", FCVAR_SERVER};
cvar_t customweapons = {"customweapons", "", FCVAR_SERVER};
cvar_t banfortk = {"banfortk", "0", FCVAR_SERVER};
cvar_t dmgreport = {"dmgreport", "0", FCVAR_SERVER};
cvar_t botarmor = {"botarmor", "0", FCVAR_SERVER};

void TSRegisterCvars()
{
	CVAR_REGISTER(&teammodels);
	CVAR_REGISTER(&initial_cash);
	CVAR_REGISTER(&cashforwin);
	CVAR_REGISTER(&cashforlose);
	CVAR_REGISTER(&roundtime);
	CVAR_REGISTER(&respawntime);
	CVAR_REGISTER(&weaponstay_time);
	CVAR_REGISTER(&damagemult);
	CVAR_REGISTER(&slowmatch);
	CVAR_REGISTER(&nopowerup);
	CVAR_REGISTER(&botlearn);
	CVAR_REGISTER(&realbullet);
	CVAR_REGISTER(&dmcashbased);
	CVAR_REGISTER(&cv_ammocount);
	CVAR_REGISTER(&bottalk);
	CVAR_REGISTER(&mp_theonemode);
	CVAR_REGISTER(&mp_theone);
	CVAR_REGISTER(&mp_theothers);
	CVAR_REGISTER(&lastmanstanding);
	CVAR_REGISTER(&spectacularness);
	CVAR_REGISTER(&respawnnumber);
	CVAR_REGISTER(&customweapons);
	CVAR_REGISTER(&banfortk);
	CVAR_REGISTER(&dmgreport);
	CVAR_REGISTER(&botarmor);
}
