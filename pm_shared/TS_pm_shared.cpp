/***
*  TS_pm_shared.cpp — RECONSTRUCTED original TS_pm_shared.c: the TS stunt/movement
*  prediction hooks, exported non-static like the original (T symbols in ts_i386.so).
*
*  Original free functions still to land here (RE'd, dossier-freefuncs.md analysis_ff_pmshared.md):
*    PseudoClimb (the one real gap — wall-climb head-call in TSHandle),
*    FixVelocity (verified 4-byte empty ret in the binary),
*    CanJumpHigh (dead code in the binary — superjump-gate hint REFUTED).
*  NO guessed bodies here — scaffold only until their reconcile lands.
***/
#include "Platform.h"
#include "mathlib.h"
#include "cdll_dll.h"
#include "const.h"
#include "usercmd.h"
#include "pm_defs.h"
#include "pm_shared.h"
#include "pm_movevars.h"
#include "TS_pm_shared.h"

// pm_shared.cpp globals we hook into (declared there, non-static)
void PM_CatagorizePosition();
void PM_FixPlayerCrouchStuck(int direction);
void PM_FixupGravityVelocity();

// @0xe8fc4 FixVelocity — EMPTY in the shipped binary (a compiled-out hook). The original
// calls it right after PM_FixupGravityVelocity in TSDive/TSWallJump/TSWallJumpClimb.
void FixVelocity()
{
}

// @0xf97cc CanJumpHigh — a 128u velocity-direction probe on the DUCKED hull, then a 164u
// point-contents check. DEAD in the shipped binary (the superjump gate stayed stock —
// hint REFUTED). QUIRK kept: usehull is NOT restored on the success path.
int CanJumpHigh()
{
	const int savehull = pmove->usehull;
	Vector dir, start, end;
	VectorCopy(pmove->velocity, dir);
	VectorNormalize(dir);
	VectorCopy(pmove->origin, start);
	VectorMA(start, 128, dir, end);
	pmove->usehull = 1;
	pmtrace_t tr = pmove->PM_PlayerTrace(start, end, PM_NORMAL, -1);
	if (tr.fraction != 1.0f)
	{
		VectorMA(pmove->origin, 164, dir, end);
		const int contents = pmove->PM_PointContents(end, nullptr);
		if (contents != CONTENTS_SOLID && contents != CONTENTS_SKY)
			return 1; // NB: original leaves usehull ducked here
	}
	pmove->usehull = savehull;
	return 0;
}

// ===== TS STUNT PREDICTION HOOKS (RE reference/STUNT.md §4) ==================
// pev->iuser4 stunt flags networked into pmove->iuser4 (mirror dlls/ts_stunt.h).
// TSHandle() is called from PM_PlayerMove (before PM_Duck) on BOTH the server and
// the client predictor, so the dive/prone hull + low view + launch velocity are
// PREDICTED identically — the fix for the DLL-only hull forcing that never stuck.
// (the TSPM_* flag bits are defined near the top of this file.)

// predicted PRONE hull (TSGoDuck @0xe93a8) — same small hull as a duck + a low eye.
void TSGoDuck()
{
	if (pmove->usehull == 1)
		return;
	pmove->usehull = 1;
	pmove->flags |= FL_DUCKING;
	// NOTE: TSGoDuck@0xe93a8 does NOT set view_ofs — matching that. The prone eye drops because the
	// server's CTSStunt Go* methods set pev->view_ofs = (0,0,0) (RE: g_vecZero -> pev+0x174 = view_ofs,
	// which I'd mislabeled as punchangle), networked to the camera. Don't fight it here.
	if (pmove->onground != -1)
	{
		for (int i = 0; i < 3; i++)
			pmove->origin[i] -= (pmove->player_mins[1][i] - pmove->player_mins[0][i]);
		PM_FixPlayerCrouchStuck(STUCK_MOVEUP);
		PM_CatagorizePosition();
	}
}

