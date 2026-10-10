//========= Copyright © 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose:
//
// $NoKeywords: $
//=============================================================================

// Triangle rendering, if any

#include "hud.h"
#include "cl_util.h"

// Triangle rendering apis are in gEngfuncs.pTriAPI

#include "const.h"
#include "entity_state.h"
#include "cl_entity.h"
#include "triangleapi.h"
#include "Exports.h"

#include "particleman.h"
#include "tri.h"
#include "ts_particles.h" // TS client impact-effect particle pool
#include "ts_attachments.h" // TS gun lasersight + flashlight render
#include "ts_bullets.h"     // the client bullet simulation
extern IParticleMan* g_pParticleMan;

/*
=================
HUD_DrawNormalTriangles

Non-transparent triangles-- add them here
=================
*/
void DLLEXPORT HUD_DrawNormalTriangles()
{
	//	RecClDrawNormalTriangles();

	// The original RUNS the stock spectator (incl. the overview map modes — its client.dll keeps
	// the whole overview parser); the cont.69n suppression was part of the suppress-and-replace
	// invention, removed cont.84. DrawOverview self-gates on the map camera modes.
	gHUD.m_Spectator.DrawOverview();
}


/*
=================
HUD_DrawTransparentTriangles

Render any triangles with transparent rendermode needs here
=================
*/
void DLLEXPORT HUD_DrawTransparentTriangles()
{
	//	RecClDrawTransparentTriangles();


	if (g_pParticleMan)
		g_pParticleMan->Update();

	// TS custom impact-effect particle pool (sparks/smoke/debris) — original client.dll
	// drew this from HUD_DrawTransparentTriangles via FUN_100520d0(pool). cont.69x.
	TSParticles_Update();

	// TS gun attachments drawn from each player's muzzle: the lasersight beam+dot and the
	// flashlight beam+light (original client.dll FUN_1003d600 / FUN_1003b750, per-frame).
	TS_Attachments_Draw();

	// TS player blob shadows (ombra.spr ground blob under each player, cl_shadow-gated) — original
	// client.dll drew these from HUD_DrawTransparentTriangles per-player. cont.86c.
	TS_Shadows_Draw();

	// The client bullet simulation: step every queued round, draw them in slow motion. Last in
	// client.dll's HUD_DrawTransparentTriangles too (FUN_10034e60 at 0x10056e59, after the
	// particle pool and before the fog is switched off).
	TSBullets_Update();
}
