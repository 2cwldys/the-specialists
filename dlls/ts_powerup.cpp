/***
*  ts_powerup.cpp — RECONSTRUCTED CTSPowerUp (+ PlayerAddPowerUp) from
*  ts_i386.so + ts_fgd.fgd. Functional/source-faithful goal (see CLAUDE.md).
*  recovered/server/CTSPowerUp.c (Spawn/OnGround were truncated by Ghidra noreturn
*  artifacts — the real bodies were read from objdump of the .so).
*
*  @PointClass base(Target) = ts_powerup : "TS powerup"
*    spawnflags = the set of powerup types this spawner may give (the random mask):
*      1 Slow Motion, 2 Infinite Clip, 4 Kung Fu, 8 Slow Pause, 16 Double Firerate,
*      32 Grenade, 64 Health, 256 Low Gravity  (== TS_PWUP_* bits)
*    pwuptype (int)     -> m_iType        : force an exact powerup (else random from flags)
*    pwupduration (int) -> m_iRespawnTime : respawn delay  (FGD "Respawn time", def 60)
*    pwupmult (string)  -> m_flMult       : duration multiplier (def 1)
*    message            -> pev->message   : HUD text shown to the picker (base Target)
*
*  The single most common pickup after ts_model — 328 placements across the stock
*  maps. Lifecycle (recovered): Spawn drops it as a SOLID_BBOX/MOVETYPE_TOSS object
*  that thinks Fall; once it lands (FL_ONGROUND) OnGround makes it a SOLID_TRIGGER
*  and arms WaitTouch/WaitThink; WaitTouch applies the powerup to the toucher via
*  PlayerAddPowerUp, then either UTIL_Removes (temp, player-dropped) or respawns
*  (map-placed). The actual *activation* of the timed powerups (slow-motion / slow-
*  pause / double-firerate, via CBasePlayer::UsePowerUp / UpdatePowerUpState) lands
*  with the slow-motion layer; PlayerAddPowerUp here applies the instant powerups
*  for real and stores the timed ones as the player's held powerup.
***/
#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "player.h"
#include "weapons.h"
#include "ts_gamerules.h" // TS_PWUP_* + TSGameRules()
#include "ts_player.h"
#include "ts_utils.h"     // TSHudMessageOne
#include "ts_weapons.h"   // GiveTSWeapon (SLICE grenade/inf-clip hooks)

#define POWERUP_MODEL "models/powerup.mdl"

class CTSPowerUp;
// @ PlayerAddPowerUp__FP11CBasePlayeriP10CTSPowerUp — apply a powerup to a player.
bool PlayerAddPowerUp(CBasePlayer *pPlayer, int type, CTSPowerUp *pPowerup);

// pev->body sub-model index used to show the right icon per powerup type
// (recovered from SetupPowerUp's icon switch).
enum
{
	PUBODY_KUNGFU = 0,
	PUBODY_DBLFIRE = 1,
	PUBODY_INFCLIP = 2,
	PUBODY_SLOWMO = 4,
	PUBODY_GRENADE = 5,
	PUBODY_HEALTH = 6,
	PUBODY_ARMOR = 7,
	PUBODY_LOWGRAV = 8,
	PUBODY_SLOWPAUSE = 9,
};

// The "instant" powerups — applied immediately on touch (vs the timed ones that
// are stored as the player's held powerup). Mask recovered from WaitTouch (0x1e6).
#define TS_PWUP_INSTANT (TS_PWUP_INFCLIP | TS_PWUP_KUNGFU | TS_PWUP_GRENADE | TS_PWUP_HEALTH | 0x80 | TS_PWUP_LOWGRAV)

// 0x80 is the otherwise-unused powerup slot; PlayerAddPowerUp treats it as "armor".
#define TS_PWUP_ARMOR 0x80

class CTSPowerUp : public CBaseAnimating
{
public:
	void Spawn() override;
	void Precache() override;
	bool KeyValue(KeyValueData *pkvd) override;
	bool Save(CSave &save) override;
	bool Restore(CRestore &restore) override;
	int ObjectCaps() override { return CBaseEntity::ObjectCaps() & ~FCAP_ACROSS_TRANSITION; }

