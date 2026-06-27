/***
*  ts_animation.cpp — RECONSTRUCTED CTSAnimation (TS 3rd-person body/gait).
*
*  Reconstructed by intent from CTSAnimation::Animate (@0x6abd0) + Init (@0x67ae0)
*  + UpdateTimers (@0x6b37c). The original is a ~6.7KB activity state machine that
*  builds TS model sequence names from a grip suffix; we reproduce the BEHAVIOUR
*  (which sequence plays for which player state) using the real model sequences,
*  not the byte layout (CLAUDE.md: behaviour + the model/protocol contract is the
*  target, not the gcc2 ABI). See ts_animation.h for what is deferred (CTSStunt).
***/
#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "player.h"
#include "ts_animation.h"

// Speed thresholds for the leg locomotion gait (run2/walk2). The original gates
// the slow/idle body on a 180 unit/s speed; walking below that, running above.
#define TSA_RUN_SPEED 135.0f // above this -> run2; below (and moving) -> walk2

void CTSAnimation::Init()
{
	m_iLastActivity = -1;
	m_iDirection = -1;
	m_iDuckLatch = 0;
	m_flLegTimer = 0;
	m_flAimTimer = 0;
	m_flGrenadeFlag = 0;
}

// Decay the per-frame timers (original @0x6b37c subtracts gpGlobals->frametime
// from each, clamped at 0). frametime is scaled by the player's slow-motion go
// factor in the original; we use raw frametime (the slow scaling is a refinement).
void CTSAnimation::UpdateTimers(CBasePlayer *pPlayer)
{
	const float dt = gpGlobals->frametime;
	if (m_flAimTimer > 0) { m_flAimTimer -= dt; if (m_flAimTimer < 0) m_flAimTimer = 0; }
	if (m_flLegTimer > 0) { m_flLegTimer -= dt; if (m_flLegTimer < 0) m_flLegTimer = 0; }
	if (m_flGrenadeFlag > 0) { m_flGrenadeFlag -= dt; if (m_flGrenadeFlag < 0) m_flGrenadeFlag = 0; }
}

// 0=left 1=right 2=forward 3=back — velocity relative to view facing.
// Mirrors GetDirection(vel, forward, ...) — the original feeds normalized velocity
// and the player's v_angle forward; we compute the same from the yaw.
int CTSAnimation::GetMoveDirection(CBasePlayer *pPlayer)
{
	Vector vel = pPlayer->pev->velocity;
	vel.z = 0;
	if (vel.Length() < 1.0f)
		return m_iDirection >= 0 ? m_iDirection : 2;

	UTIL_MakeVectors(Vector(0, pPlayer->pev->angles.y, 0));
	Vector fwd = gpGlobals->v_forward;
	Vector right = gpGlobals->v_right;
	fwd.z = right.z = 0;

	float fdot = DotProduct(vel, fwd);
	float rdot = DotProduct(vel, right);

	if (fabs(fdot) >= fabs(rdot))
		return (fdot >= 0) ? 2 /*forward*/ : 3 /*back*/;
	return (rdot >= 0) ? 1 /*right*/ : 0 /*left*/;
}

// Set the torso/body sequence (pev->sequence). Returns true if it changed.
bool CTSAnimation::SetBodySequence(CBasePlayer *pPlayer, const char *name, bool restart, bool noLerp)
{
	int seq = pPlayer->LookupSequence(name);
	if (seq == -1)
		seq = 0;

	bool changed = (pPlayer->pev->sequence != seq);
	if (changed || restart)
		pPlayer->pev->frame = 0;

	if (noLerp)
		pPlayer->pev->effects |= EF_NOINTERP;

	if (changed)
	{
		pPlayer->pev->sequence = seq;
		pPlayer->ResetSequenceInfo();
	}
	return changed;
}

// Set the leg locomotion sequence (pev->gaitsequence). NULL/"" -> 0 (legs follow
// the body sequence). The client (StudioProcessGait) plays this blended under the
// body sequence.
void CTSAnimation::SetGaitSequence(CBasePlayer *pPlayer, const char *name)
{
	int gait = 0;
	if (name && name[0])
	{
		gait = pPlayer->LookupSequence(name);
		if (gait == -1)
			gait = 0;
	}
	pPlayer->pev->gaitsequence = gait;
}

// Play a kung-fu close-combat body sequence on the player's 3rd-person model (by name) and hold the
// torso for `hold` seconds (reusing the aim-timer torso protection) so the move/idle path doesn't
// overwrite it mid-punch. Legs keep updating via the move/idle gait path.
void CTSAnimation::PlayKungFu(CBasePlayer *pPlayer, const char *seq, float hold)
{
	if (!seq || !seq[0])
		return;
	SetBodySequence(pPlayer, seq, true, true);
	if (hold > m_flAimTimer)
		m_flAimTimer = hold; // protect this torso pose (same gate the fire/reload poses use)
}

