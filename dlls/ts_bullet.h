/***
*  ts_bullet.h — CBulletManager (slow-motion "bullets as models", TS_bullet.cpp). In the original
*  it's a 256-slot ring embedded in CTSGameRules (+0x1864); we keep a single global instance. When a
*  shooter is slowed, their pellets become tracked projectiles that physically fly and damage on
*  arrival (so you can watch/dodge a bullet in slow-pause).
***/
#ifndef TS_BULLET_H
#define TS_BULLET_H

#include "cbase.h" // EHANDLE

struct TSSlowBullet
{
	int active;
	EHANDLE owner;
	Vector pos, start, dir; // dir as the shot gave it (not normalized: the spread is added to it)
	float damage;
	float fullSpeed; // per-caliber base flight speed, fixed at fire time
	int caliber;     // rec caliber slot — get_ammo_info arg (falloff form B unit)
	int kind;        // rec+0x3c — the shot's mode (ShootBulletsPlayer's iMode, FireBullet's last
	                 // argument): 2 = a pierce leg (falloff form A, dist/30); 1 = DMG bit 25;
	                 // any other value may pierce once more
	int weaponId;    // rec+0x30 — the FIRING weapon's tsweapon_t, captured at fire time: a
	                 // slow-bullet HIT publishes it into glast_weaponid so the kill feed /
	                 // TSGetPointsForFrag know what fired even if the shooter switched since
	float slow;      // rec+0x34 — the shooter's slow factor (m_flSlowMotionRamp, else GiveSlowMul),
	                 // taken at fire time and again at each step
	float nextUpdate; // rec+0x38 — when the bullet next moves (0 at fire; Think sets it)
};

class CBulletManager
{
public:
	void Reset();                                                       // @0x5ec50 — drop all in-flight bullets
	void Think();                                                       // @0x5ec7c — per-frame: fly every active bullet
	void FireBullet(CBasePlayer *owner, const Vector &src, const Vector &dir, float damage, int caliber, int mode); // @0x5eda4
	void DestroyBullet(int slot);                                       // @0x5ef40 — clear one slot
	void UpdateBullet(int slot, TraceResult *ptr);                      // @0x5ef5c — step one bullet
	void UpdateBulletTimeSlice(float flTime, CBasePlayer *pPlayer);     // @0x5f4f0 — empty in the original

	TSSlowBullet m_bullets[256];
	int m_iActive = 0; // +0x4000 (rules+0x5864): the bullets active as Think last ran, plus those fired
	                   // since; CTSGameRules::Think runs Think only while it is not 0 (@0x67635)
};

extern CBulletManager g_BulletManager;

// glast_weaponid (D 0x10100c, this TU; static-init -1): the tsweapon id of the last
// slow-bullet HIT (-1 = the kill wasn't a tracked bullet). Written by UpdateBullet
// (@0x5f2a4); read by PlayerKilled + TSGetPointsForFrag (the kung-fu kill gate).
extern int glast_weaponid;

// @0x5f4f4 UTIL_TraceLine_Expanded — the TS bullet trace: every player's absbox inflated
// +/-32u (saved in vecBoxes[33]), stock trace, then ONLY absmax restored (original quirk;
// the engine relink heals absmin next frame). Defined in ts_bullet.cpp.
void UTIL_TraceLine_Expanded(const Vector &vecStart, const Vector &vecEnd,
	IGNORE_MONSTERS igmon, edict_t *pentIgnore, TraceResult *ptr);

#endif // TS_BULLET_H
