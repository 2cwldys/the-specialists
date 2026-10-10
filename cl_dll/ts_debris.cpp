/***
*  ts_debris.cpp — the TSFrags message (gmsgTSCreateDebris; client.dll's handler 0x1004bd50). See
*  ts_debris.h. Payload: origin[3], size[3] (coords), a byte kind; kind 0 (glass) then carries
*  point[3], dir[3] (coords), the killing hit's damage bits (long) and a velocity (coord). The
*  spawners are the particle pool's (ts_particles.cpp).
***/
#include "hud.h"
#include "cl_util.h"
#include "parsemsg.h"
#include "ts_debris.h"

#include "ts_particles.h"

DECLARE_MESSAGE(m_TSDebris, TSFrags)

bool CHudTSDebris::Init()
{
	HOOK_MESSAGE(TSFrags); // the original wire name is "TSFrags" (star_LinkUserMessages.md #62)
	gHUD.AddHudElem(this);
	m_iFlags |= HUD_ACTIVE; // no Draw of its own; what it spawns draws in the particle pool
	return true;
}

bool CHudTSDebris::VidInit()
{
	return true;
}

bool CHudTSDebris::MsgFunc_TSFrags(const char* pszName, int iSize, void* pbuf)
{
	BEGIN_READ(pbuf, iSize);

	Vector origin, size, point, dir;
	origin.x = READ_COORD(); origin.y = READ_COORD(); origin.z = READ_COORD();
	size.x = READ_COORD();   size.y = READ_COORD();   size.z = READ_COORD();
	const int material = READ_BYTE();

	if (material == 0)
	{
		// glass (CBreakable::Die, 0x752e0): the hit point, then the shatter direction
		point.x = READ_COORD(); point.y = READ_COORD(); point.z = READ_COORD();
		dir.x = READ_COORD();   dir.y = READ_COORD();   dir.z = READ_COORD();
		const int dmgBits = READ_LONG();
		const float velMag = READ_COORD();
		TSP_GlassShatter(origin, size, dir, point, dmgBits, velMag);
	}
	else if (material == 1)
		TSP_Explosion(origin, (int)size.x); // a grenade: size.x is the blast magnitude
	else if (material == 2)
		TSP_MetalHit(origin);

	return true;
}
