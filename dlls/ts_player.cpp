/***
*  ts_player.cpp — RECONSTRUCTED TS player extensions (operate on CBasePlayer).
*  Functional/source-faithful (see CLAUDE.md).
***/
#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "player.h"
#include "ts_gamerules.h" // TS_PWUP_* bit constants
#include "ts_player.h"
#include "ts_utils.h"     // TSHudMessageOne

// Lazily-registered TS powerup user messages (see ts_player.h).
int gmsgTSPowerUpTime = 0;  // "PwUpTime" 1 — active countdown (UsePowerUp)
int gmsgTSMotionBlur = 0;   // "TSMBlur"  2 — entindex + 1/2 (UsePowerUp)
int gmsgTSPowerUp = 0;      // "PwUp"     3 — held type (short) + duration (byte) — the held-powerup HUD (UpdateClientData)
int gmsgTSSlowAmt = 0;      // "TSSlowMo" 2 — coord(100*applied slow) — drives the client green slow-mo overlay
// SLICE: gmsgTSBulletTime "TSBTime" (byte 0x175) drives the bullet-time render variant; deferred.

// Lazily-registered TS custom-HUD user messages. Names + fixed sizes are byte-faithful
// to the original LinkUserMessages (reference/HUD_MESSAGES.md): TSHealth/TSArmor are a
// single byte, TSCash a short. The original sends these (not the stock Health/Battery)
// from CBasePlayer::UpdateClientData; we do the same from TSUpdateClientData() below.
static int gmsgTSHealth = 0;
static int gmsgTSArmor = 0;
static int gmsgTSCash = 0;

// @ part of CBasePlayer::UpdateClientData @0x81d00 — the TS per-frame HUD sends. Each
// message is throttled-by-change (only sent when the cached value differs) exactly as
// the original does. Health/armor are rounded and clamped to a byte; cash is a short.
void CBasePlayer::TSUpdateClientData()
{
	if (gmsgTSHealth == 0)
		gmsgTSHealth = REG_USER_MSG("TSHealth", 1);
	if (gmsgTSArmor == 0)
		gmsgTSArmor = REG_USER_MSG("TSArmor", 1);
	if (gmsgTSCash == 0)
		gmsgTSCash = REG_USER_MSG("TSCash", 2);

	// TSHealth — round(health), clamped to >= 0 (the original ROUNDs pev->health and
	// floors negatives at 0 before the WRITE_BYTE).
	int health = (int)(pev->health + 0.5f);
	if (health < 0)
		health = 0;
	if (health != m_iClientTSHealth)
	{
		m_iClientTSHealth = health;
		MESSAGE_BEGIN(MSG_ONE, gmsgTSHealth, NULL, ENT(pev));
		WRITE_BYTE(health);
		MESSAGE_END();
	}

	// TSArmor — round(armorvalue).
	int armor = (int)(pev->armorvalue + 0.5f);
	if (armor < 0)
		armor = 0;
	if (armor != m_iClientTSArmor)
	{
		m_iClientTSArmor = armor;
		MESSAGE_BEGIN(MSG_ONE, gmsgTSArmor, NULL, ENT(pev));
		WRITE_BYTE(armor);
		MESSAGE_END();
	}

	// TSCash — the player's cash (a short).
	if (m_iCash != m_iClientTSCash)
	{
		m_iClientTSCash = m_iCash;
		MESSAGE_BEGIN(MSG_ONE, gmsgTSCash, NULL, ENT(pev));
		WRITE_SHORT(m_iCash);
		MESSAGE_END();
	}

	// PwUp — the HELD powerup HUD (the green top-left icon + "seconds available"). The original
	// sends gmsgTSPowerUp on change from UpdateClientData: WriteShort(held type 0x728) +
	// WriteByte(duration 0x730). The client shows the icon for the held type and the seconds
	// available; double-tap +use activates it (then PwUpTime drives the live countdown).
	if (gmsgTSPowerUp == 0)
		gmsgTSPowerUp = REG_USER_MSG("PwUp", 3);
	int heldDur = (int)m_flPowerupTime;
	if (m_iHeldPowerup != m_iClientHeldType || heldDur != m_iClientHeldDur)
	{
		m_iClientHeldType = m_iHeldPowerup;
		m_iClientHeldDur = heldDur;
		MESSAGE_BEGIN(MSG_ONE, gmsgTSPowerUp, NULL, ENT(pev));
		WRITE_SHORT(m_iHeldPowerup);
		WRITE_BYTE(heldDur);
		MESSAGE_END();
	}

	// TSSlowMo — the PERCEIVED WORLD slow amount (100 = normal). Drives the client sound-pitch + green
	// overlay. This is NOT the activator's move factor (0.6/0.8 = pev->fuser1, which only governs how
	// fast THEY move/fire); it's the slowdown of the world they perceive: 0.35 slowmo / 0.225 slowpause
	// (RE values 35 / 22.5 / 100). For a LOS VICTIM that's their own applied slow (fuser1); for the
	// ACTIVATOR it's the others-scale they impose (m_flSlowMotionRamp) — so both hear/see the same wash.
	if (gmsgTSSlowAmt == 0)
		gmsgTSSlowAmt = REG_USER_MSG("TSSlowMo", 2);
	float perceived = pev->fuser1; // victim's applied slow (or 1.0 normal)
	if ((m_iPowerups & (TS_PWUP_SLOWMO | TS_PWUP_SLOWPAUSE)) && m_flSlowMotionRamp > 0.0f && m_flSlowMotionRamp < perceived)
		perceived = m_flSlowMotionRamp; // the activator perceives the world at the others-scale
	float slowAmt = perceived * 100.0f;
	if (slowAmt != m_flClientSlowAmt)
	{
		m_flClientSlowAmt = slowAmt;
		MESSAGE_BEGIN(MSG_ONE, gmsgTSSlowAmt, NULL, ENT(pev));
		WRITE_COORD(slowAmt);
		MESSAGE_END();
	}
}

