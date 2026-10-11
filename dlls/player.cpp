/***
*
*	Copyright (c) 1996-2001, Valve LLC. All rights reserved.
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
/*

===== player.cpp ========================================================

  functions dealing with the player

*/

#include <limits>
#include <algorithm>

#include "extdll.h"
#include "util.h"

#include "cbase.h"
#include "player.h"
#include "trains.h"
#include "nodes.h"
#include "weapons.h"
#include "soundent.h"
#include "monsters.h"
#include "shake.h"
#include "decals.h"
#include "gamerules.h"
#include "game.h"
#include "pm_shared.h"
#include "hltv.h"
#include "UserMessages.h"
#include "client.h"
#include "ts_utils.h" // TSHudMessageOne (the `respawn`-gate "wait" message)
#include "ts_gamerules.h" // TSGameRules — armor_absorb (kevlar damage reduction)
#include "ts_weapons.h" // TS weapon helpers
#include "ts_gun.h" // CTSGun::Reset (StartObserver arsenal wipe), GetPlayerTSGun
#include "ts_shared.h" // InterpolateSlowMotion (the fuser1 slow-motion ease)
#include "ts_player.h" // glast_headshot (TS TraceAttack hitgroup multiplier)

// #define DUCKFIX

extern void CopyToBodyQue(entvars_t* pev);
extern edict_t* EntSelectSpawnPoint(CBaseEntity* pPlayer);

#define TRAIN_ACTIVE 0x80
#define TRAIN_NEW 0xc0
#define TRAIN_OFF 0x00
#define TRAIN_NEUTRAL 0x01
#define TRAIN_SLOW 0x02
#define TRAIN_MEDIUM 0x03
#define TRAIN_FAST 0x04
#define TRAIN_BACK 0x05


// Global Savedata for player
TYPEDESCRIPTION CBasePlayer::m_playerSaveData[] =
	{
		DEFINE_FIELD(CBasePlayer, m_flFlashLightTime, FIELD_TIME),
		DEFINE_FIELD(CBasePlayer, m_iFlashBattery, FIELD_INTEGER),

		DEFINE_FIELD(CBasePlayer, m_afButtonLast, FIELD_INTEGER),
		DEFINE_FIELD(CBasePlayer, m_afButtonPressed, FIELD_INTEGER),
		DEFINE_FIELD(CBasePlayer, m_afButtonReleased, FIELD_INTEGER),

		DEFINE_ARRAY(CBasePlayer, m_rgItems, FIELD_INTEGER, MAX_ITEMS),
		DEFINE_FIELD(CBasePlayer, m_afPhysicsFlags, FIELD_INTEGER),

		DEFINE_FIELD(CBasePlayer, m_flTimeStepSound, FIELD_TIME),
		DEFINE_FIELD(CBasePlayer, m_flTimeWeaponIdle, FIELD_TIME),
		DEFINE_FIELD(CBasePlayer, m_flSwimTime, FIELD_TIME),
		DEFINE_FIELD(CBasePlayer, m_flDuckTime, FIELD_TIME),
		DEFINE_FIELD(CBasePlayer, m_flWallJumpTime, FIELD_TIME),

		DEFINE_FIELD(CBasePlayer, m_flSuitUpdate, FIELD_TIME),
		DEFINE_ARRAY(CBasePlayer, m_rgSuitPlayList, FIELD_INTEGER, CSUITPLAYLIST),
		DEFINE_FIELD(CBasePlayer, m_iSuitPlayNext, FIELD_INTEGER),
		DEFINE_ARRAY(CBasePlayer, m_rgiSuitNoRepeat, FIELD_INTEGER, CSUITNOREPEAT),
		DEFINE_ARRAY(CBasePlayer, m_rgflSuitNoRepeatTime, FIELD_TIME, CSUITNOREPEAT),
		DEFINE_FIELD(CBasePlayer, m_lastDamageAmount, FIELD_INTEGER),

		DEFINE_ARRAY(CBasePlayer, m_rgpPlayerItems, FIELD_CLASSPTR, MAX_ITEM_TYPES),
		DEFINE_FIELD(CBasePlayer, m_pActiveItem, FIELD_CLASSPTR),
		DEFINE_FIELD(CBasePlayer, m_pLastItem, FIELD_CLASSPTR),
		DEFINE_FIELD(CBasePlayer, m_WeaponBits, FIELD_INT64),

		DEFINE_ARRAY(CBasePlayer, m_rgAmmo, FIELD_INTEGER, MAX_AMMO_SLOTS),
		DEFINE_FIELD(CBasePlayer, m_idrowndmg, FIELD_INTEGER),
		DEFINE_FIELD(CBasePlayer, m_idrownrestored, FIELD_INTEGER),
		DEFINE_FIELD(CBasePlayer, m_tSneaking, FIELD_TIME),

		DEFINE_FIELD(CBasePlayer, m_iTrain, FIELD_INTEGER),
		DEFINE_FIELD(CBasePlayer, m_bitsHUDDamage, FIELD_INTEGER),
		DEFINE_FIELD(CBasePlayer, m_flFallVelocity, FIELD_FLOAT),
		DEFINE_FIELD(CBasePlayer, m_iTargetVolume, FIELD_INTEGER),
		DEFINE_FIELD(CBasePlayer, m_iWeaponVolume, FIELD_INTEGER),
		DEFINE_FIELD(CBasePlayer, m_iExtraSoundTypes, FIELD_INTEGER),
		DEFINE_FIELD(CBasePlayer, m_iWeaponFlash, FIELD_INTEGER),
		DEFINE_FIELD(CBasePlayer, m_fLongJump, FIELD_BOOLEAN),
		DEFINE_FIELD(CBasePlayer, m_fInitHUD, FIELD_BOOLEAN),
		DEFINE_FIELD(CBasePlayer, m_tbdPrev, FIELD_TIME),

		DEFINE_FIELD(CBasePlayer, m_pTank, FIELD_EHANDLE),
		DEFINE_FIELD(CBasePlayer, m_hViewEntity, FIELD_EHANDLE),
		DEFINE_FIELD(CBasePlayer, m_iHideHUD, FIELD_INTEGER),
		DEFINE_FIELD(CBasePlayer, m_iFOV, FIELD_INTEGER),

		DEFINE_FIELD(CBasePlayer, m_SndRoomtype, FIELD_INTEGER),
		// Don't save these. Let the game recalculate the closest env_sound, and continue to use the last room type like it always has.
		//DEFINE_FIELD(CBasePlayer, m_SndLast, FIELD_EHANDLE),
		//DEFINE_FIELD(CBasePlayer, m_flSndRange, FIELD_FLOAT),

		DEFINE_FIELD(CBasePlayer, m_flStartCharge, FIELD_TIME),

		//DEFINE_FIELD( CBasePlayer, m_fDeadTime, FIELD_FLOAT ), // only used in multiplayer games
		//DEFINE_FIELD( CBasePlayer, m_fGameHUDInitialized, FIELD_INTEGER ), // only used in multiplayer games
		//DEFINE_FIELD( CBasePlayer, m_flStopExtraSoundTime, FIELD_TIME ),
		//DEFINE_FIELD( CBasePlayer, m_fKnownItem, FIELD_BOOLEAN ), // reset to zero on load
		//DEFINE_FIELD( CBasePlayer, m_iPlayerSound, FIELD_INTEGER ),	// Don't restore, set in Precache()
		//DEFINE_FIELD( CBasePlayer, m_fNewAmmo, FIELD_INTEGER ), // Don't restore, client needs reset
		//DEFINE_FIELD( CBasePlayer, m_flgeigerRange, FIELD_FLOAT ),	// Don't restore, reset in Precache()
		//DEFINE_FIELD( CBasePlayer, m_flgeigerDelay, FIELD_FLOAT ),	// Don't restore, reset in Precache()
		//DEFINE_FIELD( CBasePlayer, m_igeigerRangePrev, FIELD_FLOAT ),	// Don't restore, reset in Precache()
		//DEFINE_FIELD( CBasePlayer, m_iStepLeft, FIELD_INTEGER ), // Don't need to restore
		//DEFINE_ARRAY( CBasePlayer, m_szTextureName, FIELD_CHARACTER, CBTEXTURENAMEMAX ), // Don't need to restore
		//DEFINE_FIELD( CBasePlayer, m_chTextureType, FIELD_CHARACTER ), // Don't need to restore
		//DEFINE_FIELD( CBasePlayer, m_fNoPlayerSound, FIELD_BOOLEAN ), // Don't need to restore, debug
		//DEFINE_FIELD( CBasePlayer, m_iUpdateTime, FIELD_INTEGER ), // Don't need to restore
		//DEFINE_FIELD( CBasePlayer, m_iClientHealth, FIELD_INTEGER ), // Don't restore, client needs reset
		//DEFINE_FIELD( CBasePlayer, m_iClientBattery, FIELD_INTEGER ), // Don't restore, client needs reset
		//DEFINE_FIELD( CBasePlayer, m_iClientHideHUD, FIELD_INTEGER ), // Don't restore, client needs reset
		//DEFINE_FIELD( CBasePlayer, m_fWeapon, FIELD_BOOLEAN ),  // Don't restore, client needs reset
		//DEFINE_FIELD( CBasePlayer, m_nCustomSprayFrames, FIELD_INTEGER ), // Don't restore, depends on server message after spawning and only matters in multiplayer
		//DEFINE_FIELD( CBasePlayer, m_vecAutoAim, FIELD_VECTOR ), // Don't save/restore - this is recomputed
		//DEFINE_ARRAY( CBasePlayer, m_rgAmmoLast, FIELD_INTEGER, MAX_AMMO_SLOTS ), // Don't need to restore
		//DEFINE_FIELD( CBasePlayer, m_fOnTarget, FIELD_BOOLEAN ), // Don't need to restore
		//DEFINE_FIELD( CBasePlayer, m_nCustomSprayFrames, FIELD_INTEGER ), // Don't need to restore

};

LINK_ENTITY_TO_CLASS(player, CBasePlayer);



void CBasePlayer::Pain()
{
	// The stock SDK body (@0xc847c); nothing calls it in the original (no call site, not
	// vtable-dispatched). The live TS pain voice is the throttled pain2/3/4 block inside TraceAttack.
	float flRndSound = RANDOM_FLOAT(0, 1);

	if (flRndSound <= 0.33)
		EMIT_SOUND(ENT(pev), CHAN_VOICE, "player/pl_pain5.wav", 1, ATTN_NORM);
	else if (flRndSound <= 0.66)
		EMIT_SOUND(ENT(pev), CHAN_VOICE, "player/pl_pain6.wav", 1, ATTN_NORM);
	else
		EMIT_SOUND(ENT(pev), CHAN_VOICE, "player/pl_pain7.wav", 1, ATTN_NORM);
}

/* 
 *
 */
Vector VecVelocityForDamage(float flDamage)
{
	Vector vec(RANDOM_FLOAT(-100, 100), RANDOM_FLOAT(-100, 100), RANDOM_FLOAT(200, 300));

	if (flDamage > -50)
		vec = vec * 0.7;
	else if (flDamage > -200)
		vec = vec * 2;
	else
		vec = vec * 10;

	return vec;
}

#if 0 /*                                                                 \
static void ThrowGib(entvars_t *pev, char *szGibModel, float flDamage)   \
{                                                                        \
	edict_t *pentNew = CREATE_ENTITY();                                  \
	entvars_t *pevNew = VARS(pentNew);                                   \
																	   \ \
	pevNew->origin = pev->origin;                                        \
	SET_MODEL(ENT(pevNew), szGibModel);                                  \
	UTIL_SetSize(pevNew, g_vecZero, g_vecZero);                          \
																	   \ \
	pevNew->velocity		= VecVelocityForDamage(flDamage);                  \
	pevNew->movetype		= MOVETYPE_BOUNCE;                                 \
	pevNew->solid			= SOLID_NOT;                                         \
	pevNew->avelocity.x		= RANDOM_FLOAT(0,600);                          \
	pevNew->avelocity.y		= RANDOM_FLOAT(0,600);                          \
	pevNew->avelocity.z		= RANDOM_FLOAT(0,600);                          \
	CHANGE_METHOD(ENT(pevNew), em_think, SUB_Remove);                    \
	pevNew->ltime		= gpGlobals->time;                                    \
	pevNew->nextthink	= gpGlobals->time + RANDOM_FLOAT(10,20);           \
	pevNew->frame		= 0;                                                  \
	pevNew->flags		= 0;                                                  \
}                                                                        \
																	   \ \
																	   \ \
static void ThrowHead(entvars_t *pev, char *szGibModel, floatflDamage)   \
{                                                                        \
	SET_MODEL(ENT(pev), szGibModel);                                     \
	pev->frame			= 0;                                                    \
	pev->nextthink		= -1;                                                \
	pev->movetype		= MOVETYPE_BOUNCE;                                    \
	pev->takedamage		= DAMAGE_NO;                                        \
	pev->solid			= SOLID_NOT;                                            \
	pev->view_ofs		= Vector(0,0,8);                                      \
	UTIL_SetSize(pev, Vector(-16,-16,0), Vector(16,16,56));              \
	pev->velocity		= VecVelocityForDamage(flDamage);                     \
	pev->avelocity		= RANDOM_FLOAT(-1,1) * Vector(0,600,0);              \
	pev->origin.z -= 24;                                                 \
	ClearBits(pev->flags, FL_ONGROUND);                                  \
}                                                                        \
																	   \ \
																	   \ \
*/
#endif

int TrainSpeed(int iSpeed, int iMax)
{
	float fSpeed, fMax;
	int iRet = 0;

	fMax = (float)iMax;
	fSpeed = iSpeed;

	fSpeed = fSpeed / fMax;

	if (iSpeed < 0)
		iRet = TRAIN_BACK;
	else if (iSpeed == 0)
		iRet = TRAIN_NEUTRAL;
	else if (fSpeed < 0.33)
		iRet = TRAIN_SLOW;
	else if (fSpeed < 0.66)
		iRet = TRAIN_MEDIUM;
	else
		iRet = TRAIN_FAST;

	return iRet;
}

void CBasePlayer::DeathSound()
{
	// water death sounds
	/*
	if (pev->waterlevel == 3)
	{
		EMIT_SOUND(ENT(pev), CHAN_VOICE, "player/h2odeath.wav", 1, ATTN_NONE);
		return;
	}
	*/

	// temporarily using pain sounds for death sounds
	switch (RANDOM_LONG(1, 5))
	{
	case 1:
		EMIT_SOUND(ENT(pev), CHAN_VOICE, "player/pl_pain5.wav", 1, ATTN_NORM);
		break;
	case 2:
		EMIT_SOUND(ENT(pev), CHAN_VOICE, "player/pl_pain6.wav", 1, ATTN_NORM);
		break;
	case 3:
		EMIT_SOUND(ENT(pev), CHAN_VOICE, "player/pl_pain7.wav", 1, ATTN_NORM);
		break;
	}

	// play one of the suit death alarms
	EMIT_GROUPNAME_SUIT(ENT(pev), "HEV_DEAD");
}

// override takehealth
// bitsDamageType indicates type of damage healed.

bool CBasePlayer::TakeHealth(float flHealth, int bitsDamageType)
{
	return CBaseMonster::TakeHealth(flHealth, bitsDamageType);
}

Vector CBasePlayer::GetGunPosition()
{
	//	UTIL_MakeVectors(pev->v_angle);
	//	m_HackedGunPos = pev->view_ofs;
	Vector origin;

	origin = pev->origin + pev->view_ofs;

	return origin;
}

//=========================================================
// TraceAttack
//=========================================================
void CBasePlayer::TraceAttack(entvars_t* pevAttacker, float flDamage, Vector vecDir, TraceResult* ptr, int bitsDamageType)
{
	// Spawn protection (RE @0x801eb, first statement): a freshly-spawned player
	// (TSInit sets m_flSpawnProtectTime = time + 1.0) takes NO trace damage, and the
	// early path does nothing else — no blood, no sound, no message.
	if (gpGlobals->time < m_flSpawnProtectTime)
		return;

	// TS damage damping (RE @0x80203): while the victim is in a slow-mo / stunt lock
	// state (pev->iuser4 & 0x10c10) incoming damage is x0.7.
	if (pev->iuser4 & 0x10c10)
		flDamage *= 0.7f;

	// Bullet-time matrix-dodge window (RE @0x80228): CTSStunt::GoBulletTime (standing
	// +alt1 dodge) sets m_flBulletTimeEnd = time + 0.55; a BULLET taken inside the
	// window is x0.7. Stacks with the iuser4 damping above (0.49x combined).
	if (gpGlobals->time < m_TSStunt.m_flBulletTimeEnd && (bitsDamageType & DMG_BULLET) != 0)
		flDamage *= 0.7f;

#ifndef CLIENT_DLL
	// Friendly fire (RE @0x8024f): a teammate's trace hit does nothing at all when
	// mp_friendlyfire is off (no blood, no sound, no damage).
	{
		CBaseEntity* pAtt = GetClassPtr((CBaseEntity*)pevAttacker);
		if (pAtt && pAtt->IsPlayer() && g_pGameRules->PlayerRelationship(pAtt, this) == GR_TEAMMATE &&
			friendlyfire.value == 0)
			return;
	}
#endif

	if (0 == pev->takedamage)
		return;

	m_LastHitGroup = ptr->iHitgroup;

	// TS hitgroup multipliers (RE @0x901cc) — these REPLACE the stock gSkillData plr* values,
	// which were wrong for TS and caused the player-damage complaints:
	//  - HEAD (1): doubles ONLY for bullet-type damage (mask 0x8000002 = DMG_BULLET|0x8000000),
	//    or x1.5 for the 0x2000000 type (shotgun pellets). A MELEE headshot gets NO bonus — the
	//    stock plrHead x3 on a club hit was the jumping-kick INSTA-KILL. Sets glast_headshot.
	//  - LEGS (6/7): x0.5.
	//  - Chest/stomach/arms/generic: x1 (TS has NO torso multiplier; stock plrChest/etc inflated
	//    pistol damage -> the "2-shot beretta").
	bool bHead = false;
	switch (ptr->iHitgroup)
	{
	case HITGROUP_HEAD: // 1
		if (bitsDamageType & 0x8000002)
		{
			flDamage *= (bitsDamageType & 0x2000000) ? 1.5f : 2.0f;
			glast_headshot = true;
			bHead = true;
		}
		break;
	case HITGROUP_LEFTLEG:  // 6
	case HITGROUP_RIGHTLEG: // 7
		flDamage *= 0.5f;
		break;
	default: // generic / chest / stomach / arms — no multiplier in TS
		break;
	}

	// One flag (RE decompile l.80–83) drives BOTH the kevlarhit sound AND the +128
	// upper-body mark on the TSTakeDamage wire byte: an armored, LIVING victim hit by
	// a BULLET in chest..right-arm (GENERIC excluded, arms included).
	const bool bArmoredHit = pev->armorvalue != 0 && (bitsDamageType & DMG_BULLET) != 0 && IsAlive() &&
							 m_LastHitGroup >= HITGROUP_CHEST && m_LastHitGroup <= HITGROUP_RIGHTARM;

#ifndef CLIENT_DLL
	// A shot corpse twitches (RE decompile l.84–100): bullet-dance the body, and an
	// airborne one drifts with the shot.
	if (!IsAlive())
	{
		m_TSAnim.BulletDance(this);
		if ((pev->flags & FL_ONGROUND) == 0)
			pev->velocity = pev->velocity + vecDir * 50.0f;
	}

	// The throttled feedback block — the PTakeDam wire report AND all the victim
	// sounds live INSIDE the one 0.3 s gate (RE: one `m_flNextPTakeDamTime < time`
	// test wraps message + hit sound + pain voice).
	if (m_flNextPTakeDamTime < gpGlobals->time)
	{
		m_flNextPTakeDamTime = gpGlobals->time + 0.3f;

		// PTakeDam — the TS pain/hit-feedback wire (disassembled verbatim, dossier-gmsg U4a):
		// damage encoded as dmg*0.5 (x8 first when damage bit 25 is set), capped at 127,
		// +128 marks an ARMORED upper-body hit (bArmoredHit). MSG_PVS at the victim's
		// origin: hit point (3 COORDs) + hit direction (3 COORDs) + the encoded byte.
		float enc = flDamage * (((bitsDamageType & (1 << 25)) != 0) ? 8.0f : 1.0f) * 0.5f;
		if (enc > 127.0f)
			enc = 127.0f;
		if (bArmoredHit)
			enc += 128.0f;
		MESSAGE_BEGIN(MSG_PVS, gmsgTSTakeDamage, pev->origin);
		WRITE_COORD(ptr->vecEndPos.x);
		WRITE_COORD(ptr->vecEndPos.y);
		WRITE_COORD(ptr->vecEndPos.z);
		WRITE_COORD(vecDir.x);
		WRITE_COORD(vecDir.y);
		WRITE_COORD(vecDir.z);
		WRITE_BYTE((int)enc);
		MESSAGE_END();

		// TS body-impact sound on the victim (CHAN_BODY; bullet/BLAST only — RE mask
		// 0x42, not DMG_CLUB): a bullet-type headshot, kevlarhit off the vest
		// (bArmoredHit), or a random bodyhit. Only headshot.wav / headshot2.wav exist
		// (there is no headshot1.wav). vol 1.0, attn 1.25, pitch 95+rand(0,31).
		// (EMIT_SOUND_DYN is the TS 8-arg overload; the trailing float is the slow-motion
		// pitch WEIGHT (floor = 1-w): body-hit uses the 0.5 default (no explicit arg),
		// pain voice below passes 0.15 — it barely deepens. RE @0x8079a/@0x80828; the
		// weight-vs-floor roles in the older D14 note were swapped — cont94
		// emit_sound_dyn_overload.md §1 is the corrected model.)
		if (bitsDamageType & (DMG_BULLET | DMG_BLAST))
		{
			char snd[24];
			if (bHead)
				strcpy(snd, RANDOM_LONG(0, 1) == 0 ? "player/headshot.wav" : "player/headshot2.wav");
			else if (bArmoredHit)
				strcpy(snd, "player/kevlarhit.wav"); // round stopped by the vest
			else
				sprintf(snd, "player/bodyhit%d.wav", RANDOM_LONG(1, 4));
			EMIT_SOUND_DYN(ENT(pev), CHAN_BODY, snd, 1.0f, 1.25f, 0, 95 + RANDOM_LONG(0, 0x1f));
		}

		// TS pain voice (RE: same throttle, after the hit sound, no damage-type mask):
		// ~3/7 of throttled hits grunt pain2/3/4 on CHAN_VOICE.
		if (IsAlive())
		{
			const char* pain = NULL;
			switch (RANDOM_LONG(1, 7))
			{
			case 1: pain = "player/pain2.wav"; break;
			case 2: pain = "player/pain3.wav"; break;
			case 3: pain = "player/pain4.wav"; break;
			}
			if (pain)
				EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, pain, 1.0f, 1.25f, 0, 100, 0.15f); // 0.15f @0xF7F10 (RE @0x80828)
		}
	}
#endif

	// No SpawnBlood or TraceBleed (the stock SDK's): the hit blood is the client's, from PTakeDam.

	AddMultiDamage(pevAttacker, this, flDamage, bitsDamageType);

#ifndef CLIENT_DLL
	// dmgreport (RE @0x80842): with cheats on (as the map started) and dmgreport set, log each hit.
	if (g_flWeaponCheat != 0 && CVAR_GET_FLOAT("dmgreport") != 0)
		ALERT(at_console, "%s damaged %s (%f hp)\n", STRING(pevAttacker->netname), STRING(pev->netname), flDamage);
#endif
}

/*
	Take some damage.  
	NOTE: each call to TakeDamage with bitsDamageType set to a time-based damage
	type will cause the damage time countdown to be reset.  Thus the ongoing effects of poison, radiation
	etc are implemented with subsequent calls to TakeDamage using DMG_GENERIC.
*/

#define ARMOR_RATIO 0.2 // Armor Takes 80% of the damage
#define ARMOR_BONUS 0.5 // Each Point of Armor is work 1/x points of health

