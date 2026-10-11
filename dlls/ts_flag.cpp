/***
*  ts_flag.cpp — RECONSTRUCTED CTSFlag (original TS_TSFlag.cpp), the loose CTF flag entity.
*  Functional/source-faithful (see CLAUDE.md). Recovered addresses (nm):
*    CreateFlag @0x9d874, Spawn @0x9d978, Precache @0x9db94, BounceTouch @0x9dbbc,
*    Destroy @0x9de24, RespawnFlag @0x9de58, Fall @0x9df10, WaitThink @0x9e1b8,
*    OnGround @0x9e21c, CanInteract @0x9e2d4, PlayerTakeFlag @0x9e330 (=Use @0x9e5dc =
*    WaitTouch @0x9e604), ObjectCaps @0x9e704, CanUsedTroughWalls @0x9e70c.
*
*  Carry model (the whole CTF design): there is NO "flag follows the carrier" entity. A CTSFlag
*  entity exists ONLY while the flag is LOOSE. On pickup CBasePlayer::TakeFlag copies the flag's
*  28-byte s_flag_status onto the carrier and the flag Destroys itself; on drop a fresh loose flag
*  is CreateFlag'd. The status carries an EHANDLE back to its owning CTSBriefcase, so a returned/
*  captured flag asks that briefcase to re-drop a fresh flag at base (RespawnFlag / CaptureTouch).
***/
#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "weapons.h"        // ClearMultiDamage / ApplyMultiDamage (thrown-flag impact)
#include "player.h"
#include "gamerules.h"
#include "ts_gamerules.h"
#include "ts_flag.h"
#include "ts_briefcase.h"
#include "ts_utils.h"        // TSHudMessageAll
#include "ts_player.h"       // TSThrowSlow (the dropper's slow-motion rate)
#include "UserMessages.h"    // gmsgScoreInfo

#define FLAG_MODEL "models/briefcase.mdl"

// @ gmsgTSHaveFlag — wire name "TSFlag" (the original registers the tail of the "7CTSFlag" literal @0xf9c84; the GLOBAL is named gmsgTSHaveFlag). MSG_ALL(4): base-spawn(0)/pickup(1)/drop(2), flagID, team+1, carrier entindex.
// Registered lazily, the codebase's pattern for TS messages (see ts_player.cpp gmsgTSHealth).
int gmsgTSHaveFlag = 0;

LINK_ENTITY_TO_CLASS(ts_flag, CTSFlag)

// Per-team flag tint (recovered from CTSFlag::Spawn — pev->rendercolor per status.team).
static void TSFlagSetTeamColor(entvars_t *pev, int team)
{
	switch (team)
	{
	case 0:  pev->rendercolor = Vector(125, 165, 210); break; // blue
	case 1:  pev->rendercolor = Vector(200, 90, 70);  break;  // red
	case 2:  pev->rendercolor = Vector(225, 205, 45); break;  // yellow
	default: pev->rendercolor = Vector(145, 215, 140); break; // green / neutral
	}
}

// @0x9d874 CreateFlag — static factory. `pStatus` is the template/source status (briefcase's for a
// base spawn, the dropper's for a drop). `pOwner` is the dropping player (NULL for a base spawn).
CTSFlag *CTSFlag::CreateFlag(CBaseEntity *pOwner, s_flag_status *pStatus)
{
	CTSFlag *pFlag = GetClassPtr((CTSFlag *)NULL);
	if (!pFlag)
	{
		UTIL_ClientPrintAll(HUD_PRINTCONSOLE, "WARNING:cannot create flag!\n");
		return NULL;
	}
	pFlag->m_flagStatus = *pStatus;     // copy the full 28-byte status (id/team/contents/briefcase handle)
	pFlag->Spawn();
	pFlag->m_pOwner = pOwner;

	if (!pOwner)
	{
		// Base spawn from the briefcase: not in slow-motion, no thrower (@0x8d954: pev->owner, fuser1).
		pFlag->pev->owner = NULL;
		pFlag->pev->fuser1 = 1.0f;
	}
	else
	{
		// Dropped by a player: the flag takes his slow-motion rate (@0x8d902, TSThrowSlow, through
		// GoSlow) so a flag dropped in bullet-time flies slowed. The caller (DropFlag) sets velocity.
		pFlag->pev->owner = pOwner->edict();
		pFlag->GoSlow(TSThrowSlow((CBasePlayer *)pOwner), 0);
	}
	return pFlag;
}

// @0x9db94 Precache — the model + pickup/drop sounds (also precached at map load in ts_weapons.cpp,
// since the flag is runtime-created; re-precaching an already-loaded asset is a no-op).
void CTSFlag::Precache()
{
	PRECACHE_MODEL(FLAG_MODEL);
	PRECACHE_SOUND("items/gunpickup2.wav");
	PRECACHE_SOUND("items/weapondrop1.wav");
}

