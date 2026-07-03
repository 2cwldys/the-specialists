/***
*
*	Copyright (c) 1996-2001, Valve LLC. All rights reserved.
*	
*	This product contains software technology licensed from Id 
*	Software, Inc. ("Id Technology").  Id Technology (c) 1996 Id Software, Inc. 
*	All Rights Reserved.
*
*   Use, distribution, and modification of this source code and/or resulting
*   object code is restricted to non-commercial enhancements to products from
*   Valve LLC.  All other use, distribution, or modification is prohibited
*   without written permission from Valve LLC.
*
****/

#pragma once

#include "pm_materials.h"
#include "ts_animation.h" // TS 3rd-person body/gait (CTSAnimation m_TSAnim member)
#include "ts_stunt.h"     // TS stunt/acrobatics (CTSStunt m_TSStunt member)

// TS join/spectate flow wire constants (cont.65) — shared client/server contract.
// gmsgSpectator byte 1 (state); the client CHudSpectate draws accordingly.
#define TSSPEC_OFF 0       // not spectating (in play)
#define TSSPEC_COUNTDOWN 1 // counting down to spawn-eligibility (byte 2 = seconds)
#define TSSPEC_READY 2     // gate elapsed — "Press fire to play"
// gmsgVGUIMenu menu code for the TS buy/config menu (client maps to MENU_TS_BUY).
#define TS_VGUI_BUYMENU 20


#define PLAYER_FATAL_FALL_SPEED 1024															  // approx 60 feet
#define PLAYER_MAX_SAFE_FALL_SPEED 580															  // approx 20 feet
#define DAMAGE_FOR_FALL_SPEED (float)100 / (PLAYER_FATAL_FALL_SPEED - PLAYER_MAX_SAFE_FALL_SPEED) // damage per unit per second.
#define PLAYER_MIN_BOUNCE_SPEED 200
#define PLAYER_FALL_PUNCH_THRESHHOLD (float)350 // won't punch player's screen/make scrape noise unless player falling at least this fast.

//
// Player PHYSICS FLAGS bits
//
#define PFLAG_ONLADDER (1 << 0)
#define PFLAG_ONSWING (1 << 0)
#define PFLAG_ONTRAIN (1 << 1)
#define PFLAG_ONBARNACLE (1 << 2)
#define PFLAG_DUCKING (1 << 3)	// In the process of ducking, but totally squatted yet
#define PFLAG_USING (1 << 4)	// Using a continuous entity
#define PFLAG_OBSERVER (1 << 5) // player is locked in stationary cam mode. Spectators can move, observers can't.

//
// generic player
//
//-----------------------------------------------------
//This is Half-Life player entity
//-----------------------------------------------------
#define CSUITPLAYLIST 4 // max of 4 suit sentences queued up at any time

#define SUIT_GROUP true
#define SUIT_SENTENCE false

#define SUIT_REPEAT_OK 0
#define SUIT_NEXT_IN_30SEC 30
#define SUIT_NEXT_IN_1MIN 60
#define SUIT_NEXT_IN_5MIN 300
#define SUIT_NEXT_IN_10MIN 600
#define SUIT_NEXT_IN_30MIN 1800
#define SUIT_NEXT_IN_1HOUR 3600

#define CSUITNOREPEAT 32

#define SOUND_FLASHLIGHT_ON "items/flashlight1.wav"
#define SOUND_FLASHLIGHT_OFF "items/flashlight1.wav"

#define TEAM_NAME_LENGTH 16

typedef enum
{
	PLAYER_IDLE,
	PLAYER_WALK,
	PLAYER_JUMP,
	PLAYER_SUPERJUMP,
	PLAYER_DIE,
	PLAYER_ATTACK1,
} PLAYER_ANIM;

#define MAX_ID_RANGE 2048
#define SBAR_STRING_SIZE 128

enum sbar_data
{
	SBAR_ID_TARGETNAME = 1,
	SBAR_ID_TARGETHEALTH,
	SBAR_ID_TARGETARMOR,
	SBAR_END,
};

#define CHAT_INTERVAL 1.0f

struct TSItem;       // the TS arsenal slot (== the original's s_weapon_status); defined in ts_gun.h
class CTSWorldGun;   // the dropped-weapon entity (ts_worldgun.h) — ThrowWeapon returns one

