// view/refresh setup functions

#include "hud.h"
#include "cl_util.h"
#include "cvardef.h"
#include "usercmd.h"
#include "const.h"

#include "entity_state.h"
#include "cl_entity.h"
#include "ref_params.h"
#include "in_defs.h" // PITCH YAW ROLL
#include "pm_movevars.h"
#include "pm_shared.h"
#include "pm_defs.h"
#include "event_api.h"
#include "pmtrace.h"
#include "screenfade.h"
#include "shake.h"
#include "hltv.h"
#include "Exports.h"

int CL_IsThirdPerson();
void CL_CameraOffset(float* ofs);

void DLLEXPORT V_CalcRefdef(struct ref_params_s* pparams);

void PM_ParticleLine(float* start, float* end, int pcolor, float life, float vert);
int PM_GetVisEntInfo(int ent);
int PM_GetPhysEntInfo(int ent);
void InterpolateAngles(float* start, float* end, float* output, float frac);
void NormalizeAngles(float* angles);
float Distance(const float* v1, const float* v2);
float AngleBetweenVectors(const float* v1, const float* v2);

extern float vJumpOrigin[3];
extern float vJumpAngles[3];


void V_DropPunchAngle(float frametime, float* ev_punchangle);
void VectorAngles(const float* forward, float* angles);

#include "r_studioint.h"
#include "com_model.h"
#include "kbutton.h"
#include "ts_recoil.h"
#include "../dlls/ts_weapon_info.h"
#include "ts_emitter.h"

extern engine_studio_api_t IEngineStudio;

extern kbutton_t in_mlook;

/*
The view is allowed to move slightly from it's true position for bobbing,
but if it exceeds 8 pixels linear distance (spherical, not box), the list of
entities sent from the server may not include everything in the pvs, especially
when crossing a water boudnary.
*/

extern cvar_t* cl_forwardspeed;
extern cvar_t* chase_active;
extern cvar_t *scr_ofsx, *scr_ofsy, *scr_ofsz;
extern cvar_t* cl_vsmoothing;

// TS over-the-shoulder 3rd-person camera preset cvars (defined in in_camera.cpp)
extern cvar_t* cl_camtype;
extern cvar_t* cam_back;
extern cvar_t* cam_right;
extern cvar_t* cam_up;

#define CAM_MODE_RELAX 1
#define CAM_MODE_FOCUS 2

Vector v_origin, v_angles, v_cl_angles, v_sim_org, v_lastAngles;
// The player EYE (simorg + view_ofs) = where bullets fire from — captured BEFORE the 3rd-person
// camera offset is applied to vieworg. The 3rd-person reticle traces its aim from HERE (not the
// offset camera v_origin) and projects through the render camera, so it screen-space-maps the
// true aim point (parallax when you strafe; correct where-shots-land).
Vector v_aimeye;
float v_frametime, v_lastDistance;
Vector g_tspLocalSimVel; // client.dll 0x10191d20: the local player's simvel, read by the particle push
float v_cameraRelaxAngle = 5.0f;
float v_cameraFocusAngle = 35.0f;
int v_cameraMode = CAM_MODE_FOCUS;
bool v_resetCamera = true;

Vector v_client_aimangles;
Vector ev_punchangle;

cvar_t* scr_ofsx;
cvar_t* scr_ofsy;
cvar_t* scr_ofsz;

cvar_t* v_centermove;
cvar_t* v_centerspeed;

cvar_t* cl_bobcycle;
cvar_t* cl_bob;
cvar_t* cl_bobup;
cvar_t* cl_waterdist;

// client.dll registers no cl_chasedist: its V_GetChasePos (0x10068d40) passes 90 to the chase origin.
constexpr float V_CHASE_DISTANCE = 90.0f;

// These cvars are not registered (so users can't cheat), so set the ->value field directly
// Register these cvars in V_Init() if needed for easy tweaking
cvar_t v_iyaw_cycle = {"v_iyaw_cycle", "2", 0, 2};
cvar_t v_iroll_cycle = {"v_iroll_cycle", "0.5", 0, 0.5};
cvar_t v_ipitch_cycle = {"v_ipitch_cycle", "1", 0, 1};
cvar_t v_iyaw_level = {"v_iyaw_level", "0.3", 0, 0.3};
cvar_t v_iroll_level = {"v_iroll_level", "0.1", 0, 0.1};
cvar_t v_ipitch_level = {"v_ipitch_level", "0.3", 0, 0.3};

float v_idlescale; // used by TFC for concussion grenade effect

//=============================================================================
/*
void V_NormalizeAngles( Vector& angles )
{
	int i;
	// Normalize angles
	for ( i = 0; i < 3; i++ )
	{
		if ( angles[i] > 180.0 )
		{
			angles[i] -= 360.0;
		}
		else if ( angles[i] < -180.0 )
		{
			angles[i] += 360.0;
		}
	}
}

/*
===================
V_InterpolateAngles

Interpolate Euler angles.
FIXME:  Use Quaternions to avoid discontinuities
Frac is 0.0 to 1.0 ( i.e., should probably be clamped, but doesn't have to be )
===================

void V_InterpolateAngles( float *start, float *end, float *output, float frac )
{
	int i;
	float ang1, ang2;
	float d;
	
	V_NormalizeAngles( start );
	V_NormalizeAngles( end );

	for ( i = 0 ; i < 3 ; i++ )
	{
		ang1 = start[i];
		ang2 = end[i];

		d = ang2 - ang1;
		if ( d > 180 )
		{
			d -= 360;
		}
		else if ( d < -180 )
		{	
			d += 360;
		}

		output[i] = ang1 + d * frac;
	}

	V_NormalizeAngles( output );
} */

// The weapon bob, client.dll 0x10066e70 (with its height smoothing 0x10066de0). It replaces the stock
// Quake bob: cl_bobcycle and cl_bobup are registered but never read. It fills s_bobOffset, added to the
// view model's origin, and s_bobPitch, added to the eye's pitch, and returns the bob amplitude, which
// the gun angle adds to the view model's pitch. cl_bob picks the level, truncated: 0..3 scale the
// offset by 0.1, 0.3, 0.5, 0.7 and the eye pitch by 0, 0, 0.15, 0.5; anything else (the default 4.0)
// by 1 and 1. In third person and while spectating it returns 0 and leaves the offset as it was.
// client.dll also returns 0 while its stunt camera flags (0x101914ec, set by its client-side stunt
// camera at 0x1002c790) are set without bit 8; reTS has no such camera, so the flags are always 0.
static Vector s_bobOffset;	 // 0x10191428
static float s_bobPitch;	 // 0x10190b78
static float s_bobAmp;		 // 0x10191d6c
static float s_bobAmpLast;	 // 0x10191d78
static float s_bobPhase;	 // 0x10191d74
static float s_bobLastTime; // 0x10191d70
static float s_bobHeight;	 // 0x1019144c

// 0x10066de0: the offset's height follows its target at frametime x 20 of the gap, never past it.
static void V_SmoothBobHeight(float* offset, struct ref_params_s* pparams)
{
	const float gap = s_bobHeight - offset[2];
	float step = gap * pparams->frametime * 20.0f;
	if (fabs(step) > fabs(gap))
		step = gap;
	s_bobHeight -= step;
	offset[2] = s_bobHeight;
}

float V_CalcBob(struct ref_params_s* pparams)
{
	if (0 != CL_IsThirdPerson() || 0 != pparams->spectator)
		return 0.0f;

	float grip, pitchScale;
	switch ((int)cl_bob->value)
	{
	case 0: grip = 0.1f; pitchScale = 0.0f; break;
	case 1: grip = 0.3f; pitchScale = 0.0f; break;
	case 2: grip = 0.5f; pitchScale = 0.15f; break;
	case 3: grip = 0.7f; pitchScale = 0.5f; break;
	default: grip = 1.0f; pitchScale = 1.0f; break;
	}

	extern float g_flClientSlow; // ts_powerup_hud.cpp, client.dll's slow-motion scale 0x100963e8
	float forwardScale;
	if (0 != pparams->onground && pparams->time != s_bobLastTime && pparams->viewheight[2] != 0.0f)
	{
		s_bobLastTime = pparams->time;
		const double speed = sqrt(pparams->simvel[1] * pparams->simvel[1] + pparams->simvel[0] * pparams->simvel[0]);
		double pace = speed * 0.003333333406;
		if (pace > 1.100000024)
			pace = 1.100000024;
		s_bobPhase = (float)(pparams->frametime * 5.5f * g_flClientSlow * pace * 100.0 + s_bobPhase);
		if (s_bobPhase > 360.0)
			s_bobPhase = 0.0f;
		if (s_bobAmpLast > s_bobAmp)
			s_bobAmp = (float)((s_bobAmp + 2.5f) * pace * 0.5f);
		else
		{
			s_bobAmp = (float)(speed * 0.01);
			s_bobAmpLast = s_bobAmp;
		}
		const float up = (float)(sin(s_bobPhase * 0.03490658504) * s_bobAmp);
		const float side = (float)(sin(s_bobPhase * 0.01745329252) * (double)s_bobAmp);
		forwardScale = (float)(0.75 - s_bobAmp * 0.5);
		for (int i = 0; i < 3; i++)
			s_bobOffset[i] = (float)(side * pparams->right[i] * 0.3 + pparams->forward[i] * forwardScale + up * pparams->up[i] * 0.11);
		s_bobPitch = up * pitchScale * 0.25f;
	}
	else
	{
		if (s_bobAmp > 0.01)
			s_bobAmp = (float)(s_bobAmp * 0.5);
		forwardScale = (float)(0.75 - s_bobAmp * 0.5);
		for (int i = 0; i < 3; i++)
			s_bobOffset[i] = pparams->forward[i] * forwardScale;
		s_bobPitch = 0.0f;
	}
	s_bobOffset[0] *= grip;
	s_bobOffset[1] *= grip;
	s_bobOffset[2] = (float)((0.5 - s_bobAmp * 0.65) + s_bobOffset[2]) * grip;
	V_SmoothBobHeight(s_bobOffset, pparams);
	return s_bobAmp;
}

