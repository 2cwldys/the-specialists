/***
*  ts_stunt.cpp — RECONSTRUCTED CTSStunt (The Specialists stunt/acrobatics system).
*  Source-faithful reconstruction from reference/ts_i386.so (TS_stuntmanager.cpp).
*  Full RE map: reference/STUNT.md. See ts_stunt.h for the iuser4 contract.
*
*  Play() runs each PreThink. It decrements the stunt timers, then (when grounded)
*  dispatches Handle/Go off the buttons. Every stunt writes pev->iuser4 bits; the
*  matching pm_shared hooks (TSHandle in pm_shared.cpp) PREDICT the hull/view/launch
*  on both server and client. Movement is faithful; the 3rd-person stunt POSES
*  (CTSAnimation::Animate 0x28..0x39) are deferred — the TS specialist player models
*  are not in this data extraction (ts_animation.h note), so they no-op for now.
***/
#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "player.h"
#include "weapons.h"     // Clear/ApplyMultiDamage, DMG_CLUB
#include "ts_stunt.h"
#include "ts_weapons.h" // TSPlayerFreeSlots
#include "ts_player.h"  // KungFu()

int gmsgTSNextStunt = 0; // "NextStunt" 1 (registered centrally in LinkUserMessages) — stunt-meter byte (HUD; client handler deferred)
extern int gmsgTSMotionBlur;

// ---- helpers --------------------------------------------------------------
// The original inlines these at their call sites (and GetDirection is a FREE function,
// GetDirection__FG6VectorT0ff — shared with CTSAnimation). We keep them as file-local
// statics so CTSStunt's METHOD surface matches the original 1:1 (like TSWallTrace below).

// time-dilation factor — original calls the player virtual GiveSlowMul() == FixedSlowAmt(fuser1)
// == max(fuser1, 0.01f) (vtable+0x10). 1 = normal time, lower = slow-motion.
static float TS_TimeScale(CBasePlayer *pl)
{
	float ts = pl->pev->fuser1;
	return (ts < 0.01f) ? 0.01f : ts;
}

// 81-slot remaining capacity (CBasePlayer+0x548): lighter loadout = higher dives.
bool CTSStunt::CanDiveHigh()   { return TSPlayerFreeSlots(m_pPlayer) > 39; } // 0x51-cap < 0x2a
bool CTSStunt::CanDiveMedium() { return TSPlayerFreeSlots(m_pPlayer) > 29; } // < 0x34
bool CTSStunt::CanDiveLow()    { return TSPlayerFreeSlots(m_pPlayer) > 19; } // < 0x3e
int  CTSStunt::SlotBonus() { return 0; }     // @0x8a480 — returns 0 in the shipped binary
float CTSStunt::NextDive()  { return 0.0f; } // @0x8a59c — returns 0 (dive stunt-meter time)

// gmsgTSNextStunt drives the client stunt-meter HUD (byte = ROUND(10*time)). The original
// inlines this MessageBegin at each launch; no client handler yet, so it stays a no-op send.
static void TS_SendNextStunt(CBasePlayer *pl, float stuntTime)
{
	// SLICE: gmsgTSNextStunt has no client handler yet — deferred so we don't emit an unhooked msg.
	(void)pl; (void)stuntTime;
}

// gmsgTSMotionBlur (TSMBlur) — slow-motion blur wash; sent on a random roll the way the
// original does ( RandomLong(0,10) + 10*timescale > chance ). Reuses the existing
// (already-registered + client-hooked) powerup message. type 2 = the stunt blur.
static void TS_MotionBlur(CBasePlayer *pl, int chance)
{
	extern int gmsgTSMotionBlur;
	if (RANDOM_LONG(0, 10) + 10.0f * TS_TimeScale(pl) <= (float)chance)
		return;
	MESSAGE_BEGIN(MSG_PVS, gmsgTSMotionBlur, pl->pev->origin);
	WRITE_BYTE(ENTINDEX(pl->edict()));
	WRITE_BYTE(2);
	MESSAGE_END();
}

// ---- Init / Play ----------------------------------------------------------

void CTSStunt::Init(CBasePlayer *pPlayer)
{
	m_pPlayer = pPlayer;
	m_flStuntTime = 0;
	m_flRollTime = 0;
	m_flStunCooldown = 0;
	m_flRollNextTime = 0;
	m_flWheelNextTime = 0;
	m_flWallClimbNextTime = 0;
	m_vecStuntDir = g_vecZero;
	m_bDirForward = false;
	m_flSlideTime = 0;
	m_flBulletTimeEnd = 0;
	m_flWallClimbFuel = 0;
	m_iWallClimbCount = 0;
	m_flLastStunt = 0;
	m_flNextBreakCheck = 0;
	m_pLastBreakVictim = nullptr;
}

// Clear all stunt flags + restore gravity/friction/view (Play's ladder/water reset; inlined
// in the original). The caller also zeroes m_flRollTime (a CTSStunt member).
static void TS_ResetStuntState(CBasePlayer *pl)
{
	entvars_t *pev = pl->pev;
	pl->m_flBlockTime = 0.0f; // the STOP bit's timer resets with it (RE CTSStunt::Play reset)
	pev->iuser4 &= ~(TSSTUNT_STOP | TSSTUNT_PRONE | TSSTUNT_STANDUP | TSSTUNT_DIVE |
					 TSSTUNT_SLIDE | TSSTUNT_DIVE_HEIGHT | TSSTUNT_WHEEL_HI | TSSTUNT_WHEEL_LO |
					 TSSTUNT_WALLJUMP | TSSTUNT_WALLCLIMB | TSSTUNT_WALLJUMP_A | TSSTUNT_WALLCLIMB_A |
					 TSSTUNT_ROLL | TSSTUNT_WHEEL_DONE | TSSTUNT_WHEEL_TRIG);
	pev->friction = 1.0f;
	pev->gravity = 1.0f;
	pev->view_ofs.z = 28.0f;
	pev->iuser4 |= TSSTUNT_STANDUP; // ask pm_shared to leave the prone hull
}