// @ s_flag_status (0x1c) — the TS CTF carried-flag state. Copied byte-for-byte between the
// owning CTSBriefcase (its template), the loose CTSFlag entity (@+0x90), and the carrier
// (CBasePlayer m_flagStatus @+0x7d0) at every hop. The EHANDLE rides inside the struct, so a
// flag always "knows" which briefcase to respawn at (CTSFlag::RespawnFlag / CaptureTouch).
struct s_flag_status
{
	int      active = 0;       // +0x00 — 1 = carried/active (HaveFlag() returns this byte)
	string_t contents = 0;     // +0x04 — description text ("the documents")
	int      flagID = 0;       // +0x08 — flag/briefcase id 1..15 (CTSBriefcase::s_nextFlagID)
	int      team = -1;        // +0x0c — owning team, 0-based (-1 = neutral/unset)
	int      lastCarrier = 0;  // +0x10 — last carrier entindex (anti-respam pickup HUD)
	EHANDLE  hBriefcase;       // +0x14 — handle to the owning CTSBriefcase (NOT the carrier)
};

class CBasePlayer : public CBaseMonster
{
public:
	// Spectator camera
	void Observer_FindNextPlayer(bool bReverse);
	void Observer_Idle();                                 // @0x89ca4 — auto-advance off a dead/gone target
	void Observer_HandleButtons();
	void Observer_SetMode(int iMode);
	void Observer_CheckTarget();
	void Observer_CheckProperties();
	EHANDLE m_hObserverTarget;
	float m_flNextObserverInput;
	int m_iObserverWeapon;	 // weapon of current tracked target
	int m_iObserverLastMode; // last used observer mode
	bool IsObserver() { return 0 != pev->iuser1; }

	int random_seed; // See that is shared between client & server for shared weapons code

	int m_iPlayerSound;		// the index of the sound list slot reserved for this player
	int m_iTargetVolume;	// ideal sound volume.
	int m_iWeaponVolume;	// how loud the player's weapon is right now.
	int m_iExtraSoundTypes; // additional classification for this weapon's sound
	int m_iWeaponFlash;		// brightness of the weapon flash
	float m_flStopExtraSoundTime;

	float m_flFlashLightTime; // Time until next battery draw/Recharge
	int m_iFlashBattery;	  // Flashlight Battery Draw

	int m_afButtonLast;
	int m_afButtonPressed;
	int m_afButtonReleased;

	EHANDLE m_SndLast;	   // last sound entity to modify player room type
	int m_SndRoomtype = 0; // last roomtype set by sound entity. Defaults to 0 on new maps to disable it by default.
	int m_ClientSndRoomtype;
	float m_flSndRange; // dist from player to sound entity

	float m_flFallVelocity;

	int m_rgItems[MAX_ITEMS];
	bool m_fKnownItem; // True when a new item needs to be added
	int m_fNewAmmo;	   // True when a new item has been added

	unsigned int m_afPhysicsFlags; // physics flags - set when 'normal' physics should be revisited or overriden
	float m_fNextSuicideTime;	   // the time after which the player can next use the suicide command


	// these are time-sensitive things that we keep track of
	float m_flTimeStepSound;  // when the last stepping sound was made
	float m_flTimeWeaponIdle; // when to play another weapon idle animation.
	float m_flSwimTime;		  // how long player has been underwater
	float m_flDuckTime;		  // how long we've been ducking
	float m_flWallJumpTime;	  // how long until next walljump

	float m_flSuitUpdate;						 // when to play next suit update
	int m_rgSuitPlayList[CSUITPLAYLIST];		 // next sentencenum to play for suit update
	int m_iSuitPlayNext;						 // next sentence slot for queue storage;
	int m_rgiSuitNoRepeat[CSUITNOREPEAT];		 // suit sentence no repeat list
	float m_rgflSuitNoRepeatTime[CSUITNOREPEAT]; // how long to wait before allowing repeat
	int m_lastDamageAmount;						 // Last damage taken
	float m_tbdPrev;							 // Time-based damage timer

	float m_flgeigerRange; // range to nearest radiation source
	float m_flgeigerDelay; // delay per update of range msg to client
	int m_igeigerRangePrev;
	int m_iStepLeft;						// alternate left/right foot stepping sound
	char m_szTextureName[CBTEXTURENAMEMAX]; // current texture name we're standing on
	char m_chTextureType;					// current texture type

	int m_idrowndmg;	  // track drowning damage taken
	int m_idrownrestored; // track drowning damage restored

