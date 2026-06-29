/***
*  ts_animation.cpp — RECONSTRUCTED CTSAnimation (TS 3rd-person body/gait).
*
*  Mirrors the original's 13-method layout (re.sh CTSAnimation, @0x5b320..0x5dd14):
*    Init / UpdateTimers / Animate (the 4-phase dispatcher) / EnableBulletDance /
*    DisableBulletDance / BulletDance / BlendAnimationsForSlide / BlendAnimationsByDir /
*    BlendJumpAnimations / SetAnim / HandleCloseCombat / IsInCloseCombat / GaitCloseCombat
*    + the free GetDirection (original GetDirection__FG6VectorT0ff).
*  Behaviour (which sequence plays for which player state) is preserved from the
*  playtest-confirmed reconstruction; the byte layout is provenance only (CLAUDE.md).
***/
#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "player.h"
#include "ts_animation.h"
#include <string.h>

// Leg locomotion gait speed split (original fc_180): above -> run2, below (moving) -> walk2.
#define TSA_RUN_SPEED 180.0f

// ---- free GetDirection (original GetDirection__FG6VectorT0ff) --------------------
// 0=left 1=right 2=forward 3=back, from velocity relative to a yaw facing. The original
// takes (normalized dir, view-angles, fwdBias, sideBias); we reproduce the behaviour
// from the horizontal velocity vs the facing forward/right basis.
int GetDirection(const Vector &dir, const Vector &angles, float fwdBias, float sideBias)
{
	Vector v = dir;
	v.z = 0;
	if (v.Length() < 1.0f)
		return 2; // forward when stationary/ambiguous

	UTIL_MakeVectors(Vector(0, angles.y, 0));
	Vector fwd = gpGlobals->v_forward;  fwd.z = 0;
	Vector right = gpGlobals->v_right;  right.z = 0;

	float fdot = DotProduct(v, fwd);
	float rdot = DotProduct(v, right);

	if (fabs(fdot) >= fabs(rdot))
		return (fdot >= 0) ? 2 /*forward*/ : 3 /*back*/;
	return (rdot >= 0) ? 1 /*right*/ : 0 /*left*/;
}

// ---- Init (@0x5b320) -------------------------------------------------------------
void CTSAnimation::Init()
{
	m_iLastActivity = -1;
	m_iDuckLatch = 0;
	m_flLegTimer = 0;
	m_flAimTimer = 0;
	m_flGrenadeFlag = 0;
	m_iDirection = -1;
	m_iEnabled = 1;
	m_flBulletDanceRetrig = 0;
	m_flBulletDanceWindow = 0;
	m_bBulletDance = 0;
	m_bCCStarted = false;
}

// ---- UpdateTimers (@0x5b37c) -----------------------------------------------------
// Decay the per-frame hold timers, scaled by the player's slow-motion factor
// (GiveSlowMul). field_0x8 (leg timer) may settle slightly either side of 0.
void CTSAnimation::UpdateTimers(CBasePlayer *pPlayer)
{
	float dt = gpGlobals->frametime;
#ifndef CLIENT_DLL
	dt *= pPlayer->m_flSlowMotionGo; // GiveSlowMul(): slow-motion scales the hold timers (1 = normal)
#endif
	if (m_flBulletDanceWindow > 0) { m_flBulletDanceWindow -= dt; if (m_flBulletDanceWindow < 0) m_flBulletDanceWindow = 0; }
	if (m_flBulletDanceRetrig > 0) { m_flBulletDanceRetrig -= dt; if (m_flBulletDanceRetrig < 0) m_flBulletDanceRetrig = 0; }
	if (m_flAimTimer > 0)     { m_flAimTimer -= dt;     if (m_flAimTimer < 0) m_flAimTimer = 0; }
	if (m_flGrenadeFlag > 0)  { m_flGrenadeFlag -= dt;  if (m_flGrenadeFlag < 0) m_flGrenadeFlag = 0; }
	if (m_flLegTimer > 0)     { m_flLegTimer -= dt;     if (m_flLegTimer < 0) m_flLegTimer = 0; }
}

