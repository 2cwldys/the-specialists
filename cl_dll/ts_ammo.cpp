/***
*
*	Copyright (c) 1996-2002, Valve LLC. All rights reserved.
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
//
// ts_ammo.cpp
//
// The Specialists ammo HUD (the bottom-right weapon box). Reads the server's
// "WeaponInfo" message — { BYTE weaponType, BYTE clip, SHORT reserve, BYTE fireMode,
// BYTE flags } — byte-faithful to CTSGun::UpdateClientData (see
// reference/HUD_MESSAGES.md). Draws the original's stacked box: weapon name, caliber,
// fire-mode, then "clip / reserve" (matching the shipped HUD's Five-seveN / MP5K
// readout). TS has no stock CurWeapon/Ammo HUD, so this is the custom replacement.
//

#include "hud.h"
#include "cl_util.h"
#include "parsemsg.h"
#include "ts_hud.h"
#include "ts_attachments.h" // TS_LocalActiveAttach — which equipped attachments are toggled ON
#include "event_api.h"      // pEventAPI->EV_PlaySound (the breathing loop; PlaySoundByName is unreliable)
#include "../dlls/ts_weapon_info.h" // get_weapon_info (the melee-bar byte routing)

#include <stdio.h>

extern "C" int CL_IsThirdPerson(); // in_camera.cpp (engine export, C linkage)
void CAM_ToThirdPerson();          // in_camera.cpp — the `thirdperson`/`firstperson` toggles
void CAM_ToFirstPerson();

DECLARE_MESSAGE(m_TSAmmo, WeaponInfo)
DECLARE_MESSAGE(m_TSAmmo, KFuPower)

// Display name per tsweapon_t (the table index = the wire weaponType). Index 0 is the
// unarmed / kung-fu "weapon"; 29 is the CTF flag (no HUD). Names follow the shipped
// HUD ("Five-seveN", "MP5K", "Combat Knife", "Kung-Fu").
static const char* const s_weaponNames[] = {
	/* 0*/ "Kung-Fu",        /* 1*/ "Glock-18",       /* 2*/ "Beretta 92F",
	/* 3*/ "Mini-Uzi",       /* 4*/ "Benelli M3",     /* 5*/ "M4",
	/* 6*/ "MP5SD",          /* 7*/ "MP5K",           /* 8*/ "Akimbo Berettas",
	/* 9*/ "SOCOM-MK23",     /*10*/ "Akimbo MK23",    /*11*/ "USAS-12",
	/*12*/ "Desert Eagle",   /*13*/ "AK-47",          /*14*/ "Five-seveN",
	/*15*/ "Steyr AUG",      /*16*/ "Akimbo Mini-Uzi",/*17*/ "Skorpion",
	/*18*/ "Barrett M82A1",  /*19*/ "PDW",            /*20*/ "SPAS-12",
	/*21*/ "Golden Colts",   /*22*/ "Glock-20C",      /*23*/ "UMP",
	/*24*/ "M61 Grenade",    /*25*/ "Combat Knife",   /*26*/ "Mossberg 500",
	/*27*/ "M16",            /*28*/ "Ruger-MK1",      /*29*/ "",
	/*30*/ "Akimbo Five-seveN",/*31*/ "Raging Bull",  /*32*/ "M60",
	/*33*/ "Sawed-off",      /*34*/ "Katana",         /*35*/ "SEAL Knife",
	/*36*/ "Contender G2",   /*37*/ "Akimbo Skorpions",
};
static const int s_numWeapons = sizeof(s_weaponNames) / sizeof(s_weaponNames[0]);

// Per-weapon caliber index (s_weapon_info+0x8, recovered server-side; mirrored here for
// the cartridge label). 0 = no cartridge (melee/grenade).
static const int s_weaponCaliber[] = {
	/* 0*/ 0, /* 1*/ 1, /* 2*/ 1, /* 3*/ 1, /* 4*/ 3, /* 5*/ 4, /* 6*/ 1, /* 7*/ 1,
	/* 8*/ 1, /* 9*/ 2, /*10*/ 2, /*11*/ 3, /*12*/ 5, /*13*/ 6, /*14*/ 7, /*15*/ 4,
	/*16*/ 1, /*17*/12, /*18*/ 8, /*19*/ 7, /*20*/ 3, /*21*/ 2, /*22*/ 9, /*23*/ 2,
	/*24*/ 0, /*25*/ 0, /*26*/ 3, /*27*/ 4, /*28*/10, /*29*/ 0, /*30*/ 7, /*31*/11,
	/*32*/ 4, /*33*/ 3, /*34*/ 0, /*35*/ 0, /*36*/13, /*37*/12,
};

