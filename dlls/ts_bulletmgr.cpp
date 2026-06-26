/***
*  ts_bulletmgr.cpp — RECONSTRUCTED slow-motion "bullets as models" (CBulletManager, the original's
*  256-slot ring at CTSGameRules+0x1864). When the shooter is in slow-motion, their shots stop being
*  instant hitscans and become tracked projectiles that physically FLY across the map, doing damage
*  when/where they actually arrive — so in slow-PAUSE you can watch a bullet crawl toward you and step
*  out of its path. The instant the shooter's slow-motion ends, the in-flight bullets vanish.
*
*  RE: ShootBulletsPlayer (@0x5f69c) gates on `rbullet` = (the shooter's slow factor < 0.99) and, when
*  set, hands each pellet to CBulletManager::FireBullet instead of tracing+damaging it instantly.
*  CBulletManager::Think (@0x5ec7c) steps every active bullet (UpdateBullet @0x6ef5c): advance by
*  GetBulletVelocityMult(slow, pauseFlag) * speed * frametime, trace, damage on hit (same pierce model),
*  else fly on. GetBulletVelocityMult (@0x89160, below) is the speed fraction.
*
*  SLICEs (first pass): (1) the visible bullet is the engine TE_TRACER streak drawn each step (no custom
*  client bullet-renderer / model); (2) the pierce re-seed past a surface isn't reproduced (the slow
*  bullet stops at the first hit); (3) the full bullet speed is approximated (the original's per-ammo
*  40*ammo_info[+0x24] base isn't pinned) — TUNE; (4) range-falloff damage uses the raw pellet damage.
***/
#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "player.h"
#include "weapons.h"
#include "effects.h" // CBeam — the extending bullet-trail
#include "ts_player.h"

// @ GetBulletVelocityMult__Ffc (@0x89160, TS_shared.cpp): a slow-mo bullet's speed as a fraction of
// full bullet speed, from the shooter's slow factor + the slow-PAUSE flag. Slow-mo (pause=0) → ~slow
// (a fast tracer); slow-pause (pause=1, the ×10) → much slower (dodgeable). Not slowed (≥0.99) → 1.0.
static float GetBulletVelocityMult(float slow, int pause)
{
	if (slow >= 0.99f)
		return 1.0f;
	float denom = (1.0f - slow * slow) * (pause ? 10.0f : 1.0f);
	if (denom < 1.0f)
		denom = 1.0f;
	return slow / denom;
}

struct TSSlowBullet
{
	int active;
	EHANDLE owner;
	EHANDLE beam;    // the trail beam: muzzle → current position, extending as the bullet flies
	Vector pos, start, dir;
	float damage;
};
static TSSlowBullet g_slowBullets[256];

// Approximate full bullet speed (orig = 40 * ammo_info[+0x24], per-ammo; not pinned). TUNE.
static const float BULLET_FULL_SPEED = 6000.0f;

static void KillBeam(TSSlowBullet &b)
{
	CBaseEntity *e = b.beam;
	if (e)
		UTIL_Remove(e);
	b.beam = NULL;
}

// @ FireBullet__14CBulletManager — register a shot as a flying bullet (called from the fire path when
// the shooter is slowed). Spawns the trail beam anchored at the muzzle; UpdateBullet extends its end.
void TSFireSlowBullet(CBasePlayer *owner, const Vector &src, const Vector &dir, float damage)
{
	for (int i = 0; i < 256; i++)
	{
		if (!g_slowBullets[i].active)
		{
			TSSlowBullet &b = g_slowBullets[i];
			b.active = 1;
			b.owner = owner;
			b.pos = src;
			b.start = src;
			b.dir = dir.Normalize();
			b.damage = damage;

			// The visible "bullet tracing as it flies": a beam from the muzzle to the bullet, which
			// extends as the bullet travels (SetEndPos each frame). bullet_trail.spr is the TS trail.
			CBeam *beam = CBeam::BeamCreate("sprites/bullet_trail.spr", 8);
			if (beam)
			{
				beam->PointsInit(src, src);
				beam->SetColor(255, 200, 120); // warm tracer
				beam->SetBrightness(200);
				beam->SetNoise(0);
				b.beam = beam;
			}
			return;
		}
	}
}

// @ Reset__14CBulletManager — drop every in-flight bullet (round restart).
void TSResetSlowBullets()
{
	for (int i = 0; i < 256; i++)
	{
		KillBeam(g_slowBullets[i]);
		g_slowBullets[i].active = 0;
	}
}

// @ Think__14CBulletManager — per frame (from CTSGameRules::Think): fly every active bullet, trace,
// damage on hit, draw its tracer. Bullets vanish the instant their shooter's slow-motion ends.
void TSUpdateSlowBullets()
{
	for (int i = 0; i < 256; i++)
	{
		TSSlowBullet &b = g_slowBullets[i];
		if (!b.active)
			continue;
		CBasePlayer *owner = (CBasePlayer *)(CBaseEntity *)b.owner;
		if (!owner || !owner->IsPlayer())
		{
			KillBeam(b);
			b.active = 0;
			continue;
		}
		float slow = owner->pev->fuser1;
		if (slow < 0.01f)
			slow = 0.01f;
		if (slow >= 0.99f) // slow-motion ended → the in-flight bullets (and their trails) disappear
		{
			KillBeam(b);
			b.active = 0;
			continue;
		}

		float speed = BULLET_FULL_SPEED * GetBulletVelocityMult(slow, owner->m_bSlowBullets);
		Vector newpos = b.pos + b.dir * (speed * gpGlobals->frametime);

		TraceResult tr;
		// dont_ignore_glass: slow-motion bullets HIT glass (so you see them break/embed it) — unlike the
		// instant penetration path which passes through. (User: slow-mo bullets were ignoring glass.)
		UTIL_TraceLine(b.pos, newpos, dont_ignore_monsters, dont_ignore_glass, owner->edict(), &tr);

		if (tr.flFraction >= 1.0f)
		{
			b.pos = newpos; // flew on — extend the trail beam to the new position
			CBaseEntity *beam = b.beam;
			if (beam)
				((CBeam *)beam)->SetEndPos(b.pos);
			if (UTIL_PointContents(b.pos) == CONTENTS_SOLID || (b.pos - b.start).Length() > 8192.0f)
			{
				KillBeam(b);
				b.active = 0; // left the world / out of range
			}
			continue;
		}

		// Hit something — extend the trail to the impact, apply the pellet's damage at the real impact
		// point, leave the bullet hole, then stop (SLICE: no pierce re-seed).
		b.pos = tr.vecEndPos;
		CBaseEntity *beam = b.beam;
		if (beam)
			((CBeam *)beam)->SetEndPos(b.pos);

		CBaseEntity *pHit = CBaseEntity::Instance(tr.pHit);
		if (pHit)
		{
			ClearMultiDamage();
			pHit->TraceAttack(owner->pev, b.damage, b.dir, &tr, DMG_BULLET | DMG_NEVERGIB);
			ApplyMultiDamage(owner->pev, owner->pev);
		}
		DecalGunshot(&tr, BULLET_PLAYER_9MM); // the bullet hole (this was missing)
		KillBeam(b);
		b.active = 0;
	}
}