// @ SlowMotion__FP11CBasePlayer
bool SlowMotion(CBasePlayer *pPlayer)
{
	return (pPlayer->m_iPowerups & TS_PWUP_SLOWMO) != 0;
}

// @ InfiniteClips__FP11CBasePlayer
bool InfiniteClips(CBasePlayer *pPlayer)
{
	return (pPlayer->m_iPowerups & TS_PWUP_INFCLIP) != 0;
}

// @ SlowPause__FP11CBasePlayer
bool SlowPause(CBasePlayer *pPlayer)
{
	return (pPlayer->m_iPowerups & TS_PWUP_SLOWPAUSE) != 0;
}

// @ DoubleFireRate__FP11CBasePlayer
bool DoubleFireRate(CBasePlayer *pPlayer)
{
	return (pPlayer->m_iPowerups & TS_PWUP_DBLFIRE) != 0;
}

// @ KungFu__FP11CBasePlayer
int KungFu(CBasePlayer *pPlayer)
{
	return pPlayer->m_iKungFu;
}

// ---------------------------------------------------------------------------
// Powerup activation state machine (the held timed powerups: slow-motion,
// slow-pause, double-firerate). Reconstructed from CBasePlayer::UsePowerUp
// (@0x85980), UpdatePowerUpState (@0x85748) and DeactivateSlowMotionFlow
// (@0x86a9c). The held powerup (set by PlayerAddPowerUp on pickup) becomes
// *active* on the `usepowerup` command, runs for its duration, then expires.
//
// SLICE: this is the SERVER-SIDE state machine + the player's own slow-motion
// fields (m_flSlowMotionScale / m_flSlowMotionRamp / m_bSlowBullets). The actual
// time-dilation MOVEMENT, the SlowMotionFlow propagation to nearby players /
// projectiles (@0x86bb0, 1758 bytes — populates the affected-entity arrays this
// deactivate would otherwise walk), the per-entity SetSlowMotion virtual, and the
// client-side motion-blur are the deeper layer (deferred). The death re-drop of an
// in-use powerup (CreatePowerUp on the corpse, gated by objective game mode) is
// also deferred (precache-window + game-mode dependency).
// ---------------------------------------------------------------------------

// @ GoSlow__11CBasePlayerfc — apply a time-scale to this player (1 = normal speed).
void CBasePlayer::GoSlow(float scale, int slowBullets)
{
	// SLICE: the original zeroes the applied factor when the player is frozen
	// (a pev state flag); here it always takes.
	m_flSlowMotionGo = scale;
	m_flSlowMotionTarget = scale;
	m_bSlowBullets = slowBullets != 0;
}

// @ InSlow__11CBasePlayer — is this player currently slowed?
int CBasePlayer::InSlow()
{
	return (m_flSlowMotionGo != 1.0f || m_flSlowMotionTarget != 1.0f) ? 1 : 0;
}