// ---- SetAnim (@0x5d7e4) — the unified sequence setter --------------------------
// flags per TSSetAnimFlags. Returns 1 if it changed the sequence.
int CTSAnimation::SetAnim(CBasePlayer *pPlayer, const char *name, int flags, int frame)
{
	int seq = pPlayer->LookupSequence(name);
	if (seq == -1)
		seq = 0;

	// No-op if already on this sequence and not forcing.
	if (pPlayer->pev->sequence == seq && (flags & TSSA_FORCE) == 0)
		return 0;

	if (flags & TSSA_INTERP)
		pPlayer->pev->effects &= ~EF_NOINTERP;
	else
		pPlayer->pev->effects |= EF_NOINTERP;

	if (flags & TSSA_CLEARLEGS)
	{
		pPlayer->pev->gaitsequence = 0;
	}
	else if (flags & TSSA_LEGS)
	{
		pPlayer->pev->gaitsequence = seq; // legs-only
		return 1;
	}

	pPlayer->pev->sequence = seq;
	if (flags & TSSA_SETFRAME)
		pPlayer->pev->frame = (float)frame;
	else if ((flags & TSSA_KEEPFRAME) == 0)
		pPlayer->pev->frame = 0;
	pPlayer->ResetSequenceInfo();
	return 1;
}

// ---- bullet-dance (@0x5d2f0 / 0x5d314 / 0x5d338) --------------------------------
void CTSAnimation::DisableBulletDance(CBasePlayer *pPlayer)
{
	m_bBulletDance = 0;
	m_flBulletDanceRetrig = 0;
	m_flBulletDanceWindow = 0;
}

void CTSAnimation::EnableBulletDance(CBasePlayer *pPlayer)
{
	m_bBulletDance = 1;
	m_flBulletDanceRetrig = 0.15f;
	m_flBulletDanceWindow = 3.0f;
}

// Getting shot while in bullet-time plays a brief normal_die* flinch.
void CTSAnimation::BulletDance(CBasePlayer *pPlayer)
{
	if (!m_bBulletDance)
		return;
	if (m_flBulletDanceWindow < 0.01f)
		return;
	if (m_flBulletDanceRetrig > 0)
		return;
	if (pPlayer->m_fSequenceFinished)
	{
		m_flBulletDanceRetrig = 0.15f;
		static const char *const dies[3] = { "normal_die1", "normal_die2", "normal_die3" };
		SetAnim(pPlayer, dies[RANDOM_LONG(0, 2)], TSSA_INTERP | TSSA_CLEARLEGS, 0);
	}
}

// ---- close-combat predicates (@0x5dcf4 / 0x5dd14) -------------------------------
bool CTSAnimation::IsInCloseCombat(int activity)
{
	return activity == TSA_CC_SUPERJUMP_LAND || (unsigned)(activity - 0x0a) < 0x1e;
}

// 1 if the close-combat move should keep the legs gaiting (the player is moving/ducking).
int CTSAnimation::GaitCloseCombat(int activity, CBasePlayer *pPlayer)
{
	entvars_t *pev = pPlayer->pev;
	// katana-duck while actually ducking -> full body (no gait)
	if (activity == TSA_CC_KATANADUCK && ((pev->flags & FL_DUCKING) || (pev->button & IN_DUCK)))
		return 0;

	bool gaitMove = (activity == TSA_CC_PUNCHLEFT || activity == TSA_CC_PUNCHRIGHT ||
		(activity >= TSA_CC_KATANA1 && activity <= TSA_CC_KATANADUCK) || activity == TSA_CC_UPPERCUT ||
		activity == TSA_CC_BLOCK || activity == TSA_CC_STOCK ||
		(activity >= TSA_CC_KNIFE1 && activity <= TSA_CC_KNIFELAUNCH));
	// excludes katana_air(0x24)/katana3(0x23)/superjump_land(0x3a)
	if (activity == TSA_CC_KATANAAIR || activity == TSA_CC_KATANA3 || activity == TSA_CC_SUPERJUMP_LAND)
		gaitMove = false;

	if (gaitMove && ((pev->flags & FL_DUCKING) || pev->velocity.Length2D() > 120.0f))
		return 1;
	return 0;
}

