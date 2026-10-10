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
#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "monsters.h"
#include "weapons.h"
#include "player.h"

#include "usercmd.h"
#include "entity_state.h"
#include "demo_api.h"
#include "pm_defs.h"
#include "event_api.h"
#include "r_efx.h"

#include "cl_dll.h"
#include "../com_weapons.h"
#include "../demo.h"

#include "ts_gun.h" // CTSGun — predicted client-side so its viewmodel anims stick (step 3b)

// Pool of client side entities/entvars_t
static entvars_t ev[MAX_WEAPONS + 1];
static int num_ents = 0;

// The entity we'll use to represent the local client
static CBasePlayer player;

// Local version of game .dll global variables ( time, etc. )
static globalvars_t Globals;

static CBasePlayerWeapon* g_pWpns[MAX_WEAPONS];

float g_flApplyVel = 0.0;
// The laser dot's "settled" test (client.dll 0x1018a628) and the predicted spread the crosshair widens
// with (0x1018d188), both set by the weapon prediction below.
int g_iTSLaserSettled = 0;
float g_flTSSpreadAdd = 0;
bool g_irunninggausspred = false;

Vector previousorigin;

// HLDM Weapon placeholder entities.
CTSGun g_TSGun; // the client-predicted TS arsenal weapon (weapon_tsgun)

// Advance the client-predicted active weapon type on deploy (the draw.sc event, EV_TSDraw) so the
// predicted viewmodel body (view.cpp -> TS_PredictedViewmodelBody) is right on the deploy frame,
// not a vuser3.x reconcile late (the single->akimbo body flicker). cont.89 akimbo dossier §3.
void TS_SetPredictedWeaponType(int type)
{
	if (type < 0 || type >= TSW_COUNT)
		return;
	g_TSGun.m_weaponType = type;
	g_TSGun.m_pActiveInfo = get_weapon_info((tsweapon_t)type); // inline (fold-plan G5)
	g_TSGun.m_pActiveItem = (type > TSW_MELEE && type < TSW_COUNT) ? &g_TSGun.m_items[type] : nullptr;
}

// The local viewmodel's BODY, from the PREDICTED weapon state — the original refreshes it
// EVERY FRAME (dossier-attachments §5, DAT_10191d5c): the deploy/zoom-return anims otherwise
// run with a stale body until the next anim tick (the ~1 s attachment "blink", round 7j).
int TS_PredictedViewmodelBody()
{
	const int t = g_TSGun.m_weaponType;
	if (t < 0 || t >= TSW_COUNT)
		return 0;
	if (t == TSW_M61)
		return 1; // the m61 viewmodel hardcodes body 1 (dossier §4, grenade.sc handler)
	return GetBodyByItems(t, g_TSGun.m_items[t].attach);
}

// The event handlers' hold-stance probe (the original FUN_1003df60 reads its own client cache;
// ours is the predicted weapon's synced m_bHoldPosition).
bool TS_LocalHoldPosition()
{
	return g_TSGun.m_bHoldPosition;
}
// NB: the stock HL weapon prediction placeholders (g_Glock/g_Crowbar/g_Python/g_Mp5/g_Crossbow/
// g_Shotgun/g_Rpg/g_Gauss/g_Egon/g_HGun/g_HandGren/g_Satchel/g_Tripmine/g_Snark) were removed —
// the original never defines these concrete stock weapon classes (all absent from the binaries);
// TS predicts only CTSGun (g_TSGun above). Their <weapon>.cpp files are dropped from the build.


/*
======================
AlertMessage

Print debug messages to console
======================
*/
void AlertMessage(ALERT_TYPE atype, const char* szFmt, ...)
{
	va_list argptr;
	static char string[1024];

	va_start(argptr, szFmt);
	vsprintf(string, szFmt, argptr);
	va_end(argptr);

	gEngfuncs.Con_Printf("cl:  ");
	gEngfuncs.Con_Printf(string);
}

//Returns if it's multiplayer.
//Mostly used by the client side weapons.
bool bIsMultiplayer()
{
	return gEngfuncs.GetMaxClients() != 1;
}
//Just loads a v_ model.
void LoadVModel(const char* szViewModel, CBasePlayer* m_pPlayer)
{
	gEngfuncs.CL_LoadModel(szViewModel, &m_pPlayer->pev->viewmodel);
}

/*
=====================
HUD_PrepEntity

Links the raw entity to an entvars_s holder.  If a player is passed in as the owner, then
we set up the m_pPlayer field.
=====================
*/
void HUD_PrepEntity(CBaseEntity* pEntity, CBasePlayer* pWeaponOwner)
{
	memset(&ev[num_ents], 0, sizeof(entvars_t));
	pEntity->pev = &ev[num_ents++];

	pEntity->Precache();
	pEntity->Spawn();

	if (pWeaponOwner)
	{
		ItemInfo info;

		memset(&info, 0, sizeof(info));

		((CBasePlayerWeapon*)pEntity)->m_pPlayer = pWeaponOwner;

		((CBasePlayerWeapon*)pEntity)->GetItemInfo(&info);

		CBasePlayerItem::ItemInfoArray[info.iId] = info;

		const char* weaponName = ((info.iFlags & ITEM_FLAG_EXHAUSTIBLE) != 0) ? STRING(pEntity->pev->classname) : nullptr;

		if (info.pszAmmo1 && '\0' != *info.pszAmmo1)
		{
			AddAmmoNameToAmmoRegistry(info.pszAmmo1, weaponName);
		}

		if (info.pszAmmo2 && '\0' != *info.pszAmmo2)
		{
			AddAmmoNameToAmmoRegistry(info.pszAmmo2, weaponName);
		}

		g_pWpns[info.iId] = (CBasePlayerWeapon*)pEntity;
	}
}

