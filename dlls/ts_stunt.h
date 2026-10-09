/***
*  ts_stunt.h — RECONSTRUCTED CTSStunt (The Specialists acrobatics / stunt system).
*  Functional/source-faithful reconstruction (see CLAUDE.md + reference/STUNT.md).
*
*  CTSStunt is embedded on CBasePlayer (original: TS_stuntmanager.cpp). Play() runs
*  every PreThink; it decrements the stunt timers and, when grounded, dispatches the
*  Handle/Go state machine off the player's button state. Every stunt writes its
*  state into the NETWORKED `pev->iuser4` bit field (see TSSTUNT_* below); the engine
*  copies pev->iuser4 -> pmove->iuser4 (server) and networks it to the client's
*  prediction (clientdata -> entity_state -> pmove), so pm_shared's TSHandle() can
*  PREDICT the dive/prone small hull + low view + launch velocity on BOTH sides.
*  This is why the old DLL-only hull/view forcing never stuck — prediction owns it.
***/
#ifndef TS_STUNT_H
#define TS_STUNT_H

class CBasePlayer;

// pev->iuser4 stunt flag bits (the networked swap contract — see reference/STUNT.md §1).
#define TSSTUNT_STOP        0x000001 // damp velocity x0.5 (pm TSHandle)
#define TSSTUNT_PRONE       0x000004 // use the ducking hull (pm TSGoDuck)
#define TSSTUNT_STANDUP     0x000008 // leave the ducking hull (pm TSStandUp)
#define TSSTUNT_DIVE        0x000010 // dive in progress (Play -> HandleDive)
#define TSSTUNT_SLIDE       0x000020 // slide in progress (Play -> HandleSlide; pm damp)
#define TSSTUNT_DIVE_HIGH   0x000040 // dive height tier: high   (pm TSDive 90 / z283)
#define TSSTUNT_DIVE_LOW    0x000080 // dive height tier: low    (pm TSDive 100 / z126)
#define TSSTUNT_DIVE_MED    0x000100 // dive height tier: medium (pm TSDive 90 / z200)
#define TSSTUNT_WHEEL_HI    0x000400 // cartwheel high
#define TSSTUNT_WHEEL_LO    0x000800 // cartwheel low (0xc00 mask -> HandleWheel)
#define TSSTUNT_WALLJUMP    0x001000 // wall-jump launch  (0x7000 mask -> pm TSWallJump)
#define TSSTUNT_WALLJUMP_M  0x007000 // any wall-jump launch bit
#define TSSTUNT_WALLCLIMB   0x008000 // wall-climb launch (sign bit -> pm TSWallJumpClimb)
#define TSSTUNT_WALLJUMP_A  0x010000 // wall-jump active   (Play -> HandleWallJump)
#define TSSTUNT_WALLCLIMB_A 0x020000 // wall-climb active  (Play -> HandleWallClimb)
#define TSSTUNT_ROLL        0x040000 // landing roll
#define TSSTUNT_WHEEL_DONE  0x100000 // wheel finished
#define TSSTUNT_WHEEL_TRIG  0x200000 // wheel trigger (HandleStunt wall-dive -> wheel)
#define TSSTUNT_NOVIEWOFS   0x400000 // view_ofs is zero (PreThink @0x80b4a sets or clears it each frame)
#define TSSTUNT_WALLDIVE    0x800000 // dive faces forward, not velocity

#define TSSTUNT_DIVE_HEIGHT (TSSTUNT_DIVE_HIGH | TSSTUNT_DIVE_LOW | TSSTUNT_DIVE_MED)
#define TSSTUNT_ACTIVE_MASK 0x130c30 // any dive/slide/wheel/walljump/climb running