void CTSStunt::Play()
{
	entvars_t *pev = m_pPlayer->pev;
	const float dt = TS_TimeScale(m_pPlayer) * gpGlobals->frametime;

	// decrement the timers (floored)
	if (m_flRollTime != 0.0f)  { m_flRollTime  -= dt; if (m_flRollTime  < 0.001f) m_flRollTime  = 0.001f; }
	m_flStuntTime    -= dt; if (m_flStuntTime    < 0) m_flStuntTime    = 0;
	m_flStunCooldown -= dt; if (m_flStunCooldown < 0) m_flStunCooldown = 0;
	m_flSlideTime    -= dt; if (m_flSlideTime    < 0) m_flSlideTime    = 0;

	if (!m_pPlayer->IsAlive())
		return;

	// wheel-finish: WHEEL_TRIG -> WHEEL_DONE + the morfy-jump pose + airwheel.wav
	if (pev->iuser4 & TSSTUNT_WHEEL_TRIG)
	{
		pev->iuser4 |= TSSTUNT_WHEEL_DONE;
		pev->iuser4 &= ~TSSTUNT_WHEEL_TRIG;
		m_pPlayer->m_TSAnim.Animate(m_pPlayer, 0x39);
		EMIT_SOUND_DYN(ENT(pev), CHAN_BODY, "player/airwheel.wav", VOL_NORM, 0.8f, 0, 95 + RANDOM_LONG(0, 0x1f));
	}

	const bool onGround = (pev->flags & FL_ONGROUND) != 0;

	// FL_ONGROUND-only resets: the wall-climb budget refreshes each airtime (Play @ pev+0x1a5&2).
	// AND: m_flLastStunt is stamped to NOW every frame on the ground (RE PreThink @0x808cc line:
	// `if (FL_ONGROUND) m_flLastStunt = time`). So `time - m_flLastStunt` = how long since you LEFT
	// the ground — HandleStunt allows a mid-air dive within 0.7/timescale of leaving (jump→dive),
	// and on-ground (running) dives directly. This is the dive window the original opens.
	if (onGround)
	{
		m_flLastStunt = gpGlobals->time;
		m_flWallClimbFuel = 0;
		m_flWallClimbNextTime = 0;
		m_iWallClimbCount = 0;
	}

	// The Handle*/Go dispatch runs whether on the ground OR AIRBORNE — the original Play only
	// excludes ladders + deep water (the individual Handle* funcs gate on FL_ONGROUND internally).
	// (My first cut wrongly gated this on FL_ONGROUND, which killed the dive the moment it left
	// the ground and trapped you prone — RE Play @0x8a014 lines 90-147.)
	if (!m_pPlayer->IsOnLadder() && pev->waterlevel != 3)
	{
		// GET UP (RE Play @0x8a014 disasm @0x8a2e0-0x8a3d0): a single STATE-cleanup block — NOT an
		// animation. Fires when a roll timer is LIVE and it either expired (m_flRollTime < 0.01; the
		// decrement floors at 0.001) OR the current player ANIMATION SEQUENCE FINISHED:
		// `roll != 0 && (roll < 0.01 || m_fSequenceFinished)`. player+0x78 (.so) / +0x68 (mp.dll) IS
		// the stock CBaseAnimating::m_fSequenceFinished (live-trace cont.86: its writers are the
		// inlined StudioFrameAdvance/ResetSequenceInfo/DispatchAnimEvents tails, it toggles with every
		// sequence, and the getup fired at roll≈1.07/0.99 = exactly when the ~1s kick anim completed).
		// So the duck/side-kick "land low" holds until the KICK SEQUENCE ends (~1s), the 2.0 roll timer
		// is only the fallback, and fall height shortens the post-landing tail because the anim keeps
		// playing during the fall. HandleSlide forces m_fSequenceFinished=1 on a slide-standup
		// (@0x8baa0) to make that getup instant. Clears exactly ROLL|0x80000|PRONE|STOP|0x2 (@0x8a316;
		// 0x2/0x80000 have un-RE'd producers), zeroes the roll timer + m_flBlockTime (player+0x4dc
		// @0x8a35d), restores friction ONLY (pev+0x120 @0x8a36c — gravity is NOT touched), and restores
		// the view in ONE write (FL_DUCKING ? 16 : 28 @0x8a37b); PM_Duck re-ducks if DUCK is held.
		// The getup ANIMATION (0x2e) is played at LANDING time (Landing() roll branch / HandleSlide's
		// deliberate-prone branch), NEVER here — dispatching 0x2e here while PRONE is still set is
		// exactly what Region B of CTSAnimation::Animate swallows (the old invented two-stage getup =
		// the stuck-prone bug).
		if (m_flRollTime != 0.0f && (m_flRollTime < 0.01f || m_pPlayer->m_fSequenceFinished))
		{
			pev->iuser4 &= ~(TSSTUNT_ROLL | 0x80000 | TSSTUNT_PRONE | TSSTUNT_STOP | 0x2);
			m_flRollTime = 0;
			m_pPlayer->m_flBlockTime = 0.0f; // player+0x4dc (STOP cleared -> its timer too)
			pev->friction = 1.0f;            // pev+0x120 (gravity deliberately untouched)
			pev->view_ofs.z = (pev->flags & FL_DUCKING) ? 16.0f : 28.0f;
		}

		if (pev->iuser4 & TSSTUNT_DIVE)       HandleDive();
		if (pev->iuser4 & TSSTUNT_SLIDE)      HandleSlide();
		if (pev->iuser4 & (TSSTUNT_WHEEL_HI | TSSTUNT_WHEEL_LO)) HandleWheel();
		if (pev->iuser4 & TSSTUNT_WALLJUMP_A) HandleWallJump();
		if (pev->iuser4 & TSSTUNT_WALLCLIMB_A) HandleWallClimb();
		if (pev->button & (IN_JUMP | IN_ALT1)) HandleStunt(); // @orig: 0x180 & 0x4002, no stood-up gate
	}
	else
	{
		// on a ladder / in deep water — abort any stunt
		pev->iuser4 &= ~TSSTUNT_DIVE;
		if (pev->iuser4 & 0x70c31)
		{
			TS_ResetStuntState(m_pPlayer); m_flRollTime = 0;
			m_pPlayer->m_TSAnim.Animate(m_pPlayer, 1);
		}
	}
}