	int m_bitsHUDDamage; // Damage bits for the current fame. These get sent to
						 // the hude via the DAMAGE message
	bool m_fInitHUD;	 // True when deferred HUD restart msg needs to be sent
	bool m_fGameHUDInitialized;
	int m_iTrain;	// Train control position
	bool m_fWeapon; // Set this to false to force a reset of the current weapon HUD info

	EHANDLE m_pTank;		 // the tank which the player is currently controlling,  NULL if no tank
	EHANDLE m_hViewEntity;	 // The view entity being used, or null if the player is using itself as the view entity
	bool m_bResetViewEntity; //True if the player's view needs to be set back to the view entity
	float m_fDeadTime;		 // the time at which the player died  (used in PlayerDeathThink())

	bool m_fNoPlayerSound; // a debugging feature. Player makes no sound if this is true.
	bool m_fLongJump;	   // does this player have the longjump module?

	float m_tSneaking;
	int m_iUpdateTime;	  // stores the number of frame ticks before sending HUD update messages
	int m_iClientHealth;  // the health currently known by the client.  If this changes, send a new
	int m_iClientBattery; // the Battery currently known by the client.  If this changes, send a new
	int m_iHideHUD;		  // the players hud weapon info is to be hidden
	int m_iClientHideHUD;
	int m_iFOV;		  // field of view
	int m_iClientFOV; // client's known FOV
	// usable player items
	CBasePlayerItem* m_rgpPlayerItems[MAX_ITEM_TYPES];
	CBasePlayerItem* m_pActiveItem;
	CBasePlayerItem* m_pClientActiveItem; // client version of the active item
	CBasePlayerItem* m_pLastItem;

	std::uint64_t m_WeaponBits;

	//Not saved, used to update client.
	std::uint64_t m_ClientWeaponBits;

	// shared ammo slots
	int m_rgAmmo[MAX_AMMO_SLOTS];
	int m_rgAmmoLast[MAX_AMMO_SLOTS];

	Vector m_vecAutoAim;
	bool m_fOnTarget;
	int m_iDeaths;
	float m_flRespawnTimer; // used in PlayerDeathThink() to make sure players can always respawn

	int m_lastx, m_lasty; // These are the previous update's crosshair angles, DON"T SAVE/RESTORE

	int m_nCustomSprayFrames = -1; // Custom clan logo frames for this player
	float m_flNextDecalTime;  // next time this player can spray a decal

	char m_szTeamName[TEAM_NAME_LENGTH];

	void Spawn() override;
	void Pain();

	//	void Think() override;
	virtual void Jump();
	virtual void Duck();
	virtual void PreThink();
	virtual void PostThink();
	Vector GetGunPosition() override;
	bool TakeHealth(float flHealth, int bitsDamageType) override;
	void TraceAttack(entvars_t* pevAttacker, float flDamage, Vector vecDir, TraceResult* ptr, int bitsDamageType) override;
	bool TakeDamage(entvars_t* pevInflictor, entvars_t* pevAttacker, float flDamage, int bitsDamageType) override;
	void Killed(entvars_t* pevAttacker, int iGib) override;
	Vector BodyTarget(const Vector& posSrc) override { return Center() + pev->view_ofs * RANDOM_FLOAT(0.5, 1.1); } // position to shoot at
	void StartSneaking() override { m_tSneaking = gpGlobals->time - 1; }
	void StopSneaking() override { m_tSneaking = gpGlobals->time + 30; }
	bool IsSneaking() override { return m_tSneaking <= gpGlobals->time; }
	bool IsAlive() override { return (pev->deadflag == DEAD_NO) && pev->health > 0; }
	bool ShouldFadeOnDeath() override { return false; }
	bool IsPlayer() override { return true; } // Spectators should return false for this, they aren't "players" as far as game logic is concerned

	bool IsNetClient() override { return true; } // Bots should return false for this, they can't receive NET messages
												 // Spectators should return true for this
	const char* TeamID() override;

	bool Save(CSave& save) override;
	bool Restore(CRestore& restore) override;
	void RenewItems();
	void PackDeadPlayerItems();
	void RemoveAllItems(bool removeSuit);
	bool SwitchWeapon(CBasePlayerItem* pWeapon);

	/**
	*	@brief Equips an appropriate weapon for the player if they don't have one equipped already.
	*/
	void EquipWeapon();

	void SetWeaponBit(int id);
	void ClearWeaponBit(int id);

	bool HasSuit() const;
	void SetHasSuit(bool hasSuit);

	// JOHN:  sends custom messages if player HUD data has changed  (eg health, ammo)
	virtual void UpdateClientData();

	static TYPEDESCRIPTION m_playerSaveData[];