// @ DeactivateSlowMotionFlow__11CBasePlayer (@0x86a9c) — slow-motion ended: restore THIS player to
// normal speed and restore every player it was slowing (unless they're now slowing themselves), then
// clear the cache.
void CBasePlayer::DeactivateSlowMotionFlow()
{
	GoSlow(1.0f, 0);
	m_flSlowMotionRamp = 0;
	m_flSlowMotionScale = 0;
	m_bSlowBullets = 0;
	for (int i = 0; i < 32; i++)
	{
		CBaseEntity *e = m_hSlowPlayers[i];
		if (!e)
			continue;
		CBasePlayer *p = (CBasePlayer *)e;
		if (p->m_flSlowMotionScale == 0.0f) // they're not self-slowing → restore them
		{
			p->GoSlow(1.0f, 0);
			p->m_bSlowBullets = 0;
		}
		m_hSlowPlayers[i] = NULL;
	}
	// SLICE: the original also restores cached slowed projectiles (grenade/knife/WorldGun, 0x208[]);
	// we don't slow projectiles yet (needs a CBaseEntity::GoSlow virtual), so there's nothing to restore.
}

// @ SlowMotionFlow__11CBasePlayer (@0x86bb0) — runs every frame from PreThink; self-gated to fire
// every 0.5s while this player has an active slow powerup (m_flSlowMotionScale != 0). It slows THIS
// player to the self-scale (0.6 slowmo / 0.8 slowpause) and every LOS-visible OTHER player to the
// others-scale (0.35 / 0.225) — so the activator outruns the slowed world. A stronger slow wins ties.
// The slowed players are cached so DeactivateSlowMotionFlow can restore them.
// SLICE: the original also slows moving grenade/knife/WorldGun entities in LOS (deferred — needs a
// CBaseEntity::GoSlow virtual); here only players are slowed (the bullet-time-vs-enemies core).
void CBasePlayer::SlowMotionFlow()
{
	if (m_flSlowMotionScale == 0.0f || m_flNextSlowFlow > gpGlobals->time)
		return;
	m_flNextSlowFlow = gpGlobals->time + 0.5f;

	Vector myEye = pev->origin + pev->view_ofs;
	bool overridden = false;

	// Pass 1: decide which other players I slow (line of sight), honoring slow-tier conflicts.
	for (int i = 1; i <= gpGlobals->maxClients && !overridden; i++)
	{
		CBaseEntity *cached = m_hSlowPlayers[i - 1];
		if (cached)
		{
			if (((CBasePlayer *)cached)->m_flSlowMotionScale == 0.0f)
				continue;            // still just a victim → keep it cached
			m_hSlowPlayers[i - 1] = NULL; // they started self-slowing → drop & re-resolve
		}
		CBasePlayer *other = (CBasePlayer *)UTIL_PlayerByIndex(i);
		if (!other || !other->IsPlayer() || other == this)
			continue;
		if (other->m_flSlowMotionScale != 0.0f && other->m_flSlowMotionRamp == m_flSlowMotionRamp)
			continue;                // already slowed at my tier by someone
		Vector otherEye = other->pev->origin + other->pev->view_ofs;
		TraceResult tr;
		UTIL_TraceLine(myEye, otherEye, ignore_monsters, ignore_glass, edict(), &tr);
		if (tr.flFraction >= 1.0f)   // clear line of sight
		{
			if (other->m_flSlowMotionScale == 0.0f)
				m_hSlowPlayers[i - 1] = other;          // not slowing themselves → I slow them
			else if (m_flSlowMotionRamp <= other->m_flSlowMotionRamp)
				m_hSlowPlayers[i - 1] = other;          // my slow is equal/stronger → I slow them
			else
				overridden = true;                       // they're slowing harder → abort this tick
		}
	}
	if (overridden)
		return;

	// Apply: slow self, then the cached others to the others-scale.
	GoSlow(m_flSlowMotionScale, m_bSlowBullets);
	for (int i = 0; i < gpGlobals->maxClients && i < 32; i++)
	{
		CBaseEntity *e = m_hSlowPlayers[i];
		if (!e)
			continue;
		CBasePlayer *p = (CBasePlayer *)e;
		if (p->m_flSlowMotionScale == 0.0f || p->m_flSlowMotionRamp != m_flSlowMotionRamp)
		{
			p->GoSlow(m_flSlowMotionRamp, 0);
			p->m_bSlowBullets = m_bSlowBullets;
		}
		else
			m_hSlowPlayers[i] = NULL;
	}
}