/*
===============
V_CalcRoll
Used by view and sv_user
===============
*/
float V_CalcRoll(Vector angles, Vector velocity, float rollangle, float rollspeed)
{
	float sign;
	float side;
	float value;
	Vector forward, right, up;

	AngleVectors(angles, forward, right, up);

	side = DotProduct(velocity, right);
	sign = side < 0 ? -1 : 1;
	side = fabs(side);

	value = rollangle;
	if (side < rollspeed)
	{
		side = side * value / rollspeed;
	}
	else
	{
		side = value;
	}
	return side * sign;
}

typedef struct pitchdrift_s
{
	float pitchvel;
	int nodrift;
	float driftmove;
	double laststop;
} pitchdrift_t;

static pitchdrift_t pd;

void V_StartPitchDrift()
{
	if (pd.laststop == gEngfuncs.GetClientTime())
	{
		return; // something else is keeping it from drifting
	}

	if (0 != pd.nodrift || 0 == pd.pitchvel)
	{
		pd.pitchvel = v_centerspeed->value;
		pd.nodrift = 0;
		pd.driftmove = 0;
	}
}

void V_StopPitchDrift()
{
	pd.laststop = gEngfuncs.GetClientTime();
	pd.nodrift = 1;
	pd.pitchvel = 0;
}

/*
===============
V_DriftPitch

Moves the client pitch angle towards idealpitch sent by the server.

If the user is adjusting pitch manually, either with lookup/lookdown,
mlook and mouse, or klook and keyboard, pitch drifting is constantly stopped.
===============
*/
void V_DriftPitch(struct ref_params_s* pparams)
{
	float delta, move;

	if (0 != gEngfuncs.IsNoClipping() || 0 == pparams->onground || 0 != pparams->demoplayback || 0 != pparams->spectator)
	{
		pd.driftmove = 0;
		pd.pitchvel = 0;
		return;
	}

	// don't count small mouse motion
	if (0 != pd.nodrift)
	{
		if (v_centermove->value > 0 && (in_mlook.state & 1) == 0)
		{
			// this is for lazy players. if they stopped, looked around and then continued
			// to move the view will be centered automatically if they move more than
			// v_centermove units.

			if (fabs(pparams->cmd->forwardmove) < cl_forwardspeed->value)
				pd.driftmove = 0;
			else
				pd.driftmove += pparams->frametime;

			if (pd.driftmove > v_centermove->value)
			{
				V_StartPitchDrift();
			}
			else
			{
				return; // player didn't move enough
			}
		}

		return; // don't drift view
	}

	delta = pparams->idealpitch - pparams->cl_viewangles[PITCH];

	if (0 == delta)
	{
		pd.pitchvel = 0;
		return;
	}

	move = pparams->frametime * pd.pitchvel;

	pd.pitchvel *= (1.0f + (pparams->frametime * 0.25f)); // get faster by time

	if (delta > 0)
	{
		if (move > delta)
		{
			pd.pitchvel = 0;
			move = delta;
		}
		pparams->cl_viewangles[PITCH] += move;
	}
	else if (delta < 0)
	{
		if (move > -delta)
		{
			pd.pitchvel = 0;
			move = -delta;
		}
		pparams->cl_viewangles[PITCH] -= move;
	}
}

/* 
============================================================================== 
						VIEW RENDERING 
============================================================================== 
*/

/*
==================
V_CalcGunAngle
==================
*/
// client.dll 0x100674a0: the view model takes the view angles, its pitch inverted plus the bob's
// amplitude, and follows the client-side punch (ev_punchangle) by 1.25 in pitch and yaw. TS has no
// crosshair angle or idle sway terms here.
void V_CalcGunAngle(struct ref_params_s* pparams, float bob)
{
	cl_entity_t* viewent;

	viewent = gEngfuncs.GetViewModel();
	if (!viewent)
		return;

	viewent->angles[YAW] = pparams->viewangles[YAW];
	viewent->angles[PITCH] = bob - pparams->viewangles[PITCH];
	viewent->angles[ROLL] = pparams->viewangles[ROLL];
	viewent->angles[PITCH] -= ev_punchangle[0] * 1.25f;
	viewent->angles[YAW] += ev_punchangle[1] * 1.25f;
}

/*
==============
V_AddIdle

Idle swaying
==============
*/
void V_AddIdle(struct ref_params_s* pparams)
{
	pparams->viewangles[ROLL] += v_idlescale * sin(pparams->time * v_iroll_cycle.value) * v_iroll_level.value;
	pparams->viewangles[PITCH] += v_idlescale * sin(pparams->time * v_ipitch_cycle.value) * v_ipitch_level.value;
	pparams->viewangles[YAW] += v_idlescale * sin(pparams->time * v_iyaw_cycle.value) * v_iyaw_level.value;
}


/*
==============
V_CalcViewRoll

Roll is induced by movement and damage
==============
*/
// client.dll 0x100675b0. The roll is TS's own, not the movevars': none while the local player's
// iuser1 is set (rolled to 0); on the ground it eases toward 0; in the air it rolls with the sideways
// speed (0x100671f0: |speed x 0.01| against 10) up to 35 degrees with stunt flag 0x10, -10 with 0xc00,
// else -5.5, eased at 7.5, 5 and 15 a second (15 on the ground). Dead with a view height it is 80.
void V_CalcViewRoll(struct ref_params_s* pparams)
{
	static float s_flRoll = 0;
	cl_entity_t* local = gEngfuncs.GetLocalPlayer();
	cl_entity_t* viewentity = gEngfuncs.GetEntityByIndex(pparams->viewentity);
	if (!viewentity || !local)
		return;
	if (local->prevstate.iuser1 > 0)
	{
		pparams->viewangles[ROLL] = 0;
		return;
	}

	float rollangle = 0.0f, rate = 15.0f;
	if (0 == pparams->onground)
	{
		if ((local->prevstate.iuser4 & 0x10) != 0)
		{
			rollangle = 35.0f;
			rate = 7.5f;
		}
		else if ((local->prevstate.iuser4 & 0xc00) != 0)
		{
			rollangle = -10.0f;
			rate = 5.0f;
		}
		else
			rollangle = -5.5f;
	}

	Vector forward, right, up;
	AngleVectors(viewentity->angles, forward, right, up);
	const float dot = DotProduct(pparams->simvel, right);
	const float sign = dot >= 0.0f ? 1.0f : -1.0f;
	const float v = fabs(dot) * 0.01f;
	const float roll = (v < 10.0f ? v * rollangle / 10.0f : rollangle) * sign;

	float k = rate * pparams->frametime;
	if (k > 1.0f)
		k = 1.0f;
	s_flRoll += (roll - s_flRoll) * k;
	pparams->viewangles[ROLL] += s_flRoll;

	if (pparams->health <= 0 && pparams->viewheight[2] != 0)
		pparams->viewangles[ROLL] = 80; // dead view angle
}


/*
==================
V_CalcIntermissionRefdef

==================
*/
void V_CalcIntermissionRefdef(struct ref_params_s* pparams)
{
	cl_entity_t *ent, *view;
	float old;

	// ent is the player model ( visible when out of body )
	ent = gEngfuncs.GetLocalPlayer();

	// view is the weapon model (only visible from inside body )
	view = gEngfuncs.GetViewModel();

	VectorCopy(pparams->simorg, pparams->vieworg);
	VectorCopy(pparams->cl_viewangles, pparams->viewangles);

	view->model = NULL;

	// allways idle in intermission
	old = v_idlescale;
	v_idlescale = 1;

	V_AddIdle(pparams);

	if (0 != gEngfuncs.IsSpectateOnly())
	{
		// in HLTV we must go to 'intermission' position by ourself
		VectorCopy(gHUD.m_Spectator.m_cameraOrigin, pparams->vieworg);
		VectorCopy(gHUD.m_Spectator.m_cameraAngles, pparams->viewangles);
	}

	v_idlescale = old;

	v_cl_angles = pparams->cl_viewangles;
	v_origin = pparams->vieworg;
	v_angles = pparams->viewangles;
}

#define ORIGIN_BACKUP 64
#define ORIGIN_MASK (ORIGIN_BACKUP - 1)

typedef struct
{
	float Origins[ORIGIN_BACKUP][3];
	float OriginTime[ORIGIN_BACKUP];

	float Angles[ORIGIN_BACKUP][3];
	float AngleTime[ORIGIN_BACKUP];

	int CurrentOrigin;
	int CurrentAngle;
} viewinterp_t;

/*
==================
TS aim-recoil

The Specialists' recoil moves the player's ACTUAL aim (cl.viewangles), persistently — you must pull
down. RE of the original client.dll: FUN_10008230 (@0x8230, the client twin of the server VKickback)
adds a per-shot kick to a recoil TARGET; FUN_10057ed0 (@0x57ed0, called from V_CalcRefdef) eases the
APPLIED recoil toward the target each frame and folds the delta into the view. See
scratchpad/recoil-live-findings.md (constants live-verified against a cl.viewangles trajectory).
==================
*/
static Vector g_recoilTarget;  // pending recoil climb (deg): [PITCH] negative = up, [YAW] horizontal
static Vector g_recoilApplied; // how much of the climb has been eased into the aim so far