bool CBasePlayer::TakeDamage(entvars_t* pevInflictor, entvars_t* pevAttacker, float flDamage, int bitsDamageType)
{
	// have suit diagnose the problem - ie: report damage type
	int bitsDamage = bitsDamageType;
	bool ffound = true;
	bool fmajor;
	bool fcritical;
	bool fTookDamage;
	bool ftrivial;
	float flRatio;
	float flBonus;
	float flHealthPrev = pev->health;

	flBonus = ARMOR_BONUS;
	flRatio = ARMOR_RATIO;

	if ((bitsDamageType & DMG_BLAST) != 0 && g_pGameRules->IsMultiplayer())
	{
		// blasts damage armor more.
		flBonus *= 2;
	}

	// Already dead, or no damage (@0x7faeb)
	if (!IsAlive() || flDamage == 0)
		return false;

	// @0x7fb11: the bullet dance. A hit that is not in the legs, on a player moving under 80 units a
	// second, for 80 or less, turns it on (the 3 s window that makes the death sequence normal_die*
	// and lets shots re-pose a dying body); anything else turns it off.
	if ((unsigned int)(m_LastHitGroup - HITGROUP_LEFTLEG) < 2 || pev->velocity.Length2D() >= 80.0f ||
		flDamage > 80.0f)
		m_TSAnim.DisableBulletDance(this);
	else
		m_TSAnim.EnableBulletDance(this);


	CBaseEntity* pAttacker = CBaseEntity::Instance(pevAttacker);

	if (!g_pGameRules->FPlayerCanTakeDamage(this, pAttacker))
	{
		// Refuse the damage
		return false;
	}

	// keep track of amount of damage last sustained
	m_lastDamageAmount = flDamage;

	// Armor (TS model — RE'd from CBasePlayer::TakeDamage @0x7fa94). Kevlar is a flat
	// `armor_absorb` damage REDUCTION, not a depleting pool: while the player has armor and
	// the damage isn't one of the armor-bypassing types, scale the damage down. armorvalue is
	// never decremented (it's a binary "have kevlar" marker — see CBasePlayer::TakeArmor).
	// The bypass mask 0x80040e4 = melee (slash/club), fall, blast, radiation, + the
	// no-armor bit: kevlar stops bullets, not knives/falls/explosions. It covers only the body and
	// arms (@0x7fc17: m_LastHitGroup - 2 <= 3, hit groups 2 to 5), so a head or leg hit takes it all.
	// armor_absorb is capped at 1 and has no lower bound (@0x7fc39).
	constexpr int TS_ARMOR_BYPASS = 0x80040e4;
	if (0 != pev->armorvalue && (bitsDamageType & TS_ARMOR_BYPASS) == 0 &&
		(unsigned int)(m_LastHitGroup - HITGROUP_CHEST) <= HITGROUP_RIGHTARM - HITGROUP_CHEST)
	{
		float absorb = TSGameRules->m_flArmorAbsorb;
		if (absorb > 1.0f)
			absorb = 1.0f;
		flDamage -= absorb * flDamage; // flDamage *= (1 - armor_absorb)
	}

	// this cast to INT is critical!!! If a player ends up with 0.5 health, the engine will get that
	// as an int (zero) and think the player is dead! (this will incite a clientside screentilt, etc)
	fTookDamage = CBaseMonster::TakeDamage(pevInflictor, pevAttacker, (int)flDamage, bitsDamageType);

	// reset damage time countdown for each type of time based damage player just sustained

	{
		for (int i = 0; i < CDMG_TIMEBASED; i++)
			if ((bitsDamageType & (DMG_PARALYZE << i)) != 0)
				m_rgbTimeBasedDamage[i] = 0;
	}

	// tell director about it
	MESSAGE_BEGIN(MSG_SPEC, SVC_DIRECTOR);
	WRITE_BYTE(9);							  // command length in bytes
	WRITE_BYTE(DRC_CMD_EVENT);				  // take damage event
	WRITE_SHORT(ENTINDEX(this->edict()));	  // index number of primary entity
	WRITE_SHORT(ENTINDEX(ENT(pevInflictor))); // index number of secondary entity
	WRITE_LONG(5);							  // eventflags (priority and flags)
	MESSAGE_END();


	// how bad is it, doc?

	ftrivial = (pev->health > 75 || m_lastDamageAmount < 5);
	fmajor = (m_lastDamageAmount > 25);
	fcritical = (pev->health < 30);

	// No Pain() call: CBasePlayer::Pain (@0xc847c) has no call site; the TS pain voice is the
	// throttled pain2/3/4 block inside TraceAttack.

	// handle all bits set in this damage message,
	// let the suit give player the diagnosis

	// UNDONE: add sounds for types of damage sustained (ie: burn, shock, slash )

	// UNDONE: still need to record damage and heal messages for the following types

	// DMG_BURN
	// DMG_FREEZE
	// DMG_BLAST
	// DMG_SHOCK

	m_bitsDamageType |= bitsDamage; // Save this so we can report it to the client
	m_bitsHUDDamage = -1;			// make sure the damage bits get resent

	while (fTookDamage && (!ftrivial || (bitsDamage & DMG_TIMEBASED) != 0) && ffound && 0 != bitsDamage)
	{
		ffound = false;

		if ((bitsDamage & DMG_CLUB) != 0)
		{
			if (fmajor)
				SetSuitUpdate("!HEV_DMG4", false, SUIT_NEXT_IN_30SEC); // minor fracture
			bitsDamage &= ~DMG_CLUB;
			ffound = true;
		}
		if ((bitsDamage & (DMG_FALL | DMG_CRUSH)) != 0)
		{
			if (fmajor)
				SetSuitUpdate("!HEV_DMG5", false, SUIT_NEXT_IN_30SEC); // major fracture
			else
				SetSuitUpdate("!HEV_DMG4", false, SUIT_NEXT_IN_30SEC); // minor fracture

			bitsDamage &= ~(DMG_FALL | DMG_CRUSH);
			ffound = true;
		}

		if ((bitsDamage & DMG_BULLET) != 0)
		{
			if (m_lastDamageAmount > 5)
				SetSuitUpdate("!HEV_DMG6", false, SUIT_NEXT_IN_30SEC); // blood loss detected
			//else
			//	SetSuitUpdate("!HEV_DMG0", false, SUIT_NEXT_IN_30SEC);	// minor laceration

			bitsDamage &= ~DMG_BULLET;
			ffound = true;
		}

		if ((bitsDamage & DMG_SLASH) != 0)
		{
			if (fmajor)
				SetSuitUpdate("!HEV_DMG1", false, SUIT_NEXT_IN_30SEC); // major laceration
			else
				SetSuitUpdate("!HEV_DMG0", false, SUIT_NEXT_IN_30SEC); // minor laceration

			bitsDamage &= ~DMG_SLASH;
			ffound = true;
		}

		if ((bitsDamage & DMG_SONIC) != 0)
		{
			if (fmajor)
				SetSuitUpdate("!HEV_DMG2", false, SUIT_NEXT_IN_1MIN); // internal bleeding
			bitsDamage &= ~DMG_SONIC;
			ffound = true;
		}

		if ((bitsDamage & (DMG_POISON | DMG_PARALYZE)) != 0)
		{
			SetSuitUpdate("!HEV_DMG3", false, SUIT_NEXT_IN_1MIN); // blood toxins detected
			bitsDamage &= ~(DMG_POISON | DMG_PARALYZE);
			ffound = true;
		}

		if ((bitsDamage & DMG_ACID) != 0)
		{
			SetSuitUpdate("!HEV_DET1", false, SUIT_NEXT_IN_1MIN); // hazardous chemicals detected
			bitsDamage &= ~DMG_ACID;
			ffound = true;
		}

		if ((bitsDamage & DMG_NERVEGAS) != 0)
		{
			SetSuitUpdate("!HEV_DET0", false, SUIT_NEXT_IN_1MIN); // biohazard detected
			bitsDamage &= ~DMG_NERVEGAS;
			ffound = true;
		}

		if ((bitsDamage & DMG_RADIATION) != 0)
		{
			SetSuitUpdate("!HEV_DET2", false, SUIT_NEXT_IN_1MIN); // radiation detected
			bitsDamage &= ~DMG_RADIATION;
			ffound = true;
		}
		if ((bitsDamage & DMG_SHOCK) != 0)
		{
			bitsDamage &= ~DMG_SHOCK;
			ffound = true;
		}
	}

	// @0x7fff8: a red flash for any hit of a point or more (the armour-reduced damage); the original
	// does not kick the view (the SDK's punchangle.x = -2).
	if (flDamage >= 1.0f)
		UTIL_ScreenFade(this, Vector(150, 10, 10), 0.3f, 0.15f, 75, 0);

	if (fTookDamage && !ftrivial && fmajor && flHealthPrev >= 75)
	{
		// first time we take major damage...
		// turn automedic on if not on
		SetSuitUpdate("!HEV_MED1", false, SUIT_NEXT_IN_30MIN); // automedic on

		// give morphine shot if not given recently
		SetSuitUpdate("!HEV_HEAL7", false, SUIT_NEXT_IN_30MIN); // morphine shot
	}

	if (fTookDamage && !ftrivial && fcritical && flHealthPrev < 75)
	{

		// already took major damage, now it's critical...
		if (pev->health < 6)
			SetSuitUpdate("!HEV_HLTH3", false, SUIT_NEXT_IN_10MIN); // near death
		else if (pev->health < 20)
			SetSuitUpdate("!HEV_HLTH2", false, SUIT_NEXT_IN_10MIN); // health critical

		// give critical health warnings
		if (!RANDOM_LONG(0, 3) && flHealthPrev < 50)
			SetSuitUpdate("!HEV_DMG7", false, SUIT_NEXT_IN_5MIN); //seek medical attention
	}

	// if we're taking time based damage, warn about its continuing effects
	if (fTookDamage && (bitsDamageType & DMG_TIMEBASED) != 0 && flHealthPrev < 75)
	{
		if (flHealthPrev < 50)
		{
			if (!RANDOM_LONG(0, 3))
				SetSuitUpdate("!HEV_DMG7", false, SUIT_NEXT_IN_5MIN); //seek medical attention
		}
		else
			SetSuitUpdate("!HEV_HLTH1", false, SUIT_NEXT_IN_10MIN); // health dropping
	}

	return fTookDamage;
}

//=========================================================
// PackDeadPlayerItems - call this when a player dies to
// pack up the appropriate weapons and ammo items, and to
// destroy anything that shouldn't be packed.
//
// This is pretty brute force :(
//=========================================================
void CBasePlayer::PackDeadPlayerItems()
{
	int iWeaponRules;
	int iAmmoRules;
	int i;
	CBasePlayerWeapon* rgpPackWeapons[MAX_WEAPONS];
	int iPackAmmo[MAX_AMMO_SLOTS + 1];
	int iPW = 0; // index into packweapons array
	int iPA = 0; // index into packammo array

	memset(rgpPackWeapons, 0, sizeof(rgpPackWeapons));
	memset(iPackAmmo, -1, sizeof(iPackAmmo));

	// get the game rules
	iWeaponRules = g_pGameRules->DeadPlayerWeapons(this);
	iAmmoRules = g_pGameRules->DeadPlayerAmmo(this);

	if (iWeaponRules == GR_PLR_DROP_GUN_NO && iAmmoRules == GR_PLR_DROP_AMMO_NO)
	{
		// nothing to pack. Remove the weapons and return. Don't call create on the box!
		RemoveAllItems(true);
		return;
	}

	// go through all of the weapons and make a list of the ones to pack
	for (i = 0; i < MAX_ITEM_TYPES; i++)
	{
		if (m_rgpPlayerItems[i])
		{
			// there's a weapon here. Should I pack it?
			CBasePlayerItem* pPlayerItem = m_rgpPlayerItems[i];

			while (pPlayerItem)
			{
				switch (iWeaponRules)
				{
				case GR_PLR_DROP_GUN_ACTIVE:
					if (m_pActiveItem && pPlayerItem == m_pActiveItem)
					{
						CBasePlayerWeapon* pWeapon = (CBasePlayerWeapon*)pPlayerItem;
						int nIndex = iPW++;

						// this is the active item. Pack it.
						rgpPackWeapons[nIndex] = pWeapon;

						// Reload the weapon before dropping it if we have ammo
						int j = V_min(pWeapon->iMaxClip() - pWeapon->m_iClip, m_rgAmmo[pWeapon->m_iPrimaryAmmoType]);

						// Add them to the clip
						pWeapon->m_iClip += j;
						m_rgAmmo[pWeapon->m_iPrimaryAmmoType] -= j;

						TabulateAmmo();
					}
					break;

				case GR_PLR_DROP_GUN_ALL:
					rgpPackWeapons[iPW++] = (CBasePlayerWeapon*)pPlayerItem;
					break;

				default:
					break;
				}

				pPlayerItem = pPlayerItem->m_pNext;
			}
		}
	}

	// now go through ammo and make a list of which types to pack.
	if (iAmmoRules != GR_PLR_DROP_AMMO_NO)
	{
		for (i = 0; i < MAX_AMMO_SLOTS; i++)
		{
			if (m_rgAmmo[i] > 0)
			{
				// player has some ammo of this type.
				switch (iAmmoRules)
				{
				case GR_PLR_DROP_AMMO_ALL:
					iPackAmmo[iPA++] = i;
					break;

				case GR_PLR_DROP_AMMO_ACTIVE:
					if (m_pActiveItem && i == m_pActiveItem->PrimaryAmmoIndex())
					{
						// this is the primary ammo type for the active weapon
						iPackAmmo[iPA++] = i;
					}
					else if (m_pActiveItem && i == m_pActiveItem->SecondaryAmmoIndex())
					{
						// this is the secondary ammo type for the active weapon
						iPackAmmo[iPA++] = i;
					}
					break;

				default:
					break;
				}
			}
		}
	}

	// create a box to pack the stuff into.
	CWeaponBox* pWeaponBox = (CWeaponBox*)CBaseEntity::Create("weaponbox", pev->origin, pev->angles, edict());

	pWeaponBox->pev->angles.x = 0; // don't let weaponbox tilt.
	pWeaponBox->pev->angles.z = 0;

	pWeaponBox->SetThink(&CWeaponBox::Kill);
	pWeaponBox->pev->nextthink = gpGlobals->time + 120;

	// back these two lists up to their first elements
	iPA = 0;
	iPW = 0;

	bool bPackItems = true;

	if (iAmmoRules == GR_PLR_DROP_AMMO_ACTIVE && iWeaponRules == GR_PLR_DROP_GUN_ACTIVE)
	{
		if (rgpPackWeapons[0] && FClassnameIs(rgpPackWeapons[0]->pev, "weapon_satchel") && (iPackAmmo[0] == -1 || (m_rgAmmo[iPackAmmo[0]] == 0)))
		{
			bPackItems = false;
		}
	}

	if (bPackItems)
	{
		// pack the ammo
		while (iPackAmmo[iPA] != -1)
		{
			pWeaponBox->PackAmmo(MAKE_STRING(CBasePlayerItem::AmmoInfoArray[iPackAmmo[iPA]].pszName), m_rgAmmo[iPackAmmo[iPA]]);
			iPA++;
		}

		// now pack all of the items in the lists
		while (rgpPackWeapons[iPW])
		{
			// weapon unhooked from the player. Pack it into der box.
			pWeaponBox->PackWeapon(rgpPackWeapons[iPW]);

			iPW++;
		}
	}

	pWeaponBox->pev->velocity = pev->velocity * 1.2; // weaponbox has player's velocity, then some.

	RemoveAllItems(true); // now strip off everything that wasn't handled by the code above.
}

void CBasePlayer::RemoveAllItems(bool removeSuit)
{
	if (m_pActiveItem)
	{
		ResetAutoaim();
		m_pActiveItem->Holster();
		m_pActiveItem = NULL;
	}

	m_pLastItem = NULL;

	// @0xc8cec: no func_tank release here, unlike the SDK (the original's has none).
	int i;
	CBasePlayerItem* pPendingItem;
	for (i = 0; i < MAX_ITEM_TYPES; i++)
	{
		m_pActiveItem = m_rgpPlayerItems[i];
		while (m_pActiveItem)
		{
			pPendingItem = m_pActiveItem->m_pNext;
			m_pActiveItem->Drop();
			m_pActiveItem = pPendingItem;
		}
		m_rgpPlayerItems[i] = NULL;
	}
	m_pActiveItem = NULL;

	pev->viewmodel = 0;
	pev->weaponmodel = 0;

	// @0xc8e3c: removeSuit clears every weapon bit; otherwise only the suit bit (0x80000000) is kept,
	// if the player has it. No SetHasSuit: a player without the suit is not given it. pev->weapons
	// stays in lockstep with m_WeaponBits (see SetWeaponBit).
	if (removeSuit)
	{
		m_WeaponBits = 0ULL;
		pev->weapons = 0;
	}
	else
	{
		m_WeaponBits &= 1ULL << WEAPON_SUIT;
		pev->weapons &= 1u << WEAPON_SUIT;
	}

	for (i = 0; i < MAX_AMMO_SLOTS; i++)
		m_rgAmmo[i] = 0;

	UpdateClientData();

	// @0xc8ef9: then the ammo HUD is told there is no weapon (CurWeapon 0 0 0), on every strip: the
	// death (PlayerDeathThink), StartObserver and the rest.
	MESSAGE_BEGIN(MSG_ONE, gmsgCurWeapon, NULL, pev);
	WRITE_BYTE(0);
	WRITE_BYTE(0);
	WRITE_BYTE(0);
	MESSAGE_END();
}

/*
 * GLOBALS ASSUMED SET:  g_ulModelIndexPlayer
 *
 * ENTITY_METHOD(PlayerDie)
 */

void CBasePlayer::Killed(entvars_t* pevAttacker, int iGib)
{
	extern int gmsgTSMotionBlur; // ts_player.cpp

	// @0x7ed73: the killer's screen blurs on a roll (80% after a close-combat blow, else 50%; a
	// close-combat blow that misses its roll gets the 50% one), the victim's on a 50% roll
	// (TSMBlur {index, 1} and {index, 0}, MSG_PVS at each one's origin).
	if ((glast_closecombat && RANDOM_LONG(0, 100) <= 79) || RANDOM_LONG(0, 100) <= 49)
	{
		MESSAGE_BEGIN(MSG_PVS, gmsgTSMotionBlur, pevAttacker->origin);
		WRITE_BYTE(ENTINDEX(pevAttacker->pContainingEntity));
		WRITE_BYTE(1);
		MESSAGE_END();
	}
	if (RANDOM_LONG(0, 100) <= 49)
	{
		MESSAGE_BEGIN(MSG_PVS, gmsgTSMotionBlur, pev->origin);
		WRITE_BYTE(ENTINDEX(edict()));
		WRITE_BYTE(0);
		MESSAGE_END();
	}

	// @0x7ee97: the body lingers 3 s before he spectates (any button cuts it short), the TSCash cache
	// is cleared so it goes again, and the loadout is cleared: the next spawn has what the client
	// sends again (the buy menu reopens at spectate).
	m_flDeathCamTime = gpGlobals->time + 3.0f;
	m_vecDeathBodyAngles = pev->angles; // reTS holds the body at its death pose (PlayerDeathThink)
	m_iClientTSCash = -1;
	ResetArmament();
	SET_VIEW(edict(), edict());
	EnableControl(true);
	m_iRespawnsUsed++; // @0x7eee5: deaths this round; last man standing budgets respawns against it

	// @0x7eeeb: killed by a player's own shot (the inflictor is a player), he watches that player:
	// iuser2 and the observer target are set now, so the spectate that follows chases the killer.
	// Anything else (the world, a grenade, a thrown knife) clears iuser2.
	if (g_pevLastInflictor)
	{
		edict_t *pInflictor = g_pevLastInflictor->pContainingEntity;
		if (!pInflictor)
			pInflictor = INDEXENT(0);
		CBaseEntity *pEntity = pInflictor ? CBaseEntity::Instance(pInflictor) : NULL;
		if (pEntity && pEntity->IsPlayer())
		{
			pev->iuser2 = ENTINDEX(pInflictor);
			m_hObserverTarget = UTIL_PlayerByIndex(pev->iuser2);
		}
		else
			pev->iuser2 = 0;
	}

	// @0x7ef8c: the respawn gate is set at death, (int)clamp(respawntime, 5, 30), so the 3 s the
	// body lingers eat into it: the first number he sees is N-2 (round 8, checked against the live
	// original).
	{
		float rt = respawntime.value;
		if (rt < 5)
			rt = 5;
		if (rt > 30)
			rt = 30;
		SetPlayerRespawnTime(this, (int)rt, false); // @0x7efd5: truncated (the control word set to chop)
	}
	// @0x7efeb: TSPAward 1 (out of the round) when the gate is below 0, else 11 (will respawn), as
	// SetPlayerRespawnTime just stamped it.
	m_iAward = (m_flTSRespawnTime < 0.0f) ? 1 : 11;

	// @0x7f00f: a head shot slows his body to a third. The original also tests a rules flag here
	// (rules+0x18) that Init clears and nothing sets, so it always slows.
	if (m_LastHitGroup == HITGROUP_HEAD)
		GoSlow(0.33f, 0);

	pev->iuser4 = 0; // @0x7f038: the stunt bits go, the slow-motion physics bit (2) stays on
	pev->iuser4 |= 2;

	if (m_pActiveItem)
		m_pActiveItem->Holster();

	// @0x7f079: the ammo pack first, then the flag, then every weapon (all eleven ThrowWeapon call sites
	// are here, 0x7f085-0x7f185), and only then the rules' PlayerKilled.
	DropAmmo();
	DropFlag(false);
	TSDropWeaponsOnDeath(this);

	g_pGameRules->PlayerKilled(this, pevAttacker, g_pevLastInflictor);

	if (m_pTank != NULL)
	{
		m_pTank->Use(this, this, USE_OFF, 0);
		m_pTank = NULL;
	}

	// this client isn't going to be thinking for a while, so reset the sound until they respawn
	CSound *pSound = CSoundEnt::SoundPointerForIndex(CSoundEnt::ClientSoundIndex(edict()));
	if (pSound)
		pSound->Reset();

	// @0x7f221: the death sequence (normal_die* when TakeDamage left the bullet dance on, else a
	// directional one, CTSAnimation::Animate case 7)
	m_TSAnim.Animate(this, TSA_DIE);

	pev->modelindex = g_ulModelIndexPlayer; // don't use eyes
	pev->deadflag = DEAD_DYING;
	pev->movetype = MOVETYPE_TOSS;
	ClearBits(pev->flags, FL_ONGROUND);

	// @0x7f275: the body is pushed away from the attack by its damage force, at most 500; a fast
	// runner or a hit over 50 that does not bullet-dance is thrown up at 180 with half gravity. (The
	// SDK's random upward kick is not in the original.)
	{
		const float force = DamageForce((float)m_lastDamageAmount);
		const float push = (force >= 500.0f) ? -500.0f : -force;
		pev->velocity = pev->velocity + g_vecAttackDir.Normalize() * push;
		if ((pev->velocity.Length2D() > 80.0f || m_lastDamageAmount > 50) && !m_TSAnim.BulletDancing())
		{
			pev->velocity.z = 180.0f;
			pev->gravity = 0.5f;
		}
	}

	// clear out the suit message cache so we don't keep chattering
	SetSuitUpdate(NULL, false, 0);

	// send "health" update message to zero: TSHealth, not the stock Health, which the original
	// registers and never sends (@0x7f3fb: TSHealth with player+0x9e0 after it is set to 0). It goes
	// out here, in the frame of the death, after PlayerKilled's DeathMsg; TSX counts the death from
	// it. The UpdateClientData cache is the separate m_iClientTSHealth here.
	extern int gmsgTSHealth;
	m_iClientHealth = 0;
	m_iClientTSHealth = 0;
	MESSAGE_BEGIN(MSG_ONE, gmsgTSHealth, NULL, pev);
	WRITE_BYTE(0);
	MESSAGE_END();

	// @0x7f434: every client stops drawing his laser and flashlight (TSActiveItems {index, 0} to
	// all), where the stock SDK tells his ammo HUD he is dead with a CurWeapon the original never
	// sends here.
	extern int gmsgTSActiveItems; // ts_gun.cpp
	MESSAGE_BEGIN(MSG_ALL, gmsgTSActiveItems);
	WRITE_BYTE(ENTINDEX(edict()));
	WRITE_BYTE(0);
	MESSAGE_END();

	// reset FOV, the entvars field too (@0x7f48d)
	m_iFOV = m_iClientFOV = 0;
	pev->fov = 0;

	MESSAGE_BEGIN(MSG_ONE, gmsgSetFOV, NULL, pev);
	WRITE_BYTE(0);
	MESSAGE_END();

	// The body stays solid while it lingers (shots still hit it and make it dance), and the original
	// plays no death sound (DeathSound has no caller).
	if ((pev->health < -40 && iGib != GIB_NEVER) || iGib == GIB_ALWAYS)
	{
		pev->solid = SOLID_NOT;
		GibMonster(); // This clears pev->model
		pev->effects |= EF_NODRAW;
		return;
	}

	pev->angles.x = 0;
	pev->angles.z = 0;

	SetThink(&CBasePlayer::PlayerDeathThink);
	pev->nextthink = gpGlobals->time + 0.1;
}


// Set the activity based on an event or current state.
//
// TS makes CBasePlayer::SetAnimation an EMPTY no-op (@0xc8f50). The original drives the player
// animation by calling CTSAnimation::Animate(this, <activity>) DIRECTLY from the weapon (CTSGun::Fire/
// KnifeFire/Reload/Deploy) and the movement code (PostThink locomotion 1/6, PM_Jump 4, death 7) — there is
// no SetAnimation indirection. Our callers now do the same (m_TSAnim.Animate directly), so this stays empty.
// (Stock SDK weapons still call it; harmless — they aren't given to players in TS.)
void CBasePlayer::SetAnimation(PLAYER_ANIM playerAnim)
{
}

/*
===========
TabulateAmmo
This function is used to find and store 
all the ammo we have into the ammo vars.
============
*/
void CBasePlayer::TabulateAmmo()
{
	ammo_9mm = AmmoInventory(GetAmmoIndex("9mm"));
	ammo_357 = AmmoInventory(GetAmmoIndex("357"));
	ammo_argrens = AmmoInventory(GetAmmoIndex("ARgrenades"));
	ammo_bolts = AmmoInventory(GetAmmoIndex("bolts"));
	ammo_buckshot = AmmoInventory(GetAmmoIndex("buckshot"));
	ammo_rockets = AmmoInventory(GetAmmoIndex("rockets"));
	ammo_uranium = AmmoInventory(GetAmmoIndex("uranium"));
	ammo_hornets = AmmoInventory(GetAmmoIndex("Hornets"));
}


/*
===========
WaterMove
============
*/
#define AIRTIME 12 // lung full of air lasts this many seconds

