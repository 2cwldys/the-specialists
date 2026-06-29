/***
*  ts_flag.h — CTSFlag (original TS_TSFlag.cpp), the loose CTF flag entity.
*
*  A CTSFlag entity exists ONLY while a flag is LOOSE (sitting at its base, dropped, or in
*  flight). The carry state is NOT an entity — it is the 28-byte `s_flag_status` (player.h)
*  copied onto the carrier (CBasePlayer::m_flagStatus). On pickup the flag Destroys itself; on
*  drop a fresh flag is CreateFlag'd. classname "ts_flag" — created in code only (CreateFlag),
*  never map-placed. Derives the slow-motion physics base (the CTSWorldGun settle/bounce machine).
***/
#ifndef TS_FLAG_H
#define TS_FLAG_H

#include "ts_physicobject.h" // CTSPhysicObject (slow-motion toss/bounce physics base)

class CTSFlag : public CTSPhysicObject
{
public:
	void Spawn() override;                                  // @0x9d978
	void Precache() override;                               // @0x9db94
	void EXPORT BounceTouch(CBaseEntity *pOther);           // @0x9dbbc — airborne bounce/drop sound
	void Destroy();                                         // @0x9de24 — remove from play
	void RespawnFlag();                                     // @0x9de58 — ask the briefcase to re-drop
	void EXPORT Fall();                                     // @0x9df10 — think while loose (+thrown-flag hit)
	void EXPORT WaitThink();                                // @0x9e1b8 — settled keepalive
	void OnGround();                                        // @0x9e21c — settle into a touch/USE pickup
	bool CanInteract(CBasePlayer *pPlayer);                 // @0x9e2d4 — team gate
	void EXPORT PlayerTakeFlag(CBaseEntity *pOther);        // @0x9e330 — pickup/return (= Use/WaitTouch)
	void Use(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value) override; // @0x9e5dc
	void EXPORT WaitTouch(CBaseEntity *pOther);             // @0x9e604
	int  ObjectCaps() override { return FCAP_CONTINUOUS_USE; } // @0x9e704 -> 0x10
	virtual int CanUsedTroughWalls() { return 0; }          // @0x9e70c -> 0 (TS virtual)

	// @0x9d874 — static factory: spawn a loose flag carrying a copy of `pStatus`. `pOwner` = the
	// dropping player (NULL for a base spawn from the briefcase). Owned drops are tossed.
	static CTSFlag *CreateFlag(CBaseEntity *pOwner, s_flag_status *pStatus);

	float         m_flExpireTime = 0;   // +0x88 — absolute auto-return time (0 = never; a base flag)
	CBaseEntity  *m_pOwner = nullptr;   // +0x8c — player who dropped it (NULL = base-spawned)
	s_flag_status m_flagStatus;         // +0x90 — the carried status; .team@+0x9c, .hBriefcase@+0xa4
	bool          m_bRemoved = false;   // +0xac — guards PlayerTakeFlag against a double-take
};

// @ HaveFlag__FP11CBasePlayer (@0x67148, TS_gamerules.cpp) — is this player carrying a flag?
bool HaveFlag(CBasePlayer *pPlayer);

#endif // TS_FLAG_H