// @0x9d978 Spawn — the loose flag: briefcase model, slow-aware toss physics, fall toward the floor.
void CTSFlag::Spawn()
{
	CTSPhysicObject::Spawn();
	Precache();
	SET_MODEL(ENT(pev), FLAG_MODEL);

	pev->movetype = MOVETYPE_TOSS;
	pev->solid = SOLID_TRIGGER;
	pev->gravity = 0.7f;
	pev->friction = 0.8f;
	pev->classname = MAKE_STRING("ts_flag");
	UTIL_SetOrigin(pev, pev->origin);
	UTIL_SetSize(pev, g_vecZero, g_vecZero); // a point while in flight

	m_flExpireTime = gpGlobals->time + 20.0f; // a loose flag auto-returns after 20s (base flag clears it)
	m_pOwner = nullptr;

	SetTouch(&CTSFlag::BounceTouch);
	SetThink(&CTSFlag::Fall);
	pev->nextthink = gpGlobals->time + 0.1f;

	// Render: a faint per-team glow so the flag reads at a distance.
	pev->iuser4 |= 2;
	pev->renderamt = 5;
	pev->renderfx = kRenderFxGlowShell;
	TSFlagSetTeamColor(pev, m_flagStatus.team);

	m_bRemoved = false;
}

// @0x9dbbc BounceTouch — airborne physics-bounce handler: damp the fall, play the drop sound on a
// solid hit, and apply restitution so the flag rolls/settles (mirrors CTSWorldGun::BounceTouch).
void CTSFlag::BounceTouch(CBaseEntity *pOther)
{
	pev->velocity.z *= 0.8f;
	float slowMul = (pev->fuser1 > 0.01f) ? pev->fuser1 : 0.01f;

	if (!(pev->flags & FL_ONGROUND))
	{
		float speed = pev->velocity.Length();
		if (speed > 18.0f * slowMul)
			EMIT_SOUND_DYN(ENT(pev), CHAN_BODY, "items/weapondrop1.wav", VOL_NORM, ATTN_NORM,
				0, 95 + RANDOM_LONG(0, 10));
	}
	else
	{
		float horiz = Vector(pev->velocity.x, pev->velocity.y, 0).Length();
		if (horiz <= 40.0f)
			pev->velocity = pev->velocity * 0.7f;
		pev->avelocity = g_vecZero;
		pev->angles.z = 0;
	}
}

// @0x9de24 Destroy — take the flag out of play.
void CTSFlag::Destroy()
{
	m_bRemoved = true;
	RemoveEntityFromSlowedArray(this); // Destroy__7CTSFlag @0x8de44 — scrub the packed slowed array before removal
	UTIL_Remove(this);
}

// @0x9de58 RespawnFlag — the flag goes home: ask its owning briefcase to re-drop a fresh flag at
// base (the flag never walks itself home), announce it, then remove this loose entity.
void CTSFlag::RespawnFlag()
{
	CTSBriefcase *pBriefcase = (CTSBriefcase *)(CBaseEntity *)m_flagStatus.hBriefcase;
	if (pBriefcase)
	{
		pBriefcase->SetThink(&CTSBriefcase::FlagFall);
		pBriefcase->pev->nextthink = gpGlobals->time + 1.0f;
		// @0x9deb2: an unsigned test (cmp 3; ja), so a neutral flag (team -1) goes home without a
		// word; a team's flag names the team, held 30 (@0x9deea).
		if ((unsigned)m_flagStatus.team <= 3)
			TSHudMessageAll(0, 0xff, 0, 4, 30,
				UTIL_VarArgs("%s briefcase returned", TSGameRules->GetIndexedTeamName(m_flagStatus.team)));
	}
	Destroy();
}

// @0x9df10 Fall — think while loose. Settle once at rest; while in flight a fast pass through a
// player damages them (a thrown flag hurts); past the auto-return deadline, go home.
void CTSFlag::Fall()
{
	pev->nextthink = gpGlobals->time + 0.1f;
	float slowMul = (pev->fuser1 > 0.01f) ? pev->fuser1 : 0.01f;

	if ((pev->flags & FL_ONGROUND) && pev->velocity.Length() < 3.0f * slowMul)
	{
		EMIT_SOUND_DYN(ENT(pev), CHAN_BODY, "items/weapondrop1.wav", VOL_NORM, ATTN_NORM, 0, 100);
		pev->angles.x = 0;
		pev->angles.z = 0;
		OnGround();
		return;
	}

	if (m_flExpireTime == 0.0f || gpGlobals->time <= m_flExpireTime)
	{
		// Swept impact: a flag flung through a player at speed can kill (momentum TakeDamage).
		Vector dir = pev->velocity * 0.1f;
		TraceResult tr;
		UTIL_TraceLine(pev->origin, pev->origin + dir, dont_ignore_monsters, ENT(pev), &tr);
		if (tr.flFraction < 1.0f && tr.pHit)
		{
			CBaseEntity *pHit = CBaseEntity::Instance(tr.pHit);
			float speed = pev->velocity.Length();
			if (pHit && pHit->IsPlayer() && pHit->pev->takedamage != DAMAGE_NO &&
				speed * 30.0f >= pHit->pev->max_health)
			{
				ClearMultiDamage();
				pHit->TraceAttack(pev, speed * 30.0f * 0.001f, dir.Normalize(), &tr,
					DMG_CLUB | DMG_NEVERGIB); // dmgbits 0x1080
				ApplyMultiDamage(pev, pev);
			}
		}
		CTSPhysicObject::WaitThink(); // slow-motion physics upkeep
	}
	else
	{
		RespawnFlag();
	}
}