/*
=====================
CBaseEntity:: Killed

If weapons code "kills" an entity, just set its effects to EF_NODRAW
=====================
*/
void CBaseEntity::Killed(entvars_t* pevAttacker, int iGib)
{
	pev->effects |= EF_NODRAW;
}

/*
=====================
CBasePlayerWeapon:: DefaultDeploy

=====================
*/
bool CBasePlayerWeapon::DefaultDeploy(const char* szViewModel, const char* szWeaponModel, int iAnim, const char* szAnimExt, int body)
{
	if (!CanDeploy())
		return false;

	gEngfuncs.CL_LoadModel(szViewModel, &m_pPlayer->pev->viewmodel);

	SendWeaponAnim(iAnim, body);

	g_irunninggausspred = false;
	m_pPlayer->m_flNextAttack = 0.5;
	m_flTimeWeaponIdle = 1.0;
	return true;
}

/*
=====================
CBasePlayerWeapon:: PlayEmptySound

=====================
*/
bool CBasePlayerWeapon::PlayEmptySound()
{
	if (m_iPlayEmptySound)
	{
		HUD_PlaySound("weapons/357_cock1.wav", 0.8);
		m_iPlayEmptySound = false;
		return false;
	}
	return false;
}

/*
=====================
CBasePlayerWeapon::Holster

Put away weapon
=====================
*/
void CBasePlayerWeapon::Holster()
{
	m_fInReload = false; // cancel any reload in progress.
	g_irunninggausspred = false;
	m_pPlayer->pev->viewmodel = 0;
}

/*
=====================
CBasePlayerWeapon::SendWeaponAnim

Animate weapon model
=====================
*/
void CBasePlayerWeapon::SendWeaponAnim(int iAnim, int body)
{
	m_pPlayer->pev->weaponanim = iAnim;

	HUD_SendWeaponAnim(iAnim, body, false);
}

/*
=====================
CBaseEntity::FireBulletsPlayer

Only produces random numbers to match the server ones.
=====================
*/
Vector CBaseEntity::FireBulletsPlayer(unsigned int cShots, Vector vecSrc, Vector vecDirShooting, Vector vecSpread, float flDistance, int iBulletType, int iTracerFreq, int iDamage, entvars_t* pevAttacker, int shared_rand)
{
	float x = 0, y = 0, z;

	for (unsigned int iShot = 1; iShot <= cShots; iShot++)
	{
		if (pevAttacker == NULL)
		{
			// get circular gaussian spread
			do
			{
				x = RANDOM_FLOAT(-0.5, 0.5) + RANDOM_FLOAT(-0.5, 0.5);
				y = RANDOM_FLOAT(-0.5, 0.5) + RANDOM_FLOAT(-0.5, 0.5);
				z = x * x + y * y;
			} while (z > 1);
		}
		else
		{
			//Use player's random seed.
			// get circular gaussian spread
			x = UTIL_SharedRandomFloat(shared_rand + iShot, -0.5, 0.5) + UTIL_SharedRandomFloat(shared_rand + (1 + iShot), -0.5, 0.5);
			y = UTIL_SharedRandomFloat(shared_rand + (2 + iShot), -0.5, 0.5) + UTIL_SharedRandomFloat(shared_rand + (3 + iShot), -0.5, 0.5);
			z = x * x + y * y;
		}
	}

	return Vector(x * vecSpread.x, y * vecSpread.y, 0.0);
}

/*
=====================
CBasePlayer::SelectItem

  Switch weapons
=====================
*/
void CBasePlayer::SelectItem(const char* pstr)
{
	if (!pstr)
		return;

	CBasePlayerItem* pItem = NULL;

	if (!pItem)
		return;


	if (pItem == m_pActiveItem)
		return;

	if (m_pActiveItem)
		m_pActiveItem->Holster();

	m_pLastItem = m_pActiveItem;
	m_pActiveItem = pItem;

	if (m_pActiveItem)
	{
		m_pActiveItem->Deploy();
	}
}

/*
=====================
CBasePlayer::Killed

=====================
*/
void CBasePlayer::Killed(entvars_t* pevAttacker, int iGib)
{
	// Holster weapon immediately, to allow it to cleanup
	if (m_pActiveItem)
		m_pActiveItem->Holster();

	g_irunninggausspred = false;
}

// Client stubs for the TS slow-motion virtuals (server runs the real logic; prediction doesn't).
void CBasePlayer::GoSlow(float scale, int slowBullets) {}
int CBasePlayer::InSlow() { return 0; }

/*
=====================
CBasePlayer::Spawn

=====================
*/
void CBasePlayer::Spawn()
{
	if (m_pActiveItem)
		m_pActiveItem->Deploy();

	g_irunninggausspred = false;
}