// Cartridge name per caliber index. 1/7 confirmed against the shipped HUD (9 x 19mm /
// 5.7 x 28); the strings come from the .so's ammo registry. The rarer ones (8/10/13)
// are best-effort // VERIFY against the original.
static const char* const s_caliberNames[] = {
	/* 0*/ "",            /* 1*/ "9 x 19mm",    /* 2*/ ".45 ACP",
	/* 3*/ "12 Gauge",    /* 4*/ "5.56 Nato",   /* 5*/ ".50 AE",
	/* 6*/ "7.62 x 39mm", /* 7*/ "5.7 x 28",    /* 8*/ "7.62 x 51mm",
	/* 9*/ "10mm AUTO",   /*10*/ ".22 LR",      /*11*/ ".454 Casull",
	/*12*/ ".32 ACP",     /*13*/ ".45-70",
};
static const int s_numCalibers = sizeof(s_caliberNames) / sizeof(s_caliberNames[0]);

// Fire-mode labels, indexed by the WeaponInfo fireMode byte (tsfiremode_mode_t). RE'd from the
// client.dll ammo-panel switch (FUN_1004a3e0-adjacent, #HUD_* tokens @0x95e20..): 0 Full Auto,
// 1 Semi Auto, 2 Burst, 3 Pump Action, 4 Free Semi, 5 Free Full. "Free" = the akimbo per-barrel
// firemode (click to fire the respective gun): akimbo pistols toggle Semi Auto<->Free Semi, akimbo
// SMGs (uzi) toggle Full Auto<->Free Full. NOT "Akimbo" (that was wrong).
static const char* const s_fireModeNames[] = {"Full Auto", "Semi Auto", "Burst", "Pump Action", "Free Semi", "Free Full"};
static const int s_numFireModes = sizeof(s_fireModeNames) / sizeof(s_fireModeNames[0]);

// Melee/grenade weapons (no magazine) show just the name — no caliber/clip/reserve.
static bool TSWeaponHasAmmo(int type)
{
	return !(type == 0 || type == 24 || type == 25 || type == 34 || type == 35);
}

bool CHudTSAmmo::Init()
{
	m_iWeaponType = -1;
	m_iClip = 0;
	m_iReserve = 0;
	m_iFireMode = 1;
	m_iEquip = 0;
	m_hInstalled = 0;
	m_hNotInstalled = 0;
	m_flMeleeBar = 100.0f;

	HOOK_MESSAGE(WeaponInfo);
	HOOK_MESSAGE(KFuPower);

	TSHud_RegisterCvars();

	gHUD.AddHudElem(this);

	return true;
}

bool CHudTSAmmo::VidInit()
{
	m_flMeleeBar = 100.0f; // @0x1004cd2d: a new map starts the melee bar full
	// The attachment-row on/off dot the original draws (far right of each row). Re-resolved each map
	// (sprite handles go stale on a load — see [[map-change-state-reset]]).
	m_hInstalled = SPR_Load("sprites/weapons/item_installed.spr");
	m_hNotInstalled = SPR_Load("sprites/weapons/item_not_installed.spr");
	m_rcInstalled = {0, 0, 0, 0};
	m_rcNotInstalled = {0, 0, 0, 0};
	if (m_hInstalled)
	{
		m_rcInstalled.right = SPR_Width(m_hInstalled, 0);
		m_rcInstalled.bottom = SPR_Height(m_hInstalled, 0);
	}
	if (m_hNotInstalled)
	{
		m_rcNotInstalled.right = SPR_Width(m_hNotInstalled, 0);
		m_rcNotInstalled.bottom = SPR_Height(m_hNotInstalled, 0);
	}
	return true;
}

