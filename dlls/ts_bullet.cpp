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
#include "ts_weapons.h"
#include "ts_weapon_info.h" // get_ammo_info (the caliber speed unit) // TSSlowPitch (slow-world sound pitch)
#include "ts_bullet.h"
#include "ts_gun.h" // GetPlayerTSGun (the fire-time weapon-id capture)
#include "ts_shared.h"   // GetBulletVelocityMult (the slow-motion bullet math, original TS_shared.cpp)
#include "game.h"        // g_flWeaponCheat (sv_cheats as the map started)

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
			b.caliber = caliber;
			b.kind = get_ammo_info(caliber)->iKind; // rec+0x3c — the falloff form selector
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

// @ Think__14CBulletManager — per frame (from CTSGameRules::Think): step every active bullet.
void CBulletManager::Think()
{
	for (int i = 0; i < 256; i++)
		if (m_bullets[i].active)
			UpdateBullet(i);
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
	float slow = owner->pev->fuser1;
	if (slow < 0.01f)
		slow = 0.01f;

	const float velMult = GetBulletVelocityMult(slow, owner->m_bSlowBullets);
	float speed = b.fullSpeed * velMult;
	Vector newpos = b.pos + b.dir * (speed * gpGlobals->frametime);

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
		if (b.kind != 2)
		{
			const float dmgMult = CVAR_GET_FLOAT("damagemult");
			if (dmgMult > 0.0f && g_flWeaponCheat != 0)
				dmg *= dmgMult;
		}
		glast_weaponid = b.weaponId; // @0x5f2a4 — publish what fired, for the kill feed / scoring
		ClearMultiDamage();
		pHit->TraceAttack(owner->pev, dmg, b.dir, &tr, DMG_BULLET | DMG_NEVERGIB);
		ApplyMultiDamage(owner->pev, owner->pev);
		glast_weaponid = -1; // @0x5f311 — reset RIGHT after the damage window (PlayerKilled runs
		                     // inside ApplyMultiDamage); leaving it armed misattributes later kills
		if (g_flWeaponCheat != 0 && CVAR_GET_FLOAT("dmgreport") > 1.0f) // @0x5f317
			UTIL_ClientPrintAll(HUD_PRINTTALK, UTIL_VarArgs("damage:%f\n", dmg));
	}
	char ric[20];
	sprintf(ric, "debris/ric%d.wav", RANDOM_LONG(1, 5));
	// NO original counterpart (the census of all 156 EMIT_SOUND_DYN sites has no bullet-manager
	// ric — the original's ric is client-side event playback). Explicit weight 0.0f => scale = 1
	// so the TS overload leaves this untouched: the ad-hoc TSRecordedSlowPitch already encodes
	// the slow. Flag for a later RE pass of whether the server should emit this at all.
	EMIT_SOUND_DYN(ENT(owner->pev), CHAN_AUTO, ric, VOL_NORM, ATTN_NORM, 0, TSRecordedSlowPitch(owner), 0.0f);
	DecalGunshot(&tr, BULLET_PLAYER_9MM);
	DestroyBullet(slot);
	b.active = 0;
}
