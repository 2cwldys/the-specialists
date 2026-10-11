/***
*  ts_bullet.cpp — RECONSTRUCTED slow-motion "bullets as models" (CBulletManager, the original's
*  256-slot ring at CTSGameRules+0x1864). When the shooter is in slow-motion, their shots stop being
*  instant hitscans and become tracked projectiles that physically FLY across the map, doing damage
*  when/where they actually arrive — so in slow-PAUSE you can watch a bullet crawl toward you and step
*  out of its path. A bullet in flight keeps flying when the shooter's slow-motion ends (and with
*  realbullet on, every bullet flies, slowed shooter or not).
*
*  RE: ShootBulletsPlayer (@0x5f69c) gates on `rbullet` = (the shooter's slow factor < 0.99, or the
*  realbullet setting) and, when set, hands each pellet to CBulletManager::FireBullet instead of
*  tracing+damaging it instantly.
*  CBulletManager::Think (@0x5ec7c) moves each bullet whose next-update time has passed (UpdateBullet
*  @0x5ef5c), every 0.1/(realbullet+1) s with realbullet 0 or 1 and every frame otherwise: advance by
*  GetBulletVelocityMult(slow, pauseFlag) * speed * that interval, trace, damage on hit (same pierce model),
*  else fly on. GetBulletVelocityMult (@0x89160, below) is the speed fraction.
*
*  The server draws nothing for a flying bullet (no beam, sprite or temporary entity: FireBullet and
*  UpdateBullet send no message); the client draws it. On a hit the bullet pierces on as
*  ShootBulletsPlayer's shots do (UpdateBullet @0x5f384); the server plays no ricochet and lays no
*  decal, and a bullet that hits nothing never ends for range.
***/
#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "player.h"
#include "weapons.h"
#include "ts_player.h"
#include "ts_weapons.h"
#include "ts_weapon_info.h" // get_ammo_info (the caliber speed unit) // TSSlowPitch (slow-world sound pitch)
#include "ts_bullet.h"
#include "ts_gun.h" // GetPlayerTSGun (the fire-time weapon-id capture)
#include "ts_shared.h"   // GetBulletVelocityMult (the slow-motion bullet math, original TS_shared.cpp)
#include "game.h"        // g_flWeaponCheat (sv_cheats as the map started)
#include "ts_gamerules.h" // TSGameRules->m_iSrvRealBullet (Think's step interval)

CBulletManager g_BulletManager; // the original embeds this in CTSGameRules (+0x1864); one instance.
int glast_weaponid = -1; // @0x10100c (this TU, static-init -1 — V21 resolved from the .data image)

// GetBulletVelocityMult (@0x89160) now lives in its original home, ts_shared.cpp (the TS slow-motion
// math). A slow-mo bullet's speed as a fraction of full speed, from the shooter's slow factor + the
// slow-PAUSE flag. Slow-mo (pause=0) → ~slow (a fast tracer); slow-pause (×10) → dodgeable; ≥0.99 → 1.

// TSSlowBullet + CBulletManager are declared in ts_bullet.h; the 256-slot ring is m_bullets.

// Per-caliber bullet flight speed unit, compile_ammo_info's ammo_info[+0x24]. Full speed is 40 x this
// (UpdateBullet @0x5f016; about 13k to 40k u/s). Index = caliber (1..13; 0/14/15 have none, so 0
// speed, and no weapon that fires bullets uses them). Calibers: 1 9x19, 2 .45, 3 SHELLS, 4 5.56,
// 5 .50AE, 6 7.62, 7 5.7, 8 50BMG, 9 10mm, 10 .22, 11 .454, 12 .32, 13 .45LC.
static float BulletFullSpeed(int caliber)
{
	return 40.0f * (float)get_ammo_info(caliber)->iSpeedUnit;
}

// @0x5ef40 DestroyBullet — the slot goes back to empty (all sixteen words zeroed).
void CBulletManager::DestroyBullet(int slot)
{
	m_bullets[slot] = TSSlowBullet();
}

// @0x5f4f4 UTIL_TraceLine_Expanded — inflate every live player's absbox by +/-32u so the
// swept bullet can't thread a player hull between frames, run the stock trace, then restore
// ONLY absmax (the original's literal behavior — absmin stays inflated until the engine
// relinks the player next frame). vecBoxes[33] = the per-player saved absmax.
Vector vecBoxes[33];

void UTIL_TraceLine_Expanded(const Vector &vecStart, const Vector &vecEnd,
	IGNORE_MONSTERS igmon, edict_t *pentIgnore, TraceResult *ptr)
{
	for (int i = 1; i <= gpGlobals->maxClients && i <= 32; i++)
	{
		CBaseEntity *p = UTIL_PlayerByIndex(i);
		if (!p)
			continue;
		vecBoxes[i] = p->pev->absmax;
		for (int a = 0; a < 3; a++)
		{
			p->pev->absmin[a] -= 32.0f;
			p->pev->absmax[a] += 32.0f;
		}
	}
	UTIL_TraceLine(vecStart, vecEnd, igmon, pentIgnore, ptr);
	for (int i = 1; i <= gpGlobals->maxClients && i <= 32; i++)
	{
		CBaseEntity *p = UTIL_PlayerByIndex(i);
		if (p)
			p->pev->absmax = vecBoxes[i]; // absmax ONLY — the original never restores absmin
	}
}