// ---- HandleCloseCombat (@0x5d8d0) ----------------------------------------------
// Build the kung-fu/knife/katana body sequence for a close-combat activity code and set it.
void CTSAnimation::HandleCloseCombat(CBasePlayer *pPlayer, int activity)
{
	char name[64];
	name[0] = 0;

	switch (activity)
	{
	case TSA_CC_CIRCFRONT:    strcpy(name, "circfront");    break;
	case TSA_CC_CIRCKICK:     strcpy(name, "circkick");     break;
	case TSA_CC_CIRCKICKLOW:  strcpy(name, "circkicklow");  break;
	case TSA_CC_CIRCBACK:     strcpy(name, "circback");     break;
	case TSA_CC_LATBACK:      strcpy(name, "latback");      break;
	case TSA_CC_CIRCSPIN:     strcpy(name, "circspin");     break;
	case TSA_CC_SIDEKICK:     strcpy(name, "sidekick");     break;
	case TSA_CC_BACKKICK:     strcpy(name, "backkick");     break;
	case TSA_CC_AIRKICK:      strcpy(name, "airkick");      break;
	case TSA_CC_AIRKICK2:     strcpy(name, "airkick2");     break;
	case TSA_CC_DUCKKICK:     strcpy(name, "duckkick");     break;
	case TSA_CC_PUNCHLEFT:    strcpy(name, "punchleft");    break;
	case TSA_CC_PUNCHRIGHT:   strcpy(name, "punchright");   break;
	case TSA_CC_PUNCHAIR:     strcpy(name, "punchair");     break;
	case TSA_CC_UPPERCUT:     strcpy(name, "uppercut");     break;
	case TSA_CC_KNIFE1:       strcpy(name, "knife1");       break;
	case TSA_CC_KNIFE2:       strcpy(name, "knife2");       break;
	case TSA_CC_KNIFE3:       strcpy(name, "knife3");       break;
	case TSA_CC_KNIFELAUNCH:  strcpy(name, "knife_launch"); break;
	case TSA_CC_KATANA1:      strcpy(name, "katana1");      break;
	case TSA_CC_KATANA2:      strcpy(name, "katana2");      break;
	case TSA_CC_KATANA3:      strcpy(name, "katana3");      break;
	case TSA_CC_KATANAAIR:    strcpy(name, "katana_air");   break;
	case TSA_CC_KATANADUCK:   strcpy(name, "katana_duck");  break;
	case TSA_CC_BLOCK:
	{
		// katana_block / knife_block / kungfu_block by the active weapon-ext, + a random "1"/"2".
		const char *ext = pPlayer->m_szAnimExtention;
		if (strncmp(ext, "kat", 3) == 0)       strcpy(name, "katana_block");
		else if (strncmp(ext, "kni", 3) == 0)  strcpy(name, "knife_block");
		else                                   strcpy(name, "kungfu_block");
		strcat(name, RANDOM_LONG(0, 1) ? "1" : "2");
		break;
	}
	case TSA_CC_STOCK:        strcpy(name, "stock_"); strcat(name, pPlayer->m_szAnimExtention); break;
	case TSA_CC_SUPERJUMP_LAND: strcpy(name, "superjump_land"); break;
	default: return;
	}

	int flags = GaitCloseCombat(activity, pPlayer) ? 0x12 : 0x14; // keep-legs vs clear-legs (both interp)
	SetAnim(pPlayer, name, flags | TSSA_FORCE, 0);
	m_flAimTimer = 2.0f;
	m_flLegTimer = 3.0f;
	m_bCCStarted = false;
}

// ---- BlendAnimationsByDir (@0x5d538) — generic directional re-blend -------------
void CTSAnimation::BlendAnimationsByDir(CBasePlayer *pPlayer, const Vector &dir,
	const Vector &angles, const char *const dirTable[4], const char *grip)
{
	if (!grip || !grip[0])
		return;
	int d = GetDirection(dir, angles, 0.6f, 0.3f);
	if (d != m_iDirection && d >= 0 && d < 4)
	{
		char name[64];
		strcpy(name, dirTable[d]);
		strcat(name, grip);
		SetAnim(pPlayer, name, 0x194, 0); // blend: force+interp+clearlegs+keepframe
		m_iDirection = d;
	}
}