// ---- HandleStunt: the ground/air move chooser ------------------------------

void CTSStunt::HandleStunt()
{
	entvars_t *pev = m_pPlayer->pev;

	// only when no stunt is mid-flight
	if (m_flStuntTime > 0.0f || m_flRollTime > 0.001f || (pev->iuser4 & 0xc30))
		return;

	const bool altPressed = (m_pPlayer->m_afButtonPressed & IN_ALT1) != 0; // +alt1 edge
	const bool jumpHeld = (pev->button & IN_JUMP) != 0;

	if (!jumpHeld)
	{
		if (altPressed)
		{
			if (CheckForWallJump()) { GoWallJump(); return; }
			if (CheckForWallDive()) { pev->iuser4 |= TSSTUNT_WALLDIVE; GoDive(); return; }
		}
	}
	else
	{
		if (CheckForWallClimb()) { GoWallClimb(); return; }
	}

	// ground dive / bullet-time: +alt1, no wall move active, on ground OR recently airborne
	const bool onGround = (pev->flags & FL_ONGROUND) != 0;
	const bool recent = (pev->velocity.z >= -680.0f) &&
						(gpGlobals->time - m_flLastStunt <= 0.7f / TS_TimeScale(m_pPlayer));
	if (altPressed && (pev->iuser4 & 0x130000) == 0 && (onGround || recent))
	{
		Vector h = pev->velocity; h.z = 0;
		if (h.Length() >= 80.0f) GoDive();
		else                     GoBulletTime();
	}
}

// ---- the launches ---------------------------------------------------------

void CTSStunt::GoDive()
{
	entvars_t *pev = m_pPlayer->pev;

	// pick the dive-height tier by aim pitch, gated on the slot weight
	const float pitch = pev->v_angle.x;
	if (pitch < -10.0f && CanDiveHigh())
	{
		pev->iuser4 |= TSSTUNT_DIVE_HIGH;
		pev->gravity = 0.75f;
	}
	else if (pitch <= 10.0f && CanDiveMedium())
	{
		pev->iuser4 |= TSSTUNT_DIVE_MED;
		pev->gravity = 0.75f;
	}
	else
	{
		pev->iuser4 |= TSSTUNT_DIVE_LOW;
		if (CanDiveLow())
		{
			pev->friction = 0.25f;
			pev->gravity = 0.75f;
		}
		else
		{
			pev->friction = 0.9f;  // a flop — couldn't make the dive
			pev->gravity = 1.0f;
		}
	}

	// clear the wall/wheel "active" bits (GoDive @LAB_0009b282) before arming the dive
	pev->iuser4 &= ~(TSSTUNT_WHEEL_DONE | TSSTUNT_WALLCLIMB | TSSTUNT_WALLCLIMB_A | TSSTUNT_WALLJUMP_A);
	pev->iuser4 |= TSSTUNT_PRONE | TSSTUNT_DIVE;
	pev->view_ofs = g_vecZero;

	m_pPlayer->m_TSAnim.Animate(m_pPlayer, 0x28);
	m_flStuntTime = NextDive();
	TS_SendNextStunt(m_pPlayer, m_flStuntTime);
	TS_MotionBlur(m_pPlayer, 8);

	// dive direction: forward (wall-dive) or normalized velocity
	if (pev->iuser4 & TSSTUNT_WALLDIVE)
	{
		UTIL_MakeVectors(pev->v_angle);
		m_vecStuntDir = gpGlobals->v_forward;
	}
	else
	{
		float len = pev->velocity.Length();
		m_vecStuntDir = (len == 0.0f) ? Vector(0, 0, 1) : pev->velocity * (1.0f / len);
	}

	EMIT_SOUND_DYN(ENT(pev), CHAN_BODY, "player/dive.wav", VOL_NORM, 0.8f, 0, 95 + RANDOM_LONG(0, 0x1f));
}