// leave the prone hull when there's headroom (TSStandUp @0xe91fc; mirrors PM_UnDuck).
void TSStandUp()
{
	Vector newOrigin;
	VectorCopy(pmove->origin, newOrigin);
	if (pmove->onground != -1)
	{
		for (int i = 0; i < 3; i++)
			newOrigin[i] += (pmove->player_mins[1][i] - pmove->player_mins[0][i]);
	}
	pmtrace_t trace = pmove->PM_PlayerTrace(newOrigin, newOrigin, PM_NORMAL, -1);
	if (0 == trace.startsolid)
	{
		pmove->usehull = 0;
		trace = pmove->PM_PlayerTrace(newOrigin, newOrigin, PM_NORMAL, -1);
		if (0 != trace.startsolid)
		{
			pmove->usehull = 1; // still stuck — stay prone
			return;
		}
		pmove->flags &= ~FL_DUCKING;
		pmove->view_ofs = VEC_VIEW;
		VectorCopy(newOrigin, pmove->origin);
		PM_FixPlayerCrouchStuck(STUCK_MOVEUP);
		PM_CatagorizePosition();
		pmove->iuser4 &= ~TSPM_STANDUP;
	}
	else
	{
		pmove->usehull = 1;
		pmove->flags |= FL_DUCKING;
	}
}

// the dive launch impulse (TSDive body @0xf953f): speed + vertical boost by tier,
// then clears that tier bit so it fires exactly once.
void TSDive()
{
	int speed, zb;
	if (pmove->iuser4 & TSPM_DIVE_MED)        { speed = 90;  zb = 25; pmove->iuser4 &= ~TSPM_DIVE_MED; }
	else if (!(pmove->iuser4 & TSPM_DIVE_HIGH)) { speed = 100; zb = 10; pmove->iuser4 &= ~TSPM_DIVE_LOW; }
	else                                      { speed = 90;  zb = 50; pmove->iuser4 &= ~TSPM_DIVE_HIGH; }

	Vector dir;
	VectorCopy(pmove->velocity, dir);
	VectorNormalize(dir);
	if (pmove->iuser4 & TSPM_WALLDIVE)
		VectorScale(pmove->forward, 3.0f, dir);

	pmove->velocity[0] += speed * dir[0];
	pmove->velocity[1] += speed * dir[1];
	pmove->velocity[2] = sqrt((float)(zb * 1600));
	pmove->onground = -1;
	pmove->iuser4 &= ~TSPM_WALLDIVE;
	PM_FixupGravityVelocity();
	FixVelocity(); // @orig: called right after (empty)
}

// wall-jump (TSWallJump @0xe9444): kick back off the faced wall.
void TSWallJump()
{
	pmove->onground = -1;
	if (pmove->iuser4 & TSPM_WALLJUMP)
	{
		pmove->velocity[0] = -pmove->forward[0] * 210.0f;
		pmove->velocity[1] = -pmove->forward[1] * 210.0f;
		pmove->velocity[2] = sqrt(56000.0f); // 236.64 — RE TSWallJump@0xe9444 (.rodata double 56000.0)
		pmove->iuser4 &= ~TSPM_WALLJUMP;
	}
	PM_FixupGravityVelocity();
	FixVelocity(); // @orig: called right after (empty)
}

// wall-climb (TSWallJumpClimb @0xe94e0): a straight upward kick.
void TSWallJumpClimb()
{
	pmove->onground = -1;
	if ((short)pmove->iuser4 < 0) // TSPM_WALLCLIMB
	{
		pmove->velocity[2] = sqrt(112000.0f); // 334.66
		pmove->iuser4 &= ~TSPM_WALLCLIMB;
	}
	PM_FixupGravityVelocity();
	FixVelocity(); // @orig: called right after (empty)
}

// the pm_shared dispatcher (TSHandle @0xe96a0) — keyed on pmove->iuser4.
void TSHandle()
{
	// STOP / SLIDE damp the MOVE COMMAND (not velocity) — your launch/landing velocity carries
	// you (that IS the slide); you only get reduced input control. RE TSHandle @0xe96a0: these
	// touch cmd.forwardmove/sidemove/upmove (the same fields PM_Duck's 0.333 multiplier scales).
	if (pmove->iuser4 & TSPM_STOP)
	{
		pmove->cmd.forwardmove *= 0.5f;
		pmove->cmd.sidemove *= 0.5f;
		pmove->cmd.upmove *= 0.5f;
	}
	if (pmove->iuser4 & TSPM_SLIDE)
	{
		pmove->cmd.forwardmove = 0;    // strafe only — no forward/back thrust while prone/sliding
		pmove->cmd.sidemove *= 0.3f;   // slow strafe
		pmove->cmd.upmove *= 0.3f;
	}
	if (pmove->iuser4 & TSPM_DIVE_HEIGHT) TSDive();
	if (pmove->iuser4 & TSPM_WALLJUMP_M)  TSWallJump();
	if ((short)pmove->iuser4 < 0)         TSWallJumpClimb();
	if (pmove->iuser4 & TSPM_PRONE)       TSGoDuck();
	if (pmove->iuser4 & TSPM_STANDUP)     TSStandUp();
}
