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
#include "ts_utils.h"     // TSHudMessageOne/All
#include "ts_flag.h"      // CTSFlag::CreateFlag (CTF drop)
#include "ts_weapons.h"   // TSPlayerFreeSlots (slot-weight gate)
#include "ts_gun.h"       // GetPlayerTSGun (block-bar for GetMeleeMultiplier)
#include "ts_worldgun.h"  // CTSAmmoPack::CreateAmmoPack (DropArmor)
#include "ts_bomb.h"      // CTSBomb::CutWire (the cutwire_<n> command helper)
#include "game.h"         // friendlyfire cvar (the melee twins' teammate gate)
#include "gamerules.h"    // g_pGameRules->PlayerRelationship
#include "weapons.h"      // ClearMultiDamage/ApplyMultiDamage

extern int gmsgTSHaveFlag; // wire name "TSFlag" (4) — defined in ts_flag.cpp

// Kill-flavor globals (@0x104480/81/82 in the original .data) — set around TraceAttack/
// ApplyMultiDamage on a melee/coldcock/headshot kill blow; read by TSGetPointsForFrag +
// PlayerKilled (death-notice weapon string "Kung Fu"/"<weapon> Stock" + #TS_Coldcock HUD)
// + Killed. Wire-visible via the kill feed. RE dossier-melee.md §1.4.
bool glast_closecombat = false;
bool glast_coldcock = false;
bool glast_headshot = false;

// Lazily-registered TS powerup user messages (see ts_player.h).
int gmsgTSPowerUpTime = 0;  // "PwUpTime" 1 — active countdown (UsePowerUp)
int gmsgTSMotionBlur = 0;   // "TSMBlur"  2 — entindex + 1/2 (UsePowerUp)
int gmsgTSPowerUp = 0;      // "PwUp"     3 — held type (short) + duration (byte) — the held-powerup HUD (UpdateClientData)
int gmsgTSSlowAmt = 0;      // "TSSlowMo" 2 — coord(100*applied slow) — drives the client green slow-mo overlay
int gmsgKFuPower = 0;       // "KFuPower" 1 — the melee stamina bar (CHAR, 0..100); sent ONLY by UpdateMeleeBar
int gmsgTSKickAlign = 0;    // "TSKickAlign" — DEAD in the original: the global exists but is never
                            // registered or sent (star addendum). Kept for symbol parity only.