void CBasePlayer::WaterMove()
{
	int air;

	if (pev->movetype == MOVETYPE_NOCLIP)
		return;

	if (pev->health < 0)
		return;

	// waterlevel 0 - not in water
	// waterlevel 1 - feet in water
	// waterlevel 2 - waist in water
	// waterlevel 3 - head in water

	if (pev->waterlevel != 3)
	{
		// not underwater

		// play 'up for air' sound
		if (pev->air_finished < gpGlobals->time)
			EMIT_SOUND(ENT(pev), CHAN_VOICE, "player/pl_wade1.wav", 1, ATTN_NORM);
		else if (pev->air_finished < gpGlobals->time + 9)
			EMIT_SOUND(ENT(pev), CHAN_VOICE, "player/pl_wade2.wav", 1, ATTN_NORM);

		pev->air_finished = gpGlobals->time + AIRTIME;
		pev->dmg = 2;

		// if we took drowning damage, give it back slowly
		if (m_idrowndmg > m_idrownrestored)
		{
			// set drowning damage bit.  hack - dmg_drownrecover actually
			// makes the time based damage code 'give back' health over time.
			// make sure counter is cleared so we start count correctly.

			// NOTE: this actually causes the count to continue restarting
			// until all drowning damage is healed.

			m_bitsDamageType |= DMG_DROWNRECOVER;
			m_bitsDamageType &= ~DMG_DROWN;
			m_rgbTimeBasedDamage[itbd_DrownRecover] = 0;
		}
	}
	else
	{ // fully under water
		// stop restoring damage while underwater
		m_bitsDamageType &= ~DMG_DROWNRECOVER;
		m_rgbTimeBasedDamage[itbd_DrownRecover] = 0;

		if (pev->air_finished < gpGlobals->time) // drown!
		{
			if (pev->pain_finished < gpGlobals->time)
			{
				// take drowning damage
				pev->dmg += 1;
				if (pev->dmg > 5)
					pev->dmg = 5;

				const float oldHealth = pev->health;

				TakeDamage(CWorld::World->pev, CWorld::World->pev, pev->dmg, DMG_DROWN);
				pev->pain_finished = gpGlobals->time + 1;

				// track drowning damage, give it back when
				// player finally takes a breath

				// Account for god mode, the unkillable flag, potential damage mitigation
				// and gaining health from drowning to avoid counting damage not actually taken.
				const float drownDamageTaken = std::max(0.f, std::floor(oldHealth - pev->health));

				m_idrowndmg += drownDamageTaken;
			}
		}
		else
		{
			m_bitsDamageType &= ~DMG_DROWN;
		}
	}

	if (0 == pev->waterlevel)
	{
		if (FBitSet(pev->flags, FL_INWATER))
		{
			ClearBits(pev->flags, FL_INWATER);
		}
		return;
	}

	// make bubbles

	if (pev->waterlevel == 3)
	{
		air = (int)(pev->air_finished - gpGlobals->time);
		if (!RANDOM_LONG(0, 0x1f) && RANDOM_LONG(0, AIRTIME - 1) >= air)
		{
			switch (RANDOM_LONG(0, 3))
			{
			case 0:
				EMIT_SOUND(ENT(pev), CHAN_BODY, "player/pl_swim1.wav", 0.8, ATTN_NORM);
				break;
			case 1:
				EMIT_SOUND(ENT(pev), CHAN_BODY, "player/pl_swim2.wav", 0.8, ATTN_NORM);
				break;
			case 2:
				EMIT_SOUND(ENT(pev), CHAN_BODY, "player/pl_swim3.wav", 0.8, ATTN_NORM);
				break;
			case 3:
				EMIT_SOUND(ENT(pev), CHAN_BODY, "player/pl_swim4.wav", 0.8, ATTN_NORM);
				break;
			}
		}
	}

	if (pev->watertype == CONTENT_LAVA) // do damage
	{
		if (pev->dmgtime < gpGlobals->time)
			TakeDamage(CWorld::World->pev, CWorld::World->pev, 10 * pev->waterlevel, DMG_BURN);
	}
	else if (pev->watertype == CONTENT_SLIME) // do damage
	{
		pev->dmgtime = gpGlobals->time + 1;
		TakeDamage(CWorld::World->pev, CWorld::World->pev, 4 * pev->waterlevel, DMG_ACID);
	}

	if (!FBitSet(pev->flags, FL_INWATER))
	{
		SetBits(pev->flags, FL_INWATER);
		pev->dmgtime = 0;
	}
}


// true if the player is attached to a ladder
bool CBasePlayer::IsOnLadder()
{
	return (pev->movetype == MOVETYPE_FLY);
}

void CBasePlayer::PlayerDeathThink()
{
	float flForward;

	if (FBitSet(pev->flags, FL_ONGROUND))
	{
		flForward = pev->velocity.Length() - 20;
		if (flForward <= 0)
			pev->velocity = g_vecZero;
		else
			pev->velocity = flForward * pev->velocity.Normalize();
	}

	// NB: the stock HL `if (HasWeapons()) PackDeadPlayerItems();` weaponbox drop is intentionally
	// omitted — RE-confirmed the original TS PlayerDeathThink does NOT call it (PackDeadPlayerItems
	// is dead code in the original, 0 callers). TS drops weapons via its own DropWeapon/CTSWorldGun
	// path from CTSGameRules::PlayerKilled → TSDropWeaponsOnDeath, so a second stock weaponbox drop
	// here would be a divergence (and would spawn a CWeaponBox → precache w_weaponbox.mdl mid-game →
	// Host_Error on a strict engine). See ts_weapons.cpp (w_weaponbox not precached).
	// What it does instead (@0x7ec03-0x7ec15): a dead player still holding anything is stripped,
	// suit included, which tells his ammo HUD there is no weapon (CurWeapon 0 0 0, RemoveAllItems).
	if (HasWeapons())
		RemoveAllItems(true);

	// @0x7ec1a: the body is dying until its death sequence ends, however long that is (the original
	// has no frame cap and makes no body-queue copy).
	if (0 != pev->modelindex && (!m_fSequenceFinished) && (pev->deadflag == DEAD_DYING))
	{
		StudioFrameAdvance();
		return;
	}

	if (pev->deadflag == DEAD_DYING)
		pev->deadflag = DEAD_DEAD;

	// once we're done animating our death and we're on the ground, we want to set movetype to None so our dead body won't do collisions and stuff anymore
	// this prevents a bug where the dead body would go to a player's head if he walked over it while the dead player was clicking their button to respawn
	if (pev->movetype != MOVETYPE_NONE && FBitSet(pev->flags, FL_ONGROUND))
		pev->movetype = MOVETYPE_NONE;

	StopAnimation();

	pev->effects |= EF_NOINTERP;
	pev->framerate = 0.0;

	// TS: death IS the spectator state. Once the death animation finishes, the dead player enters the
	// spectate/buy phase (the same one used on join) — they respawn by pressing FIRE after
	// the respawn gate, NOT automatically on a timer. The buy/config menu reopens so they can re-pick
	// a loadout before pressing fire. (Replaces the stock button/forcerespawn-timer respawn + deathcam.)
	if (pev->deadflag == DEAD_DEAD)
	{
		// @0x7eaec staging tick: 2 -> 3; the transition test runs on the NEXT think.
		pev->deadflag = DEAD_RESPAWNABLE;
		pev->nextthink = gpGlobals->time + 0.1f;
		return;
	}
	if (pev->deadflag == DEAD_RESPAWNABLE)
	{
		// @0x7eaec: LINGER on the body until the 3 s death-cam deadline elapses OR any
		// button (except IN_SCORE) is pressed — THEN the corpse spawns (the settled pose,
		// lifetime 10.0) and the player vanishes into observer (the original's later snap).
		if ((m_flDeathCamTime < gpGlobals->time || (pev->button & ~IN_SCORE) != 0) &&
			pev->iuser1 == 0)
		{
			{
				extern CBaseEntity *TSCreateDeadCorpse(CBasePlayer *pPlayer, float lifetime);
				TSCreateDeadCorpse(this, 10.0f);
			}
			TSEnterSpectate();   // observer + buy menu; fire-to-respawn via the respawn cmd
			pev->nextthink = -1; // hand off to PlayerPreThink (the death think is done)
			return;
		}
		// The body keeps its angles at death while it lingers. The original server has no such pin: a
		// puppet turning its view on TS 3.0 turns the dead body with it, from the death frame until it
		// spectates. A real player on the original does not see his body turn (round 8), so the turn
		// is not sent then; the pin keeps that look whatever the client sends.
		pev->angles = m_vecDeathBodyAngles;
		pev->nextthink = gpGlobals->time + 0.1f; // keep lingering
	}
}

//=========================================================
// StartDeathCam - find an intermission spot and send the
// player off into observer mode
//=========================================================
void CBasePlayer::StartDeathCam()
{
	edict_t *pSpot, *pNewSpot;
	int iRand;

	if (pev->view_ofs == g_vecZero)
	{
		// don't accept subsequent attempts to StartDeathCam()
		return;
	}

	pSpot = FIND_ENTITY_BY_CLASSNAME(NULL, "info_intermission");

	if (!FNullEnt(pSpot))
	{
		// at least one intermission spot in the world.
		iRand = RANDOM_LONG(0, 3);

		while (iRand > 0)
		{
			pNewSpot = FIND_ENTITY_BY_CLASSNAME(pSpot, "info_intermission");

			if (pNewSpot)
			{
				pSpot = pNewSpot;
			}

			iRand--;
		}

		CopyToBodyQue(pev);

		UTIL_SetOrigin(pev, pSpot->v.origin);
		pev->angles = pev->v_angle = pSpot->v.v_angle;
	}
	else
	{
		// no intermission spot. Push them up in the air, looking down at their corpse
		TraceResult tr;
		CopyToBodyQue(pev);
		UTIL_TraceLine(pev->origin, pev->origin + Vector(0, 0, 128), ignore_monsters, edict(), &tr);

		UTIL_SetOrigin(pev, tr.vecEndPos);
		pev->angles = pev->v_angle = UTIL_VecToAngles(tr.vecEndPos - pev->origin);
	}

	// start death cam

	m_afPhysicsFlags |= PFLAG_OBSERVER;
	pev->view_ofs = g_vecZero;
	pev->fixangle = 1;
	pev->solid = SOLID_NOT;
	pev->takedamage = DAMAGE_NO;
	pev->movetype = MOVETYPE_NONE;
	pev->modelindex = 0;
}

void CBasePlayer::StartObserver(Vector vecPosition, Vector vecViewAngle)
{
	// @0x89634 FIRST act (fold-plan cont94 #13, found gap — decompile inv_BC lines 36-42): flush
	// the cash HUD (client cache +0x544 = cash +0x540 + a direct TSCash MSG_ONE) and clear the
	// slow-motion state (iuser4 lock bits, fuser1/Go/Target back to 1.0), and zero Observer_Idle's
	// clock (player+0x764, @0x8968d).
	{
		extern int gmsgTSCash; // ts_player.cpp global
		m_iClientTSCash = m_iCash;
		MESSAGE_BEGIN(MSG_ONE, gmsgTSCash, NULL, ENT(pev));
		WRITE_SHORT(m_iCash);
		MESSAGE_END();
	}
	pev->iuser4 = 0;
	pev->fuser1 = 1.0f;
	m_flSlowMotionTarget = 1.0f;
	m_flSlowMotionGo = 1.0f;
	m_flNextObserverIdle = 0;

	// clear any clientside entities attached to this player
	MESSAGE_BEGIN(MSG_PAS, SVC_TEMPENTITY, pev->origin);
	WRITE_BYTE(TE_KILLPLAYERATTACHMENTS);
	WRITE_BYTE((byte)entindex());
	MESSAGE_END();

	// @0x8970f-0x89755: with the gun (id 16) in hand, wipe the TS arsenal to bare kung fu (Reset) and
	// holster it (HolsterWeapon); then the active item's Holster(0), which on the gun is HolsterWeapon
	// again. Each HolsterWeapon of a live player sends the kung fu draw with skiplocal 0, so a joiner,
	// alive since ClientPutInServer's Spawn, gets two svc_weaponanim here.
	if (m_pActiveItem)
	{
		if (m_pActiveItem->m_iId == TSGUN_WEAPON_ID)
		{
			CTSGun *pGun = (CTSGun *)m_pActiveItem;
			pGun->Reset();
			pGun->HolsterWeapon();
		}
		m_pActiveItem->Holster();
	}

	// @0x8975f: then a carried CTF flag is dropped (spawns a loose flag), after the holster
	DropFlag(false);

	if (m_pTank != NULL)
	{
		m_pTank->Use(this, this, USE_OFF, 0);
		m_pTank = NULL;
	}

	// clear out the suit message cache so we don't keep chattering
	SetSuitUpdate(NULL, false, 0);

	// @0x897b4: everyone is told the player has no active items (TSActiveItems {idx, 0}), so every
	// client stops drawing his laser and flashlight beams (round-7g: dead players' beams haunted the
	// corpse). Here in StartObserver, as the original, so a teamplay joiner's team change, which calls
	// StartObserver alone, sends it too.
	extern int gmsgTSActiveItems;
	MESSAGE_BEGIN(MSG_ALL, gmsgTSActiveItems);
	WRITE_BYTE(ENTINDEX(edict()));
	WRITE_BYTE(0);
	MESSAGE_END();

	// (no CurWeapon to the ammo HUD here: the original sends CurWeapon only from RemoveAllItems
	// and CBasePlayerWeapon::UpdateClientData)

	// reset FOV
	m_iFOV = m_iClientFOV = 0;
	MESSAGE_BEGIN(MSG_ONE, gmsgSetFOV, NULL, pev);
	WRITE_BYTE(0);
	MESSAGE_END();

	// Setup flags (the HUD stays as it is: the original writes m_iHideHUD only for func_tank)
	m_afPhysicsFlags |= PFLAG_OBSERVER;
	pev->effects = EF_NODRAW;
	pev->view_ofs = g_vecZero;
	pev->angles = pev->v_angle = vecViewAngle;
	pev->fixangle = 1;
	pev->solid = SOLID_NOT;
	pev->takedamage = DAMAGE_NO;
	pev->movetype = MOVETYPE_NONE;
	ClearBits(m_afPhysicsFlags, PFLAG_DUCKING);
	ClearBits(pev->flags, FL_DUCKING);
	pev->deadflag = DEAD_RESPAWNABLE;
	pev->health = 1;

	// Clear out the status bar
	m_fInitHUD = true;

	// (no pev->team write: the original never stores a player's entvars team — its only store of
	// entvars+0x1ac is CWorld::KeyValue @0xe4ca2; @0x898d3 clears takedamage, done above.)
	// @0x89922-0x8997a (disasm-verified this session): the original sends the player's REAL
	// team name (TeamID()) — NOT "" — so a dead/spectating player KEEPS their scoreboard team.
	// The stock hlu "" emptied it -> dead players flapped into the "Spectate" group (user bug).
	MESSAGE_BEGIN(MSG_ALL, gmsgTeamInfo);
	WRITE_BYTE(ENTINDEX(edict()));
	WRITE_STRING(TeamID());
	MESSAGE_END();

	// Remove all the player's stuff, the suit too (@0x89983 passes 1), so a spectator has no weapon bits
	RemoveAllItems(true);

	// Move them to the new position
	UTIL_SetOrigin(pev, vecPosition);

	// Find a player to watch. (The original leaves Observer_HandleButtons' clock, player+0x760, as it
	// was: StartObserver writes only Observer_Idle's, @0x8968d.)
	// @0x8999d..0x899ba: the last mode StopObserver saved, or chase (OBS_CHASE_FREE) when there is
	// none yet, so a joiner starts out chasing a player (Observer_SetMode falls back to roaming
	// when there is nobody to chase).
	Observer_SetMode(m_iObserverLastMode != 0 ? m_iObserverLastMode : OBS_CHASE_FREE);

	// @0x899c2: everyone is told he spectates (Spectator {idx, 1}). Sent here, so a teamplay joiner's
	// team change, which calls StartObserver alone, sends one too (two for him, as TS 3.0).
	MESSAGE_BEGIN(MSG_ALL, gmsgSpectator);
	WRITE_BYTE(ENTINDEX(edict()));
	WRITE_BYTE(1);
	MESSAGE_END();
}

//=========================================================
// TS join/spectate spawn-gate (cont.65).
// A joining (or just-respawned) player is held in spectate until the
// respawn gate elapses and they press fire — mirrors the original StartObserver +
// SetPlayerRespawnTime + "Press Fire To Play!" path. The buy/config menu opens on
// entry. Called from CTSGameRules::InitHUD for a joiner, from PlayerDeathThink after a
// death, and from a round restart.
//=========================================================
// The original's ScoreInfo senders are all inline and write 0 for playerclass (AddPoints
// @0xc9e18); the scoreboard's WAITING (client case 0xb) travels on the award byte (player+0x74c =
// m_iAward, stamped 1/11 by SetPlayerRespawnTime @0x65ff4 and sent by TSPAward from
// UpdateClientData).

void CBasePlayer::TSEnterSpectate(bool bResetArmament)
{
	// observer at the spawn spot, in the mode StartObserver picks (no forced mode: the original's
	// InitHUD and PlayerDeathThink call StartObserver alone, @0x694c8 and @0x7ed49)
	StartObserver(pev->origin, pev->v_angle);

	if (bResetArmament)
		ResetArmament(); // no weapons until they pick a loadout + press fire
	m_iClientSpecSecs = -1;

	// (fold-plan Y6: the WAITING scoreboard status travels on the award byte via
	// SetPlayerRespawnTime -> TSPAward, not on ScoreInfo playerclass — see below.)

	// @0x69511 (InitHUD): the JOIN/none-running path sets the gate here; a DEATH gate is set
	// in Killed (@0x7ef8c) BEFORE the death-cam and must NOT be re-anchored at spectate entry
	// (the death-cam seconds eat into it — the original's observed N-2 first number).
	if (m_flTSRespawnTime <= 0.0f || m_flTSRespawnTime <= gpGlobals->time)
	{
		float rt = respawntime.value;
		if (rt < 5)
			rt = 5;
		if (rt > 30)
			rt = 30;
		SetPlayerRespawnTime(this, (int)rt, false); // @0x6955a (InitHUD): truncated
	}

	// fold-plan G7: the original has no entry-time paint. The PreThink spectate block paints the
	// countdown: for a joiner, whose StartObserver runs from UpdateClientData near the top of PreThink,
	// in the same frame.
}

// The original has no server-side buy menu: the buy/config menu is the client's
// (cl_dll/ts_buymenu.cpp); purchases arrive as tkw/tki/tka console commands and the server's
// "menuselect" is a dead swallow in the gamerules ClientCommand. Fire-to-play is the client's
// "respawn" command.

// The spectate countdown/prompt is the pev->iuser1 != 0 block of CBasePlayer::PreThink @0x808cc
// (region ~0x81458), inline in PreThink.

// Press-fire: leave spectate and enter play with the chosen loadout.
void CBasePlayer::StopObserver()
{
	// @0x89a27: nothing happens while the respawn gate (+0x770) is still ahead.
	if (m_flTSRespawnTime > 0.0f && m_flTSRespawnTime > gpGlobals->time)
		return;

	m_iClientSpecSecs = -1;

	// (fold-plan Y6: no ScoreInfo/playerclass here — the original StopObserver @0x89a14 sends
	// none; the WAITING status clears via the award byte when TSInit resets m_iAward on Spawn.)

	// @0x89a53..0x89ac8: only an observer (iuser1 or iuser2 set) has his mode kept for his next
	// StartObserver, is announced out of spectate to everyone (the stock-shaped spectator-state
	// broadcast, BYTE playerIndex, BYTE 0 (off)) and leaves observer mode: iuser2 and iuser1 are
	// cleared. Nothing else is: not iuser3, not PFLAG_OBSERVER (Spawn clears the physics flags), not
	// m_iHideHUD (StartObserver never hides the HUD).
	if (pev->iuser1 != 0 || pev->iuser2 != 0)
	{
		m_iObserverLastMode = pev->iuser1;
		MESSAGE_BEGIN(MSG_ALL, gmsgSpectator);
		WRITE_BYTE(ENTINDEX(edict()));
		WRITE_BYTE(0);
		MESSAGE_END();
		pev->iuser2 = 0;
		pev->iuser1 = 0;
	}
	// (the watched player, player+0x75c, is kept, @0x89abc: the next StartObserver's
	// Observer_SetMode goes back to him if he is still there)

	// @0x89ad5: the held buttons are let go before respawning into play, so the fire-to-play click
	// (IN_ATTACK) does not carry into the freshly-deployed weapon. The original also zeroes
	// player+0xb34, a field written only here and in Killed and never read, which reTS does not keep.
	// m_afButtonPressed is left alone, as in the original: PreThink works it out again from
	// pev->button before anything reads it.
	pev->button = 0;

	// enter play: respawn -> Spawn -> CTSGameRules::PlayerSpawn equips (the flag is
	// now clear, so the end-of-Spawn guard does not re-enter spectate).
	respawn(pev, false);
	pev->nextthink = -1;
}

// (TSTryRespawn DELETED, fold-plan G6: the original has no such method — the gates live
// INLINE in the "respawn" branch of ClientCommand @0x9d9fc, dlls/client.cpp, where they now
// are. StopObserver @0x89a14 additionally carries its OWN redundant elapsed-gate — Y7 follow-up.)

//
// PlayerUse - handles USE keypress
//
#define PLAYER_SEARCH_RADIUS (float)64

void CBasePlayer::PlayerUse()
{
	if (IsObserver())
		return;

	// Was use pressed or released?
	if (((pev->button | m_afButtonPressed | m_afButtonReleased) & IN_USE) == 0)
		return;

	// Hit Use on a train?
	if ((m_afButtonPressed & IN_USE) != 0)
	{
		if (m_pTank != NULL)
		{
			// Stop controlling the tank
			// TODO: Send HUD Update
			m_pTank->Use(this, this, USE_OFF, 0);
			m_pTank = NULL;
			return;
		}
		else
		{
			if ((m_afPhysicsFlags & PFLAG_ONTRAIN) != 0)
			{
				m_afPhysicsFlags &= ~PFLAG_ONTRAIN;
				m_iTrain = TRAIN_NEW | TRAIN_OFF;
				CBaseEntity* pTrain = CBaseEntity::Instance(pev->groundentity);
				if (pTrain && (pTrain->Classify() == CLASS_VEHICLE))
					((CFuncVehicle*)pTrain)->m_pDriver = NULL;
				return;
			}
			else
			{ // Start controlling the train!
				CBaseEntity* pTrain = CBaseEntity::Instance(pev->groundentity);

				if (pTrain && (pev->button & IN_JUMP) == 0 && FBitSet(pev->flags, FL_ONGROUND) && (pTrain->ObjectCaps() & FCAP_DIRECTIONAL_USE) != 0 && pTrain->OnControls(pev))
				{
					m_afPhysicsFlags |= PFLAG_ONTRAIN;
					m_iTrain = TrainSpeed(pTrain->pev->speed, pTrain->pev->impulse);
					m_iTrain |= TRAIN_NEW;
					if (pTrain->Classify() == CLASS_VEHICLE)
					{
						EMIT_SOUND(ENT(pev), CHAN_ITEM, "plats/vehicle_ignition.wav", 0.8, ATTN_NORM);
						((CFuncVehicle*)pTrain)->m_pDriver = this;
					}
					else
						EMIT_SOUND(ENT(pev), CHAN_ITEM, "plats/train_use1.wav", 0.8, ATTN_NORM);
					return;
				}
			}
		}
	}

	CBaseEntity* pObject = NULL;
	CBaseEntity* pClosest = NULL;
	Vector vecLOS;
	float flMaxDot = VIEW_FIELD_NARROW;
	float flDot;

	UTIL_MakeVectors(pev->v_angle); // so we know which way we are facing

	while ((pObject = UTIL_FindEntityInSphere(pObject, pev->origin, PLAYER_SEARCH_RADIUS)) != NULL)
	{

		if ((pObject->ObjectCaps() & (FCAP_IMPULSE_USE | FCAP_CONTINUOUS_USE | FCAP_ONOFF_USE)) != 0)
		{
			// !!!PERFORMANCE- should this check be done on a per case basis AFTER we've determined that
			// this object is actually usable? This dot is being done for every object within PLAYER_SEARCH_RADIUS
			// when player hits the use key. How many objects can be in that area, anyway? (sjb)
			vecLOS = (VecBModelOrigin(pObject->pev) - (pev->origin + pev->view_ofs));

			// This essentially moves the origin of the target to the corner nearest the player to test to see
			// if it's "hull" is in the view cone
			vecLOS = UTIL_ClampVectorToBox(vecLOS, pObject->pev->size * 0.5);

			flDot = DotProduct(vecLOS, gpGlobals->v_forward);
			if (flDot > flMaxDot)
			{ // only if the item is in front of the user
				pClosest = pObject;
				flMaxDot = flDot;
				//				ALERT( at_console, "%s : %f\n", STRING( pObject->pev->classname ), flDot );
			}
			//			ALERT( at_console, "%s : %f\n", STRING( pObject->pev->classname ), flDot );
		}
	}
	pObject = pClosest;

	// Found an object
	if (pObject)
	{
		//!!!UNDONE: traceline here to prevent USEing buttons through walls
		int caps = pObject->ObjectCaps();

		// TS plays NO +use button click (the stock common/wpn_select.wav is removed) — +use is also
		// the powerup double-tap activator, so the constant clicking is gone in the original.

		if (((pev->button & IN_USE) != 0 && (caps & FCAP_CONTINUOUS_USE) != 0) ||
			((m_afButtonPressed & IN_USE) != 0 && (caps & (FCAP_IMPULSE_USE | FCAP_ONOFF_USE)) != 0))
		{
			if ((caps & FCAP_CONTINUOUS_USE) != 0)
				m_afPhysicsFlags |= PFLAG_USING;

			pObject->Use(this, this, USE_SET, 1);
		}
		// UNDONE: Send different USE codes for ON/OFF.  Cache last ONOFF_USE object to send 'off' if you turn away
		else if ((m_afButtonReleased & IN_USE) != 0 && (pObject->ObjectCaps() & FCAP_ONOFF_USE) != 0) // BUGBUG This is an "off" use
		{
			pObject->Use(this, this, USE_SET, 0);
		}
	}
	// (TS: no common/wpn_denyselect.wav on an empty +use either — the +use sound is gone entirely.)
}



