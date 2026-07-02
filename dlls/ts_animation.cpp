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

// ---- free GetDirection (original GetDirection__FG6VectorT0ff @0x6b008, VERBATIM) --------
// 0=left 1=right 2=forward 3=back. Takes a NORMALIZED direction + view angles; the basis is
// yaw+roll with PITCH ZEROED. Priority: right-dot vs ±sideThresh(0.6) first, then
// forward-dot < -fwdThresh(0.3) = back, else forward. Callers MUST pass a unit vector
// (the original normalizes at every call site; zero velocity becomes (0,0,1) -> forward).
int GetDirection(const Vector &dir, const Vector &angles, float sideThresh, float fwdThresh)
{
	UTIL_MakeVectors(Vector(0, angles.y, angles.z)); // pitch zeroed, yaw+roll kept (RE)
	float rdot = DotProduct(gpGlobals->v_right, dir);
	if (rdot < -sideThresh)
		return 0; // left
	if (rdot > sideThresh)
		return 1; // right
	return (DotProduct(gpGlobals->v_forward, dir) < -fwdThresh) ? 3 /*back*/ : 2 /*forward*/;
}

// The original's per-call-site normalization (velocity -> unit dir; zero -> (0,0,1)).
static Vector TS_DirNormalize(const Vector &v)
{
	float len = v.Length();
	return (len == 0.0f) ? Vector(0, 0, 1) : v * (1.0f / len);
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

// ---- BlendAnimationsForSlide (@0x5d400, VERBATIM) — the prone AIM-TRACKING pose swapper.
// Runs every frame from Region B while the last activity is 3 (prone-idle). ONLY when the
// launch was non-forward (m_bDirForward): direction = GetDirection(m_vecStuntDir, v_angle)
// — the dive-vector-vs-CURRENT-AIM delta — so aiming around swaps the lying side
// continuously; the strafe-roll re-anchor clears m_bDirForward and the tracking stops
// ("until you move while prone"). !=forward is rejected (keeps the last side); the swap is
// a KEEPFRAME blend (0x194) and updates the m_iDirection latch.
void CTSAnimation::BlendAnimationsForSlide(CBasePlayer *pPlayer)
{
	if (!pPlayer->m_TSStunt.m_bDirForward)
		return;
	int d = GetDirection(pPlayer->m_TSStunt.m_vecStuntDir, pPlayer->pev->v_angle, 0.6f, 0.3f);
	if (d != m_iDirection && d != 2)
	{
		static const char *const slideTable[4] = { "slide_l_", "slide_r_", "slide_f_", "slide_b_" };
		char name[64];
		strcpy(name, slideTable[d & 3]);
		const char *grip = pPlayer->m_szAnimExtention;
		strcat(name, grip[0] ? grip : "akimbo");
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
	int d = GetDirection(TS_DirNormalize(pPlayer->pev->velocity), pPlayer->pev->v_angle, 0.6f, 0.3f);
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

// ---- Animate (@0x5b470) — the 4-phase dispatcher --------------------------------
void CTSAnimation::Animate(CBasePlayer *pPlayer, int activity)
{
	entvars_t *pev = pPlayer->pev;
	if (activity < 0)
		return;

	// Grip suffix (m_szAnimExtention) — EMPTY for bare-hands kung-fu (selects combat_idle
	// + the close-combat attack set), else onehanded/twohanded/akimbo for a held weapon.
	const char *grip = pPlayer->m_szAnimExtention;
	if (!grip)
		grip = "";
	const bool kungfu = (grip[0] == 0);

	const bool ducking = (pev->flags & FL_DUCKING) != 0;
	const bool onground = (pev->flags & FL_ONGROUND) != 0;
	const float speed = pev->velocity.Length2D();

	char name[64];

	// ---- Phase A: continue an in-flight directional blend --------------------
	if (pPlayer->IsAlive() && !(pev->flags & FL_FROZEN))
	{
		if (m_iLastActivity == TSA_JUMP && speed > 96.0f && !IsInCloseCombat(m_iLastActivity))
			BlendJumpAnimations(pPlayer);
		else if (activity == TSA_BLENDDIR)
		{
			// RE (Animate head, a1==2 caller): the mid-air dive re-blend feeds the STORED dive
			// vector (m_vecStuntDir = player+0x510) vs the CURRENT aim — aiming mid-flight swaps
			// the dive pose. Empty grip = no-op inside ByDir (no akimbo fallback here, verbatim).
			static const char *const diveTable[4] = { "dive_l_", "dive_r_", "dive_f_", "dive_b_" };
			BlendAnimationsByDir(pPlayer, pPlayer->m_TSStunt.m_vecStuntDir, pev->v_angle, diveTable, grip);
		}
	}

	// ---- Region B: hold the current (locked) pose while its stunt/jump/slide sequence is still
	// playing (RE Animate@0x5b470 switch(m_iLastActivity), reached BEFORE Region C so the per-frame
	// PostThink Animate(RUN/IDLE) — or a competing fire dispatch — cannot stomp an in-progress stunt).
	// Each case releases when: the sequence finishes (m_fSequenceFinished, now confirmed advanced
	// server-side via PostThink StudioFrameAdvance), OR the stunt bit clears (iuser4 & TSSTUNT_PRONE=0x4),
	// OR the incoming activity is a legitimate transition (the a1 exclusions). Every case ends in `break`
	// (the ORIGINAL does — omitting them was the freeze bug in the reverted 4-region rewrite).
	const bool seqPlaying = (pPlayer->m_fSequenceFinished == 0);
	const bool alive = pPlayer->IsAlive() != 0;
	const bool onladder = pPlayer->IsOnLadder() != 0;
	if (!IsInCloseCombat(m_iLastActivity))
	{
		switch (m_iLastActivity)
		{
		case TSA_SLIDE: // 0x3 — keep re-blending the slide direction
			if (alive && activity != TSA_JUMP && activity != TSA_MORFY &&
				activity != TSA_SLIDE_BL && activity != TSA_SLIDE_BR && activity != TSA_PRONE &&
				activity != TSA_FIRE && activity != TSA_FIRE_SLIDEL && activity != TSA_FIRE_SLIDER &&
				!onladder)
			{
				BlendAnimationsForSlide(pPlayer);
				return;
			}
			break;
		case TSA_JUMP: // 0x4
			if (seqPlaying && alive && !onground && !IsInCloseCombat(activity) &&
				activity != TSA_JUMP && activity != TSA_DIVE && activity != TSA_ROLL &&
				activity != TSA_DIVE_END && activity != TSA_WHEEL && activity != TSA_LOWWHEEL &&
				activity != TSA_WALLJUMP && activity != TSA_WALLCLIMB && activity != TSA_BULLETTIME)
				return;
			break;
		case TSA_DIE: // 0x7 — death holds (the die sequence was set once)
			return;
		case TSA_DIVE: // 0x28
			if (seqPlaying && alive && !onladder &&
				activity != TSA_PRONE && activity != TSA_GETUP && activity != TSA_SLIDE)
				return;
			break;
		case TSA_PRONE: // 0x29
		case TSA_GETUP: // 0x2e
		case TSA_ROLL:  // 0x2f
			if (seqPlaying && alive && (pev->iuser4 & 0x4 /*TSSTUNT_PRONE*/))
				return;
			break;
		case TSA_SLIDE_BL: // 0x2a
		case TSA_SLIDE_BR: // 0x2b
		{
			// hold the slide-back pose while stationary, or while still moving along the strafe axis
			bool hold = false;
			if (alive && !onladder && activity != TSA_SLIDE_BL && activity != TSA_SLIDE_BR &&
				activity != TSA_SLIDE && activity != TSA_PRONE)
			{
				Vector vel = pev->velocity;
				float spd = vel.Length();
				if (spd == 0.0f)
					hold = true;
				else
				{
					UTIL_MakeVectors(pev->v_angle);
					Vector vd = vel * (1.0f / spd);
					Vector r = gpGlobals->v_right;
					float rl = r.Length();
					if (rl != 0.0f) r = r * (1.0f / rl);
					if (DotProduct(vd, r) != 0.0f)
						hold = true;
				}
			}
			if (hold) return;
			break;
		}
		case TSA_DIVE_END: // 0x30
			if (seqPlaying && alive)
				return;
			break;
		case TSA_WHEEL: // 0x31
			if (seqPlaying && alive && (pev->iuser4 & 0x4 /*TSSTUNT_PRONE*/) && !onladder)
				return;
			break;
		case TSA_LOWWHEEL: // 0x32
			if (seqPlaying && alive && !onladder &&
				activity != TSA_PRONE && activity != TSA_GETUP)
				return;
			break;
		case TSA_BULLETTIME: // 0x36
			if (seqPlaying && alive)
				return;
			break;
		case TSA_WALLJUMP: // 0x37
			if (seqPlaying && alive && (pev->iuser4 & 0x4 /*TSSTUNT_PRONE*/) && !onladder && !onground &&
				activity != TSA_WALLJUMP && activity != TSA_WALLCLIMB && activity != TSA_DIVE)
				return;
			break;
		case TSA_WALLCLIMB: // 0x38
			if (seqPlaying && alive && (pev->iuser4 & 0x4 /*TSSTUNT_PRONE*/) && !onladder && !onground &&
				!IsInCloseCombat(activity) &&
				activity != TSA_WALLJUMP && activity != TSA_WALLCLIMB && activity != TSA_DIVE)
				return;
			break;
		case TSA_MORFY: // 0x39
			if (seqPlaying && alive && !onground && !IsInCloseCombat(activity) &&
				activity != TSA_MORFY && activity != TSA_DIVE && activity != TSA_ROLL &&
				activity != TSA_DIVE_END && activity != TSA_WHEEL && activity != TSA_LOWWHEEL &&
				activity != TSA_WALLJUMP && activity != TSA_WALLCLIMB && activity != TSA_BULLETTIME)
				return;
			break;
		}
	}

	// ---- Phase C: pick & set the NEW body sequence ---------------------------
	bool ccHandled = false;
	if (IsInCloseCombat(activity))
	{
		HandleCloseCombat(pPlayer, activity);
		ccHandled = true;
	}
	else switch (activity)
	{
	// ---- Fire (upper body). RE Animate@0x5b470 case 0/8/9 -------------------
	case TSA_FIRE:
	case TSA_FIRE_SLIDEL:
	case TSA_FIRE_SLIDER:
	{
		if (!onground)
			return; // airborne: no fire body pose
		int fflags;
		if ((pev->iuser4 & 0x20) == 0) // NOT sliding (TSSTUNT_SLIDE 0x20)
		{
			m_flAimTimer = 2.0f;
			strcpy(name, ducking ? "crouch_shoot_" : "shoot_");
			strcat(name, grip);
			// alt fire-stance "2" variant: recent stunt (leg timer) + standing + not scoped + one/two-handed
			if (m_flLegTimer > 0.001f && !ducking && pev->fov == 0 &&
				(strncmp(grip, "one", 3) == 0 || strncmp(grip, "two", 3) == 0))
			{
				strcat(name, "2");
				m_flAimTimer = 0.0f;
			}
			fflags = 0x92; // force+interp, keep legs
		}
		else // sliding: the forward slide-fire combined pose (player+0x51c dir path is dead code)
		{
			strcpy(name, "slide_f_");
			strcat(name, grip);
			strcat(name, "_shoot");
			fflags = 0x94; // force+interp, clear legs
		}
		// fire-while-sliding-back left/right variant (slide_b_ suffix)
		if (activity == TSA_FIRE_SLIDER)      strcat(name, "_right");
		else if (activity == TSA_FIRE_SLIDEL) strcat(name, "_left");
		SetAnim(pPlayer, name, fflags, 0);
		m_iLastActivity = activity;
		return;
	}

	// ---- Jump (full body) --------------------------------------------------
	case TSA_JUMP:
	{
		if (speed > 16.0f)
		{
			int dir = GetDirection(TS_DirNormalize(pev->velocity), pev->v_angle, 0.6f, 0.3f);
			m_iDirection = dir;
			const char *jd = (dir == 0) ? "jump_left_" : (dir == 1) ? "jump_right_" : "jump_forward_";
			strcpy(name, jd);
			strcat(name, grip);
			SetAnim(pPlayer, name, 0x94, 0);
		}
		else
		{
			SetAnim(pPlayer, "standard_jump", 0x94, 0);
		}
		m_iLastActivity = activity;
		return;
	}

	// ---- Duck transition ---------------------------------------------------
	case TSA_DUCK:
	{
		if (m_iDuckLatch == 0)
		{
			SetAnim(pPlayer, "goduck", 0x94, 0);
			m_iDuckLatch = 1;
		}
		m_iLastActivity = activity;
		return;
	}

	// ---- Death (full body) -------------------------------------------------
	case TSA_DIE:
	{
		if (m_bBulletDance)
		{
			static const char *const dies[3] = { "normal_die1", "normal_die2", "normal_die3" };
			SetAnim(pPlayer, dies[RANDOM_LONG(0, 2)], 0x14, 0);
		}
		else
		{
			// Directional death needs the attack direction (deferred) — use the spin death.
			SetAnim(pPlayer, "die_spin", 0x14, 0);
		}
		m_iLastActivity = activity;
		return;
	}

	// ---- Grenade throw (upper body, keep legs). RE case 0x35 ---------------
	case TSA_GRENADE:
		if (!onground)
			return;
		if (pev->iuser4 & 0x30) // TSSTUNT_SLIDE(0x20)|TSSTUNT_DIVE(0x10): no throw pose mid-stunt
			return;
		SetAnim(pPlayer, "grenade", 0x92, 0);
		m_flAimTimer = 2.0f;
		m_flGrenadeFlag = 1.0f;
		m_iLastActivity = activity;
		return;

	// ---- Reload / draw (upper body, keep legs) -----------------------------
	case TSA_RELOAD:
	case TSA_DRAW:
		if (!onground)
			break;
		strcpy(name, (activity == TSA_RELOAD) ? "relo_" : "draw_");
		strcat(name, grip);
		SetAnim(pPlayer, name, 0x92, 0);
		m_flAimTimer = 2.0f;
		m_iLastActivity = activity;
		return;

	// ---- STUNT poses (CTSStunt drives these) -------------------------------
	case TSA_DIVE:
	{
		static const char *const ds[4] = { "l", "r", "f", "b" };
		int dir = GetDirection(TS_DirNormalize(pev->velocity), pev->v_angle, 0.6f, 0.3f);
		m_iDirection = dir; // freeze the dive direction — the following prone pose (0x29) indexes by it
		snprintf(name, sizeof(name), "dive_%s_%s", ds[dir & 3], grip[0] ? grip : "akimbo"); // grip fallback per LAB_0006c776
		SetAnim(pPlayer, name, 0x94, 0);
		m_flLegTimer = 3.0f;
		m_iLastActivity = activity;
		return;
	}
	// RE case 0x2e/0x2f/0x30 (CORRECTED cont.81): direction = GetDirection(player+0x510, v_angle,
	// 0.6, 0.3). player+0x510 IS m_vecStuntDir — CTSStunt is EMBEDDED at CBasePlayer+0x4f0 and
	// m_vecStuntDir is CTSStunt+0x20 (0x4f0+0x20=0x510); GoRoll/GoWheel/GoDive write it through the
	// stunt this-pointer, which is why every player-relative write-scan called it "never written".
	// So these poses ARE directional: the direction the stunt was LAUNCHED in, relative to view
	// (roll_b for a backward dive-roll, etc.). Pose tables from the ctor: +0x11c getup
	// {dive_l/r/f/b_end}, +0x15c roll {roll_l/r/f/b}, +0x19c dive_end {roll_l, roll_r,
	// roll_f_high, roll_b} (the forward slot alone gets the _high variant).
	case TSA_GETUP:
	{
		static const char *const getupTable[4] = { "dive_l_end", "dive_r_end", "dive_f_end", "dive_b_end" };
		int dir = GetDirection(pPlayer->m_TSStunt.m_vecStuntDir, pev->v_angle, 0.6f, 0.3f);
		SetAnim(pPlayer, getupTable[dir & 3], 0x94, 0);
		m_flLegTimer = 3.0f;
		m_iLastActivity = activity;
		return;
	}
	case TSA_ROLL:
	{
		static const char *const rollTable[4] = { "roll_l", "roll_r", "roll_f", "roll_b" };
		int dir = GetDirection(pPlayer->m_TSStunt.m_vecStuntDir, pev->v_angle, 0.6f, 0.3f);
		SetAnim(pPlayer, rollTable[dir & 3], 0x94, 0);
		m_flLegTimer = 3.0f;
		m_iLastActivity = activity;
		return;
	}
	case TSA_DIVE_END:
	{
		static const char *const diveEndTable[4] = { "roll_l", "roll_r", "roll_f_high", "roll_b" };
		int dir = GetDirection(pPlayer->m_TSStunt.m_vecStuntDir, pev->v_angle, 0.6f, 0.3f);
		SetAnim(pPlayer, diveEndTable[dir & 3], 0x94, 0);
		m_iLastActivity = activity;
		return;
	}
	// case 0x29: the settled prone LYING pose, indexed by the FROZEN m_iDirection. IMPORTANT: the ctor
	// pose table this+0xdc = slide_<dir>_end, but those are the dive-RECOVERY (getup) animations — in the
	// original, Animate(0x29)=slide_<dir>_end is dispatched ONCE for the deliberate standup, while the
	// every-frame auto-slide uses the slide_<dir>_<grip> LYING pose. Our HandleSlide dispatches 0x29 every
	// frame with FORCE, so slide_<dir>_end restarted the getup continuously ("constantly getting up"); the
	// static lying pose here restarts invisibly. (Backward-feet: dir 3 -> slide_b_<grip> lying-back for now;
	// the on-feet backward landing needs the auto-slide-vs-deliberate split — VERIFY / follow-up.)
	// RE case 0x29 (VERBATIM): the prone pose = pronetable[m_iDirection] — the ctor table
	// @this+0xdc = {slide_l/r/f/b_end}, indexed by the LATCHED direction (m_iDirection*0x10+0xdc,
	// then the shared SetAnim(...,0x94,0) tail). NO grip suffix — the cont.80 slide_<dir>_<grip>
	// substitution broke to IDLE once the direction latch started carrying 0/1/3 (missing
	// grip-variant sequences); the original's table names always exist.
	case TSA_PRONE:
	{
		static const char *const proneTable[4] = { "slide_l_end", "slide_r_end", "slide_f_end", "slide_b_end" };
		SetAnim(pPlayer, proneTable[m_iDirection & 3], 0x94, 0);
		m_flLegTimer = 3.0f;
		m_iLastActivity = activity;
		return;
	}
	// RE case 0x2a/0x2b: "slide_f_" + grip + "_left"/"_right" (the player+0x51c stored-dir path is
	// dead code — 0x51c is never written, so it is always this forward slide-back-fire form).
	// RE case 0x2a/0x2b (VERBATIM): the prone ROLL. Forward launches (m_bDirForward false):
	// "slide_f_" + grip(/akimbo) + "_left"/"_right" (the LOOPING roll, agent.mdl 123/124 etc.).
	// Non-forward: the bare "slide_b_left"/"slide_b_right" — NOT present in agent.mdl (an
	// original-content quirk; the lookup falls through exactly as the original's would).
	case TSA_SLIDE_BL:
	case TSA_SLIDE_BR:
		if (!pPlayer->m_TSStunt.m_bDirForward)
		{
			strcpy(name, "slide_f_");
			strcat(name, grip[0] ? grip : "akimbo");
			strcat(name, (activity == TSA_SLIDE_BL) ? "_left" : "_right");
		}
		else
			strcpy(name, (activity == TSA_SLIDE_BL) ? "slide_b_left" : "slide_b_right");
		SetAnim(pPlayer, name, 0x14, 0);
		m_flLegTimer = 3.0f;
		m_iLastActivity = activity;
		return;
	// RE case 0x31/0x32 (CORRECTED cont.81, same +0x510 = m_vecStuntDir resolution as above):
	// the wheels ARE directional off the stunt-launch direction. 0x31: switch 0->wheel_l,
	// 1->wheel_r, 3->wheel_b (the standing wheel: GoWheel stores -v_forward -> backward ->
	// wheel_b), default->wheel_f. 0x32: 2->lowwheel_f, 0->lowwheel_l, else->lowwheel_r
	// (there is NO lowwheel_b — dir 3 falls to lowwheel_r, per the original's exact branch shape).
	case TSA_WHEEL:
	{
		int dir = GetDirection(pPlayer->m_TSStunt.m_vecStuntDir, pev->v_angle, 0.6f, 0.3f);
		const char *pose = (dir == 0) ? "wheel_l" : (dir == 1) ? "wheel_r" : (dir == 3) ? "wheel_b" : "wheel_f";
		SetAnim(pPlayer, pose, 0x94, 0);
		m_flLegTimer = 3.0f; // this+0x8 = 3.0 (0x40400000)
		m_iLastActivity = activity;
		return;
	}
	case TSA_LOWWHEEL:
	{
		int dir = GetDirection(pPlayer->m_TSStunt.m_vecStuntDir, pev->v_angle, 0.6f, 0.3f);
		const char *pose = (dir == 2) ? "lowwheel_f" : (dir == 0) ? "lowwheel_l" : "lowwheel_r";
		SetAnim(pPlayer, pose, 0x94, 0);
		m_flLegTimer = 3.0f;
		m_iLastActivity = activity;
		return;
	}
	case TSA_WALLJUMP:
		SetAnim(pPlayer, "walljump_end", 0x94, 0);
		m_flLegTimer = 3.0f;
		m_iLastActivity = activity;
		return;
	case TSA_WALLCLIMB:
		SetAnim(pPlayer, "walljump_climb", 0x94, 0);
		m_flLegTimer = 3.0f;
		m_iLastActivity = activity;
		return;
	case TSA_BULLETTIME:
		SetAnim(pPlayer, "bullettime", 0x94, 0);
		m_iLastActivity = activity;
		return;
	case TSA_MORFY:
		SetAnim(pPlayer, grip[0] ? "morfy_jump_guns" : "morfy_jump", 0x94, 0);
		m_iLastActivity = activity;
		return;

	// RE case 3 (VERBATIM): the settled PRONE LYING pose — the LOOPING slide_<dir>_<grip>
	// (agent.mdl 111-114/125-128/135-138). Direction: non-directional launches
	// (m_bDirForward false) lie forward; else GetDirection(m_vecStuntDir) with the
	// !=forward latch update into m_iDirection. Grip falls back to "akimbo". Entry guard:
	// don't stomp a still-playing fire pose (last 0/8/9 = FIRE/FIRE_SLIDEL/R).
	case TSA_SLIDE:
	{
		if ((m_iLastActivity != TSA_FIRE && m_iLastActivity != TSA_FIRE_SLIDEL &&
			 m_iLastActivity != TSA_FIRE_SLIDER) || pPlayer->m_fSequenceFinished)
		{
			if (!pPlayer->m_TSStunt.m_bDirForward)
				strcpy(name, "slide_f_");
			else
			{
				int dir = GetDirection(pPlayer->m_TSStunt.m_vecStuntDir, pev->v_angle, 0.6f, 0.3f);
				if (dir != m_iDirection && dir != 2)
					m_iDirection = dir;
				static const char *const slideTable[4] = { "slide_l_", "slide_r_", "slide_f_", "slide_b_" };
				strcpy(name, slideTable[m_iDirection & 3]);
			}
			strcat(name, grip[0] ? grip : "akimbo");
			SetAnim(pPlayer, name, 0x94, 0);
			m_iLastActivity = activity;
		}
		return;
	}

	// ---- Move / idle (handled by Phase D) ----------------------------------
	case TSA_RUN:
	case TSA_IDLE:
	default:
		break;
	}

	// ---- Phase D: record + legs/gait + idle/aim torso ------------------------
	if (!ccHandled)
		m_iLastActivity = activity;

	if (!onground)
		return; // airborne non-jump: keep whatever's playing

	m_iDuckLatch = 0; // reset goduck latch once grounded

	// Legs: crawl when ducking+moving, run2/walk2 by speed, none when still.
	if (speed > 1.0f)
	{
		m_iDirection = GetDirection(TS_DirNormalize(pev->velocity), pev->v_angle, 0.6f, 0.3f);
		if (ducking)
			SetAnim(pPlayer, "crawl", TSSA_LEGS, 0);
		else
			SetAnim(pPlayer, (speed > TSA_RUN_SPEED) ? "run2" : "walk2", TSSA_LEGS, 0);
	}
	else
	{
		SetAnim(pPlayer, "", TSSA_LEGS, 0); // clear legs
	}

	// ---- The per-frame STATE-DRIVEN torso pose (RE Animate@0x5b470 case 1/6). Everything below reads
	// PLAYER STATE this frame to choose the body sequence — there is no activity argument here; the weapon's
	// fire/reload/deploy calls only SET state (m_szAnimOverride, m_flAimTimer, m_iLastActivity), and THIS
	// reads it every frame. State consulted, in the original's order: m_szAnimOverride(player+0x478) >
	// "weapon pose still playing" (m_flAimTimer; the orig uses m_fSequenceFinished which isn't advanced for
	// the server-side player, so we gate on the timer) > grip-empty(kungfu) > aim/idle by (m_flAimTimer||fov).
	if (ccHandled)
		return;

	// (1) player+0x478 anim-OVERRIDE — a dedicated melee idle (knife_idle/katana_idle/briefcase_idle) the
	// weapon set on deploy. Non-empty => play it directly as the body. (knife/katana dedicated 3rd-person idle.)
	if (pPlayer->m_szAnimOverride[0] != 0)
	{
		SetAnim(pPlayer, pPlayer->m_szAnimOverride, 0x12, 0);
		return;
	}

	// (2) a weapon pose (shoot_/relo_/draw_, set when the weapon last called Animate) is still "raised":
	// hold it. The original gates this on the sequence not being finished (m_fSequenceFinished); that field
	// isn't advanced for the server-side player, so we read the m_flAimTimer state the fire/reload set.
	if (m_flAimTimer >= 0.01f)
		return;

	// (3) bare-hands kung-fu (empty grip) => the combat_idle stance (state: grip).
	if (kungfu)
	{
		SetAnim(pPlayer, ducking ? "combat_idle_low" : "combat_idle", 0x12, 0);
		return;
	}

	// (4) aim_ vs idle_ — read per-frame from state (m_flAimTimer recently-raised OR fov zoomed), per the
	// decompile's `(m_flAimTimer>=0.01 || fov!=0)` (NOT the attack button). With (2) holding the timer window,
	// this resolves the fov (scoped) case; both are state, not an activity arg.
	const bool aiming = (m_flAimTimer >= 0.01f || pev->fov != 0);
	if (ducking)
		strcpy(name, aiming ? "crouch_aim_" : "duck_idle_");
	else
		strcpy(name, aiming ? "aim_" : "idle_");
	strcat(name, grip);
	SetAnim(pPlayer, name, 0x12, 0);
}