void CHudTSAmmo::Reset()
{
	// Keep drawing across respawns; the server resends WeaponInfo on weapon deploy.
	m_iFlags |= HUD_ACTIVE;
}

// WeaponInfo (client.dll @0x1004f800): the type (a failed read keeps the last one), the clip, the
// reserve, the fire mode and the equipped attachments, all stored as they come; the clip byte of a
// kung fu WeaponInfo (the server's melee bar) lands in the clip too, which is not drawn for it. A
// weapon (type 1 and up) also gets its weapon bar cell with that clip. Between these the predicted
// gun keeps the clip, fire mode and attachments (GunUpdate below).
bool CHudTSAmmo::MsgFunc_WeaponInfo(const char* pszName, int iSize, void* pbuf)
{
	BEGIN_READ(pbuf, iSize);
	const int oldType = m_iWeaponType; // the previous weapon type — cl_thirdpersonswitch keys off it
	const int type = READ_BYTE();
	if (type != -1)
		m_iWeaponType = type;
	m_iClip = READ_BYTE();
	m_iReserve = READ_SHORT();
	m_iFireMode = READ_BYTE();
	m_iEquip = READ_BYTE(); // EQUIPPED attachment mask (silencer 1 / laser 2 / flash 4 / scope 8)
	if (m_iWeaponType >= 1)
		gHUD.m_TSWeaponBar.SetClip(m_iWeaponType, m_iClip, true);

	g_iTSActiveWeapon = m_iWeaponType; // the weapon-select bar highlights this slot

	// cl_thirdpersonswitch (dossier cont94 §8 — the WeaponInfo handler FUN_1004f800 @0x4f93b/0x4f958):
	// auto third-person while on BARE HANDS (type 0 = kung fu), back to first person on leaving them.
	// The test is the original's BIT-EXACT == 1.0f — value 2 must NOT trigger. The original's toggle
	// (FUN_1004fe00) skips while spectating; VERIFY our CAM_To*Person no-ops match there.
	{
		static cvar_t* s_tpswitch = nullptr;
		if (s_tpswitch == nullptr)
			s_tpswitch = gEngfuncs.pfnGetCvarPointer("cl_thirdpersonswitch");
		if (s_tpswitch != nullptr && s_tpswitch->value == 1.0f)
		{
			if (m_iWeaponType == 0 && 0 == CL_IsThirdPerson())
				CAM_ToThirdPerson();
			else if (m_iWeaponType > 0 && 0 != CL_IsThirdPerson() && oldType == 0)
				CAM_ToFirstPerson();
		}
	}

	m_iFlags |= HUD_ACTIVE;
	return true;
}

// KFuPower (client.dll @0x1004f6f0) — the melee block bar, sent when a blow drains it: a signed byte,
// kept within -100..100. Think regenerates it in between. Drawn as the bottom weapon-box bar, but only
// while a melee weapon is held.
bool CHudTSAmmo::MsgFunc_KFuPower(const char* pszName, int iSize, void* pbuf)
{
	BEGIN_READ(pbuf, iSize);
	m_flMeleeBar = (float)READ_CHAR();
	if (m_flMeleeBar < -100.0f)
		m_flMeleeBar = -100.0f;
	else if (m_flMeleeBar > 100.0f)
		m_flMeleeBar = 100.0f;
	m_iFlags |= HUD_ACTIVE;
	return true;
}

static const int TS_SND_CHANGE_VOL = 1 << 6; // SND_CHANGE_VOL (dlls/util.h), the 0x40 the original passes

