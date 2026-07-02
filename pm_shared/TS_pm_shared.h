/***
*  TS_pm_shared.h — the TS player-movement additions (original TS_pm_shared.c).
*  Shared by pm_shared.cpp (PM_PlayerMove/PM_Duck gates) and TS_pm_shared.cpp.
***/
#pragma once

// PM_FixPlayerCrouchStuck directions (used by the TS hull transitions too)
#define STUCK_MOVEUP 1
#define STUCK_MOVEDOWN -1

// TS stunt prediction flags — pev->iuser4 networked into pmove->iuser4 (mirror
// dlls/ts_stunt.h). Read by the TS* hooks below + the PM_Duck gate. See reference/STUNT.md.
#define TSPM_STOP        0x000001 // damp velocity x0.5
#define TSPM_PRONE       0x000004 // use the ducking hull
#define TSPM_STANDUP     0x000008 // leave the ducking hull
#define TSPM_DIVE        0x000010 // dive in progress (strafe-only air control)
#define TSPM_SLIDE       0x000020 // slide: vel.x=0, vel.{y,z} x0.3
#define TSPM_DIVE_HIGH   0x000040
#define TSPM_DIVE_LOW    0x000080
#define TSPM_DIVE_MED    0x000100
#define TSPM_DIVE_HEIGHT (TSPM_DIVE_HIGH | TSPM_DIVE_LOW | TSPM_DIVE_MED)
#define TSPM_WALLJUMP    0x001000
#define TSPM_WALLJUMP_M  0x007000 // any wall-jump bit
#define TSPM_WALLCLIMB   0x008000 // sign bit of the low short
#define TSPM_WALLDIVE    0x800000
#define TSPM_STUNT_MASK  0x03CC3C // any stunt -> PM_Duck skips the normal duck

// The TS stunt prediction entry point — called from PM_PlayerMove before PM_Duck
// on BOTH the server and the client predictor.
void TSHandle();
void TSGoDuck();
void TSStandUp();
void TSDive();
void TSWallJump();
void TSWallJumpClimb();