	static TYPEDESCRIPTION m_SaveData[];

	// @ SetupPowerUp(type, duration, fallDelay, respawns)
	void SetupPowerUp(int type, int duration, float fallDelay, bool respawns);
	void EXPORT Fall();      // think: settle to the floor
	void OnGround();         // arm as a touch-pickup once landed
	void EXPORT WaitThink(); // think: animate / fade a waiting powerup
	void EXPORT WaitTouch(CBaseEntity *pOther);

	// @ CreatePowerUp(type, duration) — spawn a temporary (player-dropped/dynamic)
	// powerup entity of the given type.
	static CTSPowerUp *CreatePowerUp(int type, int duration);

	int m_iType = 0;          // 0x84 — powerup type bits (pwuptype, or random)
	int m_iDuration = 0;      // 0x90 — effect duration handed to the player (s)
	int m_iRespawnTime = 0;   // 0x94 — pwupduration: respawn delay
	float m_flMult = 1.0f;    // 0x98 — pwupmult: duration multiplier
	float m_flTimeout = 0;    // 0x88 — temp-powerup lifetime expiry
	float m_flArmor = 100.0f; // 0x8c — armor amount given by the armor powerup
	bool m_bRespawns = false; // 0x80 — map-placed (respawns) vs temp (UTIL_Remove)
	bool m_bRandom = false;   // 0x81 — type chosen randomly -> re-roll on respawn
};

LINK_ENTITY_TO_CLASS(ts_powerup, CTSPowerUp);

// Save only what the original did: the type and the respawn delay (MP has no level
// transitions, so this is provenance-faithful rather than load-bearing).
TYPEDESCRIPTION CTSPowerUp::m_SaveData[] = {
	DEFINE_FIELD(CTSPowerUp, m_iType, FIELD_INTEGER),
	DEFINE_FIELD(CTSPowerUp, m_iRespawnTime, FIELD_INTEGER),
};
IMPLEMENT_SAVERESTORE(CTSPowerUp, CBaseAnimating);

bool CTSPowerUp::KeyValue(KeyValueData *pkvd)
{
	if (FStrEq(pkvd->szKeyName, "pwuptype"))
	{
		m_iType = atoi(pkvd->szValue);
		return true;
	}
	if (FStrEq(pkvd->szKeyName, "pwupduration"))
	{
		m_iRespawnTime = atoi(pkvd->szValue);
		return true;
	}
	if (FStrEq(pkvd->szKeyName, "pwupmult"))
	{
		m_flMult = atof(pkvd->szValue);
		return true;
	}
	return CBaseEntity::KeyValue(pkvd);
}

void CTSPowerUp::Precache()
{
	PRECACHE_MODEL(POWERUP_MODEL);
	PRECACHE_SOUND("items/gunpickup2.wav");
}

void CTSPowerUp::Spawn()
{
	Precache();
	pev->classname = MAKE_STRING("ts_powerup");
	SET_MODEL(ENT(pev), POWERUP_MODEL);

	pev->movetype = MOVETYPE_TOSS;
	pev->solid = SOLID_BBOX;

	// glow/fade rendering (recovered: rendermode 1, amt 255, colour 0, fx 14)
	pev->rendermode = kRenderTransColor;
	pev->renderamt = 255;
	pev->rendercolor = Vector(0, 0, 0);
	pev->renderfx = kRenderFxNoDissipation;

	UTIL_SetOrigin(pev, pev->origin);
	UTIL_SetSize(pev, Vector(-16, -16, -36), Vector(16, 16, 36));

	SetTouch(NULL);
	SetThink(&CTSPowerUp::Fall);
	pev->nextthink = gpGlobals->time + 0.1f;
	pev->framerate = 1.0f;
	ResetSequenceInfo();

	// type 0 here -> SetupPowerUp rolls a random type from spawnflags; permanent
	// (map-placed) powerup, falls immediately.
	SetupPowerUp(m_iType, -1, 0.0f, true);
}

