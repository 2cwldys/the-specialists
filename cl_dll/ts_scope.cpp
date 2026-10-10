/***
*  ts_scope.cpp — the TS scope overlay. TS ships only `quarterscope.spr` (the top-left quarter of
*  the mask); client.dll's scope draw (0x10054510, in its FX pass, hardware renderer only, first
*  person, alive and playing, field of view under 40) paints that quarter four times, each rotated
*  into its quadrant, as a TriAPI quad in front of the eye sized by the field of view. reTS paints
*  the same four quarters in screen space at Steven's size (1.45 x the screen height, 7m) with
*  black side bars, so the reticle's side bars and pinned laser dot draw around and over it.
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
// The quarter sprite's texture maps: uv(1,1) = the circle CENTRE, uv(0,0) = the outer corner of
// the circle. We place uv(1,1) at the screen centre for all 4 quads and the outer corner at each
// screen corner — that rotates the one quarter into the 4 quadrants to form the full circle.
void TS_DrawScopeOverlayHUD()
{
	static HSPRITE s_hQuarter;
	if (!s_hQuarter)
		s_hQuarter = SPR_Load("sprites/quarterscope.spr");
	if (!s_hQuarter)
		return;
	struct model_s *pSpr = (struct model_s *)gEngfuncs.GetSpritePointer(s_hQuarter);
	if (!pSpr)
		return;

	// The circle scales PAST the screen height so it fills more of the view; the black side bars
	// crop the left/right. Round-7m user tune: "it should be a bigger hole".
	const float TS_SCOPE_SCALE = 1.45f;
	const float r = ScreenHeight * TS_SCOPE_SCALE * 0.5f; // radius
	const float cx = ScreenWidth * 0.5f;
	const float cy = ScreenHeight * 0.5f;

	const int lx = (int)(cx - r); // circle-square left edge
	const int rx = (int)(cx + r); // right edge
	// The black SIDE BARS filling the screen LEFT/RIGHT of the circle square. Draw them FIRST —
	// FillRGBABlend then runs with clean state (before the TriAPI SpriteTexture bind). The circle's
	// transparent edge touches the square edge at lx/rx (3 & 9 o'clock), so the bars must OVERLAP a
	// couple px past that edge or a thin line of the game shows through the seam. 2px clip of the
	// circle's extreme left/right is imperceptible.
	const int OVERLAP = 2;
	if (lx > 0)
		gEngfuncs.pfnFillRGBABlend(0, 0, lx + OVERLAP, ScreenHeight, 0, 0, 0, 255);
	if (rx < ScreenWidth)
		gEngfuncs.pfnFillRGBABlend(rx - OVERLAP, 0, ScreenWidth - (rx - OVERLAP), ScreenHeight, 0, 0, 0, 255);

	// The circle mask: quarterscope.spr painted 4x rotated via the TriAPI.
	triangleapi_s *tri = gEngfuncs.pTriAPI;
	tri->RenderMode(kRenderTransAlpha);
	tri->CullFace(TRI_NONE);
	tri->SpriteTexture(pSpr, 0);
	tri->Begin(TRI_QUADS);
	tri->Color4f(1.0f, 1.0f, 1.0f, 1.0f);
	// each quadrant: outer screen corner (ox,oy) = uv(0,0); centre (cx,cy) = uv(1,1);
	// the two axis edges = uv(0,1)/(1,0). Shared-edge uvs match across quadrants → seamless circle.
	const float qx[4] = { cx - r, cx + r, cx + r, cx - r }; // outer x: TL,TR,BR,BL
	const float qy[4] = { cy - r, cy - r, cy + r, cy + r }; // outer y
	for (int q = 0; q < 4; q++)
	{
		const float ox = qx[q], oy = qy[q];
		tri->TexCoord2f(0.0f, 0.0f); tri->Vertex3f(ox, oy, 0.0f); // outer corner
		tri->TexCoord2f(0.0f, 1.0f); tri->Vertex3f(ox, cy, 0.0f); // along outer-x to centre-y
		tri->TexCoord2f(1.0f, 1.0f); tri->Vertex3f(cx, cy, 0.0f); // circle centre
		tri->TexCoord2f(1.0f, 0.0f); tri->Vertex3f(cx, oy, 0.0f); // along centre-x to outer-y
	}
	tri->End();
	tri->RenderMode(kRenderNormal);
}
