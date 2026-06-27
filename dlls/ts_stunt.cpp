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
#include "ts_stunt.h"
#include "ts_weapons.h" // TSPlayerFreeSlots

int gmsgTSNextStunt = 0; // "TSNextStunt" 1 — stunt-meter byte (HUD; client handler deferred)
extern int gmsgTSMotionBlur;

// ---- helpers --------------------------------------------------------------

float CTSStunt::TimeScale()
{
	float ts = m_pPlayer->pev->fuser1;
	return (ts == 0.0f) ? 1.0f : ts;
}

// 81-slot remaining capacity (CBasePlayer+0x548): lighter loadout = higher dives.
bool CTSStunt::CanDiveHigh()   { return TSPlayerFreeSlots(m_pPlayer) > 39; } // 0x51-cap < 0x2a
bool CTSStunt::CanDiveMedium() { return TSPlayerFreeSlots(m_pPlayer) > 29; } // < 0x34
bool CTSStunt::CanDiveLow()    { return TSPlayerFreeSlots(m_pPlayer) > 19; } // < 0x3e

// 0 left / 1 right / 2 forward / 3 back, from horizontal velocity vs facing.
int CTSStunt::GetDirection()
{
	UTIL_MakeVectors(m_pPlayer->pev->v_angle);
	Vector v = m_pPlayer->pev->velocity;
	v.z = 0;
	if (v.Length() < 1.0f)
		return 2;
	v = v.Normalize();
	float f = DotProduct(v, gpGlobals->v_forward);
	float r = DotProduct(v, gpGlobals->v_right);
	if (fabs(f) >= fabs(r))
		return (f >= 0.0f) ? 2 : 3;
	return (r >= 0.0f) ? 1 : 0;
}

void CTSStunt::SendNextStunt()
{
	// gmsgTSNextStunt drives the client stunt-meter HUD (byte = ROUND(10*time)).
	// SLICE: no client handler yet — deferred so we don't emit an unhooked msg.
}

// gmsgTSMotionBlur (TSMBlur) — slow-motion blur wash; sent on a random roll the way
// the original does ( RandomLong(0,10) + 10*timescale > chance ). Reuses the existing
// (already-registered + client-hooked) powerup message. type 2 = the stunt blur.
void CTSStunt::MotionBlur(int chance)
{
	if (RANDOM_LONG(0, 10) + 10.0f * TimeScale() <= (float)chance)
		return;
	if (gmsgTSMotionBlur == 0)
		gmsgTSMotionBlur = REG_USER_MSG("TSMBlur", 2);
	MESSAGE_BEGIN(MSG_PVS, gmsgTSMotionBlur, m_pPlayer->pev->origin);
	WRITE_BYTE(ENTINDEX(m_pPlayer->edict()));
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
}

// Clear all stunt flags + restore gravity/friction/view (Play's ladder/water reset).
void CTSStunt::ResetStuntState()
{
	entvars_t *pev = m_pPlayer->pev;
	pev->iuser4 &= ~(TSSTUNT_STOP | TSSTUNT_PRONE | TSSTUNT_STANDUP | TSSTUNT_DIVE |
					 TSSTUNT_SLIDE | TSSTUNT_DIVE_HEIGHT | TSSTUNT_WHEEL_HI | TSSTUNT_WHEEL_LO |
					 TSSTUNT_WALLJUMP | TSSTUNT_WALLCLIMB | TSSTUNT_WALLJUMP_A | TSSTUNT_WALLCLIMB_A |
					 TSSTUNT_ROLL | TSSTUNT_WHEEL_DONE | TSSTUNT_WHEEL_TRIG);
	pev->friction = 1.0f;
	pev->gravity = 1.0f;
	pev->view_ofs.z = 28.0f;
	m_flRollTime = 0;
	pev->iuser4 |= TSSTUNT_STANDUP; // ask pm_shared to leave the prone hull
}

