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
*  SLICEs (first pass): (1) the visible bullet is the engine TE_TRACER streak drawn each step (no custom
*  client bullet-renderer / model); (3) the full bullet speed is approximated (the original's per-ammo
*  40*ammo_info[+0x24] base isn't pinned) — TUNE; (4) range-falloff damage uses the raw pellet damage.
*  On a hit the bullet pierces on as ShootBulletsPlayer's shots do (UpdateBullet @0x5f384); the server
*  plays no ricochet and lays no decal, and a bullet that hits nothing never ends for range.
***/
#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "player.h"
#include "weapons.h"
#include "effects.h" // CBeam — the extending bullet-trail
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

// Per-caliber bullet flight speed unit, RE'd from compile_ammo_info (ammo_info[+0x24]). Full speed is
// the literal original 40 * this (≈13k–40k u/s) — the muzzle→bullet growing trail keeps the path
// visible even at these speeds. Index = caliber (1..13; 0/14/15 = none). Calibers: 1 9x19, 2 .45,
// 3 SHELLS, 4 5.56, 5 .50AE, 6 7.62, 7 5.7, 8 50BMG, 9 10mm, 10 .22, 11 .454, 12 .32, 13 .45LC.
static float BulletFullSpeed(int caliber)
{
	int unit = get_ammo_info(caliber)->iSpeedUnit; // +0x24, caliber-ordered (the old local table was in compile-WRITE order)
	if (unit == 0)
		unit = 350; // fallback (~10mm) for melee/grenade/unknown
	return 40.0f * (float)unit; // RE-faithful 40 * ammo_info[+0x24]
}

// @ DestroyBullet — free one slot's trail/head sprites (called when a bullet ends).
void CBulletManager::DestroyBullet(int slot)
{
	TSSlowBullet &b = m_bullets[slot];
	CBaseEntity *e = b.beam;
	if (e)
		UTIL_Remove(e);
	b.beam = NULL;
	CBaseEntity *f = b.flash;
	if (f)
		UTIL_Remove(f);
	b.flash = NULL;
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

// @ FireBullet__14CBulletManager — register a shot as a flying bullet (called from the fire path when
// the shooter is slowed). Spawns the trail beam anchored at the muzzle; UpdateBullet extends its end.
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
			b.dir = dir.Normalize();
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

			// The visible flying bullet, RE-matched to the client renderer (FUN_10035010): a SHORT
			// trail dash (bullet_trail.spr, ~5..25u, speed-scaled) that RIDES the bullet head — NOT a
			// long muzzle→current beam (which reads as a static line). UpdateBullet drags both ends each
			// frame so the dash flies with the bullet.
			// RE bullet visual (client FUN_10035010): a short bullet_trail.spr dash + a shotgun_pellets.spr
			// head sprite, both at NATIVE color (the warm amber comes from the sprite textures themselves —
			// render white + additive so they look like the original, not a tint).
			CBeam *beam = CBeam::BeamCreate("sprites/bullet_trail.spr", 10);
			if (beam)
			{
				beam->PointsInit(src, src);
				beam->SetColor(255, 255, 255); // native sprite color (warm amber tracer)
				beam->SetBrightness(255);
				beam->SetNoise(0);
				b.beam = beam;
			}
			// The bullet head: shotgun_pellets.spr (what the original client draws at the round's position).
			CSprite *flash = CSprite::SpriteCreate("sprites/shotgun_pellets.spr", src, 0);
			if (flash)
			{
				flash->SetTransparency(kRenderGlow, 255, 255, 255, 255, kRenderFxNoDissipation); // native, full bright
				flash->SetScale(0.25f);
				UTIL_SetOrigin(flash->pev, src);
				b.flash = flash;
			}
			return;
		}
	}
}

// @ Reset__14CBulletManager — drop every in-flight bullet (round restart).
void CBulletManager::Reset()
{
	for (int i = 0; i < 256; i++)
	{
		DestroyBullet(i);
		m_bullets[i].active = 0;
	}
}