	// Player is moved across the transition by other means
	int ObjectCaps() override { return CBaseMonster::ObjectCaps() & ~FCAP_ACROSS_TRANSITION; }
	void Precache() override;
	bool IsOnLadder();
	bool FlashlightIsOn();
	void FlashlightTurnOn();
	void FlashlightTurnOff();

	void UpdatePlayerSound();
	void DeathSound() override;

	int Classify() override;
	void SetAnimation(PLAYER_ANIM playerAnim);
	void SetWeaponAnimType(const char* szExtention);
	char m_szAnimExtention[32];
	// player+0x478: the per-frame 3rd-person body-anim OVERRIDE string (RE Animate@0x5b470 case 1/6 +
	// DefaultDeploy@0x90178). When non-empty, CTSAnimation::Animate plays it directly as the body idle
	// (instead of idle_/aim_<grip>) — the dedicated melee idles: knife/sealknife -> "knife_idle",
	// katana -> "katana_idle", flag -> "briefcase_idle". Set on weapon deploy, cleared on holster.
	char m_szAnimOverride[16] = {};

	// custom player functions
	virtual void ImpulseCommands();
	void CheatImpulseCommands(int iImpulse);

	void StartDeathCam();
	void StartObserver(Vector vecPosition, Vector vecViewAngle);

	void AddPoints(int score, bool bAllowNegativeScore) override;
	void AddPointsToTeam(int score, bool bAllowNegativeScore) override;
	bool AddPlayerItem(CBasePlayerItem* pItem) override;
	bool RemovePlayerItem(CBasePlayerItem* pItem) override;
	void DropPlayerItem(char* pszItemName);
	bool HasPlayerItem(CBasePlayerItem* pCheckItem);
	bool HasNamedPlayerItem(const char* pszItemName);
	bool HasPlayerItemFromID(int nID);
	bool HasWeapons(); // do I have ANY weapons?
	void SelectPrevItem(int iItem);
	void SelectNextItem(int iItem);
	void SelectLastItem();
	void SelectItem(const char* pstr);
	void ItemPreFrame();
	void ItemPostFrame();
	void GiveNamedItem(const char* szName);
	void GiveNamedItem(const char* szName, int defaultAmmo);
	void EnableControl(bool fControl);

	int GiveAmmo(int iAmount, const char* szName, int iMax) override;
	void SendAmmoUpdate();
	void SendSingleAmmoUpdate(int ammoIndex);

private:
	void InternalSendSingleAmmoUpdate(int ammoIndex);

public:
	void WaterMove();
	void EXPORT PlayerDeathThink();
	void PlayerUse();

	void CheckSuitUpdate();
	void SetSuitUpdate(const char* name, bool fgroup, int iNoRepeat);
	void UpdateGeigerCounter();
	void CheckTimeBasedDamage();

	bool FBecomeProne() override;
	void BarnacleVictimBitten(entvars_t* pevBarnacle) override;
	void BarnacleVictimReleased() override;
	static int GetAmmoIndex(const char* psz);
	int AmmoInventory(int iAmmoIndex);
	int Illumination() override;

	void ResetAutoaim();
	Vector GetAutoaimVector(float flDelta);
	Vector AutoaimDeflection(Vector& vecSrc, float flDist, float flDelta);

	void ForceClientDllUpdate(); // Forces all client .dll specific data to be resent to client.

	void DeathMessage(entvars_t* pevKiller);

	void SetCustomDecalFrames(int nFrames);
	int GetCustomDecalFrames();

	void TabulateAmmo();

	float m_flStartCharge;
	float m_flAmmoStartCharge;
	float m_flPlayAftershock;
	float m_flNextAmmoBurn; // while charging, when to absorb another unit of player's ammo?

	//Player ID
	void InitStatusBar();
	void UpdateStatusBar();
	int m_izSBarState[SBAR_END];
	float m_flNextSBarUpdateTime;
	float m_flStatusBarDisappearDelay;
	char m_SbarString0[SBAR_STRING_SIZE];
	char m_SbarString1[SBAR_STRING_SIZE];

	float m_flNextChatTime;

	void SetPrefsFromUserinfo(char* infobuffer);

	int m_iAutoWepSwitch;

	bool m_bRestored;

	//True if the player is currently spawning.
	bool m_bIsSpawning = false;