void CBasePlayer::Jump()
{
	Vector vecWallCheckDir; // direction we're tracing a line to find a wall when walljumping
	Vector vecAdjustedVelocity;
	Vector vecSpot;
	TraceResult tr;

	if (FBitSet(pev->flags, FL_WATERJUMP))
		return;

	if (pev->waterlevel >= 2)
	{
		return;
	}

	// TS jump-kick (orig CBasePlayer::Jump tries CTSStunt::CloseAttack first): jumping into a
	// breakable/person with a real weapon kicks it. Runs BEFORE the pogo gate so it fires every
	// think while jump is held — i.e. mid-flight as you reach the target, not only on the press
	// frame. (The original skips its wall-flip acrobatics when the kick connects; we have no
	// acrobatics yet, so the normal jump just proceeds.)
	m_TSStunt.CloseAttack();

	// DOUBLE-JUMP = the wheel/cartwheel flip (RE CBasePlayer::Jump@0x82864): on the IN_JUMP edge try
	// CTSStunt::GoWheel — a double-tap (its own m_flWheelNextTime window) that launches a front/side/back
	// flip landing prone (backward = on feet). If it takes, the normal jump is skipped. GoWheel had no
	// caller here, so the double-jump did nothing.
	if (FBitSet(m_afButtonPressed, IN_JUMP) && m_TSStunt.GoWheel())
		return;

	// jump velocity is sqrt( height * gravity * 2)

	// If this isn't the first frame pressing the jump button, break out.
	if (!FBitSet(m_afButtonPressed, IN_JUMP))
		return; // don't pogo stick

	if ((pev->flags & FL_ONGROUND) == 0 || !pev->groundentity)
	{
		return;
	}

	// many features in this function use v_forward, so makevectors now.
	UTIL_MakeVectors(pev->angles);

	// ClearBits(pev->flags, FL_ONGROUND);		// don't stairwalk

	m_TSAnim.Animate(this, TSA_JUMP);

	if (m_fLongJump &&
		(pev->button & IN_DUCK) != 0 &&
		(pev->flDuckTime > 0) &&
		pev->velocity.Length() > 50)
	{
		m_TSAnim.Animate(this, TSA_JUMP);
	}

	// If you're standing on a conveyor, add it's velocity to yours (for momentum)
	entvars_t* pevGround = VARS(pev->groundentity);
	if (pevGround && (pevGround->flags & FL_CONVEYOR) != 0)
	{
		pev->velocity = pev->velocity + pev->basevelocity;
	}

	// JoshA: CS behaviour does this for tracktrain + train as well,
	// but let's just do this for func_vehicle to avoid breaking existing content.
	//
	// If you're standing on a moving train... then add the velocity of the train to yours.
	if (pevGround && (/*(!strcmp( "func_tracktrain", STRING(pevGround->classname))) ||
			(!strcmp( "func_train", STRING(pevGround->classname))) ) ||*/
			(!strcmp("func_vehicle", STRING(pevGround->classname)))))
		pev->velocity = pev->velocity + pevGround->velocity;
}



// This is a glorious hack to find free space when you've crouched into some solid space
// Our crouching collisions do not work correctly for some reason and this is easier
// than fixing the problem :(
void FixPlayerCrouchStuck(edict_t* pPlayer)
{
	TraceResult trace;

	// Move up as many as 18 pixels if the player is stuck.
	for (int i = 0; i < 18; i++)
	{
		UTIL_TraceHull(pPlayer->v.origin, pPlayer->v.origin, dont_ignore_monsters, head_hull, pPlayer, &trace);
		if (0 != trace.fStartSolid)
			pPlayer->v.origin.z++;
		else
			break;
	}
}

void CBasePlayer::Duck()
{
	// RE CBasePlayer::Duck: on the IN_DUCK EDGE, attempt the dive-ROLL (CTSStunt::GoRoll) — the fast
	// run+duck prone dive that lands prone/feet (players call this the "double jump"). Without this call
	// GoRoll had ZERO callers, so the move was dead code (= the "double-jump does nothing" bug). If the
	// roll doesn't take (too slow / stunt active / cooldown), fall back to the plain duck-transition
	// animation (goduck) while starting to duck.
	if ((m_afButtonPressed & IN_DUCK) && m_TSStunt.GoRoll(0x30))
		return;
	if ((pev->button & IN_DUCK) && !FBitSet(pev->flags, FL_DUCKING))
		m_TSAnim.Animate(this, TSA_DUCK);
}

//
// ID's player as such.
//
int CBasePlayer::Classify()
{
	return CLASS_PLAYER;
}


void CBasePlayer::AddPoints(int score, bool bAllowNegativeScore)
{
	// Positive score always adds
	if (score < 0)
	{
		if (!bAllowNegativeScore)
		{
			if (pev->frags < 0) // Can't go more negative
				return;

			if (-score > pev->frags) // Will this go negative?
			{
				score = -pev->frags; // Sum will be 0
			}
		}
	}

	pev->frags += score;

	MESSAGE_BEGIN(MSG_ALL, gmsgScoreInfo);
	WRITE_BYTE(ENTINDEX(edict()));
	WRITE_SHORT(pev->frags);
	WRITE_SHORT(m_iDeaths);
	WRITE_SHORT(0);
	WRITE_SHORT(g_pGameRules->GetTeamIndex(m_szTeamName) + 1);
	MESSAGE_END();
}


void CBasePlayer::AddPointsToTeam(int score, bool bAllowNegativeScore)
{
	int index = entindex();

	for (int i = 1; i <= gpGlobals->maxClients; i++)
	{
		CBaseEntity* pPlayer = UTIL_PlayerByIndex(i);

		if (pPlayer && i != index)
		{
			if (g_pGameRules->PlayerRelationship(this, pPlayer) == GR_TEAMMATE)
			{
				pPlayer->AddPoints(score, bAllowNegativeScore);
			}
		}
	}
}

// (no InitStatusBar or UpdateStatusBar: the original has neither, and TS has no aim-ID status bar)









#define CLIMB_SHAKE_FREQUENCY 22 // how many frames in between screen shakes when climbing
#define MAX_CLIMB_SPEED 200		 // fastest vertical climbing speed possible
#define CLIMB_SPEED_DEC 15		 // climbing deceleration rate
#define CLIMB_PUNCH_X -7		 // how far to 'punch' client X axis when climbing
#define CLIMB_PUNCH_Z 7			 // how far to 'punch' client Z axis when climbing

void CBasePlayer::PreThink()
{
	// (UpdatePlayerState and CheckForWires run near the end, past the spectator, dead and train
	// returns, where the original calls them: @0x81416, @0x81450.)

	int buttonsChanged = (m_afButtonLast ^ pev->button); // These buttons have changed this frame

	// Debounced button codes for pressed/released
	// UNDONE: Do we need auto-repeat?
	m_afButtonPressed = buttonsChanged & pev->button;	  // The changed ones still down are "pressed"
	m_afButtonReleased = buttonsChanged & (~pev->button); // The ones not down are "released"

	g_pGameRules->PlayerThink(this);

	if (g_fGameOver)
		return; // intermission or finale

	UTIL_MakeVectors(pev->v_angle); // is this still used?

	ItemPreFrame();
	WaterMove();

	// (The stock SDK sets or clears HIDEHUD_FLASHLIGHT here every frame. The original PreThink
	// @0x808cc never writes m_iHideHUD, so TS sends HideWeapon 0, not the flashlight bit. Nor has it the
	// stock view-entity restore or the node-graph pointer fix-up here.)

	// @0x8095b: the client update runs here, before the TS upkeep and the spectate block below, so the
	// InitHUD frame's StartObserver is followed by that frame's first countdown line.
	// JOHN: checks if new client data (for HUD and view control) needs to be sent to the client
	UpdateClientData();

	CheckTimeBasedDamage();

	CheckSuitUpdate();

	// TS slow-motion time-dilation (@0x80974-0x8099f: right after CheckSuitUpdate, for every player,
	// spectators included). pev->fuser1 is the CURRENT slow factor, eased toward the GoSlow
	// TARGET (m_flSlowMotionGo). It's a stock NETWORKED entvars float, so the client receives it and
	// runs the identical weapon-timer decrement → fire-rate prediction stays in sync. It scales
	// movement (maxspeed), animation (pev->framerate) and the weapon-timer decrement (PostThink).
	// RE scratchpad/SLOWMO_PRESENTATION_RE.md §B: ease = InterpolateSlowMotion, anim = max(fuser1,0.01).
	// The ease starts from whatever fuser1 holds: TSInit and StartObserver set it to 1.0, and nothing
	// here lifts a 0 (the original has no such step before the call @0x80974), so a 0 eases up from
	// 0, with pm_shared freezing the move while it is below 0.1.
	// ease CURRENT (fuser1) → TARGET (m_flSlowMotionGo) — the original's TS_shared.cpp helper.
	InterpolateSlowMotion(gpGlobals->frametime, &pev->fuser1, m_flSlowMotionGo);
	// Base move speed comes from the inventory-slot encumbrance (GetSpeedBySlots: more/heavier weapons →
	// slower). Slow-motion does NOT scale maxspeed — the original slows MOVEMENT by dilating the pm_shared
	// frametime (PM_PlayerMove: frametime *= fuser1), which leaves pev->velocity MAGNITUDE intact, so the
	// side-kick speed>270 gate still passes in slowmo. RE PreThink @0x808cc (no fuser1 on maxspeed) +
	// PM_PlayerMove @0xf08d8. (Scaling maxspeed here dropped |velocity| below 270 and blocked the kick.)
	// cont.89: the encumbrance speed MUST go through SetPlayerSpeed → pfnSetClientMaxspeed. RE-confirmed
	// (movement_prediction.md §cont.89c) the original CLIENT predicts off `clientmaxspeed` (set ONLY by
	// pfnSetClientMaxspeed), NOT cd->maxspeed — so a raw pev->maxspeed write leaves the client predicting
	// at a stale clientmaxspeed → a CONSTANT cl_showerror seam on ground AND air. The original sets speed
	// exclusively via pfnSetClientMaxspeed, and only on loadout events (the 11 SetPlayerSpeed call sites
	// @0x83068: TSInit, PickUpWeapon, DropWeapon, TakeArmor, TakeFlag, DropFlag, SetPlayerTheOne, and the
	// knife/katana/grenade throws and MultiplyClips), never here, so a plugin's maxspeed holds until the
	// next of those.
	// @0x80993-0x8099f: the animation rate is the slow factor, written every frame for every player.
	pev->framerate = GiveSlowMul();
	// @0x809ab: on the ground, the last-on-ground stamp (+0xb88) for every player, spectators and the
	// dead included; the dive window in HandleStunt reads how long ago he left the ground.
	if ((pev->flags & FL_ONGROUND) != 0)
		m_TSStunt.m_flLastStunt = gpGlobals->time;

	// TS melee timer lifecycle (RE PreThink @0x808cc, addendum L2): the deflect windows, the
	// BlockPlayer root timer and the whiff-lunge window are all VALUES counted down by the same
	// slow-scaled delta. @0x809be-0x80b22: the countdowns run here, for every player, spectators
	// included; the root timer's and the run-through window's expiries come after the powerup state
	// below (@0x80cda, @0x80e13).
	float flDecay = GiveSlowMul() * gpGlobals->frametime;
	if ((m_iPowerups & TS_PWUP_DBLFIRE) != 0)
		flDecay *= 1.5f;
	// the run-through window (+0x79c), the root timer (+0x4dc) and the gravity timer (+0x4ec) run
	// down while set, floored at 0.001 (@0x809f4-0x80a90); their expiries come further down.
	if (m_flWhiffLungeTime != 0.0f)
		m_flWhiffLungeTime = V_max(m_flWhiffLungeTime - flDecay, 0.001f);
	if (m_flBlockTime != 0.0f)
		m_flBlockTime = V_max(m_flBlockTime - flDecay, 0.001f);
	if (m_flGravityValue != 0.0f)
		m_flGravityValue = V_max(m_flGravityValue - flDecay, 0.001f);
	// deflect windows (+0x7a8/b0/ac): floor 0
	m_flKungFuBlock = V_max(m_flKungFuBlock - flDecay, 0.0f);
	m_flKatanaBlock = V_max(m_flKatanaBlock - flDecay, 0.0f);
	m_flKnifeBlock = V_max(m_flKnifeBlock - flDecay, 0.0f);
	// disarm pressure (+0x7b4): decays at 5x the delta (RE PreThink +0x237)
	m_flDisarmPressure = V_max(m_flDisarmPressure - flDecay * 5.0f, 0.0f);

	// TS STUNT/ACROBATICS (CTSStunt — dlls/ts_stunt.cpp, RE reference/STUNT.md). Play() runs the
	// CTSStunt state machine; it writes the networked pev->iuser4 stunt flags, and pm_shared's
	// TSHandle() (in BOTH the server and the client predictor) reads them to PREDICT the small
	// dive/prone hull + low view + dive/wall launch velocity — which the DLL alone could never
	// make stick (the predictor recomputed the hull every frame). +alt1 = dive / bullet-time,
	// double-jump into a wall = wall-climb / wall-jump.
	m_TSStunt.Play(); // @0x80b32

	m_TSAnim.UpdateTimers(this); // TS: decay the gait aim/leg/grenade hold timers (@0x80b42)

	// @0x80b4a-0x80ba3: iuser4 bit 0x400000 says the view offset is zero; set or cleared every frame.
	if (pev->view_ofs == g_vecZero)
		pev->iuser4 |= TSSTUNT_NOVIEWOFS;
	else
		pev->iuser4 &= ~TSSTUNT_NOVIEWOFS;

	// @0x80bb1: the powerup state and the slow-motion flow run for every player, before the
	// spectator block below, as in the original: a spectator's powerup is dropped or ends there too.
	UpdatePowerUpState(); // TS: expire/apply the active timed powerup each frame (sets the slow triple)
	SlowMotionFlow();     // TS: every 0.5s, slow self + LOS-visible enemies while a slow powerup is active

	// TS health-powerup over-heal decay (@0x80bc1, before the spectate block, for every player,
	// player+0x73c = m_flHealthRegen): the health powerup boosts health past max_health; while
	// m_flHealthRegen is armed it bleeds 1 HP/sec back down until health reaches max_health, then
	// disarms. (Set by the health powerup grant.)
	// The One instead regenerates (@0x80bd8-0x80c7d): each tick adds 0.16 health for every other
	// player Think last counted, up to 150, and the ticks come faster with more of them, every
	// 1 / max(0.32 x others, 1) seconds. His clock never disarms.
	if (m_flHealthRegen != 0.0f)
	{
		if (m_bTheOne)
		{
			if (m_flHealthRegen < gpGlobals->time)
			{
				const float others = TSGameRules ? (float)TSGameRules->m_iTheOneOthers : 0.0f;
				m_flHealthRegen = gpGlobals->time + 1.0f / V_max(others * 0.32f, 1.0f);
				pev->health = V_min(others * 0.16f + pev->health, 150.0f);
			}
		}
		else if (pev->health <= pev->max_health)
		{
			m_flHealthRegen = 0.0f; // back to max — stop the decay
		}
		else if (m_flHealthRegen < gpGlobals->time)
		{
			m_flHealthRegen = gpGlobals->time + 1.0f; // next tick in 1s
			pev->health -= 1.0f;
		}
	}

	// BlockPlayer root timer (+0x4dc) expiry (@0x80cda): only with the STOP bit set (@0x80cf6), clear
	// it + zero. Without the bit the timer waits at its 0.001 floor.
	if (m_flBlockTime != 0.0f && (pev->iuser4 & TSSTUNT_STOP) != 0 && m_flBlockTime < 0.01f)
	{
		pev->iuser4 &= ~TSSTUNT_STOP;
		m_flBlockTime = 0.0f;
	}

	// gravity timer (+0x4ec, SetPlayerGravity) expiry (@0x80d24-0x80d55): normal gravity back
	if (m_flGravityValue != 0.0f && m_flGravityValue < 0.01f)
	{
		pev->gravity = 1.0f;
		m_flGravityValue = 0.0f;
	}

	// @0x80d66: a living player's only.
	if (IsAlive())
	{
		// TS dynamic-accuracy recovery (@0x80d77): the spread accumulator decays back toward 0 by the
		// timers' delta x 0.1 (the slow factor x frametime, x1.5 under double fire rate), so accuracy
		// returns when you stop spraying. Growth is in CTSGun::ScaleSpread on each shot.
		if (m_flSpreadAccum > 0.0f)
		{
			m_flSpreadAccum -= flDecay * 0.1f;
			if (m_flSpreadAccum < 0.0f)
				m_flSpreadAccum = 0.0f;
		}

		// TS melee-stamina regen (@0x80db7, dossier-melee addendum L1): the bar refills at 15/s in
		// slow-scaled game time (x1.5 under the double-fire powerup), clamped 100. The regen is
		// SILENT — KFuPower is only sent by UpdateMeleeBar (drains); the client tracks the refill via
		// the WeaponInfo melee bar byte.
		if (m_flMeleeBar < 100.0f)
		{
			// the original adds the raw (unscaled) delta on a negative-bar sentinel; kept verbatim
			m_flMeleeBar += (m_flMeleeBar >= -0.001f) ? flDecay * 15.0f : flDecay;
			if (m_flMeleeBar >= 100.0f)
				m_flMeleeBar = 100.0f;
		}
	}

	// run-through window (+0x79c) expiry (@0x80e13): <0.01, or force==0 (the halving in
	// CheckForBreakables collapses it), zeroes the triple. The window's CONSUMER is
	// CTSStunt::CheckForBreakables (the run-through melee resolver, cont.96) — the fields are
	// NOT an attacker velocity lunge (@0x808cc + dossier cont96 §2c).
	if (m_flWhiffLungeTime != 0.0f)
	{
		if (m_flWhiffLungeTime < 0.01f || m_flWhiffLungeForce == 0.0f)
		{
			m_flWhiffLungeTime = 0.0f;
			m_flWhiffLungeForce = 0.0f;
			m_iWhiffLungeType = 0;
		}
		// @0x80e65: EVERY frame the window is live (not only on expiry) — you cannot hold a
		// kung-fu block mid-window.
		m_flKungFuBlock = 0.0f;
	}

	// @0x80e73: an observer (iuser1 set: the spectate wait after a death or a join, or a team change
	// while alive) runs the countdown lines and the observer controls and nothing of the in-play
	// think. The 1 Hz HUD-line throttles run off +0x754. No ScoreInfo here (the WAITING status rides
	// the award byte).
	if (pev->iuser1 != 0)
	{
		// Fire-to-play is the CLIENT's "respawn" console command (the original mechanism —
		// the ClientCommand "respawn" branch gates it); the server watches no buttons here.

		// @0x80e83-0x80fab (PreThink iuser1 block): while the gate (+0x770) is set and still ahead, once a
		// second on the +0x754 clock, the seconds left, truncated (the fistp with the rounding set to
		// chop) after a cap at 30; when that truncates to 0, in the last second before the gate,
		// "Press Fire To Play!" (hold 30). Nothing is painted once the gate has passed.
		if (m_flTSRespawnTime > 0.0f && m_flTSRespawnTime > gpGlobals->time && m_flSpecMsgTime < gpGlobals->time)
		{
			m_flSpecMsgTime = gpGlobals->time + 1.0f;
			// in double, exact as the original's x87 difference is: a float difference can round up to
			// the next whole second (5 - 2^-22 to 5.0) and show a number the original does not
			const double rem = (double)m_flTSRespawnTime - (double)gpGlobals->time;
			if ((int)rem <= 0)
				TSHudMessageOne(0, 0, 0, 16, 30, "Press Fire To Play!", pev);
			else
				TSHudMessageOne(0, 0, 0, 16, 20, UTIL_VarArgs("%i", (int)(rem > 30.0 ? 30.0 : rem)), pev);
		}
		// @0x80fb3: the LMS re-entry HOLD, armed by SetPlayerRespawnTime with min(roundLeft,10)s. While it
		// runs, a 1 Hz line on the same +0x754 clock (so it is skipped in a second the countdown line
		// took), its seconds truncated (@0x81063, the fistp with the rounding set to chop). Letting it
		// EXPIRE without re-entering locks you out for the round (@0x80fdf): window 0, gate -4.0
		// (wait-next-round sentinel), award 1 (out-of-round), the red wait line, and a deads recount
		// (a lockout can end the round).
		if (m_flRespawnHoldTime != 0.0f)
		{
			if (gpGlobals->time <= m_flRespawnHoldTime)
			{
				if (m_flSpecMsgTime < gpGlobals->time)
				{
					m_flSpecMsgTime = gpGlobals->time + 1.0f;
					// in double, exact as the original's x87 difference is (see the countdown above)
					const double rem = (double)m_flRespawnHoldTime - (double)gpGlobals->time;
					TSHudMessageOne(0, 0, 0, 16, 20,
						UTIL_VarArgs("%i seconds left for respawn, press Fire to Play!", (int)rem), pev);
				}
			}
			else
			{
				m_flRespawnHoldTime = 0;
				m_flTSRespawnTime = -1.0f; // @0x80fe9
				m_iAward = 1;
				TSHudMessageOne(195, 45, 45, 16, 20, "You have to wait until the next round.", pev);
				if (TSGameRules)
					TSGameRules->CallCheckDeads();
			}
		}
		// (UpdateClientData already ran this frame, near the top of PreThink as in the original, so a
		// spectator gets his HUD updates: cont.98's slow motion and WAITING fix.)
		// @0x810c3, @0x810cb, @0x810d3: the block ends with the observer controls, the dead-target
		// check and the impulse cleared, as the original's does.
		Observer_HandleButtons();
		Observer_Idle();
		pev->impulse = 0;
		return;
	}

	// TS run-through / breakable-barge sweep (orig CheckForBreakables @0x8bd9c, per-tick in
	// PreThink, @0x810ee: first thing after the spectate block): diving fast into a world breakable
	// shatters it; with the whiff window live (armed by a missed melee swing) it delivers the full
	// close-combat package to PLAYERS — disarm + >30 knockback (cont.96; dossier melee_knockback_disarm.md). The former duplicate
	// TSCheckRunThrough (ts_gun.cpp) is deleted — the original has ONE function.
	m_TSStunt.CheckForBreakables();

	if (pev->deadflag >= DEAD_DYING)
	{
		PlayerDeathThink();
		return;
	}

	// So the correct flags get sent to client asap.
	//
	if ((m_afPhysicsFlags & PFLAG_ONTRAIN) != 0)
		pev->flags |= FL_ONTRAIN;
	else
		pev->flags &= ~FL_ONTRAIN;

	// Train speed control
	if ((m_afPhysicsFlags & PFLAG_ONTRAIN) != 0)
	{
		CBaseEntity* pTrain = CBaseEntity::Instance(pev->groundentity);
		float vel;

		if (!pTrain)
		{
			TraceResult trainTrace;
			// Maybe this is on the other side of a level transition
			UTIL_TraceLine(pev->origin, pev->origin + Vector(0, 0, -38), ignore_monsters, ENT(pev), &trainTrace);

			// HACKHACK - Just look for the func_tracktrain classname
			if (trainTrace.flFraction != 1.0 && trainTrace.pHit)
				pTrain = CBaseEntity::Instance(trainTrace.pHit);


			if (!pTrain || (pTrain->ObjectCaps() & FCAP_DIRECTIONAL_USE) == 0 || !pTrain->OnControls(pev))
			{
				//ALERT( at_error, "In train mode with no train!\n" );
				m_afPhysicsFlags &= ~PFLAG_ONTRAIN;
				m_iTrain = TRAIN_NEW | TRAIN_OFF;
				if (pTrain->Classify() == CLASS_VEHICLE)
					((CFuncVehicle*)pTrain)->m_pDriver = NULL;
				return;
			}
		}
		else if (!FBitSet(pev->flags, FL_ONGROUND) || FBitSet(pTrain->pev->spawnflags, SF_TRACKTRAIN_NOCONTROL) || ((pev->button & (IN_MOVELEFT | IN_MOVERIGHT)) != 0 && pTrain->Classify() != CLASS_VEHICLE))
		{
			// Turn off the train if you jump, strafe, or the train controls go dead
			// and it isn't a func_vehicle
			m_afPhysicsFlags &= ~PFLAG_ONTRAIN;
			m_iTrain = TRAIN_NEW | TRAIN_OFF;
			return;
		}

		pev->velocity = g_vecZero;
		vel = 0;
		if (pTrain->Classify() == CLASS_VEHICLE)
		{
			if (pev->button & IN_FORWARD)
			{
				vel = 1;
				pTrain->Use(this, this, USE_SET, (float)vel);
			}
			if (pev->button & IN_BACK)
			{
				vel = -1;
				pTrain->Use(this, this, USE_SET, (float)vel);
			}
			if (pev->button & IN_MOVELEFT)
			{
				vel = 20;
				pTrain->Use(this, this, USE_SET, (float)vel);
			}
			if (pev->button & IN_MOVERIGHT)
			{
				vel = 30;
				pTrain->Use(this, this, USE_SET, (float)vel);
			}
		}
		else
		{
			if ((m_afButtonPressed & IN_FORWARD) != 0)
			{
				vel = 1;
				pTrain->Use(this, this, USE_SET, (float)vel);
			}
			else if ((m_afButtonPressed & IN_BACK) != 0)
			{
				vel = -1;
				pTrain->Use(this, this, USE_SET, (float)vel);
			}
		}

		if (0 != vel)
		{
			m_iTrain = TrainSpeed(pTrain->pev->speed, pTrain->pev->impulse);
			m_iTrain |= TRAIN_ACTIVE | TRAIN_NEW;
		}
	}
	else if ((m_iTrain & TRAIN_ACTIVE) != 0)
		m_iTrain = TRAIN_NEW; // turn off train

	if ((pev->button & IN_JUMP) != 0)
	{
		// If on a ladder, jump off the ladder
		// else Jump
		Jump();
	}


	// If trying to duck, already ducked, or in the process of ducking
	if ((pev->button & IN_DUCK) != 0 || FBitSet(pev->flags, FL_DUCKING) || (m_afPhysicsFlags & PFLAG_DUCKING) != 0)
		Duck();

	if (!FBitSet(pev->flags, FL_ONGROUND))
	{
		m_flFallVelocity = -pev->velocity.z;
	}

	// @0x81416: refresh the networked pose state (TSState). Only a live player in play gets here,
	// so a spectator or a dead player keeps the pose he had.
	UpdatePlayerState();

	// @0x8141b (right after UpdatePlayerState): on mode-3 (objective team) maps, refresh the
	// nearby-bomb wire triple every 0.5 s (player+0x794 throttle) — the Wires HUD source.
	if (TSGameRules && TSGameRules->m_iGameMode == 3 && m_flNextWiresCheck < gpGlobals->time)
	{
		m_flNextWiresCheck = gpGlobals->time + 0.5f;
		CheckForWires();
	}

	// StudioFrameAdvance( );//!!!HACKHACK!!! Can't be hit by traceline when not animating?

	// Clear out ladder pointer
	m_hEnemy = NULL;

	if ((m_afPhysicsFlags & PFLAG_ONBARNACLE) != 0)
	{
		pev->velocity = g_vecZero;
	}

	// @0x81458 (TSInit's counterpart): once the spawn-protection deadline passes, drop the
	// translucent shimmer TSInit applied. Here, past the spectator, observer and dead returns, as
	// in the original: a player in spectate keeps the shimmer until he spawns.
	if (m_flSpawnProtectTime < gpGlobals->time && pev->rendermode != kRenderNormal)
	{
		pev->rendermode = kRenderNormal;
		pev->renderfx = kRenderFxNone;
		pev->renderamt = 0;
		pev->rendercolor = Vector(0, 0, 0);
	}

	// @0x814c4-0x81575: a kick set for a time (player+0x7cc) falls due: he is told, everyone is
	// told, and the server kicks him by user id. Nothing in the game sets the time.
	if (m_flKickTime != 0 && m_flKickTime < gpGlobals->time)
	{
		ClientPrint(pev, HUD_PRINTCONSOLE, "You have been kicked for some reason...\n");
		UTIL_ClientPrintAll(HUD_PRINTNOTIFY, UTIL_VarArgs("%s kicked.\n", STRING(pev->netname)));
		SERVER_COMMAND(UTIL_VarArgs("kick #%i\n", GETPLAYERUSERID(edict())));
		m_flKickTime = 0;
	}

	// @0x815b6: the suicide ClientKill armed (player+0x84c) falls due: health 0 and Killed with
	// the player as the attacker. A spectator, a dead player and a train-release frame return
	// above and never get here, so the timer waits for Spawn's TSInit to clear it.
	if (m_fNextSuicideTime > 0.001f && m_fNextSuicideTime < gpGlobals->time)
	{
		m_fNextSuicideTime = 0;
		pev->health = 0;
		Killed(pev, GIB_NEVER);
	}
}
/* Time based Damage works as follows: 
	1) There are several types of timebased damage:

		#define DMG_PARALYZE		(1 << 14)	// slows affected creature down
		#define DMG_NERVEGAS		(1 << 15)	// nerve toxins, very bad
		#define DMG_POISON			(1 << 16)	// blood poisioning
		#define DMG_RADIATION		(1 << 17)	// radiation exposure
		#define DMG_DROWNRECOVER	(1 << 18)	// drown recovery
		#define DMG_ACID			(1 << 19)	// toxic chemicals or acid burns
		#define DMG_SLOWBURN		(1 << 20)	// in an oven
		#define DMG_SLOWFREEZE		(1 << 21)	// in a subzero freezer

	2) A new hit inflicting tbd restarts the tbd counter - each monster has an 8bit counter,
		per damage type. The counter is decremented every second, so the maximum time 
		an effect will last is 255/60 = 4.25 minutes.  Of course, staying within the radius
		of a damaging effect like fire, nervegas, radiation will continually reset the counter to max.

	3) Every second that a tbd counter is running, the player takes damage.  The damage
		is determined by the type of tdb.  
			Paralyze		- 1/2 movement rate, 30 second duration.
			Nervegas		- 5 points per second, 16 second duration = 80 points max dose.
			Poison			- 2 points per second, 25 second duration = 50 points max dose.
			Radiation		- 1 point per second, 50 second duration = 50 points max dose.
			Drown			- 5 points per second, 2 second duration.
			Acid/Chemical	- 5 points per second, 10 second duration = 50 points max.
			Burn			- 10 points per second, 2 second duration.
			Freeze			- 3 points per second, 10 second duration = 30 points max.

	4) Certain actions or countermeasures counteract the damaging effects of tbds:

		Armor/Heater/Cooler - Chemical(acid),burn, freeze all do damage to armor power, then to body
							- recharged by suit recharger
		Air In Lungs		- drowning damage is done to air in lungs first, then to body
							- recharged by poking head out of water
							- 10 seconds if swiming fast
		Air In SCUBA		- drowning damage is done to air in tanks first, then to body
							- 2 minutes in tanks. Need new tank once empty.
		Radiation Syringe	- Each syringe full provides protection vs one radiation dosage
		Antitoxin Syringe	- Each syringe full provides protection vs one poisoning (nervegas or poison).
		Health kit			- Immediate stop to acid/chemical, fire or freeze damage.
		Radiation Shower	- Immediate stop to radiation damage, acid/chemical or fire damage.
		
	
*/

