/***
*
*	Copyright (c) 1996-2002, Valve LLC. All rights reserved.
*
*	This product contains software technology licensed from Id
*	Software, Inc. ("Id Technology").  Id Technology (c) 1996 Id Software, Inc.
*	All Rights Reserved.
*
*   Use, distribution, and modification of this source code and/or resulting
*   object code is restricted to non-commercial enhancements to products from
*   Valve LLC.  All other use, distribution, or modification is prohibited
*   without written permission from Valve LLC.
*
****/
//
// ev_ts.cpp
//
// The Specialists predicted weapon fire event (events/fire.sc → EV_TSFire). The server
// (CTSGun::StandardFire) plays the event with iparam1 = the tsweapon_t; here we play the
// per-weapon fire sound, the viewmodel muzzleflash + "shoot" animation, and eject a
// shell — the faithful event path that replaces the server-driven SendWeaponAnim +
// EMIT_SOUND slice. The sound/anim tables mirror the server s_weapon_info
// (dlls/ts_weapons.cpp).
//

#include "hud.h"
#include "cl_util.h"
#include "const.h"
#include "entity_state.h"
#include "cl_entity.h"
#include "event_api.h"
#include "pm_defs.h"
#include "eventscripts.h"

#include <string.h>

void V_PunchAxis(int axis, float punch); // view recoil kick (cl_dll/view.cpp)

// Per-tsweapon_t fire sound (nullptr = silent: melee/grenade/silenced-base/akimbo).
static const char* const s_fireSound[] = {
	/* 0*/ nullptr, /* 1*/ "weapons/glock/glock-fire.wav", /* 2*/ "weapons/beretta/fire.wav",
	/* 3*/ "weapons/uzi/uzi-fire.wav", /* 4*/ "weapons/m3/m3-fire.wav", /* 5*/ "weapons/m4/m4-fire.wav",
	/* 6*/ nullptr, /* 7*/ "weapons/mp5k/mp5k-fire.wav", /* 8*/ "weapons/beretta/fire.wav",
	/* 9*/ "weapons/mk23/mk23-fire.wav", /*10*/ "weapons/mk23/mk23-fire.wav", /*11*/ "weapons/usas/usas-fire.wav",
	/*12*/ "weapons/deagle/fire.wav", /*13*/ "weapons/ak47/fire.wav", /*14*/ "weapons/fnh/fnh-fire.wav",
	/*15*/ "weapons/aug/aug-fire.wav", /*16*/ "weapons/uzi/uzi-fire.wav", /*17*/ "weapons/tmp/fire.wav",
	/*18*/ "weapons/barrett/fire.wav", /*19*/ "weapons/pdw/fire.wav", /*20*/ "weapons/spas12/spas12-fire.wav",
	/*21*/ "weapons/gold/gold-fire.wav", /*22*/ "weapons/glock22/glock22-fire.wav", /*23*/ "weapons/ump/ump-fire.wav",
	/*24*/ nullptr, /*25*/ nullptr, /*26*/ "weapons/mossberg/mossberg-fire.wav", /*27*/ "weapons/m16/fire.wav",
	/*28*/ "weapons/ruger/ruger-fire.wav", /*29*/ nullptr, /*30*/ "weapons/fnh/fnh-fire.wav",
	/*31*/ "weapons/bull/fire01.wav", /*32*/ "weapons/m60/m60-fire.wav", /*33*/ "weapons/sawedoff/fire.wav",
	/*34*/ nullptr, /*35*/ nullptr, /*36*/ "weapons/contender/fire.wav", /*37*/ "weapons/tmp/fire.wav",
};

// Per-tsweapon_t viewmodel "shoot" sequence (-1 = none), mirroring dlls/ts_weapons.cpp kFireAnim.
static const int s_fireAnim[] = {
	-1, 3, 1, 3, 6, 3, 1, 3, 3, 1, 1, 3, 3, 3, 3, 2, 18, 3, 3, 3,
	6, 3, 3, 3, -1, -1, 1, 3, 1, -1, 3, 1, 2, 3, -1, -1, 3, 6,
};
static const int TSW_NUM = sizeof(s_fireAnim) / sizeof(s_fireAnim[0]);

void EV_TSFire(event_args_t* args)
{
	const int idx = args->entindex;
	const int weaponType = args->iparam1;
	if (weaponType < 0 || weaponType >= TSW_NUM)
		return;

	Vector origin;
	VectorCopy(args->origin, origin);

	if (EV_IsLocal(idx))
	{
		EV_MuzzleFlash();
		// NOTE: the viewmodel "shoot" anim is driven server-side via SendWeaponAnim
		// (CTSGun::StandardFire) — EV_WeaponAnimation here doesn't stick because CTSGun
		// isn't client-predicted.
		V_PunchAxis(0, -2.0);
	}

	// Eject a brass shell from the viewmodel/player.
	Vector angles, velocity, fwd, right, up, shellVel, shellOrigin;
	VectorCopy(args->angles, angles);
	VectorCopy(args->velocity, velocity);
	AngleVectors(angles, fwd, right, up);
	const int shell = gEngfuncs.pEventAPI->EV_FindModelIndex("models/shell.mdl");
	EV_GetDefaultShellInfo(args, origin, velocity, shellVel, shellOrigin, fwd, right, up, 20, -12, 4);
	EV_EjectBrass(shellOrigin, shellVel, angles[1], shell, TE_BOUNCE_SHELL);

	const char* snd = s_fireSound[weaponType];
	if (snd)
		gEngfuncs.pEventAPI->EV_PlaySound(idx, origin, CHAN_WEAPON, snd, VOL_NORM, ATTN_NORM, 0, PITCH_NORM);
}
