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
#include "ts_emitter.h" // TS map fog
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

	// client.dll's FX root (0x10056dd0) runs, in order: the player shadows (0x10055760), the lasers
	// (0x1003de00) and flashlights (0x1003bad0), then the particle pool (0x100520d0).
	TS_Shadows_Draw();
	TS_Attachments_Draw();
	TSParticles_Update();

	// The client bullet simulation: step every queued round, draw them in slow motion. Last in
	// client.dll's HUD_DrawTransparentTriangles too (FUN_10034e60 at 0x10056e59, after the
	// particle pool and before the fog is switched off).
	TSBullets_Update();

	// client.dll's FX root (FUN_10056dd0) ends with glDisable(GL_FOG): the fog the V_CalcRefdef tail
	// turned on covers the world and the transparent pass, not what is drawn after it.
	TS_FogFrameEnd();
}
