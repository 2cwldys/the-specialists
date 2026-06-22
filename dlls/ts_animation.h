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
*  The grip suffix is the player's m_szAnimExtention (onehanded/twohanded/akimbo),
*  set by CTSGun::Deploy — the same field the stock SDK uses, which is the
*  original's player+0x2d6.
*
*  Contract (verified against the model + cl_dll StudioProcessGait):
*    pev->sequence     = torso/full-body action sequence
*    pev->gaitsequence = leg locomotion sequence (client blends it under sequence)
*
*  NOT reconstructed (CTSStunt-gated — CTSStunt is not ported, so these activity
*  codes never fire): dives (0x28-0x2b,0x2e-0x30), cartwheels/walljump (0x31,0x32,
*  0x37,0x38), morfy-jump (0x39), rolls/bullet-dance (0x36), close-combat blends.
***/
#ifndef TS_ANIMATION_H
#define TS_ANIMATION_H

class CBasePlayer;

// TS animation activity codes (the `param_3` of the original Animate). Only the
// core gait codes that actually fire from ported code are handled; the stunt
// codes (0x28-0x32, 0x36-0x39) are recognised but deferred (see header note).
enum TSAnimActivity
{
	TSA_FIRE = 0,         // primary attack -> shoot_<grip>
	TSA_RUN = 1,          // moving (has horizontal velocity)
	TSA_SLIDE = 3,        // slide (stunt-adjacent; treated as move)
	TSA_JUMP = 4,         // jump -> standard_jump / jump_<dir>_<grip>
	TSA_DUCK = 5,         // duck transition -> goduck
	TSA_IDLE = 6,         // standing still
	TSA_DIE = 7,          // death -> die_* / normal_die*
	TSA_FIRE_SLIDEL = 8,  // fire while sliding back-left
	TSA_FIRE_SLIDER = 9,  // fire while sliding back-right
	TSA_RELOAD = 0x33,    // reload -> relo_<grip>
	TSA_DRAW = 0x34,      // deploy -> draw_<grip>
	TSA_GRENADE = 0x35,   // grenade throw -> grenade
};

// Embedded on CBasePlayer. Members mirror the original's intent (the recovered
// offsets are provenance only — see CTSAnimation.c decompile); written naturally.
class CTSAnimation
{
public:
	void Init();                                  // reset per-spawn (@0x67ae0)
	void Animate(CBasePlayer *pPlayer, int activity); // the dispatcher (@0x6abd0)
	void UpdateTimers(CBasePlayer *pPlayer);      // decay timers each frame (@0x6b37c)

	int m_iLastActivity = -1;  // last activity code dispatched
	int m_iDirection = -1;     // last 8-way movement direction (0 L,1 R,2 F,3 B)
	int m_iDuckLatch = 0;      // goduck transition fired once
	float m_flLegTimer = 0;    // one-shot leg-anim hold (wheels/walljump — deferred)
	float m_flAimTimer = 0;    // weapon-raised hold after firing/reload (~2s)
	float m_flGrenadeFlag = 0; // grenade throw in progress

private:
	// 0=left 1=right 2=forward 3=back, from velocity relative to facing.
	int GetMoveDirection(CBasePlayer *pPlayer);
	// Set pev->sequence (torso). Returns true if it changed. restart resets frame.
	bool SetBodySequence(CBasePlayer *pPlayer, const char *name, bool restart, bool noLerp);
	// Set pev->gaitsequence (legs) by name ("" / NULL clears to 0).
	void SetGaitSequence(CBasePlayer *pPlayer, const char *name);
};

#endif // TS_ANIMATION_H
