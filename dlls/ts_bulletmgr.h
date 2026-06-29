/***
*  ts_bulletmgr.h — CBulletManager (slow-motion "bullets as models", TS_bullet.cpp). In the original
*  it's a 256-slot ring embedded in CTSGameRules (+0x1864); we keep a single global instance. When a
*  shooter is slowed, their pellets become tracked projectiles that physically fly and damage on
*  arrival (so you can watch/dodge a bullet in slow-pause).
***/
#ifndef TS_BULLETMGR_H
#define TS_BULLETMGR_H

#include "cbase.h" // EHANDLE

struct TSSlowBullet
{
	int active;
	EHANDLE owner;
	EHANDLE beam;    // short speed-scaled trail dash riding the bullet head
	EHANDLE flash;   // a head sprite so a flying bullet reads at a glance
	Vector pos, start, dir;
	float damage;
	float fullSpeed; // per-caliber base flight speed, fixed at fire time
};

class CBulletManager
{
public:
	void Reset();                                                       // @0x5ec50 — drop all in-flight bullets
	void Think();                                                       // @0x5ec7c — per-frame: fly every active bullet
	void FireBullet(CBasePlayer *owner, const Vector &src, const Vector &dir, float damage, int caliber); // @0x5eda4
	void DestroyBullet(int slot);                                       // kill one bullet's slot/trail
	void UpdateBullet(int slot);                                        // @0x6ef5c — step one bullet this frame
	void UpdateBulletTimeSlice(int slot);                              // advance one bullet over a time slice

	TSSlowBullet m_bullets[256];
};

extern CBulletManager g_BulletManager;

#endif // TS_BULLETMGR_H