int CTSStunt::GoRoll(int dir)
{
	entvars_t *pev = m_pPlayer->pev;
	const bool onGround = (pev->flags & FL_ONGROUND) != 0;
	bool roll = true;

	// DOUBLE-TAP window (RE GoRoll@0x8cfe8): m_flRollNextTime is an ABSOLUTE re-arm timestamp — set to
	// time+0.3 on a FAILED attempt, 0 on success. Airborne you may only roll if the window is armed
	// (m_flRollNextTime >= time, i.e. a recent tap); on the ground the FIRST tap finds it expired ->
	// roll=false -> arms below, and the SECOND tap within 0.3s finds it armed -> proceeds. => the
	// dive-roll is a DOUBLE-TAP. (Our old code compared the roll DURATION m_flRollTime against absolute
	// gpGlobals->time — always true -> the roll ALWAYS failed AND the fail-path corrupted m_flRollTime
	// with an absolute value, which then broke the getup timer = the "double-jump does nothing" +
	// "press duck -> stuck ducked" bugs.)
	if (!onGround)
	{
		if (m_flRollNextTime < gpGlobals->time)
			return 0;
	}
	else if (m_flRollNextTime < gpGlobals->time)
		roll = false;

	Vector h = pev->velocity; h.z = 0;
	if (h.Length() < 80.0f) roll = false;              // need run speed
	if (pev->iuser4 & 0x130c30) roll = false;          // any active stunt (dive/slide/wheel/walljump/climb)
	if (m_flRollTime > 0.0f) roll = false;             // already rolling
	if (m_pPlayer->IsOnLadder()) roll = false;
	if (m_flStunCooldown > 0.0f) roll = false;         // shared kick/roll cooldown

	if (!roll)
	{
		m_flRollNextTime = gpGlobals->time + 0.3f;     // arm the 0.3s double-tap window
		return 0;
	}

	float len = pev->velocity.Length();
	m_vecStuntDir = (len == 0.0f) ? Vector(0, 0, 1) : pev->velocity * (1.0f / len);
	m_bDirForward = (GetDirection(m_vecStuntDir, pev->v_angle, 0.6f, 0.3f) != 2); // normalized (just stored)

	pev->friction = 0.3f;                               // RE: pev+0x120 = 0.3 = FRICTION (offsetof-proven; the cont.80 gravity reading was the inverted map)
	pev->iuser4 |= TSSTUNT_ROLL | TSSTUNT_PRONE | TSSTUNT_STOP; // 0x40005
	pev->view_ofs = g_vecZero;
	m_flRollTime = 0.75f;

	m_pPlayer->m_TSAnim.Animate(m_pPlayer, 0x2f);
	m_flRollNextTime = 0;
	EMIT_SOUND_DYN(ENT(pev), CHAN_BODY, "player/land.wav", VOL_NORM, 0.8f, 0, 95 + RANDOM_LONG(0, 0x1f));
	TS_MotionBlur(m_pPlayer, 9);
	return 1;
}

void CTSStunt::GoWallClimb()
{
	entvars_t *pev = m_pPlayer->pev;

	UTIL_MakeVectors(pev->v_angle);
	m_vecStuntDir = gpGlobals->v_forward * -1.0f;

	pev->iuser4 |= TSSTUNT_PRONE | TSSTUNT_WALLCLIMB | TSSTUNT_WALLCLIMB_A;
	pev->iuser4 &= ~TSSTUNT_WALLDIVE;
	pev->view_ofs = g_vecZero;

	m_pPlayer->m_TSAnim.Animate(m_pPlayer, 0x38);
	pev->gravity = 1.0f;

	float add = CanDiveHigh() ? 0.3f : (CanDiveMedium() ? 0.5f : 0.75f);
	m_flWallClimbFuel += add;
	m_flStuntTime = (m_flWallClimbFuel < 0.5f) ? 0.5f : m_flWallClimbFuel;

	TS_SendNextStunt(m_pPlayer, m_flStuntTime);
	EMIT_SOUND_DYN(ENT(pev), CHAN_BODY, "player/airwheel.wav", VOL_NORM, 0.8f, 0, 95 + RANDOM_LONG(0, 0x1f));
	m_iWallClimbCount++;
}

void CTSStunt::GoWallJump()
{
	entvars_t *pev = m_pPlayer->pev;

	UTIL_MakeVectors(pev->v_angle);
	m_vecStuntDir = gpGlobals->v_forward * -1.0f;

	pev->iuser4 |= TSSTUNT_PRONE | TSSTUNT_WALLJUMP | TSSTUNT_WALLJUMP_A;
	pev->iuser4 &= ~TSSTUNT_WALLDIVE;
	pev->view_ofs = g_vecZero;

	m_pPlayer->m_TSAnim.Animate(m_pPlayer, 0x37);
	pev->gravity = 0.75f;  // RE GoWallJump@0x8a7d4: pev+0x11c = 0.75 = GRAVITY (offsetof-proven)

	m_flWallClimbFuel += 0.35f;
	m_flStuntTime = (m_flWallClimbFuel < 0.5f) ? 0.5f : m_flWallClimbFuel;

	TS_SendNextStunt(m_pPlayer, m_flStuntTime);
	EMIT_SOUND_DYN(ENT(pev), CHAN_BODY, "player/airwheel.wav", VOL_NORM, 0.8f, 0, 95 + RANDOM_LONG(0, 0x1f));
	TS_MotionBlur(m_pPlayer, 5);
}

void CTSStunt::GoBulletTime()
{
	entvars_t *pev = m_pPlayer->pev;
	if (TS_TimeScale(m_pPlayer) > 0.99f)
		return; // only the standing matrix-dodge while time is already slowed

	UTIL_MakeVectors(pev->v_angle);
	m_vecStuntDir = gpGlobals->v_forward * -1.0f;
	m_bDirForward = true;

	m_flStuntTime = 0.55f;
	m_flBulletTimeEnd = gpGlobals->time + 0.55f;
	TS_SendNextStunt(m_pPlayer, m_flStuntTime);

	pev->iuser4 |= TSSTUNT_PRONE | TSSTUNT_SLIDE;
	m_flSlideTime = 0.55f;
	pev->view_ofs = g_vecZero;

	m_pPlayer->m_TSAnim.Animate(m_pPlayer, 0x36);
	EMIT_SOUND_DYN(ENT(pev), CHAN_BODY, "player/land.wav", VOL_NORM, 0.8f, 0, 95 + RANDOM_LONG(0, 0x1f));
	TS_MotionBlur(m_pPlayer, -1); // always
}

