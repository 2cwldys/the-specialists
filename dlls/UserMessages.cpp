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

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "shake.h"
#include "UserMessages.h"

// The TS gmsg globals are DEFINED in their original-file homes; they are all REGISTERED here,
// centrally, exactly like the original's LinkUserMessages @0xc7db0 (star_LinkUserMessages.md):
// one block, fixed order (message IDs are assigned in registration order), NO lazy first-use
// registrations anywhere else (the sole exception is the stock impulse-99 "Logo" toggle in
// player.cpp, which the original also keeps lazy — gmsgLogo is absent from its block).
extern int gmsgWeaponInfo, gmsgTSWeaponStatus, gmsgTSActiveItems;               // ts_gun.cpp
extern int gmsgKFuPower, gmsgTSHealth, gmsgTSArmor, gmsgTSState, gmsgTSCash,    // ts_player.cpp
	gmsgTSSpace, gmsgTSPowerUp, gmsgTSPowerUpTime, gmsgTSWires, gmsgTSPlayerAward,
	gmsgTSBulletTime, gmsgTSMotionBlur, gmsgTSSlowAmt;
extern int gmsgTSHaveFlag;                                                      // ts_flag.cpp
extern int gmsgTSNextStunt;                                                     // ts_stunt.cpp
extern int gmsgTSRoundTime, gmsgTSObjective, gmsgTSShowObjective,               // TS_gamerulesclass.cpp
	gmsgTSServerSetting, gmsgTSVoiceMessage;
extern int gmsgTSCustomWeaponParams;                                            // TS_util.cpp
extern int gmsgTSMessage;                                                       // ts_utils.cpp
extern int gmsgTSCreateDebris;                                                  // func_break.cpp
extern int gmsgTSPlayerInfo;                                                    // client.cpp
extern int gmsgTSParticleSystem;                                                // ts_sharedemitter.cpp

