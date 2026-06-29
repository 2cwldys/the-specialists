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
			static const char *const diveTable[4] = { "dive_l_", "dive_r_", "dive_f_", "dive_b_" };
			BlendAnimationsByDir(pPlayer, pev->velocity, pev->v_angle, diveTable, grip[0] ? grip : "akimbo");
		}
	}

	// ---- Phase B: keep the current (locked) pose if it is still playing -------
	if (m_iLastActivity == TSA_SLIDE)
	{
		BlendAnimationsForSlide(pPlayer);
		// slide keeps blending; fall through to record only
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
	// ---- Fire (upper body, keep legs) --------------------------------------
	case TSA_FIRE:
	case TSA_FIRE_SLIDEL:
	case TSA_FIRE_SLIDER:
	{
		if (ducking)
		{
			strcpy(name, "crouch_shoot_");
			strcat(name, grip);
		}
		else
		{
			strcpy(name, "shoot_");
			if (strncmp(grip, "akimbo", 6) == 0)
				strcpy(name, "shoot_akimbo_left"); // akimbo alternates barrels; default left
			else
				strcat(name, grip);
		}
		SetAnim(pPlayer, name, 0x92, 0); // force+interp, keep legs
		m_flAimTimer = 2.0f;
		m_iLastActivity = activity;
		return;
	}

	// ---- Jump (full body) --------------------------------------------------
	case TSA_JUMP:
	{
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

	// ---- Grenade throw (upper body, keep legs) -----------------------------
	case TSA_GRENADE:
		if (!onground)
			break;
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
		int dir = GetDirection(pev->velocity, pev->v_angle, 0.6f, 0.3f);
		snprintf(name, sizeof(name), "dive_%s_%s", ds[dir & 3], grip);
		SetAnim(pPlayer, name, 0x94, 0);
		m_flLegTimer = 3.0f;
		m_iLastActivity = activity;
		return;
	}
	case TSA_GETUP:
	case TSA_ROLL:
	{
		static const char *const ds[4] = { "l", "r", "f", "b" };
		int dir = GetDirection(pev->velocity, pev->v_angle, 0.6f, 0.3f);
		snprintf(name, sizeof(name), "dive_%s_end", ds[dir & 3]);
		SetAnim(pPlayer, name, 0x94, 0);
		m_flLegTimer = 3.0f;
		m_iLastActivity = activity;
		return;
	}
	case TSA_PRONE:
	case TSA_SLIDE_BL:
	case TSA_SLIDE_BR:
	{
		int dir = GetDirection(pev->velocity, pev->v_angle, 0.6f, 0.3f);
		if (activity == TSA_SLIDE_BL || dir == 3)
			strcpy(name, "slide_b_left");
		else if (activity == TSA_SLIDE_BR)
			strcpy(name, "slide_b_right");
		else
		{
			static const char *const ds[4] = { "l", "r", "f", "b" };
			snprintf(name, sizeof(name), "slide_%s_%s", ds[dir & 3], grip);
		}
		SetAnim(pPlayer, name, 0x14, 0);
		m_flLegTimer = 3.0f;
		m_iLastActivity = activity;
		return;
	}
	case TSA_DIVE_END:
	{
		static const char *const ds[4] = { "l", "r", "f", "b" };
		int dir = GetDirection(pev->velocity, pev->v_angle, 0.6f, 0.3f);
		snprintf(name, sizeof(name), "dive_%s_end", ds[dir & 3]);
		SetAnim(pPlayer, name, 0x94, 0);
		m_iLastActivity = activity;
		return;
	}
	case TSA_WHEEL:
	case TSA_LOWWHEEL:
	{
		static const char *const ds[4] = { "l", "r", "f", "b" };
		int dir = GetDirection(pev->velocity, pev->v_angle, 0.6f, 0.3f);
		snprintf(name, sizeof(name), "%swheel_%s", (activity == TSA_LOWWHEEL) ? "low" : "", ds[dir & 3]);
		SetAnim(pPlayer, name, 0x94, 0);
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

	// ---- Move / idle (handled by Phase D) ----------------------------------
	case TSA_RUN:
	case TSA_IDLE:
	case TSA_SLIDE:
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
		m_iDirection = GetDirection(pev->velocity, pev->v_angle, 0.6f, 0.3f);
		if (ducking)
			SetAnim(pPlayer, "crawl", TSSA_LEGS, 0);
		else
			SetAnim(pPlayer, (speed > TSA_RUN_SPEED) ? "run2" : "walk2", TSSA_LEGS, 0);
	}
	else
	{
		SetAnim(pPlayer, "", TSSA_LEGS, 0); // clear legs
	}

	// Torso: close-combat already set its pose; otherwise don't stomp an in-progress
	// fire/reload pose (aim timer running), then select the idle/aim/combat stance.
	if (ccHandled)
		return;

	// player+0x478 anim-OVERRIDE (RE Animate@0x5b470 case 1/6, checked FIRST): a dedicated melee idle
	// (knife_idle/katana_idle/briefcase_idle) the weapon set on deploy. Non-empty -> play it directly as
	// the body (instead of idle_/aim_<grip>), flags 0x12. This is the knife/katana dedicated 3rd-person idle.
	if (pPlayer->m_szAnimOverride[0] != 0)
	{
		SetAnim(pPlayer, pPlayer->m_szAnimOverride, 0x12, 0);
		return;
	}

	if (m_flAimTimer >= 0.01f)
		return; // weapon raised recently — leave the shoot/relo/aim torso playing

	if (kungfu)
	{
		// Bare-hands fighting stance (not an aim_<grip> invisible-pistol pose).
		SetAnim(pPlayer, ducking ? "combat_idle_low" : "combat_idle", 0x12, 0);
		return;
	}

	// RE Animate@0x5b470 case 1/6: aim_ when (m_flAimTimer>=0.01 || fov!=0), NOT the attack button. The
	// m_flAimTimer>=0.01 case is intercepted by the hold above (we keep the shoot_/relo_ pose); here that
	// leaves the zoom (fov) case — a scoped player shows the aim pose. (Original ALSO consults a per-frame
	// player anim-override string player+0x478 — empty>idle_+grip, non-empty>play it directly; we don't model
	// that member yet — it's what drives the knife/katana idle + the melee block. TODO: add m_szAnimOverride.)
	const bool aiming = (pev->fov != 0);
	if (ducking)
		strcpy(name, aiming ? "crouch_aim_" : "duck_idle_");
	else
		strcpy(name, aiming ? "aim_" : "idle_");
	strcat(name, grip);
	SetAnim(pPlayer, name, 0x12, 0);
}