// If player is taking time based damage, continue doing damage to player -
// this simulates the effect of being poisoned, gassed, dosed with radiation etc -
// anything that continues to do damage even after the initial contact stops.
// Update all time based damage counters, and shut off any that are done.

// The m_bitsDamageType bit MUST be set if any damage is to be taken.
// This routine will detect the initial on value of the m_bitsDamageType
// and init the appropriate counter.  Only processes damage every second.

//#define PARALYZE_DURATION	30		// number of 2 second intervals to take damage
//#define PARALYZE_DAMAGE		0.0		// damage to take each 2 second interval

//#define NERVEGAS_DURATION	16
//#define NERVEGAS_DAMAGE		5.0

//#define POISON_DURATION		25
//#define POISON_DAMAGE		2.0

//#define RADIATION_DURATION	50
//#define RADIATION_DAMAGE	1.0

//#define ACID_DURATION		10
//#define ACID_DAMAGE			5.0

//#define SLOWBURN_DURATION	2
//#define SLOWBURN_DAMAGE		1.0

//#define SLOWFREEZE_DURATION	1.0
//#define SLOWFREEZE_DAMAGE	3.0

/* */


void CBasePlayer::CheckTimeBasedDamage()
{
	int i;
	byte bDuration = 0;

	static float gtbdPrev = 0.0;

	if ((m_bitsDamageType & DMG_TIMEBASED) == 0)
		return;

	// only check for time based damage approx. every 2 seconds
	if (fabs(gpGlobals->time - m_tbdPrev) < 2.0)
		return;

	m_tbdPrev = gpGlobals->time;

	for (i = 0; i < CDMG_TIMEBASED; i++)
	{
		// make sure bit is set for damage type
		if ((m_bitsDamageType & (DMG_PARALYZE << i)) != 0)
		{
			switch (i)
			{
			case itbd_Paralyze:
				// UNDONE - flag movement as half-speed
				bDuration = PARALYZE_DURATION;
				break;
			case itbd_NerveGas:
				//				TakeDamage(pev, pev, NERVEGAS_DAMAGE, DMG_GENERIC);
				bDuration = NERVEGAS_DURATION;
				break;
			case itbd_Poison:
				TakeDamage(pev, pev, POISON_DAMAGE, DMG_GENERIC);
				bDuration = POISON_DURATION;
				break;
			case itbd_Radiation:
				//				TakeDamage(pev, pev, RADIATION_DAMAGE, DMG_GENERIC);
				bDuration = RADIATION_DURATION;
				break;
			case itbd_DrownRecover:
				// NOTE: this hack is actually used to RESTORE health
				// after the player has been drowning and finally takes a breath
				if (m_idrowndmg > m_idrownrestored)
				{
					int idif = V_min(m_idrowndmg - m_idrownrestored, 10);

					TakeHealth(idif, DMG_GENERIC);
					m_idrownrestored += idif;
				}
				bDuration = 4; // get up to 5*10 = 50 points back
				break;
			case itbd_Acid:
				//				TakeDamage(pev, pev, ACID_DAMAGE, DMG_GENERIC);
				bDuration = ACID_DURATION;
				break;
			case itbd_SlowBurn:
				//				TakeDamage(pev, pev, SLOWBURN_DAMAGE, DMG_GENERIC);
				bDuration = SLOWBURN_DURATION;
				break;
			case itbd_SlowFreeze:
				//				TakeDamage(pev, pev, SLOWFREEZE_DAMAGE, DMG_GENERIC);
				bDuration = SLOWFREEZE_DURATION;
				break;
			default:
				bDuration = 0;
			}

			if (0 != m_rgbTimeBasedDamage[i])
			{
				// use up an antitoxin on poison or nervegas after a few seconds of damage
				if (((i == itbd_NerveGas) && (m_rgbTimeBasedDamage[i] < NERVEGAS_DURATION)) ||
					((i == itbd_Poison) && (m_rgbTimeBasedDamage[i] < POISON_DURATION)))
				{
					if (0 != m_rgItems[ITEM_ANTIDOTE])
					{
						m_rgbTimeBasedDamage[i] = 0;
						m_rgItems[ITEM_ANTIDOTE]--;
						SetSuitUpdate("!HEV_HEAL4", false, SUIT_REPEAT_OK);
					}
				}


				// decrement damage duration, detect when done.
				if (0 == m_rgbTimeBasedDamage[i] || --m_rgbTimeBasedDamage[i] == 0)
				{
					m_rgbTimeBasedDamage[i] = 0;
					// if we're done, clear damage bits
					m_bitsDamageType &= ~(DMG_PARALYZE << i);
				}
			}
			else
				// first time taking this damage type - init damage duration
				m_rgbTimeBasedDamage[i] = bDuration;
		}
	}
}

/*
THE POWER SUIT

The Suit provides 3 main functions: Protection, Notification and Augmentation. 
Some functions are automatic, some require power. 
The player gets the suit shortly after getting off the train in C1A0 and it stays
with him for the entire game.

Protection

	Heat/Cold
		When the player enters a hot/cold area, the heating/cooling indicator on the suit 
		will come on and the battery will drain while the player stays in the area. 
		After the battery is dead, the player starts to take damage. 
		This feature is built into the suit and is automatically engaged.
	Radiation Syringe
		This will cause the player to be immune from the effects of radiation for N seconds. Single use item.
	Anti-Toxin Syringe
		This will cure the player from being poisoned. Single use item.
	Health
		Small (1st aid kits, food, etc.)
		Large (boxes on walls)
	Armor
		The armor works using energy to create a protective field that deflects a
		percentage of damage projectile and explosive attacks. After the armor has been deployed,
		it will attempt to recharge itself to full capacity with the energy reserves from the battery.
		It takes the armor N seconds to fully charge. 

Notification (via the HUD)

x	Health
x	Ammo  
x	Automatic Health Care
		Notifies the player when automatic healing has been engaged. 
x	Geiger counter
		Classic Geiger counter sound and status bar at top of HUD 
		alerts player to dangerous levels of radiation. This is not visible when radiation levels are normal.
x	Poison
	Armor
		Displays the current level of armor. 

Augmentation 

	Reanimation (w/adrenaline)
		Causes the player to come back to life after he has been dead for 3 seconds. 
		Will not work if player was gibbed. Single use.
	Long Jump
		Used by hitting the ??? key(s). Caused the player to further than normal.
	SCUBA	
		Used automatically after picked up and after player enters the water. 
		Works for N seconds. Single use.	
	
Things powered by the battery

	Armor		
		Uses N watts for every M units of damage.
	Heat/Cool	
		Uses N watts for every second in hot/cold area.
	Long Jump	
		Uses N watts for every jump.
	Alien Cloak	
		Uses N watts for each use. Each use lasts M seconds.
	Alien Shield	
		Augments armor. Reduces Armor drain by one half
 
*/

// if in range of radiation source, ping geiger counter

#define GEIGERDELAY 0.25

void CBasePlayer::UpdateGeigerCounter()
{
	byte range;

	// delay per update ie: don't flood net with these msgs
	if (gpGlobals->time < m_flgeigerDelay)
		return;

	m_flgeigerDelay = gpGlobals->time + GEIGERDELAY;

	// send range to radition source to client

	range = (byte)(m_flgeigerRange / 4);

	if (range != m_igeigerRangePrev)
	{
		m_igeigerRangePrev = range;

		MESSAGE_BEGIN(MSG_ONE, gmsgGeigerRange, NULL, pev);
		WRITE_BYTE(range);
		MESSAGE_END();
	}

	// reset counter and semaphore
	if (!RANDOM_LONG(0, 3))
		m_flgeigerRange = 1000;
}

/*
================
CheckSuitUpdate

Play suit update if it's time
================
*/

#define SUITUPDATETIME 3.5
#define SUITFIRSTUPDATETIME 0.1

void CBasePlayer::CheckSuitUpdate()
{
	int i;
	int isentence = 0;
	int isearch = m_iSuitPlayNext;

	// Ignore suit updates if no suit
	if (!HasSuit())
		return;

	// if in range of radiation source, ping geiger counter
	UpdateGeigerCounter();

	if (g_pGameRules->IsMultiplayer())
	{
		// don't bother updating HEV voice in multiplayer.
		return;
	}

	if (gpGlobals->time >= m_flSuitUpdate && m_flSuitUpdate > 0)
	{
		// play a sentence off of the end of the queue
		for (i = 0; i < CSUITPLAYLIST; i++)
		{
			isentence = m_rgSuitPlayList[isearch];
			if (0 != isentence)
				break;

			if (++isearch == CSUITPLAYLIST)
				isearch = 0;
		}

		if (0 != isentence)
		{
			m_rgSuitPlayList[isearch] = 0;
			if (isentence > 0)
			{
				// play sentence number

				char sentence[CBSENTENCENAME_MAX + 1];
				strcpy(sentence, "!");
				strcat(sentence, gszallsentencenames[isentence]);
				EMIT_SOUND_SUIT(ENT(pev), sentence);
			}
			else
			{
				// play sentence group
				EMIT_GROUPID_SUIT(ENT(pev), -isentence);
			}
			m_flSuitUpdate = gpGlobals->time + SUITUPDATETIME;
		}
		else
			// queue is empty, don't check
			m_flSuitUpdate = 0;
	}
}

// add sentence to suit playlist queue. if fgroup is true, then
// name is a sentence group (HEV_AA), otherwise name is a specific
// sentence name ie: !HEV_AA0.  If iNoRepeat is specified in
// seconds, then we won't repeat playback of this word or sentence
// for at least that number of seconds.

void CBasePlayer::SetSuitUpdate(const char* name, bool fgroup, int iNoRepeatTime)
{
	int i;
	int isentence;
	int iempty = -1;


	// Ignore suit updates if no suit
	if (!HasSuit())
		return;

	if (g_pGameRules->IsMultiplayer())
	{
		// due to static channel design, etc. We don't play HEV sounds in multiplayer right now.
		return;
	}

	// if name == NULL, then clear out the queue

	if (!name)
	{
		for (i = 0; i < CSUITPLAYLIST; i++)
			m_rgSuitPlayList[i] = 0;
		return;
	}
	// get sentence or group number
	if (!fgroup)
	{
		isentence = SENTENCEG_Lookup(name, NULL);
		if (isentence < 0)
			return;
	}
	else
		// mark group number as negative
		isentence = -SENTENCEG_GetIndex(name);

	// check norepeat list - this list lets us cancel
	// the playback of words or sentences that have already
	// been played within a certain time.

	for (i = 0; i < CSUITNOREPEAT; i++)
	{
		if (isentence == m_rgiSuitNoRepeat[i])
		{
			// this sentence or group is already in
			// the norepeat list

			if (m_rgflSuitNoRepeatTime[i] < gpGlobals->time)
			{
				// norepeat time has expired, clear it out
				m_rgiSuitNoRepeat[i] = 0;
				m_rgflSuitNoRepeatTime[i] = 0.0;
				iempty = i;
				break;
			}
			else
			{
				// don't play, still marked as norepeat
				return;
			}
		}
		// keep track of empty slot
		if (0 == m_rgiSuitNoRepeat[i])
			iempty = i;
	}

	// sentence is not in norepeat list, save if norepeat time was given

	if (0 != iNoRepeatTime)
	{
		if (iempty < 0)
			iempty = RANDOM_LONG(0, CSUITNOREPEAT - 1); // pick random slot to take over
		m_rgiSuitNoRepeat[iempty] = isentence;
		m_rgflSuitNoRepeatTime[iempty] = iNoRepeatTime + gpGlobals->time;
	}

	// find empty spot in queue, or overwrite last spot

	m_rgSuitPlayList[m_iSuitPlayNext++] = isentence;
	if (m_iSuitPlayNext == CSUITPLAYLIST)
		m_iSuitPlayNext = 0;

	if (m_flSuitUpdate <= gpGlobals->time)
	{
		if (m_flSuitUpdate == 0)
			// play queue is empty, don't delay too long before playback
			m_flSuitUpdate = gpGlobals->time + SUITFIRSTUPDATETIME;
		else
			m_flSuitUpdate = gpGlobals->time + SUITUPDATETIME;
	}
}

/*
================
CheckPowerups

Check for turning off powerups

GLOBALS ASSUMED SET:  g_ulModelIndexPlayer
================
*/
static void
CheckPowerups(entvars_t* pev)
{
	if (pev->health <= 0)
		return;

	pev->modelindex = g_ulModelIndexPlayer; // don't use eyes
}


//=========================================================
// UpdatePlayerSound - updates the position of the player's
// reserved sound slot in the sound list.
//=========================================================
void CBasePlayer::UpdatePlayerSound()
{
	int iBodyVolume;
	int iVolume;
	CSound* pSound;

	pSound = CSoundEnt::SoundPointerForIndex(CSoundEnt::ClientSoundIndex(edict()));

	if (!pSound)
	{
		ALERT(at_console, "Client lost reserved sound!\n");
		return;
	}

	pSound->m_iType = bits_SOUND_NONE;

	// now calculate the best target volume for the sound. If the player's weapon
	// is louder than his body/movement, use the weapon volume, else, use the body volume.

	if (FBitSet(pev->flags, FL_ONGROUND))
	{
		iBodyVolume = pev->velocity.Length();

		// clamp the noise that can be made by the body, in case a push trigger,
		// weapon recoil, or anything shoves the player abnormally fast.
		if (iBodyVolume > 512)
		{
			iBodyVolume = 512;
		}
	}
	else
	{
		iBodyVolume = 0;
	}

	if ((pev->button & IN_JUMP) != 0)
	{
		iBodyVolume += 100;
	}

	// convert player move speed and actions into sound audible by monsters.
	if (m_iWeaponVolume > iBodyVolume)
	{
		m_iTargetVolume = m_iWeaponVolume;

		// OR in the bits for COMBAT sound if the weapon is being louder than the player.
		pSound->m_iType |= bits_SOUND_COMBAT;
	}
	else
	{
		m_iTargetVolume = iBodyVolume;
	}

	// decay weapon volume over time so bits_SOUND_COMBAT stays set for a while
	m_iWeaponVolume -= 250 * gpGlobals->frametime;
	if (m_iWeaponVolume < 0)
	{
		iVolume = 0;
	}


	// if target volume is greater than the player sound's current volume, we paste the new volume in
	// immediately. If target is less than the current volume, current volume is not set immediately to the
	// lower volume, rather works itself towards target volume over time. This gives monsters a much better chance
	// to hear a sound, especially if they don't listen every frame.
	iVolume = pSound->m_iVolume;

	if (m_iTargetVolume > iVolume)
	{
		iVolume = m_iTargetVolume;
	}
	else if (iVolume > m_iTargetVolume)
	{
		iVolume -= 250 * gpGlobals->frametime;

		if (iVolume < m_iTargetVolume)
		{
			iVolume = 0;
		}
	}

	if (m_fNoPlayerSound)
	{
		// debugging flag, lets players move around and shoot without monsters hearing.
		iVolume = 0;
	}

	if (gpGlobals->time > m_flStopExtraSoundTime)
	{
		// since the extra sound that a weapon emits only lasts for one client frame, we keep that sound around for a server frame or two
		// after actual emission to make sure it gets heard.
		m_iExtraSoundTypes = 0;
	}

	if (pSound)
	{
		pSound->m_vecOrigin = pev->origin;
		pSound->m_iType |= (bits_SOUND_PLAYER | m_iExtraSoundTypes);
		pSound->m_iVolume = iVolume;
	}

	// keep track of virtual muzzle flash
	m_iWeaponFlash -= 256 * gpGlobals->frametime;
	if (m_iWeaponFlash < 0)
		m_iWeaponFlash = 0;

	//UTIL_MakeVectors ( pev->angles );
	//gpGlobals->v_forward.z = 0;

	// Below are a couple of useful little bits that make it easier to determine just how much noise the
	// player is making.
	// UTIL_ParticleEffect ( pev->origin + gpGlobals->v_forward * iVolume, g_vecZero, 255, 25 );
	//ALERT ( at_console, "%d/%d\n", iVolume, m_iTargetVolume );
}