// --- MELEE screen-SHAKE (the knife/katana "bounce-back") -------------------------------------------------
// RE: the client melee attack FUN_1000a270 (@0xa345) calls Shake_Setup(amp = flDamage*0.15 [*0.66667 in
// bullet-time], dur 0.2, freq 10) on each swing (live-verified: knife dmg 30 -> amp 4.5). The shake is a
// damped oscillation (FUN_10057fa0 setup + FUN_10058000 apply): a random +/-amp offset that sign-flips and
// halves each 1/freq cycle, smoothed onto the view and settling back to 0 — a transient "shake + reset",
// NOT an aim move. GUNS use the climb above and do NOT shake (the gun-recoil shake branch in FUN_10008230
// did not fire in the live trace). Applied to the RENDER angle only (pparams->viewangles), so the aim is
// untouched and it returns to where it was. See scratchpad/recoil-live-findings.md.
static struct
{
	Vector offset;    // current random target (sign-flips + halves each cycle)
	Vector applied;   // smoothed offset currently added to the render view
	float duration;   // time remaining
	float frequency;
	float cycleTimer; // counts down 1/frequency
} g_meleeShake;

void TS_TriggerViewShake(float amplitude)
{
	g_meleeShake.duration = 0.2f;   // dur, freq fixed for the melee/kung-fu bounce (RE: both 0.2 / 10)
	g_meleeShake.frequency = 10.0f;
	g_meleeShake.cycleTimer = 1.0f / 10.0f;
	g_meleeShake.offset[0] = gEngfuncs.pfnRandomFloat(-amplitude, amplitude);
	g_meleeShake.offset[1] = gEngfuncs.pfnRandomFloat(-amplitude, amplitude);
	g_meleeShake.offset[2] = 0.0f;
}

// Advance the shake one frame and add the current (transient) offset to the RENDER view angles. Called
// AFTER pparams->viewangles is seeded from cl_viewangles, so it perturbs only the rendered view and decays
// back to zero (the aim/usercmd is never touched).
void TS_ApplyMeleeShake(struct ref_params_s* pparams)
{
	if (g_meleeShake.duration <= 0.0f && g_meleeShake.applied[0] == 0.0f &&
		g_meleeShake.applied[1] == 0.0f && g_meleeShake.applied[2] == 0.0f)
		return; // inactive
	float dt = pparams->frametime;
	Vector ofs;
	float cyc;
	if (g_meleeShake.duration <= 0.0f)
	{
		ofs[0] = ofs[1] = ofs[2] = 0.0f; // expired -> ease applied back to 0
		cyc = 20.0f;
	}
	else
	{
		g_meleeShake.duration -= dt;
		g_meleeShake.cycleTimer -= dt;
		if (g_meleeShake.cycleTimer < 0.0f)
		{
			g_meleeShake.cycleTimer = 1.0f / g_meleeShake.frequency;
			g_meleeShake.offset[0] = -g_meleeShake.offset[0] * 0.5f; // flip + halve
			g_meleeShake.offset[1] = -g_meleeShake.offset[1] * 0.5f;
			g_meleeShake.offset[2] = -g_meleeShake.offset[2] * 0.5f;
		}
		ofs = g_meleeShake.offset;
		cyc = g_meleeShake.frequency;
	}
	float frac = cyc * dt;
	if (frac > 1.0f)
		frac = 1.0f;
	g_meleeShake.applied[0] += (ofs[0] - g_meleeShake.applied[0]) * frac;
	g_meleeShake.applied[1] += (ofs[1] - g_meleeShake.applied[1]) * frac;
	g_meleeShake.applied[2] += (ofs[2] - g_meleeShake.applied[2]) * frac;
	pparams->viewangles[PITCH] += g_meleeShake.applied[0];
	pparams->viewangles[YAW] += g_meleeShake.applied[1];
	pparams->viewangles[ROLL] += g_meleeShake.applied[2];
}

void TS_AddViewRecoil(float flViewKick, int iFireType, bool bDuck, bool bBulletTime)
{
	// FUN_10008230: vert = amount*0.85; horiz = vert*firetypeScale; *0.775 duck; *0.66667 bullet-time.
	float vert = flViewKick * 0.85f;
	float scale;
	switch (iFireType)
	{
	case 1:         scale = -0.25f; break; // TSFT_PUMP
	case 4: case 5: scale = 0.15f;  break; // TSFT_AKIMBO / TSFT_AKIMBO_ONLY
	case 6: case 7: scale = -0.30f; break; // TSFT_AUTOSHOTGUN / TSFT_SNIPER
	case 8:         scale = 0.35f;  break; // TSFT_SEMI8 (pistols)
	case 9:         scale = -0.35f; break; // TSFT_AUTO (SMGs)
	default:        scale = 0.0f;   break; // semi/grenade/knife/katana: vertical only
	}
	float horiz = vert * scale;
	if (bDuck)
	{
		vert *= 0.775f;
		horiz *= 0.775f;
	}
	if (bBulletTime)
	{
		vert *= 0.66667f;
		horiz *= 0.66667f;
	}
	g_recoilTarget[PITCH] -= vert; // negative pitch = kick UP
	g_recoilTarget[YAW] += horiz;
}

void TS_UpdateViewRecoil(struct ref_params_s* pparams)
{
	// FUN_10057ed0: ease the climb (applied -> target) at rate min(frametime*15, 1); snap to 0 when the
	// remaining kick is within ~0.1deg. The delta is committed to the real aim (SetViewAngles) so it
	// persists into the next usercmd, exactly like the original's viewport commit.
	float remPitch = g_recoilTarget[PITCH] - g_recoilApplied[PITCH];
	float remYaw = g_recoilTarget[YAW] - g_recoilApplied[YAW];
	if (remPitch * remPitch + remYaw * remYaw < 0.01f) // 0x10079e60 (double) snap threshold
	{
		g_recoilTarget[PITCH] = g_recoilTarget[YAW] = 0.0f;
		g_recoilApplied[PITCH] = g_recoilApplied[YAW] = 0.0f;
		return;
	}
	float rate = pparams->frametime * 15.0f; // 0x1007a890 = 15.0
	if (rate > 1.0f)
		rate = 1.0f; // 0x10079e80 (double) = 1.0 clamp
	float dPitch = remPitch * rate;
	float dYaw = remYaw * rate;
	g_recoilApplied[PITCH] += dPitch;
	g_recoilApplied[YAW] += dYaw;

	Vector va;
	gEngfuncs.GetViewAngles(va);
	va[PITCH] += dPitch;
	va[YAW] += dYaw;
	gEngfuncs.SetViewAngles(va);
	// reflect it in THIS frame's render too (next frame GetViewAngles already includes it)
	pparams->cl_viewangles[PITCH] += dPitch;
	pparams->cl_viewangles[YAW] += dYaw;
}

