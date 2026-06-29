/***
*  ts_animation.h — RECONSTRUCTED CTSAnimation (the TS 3rd-person gait/body
*  animation subsystem). Functional/source-faithful (see CLAUDE.md).
*
*  TS embeds a CTSAnimation object on CBasePlayer (original: player+0x28c) and
*  drives ALL player body animation through CTSAnimation::Animate(player,activity)
*  — the stock CBasePlayer::SetAnimation(PLAYER_ANIM) is a no-op stub in the .so
*  (@0xd8f50, 21 bytes). The stock SDK SetAnimation looks up "ref_aim_<ext>" /
*  ACT_* sequences that the TS player.mdl does NOT define, so the model rendered
*  permanently idle. This reconstruction selects the real TS model sequences
*  (idle_/aim_/shoot_/relo_/draw_ + grip, crouch_/duck_ variants, run2/walk2/crawl
*  legs, standard_jump, die_*) exactly as the original Animate (@0x6abd0) does.
*
*  STRUCTURE: this file mirrors the original's METHOD layout 1:1 (re.sh CTSAnimation,
*  13 methods @0x5b320..0x5dd14). The original has NO m_bKungFu / GetMoveDirection /
*  SetBodySequence / SetGaitSequence — those were our collapsed helpers. Restored to:
*    Init, UpdateTimers, Animate, EnableBulletDance, DisableBulletDance, BulletDance,
*    BlendAnimationsForSlide, BlendAnimationsByDir, BlendJumpAnimations, SetAnim,
*    HandleCloseCombat, IsInCloseCombat, GaitCloseCombat   (+ the free GetDirection).
*  Members are unnamed in the binary (all `field_0xNN`, save-source `auto`) so member
*  NAME parity is unrecoverable; we keep readable names matched to the recovered
*  offset semantics (provenance in the comments).
*
*  The grip suffix is the player's m_szAnimExtention (onehanded/twohanded/akimbo, or
*  EMPTY for bare-hands kung-fu) — the original's player+0x2d6. Empty grip selects the
*  combat_idle fighting stance and the close-combat (HandleCloseCombat) attack set.
*
*  Contract (verified against the model + cl_dll StudioProcessGait):
*    pev->sequence     = torso/full-body action sequence
*    pev->gaitsequence = leg locomotion sequence (client blends it under sequence)
***/
#ifndef TS_ANIMATION_H
#define TS_ANIMATION_H

#include "vector.h"

class CBasePlayer;

// TS animation activity codes (the `param_3` of the original Animate). The locomotion
// codes drive idle/run/jump/duck/fire/reload/draw/grenade; the close-combat codes
// (0xa..0x27, 0x3a) feed HandleCloseCombat; the stunt codes (0x28..0x39) come from
// CTSStunt. Raw values match the original exactly.
enum TSAnimActivity
{
	TSA_FIRE = 0,         // primary attack -> shoot_<grip>
	TSA_RUN = 1,          // moving (has horizontal velocity)
	TSA_BLENDDIR = 2,     // directional move-blend in progress (HandleDive entry / continue-blend)
	TSA_SLIDE = 3,        // slide (stunt-adjacent; continuous slide-dir blend)
	TSA_JUMP = 4,         // jump -> standard_jump / jump_<dir>_<grip>
	TSA_DUCK = 5,         // duck transition -> goduck
	TSA_IDLE = 6,         // standing still
	TSA_DIE = 7,          // death -> die_* / normal_die*
	TSA_FIRE_SLIDEL = 8,  // fire while sliding back-left
	TSA_FIRE_SLIDER = 9,  // fire while sliding back-right

	// ---- close-combat (HandleCloseCombat 0xa..0x27, 0x3a) ----
	TSA_CC_CIRCFRONT = 0x0a, TSA_CC_CIRCKICK = 0x0b, TSA_CC_CIRCKICKLOW = 0x0c,
	TSA_CC_CIRCBACK = 0x0d,  TSA_CC_LATBACK = 0x0e,  TSA_CC_CIRCSPIN = 0x0f,
	TSA_CC_SIDEKICK = 0x10,  TSA_CC_BACKKICK = 0x11, TSA_CC_AIRKICK = 0x12,
	TSA_CC_AIRKICK2 = 0x13,  TSA_CC_DUCKKICK = 0x14, TSA_CC_PUNCHLEFT = 0x16,
	TSA_CC_PUNCHRIGHT = 0x17, TSA_CC_PUNCHAIR = 0x18, TSA_CC_UPPERCUT = 0x19,
	TSA_CC_KNIFE1 = 0x1d, TSA_CC_KNIFE2 = 0x1e, TSA_CC_KNIFE3 = 0x1f,
	TSA_CC_KNIFELAUNCH = 0x20, TSA_CC_KATANA1 = 0x21, TSA_CC_KATANA2 = 0x22,
	TSA_CC_KATANA3 = 0x23, TSA_CC_KATANAAIR = 0x24, TSA_CC_KATANADUCK = 0x25,
	TSA_CC_BLOCK = 0x26, TSA_CC_STOCK = 0x27, TSA_CC_SUPERJUMP_LAND = 0x3a,

