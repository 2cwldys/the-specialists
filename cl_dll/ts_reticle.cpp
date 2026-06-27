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
#include "cl_entity.h"  // cl_entity_t (GetLocalPlayer)
#include "pmtrace.h"    // pmtrace_t (laser distance trace)
#include "ts_hud.h"     // TSHud_DrawNumber (numbers.spr)
#include "ts_attachments.h" // TS_LocalHasLaser
#include "ts_reticle.h"

#include <math.h>

extern Vector v_origin, v_angles; // view origin/angles (cl_dll/view.cpp) — for the aim trace

static inline int clampc(int v) { return v < 0 ? 0 : (v > 255 ? 255 : v); }

bool CHudTSReticle::Init()
{
	gHUD.AddHudElem(this);
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

	const int cx = ScreenWidth / 2;
	const int cy = ScreenHeight / 2;

	// center crosshair (64x64). The DARK donut RING is cross2 (transparent bg) drawn near-black; the
	// four NSEW ticks + the centre dot are small (~4x1px) LIGHT line-draws in the ring's gaps (the
	// original draws them as lines, not sprite art).
	if (m_hCross2)
	{
		SPR_Set(m_hCross2, 16, 16, 16); // near-black donut ring — TUNED
		SPR_DrawHoles(0, cx - 32, cy - 32, NULL);
	}
	// NSEW ticks 2x4 (4x2 horizontals): the dot's BASE blue (the lit dot is this +30%). No centre dot.
	// With a LASERSIGHT the ticks pull IN toward centre (the laser gives the precise aim).
	const bool laser = TS_LocalHasLaser();
	const int tr = 80, tg = 130, tb = 185;
	const int to = laser ? 15 : 21; // outer offset of the N/W ticks (S/E inner edge = to-4)
	FillRGBA(cx - 1,      cy - to,     2, 4, tr, tg, tb, 255); // N tick
	FillRGBA(cx - 1,      cy + to - 4, 2, 4, tr, tg, tb, 255); // S tick
	FillRGBA(cx + to - 4, cy - 1,      4, 2, tr, tg, tb, 255); // E tick
	FillRGBA(cx - to,     cy - 1,      4, 2, tr, tg, tb, 255); // W tick

	// LEFT arc: divestept / divestepc / divestepb (40x80 each). Each sprite holds its third of the
	// "(" arc at a different vertical spot in its own canvas, so they're drawn AT THE SAME POSITION
	// (overlaid) to compose one continuous arc — each third tinted independently by its dive tier.
	const int ax = cx - 40;      // x of the arc, just left of the donut — TUNED
	const int ay = cy - 40;      // 80px-tall arc, vertically centered — TUNED
	const HSPRITE step[3] = { m_hDiveT, m_hDiveC, m_hDiveB };
	for (int i = 0; i < 3; i++)
	{
		if (!step[i])
			continue;
		int r, g, b;
		if (avail[i])
		{
			r = 45; g = 85; b = 150; // blue = this dive tier is available
			if (i == aimStep) { r = clampc(r + 55); g = clampc(g + 55); b = clampc(b + 55); } // aimed third brightens (only when available)
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
	FillRGBA(dotX - 1, dotY - 1, 2, 2, 104, 169, 240, 255);

	// LASERSIGHT HUD dot: a small (red) 2x2 marker at where the gun MODEL is pointing, projected to the
	// screen near the reticle (the local player's laser is the HUD dot, NOT a world sprite — RE
	// FUN_1003d600 skips local). It shifts with the gun's aim (parallax vs the view centre).
	{
		int lx, ly, lr, lg, lb;
		if (TS_LocalLaserDot(lx, ly, lr, lg, lb))
			FillRGBA(lx - 1, ly - 1, 2, 2, lr, lg, lb, 255);
	}

	// LASERSIGHT distance readout: trace from the eye along aim, show the distance (metres) right of
	// the reticle in numbers.spr. Only with a lasersight (the laser gives you the ranged readout).
	if (laser)
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
			TSHud_DrawNumber(cx + 28, cy - 8, meters, tr, tg, tb);
		}
	}
	return true;
}