/*
==================
V_CalcRefdef

==================
*/
void V_CalcNormalRefdef(struct ref_params_s* pparams)
{
	cl_entity_t *ent, *view;
	int i;
	Vector angles;
	float bob, waterOffset;
	static viewinterp_t ViewInterp;

	static float oldz = 0;
	static float lasttime;

	Vector camAngles, camForward, camRight, camUp;
	cl_entity_t* pwater;

	V_DriftPitch(pparams);

	// TS aim-recoil: ease the per-shot kick into the actual aim (persistent). Must run before the aim is
	// copied into pparams->viewangles below so the kick shows this frame.
	TS_UpdateViewRecoil(pparams);

	if (0 != gEngfuncs.IsSpectateOnly())
	{
		ent = gEngfuncs.GetEntityByIndex(g_iUser2);
	}
	else
	{
		// ent is the player model ( visible when out of body )
		ent = gEngfuncs.GetLocalPlayer();
	}

	// TS 3rd-person self-transparency (RE dossier cont91 / client.dll V_CalcNormalRefdef @0x678b8):
	// fade the LOCAL player's own model toward alpha 100 while the crosshair aim-trace is on a
	// player, ease back to 255 (opaque) otherwise, so your own shoulder stops blocking the target.
	// Hard-reset to opaque in spectator/dead/first-person (the original's FUN_1004c250 state gate;
	// the smoothed alpha itself is NOT reset there — it persists across the reset). Written every
	// frame on the local cl_entity's curstate here at the TOP of the refdef pass, matching the
	// original's placement — StudioRenderFinal later consumes curstate.rendermode
	// (StudioModelRenderer.cpp:1779). Nothing re-derives the local curstate after this:
	// HUD_ProcessPlayerState (entity.cpp:123) writes the networked entity_state used for prediction,
	// not this post-refdef rendered curstate, so the value survives to the studio draw.
	{
		extern int g_iTSAimTarget; // set by the status draw's aim trace (ts_reticle.cpp, client.dll 0x1004acc0)
		cl_entity_t* lp = gEngfuncs.GetLocalPlayer();
		if (lp)
		{
			// cl_thirdtransparency gates the fade TARGET only (dossier cont94 §1, read @0x678e6):
			// registered default "1" = ON — the original's registration default, the parity target
			// (the .scr's 0 is only the listen-server GUI suggestion, NOT the binary default).
			static cvar_t* s_thirdtrans = nullptr;
			if (s_thirdtrans == nullptr)
				s_thirdtrans = gEngfuncs.pfnGetCvarPointer("cl_thirdtransparency");
			static float s_alpha = 255.0f;
			// Hard-reset ALSO fires in FIRST person (the original's FUN_1004c250 gate is spectator
			// OR !thirdperson) — and it does NOT touch the smoothed alpha (DAT_10097214 persists).
			if (0 != gEngfuncs.IsSpectateOnly() || 0 != g_iUser1 || gHUD.m_Health.m_iHealth <= 0 ||
				0 == CL_IsThirdPerson())
			{
				lp->curstate.rendermode = kRenderNormal; // 0 — opaque hard-reset
				lp->curstate.renderamt = 255;
			}
			else
			{
				// §1a: 100 aiming at a player AND cl_thirdtransparency on / 255 opaque.
				// Test form is the original's ftol truncation — (int)value != 0, so 0.5 reads OFF.
				float target = (g_iTSAimTarget != 0 && s_thirdtrans != nullptr &&
								   (int)s_thirdtrans->value != 0) ?
								   100.0f :
								   255.0f;
				float step = (s_alpha >= target) ? 50.0f : 5.0f; // fade-out 50 / fade-back 5 per frame (confirmed @0x67911–0x6794b)
				if (s_alpha < target)
					s_alpha = (s_alpha + step > target) ? target : s_alpha + step;
				else
					s_alpha = (s_alpha - step < target) ? target : s_alpha - step;
				lp->curstate.rendermode = kRenderTransColor; // 1 (§1a) — always driven while the fade logic is active
				lp->curstate.renderamt = (int)s_alpha;
			}
		}
	}

	// view is the weapon model (only visible from inside body )
	view = gEngfuncs.GetViewModel();

	// TS: the viewmodel BODY refreshes EVERY FRAME from the predicted weapon state (the
	// original's DAT_10191d5c mechanism) — deploy/zoom-return anims otherwise show a stale
	// body until the next anim tick (the ~1 s attachment blink, round 7j).
	{
		extern int TS_PredictedViewmodelBody();
		view->curstate.body = TS_PredictedViewmodelBody();
	}

	// transform the view offset by the model's matrix to get the offset from
	// model origin for the view
	bob = V_CalcBob(pparams);

	// refresh position
	VectorCopy(pparams->simorg, pparams->vieworg);
	// client.dll (0x10067890) bobs only the view model, never the eye's origin (stock HL adds the bob
	// to vieworg[2] here).
	VectorAdd(pparams->vieworg, pparams->viewheight, pparams->vieworg);
	VectorCopy(pparams->vieworg, v_aimeye); // the true eye (pre-3rd-person-offset) = bullet origin

	// the bob's eye pitch, a tenth of it, then cleared (client.dll 0x10067d14)
	pparams->cl_viewangles[PITCH] += s_bobPitch * 0.1f;
	s_bobPitch = 0.0f;

	VectorCopy(pparams->cl_viewangles, pparams->viewangles);

	gEngfuncs.V_CalcShake();
	gEngfuncs.V_ApplyShake(pparams->vieworg, pparams->viewangles, 1.0);

	// never let view origin sit exactly on a node line, because a water plane can
	// dissapear when viewed with the eye exactly on it.
	// FIXME, we send origin at 1/128 now, change this?
	// the server protocol only specifies to 1/16 pixel, so add 1/32 in each axis

	pparams->vieworg[0] += 1.0 / 32;
	pparams->vieworg[1] += 1.0 / 32;
	pparams->vieworg[2] += 1.0 / 32;

	// Check for problems around water, move the viewer artificially if necessary
	// -- this prevents drawing errors in GL due to waves

	waterOffset = 0;
	if (pparams->waterlevel >= 2)
	{
		int i, contents, waterDist, waterEntity;
		Vector point;
		waterDist = cl_waterdist->value;

		if (0 != pparams->hardware)
		{
			waterEntity = gEngfuncs.PM_WaterEntity(pparams->simorg);
			if (waterEntity >= 0 && waterEntity < pparams->max_entities)
			{
				pwater = gEngfuncs.GetEntityByIndex(waterEntity);
				if (pwater && (pwater->model != NULL))
				{
					waterDist += (pwater->curstate.scale * 16); // Add in wave height
				}
			}
		}
		else
		{
			waterEntity = 0; // Don't need this in software
		}

		VectorCopy(pparams->vieworg, point);

		// Eyes are above water, make sure we're above the waves
		if (pparams->waterlevel == 2)
		{
			point[2] -= waterDist;
			for (i = 0; i < waterDist; i++)
			{
				contents = gEngfuncs.PM_PointContents(point, NULL);
				if (contents > CONTENTS_WATER)
					break;
				point[2] += 1;
			}
			waterOffset = (point[2] + waterDist) - pparams->vieworg[2];
		}
		else
		{
			// eyes are under water.  Make sure we're far enough under
			point[2] += waterDist;

			for (i = 0; i < waterDist; i++)
			{
				contents = gEngfuncs.PM_PointContents(point, NULL);
				if (contents <= CONTENTS_WATER)
					break;
				point[2] -= 1;
			}
			waterOffset = (point[2] - waterDist) - pparams->vieworg[2];
		}
	}

	pparams->vieworg[2] += waterOffset;

	V_CalcViewRoll(pparams);

	V_AddIdle(pparams);

	// offsets
	VectorCopy(pparams->cl_viewangles, angles);

	AngleVectors(angles, pparams->forward, pparams->right, pparams->up);

	// (client.dll registers scr_ofsx/y/z but never reads them: no single player view offset here.)

	// The third-person camera (client.dll 0x10067fb0..0x10068508). Its direction is cam_ofs's pitch and
	// yaw (smoothed toward the view by CAM_Think); its offset is a cl_camtype preset of back, side and
	// up (0: -48, 16, 12; 1: -48, -16, 12; 2: -90, 0, 12; 3: -cam_back, cam_right, cam_up), back held to
	// -128..24 and side and up to -24..24. Two eases at 5 a second ride on it: the height, toward
	// 32 - up while ducked (view height under 28 and not 0), else 0, added only while alive; and the
	// distance, toward -180 - back while dead, toward -92 - back while a stunt flag of 0x130c30 is set
	// (with back above -128), else 0. Dying turns the camera to third person (TSHealth, ts_status.cpp),
	// so this is also the death camera. A back of 0 puts the eye at the player's eye, 8 up ducked and 1
	// otherwise, pushed by the pitch; anything else offsets the eye, then a line from 28 + up above the
	// player's origin (the origin itself when that point is in solid) to it pulls the eye in to 4 units
	// off the wall it hits, 1 higher. The local model's height then eases toward its origin at 16 a
	// second.
	if (0 != CL_IsThirdPerson())
	{
		static float s_flCamUp = 0, s_flCamBack = 0, s_flModelZ = 0;
		Vector camOfs;
		camOfs[0] = camOfs[1] = camOfs[2] = 0.0f;
		CL_CameraOffset((float*)&camOfs);

		VectorCopy(camOfs, camAngles);
		camAngles[ROLL] = 0;
		AngleVectors(camAngles, camForward, camRight, camUp);

		const bool ducked = pparams->viewheight[2] < 28.0f && pparams->viewheight[2] != 0.0f;
		float back, side, lift = 12.0f;
		switch ((int)cl_camtype->value)
		{
		case 1:  back = -48.0f; side = -16.0f; break;
		case 2:  back = -90.0f; side =   0.0f; break;
		case 3:  back = -cam_back->value; side = cam_right->value; lift = cam_up->value; break;
		default: back = -48.0f; side =  16.0f; break;
		}
		if (back > -128.0f && back >= 24.0f)
			back = 24.0f;
		else if (back <= -128.0f)
			back = -128.0f;
		if (side < -24.0f) side = -24.0f; else if (side > 24.0f) side = 24.0f;
		if (lift < -24.0f) lift = -24.0f; else if (lift > 24.0f) lift = 24.0f;

		const float upTarget = (ducked && lift < 32.0f) ? 32.0f - lift : 0.0f;
		s_flCamUp += (upTarget - s_flCamUp) * pparams->frametime * 5.0f;
		float backTarget = 0.0f;
		if (pparams->health < 1)
			backTarget = -180.0f - back;
		else if (ent && (ent->prevstate.iuser4 & 0x130c30) != 0 && back > -128.0f)
			backTarget = -92.0f - back;
		s_flCamBack += (backTarget - s_flCamBack) * pparams->frametime * 5.0f;
		back += s_flCamBack;
		if (pparams->health > 0)
			lift += s_flCamUp;

		if (back == 0.0f)
		{
			float push = sin(camAngles[PITCH] * (M_PI / 180.0));
			if (push < 0.0f)
				push = 0.0f;
			for (i = 0; i < 3; i++)
				pparams->vieworg[i] = (ent ? ent->origin[i] : 0.0f) + pparams->viewheight[i];
			pparams->vieworg[2] += ducked ? 8.0f : 1.0f;
			for (i = 0; i < 3; i++)
			{
				if (ducked)
					pparams->vieworg[i] += (1.0f - push) * camForward[i] * 8.0f;
				pparams->vieworg[i] += push * camForward[i] * 4.0f + push * camUp[i] * 18.0f;
			}
		}
		else
		{
			for (i = 0; i < 3; i++)
				pparams->vieworg[i] += back * camForward[i] + lift * camUp[i] + side * camRight[i];
			if (ent)
			{
				Vector start = ent->origin;
				start[2] += lift + 28.0f;
				if (gEngfuncs.PM_PointContents(start, NULL) == CONTENTS_SOLID)
					start = ent->origin;
				pmtrace_t* tr = gEngfuncs.PM_TraceLine(start, pparams->vieworg, PM_NORMAL, 2, -1);
				if (tr && tr->fraction != 1.0f)
				{
					for (i = 0; i < 3; i++)
						pparams->vieworg[i] = tr->endpos[i] + tr->plane.normal[i] * 4.0f;
					pparams->vieworg[2] += 1.0f;
				}
			}
		}

		if (ent)
		{
			const float d = s_flModelZ - ent->origin[2];
			float step = d * pparams->frametime * 16.0f;
			if (fabs(d) < fabs(step))
				step = d;
			s_flModelZ -= step;
			ent->origin[2] = s_flModelZ;
		}
	}

	// Give gun our viewangles
	VectorCopy(pparams->cl_viewangles, view->angles);

	// set up gun position; the bob's amplitude goes on its pitch, so the gun aims higher the faster it
	// moves (0x100674a0)
	V_CalcGunAngle(pparams, bob);

	// client.dll (0x10068547) starts the view model at the eye as it stands here, already shaken, and
	// shakes it again at 0.9.
	VectorCopy(pparams->vieworg, view->origin);
	gEngfuncs.V_ApplyShake(view->origin, view->angles, 0.9);

	// The weapon bob (V_CalcBob, client.dll 0x10066e70): its offset goes on the view model's origin
	// after the shake (0x10068571).
	VectorAdd(view->origin, s_bobOffset, view->origin);

	// Then forward by (80 - the HUD field of view) x 0.15 (0x100685a7): a zoomed gun sits further out.
	for (i = 0; i < 3; i++)
		view->origin[i] += (80 - gHUD.m_iFOV) * 0.15f * pparams->forward[i];

	// pushing the view origin down off of the same X/Z plane as the ent's origin will give the
	// gun a very nice 'shifting' effect when the player looks up/down. If there is a problem
	// with view model distortion, this may be a cause. (SJB).
	view->origin[2] -= 1;

	// fudge position around to keep amount of weapon visible
	// roughly equal with different FOV
	if (pparams->viewsize == 110)
	{
		view->origin[2] += 1;
	}
	else if (pparams->viewsize == 100)
	{
		view->origin[2] += 2;
	}
	else if (pparams->viewsize == 90)
	{
		view->origin[2] += 1;
	}
	else if (pparams->viewsize == 80)
	{
		view->origin[2] += 0.5;
	}

	// Add in the punchangle, if any
	VectorAdd(pparams->viewangles, pparams->punchangle, pparams->viewangles);

	// Include client side punch, too
	VectorAdd(pparams->viewangles, (float*)&ev_punchangle, pparams->viewangles);

	V_DropPunchAngle(pparams->frametime, (float*)&ev_punchangle);

	// TS melee shake ("bounce-back"): a transient damped oscillation on the RENDER view only (decays to 0,
	// returns to where it was). Triggered per knife/katana swing by TS_TriggerMeleeShake (CTSGun::MeleeAttack).
	TS_ApplyMeleeShake(pparams);

	// smooth out stair step ups
#if 1
	if (0 == pparams->smoothing && 0 != pparams->onground && pparams->simorg[2] - oldz > 0)
	{
		float steptime;

		steptime = pparams->time - lasttime;
		if (steptime < 0)
			//FIXME		I_Error ("steptime < 0");
			steptime = 0;

		oldz += steptime * 150;
		if (oldz > pparams->simorg[2])
			oldz = pparams->simorg[2];
		if (pparams->simorg[2] - oldz > 18)
			oldz = pparams->simorg[2] - 18;
		pparams->vieworg[2] += oldz - pparams->simorg[2];
		view->origin[2] += oldz - pparams->simorg[2];
	}
	else
	{
		oldz = pparams->simorg[2];
	}
#endif

	{
		static float lastorg[3];
		Vector delta;

		VectorSubtract(pparams->simorg, lastorg, delta);

		if (Length(delta) != 0.0)
		{
			VectorCopy(pparams->simorg, ViewInterp.Origins[ViewInterp.CurrentOrigin & ORIGIN_MASK]);
			ViewInterp.OriginTime[ViewInterp.CurrentOrigin & ORIGIN_MASK] = pparams->time;
			ViewInterp.CurrentOrigin++;

			VectorCopy(pparams->simorg, lastorg);
		}
	}

	// Smooth out whole view in multiplayer when on trains, lifts
	if (cl_vsmoothing && 0 != cl_vsmoothing->value &&
		(0 != pparams->smoothing && (pparams->maxclients > 1)))
	{
		int foundidx;
		int i;
		float t;

		if (cl_vsmoothing->value < 0.0)
		{
			gEngfuncs.Cvar_SetValue("cl_vsmoothing", 0.0);
		}

		t = pparams->time - cl_vsmoothing->value;

		for (i = 1; i < ORIGIN_MASK; i++)
		{
			foundidx = ViewInterp.CurrentOrigin - 1 - i;
			if (ViewInterp.OriginTime[foundidx & ORIGIN_MASK] <= t)
				break;
		}

		if (i < ORIGIN_MASK && ViewInterp.OriginTime[foundidx & ORIGIN_MASK] != 0.0)
		{
			// Interpolate
			Vector delta;
			double frac;
			double dt;
			Vector neworg;

			dt = ViewInterp.OriginTime[(foundidx + 1) & ORIGIN_MASK] - ViewInterp.OriginTime[foundidx & ORIGIN_MASK];
			if (dt > 0.0)
			{
				frac = (t - ViewInterp.OriginTime[foundidx & ORIGIN_MASK]) / dt;
				frac = V_min(1.0, frac);
				VectorSubtract(ViewInterp.Origins[(foundidx + 1) & ORIGIN_MASK], ViewInterp.Origins[foundidx & ORIGIN_MASK], delta);
				VectorMA(ViewInterp.Origins[foundidx & ORIGIN_MASK], frac, delta, neworg);

				// Dont interpolate large changes
				if (Length(delta) < 64)
				{
					VectorSubtract(neworg, pparams->simorg, delta);

					VectorAdd(pparams->simorg, delta, pparams->simorg);
					VectorAdd(pparams->vieworg, delta, pparams->vieworg);
					VectorAdd(view->origin, delta, view->origin);
				}
			}
		}
	}

	// Store off v_angles before munging for third person
	v_angles = pparams->viewangles;
	v_client_aimangles = pparams->cl_viewangles;
	v_lastAngles = pparams->viewangles;
	//	v_cl_angles = pparams->cl_viewangles;	// keep old user mouse angles !
	if (0 != CL_IsThirdPerson())
	{
		VectorCopy(camAngles, pparams->viewangles);
	}

	// client.dll slams the local model's pitch from the camera's only in third person (0x10068a2c)
	if (0 != CL_IsThirdPerson())
	{
		float pitch = pparams->viewangles[0];

		// Normalize angles
		if (pitch > 180)
			pitch -= 360.0;
		else if (pitch < -180)
			pitch += 360;

		// Player pitch is inverted
		pitch /= -3.0;

		// Slam local player's pitch value
		ent->angles[0] = pitch;
		ent->curstate.angles[0] = pitch;
		ent->prevstate.angles[0] = pitch;
		ent->latched.prevangles[0] = pitch;
	}

	// override all previous settings if the viewent isn't the client
	if (pparams->viewentity > pparams->maxclients)
	{
		cl_entity_t* viewentity;
		viewentity = gEngfuncs.GetEntityByIndex(pparams->viewentity);
		if (viewentity)
		{
			VectorCopy(viewentity->origin, pparams->vieworg);
			VectorCopy(viewentity->angles, pparams->viewangles);

			// Store off overridden viewangles
			v_angles = pparams->viewangles;
		}
	}

	// Update the latched view origin/angles here, this was
	// previously done in V_CalcGunAngle but that happens
	// before a bunch of other stuff happens, which nukes
	// a bunch of the viewbob fx.
	VectorCopy(view->origin, view->curstate.origin);
	VectorCopy(view->origin, view->latched.prevorigin);
	VectorCopy(view->angles, view->curstate.angles);
	VectorCopy(view->angles, view->latched.prevangles);

	lasttime = pparams->time;

	v_origin = pparams->vieworg;
}

