/***
*  ts_reticle.cpp — The Specialists DIVE RETICLE. See ts_reticle.h.
*
*  RE: the original loads the reticle sprites in client.dll FUN_1004cb50 into a static object
*  (client.dll+0xd0ca0) at handle offsets +0x14c cross_e / +0x150 cross2 / +0x154 cross_t /
*  +0x158 divestept / +0x15c divestepc / +0x160 divestepb (confirmed via a live read: handles
*  0x3b..0x40). The per-frame draw (FUN_1003254e) decompiles unusably (Ghidra lost the frames),
*  so the EXACT per-segment colors/positions were taken from the user's screenshots + the dive
*  logic already RE'd server-side, and are tuned in the engine (playtest loop). The behaviour is
*  faithful: three left-arc steps = the three dive height tiers (CanDiveHigh/Med/Low = free slots
*  > 39/29/19, RE dlls/ts_stunt.cpp), blue when a tier is available, red when too heavy to dive,
*  with the step matching your current aim pitch highlighted (GoDive: pitch<-10 high / <=10 med /
*  else low). Weight is read client-side from the networked maxspeed (cont.69z slot speed:
*  maxspeed = 210 + (120/81)*freeSlots), pitch from the view angles.
***/
#include "hud.h"
#include "cl_util.h"
#include "parsemsg.h"   // NextStunt (the stunt-meter byte)
#include "cl_entity.h"  // cl_entity_t (GetLocalPlayer)
#include "pmtrace.h"    // pmtrace_t (laser distance trace)
#include "triangleapi.h" // pTriAPI->WorldToScreen (3rd-person reticle projection)
#include "ts_hud.h"     // TSHud_DrawNumber (numbers.spr)
#include "ts_attachments.h" // TS_LocalHasLaser
#include "ts_reticle.h"
#include "player_info.h" // g_PlayerExtraInfo (hit player's team, for the aim classifier)

#include <math.h>

extern Vector v_origin, v_angles; // view origin/angles (cl_dll/view.cpp) — for the aim trace
extern Vector v_aimeye;            // player eye (bullet origin), pre-3rd-person-offset (view.cpp)
extern "C" int CL_IsThirdPerson(); // in_camera.cpp (engine export, C linkage)

// ---- 3rd-person self-transparency: aim-at-player classifier (RE dossier cont91 §2) ----------
// The original client.dll (FUN_1004acc0, run once/frame from the reticle update) traces the aim
// and sets a tri-state flag: 0 = nothing/world, 1 = teammate under aim, 2 = enemy under aim.
// V_CalcNormalRefdef fades the LOCAL player's own model whenever this is != 0 (see view.cpp).
// This uses the SAME eye/forward/trace the 3rd-person reticle above uses (v_aimeye + view angles).
int g_iTSAimTarget = 0; // 0 none / 1 teammate / 2 enemy — transparency fires on != 0

void TS_UpdateAimTarget()
{
	g_iTSAimTarget = 0;
	float va[3];
	gEngfuncs.GetViewAngles(va);
	Vector fwd;
	gEngfuncs.pfnAngleVectors(va, fwd, NULL, NULL);
	Vector eye = v_aimeye; // the true eye / bullet origin (pre-3rd-person offset), same as the reticle
	Vector end = eye + fwd * 8192.0f;
	pmtrace_t* tr = gEngfuncs.PM_TraceLine(eye, end, 0 /*PM_NORMAL*/, 2 /*studio hull*/, -1);
	if (!tr || tr->fraction >= 1.0f)
		return;
	// WARNING: tr->ent is a physent-list index, NOT the entity number — map it exactly as the
	// aim/pickup trace in view.cpp:1188 does (PM_GetPhysEntInfo -> real index -> GetEntityByIndex).
	if (tr->ent <= 0)
		return;
	extern int PM_GetPhysEntInfo(int ent); // pm_shared.cpp (declared locally, as in view.cpp:30)
	cl_entity_t* hit = gEngfuncs.GetEntityByIndex(PM_GetPhysEntInfo(tr->ent));
	if (!hit || 0 == hit->player)
		return;
	// team compare: local g_iTeamNumber (entity.cpp:153) vs the hit player's networked team.
	// The original fades on ANY hit player (!= 0); the 1-vs-2 split just mirrors its tri-state.
	int hitTeam = g_PlayerExtraInfo[hit->index].teamnumber;
	g_iTSAimTarget = (hitTeam == g_iTeamNumber) ? 1 : 2; // both != 0 -> the local model fades
}