	// --- mod-ts (The Specialists) additions ---
	// TS adds its player state directly onto CBasePlayer (there is no CTSPlayer).
	// Offsets in comments are the original gcc2 layout (provenance, not targets).
	int m_iPowerups = 0;            // 0x724 — active powerup bitfield (TS_PWUP_*)
	int m_iHeldPowerup = 0;         // 0x728 — held powerup awaiting activation
	float m_flPowerupExpireTime = 0;// 0x72c — active timed-powerup end time
	float m_flPowerupTime = 0;      // 0x730 — held powerup duration (seconds)
	float m_flHealthRegen = 0;      // 0x73c — health-powerup regen tick time
	float m_flSpreadAccum = 0;        // 0x778 — TS dynamic-accuracy spread accumulator: grows per shot in
	                                  // CTSGun::ScaleSpread (capped 0.1), recovers in PreThink. Effective
	                                  // bullet spread = weapon flSpread + this (so spraying widens the cone).
	byte m_bSlowBullets = 0;          // 0x175 — bullet-time/"pause" flag (0=slowmo, 1=slowpause)
	float m_flSlowMotionGo = 1.0f;    // 0x168 — applied slow factor (1 = normal) — drives maxspeed
	float m_flSlowMotionTarget = 1.0f;// 0x170 — target slow factor (1 = normal)
	float m_flSlowMotionScale = 0;    // 0x17c — SELF slow scale in SlowMotionFlow (0.6 slowmo / 0.8 pause); 0 = inactive (gate)
	float m_flSlowMotionRamp = 0;     // 0x180 — OTHERS slow scale (0.35 slowmo / 0.225 pause) applied to LOS victims
	float m_flNextSlowFlow = 0;       // 0x178 — next SlowMotionFlow tick (runs every 0.5s while active)
	EHANDLE m_hSlowPlayers[32];       // 0x188 — per-player cache of who THIS player is slowing (for restore)
	int m_iClientHeldType = -1;       // 0x738 — last-sent held powerup type (gmsgTSPowerUp change-detect)
	int m_iClientHeldDur = -1;        // 0x734 — last-sent held powerup duration
	float m_flClientSlowAmt = -1;     // 0x16c — last-sent slow amount (gmsgTSSlowAmt change-detect)
	int m_iClientBulletTime = 0;      // 0x174 — last-sent bullet-time flag (gmsgTSBulletTime; byte cache, init 0 like the original)
	byte m_iKungFu = 0;               // 0x74e — kung-fu state (kung-fu powerup)
	byte m_bLowGravity = 0;           // 0x74f — super-jump powerup active (engine physinfo "mj")
	byte m_bTheOne = 0;               // 0x7b9 — this player is "The One" (the juggernaut game mode)
	float m_flKungFuBlock = 0;        // 0x7a8 — kungfu block window: VALUE 0.5 set by CCombatBlock, PreThink-decayed, floor 0
	// ---- melee stamina/whiff/block state (transient, NOT saved; RE dossier-melee.md + the L1/L2 addendum) ----
	float m_flMeleeBar = 100.0f;      // 0x774 — melee stamina 0..100; UpdateMeleeBar drains, PreThink regens 15/s, TSInit=100
	float m_flWhiffLungeTime = 0;     // 0x79c — whiff-lunge window, armed on a MISSED swing; PreThink counts down
	float m_flWhiffLungeForce = 0;    // 0x7a0 — SIGNED lunge force (backkick/backslash negative); doubled if KungFu; zeroed on a block
	int m_iWhiffLungeType = 0;        // 0x7a4 — 0 kungfu / 1 knife / 2 katana (VictimKungfuHold's counter reads ==0)
	float m_flKnifeBlock = 0;         // 0x7ac — knife block window: VALUE 0.5 set by CCombatBlock, PreThink-decayed, floor 0
	float m_flKatanaBlock = 0;        // 0x7b0 — katana block window (also deflects a knife); same decay law
	float m_flBlockTime = 0;          // 0x4dc — BlockPlayer() self-root timer (pairs w/ iuser4 STOP bit); PreThink-decayed
	float m_flLastMeleeRate = 0;      // 0x4f8 — last melee refire interval (written by every melee fire; consumer VERIFY)
	float m_flDuckSlashAux = 0;       // 0x4f4 — katana duck-slash aux (=2.0; consumer VERIFY)
	float m_flDisarmTime = 0;         // 0x71c — disarm-immunity gate for DropWeapon(-2) in the melee twins
	float m_flDisarmPressure = 0;     // 0x7b4 — accumulated melee disarm pressure; PreThink decays x5; DropWeapon(-2) threshold
	int m_iStandUpLatch = 0;          // 0x78 — set 1 by HandleSlide's deliberate standup (consumer VERIFY)
	float m_flSuperJumpFuel = 0;      // super-jump remaining lift time (refills on the ground)
	float m_flNextPainTime = 0;       // cooldown for the TS pain voice (player/pain1-4)
	byte m_bSuperJumpAir = 0;         // super-jump: was airborne last frame (to detect the landing thud)
	float m_flWallClimbFuel = 0;      // double-jump wall-climb budget (refills on ground; weight-scaled)
	byte m_bDiveRolling = 0;          // mid double-jump dive-roll (land prone on touchdown)
	byte m_iDiveRollDir = 0;          // roll direction: 0 fwd / 1 back / 2 left / 3 right (back = land on feet)
	float m_flProneUntil = 0;         // forced-prone (ducked) until this time after a roll landing
	byte m_bWasClimbing = 0;          // wall-climb engaged last frame (so the kick sound plays once per engage)
	byte m_bDivePressed = 0;          // +alt1 dive consumed (one dive per press)
	byte m_bProneSize = 0;            // dive/prone has shrunk the hull (so we restore it on exit)
	float m_flNextClimbKick = 0;      // wall-climb: next auto-kick time (paces the kicks while held)
	EHANDLE m_hRunThroughLast;        // last entity barged through (orig player+0x798): don't re-hit it
	bool m_bSlowMoveApplied = false;  // slow-motion has lowered pev->maxspeed (restore on exit)