void V_SmoothInterpolateAngles(float* startAngle, float* endAngle, float* finalAngle, float degreesPerSec)
{
	float absd, frac, d, threshhold;

	NormalizeAngles(startAngle);
	NormalizeAngles(endAngle);

	for (int i = 0; i < 3; i++)
	{
		d = endAngle[i] - startAngle[i];

		if (d > 180.0f)
		{
			d -= 360.0f;
		}
		else if (d < -180.0f)
		{
			d += 360.0f;
		}

		absd = fabs(d);

		if (absd > 0.01f)
		{
			frac = degreesPerSec * v_frametime;

			threshhold = degreesPerSec / 4;

			if (absd < threshhold)
			{
				float h = absd / threshhold;
				h *= h;
				frac *= h; // slow down last degrees
			}

			if (frac > absd)
			{
				finalAngle[i] = endAngle[i];
			}
			else
			{
				if (d > 0)
					finalAngle[i] = startAngle[i] + frac;
				else
					finalAngle[i] = startAngle[i] - frac;
			}
		}
		else
		{
			finalAngle[i] = endAngle[i];
		}
	}

	NormalizeAngles(finalAngle);
}

// Get the origin of the Observer based around the target's position and angles
void V_GetChaseOrigin(float* angles, float* origin, float distance, float* returnvec)
{
	Vector vecEnd;
	Vector forward;
	Vector vecStart;
	pmtrace_t* trace;
	int maxLoops = 8;

	int ignoreent = -1; // first, ignore no entity

	cl_entity_t* ent = NULL;

	// Trace back from the target using the player's view angles
	AngleVectors(angles, forward, NULL, NULL);

	VectorScale(forward, -1, forward);

	VectorCopy(origin, vecStart);

	VectorMA(vecStart, distance, forward, vecEnd);

	while (maxLoops > 0)
	{
		trace = gEngfuncs.PM_TraceLine(vecStart, vecEnd, PM_TRACELINE_PHYSENTSONLY, 2, ignoreent);

		// WARNING! trace->ent is is the number in physent list not the normal entity number

		if (trace->ent <= 0)
			break; // we hit the world or nothing, stop trace

		ent = gEngfuncs.GetEntityByIndex(PM_GetPhysEntInfo(trace->ent));

		if (ent == NULL)
			break;

		// hit non-player solid BSP , stop here
		if (ent->curstate.solid == SOLID_BSP && 0 == ent->player)
			break;

		// if close enought to end pos, stop, otherwise continue trace
		if (Distance(trace->endpos, vecEnd) < 1.0f)
		{
			break;
		}
		else
		{
			ignoreent = trace->ent; // ignore last hit entity
			VectorCopy(trace->endpos, vecStart);
		}

		maxLoops--;
	}

	/*	if ( ent )
	{
		gEngfuncs.Con_Printf("Trace loops %i , entity %i, model %s, solid %i\n",(8-maxLoops),ent->curstate.number, ent->model->name , ent->curstate.solid ); 
	} */

	VectorMA(trace->endpos, 4, trace->plane.normal, returnvec);

	v_lastDistance = Distance(trace->endpos, origin); // real distance without offset
}