void LinkUserMessages()
{
	// Already taken care of?
	if (0 != gmsgSelAmmo)
	{
		return;
	}

	// SelAmmo is the original's FIRST registration (star_LinkUserMessages.md) and doubles as
	// the already-linked guard. Registered, never sent (original parity).
	gmsgSelAmmo = REG_USER_MSG("SelAmmo", 4);
	gmsgCurWeapon = REG_USER_MSG("CurWeapon", 3);
	gmsgGeigerRange = REG_USER_MSG("Geiger", 1);
	// (Flashlight/FlashBat NOT registered — the original TS has no flashlight messages; the
	// flashlight attachment rides ActItems. Registration-set parity, dossier-gmsg U0.)
	gmsgHealth = REG_USER_MSG("Health", 1); // original TS wire size = 1 byte (health byte-capped)
	gmsgDamage = REG_USER_MSG("Damage", 12);
	gmsgBattery = REG_USER_MSG("Battery", 2);
	gmsgTrain = REG_USER_MSG("Train", 1);
	//gmsgHudText = REG_USER_MSG( "HudTextPro", -1 );
	gmsgHudText = REG_USER_MSG("HudText", -1); // we don't use the message but 3rd party addons may!
	gmsgSayText = REG_USER_MSG("SayText", -1);
	gmsgTextMsg = REG_USER_MSG("TextMsg", -1);
	gmsgWeaponList = REG_USER_MSG("WeaponList", -1);
	gmsgResetHUD = REG_USER_MSG("ResetHUD", 1); // called every respawn
	gmsgInitHUD = REG_USER_MSG("InitHUD", 0);	// called every time a new player joins the server
	gmsgShowGameTitle = REG_USER_MSG("GameTitle", 1);
	gmsgDeathMsg = REG_USER_MSG("DeathMsg", -1);
	gmsgScoreInfo = REG_USER_MSG("ScoreInfo", 9);
	gmsgTeamInfo = REG_USER_MSG("TeamInfo", -1);   // sets the name of a player's team
	gmsgTeamScore = REG_USER_MSG("TeamScore", -1); // sets the score of a team on the scoreboard
	gmsgGameMode = REG_USER_MSG("GameMode", 1);
	gmsgMOTD = REG_USER_MSG("MOTD", -1);
	gmsgServerName = REG_USER_MSG("ServerName", -1);
	gmsgAmmoPickup = REG_USER_MSG("AmmoPickup", 2);
	gmsgWeapPickup = REG_USER_MSG("WeapPickup", 1);
	gmsgItemPickup = REG_USER_MSG("ItemPickup", -1);
	gmsgHideWeapon = REG_USER_MSG("HideWeapon", 1);
	gmsgSetFOV = REG_USER_MSG("SetFOV", 1);
	gmsgShowMenu = REG_USER_MSG("ShowMenu", -1);
	gmsgShake = REG_USER_MSG("ScreenShake", sizeof(ScreenShake));               // 28
	gmsgFade = REG_USER_MSG("ScreenFade", sizeof(ScreenFade));                  // 29 (registered-but-never-sent; TSFade carries the fades)
	gmsgTSFade = REG_USER_MSG("TSFade", -1);                                    // 30
	gmsgAmmoX = REG_USER_MSG("AmmoX", 2);                                       // 31
	gmsgTeamNames = REG_USER_MSG("TeamNames", -1);                              // 32
	gmsgVGUIMenu = REG_USER_MSG("VGUIMenu", 1);                                 // 33 BYTE menuCode (open a client menu)
	gmsgWeaponInfo = REG_USER_MSG("WeaponInfo", 6);                             // 34
	gmsgClipInfo = REG_USER_MSG("ClipInfo", 1);                                 // 35 registered, never sent (the arsenal grid rides WStatus)
	gmsgTSWeaponStatus = REG_USER_MSG("WStatus", 3);                            // 36
	gmsgTSActiveItems = REG_USER_MSG("ActItems", 2);                            // 37
	gmsgKFuPower = REG_USER_MSG("KFuPower", 1);                                 // 38
	gmsgTSHealth = REG_USER_MSG("TSHealth", 1);                                 // 39
	gmsgTSArmor = REG_USER_MSG("TSArmor", 1);                                   // 40
	gmsgTSState = REG_USER_MSG("TSState", 1);                                   // 41
	gmsgTSCash = REG_USER_MSG("TSCash", 2);                                     // 42
	gmsgTSSpace = REG_USER_MSG("TSSpace", 1);                                   // 43
	gmsgTSNextStunt = REG_USER_MSG("NextStunt", 1);                             // 44
	gmsgTSPowerUp = REG_USER_MSG("PwUp", 3);                                    // 45
	gmsgTSPowerUpTime = REG_USER_MSG("PwUpTime", 1);                            // 46
	gmsgTSRoundTime = REG_USER_MSG("RoundTime", 2);                             // 47
	gmsgTSObjective = REG_USER_MSG("Objective", -1);                            // 48
	gmsgTSShowObjective = REG_USER_MSG("ShowObj", 2);                           // 49
	gmsgTSWires = REG_USER_MSG("Wires", 3);                                     // 50
	gmsgTSServerSetting = REG_USER_MSG("SrvSett", 3);                           // 51
	gmsgTSPlayerAward = REG_USER_MSG("TSPAward", 2);                            // 52
	gmsgTSBulletTime = REG_USER_MSG("TSBTime", 1);                              // 53
	gmsgTSVoiceMessage = REG_USER_MSG("TSHVoice", 1);                           // 54 (the wire name IS "TSHVoice")
	gmsgTSHaveFlag = REG_USER_MSG("TSFlag", 4);                                 // 55
	gmsgTSCustomWeaponParams = REG_USER_MSG("CustomWP", -1);                    // 56
	gmsgSpectator = REG_USER_MSG("Spectator", 2);                               // 57 BYTE state, BYTE secondsLeft (VERIFY: original = stock (idx,flag))
	gmsgTSMotionBlur = REG_USER_MSG("TSMBlur", 2);                              // 58
	gmsgTSMessage = REG_USER_MSG("TSMessage", -1);                              // 59
	gmsgTSSlowAmt = REG_USER_MSG("TSSlowMo", 2);                                // 60
	gmsgTSTakeDamage = REG_USER_MSG("PTakeDam", 13);                            // 61 (the stock Damage message above stays registered-but-never-sent)
	gmsgTSCreateDebris = REG_USER_MSG("TSFrags", -1);                           // 62 (the wire name IS "TSFrags", not "TSCreateDebris")
	gmsgTSPlayerInfo = REG_USER_MSG("PlInfo", 3);                               // 63
	gmsgTSParticleSystem = REG_USER_MSG("TSPSys", -1);                          // 64

	// (StatusText/StatusValue/Weapons/Flashlight/FlashBat NOT registered — proven absent from
	// the original's registration block, dossier-gmsg U0 + star_LinkUserMessages.md. The stock
	// sends are neutered in player.cpp; the HLU 64-bit weapon-bits message removal is
	// PLAYTEST-GATED.)
}

// Declared-only DEAD globals in the original (present in .data, ZERO references — no relocs,
// absent from the registration block): symbol parity only, never registered, never sent.
int gmsgFlashlight = 0;
int gmsgFlashBattery = 0;
int gmsgSetCurWeap = 0;