// The melee bar regenerates on the client as on the server (client.dll Think @0x1004ceef-0x1004cf53):
// below 100 it gains 15 a second, or 1 a second while below -0.001, in the time the local slow motion
// leaves (the frame time scaled by the eased slow factor, at least 0.01), up to 100.
void CHudTSAmmo::Think()
{
	if (m_flMeleeBar < 100.0f)
	{
		extern float g_flClientSlow; // ts_powerup_hud.cpp — the eased local slow factor
		float dt = (float)gHUD.m_flTimeDelta * (g_flClientSlow < 0.01f ? 0.01f : g_flClientSlow);
		if (m_flMeleeBar >= -0.001f)
			dt *= 15.0f;
		m_flMeleeBar += dt;
		if (m_flMeleeBar > 100.0f)
			m_flMeleeBar = 100.0f;
	}

	// The heartbeat (client.dll Think @0x1004cf8b-0x1004d0e4), the same machine on the local slow
	// factor: while it is under 1, heartbeat1.wav starts at volume 1.0; back at 1 it fades, 0.01 a
	// frame, re-sent with SND_CHANGE_VOL, and stops at 0. Slowing again while it fades starts it again
	// at 1.0. Channel 501, attenuation 0.8, the breathing's pitch.
	extern float g_flClientSlow; // ts_powerup_hud.cpp — the eased local slow factor
	if (g_flClientSlow < 1.0f)
	{
		if (m_iHeartState == 0 || m_iHeartState == 1)
		{
			m_flHeartVolume = 1.0f;
			PlayLoop(501, "player/heartbeat1.wav", m_flHeartVolume, m_iHeartState == 1 ? TS_SND_CHANGE_VOL : 0);
			m_iHeartState = 2;
		}
	}
	else if (m_iHeartState == 2)
		m_iHeartState = 1;
	if (m_iHeartState == 1)
	{
		m_flHeartVolume -= 0.01f;
		if (m_flHeartVolume <= 0.0f)
			m_iHeartState = 0;
		else
			PlayLoop(501, "player/heartbeat1.wav", m_flHeartVolume, TS_SND_CHANGE_VOL);
	}

	// The breathing (client.dll Think @0x1004d0ee-0x1004d24c), whatever is in his hands: below 50 on
	// the melee bar breathe2.wav starts at 0.7 (it is loop-marked, so it keeps going); back at 50 it
	// fades, 0.005 a frame, re-sent with SND_CHANGE_VOL, and stops at 0. Falling below 50 while it
	// fades starts it again at 0.7. Channel 502, attenuation 0.8, pitch 25 + 75 x the slow factor,
	// +-5.
	if (m_flMeleeBar < 50.0f)
	{
		if (m_iBreathState == 0 || m_iBreathState == 1)
		{
			m_flBreathVolume = 0.7f;
			PlayLoop(502, "player/breathe2.wav", m_flBreathVolume, m_iBreathState == 1 ? TS_SND_CHANGE_VOL : 0);
			m_iBreathState = 2;
		}
	}
	else if (m_iBreathState == 2)
		m_iBreathState = 1;
	if (m_iBreathState == 1)
	{
		m_flBreathVolume -= 0.005f;
		if (m_flBreathVolume <= 0.0f)
			m_iBreathState = 0;
		else
			PlayLoop(502, "player/breathe2.wav", m_flBreathVolume, TS_SND_CHANGE_VOL);
	}
}

// The heartbeat's and the breathing's sound: on the local player, attenuation 0.8, pitch 25 + 75 x
// the slow factor, +-5.
void CHudTSAmmo::PlayLoop(int channel, const char* sample, float volume, int flags)
{
	extern float g_flClientSlow; // ts_powerup_hud.cpp — the eased local slow factor
	const int pitch = gEngfuncs.pfnRandomLong(-5, 5) + 25 - (int)(g_flClientSlow * -75.0f);
	cl_entity_t* lp = gEngfuncs.GetLocalPlayer();
	if (!lp)
		return;
	gEngfuncs.pEventAPI->EV_PlaySound(lp->index, lp->origin, channel, sample, volume, 0.8f, flags, pitch);
}

// The client's CTSGun::UpdateClientData with a weapon out (client.dll @0x100581c0): the predicted
// clip, fire mode and attachments.
void CHudTSAmmo::GunUpdate(int clip, int fireMode, int equip)
{
	m_iClip = clip;
	m_iFireMode = fireMode;
	m_iEquip = equip;
}