// @0x5eda4 FireBullet — register a shot as a flying bullet in the first free slot (called from
// ShootBulletsPlayer when the shooter is slowed or realbullet is on).
void CBulletManager::FireBullet(CBasePlayer *owner, const Vector &src, const Vector &dir, float damage, int caliber, int mode)
{
	for (int i = 0; i < 256; i++)
	{
		if (!m_bullets[i].active)
		{
			TSSlowBullet &b = m_bullets[i];
			b.active = 1;
			b.owner = owner;
			b.pos = src;
			b.start = src;
			b.dir = dir;
			// rec+0x34 (@0x5eec2): the shooter's slow factor; rec+0x38: no step taken yet
			b.slow = owner->m_flSlowMotionRamp != 0.0f ? owner->m_flSlowMotionRamp : owner->GiveSlowMul();
			b.nextUpdate = 0.0f;
			b.damage = damage;
			b.fullSpeed = BulletFullSpeed(caliber);
			b.caliber = caliber;
			b.kind = mode; // rec+0x3c — the shot's mode (@0x5ef2e)
			{
				CTSGun *pGun = GetPlayerTSGun(owner);
				b.weaponId = pGun ? pGun->m_weaponType : -1; // rec+0x30 — captured at fire time
			}
			m_iActive++; // @0x5ef32
			return;
		}
	}
	// @0x5edf8: every slot is taken; the shot is dropped, and everyone is told (talk)
	UTIL_ClientPrintAll(HUD_PRINTTALK, "WARNING:cannot create bullet!\n");
}

// @ Reset__14CBulletManager (@0x5ec50) — clear every slot. The original zeroes the whole ring and
// the two words after it (+0x4000, the active count m_iActive; +0x4004, which reTS does not keep);
// CTSGameRules::Init calls it as each map starts (@0x67e83).
void CBulletManager::Reset()
{
	for (int i = 0; i < 256; i++)
		m_bullets[i] = TSSlowBullet();
	m_iActive = 0; // +0x4000 (@0x5ec66)
}

// @ Think__14CBulletManager (@0x5ec7c) — per frame (from CTSGameRules::Think): a bullet whose
// next-update time has passed moves now. With realbullet 0 or 1 (the rules' cached setting, as
// unsigned, < 2) it then waits 0.1/(realbullet+1) s and the step covers that interval; with any
// other value it moves every frame by the frame's time.
void CBulletManager::Think()
{
	CTSGameRules *rules = TSGameRules;
	TraceResult tr;
	m_iActive = 0; // @0x5ec96: recounted here, each active bullet whether or not it moves this frame
	for (int i = 0; i < 256; i++)
	{
		TSSlowBullet &b = m_bullets[i];
		if (!b.active)
			continue;
		if (b.nextUpdate < gpGlobals->time)
		{
			b.nextUpdate = gpGlobals->time;
			if ((unsigned int)rules->m_iSrvRealBullet < 2)
				b.nextUpdate += 0.1f / (float)(rules->m_iSrvRealBullet + 1);
			UpdateBullet(i, &tr);
		}
		m_iActive++; // @0x5ed0c
	}
}