// Lazily-registered TS custom-HUD user messages. Names + fixed sizes are byte-faithful
// to the original LinkUserMessages (reference/HUD_MESSAGES.md): TSHealth/TSArmor are a
// single byte, TSCash a short. The original sends these (not the stock Health/Battery)
// from CBasePlayer::UpdateClientData; we do the same from TSUpdateClientData() below.
static int gmsgTSHealth = 0;
static int gmsgTSArmor = 0;
static int gmsgTSCash = 0;
static int gmsgTSState = 0;      // "TSState" 1 — posture byte 0x498 (star_TSState_TSSpace_TSBTime.md)
static int gmsgTSSpace = 0;      // "TSSpace" 1 — free carry slots (int 0x548)
static int gmsgTSBulletTime = 0; // "TSBTime" 1 — the bullet-time variant flag (byte 0x175 = m_bSlowBullets)
static int gmsgTSPlayerAward = 0; // "TSPAward" 2 — MSG_ALL: entindex + award byte 0x74c (star_TSPAward.md)
static int gmsgTSWires = 0;       // "Wires" 3 — the nearby-bomb wire triple, MSG_ONE on change (star_Wires.md)

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

	// TSState — the posture byte (0 stand / 1 duck / 2 stunt-prone), source 0x498 / cache 0x499.
	// Plain on-change, NO ResetHUD-forced resend (the original's InitHUD memset deliberately
	// starts at 0x49A and skips this cache). Original send order: TSArmor -> TSState -> TSCash.
	if (gmsgTSState == 0)
		gmsgTSState = REG_USER_MSG("TSState", 1);
	if (m_iPlayerState != m_iClientTSState)
	{
		m_iClientTSState = m_iPlayerState;
		MESSAGE_BEGIN(MSG_ONE, gmsgTSState, NULL, ENT(pev));
		WRITE_BYTE(m_iPlayerState);
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

	// TSSpace — the free carry slots. The original sends the MAINTAINED int at 0x548 directly
	// (no clamp at the write); our slots derive from the arsenal, so the recompute IS the member.
	if (gmsgTSSpace == 0)
		gmsgTSSpace = REG_USER_MSG("TSSpace", 1);
	const int freeSlots = TSPlayerFreeSlots(this);
	if (freeSlots != m_iClientTSSpace)
	{
		m_iClientTSSpace = freeSlots;
		MESSAGE_BEGIN(MSG_ONE, gmsgTSSpace, NULL, ENT(pev));
		WRITE_BYTE(freeSlots);
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

	// TSBTime — the bullet-time variant flag: source byte 0x175 (m_bSlowBullets — the SAME byte,
	// by construction; star addendum settles the 0x175-vs-[0x5d] ambiguity), cache 0x174.
	// Client consumer: the recoil x0.66667 reduction while in bullet-time.
	if (gmsgTSBulletTime == 0)
		gmsgTSBulletTime = REG_USER_MSG("TSBTime", 1);
	if ((int)m_bSlowBullets != m_iClientBulletTime)
	{
		m_iClientBulletTime = m_bSlowBullets;
		MESSAGE_BEGIN(MSG_ONE, gmsgTSBulletTime, NULL, ENT(pev));
		WRITE_BYTE(m_bSlowBullets);
		MESSAGE_END();
	}

	// TSPAward — the broadcast award/status byte (source 0x74c, cache 0x74d): MSG_ALL keyed by
	// entindex so every client tracks all players' status (scoreboard/observer UI). On-change
	// only, no forced resend (star_TSPAward.md — the sole send site in the original).
	if (gmsgTSPlayerAward == 0)
		gmsgTSPlayerAward = REG_USER_MSG("TSPAward", 2);
	if (m_iAward != m_iClientAward)
	{
		m_iClientAward = m_iAward;
		MESSAGE_BEGIN(MSG_ALL, gmsgTSPlayerAward);
		WRITE_BYTE(entindex());
		WRITE_BYTE(m_iAward);
		MESSAGE_END();
	}

	// Wires — the nearby-bomb wire ROLE triple (source 0x78c-0x78e, cache 0x78f-0x791):
	// MSG_ONE on any change of the three bytes (star_Wires.md). CheckForWires (PreThink,
	// mode-3 maps, 0.5 s throttle) maintains the source; zeros = walked away.
	if (gmsgTSWires == 0)
		gmsgTSWires = REG_USER_MSG("Wires", 3);
	if (m_iWires[0] != m_iClientWires[0] || m_iWires[1] != m_iClientWires[1] ||
		m_iWires[2] != m_iClientWires[2])
	{
		m_iClientWires[0] = m_iWires[0];
		m_iClientWires[1] = m_iWires[1];
		m_iClientWires[2] = m_iWires[2];
		MESSAGE_BEGIN(MSG_ONE, gmsgTSWires, NULL, ENT(pev));
		WRITE_BYTE(m_iWires[0]);
		WRITE_BYTE(m_iWires[1]);
		WRITE_BYTE(m_iWires[2]);
		MESSAGE_END();
	}
}

// @0x82f08 CheckForWires — clear the wire triple, then sphere-64 scan for an ACTIVE ts_bomb
// this player CanInteract with; the LAST match in the sphere wins (no break). The copied bytes
// are the bomb's wire ROLE codes (0 cut / 1 defuse / 2 boobytrap / 3 intact) — SetRandomWires
// deals {3,3,3} then one 1 and one 2. Caller: PreThink, mode-3 maps only, throttled 0.5 s.
void CBasePlayer::CheckForWires()
{
	m_iWires[0] = m_iWires[1] = m_iWires[2] = 0;
	CBaseEntity *e = nullptr;
	while ((e = UTIL_FindEntityInSphere(e, pev->origin, 64.0f)) != nullptr)
	{
		if (!FClassnameIs(e->pev, "ts_bomb"))
			continue;
		CTSBomb *pBomb = (CTSBomb *)e;
		if (!pBomb->m_bActive || !pBomb->CanInteract(this))
			continue;
		m_iWires[0] = (byte)pBomb->m_wires[0];
		m_iWires[1] = (byte)pBomb->m_wires[1];
		m_iWires[2] = (byte)pBomb->m_wires[2];
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

// ---------------------------------------------------------------------------
// CTF flag carry (CBasePlayer::TakeFlag / DropFlag). The carry state is the 28-byte
// s_flag_status copied inline onto the player (m_flagStatus); the loose flag entity is
// CTSFlag (ts_flag.cpp). Carrying costs 20 slot-weight (heavier → slower, via the slot
// speed helper) and a flag can only be carried one at a time.
// ---------------------------------------------------------------------------

// @0x83830 TakeFlag — copy a flag's status onto this player (the pickup). Refuses if the player is
// over the slot-weight budget or is already carrying a flag. Returns true if taken.
bool CBasePlayer::TakeFlag(s_flag_status *src)
{
	if (TSPlayerFreeSlots(this) < 20 || m_flagStatus.active)
		return false; // not enough free slots, or already carrying

	m_flagStatus = *src;     // copy the full 28-byte status (incl. the source lastCarrier)
	m_flagStatus.active = 1; // now carried (slot-weight debit is folded into GetSpeedBySlots)
	EMIT_SOUND(ENT(pev), CHAN_ITEM, "items/gunpickup2.wav", VOL_NORM, ATTN_NORM);

	if (gmsgTSHaveFlag == 0)
		gmsgTSHaveFlag = REG_USER_MSG("TSFlag", 4);
	MESSAGE_BEGIN(MSG_ALL, gmsgTSHaveFlag);
	WRITE_BYTE(1); // type 1 = pickup
	WRITE_BYTE(m_flagStatus.flagID);
	WRITE_BYTE(m_flagStatus.team + 1);
	WRITE_BYTE(ENTINDEX(edict()));
	MESSAGE_END();

	// Announce a NEW carrier only (anti-respam: skip if this player already last carried it).
	int idx = ENTINDEX(edict());
	if (m_flagStatus.lastCarrier != idx)
	{
		const char *contents = !FStringNull(m_flagStatus.contents) ? STRING(m_flagStatus.contents) : "a flag";
		const char *name = pev->netname ? STRING(pev->netname) : "Someone";
		TSHudMessageAll(0, 0xff, 0, 4, 20,
			UTIL_VarArgs("%s picked up a briefcase containing %s!", name, contents));
		m_flagStatus.lastCarrier = idx;
		UTIL_LogPrintf("\"%s<%i>\" triggered \"aquired_briefcase\" (contents \"%s\")\n",
			name, GETPLAYERUSERID(edict()), contents);
	}
	return true;
}

// @0x8334c DropFlag — give up the carried flag. bSilent (capture) just clears the carrier; a normal
// drop (death/disconnect/spectate/DropWeapon) spawns a fresh loose CTSFlag tossed from the player.
void CBasePlayer::DropFlag(bool bSilent)
{
	if (!m_flagStatus.active)
		return;

	if (!bSilent)
	{
		CTSFlag *pFlag = CTSFlag::CreateFlag(this, &m_flagStatus);
		if (pFlag)
		{
			Vector forward, dir;
			float dirSpeed;
			if (!IsAlive())
			{
				// dead → fling in a random direction
				Vector ang(0, RANDOM_FLOAT(0, 360), 0);
				UTIL_MakeVectors(ang);
				forward = gpGlobals->v_forward;
				dirSpeed = RANDOM_FLOAT(60, 160);
			}
			else
			{
				UTIL_MakeVectors(pev->v_angle);
				forward = gpGlobals->v_forward;
				dirSpeed = 200.0f; // VERIFY: alive-drop toss speed (dead uses 60..160)
			}
			UTIL_SetOrigin(pFlag->pev, pev->origin + pev->view_ofs + forward * 16.0f);
			pFlag->pev->velocity = pev->velocity * 0.9f + forward * dirSpeed;
			pFlag->pev->avelocity = Vector(RANDOM_FLOAT(-300, 300), RANDOM_FLOAT(-300, 300),
				RANDOM_FLOAT(-300, 300));
		}

		if (gmsgTSHaveFlag == 0)
			gmsgTSHaveFlag = REG_USER_MSG("TSFlag", 4);
		MESSAGE_BEGIN(MSG_ALL, gmsgTSHaveFlag);
		WRITE_BYTE(2); // type 2 = drop
		WRITE_BYTE(m_flagStatus.flagID);
		WRITE_BYTE(m_flagStatus.team + 1);
		WRITE_BYTE(ENTINDEX(edict()));
		MESSAGE_END();

		const char *name = pev->netname ? STRING(pev->netname) : "Someone";
		UTIL_LogPrintf("\"%s<%i>\" triggered \"dropped_briefcase\"\n", name, GETPLAYERUSERID(edict()));
	}

	m_flagStatus = s_flag_status(); // clear the carried state (active = 0)
}

// @0x83068 SetPlayerSpeed — clamp the requested base speed to [0, 330] and push it to the engine
// (client prediction) via pfnSetClientMaxspeed. Stores the clamped value.
void CBasePlayer::SetPlayerSpeed(float speed)
{
	if (speed > 330.0f)
		speed = 330.0f;
	if (speed < 0.0f)
		speed = 0.0f;
	m_flSetMaxSpeed = speed;
	g_engfuncs.pfnSetClientMaxspeed(edict(), (float)(int)(m_flSetMaxSpeed + 0.5f));
}

// @0x85b70 SetPlayerGravity — set the player's gravity, but only when they're at normal gravity
// (>= 1.0) so an already-applied low-gravity effect isn't stomped. Stores the set value.
void CBasePlayer::SetPlayerGravity(float setval, float gravity)
{
	if (pev->gravity >= 1.0f)
	{
		pev->gravity = gravity;
		m_flGravityValue = setval;
	}
}

// @0x85f70 PunchScreen — a no-op in the shipped game (the screen-punch effect was disabled).
void CBasePlayer::PunchScreen(float magnitude, Vector dir)
{
}

// @0x856e0 TSUse — the server-side double-tap +use detector: a second +use within 0.3s activates the
// held powerup (UsePowerUp), then re-arms the window. NB: our build detects the double-tap CLIENT-side
// (cl_usepowerup -> the `usepowerup` command), so this is NOT wired into PreThink to avoid a
// double-activation; kept for source parity / as the server-side alternative.
void CBasePlayer::TSUse()
{
	if (pev->iuser1 == 0 && (m_afButtonPressed & IN_USE))
	{
		if (gpGlobals->time <= m_flUseDoubleTap)
			UsePowerUp();
		m_flUseDoubleTap = gpGlobals->time + 0.3f;
	}
}

// @0x82a08 UpdatePlayerState — derive the pose state: 2 = stunt/locked (an iuser4 lock bit set),
// 1 = ducking (FL_DUCKING = 0x4000), 0 = normal.
void CBasePlayer::UpdatePlayerState()
{
	if (pev->iuser4 & 0x30)
		m_iPlayerState = 2;
	else if (pev->flags & FL_DUCKING)
		m_iPlayerState = 1;
	else
		m_iPlayerState = 0;
}

// @ PickUpWeapon__11CBasePlayer10tsweapon_tPvibT4 (@0x8314c, decompile-exact) — the ONE gate
// every weapon acquisition goes through. Callers + args recovered from the call sites:
//   CTSWorldGun::GetWeapon   (type, &status, 0,      type != FLAG, false)  @0x91224
//   CTSGroundWeapon::GetWeapon(type, NULL,   attach,  type != FLAG, false)  @0x924ff
//   CTSKnife::Use            (type, NULL,    0,       false,        false)  @0x6de93 — NO switch!
//   CBasePlayer::BuyWeapon   (type, NULL,    attach,  true,         true)   @0x846ca — the sole owner-record
//   PlayerAddPowerUp / give  (M61,  NULL,    0,       true,         false)  @0x87d4e/@0x9dbdf
// The int param is the BOUGHT ATTACHMENT MASK (silencer 1/laser 2/flash 4/scope 8 — BuyWeapon
// prices exactly those bits), passed through to TakeWeapon — NOT a clip. Flow: refuse a direct
// akimbo-class pickup (its SINGLE merges instead); the slot gate (a carried STACK charges
// slotCost per unit); TakeWeapon (stack merge / akimbo combine / fresh install with the carried
// status); record the owner EHANDLE(s) if asked; speed recompute; gunpickup2 (CHAN_ITEM,
// attn 0.8); select the picked type — or its AKIMBO if that is owned — when asked OR bare-handed.
bool CBasePlayer::PickUpWeapon(int type, TSItem *pStatus, int iAttach, bool bSelect, bool bRecordOwner)
{
	CTSGun *pGun = GetPlayerTSGun(this); // orig: this->m_pActiveItem gated on m_iId == 0x10
	if (!pGun || type < 0 || type >= TSW_COUNT)
		return false;
	const s_weapon_info *wi = get_weapon_info((tsweapon_t)type);
	if (wi->iFireType == TSFT_AKIMBO && wi->iSingleType != -1)
		return false; // @0x83197 — pick the single up; it merges into the akimbo

	// the slot gate (@0x831bd): a dropped STACK charges slotCost per carried unit
	float weight = (float)wi->slotCost;
	if (wi->iStackable && pStatus && pStatus->clip > 0)
		weight *= (float)pStatus->clip;
	if ((float)TSPlayerFreeSlots(this) - weight < 0.0f)
		return false;

	if (!pGun->TakeWeapon((tsweapon_t)type, pStatus, iAttach))
		return false;

	if (bRecordOwner)
	{
		// item+0x30 (@0x83234): the buyer is recorded on the item — and on its akimbo pair
		// when that is owned (consumer VERIFY — likely the sell-zone's yours-check)
		pGun->m_items[type].hOwner = this;
		if (wi->iAkimboType != -1 && pGun->IsOwned(wi->iAkimboType))
			pGun->m_items[wi->iAkimboType].hOwner = this;
	}

	SetPlayerSpeed(GetSpeedBySlots(this));
	EMIT_SOUND_DYN(ENT(pev), CHAN_ITEM, "items/gunpickup2.wav", VOL_NORM, 0.8f, 0, PITCH_NORM);

	if (bSelect || !pGun->m_pActiveItem)
	{
		int sel = type;
		if (wi->iAkimboType != -1 && pGun->IsOwned(wi->iAkimboType))
			sel = wi->iAkimboType; // picking the single while owning the pair wields the PAIR
		pGun->SelectWeapon(sel);   // ours deploys inside (the original SelectWeapon+Deploy)
	}
	return true;
}

// @ TSInit__11CBasePlayer (@0x82a4c, decompile-exact) — the TS per-life reset, the FIRST call in
// the original's Spawn (@0x7f586). Everything TS about a player resets here: the anim/stunt
// machines, slow-motion, posture + the client change-detect caches (0xff = force the first
// resend), speed, SPAWN PROTECTION (a 1-second grace: the renderfx shimmer + the (200,200,250)
// screen fade + the m_flDisarmTime deadline that TraceAttack, the melee disarm twins and
// PreThink's shimmer-clear all consume, @0x801eb/@0x81458), powerups, the map-global superjump
// mode, the spectate clear, the whole melee/whiff/block/wires/flag block, and the anim state.
void CBasePlayer::TSInit()
{
	m_TSAnim.Init();      // player+0x28c
	m_TSStunt.Init(this); // player+0x4f0
	pev->fuser1 = 1.0f;
	m_flSlowMotionGo = 1.0f;
	m_flSlowMotionTarget = 1.0f;
	m_flClientSlowAmt = -1.0f;
	m_bSlowBullets = 0;         // 0x175
	m_iClientBulletTime = 0xff; // 0x174 — force the first TSBTime send
	m_iPlayerState = 0;         // 0x498
	m_iClientTSState = 0xff;    // 0x499 — force the first TSState send
	// player+0x9e0 = -1 (unnamed transient; no reconstructed state)
	pev->iuser4 = 0;
	SetPlayerSpeed(330.0f);
	// free-slot counter reset (the original's player+0x548 = 81; ours derives from the arsenal)
	m_iClientTSSpace = -1; // 0x54c — force the first TSSpace send
	// player+0x544 = -1 / +0x534 = 0 (unnamed transients)
	m_flDisarmTime = gpGlobals->time + 1.0f; // 0x71c — the SPAWN-PROTECTION deadline
	m_flNextAttack = UTIL_WeaponTimeBase() + 1.0f; // 0x12c (the stock Spawn tail re-arms it later)
	// the spawn-protection shimmer — PreThink clears it once the deadline passes (@0x81458)
	pev->renderamt = 125.0f;
	pev->rendermode = kRenderTransColor;
	pev->rendercolor = Vector(0, 0, 0);
	pev->renderfx = kRenderFxDistort; // 0xe
	UTIL_ScreenFade(this, Vector(200, 200, 250), 0.2f, m_flDisarmTime - gpGlobals->time, 25, 0);
	m_flUseDoubleTap = 0; // 0x164
	// player+0x720 = 0 (unnamed)
	m_iPowerups = 0;
	m_iHeldPowerup = 0;
	m_flPowerupExpireTime = 0;
	m_flPowerupTime = 0;
	m_iClientHeldDur = 0;
	m_iClientHeldType = 0xff;
	// player+0x740/0x744/0x748 = 0 (unnamed transients)
	m_iAward = 0;          // 0x74c
	m_iClientAward = 0xff; // 0x74d — force the first TSPAward send
	m_iKungFu = 0;         // 0x74e
	// the map-global superjump mode (rules+0x5885, ts_mapglobals SF & 0x40): everyone spawns
	// with the low-gravity/superjump physinfo ("mj")
	CTSGameRules *pRules = dynamic_cast<CTSGameRules *>(g_pGameRules);
	if (pRules && pRules->m_bSrvSuperJump)
	{
		m_bLowGravity = 1;
		g_engfuncs.pfnSetPhysicsKeyValue(edict(), "mj", "1");
	}
	else
	{
		m_bLowGravity = 0;
		g_engfuncs.pfnSetPhysicsKeyValue(edict(), "mj", "0");
	}
	// player+0x754 = 0 (unnamed)
	if (pev->iuser1 != 0 || pev->iuser2 != 0)
	{
		// the original broadcasts gmsgSpectator (playerIdx, 0) here. Our "Spectator" message
		// carries a DIFFERENT payload (state, seconds — the spectate-wait HUD; an INVENTED wire
		// vs the original's stock spectator-panel broadcast, flagged for the invented review),
		// so only the observer state clears here. VERIFY: reconcile the Spectator wire.
		pev->iuser1 = 0;
		pev->iuser2 = 0;
	}
	m_flBlockTime = 0;     // 0x4dc
	m_flGravityValue = 0;  // 0x4ec
	m_bHasKevlar = false;  // 0x718
	m_flTSRespawnTime = 0; // 0x770
	// player+0x7bc = 0 (unnamed)
	m_hRunThroughLast = NULL; // 0x788 region
	m_iWires[0] = m_iWires[1] = m_iWires[2] = 0;                   // 0x78c-0x78e
	m_iClientWires[0] = m_iClientWires[1] = m_iClientWires[2] = 0; // 0x78f-0x791
	m_flNextWiresCheck = 0; // 0x794
	m_flMeleeBar = 100.0f;  // 0x774 (0x42c80000)
	// player+0x798 = 0 (unnamed)
	m_flWhiffLungeTime = 0;  // 0x79c
	m_flWhiffLungeForce = 0; // 0x7a0
	m_iWhiffLungeType = 0;   // 0x7a4
	m_flKungFuBlock = 0;     // 0x7a8
	m_flKnifeBlock = 0;      // 0x7ac
	m_flKatanaBlock = 0;     // 0x7b0
	m_flDisarmPressure = 0;  // 0x7b4
	if (m_bTheOne && pRules)
		pRules->m_bTheOneActive = false; // rules+0x5878 — the One left the field
	m_bTheOne = 0; // 0x7b9
	m_flHealthRegen = 0;
	// player+0x184[33] = 0 (an unreconstructed transient block)
	m_flSlowMotionScale = 0;
	m_flSlowMotionRamp = 0;
	m_flNextSlowFlow = 0;
	memset(m_pSlowedEnts, 0, sizeof(m_pSlowedEnts)); // 0x208[32] — the packed slowed array
	m_iSlowedEntCount = 0;                           // 0x288
	// player+0x77c/0x780/0x784 = 0 (unnamed)
	m_flSpreadAccum = 0;
	// player+0x7c0 = 0 (unnamed)
	memset(&m_flagStatus, 0, sizeof(m_flagStatus)); // 0x7d0
	PlayerIsCheating(this, g_engfuncs.pfnGetInfoKeyBuffer(edict()));
	pev->gaitsequence = 0;
	pev->sequence = 0;
	pev->frame = 0;
	ResetSequenceInfo();
	// player+0x84c = 0 (unnamed)
}

// @0x865d0 GetMeleeMultiplier — melee damage scales with the block-bar stamina: full (100) -> 1.0,
// empty (0) -> 0.5. (The block bar is field_0x774 on the original player; we keep it on the TS gun.)
float CBasePlayer::GetMeleeMultiplier()
{
	float bar = m_flMeleeBar; // the bar lives on the PLAYER (+0x774), like the original
	if (bar < 0.0f)
		bar = 0.0f;
	return (bar * 0.5f + 50.0f) * 0.01f;
}

// @0x8650c UpdateMeleeBar — THE melee-stamina drain choke-point (the only writer besides the
// PreThink regen + TSInit refill): subtract, floor at 0 with a 1.5s global attack lockout on
// depletion, then ALWAYS network the bar (KFuPower MSG_ONE CHAR — one send per call, no
// change-detect). Callers: CCombatBlock(33) raise-block, Victim*Hold(attacker, 33) got-blocked,
// the jump-kick (dmg-scaled) and the GoWheel backkick (22.5). No upper clamp here.
void CBasePlayer::UpdateMeleeBar(float flAmount)
{
	m_flMeleeBar -= flAmount;
	if (m_flMeleeBar < 0.01f)
	{
		m_flMeleeBar = 0.0f;
		m_flNextAttack = 1.5f; // depleted: 1.5s global attack lockout (raw store; CLIENT_WEAPONS base 0)
	}
	if (gmsgKFuPower == 0)
		gmsgKFuPower = REG_USER_MSG("KFuPower", 1);
	MESSAGE_BEGIN(MSG_ONE, gmsgKFuPower, NULL, pev);
	WRITE_CHAR((int)(m_flMeleeBar + 0.5f));
	MESSAGE_END();
}

// @0x85b58 BlockPlayer — a movement SELF-ROOT (NOT the deflect-block; 24 bytes in the original):
// arm the stop timer and set the STOP stunt bit (pm_shared TSHandle halves the move command
// while it's set). PreThink counts the timer down and clears the bit on expiry; the stunt reset
// paths zero it. Callers: the CCombatAttack tail (roots the ATTACKER after a swing, refire*0.75)
// + KatanaFire's back/duck slash lock (delay*0.9) — both land in dossier S5/S7b.
void CBasePlayer::BlockPlayer(float flDuration)
{
	m_flBlockTime = flDuration;
	pev->iuser4 |= TSSTUNT_STOP;
}

// @0x85bac FindHullIntersectionplayer — byte-for-byte the stock crowbar FindHullIntersection,
// copied into TS_player.cpp and renamed by the original (avoids the static-linkage collision
// with crowbar.cpp's). Double the reach, straight re-trace, else the 2x2x2 corner loop keeping
// the closest hit (1e6 sentinel).
void FindHullIntersectionplayer(const Vector &vecSrc, TraceResult &tr,
	const float *mins, const float *maxs, edict_t *pEntity)
{
	float distance = 1e6f;
	const float *minmaxs[2] = {mins, maxs};
	TraceResult tmpTrace;

	Vector vecHullEnd = vecSrc + ((tr.vecEndPos - vecSrc) * 2.0f);
	UTIL_TraceLine(vecSrc, vecHullEnd, dont_ignore_monsters, pEntity, &tmpTrace);
	if (tmpTrace.flFraction < 1.0f)
	{
		tr = tmpTrace;
		return;
	}

	for (int i = 0; i < 2; i++)
		for (int j = 0; j < 2; j++)
			for (int k = 0; k < 2; k++)
			{
				Vector vecEnd(vecHullEnd.x + minmaxs[i][0],
							  vecHullEnd.y + minmaxs[j][1],
							  vecHullEnd.z + minmaxs[k][2]);
				UTIL_TraceLine(vecSrc, vecEnd, dont_ignore_monsters, pEntity, &tmpTrace);
				if (tmpTrace.flFraction < 1.0f)
				{
					float thisDistance = (tmpTrace.vecEndPos - vecSrc).Length();
					if (thisDistance < distance)
					{
						tr = tmpTrace;
						distance = thisDistance;
					}
				}
			}
}

// @0x85e28 CheckHull — the shared melee trace: a line trace first; on a miss, a hull trace
// (iHull: 3 = head_hull for most melee, 1 vs the small attack class) refined through
// FindHullIntersectionplayer. tr.flFraction >= 1 after this = a miss. (The original tail
// has an unread vtable-slot-46 read — dead; VERIFY if it ever matters. Hull mins/maxs
// mapping VERIFY: head hull uses the duck hull extents, like the crowbar.)
void CBasePlayer::CheckHull(Vector vecSrc, Vector vecEnd, TraceResult &tr, int iHull)
{
	UTIL_TraceLine(vecSrc, vecEnd, dont_ignore_monsters, edict(), &tr);
	if (tr.flFraction >= 1.0f)
	{
		UTIL_TraceHull(vecSrc, vecEnd, dont_ignore_monsters, iHull, edict(), &tr);
		if (tr.flFraction < 1.0f)
		{
			if (iHull == head_hull)
				FindHullIntersectionplayer(vecSrc, tr, VEC_DUCK_HULL_MIN, VEC_DUCK_HULL_MAX, edict());
			else
				FindHullIntersectionplayer(vecSrc, tr, VEC_HULL_MIN, VEC_HULL_MAX, edict());
		}
	}
}

// The TS close-combat damage marker bit riding TraceAttack's bitsDamageType alongside
// DMG_CLUB|DMG_NEVERGIB (RE CloseCombatAttack @0x8646c: bits 0x4001080).
#define DMG_TSCLOSECOMBAT 0x4000000

// @0x84900 (the -2 disarm path of) DropWeapon(tsweapon_t, float) — RE'd from the decompile
// (scratchpad/dropweapon.c): melee hits ACCUMULATE disarm pressure (player+0x7b4, PreThink-
// decayed at 5x the frame delta); crossing the per-weapon threshold — katana 30 / the melee
// weapon category 25 / clamp(slotCost, 20, 50) for guns — knocks the ACTIVE weapon out and
// resets the accumulator. The flag (TSW_FLAG) and empty hands can't be disarmed.
// Regular-type drops stay on the drop-key DropWeapon() (ts_gun.cpp); only -2 routes here.
bool CBasePlayer::DropWeapon(int iType, float flDamage)
{
	if (iType != -2)
		return false; // only the disarm sentinel is routed through this shape

	CTSGun *pGun = GetPlayerTSGun(this);
	if (!pGun || !m_pActiveItem)
		return false;
	const int wt = pGun->m_weaponType;
	if (wt == TSW_FLAG || wt < 1)
		return false;

	m_flDisarmPressure += flDamage;

	float threshold;
	const s_weapon_info *wi = get_weapon_info((tsweapon_t)wt);
	if (wt == TSW_KATANA)
		threshold = 30.0f;
	else if (wi && wi->iFireType == 3) // the melee weapon category (GetWeaponType()==3)
		threshold = 25.0f;
	else
	{
		int slot = wi ? wi->slotCost : 20;
		threshold = (float)(slot < 21 ? 20 : (slot < 50 ? slot : 50));
	}
	if (m_flDisarmPressure < threshold)
		return false;

	m_flDisarmPressure = 0.0f;
	return DropWeapon(); // knock the active weapon out (the drop machinery, ts_gun.cpp)
}

// @0x85f88 CloseCombatAttack — the shared melee hit-resolver (kung-fu twin; the coldcock sibling
// below sets glast_coldcock instead). Trace via CheckHull, gates, VictimKungfuHold deflect,
// disarm/knockback/sound for player victims, then the marked TraceAttack. Returns the damaged
// entity or NULL (miss/invalid/teammate/blocked). Callers (S5): CTSGun::CCombatAttack
// (hull class==3?1:3, disarm 1, playersOnly 0, knockback 1, teamSafe 0) and
// CTSStunt::CloseAttack (hull 3, iAnim -1, teamSafe 1).
CBaseEntity *CBasePlayer::CloseCombatAttack(Vector vecSrc, Vector vecEnd, Vector vecDir,
	int iHull, float flDamage, int iAnim, bool bDisarm, bool bPlayersOnly, bool bKnockback,
	bool bForceTeamSafe)
{
	if (iAnim != -1)
		m_TSAnim.Animate(this, iAnim);

	// damage-scaled motion-blur roll (dmg 35 => ~35%, > 80 => always; level 2 above 50)
	if (RANDOM_LONG(10, 80) < (int)flDamage)
	{
		if (gmsgTSMotionBlur == 0)
			gmsgTSMotionBlur = REG_USER_MSG("TSMBlur", 2);
		MESSAGE_BEGIN(MSG_PVS, gmsgTSMotionBlur, pev->origin);
		WRITE_BYTE(ENTINDEX(edict()));
		WRITE_BYTE(flDamage > 50.0f ? 2 : 1);
		MESSAGE_END();
	}

	TraceResult tr;
	CheckHull(vecSrc, vecEnd, tr, iHull);
	if (tr.flFraction >= 1.0f)
		return nullptr; // miss

	edict_t *pHitEnt = tr.pHit ? tr.pHit : ENT(0);
	CBaseEntity *pVictim = CBaseEntity::Instance(pHitEnt);
	if (!pVictim)
		return nullptr;
	if (pVictim->pev->takedamage == DAMAGE_NO)
		return nullptr;
	if ((pVictim->pev->flags & FL_WORLDBRUSH) != 0)
		return nullptr;
	if ((pVictim->pev->effects & EF_NODRAW) != 0)
		return nullptr;

	if (pVictim->IsPlayer())
	{
		if ((bForceTeamSafe || friendlyfire.value == 0.0f) &&
			g_pGameRules->PlayerRelationship(pVictim, this) == GR_TEAMMATE)
			return nullptr;

		if (VictimKungfuHold(this, pVictim, flDamage, &tr))
			return nullptr; // deflected (the Hold plays the clack + stamps the locks)

		CBasePlayer *pPlayerVictim = (CBasePlayer *)pVictim;
		if (bDisarm && pPlayerVictim->m_flDisarmTime < gpGlobals->time)
			pPlayerVictim->DropWeapon(-2, flDamage); // -2 = active weapon (roll VERIFY-gated)

		if (bKnockback && flDamage > 30.0f)
		{
			float speed = pev->velocity.Length();
			Vector dir = pVictim->pev->origin - pev->origin;
			dir = dir.Length() > 0.0f ? dir.Normalize() : Vector(0, 0, 1);
			pVictim->pev->velocity = pVictim->pev->velocity + dir * speed;
			pVictim->pev->velocity.z = 235.0f; // hard-set vertical pop (disasm 0x863a0)
		}

		EMIT_SOUND_DYN(ENT(pVictim->pev), CHAN_BODY, "player/kungfuhit.wav", VOL_NORM, 0.8f, 0,
			95 + RANDOM_LONG(0, 31));

		// dead code in the shipped .so (PunchScreen is an empty stub) — shape kept faithful
		if (m_pActiveItem && flDamage > 20.0f)
			PunchScreen(flDamage * 0.5f, pev->origin - pVictim->pev->origin);
	}
	else if (bPlayersOnly)
		return nullptr;

	glast_closecombat = true;
	ClearMultiDamage();
	pVictim->TraceAttack(pev, flDamage, vecDir, &tr, DMG_CLUB | DMG_NEVERGIB | DMG_TSCLOSECOMBAT);
	ApplyMultiDamage(pev, pev);
	glast_closecombat = false;

	return pVictim;
}

// @0x86610 ColdCockAttack — the pistol-whip sibling twin: identical skeleton, sets
// glast_coldcock, and its knockback path is never exercised (the ColdCock caller passes
// bKnockback=false). Damage bits VERIFY (L5): DMG_CLUB|DMG_NEVERGIB kept pending the raw
// disasm read at 0x86610's TraceAttack push.
CBaseEntity *CBasePlayer::ColdCockAttack(Vector vecSrc, Vector vecEnd, Vector vecDir,
	int iHull, float flDamage, int iAnim, bool bDisarm, bool bPlayersOnly, bool bKnockback,
	bool bForceTeamSafe)
{
	if (iAnim != -1)
		m_TSAnim.Animate(this, iAnim);

	if (RANDOM_LONG(10, 80) < (int)flDamage)
	{
		if (gmsgTSMotionBlur == 0)
			gmsgTSMotionBlur = REG_USER_MSG("TSMBlur", 2);
		MESSAGE_BEGIN(MSG_PVS, gmsgTSMotionBlur, pev->origin);
		WRITE_BYTE(ENTINDEX(edict()));
		WRITE_BYTE(flDamage > 50.0f ? 2 : 1);
		MESSAGE_END();
	}

	TraceResult tr;
	CheckHull(vecSrc, vecEnd, tr, iHull);
	if (tr.flFraction >= 1.0f)
		return nullptr;

	edict_t *pHitEnt = tr.pHit ? tr.pHit : ENT(0);
	CBaseEntity *pVictim = CBaseEntity::Instance(pHitEnt);
	if (!pVictim)
		return nullptr;
	if (pVictim->pev->takedamage == DAMAGE_NO)
		return nullptr;
	if ((pVictim->pev->flags & FL_WORLDBRUSH) != 0)
		return nullptr;
	if ((pVictim->pev->effects & EF_NODRAW) != 0)
		return nullptr;

	bool bApplyDamage = true;
	if (pVictim->IsPlayer())
	{
		if ((bForceTeamSafe || friendlyfire.value == 0.0f) &&
			g_pGameRules->PlayerRelationship(pVictim, this) == GR_TEAMMATE)
			return nullptr;

		if (VictimKungfuHold(this, pVictim, flDamage, &tr))
			bApplyDamage = false; // held: no drop/knockback/sound/damage
		else
		{
			CBasePlayer *pPlayerVictim = (CBasePlayer *)pVictim;
			if (bDisarm && pPlayerVictim->m_flDisarmTime < gpGlobals->time)
				pPlayerVictim->DropWeapon(-2, flDamage);

			if (bKnockback && flDamage > 30.0f) // not exercised by coldcock (bKnockback=false)
			{
				float speed = pev->velocity.Length();
				Vector dir = pVictim->pev->origin - pev->origin;
				dir = dir.Length() > 0.0f ? dir.Normalize() : Vector(0, 0, 1);
				pVictim->pev->velocity = pVictim->pev->velocity + dir * speed;
				pVictim->pev->velocity.z = 235.0f;
			}

			EMIT_SOUND_DYN(ENT(pVictim->pev), CHAN_BODY, "player/kungfuhit.wav", VOL_NORM, 0.8f, 0,
				95 + RANDOM_LONG(0, 31));

			if (m_pActiveItem && flDamage > 20.0f)
				PunchScreen(flDamage * 0.5f, pev->origin - pVictim->pev->origin);
		}
	}
	else if (bPlayersOnly)
		return nullptr;

	if (!bApplyDamage)
		return nullptr;

	glast_coldcock = true;
	ClearMultiDamage();
	pVictim->TraceAttack(pev, flDamage, vecDir, &tr, DMG_CLUB | DMG_NEVERGIB); // bits VERIFY (L5)
	ApplyMultiDamage(pev, pev);
	glast_coldcock = false;

	return pVictim;
}

// @0x83a3c DropArmor — drop the player's armor as a CTSAmmoPack (armor-only) tossed in front of them,
// then zero the armor. (Dead drop randomizes the toss direction.)
void CBasePlayer::DropArmor()
{
	if (pev->armorvalue <= 0.0f)
		return;

	Vector angles = IsAlive() ? pev->v_angle
							   : Vector(RANDOM_FLOAT(-60, 60), RANDOM_FLOAT(-180, 180), 0);
	UTIL_MakeVectors(angles);
	Vector origin = pev->origin + gpGlobals->v_forward * 8.0f + gpGlobals->v_right * 8.0f;

	int armor = (int)(pev->armorvalue + 0.5f);
	CTSAmmoPack::CreateAmmoPack(pev, nullptr, armor, origin, g_vecZero);
	pev->armorvalue = 0;
	EMIT_SOUND(ENT(pev), CHAN_BODY, "weapons/weapondrop.wav", VOL_NORM, ATTN_NORM);
}

// @0x846f0 DropAmmo — drop the active gun's per-caliber reserve as a CTSAmmoPack (no-op if empty),
// tossed in a random nearby direction, then clear the reserve.
void CBasePlayer::DropAmmo()
{
	CTSGun *pGun = GetPlayerTSGun(this);
	if (!pGun)
		return;

	bool any = false;
	for (int c = 0; c < TS_NUM_CALIBERS; c++)
		if (pGun->m_ammoReserve[c] > 0)
		{
			any = true;
			break;
		}
	if (!any)
		return;

	Vector angles(RANDOM_FLOAT(-100, 100), RANDOM_FLOAT(-100, 100), 0);
	UTIL_MakeVectors(angles);
	Vector origin = pev->origin + gpGlobals->v_forward * 8.0f + gpGlobals->v_right * 8.0f;

	CTSAmmoPack::CreateAmmoPack(pev, pGun->m_ammoReserve, 0, origin, g_vecZero);
	for (int c = 0; c < TS_NUM_CALIBERS; c++)
		pGun->m_ammoReserve[c] = 0;
}

// Free helper for the wire-cut console command (the cl_dll wire UI isn't built).
void CBasePlayer::CutWire(int wire)
{
	CBaseEntity *pEnt = nullptr;
	while ((pEnt = UTIL_FindEntityByClassname(pEnt, "ts_bomb")) != nullptr)
		((CTSBomb *)pEnt)->CutWire(wire, this);
}