/*
=====================
UTIL_TraceLine

Don't actually trace, but act like the trace didn't hit anything.
=====================
*/
void UTIL_TraceLine(const Vector& vecStart, const Vector& vecEnd, IGNORE_MONSTERS igmon, edict_t* pentIgnore, TraceResult* ptr)
{
	memset(ptr, 0, sizeof(*ptr));
	ptr->flFraction = 1.0;
}

/*
=====================
UTIL_ParticleBox

For debugging, draw a box around a player made out of particles
=====================
*/
void UTIL_ParticleBox(CBasePlayer* player, float* mins, float* maxs, float life, unsigned char r, unsigned char g, unsigned char b)
{
	int i;
	Vector mmin, mmax;

	for (i = 0; i < 3; i++)
	{
		mmin[i] = player->pev->origin[i] + mins[i];
		mmax[i] = player->pev->origin[i] + maxs[i];
	}

	gEngfuncs.pEfxAPI->R_ParticleBox((float*)&mmin, (float*)&mmax, 5.0, 0, 255, 0);
}

/*
=====================
UTIL_ParticleBoxes

For debugging, draw boxes for other collidable players
=====================
*/
void UTIL_ParticleBoxes()
{
	int idx;
	physent_t* pe;
	cl_entity_t* player;
	Vector mins, maxs;

	gEngfuncs.pEventAPI->EV_SetUpPlayerPrediction(0, 1);

	// Store off the old count
	gEngfuncs.pEventAPI->EV_PushPMStates();

	player = gEngfuncs.GetLocalPlayer();
	// Now add in all of the players.
	gEngfuncs.pEventAPI->EV_SetSolidPlayers(player->index - 1);

	for (idx = 1; idx < 100; idx++)
	{
		pe = gEngfuncs.pEventAPI->EV_GetPhysent(idx);
		if (!pe)
			break;

		if (pe->info >= 1 && pe->info <= gEngfuncs.GetMaxClients())
		{
			mins = pe->origin + pe->mins;
			maxs = pe->origin + pe->maxs;

			gEngfuncs.pEfxAPI->R_ParticleBox((float*)&mins, (float*)&maxs, 0, 0, 255, 2.0);
		}
	}

	gEngfuncs.pEventAPI->EV_PopPMStates();
}

/*
=====================
UTIL_ParticleLine

For debugging, draw a line made out of particles
=====================
*/
void UTIL_ParticleLine(CBasePlayer* player, float* start, float* end, float life, unsigned char r, unsigned char g, unsigned char b)
{
	gEngfuncs.pEfxAPI->R_ParticleLine(start, end, r, g, b, life);
}

/*
=====================
HUD_InitClientWeapons

Set up weapons, player and functions needed to run weapons code client-side.
=====================
*/
void HUD_InitClientWeapons()
{
	static bool initialized = false;
	if (initialized)
		return;

	initialized = true;

	// Set up pointer ( dummy object )
	gpGlobals = &Globals;

	// Fill in current time ( probably not needed )
	gpGlobals->time = gEngfuncs.GetClientTime();

	// On 64-bit, MAKE_STRING() in shared weapon code (e.g. CTSGun::Spawn) is an
	// engine AllocString call; give it a local string pool so it doesn't deref a
	// NULL pointer, and point pStringBase at the same pool so STRING() reads back.
	gpGlobals->pStringBase = stub_StringBase();

	// Fake functions
	g_engfuncs.pfnAllocString = stub_AllocString;
	g_engfuncs.pfnPrecacheModel = stub_PrecacheModel;
	g_engfuncs.pfnPrecacheSound = stub_PrecacheSound;
	g_engfuncs.pfnPrecacheEvent = stub_PrecacheEvent;
	g_engfuncs.pfnNameForFunction = stub_NameForFunction;
	g_engfuncs.pfnSetModel = stub_SetModel;
	g_engfuncs.pfnSetClientMaxspeed = HUD_SetMaxSpeed;

	// Handled locally
	g_engfuncs.pfnPlaybackEvent = HUD_PlaybackEvent;
	g_engfuncs.pfnAlertMessage = AlertMessage;

	// Pass through to engine
	g_engfuncs.pfnPrecacheEvent = gEngfuncs.pfnPrecacheEvent;
	g_engfuncs.pfnRandomFloat = gEngfuncs.pfnRandomFloat;
	g_engfuncs.pfnRandomLong = gEngfuncs.pfnRandomLong;
	g_engfuncs.pfnCVarGetPointer = gEngfuncs.pfnGetCvarPointer;
	g_engfuncs.pfnCVarGetString = gEngfuncs.pfnGetCvarString;
	g_engfuncs.pfnCVarGetFloat = gEngfuncs.pfnGetCvarFloat;

	// Allocate a slot for the local player
	HUD_PrepEntity(&player, NULL);

	// Allocate slot(s) for each weapon that we are going to be predicting.
	// TS predicts only its own arsenal weapon (the stock HL weapon placeholders were removed).
	HUD_PrepEntity(&g_TSGun, &player); // TS: predict CTSGun so its viewmodel anims stick
}

/*
=====================
HUD_GetLastOrg

Retruns the last position that we stored for egon beam endpoint.
=====================
*/
void HUD_GetLastOrg(float* org)
{
	int i;

	// Return last origin
	for (i = 0; i < 3; i++)
	{
		org[i] = previousorigin[i];
	}
}