// @ UpdateBullet__14CBulletManager (@0x5ef5c) — advance one bullet one time slice: trace, damage on
// a hit, then it pierces on or ends. The original never drops a bullet for its shooter's slow state:
// a slowed shooter's bullets fly on after his slow-motion ends, and realbullet's bullets (shooter not
// slowed) fly at GetBulletVelocityMult's full speed until they hit.
void CBulletManager::UpdateBullet(int slot, TraceResult *ptr)
{
	TSSlowBullet &b = m_bullets[slot];
	if (!b.active)
		return;
	CBasePlayer *owner = (CBasePlayer *)(CBaseEntity *)b.owner;
	// Kept: a shooter who left drops his bullets. The original keeps the raw pointer and steps on.
	if (!owner || !owner->IsPlayer())
	{
		DestroyBullet(slot);
		return;
	}
	// @0x5ef9b: the shooter's slow factor now — m_flSlowMotionRamp (+0x180) when set, else his
	// GiveSlowMul (vtable 0x10) — and the slow-pause byte (+0x175) choose the speed fraction.
	b.slow = owner->m_flSlowMotionRamp != 0.0f ? owner->m_flSlowMotionRamp : owner->GiveSlowMul();
	const float velMult = GetBulletVelocityMult(b.slow, owner->m_bSlowBullets);
	// @0x5eff1: the step covers the time until the next update Think just set, or this frame's
	// time when that is under a millisecond (realbullet 2 and up: every frame).
	float dt = b.nextUpdate - gpGlobals->time;
	if (dt < 0.001f)
		dt = gpGlobals->frametime;
	float speed = b.fullSpeed * velMult;
	Vector newpos = b.pos + b.dir * (speed * dt);

	TraceResult &tr = *ptr;
	// the TS bullet trace: player absboxes inflated so a fast bullet can't thread a hull
	// (glass still hit — the plain 4-arg trace never ignores glass).
	UTIL_TraceLine_Expanded(b.pos, newpos, dont_ignore_monsters, owner->edict(), &tr);

	if (tr.flFraction >= 1.0f)
	{
		b.pos = newpos;
		return; // @0x5f4b4: a bullet that hits nothing flies on, however far it has gone
	}

	// Hit something — apply the pellet's damage; then it pierces on, or stops.
	b.pos = tr.vecEndPos;

	CBaseEntity *pHit = CBaseEntity::Instance(tr.pHit);
	bool pierced = false;
	if (pHit)
	{
		// range-damage falloff (@0x5f1ba/@0x5f24a): kind 2 (rifle/shotgun calibers) -> form A
		// idx = dist/30 (full damage to 300u); else form B idx = dist/rangeUnit (full damage
		// to 10x the caliber's range unit); damage *= clamp(1/sqrt(0.1 idx), 0, 1). The index is
		// truncated, not rounded (fistp under a chop control word, @0x5f1ad/@0x5f23d).
		const float dist = (tr.vecEndPos - b.start).Length();
		int idx;
		if (b.kind == 2)
			idx = (int)(dist * (1.0f / 300.0f) * 10.0f);
		else
		{
			const s_ammo_info *ai = get_ammo_info(b.caliber);
			idx = (ai->iRangeUnit > 0) ? (int)(10.0f * (dist / (10.0f * (float)ai->iRangeUnit))) : 0;
		}
		float dmg = b.damage * get_range_multipler(idx);
		// `damagemult` (@0x5f256): cheat-gated, as in ShootBulletsPlayer, and not for kind 2
		// (that branch jumps past it, @0x5f1c9).
		float mult = 1.0f;
		if (b.kind != 2)
		{
			const float dmgMult = CVAR_GET_FLOAT("damagemult");
			if (dmgMult > 0.0f && g_flWeaponCheat != 0)
				mult = dmgMult;
		}
		glast_weaponid = b.weaponId; // @0x5f2a4 — publish what fired, for the kill feed / scoring
		ClearMultiDamage();
		// @0x5f287: mode 1 adds DMG bit 25, as in ShootBulletsPlayer
		const int dmgFlags = (b.kind == 1) ? (DMG_BULLET | DMG_NEVERGIB | (1 << 25)) : (DMG_BULLET | DMG_NEVERGIB);
		pHit->TraceAttack(owner->pev, dmg * mult, b.dir, &tr, dmgFlags);
		ApplyMultiDamage(owner->pev, owner->pev);
		glast_weaponid = -1; // @0x5f311 — reset RIGHT after the damage window (PlayerKilled runs
		                     // inside ApplyMultiDamage); leaving it armed misattributes later kills
		if (g_flWeaponCheat != 0 && CVAR_GET_FLOAT("dmgreport") > 1.0f) // @0x5f317
			UTIL_ClientPrintAll(HUD_PRINTTALK, UTIL_VarArgs("damage:%f\n", dmg * mult));

		// @0x5f384 the pierce, as ShootBulletsPlayer's: what is left is the damage times the ammo's
		// pierce column (+0x18), halved through a player; a bullet of any mode but 1 and 2 with more
		// than 5 left goes on from min(left, 48) units past the hit, unless that point is in solid
		// world, as a mode 2 bullet carrying left * damagemult.
		float left = dmg * get_ammo_info(b.caliber)->flFactor2;
		if (pHit->IsPlayer())
			left *= 0.5f;
		if ((unsigned int)(b.kind - 1) > 1 && left > 5.0f)
		{
			const Vector past = tr.vecEndPos + b.dir * (left > 48.0f ? 48.0f : left);
			if (UTIL_PointContents(past) != CONTENTS_SOLID)
			{
				b.pos = past;
				b.start = past;
				b.damage = left * mult;
				b.kind = 2;
				pierced = true;
			}
		}
	}
	// The server plays no ricochet and lays no decal for a bullet (the client does both from the
	// shot's event); a bullet that does not pierce ends here.
	if (!pierced)
		DestroyBullet(slot);
}

// @0x5f4f0 UpdateBulletTimeSlice — a lone ret in the original; nothing calls it.
void CBulletManager::UpdateBulletTimeSlice(float flTime, CBasePlayer *pPlayer)
{
}