static inline int clampc(int v) { return v < 0 ? 0 : (v > 255 ? 255 : v); }

DECLARE_MESSAGE(m_TSReticle, NextStunt)

bool CHudTSReticle::Init()
{
	m_flStuntTime = 0;
	// The original hooks NextStunt on CHudTSWeaponBar (thunk 0x4c880 → FUN_1004fac0); the
	// consumer is the dive-arc color, which lives here in our split of that object.
	HOOK_MESSAGE(NextStunt);
	gHUD.AddHudElem(this);
	m_iFlags |= HUD_ACTIVE;
	return true;
}

// NextStunt (FUN_1004fac0): m_flStuntTime = READ_BYTE * 0.1f. No client-side decay — the
// server re-sends on every change including ->0 (TS_SendNextStunt, dlls/ts_stunt.cpp).
bool CHudTSReticle::MsgFunc_NextStunt(const char* pszName, int iSize, void* pbuf)
{
	BEGIN_READ(pbuf, iSize);
	m_flStuntTime = READ_BYTE() * 0.1f;
	m_iFlags |= HUD_ACTIVE;
	return true;
}

bool CHudTSReticle::VidInit()
{
	// RE: FUN_1004cb50 loads exactly these (sprites/cross_*.spr + sprites/player/divestep*.spr).
	m_hCrossE = SPR_Load("sprites/cross_e.spr");
	m_hCross2 = SPR_Load("sprites/cross2.spr");
	m_hCrossT = SPR_Load("sprites/cross_t.spr");
	m_hDiveT = SPR_Load("sprites/player/divestept.spr");
	m_hDiveC = SPR_Load("sprites/player/divestepc.spr");
	m_hDiveB = SPR_Load("sprites/player/divestepb.spr");
	return true;
}

