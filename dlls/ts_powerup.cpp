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
*    pwuptype (int)     -> poweruptype        : force an exact powerup (else random from flags)
*    pwupduration (int) -> pwupduration : respawn delay  (FGD "Respawn time", def 60)
*    pwupmult (string)  -> pwupmult       : duration multiplier (def 1)
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
#include <math.h>
#include "util.h"
#include "cbase.h"
#include "player.h"
#include "weapons.h"
#include "ts_gamerules.h" // TS_PWUP_* + TSGameRules
#include "ts_player.h"
#include "ts_utils.h"     // TSHudMessageOne
#include "ts_weapons.h"   // GiveTSWeapon (SLICE grenade/inf-clip hooks)

#define POWERUP_MODEL "models/powerup.mdl"

#include "ts_powerup.h"

// @0x87894 GetRandomPowerup__12CTSGameRulesi — collect the permitted powerup bits from
// `mask` and return a random one (0 if the pool is empty). Faithful to the .so:
// the 0x80 slot is always skipped, and 0x100 (superjump) is left out on a map whose
// ts_mapglobals gives everyone superjump (TSGameRules+0x5885, read @0x878d3).
int CTSGameRules::GetRandomPowerup(int mask)
{
	int list[9];
	int count = 0;

	for (int bit = 1; bit <= TS_PWUP_LOWGRAV; bit <<= 1)
	{
		if ((mask & bit) == 0)
			continue;
		if (bit == 0x80) // unused powerup slot
			continue;
		if (bit == TS_PWUP_LOWGRAV && TSGameRules->m_bSrvSuperJump != 0)
			continue;
		list[count++] = bit;
	}

	if (count == 0)
		return 0;

	return list[RANDOM_LONG(0, count - 1)];
}

LINK_ENTITY_TO_CLASS(ts_powerup, CTSPowerUp);

// Save only what the original did: the type and the respawn delay (MP has no level
// transitions, so this is provenance-faithful rather than load-bearing).
TYPEDESCRIPTION CTSPowerUp::m_SaveData[] = {
	DEFINE_FIELD(CTSPowerUp, poweruptype, FIELD_INTEGER),
	DEFINE_FIELD(CTSPowerUp, pwupduration, FIELD_INTEGER),
};
IMPLEMENT_SAVERESTORE(CTSPowerUp, CBaseAnimating);