// @ Think__14CBulletManager (@0x5ec7c) — per frame (from CTSGameRules::Think): a bullet whose
// next-update time has passed moves now. With realbullet 0 or 1 (the rules' cached setting, as
// unsigned, < 2) it then waits 0.1/(realbullet+1) s and the step covers that interval; with any
// other value it moves every frame by the frame's time.
void CBulletManager::Think()
{
	CTSGameRules *rules = TSGameRules;
	for (int i = 0; i < 256; i++)
	{
		TSSlowBullet &b = m_bullets[i];
		if (!b.active || b.nextUpdate >= gpGlobals->time)
			continue;
		b.nextUpdate = gpGlobals->time;
		if (rules && (unsigned int)rules->m_iSrvRealBullet < 2)
			b.nextUpdate += 0.1f / (float)(rules->m_iSrvRealBullet + 1);
		UpdateBullet(i);
	}
}

// @ UpdateBullet__14CBulletManager (@0x5ef5c) — validate one bullet's owner, then advance it one
// time slice. The original never drops a bullet for its shooter's slow state: a slowed shooter's
// bullets fly on after his slow-motion ends, and realbullet's bullets (shooter not slowed) fly at
// GetBulletVelocityMult's full speed until they hit.
void CBulletManager::UpdateBullet(int slot)
{
	TSSlowBullet &b = m_bullets[slot];
	if (!b.active)
		return;
	CBasePlayer *owner = (CBasePlayer *)(CBaseEntity *)b.owner;
	if (!owner || !owner->IsPlayer())
	{
		DestroyBullet(slot);
		b.active = 0;
		return;
	}
	UpdateBulletTimeSlice(slot);
}

// @ UpdateBulletTimeSlice — fly one bullet across this frame's slice: trace, draw the trail, and
// damage on hit (SLICE: a single step per frame; no pierce re-seed).
void CBulletManager::UpdateBulletTimeSlice(int slot)
{
	TSSlowBullet &b = m_bullets[slot];
	CBasePlayer *owner = (CBasePlayer *)(CBaseEntity *)b.owner;
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

	TraceResult tr;
	// the TS bullet trace: player absboxes inflated so a fast bullet can't thread a hull
	// (glass still hit — the plain 4-arg trace never ignores glass).
	UTIL_TraceLine_Expanded(b.pos, newpos, dont_ignore_monsters, owner->edict(), &tr);

	if (tr.flFraction >= 1.0f)
	{
		b.pos = newpos; // flew on — drag the trail end + head sprite to the new position.
		CBaseEntity *beam = b.beam;
		if (beam)
			((CBeam *)beam)->SetEndPos(b.pos);
		CBaseEntity *flash = b.flash;
		if (flash)
			UTIL_SetOrigin(flash->pev, b.pos);
		return; // @0x5f4b4: a bullet that hits nothing flies on, however far it has gone
	}

	// Hit something — extend the trail to the impact and apply the pellet's damage; then it
	// pierces on, or stops.
	b.pos = tr.vecEndPos;
	CBaseEntity *beam = b.beam;
	if (beam)
		((CBeam *)beam)->SetEndPos(b.pos);

	CBaseEntity *pHit = CBaseEntity::Instance(tr.pHit);
	bool pierced = false;
	if (pHit)
	{
		// range-damage falloff (@0x5f1ba/@0x5f24a): kind 2 (rifle/shotgun calibers) -> form A
		// idx = dist/30 (full damage to 300u); else form B idx = dist/rangeUnit (full damage
		// to 10x the caliber's range unit); damage *= clamp(1/sqrt(0.1 idx), 0, 1).
		const float dist = (tr.vecEndPos - b.start).Length();
		int idx;
		if (b.kind == 2)
			idx = (int)(dist * (1.0f / 300.0f) * 10.0f + 0.5f);
		else
		{
			const s_ammo_info *ai = get_ammo_info(b.caliber);
			idx = (ai->iRangeUnit > 0) ? (int)(10.0f * (dist / (10.0f * (float)ai->iRangeUnit)) + 0.5f) : 0;
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
	{
		DestroyBullet(slot);
		b.active = 0;
	}
}
