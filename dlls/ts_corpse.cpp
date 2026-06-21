/***
*  ts_corpse.cpp — RECONSTRUCTED CTSDeadCorpse (the fading death body).
*  Functional/source-faithful (see CLAUDE.md). Recovered from ts_i386.so:
*  CTSDeadCorpse (@0x7040c Spawn / @0x70460 WaitThink) + TSCreateDeadCorpse (@0x60234).
*
*  On death TS leaves a copy of the player's studio model frozen in its death pose, which
*  fades out over its lifetime and removes itself (so bodies don't pile up). The original
*  uses MOVETYPE_TOSS (@+0x108 = 6) so the body drops, and a 0.5s WaitThink that ramps the
*  render amount down. This is a faithful functional reconstruction.
***/
#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "player.h"
#include "monsters.h"

class CTSDeadCorpse : public CBaseAnimating
{
public:
	void Spawn() override;
	void EXPORT FadeThink(); // @WaitThink — fade the body out, then remove
	int ObjectCaps() override { return CBaseEntity::ObjectCaps() & ~FCAP_ACROSS_TRANSITION; }

	float m_flRemoveTime = 0; // absolute time the body is fully gone (@+0x64)
	float m_flLifetime = 0;   // total lifetime, drives the fade rate (@+0x68)
};

LINK_ENTITY_TO_CLASS(ts_corpse, CTSDeadCorpse);

// @Spawn__13CTSDeadCorpse — MOVETYPE_TOSS so the body settles; poll to fade it out.
void CTSDeadCorpse::Spawn()
{
	pev->movetype = MOVETYPE_TOSS;
	pev->solid = SOLID_NOT;
	pev->takedamage = DAMAGE_NO;
	pev->rendermode = kRenderTransTexture; // so renderamt actually fades it
	pev->renderamt = 255;
	SetThink(&CTSDeadCorpse::FadeThink);
	pev->nextthink = gpGlobals->time + 0.5f;
}

// @WaitThink__13CTSDeadCorpse — every 0.5s: ramp renderamt toward 0 over ~2/3 the lifetime,
// then remove the body once the remove time passes.
void CTSDeadCorpse::FadeThink()
{
	pev->nextthink = gpGlobals->time + 0.5f;
	if (gpGlobals->time > m_flRemoveTime)
	{
		UTIL_Remove(this);
		return;
	}
	float span = m_flLifetime * 0.6666667f;
	if (span < 1.0f)
		span = 1.0f;
	float amt = ((m_flRemoveTime - gpGlobals->time) / span) * 255.0f;
	if (amt > 255.0f)
		amt = 255.0f;
	pev->renderamt = amt;
}

// @TSCreateDeadCorpse(player, lifetime) — spawn a corpse copying the player's death pose
// (model/origin/angles/sequence/frame/body/skin), fading over `lifetime` seconds.
CBaseEntity *TSCreateDeadCorpse(CBasePlayer *pPlayer, float lifetime)
{
	CBaseEntity *pCorpse = CBaseEntity::Create("ts_corpse", pPlayer->pev->origin, pPlayer->pev->angles, NULL);
	if (!pCorpse)
		return NULL;

	CTSDeadCorpse *pBody = (CTSDeadCorpse *)pCorpse;
	pBody->m_flRemoveTime = gpGlobals->time + lifetime;
	pBody->m_flLifetime = lifetime;

	// Copy the player's appearance + frozen death pose.
	if (!FStringNull(pPlayer->pev->model))
		SET_MODEL(ENT(pBody->pev), STRING(pPlayer->pev->model));
	pBody->pev->sequence = pPlayer->pev->sequence;
	pBody->pev->frame = pPlayer->pev->frame;
	pBody->pev->framerate = 0; // freeze the pose
	pBody->pev->body = pPlayer->pev->body;
	pBody->pev->skin = pPlayer->pev->skin;
	pBody->pev->colormap = pPlayer->pev->colormap; // player team/skin colours
	pBody->pev->velocity = pPlayer->pev->velocity * 0.5f;
	UTIL_SetOrigin(pBody->pev, pPlayer->pev->origin);
	return pBody;
}
