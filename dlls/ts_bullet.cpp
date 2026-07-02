/***
*  ts_bullet.cpp — RECONSTRUCTED slow-motion "bullets as models" (CBulletManager, the original's
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
#include "ts_weapons.h" // TSSlowPitch (slow-world sound pitch)
#include "ts_bullet.h"
#include "ts_shared.h"   // GetBulletVelocityMult (the slow-motion bullet math, original TS_shared.cpp)

CBulletManager g_BulletManager; // the original embeds this in CTSGameRules (+0x1864); one instance.

// GetBulletVelocityMult (@0x89160) now lives in its original home, ts_shared.cpp (the TS slow-motion
// math). A slow-mo bullet's speed as a fraction of full speed, from the shooter's slow factor + the
// slow-PAUSE flag. Slow-mo (pause=0) → ~slow (a fast tracer); slow-pause (×10) → dodgeable; ≥0.99 → 1.

// TSSlowBullet + CBulletManager are declared in ts_bullet.h; the 256-slot ring is m_bullets.

// Per-caliber bullet flight speed unit, RE'd from compile_ammo_info (ammo_info[+0x24]). Full speed is
// the literal original 40 * this (≈13k–40k u/s) — the muzzle→bullet growing trail keeps the path
// visible even at these speeds. Index = caliber (1..13; 0/14/15 = none). Calibers: 1 9x19, 2 .45,
// 3 SHELLS, 4 5.56, 5 .50AE, 6 7.62, 7 5.7, 8 50BMG, 9 10mm, 10 .22, 11 .454, 12 .32, 13 .45LC.
static const int kBulletSpeedUnit[16] = {
	0, 330, 480, 730, 280, 450, 900, 790, 1000, 350, 350, 400, 300, 850, 0, 0,
};
static float BulletFullSpeed(int caliber)
{
	int unit = (caliber >= 0 && caliber < 16) ? kBulletSpeedUnit[caliber] : 0;
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

// @ FireBullet__14CBulletManager — register a shot as a flying bullet (called from the fire path when
// the shooter is slowed). Spawns the trail beam anchored at the muzzle; UpdateBullet extends its end.
void CBulletManager::FireBullet(CBasePlayer *owner, const Vector &src, const Vector &dir, float damage, int caliber)
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
			b.damage = damage;
			b.fullSpeed = BulletFullSpeed(caliber);

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

// @ Think__14CBulletManager — per frame (from CTSGameRules::Think): step every active bullet.
void CBulletManager::Think()
{
	for (int i = 0; i < 256; i++)
		if (m_bullets[i].active)
			UpdateBullet(i);
}

// @ UpdateBullet__14CBulletManager — validate one bullet's owner/slow-state; bullets vanish the
// instant their shooter's slow-motion ends. Otherwise advance it one time slice.
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
	// RE-faithful slow: the bullet flies at GiveSlowMul = FixedSlowAmt(owner->fuser1) — max(fuser1,0.01).
	float slow = owner->pev->fuser1;
	if (slow >= 0.99f) // slow-motion ended → the in-flight bullets (and their trails) disappear
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
	float slow = owner->pev->fuser1;
	if (slow < 0.01f)
		slow = 0.01f;

	const float velMult = GetBulletVelocityMult(slow, owner->m_bSlowBullets);
	float speed = b.fullSpeed * velMult;
	Vector newpos = b.pos + b.dir * (speed * gpGlobals->frametime);

	TraceResult tr;
	// dont_ignore_glass: slow-motion bullets HIT glass (so you see them break/embed it).
	UTIL_TraceLine(b.pos, newpos, dont_ignore_monsters, dont_ignore_glass, owner->edict(), &tr);

	if (tr.flFraction >= 1.0f)
	{
		b.pos = newpos; // flew on — drag the trail end + head sprite to the new position.
		CBaseEntity *beam = b.beam;
		if (beam)
			((CBeam *)beam)->SetEndPos(b.pos);
		CBaseEntity *flash = b.flash;
		if (flash)
			UTIL_SetOrigin(flash->pev, b.pos);
		if (UTIL_PointContents(b.pos) == CONTENTS_SOLID || (b.pos - b.start).Length() > 8192.0f)
		{
			DestroyBullet(slot);
			b.active = 0; // left the world / out of range
		}
		return;
	}

	// Hit something — extend the trail to the impact, apply the pellet's damage, decal, then stop.
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
	char ric[20];
	sprintf(ric, "debris/ric%d.wav", RANDOM_LONG(1, 5));
	EMIT_SOUND_DYN(ENT(owner->pev), CHAN_AUTO, ric, VOL_NORM, ATTN_NORM, 0, TSRecordedSlowPitch(owner));
	DecalGunshot(&tr, BULLET_PLAYER_9MM);
	DestroyBullet(slot);
	b.active = 0;
}