/*void V_GetDeathCam(cl_entity_t * ent1, cl_entity_t * ent2, float * angle, float * origin)
{
	float newAngle[3]; float newOrigin[3]; 

	float distance = 168.0f;

	v_lastDistance+= v_frametime * 96.0f;	// move unit per seconds back

	if ( v_resetCamera )
		v_lastDistance = 64.0f;

	if ( distance > v_lastDistance )
		distance = v_lastDistance;

	VectorCopy(ent1->origin, newOrigin);

	if ( ent1->player )
		newOrigin[2]+= 17; // head level of living player

	// get new angle towards second target
	if ( ent2 )
	{
		VectorSubtract( ent2->origin, ent1->origin, newAngle );
		VectorAngles( newAngle, newAngle );
		newAngle[0] = -newAngle[0];
	}
	else
	{
		// if no second target is given, look down to dead player
		newAngle[0] = 90.0f;
		newAngle[1] = 0.0f;
		newAngle[2] = 0;
	}

	// and smooth view
	V_SmoothInterpolateAngles( v_lastAngles, newAngle, angle, 120.0f );
			
	V_GetChaseOrigin( angle, newOrigin, distance, origin );

	VectorCopy(angle, v_lastAngles);
}*/

void V_GetSingleTargetCam(cl_entity_t* ent1, float* angle, float* origin)
{
	float newAngle[3];
	float newOrigin[3];

	int flags = gHUD.m_Spectator.m_iObserverFlags;

	// see is target is a dead player
	bool deadPlayer = 0 != ent1->player && (ent1->curstate.solid == SOLID_NOT);

	float dfactor = (flags & DRC_FLAG_DRAMATIC) != 0 ? -1.0f : 1.0f;

	float distance = 112.0f + (16.0f * dfactor); // get close if dramatic;

	// go away in final scenes or if player just died
	if ((flags & DRC_FLAG_FINAL) != 0)
		distance *= 2.0f;
	else if (deadPlayer)
		distance *= 1.5f;

	// let v_lastDistance float smoothly away
	v_lastDistance += v_frametime * 32.0f; // move unit per seconds back

	if (distance > v_lastDistance)
		distance = v_lastDistance;

	VectorCopy(ent1->origin, newOrigin);

	if (0 != ent1->player)
	{
		if (deadPlayer)
			newOrigin[2] += 2; //laying on ground
		else
			newOrigin[2] += 17; // head level of living player
	}
	else
		newOrigin[2] += 8; // object, tricky, must be above bomb in CS

	// we have no second target, choose view direction based on
	// show front of primary target
	VectorCopy(ent1->angles, newAngle);

	// show dead players from front, normal players back
	if ((flags & DRC_FLAG_FACEPLAYER) != 0)
		newAngle[1] += 180.0f;


	newAngle[0] += 12.5f * dfactor; // lower angle if dramatic

	// if final scene (bomb), show from real high pos
	if ((flags & DRC_FLAG_FINAL) != 0)
		newAngle[0] = 22.5f;

	// choose side of object/player
	if ((flags & DRC_FLAG_SIDE) != 0)
		newAngle[1] += 22.5f;
	else
		newAngle[1] -= 22.5f;

	V_SmoothInterpolateAngles(v_lastAngles, newAngle, angle, 120.0f);

	// HACK, if player is dead don't clip against his dead body, can't check this
	V_GetChaseOrigin(angle, newOrigin, distance, origin);
}

float MaxAngleBetweenAngles(float* a1, float* a2)
{
	float d, maxd = 0.0f;

	NormalizeAngles(a1);
	NormalizeAngles(a2);

	for (int i = 0; i < 3; i++)
	{
		d = a2[i] - a1[i];
		if (d > 180)
		{
			d -= 360;
		}
		else if (d < -180)
		{
			d += 360;
		}

		d = fabs(d);

		if (d > maxd)
			maxd = d;
	}

	return maxd;
}

void V_GetDoubleTargetsCam(cl_entity_t* ent1, cl_entity_t* ent2, float* angle, float* origin)
{
	float newAngle[3];
	float newOrigin[3];
	float tempVec[3];

	int flags = gHUD.m_Spectator.m_iObserverFlags;

	float dfactor = (flags & DRC_FLAG_DRAMATIC) != 0 ? -1.0f : 1.0f;

	float distance = 112.0f + (16.0f * dfactor); // get close if dramatic;

	// go away in final scenes or if player just died
	if ((flags & DRC_FLAG_FINAL) != 0)
		distance *= 2.0f;

	// let v_lastDistance float smoothly away
	v_lastDistance += v_frametime * 32.0f; // move unit per seconds back

	if (distance > v_lastDistance)
		distance = v_lastDistance;

	VectorCopy(ent1->origin, newOrigin);

	if (0 != ent1->player)
		newOrigin[2] += 17; // head level of living player
	else
		newOrigin[2] += 8; // object, tricky, must be above bomb in CS

	// get new angle towards second target
	VectorSubtract(ent2->origin, ent1->origin, newAngle);

	VectorAngles(newAngle, newAngle);
	newAngle[0] = -newAngle[0];

	// set angle diffrent in Dramtaic scenes
	newAngle[0] += 12.5f * dfactor; // lower angle if dramatic

	if ((flags & DRC_FLAG_SIDE) != 0)
		newAngle[1] += 22.5f;
	else
		newAngle[1] -= 22.5f;

	float d = MaxAngleBetweenAngles(v_lastAngles, newAngle);

	if ((d < v_cameraFocusAngle) && (v_cameraMode == CAM_MODE_RELAX))
	{
		// difference is to small and we are in relax camera mode, keep viewangles
		VectorCopy(v_lastAngles, newAngle);
	}
	else if ((d < v_cameraRelaxAngle) && (v_cameraMode == CAM_MODE_FOCUS))
	{
		// we catched up with our target, relax again
		v_cameraMode = CAM_MODE_RELAX;
	}
	else
	{
		// target move too far away, focus camera again
		v_cameraMode = CAM_MODE_FOCUS;
	}

	// and smooth view, if not a scene cut
	if (v_resetCamera || (v_cameraMode == CAM_MODE_RELAX))
	{
		VectorCopy(newAngle, angle);
	}
	else
	{
		V_SmoothInterpolateAngles(v_lastAngles, newAngle, angle, 180.0f);
	}

	V_GetChaseOrigin(newAngle, newOrigin, distance, origin);

	// move position up, if very close at target
	if (v_lastDistance < 64.0f)
		origin[2] += 16.0f * (1.0f - (v_lastDistance / 64.0f));

	// calculate angle to second target
	VectorSubtract(ent2->origin, origin, tempVec);
	VectorAngles(tempVec, tempVec);
	tempVec[0] = -tempVec[0];

	/* take middle between two viewangles
	InterpolateAngles( newAngle, tempVec, newAngle, 0.5f); */
}

void V_GetDirectedChasePosition(cl_entity_t* ent1, cl_entity_t* ent2, float* angle, float* origin)
{

	if (v_resetCamera)
	{
		v_lastDistance = 4096.0f;
		// v_cameraMode = CAM_MODE_FOCUS;
	}

	if ((ent2 == (cl_entity_t*)0xFFFFFFFF) || (0 != ent1->player && (ent1->curstate.solid == SOLID_NOT)))
	{
		// we have no second target or player just died
		V_GetSingleTargetCam(ent1, angle, origin);
	}
	else if (ent2)
	{
		// keep both target in view
		V_GetDoubleTargetsCam(ent1, ent2, angle, origin);
	}
	else
	{
		// second target disappeard somehow (dead)

		// keep last good viewangle
		float newOrigin[3];

		int flags = gHUD.m_Spectator.m_iObserverFlags;

		float dfactor = (flags & DRC_FLAG_DRAMATIC) != 0 ? -1.0f : 1.0f;

		float distance = 112.0f + (16.0f * dfactor); // get close if dramatic;

		// go away in final scenes or if player just died
		if ((flags & DRC_FLAG_FINAL) != 0)
			distance *= 2.0f;

		// let v_lastDistance float smoothly away
		v_lastDistance += v_frametime * 32.0f; // move unit per seconds back

		if (distance > v_lastDistance)
			distance = v_lastDistance;

		VectorCopy(ent1->origin, newOrigin);

		if (0 != ent1->player)
			newOrigin[2] += 17; // head level of living player
		else
			newOrigin[2] += 8; // object, tricky, must be above bomb in CS

		V_GetChaseOrigin(angle, newOrigin, distance, origin);
	}

	VectorCopy(angle, v_lastAngles);
}