class CTSStunt
{
	// The original writes stunt state (esp. m_flRollTime = stunt+0x4) directly from the melee/katana
	// code (kick/katana duck-slash "land low, get up"); grant that direct access rather than invent
	// a setter. See CTSGun::CCombatAttack / KatanaFire.
	friend class CTSGun;
	friend class CBasePlayer; // PreThink stamps m_flLastStunt (@0x809ab); PostThink's superjump landing writes m_flRollTime (@0x81a15)
public:
	// +0x20 — the stunt/dive direction stored by GoRoll/GoWheel/GoDive (normalize(velocity), or
	// -v_forward for a standing wheel). CTSStunt is embedded at CBasePlayer+0x4f0 in the original,
	// so this IS "player+0x510" — the direction source Animate's roll/getup/dive_end/wheel cases
	// read (GetDirection(m_vecStuntDir, v_angle, 0.6, 0.3) -> the pose direction).
	Vector m_vecStuntDir;
	// +0x2c — launch direction != forward ("player+0x51c"): gates Animate's directional lying/roll
	// pose paths (cases 3/0x2a/0x2b). Written by GoRoll/GoWheel/Landing.
	bool  m_bDirForward = false;

	void Init(CBasePlayer *pPlayer);          // @0x89fbc — reset per spawn
	void Play();                               // @0x8a014 — per-frame driver (PreThink)

	// movement chooser + the launches (set pev->iuser4 + gravity/friction/anim/sound)
	void HandleStunt();                        // @0x8b000
	void GoDive();                             // @0x8b1a0
	int  GoRoll(int dir);                      // @0x8cfe8
	void GoWallClimb();                        // @0x8aa1c
	void GoWallJump();                         // @0x8a7d4
	void GoBulletTime();                       // @0x8a5b4
	int  GoWheel();                            // @0x8c9bc

	// per-frame continuation while a flag is active (ground)
	void HandleDive();                         // @0x8b7e4
	void HandleSlide();                        // @0x8b9f8
	void HandleWheel();                        // @0x8b820
	void HandleWallJump();                     // @0x8b4fc
	void HandleWallClimb();                    // @0x8b644
	void Landing();                            // @0x8bbf4

	bool CanDiveHigh();                        // @0x8a494
	bool CanDiveMedium();                      // @0x8a4ec
	bool CanDiveLow();                         // @0x8a544
	int  SlotBonus();                          // @0x8a480 (returns 0 in the shipped binary)
	float NextDive();                          // @0x8a59c (returns 0)

	bool CheckForWallJump();                   // @0x8ac14
	bool CheckForWallDive();                   // @0x8ad44
	bool CheckForWallClimb();                  // @0x8ae6c

	void CheckForBreakables();                 // @0x8bd9c — run/dive-into-body melee sweep
	int  CloseAttack();                        // @0x8c630 — kung-fu close-combat strike (jump-kick)

private:
	CBasePlayer *m_pPlayer = nullptr;          // +0x40

	float m_flStuntTime = 0;        // +0x00 main stunt timer (gates HandleStunt/GoWheel)
	float m_flRollTime = 0;         // +0x04 dive-roll active window
	float m_flStunCooldown = 0;     // +0x08 generic cooldown
	float m_flRollNextTime = 0;     // +0x0c earliest next roll
	float m_flWheelNextTime = 0;    // +0x10 earliest next wheel
	float m_flWallClimbNextTime = 0;// +0x14 wall-climb retry cooldown
	float m_flSlideTime = 0;        // +0x30 slide/bullet-time timer
	float m_flBulletTimeEnd = 0;    // +0x34 bullet-time end (player+0x524). Only writer: GoBulletTime
	                                //   (= time + 0.55); consumer: CBasePlayer::TraceAttack x0.7 bullet
	                                //   damping @0x80228. Init deliberately does NOT clear it.
	float m_flWallClimbFuel = 0;    // +0x38 climb budget
	int   m_iWallClimbCount = 0;    // +0x3c climbs this airtime
	float m_flLastStunt = 0;        // player+0xb88 — time of last stunt (HandleStunt window)
	float m_flGetupTime = 0;        // getup transition end (0 = not getting up); two-stage view raise
	float m_flNextBreakCheck = 0;   // +0x18 — CheckForBreakables throttle (next-check time)
	// (the CheckForBreakables dedupe lives on the PLAYER: m_hRunThroughLast @0x798 — the
	// former CTSStunt::m_pLastBreakVictim here was invented; removed cont.96)

	// NB: the original's TimeScale/SendNextStunt/MotionBlur/ResetStuntState are INLINED (not
	// methods) and GetDirection is a FREE function — see the file-local statics in ts_stunt.cpp.
};

#endif // TS_STUNT_H