/*
=====================
HUD_SetLastOrg

Remember our exact predicted origin so we can draw the egon to the right position.
=====================
*/
void HUD_SetLastOrg()
{
	int i;

	// Offset final origin by view_offset
	for (i = 0; i < 3; i++)
	{
		previousorigin[i] = g_finalstate->playerstate.origin[i] + g_finalstate->client.view_ofs[i];
	}
}

/*
=====================
HUD_WeaponsPostThink

Run Weapon firing code on client
=====================
*/
void HUD_WeaponsPostThink(local_state_s* from, local_state_s* to, usercmd_t* cmd, double time, unsigned int random_seed)
{
	int i;
	int buttonsChanged;
	CBasePlayerWeapon* pWeapon = NULL;
	CBasePlayerWeapon* pCurrent;
	weapon_data_t nulldata, *pfrom, *pto;
	static int lasthealth;

	memset(&nulldata, 0, sizeof(nulldata));

	// Get current clock
	//Use actual time instead of prediction frame time because that time value breaks anything that uses absolute time values.
	gpGlobals->time = gEngfuncs.GetClientTime(); //time;

	//Lets weapons code use frametime to decrement timers and stuff.
	gpGlobals->frametime = cmd->msec / 1000.0f;

	// Fill in data based on selected weapon
	// FIXME, make this a method in each weapon?  where you pass in an entity_state_t *?
	switch (from->client.m_iId)
	{
	// NB: the stock HL weapon cases (WEAPON_CROWBAR/GLOCK/PYTHON/MP5/CROSSBOW/SHOTGUN/RPG/GAUSS/
	// EGON/HORNETGUN/HANDGRENADE/SATCHEL/TRIPMINE/SNARK → their g_* placeholders) were removed —
	// those concrete stock weapon classes don't exist in the original; TS predicts only CTSGun.
	case TSGUN_WEAPON_ID:
		pWeapon = &g_TSGun; // TS: run CTSGun::ItemPostFrame client-side each prediction frame
		break;
	}

	// Store pointer to our destination entity_state_t so we can get our origin, etc. from it
	//  for setting up events on the client
	g_finalstate = to;

	// If we are running events/etc. go ahead and see if we
	//  managed to die between last frame and this one
	// If so, run the appropriate player killed or spawn function
	if (g_runfuncs)
	{
		if (to->client.health <= 0 && lasthealth > 0)
		{
			player.Killed(NULL, 0);
		}
		else if (to->client.health > 0 && lasthealth <= 0)
		{
			player.Spawn();
		}

		lasthealth = to->client.health;
	}

	// We are not predicting the current weapon, just bow out here.
	if (!pWeapon)
		return;

	for (i = 0; i < MAX_WEAPONS; i++)
	{
		pCurrent = g_pWpns[i];
		if (!pCurrent)
		{
			continue;
		}

		pfrom = &from->weapondata[i];

		pCurrent->m_fInReload = 0 != pfrom->m_fInReload;
		pCurrent->m_fInSpecialReload = pfrom->m_fInSpecialReload;
		//		pCurrent->m_flPumpTime			= pfrom->m_flPumpTime;
		pCurrent->m_iClip = pfrom->m_iClip;
		pCurrent->m_flNextPrimaryAttack = pfrom->m_flNextPrimaryAttack;
		pCurrent->m_flNextSecondaryAttack = pfrom->m_flNextSecondaryAttack;
		pCurrent->m_flTimeWeaponIdle = pfrom->m_flTimeWeaponIdle;
		pCurrent->pev->fuser1 = pfrom->fuser1;
		pCurrent->m_flStartThrow = pfrom->fuser2;
		pCurrent->m_flReleaseThrow = pfrom->fuser3;
		pCurrent->m_chargeReady = pfrom->iuser1;
		pCurrent->m_fInAttack = pfrom->iuser2;
		pCurrent->m_fireState = pfrom->iuser3;

		// TS weapon-state channel (cont.89): the active weapon's predicted state comes from the clientdata
		// vuser block the server sends (this REPLACES the removed invented CTSGun::SetWeaponData/weapon_data
		// override, which corrupted the stock fire-state fields for the ORIGINAL client). Same slot layout the
		// original client reads: vuser3.x=type, vuser3.y=fireMode, vuser3.z=attach, vuser4.x=altMode,
		// vuser4.y=clip, vuser4.z=2nd/left clip, vuser2.x=bloody, vuser1.z=active-attach.
		if (pCurrent == &g_TSGun)
		{
			const int wt = (int)from->client.vuser3[0];
			g_iTSLaserSettled = 0; // 0x10003a04: 0 with kung fu (type 0)
			g_TSGun.m_weaponType = wt;
			g_TSGun.m_fireMode = (int)from->client.vuser3[1];
			g_TSGun.m_pActiveInfo = get_weapon_info((tsweapon_t)wt); // inline (fold-plan G5)
			g_TSGun.m_pActiveItem = (wt > TSW_MELEE && wt < TSW_COUNT) ? &g_TSGun.m_items[wt] : nullptr;
			if (wt >= 0 && wt < TSW_COUNT)
			{
				const s_weapon_info* awi = get_weapon_info((tsweapon_t)wt);
				// cont.89 reload: seed the active caliber's RESERVE so the client can PREDICT Reload()
				// (its refill reads m_ammoReserve[caliber]; without this it stays 0 → reload no-ops).
				const int cal = awi->iCaliber;
				if (cal > 0 && cal < TS_NUM_CALIBERS)
					g_TSGun.m_ammoReserve[cal] = (int)from->client.vuser2[1];
				TSItem& it = g_TSGun.m_items[wt];
				// cont.90: vuser2.x = LIVE item+0x8 (reload/burst PHASE for guns, bloody for blades); vuser4.x =
				// LIVE item+0x24 (pump RACK toggle for pump guns, akimbo barrel otherwise). Inverse of the server
				// pack in dlls/client.cpp — re-seed the field our prediction reads so ShotgunFire alternates and
				// ShotgunReload runs shell-by-shell against the ORIGINAL server too. [[dropin-both-directions]]
				const bool awBlade = (awi->iFireType == TSFT_KNIFE || awi->iFireType == TSFT_KATANA);
				const bool awPump  = (awi->iFireType == TSFT_PUMP);
				if (awBlade)
				{
					it.bloody = (int)from->client.vuser2[0];
					// cont.96: seed the PREDICTED weapon entvars' body too — the bloody blade is a
					// viewmodel SUBMODEL (body 1) and the predicted WeaponIdle sends
					// SendWeaponAnim(idle, pev->body). The server-side bloody writes (KatanaFire/
					// KnifeFire, ts_gun.cpp) are #ifndef CLIENT_DLL, so without this the predicted
					// idle re-sent body 0 ~3s after a flesh hit and the blade BLINKED CLEAN.
					if (g_TSGun.pev)
						g_TSGun.pev->body = it.bloody;
				}
				else
					g_TSGun.m_burstOrPhase = (int)from->client.vuser2[0];
				it.mode = (int)from->client.vuser3[1];
				it.attach = (int)from->client.vuser3[2];
				if (awPump) g_TSGun.m_fInSpecialReload = (int)from->client.vuser4[0];
				else        it.altMode = (int)from->client.vuser4[0];
				it.clip = (int)from->client.vuser4[1];
				g_TSGun.m_fInReload = (from->client.vuser4[2] != 0.0f); // cd+0x1d8 = the RELOAD flag (not leftClip)
				it.leftClip = (int)from->client.ammo_nails;            // cd+0x70 = akimbo LEFT barrel clip
				it.active = (int)from->client.vuser1[2];
				// client.dll 0x10003b9c: the laser dot's "settled" test is the active attachments as the
				// server sends them, cleared below while the player may not attack yet.
				g_iTSLaserSettled = wt != 0 ? it.active : 0;
				if (from->client.m_flNextAttack > 0.0f)
					g_iTSLaserSettled = 0;
				it.holdPosition = (from->client.vuser2[2] != 0.0f);   // cd+0x1c0 = hold-position stance
				g_TSGun.m_bHoldPosition = it.holdPosition;            // seed the PREDICTED base (the active-pointer
				                                                      //   reseed above ran off the stale item) so SecondaryAttack
				                                                      //   toggles from the server truth -> no stance blink
				// No owned flag: client.dll fills the wielded item here (@0x10003a4d-0x10003b20) but never
				// marks any item owned (Reset @0x10008470 clears them, nothing sets them), so its predicted
				// DropWeapon (@0x100085d0) is a no-op and the last grenade's throw picks no next weapon
				// locally; the server's vuser3.x brings the next one. Marking every wielded type owned
				// made the predicted GetBetterWeapon pick a weapon dropped long ago and give it a bar cell.
				// cd+0x6c (ammo_shells) = the PAIRED akimbo/single variant's clip (server pack @0xb0294). Store
				// it back into the parked variant's clip so our client knows it for the bar/prediction — the
				// inverse of the server pack, completing drop-in parity vs the ORIGINAL server. [[dropin-both-directions]]
				{
					int paired = -1;
					if (awi->iAkimboType >= 0 && awi->iAkimboType < TSW_COUNT)
						paired = awi->iAkimboType;
					else if (awi->iSingleType >= 0 && awi->iSingleType < TSW_COUNT && awi->iSingleType != wt)
						paired = awi->iSingleType;
					if (paired >= 0)
						g_TSGun.m_items[paired].clip = (int)from->client.ammo_shells;
				}
			}
		}
	}

	// For random weapon events, use this seed to seed random # generator
	player.random_seed = random_seed;

	// Get old buttons from previous state.
	player.m_afButtonLast = from->playerstate.oldbuttons;

	// Which buttsons chave changed
	buttonsChanged = (player.m_afButtonLast ^ cmd->buttons); // These buttons have changed this frame

	// Debounced button codes for pressed/released
	// The changed ones still down are "pressed"
	player.m_afButtonPressed = buttonsChanged & cmd->buttons;
	// The ones not down are "released"
	player.m_afButtonReleased = buttonsChanged & (~cmd->buttons);
	player.pev->v_angle = cmd->viewangles;
	player.pev->origin = from->client.origin;

	// Set player variables that weapons code might check/alter
	player.pev->button = cmd->buttons;

	player.pev->velocity = from->client.velocity;
	player.pev->flags = from->client.flags;

	player.pev->deadflag = from->client.deadflag;
	player.pev->waterlevel = from->client.waterlevel;
	player.pev->maxspeed = from->client.maxspeed;
	player.m_iFOV = from->client.fov;
	player.pev->weaponanim = from->client.weaponanim;
	player.pev->viewmodel = from->client.viewmodel;
	player.m_flNextAttack = from->client.m_flNextAttack;
	player.m_flNextAmmoBurn = from->client.fuser2;
	player.m_flAmmoStartCharge = from->client.fuser3;
	// clientdata.iuser4 carries the stunt flags (pev->iuser4), which the stunt prediction reads. The
	// powerup bits ride vuser1.y: TS 3.0's UpdateClientData (0xa0475) sends (float)m_iPowerups there and
	// client.dll (0x10003949) reads it into the predicted player.
	player.m_iPowerups = (int)from->client.vuser1[1];

	//Stores all our ammo info, so the client side weapons can use them.
	player.ammo_9mm = (int)from->client.vuser1[0];
	player.ammo_argrens = (int)from->client.vuser1[2];
	player.ammo_bolts = (int)from->client.ammo_nails; //is an int anyways...
	player.ammo_buckshot = (int)from->client.ammo_shells;
	player.ammo_uranium = (int)from->client.ammo_cells;
	player.ammo_hornets = (int)from->client.vuser2[0];
	player.ammo_rockets = (int)from->client.ammo_rockets;


	// Point to current weapon object
	if (WEAPON_NONE != from->client.m_iId)
	{
		player.m_pActiveItem = g_pWpns[from->client.m_iId];
	}

	if (player.m_pActiveItem->m_iId == WEAPON_RPG)
	{
		((CRpg*)player.m_pActiveItem)->m_fSpotActive = static_cast<bool>(from->client.vuser2[1]);
		((CRpg*)player.m_pActiveItem)->m_cActiveRockets = (int)from->client.vuser2[2];
	}

	// Don't go firing anything if we have died. client.dll's gate (0x10003d46) is only this:
	// deadflag != DEAD_DISCARDBODY + 1, !CL_IsDead() (0x10026860, the TSHealth byte < 1) and
	// m_flNextAttack <= 0. The stock spectator and viewmodel tests are not in it, so a scope that
	// hides the viewmodel still predicts its shots (round 7k-c's symptom). (Round 7l narrowed an
	// earlier removal of the viewmodel test for a hang that was TS_EvWeaponAnim calling itself,
	// fixed in 7l-b.)
	if ((player.pev->deadflag != (DEAD_DISCARDBODY + 1)) && !CL_IsDead())
	{
		if (player.m_flNextAttack <= 0)
		{
			pWeapon->ItemPostFrame();
		}
	}

	// Assume that we are not going to switch weapons
	to->client.m_iId = from->client.m_iId;

	// Now see if we issued a changeweapon command ( and we're not dead )
	if (0 != cmd->weaponselect && (player.pev->deadflag != (DEAD_DISCARDBODY + 1)))
	{
		// Switched to a different weapon?
		if (from->weapondata[cmd->weaponselect].m_iId == cmd->weaponselect)
		{
			CBasePlayerWeapon* pNew = g_pWpns[cmd->weaponselect];
			if (pNew && (pNew != pWeapon))
			{
				// Put away old weapon
				if (player.m_pActiveItem)
					player.m_pActiveItem->Holster();

				player.m_pLastItem = player.m_pActiveItem;
				player.m_pActiveItem = pNew;

				// Deploy new weapon
				if (player.m_pActiveItem)
				{
					player.m_pActiveItem->Deploy();
				}

				// Update weapon id so we can predict things correctly.
				to->client.m_iId = cmd->weaponselect;
			}
		}
	}

	// Copy in results of prediction code
	to->client.viewmodel = player.pev->viewmodel;
	to->client.fov = player.m_iFOV;
	to->client.weaponanim = player.pev->weaponanim;
	to->client.m_flNextAttack = player.m_flNextAttack;
	to->client.fuser2 = player.m_flNextAmmoBurn;
	to->client.fuser3 = player.m_flAmmoStartCharge;
	to->client.maxspeed = player.pev->maxspeed;

	//HL Weapons
	to->client.vuser1[0] = player.ammo_9mm;
	to->client.vuser1[1] = (float)player.m_iPowerups;
	to->client.vuser1[2] = player.ammo_argrens;

	to->client.ammo_nails = player.ammo_bolts;
	to->client.ammo_shells = player.ammo_buckshot;
	to->client.ammo_cells = player.ammo_uranium;
	to->client.vuser2[0] = player.ammo_hornets;
	to->client.ammo_rockets = player.ammo_rockets;

	if (player.m_pActiveItem->m_iId == WEAPON_RPG)
	{
		to->client.vuser2[1] = static_cast<float>(((CRpg*)player.m_pActiveItem)->m_fSpotActive);
		to->client.vuser2[2] = ((CRpg*)player.m_pActiveItem)->m_cActiveRockets;
	}

	// (cont.89: the TS weapon-state vuser mirror lives in the per-weapon loop below, next to DecrementTimers.)

	// Make sure that weapon animation matches what the game .dll is telling us
	//  over the wire ( fixes some animation glitches )
	// ALSO force a replay when the VIEWMODEL changed (a weapon switch), even if the new deploy anim
	// index equals the outgoing weapon's last anim — otherwise two weapons whose draw sequence share an
	// index (e.g. MP5K & AK-47 are both 2) would not replay the unholster animation on the switch.
	static int s_lastViewmodel = -1;
	const int curViewmodel = from->client.viewmodel;
	const bool vmChanged = (curViewmodel != s_lastViewmodel);
	if (g_runfuncs)
		s_lastViewmodel = curViewmodel;
	if (g_runfuncs && ((HUD_GetWeaponAnim() != to->client.weaponanim) || (vmChanged && curViewmodel != 0)))
	{
		// (stock "make sure the 357 has the right body" g_Python fixup removed — no stock weapons;
		// CTSGun drives its own viewmodel body via TS_PredictedViewmodelBody)
		// Force a fixed anim down to viewmodel
		HUD_SendWeaponAnim(to->client.weaponanim, pWeapon->pev->body, true);
	}

	// TS slow-motion fire-rate prediction (RE scratchpad/SLOWMO_PRESENTATION_RE.md §B): scale the
	// per-frame weapon-timer decrement by the player's current slow factor — pev->fuser1 as the
	// clientdata carries it (UpdateClientData's cd->fuser1; client.dll 0x10003830 copies from+0x2f0,
	// client.fuser1, into the player's pev) — so the predicted fire rate matches the server PostThink.
	// Not the playerstate: entity_state_player_t in delta.lst has no fuser1, so that one is always 0.
	// Floored at 0.01 (FixedSlowAmt, client.dll 0x1000b9b0 -> 0x10055aa0), 0 included, as the server's
	// PostThink, and x1.5 with the double fire rate powerup (0x10003ece).
	float predSlow = from->client.fuser1;
	if (predSlow < 0.01f) predSlow = 0.01f;
	double slowMsec = (cmd->msec * 0.001) * predSlow;
	if ((player.m_iPowerups & 0x10) != 0)
		slowMsec *= 1.5;

	for (i = 0; i < MAX_WEAPONS; i++)
	{
		pCurrent = g_pWpns[i];

		pto = &to->weapondata[i];

		if (!pCurrent)
		{
			memset(pto, 0, sizeof(weapon_data_t));
			continue;
		}

		pto->m_fInReload = static_cast<int>(pCurrent->m_fInReload);
		pto->m_fInSpecialReload = pCurrent->m_fInSpecialReload;
		//		pto->m_flPumpTime				= pCurrent->m_flPumpTime;
		pto->m_iClip = pCurrent->m_iClip;
		pto->m_flNextPrimaryAttack = pCurrent->m_flNextPrimaryAttack;
		pto->m_flNextSecondaryAttack = pCurrent->m_flNextSecondaryAttack;
		pto->m_flTimeWeaponIdle = pCurrent->m_flTimeWeaponIdle;
		pto->fuser1 = pCurrent->pev->fuser1;
		pto->fuser2 = pCurrent->m_flStartThrow;
		pto->fuser3 = pCurrent->m_flReleaseThrow;
		pto->iuser1 = pCurrent->m_chargeReady;
		pto->iuser2 = pCurrent->m_fInAttack;
		pto->iuser3 = pCurrent->m_fireState;

		// Decrement weapon counters, server does this at same time ( during post think, after doing everything else )
		pto->m_flNextReload -= cmd->msec / 1000.0;
		pto->m_fNextAimBonus -= cmd->msec / 1000.0;
		pto->m_flNextPrimaryAttack -= slowMsec;   // TS: slow-scaled (fire rate dilates in slow-motion)
		pto->m_flNextSecondaryAttack -= slowMsec;
		pto->m_flTimeWeaponIdle -= slowMsec;
		pto->fuser1 -= slowMsec;

		// TS weapon-state channel (cont.89): mirror the predicted active-weapon state into the vuser block
		// (the inverse of the from-read above) so OUR client's prediction reconciliation matches the server's
		// clientdata layout. Same slots the original client reads.
		if (pCurrent == &g_TSGun)
		{
			const int wt = g_TSGun.m_weaponType;
			to->client.vuser3[0] = (float)wt;
			to->client.vuser3[1] = (float)g_TSGun.m_fireMode; // LIVE fire-mode (matches server)
			to->client.vuser4[1] = (float)g_TSGun.m_iClip;    // LIVE clip (matches server)
			if (wt >= 0 && wt < TSW_COUNT)
			{
				const s_weapon_info* awi = get_weapon_info((tsweapon_t)wt);
				const int cal = awi->iCaliber; // reload reserve (matches server)
				if (cal > 0 && cal < TS_NUM_CALIBERS)
					to->client.vuser2[1] = (float)g_TSGun.m_ammoReserve[cal];
				const TSItem& it = g_TSGun.m_items[wt];
				// cont.90: LIVE item+0x8 (phase for guns / bloody for blades) on vuser2.x, LIVE item+0x24
				// (pump RACK toggle / akimbo barrel) on vuser4.x — matches the server pack in dlls/client.cpp.
				const bool awBlade = (awi->iFireType == TSFT_KNIFE || awi->iFireType == TSFT_KATANA);
				const bool awPump  = (awi->iFireType == TSFT_PUMP);
				to->client.vuser2[0] = (float)(awBlade ? it.bloody : g_TSGun.m_burstOrPhase);
				to->client.vuser3[2] = (float)it.attach;
				to->client.vuser4[0] = (float)(awPump ? g_TSGun.m_fInSpecialReload : it.altMode);
				to->client.vuser4[2] = (float)(g_TSGun.m_fInReload ? 1 : 0); // RELOAD flag (cd+0x1d8)
				to->client.ammo_nails = (float)it.leftClip;                  // akimbo LEFT barrel (cd+0x70)
				to->client.vuser1[2] = (float)it.active;
			}
		}

		pCurrent->DecrementTimers();

		// TS: the alternate-attack gate decrements like the other predicted timers
		if (pCurrent == &g_TSGun)
			g_TSGun.m_flNextAlternateAttack = V_max((float)(g_TSGun.m_flNextAlternateAttack - slowMsec), -0.001f);

		// (cont.89: the invented pCurrent->GetWeaponData(*pto) is removed — TS predicted weapon state now
		// rides the clientdata vuser block written above, matching the original client's layout.)

		/*		if ( pto->m_flPumpTime != -9999 )
		{
			pto->m_flPumpTime -= cmd->msec / 1000.0;
			if ( pto->m_flPumpTime < -0.001 )
				pto->m_flPumpTime = -0.001;
		}*/

		if (pto->m_fNextAimBonus < -1.0)
		{
			pto->m_fNextAimBonus = -1.0;
		}

		if (pto->m_flNextPrimaryAttack < -1.0)
		{
			pto->m_flNextPrimaryAttack = -1.0;
		}

		if (pto->m_flNextSecondaryAttack < -0.001)
		{
			pto->m_flNextSecondaryAttack = -0.001;
		}

		if (pto->m_flTimeWeaponIdle < -0.001)
		{
			pto->m_flTimeWeaponIdle = -0.001;
		}

		if (pto->m_flNextReload < -0.001)
		{
			pto->m_flNextReload = -0.001;
		}

		if (pto->fuser1 < -0.001)
		{
			pto->fuser1 = -0.001;
		}
	}

	// m_flNextAttack is now part of the weapons, but is part of the player instead
	to->client.m_flNextAttack -= slowMsec; // TS: slow-scaled (matches the server PostThink)
	if (to->client.m_flNextAttack < -0.001)
	{
		to->client.m_flNextAttack = -0.001;
	}

	to->client.fuser2 -= cmd->msec / 1000.0;
	if (to->client.fuser2 < -0.001)
	{
		to->client.fuser2 = -0.001;
	}

	to->client.fuser3 -= cmd->msec / 1000.0;
	if (to->client.fuser3 < -0.001)
	{
		to->client.fuser3 = -0.001;
	}

	// client.dll 0x10004083: on a first prediction the spread accumulator recovers by a tenth of the
	// weapon frame time, to 0, and the crosshair reads it.
	if (g_runfuncs)
	{
		if (player.m_flSpreadAccum > 0.0f)
		{
			player.m_flSpreadAccum -= (float)slowMsec * 0.1f;
			if (player.m_flSpreadAccum < 0.0f)
				player.m_flSpreadAccum = 0.0f;
		}
		g_flTSSpreadAdd = player.m_flSpreadAccum;
	}

	// client.dll @0x10003f6c: on a first prediction with the arsenal out, the predicted gun hands its
	// clip, fire mode and attachments to the HUD (CTSGun::UpdateClientData, client side).
	if (g_runfuncs && to->client.m_iId == TSGUN_WEAPON_ID)
		g_TSGun.UpdateClientData(&player);

	// Store off the last position from the predicted state.
	HUD_SetLastOrg();

	// Wipe it so we can't use it after this frame
	g_finalstate = NULL;
}

