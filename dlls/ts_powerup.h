/***
*  ts_powerup.h — CTSPowerUp (original TS_powerups.cpp), the map/dynamic powerup pickup.
*  Declared in a header so the gamerules/player subsystems can reference it and the audit sees
*  its members (FGD pwuptype/pwupduration/pwupmult). See ts_powerup.cpp for behaviour.
***/
#ifndef TS_POWERUP_H
#define TS_POWERUP_H

class CTSPowerUp;
// @ PlayerAddPowerUp__FP11CBasePlayeriP10CTSPowerUp — apply a powerup to a player.
bool PlayerAddPowerUp(CBasePlayer *pPlayer, int type, CTSPowerUp *pPowerup);

// pev->body sub-model index used to show the right icon per powerup type
// (recovered from SetupPowerUp's icon switch).
enum
{
	PUBODY_KUNGFU = 0,
	PUBODY_DBLFIRE = 1,
	PUBODY_INFCLIP = 2,
	PUBODY_SLOWMO = 4,
	PUBODY_GRENADE = 5,
	PUBODY_HEALTH = 6,
	PUBODY_ARMOR = 7,
	PUBODY_LOWGRAV = 8,
	PUBODY_SLOWPAUSE = 9,
};

// Powerup-mask constants (TS_PWUP_* come from ts_gamerules.h; include this header after it).
#define TS_PWUP_INSTANT (TS_PWUP_INFCLIP | TS_PWUP_KUNGFU | TS_PWUP_GRENADE | TS_PWUP_HEALTH | 0x80 | TS_PWUP_LOWGRAV)
#define TS_PWUP_ARMOR 0x80

class CTSPowerUp : public CBaseAnimating
{
public:
	void Spawn() override;
	void Precache() override;
	bool KeyValue(KeyValueData *pkvd) override;
	bool Save(CSave &save) override;
	bool Restore(CRestore &restore) override;
	// (no ObjectCaps override — the original CTSPowerUp uses the base CBaseAnimating::ObjectCaps,
	// keeping FCAP_ACROSS_TRANSITION; deathmatch maps don't level-transition so this is inert.)

	static TYPEDESCRIPTION m_SaveData[];

	// @ SetupPowerUp(type, duration, fallDelay, respawns)
	void SetupPowerUp(int type, int duration, float fallDelay, bool respawns);
	void EXPORT Fall();      // think: settle to the floor
	void OnGround();         // arm as a touch-pickup once landed
	void EXPORT WaitThink(); // think: animate / fade a waiting powerup
	void EXPORT WaitTouch(CBaseEntity *pOther);

	// @ CreatePowerUp(type, duration) — spawn a temporary (player-dropped/dynamic)
	// powerup entity of the given type.
	static CTSPowerUp *CreatePowerUp(int type, int duration);

	int poweruptype = 0;      // 0x84 — powerup type bits (pwuptype, or random)
	int m_iDuration = 0;      // 0x90 — effect duration handed to the player (s)
	int pwupduration = 0;     // 0x94 — pwupduration: respawn delay
	float pwupmult = 1.0f;    // 0x98 — pwupmult: duration multiplier
	float m_flTimeout = 0;    // 0x88 — temp-powerup lifetime expiry
	float m_flArmor = 100.0f; // 0x8c — armor amount given by the armor powerup
	bool m_bRespawns = false; // 0x80 — map-placed (respawns) vs temp (UTIL_Remove)
	bool m_bRandom = false;   // 0x81 — type chosen randomly -> re-roll on respawn
};

#endif // TS_POWERUP_H