// Called by the client CTSGun::UpdateClientData (dlls/ts_gun.cpp): the ammo HUD, then the weapon bar
// cells of the weapon, of its akimbo variant and of its single base (when not the weapon itself).
void TSHud_GunUpdate(int type, int clip, int fireMode, int equip, int akimboType, int akimboClip,
	int singleType, int singleClip)
{
	gHUD.m_TSAmmo.GunUpdate(clip, fireMode, equip);
	gHUD.m_TSWeaponBar.SetClip(type, clip, true);
	if (akimboType != -1)
		gHUD.m_TSWeaponBar.SetClip(akimboType, akimboClip, false);
	if (singleType != -1 && singleType != type)
		gHUD.m_TSWeaponBar.SetClip(singleType, singleClip, false);
}

bool CHudTSAmmo::Draw(float flTime)
{
	if ((m_iFlags & HUD_ACTIVE) == 0 || m_iWeaponType < 0 || m_iWeaponType >= s_numWeapons)
		return true;

	if (gHUD.m_fPlayerDead || (gHUD.m_iHideHUDDisplay & (HIDEHUD_WEAPONS | HIDEHUD_ALL)))
		return true;

	const char* szName = s_weaponNames[m_iWeaponType];
	if (!szName || !szName[0])
		return true;

	// Exact geometry recovered from FUN_1004d7e0 (reference/HUD_DRAWCHAIN.md): a
	// right-anchored box, 0x80 (128px) wide, x = ScreenWidth - 0x88 (136); lines are
	// 0x14 (20px) tall and stack UPWARD from y = ScreenHeight - 8 (each line drops y
	// by 20 before drawing). Each strip is a FillRGBA of the HUD colour at alpha 0x0f.
	// Text is inset +2px; descriptive lines (caliber/fire-mode) draw at half
	// brightness, the name + ammo numbers at full.
	const int BOXW = 0x80;          // 128
	const int LINEH = 0x14;         // 20
	const int x = ScreenWidth - 0x88;
	const bool hasAmmo = TSWeaponHasAmmo(m_iWeaponType);

	int hr, hg, hb;
	TSHud_Color(hr, hg, hb);
	// (No dimmed label tint: the original draws caliber + fire-mode at the FULL hud color, not
	// half — RE'd from the ammo-panel draw (firemode string drawn at the raw hud-color ftol, no /2),
	// user-confirmed against the original. Our earlier half-brightness note was wrong.)

	// The box, top→bottom (matching the reference image): name, caliber, fire-mode, ammo, then one line
	// per EQUIPPED attachment (dot bullet + label), then the bar at the very bottom. The active
	// attachments (laser/flash toggled ON via `useitems`) are drawn at full brightness with a bright dot;
	// inactive ones are dimmed. Silencer/scope are always-on when equipped.
	enum LineKind { LK_TEXT, LK_AMMO, LK_ATTACH, LK_COUNT };
	struct Line { LineKind kind; const char* s; int r, g, b; bool light; bool active; };
	Line lines[10];
	int n = 0;

	lines[n++] = {LK_TEXT, szName, hr, hg, hb, true, false}; // name (top) — lighter background (header)

	if (hasAmmo)
	{
		const int cal = s_weaponCaliber[m_iWeaponType];
		if (cal > 0 && cal < s_numCalibers && s_caliberNames[cal][0])
			lines[n++] = {LK_TEXT, s_caliberNames[cal], hr, hg, hb, false, false};

		const int mode = (m_iFireMode >= 0 && m_iFireMode < s_numFireModes) ? m_iFireMode : 1;
		lines[n++] = {LK_TEXT, s_fireModeNames[mode], hr, hg, hb, false, false};

		int ar = hr, ag = hg, ab = hb;
		if (m_iClip == 0)
		{
			UnpackRGB(ar, ag, ab, RGB_REDISH);
			ScaleColors(ar, ag, ab, 255);
		}
		lines[n++] = {LK_AMMO, nullptr, ar, ag, ab, false, false};
	}

	// Equipped attachments, in the reference order (top→bottom): Flashlight, Lasersight, Scope, Silencer.
	// The ACTIVE ones (laser/flash toggled on; silencer/scope always-on) get a LIGHTER background + the
	// item_installed dot; inactive ones a normal background + the item_not_installed dot. The TEXT
	// brightness stays constant — the highlight is the background + the dot, not the text.
	struct Att { int bit; const char* name; bool toggleable; };
	static const Att atts[] = {
		{4, "Flashlight", true}, {2, "Lasersight", true}, {8, "Scope", false}, {1, "Silencer", false}};
	const int activeMask = TS_LocalActiveAttach();
	for (const Att& a : atts)
	{
		if ((m_iEquip & a.bit) == 0)
			continue;
		const bool act = a.toggleable ? ((activeMask & a.bit) != 0) : true;
		lines[n++] = {LK_ATTACH, a.name, hr, hg, hb, act, act};
		if (n >= (int)(sizeof(lines) / sizeof(lines[0])))
			break;
	}

	// The weapons that count what he carries (weapon info +0xdc, set by Write_knife, Write_m61 and
	// Write_sealknife) show the CLIP as a number line just ABOVE the block bar (client.dll draw
	// @0x1004dabe); kung-fu/katana have no count. Not for guns (they use the ammo line).
	const bool knifeCount = (m_iWeaponType == 25 /*knife*/ || m_iWeaponType == 24 /*m61*/ || m_iWeaponType == 35 /*sealknife*/);
	if (knifeCount && n < (int)(sizeof(lines) / sizeof(lines[0])))
		lines[n++] = {LK_COUNT, nullptr, hr, hg, hb, false, false};

	// Every weapon without a caliber (weapon info +0x8 == 0: kung-fu, the M61, knife, katana, seal
	// knife) carries the block bar (client.dll draw @0x1004da71); guns have no bar.
	const bool meleeBar = s_weaponCaliber[m_iWeaponType] == 0;

	// Anchor the whole box bottom-right so the bar ends near ScreenHeight - 8 (the original's anchor).
	const int barH = meleeBar ? LINEH : 0;
	int y = ScreenHeight - 8 - (n * LINEH + barH);
	for (int i = 0; i < n; i++)
	{
		// Background panel — lighter (higher alpha) for the name row + active-attachment rows.
		FillRGBA(x, y, BOXW, LINEH - 1, hr, hg, hb, lines[i].light ? 0x2a : 0x0f);
		if (lines[i].kind == LK_AMMO)
		{
			// "[  clip  /  reserve  ]" — slash centred in the box, each number centred in its half.
			TSHud_DrawNumberPair(x + 2, x + BOXW - 2, y + (LINEH - 1 - TS_NUMBER_H) / 2, m_iClip, m_iReserve,
				lines[i].r, lines[i].g, lines[i].b);
		}
		else if (lines[i].kind == LK_ATTACH)
		{
			TSHud_DrawString(x + 4, y, x + BOXW, lines[i].s, lines[i].r, lines[i].g, lines[i].b);
			// on/off dot at the FAR RIGHT (item_installed = active, item_not_installed = off).
			const HSPRITE dot = lines[i].active ? m_hInstalled : m_hNotInstalled;
			const Rect* rc = lines[i].active ? &m_rcInstalled : &m_rcNotInstalled;
			if (dot)
			{
				const int dw = rc->right - rc->left, dh = rc->bottom - rc->top;
				SPR_Set(dot, 255, 255, 255);
				SPR_DrawAdditive(0, x + BOXW - dw - 3, y + (LINEH - 1 - dh) / 2, (Rect*)rc);
			}
		}
		else if (lines[i].kind == LK_COUNT)
		{
			// the carry count — a single number, centred in the box (like the ammo readout).
			const int cw = TSHud_NumberWidth(m_iClip);
			TSHud_DrawNumber(x + (BOXW - cw) / 2, y + (LINEH - 1 - TS_NUMBER_H) / 2, m_iClip,
				lines[i].r, lines[i].g, lines[i].b);
		}
		else
		{
			TSHud_DrawString(x + 2, y, x + BOXW, lines[i].s, lines[i].r, lines[i].g, lines[i].b);
		}
		y += LINEH;
	}

	// The MELEE BLOCK bar at the bottom of the box (the original's FUN_1004d5f0) — the block stamina the
	// server networks via gmsgKFuPower. Shown for every weapon without a caliber (above);
	// each RELOAD-block drains it, and it regens. NOT health, NOT shown on guns.
	if (meleeBar)
		TSHud_MeleeBar(x, y, m_flMeleeBar);

	return true;
}
