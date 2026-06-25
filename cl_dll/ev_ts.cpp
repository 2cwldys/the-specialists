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
#include "pm_materials.h" // CHAR_TEX_* material classes
#include "pm_shared.h"    // PM_FindTextureType
#include "eventscripts.h"
#include "ts_weapon_info.h" // the SHARED s_weapon_info table (now built into the client too)
#include "ts_weapon_predict.h" // the active-weapon param cache (refreshed by each TS event)
#include "ts_recoil.h"      // TS_AddViewRecoil — the per-shot aim-recoil kick
#include "ts_particles.h"   // TS_ImpactEffect — the byte-ported per-surface impact dispatcher

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

// Muzzle-flash grip for tsweapon_t `w`: true = one-handed (pistols + akimbo barrels) -> the small
// ts_muzzleflash1h sprite; false = two-handed (rifles/SMGs/shotguns) -> the larger ts_muzzleflash6.
// Read from the SHARED s_weapon_info pszAnimExt (onehanded/akimbo vs twohanded) — the same table
// the server builds. (RE: client.dll FUN_100474b0 loads ts_muzzleflash1h + ts_muzzleflash6.)
static bool TSWeaponOneHanded(int w)
{
	const s_weapon_info* wi = get_weapon_info((tsweapon_t)w);
	return wi->pszAnimExt && strcmp(wi->pszAnimExt, "twohanded") != 0; // onehanded/akimbo -> 1h
}

// Draw the TS muzzle-flash flare sprite at the firing gun's muzzle attachment. `side` selects the
// attachment (0 = right/primary barrel, 1 = left barrel for akimbo). RE: client.dll FUN_100474b0
// spawns a short-lived additive sprite at the muzzle; we use the engine temp-sprite at the model's
// attachment[side]. For the local player it's the viewmodel; otherwise the player's world model.
static void EV_TSMuzzleFlare(int idx, int weaponType, int side)
{
	static int s_spr1h = 0, s_spr6 = 0;
	if (s_spr1h == 0)
		s_spr1h = gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/ts_muzzleflash1h.spr");
	if (s_spr6 == 0)
		s_spr6 = gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/ts_muzzleflash6.spr");

	cl_entity_t* ent = EV_IsLocal(idx) ? gEngfuncs.GetViewModel() : gEngfuncs.GetEntityByIndex(idx);
	if (!ent)
		return;
	if (side < 0 || side > 3)
		side = 0;

	const bool oneHand = (weaponType >= 0 && weaponType < TSW_COUNT) && TSWeaponOneHanded(weaponType);
	const int spr = oneHand ? s_spr1h : s_spr6;
	if (spr <= 0)
		return;

	float pos[3];
	VectorCopy(ent->attachment[side], pos);
	const float scale = oneHand ? 0.4f : 0.7f;
	// A muzzle flash is a single brief pop — a very short life and NO frame-cycle (FTENT_SPRCYCLE
	// kept it animating/visible far too long, "slow enough to walk around"). One frame, gone fast.
	gEngfuncs.pEfxAPI->R_TempSprite(pos, vec3_origin, scale, spr,
		kRenderTransAdd, kRenderFxNoDissipation, 1.0f, 0.01f, 0);
}