	// TS 3rd-person body/gait animation (ts_animation.cpp). The original embeds
	// this object on the player (player+0x28c) and drives ALL body animation
	// through it; the stock SetAnimation is a no-op stub in the .so. SetAnimation
	// (below) routes the PLAYER_ANIM call sites into m_TSAnim.Animate; CTSGun
	// drives reload/draw/grenade directly. See ts_animation.h.
	CTSAnimation m_TSAnim;

	// TS stunt/acrobatics system (ts_stunt.cpp). The original embeds a CTSStunt on
	// the player; Play() runs each PreThink and drives the dive/roll/wall-climb/
	// wall-jump/bullet-time state machine through the networked pev->iuser4 flags so
	// pm_shared predicts the small hull + low view + launch velocity. See ts_stunt.h.
	CTSStunt m_TSStunt;

	// TS custom-HUD networking (ts_player.cpp). The original sends its OWN per-frame
	// HUD messages (TSHealth/TSArmor/TSCash/...) from CBasePlayer::UpdateClientData
	// instead of the stock Health/Battery — TS ships a fully custom client HUD. We
	// mirror that from TSUpdateClientData(), cached + sent only on change. See
	// reference/HUD_MESSAGES.md for the recovered wire formats.
	void TSUpdateClientData();
	int m_iClientTSHealth = -1;       // last TSHealth byte sent (round(health), >=0)
	int m_iClientTSArmor = -1;        // last TSArmor byte sent (round(armorvalue))
	int m_iClientTSCash = -1;         // last TSCash short sent (m_iCash)

	// TS cash + buy/armament loadout (ts_buy.cpp). In the shipped game CashBased() is
	// hardcoded false (weapons are FREE), so the "buy" system is really a free loadout
	// selector the client menu drives; cash is tracked (cash-for-win/lose) but never charged.
	int m_iCash = 0;                  // 0x540 — player cash (clamped 0..30000)
	int m_iGrenadeBuys = 0;           // 0x714 — M61 grenades bought this life (cap 2)
	int m_armamentAmmo[16] = {};      // 0x554 — pending per-caliber ammo loadout (rounds)
	// An armament entry is {type, count, isItem}. BuyArmament routes isItem==false ->
	// BuyWeapon (a weapon type) and isItem==true -> AddItem (an item bitmask: kevlar/
	// grenade/attachments), matching the original BuyArmament @0x83f10.
	struct s_armament_item { int type = 0; int count = 0; bool isItem = false; };
	s_armament_item m_armament[32];   // 0x594 — pending weapon/item loadout (stride 0xc ×32)
	bool m_bHasKevlar = false;        // 0x718 bit 0x10 — has bought kevlar (binary)

	s_flag_status m_flagStatus;       // 0x7d0 — the carried CTF flag state (active byte gates HaveFlag)