bool CHudTSReticle::Draw(float flTime)
{
	cl_entity_t* lp = gEngfuncs.GetLocalPlayer();
	if (!lp)
		return true;
	// don't draw while dead / HUD-hidden, or in ANY spectator/observer mode (g_iUser1 != 0).
	if (gHUD.m_iHideHUDDisplay & (HIDEHUD_ALL | HIDEHUD_HEALTH))
		return true;
	if (g_iUser1 != 0 || 0 != gEngfuncs.IsSpectateOnly())
		return true;

	// SCOPED (sub-40 zoom): the ENTIRE reticle swaps for the scope overlay (drawn in the tri
	// pass) — here only the black SIDE BARS (screen-exact, beyond the circle) + the pinned
	// laser dot. No crosshair, no dive arc (round-7h/7j user spec).
	{
		extern bool TS_ScopedIn();
		extern void TS_DrawScopeOverlayHUD();
		if (TS_ScopedIn())
		{
			TS_DrawScopeOverlayHUD(); // the mask + its side bars (one geometry owner)
			int lx, ly, lr2, lg2, lb2;
			extern bool TS_LocalLaserDot(int&, int&, int&, int&, int&);
			if (TS_LocalHasLaser() && TS_LocalLaserDot(lx, ly, lr2, lg2, lb2))
				FillRGBA(ScreenWidth / 2 - 1, ScreenHeight / 2 - 1, 3, 3, lr2, lg2, lb2, 235); // pinned centre
			return true;
		}
	}

	// --- dive readiness from the local player's maxspeed (slot weight) ------
	// cont.69z: maxspeed = 210 + (120/81)*freeSlots  ->  freeSlots = (maxspeed-210)*81/120.
	float maxspeed = gEngfuncs.GetClientMaxspeed();
	if (maxspeed <= 0.0f)
		maxspeed = 320.0f; // no data yet -> treat as light
	int freeSlots = (int)((maxspeed - 210.0f) * (81.0f / 120.0f) + 0.5f);
	if (freeSlots < 0) freeSlots = 0;
	if (freeSlots > 81) freeSlots = 81;
	// the three dive height tiers (RE ts_stunt.cpp CanDive{High,Med,Low})
	const bool avail[3] = {
		freeSlots > 39, // top    = HIGH
		freeSlots > 29, // center = MEDIUM
		freeSlots > 19, // bottom = LOW
	};

	// --- aim pitch -> which step the dot/highlight sits on -------------------
	float va[3];
	gEngfuncs.GetViewAngles(va);
	const float pitch = va[0]; // +down / -up (GoDive thresholds: <-10 high, <=10 med, else low)
	const int aimStep = (pitch < -10.0f) ? 0 : (pitch <= 10.0f ? 1 : 2);

	int cx = ScreenWidth / 2;
	int cy = ScreenHeight / 2;

	// 3rd-person FLOATING reticle (RE dossier cont87, task C — PROVISIONAL): the gun fires along
	// the INSTANT view aim, but TS renders the camera with the SMOOTHED, over-the-shoulder cam_ofs
	// angles (view.cpp / V_CalcNormalRefdef). Projecting the instant aim point through that lagging,
	// offset view makes the whole reticle trail the mouse then re-converge — the "float". In 1st
	// person (and scoped, handled above) it stays dead-centre.
	// 3rd-person reticle = the SCREEN-SPACE PROJECTION of the actual traced aim point (user-RE'd:
	// "it's screen space mapping the actual traced location of the aim"). The gun fires along the
	// INSTANT view dir; the reticle is that aim point run through WorldToScreen — which uses the
	// current render camera (over-the-shoulder, SMOOTHED cam_ofs angles from view.cpp). So (a) moving
	// shifts the eye => the projected point moves (parallax), and (b) turning => the camera angles
	// lag => the point trails then re-converges. No explicit lerp — the float is inherent geometry.
	if (0 != CL_IsThirdPerson())
	{
		Vector fwd;
		gEngfuncs.pfnAngleVectors(va, fwd, NULL, NULL); // va = instant view angles (above)
		Vector eye = v_aimeye; // trace from the EYE (bullet origin), NOT the offset camera v_origin
		Vector end = eye + fwd * 8192.0f;
		pmtrace_t* aimtr = gEngfuncs.PM_TraceLine(eye, end, 0 /*PM_NORMAL*/, 2, -1);
		Vector aim = (aimtr && aimtr->fraction < 1.0f) ? Vector(aimtr->endpos) : end;
		float screen[3];
		if (0 == gEngfuncs.pTriAPI->WorldToScreen((float*)&aim, screen)) // 0 == on-screen
		{
			cx = (int)((1.0f + screen[0]) * 0.5f * (float)ScreenWidth);
			cy = (int)((1.0f - screen[1]) * 0.5f * (float)ScreenHeight);
		}
	}

	const bool laser = TS_LocalHasLaser();
	const int tr = 80, tg = 130, tb = 185;

	// cl_realhud: client.dll's crosshair draw (0x1004e9f0: the crosshair, the dive arc of 0x1004ef50
	// and the laser range number) returns at once unless cl_realhud is 0. The laser dot is the weapon
	// bar's own call (0x1004e6f0, before the nohud test) and has no such test, so it stays.
	// nohud: the weapon bar Draw (0x1004d3f9) stops after the laser dot, before the crosshair.
	const bool hideCross = TSHud_RealHud() || TSHud_NoHud();

	// The aiming crosshair (donut "sprite sight" + the NSEW ticks) shows ONLY for guns — melee/
	// grenade/flag suppress it (only the dive/weight arc below stays). RE: the original client.dll
	// gates the reticle crosshair on the active weapon (column>=2 == a firearm).
	if (!hideCross && TS_ActiveWeaponIsGun())
	{
		// center crosshair (64x64) — the donut RING, style-gated by cl_solidcross (dossier cont94 §6,
		// FUN_1004e9f0 @0x4ed01): 0 = ADDITIVE cross_e.spr in the HUD colour; >0 = SOLID cross2.spr,
		// HUD colour at 1, forced BLACK (0,0,0) above 1 (default "2" — the original writes literal
		// black, not near-black). The original also requires hardware GL for the solid branch
		// (software always takes the additive cross_e path). The four NSEW ticks below are small
		// LIGHT line-draws in the ring's gaps (the original draws them as lines, not sprite art).
		{
			static cvar_t* s_solidcross = nullptr;
			if (s_solidcross == nullptr)
				s_solidcross = gEngfuncs.pfnGetCvarPointer("cl_solidcross");
			int hr, hg, hb;
			TSHud_Color(hr, hg, hb);
			if (s_solidcross != nullptr && s_solidcross->value > 0.0f)
			{
				if (m_hCross2)
				{
					if (s_solidcross->value > 1.0f)
						SPR_Set(m_hCross2, 0, 0, 0); // solid black ring (all components forced 0)
					else
						SPR_Set(m_hCross2, hr, hg, hb); // solid ring in the HUD colour
					SPR_DrawHoles(0, cx - 32, cy - 32, NULL);
				}
			}
			else if (m_hCrossE)
			{
				SPR_Set(m_hCrossE, hr, hg, hb); // additive crosshair in the HUD colour
				SPR_DrawAdditive(0, cx - 32, cy - 32, NULL);
			}
		}
		// NSEW ticks 2x4 (4x2 horizontals): the dot's BASE blue (the lit dot is this +30%). No centre dot.
		// With a LASERSIGHT the ticks pull IN toward centre (the laser gives the precise aim).
		const int to = laser ? 15 : 21; // outer offset of the N/W ticks (S/E inner edge = to-4)
		FillRGBA(cx - 1,      cy - to,     2, 4, tr, tg, tb, 255); // N tick
		FillRGBA(cx - 1,      cy + to - 4, 2, 4, tr, tg, tb, 255); // S tick
		FillRGBA(cx + to - 4, cy - 1,      4, 2, tr, tg, tb, 255); // E tick
		FillRGBA(cx - to,     cy - 1,      4, 2, tr, tg, tb, 255); // W tick
	}

	// LEFT arc: divestept / divestepc / divestepb (40x80 each). Each sprite holds its third of the
	// "(" arc at a different vertical spot in its own canvas, so they're drawn AT THE SAME POSITION
	// (overlaid) to compose one continuous arc — each third tinted independently by its dive tier.
	const int ax = cx - 40;      // x of the arc, just left of the donut — TUNED
	const int ay = cy - 40;      // 80px-tall arc, vertically centered — TUNED
	const HSPRITE step[3] = { m_hDiveT, m_hDiveC, m_hDiveB };
	for (int i = 0; i < 3 && !hideCross; i++)
	{
		if (!step[i])
			continue;
		int r, g, b;
		if (avail[i])
		{
			// NextStunt gate (RE FUN_1004ef50): while a stunt/dive timer is pending
			// (m_flStuntTime >= 0.01) the WHOLE arc draws the original's RED (0xc3,0x2d,0x2d)
			// — it replaces the blue for available tiers (unavailable tiers are red anyway).
			if (m_flStuntTime >= 0.01f)
			{
				r = 0xc3; g = 0x2d; b = 0x2d; // stunt pending — no highlight while red
			}
			else
			{
				r = 45; g = 85; b = 150; // blue = this dive tier is available
				if (i == aimStep) { r = clampc(r + 55); g = clampc(g + 55); b = clampc(b + 55); } // aimed third brightens (only when available)
			}
		}
		else
		{
			r = 130; g = 35; b = 35; // red = too heavy for this tier — never highlights
		}
		SPR_Set(step[i], r, g, b);
		SPR_DrawAdditive(0, ax, ay, NULL); // same position -> the thirds compose the arc
	}

	// the PITCH DOT: slides ALONG the "(" arc (not straight up/down) with your view pitch — 45 up ->
	// top of the arc, level -> the left point, 45 down -> bottom. The arc is a circle centred on the
	// crosshair, so the dot follows it: x = cx - R*cos(pitch), y = cy + R*sin(pitch). Colour = the
	// aimed tier's blue/red. (RE: the dot shows which view-cone third = which dive height tier.)
	// A 60-degree view cone (+-30 pitch) maps onto the 180 degrees of the arc: aim 30 down -> dot at
	// the very BOTTOM, level -> the left point, 30 up -> the very TOP. So the arc angle = pitch*3.
	float p = pitch;
	if (p < -30.0f) p = -30.0f;
	if (p > 30.0f) p = 30.0f;
	const float pr = p * 3.0f * (3.14159265f / 180.0f); // +-30 deg pitch -> +-90 deg of arc
	const float R = 33.0f; // the dot rides a CIRCLE of this radius around the crosshair centre — TUNED
	const int dotX = cx - (int)(R * cosf(pr)); // dot rides the arc, same x-origin as the arc sprite
	const int dotY = cy + (int)(R * sinf(pr));
	// the dot is ALWAYS the tick blue ~30% brighter — it does NOT turn red over unavailable tiers.
	if (!hideCross)
		FillRGBA(dotX - 1, dotY - 1, 2, 2, 104, 169, 240, 255);

	// LASERSIGHT HUD dot: a small (red) 2x2 marker at where the gun MODEL is pointing, projected to the
	// screen near the reticle (the local player's laser is the HUD dot, NOT a world sprite — RE
	// FUN_1003d600 skips local). It shifts with the gun's aim (parallax vs the view centre).
	{
		int lx, ly, lr, lg, lb;
		if (TS_LocalLaserDot(lx, ly, lr, lg, lb))
		{
			// cl_laser dot style (dossier cont94 §7 — FUN_1004e6f0 @0x4e7f6/0x4e8a9/0x4e950): with
			// cl_laser < 1.0f (default 0) the ORIGINAL snaps the 2x2 dot to the CROSSHAIR centre
			// with a per-frame +-1px jitter (rand(-10,10)/10) while the "laser settled" flag holds,
			// else it rides the projected beam end. VERIFY: the settled flag (DAT_1018a628, zeroed
			// while wd+0x1d0 <= 0) is unpinned — treated as always held here, so the style-0 dot
			// always centre-snaps. TODO(cont94 §7): pin DAT_1018a628 with a live A/B trace and fall
			// back to the projected beam end while it is 0.
			static cvar_t* s_cl_laser = nullptr;
			if (s_cl_laser == nullptr)
				s_cl_laser = gEngfuncs.pfnGetCvarPointer("cl_laser");
			if (s_cl_laser != nullptr && s_cl_laser->value < 1.0f)
			{
				lx = cx + gEngfuncs.pfnRandomLong(-10, 10) / 10;
				ly = cy + gEngfuncs.pfnRandomLong(-10, 10) / 10;
			}
			FillRGBA(lx - 1, ly - 1, 2, 2, lr, lg, lb, 255);
		}
	}

	// LASERSIGHT distance readout: trace from the eye along aim, show the distance (metres) right of
	// the reticle in numbers.spr. Only with a lasersight (the laser gives you the ranged readout).
	if (laser && !hideCross)
	{
		float fwd[3];
		gEngfuncs.pfnAngleVectors(v_angles, fwd, NULL, NULL);
		Vector start = v_origin;
		Vector end = v_origin + Vector(fwd) * 8192.0f;
		pmtrace_t* tr2 = gEngfuncs.PM_TraceLine(start, end, 0 /*PM_NORMAL*/, 2, -1);
		if (tr2)
		{
			Vector hit = tr2->endpos;
			float dist = (hit - start).Length();
			int meters = (int)(dist / 52.0f + 0.5f); // HL units -> metres — TUNED
			TSHud_DrawNumber(cx + 28, cy - 8, TSHN_DRAWZERO, meters, tr, tg, tb);
		}
	}
	return true;
}