// ---- BlendAnimationsForSlide (@0x5d400) — continuous slide-dir update -----------
void CTSAnimation::BlendAnimationsForSlide(CBasePlayer *pPlayer)
{
	static const char *const slideTable[4] = { "slide_l_", "slide_r_", "slide_f_", "slide_b_" };
	const char *grip = pPlayer->m_szAnimExtention;
	int d = GetDirection(pPlayer->pev->velocity, pPlayer->pev->v_angle, 0.6f, 0.3f);
	if (d != m_iDirection && d != 2 && d >= 0 && d < 4)
	{
		char name[64];
		strcpy(name, slideTable[d]);
		strcat(name, (grip && grip[0]) ? grip : "akimbo");
		SetAnim(pPlayer, name, 0x194, 0);
		m_iDirection = d;
	}
}

// ---- BlendJumpAnimations (@0x5d630) — directional jump re-blend -----------------
void CTSAnimation::BlendJumpAnimations(CBasePlayer *pPlayer)
{
	const char *grip = pPlayer->m_szAnimExtention;
	if (!grip || !grip[0])
		return;
	int d = GetDirection(pPlayer->pev->velocity, pPlayer->pev->v_angle, 0.6f, 0.3f);
	if (d != m_iDirection)
	{
		const char *jd = (d == 0) ? "jump_left_" : (d == 1) ? "jump_right_" : "jump_forward_";
		char name[64];
		strcpy(name, jd);
		strcat(name, grip);
		SetAnim(pPlayer, name, 0x194, 0);
		m_iDirection = d;
	}
}