int CTSStunt::GoWheel()
{
	// cartwheel/flip — DOUBLE-TAP gated (RE GoWheel@0x9c9bc), the double-JUMP move. m_flWheelNextTime is
	// an ABSOLUTE re-arm timestamp: the 1st jump fails the window (time > m_flWheelNextTime) and arms it
	// to time+0.3; the 2nd jump within 0.3s passes (time <= m_flWheelNextTime). Must also be on the
	// ground OR within 0.7s of leaving it (the mid-air double-jump window, via m_flLastStunt). (Our old
	// gate never CHECKED m_flWheelNextTime, so with no Jump caller either the double-jump did nothing.)
	entvars_t *pev = m_pPlayer->pev;
	const bool onGround = (pev->flags & FL_ONGROUND) != 0;
	bool ok = (m_flStuntTime <= 0.0f) && (m_flRollTime <= 0.0f) && (m_flStunCooldown <= 0.0f) &&
			  !m_pPlayer->IsOnLadder() && (pev->iuser4 & 0x130c30) == 0 &&
			  (gpGlobals->time <= m_flWheelNextTime);
	if (!onGround && (0.7f / TS_TimeScale(m_pPlayer)) < (gpGlobals->time - m_flLastStunt))
		ok = false;
	if (!ok) { m_flWheelNextTime = gpGlobals->time + 0.3f; return 0; }
	m_flWheelNextTime = 0; // consumed on success (RE)

	Vector h = pev->velocity; h.z = 0;
	int dir;
	if (h.Length() >= 80.0f)
	{
		float len = pev->velocity.Length();
		m_vecStuntDir = (len == 0.0f) ? Vector(0, 0, 1) : pev->velocity * (1.0f / len);
		dir = GetDirection(m_vecStuntDir, pev->v_angle, 0.6f, 0.3f); m_bDirForward = (dir != 2); // normalized (just stored)
	}
	else
	{
		UTIL_MakeVectors(pev->v_angle);
		m_vecStuntDir = gpGlobals->v_forward * -1.0f;
		dir = 3;
	}
	// RE: probabilistic motion-blur (amount 2) — fires when 10*timescale + rand(0,10) > 7 (so almost
	// always at normal time, rarely under heavy slow-mo). Ours was an unconditional amount-7 blur.
	if (10.0f * TS_TimeScale(m_pPlayer) + RANDOM_LONG(0, 10) > 7.0f)
		TS_MotionBlur(m_pPlayer, 2);

	// HIGH wheel — aim up (v_angle.x < -10) OR backward: a MEDIUM-tier dive (the "aim ~30° up dives &
	// lands on feet" move). RE GoWheel@0x9c9bc: iuser4 |= PRONE|WHEEL_HI|DIVE_MED, friction(0x11c)=0.75
	// (NOT gravity). (Ours had STOP|WHEEL_HI|DIVE_LOW + gravity=0.75 -> low tier -> "behaved like a roll".)
	if (CanDiveHigh() && (dir == 3 || pev->v_angle.x < -10.0f))
	{
		pev->iuser4 |= TSSTUNT_PRONE | TSSTUNT_WHEEL_HI;
		pev->view_ofs = g_vecZero;
		m_pPlayer->m_TSAnim.Animate(m_pPlayer, 0x31);
		pev->iuser4 |= TSSTUNT_DIVE_MED;
		pev->gravity = 0.75f;  // pev+0x11c = 0.75 (offsetof-proven: GRAVITY)
		m_flStuntTime = NextDive();
		TS_SendNextStunt(m_pPlayer, m_flStuntTime);
		EMIT_SOUND_DYN(ENT(pev), CHAN_BODY, "player/airwheel.wav", VOL_NORM, 0.8f, 0, 95 + RANDOM_LONG(0, 0x1f));
		// BACKKICK combo (RE the GoWheel dir==3 tail @0x9cdb8): drain 22.5 stamina and arm the
		// whiff-lunge BACKWARD — window 0.3, force = -kick damage (doubled under KungFu), kungfu
		// type, own block window closed. The lunge CONSUMER (PreThink velocity apply) is
		// stunt-cluster scope — until it lands the arm only feeds the KungfuHold counter gate.
		if (dir == 3)
		{
			m_pPlayer->UpdateMeleeBar(22.5f); // fc_15 * fc_1_5 (decompile-exact)
			float kick = 15.0f; // RE gowheel.txt: force base = fc_15 (POSITIVE), doubled under KungFu
			if (KungFu(m_pPlayer))
				kick *= 2.0f;
			m_pPlayer->m_flWhiffLungeTime = 0.3f; // 0x3e99999a
			m_pPlayer->m_flWhiffLungeForce = kick;
			m_pPlayer->m_iWhiffLungeType = 0;
			m_pPlayer->m_flKungFuBlock = 0.0f;
		}
		return 1;
	}
	// LOW wheel — level/down, not backward: a LOW-tier dive. RE: iuser4 |= PRONE|WHEEL_LO|DIVE_LOW,
	// friction=0.75, and for a FORWARD wheel (dir==2) gravity(0x120)=0.25 (floaty forward flip).
	if (CanDiveLow() && dir != 3)
	{
		pev->iuser4 |= TSSTUNT_PRONE | TSSTUNT_WHEEL_LO;
		pev->view_ofs = g_vecZero;
		m_pPlayer->m_TSAnim.Animate(m_pPlayer, 0x32);
		pev->iuser4 |= TSSTUNT_DIVE_LOW;
		pev->gravity = 0.75f;  // pev+0x11c = 0.75 (offsetof-proven: GRAVITY)
		if (dir == 2) pev->friction = 0.25f; // pev+0x120 = 0.25 = FRICTION (was gravity = the "flying" bug)
		m_flStuntTime = NextDive();
		TS_SendNextStunt(m_pPlayer, m_flStuntTime);
		EMIT_SOUND_DYN(ENT(pev), CHAN_BODY, "player/airwheel.wav", VOL_NORM, 0.8f, 0, 95 + RANDOM_LONG(0, 0x1f));
		return 1;
	}
	m_flWheelNextTime = gpGlobals->time + 0.3f;
	return 0;
}

// ---- per-frame continuations (ground) -------------------------------------

void CTSStunt::HandleDive()
{
	Landing();
	m_pPlayer->m_TSAnim.Animate(m_pPlayer, 2);
}