// @0x9e1b8 WaitThink — settled keepalive: auto-return once the (set) deadline passes.
void CTSFlag::WaitThink()
{
	pev->nextthink = gpGlobals->time + 0.1f;
	if (m_flExpireTime != 0.0f && gpGlobals->time > m_flExpireTime)
		RespawnFlag();
}

// @0x9e21c OnGround — settled: become a re-touchable pickup with a real grab box.
void CTSFlag::OnGround()
{
	pev->owner = NULL;
	pev->solid = SOLID_BBOX;
	UTIL_SetOrigin(pev, pev->origin);
	UTIL_SetSize(pev, Vector(-16, -16, 0), Vector(16, 16, 48));
	SetTouch(&CTSFlag::WaitTouch);
	SetThink(&CTSFlag::WaitThink);
	pev->nextthink = gpGlobals->time + 0.1f;
}

// @0x9e2d4 CanInteract — neutral flag: anyone; team flag: only that team.
bool CTSFlag::CanInteract(CBasePlayer *pPlayer)
{
	if (m_flagStatus.team == -1)
		return true;
	return m_flagStatus.team == TSGameRules->GetTeamIndex(pPlayer->TeamID());
}

// @0x9e330 PlayerTakeFlag — a player touched the loose flag. An ENEMY/NEUTRAL flag is picked up
// (copied onto the carrier, flag destroyed). The player's OWN team's DROPPED flag is RETURNED for
// a +5 frag bonus. (Use @0x9e5dc and WaitTouch @0x9e604 both route here.)
void CTSFlag::PlayerTakeFlag(CBaseEntity *pOther)
{
	if (m_bRemoved || !pOther || !pOther->IsPlayer() || !pOther->IsAlive())
		return;
	if (pOther->pev->effects & EF_NODRAW) // spectator/observer guard (high effects bit)
		return;

	CBasePlayer *pPlayer = (CBasePlayer *)pOther;
	int pt = TSGameRules->GetTeamIndex(pPlayer->TeamID());

	if (m_flagStatus.team == -1 || m_flagStatus.team != pt)
	{
		// Enemy/neutral flag: pick it up (TakeFlag refuses if over slot weight or already carrying).
		if (pPlayer->TakeFlag(&m_flagStatus))
		{
			if (m_pOwner == NULL)
			{
				// A base flag was taken — its briefcase no longer has a flag out.
				CTSBriefcase *pBriefcase = (CTSBriefcase *)(CBaseEntity *)m_flagStatus.hBriefcase;
				if (pBriefcase)
					pBriefcase->m_bFlagOut = false;
			}
			Destroy();
		}
	}
	else if (m_pOwner != NULL)
	{
		// Own team's DROPPED flag → return it home and reward the returner (+5).
		CTSBriefcase *pBriefcase = (CTSBriefcase *)(CBaseEntity *)m_flagStatus.hBriefcase;
		if (pBriefcase)
		{
			pBriefcase->SetThink(&CTSBriefcase::FlagFall);
			pBriefcase->pev->nextthink = gpGlobals->time + 1.0f;
		}
		Destroy();
		EMIT_SOUND(ENT(pPlayer->pev), CHAN_ITEM, "items/gunpickup2.wav", VOL_NORM, ATTN_NORM);

		if (TSGameRules->CountPlayers() > 1)
		{
			// @0x9e49e: the line announces the 5 points the return gives, not the returner's frags
			// (push 5), held 30 (@0x9e4c5).
			pPlayer->pev->frags += 5;
			TSHudMessageAll(0, 0xff, 0, 4, 30,
				UTIL_VarArgs("%s returned the briefcase! %i #TS_Frg", STRING(pPlayer->pev->netname), 5));

			if (gmsgScoreInfo)
			{
				MESSAGE_BEGIN(MSG_ALL, gmsgScoreInfo);
				WRITE_BYTE(ENTINDEX(pPlayer->edict()));
				WRITE_SHORT((int)pPlayer->pev->frags);
				WRITE_SHORT(pPlayer->m_iDeaths);
				WRITE_SHORT(0);
				WRITE_SHORT(TSGameRules->GetTeamIndex(pPlayer->m_szTeamName) + 1);
				MESSAGE_END();
			}
		}
	}
}

// @0x9e5dc Use / @0x9e604 WaitTouch — both pickup entry points route to PlayerTakeFlag.
void CTSFlag::Use(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value)
{
	PlayerTakeFlag(pActivator);
}

void CTSFlag::WaitTouch(CBaseEntity *pOther)
{
	PlayerTakeFlag(pOther);
}