// ---- Animate (@0x5b470) — the 4-region gait/animation dispatcher ----------------
// Region A: continue an in-flight directional blend. Region B: HOLD guards — PostThink fires
// Animate(1=run/6=idle) EVERY frame, so without these the next frame stomps an in-progress
// dive/stunt (this was the dominant cause of "dive looks like a crouch"). Region C: set the NEW
// body sequence; cases BREAK (not return) into Region D. Region D: set the LEG gait
// (pev->gaitsequence) from velocity for all but the excluded activities — skipping that tail
// left the legs at sequence-0 (a crouch) under the dive body (the other half of the dive bug).
void CTSAnimation::Animate(CBasePlayer *pPlayer, int activity)
{
	entvars_t *pev = pPlayer->pev;
	if (activity < 0)
		return;

	// Grip suffix (m_szAnimExtention) — EMPTY for bare-hands kung-fu (combat_idle + close-combat),
	// else onehanded/twohanded/akimbo. Stunt sequences use "akimbo" as the grip when bare-handed.
	const char *grip = pPlayer->m_szAnimExtention;
	if (!grip)
		grip = "";
	const bool kungfu = (grip[0] == 0);
	const char *const sgrip = kungfu ? "akimbo" : grip;

	const bool ducking = (pev->flags & FL_DUCKING) != 0;
	const bool onground = (pev->flags & FL_ONGROUND) != 0;
	const bool onladder = (pev->movetype == MOVETYPE_FLY);
	const bool alive = pPlayer->IsAlive();
	const bool frozen = (pev->flags & FL_FROZEN) != 0;
	const bool seqPlaying = (pPlayer->m_fSequenceFinished == 0);
	const float speed = pev->velocity.Length2D();

	char name[64];

	// ---- Region A: continue an in-flight directional blend -------------------
	if (alive && !frozen)
	{
		if (m_iLastActivity == TSA_JUMP && speed > 96.0f && !IsInCloseCombat(m_iLastActivity))
			BlendJumpAnimations(pPlayer);
		else if (activity == TSA_BLENDDIR)
		{
			static const char *const diveTable[4] = { "dive_l_", "dive_r_", "dive_f_", "dive_b_" };
			BlendAnimationsByDir(pPlayer, pev->velocity, pev->v_angle, diveTable, sgrip);
		}
	}

	// ---- Region B: HOLD guards — keep an in-progress stunt/fire playing -------
	// (the original's switch(m_iLastActivity); each case returns early while its sequence is still
	// playing and the new activity isn't an interrupt). Without these the per-frame Animate(1/6)
	// overwrites every stunt one frame after it starts.
	if (alive && !frozen && !IsInCloseCombat(activity))
	{
		switch (m_iLastActivity)
		{
		case TSA_SLIDE:
			if (activity != TSA_JUMP && activity != TSA_MORFY && activity != TSA_SLIDE_BL &&
				activity != TSA_SLIDE_BR && activity != TSA_PRONE && activity != TSA_FIRE &&
				activity != TSA_FIRE_SLIDEL && activity != TSA_FIRE_SLIDER && !onladder)
			{
				BlendAnimationsForSlide(pPlayer);
				return;
			}
			break;
		case TSA_JUMP:
			if (seqPlaying && activity != TSA_JUMP && activity != TSA_DIVE && activity != TSA_ROLL &&
				activity != TSA_DIVE_END && activity != TSA_WHEEL && activity != TSA_LOWWHEEL &&
				activity != TSA_WALLJUMP && activity != TSA_WALLCLIMB && activity != TSA_BULLETTIME)
				return;
			break;
		case TSA_DIE:
			return; // death holds unconditionally
		case TSA_DIVE:
			if (seqPlaying && activity != TSA_PRONE && activity != TSA_GETUP && activity != TSA_SLIDE)
				return; // hold the dive (directly protects it from the per-frame Animate(1))
			break;
		case TSA_PRONE:
		case TSA_GETUP:
		case TSA_ROLL:
			if (seqPlaying && (pev->iuser4 & 4))
				return;
			break;
		case TSA_SLIDE_BL:
		case TSA_SLIDE_BR:
			// still moving along the slide side -> hold. VERIFY: original uses a velocity·v_right sign test.
			if (seqPlaying && speed > 1.0f)
				return;
			break;
		case TSA_DIVE_END:
		case TSA_BULLETTIME:
		case TSA_WHEEL:
		case TSA_LOWWHEEL:
		case TSA_WALLJUMP:
		case TSA_WALLCLIMB:
		case TSA_MORFY:
			if (seqPlaying)
				return; // hold the stunt while its sequence plays
			break;
		}
	}

	// ---- Region C: pick & set the NEW body sequence (cases BREAK into Region D) --
	bool ccHandled = false;
	if (IsInCloseCombat(activity))
	{
		HandleCloseCombat(pPlayer, activity);
		ccHandled = true;
	}
	else switch (activity)
	{
	// ---- Fire (upper body, keep legs) --------------------------------------
	case TSA_FIRE:
	case TSA_FIRE_SLIDEL:
	case TSA_FIRE_SLIDER:
		if (ducking)
		{
			strcpy(name, "crouch_shoot_");
			strcat(name, grip);
		}
		else if (strncmp(grip, "akimbo", 6) == 0)
			strcpy(name, "shoot_akimbo_left"); // akimbo alternates barrels; default left
		else
		{
			strcpy(name, "shoot_");
			strcat(name, grip);
		}
		if (activity == TSA_FIRE_SLIDEL) strcat(name, "slide_b_left");
		else if (activity == TSA_FIRE_SLIDER) strcat(name, "slide_b_right");
		SetAnim(pPlayer, name, 0x92, 0); // force+interp, keep legs
		m_flAimTimer = 2.0f;
		break;

	// ---- Move / idle TORSO (legs are Region D) -----------------------------
	case TSA_RUN:
	case TSA_IDLE:
		if (!onground && !onladder)
			return; // airborne non-jump: keep whatever's playing
		// Keep an in-progress fire/draw/reload upper body playing instead of stomping it with idle.
		if ((m_iLastActivity == TSA_FIRE || m_iLastActivity == TSA_FIRE_SLIDEL ||
			 m_iLastActivity == TSA_FIRE_SLIDER || m_iLastActivity == TSA_DRAW ||
			 m_iLastActivity == TSA_RELOAD) && seqPlaying)
			break; // legs still update in Region D; torso left alone
		if (kungfu)
			SetAnim(pPlayer, ducking ? "combat_idle_low" : "combat_idle", 0x12, 0);
		else
		{
			const bool aim = (m_flAimTimer >= 0.01f) || (pev->fov != 0);
			if (ducking)
				strcpy(name, aim ? "crouch_aim_" : "duck_idle_");
			else
				strcpy(name, aim ? "aim_" : "idle_");
			strcat(name, grip);
			SetAnim(pPlayer, name, 0x12, 0);
		}
		break;

	case TSA_SLIDE:
		break; // slide body handled by the Region-B blend; legs in Region D

	// ---- Jump (full body; EXCLUDED from gait) ------------------------------
	case TSA_JUMP:
		if (speed > 16.0f)
		{
			int dir = GetDirection(pev->velocity, pev->v_angle, 0.6f, 0.3f);
			m_iDirection = dir;
			const char *jd = (dir == 0) ? "jump_left_" : (dir == 1) ? "jump_right_" : "jump_forward_";
			strcpy(name, jd);
			strcat(name, grip);
			SetAnim(pPlayer, name, 0x94, 0);
		}
		else
			SetAnim(pPlayer, "standard_jump", 0x94, 0);
		break;

	// ---- Duck transition (EXCLUDED from gait) ------------------------------
	case TSA_DUCK:
		if (m_iDuckLatch == 0)
		{
			SetAnim(pPlayer, "goduck", 0x94, 0);
			m_iDuckLatch = 1;
		}
		break;

	// ---- Death (full body; EXCLUDED from gait) -----------------------------
	case TSA_DIE:
		if (m_bBulletDance)
		{
			static const char *const dies[3] = { "normal_die1", "normal_die2", "normal_die3" };
			SetAnim(pPlayer, dies[RANDOM_LONG(0, 2)], 0x14, 0);
		}
		else
			SetAnim(pPlayer, "die_spin", 0x14, 0); // directional death deferred — spin death
		break;

	// ---- Grenade throw (upper body, keep legs) -----------------------------
	case TSA_GRENADE:
		if (!onground)
			break;
		SetAnim(pPlayer, "grenade", 0x92, 0);
		m_flAimTimer = 2.0f;
		m_flGrenadeFlag = 1.0f;
		break;

	// ---- Reload / draw (upper body, keep legs) -----------------------------
	case TSA_RELOAD:
	case TSA_DRAW:
		if (!onground)
			break;
		strcpy(name, (activity == TSA_RELOAD) ? "relo_" : "draw_");
		strcat(name, grip);
		SetAnim(pPlayer, name, 0x92, 0);
		m_flAimTimer = 2.0f;
		break;

	// ---- DIVE (full body; gait RUNS so the legs read as a dive, not a crouch) --
	case TSA_DIVE:
	{
		int dir = GetDirection(pev->velocity, pev->v_angle, 0.6f, 0.3f);
		m_iDirection = dir;
		static const char *const ds[4] = { "l", "r", "f", "b" };
		snprintf(name, sizeof(name), "dive_%s_%s", ds[dir & 3], sgrip);
		SetAnim(pPlayer, name, 0x94, 0);
		break;
	}

	// ---- PRONE (slide_<dir>_end; EXCLUDED) — P2 fix: was slide_b_left/right -----
	case TSA_PRONE:
	{
		static const char *const ds[4] = { "l", "r", "f", "b" };
		int dir = (m_iDirection >= 0 && m_iDirection < 4) ? m_iDirection
			: GetDirection(pev->velocity, pev->v_angle, 0.6f, 0.3f);
		snprintf(name, sizeof(name), "slide_%s_end", ds[dir & 3]);
		SetAnim(pPlayer, name, 0x94, 0);
		m_flLegTimer = 3.0f;
		break;
	}

	// ---- SLIDE_BL / SLIDE_BR (gait RUNS) -----------------------------------
	case TSA_SLIDE_BL:
		SetAnim(pPlayer, "slide_b_left", 0x14, 0);
		break;
	case TSA_SLIDE_BR:
		SetAnim(pPlayer, "slide_b_right", 0x14, 0);
		break;

	// ---- GETUP (dive_<dir>_end; EXCLUDED) ----------------------------------
	case TSA_GETUP:
	{
		static const char *const ds[4] = { "l", "r", "f", "b" };
		int dir = GetDirection(pev->velocity, pev->v_angle, 0.6f, 0.3f);
		snprintf(name, sizeof(name), "dive_%s_end", ds[dir & 3]);
		SetAnim(pPlayer, name, 0x94, 0);
		break;
	}

	// ---- ROLL (roll_<dir>; EXCLUDED) — P2 fix: was dive_<dir>_end ----------
	case TSA_ROLL:
	{
		static const char *const ds[4] = { "l", "r", "f", "b" };
		int dir = GetDirection(pev->velocity, pev->v_angle, 0.6f, 0.3f);
		snprintf(name, sizeof(name), "roll_%s", ds[dir & 3]);
		SetAnim(pPlayer, name, 0x94, 0);
		m_flLegTimer = 3.0f;
		break;
	}

	// ---- DIVE_END (roll_l/roll_r/roll_f_high/roll_b; EXCLUDED) — P2 fix -----
	case TSA_DIVE_END:
	{
		static const char *const ds[4] = { "roll_l", "roll_r", "roll_f_high", "roll_b" };
		int dir = GetDirection(pev->velocity, pev->v_angle, 0.6f, 0.3f);
		SetAnim(pPlayer, ds[dir & 3], 0x94, 0);
		break;
	}

	// ---- WHEEL / LOWWHEEL (gait RUNS) — P2: lowwheel has no _b --------------
	case TSA_WHEEL:
	{
		static const char *const ds[4] = { "l", "r", "f", "b" };
		int dir = GetDirection(pev->velocity, pev->v_angle, 0.6f, 0.3f);
		snprintf(name, sizeof(name), "wheel_%s", ds[dir & 3]);
		SetAnim(pPlayer, name, 0x94, 0);
		m_flLegTimer = 3.0f;
		break;
	}
	case TSA_LOWWHEEL:
	{
		int dir = GetDirection(pev->velocity, pev->v_angle, 0.6f, 0.3f);
		const char *lw = (dir == 0) ? "lowwheel_l" : (dir == 1) ? "lowwheel_r" : "lowwheel_f";
		SetAnim(pPlayer, lw, 0x94, 0);
		m_flLegTimer = 3.0f;
		break;
	}

	// ---- WALLJUMP / WALLCLIMB (gait RUNS) ----------------------------------
	case TSA_WALLJUMP:
		SetAnim(pPlayer, "walljump_end", 0x94, 0);
		m_flLegTimer = 3.0f;
		break;
	case TSA_WALLCLIMB:
		SetAnim(pPlayer, "walljump_climb", 0x94, 0);
		break;

	// ---- BULLETTIME / MORFY (EXCLUDED) -------------------------------------
	case TSA_BULLETTIME:
		SetAnim(pPlayer, "bullettime", 0x94, 0);
		break;
	case TSA_MORFY:
		SetAnim(pPlayer, kungfu ? "morfy_jump" : "morfy_jump_guns", 0x94, 0);
		break;

	default:
		break;
	}

	// ---- Region D: record the activity, then set the LEG gait --------------
	if (!ccHandled)
		m_iLastActivity = activity;

	// Activities that own the whole body (jump/duck/death/prone/getup/roll/dive_end/bullettime/
	// morfy) DON'T run the leg gait; nor does a close-combat move that isn't a gait-move. The stunt
	// state bits (iuser4) also freeze the legs mid-stunt.
	switch (activity)
	{
	case TSA_JUMP: case TSA_MORFY: case TSA_DUCK: case TSA_DIE:
	case TSA_PRONE: case TSA_GETUP: case TSA_ROLL: case TSA_DIVE_END: case TSA_BULLETTIME:
		return; // excluded from the gait tail
	}
	if (ccHandled && !GaitCloseCombat(activity, pPlayer))
		return;
	if (pev->iuser4 & 0x30c30)
		return; // mid-stunt: legs frozen by the movement code

	m_iDuckLatch = 0; // reset goduck latch once running the normal gait

	// LEGS: gaitsequence set DIRECTLY from velocity (run2/walk2/crawl/crouch_idle/idle_/combat_idle).
	if (ducking)
		SetAnim(pPlayer, (speed == 0.0f) ? "crouch_idle" : "crawl", TSSA_LEGS, 0);
	else if (speed > TSA_RUN_SPEED)
		SetAnim(pPlayer, "run2", TSSA_LEGS, 0);
	else if (speed > 0.0f)
		SetAnim(pPlayer, "walk2", TSSA_LEGS, 0);
	else
	{
		// standing still: VERIFY — the original also consults a second player anim-string
		// (player+0x478) we don't model yet; with it empty this is the common path.
		if (kungfu)
			SetAnim(pPlayer, (m_flAimTimer >= 0.01f) ? "combat_idle" : "combat_idle_low", TSSA_LEGS, 0);
		else
		{
			strcpy(name, "idle_");
			strcat(name, grip);
			SetAnim(pPlayer, name, TSSA_LEGS, 0);
		}
	}
}