void CBasePlayer::PostThink()
{
	if (g_fGameOver)
		goto pt_end; // intermission or finale

	if (!IsAlive())
		goto pt_end;

	// Handle Tank controlling
	if (m_pTank != NULL)
	{ // if they've moved too far from the gun,  or selected a weapon, unuse the gun
		if (m_pTank->OnControls(pev) && 0 == pev->weaponmodel)
		{
			m_pTank->Use(this, this, USE_SET, 2); // try fire the gun
		}
		else
		{ // they've moved off the platform
			m_pTank->Use(this, this, USE_OFF, 0);
			m_pTank = NULL;
		}
	}

	// do weapon stuff
	ItemPostFrame();

	// check to see if player landed hard enough to make a sound
	// falling farther than half of the maximum safe distance, but not as far a max safe distance will
	// play a bootscrape sound, and no damage will be inflicted. Fallling a distance shorter than half
	// of maximum safe distance will make no sound. Falling farther than max safe distance will play a
	// fallpain sound, and damage will be inflicted based on how far the player fell

	if ((FBitSet(pev->flags, FL_ONGROUND)) && (pev->health > 0) && m_flFallVelocity >= PLAYER_FALL_PUNCH_THRESHHOLD)
	{
		// ALERT ( at_console, "%f\n", m_flFallVelocity );

		if (pev->watertype == CONTENT_WATER)
		{
			// Did he hit the world or a non-moving entity?
			// BUG - this happens all the time in water, especially when
			// BUG - water has current force
			// if ( !pev->groundentity || VARS(pev->groundentity)->velocity.z == 0 )
			// EMIT_SOUND(ENT(pev), CHAN_BODY, "player/pl_wade1.wav", 1, ATTN_NORM);
		}
		else if (m_flFallVelocity > PLAYER_MAX_SAFE_FALL_SPEED)
		{ // after this point, we start doing damage

			float flFallDamage = g_pGameRules->FlPlayerFallDamage(this);

			if (flFallDamage > pev->health)
			{ //splat
				// note: play on item channel because we play footstep landing on body channel
				EMIT_SOUND(ENT(pev), CHAN_ITEM, "common/bodysplat.wav", 1, ATTN_NORM);
			}

			if (flFallDamage > 0)
			{
				TakeDamage(CWorld::World->pev, CWorld::World->pev, flFallDamage, DMG_FALL);
				pev->punchangle.x = 0;
			}
		}

		if (IsAlive())
		{
			m_TSAnim.Animate(this, TSA_RUN);
		}
	}

	if (FBitSet(pev->flags, FL_ONGROUND))
	{
		// @0x8186f: a player on the ground gets normal gravity back every frame, so a lowered gravity
		// lasts only until he lands (the superjump powerup sets 0.15 again in each PreThink).
		pev->gravity = 1.0;

		if (m_flFallVelocity > 64 && !g_pGameRules->IsMultiplayer())
		{
			CSoundEnt::InsertSound(bits_SOUND_PLAYER, pev->origin, m_flFallVelocity, 0.2);
			// ALERT( at_console, "fall %f\n", m_flFallVelocity );
		}
		m_flFallVelocity = 0;

		// TS SUPER-JUMP LANDING (@0x818ec-0x81a31: in the landing block, after the fall velocity is
		// cleared, a living player's only). The jump itself is the "mj" high jump in PM_Jump
		// (velocity.z=400, one-shot, DUCK+JUMP) which arms iuser4 |= WHEEL_TRIG -> CTSStunt::Play plays the
		// morfy_jump(_guns) flight pose + airwheel and sets WHEEL_DONE. When that flight touches down, screen-
		// shake + either a GoRoll (DUCK held on landing) or the superjump_land plant (view low + PRONE|STOP +
		// roll 0.75 + the superjump_land pose). No sustained lift, no fuel — the arc is pure jump physics, so
		// hitting a ceiling just drops you.
		if ((pev->iuser4 & TSSTUNT_WHEEL_DONE) != 0)
		{
			pev->iuser4 &= ~TSSTUNT_WHEEL_DONE;
			UTIL_ScreenShake(pev->origin, 20.0f, 30.0f, 0.3f, 512.0f);
			bool didRoll = false;
			if (pev->button & IN_DUCK) // duck on landing -> a landing roll instead of the plant
			{
				// @0x81940: the roll's double-tap window is armed first, so GoRoll on the ground takes
				// the landing as the second tap
				m_TSStunt.m_flRollNextTime = gpGlobals->time + 0.3; // double 0.3, as the original
				if (m_TSStunt.GoRoll(0))
				{
					didRoll = true;
					m_flNextAttack = UTIL_WeaponTimeBase() + 0.75f;
				}
			}
			if (!didRoll)
			{
				// @0x81997: pitch 95..126
				EMIT_SOUND_DYN(ENT(pev), CHAN_BODY, "player/superjump-land.wav", VOL_NORM, ATTN_NORM, 0,
					95 + RANDOM_LONG(0, 31));
				pev->iuser4 |= TSSTUNT_PRONE | TSSTUNT_STOP;
				pev->view_ofs = Vector(0, 0, 16);
				m_TSStunt.m_flRollTime = 0.75f;
				m_TSAnim.Animate(this, TSA_CC_SUPERJUMP_LAND); // superjump_land pose
			}
		}
	}

	// select the proper animation for the player character
	if (IsAlive())
	{
		if (0 == pev->velocity.x && 0 == pev->velocity.y)
			m_TSAnim.Animate(this, TSA_IDLE);
		else if ((0 != pev->velocity.x || 0 != pev->velocity.y) && (FBitSet(pev->flags, FL_ONGROUND)))
			m_TSAnim.Animate(this, TSA_RUN);
		else if (pev->waterlevel > 1)
			m_TSAnim.Animate(this, TSA_RUN);
	}

	StudioFrameAdvance();
	CheckPowerups(pev);

	UpdatePlayerSound();

pt_end:
#if defined(CLIENT_WEAPONS)
	// TS slow-motion fire-rate dilation (RE scratchpad/SLOWMO_PRESENTATION_RE.md §B): the per-frame
	// weapon-timer decrement is the player's effective frametime, scaled by the current slow factor
	// (pev->fuser1, floored at 0.01) — so a slowed player's next-attack countdowns drain slower and
	// they fire slower. The DoubleFireRate powerup (m_iPowerups & 0x10) drains 1.5x faster. The client
	// runs the identical scaled decrement in its prediction (fuser1 is networked), so it stays in sync.
	float slowFrametime = (pev->fuser1 < 0.01f ? 0.01f : pev->fuser1) * gpGlobals->frametime;
	if (m_iPowerups & 0x10) // TS_PWUP_DBLFIRE
		slowFrametime *= 1.5f;

	// Decay timers on weapons
	// go through all of the weapons and make a list of the ones to pack
	for (int i = 0; i < MAX_ITEM_TYPES; i++)
	{
		if (m_rgpPlayerItems[i])
		{
			CBasePlayerItem* pPlayerItem = m_rgpPlayerItems[i];

			while (pPlayerItem)
			{
				CBasePlayerWeapon* gun = pPlayerItem->GetWeaponPtr();

				if (gun && gun->UseDecrement())
				{
					gun->m_flNextPrimaryAttack = V_max(gun->m_flNextPrimaryAttack - slowFrametime, -1.0f);
					gun->m_flNextSecondaryAttack = V_max(gun->m_flNextSecondaryAttack - slowFrametime, -0.001f);

					if (gun->m_flTimeWeaponIdle != 1000)
					{
						gun->m_flTimeWeaponIdle = V_max(gun->m_flTimeWeaponIdle - slowFrametime, -0.001f);
					}

					if (gun->pev->fuser1 != 1000)
					{
						gun->pev->fuser1 = V_max(gun->pev->fuser1 - slowFrametime, -0.001f);
					}

					gun->DecrementTimers();

					// TS: the alternate-attack gate (gun+0xa4) decrements like the other
					// predicted timers (the base ItemPostFrame CanAttack()s it for IN_CANCEL).
					if (gun->m_iId == TSGUN_WEAPON_ID)
					{
						CTSGun* ts = static_cast<CTSGun*>(gun);
						ts->m_flNextAlternateAttack = V_max(ts->m_flNextAlternateAttack - slowFrametime, -0.001f);
					}

					// Only decrement if not flagged as NO_DECREMENT
					//					if ( gun->m_flPumpTime != 1000 )
					//	{
					//		gun->m_flPumpTime	= V_max( gun->m_flPumpTime - gpGlobals->frametime, -0.001 );
					//	}
				}

				pPlayerItem = pPlayerItem->m_pNext;
			}
		}
	}

	m_flNextAttack -= slowFrametime; // TS: slow-scaled (the player-level attack gate)
	if (m_flNextAttack < -0.001)
		m_flNextAttack = -0.001;

	if (m_flNextAmmoBurn != 1000)
	{
		m_flNextAmmoBurn -= gpGlobals->frametime;

		if (m_flNextAmmoBurn < -0.001)
			m_flNextAmmoBurn = -0.001;
	}

	if (m_flAmmoStartCharge != 1000)
	{
		m_flAmmoStartCharge -= gpGlobals->frametime;

		if (m_flAmmoStartCharge < -0.001)
			m_flAmmoStartCharge = -0.001;
	}
#endif

	// Track button info so we can detect 'pressed' and 'released' buttons next frame
	m_afButtonLast = pev->button;
}


// checks if the spot is clear of players
bool IsSpawnPointValid(CBaseEntity* pPlayer, CBaseEntity* pSpot)
{
	CBaseEntity* ent = NULL;

	if (!pSpot->IsTriggered(pPlayer))
	{
		return false;
	}

	while ((ent = UTIL_FindEntityInSphere(ent, pSpot->pev->origin, 128)) != NULL)
	{
		// if ent is a client, don't spawn on 'em
		if (ent->IsPlayer() && ent != pPlayer)
			return false;
	}

	return true;
}

/*
@0x66128 EntSelectSpawnPoint — the original REPLACED the stock spot walker: pick the class and
delegate to TSSelectSpawnSpot (the scorer: teammate-cover band / unseen band / nearest-distance,
crowded-winner telefrag, info_player_deathmatch/start fallback; sets g_pLastSpawn).
- Teamplay rules (g_pGameRules->IsTeamplay, @0x6614d) and a team TSGameRules knows (GetTeamIndex of
  TeamID(), @0x66177): that team's spot class and fallback from its team record (+0x68/+0x88),
  "info_player_team<N>" and "info_player_deathmatch" (written by InitTeams @0x6801b/@0x68032; its
  info_player_start fallback for team 1 needs mode 2/3, which InitTeams never sees, so it is dead).
- Teamplay and an unknown team: "info_player_deathmatch" with fallback "" (@0x661bc).
- Otherwise "info_player_deathmatch" with no fallback (@0x661e0).
*/
edict_t* EntSelectSpawnPoint(CBaseEntity* pPlayer)
{
	char szSpotClass[32];
	char szFallback[32];
	const char* pFallback = NULL;
	strcpy(szSpotClass, "info_player_deathmatch");
	if (g_pGameRules->IsTeamplay())
	{
		szFallback[0] = '\0';
		pFallback = szFallback;
		const int team = TSGameRules ? TSGameRules->GetTeamIndex(pPlayer->TeamID()) : -1;
		if (team >= 0)
		{
			strcpy(szSpotClass, TSGameRules->m_teamSpot[team]);
			strcpy(szFallback, TSGameRules->m_teamSpotFallback[team]);
		}
	}
	edict_t* pent = TSSelectSpawnSpot(szSpotClass, pPlayer, pFallback);
	if (FNullEnt(pent))
		return CWorld::World->edict(); // @0x6621e: the world, silently
	return pent;
}

void CBasePlayer::Spawn()
{
	m_flStartCharge = gpGlobals->time;
	m_bIsSpawning = true;

	//Make sure this gets reset even if somebody adds an early return or throws an exception.
	const CallOnDestroy resetIsSpawning{[this]()
		{
			//Done spawning; reset.
			m_bIsSpawning = false;
		}};

	TSInit(); // @0x7f586 — the TS per-life reset, the FIRST call in the original's Spawn

	pev->classname = MAKE_STRING("player");
	pev->health = 100;
	pev->armorvalue = 0;
	m_bHasKevlar = false; // kevlar is per-life — re-applied each spawn via the loadout (AddItem)
	pev->takedamage = DAMAGE_AIM;
	pev->solid = SOLID_SLIDEBOX;
	pev->movetype = MOVETYPE_WALK;
	pev->max_health = pev->health;
	pev->flags &= FL_PROXY | FL_FAKECLIENT; // keep proxy and fakeclient flags set by engine
	pev->flags |= FL_CLIENT;
	pev->air_finished = gpGlobals->time + 12;
	pev->dmg = 2; // initial water damage
	pev->effects = 0;
	pev->deadflag = DEAD_NO;
	pev->dmg_take = 0;
	pev->dmg_save = 0;
	pev->friction = 1.0;
	pev->gravity = 1.0;
	// (super-jump/kung-fu per-life clears live in TSInit now — pev->gravity 1.0 above
	// covers the super-jump restore; TSInit also re-grants map-global superjump.)
	m_bitsHUDDamage = -1;
	m_bitsDamageType = 0;
	m_afPhysicsFlags = 0;
	m_fLongJump = false; // no longjump module.

	g_engfuncs.pfnSetPhysicsKeyValue(edict(), "slj", "0");
	g_engfuncs.pfnSetPhysicsKeyValue(edict(), "hl", "1");

	m_iFOV = 0;		   // init field of view.
	m_iClientFOV = -1; // make sure fov reset is sent

	m_flNextDecalTime = 0; // let this player decal as soon as he spawns.

	m_flgeigerDelay = gpGlobals->time + 2.0; // wait a few seconds until user-defined message registrations
											 // are recieved by all clients

	m_flTimeStepSound = 0;
	m_iStepLeft = 0;
	m_flFieldOfView = 0.5; // some monsters use this to determine whether or not the player is looking at them.

	m_bloodColor = BLOOD_COLOR_RED;
	m_flNextAttack = UTIL_WeaponTimeBase();
	StartSneaking();

	m_iFlashBattery = 99;
	m_flFlashLightTime = 1; // force first message

	// dont let uninitialized value here hurt the player
	m_flFallVelocity = 0;

	g_pGameRules->SetDefaultPlayerTeam(this);
	g_pGameRules->GetPlayerSpawnSpot(this);

	SET_MODEL(ENT(pev), "models/player.mdl");
	g_ulModelIndexPlayer = pev->modelindex;
	pev->sequence = LookupActivity(ACT_IDLE);

	if (FBitSet(pev->flags, FL_DUCKING))
		UTIL_SetSize(pev, VEC_DUCK_HULL_MIN, VEC_DUCK_HULL_MAX);
	else
		UTIL_SetSize(pev, VEC_HULL_MIN, VEC_HULL_MAX);

	pev->view_ofs = VEC_VIEW;
	Precache();
	m_HackedGunPos = Vector(0, 32, 0);

	if (m_iPlayerSound == SOUNDLIST_EMPTY)
	{
		ALERT(at_console, "Couldn't alloc player sound slot!\n");
	}

	m_fNoPlayerSound = false; // normal sound behavior.

	m_pLastItem = NULL;
	m_fInitHUD = true;
	m_iClientHideHUD = -1; // force this to be recalculated
	m_fWeapon = false;
	m_pClientActiveItem = NULL;
	m_iClientBattery = -1;
	m_iClientTSArmor = -1; // @0x7f9a0: the original's armor cache is this one (player+0x9e4)

	// reset all ammo values to 0
	for (int i = 0; i < MAX_AMMO_SLOTS; i++)
	{
		m_rgAmmo[i] = 0;
		m_rgAmmoLast[i] = 0; // client ammo values also have to be reset  (the death hud clear messages does on the client side)
	}

	m_lastx = m_lasty = 0;

	// (no m_flNextChatTime = time here: the original's Spawn leaves 0x7c0 to TSInit, which zeroes it)

	// A spawn puts him in play, whatever he was doing: TSInit ended any observer mode at the top, and
	// PlayerSpawn equips him. A joiner goes to spectate from CTSGameRules::InitHUD (@0x694c8).
	g_pGameRules->PlayerSpawn(this);
}


void CBasePlayer::Precache()
{
	// SOUNDS / MODELS ARE PRECACHED in ClientPrecache() (game specific)
	// because they need to precache before any clients have connected

	// init geiger counter vars during spawn and each time
	// we cross a level transition

	m_flgeigerRange = 1000;
	m_igeigerRangePrev = 1000;

	m_bitsDamageType = 0;
	m_bitsHUDDamage = -1;

	m_iClientBattery = -1;
	m_iClientTSArmor = -1; // @0xca997

	m_iTrain |= TRAIN_NEW;

	// Make sure any necessary user messages have been registered
	LinkUserMessages();

	m_iUpdateTime = 5; // won't update for 1/2 a second

	if (gInitHUD)
		m_fInitHUD = true;
}


bool CBasePlayer::Save(CSave& save)
{
	if (!CBaseMonster::Save(save))
		return false;

	return save.WriteFields("PLAYER", this, m_playerSaveData, ARRAYSIZE(m_playerSaveData));
}


//
// Marks everything as new so the player will resend this to the hud.
//
void CBasePlayer::RenewItems()
{
}


bool CBasePlayer::Restore(CRestore& restore)
{
	if (!CBaseMonster::Restore(restore))
		return false;

	bool status = restore.ReadFields("PLAYER", this, m_playerSaveData, ARRAYSIZE(m_playerSaveData));

	SAVERESTOREDATA* pSaveData = (SAVERESTOREDATA*)gpGlobals->pSaveData;
	// landmark isn't present.
	if (0 == pSaveData->fUseLandmark)
	{
		ALERT(at_console, "No Landmark:%s\n", pSaveData->szLandmarkName);

		// default to normal spawn
		edict_t* pentSpawnSpot = EntSelectSpawnPoint(this);
		pev->origin = VARS(pentSpawnSpot)->origin + Vector(0, 0, 1);
		pev->angles = VARS(pentSpawnSpot)->angles;
	}
	pev->v_angle.z = 0; // Clear out roll
	pev->angles = pev->v_angle;

	pev->fixangle = 1; // turn this way immediately

	m_iClientFOV = -1; // Make sure the client gets the right FOV value.

	// Reset room type on level change.
	if (!FStrEq(restore.GetData().szCurrentMapName, STRING(gpGlobals->mapname)))
	{
		m_SndRoomtype = 0;
	}

	// Copied from spawn() for now
	m_bloodColor = BLOOD_COLOR_RED;

	g_ulModelIndexPlayer = pev->modelindex;

	if (FBitSet(pev->flags, FL_DUCKING))
	{
		// Use the crouch HACK
		//FixPlayerCrouchStuck( edict() );
		// Don't need to do this with new player prediction code.
		UTIL_SetSize(pev, VEC_DUCK_HULL_MIN, VEC_DUCK_HULL_MAX);
	}
	else
	{
		UTIL_SetSize(pev, VEC_HULL_MIN, VEC_HULL_MAX);
	}

	g_engfuncs.pfnSetPhysicsKeyValue(edict(), "hl", "1");

	if (m_fLongJump)
	{
		g_engfuncs.pfnSetPhysicsKeyValue(edict(), "slj", "1");
	}
	else
	{
		g_engfuncs.pfnSetPhysicsKeyValue(edict(), "slj", "0");
	}

	RenewItems();

	TabulateAmmo();

#if defined(CLIENT_WEAPONS)
	// HACK:	This variable is saved/restored in CBaseMonster as a time variable, but we're using it
	//			as just a counter.  Ideally, this needs its own variable that's saved as a plain float.
	//			Barring that, we clear it out here instead of using the incorrect restored time value.
	m_flNextAttack = UTIL_WeaponTimeBase();
#endif

	m_bResetViewEntity = true;

	m_bRestored = true;

	return status;
}



void CBasePlayer::SelectNextItem(int iItem)
{
	CBasePlayerItem* pItem;

	pItem = m_rgpPlayerItems[iItem];

	if (!pItem)
		return;

	if (pItem == m_pActiveItem)
	{
		// select the next one in the chain
		pItem = m_pActiveItem->m_pNext;
		if (!pItem)
		{
			return;
		}

		CBasePlayerItem* pLast;
		pLast = pItem;
		while (pLast->m_pNext)
			pLast = pLast->m_pNext;

		// relink chain
		pLast->m_pNext = m_pActiveItem;
		m_pActiveItem->m_pNext = NULL;
		m_rgpPlayerItems[iItem] = pItem;
	}

	ResetAutoaim();

	// FIX, this needs to queue them up and delay
	if (m_pActiveItem)
	{
		m_pActiveItem->Holster();
	}

	m_pActiveItem = pItem;

	if (m_pActiveItem)
	{
		m_pActiveItem->Deploy();
		m_pActiveItem->UpdateItemInfo();
	}
}

void CBasePlayer::SelectItem(const char* pstr)
{
	if (!pstr)
		return;

	CBasePlayerItem* pItem = NULL;

	for (int i = 0; i < MAX_ITEM_TYPES; i++)
	{
		if (m_rgpPlayerItems[i])
		{
			pItem = m_rgpPlayerItems[i];

			while (pItem)
			{
				if (FClassnameIs(pItem->pev, pstr))
					break;
				pItem = pItem->m_pNext;
			}
		}

		if (pItem)
			break;
	}

	if (!pItem)
		return;


	if (pItem == m_pActiveItem)
		return;

	ResetAutoaim();

	// FIX, this needs to queue them up and delay
	if (m_pActiveItem)
		m_pActiveItem->Holster();

	m_pLastItem = m_pActiveItem;
	m_pActiveItem = pItem;

	if (m_pActiveItem)
	{
		m_pActiveItem->m_ForceSendAnimations = true;
		m_pActiveItem->Deploy();
		m_pActiveItem->m_ForceSendAnimations = false;
		m_pActiveItem->UpdateItemInfo();
	}
}

//==============================================
// HasWeapons - do I have any weapons at all?
//==============================================
bool CBasePlayer::HasWeapons()
{
	int i;

	for (i = 0; i < MAX_ITEM_TYPES; i++)
	{
		if (m_rgpPlayerItems[i])
		{
			return true;
		}
	}

	return false;
}

void CBasePlayer::SelectPrevItem(int iItem)
{
}


const char* CBasePlayer::TeamID()
{
	if (pev == NULL) // Not fully connected yet
		return "";

	// return their team name
	return m_szTeamName;
}


//==============================================
// !!!UNDONE:ultra temporary SprayCan entity to apply
// decal frame at a time. For PreAlpha CD
//==============================================
class CSprayCan : public CBaseEntity
{
public:
	void Spawn(entvars_t* pevOwner);
	void Think() override;

	int ObjectCaps() override { return FCAP_DONT_SAVE; }
};

void CSprayCan::Spawn(entvars_t* pevOwner)
{
	pev->origin = pevOwner->origin + Vector(0, 0, 32);
	pev->angles = pevOwner->v_angle;
	pev->owner = ENT(pevOwner);
	pev->frame = 0;

	pev->nextthink = gpGlobals->time + 0.1;
	EMIT_SOUND(ENT(pev), CHAN_VOICE, "player/sprayer.wav", 1, ATTN_NORM);
}

void CSprayCan::Think()
{
	TraceResult tr;
	int playernum;
	int nFrames;
	CBasePlayer* pPlayer;

	pPlayer = (CBasePlayer*)GET_PRIVATE(pev->owner);

	if (pPlayer)
		nFrames = pPlayer->GetCustomDecalFrames();
	else
		nFrames = -1;

	playernum = ENTINDEX(pev->owner);

	// ALERT(at_console, "Spray by player %i, %i of %i\n", playernum, (int)(pev->frame + 1), nFrames);

	UTIL_MakeVectors(pev->angles);
	UTIL_TraceLine(pev->origin, pev->origin + gpGlobals->v_forward * 128, ignore_monsters, pev->owner, &tr);

	// No customization present.
	if (nFrames == -1)
	{
		UTIL_DecalTrace(&tr, DECAL_LAMBDA6);
		UTIL_Remove(this);
	}
	else
	{
		UTIL_PlayerDecalTrace(&tr, playernum, pev->frame, true);
		// Just painted last custom frame.
		if (pev->frame++ >= (nFrames - 1))
			UTIL_Remove(this);
	}

	pev->nextthink = gpGlobals->time + 0.1;
}

class CBloodSplat : public CBaseEntity
{
public:
	void Spawn(entvars_t* pevOwner);
	void Spray();
};

void CBloodSplat::Spawn(entvars_t* pevOwner)
{
	pev->origin = pevOwner->origin + Vector(0, 0, 32);
	pev->angles = pevOwner->v_angle;
	pev->owner = ENT(pevOwner);

	SetThink(&CBloodSplat::Spray);
	pev->nextthink = gpGlobals->time + 0.1;
}

void CBloodSplat::Spray()
{
	TraceResult tr;

	if (g_Language != LANGUAGE_GERMAN)
	{
		UTIL_MakeVectors(pev->angles);
		UTIL_TraceLine(pev->origin, pev->origin + gpGlobals->v_forward * 128, ignore_monsters, pev->owner, &tr);

		UTIL_BloodDecalTrace(&tr, BLOOD_COLOR_RED);
	}
	SetThink(&CBloodSplat::SUB_Remove);
	pev->nextthink = gpGlobals->time + 0.1;
}

//==============================================

static edict_t* GiveNamedItem_Common(entvars_t* pev, const char* pszName)
{
	int istr = MAKE_STRING(pszName);

	edict_t* pent = CREATE_NAMED_ENTITY(istr);
	if (FNullEnt(pent))
	{
		ALERT(at_console, "NULL Ent in GiveNamedItem!\n");
		return nullptr;
	}
	VARS(pent)->origin = pev->origin;
	pent->v.spawnflags |= SF_NORESPAWN;

	DispatchSpawn(pent);

	return pent;
}

void CBasePlayer::GiveNamedItem(const char* szName)
{
	auto pent = GiveNamedItem_Common(pev, szName);

	if (!pent)
	{
		return;
	}

	DispatchTouch(pent, ENT(pev));
}

void CBasePlayer::GiveNamedItem(const char* szName, int defaultAmmo)
{
	auto pent = GiveNamedItem_Common(pev, szName);

	if (!pent)
	{
		return;
	}

	auto entity = CBaseEntity::Instance(pent);

	if (!entity)
	{
		return;
	}

	if (auto weapon = dynamic_cast<CBasePlayerWeapon*>(entity); weapon)
	{
		weapon->m_iDefaultAmmo = defaultAmmo;
	}

	DispatchTouch(pent, ENT(pev));
}

/*
===============
ForceClientDllUpdate

When recording a demo, we need to have the server tell us the entire client state
so that the client side .dll can behave correctly.
Reset stuff so that the state is transmitted.
===============
*/
void CBasePlayer::ForceClientDllUpdate()
{
	m_iClientHealth = -1;
	m_iClientBattery = -1;
	m_iClientTSHealth = -1; // @0x825a7, @0x825b1, @0x825ea: the TS HUD caches (player+0x9e0, 0x9e4, 0x544)
	m_iClientTSArmor = -1;
	m_iClientTSCash = -1;
	m_iClientHideHUD = -1;
	m_iClientFOV = -1;
	m_ClientWeaponBits = 0;

	for (int i = 0; i < MAX_AMMO_SLOTS; ++i)
	{
		m_rgAmmoLast[i] = 0;
	}

	m_iTrain |= TRAIN_NEW; // Force new train message.
	m_fWeapon = false;	   // Force weapon send
	m_fKnownItem = false;  // Force weaponinit messages.
	m_fInitHUD = true;	   // Force HUD gmsgResetHUD message
	m_flNextClientUpdate = 0; // @0x82620: the update below runs now, whatever the 0.1 s throttle says
	m_iClientActiveItems = 0; // @0x825f4: ActItems goes again once the mask is not 0

	// @0x82594: the original also re-tells this client its own spectator state —
	// MSG_ONE (idx, deadflag == DEAD_RESPAWNABLE).
	MESSAGE_BEGIN(MSG_ONE, gmsgSpectator, NULL, pev);
	WRITE_BYTE(ENTINDEX(edict()));
	WRITE_BYTE(pev->deadflag == DEAD_RESPAWNABLE ? 1 : 0);
	MESSAGE_END();

	// Now force all the necessary messages
	//  to be sent.
	UpdateClientData();
}