void CTSStunt::Play()
{
	entvars_t *pev = m_pPlayer->pev;
	const float dt = TimeScale() * gpGlobals->frametime;

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
		// GET UP — a SHARED, TIMED two-stage transition (RE: HandleSlide-end sets m_flRollTime = 0.75 as
		// the getup duration). Pressing JUMP / +alt1 / DUCK while settled-prone (PM_Jump is gated, so jump
		// never hops) STARTS the getup: keep the PRONE hull + drop to the intermediate eye (16) and play
		// the getup ANIMATION (dive_<dir>_end), set STOP so you can move freely but slowed, for 0.75s.
		// When the timer expires (same path the roll/landing recovery uses) we clear PRONE and go to FULL
		// standing (28) — ALWAYS, regardless of holding duck (shared logic). PM_Duck then independently
		// re-ducks if DUCK is held, so duck-getup reads as: prone -> up to full height -> back down to duck.
		bool stoodUp = false;
		if (onGround && (pev->iuser4 & TSSTUNT_PRONE) && m_flRollTime == 0.0f &&
			(pev->iuser4 & (TSSTUNT_DIVE_HEIGHT | TSSTUNT_WALLJUMP_M | TSSTUNT_WALLCLIMB |
							TSSTUNT_WALLJUMP_A | TSSTUNT_WALLCLIMB_A)) == 0 &&
			(m_pPlayer->m_afButtonPressed & (IN_JUMP | IN_ALT1 | IN_DUCK)))
		{
			pev->iuser4 &= ~(TSSTUNT_DIVE | TSSTUNT_SLIDE | TSSTUNT_ROLL); // free movement (no strafe-lock)
			pev->iuser4 |= TSSTUNT_STOP;                                   // STOP halves the move cmd = slow
			pev->view_ofs.z = 16.0f;                                      // stage 1: intermediate eye
			pev->friction = 1.0f;
			m_flRollTime = 0.75f;                                         // getup duration (RE HandleSlide)
			m_flStuntTime = 0;
			m_pPlayer->m_TSAnim.Animate(m_pPlayer, 0x2e);                 // getup animation (dive_<dir>_end)
			stoodUp = true;
		}
		// stage 2: the getup / roll timer ran out -> leave the prone stunt. The END eye depends on the
		// duck button (the prone left FL_DUCKING + the duck hull set, which PM_Duck owns from here):
		//   - DUCK held  -> settle at the DUCK eye (12); PM_Duck keeps you in the duck hull -> prone ->
		//                   higher (16 during getup) -> LOWER (duck). Not stuck high.
		//   - otherwise  -> standing eye (28); PM_Duck unducks the hull (FL_DUCKING set, duck released).
		if (m_flRollTime != 0.0f && m_flRollTime < 0.01f)
		{
			pev->iuser4 &= ~(TSSTUNT_DIVE | TSSTUNT_SLIDE | TSSTUNT_STOP | TSSTUNT_PRONE | TSSTUNT_ROLL);
			pev->gravity = 1.0f;
			pev->friction = 1.0f;
			pev->view_ofs.z = (pev->button & IN_DUCK) ? 12.0f : 28.0f;
			m_flRollTime = 0;
			m_flStuntTime = 0;
			stoodUp = true;
		}

		if (pev->iuser4 & TSSTUNT_DIVE)       HandleDive();
		if (pev->iuser4 & TSSTUNT_SLIDE)      HandleSlide();
		if (pev->iuser4 & (TSSTUNT_WHEEL_HI | TSSTUNT_WHEEL_LO)) HandleWheel();
		if (pev->iuser4 & TSSTUNT_WALLJUMP_A) HandleWallJump();
		if (pev->iuser4 & TSSTUNT_WALLCLIMB_A) HandleWallClimb();
		if (!stoodUp && (pev->button & (IN_JUMP | IN_ALT1))) HandleStunt();
	}
	else
	{
		// on a ladder / in deep water — abort any stunt
		pev->iuser4 &= ~TSSTUNT_DIVE;
		if (pev->iuser4 & 0x70c31)
		{
			ResetStuntState();
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
						(gpGlobals->time - m_flLastStunt <= 0.7f / TimeScale());
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
	SendNextStunt();
	MotionBlur(8);

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
	bool roll = true;

	const bool onGround = (pev->flags & FL_ONGROUND) != 0;
	if (onGround) { if (m_flRollTime < gpGlobals->time) roll = false; }
	else if (m_flRollTime < gpGlobals->time) roll = false;

	Vector h = pev->velocity; h.z = 0;
	if (h.Length() < 80.0f) roll = false;
	if (pev->iuser4 & TSSTUNT_ACTIVE_MASK) roll = false;
	if (m_flRollTime > 0.0f) roll = false;
	if (m_pPlayer->IsOnLadder()) roll = false;
	if (m_flStunCooldown > 0.0f) roll = false;

	if (!roll)
	{
		m_flRollTime = gpGlobals->time + 0.3f; // (cooldown shares the field on a fail)
		return 0;
	}

	float len = pev->velocity.Length();
	m_vecStuntDir = (len == 0.0f) ? Vector(0, 0, 1) : pev->velocity * (1.0f / len);
	GetDirection();

	pev->friction = 0.3f;
	pev->iuser4 |= TSSTUNT_ROLL | TSSTUNT_PRONE | TSSTUNT_STOP;
	pev->view_ofs = g_vecZero;
	m_flRollTime = 0.75f;

	m_pPlayer->m_TSAnim.Animate(m_pPlayer, 0x2f);
	EMIT_SOUND_DYN(ENT(pev), CHAN_BODY, "player/land.wav", VOL_NORM, 0.8f, 0, 95 + RANDOM_LONG(0, 0x1f));
	m_flRollNextTime = 0;
	MotionBlur(9);
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

	SendNextStunt();
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
	pev->gravity = 0.75f;

	m_flWallClimbFuel += 0.35f;
	m_flStuntTime = (m_flWallClimbFuel < 0.5f) ? 0.5f : m_flWallClimbFuel;

	SendNextStunt();
	EMIT_SOUND_DYN(ENT(pev), CHAN_BODY, "player/airwheel.wav", VOL_NORM, 0.8f, 0, 95 + RANDOM_LONG(0, 0x1f));
	MotionBlur(5);
}

void CTSStunt::GoBulletTime()
{
	entvars_t *pev = m_pPlayer->pev;
	if (TimeScale() > 0.99f)
		return; // only the standing matrix-dodge while time is already slowed

	UTIL_MakeVectors(pev->v_angle);
	m_vecStuntDir = gpGlobals->v_forward * -1.0f;
	m_bDirForward = true;

	m_flStuntTime = 0.55f;
	m_flBulletTimeEnd = gpGlobals->time + 0.55f;
	SendNextStunt();

	pev->iuser4 |= TSSTUNT_PRONE | TSSTUNT_SLIDE;
	m_flSlideTime = 0.55f;
	pev->view_ofs = g_vecZero;

	m_pPlayer->m_TSAnim.Animate(m_pPlayer, 0x36);
	EMIT_SOUND_DYN(ENT(pev), CHAN_BODY, "player/land.wav", VOL_NORM, 0.8f, 0, 95 + RANDOM_LONG(0, 0x1f));
	MotionBlur(-1); // always
}

int CTSStunt::GoWheel()
{
	// cartwheel — same gating as GoRoll; defers to a dive launch on success
	entvars_t *pev = m_pPlayer->pev;
	bool ok = (m_flStuntTime <= 0.0f) && (m_flRollTime <= 0.0f) && (m_flStunCooldown <= 0.0f) &&
			  !m_pPlayer->IsOnLadder() && (pev->iuser4 & TSSTUNT_ACTIVE_MASK) == 0;
	if (!ok) { m_flWheelNextTime = gpGlobals->time + 0.3f; return 0; }

	Vector h = pev->velocity; h.z = 0;
	int dir;
	if (h.Length() >= 80.0f)
	{
		float len = pev->velocity.Length();
		m_vecStuntDir = (len == 0.0f) ? Vector(0, 0, 1) : pev->velocity * (1.0f / len);
		dir = GetDirection();
	}
	else
	{
		UTIL_MakeVectors(pev->v_angle);
		m_vecStuntDir = gpGlobals->v_forward * -1.0f;
		dir = 3;
	}
	MotionBlur(7);

	if (CanDiveHigh() && (dir == 3 || pev->v_angle.x < -10.0f))
	{
		pev->iuser4 |= TSSTUNT_STOP | TSSTUNT_WHEEL_HI | TSSTUNT_DIVE_LOW;
		pev->view_ofs = g_vecZero;
		m_pPlayer->m_TSAnim.Animate(m_pPlayer, 0x31);
		pev->gravity = 0.75f;
		m_flStuntTime = NextDive();
		SendNextStunt();
		EMIT_SOUND_DYN(ENT(pev), CHAN_BODY, "player/airwheel.wav", VOL_NORM, 0.8f, 0, 95 + RANDOM_LONG(0, 0x1f));
		return 1;
	}
	if (CanDiveLow() && dir != 3)
	{
		pev->iuser4 |= TSSTUNT_STOP | TSSTUNT_WHEEL_LO | TSSTUNT_DIVE_LOW;
		pev->view_ofs = g_vecZero;
		m_pPlayer->m_TSAnim.Animate(m_pPlayer, 0x32);
		pev->gravity = 0.75f;
		if (dir == 2) pev->friction = 0.25f;
		m_flStuntTime = NextDive();
		SendNextStunt();
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

void CTSStunt::HandleSlide()
{
	entvars_t *pev = m_pPlayer->pev;
	// the prone slide pose every frame: slide_<dir>_<grip> picked from the strafe/move direction —
	// strafing left/right rolls you onto your side (slide_l/slide_r), back = slide_b. The Animate
	// 0x29 case derives the direction from velocity, so it updates as you strafe while prone.
	m_pPlayer->m_TSAnim.Animate(m_pPlayer, 0x29);
	pev->friction = 1.0f;
}

void CTSStunt::HandleWheel()
{
	entvars_t *pev = m_pPlayer->pev;
	pev->iuser4 &= ~(TSSTUNT_WHEEL_HI | TSSTUNT_WHEEL_LO);
	pev->iuser4 |= TSSTUNT_SLIDE;
	pev->friction = 0.25f;
	pev->gravity = 1.0f;
	EMIT_SOUND_DYN(ENT(pev), CHAN_BODY, "player/land.wav", VOL_NORM, 0.8f, 0, 95 + RANDOM_LONG(0, 0x1f));
}

void CTSStunt::HandleWallJump()
{
	entvars_t *pev = m_pPlayer->pev;
	if (!(pev->flags & FL_ONGROUND))
		return;
	pev->iuser4 &= ~TSSTUNT_WALLJUMP_A;
	pev->friction = 0.5f;
	pev->view_ofs = g_vecZero;
	m_flRollTime = 0.262f;
	if (m_flStuntTime < 0.01f) { m_flStuntTime = 0; SendNextStunt(); }
	pev->gravity = 1.0f;
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
	pev->friction = 0.5f;
	m_flRollTime = 0.262f;
	if (m_flStuntTime < 0.01f) { m_flStuntTime = 0; SendNextStunt(); }
	pev->gravity = 1.0f;
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
		// slide out of the dive — keep DIVE's landing velocity, just switch to the slide pose/control
		pev->iuser4 &= ~TSSTUNT_DIVE;
		pev->iuser4 |= TSSTUNT_SLIDE;
		m_pPlayer->m_TSAnim.Animate(m_pPlayer, 0x29); // slide_<dir> prone pose
		GetDirection();
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
