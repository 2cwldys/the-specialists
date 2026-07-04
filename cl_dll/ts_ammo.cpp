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

DECLARE_MESSAGE(m_TSAmmo, WeaponInfo)
DECLARE_MESSAGE(m_TSAmmo, KFuPower)

// Melee weapons (kung-fu / knife / katana / seal-knife) — only these show the block bar.
static bool TSIsMeleeType(int type)
{
	return type == 0 || type == 25 || type == 34 || type == 35;
}

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
	m_iMeleeBar = 100;

	HOOK_MESSAGE(WeaponInfo);
	HOOK_MESSAGE(KFuPower);

	TSHud_RegisterCvars();

	gHUD.AddHudElem(this);

	return true;
}

bool CHudTSAmmo::VidInit()
{
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

bool CHudTSAmmo::MsgFunc_WeaponInfo(const char* pszName, int iSize, void* pbuf)
{
	BEGIN_READ(pbuf, iSize);
	m_iWeaponType = READ_BYTE();
	int clipByte = READ_BYTE();
	m_iReserve = READ_SHORT();
	m_iFireMode = READ_BYTE();
	m_iEquip = READ_BYTE(); // EQUIPPED attachment mask (silencer 1 / laser 2 / flash 4 / scope 8)

	// The clip byte carries the MELEE BAR for melee weapons (the server mirrors the original
	// UpdateClientData's read of player+0x774) — this is how the HUD tracks the silent PreThink
	// regen and the respawn refill (KFuPower only fires on drains).
	const s_weapon_info *wi = get_weapon_info((tsweapon_t)m_iWeaponType);
	if (wi && (wi->iFireType == 3 || wi->iFireType == 10))
		m_iMeleeBar = clipByte;
	else
		m_iClip = clipByte;

	g_iTSActiveWeapon = m_iWeaponType; // the weapon-select bar highlights this slot

	m_iFlags |= HUD_ACTIVE;
	return true;
}

// KFuPower — the melee block bar (stamina 0..100), networked from CTSGun::UpdateClientData. Drawn as the
// bottom weapon-box bar, but only while a melee weapon is held.
bool CHudTSAmmo::MsgFunc_KFuPower(const char* pszName, int iSize, void* pbuf)
{
	BEGIN_READ(pbuf, iSize);
	m_iMeleeBar = READ_CHAR();
	if (m_iMeleeBar < 0)
		m_iMeleeBar = 0;
	m_iFlags |= HUD_ACTIVE;
	return true;
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

	// Exhausted breathing: while holding a melee weapon with LOW block stamina (<50%), loop the
	// breathe2.wav "gasping" clip — RE: client.dll plays player/breathe2.wav off the low KFuPower
	// melee bar (user-confirmed: below 50%). breathe2.wav is a single breath, so replay it on a timer
	// while the bar stays low. VERIFY the exact threshold/interval + whether guns keep breathing.
	{
		static float s_nextBreath = 0.0f;
		if (m_iMeleeBar < 50 && TSIsMeleeType(m_iWeaponType))
		{
			if (flTime >= s_nextBreath)
			{
				// EV_PlaySound on the local player, a FIXED channel (CHAN_VOICE), so each replay cleanly
				// REPLACES the previous breath — no overlap and no "third breath drops" (pfnPlaySoundByName
				// was unreliable, same as goslow — see ts_powerup_hud). Replay at the clip length (1.799s)
				// for back-to-back breaths.
				cl_entity_t* lp = gEngfuncs.GetLocalPlayer();
				if (lp)
					gEngfuncs.pEventAPI->EV_PlaySound(lp->index, lp->origin, CHAN_VOICE,
						"player/breathe2.wav", 1.0f, 0.8f, 0, 100);
				s_nextBreath = flTime + 1.8f; // = breathe2.wav length (1.799s)
			}
		}
		else
			s_nextBreath = 0.0f; // reset so it restarts promptly next time the bar drops below 50
	}

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
	enum LineKind { LK_TEXT, LK_AMMO, LK_ATTACH };
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

	// Only MELEE weapons (kung-fu/knife/katana/seal-knife) carry the block bar; guns have no bar.
	const bool meleeBar = TSIsMeleeType(m_iWeaponType);

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
		else
		{
			TSHud_DrawString(x + 2, y, x + BOXW, lines[i].s, lines[i].r, lines[i].g, lines[i].b);
		}
		y += LINEH;
	}

	// The MELEE BLOCK bar at the bottom of the box (the original's FUN_1004d5f0) — the block stamina the
	// server networks via gmsgKFuPower. Shown ONLY for melee weapons (kung-fu/knife/katana/seal-knife);
	// each RELOAD-block drains it, and it regens. NOT health, NOT shown on guns. (TSHud_HealthBar is just
	// the generic bar primitive — full = rested, low = tired/red.)
	if (meleeBar)
		TSHud_HealthBar(x, y, m_iMeleeBar);

	return true;
}