bool CTSPowerUp::KeyValue(KeyValueData *pkvd)
{
	if (FStrEq(pkvd->szKeyName, "pwuptype"))
	{
		poweruptype = atoi(pkvd->szValue);
		return true;
	}
	if (FStrEq(pkvd->szKeyName, "pwupduration"))
	{
		pwupduration = atoi(pkvd->szValue);
		return true;
	}
	if (FStrEq(pkvd->szKeyName, "pwupmult"))
	{
		pwupmult = atof(pkvd->szValue);
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
	UTIL_SetSize(pev, g_vecZero, g_vecZero); // @Spawn: a POINT hull while falling (orig zero size)

	SetTouch(NULL);
	SetThink(&CTSPowerUp::Fall);
	pev->nextthink = gpGlobals->time + 0.1f;
	pev->framerate = 1.0f;
	ResetSequenceInfo();

	// type 0 here -> SetupPowerUp rolls a random type from spawnflags; permanent
	// (map-placed) powerup, falls immediately.
	SetupPowerUp(poweruptype, -1, 0.0f, true);
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
		poweruptype = TSGameRules->GetRandomPowerup(pev->spawnflags);
		if (poweruptype == 0)
			poweruptype = TSGameRules->GetRandomPowerup(TS_PWUP_ALL);
		m_bRandom = true;
	}
	else
	{
		// Explicit type — re-rolled when it is the 0x80 (armor) slot, or superjump while the
		// map gives everyone superjump (@0x88047: rules+0x5885 and type & 0x100).
		if (((char)type >= 0) && !(TSGameRules->m_bSrvSuperJump && (type & TS_PWUP_LOWGRAV)))
			poweruptype = type;
		else
			poweruptype = TSGameRules->GetRandomPowerup(TS_PWUP_ALL);
		m_bRandom = false;
	}

	// Icon + default duration by type (lowest set bit wins — slow-motion highest).
	if (poweruptype & TS_PWUP_SLOWMO)        { pev->body = PUBODY_SLOWMO;    m_iDuration = 3; }
	else if (poweruptype & TS_PWUP_INFCLIP)  { pev->body = PUBODY_INFCLIP;   m_iDuration = 3; }
	else if (poweruptype & TS_PWUP_KUNGFU)   { pev->body = PUBODY_KUNGFU;                    }
	else if (poweruptype & TS_PWUP_SLOWPAUSE){ pev->body = PUBODY_SLOWPAUSE; m_iDuration = 2; }
	else if (poweruptype & TS_PWUP_DBLFIRE)  { pev->body = PUBODY_DBLFIRE;   m_iDuration = 3; }
	else if (poweruptype & TS_PWUP_GRENADE)  { pev->body = PUBODY_GRENADE;                   }
	else if (poweruptype & TS_PWUP_HEALTH)   { pev->body = PUBODY_HEALTH;                    }
	else if ((char)poweruptype < 0)          { pev->body = PUBODY_ARMOR;                     } // 0x80
	else if (poweruptype & TS_PWUP_LOWGRAV)  { pev->body = PUBODY_LOWGRAV;                   }

	if (duration < 1)
		m_iDuration = (int)(m_iDuration * pwupmult + 0.5f);
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
	pev->effects |= EF_MUZZLEFLASH; // @OnGround disasm: orl $2 — NOT a dynamic light (the old
	                                // EF_DIMLIGHT was invented; the original has no light glow)
	UTIL_SetOrigin(pev, pev->origin);
	// bbox BOTTOM at the origin (z 0..64) — the model sits ON the floor (the old -16..16 hull
	// floated the origin 16u up, drawing the powerup twice as high as the original).
	UTIL_SetSize(pev, Vector(-16, -16, 0), Vector(16, 16, 64));

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
		(pPlayer->m_iHeldPowerup == TS_PWUP_SLOWMO && poweruptype == TS_PWUP_SLOWMO && pPlayer->m_flPowerupTime < 3);
	bool pressingUse = (pPlayer->pev->button & IN_USE) != 0;
	bool slotFree = (canTakeTimed || pressingUse) && (pPlayer->m_iPowerups == 0);
	bool apply = slotFree || (poweruptype & TS_PWUP_INSTANT);
	if (!apply)
		return;

	// If swapping in a timed powerup while USE-holding a held one, re-drop the old.
	int swapType = 0;
	if (pressingUse && pPlayer->m_iHeldPowerup != 0 && !(poweruptype & TS_PWUP_INSTANT))
	{
		if (pPlayer->m_iHeldPowerup != TS_PWUP_SLOWMO || pPlayer->m_flPowerupTime > 2)
			swapType = pPlayer->m_iHeldPowerup;
	}

	if (!PlayerAddPowerUp(pPlayer, poweruptype, this))
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
	if (pwupduration < 1)
		delay = 60.0f;
	else if (pwupduration < 11)
		delay = 10.0f;
	else
		delay = (float)(pwupduration > 180 ? 180 : pwupduration);
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

// @0x874f4 GetRandomPosition — a random reachable point around `center` (decompile-exact):
// 16 compass sectors of round(360/16) = 22 deg (the FPU round of 22.5 — quirk kept), start
// sector RANDOM_LONG(0,16) (17 values — quirk kept); per try: dist = RANDOM_LONG(128,1024),
// flat dir (cos,sin,0) normalized, TraceLine(center -> center+dir*dist, ignore_monsters),
// candidate = hitpoint - dir*64; accept unless inside CONTENTS_SOLID; advance one sector,
// 16 tries; all rejected -> the center itself.
Vector GetRandomPosition(Vector &center)
{
	const int step = 22; // (int)ROUND(360.0 / 16.0) — 22, not 22.5 (the original's FPU round)
	int angle = RANDOM_LONG(0, 16) * step;
	for (int i = 1; i <= 16; i++, angle += step)
	{
		const float dist = (float)RANDOM_LONG(0x80, 0x400);
		Vector dir(cosf(angle * (float)(M_PI / 180.0)), sinf(angle * (float)(M_PI / 180.0)), 0.0f);
		const float len = dir.Length();
		dir = (len > 0.0f) ? dir * (1.0f / len) : Vector(0, 0, 1);
		TraceResult tr;
		UTIL_TraceLine(center, center + dir * dist, ignore_monsters, nullptr, &tr);
		Vector pos = ((tr.flFraction >= 1.0f) ? (center + dir * dist) : tr.vecEndPos) - dir * 64.0f;
		if (UTIL_PointContents(pos) != CONTENTS_SOLID)
			return pos;
	}
	return center;
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

// @ PutPowerUp__12CTSGameRulesi — spawn a powerup at a random info_player_deathmatch
// spawn point. Skips the spot if a ts_powerup is already nearby (so dynamically-spawned
// powerups don't stack). Returns true if one was created.
bool CTSGameRules::PutPowerUp(int type)
{
	// Collect the deathmatch spawn points (the .so caps at 32).
	CBaseEntity *spawns[32];
	int count = 0;
	CBaseEntity *pSpot = nullptr;
	while (count < 32 &&
		(pSpot = UTIL_FindEntityByClassname(pSpot, "info_player_deathmatch")) != nullptr)
		spawns[count++] = pSpot;

	if (count == 0)
		return false;

	Vector vecOrigin = spawns[RANDOM_LONG(0, count - 1)]->pev->origin + Vector(0, 0, 16);

	// Don't pile powerups onto a spot that already has one.
	CBaseEntity *pNear = nullptr;
	while ((pNear = UTIL_FindEntityInSphere(pNear, vecOrigin, 128.0f)) != nullptr)
		if (FClassnameIs(pNear->pev, "ts_powerup"))
			return false;

	return TSCreatePowerUpAt(type, vecOrigin) != nullptr;
}

// Notify the client to show the bottom-center indicator for an INDEPENDENT powerup (kung-fu /
// super-jump). RE PlayerAddPowerUp@0x97bc8: the original sends gmsgTSPowerUp ("PwUp") = WriteShort(bit)
// + WriteByte(0) for these (the dur-0 grant), distinct from the held timed-powerup slot (top-left).
static void TSSendPowerupNotify(CBasePlayer *pPlayer, int bit)
{
	extern int gmsgTSPowerUp; // ts_player.cpp — registered centrally in LinkUserMessages
	MESSAGE_BEGIN(MSG_ONE, gmsgTSPowerUp, NULL, pPlayer->pev);
	WRITE_SHORT(bit); // 4 = kung-fu, 0x100 = super-jump (low-gravity)
	WRITE_BYTE(0);    // dur 0 -> persistent independent powerup (not the held countdown slot)
	MESSAGE_END();
}

// @ PlayerAddPowerUp(CBasePlayer*, int type, CTSPowerUp*) — apply a powerup to a
// player. Returns true if it took (and the powerup should be consumed). Instant
// powerups are applied immediately; the timed ones (slow-motion/-pause/double-fire)
// are stored as the held powerup, activated later by CBasePlayer::UsePowerUp.
bool PlayerAddPowerUp(CBasePlayer *pPlayer, int type, CTSPowerUp *pPowerup)
{
	if ((char)type < 0) // 0x80 — armor
	{
		// @0x87c16: kevlar through CBasePlayer::TakeArmor. A refusal (fewer than 10 free slots)
		// still uses the powerup up.
		pPlayer->TakeArmor((int)(pPowerup->m_flArmor + 0.5f));
	}
	else if (type & TS_PWUP_KUNGFU)
	{
		if (!pPlayer->m_iKungFu)
		{
			pPlayer->m_iKungFu = 1;
			TSSendPowerupNotify(pPlayer, TS_PWUP_KUNGFU); // bottom-center HUD "Kung-Fu" icon
		}
	}
	else if (type & TS_PWUP_LOWGRAV)
	{
		if (!pPlayer->m_bLowGravity)
		{
			pPlayer->m_bLowGravity = 1;
			g_engfuncs.pfnSetPhysicsKeyValue(pPlayer->edict(), "mj", "1"); // PM_Jump reads "mj" -> the DUCK+JUMP high jump
			TSSendPowerupNotify(pPlayer, TS_PWUP_LOWGRAV); // bottom-center HUD "Super Jump" icon
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
		// @0x87d4e: PickUpWeapon(M61, NULL, 0, SELECT true, no owner record) — the powerup
		// hands the grenade AND wields it.
		if (!pPlayer->PickUpWeapon(TSW_M61, NULL, 0, true, false))
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

	EMIT_SOUND_DYN(ENT(pPlayer->pev), CHAN_ITEM, "items/gunpickup2.wav", VOL_NORM, 0.8f, 0, 100); // @0x87dd9
	return true;
}

// @ ClientCommand `powerup <N>` (RE: ts_i386.so ClientCommand 0x9db8a..0x9dd42, jump table at
// GOT-0xc8264) — the debug cheat that grants a powerup directly. The caller checks g_flWeaponCheat, the
// argument, that the player is alive and not observing. The original's table:
//   1 slow-pause, 5 double fire rate, anything else slow motion: held for 5 s (5 more if the same
//     type is already held), only while no powerup is running, with the TSPowerUp held message;
//   2 PickUpWeapon(M61); 3 kung fu AND superjump, each with its TSPowerUp message, even when already
//   had; 4 MultiplyClips on the held gun. No pickup sound.
void TSPowerupCheat(CBasePlayer *pPlayer, int n)
{
	int timed = TS_PWUP_SLOWMO;
	switch (n)
	{
	case 1:
		timed = TS_PWUP_SLOWPAUSE;
		break;
	case 2: // @0x9dbcc
		pPlayer->PickUpWeapon(TSW_M61, NULL, 0, true, false);
		return;
	case 3: // @0x9dbec
		pPlayer->m_iKungFu = 1;
		TSSendPowerupNotify(pPlayer, TS_PWUP_KUNGFU);
		pPlayer->m_bLowGravity = 1;
		g_engfuncs.pfnSetPhysicsKeyValue(pPlayer->edict(), "mj", "1");
		TSSendPowerupNotify(pPlayer, TS_PWUP_LOWGRAV);
		return;
	case 4: // @0x9dc90
		TSMultiplyClips(pPlayer);
		return;
	case 5:
		timed = TS_PWUP_DBLFIRE;
		break;
	}

	// @0x9dcc9
	if (pPlayer->m_iPowerups != 0)
		return;
	extern int gmsgTSPowerUp;
	MESSAGE_BEGIN(MSG_ONE, gmsgTSPowerUp, NULL, pPlayer->pev);
	WRITE_SHORT(timed);
	WRITE_BYTE(0);
	MESSAGE_END();
	if (pPlayer->m_iHeldPowerup == timed)
		pPlayer->m_flPowerupTime += 5;
	else
		pPlayer->m_flPowerupTime = 5;
	pPlayer->m_iHeldPowerup = timed;
	pPlayer->m_flPowerupExpireTime = 0;
}