// TS bullet impact: the byte-ported per-surface client particle dispatcher (FUN_10048f60),
// fed by a surface trace. Metal/glass -> scintille glint + sparks.spr shower (+ pellet marks);
// wood/concrete -> tumbling debris chips + smoke; dirt -> smoke; flesh/water -> nothing (the
// server's FireBulletsPlayer already does blood + the texture impact SOUND + the bullet decal,
// this only adds the missing VISUAL). Client-side + predicted via the fire event (no lag, no
// double sound/decal). The old R_SparkShower/R_TempSprite approximation is gone (cont.69x).
static void EV_TSBulletImpact(int idx, const float* src, const float* dir)
{
	Vector end;
	for (int i = 0; i < 3; i++)
		end[i] = src[i] + dir[i] * 8192.0f;

	gEngfuncs.pEventAPI->EV_SetUpPlayerPrediction(0, 1);
	gEngfuncs.pEventAPI->EV_PushPMStates();
	gEngfuncs.pEventAPI->EV_SetSolidPlayers(idx - 1);
	gEngfuncs.pEventAPI->EV_SetTraceHull(2);
	pmtrace_t tr;
	gEngfuncs.pEventAPI->EV_PlayerTrace((float*)src, end, PM_NORMAL, -1, &tr);

	if (tr.fraction < 1.0f)
	{
		// Classify the hit surface. Brush (world/breakable) -> texture material char; anything else
		// (player/monster bbox) -> flesh.
		physent_t* pe = gEngfuncs.pEventAPI->EV_GetPhysent(tr.ent);
		char chType = CHAR_TEX_CONCRETE;
		bool isBrush = pe && (pe->solid == SOLID_BSP || pe->movetype == MOVETYPE_PUSH || tr.ent == 0);
		if (isBrush)
		{
			const char* texName = gEngfuncs.pEventAPI->EV_TraceTexture(tr.ent, (float*)src, end);
			if (texName && texName[0])
			{
				char buf[64];
				strncpy(buf, texName, sizeof(buf) - 1);
				buf[sizeof(buf) - 1] = 0;
				chType = PM_FindTextureType(buf);
			}
		}
		else
		{
			chType = CHAR_TEX_FLESH; // player/monster — server already sprays blood
		}

		// Off the surface a touch so the sprites aren't clipped into the wall.
		Vector pos;
		for (int i = 0; i < 3; i++)
			pos[i] = tr.endpos[i] + tr.plane.normal[i] * 2.0f;

		TS_ImpactEffect(pos, tr.plane.normal, 0, chType);
	}

	gEngfuncs.pEventAPI->EV_PopPMStates();
}

// events/knife.sc → EV_TSMelee — the original FUN_10043c90. A knife/katana slash hitting a HARD
// surface throws the per-surface impact effect (scintille glint + sparks on metal/glass, debris on
// concrete/wood). The server only plays this on non-flesh hits; the impact point rides in
// args->origin and the player aim in args->angles. We trace a short ray through the impact point to
// recover the surface material, then run the byte-ported dispatcher (cont.69x; was R_SparkShower).
void EV_TSMelee(event_args_t* args)
{
	Vector fwd, r, u;
	AngleVectors(args->angles, fwd, r, u);

	Vector start, stop;
	for (int i = 0; i < 3; i++)
	{
		start[i] = args->origin[i] - fwd[i] * 16.0f;
		stop[i] = args->origin[i] + fwd[i] * 16.0f;
	}

	gEngfuncs.pEventAPI->EV_SetUpPlayerPrediction(0, 1);
	gEngfuncs.pEventAPI->EV_PushPMStates();
	gEngfuncs.pEventAPI->EV_SetSolidPlayers(args->entindex - 1);
	gEngfuncs.pEventAPI->EV_SetTraceHull(2);
	pmtrace_t tr;
	gEngfuncs.pEventAPI->EV_PlayerTrace(start, stop, PM_NORMAL, -1, &tr);

	char chType = CHAR_TEX_METAL; // melee on a hard surface defaults to a metallic scrape
	Vector pos, normal(-fwd.x, -fwd.y, -fwd.z);
	VectorCopy(args->origin, pos);
	if (tr.fraction < 1.0f)
	{
		VectorCopy(tr.plane.normal, normal);
		for (int i = 0; i < 3; i++)
			pos[i] = tr.endpos[i] + tr.plane.normal[i] * 2.0f;
		physent_t* pe = gEngfuncs.pEventAPI->EV_GetPhysent(tr.ent);
		bool isBrush = pe && (pe->solid == SOLID_BSP || pe->movetype == MOVETYPE_PUSH || tr.ent == 0);
		if (isBrush)
		{
			const char* texName = gEngfuncs.pEventAPI->EV_TraceTexture(tr.ent, start, stop);
			if (texName && texName[0])
			{
				char buf[64];
				strncpy(buf, texName, sizeof(buf) - 1);
				buf[sizeof(buf) - 1] = 0;
				chType = PM_FindTextureType(buf);
			}
		}
	}

	gEngfuncs.pEventAPI->EV_PopPMStates();
	TS_ImpactEffect(pos, normal, 0, chType);
}