void CTSAnimation::Animate(CBasePlayer *pPlayer, int activity)
{
	entvars_t *pev = pPlayer->pev;
	if (activity < 0)
		return;

	const char *grip = pPlayer->m_szAnimExtention;
	if (!grip || !grip[0])
		grip = "onehanded"; // a held TS weapon always sets one of onehanded/twohanded/akimbo

	const bool ducking = (pev->flags & FL_DUCKING) != 0;
	const bool onground = (pev->flags & FL_ONGROUND) != 0;
	const float speed = pev->velocity.Length2D();

	char name[64];

	switch (activity)
	{
	// ---- Death (full body) -------------------------------------------------
	case TSA_DIE:
	{
		// The original picks a directional/special death; we use the simple
		// spin death (die_spin) — the variants need the attack-direction state
		// (g_vecAttackDir) wired through, a later refinement.
		SetBodySequence(pPlayer, "die_spin", true, true);
		SetGaitSequence(pPlayer, NULL);
		m_iLastActivity = activity;
		return;
	}

	// ---- Jump (full body) --------------------------------------------------
	case TSA_JUMP:
	{
		if (speed > 16.0f)
		{
			int dir = GetMoveDirection(pPlayer);
			m_iDirection = dir;
			const char *jd = (dir == 0) ? "jump_left_" : (dir == 1) ? "jump_right_" : "jump_forward_";
			strcpy(name, jd);
			strcat(name, grip); // jump_forward_onehanded, ...
			SetBodySequence(pPlayer, name, false, false);
		}
		else
		{
			SetBodySequence(pPlayer, "standard_jump", false, false);
		}
		SetGaitSequence(pPlayer, NULL);
		m_iLastActivity = activity;
		return;
	}

	// ---- Duck transition ---------------------------------------------------
	case TSA_DUCK:
	{
		if (m_iDuckLatch == 0)
		{
			SetBodySequence(pPlayer, "goduck", false, false);
			m_iDuckLatch = 1;
		}
		m_iLastActivity = activity;
		return;
	}

	// ---- Grenade throw (upper body, keep legs) -----------------------------
	case TSA_GRENADE:
	{
		if (!onground)
			break;
		SetBodySequence(pPlayer, "grenade", true, false);
		m_flAimTimer = 2.0f;
		m_flGrenadeFlag = 1.0f;
		m_iLastActivity = activity;
		return;
	}

	// ---- Reload / draw (upper body, keep legs) -----------------------------
	case TSA_RELOAD:
	case TSA_DRAW:
	{
		if (!onground)
			break;
		strcpy(name, (activity == TSA_RELOAD) ? "relo_" : "draw_");
		strcat(name, grip); // relo_onehanded, draw_twohanded, ...
		SetBodySequence(pPlayer, name, true, false);
		m_flAimTimer = 2.0f;
		m_iLastActivity = activity;
		return;
	}

	// ---- Fire (upper body, keep legs) --------------------------------------
	case TSA_FIRE:
	case TSA_FIRE_SLIDEL:
	case TSA_FIRE_SLIDER:
	{
		if (ducking)
		{
			strcpy(name, "crouch_shoot_");
			strcat(name, grip); // crouch_shoot_onehanded, ...
		}
		else
		{
			strcpy(name, "shoot_");
			strcat(name, grip); // shoot_onehanded, shoot_twohanded, shoot_akimbo_* handled below
			// onehanded/twohanded have an alternating "2" recoil variant the
			// original toggles; akimbo fire is split L/R by AkimboFire's caller.
			if (strncmp(grip, "akimbo", 6) == 0)
			{
				// akimbo: pick a side; the gun code alternates barrels, default left.
				strcpy(name, "shoot_akimbo_left");
			}
		}
		SetBodySequence(pPlayer, name, true, true);
		m_flAimTimer = 2.0f; // hold the weapon raised; protects the torso pose below
		// fire does NOT touch the legs — keep whatever the move/idle path set.
		m_iLastActivity = activity;
		return;
	}

	// ---- STUNT poses (RE CTSAnimation::Animate @0x5b470 + the model sequence set) ----
	// The TS player model carries the full acrobatics set: dive_<dir>_<grip> + dive_<dir>_end,
	// slide_<dir>_<grip> / slide_b_left|right, crawl, wheel_<dir>/lowwheel_<dir>, walljump_climb/
	// walljump_end, morfy_jump, bullettime. CTSStunt drives these via the activity codes below.
	// dir suffix: 0 L / 1 R / 2 F / 3 B  (GetMoveDirection).
	case 0x28: // dive (airborne)
	case 2:    // dive-in-progress (HandleDive)
	{
		static const char *ds[4] = {"l", "r", "f", "b"};
		int dir = GetMoveDirection(pPlayer);
		snprintf(name, sizeof(name), "dive_%s_%s", ds[dir], grip);
		SetBodySequence(pPlayer, name, false, true);
		SetGaitSequence(pPlayer, NULL);
		m_flAimTimer = 0.4f; // hold the dive torso pose
		m_iLastActivity = activity;
		return;
	}
	case 0x2e: // landing (dive end / get up)
	case 0x2f: // roll
	{
		static const char *ds[4] = {"l", "r", "f", "b"};
		int dir = GetMoveDirection(pPlayer);
		snprintf(name, sizeof(name), "dive_%s_end", ds[dir]);
		SetBodySequence(pPlayer, name, true, true);
		SetGaitSequence(pPlayer, NULL);
		m_flAimTimer = 0.5f;
		m_iLastActivity = activity;
		return;
	}
	case 0x29: // slide
	case 0x2a: // slide back-left
	case 0x2b: // slide back-right
	{
		int dir = GetMoveDirection(pPlayer);
		if (activity == 0x2a || (dir == 3))
			strcpy(name, "slide_b_left");
		else if (activity == 0x2b)
			strcpy(name, "slide_b_right");
		else
		{
			static const char *ds[4] = {"l", "r", "f", "b"};
			snprintf(name, sizeof(name), "slide_%s_%s", ds[dir], grip);
		}
		SetBodySequence(pPlayer, name, false, true);
		SetGaitSequence(pPlayer, NULL);
		m_flAimTimer = 0.3f;
		m_iLastActivity = activity;
		return;
	}
	case 0x31: // cartwheel (high)
	case 0x32: // cartwheel (low)
	{
		static const char *ds[4] = {"l", "r", "f", "b"};
		int dir = GetMoveDirection(pPlayer);
		snprintf(name, sizeof(name), "%swheel_%s", (activity == 0x32) ? "low" : "", ds[dir]);
		SetBodySequence(pPlayer, name, true, true);
		SetGaitSequence(pPlayer, NULL);
		m_flAimTimer = 0.5f;
		m_iLastActivity = activity;
		return;
	}
	case 0x37: // wall-jump
		SetBodySequence(pPlayer, "walljump_end", true, true);
		SetGaitSequence(pPlayer, NULL);
		m_flAimTimer = 0.4f;
		m_iLastActivity = activity;
		return;
	case 0x38: // wall-climb
		SetBodySequence(pPlayer, "walljump_climb", true, true);
		SetGaitSequence(pPlayer, NULL);
		m_flAimTimer = 0.4f;
		m_iLastActivity = activity;
		return;
	case 0x36: // bullet-time dodge
		SetBodySequence(pPlayer, "bullettime", true, true);
		SetGaitSequence(pPlayer, NULL);
		m_flAimTimer = 0.55f;
		m_iLastActivity = activity;
		return;
	case 0x39: // morfy jump (wheel finish)
		SetBodySequence(pPlayer, "morfy_jump", true, true);
		SetGaitSequence(pPlayer, NULL);
		m_flAimTimer = 0.4f;
		m_iLastActivity = activity;
		return;

	// ---- Move / idle (the per-frame PostThink path) ------------------------
	case TSA_RUN:
	case TSA_IDLE:
	case TSA_SLIDE:
	default:
		break;
	}

	// Fall-through: the move/idle gait. Set the leg locomotion (gaitsequence)
	// always; set the torso pose (idle_/aim_<grip>) only when not mid-action
	// (firing/reloading) — the aim timer protects a just-set shoot/relo pose,
	// exactly as the original gates the sequence overwrite on param_1[3] (@0x6cf..).
	if (!onground)
	{
		// airborne but not a jump activity: keep current, just record.
		m_iLastActivity = activity;
		return;
	}

	m_iDuckLatch = 0; // reset goduck latch once grounded/standing

	// Legs: crawl when ducking+moving, run2/walk2 by speed, none when still.
	if (speed > 1.0f)
	{
		m_iDirection = GetMoveDirection(pPlayer);
		if (ducking)
			SetGaitSequence(pPlayer, "crawl");
		else
			SetGaitSequence(pPlayer, (speed > TSA_RUN_SPEED) ? "run2" : "walk2");
	}
	else
	{
		SetGaitSequence(pPlayer, NULL);
	}

	// Torso: don't stomp an in-progress fire/reload pose (aim timer running).
	const bool aiming = (m_flAimTimer >= 0.01f) || (pev->button & IN_ATTACK);
	if (m_flAimTimer >= 0.01f)
	{
		// weapon raised recently — leave the shoot/relo/aim torso sequence playing.
		m_iLastActivity = activity;
		return;
	}

	if (m_bKungFu)
	{
		// Kung-fu (bare hands): the fighting stance, NOT an aim_<grip> invisible-pistol pose. This
		// is the torso the gait blends with while walking (legs from gaitsequence, fists up here).
		SetBodySequence(pPlayer, ducking ? "combat_idle_low" : "combat_idle", false, false);
		m_iLastActivity = activity;
		return;
	}

	if (ducking)
		strcpy(name, aiming ? "crouch_aim_" : "duck_idle_");
	else
		strcpy(name, aiming ? "aim_" : "idle_");
	strcat(name, grip); // idle_onehanded, aim_twohanded, duck_idle_akimbo, ...

	SetBodySequence(pPlayer, name, false, false);
	m_iLastActivity = activity;
}