/*
============
ImpulseCommands
============
*/
void CBasePlayer::ImpulseCommands()
{
	TraceResult tr; // UNDONE: kill me! This is temporary for PreAlpha CDs

	// Handle use events
	PlayerUse();

	int iImpulse = (int)pev->impulse;
	switch (iImpulse)
	{
	case 99:
	{

		bool iOn;

		if (0 == gmsgLogo)
		{
			iOn = true;
			gmsgLogo = REG_USER_MSG("Logo", 1);
		}
		else
		{
			iOn = false;
		}

		ASSERT(gmsgLogo > 0);
		// send "health" update message
		MESSAGE_BEGIN(MSG_ONE, gmsgLogo, NULL, pev);
		WRITE_BYTE(static_cast<int>(iOn));
		MESSAGE_END();

		if (!iOn)
			gmsgLogo = 0;
		break;
	}
	// (no case 100: the original's ImpulseCommands @0xcb780 has no flashlight impulse, so impulse 100
	// goes on to CheatImpulseCommands, which has none either)

	case 201: // paint decal

		if (gpGlobals->time < m_flNextDecalTime)
		{
			// too early!
			break;
		}

		UTIL_MakeVectors(pev->v_angle);
		UTIL_TraceLine(pev->origin + pev->view_ofs, pev->origin + pev->view_ofs + gpGlobals->v_forward * 128, ignore_monsters, ENT(pev), &tr);

		if (tr.flFraction != 1.0)
		{ // line hit something, so paint a decal
			m_flNextDecalTime = gpGlobals->time + decalfrequency.value;
			CSprayCan* pCan = GetClassPtr((CSprayCan*)NULL);
			pCan->Spawn(pev);
		}

		break;

	case 204: // @0xcb9b0: resend the client .dll's data, no cheats needed
		ForceClientDllUpdate();
		break;

	default:
		// check all of the cheat impulse commands now
		CheatImpulseCommands(iImpulse);
		break;
	}

	pev->impulse = 0;
}

//=========================================================
//=========================================================
void CBasePlayer::CheatImpulseCommands(int iImpulse)
{
	if (0 == g_flWeaponCheat) // @0xcba01: sv_cheats as the map started
	{
		return;
	}

	CBaseEntity* pEntity;
	TraceResult tr;

	switch (iImpulse)
	{
	// (the original's table @0xcba3c starts at 76 but sends 76 and 101 nowhere, nor 203: no
	// grunt-o-matic, no impulse 101 give-all, no remove creature)
	case 102:
		// Gibbage!!!
		CGib::SpawnRandomGibs(pev, 1, true);
		break;

	case 103:
		// What the hell are you doing?
		pEntity = UTIL_FindEntityForward(this);
		if (pEntity)
		{
			CBaseMonster* pMonster = pEntity->MyMonsterPointer();
			if (pMonster)
				pMonster->ReportAIState();
		}
		break;

	case 104:
		// Dump all of the global state varaibles (and global entity names)
		gGlobalState.DumpGlobals();
		break;

	case 105: // player makes no sound for monsters to hear.
	{
		if (m_fNoPlayerSound)
		{
			ALERT(at_console, "Player is audible\n");
			m_fNoPlayerSound = false;
		}
		else
		{
			ALERT(at_console, "Player is silent\n");
			m_fNoPlayerSound = true;
		}
		break;
	}

	case 106:
		// Give me the classname and targetname of this entity.
		pEntity = UTIL_FindEntityForward(this);
		if (pEntity)
		{
			ALERT(at_console, "Classname: %s", STRING(pEntity->pev->classname));

			if (!FStringNull(pEntity->pev->targetname))
			{
				ALERT(at_console, " - Targetname: %s\n", STRING(pEntity->pev->targetname));
			}
			else
			{
				ALERT(at_console, " - TargetName: No Targetname\n");
			}

			ALERT(at_console, "Model: %s\n", STRING(pEntity->pev->model));
			if (!FStringNull(pEntity->pev->globalname))
				ALERT(at_console, "Globalname: %s\n", STRING(pEntity->pev->globalname));
		}
		break;

	case 107:
	{
		TraceResult tr;

		edict_t* pWorld = CWorld::World->edict();

		Vector start = pev->origin + pev->view_ofs;
		Vector end = start + gpGlobals->v_forward * 1024;
		UTIL_TraceLine(start, end, ignore_monsters, edict(), &tr);
		if (tr.pHit)
			pWorld = tr.pHit;
		const char* pTextureName = TRACE_TEXTURE(pWorld, start, end);
		if (pTextureName)
			ALERT(at_console, "Texture: %s (%c)\n", pTextureName, PM_FindTextureType(pTextureName));
	}
	break;
	case 195: // show shortest paths for entire level to nearest node
	{
		Create("node_viewer_fly", pev->origin, pev->angles);
	}
	break;
	case 196: // show shortest paths for entire level to nearest node
	{
		Create("node_viewer_large", pev->origin, pev->angles);
	}
	break;
	case 197: // show shortest paths for entire level to nearest node
	{
		Create("node_viewer_human", pev->origin, pev->angles);
	}
	break;
	case 199: // show nearest node and all connections
	{
		ALERT(at_console, "%d\n", WorldGraph.FindNearestNode(pev->origin, bits_NODE_GROUP_REALM));
		WorldGraph.ShowNodeConnections(WorldGraph.FindNearestNode(pev->origin, bits_NODE_GROUP_REALM));
	}
	break;
	case 202: // Random blood splatter
		UTIL_MakeVectors(pev->v_angle);
		UTIL_TraceLine(pev->origin + pev->view_ofs, pev->origin + pev->view_ofs + gpGlobals->v_forward * 128, ignore_monsters, ENT(pev), &tr);

		if (tr.flFraction != 1.0)
		{ // line hit something, so paint a decal
			CBloodSplat* pBlood = GetClassPtr((CBloodSplat*)NULL);
			pBlood->Spawn(pev);
		}
		break;
	}
}

//
// Add a weapon to the player (Item == Weapon == Selectable Object)
//
bool CBasePlayer::AddPlayerItem(CBasePlayerItem* pItem)
{
	CBasePlayerItem* pInsert;

	pInsert = m_rgpPlayerItems[pItem->iItemSlot()];

	while (pInsert)
	{
		if (FClassnameIs(pInsert->pev, STRING(pItem->pev->classname)))
		{
			if (pItem->AddDuplicate(pInsert))
			{
				g_pGameRules->PlayerGotWeapon(this, pItem);
				pItem->CheckRespawn();

				// ugly hack to update clip w/o an update clip message
				pInsert->UpdateItemInfo();
				if (m_pActiveItem)
					m_pActiveItem->UpdateItemInfo();

				pItem->Kill();
			}
			else if (gEvilImpulse101)
			{
				// FIXME: remove anyway for deathmatch testing
				pItem->Kill();
			}
			return false;
		}
		pInsert = pInsert->m_pNext;
	}


	if (pItem->CanAddToPlayer(this))
	{
		pItem->AddToPlayer(this);

		g_pGameRules->PlayerGotWeapon(this, pItem);
		pItem->CheckRespawn();

		// Add to the list before extracting ammo so weapon ownership checks work properly.
		pItem->m_pNext = m_rgpPlayerItems[pItem->iItemSlot()];
		m_rgpPlayerItems[pItem->iItemSlot()] = pItem;

		if (auto weapon = pItem->GetWeaponPtr(); weapon)
		{
			weapon->ExtractAmmo(weapon);

			//Immediately update the ammo HUD so weapon pickup isn't sometimes red because the HUD doesn't know about regenerating/free ammo yet.
			if (-1 != weapon->m_iPrimaryAmmoType)
			{
				SendSingleAmmoUpdate(CBasePlayer::GetAmmoIndex(weapon->pszAmmo1()));
			}

			if (-1 != weapon->m_iSecondaryAmmoType)
			{
				SendSingleAmmoUpdate(CBasePlayer::GetAmmoIndex(weapon->pszAmmo2()));
			}

			//Don't show weapon pickup if we're spawning or if it's an exhaustible weapon (will show ammo pickup instead).
			if (!m_bIsSpawning && (weapon->iFlags() & ITEM_FLAG_EXHAUSTIBLE) == 0)
			{
				MESSAGE_BEGIN(MSG_ONE, gmsgWeapPickup, NULL, pev);
				WRITE_BYTE(weapon->m_iId);
				MESSAGE_END();
			}
		}

		// should we switch to this item?
		if (g_pGameRules->FShouldSwitchWeapon(this, pItem))
		{
			SwitchWeapon(pItem);
		}

		return true;
	}
	else if (gEvilImpulse101)
	{
		// FIXME: remove anyway for deathmatch testing
		pItem->Kill();
	}
	return false;
}



bool CBasePlayer::RemovePlayerItem(CBasePlayerItem* pItem)
{
	if (m_pActiveItem == pItem)
	{
		ResetAutoaim();
		pItem->Holster();
		pItem->pev->nextthink = 0; // crowbar may be trying to swing again, etc.
		pItem->SetThink(NULL);
		m_pActiveItem = NULL;
		pev->viewmodel = 0;
		pev->weaponmodel = 0;
	}
	if (m_pLastItem == pItem)
		m_pLastItem = NULL;

	CBasePlayerItem* pPrev = m_rgpPlayerItems[pItem->iItemSlot()];

	if (pPrev == pItem)
	{
		m_rgpPlayerItems[pItem->iItemSlot()] = pItem->m_pNext;
		return true;
	}
	else
	{
		while (pPrev && pPrev->m_pNext != pItem)
		{
			pPrev = pPrev->m_pNext;
		}
		if (pPrev)
		{
			pPrev->m_pNext = pItem->m_pNext;
			return true;
		}
	}
	return false;
}


//
// Returns the unique ID for the ammo, or -1 if error
//
int CBasePlayer::GiveAmmo(int iCount, const char* szName, int iMax)
{
	if (!szName)
	{
		// no ammo.
		return -1;
	}

	if (!g_pGameRules->CanHaveAmmo(this, szName, iMax))
	{
		// game rules say I can't have any more of this ammo type.
		return -1;
	}

	int i = 0;

	i = GetAmmoIndex(szName);

	if (i < 0 || i >= MAX_AMMO_SLOTS)
		return -1;

	int iAdd = V_min(iCount, iMax - m_rgAmmo[i]);
	if (iAdd < 1)
		return i;

	// If this is an exhaustible weapon make sure the player has it.
	if (const auto& ammoType = CBasePlayerItem::AmmoInfoArray[i]; ammoType.WeaponName != nullptr)
	{
		if (!HasNamedPlayerItem(ammoType.WeaponName))
		{
			GiveNamedItem(ammoType.WeaponName, 0);
		}
	}

	m_rgAmmo[i] += iAdd;


	if (0 != gmsgAmmoPickup) // make sure the ammo messages have been linked first
	{
		// Send the message that ammo has been picked up
		MESSAGE_BEGIN(MSG_ONE, gmsgAmmoPickup, NULL, pev);
		WRITE_BYTE(GetAmmoIndex(szName)); // ammo ID
		WRITE_BYTE(iAdd);				  // amount
		MESSAGE_END();
	}

	TabulateAmmo();

	return i;
}


/*
============
ItemPreFrame

Called every frame by the player PreThink
============
*/
void CBasePlayer::ItemPreFrame()
{
#if defined(CLIENT_WEAPONS)
	if (m_flNextAttack > 0)
#else
	if (gpGlobals->time < m_flNextAttack)
#endif
	{
		return;
	}

	if (!m_pActiveItem)
		return;

	m_pActiveItem->ItemPreFrame();
}


/*
============
ItemPostFrame

Called every frame by the player PostThink
============
*/
void CBasePlayer::ItemPostFrame()
{
	// check if the player is using a tank
	if (m_pTank != NULL)
		return;

	// @0xcc4ff-0xcc544: the attack delay holds back the impulses and the item's frame only while the
	// gun has a weapon in hand (its +0xcc item); then +alt2 (IN_CANCEL) on a reloading weapon that
	// can coldcock still gets through, so the coldcock can cut the reload. With no active item or
	// bare hands nothing is held back: impulses work from the first frame after a spawn. (A stock
	// Half-Life weapon, which a TS player does not hold, keeps the stock hold.)
	if (m_flNextAttack > 0 && m_pActiveItem)
	{
		if (m_pActiveItem->m_iId != TSGUN_WEAPON_ID)
			return;
		CTSGun* pGun = (CTSGun*)m_pActiveItem;
		if (pGun->m_pActiveItem)
		{
			if ((pev->button & IN_CANCEL) == 0 || !pGun->m_fInReload || !pGun->m_pActiveInfo
				|| pGun->m_pActiveInfo->iColdCockDamage == 0)
				return;
		}
	}

	ImpulseCommands();

	// @0xcc556: no second m_pTank check (halflife-updated's #3345 fix is not in the original); then
	// after the item's frame the shared seed is rerolled (@0xcc575, the original's +0x7ec). CmdStart
	// writes it again before the next command's frame, so the weapons only ever read the usercmd's.
	if (!m_pActiveItem)
		return;

	m_pActiveItem->ItemPostFrame();
	random_seed = RANDOM_LONG(0, 32000);
}

int CBasePlayer::AmmoInventory(int iAmmoIndex)
{
	if (iAmmoIndex == -1)
	{
		return -1;
	}

	return m_rgAmmo[iAmmoIndex];
}

int CBasePlayer::GetAmmoIndex(const char* psz)
{
	int i;

	if (!psz)
		return -1;

	for (i = 1; i < MAX_AMMO_SLOTS; i++)
	{
		if (!CBasePlayerItem::AmmoInfoArray[i].pszName)
			continue;

		if (stricmp(psz, CBasePlayerItem::AmmoInfoArray[i].pszName) == 0)
			return i;
	}

	return -1;
}

// Called from UpdateClientData
// makes sure the client has all the necessary ammo info,  if values have changed
void CBasePlayer::SendAmmoUpdate()
{
	for (int i = 0; i < MAX_AMMO_SLOTS; i++)
	{
		InternalSendSingleAmmoUpdate(i);
	}
}

void CBasePlayer::SendSingleAmmoUpdate(int ammoIndex)
{
	if (ammoIndex < 0 || ammoIndex >= MAX_AMMO_SLOTS)
	{
		return;
	}

	InternalSendSingleAmmoUpdate(ammoIndex);
}

void CBasePlayer::InternalSendSingleAmmoUpdate(int ammoIndex)
{
	if (m_rgAmmo[ammoIndex] != m_rgAmmoLast[ammoIndex])
	{
		m_rgAmmoLast[ammoIndex] = m_rgAmmo[ammoIndex];

		ASSERT(m_rgAmmo[ammoIndex] >= 0);
		ASSERT(m_rgAmmo[ammoIndex] < 255);

		// send "Ammo" update message
		MESSAGE_BEGIN(MSG_ONE, gmsgAmmoX, NULL, pev);
		WRITE_BYTE(ammoIndex);
		WRITE_BYTE(V_max(V_min(m_rgAmmo[ammoIndex], 254), 0)); // clamp the value to one byte
		MESSAGE_END();
	}
}

/*
=========================================================
	UpdateClientData

resends any changed player HUD info to the client.
Called every frame by PlayerPreThink
Also called at start of demo recording and playback by
ForceClientDllUpdate to ensure the demo gets messages
reflecting all of the HUD state info.
=========================================================
*/
void CBasePlayer::UpdateClientData()
{
	// @0x81d1f: the whole update runs at most every 0.1 s, and not at all for a leaver (-1).
	if (m_flNextClientUpdate > gpGlobals->time || m_flNextClientUpdate < 0.0f)
		return;
	m_flNextClientUpdate = gpGlobals->time + 0.1f;

	const bool fullHUDInitRequired = m_fInitHUD != false;

	if (m_fInitHUD)
	{
		m_fInitHUD = false;
		gInitHUD = false;

		MESSAGE_BEGIN(MSG_ONE, gmsgResetHUD, NULL, pev);
		WRITE_BYTE(0);
		MESSAGE_END();

		if (!m_fGameHUDInitialized)
		{
			MESSAGE_BEGIN(MSG_ONE, gmsgInitHUD, NULL, pev);
			MESSAGE_END();

			// The original sets this guard after both InitHUD calls (@0x81dff). reTS sets it first:
			// here the TS-patched Teamplay::InitHUD assigns the team via ChangePlayerTeam, whose
			// no-kill path calls StartObserver, which runs UpdateClientData again, and with the guard
			// set after that recursed to a stack overflow (seen in gdb).
			m_fGameHUDInitialized = true;
			g_pGameRules->InitHUD(this);
			// R1 phase 2: the TS InitHUD content is the standalone object's NON-VIRTUAL
			// InitHUD @0x69134, called from THIS UpdateClientData block in the original
			// (@0x81d00) — spectator-state catch-up, roster, SrvSett, round clock, MOTD gate.
			if (TSGameRules)
				TSGameRules->InitHUD(this);

			// (no m_iObserverLastMode = OBS_ROAMING here, the stock line: the original writes
			// player+0x76c only in StopObserver @0x89a6f, so the joiner's chase mode holds.)

			if (g_pGameRules->IsMultiplayer())
			{
				FireTargets("game_playerjoin", this, this, USE_TOGGLE, 0);
			}
		}

		FireTargets("game_playerspawn", this, this, USE_TOGGLE, 0);

		// @0x81e47: every pass through this block (not only the first) catches the client up on
		// the round clock, as CTSGameRules::InitHUD does: truncated seconds left, flag 0, when the
		// clock is armed and the round timer is on. A joiner gets it from InitHUD and here in the
		// same frame, and once more on the pass his StartObserver arms, 0.1 s later (the throttle).
		if (TSGameRules && TSGameRules->m_flRoundEnd != 0.0f && TSGameRules->m_bRoundTimerEnabled)
		{
			extern int gmsgTSRoundTime; // TS_gamerulesclass.cpp
			float left = TSGameRules->m_flRoundEnd - gpGlobals->time;
			if (left < 0)
				left = 0;
			MESSAGE_BEGIN(MSG_ONE, gmsgTSRoundTime, NULL, pev);
			WRITE_BYTE((int)left); // truncated (@0x81eac: fistp in round-toward-zero)
			WRITE_BYTE(0);
			MESSAGE_END();
		}

		// @0x81ef6: on a map whose ts_mapglobals gives everyone superjump, a player who has it is
		// shown the superjump powerup (0x100) on each HUD reset.
		if (TSGameRules && TSGameRules->m_bSrvSuperJump && m_bLowGravity)
		{
			extern int gmsgTSPowerUp; // ts_player.cpp
			MESSAGE_BEGIN(MSG_ONE, gmsgTSPowerUp, NULL, pev);
			WRITE_SHORT(TS_PWUP_LOWGRAV);
			WRITE_BYTE(0);
			MESSAGE_END();
		}

		// (no forced TSHealth/TSArmor/TSCash here: this block of the original clears none of their
		// caches; TSInit, Spawn, Precache, Killed and ForceClientDllUpdate do.)
	}

	if (m_iHideHUD != m_iClientHideHUD)
	{
		MESSAGE_BEGIN(MSG_ONE, gmsgHideWeapon, NULL, pev);
		WRITE_BYTE(m_iHideHUD);
		MESSAGE_END();

		m_iClientHideHUD = m_iHideHUD;
	}

	// No SetFOV on a change: the original sends SetFOV only from Killed and StartObserver (every
	// reference to its gmsgSetFOV slot). A scope's zoom reaches the client in the client data
	// (cd->fov), which HUD_PostRunCmd copies to g_lastFOV, in client.dll as in the SDK. m_iClientFOV
	// is still cached at the end of this function for the weapons' change checks.

	// (no game title: the original registers ShowGameTitle and never sends it; its only GOT use of
	// gDisplayTitle is CWorld::Precache, which sets it.)

	// (no Health or Battery message: the original registers both and sends neither; its only GOT uses
	// of gmsgHealth and gmsgBattery are in LinkUserMessages. The HUD and TSX read TSHealth/TSArmor below.)

	// (no Weapons message: the original registers none; the TS weapon HUD rides WeaponInfo/WStatus,
	// and the single weapon_tsgun is known to the client without it.)

	// (no damage block — the original's UpdateClientData @0x81d00 sends no Damage message and never
	// clears dmg_take/dmg_save or the damage bits: no write to entvars+0x1d8/0x1dc or player+0x9b0 in
	// it. TSX reads dmg_take and dmg_inflictor when TSHealth goes out below and clears dmg_take
	// itself, so a clear here hid every hit and kill from it. dossier-gmsg U4a.)

	// (no flashlight upkeep: the original has no flashlight functions and no battery handling here;
	// no WeaponList, which the original registers and never sends, its only GOT use of gmsgWeaponList
	// being LinkUserMessages; no SendAmmoUpdate, which nothing in the original calls; no CurWeapon;
	// no status bar, which the original lacks; no room type, which env_sound sends when it takes
	// over the player, @0xd0003.)

	// The Specialists custom-HUD messages (TSHealth/TSArmor/TSCash/...), in the original's order:
	// HideWeapon (above), TSHealth, TSArmor, Train, TSState, TSCash, TSSpace, PwUp, TSSlowMo, TSBTime,
	// ActItems, TSPAward, Wires, then the active gun's own update (@0x81f4c-0x82586). Each goes out
	// when its value differs from the one cached at the last send.
	{
		extern int gmsgTSHealth, gmsgTSArmor, gmsgTSState, gmsgTSCash, gmsgTSSpace; // ts_player.cpp globals
		extern int gmsgTSPowerUp, gmsgTSSlowAmt, gmsgTSBulletTime, gmsgTSPlayerAward, gmsgTSWires;

		// TSHealth (@0x81fa2-0x8204b): sent when the health differs from the whole number cached at the
		// last send, so a fractional health goes out on every update, as on the original. The value is
		// the health floored at 0 and truncated (the fistp runs with the control word's rounding set to
		// chop, @0x81fda); the cache is the health truncated, not floored.
		if ((float)m_iClientTSHealth != pev->health)
		{
			const float health = (0.0f > pev->health) ? 0.0f : pev->health;
			MESSAGE_BEGIN(MSG_ONE, gmsgTSHealth, NULL, ENT(pev));
			WRITE_BYTE((int)health);
			MESSAGE_END();
			m_iClientTSHealth = (int)pev->health;
		}

		// TSArmor (@0x82057-0x820f7): as TSHealth, sent when the armor differs from the whole number
		// cached at the last send, the armor truncated (the fistp with the rounding set to chop).
		if ((float)m_iClientTSArmor != pev->armorvalue)
		{
			m_iClientTSArmor = (int)pev->armorvalue;
			MESSAGE_BEGIN(MSG_ONE, gmsgTSArmor, NULL, ENT(pev));
			WRITE_BYTE((int)pev->armorvalue);
			MESSAGE_END();
		}

		// Train (@0x820f7-0x82146): after TSArmor, when the train state is flagged new.
		if ((m_iTrain & TRAIN_NEW) != 0)
		{
			MESSAGE_BEGIN(MSG_ONE, gmsgTrain, NULL, pev);
			WRITE_BYTE(m_iTrain & 0xF);
			MESSAGE_END();

			m_iTrain &= ~TRAIN_NEW;
		}

		// TSState — the posture byte (0 stand / 1 duck / 2 stunt-prone), source 0x498 / cache 0x499.
		// Plain on-change, NO ResetHUD-forced resend (the original's InitHUD memset deliberately
		// starts at 0x49A and skips this cache). Original send order: Train -> TSState -> TSCash.
		if (m_iPlayerState != m_iClientTSState)
		{
			m_iClientTSState = m_iPlayerState;
			MESSAGE_BEGIN(MSG_ONE, gmsgTSState, NULL, ENT(pev));
			WRITE_BYTE(m_iPlayerState);
			MESSAGE_END();
		}

		// TSCash — the player's cash (a short).
		if (m_iCash != m_iClientTSCash)
		{
			m_iClientTSCash = m_iCash;
			MESSAGE_BEGIN(MSG_ONE, gmsgTSCash, NULL, ENT(pev));
			WRITE_SHORT(m_iCash);
			MESSAGE_END();
		}

		// TSSpace — the free carry slots. The original sends the MAINTAINED int at 0x548 directly
		// (no clamp at the write); our slots derive from the arsenal, so the recompute IS the member.
		const int freeSlots = TSPlayerFreeSlots(this);
		if (freeSlots != m_iClientTSSpace)
		{
			m_iClientTSSpace = freeSlots;
			MESSAGE_BEGIN(MSG_ONE, gmsgTSSpace, NULL, ENT(pev));
			WRITE_BYTE(freeSlots);
			MESSAGE_END();
		}

		// PwUp — the HELD powerup HUD (the green top-left icon + "seconds available"). The original
		// sends gmsgTSPowerUp on change from UpdateClientData: WriteShort(held type 0x728) +
		// WriteByte(duration 0x730). The client shows the icon for the held type and the seconds
		// available; double-tap +use activates it (then PwUpTime drives the live countdown).
		int heldDur = m_iPowerupTime;
		if (m_iHeldPowerup != m_iClientHeldType || heldDur != m_iClientHeldDur)
		{
			m_iClientHeldType = m_iHeldPowerup;
			m_iClientHeldDur = heldDur;
			MESSAGE_BEGIN(MSG_ONE, gmsgTSPowerUp, NULL, ENT(pev));
			WRITE_SHORT(m_iHeldPowerup);
			WRITE_BYTE(heldDur < 0 ? 0 : heldDur); // @0x82295: the byte is clamped at 0 (the cache keeps the raw value)
			MESSAGE_END();
		}

		// TSSlowMo — the PERCEIVED WORLD slow amount sent to the client (WRITE_COORD(100 * factor); the
		// client normalizes 100 -> 1.0 = normal). @0x81d00 (UCD, decompile-exact): the factor is a FIXED
		// value per active powerup bit — slowpause (0.225) OR slowmo (0.35) — else the player's own slow
		// TARGET (m_flSlowMotionTarget @0x170). slowpause is tested FIRST, so it wins when both bits are
		// set. m_flClientSlowAmt caches the FACTOR (not the ×100 wire value), change-detected. NB the
		// original keys off the networked TARGET here, not pev->fuser1 (the eased CURRENT) and not
		// m_flSlowMotionRamp (the others-scale).
		float slowFactor = 0.225f; // TS_PWUP_SLOWPAUSE
		if (!(m_iPowerups & TS_PWUP_SLOWPAUSE))
		{
			slowFactor = 0.35f; // TS_PWUP_SLOWMO
			if (!(m_iPowerups & TS_PWUP_SLOWMO))
				slowFactor = m_flSlowMotionTarget; // default: the networked target (1.0 normal)
		}
		if (slowFactor != m_flClientSlowAmt)
		{
			m_flClientSlowAmt = slowFactor;
			MESSAGE_BEGIN(MSG_ONE, gmsgTSSlowAmt, NULL, ENT(pev));
			WRITE_COORD(100.0f * slowFactor);
			MESSAGE_END();
		}

		// TSBTime — the bullet-time variant flag: source byte 0x175 (m_bSlowBullets — the SAME byte,
		// by construction; star addendum settles the 0x175-vs-[0x5d] ambiguity), cache 0x174.
		// Client consumer: the recoil x0.66667 reduction while in bullet-time.
		if ((int)m_bSlowBullets != m_iClientBulletTime)
		{
			m_iClientBulletTime = m_bSlowBullets;
			MESSAGE_BEGIN(MSG_ONE, gmsgTSBulletTime, NULL, ENT(pev));
			WRITE_BYTE(m_bSlowBullets);
			MESSAGE_END();
		}

		// ActItems (@0x82393-0x82440): a live player holding the gun (id 16) broadcasts his weapon's
		// active attachments (CTSGun::GetActiveItems: laser 2 / flashlight 4, +0x40 akimbo) to
		// everyone when they differ from the last sent (player+0x720).
		if (m_pActiveItem && IsAlive() && m_pActiveItem->m_iId == TSGUN_WEAPON_ID)
		{
			const int activeItems = ((CTSGun *)m_pActiveItem)->GetActiveItems();
			if (m_iClientActiveItems != activeItems)
			{
				m_iClientActiveItems = activeItems;
				extern int gmsgTSActiveItems; // ts_gun.cpp
				MESSAGE_BEGIN(MSG_ALL, gmsgTSActiveItems);
				WRITE_BYTE(ENTINDEX(edict()));
				WRITE_BYTE(m_iClientActiveItems);
				MESSAGE_END();
			}
		}

		// TSPAward — the broadcast award/status byte (source 0x74c, cache 0x74d): MSG_ALL keyed by
		// entindex so every client tracks all players' status (scoreboard/observer UI). On-change
		// only, no forced resend (star_TSPAward.md — the sole send site in the original).
		if (m_iAward != m_iClientAward)
		{
			m_iClientAward = m_iAward;
			MESSAGE_BEGIN(MSG_ALL, gmsgTSPlayerAward);
			WRITE_BYTE(entindex());
			WRITE_BYTE(m_iAward);
			MESSAGE_END();
		}

		// Wires — the nearby-bomb wire ROLE triple (source 0x78c-0x78e, cache 0x78f-0x791):
		// MSG_ONE on any change of the three bytes (star_Wires.md). CheckForWires (PreThink,
		// mode-3 maps, 0.5 s throttle) maintains the source; zeros = walked away.
		if (m_iWires[0] != m_iClientWires[0] || m_iWires[1] != m_iClientWires[1] ||
			m_iWires[2] != m_iClientWires[2])
		{
			m_iClientWires[0] = m_iWires[0];
			m_iClientWires[1] = m_iWires[1];
			m_iClientWires[2] = m_iWires[2];
			MESSAGE_BEGIN(MSG_ONE, gmsgTSWires, NULL, ENT(pev));
			WRITE_BYTE(m_iWires[0]);
			WRITE_BYTE(m_iWires[1]);
			WRITE_BYTE(m_iWires[2]);
			MESSAGE_END();
		}
	}

	// @0x82546: only the active item updates its client, and only when it is the gun (id 16): no loop
	// over every item. Then the active item and the field of view are cached (@0x82574, @0x82580).
	if (m_pActiveItem && m_pActiveItem->m_iId == TSGUN_WEAPON_ID)
		m_pActiveItem->UpdateClientData(this);

	m_pClientActiveItem = m_pActiveItem;
	m_iClientFOV = m_iFOV;
}