// @ SetupPowerUp — resolve the powerup type (explicit/random), pick the icon body
// and the effect duration, and stamp the lifetime/respawn fields.
void CTSPowerUp::SetupPowerUp(int type, int duration, float fallDelay, bool respawns)
{
	m_iDuration = 0;

	if (type < 1)
	{
		// No explicit type: roll a random one out of the permitted set (spawnflags),
		// falling back to "any" if the flags select nothing.
		m_iType = TSGameRules()->GetRandomPowerup(pev->spawnflags);
		if (m_iType == 0)
			m_iType = TSGameRules()->GetRandomPowerup(TS_PWUP_ALL);
		m_bRandom = true;
	}
	else
	{
		// Explicit type — but never honour low-gravity (gated unused) / the 0x80
		// slot as a forced type; re-roll those.
		if (((char)type >= 0) && !(type & TS_PWUP_LOWGRAV))
			m_iType = type;
		else
			m_iType = TSGameRules()->GetRandomPowerup(TS_PWUP_ALL);
		m_bRandom = false;
	}

	// Icon + default duration by type (lowest set bit wins — slow-motion highest).
	if (m_iType & TS_PWUP_SLOWMO)        { pev->body = PUBODY_SLOWMO;    m_iDuration = 3; }
	else if (m_iType & TS_PWUP_INFCLIP)  { pev->body = PUBODY_INFCLIP;   m_iDuration = 3; }
	else if (m_iType & TS_PWUP_KUNGFU)   { pev->body = PUBODY_KUNGFU;                    }
	else if (m_iType & TS_PWUP_SLOWPAUSE){ pev->body = PUBODY_SLOWPAUSE; m_iDuration = 2; }
	else if (m_iType & TS_PWUP_DBLFIRE)  { pev->body = PUBODY_DBLFIRE;   m_iDuration = 3; }
	else if (m_iType & TS_PWUP_GRENADE)  { pev->body = PUBODY_GRENADE;                   }
	else if (m_iType & TS_PWUP_HEALTH)   { pev->body = PUBODY_HEALTH;                    }
	else if ((char)m_iType < 0)          { pev->body = PUBODY_ARMOR;                     } // 0x80
	else if (m_iType & TS_PWUP_LOWGRAV)  { pev->body = PUBODY_LOWGRAV;                   }

	if (duration < 1)
		m_iDuration = (int)(m_iDuration * m_flMult + 0.5f);
	else
		m_iDuration = duration;

	m_flTimeout = fallDelay + gpGlobals->time;
	m_flArmor = 100.0f;
	m_bRespawns = respawns;
}

// @ Fall — think while the powerup is settling. Hides itself when powerups are
// disabled (nopowerup) or in the objective team modes; otherwise lands -> OnGround.
void CTSPowerUp::Fall()
{
	if (CVAR_GET_FLOAT("nopowerup") == 1.0f /* SLICE: || objective team mode */)
	{
		// powerups suppressed: hide and idle
		pev->nextthink = gpGlobals->time + 5.0f;
		pev->effects = EF_NODRAW;
		return;
	}

	pev->nextthink = gpGlobals->time + 0.1f;

	if (pev->flags & FL_ONGROUND)
		OnGround();

	if (!m_bRespawns && m_flTimeout < gpGlobals->time)
		UTIL_Remove(this);
}

// @ OnGround — the powerup has landed: become a touch-pickup and start animating.
// (Recovered Spawn/OnGround were truncated by Ghidra; the SetTouch/SetThink tail
//  was read from objdump of the .so.)
void CTSPowerUp::OnGround()
{
	pev->solid = SOLID_TRIGGER;
	pev->effects &= ~EF_NODRAW;
	pev->effects |= EF_DIMLIGHT;
	UTIL_SetOrigin(pev, pev->origin);
	UTIL_SetSize(pev, Vector(-16, -16, -16), Vector(16, 16, 16));

	SetTouch(&CTSPowerUp::WaitTouch);
	SetThink(&CTSPowerUp::WaitThink);
}

