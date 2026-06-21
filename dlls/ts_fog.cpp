/***
*  ts_fog.cpp — RECONSTRUCTED CTSFog from ts_i386.so + ts_fgd.fgd.
*  Functional/source-faithful (see CLAUDE.md). recovered/server/CTSFog.c.
*
*  @PointClass = ts_fog : "TS Fog"  — keyvalues near/far/color.
*
*  The original Spawn does exactly one thing: UTIL_Remove(this). The fog is a
*  client-side effect read from the map's entity data; the server entity has no
*  runtime role and deletes itself on spawn.
***/
#include "extdll.h"
#include "util.h"
#include "cbase.h"

class CTSFog : public CBaseEntity
{
public:
	void Spawn(void) override;
};

LINK_ENTITY_TO_CLASS(ts_fog, CTSFog);

void CTSFog::Spawn(void)
{
	UTIL_Remove(this);
}