// @0x9b9f8 HandleSlide — the per-frame PRONE handler (VERBATIM). Gated on FL_ONGROUND.
// Branches (all action branches also gated on m_flSlideTime < 0.01):
//  - JUMP/DUCK/ALT1 = the deliberate STANDUP: STOP on, SLIDE off, view 28, arm the 0.75s
//    getup timer + the player+0x78 stand-up latch, pose 0x29 (the slide_*_end getup family).
//  - strafe = the prone ROLL (0x2a/0x2b) AND the direction re-anchor: m_vecStuntDir = the
//    CURRENT aim forward, m_bDirForward = false, anim latch = forward — so the roll (and the
//    following lying pose) is aim-relative no matter how the dive was launched. (Without the
//    re-anchor a sideways-launched prone strafes into the missing slide_b_left/right = idle.)
//  - else = the settled LYING pose (case 3).
// Moving-back (IN_BACK) additionally pins friction 1.0 each frame.
void CTSStunt::HandleSlide()
{
	entvars_t *pev = m_pPlayer->pev;
	if (!(pev->flags & FL_ONGROUND))
		return;
	if (pev->button & IN_BACK)
		pev->friction = 1.0f;

	if ((pev->button & (IN_JUMP | IN_DUCK | IN_ALT1)) && m_flSlideTime < 0.01f)
	{
		pev->iuser4 |= TSSTUNT_STOP;
		pev->iuser4 &= ~TSSTUNT_SLIDE;
		pev->view_ofs = Vector(0, 0, 28);
		m_flRollTime = 0.75f;
		m_pPlayer->m_fSequenceFinished = true; // player+0x78 @0x8baa0 — force sequence-finish => Play's getup fires now
		m_pPlayer->m_TSAnim.Animate(m_pPlayer, 0x29);
		pev->friction = 1.0f;
	}
	else if ((pev->button & IN_MOVELEFT) && m_flSlideTime < 0.01f)
	{
		m_pPlayer->m_TSAnim.Animate(m_pPlayer, 0x2a);
		UTIL_MakeVectors(pev->v_angle);
		m_vecStuntDir = gpGlobals->v_forward; // re-anchor the stunt dir to the CURRENT aim
		pev->friction = 1.0f;
		m_bDirForward = false;
		m_pPlayer->m_TSAnim.m_iDirection = 2; // player+0x2a0 = forward
	}
	else if ((pev->button & IN_MOVERIGHT) && m_flSlideTime < 0.01f)
	{
		m_pPlayer->m_TSAnim.Animate(m_pPlayer, 0x2b);
		UTIL_MakeVectors(pev->v_angle);
		m_vecStuntDir = gpGlobals->v_forward;
		pev->friction = 1.0f;
		m_bDirForward = false;
		m_pPlayer->m_TSAnim.m_iDirection = 2;
	}
	else
		m_pPlayer->m_TSAnim.Animate(m_pPlayer, 3);
}

// @ HandleWheel — the wheel LANDING handler (RE CTSStunt_all @0x9b...: gated on pev->flags
// FL_ONGROUND — it does NOTHING mid-flight; our old ungated version converted the wheel to
// SLIDE on the very next frame, so HandleSlide's per-frame prone dispatch stomped the
// lowwheel_* anim through the 0x32 guard's deliberate 0x29 pass-through = the "low wheel
// goes idle mid-air" glitch). LOW tier: land into the prone SLIDE, pose oriented by the
// REVERSED stunt direction (the flip turns you over), latched into the anim m_iDirection
// (player+0x2a0). HIGH tier: STOP root + gravity 0.25 + the 0.375s auto-getup timer.
void CTSStunt::HandleWheel()
{
	entvars_t *pev = m_pPlayer->pev;
	if (!(pev->flags & FL_ONGROUND))
		return;

	if (!(pev->iuser4 & TSSTUNT_WHEEL_HI))
	{
		pev->iuser4 &= ~TSSTUNT_WHEEL_LO;
		pev->iuser4 |= TSSTUNT_SLIDE;
		pev->view_ofs = g_vecZero;
		m_vecStuntDir = m_vecStuntDir * -1.0f;
		int dir = GetDirection(m_vecStuntDir, pev->v_angle, 0.6f, 0.3f);
		m_bDirForward = (dir != 2);
		m_pPlayer->m_TSAnim.m_iDirection = dir; // player+0x2a0 — the prone pose direction latch
	}
	else
	{
		pev->iuser4 &= ~TSSTUNT_WHEEL_HI;
		pev->iuser4 |= TSSTUNT_STOP;
		pev->friction = 0.25f; // pev+0x120 = 0.25 = FRICTION (offsetof-proven)
		pev->view_ofs = g_vecZero;
		m_flRollTime = 0.375f; // the auto-getup timer (Play's getup block consumes it)
	}
	pev->gravity = 1.0f;       // pev+0x11c = 1.0 = GRAVITY restore at landing (both tiers)
	EMIT_SOUND_DYN(ENT(pev), CHAN_BODY, "player/land.wav", VOL_NORM, 0.8f, 0, 95 + RANDOM_LONG(0, 0x1f));
}

void CTSStunt::HandleWallJump()
{
	entvars_t *pev = m_pPlayer->pev;
	if (!(pev->flags & FL_ONGROUND))
		return;
	pev->iuser4 &= ~TSSTUNT_WALLJUMP_A;
	pev->friction = 0.5f;  // RE HandleWallJump@0x8b4fc: pev+0x120 = 0.5 = FRICTION (offsetof-proven)
	pev->gravity = 1.0f;   // pev+0x11c = 1.0 = GRAVITY restore
	pev->view_ofs = g_vecZero;
	m_flRollTime = 0.262f;
	if (m_flStuntTime < 0.01f) { m_flStuntTime = 0; TS_SendNextStunt(m_pPlayer, m_flStuntTime); }
	EMIT_SOUND_DYN(ENT(pev), CHAN_BODY, "player/land.wav", VOL_NORM, 0.8f, 0, 95 + RANDOM_LONG(0, 0x1f));
}