/*
=====================
HUD_PostRunCmd

Client calls this during prediction, after it has moved the player and updated any info changed into to->
time is the current client clock based on prediction
cmd is the command that caused the movement, etc
runfuncs is 1 if this is the first time we've predicted this command.  If so, sounds and effects should play, otherwise, they should
be ignored
=====================
*/
void DLLEXPORT HUD_PostRunCmd(struct local_state_s* from, struct local_state_s* to, struct usercmd_s* cmd, int runfuncs, double time, unsigned int random_seed)
{
	//	RecClPostRunCmd(from, to, cmd, runfuncs, time, random_seed);

	g_runfuncs = runfuncs != 0;

	//Event code depends on this stuff, so always initialize it.
	HUD_InitClientWeapons();

#if defined(CLIENT_WEAPONS)
	if (cl_lw && 0 != cl_lw->value)
	{
		HUD_WeaponsPostThink(from, to, cmd, time, random_seed);
	}
	else
#endif
	{
		to->client.fov = g_lastFOV;
	}

	if (g_irunninggausspred)
	{
		Vector forward;
		gEngfuncs.pfnAngleVectors(v_angles, forward, NULL, NULL);
		to->client.velocity = to->client.velocity - forward * g_flApplyVel * 5;
		g_irunninggausspred = false;
	}

	// All games can use FOV state
	g_lastFOV = to->client.fov;
	g_CurrentWeaponId = to->client.m_iId;
}
