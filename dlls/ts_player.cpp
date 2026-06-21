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
int gmsgTSPowerUpTime = 0;
int gmsgTSMotionBlur = 0;

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

// @ DeactivateSlowMotionFlow__11CBasePlayer — restore this player to normal speed.
void CBasePlayer::DeactivateSlowMotionFlow()
{
	m_flSlowMotionRamp = 0;
	m_flSlowMotionScale = 0;
	m_bSlowBullets = 0;
	// SLICE: faithful also restores SlowMotionFlow's affected players/projectiles.
}

// @ UsePowerUp__11CBasePlayer — `usepowerup` command: activate the held powerup.
bool CBasePlayer::UsePowerUp()
{
	if (!IsAlive()) // SLICE: original also gates on a frozen-state pev flag
		return false;

	if (m_iHeldPowerup == 0 || m_iPowerups != 0)
		return false; // nothing held, or one is already active

	// SLICE: the original can forbid slow-motion via a game-mode flag (DAT_001781b8);
	// here slow-motion is always permitted.
	m_iPowerups = m_iHeldPowerup;
	m_flPowerupExpireTime = m_flPowerupTime + gpGlobals->time;

	if (gmsgTSPowerUpTime == 0)
		gmsgTSPowerUpTime = REG_USER_MSG("TSPowerUpTime", -1);
	MESSAGE_BEGIN(MSG_ONE, gmsgTSPowerUpTime, NULL, ENT(pev));
	WRITE_BYTE((int)(m_flPowerupExpireTime - gpGlobals->time + 0.5f));
	MESSAGE_END();

	// Slow-motion / slow-pause kick off the motion-blur effect (1 / 2).
	int blur = 0;
	if (m_iPowerups & TS_PWUP_SLOWMO)
		blur = 1;
	else if (m_iPowerups & TS_PWUP_SLOWPAUSE)
		blur = 2;
	if (blur)
	{
		if (gmsgTSMotionBlur == 0)
			gmsgTSMotionBlur = REG_USER_MSG("TSMotionBlur", -1);
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
			// double-firerate (no slow-motion): only the low-gravity tweak applies
			if (m_iPowerups & TS_PWUP_LOWGRAV)
				pev->gravity = 0.15f;
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
			// slow-pause (slower, slows bullets too)
			m_bSlowBullets = 1;
			m_flSlowMotionRamp = 0.225f;
			m_flSlowMotionScale = 0.8f;
		}

		if (gpGlobals->time <= m_flPowerupExpireTime && IsAlive())
			return; // still active

		// expired (or dead): SLICE re-drop deferred; restore low-gravity tweak
		if (m_iPowerups & TS_PWUP_LOWGRAV)
			pev->gravity = 1.0f;
	}

	m_flPowerupExpireTime = 0;
	m_flPowerupTime = 0;
	m_iPowerups = 0;
	m_iHeldPowerup = 0;
}