void CTSStunt::HandleWallClimb()
{
	entvars_t *pev = m_pPlayer->pev;
	if (!(pev->flags & FL_ONGROUND))
	{
		// still airborne — chain into a wall-jump / wall-dive if the buttons allow
		if (m_pPlayer->m_afButtonPressed & IN_ALT1)
		{
			if (CheckForWallJump()) { GoWallJump(); return; }
			if (CheckForWallDive()) { pev->iuser4 |= TSSTUNT_WALLDIVE; GoDive(); }
		}
		return;
	}
	pev->iuser4 &= ~TSSTUNT_WALLCLIMB_A;
	pev->friction = 0.5f;  // RE HandleWallClimb: pev+0x120 = 0.5 = FRICTION (offsetof-proven)
	pev->gravity = 1.0f;   // pev+0x11c = 1.0 = GRAVITY restore
	m_flRollTime = 0.262f;
	if (m_flStuntTime < 0.01f) { m_flStuntTime = 0; TS_SendNextStunt(m_pPlayer, m_flStuntTime); }
	EMIT_SOUND_DYN(ENT(pev), CHAN_BODY, "player/land.wav", VOL_NORM, 0.8f, 0, 95 + RANDOM_LONG(0, 0x1f));
}

void CTSStunt::Landing()
{
	entvars_t *pev = m_pPlayer->pev;
	if (!(pev->flags & FL_ONGROUND) || !(pev->iuser4 & TSSTUNT_DIVE))
		return;

	// slide unless JUMP/ALT1/DUCK is held (RE Landing @0x8bbf4: button & 0x4006). Plain movement
	// keys do NOT cancel the slide — you slide on every dive landing by default.
	if ((pev->button & (IN_JUMP | IN_ALT1 | IN_DUCK)) == 0)
	{
		// slide out of the dive — keep DIVE's landing velocity, just switch to the slide pose/control.
		// RE Landing@0x8bbf4: store the landing direction in m_iDirection (orig player+0x2a0); the prone
		// pose (Animate 0x29) indexes it -> backward(3)=slide_b_end=feet, fwd/L/R=prone. Do NOT animate
		// here — HandleSlide (which runs the same Play frame once SLIDE is set) picks the pose.
		pev->iuser4 &= ~TSSTUNT_DIVE;
		pev->iuser4 |= TSSTUNT_SLIDE;
		Vector landDir = pev->velocity;
		float landLen = landDir.Length();
		landDir = (landLen == 0.0f) ? Vector(0, 0, 1) : landDir * (1.0f / landLen);
		int dir = GetDirection(landDir, pev->v_angle, 0.6f, 0.3f);
		m_pPlayer->m_TSAnim.m_iDirection = dir;
		m_bDirForward = (dir != 2);
	}
	else
	{
		// roll out of the dive
		pev->iuser4 &= ~TSSTUNT_DIVE;
		pev->iuser4 |= TSSTUNT_ROLL | TSSTUNT_STOP;
		pev->friction = 0.5f;
		m_flRollTime = 0.5625f;
		m_pPlayer->m_TSAnim.Animate(m_pPlayer, 0x2e);
	}
	pev->gravity = 1.0f;
	EMIT_SOUND_DYN(ENT(pev), CHAN_BODY, "player/land.wav", VOL_NORM, 0.8f, 0, 95 + RANDOM_LONG(0, 0x1f));
}

// ---- wall checks ----------------------------------------------------------

// shared: is there a (near-vertical, BSP) wall this far along +/- v_forward, and are
// we not falling too fast? (RE CheckForWallJump/Dive/Climb @0x8ac14/0x8ad44/0x8ae6c)
static bool TSWallTrace(CBasePlayer *pl, float dist, bool ahead, bool needBSP)
{
	UTIL_MakeVectors(pl->pev->v_angle);
	Vector dir = gpGlobals->v_forward * (ahead ? dist : -dist);
	TraceResult tr;
	UTIL_TraceLine(pl->pev->origin, pl->pev->origin + dir, ignore_monsters, ENT(pl->pev), &tr);
	if (tr.flFraction >= 1.0f)
		return false;
	if (needBSP && (!tr.pHit || VARS(tr.pHit)->solid != SOLID_BSP))
		return false;
	if (fabs(tr.vecPlaneNormal.z) >= 0.5f)   // wall must be roughly vertical
		return false;
	if (pl->pev->velocity.z <= -370.0f)      // not falling too fast
		return false;
	return true;
}

bool CTSStunt::CheckForWallJump()
{
	if (!CanDiveHigh())
		return false;
	return TSWallTrace(m_pPlayer, 48.0f, true, false);
}

bool CTSStunt::CheckForWallDive()
{
	if (m_pPlayer->pev->flags & FL_ONGROUND)
		return false;
	return TSWallTrace(m_pPlayer, 48.0f, false, false);
}

bool CTSStunt::CheckForWallClimb()
{
	entvars_t *pev = m_pPlayer->pev;
	// RE CheckForWallClimb @0x8ae6c: when NOT already climbing (WALLCLIMB_A clear), you need a fresh
	// JUMP press (the "double jump", player[0x205]&2) and you must be inside the 0.3s climb window —
	// the FIRST press arms the window (returns false), the SECOND press within 0.3s starts the climb.
	// Once WALLCLIMB_A is set (climbing), this gate is skipped, so HOLDING jump auto-climbs each time
	// the stunt timer frees HandleStunt (paced auto-kicks up the wall). (My earlier FL_ONGROUND check
	// was wrong — climbing happens airborne, so it always returned false and walls couldn't be scaled.)
	if ((pev->iuser4 & TSSTUNT_WALLCLIMB_A) == 0)
	{
		if ((m_pPlayer->m_afButtonPressed & IN_JUMP) == 0)
			return false;
		if (m_flWallClimbNextTime < gpGlobals->time)
		{
			m_flWallClimbNextTime = gpGlobals->time + 0.3f; // arm the double-jump window, this press fails
			return false;
		}
	}
	return TSWallTrace(m_pPlayer, 54.0f, true, true);
}