void V_GetChasePos(int target, float* cl_angles, float* origin, float* angles)
{
	cl_entity_t* ent = NULL;

	if (0 != target)
	{
		ent = gEngfuncs.GetEntityByIndex(target);
	}

	if (!ent)
	{
		// just copy a save in-map position
		VectorCopy(vJumpAngles, angles);
		VectorCopy(vJumpOrigin, origin);
		return;
	}



	if (0 != gHUD.m_Spectator.m_autoDirector->value)
	{
		if (0 != g_iUser3)
			V_GetDirectedChasePosition(ent, gEngfuncs.GetEntityByIndex(g_iUser3),
				angles, origin);
		else
			V_GetDirectedChasePosition(ent, (cl_entity_t*)0xFFFFFFFF,
				angles, origin);
	}
	else
	{
		if (cl_angles == NULL) // no mouse angles given, use entity angles ( locked mode )
		{
			VectorCopy(ent->angles, angles);
			angles[0] *= -1;
		}
		else
			VectorCopy(cl_angles, angles);


		VectorCopy(ent->origin, origin);

		VectorAdd(origin, VEC_VIEW, origin); // some offset

		V_GetChaseOrigin(angles, origin, V_CHASE_DISTANCE, origin);
	}

	v_resetCamera = false;
}

void V_ResetChaseCam()
{
	v_resetCamera = true;
}


void V_GetInEyePos(int target, float* origin, float* angles)
{
	if (0 == target)
	{
		// just copy a save in-map position
		VectorCopy(vJumpAngles, angles);
		VectorCopy(vJumpOrigin, origin);
		return;
	}


	cl_entity_t* ent = gEngfuncs.GetEntityByIndex(target);

	if (!ent)
		return;

	VectorCopy(ent->origin, origin);
	VectorCopy(ent->angles, angles);

	angles[PITCH] *= -3.0f; // see CL_ProcessEntityUpdate()

	if (ent->curstate.solid == SOLID_NOT)
	{
		angles[ROLL] = 80; // dead view angle
		VectorAdd(origin, VEC_DEAD_VIEW, origin);
	}
	else if (ent->curstate.usehull == 1)
	{
		VectorAdd(origin, VEC_DUCK_VIEW, origin);
	}
	else
		// exacty eye position can't be caluculated since it depends on
		// client values like cl_bobcycle, this offset matches the default values
		VectorAdd(origin, VEC_VIEW, origin);
}

void V_GetMapFreePosition(float* cl_angles, float* origin, float* angles)
{
	Vector forward;
	Vector zScaledTarget;

	VectorCopy(cl_angles, angles);

	// modify angles since we don't wanna see map's bottom
	angles[0] = 51.25f + 38.75f * (angles[0] / 90.0f);

	zScaledTarget[0] = gHUD.m_Spectator.m_mapOrigin[0];
	zScaledTarget[1] = gHUD.m_Spectator.m_mapOrigin[1];
	zScaledTarget[2] = gHUD.m_Spectator.m_mapOrigin[2] * ((90.0f - angles[0]) / 90.0f);


	AngleVectors(angles, forward, NULL, NULL);

	VectorNormalize(forward);

	VectorMA(zScaledTarget, -(4096.0f / gHUD.m_Spectator.m_mapZoom), forward, origin);
}

void V_GetMapChasePosition(int target, float* cl_angles, float* origin, float* angles)
{
	Vector forward;

	if (0 != target)
	{
		cl_entity_t* ent = gEngfuncs.GetEntityByIndex(target);

		if (0 != gHUD.m_Spectator.m_autoDirector->value)
		{
			// this is done to get the angles made by director mode
			V_GetChasePos(target, cl_angles, origin, angles);
			VectorCopy(ent->origin, origin);

			// keep fix chase angle horizontal
			angles[0] = 45.0f;
		}
		else
		{
			VectorCopy(cl_angles, angles);
			VectorCopy(ent->origin, origin);

			// modify angles since we don't wanna see map's bottom
			angles[0] = 51.25f + 38.75f * (angles[0] / 90.0f);
		}
	}
	else
	{
		// keep out roaming position, but modify angles
		VectorCopy(cl_angles, angles);
		angles[0] = 51.25f + 38.75f * (angles[0] / 90.0f);
	}

	origin[2] *= ((90.0f - angles[0]) / 90.0f);
	angles[2] = 0.0f; // don't roll angle (if chased player is dead)

	AngleVectors(angles, forward, NULL, NULL);

	VectorNormalize(forward);

	VectorMA(origin, -1536, forward, origin);
}

// Spectator in-eye: the view model for the spectated player's world weapon model. client.dll
// (0x10068ef0) takes the model's name and finds the first weapon whose p_ model, or silenced p_
// model, it starts with (types 0..37), then loads that weapon's v_ model (0x10069a3e).
int V_FindViewModelByWeaponModel(int weaponindex)
{
	model_t* m = IEngineStudio.GetModelByIndex(weaponindex);
	if (m == nullptr || m->name[0] == '\0')
		return 0;
	const size_t len = strlen(m->name);
	for (int t = 0; t < TSW_COUNT; t++)
	{
		const s_weapon_info* wi = get_weapon_info((tsweapon_t)t);
		if ((wi->pszPlayerModel && strncmp(m->name, wi->pszPlayerModel, len) == 0) ||
			(wi->pszPlayerModelSil && strncmp(m->name, wi->pszPlayerModelSil, len) == 0))
			return wi->pszViewModel ? gEngfuncs.pEventAPI->EV_FindModelIndex(wi->pszViewModel) : 0;
	}
	return 0;
}


/*
==================
V_CalcSpectatorRefdef

==================
*/
void V_CalcSpectatorRefdef(struct ref_params_s* pparams)
{
	static Vector velocity(0.0f, 0.0f, 0.0f);

	static int lastWeaponModelIndex = 0;
	static int lastViewModelIndex = 0;

	cl_entity_t* ent = gEngfuncs.GetEntityByIndex(g_iUser2);

	pparams->onlyClientDraw = 0;

	// refresh position
	VectorCopy(pparams->simorg, v_sim_org);

	// get old values
	VectorCopy(pparams->cl_viewangles, v_cl_angles);
	VectorCopy(pparams->viewangles, v_angles);
	VectorCopy(pparams->vieworg, v_origin);

	if ((g_iUser1 == OBS_IN_EYE || gHUD.m_Spectator.m_pip->value == INSET_IN_EYE) && ent)
	{
		// calculate player velocity
		float timeDiff = ent->curstate.msg_time - ent->prevstate.msg_time;

		if (timeDiff > 0)
		{
			Vector distance;
			VectorSubtract(ent->prevstate.origin, ent->curstate.origin, distance);
			VectorScale(distance, 1 / timeDiff, distance);

			velocity[0] = velocity[0] * 0.9f + distance[0] * 0.1f;
			velocity[1] = velocity[1] * 0.9f + distance[1] * 0.1f;
			velocity[2] = velocity[2] * 0.9f + distance[2] * 0.1f;

			VectorCopy(velocity, pparams->simvel);
		}

		// predict missing client data and set weapon model ( in HLTV mode or inset in eye mode )
		if (0 != gEngfuncs.IsSpectateOnly())
		{
			V_GetInEyePos(g_iUser2, pparams->simorg, pparams->cl_viewangles);

			pparams->health = 1;

			cl_entity_t* gunModel = gEngfuncs.GetViewModel();

			if (lastWeaponModelIndex != ent->curstate.weaponmodel)
			{
				// weapon model changed

				lastWeaponModelIndex = ent->curstate.weaponmodel;
				lastViewModelIndex = V_FindViewModelByWeaponModel(lastWeaponModelIndex);
				if (0 != lastViewModelIndex)
				{
					gEngfuncs.pfnWeaponAnim(0, 0); // reset weapon animation
				}
				else
				{
					// model not found
					gunModel->model = NULL; // disable weapon model
					lastWeaponModelIndex = lastViewModelIndex = 0;
				}
			}

			if (0 != lastViewModelIndex)
			{
				gunModel->model = IEngineStudio.GetModelByIndex(lastViewModelIndex);
				gunModel->curstate.modelindex = lastViewModelIndex;
				gunModel->curstate.frame = 0;
				gunModel->curstate.colormap = 0;
				gunModel->index = g_iUser2;
			}
			else
			{
				gunModel->model = NULL; // disable weaopn model
			}
		}
		else
		{
			// only get viewangles from entity
			VectorCopy(ent->angles, pparams->cl_viewangles);
			pparams->cl_viewangles[PITCH] *= -3.0f; // see CL_ProcessEntityUpdate()
		}
	}

	if (pparams->nextView == 0)
	{
		// first renderer cycle, full screen

		switch (g_iUser1)
		{
		case OBS_CHASE_LOCKED:
			V_GetChasePos(g_iUser2, NULL, v_origin, v_angles);
			break;

		case OBS_CHASE_FREE:
			V_GetChasePos(g_iUser2, v_cl_angles, v_origin, v_angles);
			break;

		case OBS_ROAMING:
			VectorCopy(v_cl_angles, v_angles);
			VectorCopy(v_sim_org, v_origin);

			// override values if director is active
			gHUD.m_Spectator.GetDirectorCamera(v_origin, v_angles);
			break;

		case OBS_IN_EYE:
			V_CalcNormalRefdef(pparams);
			break;

		case OBS_MAP_FREE:
			pparams->onlyClientDraw = 1;
			V_GetMapFreePosition(v_cl_angles, v_origin, v_angles);
			break;

		case OBS_MAP_CHASE:
			pparams->onlyClientDraw = 1;
			V_GetMapChasePosition(g_iUser2, v_cl_angles, v_origin, v_angles);
			break;
		}

		if (0 != gHUD.m_Spectator.m_pip->value)
			pparams->nextView = 1; // force a second renderer view

		gHUD.m_Spectator.m_iDrawCycle = 0;
	}
	else
	{
		// second renderer cycle, inset window

		// set inset parameters
		pparams->viewport[0] = XRES_HD(gHUD.m_Spectator.m_OverviewData.insetWindowX); // change viewport to inset window
		pparams->viewport[1] = YRES_HD(gHUD.m_Spectator.m_OverviewData.insetWindowY);
		pparams->viewport[2] = XRES_HD(gHUD.m_Spectator.m_OverviewData.insetWindowWidth);
		pparams->viewport[3] = YRES_HD(gHUD.m_Spectator.m_OverviewData.insetWindowHeight);
		pparams->nextView = 0; // on further view

		// override some settings in certain modes
		switch ((int)gHUD.m_Spectator.m_pip->value)
		{
		case INSET_CHASE_FREE:
			V_GetChasePos(g_iUser2, v_cl_angles, v_origin, v_angles);
			break;

		case INSET_IN_EYE:
			V_CalcNormalRefdef(pparams);
			break;

		case INSET_MAP_FREE:
			pparams->onlyClientDraw = 1;
			V_GetMapFreePosition(v_cl_angles, v_origin, v_angles);
			break;

		case INSET_MAP_CHASE:
			pparams->onlyClientDraw = 1;

			if (g_iUser1 == OBS_ROAMING)
				V_GetMapChasePosition(0, v_cl_angles, v_origin, v_angles);
			else
				V_GetMapChasePosition(g_iUser2, v_cl_angles, v_origin, v_angles);

			break;
		}

		gHUD.m_Spectator.m_iDrawCycle = 1;
	}

	// write back new values into pparams
	VectorCopy(v_cl_angles, pparams->cl_viewangles);
	VectorCopy(v_angles, pparams->viewangles)
		VectorCopy(v_origin, pparams->vieworg);
}