// @ WaitThink — animate the waiting powerup; fade + remove a temp one as it expires.
void CTSPowerUp::WaitThink()
{
	StudioFrameAdvance(0);
	pev->nextthink = gpGlobals->time + 1.0f;

	if (m_bRespawns)
		return; // map powerups wait forever

	if (gpGlobals->time <= m_flTimeout)
	{
		float amt = (m_flTimeout - gpGlobals->time) * (1.0f / 30.0f) * 255.0f;
		if (amt > 255.0f)
			amt = 255.0f;
		pev->renderamt = amt;
	}
	else
	{
		UTIL_Remove(this);
	}
}

// @ WaitTouch — apply the powerup to the toucher, then remove (temp) or respawn (map).
void CTSPowerUp::WaitTouch(CBaseEntity *pOther)
{
	if (!pOther->IsPlayer() || !pOther->IsAlive())
		return;

	CBasePlayer *pPlayer = (CBasePlayer *)pOther;

	// --- pickup gate (recovered from WaitTouch's boolean chain) ---
	// A timed powerup is auto-taken only if the player isn't already holding one
	// (special case: a near-spent slow-motion may be re-taken). Pressing USE lets
	// the player swap. Instant powerups bypass the held-slot gate entirely.
	bool canTakeTimed = (pPlayer->m_iHeldPowerup == 0) ||
		(pPlayer->m_iHeldPowerup == TS_PWUP_SLOWMO && m_iType == TS_PWUP_SLOWMO && pPlayer->m_flPowerupTime < 3);
	bool pressingUse = (pPlayer->pev->button & IN_USE) != 0;
	bool slotFree = (canTakeTimed || pressingUse) && (pPlayer->m_iPowerups == 0);
	bool apply = slotFree || (m_iType & TS_PWUP_INSTANT);
	if (!apply)
		return;

	// If swapping in a timed powerup while USE-holding a held one, re-drop the old.
	int swapType = 0;
	if (pressingUse && pPlayer->m_iHeldPowerup != 0 && !(m_iType & TS_PWUP_INSTANT))
	{
		if (pPlayer->m_iHeldPowerup != TS_PWUP_SLOWMO || pPlayer->m_flPowerupTime > 2)
			swapType = pPlayer->m_iHeldPowerup;
	}

	if (!PlayerAddPowerUp(pPlayer, m_iType, this))
		return;

	if (swapType != 0)
	{
		CTSPowerUp *pDrop = CreatePowerUp(swapType, -1);
		if (pDrop)
			pDrop->pev->origin = pPlayer->pev->origin;
	}

	if (!m_bRespawns)
	{
		UTIL_Remove(this);
		return;
	}

	// Map powerup: re-roll (if random), hide, and respawn after the delay.
	if (m_bRandom)
		SetupPowerUp(0, -1, 0.0f, true);

	SetTouch(NULL);
	pev->effects |= EF_NODRAW;
	SetThink(&CTSPowerUp::Fall);

	float delay;
	if (m_iRespawnTime < 1)
		delay = 60.0f;
	else if (m_iRespawnTime < 11)
		delay = 10.0f;
	else
		delay = (float)(m_iRespawnTime > 180 ? 180 : m_iRespawnTime);
	pev->nextthink = gpGlobals->time + delay;

	if (!FStringNull(pev->message))
		TSHudMessageOne(0, 0, 0, 1, 30, STRING(pev->message), pPlayer->pev);

	if (!FStringNull(pev->target))
		FireTargets(STRING(pev->target), this, this, USE_TOGGLE, 0);
}

// @ CreatePowerUp — spawn a temporary powerup entity (player-dropped / dynamic).
CTSPowerUp *CTSPowerUp::CreatePowerUp(int type, int duration)
{
	CTSPowerUp *pPowerup = GetClassPtr((CTSPowerUp *)NULL);
	if (!pPowerup)
	{
		UTIL_ClientPrintAll(HUD_PRINTTALK, "WARNING:cannot create powerup!\n");
		return NULL;
	}
	pPowerup->Spawn();
	pPowerup->SetupPowerUp(type, duration, 60.0f, false);
	return pPowerup;
}

