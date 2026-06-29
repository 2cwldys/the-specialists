/***
*  ts_physicobject.h — CTSPhysicObject (the slow-motion-aware physics base, TS_grenades.cpp).
*  Base for CTSGrenade and CTSWorldGun. When the object is in slow-motion (pev->fuser1 < 1) its
*  velocity/avelocity/gravity are scaled down (and the originals saved) so it moves in bullet-time;
*  RestorePhysics undoes it when un-slowed. Declared in a header so both subclasses can derive it.
***/
#ifndef TS_PHYSICOBJECT_H
#define TS_PHYSICOBJECT_H

class CTSPhysicObject : public CBaseEntity
{
public:
	void Spawn() override;
	void Precache() override {}          // @0x7babc — empty
	virtual void SavePhysics();          // @0x7bac0 — scale velocity/gravity by the slow factor
	virtual void RestorePhysycs();       // @0x7bcd4 — restore the saved physics (note the orig's spelling)
	void EXPORT WaitThink();             // @0x7bd98 — InSlow ? SavePhysics : RestorePhysycs

	Vector m_savedVelocity = g_vecZero;  // +0x64 — pre-slow velocity (g_vecZero = "not saved")
	Vector m_savedAVelocity = g_vecZero; // +0x70 — pre-slow angular velocity
	float  m_savedGravity = 0;           // +0x7c — pre-slow gravity
	int    m_field80 = 0;                // +0x80 — (used by the derived grenade/worldgun)
	float  m_lastSlow = 1.0f;            // +0x84 — last applied slow factor (fuser1 snapshot)
};

#endif // TS_PHYSICOBJECT_H