// @ UsePowerUp__11CBasePlayer (@0x85980) — the `usepowerup` command: ACTIVATE the held powerup.
// The double-tap +use that issues this command is detected CLIENT-side (cl_dll/input.cpp CreateMove,
// gated by cl_usepowerup, 0.30s window). Activation just promotes held→active + sets the expire time
// + notifies the client; the actual slowing happens in UpdatePowerUpState (sets the slow triple) +
// SlowMotionFlow (propagates to LOS enemies) each frame. Returns true if it took.
bool CBasePlayer::UsePowerUp()
{
	if (pev->iuser1 != 0 || !IsAlive())   // spectating/frozen, or dead
		return false;
	if (m_iHeldPowerup == 0 || m_iPowerups != 0)
		return false; // nothing held, or one is already active

	// SLICE: the objective game modes can forbid activating slow-motion (gamerules[0x18] flag) →
	// the original refuses with TSHudMessage "You can't use Slow-Motion now!" (color c3,2d,2d). We
	// don't model that mode flag yet, so slow-motion is always permitted here.

	m_iPowerups = m_iHeldPowerup;
	m_flPowerupExpireTime = (float)(int)m_flPowerupTime + gpGlobals->time;

	// PwUpTime — the green countdown the client ticks down locally (whole seconds remaining).
	if (gmsgTSPowerUpTime == 0)
		gmsgTSPowerUpTime = REG_USER_MSG("PwUpTime", 1);
	MESSAGE_BEGIN(MSG_ONE, gmsgTSPowerUpTime, NULL, ENT(pev));
	WRITE_BYTE((int)(m_flPowerupExpireTime - gpGlobals->time + 0.5f));
	MESSAGE_END();

	// TSMBlur — kick off the client motion-blur (1 = slow-motion, 2 = slow-pause). Only the two
	// slow powerups send it (double-firerate doesn't). MSG_PVS so nearby players see the wash too.
	int blur = (m_iHeldPowerup & TS_PWUP_SLOWMO) ? 1 : (m_iHeldPowerup & TS_PWUP_SLOWPAUSE) ? 2 : 0;
	if (blur)
	{
		if (gmsgTSMotionBlur == 0)
			gmsgTSMotionBlur = REG_USER_MSG("TSMBlur", 2);
		MESSAGE_BEGIN(MSG_PVS, gmsgTSMotionBlur, pev->origin);
		WRITE_BYTE(ENTINDEX(edict()));
		WRITE_BYTE(blur);
		MESSAGE_END();
	}
	return true;
}

// @ UpdatePowerUpState__11CBasePlayer — per-frame (PreThink): apply the active
// powerup's slow-motion fields, expire it when its time runs out.
void CBasePlayer::UpdatePowerUpState()
{
	if (m_iHeldPowerup == 0)
		return;

	if (m_iPowerups == 0)
	{
		// Held but not activated: wait (alive) — SLICE: re-drop on death deferred.
		if (IsAlive())
			return;
	}
	else
	{
		if ((m_iPowerups & (TS_PWUP_SLOWMO | TS_PWUP_SLOWPAUSE)) == 0)
		{
			// double-firerate (no slow-motion): nothing to apply here. (Super-jump/low-gravity is an
			// INDEPENDENT powerup driven from PreThink — m_bLowGravity — not this held-slot machine.)
		}
		else if (m_flPowerupExpireTime < gpGlobals->time || !IsAlive())
		{
			DeactivateSlowMotionFlow();
		}
		else if ((m_iPowerups & TS_PWUP_SLOWPAUSE) == 0)
		{
			// slow-motion
			if (m_iPowerups & TS_PWUP_SLOWMO)
			{
				m_bSlowBullets = 0;
				m_flSlowMotionRamp = 0.35f;
				m_flSlowMotionScale = 0.6f;
			}
		}
		else
		{
			// slow-pause: the player experience MATCHES slow-motion (same self-slow → fire/move don't
			// speed up), and ONLY the bullets go slower (the m_bSlowBullets pause flag → GetBulletVelocityMult
			// ×10 denominator, so slow-pause bullets crawl/dodgeable). USER-DIRECTED: the RE self-scale here
			// is 0.8 (less self-slow than slow-mo, "bullet-time" style) which made fire speed UP vs slow-mo;
			// the user wants slow-pause to NOT fire faster than slow-mo ("slow-pause is all that, except the
			// bullets are slower"), so the self-slow is slow-mo's 0.6. (Flagged as a deviation from the literal
			// RE 0.8 — live-trace the original to pin it if exactness is wanted.)
			m_bSlowBullets = 1;
			m_flSlowMotionRamp = 0.225f;
			m_flSlowMotionScale = 0.6f;
		}

		if (gpGlobals->time <= m_flPowerupExpireTime && IsAlive())
			return; // still active
		// expired (or dead): SLICE re-drop deferred. (Low-gravity is independent — see PreThink.)
	}

	m_flPowerupExpireTime = 0;
	m_flPowerupTime = 0;
	m_iPowerups = 0;
	m_iHeldPowerup = 0;
}
