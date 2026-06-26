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

	// TS has no spectator overview/inset map (the green-grid top-left overlay) — suppress it during
	// the TS free-look spectate/buy phase. (cont.69n)
	if (!gHUD.m_Spectate.IsActive())
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
}
