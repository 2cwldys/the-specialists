/***
*  TS_combat.cpp — CBasePlayer::ShootBulletsPlayer, the TS bullet path.
*  Functional/source-faithful reconstruction (see CLAUDE.md). The original TU holds
*  exactly this one function (funcfile.tsv); decompile @0x5f69c (ghidra 0x6f69c),
*  argument packing recovered from the StandardFire call site @0x72759.
***/

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "player.h"
#include "weapons.h"
#include "gamerules.h"
#include "ts_weapons.h"   // s_ammo_info / get_ammo_info / get_range_multipler
#include "ts_bullet.h"    // g_BulletManager + UTIL_TraceLine_Expanded
#include "ts_gamerules.h" // CTSGameRules::m_iSrvRealBullet (the realbullet cache, rules+0x578)
#include "game.h"         // g_flWeaponCheat (sv_cheats as the map started)

// @ ShootBulletsPlayer__11CBasePlayerUcG6VectorN22fiii (@0x5f69c, decompile-exact) — fire cShots
// pellets with the TS spread/falloff/penetration model. Returns the LAST pellet's spread offsets
// (x*spread.x, y*spread.y, 0), like the stock FireBulletsPlayer contract.
//
// iMode (= the weapon's s_weapon_info+0x20 code; 0 for every firearm):
//   0 (or >2) — the normal volley; each pellet may PIERCE one extra surface via a depth-1
//               recursion with iMode=2;
//   1         — adds DMG bit 25 to the TraceAttack flags (no shipping caller — kept faithful);
//   2         — the PIERCE CONTINUATION leg: a single 4096u trace, falloff on the /30 form,
//               no further recursion.
//
// The rbullet gate (@0x5f6f0): the `realbullet` server setting OR the shooter slowed
// (GiveSlowMul() < 0.99) routes every pellet through CBulletManager::FireBullet — a tracked
// flying bullet (dodgeable in slow-motion) instead of an instant hitscan.
//
// Spread is POLAR with cubic shaping (NOT the stock gaussian double-sum): angle uniform in
// (-pi, pi], radius r in [0, ai->flMult (1.0)] shaped (1 - r^3) — center-weighted.
//
// Damage: range falloff idx = dist/iRangeUnit (the ammo's +0x20 column) through the LUT;
// `damagemult` is a CHEAT-GATED global multiplier (applies only while sv_cheats was on at map start —
// @0x5fb04); the pierce carry-over decays by ai->flFactor2 (+0x18 — its consumer) and HALVES
// through a player's body (IsPlayer, vtable slot 50). One ClearMultiDamage/ApplyMultiDamage
// batch brackets the WHOLE volley (a shotgun blast is one damage event).
//
// The client lays the impact decal + ric sound (EV_TSBulletImpact) — the server lays none.
Vector CBasePlayer::ShootBulletsPlayer(unsigned char cShots, Vector vecSrc, Vector vecDirShooting,
	Vector vecSpread, float flDamage, int iMode, int iCaliber, int shared_rand)
{
	float x = 0.0f, y = 0.0f;
	TraceResult tr;
	memset(&tr, 0, sizeof(tr));
	Vector vecDir = vecDirShooting;

	// the original caches v_right/v_up (and the trace state) in function statics at entry
	const Vector vecRight = gpGlobals->v_right;
	const Vector vecUp = gpGlobals->v_up;

	int rbullet = 0;
	CTSGameRules *pRules = TSGameRules; // @0x5f741: TSGameRules+0x578
	if ((pRules && pRules->m_iSrvRealBullet != 0) || GiveSlowMul() < 0.99f)
		rbullet = 1;

	const s_ammo_info *ai = get_ammo_info(iCaliber);

	if (!rbullet)
		ClearMultiDamage(); // ONE batch for the whole volley
	gMultiDamage.type = DMG_BULLET | DMG_NEVERGIB; // 0x1002

	float flDamageAmount = 0.0f;
	float flDamageAdjust = 1.0f;

	if (iMode == 2)
	{
		// THE PIERCE CONTINUATION (depth 1): one 4096u trace from just past the pierced
		// surface; falloff on the /30 form regardless of the ammo's range unit.
		Vector vecEnd = vecSrc + vecDirShooting * 4096.0f;
		UTIL_TraceLine_Expanded(vecSrc, vecEnd, dont_ignore_monsters, ENT(pev), &tr);
		if (tr.flFraction != 1.0f)
		{
			CBaseEntity *pEntity = CBaseEntity::Instance(tr.pHit);
			const float dist = (vecSrc - tr.vecEndPos).Length();
			const int idx = (int)(dist * (1.0f / 300.0f) * 10.0f + 0.5f);
			flDamageAmount = get_range_multipler(idx) * flDamage;
			if (pEntity)
				pEntity->TraceAttack(pev, flDamageAmount, vecDirShooting, &tr,
					DMG_BULLET | DMG_NEVERGIB);
			if (g_flWeaponCheat != 0 && CVAR_GET_FLOAT("dmgreport") > 1.0f) // @0x5f9ad
				UTIL_ClientPrintAll(HUD_PRINTTALK,
					UTIL_VarArgs("damage(piercing):%f\n", flDamageAmount));
		}
		UTIL_BubbleTrail(vecSrc, tr.vecEndPos, (int)(4096.0f * tr.flFraction * 0.015625f + 0.5f));
		ApplyMultiDamage(pev, pev);
		return vecSpread; // the pierce leg returns its spread param unchanged
	}

	for (int iShot = 1; iShot <= (int)cShots; iShot++)
	{
		const unsigned int seed = (unsigned int)(iShot + shared_rand);
		// polar spread, cubic-shaped toward the center
		const float ang = UTIL_SharedRandomFloat(seed, -3.1415927f, 3.1415927f);
		const float r = UTIL_SharedRandomFloat(seed + 1, 0.0f, ai->flMult);
		const float shape = 1.0f - r * r * r;
		x = cos(ang) * shape;
		y = sin(ang) * shape;
		vecDir = vecDirShooting + x * vecSpread.x * vecRight + y * vecSpread.y * vecUp;
		Vector vecEnd = vecSrc + vecDir * (float)ai->iMaxTravel; // +0x14 (8192)

		if (!rbullet)
		{
			// the TS bullet trace (player absboxes inflated +/-32u)
			UTIL_TraceLine_Expanded(vecSrc, vecEnd, dont_ignore_monsters, ENT(pev), &tr);
			if (tr.flFraction != 1.0f)
			{
				CBaseEntity *pEntity = CBaseEntity::Instance(tr.pHit);
				const float dist = (vecSrc - tr.vecEndPos).Length();
				const int idx = (ai->iRangeUnit > 0)
					? (int)((dist / (10.0f * (float)ai->iRangeUnit)) * 10.0f + 0.5f) : 0;
				flDamageAmount = get_range_multipler(idx) * flDamage;
				// `damagemult` — CHEAT-GATED global damage multiplier (@0x5fb04)
				const float dmgMult = CVAR_GET_FLOAT("damagemult");
				if (dmgMult > 0.0f && g_flWeaponCheat != 0) // @0x5fdf7
					flDamageAdjust = dmgMult;
				const int dmgFlags = (iMode == 1)
					? (DMG_BULLET | DMG_NEVERGIB | (1 << 25)) // mode-1 bit (no shipping caller)
					: (DMG_BULLET | DMG_NEVERGIB);
				if (pEntity)
				{
					pEntity->TraceAttack(pev, flDamageAmount * flDamageAdjust, vecDir, &tr, dmgFlags);
					if (g_flWeaponCheat != 0 && CVAR_GET_FLOAT("dmgreport") > 1.0f) // @0x5fee2
						UTIL_ClientPrintAll(HUD_PRINTTALK,
							UTIL_VarArgs("damage:%f\n", flDamageAmount * flDamageAdjust));
				}
				// The pierce carry-over decays on ANY hit (flFraction != 1.0), UNCONDITIONALLY — RE
				// ShootBulletsPlayer @0x5f69c applies `*= flFactor2` OUTSIDE the pEntity path. Wrapping it
				// inside `if (pEntity)` let a hit whose CBaseEntity::Instance(tr.pHit) is null keep UNDECAYED
				// damage feeding the pierce step -> shotguns (flFactor2 = 0.0 -> should NEVER pierce) and
				// low-flFactor2 ammo (9mm/.45 = 0.2) wrongly punched through walls (user report). The body
				// half-damage still needs a real player, so it stays gated on pEntity.
				flDamageAmount *= ai->flFactor2; // +0x18 — the ammo's pierce-decay column
				if (pEntity && pEntity->IsPlayer())
					flDamageAmount *= 0.5f;
			}
		}
		else
		{
			// realbullet / slow-motion: the pellet flies as a tracked bullet (rules+0x1864)
			g_BulletManager.FireBullet(this, vecSrc, vecDir, flDamage, iCaliber, iMode); // @0x5fc90
		}
		UTIL_BubbleTrail(vecSrc, tr.vecEndPos,
			(int)((float)ai->iMaxTravel * tr.flFraction * 0.015625f + 0.5f));
	}

	if (!rbullet)
	{
		ApplyMultiDamage(pev, pev); // the whole volley lands as one damage event
		// PIERCE: step min(2*dmg, 48) past the last hit; if that point isn't inside world
		// solid, continue as a single mode-2 pellet (depth-1 recursion — max two surfaces).
		if ((iMode == 0 || iMode > 2) && flDamageAmount > 5.0f)
		{
			float step = flDamageAmount * 2.0f;
			if (step > 48.0f)
				step = 48.0f;
			Vector vecPierceSrc = tr.vecEndPos + vecDir * step;
			if (UTIL_PointContents(vecPierceSrc) != CONTENTS_SOLID)
				ShootBulletsPlayer(1, vecPierceSrc, vecDir,
					Vector(x * vecSpread.x, y * vecSpread.y, 0.0f),
					flDamageAmount * flDamageAdjust, 2, iCaliber, shared_rand);
		}
	}

	return Vector(x * vecSpread.x, y * vecSpread.y, 0.0f);
}