	// ---- stunt poses (driven by CTSStunt) ----
	TSA_DIVE = 0x28, TSA_PRONE = 0x29, TSA_SLIDE_BL = 0x2a, TSA_SLIDE_BR = 0x2b,
	TSA_GETUP = 0x2e, TSA_ROLL = 0x2f, TSA_DIVE_END = 0x30,
	TSA_WHEEL = 0x31, TSA_LOWWHEEL = 0x32,
	TSA_BULLETTIME = 0x36, TSA_WALLJUMP = 0x37, TSA_WALLCLIMB = 0x38, TSA_MORFY = 0x39,

	TSA_RELOAD = 0x33,    // reload -> relo_<grip>
	TSA_DRAW = 0x34,      // deploy -> draw_<grip>
	TSA_GRENADE = 0x35,   // grenade throw -> grenade
};

// SetAnim flag bits (recovered from SetAnim @0x5d7e4). Combine as needed.
enum TSSetAnimFlags
{
	TSSA_LEGS      = 0x01, // set this as the GAIT (leg) sequence, return
	TSSA_CLEARLEGS = 0x04, // clear gaitsequence, then set as body
	TSSA_INTERP    = 0x10, // allow interpolation (CLEAR this bit => force EF_NOINTERP)
	TSSA_SETFRAME  = 0x20, // set pev->frame = frameArg
	TSSA_FORCE     = 0x80, // apply even if the sequence is unchanged
	TSSA_KEEPFRAME = 0x100,// do NOT reset pev->frame to 0
};

// 0=left 1=right 2=forward 3=back 4=back(alt), from a normalized direction relative to
// facing. The original is a FREE function GetDirection__FG6VectorT0ff (not a method).
int GetDirection(const Vector &dir, const Vector &angles, float fwdBias, float sideBias);

// Embedded on CBasePlayer. Members mirror the original's intent (the recovered
// offsets are provenance only — the binary's fields are unnamed; written naturally).
class CTSAnimation
{
public:
	void Init();                                       // reset per-spawn (@0x5b320)
	void UpdateTimers(CBasePlayer *pPlayer);           // decay timers each frame (@0x5b37c)
	void Animate(CBasePlayer *pPlayer, int activity);  // the dispatcher (@0x5b470)

	// Bullet-time "death twitch": while in bullet-time, taking a hit plays a brief
	// normal_die* flinch (@0x5d2f0/0x5d314/0x5d338). Enabled/disabled by the slow-mo path.
	void EnableBulletDance(CBasePlayer *pPlayer);
	void DisableBulletDance(CBasePlayer *pPlayer);
	void BulletDance(CBasePlayer *pPlayer);

	// True for the close-combat activity codes (0xa..0x27, 0x3a) (@0x5dcf4).
	bool IsInCloseCombat(int activity);

	float m_flLegTimer = 0;    // +0x8  signed leg/stunt hold (slide/wheel/walljump/close-combat = 3.0)
	float m_flAimTimer = 0;    // +0xc  upper-body hold after fire/reload/grenade/close-combat (2.0)
	float m_flGrenadeFlag = 0; // +0x10 grenade-throw timer (1.0)

private:
	// The real sequence setter (@0x5d7e4) — flags per TSSetAnimFlags. Returns 1 if it changed.
	int SetAnim(CBasePlayer *pPlayer, const char *name, int flags, int frame);

	// Continuous directional re-blend while a move is in flight (called from Animate top).
	void BlendAnimationsByDir(CBasePlayer *pPlayer, const Vector &dir, const Vector &angles,
		const char *const dirTable[4], const char *grip);  // (@0x5d538)
	void BlendAnimationsForSlide(CBasePlayer *pPlayer);    // (@0x5d400)
	void BlendJumpAnimations(CBasePlayer *pPlayer);        // (@0x5d630)

	// Close-combat (kung-fu/knife/katana) body-pose dispatch by activity code (@0x5d8d0).
	void HandleCloseCombat(CBasePlayer *pPlayer, int activity);
	// 1 if the close-combat move should keep the legs gaiting (moving/ducking) (@0x5dd14).
	int GaitCloseCombat(int activity, CBasePlayer *pPlayer);

	int m_iLastActivity = -1;  // +0x0  PREVIOUS activity code (read first each frame; drives Phase A/B)
	int m_iDuckLatch = 0;      // +0x4  goduck transition fired once
	int m_iDirection = -1;     // +0x14 last move direction (0 L,1 R,2 F,3 B); Init=-1
	int m_iEnabled = 1;        // +0x18 Init=1 (first-frame/enabled flag)
	// Bullet-dance state (+0x1dc/0x1e0/0x1e4).
	float m_flBulletDanceRetrig = 0; // +0x1dc retrigger timer (0.15)
	float m_flBulletDanceWindow = 0; // +0x1e0 active window timer (3.0)
	int   m_bBulletDance = 0;        // +0x1e4 enabled flag
	bool  m_bCCStarted = false;      // +0x1e8 close-combat just-started flag (gait gate)
};

#endif // TS_ANIMATION_H