	bool TakeFlag(s_flag_status *src);                    // @0x83830 — pick up a flag onto m_flagStatus
	void DropFlag(bool bSilent);                          // @0x8334c — drop it (false=spawn loose flag)
	bool DropWeapon();                                    // @0x84900 — drop the active TS weapon (the drop key)
	CTSWorldGun *ThrowWeapon(int type, const TSItem &status, const Vector *angleOffset, const Vector *velOffset); // @0x8529c
	void CutWire(int wire);                               // cut a bomb wire (the cutwire_<n> command)
	void SetPlayerSpeed(float speed);                     // @0x83068 — clamp [0,330] + SetClientMaxspeed
	void SetPlayerGravity(float setval, float gravity);   // @0x85b70 — set pev->gravity (gated on >= 1.0)
	void PunchScreen(float magnitude, Vector dir);        // @0x85f70 — empty in the shipped game
	float GetMeleeMultiplier();                           // @0x865d0 — melee dmg mult from melee-bar stamina
	void UpdateMeleeBar(float flAmount);                  // @0x8650c — THE melee-stamina drain choke-point (sends KFuPower)
	void BlockPlayer(float flDuration);                   // @0x85b58 — post-swing movement self-root (STOP bit + m_flBlockTime)
	void CheckHull(Vector vecSrc, Vector vecEnd, TraceResult &tr, int iHull); // @0x85e28 — melee line+hull trace
	bool DropWeapon(int iType, float flDamage);           // @0x84900 (disarm shape): -2 = active weapon; roll VERIFY-gated
	// @0x85f88 / @0x86610 — the shared melee hit-resolver twins (kung-fu / coldcock)
	CBaseEntity *CloseCombatAttack(Vector vecSrc, Vector vecEnd, Vector vecDir, int iHull, float flDamage,
		int iAnim, bool bDisarm, bool bPlayersOnly, bool bKnockback, bool bForceTeamSafe);
	CBaseEntity *ColdCockAttack(Vector vecSrc, Vector vecEnd, Vector vecDir, int iHull, float flDamage,
		int iAnim, bool bDisarm, bool bPlayersOnly, bool bKnockback, bool bForceTeamSafe);
	void DropArmor();                                     // @0x83a3c — drop armorvalue as a CTSAmmoPack
	void DropAmmo();                                      // @0x846f0 — drop the gun's reserve as a CTSAmmoPack
	void UpdatePlayerState();                             // @0x82a08 — derive m_iPlayerState (locked/frozen/normal)
	int m_iPlayerState = 0;                               // 0x498 — pose: 0 normal, 1 ducking, 2 stunt/locked
	int m_iClientTSState = 0;                             // 0x499 — last-sent TSState byte (plain on-change; the
	                                                      // original's InitHUD memset deliberately skips this cache)
	int m_iClientTSSpace = 0;                             // 0x54c — last-sent TSSpace free-slots value
	byte m_iAward = 0;                                    // 0x74c — the broadcast award/status byte: 0 clear,
	                                                      // 1 out-of-round, 2/3/4 kill tiers, 10 The One, 11 respawning
	byte m_iClientAward = 0;                              // 0x74d — last TSPAward value broadcast
	float m_flNextPTakeDamTime = 0;                       // 0x750 (invented name) — PTakeDam send throttle (+0.3 s)
	byte m_iWires[3] = {};                                // 0x78c-0x78e — the nearby armed bomb's wire ROLE codes
	                                                      // (0 cut / 1 defuse / 2 boobytrap / 3 intact)
	byte m_iClientWires[3] = {};                          // 0x78f-0x791 — the last-sent Wires triple
	float m_flNextWiresCheck = 0;                         // 0x794 — the 0.5 s CheckForWires throttle
	void CheckForWires();                                 // @0x82f08 — sphere-64 armed-ts_bomb wire scan
	void TSUse();                                         // @0x856e0 — server-side double-tap +use -> UsePowerUp
	float m_flUseDoubleTap = 0;                           // 0x164 — the double-tap +use window deadline
	float m_flSetMaxSpeed = 0;                            // 0x550 — the last SetPlayerSpeed value (clamped)
	float m_flGravityValue = 0;                           // 0x4ec — the SetPlayerGravity stored value

	int  GiveCash(int amount);                            // add cash (clamped 0..30000)
	void ResetArmament();                                 // clear the pending loadout
	void AddItemToArmament(int type, int count, bool isItem); // queue a weapon/item into the loadout
	void AddAmmoToArmament(int caliber, int rounds);      // queue ammo into the loadout
	bool BuyWeapon(int type);                             // give a weapon (free; restriction-gated)
	bool BuyAmmo(int caliber, int rounds);               // give ammo (free)
	void BuyArmament();                                   // apply the pending loadout
	void AddItem(int itemBits);                           // @0x8440c — grant a loadout item (kevlar/grenade/attachment)
	void TakeArmor(int amount);                           // @0x83da4 — grant/remove kevlar (binary flag + armorvalue)