// Free helper for CTSGameRules::PutPowerUp (dynamic deathmatch powerup spawning): create a
// temporary powerup of `type` at `vecOrigin` (it then falls/settles like a placed one). The
// origin is set after Spawn so the entity drops from the chosen spawn point. Returns it, or NULL.
CBaseEntity *TSCreatePowerUpAt(int type, const Vector &vecOrigin)
{
	CTSPowerUp *pPowerup = CTSPowerUp::CreatePowerUp(type, -1);
	if (pPowerup)
		UTIL_SetOrigin(pPowerup->pev, vecOrigin);
	return pPowerup;
}

// @ PlayerAddPowerUp(CBasePlayer*, int type, CTSPowerUp*) — apply a powerup to a
// player. Returns true if it took (and the powerup should be consumed). Instant
// powerups are applied immediately; the timed ones (slow-motion/-pause/double-fire)
// are stored as the held powerup, activated later by CBasePlayer::UsePowerUp.
bool PlayerAddPowerUp(CBasePlayer *pPlayer, int type, CTSPowerUp *pPowerup)
{
	if ((char)type < 0) // 0x80 — armor
	{
		// SLICE: original calls CBasePlayer::TakeArmor (TS-added). Add armour directly.
		pPlayer->pev->armorvalue += (int)(pPowerup->m_flArmor + 0.5f);
		if (pPlayer->pev->armorvalue > MAX_NORMAL_BATTERY)
			pPlayer->pev->armorvalue = MAX_NORMAL_BATTERY;
	}
	else if (type & TS_PWUP_KUNGFU)
	{
		if (!pPlayer->m_iKungFu)
			pPlayer->m_iKungFu = 1; // SLICE: gmsgTSPowerUp HUD notify deferred (client side)
	}
	else if (type & TS_PWUP_LOWGRAV)
	{
		if (!pPlayer->m_bLowGravity)
		{
			pPlayer->m_bLowGravity = 1;
			pPlayer->pev->gravity = 0.4f; // SLICE: original sets a client physics key
		}
	}
	else if (type & TS_PWUP_HEALTH)
	{
		if (pPlayer->pev->health < 150.0f)
			pPlayer->pev->health = 150.0f;
		pPlayer->m_flHealthRegen = gpGlobals->time + 1.0f;
	}
	else if (type & TS_PWUP_GRENADE)
	{
		// SLICE: original arms via CBasePlayer::PickUpWeapon(grenade); hand the M61.
		if (!GiveTSWeapon(pPlayer, TSW_M61))
			return false;
	}
	else if (type & TS_PWUP_INFCLIP)
	{
		// @ CTSGun::MultiplyClips — multiply the held weapon's reserve ammo.
		if (!TSMultiplyClips(pPlayer))
			return false;
	}
	else
	{
		// Timed powerup (slow-motion / slow-pause / double-firerate): hold it for the
		// player to activate (CBasePlayer::UsePowerUp, slow-motion layer).
		pPlayer->m_iPowerups = 0;
		pPlayer->m_iHeldPowerup = type;
		pPlayer->m_flPowerupExpireTime = 0;
		pPlayer->m_flPowerupTime = pPowerup->m_iDuration;
	}

	EMIT_SOUND_DYN(ENT(pPlayer->pev), CHAN_VOICE, "items/gunpickup2.wav", VOL_NORM, 0.8f, 0, 100);
	return true;
}

//============================================================================
// CTSSuperJump — a map-placed powerup pre-set to the low-gravity / super-jump type.
// The .so's CTSSuperJump::Spawn (@0xc9f44) sets m_iType = 0x100 (TS_PWUP_LOWGRAV) then
// chains CTSPowerUp::Spawn, so it always gives the super-jump (low-gravity) effect.
//============================================================================
class CTSSuperJump : public CTSPowerUp
{
public:
	void Spawn() override
	{
		m_iType = TS_PWUP_LOWGRAV; // force the super-jump (low-gravity) powerup
		CTSPowerUp::Spawn();
	}
};

LINK_ENTITY_TO_CLASS(ts_superjump, CTSSuperJump);
