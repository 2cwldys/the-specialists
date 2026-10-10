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
#include "in_buttons.h"
#include "../dlls/ts_weapon_info.h" // get_weapon_info, get_ammo_info

#include <stdio.h>

extern "C" int CL_IsThirdPerson(); // in_camera.cpp (engine export, C linkage)
void CAM_ToThirdPerson();          // in_camera.cpp — the `thirdperson`/`firstperson` toggles
void CAM_ToFirstPerson();

DECLARE_MESSAGE(m_TSAmmo, WeaponInfo)
DECLARE_MESSAGE(m_TSAmmo, KFuPower)

bool CHudTSAmmo::Init()
{
	m_iWeaponType = -1;
	m_iClip = 0;
	m_iReserve = 0;
	m_iFireMode = 1;
	m_iEquip = 0;
	m_hInstalled = 0;
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
	// The attachment rows' dot (weapon bar +0x144, loaded with the weapon sprites at 0x1004cbb5);
	// handles go stale across a map change, so it is reloaded here.
	m_hInstalled = SPR_Load("sprites/weapons/item_installed.spr");
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

	// cl_thirdpersonswitch (the WeaponInfo handler 0x1004f93b/0x1004f958): exactly 1.0 turns third
	// person on with kung fu (type 0) and off again on leaving it, both through the thirdperson toggle
	// (0x1004fe00, CAM_ToThirdPerson), which does nothing while spectating in first person.
	{
		static cvar_t* s_tpswitch = nullptr;
		if (s_tpswitch == nullptr)
			s_tpswitch = gEngfuncs.pfnGetCvarPointer("cl_thirdpersonswitch");
		if (s_tpswitch != nullptr && s_tpswitch->value == 1.0f)
		{
			if (m_iWeaponType == 0 && 0 == CL_IsThirdPerson())
				CAM_ToThirdPerson();
			else if (m_iWeaponType > 0 && 0 != CL_IsThirdPerson() && oldType == 0)
				CAM_ToThirdPerson();
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
	// The powerup double tap (0x1004ce5b): with cl_usepowerup not 0 and a powerup held, a new +use press
	// (the HUD's key bits against the last think's) within 0.3 s of the one before sends usepowerup.
	// The last bits are kept every think.
	{
		static cvar_t* s_usepowerup = nullptr;
		if (s_usepowerup == nullptr)
			s_usepowerup = gEngfuncs.pfnGetCvarPointer("cl_usepowerup");
		if (s_usepowerup != nullptr && s_usepowerup->value != 0.0f && gHUD.m_TSPowerUp.HeldFlags() != 0 &&
			((m_iLastKeyBits ^ gHUD.m_iKeyBits) & gHUD.m_iKeyBits & IN_USE) != 0)
		{
			if (gHUD.m_flTime <= m_flUseDeadline)
				gEngfuncs.pfnClientCmd("usepowerup");
			m_flUseDeadline = gHUD.m_flTime + 0.3f;
		}
		m_iLastKeyBits = gHUD.m_iKeyBits;
	}

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

	// The fire mode switch (0x1004d340): alive and playing with a weapon (type 1 and up), a fire mode
	// other than the one last heard plays weapons/switch.wav on the local player, channel 3, volume
	// 0.92..1, attenuation 1.25, the breathing's pitch.
	if (!TSHud_PlayerPartsHidden() && m_iWeaponType > 0 && m_iLastFireMode != m_iFireMode)
	{
		extern float g_flClientSlow;
		const int pitch = gEngfuncs.pfnRandomLong(-5, 5) + 25 - (int)(g_flClientSlow * -75.0f);
		cl_entity_t* lp = gEngfuncs.GetLocalPlayer();
		if (lp)
			gEngfuncs.pEventAPI->EV_PlaySound(lp->index, lp->origin, CHAN_ITEM, "weapons/switch.wav",
				gEngfuncs.pfnRandomFloat(0.92f, 1.0f), 1.25f, 0, pitch);
		m_iLastFireMode = m_iFireMode;
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
	if ((m_iFlags & HUD_ACTIVE) == 0 || m_iWeaponType < 0 || m_iWeaponType >= TSW_COUNT)
		return true;

	if (gHUD.m_iHideHUDDisplay & (HIDEHUD_WEAPONS | HIDEHUD_ALL))
		return true;

	// Both boxes come after the weapon bar Draw's nohud test (0x1004d3f9) and its dead and spectating
	// tests (0x1004c250, 0x1004c280). With kung fu (type 0) the
	// weapon bar Draw calls 0x1004d700, which has no cl_realhud test; otherwise 0x1004d7e0, which
	// returns at once unless cl_realhud is 0.
	if (TSHud_NoHud() || TSHud_PlayerPartsHidden())
		return true;
	if (m_iWeaponType == 0)
	{
		DrawKungFu();
		return true;
	}
	if (TSHud_RealHud())
		return true;

	// client.dll 0x1004d7e0: a 128 wide box at x = ScreenWidth - 136, rows 20 tall stacked upward from
	// ScreenHeight - 8, each backed by a 128 x 19 fill of the HUD colour. Bottom to top: the equipped
	// attachments, then for a gun the ammo, fire mode and caliber rows, for a weapon without a caliber
	// the melee bar and (for the stackable knives and the M61) the count; the name row on top.
	int hr, hg, hb;
	TSHud_Color(hr, hg, hb);
	const int tr = hr * 2 / 3, tg = hg * 2 / 3, tb = hb * 2 / 3; // text: two thirds of the HUD colour
	const int SW = ScreenWidth;
	const int x = SW - 0x88;
	const int LINEH = 0x14;
	int y = ScreenHeight - 8;
	const s_weapon_info* wi = get_weapon_info((tsweapon_t)m_iWeaponType);

	// The attachments, silencer at the bottom: an active one (the local player's ActItems bit, which
	// the weapon bar Draw copies to +0x14 each frame) gets a fill of alpha 0x2d instead of 0x0f and
	// its item_installed dot in the full colour instead of half.
	static const char* const s_attach[4] = {"#HUD_Silencer", "#HUD_Lasersight", "#HUD_Flashlight", "#HUD_Scope"};
	const int active = TS_LocalActiveAttach();
	for (int i = 0; i < 4; i++)
	{
		if ((m_iEquip & (1 << i)) == 0)
			continue;
		const bool on = (active & (1 << i)) != 0;
		y -= LINEH;
		FillRGBA(x, y, 0x80, LINEH - 1, hr, hg, hb, on ? 0x2d : 0x0f);
		TSHud_DrawString(x + 2, y, SW, CHudTextMessage::BufferedLocaliseTextString(s_attach[i]), hr / 2, hg / 2, hb / 2);
		if (on)
			SPR_Set(m_hInstalled, hr, hg, hb);
		else
			SPR_Set(m_hInstalled, hr / 2, hg / 2, hb / 2);
		SPR_DrawAdditive(0, x + 0x70, y, nullptr);
	}

	y -= LINEH;
	FillRGBA(x, y, 0x80, LINEH - 1, hr, hg, hb, 0x0f);
	if (wi->iCaliber == 0)
	{
		TSHud_MeleeBar(x, y, m_flMeleeBar);
		y -= LINEH;
		if (wi->iStackable != 0)
		{
			const bool lo = m_iClip <= 0;
			TSHud_DrawNumber(x + 94, y, TSHN_DRAWZERO | TSHN_3DIGITS, m_iClip, lo ? 195 : tr, lo ? 45 : tg, lo ? 45 : tb);
		}
	}
	else
	{
		// The ammo row: each number in two thirds of the HUD colour, (195, 45, 45) at 0, the slash in a
		// quarter of it. client.dll draws the clip three digits wide at x+2, the slash at x+64 and the
		// reserve three digits wide at x+94; reTS centres each number in its half of the row and the
		// slash in the row (Steven's layout, 1cf4923).
		const int slashW = gHUD.m_scrinfo.charWidths[(unsigned char)'/'];
		const int slashX = x + 0x40 - slashW / 2;
		const bool loClip = m_iClip <= 0, loRes = m_iReserve <= 0;
		TSHud_DrawNumber((x + 2 + slashX) / 2 - TSHud_NumberWidth(m_iClip) / 2, y, TSHN_DRAWZERO, m_iClip,
			loClip ? 195 : tr, loClip ? 45 : tg, loClip ? 45 : tb);
		TSHud_DrawString(slashX, y, SW, "/", hr / 4, hg / 4, hb / 4);
		TSHud_DrawNumber((slashX + slashW + x + 0x7e) / 2 - TSHud_NumberWidth(m_iReserve) / 2, y, TSHN_DRAWZERO, m_iReserve,
			loRes ? 195 : tr, loRes ? 45 : tg, loRes ? 45 : tb);

		// Fire mode (0x1004dd74: modes 1..5, anything else Full Auto), then the caliber's name.
		static const char* const s_mode[6] = {"#HUD_FullAuto", "#HUD_SemiAuto", "#HUD_Burst", "#HUD_PumpAction", "#HUD_FreeSemi", "#HUD_FreeFull"};
		const int mode = (m_iFireMode >= 1 && m_iFireMode <= 5) ? m_iFireMode : 0;
		y -= LINEH;
		FillRGBA(x, y, 0x80, LINEH - 1, hr, hg, hb, 0x0f);
		TSHud_DrawString(x + 2, y, SW, CHudTextMessage::BufferedLocaliseTextString(s_mode[mode]), tr, tg, tb);
		y -= LINEH;
		FillRGBA(x, y, 0x80, LINEH - 1, hr, hg, hb, 0x0f);
		TSHud_DrawString(x + 2, y, SW, get_ammo_info(wi->iCaliber)->szName, tr, tg, tb);
	}

	// The name row, alpha 0x23: the weapon table's name (weapon info +0x44).
	y -= LINEH;
	FillRGBA(x, y, 0x80, LINEH - 1, hr, hg, hb, 0x23);
	TSHud_DrawString(x + 2, y, SW, wi->pszName, tr, tg, tb);

	// Zoomed below a field of view of 40, "<80 / fov>x" at (24, 24) in the full colour (0x1004dfb8).
	if (gHUD.m_iFOV < 40 && gHUD.m_iFOV > 0)
	{
		char buf[16];
		sprintf(buf, "%ix", 80 / gHUD.m_iFOV);
		TSHud_DrawString(0x18, 0x18, SW, buf, hr, hg, hb);
	}

	return true;
}

// client.dll 0x1004d700, kung fu's box: the melee bar at (ScreenWidth - 136, ScreenHeight - 28), the
// name row above it (alpha 0x23) with "Kung-Fu" in the full HUD colour.
void CHudTSAmmo::DrawKungFu()
{
	int hr, hg, hb;
	TSHud_Color(hr, hg, hb);
	const int x = ScreenWidth - 0x88;
	TSHud_MeleeBar(x, ScreenHeight - 0x1c, m_flMeleeBar);
	FillRGBA(x, ScreenHeight - 0x30, 0x80, 0x13, hr, hg, hb, 0x23);
	TSHud_DrawString(ScreenWidth - 0x86, ScreenHeight - 0x30, ScreenWidth, "Kung-Fu", hr, hg, hb);
}