void EV_TSFire(event_args_t* args)
{
	const int idx = args->entindex;
	const int weaponType = args->iparam1;
	const int barrelSide = args->iparam2; // akimbo: 0 = right barrel (attachment 0), 1 = left (1)
	if (weaponType < 0 || weaponType >= TSW_COUNT)
		return;

	g_tsActive.UpdateFromEvent(args); // refresh the active-weapon cache (matches the original fire handler)

	Vector origin;
	VectorCopy(args->origin, origin);

	EV_TSMuzzleFlare(idx, weaponType, barrelSide); // the TS muzzle-flash sprite at the barrel

	if (EV_IsLocal(idx))
	{
		EV_MuzzleFlash(); // the dynamic light / model flash on top of the sprite
		// NOTE: the viewmodel "shoot" anim is now CLIENT-PREDICTED in CTSGun::StandardFire's
		// SendWeaponAnim (CTSGun is a UseDecrement weapon, cont.69s 3d) — this event handles the
		// muzzle-flash sprite + sound + shell + recoil; the anim plays via the predicted ItemPostFrame.
		// TS AIM-recoil (RE client.dll FUN_10008230): kick the actual aim up + per-firetype horizontal,
		// eased into cl.viewangles in V_CalcRefdef (TS_UpdateViewRecoil). Persistent — the player pulls
		// down. Replaces the old -2.0 punchangle placeholder (the original gates server punchangle to
		// bots; the real recoil moves the aim client-side). bullet-time: VERIFY (no client slow-mo flag
		// wired yet -> false).
		const s_weapon_info* wiLocal = get_weapon_info((tsweapon_t)weaponType);
		TS_AddViewRecoil(wiLocal->flViewKick, wiLocal->iFireType, args->ducking != 0, false);
	}

	// Eject a brass shell from the viewmodel/player.
	Vector angles, velocity, fwd, right, up, shellVel, shellOrigin;
	VectorCopy(args->angles, angles);
	VectorCopy(args->velocity, velocity);
	AngleVectors(angles, fwd, right, up);
	const int shell = gEngfuncs.pEventAPI->EV_FindModelIndex("models/shell.mdl");
	EV_GetDefaultShellInfo(args, origin, velocity, shellVel, shellOrigin, fwd, right, up, 20, -12, 4);
	EV_EjectBrass(shellOrigin, shellVel, angles[1], shell, TE_BOUNCE_SHELL);

	// Bullet impact puff (smoke on surfaces, sparks on metal/glass) at where the shot lands. Melee/
	// grenade fire-types don't fire bullets, so skip them; the rest trace a centre ray (the server
	// does the per-pellet damage/decal/sound — this is the missing visual feedback).
	const s_weapon_info* wi = get_weapon_info((tsweapon_t)weaponType);
	if (wi->iFireType != TSFT_GRENADE && wi->iFireType != TSFT_KNIFE && wi->iFireType != TSFT_KATANA)
	{
		Vector vecSrc;
		EV_GetGunPosition(args, vecSrc, origin);
		EV_TSBulletImpact(idx, vecSrc, fwd);
	}

	const char* snd = s_fireSound[weaponType];
	if (snd)
		gEngfuncs.pEventAPI->EV_PlaySound(idx, origin, CHAN_WEAPON, snd, VOL_NORM, ATTN_NORM, 0, PITCH_NORM);
}