void DLLEXPORT V_CalcRefdef(struct ref_params_s* pparams)
{
	// client.dll 0x10069b9e: every view frame keeps its frametime (the HUD's FOV easing and the
	// spectator cameras read it), not only the spectator one
	v_frametime = pparams->frametime;

	//	RecClCalcRefdef(pparams);

	// intermission / finale rendering
	if (0 != pparams->intermission)
	{
		V_CalcIntermissionRefdef(pparams);
	}
	else if (0 != pparams->spectator || 0 != g_iUser1) // g_iUser true if in spectator mode
	{
		V_CalcSpectatorRefdef(pparams);
	}
	else if (0 == pparams->paused)
	{
		g_tspLocalSimVel = pparams->simvel; // client.dll keeps it in its normal refdef (0x1006789f)
		V_CalcNormalRefdef(pparams);
	}

	// The screen fade, set every frame (client.dll 0x10069bd0..0x10069e1e): the slow motion tint while
	// cl_slowfade is not 0 and the slow factor is under 1, alpha (1 - slow) x 50, else nothing; while the
	// last TSFade runs (its arrival to arrival + duration / 4096 s) a blend toward its colour, rising over
	// the first half of the time outside the hold, holding, then falling; flags 5 (out, stay out). The
	// fade's own flags and the long fade scale are not used. client.dll's tint is (0, 100, 125); reTS
	// keeps (0, 60, 160), set in the playtest of b8080fb.
	{
		extern float g_flClientSlow; // cl_dll/ts_powerup_hud.cpp
		static cvar_t* cl_slowfade = NULL;
		if (!cl_slowfade)
			cl_slowfade = gEngfuncs.pfnGetCvarPointer("cl_slowfade");
		int tr = 0, tg = 0, tb = 0, ta = 0;
		if (cl_slowfade && cl_slowfade->value != 0.0f && g_flClientSlow < 1.0f)
		{
			tg = 60;
			tb = 160;
			ta = (int)((1.0f - g_flClientSlow) * 50.0f);
		}
		screenfade_t sf;
		gEngfuncs.pfnGetScreenFade(&sf);
		const float dur = g_tsFade.duration * (1.0f / 4096.0f);
		const float hold = g_tsFade.hold * (1.0f / 4096.0f);
		const float t = pparams->time - g_tsFade.start;
		if (pparams->time > g_tsFade.start && g_tsFade.start + dur > pparams->time)
		{
			const float half = (dur - hold) * 0.5f;
			float k = 1.0f;
			if (t < half)
				k = t / half;
			else if (half + hold < t)
				k = 1.0f - (t - hold - half) / half;
			const float fa = g_tsFade.a * (1.0f / 255.0f);
			sf.fader = (byte)(int)(g_tsFade.r * k);
			sf.fadeg = (byte)(int)(g_tsFade.g * k + tg * (1.0f - k));
			sf.fadeb = (byte)(int)(g_tsFade.b * k + tb * (1.0f - k));
			sf.fadealpha = (byte)(int)(ta * (1.0f - k) + k * fa * 255.0f);
		}
		else
		{
			sf.fader = (byte)tr;
			sf.fadeg = (byte)tg;
			sf.fadeb = (byte)tb;
			sf.fadealpha = (byte)ta;
		}
		sf.fadeFlags = FFADE_OUT | FFADE_STAYOUT;
		gEngfuncs.pfnSetScreenFade(&sf);
	}

	// client.dll's last call here (0x10069e29): the fog pass, FUN_10056c20 (cl_dll/ts_emitter.cpp)
	TS_ApplyFog();

	/*
// Example of how to overlay the whole screen with red at 50 % alpha
#define SF_TEST
#if defined SF_TEST
	{
		screenfade_t sf;
		gEngfuncs.pfnGetScreenFade( &sf );

		sf.fader = 255;
		sf.fadeg = 0;
		sf.fadeb = 0;
		sf.fadealpha = 128;
		sf.fadeFlags = FFADE_STAYOUT | FFADE_OUT;

		gEngfuncs.pfnSetScreenFade( &sf );
	}
#endif
*/
}

/*
=============
V_DropPunchAngle

=============
*/
// client.dll 0x10069e40: the stock decay, slowed by the client's slow motion factor.
void V_DropPunchAngle(float frametime, float* ev_punchangle)
{
	float len;
	extern float g_flClientSlow;

	len = VectorNormalize(ev_punchangle);
	len -= (10.0 + len * 0.5) * g_flClientSlow * frametime;
	len = V_max(len, 0.0f);
	VectorScale(ev_punchangle, len, ev_punchangle);
}

/*
=============
V_PunchAxis

Client side punch effect
=============
*/
void V_PunchAxis(int axis, float punch)
{
	ev_punchangle[axis] = punch;
}

/*
=============
V_Init
=============
*/
void V_Init()
{
	gEngfuncs.pfnAddCommand("centerview", V_StartPitchDrift);

	scr_ofsx = gEngfuncs.pfnRegisterVariable("scr_ofsx", "0", 0);
	scr_ofsy = gEngfuncs.pfnRegisterVariable("scr_ofsy", "0", 0);
	scr_ofsz = gEngfuncs.pfnRegisterVariable("scr_ofsz", "0", 0);

	v_centermove = gEngfuncs.pfnRegisterVariable("v_centermove", "0.15", 0);
	v_centerspeed = gEngfuncs.pfnRegisterVariable("v_centerspeed", "500", 0);

	// client.dll V_Init (0x10069e40): cl_bobcycle and cl_bobup are registered and never read; cl_bob is the
	// weapon bob's level (V_CalcBob), default "4.0"; cl_slowfade (flags 0) gates the slow-motion tint.
	cl_bobcycle = gEngfuncs.pfnRegisterVariable("cl_bobcycle", "0.8", 0);
	cl_bob = gEngfuncs.pfnRegisterVariable("cl_bob", "4.0", FCVAR_ARCHIVE);
	cl_bobup = gEngfuncs.pfnRegisterVariable("cl_bobup", "0.5", 0);
	cl_waterdist = gEngfuncs.pfnRegisterVariable("cl_waterdist", "4", 0);
	gEngfuncs.pfnRegisterVariable("cl_slowfade", "1", 0);
}


//#define TRACE_TEST
#if defined(TRACE_TEST)

extern float in_fov;
/*
====================
CalcFov
====================
*/
float CalcFov(float fov_x, float width, float height)
{
	float a;
	float x;

	if (fov_x < 1 || fov_x > 179)
		fov_x = 90; // error, set to 90

	x = width / tan(fov_x / 360 * M_PI);

	a = atan(height / x);

	a = a * 360 / M_PI;

	return a;
}

int hitent = -1;

void V_Move(int mx, int my)
{
	float fov;
	float fx, fy;
	float dx, dy;
	float c_x, c_y;
	float dX, dY;
	Vector forward, up, right;
	Vector newangles;

	Vector farpoint;
	pmtrace_t tr;

	fov = CalcFov(in_fov, (float)ScreenWidth, (float)ScreenHeight);

	c_x = (float)ScreenWidth / 2.0;
	c_y = (float)ScreenHeight / 2.0;

	dx = (float)mx - c_x;
	dy = (float)my - c_y;

	// Proportion we moved in each direction
	fx = dx / c_x;
	fy = dy / c_y;

	dX = fx * in_fov / 2.0;
	dY = fy * fov / 2.0;

	newangles = v_angles;

	newangles[YAW] -= dX;
	newangles[PITCH] += dY;

	// Now rotate v_forward around that point
	AngleVectors(newangles, forward, right, up);

	farpoint = v_origin + 8192 * forward;

	// Trace
	tr = *(gEngfuncs.PM_TraceLine((float*)&v_origin, (float*)&farpoint, PM_TRACELINE_PHYSENTSONLY, 2 /*point sized hull*/, -1));

	if (tr.fraction != 1.0 && tr.ent != 0)
	{
		hitent = PM_GetPhysEntInfo(tr.ent);
		PM_ParticleLine((float*)&v_origin, (float*)&tr.endpos, 5, 1.0, 0.0);
	}
	else
	{
		hitent = -1;
	}
}

#endif