	// TS powerup activation state machine (ts_player.cpp). UpdatePowerUpState runs
	// each frame from PreThink; UsePowerUp is the `usepowerup` command handler.
	bool UsePowerUp();           // activate the held timed powerup
	void UpdatePowerUpState();   // per-frame: apply slow-motion fields / expire
	void SlowMotionFlow();       // per-frame: slow self + LOS-visible enemies while active
	void DeactivateSlowMotionFlow(); // restore normal speed when slow-motion ends

	// TS slow-motion state (GoSlow__11CBasePlayer / InSlow__11CBasePlayer). GoSlow
	// applies a time-scale to this player; InSlow reports whether it's slowed.
	// (SLICE: the actual movement time-dilation reads these client-side.)
	void GoSlow(float scale, int slowBullets);
	int InSlow();

	// TS join/spectate spawn-gate (cont.65, ts_player.cpp). A joining (or just-
	// respawned) player is held in free-look spectate (StartObserver, OBS_ROAMING)
	// until the gate elapses and they press fire — only then do they enter play with
	// their chosen loadout. Mirrors the original StartObserver/SetPlayerRespawnTime/
	// "Press Fire To Play!" path. The buy/config menu (VGUIMenu) opens on entry.
	bool m_fInSpectateWait = false; // held in free-look, waiting to press fire
	bool m_fConfigChosen = false;   // the player confirmed a loadout (first +attack)
	float m_flSpawnGateTime = 0;    // gpGlobals->time at which fire-to-play is allowed
	int m_iClientSpecSecs = -1;     // last Spectator countdown byte sent (resend on change)
	void TSEnterSpectate();         // enter free-look spectate-wait (called at end of Spawn)
	void StopObserver();         // press-fire: leave spectate -> respawn into play
	void TSSpectateThink();         // per-frame while waiting: countdown msg + fire check
	void TSTryRespawn();            // `respawn` cmd: leave spectate if the gate elapsed, else "wait" msg
	void TSSendScoreInfo();         // broadcast this player's scoreboard row (class=WAITING while spectating)
	void TSSendBuyMenu();           // send the buy/config menu (gmsgShowMenu, compact list)
	void TSCloseMenu();             // turn the menu off (ShowMenu with 0 valid slots)
	void TSMenuSelect(int slot);    // handle a buy/config menu selection (menuselect / click)
};

inline void CBasePlayer::SetWeaponBit(int id)
{
	m_WeaponBits |= 1ULL << id;
	// Mirror to pev->weapons so server-side consumers (Metamod plugins,
	// the engine's cdata->iWeaponBits delivery to vanilla clients, any
	// HL mod code that pre-dates m_WeaponBits) see the player's actual
	// weapon mask. Bits 32..63 don't fit in pev->weapons (int32), but
	// today only WEAPON_SUIT (31) and weapons 0..30 are used.
	if (id < 32)
		pev->weapons |= 1 << id;
}

inline void CBasePlayer::ClearWeaponBit(int id)
{
	m_WeaponBits &= ~(1ULL << id);
	if (id < 32)
		pev->weapons &= ~(1 << id);
}

inline bool CBasePlayer::HasSuit() const
{
	return (m_WeaponBits & (1ULL << WEAPON_SUIT)) != 0;
}

inline void CBasePlayer::SetHasSuit(bool hasSuit)
{
	if (hasSuit)
	{
		SetWeaponBit(WEAPON_SUIT);
	}
	else
	{
		ClearWeaponBit(WEAPON_SUIT);
	}
}

#define AUTOAIM_2DEGREES 0.0348994967025
#define AUTOAIM_5DEGREES 0.08715574274766
#define AUTOAIM_8DEGREES 0.1391731009601
#define AUTOAIM_10DEGREES 0.1736481776669

inline bool gInitHUD = true;
inline bool gEvilImpulse101 = false;
inline bool giPrecacheGrunt = false;

/**
*	@brief Display the game title if this key is set
*/
inline DLL_GLOBAL bool gDisplayTitle = false;
inline DLL_GLOBAL unsigned int g_ulModelIndexPlayer = 0;
inline DLL_GLOBAL CBaseEntity* g_pLastSpawn = nullptr;
