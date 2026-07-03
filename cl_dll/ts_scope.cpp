/***
*  ts_scope.cpp — the TS scope overlay (sprites/quarterscope.spr): one 256x256 quarter of the
*  black mask + cross, mirrored 4x into a screen-height circle, black bars filling the sides.
*  Shown ONLY in the sub-40 zoom class (dossier-attachments addendum: the fov-40 threshold
*  drives viewmodel-hide + overlay + reticle swap as ONE bundle; deagle-class zooms get none).
*  Drawn as a camera-facing quad at near distance from HUD_DrawTransparentTriangles (SPR_Draw
*  cannot mirror); the side bars are HUD-space fills in CHudTSReticle (screen-exact edges).
***/
#include "hud.h"
#include "cl_util.h"
#include "triangleapi.h"
#include "r_efx.h"
#include "com_model.h"

#include <math.h>

extern Vector v_origin, v_angles; // view.cpp
extern "C" int CL_IsThirdPerson();
extern int g_iUser1;

// scoped-in = the CURRENT fov sits in the sub-40 zoom class (mp5k 32/13, m4 26/8, ...).
bool TS_ScopedIn()
{
	if (g_iUser1 != 0 || CL_IsThirdPerson() != 0)
		return false;
	const int fov = (int)gHUD.m_iFOV;
	return fov > 0 && fov < 40;
}

void TS_DrawScopeOverlay()
{
	if (!TS_ScopedIn())
		return;
	static HSPRITE s_hQuarter;
	if (!s_hQuarter)
		s_hQuarter = SPR_Load("sprites/quarterscope.spr");
	const model_s* spr = (const model_s*)gEngfuncs.GetSpritePointer(s_hQuarter);
	if (!spr)
		return;

	Vector fwd, right, up, ang(v_angles[0], v_angles[1], v_angles[2]);
	AngleVectors(ang, fwd, right, up);
	const float dist = 4.0f;
	// vertical half-extent of the view frustum at `dist` (fov is horizontal; scale by aspect)
	const float fovx = gHUD.m_iFOV > 0 ? (float)gHUD.m_iFOV : 90.0f;
	const float halfW = dist * tanf(fovx * (M_PI / 360.0f));
	const float halfH = halfW * (float)ScreenHeight / (float)ScreenWidth;
	const Vector c = Vector(v_origin[0], v_origin[1], v_origin[2]) + fwd * dist;

	struct triangleapi_s* tri = gEngfuncs.pTriAPI;
	tri->RenderMode(kRenderTransAlpha);
	tri->SpriteTexture((struct model_s*)spr, 0);
	tri->Color4f(1.0f, 1.0f, 1.0f, 1.0f);
	tri->CullFace(TRI_NONE);
	// the circle spans the screen HEIGHT: a centred square of half-extent halfH; four quarters
	// with mirrored texcoords (the quarter's OPAQUE corner faces out, the arc faces centre).
	tri->Begin(TRI_QUADS);
	for (int qx = 0; qx < 2; qx++)
	{
		for (int qy = 0; qy < 2; qy++)
		{
			// quarter rect: x0..x1 (right axis), y0..y1 (up axis)
			const float x0 = (qx == 0) ? -halfH : 0.0f;
			const float x1 = x0 + halfH;
			const float y0 = (qy == 0) ? 0.0f : -halfH;
			const float y1 = y0 + halfH;
			// texcoords: u mirrors on the right half, v mirrors on the bottom half.
			// Base quarter assumed TOP-LEFT (u 0->1 left->centre, v 0->1 top->centre);
			// flip per quadrant so the arc always faces the centre.
			const float uL = (qx == 0) ? 0.0f : 1.0f, uR = (qx == 0) ? 1.0f : 0.0f;
			const float vT = (qy == 1) ? 0.0f : 1.0f, vB = (qy == 1) ? 1.0f : 0.0f;
			tri->TexCoord2f(uL, vT); { Vector p = c + right * x0 + up * y1; tri->Vertex3fv(p); }
			tri->TexCoord2f(uR, vT); { Vector p = c + right * x1 + up * y1; tri->Vertex3fv(p); }
			tri->TexCoord2f(uR, vB); { Vector p = c + right * x1 + up * y0; tri->Vertex3fv(p); }
			tri->TexCoord2f(uL, vB); { Vector p = c + right * x0 + up * y0; tri->Vertex3fv(p); }
		}
	}
	tri->End();
	tri->RenderMode(kRenderNormal);
}