//=========================================================
// FBecomeProne - Overridden for the player to set the proper
// physics flags when a barnacle grabs player.
//=========================================================
bool CBasePlayer::FBecomeProne()
{
	m_afPhysicsFlags |= PFLAG_ONBARNACLE;
	return true;
}

//=========================================================
// BarnacleVictimBitten - bad name for a function that is called
// by Barnacle victims when the barnacle pulls their head
// into its mouth. For the player, just die.
//=========================================================
void CBasePlayer::BarnacleVictimBitten(entvars_t* pevBarnacle)
{
	TakeDamage(pevBarnacle, pevBarnacle, pev->health + pev->armorvalue, DMG_SLASH | DMG_ALWAYSGIB);
}

//=========================================================
// BarnacleVictimReleased - overridden for player who has
// physics flags concerns.
//=========================================================
void CBasePlayer::BarnacleVictimReleased()
{
	m_afPhysicsFlags &= ~PFLAG_ONBARNACLE;
}


//=========================================================
// Illumination
// return player light level plus virtual muzzle flash
//=========================================================
int CBasePlayer::Illumination()
{
	int iIllum = CBaseEntity::Illumination();

	iIllum += m_iWeaponFlash;
	if (iIllum > 255)
		return 255;
	return iIllum;
}


void CBasePlayer::EnableControl(bool fControl)
{
	if (!fControl)
		pev->flags |= FL_FROZEN;
	else
		pev->flags &= ~FL_FROZEN;
}


#define DOT_1DEGREE 0.9998476951564
#define DOT_2DEGREE 0.9993908270191
#define DOT_3DEGREE 0.9986295347546
#define DOT_4DEGREE 0.9975640502598
#define DOT_5DEGREE 0.9961946980917
#define DOT_6DEGREE 0.9945218953683
#define DOT_7DEGREE 0.9925461516413
#define DOT_8DEGREE 0.9902680687416
#define DOT_9DEGREE 0.9876883405951
#define DOT_10DEGREE 0.9848077530122
#define DOT_15DEGREE 0.9659258262891
#define DOT_20DEGREE 0.9396926207859
#define DOT_25DEGREE 0.9063077870367

//=========================================================
// Autoaim
// set crosshair position to point to enemey
//=========================================================
Vector CBasePlayer::GetAutoaimVector(float flDelta)
{
	if (g_iSkillLevel == SKILL_HARD)
	{
		UTIL_MakeVectors(pev->v_angle + pev->punchangle);
		return gpGlobals->v_forward;
	}

	Vector vecSrc = GetGunPosition();
	float flDist = 8192;

	// always use non-sticky autoaim
	// UNDONE: use sever variable to chose!
	if (true || g_iSkillLevel == SKILL_MEDIUM)
	{
		m_vecAutoAim = Vector(0, 0, 0);
		// flDelta *= 0.5;
	}

	bool m_fOldTargeting = m_fOnTarget;
	Vector angles = AutoaimDeflection(vecSrc, flDist, flDelta);

	// update ontarget if changed
	if (!g_pGameRules->AllowAutoTargetCrosshair())
		m_fOnTarget = false;
	else if (m_fOldTargeting != m_fOnTarget)
	{
		m_pActiveItem->UpdateItemInfo();
	}

	if (angles.x > 180)
		angles.x -= 360;
	if (angles.x < -180)
		angles.x += 360;
	if (angles.y > 180)
		angles.y -= 360;
	if (angles.y < -180)
		angles.y += 360;

	if (angles.x > 25)
		angles.x = 25;
	if (angles.x < -25)
		angles.x = -25;
	if (angles.y > 12)
		angles.y = 12;
	if (angles.y < -12)
		angles.y = -12;


	// always use non-sticky autoaim
	// UNDONE: use sever variable to chose!
	if (false || g_iSkillLevel == SKILL_EASY)
	{
		m_vecAutoAim = m_vecAutoAim * 0.67 + angles * 0.33;
	}
	else
	{
		m_vecAutoAim = angles * 0.9;
	}

	// m_vecAutoAim = m_vecAutoAim * 0.99;

	// Don't send across network if sv_aim is 0
	if (g_psv_aim->value != 0 && g_psv_allow_autoaim != 0)
	{
		if (m_vecAutoAim.x != m_lastx ||
			m_vecAutoAim.y != m_lasty)
		{
			SET_CROSSHAIRANGLE(edict(), -m_vecAutoAim.x, m_vecAutoAim.y);

			m_lastx = m_vecAutoAim.x;
			m_lasty = m_vecAutoAim.y;
		}
	}
	else
	{
		ResetAutoaim();
	}

	// ALERT( at_console, "%f %f\n", angles.x, angles.y );

	UTIL_MakeVectors(pev->v_angle + pev->punchangle + m_vecAutoAim);
	return gpGlobals->v_forward;
}


Vector CBasePlayer::AutoaimDeflection(Vector& vecSrc, float flDist, float flDelta)
{
	edict_t* pEdict = UTIL_GetEntityList() + 1;
	CBaseEntity* pEntity;
	float bestdot;
	Vector bestdir;
	edict_t* bestent;
	TraceResult tr;

	if (g_psv_aim->value == 0 || g_psv_allow_autoaim->value == 0)
	{
		m_fOnTarget = false;
		return g_vecZero;
	}

	UTIL_MakeVectors(pev->v_angle + pev->punchangle + m_vecAutoAim);

	// try all possible entities
	bestdir = gpGlobals->v_forward;
	bestdot = flDelta; // +- 10 degrees
	bestent = NULL;

	m_fOnTarget = false;

	UTIL_TraceLine(vecSrc, vecSrc + bestdir * flDist, dont_ignore_monsters, edict(), &tr);


	if (tr.pHit && tr.pHit->v.takedamage != DAMAGE_NO)
	{
		// don't look through water
		if (!((pev->waterlevel != 3 && tr.pHit->v.waterlevel == 3) || (pev->waterlevel == 3 && tr.pHit->v.waterlevel == 0)))
		{
			if (tr.pHit->v.takedamage == DAMAGE_AIM)
				m_fOnTarget = true;

			return m_vecAutoAim;
		}
	}

	for (int i = 1; i < gpGlobals->maxEntities; i++, pEdict++)
	{
		Vector center;
		Vector dir;
		float dot;

		if (0 != pEdict->free) // Not in use
			continue;

		if (pEdict->v.takedamage != DAMAGE_AIM)
			continue;
		if (pEdict == edict())
			continue;
		//		if (pev->team > 0 && pEdict->v.team == pev->team)
		//			continue;	// don't aim at teammate
		if (!g_pGameRules->ShouldAutoAim(this, pEdict))
			continue;

		pEntity = Instance(pEdict);
		if (pEntity == NULL)
			continue;

		if (!pEntity->IsAlive())
			continue;

		// don't look through water
		if ((pev->waterlevel != 3 && pEntity->pev->waterlevel == 3) || (pev->waterlevel == 3 && pEntity->pev->waterlevel == 0))
			continue;

		center = pEntity->BodyTarget(vecSrc);

		dir = (center - vecSrc).Normalize();

		// make sure it's in front of the player
		if (DotProduct(dir, gpGlobals->v_forward) < 0)
			continue;

		dot = fabs(DotProduct(dir, gpGlobals->v_right)) + fabs(DotProduct(dir, gpGlobals->v_up)) * 0.5;

		// tweek for distance
		dot *= 1.0 + 0.2 * ((center - vecSrc).Length() / flDist);

		if (dot > bestdot)
			continue; // to far to turn

		UTIL_TraceLine(vecSrc, center, dont_ignore_monsters, edict(), &tr);
		if (tr.flFraction != 1.0 && tr.pHit != pEdict)
		{
			// ALERT( at_console, "hit %s, can't see %s\n", STRING( tr.pHit->v.classname ), STRING( pEdict->v.classname ) );
			continue;
		}

		// don't shoot at friends
		if (IRelationship(pEntity) < 0)
		{
			if (!pEntity->IsPlayer() && !g_pGameRules->IsDeathmatch())
				// ALERT( at_console, "friend\n");
				continue;
		}

		// can shoot at this one
		bestdot = dot;
		bestent = pEdict;
		bestdir = dir;
	}

	if (bestent)
	{
		bestdir = UTIL_VecToAngles(bestdir);
		bestdir.x = -bestdir.x;
		bestdir = bestdir - pev->v_angle - pev->punchangle;

		if (bestent->v.takedamage == DAMAGE_AIM)
			m_fOnTarget = true;

		return bestdir;
	}

	return Vector(0, 0, 0);
}


void CBasePlayer::ResetAutoaim()
{
	if (m_vecAutoAim.x != 0 || m_vecAutoAim.y != 0)
	{
		m_vecAutoAim = Vector(0, 0, 0);
		SET_CROSSHAIRANGLE(edict(), 0, 0);
	}
	m_fOnTarget = false;
}

/*
=============
SetCustomDecalFrames

  UNDONE:  Determine real frame limit, 8 is a placeholder.
  Note:  -1 means no custom frames present.
=============
*/
void CBasePlayer::SetCustomDecalFrames(int nFrames)
{
	if (nFrames > 0 &&
		nFrames < 8)
		m_nCustomSprayFrames = nFrames;
	else
		m_nCustomSprayFrames = -1;
}

/*
=============
GetCustomDecalFrames

  Returns the # of custom frames this player's custom clan logo contains.
=============
*/
int CBasePlayer::GetCustomDecalFrames()
{
	return m_nCustomSprayFrames;
}


//=========================================================
// DropPlayerItem - drop the named item, or if no name,
// the active item.
//=========================================================
void CBasePlayer::DropPlayerItem(char* pszItemName)
{
	if (!g_pGameRules->IsMultiplayer() || (weaponstay.value > 0))
	{
		// no dropping in single player.
		return;
	}

	if (0 == strlen(pszItemName))
	{
		// if this string has no length, the client didn't type a name!
		// assume player wants to drop the active item.
		// make the string null to make future operations in this function easier
		pszItemName = NULL;
	}

	CBasePlayerItem* pWeapon;
	int i;

	for (i = 0; i < MAX_ITEM_TYPES; i++)
	{
		pWeapon = m_rgpPlayerItems[i];

		while (pWeapon)
		{
			if (pszItemName)
			{
				// try to match by name.
				if (0 == strcmp(pszItemName, STRING(pWeapon->pev->classname)))
				{
					// match!
					break;
				}
			}
			else
			{
				// trying to drop active item
				if (pWeapon == m_pActiveItem)
				{
					// active item!
					break;
				}
			}

			pWeapon = pWeapon->m_pNext;
		}


		// if we land here with a valid pWeapon pointer, that's because we found the
		// item we want to drop and hit a BREAK;  pWeapon is the item.
		if (pWeapon)
		{
			if (!g_pGameRules->GetNextBestWeapon(this, pWeapon))
				return; // can't drop the item they asked for, may be our last item or something we can't holster

			UTIL_MakeVectors(pev->angles);

			ClearWeaponBit(pWeapon->m_iId); // take item off hud

			CWeaponBox* pWeaponBox = (CWeaponBox*)CBaseEntity::Create("weaponbox", pev->origin + gpGlobals->v_forward * 10, pev->angles, edict());
			pWeaponBox->pev->angles.x = 0;
			pWeaponBox->pev->angles.z = 0;
			pWeaponBox->PackWeapon(pWeapon);
			pWeaponBox->pev->velocity = gpGlobals->v_forward * 300 + gpGlobals->v_forward * 100;

			// drop half of the ammo for this weapon.
			int iAmmoIndex;

			iAmmoIndex = GetAmmoIndex(pWeapon->pszAmmo1()); // ???

			if (iAmmoIndex != -1)
			{
				// this weapon weapon uses ammo, so pack an appropriate amount.
				if ((pWeapon->iFlags() & ITEM_FLAG_EXHAUSTIBLE) != 0)
				{
					// pack up all the ammo, this weapon is its own ammo type
					pWeaponBox->PackAmmo(MAKE_STRING(pWeapon->pszAmmo1()), m_rgAmmo[iAmmoIndex]);
					m_rgAmmo[iAmmoIndex] = 0;
				}
				else
				{
					// pack half of the ammo
					pWeaponBox->PackAmmo(MAKE_STRING(pWeapon->pszAmmo1()), m_rgAmmo[iAmmoIndex] / 2);
					m_rgAmmo[iAmmoIndex] /= 2;
				}
			}

			return; // we're done, so stop searching with the FOR loop.
		}
	}
}

//=========================================================
// HasPlayerItem Does the player already have this item?
//=========================================================
bool CBasePlayer::HasPlayerItem(CBasePlayerItem* pCheckItem)
{
	CBasePlayerItem* pItem = m_rgpPlayerItems[pCheckItem->iItemSlot()];

	while (pItem)
	{
		if (FClassnameIs(pItem->pev, STRING(pCheckItem->pev->classname)))
		{
			return true;
		}
		pItem = pItem->m_pNext;
	}

	return false;
}

//=========================================================
// HasNamedPlayerItem Does the player already have this item?
//=========================================================
bool CBasePlayer::HasNamedPlayerItem(const char* pszItemName)
{
	CBasePlayerItem* pItem;
	int i;

	for (i = 0; i < MAX_ITEM_TYPES; i++)
	{
		pItem = m_rgpPlayerItems[i];

		while (pItem)
		{
			if (0 == strcmp(pszItemName, STRING(pItem->pev->classname)))
			{
				return true;
			}
			pItem = pItem->m_pNext;
		}
	}

	return false;
}

//=========================================================
// HasPlayerItemFromID
// Just compare IDs, rather than classnames
//=========================================================
bool CBasePlayer::HasPlayerItemFromID(int nID)
{
	CBasePlayerItem* pItem;
	int i;

	for (i = 0; i < MAX_ITEM_TYPES; i++)
	{
		pItem = m_rgpPlayerItems[i];

		while (pItem)
		{
			if (pItem->m_iId == nID)
			{
				return true;
			}

			pItem = pItem->m_pNext;
		}
	}

	return false;
}

//=========================================================
//
//=========================================================
bool CBasePlayer::SwitchWeapon(CBasePlayerItem* pWeapon)
{
	if (pWeapon && !pWeapon->CanDeploy())
	{
		return false;
	}

	ResetAutoaim();

	if (m_pActiveItem)
	{
		m_pActiveItem->Holster();
	}

	m_pActiveItem = pWeapon;

	if (pWeapon)
	{
		pWeapon->m_ForceSendAnimations = true;
		pWeapon->Deploy();
		pWeapon->m_ForceSendAnimations = false;
	}

	return true;
}

void CBasePlayer::EquipWeapon()
{
	if (m_pActiveItem)
	{
		if ((!FStringNull(pev->viewmodel) || !FStringNull(pev->weaponmodel)))
		{
			//Already have a weapon equipped and deployed.
			return;
		}

		//Have a weapon equipped, but not deployed.
		if (m_pActiveItem->CanDeploy() && m_pActiveItem->Deploy())
		{
			return;
		}
	}

	//No weapon equipped or couldn't deploy it, find a suitable alternative.
	g_pGameRules->GetNextBestWeapon(this, m_pActiveItem, true);
}

void CBasePlayer::SetPrefsFromUserinfo(char* infobuffer)
{
	// TS parity: the original server never reads cl_autowepswitch (no such string in
	// ts_i386.so) and the original client never registers/sends it — the whole userinfo
	// preference read is an SDK addition, removed. Auto-switch keeps the stock always-on
	// behavior (m_iAutoWepSwitch = 1 = the SDK-2.3 weight rule; CTSGameRules' own switch
	// logic is the original mechanism).
	m_iAutoWepSwitch = 1;
}

//=========================================================
// Dead HEV suit prop
//=========================================================
class CDeadHEV : public CBaseMonster
{
public:
	void Spawn() override;
	int Classify() override { return CLASS_HUMAN_MILITARY; }

	bool KeyValue(KeyValueData* pkvd) override;

	int m_iPose; // which sequence to display	-- temporary, don't need to save
	static const char* m_szPoses[4];
};

const char* CDeadHEV::m_szPoses[] = {"deadback", "deadsitting", "deadstomach", "deadtable"};

bool CDeadHEV::KeyValue(KeyValueData* pkvd)
{
	if (FStrEq(pkvd->szKeyName, "pose"))
	{
		m_iPose = atoi(pkvd->szValue);
		return true;
	}

	return CBaseMonster::KeyValue(pkvd);
}

LINK_ENTITY_TO_CLASS(monster_hevsuit_dead, CDeadHEV);

//=========================================================
// ********** DeadHEV SPAWN **********
//=========================================================
void CDeadHEV::Spawn()
{
	PRECACHE_MODEL("models/player.mdl");
	SET_MODEL(ENT(pev), "models/player.mdl");

	pev->effects = 0;
	pev->yaw_speed = 8;
	pev->sequence = 0;
	pev->body = 1;
	m_bloodColor = BLOOD_COLOR_RED;

	pev->sequence = LookupSequence(m_szPoses[m_iPose]);

	if (pev->sequence == -1)
	{
		ALERT(at_console, "Dead hevsuit with bad pose\n");
		pev->sequence = 0;
		pev->effects = EF_BRIGHTFIELD;
	}

	// Corpses have less health
	pev->health = 8;

	MonsterInitDead();
}


class CStripWeapons : public CPointEntity
{
public:
	void Use(CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value) override;

private:
};

LINK_ENTITY_TO_CLASS(player_weaponstrip, CStripWeapons);

void CStripWeapons::Use(CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value)
{
	CBasePlayer* pPlayer = NULL;

	if (pActivator && pActivator->IsPlayer())
	{
		pPlayer = (CBasePlayer*)pActivator;
	}
	else if (!g_pGameRules->IsDeathmatch())
	{
		pPlayer = (CBasePlayer*)UTIL_GetLocalPlayer();
	}

	if (pPlayer)
		pPlayer->RemoveAllItems(false);
}


class CRevertSaved : public CPointEntity
{
public:
	void Use(CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value) override;
	void EXPORT MessageThink();
	void EXPORT LoadThink();
	bool KeyValue(KeyValueData* pkvd) override;

	bool Save(CSave& save) override;
	bool Restore(CRestore& restore) override;
	static TYPEDESCRIPTION m_SaveData[];

	inline float Duration() { return pev->dmg_take; }
	inline float HoldTime() { return pev->dmg_save; }
	inline float MessageTime() { return m_messageTime; }
	inline float LoadTime() { return m_loadTime; }

	inline void SetDuration(float duration) { pev->dmg_take = duration; }
	inline void SetHoldTime(float hold) { pev->dmg_save = hold; }
	inline void SetMessageTime(float time) { m_messageTime = time; }
	inline void SetLoadTime(float time) { m_loadTime = time; }

private:
	float m_messageTime;
	float m_loadTime;
};

LINK_ENTITY_TO_CLASS(player_loadsaved, CRevertSaved);

TYPEDESCRIPTION CRevertSaved::m_SaveData[] =
	{
		DEFINE_FIELD(CRevertSaved, m_messageTime, FIELD_FLOAT), // These are not actual times, but durations, so save as floats
		DEFINE_FIELD(CRevertSaved, m_loadTime, FIELD_FLOAT),
};

IMPLEMENT_SAVERESTORE(CRevertSaved, CPointEntity);

bool CRevertSaved::KeyValue(KeyValueData* pkvd)
{
	if (FStrEq(pkvd->szKeyName, "duration"))
	{
		SetDuration(atof(pkvd->szValue));
		return true;
	}
	else if (FStrEq(pkvd->szKeyName, "holdtime"))
	{
		SetHoldTime(atof(pkvd->szValue));
		return true;
	}
	else if (FStrEq(pkvd->szKeyName, "messagetime"))
	{
		SetMessageTime(atof(pkvd->szValue));
		return true;
	}
	else if (FStrEq(pkvd->szKeyName, "loadtime"))
	{
		SetLoadTime(atof(pkvd->szValue));
		return true;
	}

	return CPointEntity::KeyValue(pkvd);
}

void CRevertSaved::Use(CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value)
{
	UTIL_ScreenFadeAll(pev->rendercolor, Duration(), HoldTime(), pev->renderamt, FFADE_OUT);
	pev->nextthink = gpGlobals->time + MessageTime();
	SetThink(&CRevertSaved::MessageThink);
}


void CRevertSaved::MessageThink()
{
	UTIL_ShowMessageAll(STRING(pev->message));
	float nextThink = LoadTime() - MessageTime();
	if (nextThink > 0)
	{
		pev->nextthink = gpGlobals->time + nextThink;
		SetThink(&CRevertSaved::LoadThink);
	}
	else
		LoadThink();
}


void CRevertSaved::LoadThink()
{
	if (0 == gpGlobals->deathmatch)
	{
		SERVER_COMMAND("reload\n");
	}
}


//=========================================================
// Multiplayer intermission spots.
//=========================================================
class CInfoIntermission : public CPointEntity
{
	void Spawn() override;
	void Think() override;
};

void CInfoIntermission::Spawn()
{
	UTIL_SetOrigin(pev, pev->origin);
	pev->solid = SOLID_NOT;
	pev->effects = EF_NODRAW;
	pev->v_angle = g_vecZero;

	pev->nextthink = gpGlobals->time + 2; // let targets spawn!
}

void CInfoIntermission::Think()
{
	edict_t* pTarget;

	// find my target
	pTarget = FIND_ENTITY_BY_TARGETNAME(NULL, STRING(pev->target));

	if (!FNullEnt(pTarget))
	{
		pev->v_angle = UTIL_VecToAngles((pTarget->v.origin - pev->origin).Normalize());
		pev->v_angle.x = -pev->v_angle.x;
	}
}

LINK_ENTITY_TO_CLASS(info_intermission, CInfoIntermission);
