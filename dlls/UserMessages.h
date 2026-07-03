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

inline int gmsgShake = 0;
inline int gmsgFade = 0;
inline int gmsgResetHUD = 0;
inline int gmsgInitHUD = 0;
inline int gmsgShowGameTitle = 0;
inline int gmsgCurWeapon = 0;
inline int gmsgHealth = 0;
inline int gmsgDamage = 0;
inline int gmsgBattery = 0;
inline int gmsgTrain = 0;
inline int gmsgLogo = 0;
inline int gmsgWeaponList = 0;
inline int gmsgAmmoX = 0;
inline int gmsgHudText = 0;
inline int gmsgDeathMsg = 0;
inline int gmsgScoreInfo = 0;
inline int gmsgTeamInfo = 0;
inline int gmsgTeamScore = 0;
inline int gmsgGameMode = 0;
inline int gmsgMOTD = 0;
inline int gmsgServerName = 0;
inline int gmsgAmmoPickup = 0;
inline int gmsgWeapPickup = 0;
inline int gmsgItemPickup = 0;
inline int gmsgHideWeapon = 0;
extern int gmsgSetCurWeap; // DEAD in the original (declared-only, never registered/referenced) — defined in UserMessages.cpp for symbol parity
inline int gmsgSayText = 0;
inline int gmsgTextMsg = 0;
inline int gmsgSetFOV = 0;
inline int gmsgShowMenu = 0;
inline int gmsgGeigerRange = 0;
inline int gmsgTeamNames = 0;

// TS join/spectate flow (cont.65): Spectator drives the free-look spectate HUD
// (countdown + "Press fire to play"); VGUIMenu opens the buy/config menu on join.
inline int gmsgSpectator = 0;
inline int gmsgVGUIMenu = 0;

// Registered-but-never-sent originals (registration-set parity — dossier-gmsg U0):
inline int gmsgSelAmmo = 0;  // "SelAmmo" 4 — the FIRST registration + the already-linked guard
inline int gmsgClipInfo = 0; // "ClipInfo" 1 — original registers it; the grid rides WStatus

// "PTakeDam" 13 — the pain-direction/hit-feedback message (6 COORDs + BYTE encoded damage),
// sent MSG_PVS from CBasePlayer::TraceAttack (throttled 0.3 s). dossier-gmsg U4a.
inline int gmsgTSTakeDamage = 0;

// "TSFade" -1 — the TS screen fade: the stock ScreenFade layout but with duration+holdTime
// divided by the RECIPIENT's GiveSlowMul() (fades stretch for slowed players). The stock
// ScreenFade message stays registered-but-never-sent. dossier-gmsg U4b.
inline int gmsgTSFade = 0;

void LinkUserMessages();