// ---- close-combat (jump-kick) + the run-into sweep -------------------------

// @0x8c630 CloseAttack — the JUMP-KICK (called from CBasePlayer::Jump). Speed-scaled melee
// when you jump into a breakable/person with a real weapon. Returns true on a hit. (This was
// the free function TSTryJumpKick; relocated to its real class. The original routes the hit
// through CBasePlayer::CloseCombatAttack; we inline the trace+TraceAttack — same result.)
int CTSStunt::CloseAttack()
{
	entvars_t *pev = m_pPlayer->pev;

	// cooldown (CTSStunt+8 = m_flStunCooldown, decremented in Play) + FL_FROZEN guard
	if (m_flStunCooldown > 0.0f)
		return 0;
	if (pev->flags & FL_FROZEN)
		return 0;

	// no kick when bare-hands KUNG-FU is active (empty grip) — kung-fu is manual when equipped.
	if (m_pPlayer->m_szAnimExtention[0] == 0)
		return 0;

	// speed-scaled melee damage: clamp(|velocity| / 15, 10, 20), ×2 under the kung-fu powerup.
	float spd = pev->velocity.Length() * (1.0f / 15.0f);
	float dmg = (spd < 10.0f) ? 10.0f : (spd <= 20.0f) ? spd : 20.0f;
	if (KungFu(m_pPlayer))
		dmg *= 2.0f;

	// forward reach 48u; route through the shared resolver (RE @0x8c8b9: hull 3, iAnim -1 —
	// it animates only on a HIT; disarm on, playersOnly off, knockback on, teamSafe FORCED).
	UTIL_MakeVectors(pev->v_angle);
	Vector vecSrc = m_pPlayer->GetGunPosition();
	Vector vecEnd = vecSrc + gpGlobals->v_forward * 48.0f;

	CBaseEntity *pHit = m_pPlayer->CloseCombatAttack(vecSrc, vecEnd, gpGlobals->v_forward,
		3, dmg, -1, true, false, true, true);
	if (!pHit)
		return 0;

	// hit tail per the fragment @0x9c70c: close own kungfu window, random kick body anim
	// (circfront/latback/circback = ids 0xa/0xe/0xd), top up the attack gate if expired.
	// DEFERRED (VERIFY L10): UpdateMeleeBar(dmg * const) — the drain constant is un-RE'd.
	m_pPlayer->m_flKungFuBlock = 0.0f;
	static const int kickActs[3] = { TSA_CC_CIRCFRONT, TSA_CC_LATBACK, TSA_CC_CIRCBACK };
	m_pPlayer->m_TSAnim.Animate(m_pPlayer, kickActs[RANDOM_LONG(0, 2)]);
	if (m_pPlayer->m_flNextAttack <= 0.0f)
		m_pPlayer->m_flNextAttack = 0.5f; // top-up (VERIFY exact value at @0x9c70c)

	EMIT_SOUND(ENT(pev), CHAN_WEAPON, "player/closecombat.wav", VOL_NORM, ATTN_NORM);
	m_flStunCooldown = 0.7f; // one kick per ~0.7s (also gates roll/wheel)
	return 1;
}

// @0x8bd9c CheckForBreakables — per-tick (CBasePlayer::PreThink) sweep: running/diving into a
// surface at speed counts as a melee hit. TWO modes in the original: a non-melee BREAKABLE sweep
// (this, faithful) and a close-combat hold-deflection path against PLAYERS that calls
// CBasePlayer::CloseCombatAttack + Victim{Kungfu,Katana,Knife}Hold — those subsystems aren't built
// yet, so the player/melee branch is DEFERRED (VERIFY). The non-melee branch deliberately skips
// players, so it only ever shatters world breakables — safe to run each frame.
void CTSStunt::CheckForBreakables()
{
	entvars_t *pev = m_pPlayer->pev;

	// throttle (orig field_0x18; -1 disables)
	if (m_flNextBreakCheck == -1.0f)
		return;
	if (gpGlobals->time < m_flNextBreakCheck)
		return;
	m_flNextBreakCheck = gpGlobals->time;

	// grounded & not in close-combat melee mode -> clear the last victim and bail.
	if (pev->flags & FL_ONGROUND)
	{
		m_pLastBreakVictim = nullptr;
		return;
	}

	const float speed = pev->velocity.Length();
	if (speed < 100.0f)
		return;

	// sweep 100u along the velocity and hull-trace for what we're about to hit.
	Vector dir = pev->velocity * (1.0f / speed);
	Vector end = pev->origin + dir * 100.0f;
	TraceResult tr;
	UTIL_TraceHull(pev->origin, end, dont_ignore_monsters, head_hull, ENT(pev), &tr);
	if (tr.flFraction >= 1.0f)
		return;

	CBaseEntity *pHit = CBaseEntity::Instance(tr.pHit);
	if (!pHit)
		return;
	// non-melee mode never run-damages players (the close-combat player path is deferred).
	if (pHit->IsPlayer())
		return;
	if (pHit->pev->takedamage == DAMAGE_NO)
		return;
	if (pHit->pev->effects & EF_NODRAW)
		return;
	if (m_pLastBreakVictim == pHit)
		return; // already hit this pass

	// damage = the impact speed (the original gates speed vs 0.1*speed on a player virtual at
	// vtable+0xb4 — VERIFY; for a world breakable either value just shatters it).
	ClearMultiDamage();
	pHit->TraceAttack(pev, speed, dir, &tr, DMG_CLUB | DMG_NEVERGIB);
	ApplyMultiDamage(pev, pev);
	m_pLastBreakVictim = pHit;
}
