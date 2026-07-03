/***
*  ts_scope.cpp — the TS scope overlay, SCREEN-SPACE (round-7l: the world-quad version was
*  depth-sliced by geometry and sorted under transparent props — xash render-order semantics).
*  ts_scopefull.spr = the pre-composed 512x512 full mask (quarterscope.spr mirrored 4x at
*  build time — game/ts/sprites, generated; the arc corner is the quarter's BR, decoded from
*  the palette-index alpha). Drawn scaled to a screen-height square via pfnSPR_DrawGeneric
*  (GL_SRC_ALPHA / GL_ONE_MINUS_SRC_ALPHA), exactly like a HUD element — the reticle/side
*  bars/laser dot draw around and over it in CHudTSReticle.
***/
#include "hud.h"
#include "cl_util.h"
#include "triangleapi.h"

extern "C" int CL_IsThirdPerson();
extern int g_iUser1;

// scoped-in = the CURRENT fov sits in the sub-40 zoom class (mp5k 32/13, m4 26/8, ...).
bool TS_ScopedIn()
{
	if (g_iUser1 != 0 || CL_IsThirdPerson() != 0)
		return false;
	extern float g_tsLocalFov;
	const int fov = (int)(g_tsLocalFov > 0.0f ? g_tsLocalFov : (float)gHUD.m_iFOV);
	return fov > 0 && fov < 40;
}

// Draw the fullscreen mask (called from CHudTSReticle::Draw while scoped, HUD space).
void TS_DrawScopeOverlayHUD()
{
	static HSPRITE s_hFull;
	if (!s_hFull)
		s_hFull = SPR_Load("sprites/ts_scopefull.spr");
	if (!s_hFull)
		return;
	// The mask square scales PAST the screen height so the circle (a fraction of the sprite)
	// takes up more of the view — the black border crops harmlessly top/bottom. Round-7m
	// user tune: "it should be a bigger hole".
	const float TS_SCOPE_SCALE = 1.45f;
	const int side = (int)(ScreenHeight * TS_SCOPE_SCALE);
	const int x = ScreenWidth / 2 - side / 2;
	const int y = (ScreenHeight - side) / 2;
	// xash's pfnSPR_DrawGeneric IGNORES its blendsrc/blenddst args (#if 0 in cl_game.c) and
	// inherits the CURRENT render mode — set TransAlpha via the TriAPI around the draw (the
	// shared clgame.ds.renderMode), else the INDEXALPHA mask renders fully OPAQUE (the
	// round-7m black screen).
	gEngfuncs.pTriAPI->RenderMode(kRenderTransAlpha);
	SPR_Set(s_hFull, 255, 255, 255);
	gEngfuncs.pfnSPR_DrawGeneric(0, x, y, NULL, 0, 0, side, side);
	gEngfuncs.pTriAPI->RenderMode(kRenderNormal);
	// the black SIDE BARS beyond the mask square (one owner for the whole geometry)
	if (x > 0)
	{
		gEngfuncs.pfnFillRGBABlend(0, 0, x, ScreenHeight, 0, 0, 0, 255);
		gEngfuncs.pfnFillRGBABlend(x + side, 0, ScreenWidth - (x + side), ScreenHeight, 0, 0, 0, 255);
	}
}
