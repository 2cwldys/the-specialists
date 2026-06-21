/***
*  ts_model.cpp — RECONSTRUCTED CTSModel from ts_i386.so + ts_fgd.fgd.
*  Functional/source-faithful (see CLAUDE.md). Verified against the binary's
*  Spawn__8CTSModel disassembly (the recovered/ decompile was truncated by a
*  Ghidra noreturn artifact at UTIL_SetSize; the real body continues).
*
*  @PointClass base(Targetname, Angles, Sequence) studio() = ts_model : "TS model"
*    "model"(studio) + render / sequence / angles keyvalues (all stock CBaseEntity).
*
*  A decorative animated studio-model prop — the generic-cycler pattern
*  (cf. CCycler in h_cycler.cpp): precache, set model, init bones, set the
*  cycler fields, reset sequence info; animates client-side. With no model set
*  it removes itself. By far the most common TS map entity (~858 placements).
***/
#include "extdll.h"
#include "util.h"
#include "cbase.h"

class CTSModel : public CBaseAnimating
{
public:
	void Spawn(void) override;
	void Precache(void) override;
};

LINK_ENTITY_TO_CLASS(ts_model, CTSModel);

// Original Precache is empty (the model is precached in Spawn); kept because
// Spawn calls it through the vtable.
void CTSModel::Precache(void)
{
}

void CTSModel::Spawn(void)
{
	Precache();
	pev->classname = MAKE_STRING("ts_model");

	if (FStringNull(pev->model))
	{
		// No model assigned — nothing to display, so remove ourselves.
		REMOVE_ENTITY(ENT(pev));
		return;
	}

	PRECACHE_MODEL((char *)STRING(pev->model));
	SET_MODEL(ENT(pev), STRING(pev->model));
	UTIL_SetSize(pev, Vector(0, 0, 0), Vector(0, 0, 0));
	UTIL_SetOrigin(pev, pev->origin);

	InitBoneControllers();

	// Cycler-style decorative prop: non-moving, animates client-side from its
	// sequence. (Unlike CCycler it is not a damageable target and is point-sized.)
	pev->movetype = MOVETYPE_NONE;
	pev->solid = SOLID_SLIDEBOX;
	pev->effects = 0;
	pev->framerate = 1.0f;
	pev->yaw_speed = 5;
	pev->ideal_yaw = pev->angles.y;

	m_flFrameRate = 75;
	m_flGroundSpeed = 0;

	pev->nextthink += 1.0;

	ResetSequenceInfo();
}
