/***
*  ts_motionblur.cpp — The Specialists slow-motion GHOST TRAILS. See ts_motionblur.h.
*
*  Faithful port of the original client.dll blur pipeline (RE dossier cont94 §1 +
*  client_display_cvars.md §4):
*    handler FUN_1004bf90  : BEGIN_READ; idx=READ_BYTE; lvl=READ_BYTE; arm(idx, lvl)
*                            (two READ_BYTEs — NOT a READ_SHORT; wire size 2 ✓ our senders)
*    arm     FUN_10029680  : 32-entry array {active,count,frames,nextTime}, entry idx == entindex
*    spawner FUN_10029780  : from HUD_CreateEntities — CL_TentEntAllocCustom ghost tempents
*    fade    FUN_10029710  : 255 alpha/sec ÷ max(timescale, 0.01); die = now ± 1
*
*  Server senders (all `WRITE_BYTE(entindex); WRITE_BYTE(level 1|2)`): dlls/ts_player.cpp
*  UsePowerUp/TakeDamage rolls + dlls/ts_stunt.cpp TS_MotionBlur.
***/
#include "hud.h"
#include "cl_util.h"
#include "parsemsg.h"
#include "const.h"
#include "entity_state.h"
#include "cl_entity.h"
#include "r_efx.h"
#include "com_model.h"
#include "studio.h"
#include "r_studioint.h"
#include "ts_motionblur.h"

extern engine_studio_api_t IEngineStudio;
extern float g_flClientSlow;        // the DAT_100963e8 equivalent — smoothed slow-mo timescale (ts_powerup_hud.cpp)
extern "C" int CL_IsThirdPerson();  // in_camera.cpp (engine export, C linkage)

// The 32-entry blur array (original DAT_100dff48, entry stride 0x18), entry index == entindex.
struct blurentry_t
{
	int active;      // +0x00
	int count;       // +0x04  ghosts spawned so far
	int frames;      // +0x08  ghosts to spawn in total
	double nextTime; // +0x10  next spawn time (gHUD.m_flTime clock)
};
static blurentry_t g_TSBlur[32];

static cvar_t* s_cl_motionblur = nullptr;
static cvar_t* s_cl_fx = nullptr;

// GhostFade — exact port of FUN_10029710(te, frametime, currenttime):
// fuser2 is the float alpha accumulator; drain 255 alpha/sec scaled by the slow-mo timescale.
static void TS_GhostFadeCallback(struct tempent_s* te, float frametime, float currenttime)
{
	float ts = g_flClientSlow;
	if (ts < 0.01f)
		ts = 0.01f;
	const float a = te->entity.curstate.fuser2 - frametime * 255.0f / ts;
	te->entity.curstate.fuser2 = a;
	te->entity.curstate.renderamt = (int)a;
	if (a > 0.0f)
	{
		te->die = currenttime + 1.0f;
	}
	else
	{
		te->entity.curstate.renderamt = 0;
		te->die = currenttime - 1.0f;
	}
}

// Handler (FUN_1004bf90) + arm (FUN_10029680): BYTE entindex, BYTE level.
static int __MsgFunc_TSMBlur(const char* pszName, int iSize, void* pbuf)
{
	BEGIN_READ(pbuf, iSize);
	const int idx = READ_BYTE();
	const int lvl = READ_BYTE();

	if (s_cl_motionblur == nullptr)
		s_cl_motionblur = gEngfuncs.pfnGetCvarPointer("cl_motionblur");
	if (s_cl_motionblur == nullptr || (int)s_cl_motionblur->value == 0)
		return 1;
	if (idx < 0 || idx >= 32) // entry index == entindex; the spawner loops 0..31 (entindex 32 never ghosts)
		return 1;

	blurentry_t* e = &g_TSBlur[idx];
	if (e->active != 0)
	{
		e->frames += 5; // already trailing: extend
		return 1;
	}

	// ghost count gated by min(level, cl_fx) — the client_display_cvars table.
	if (s_cl_fx == nullptr)
		s_cl_fx = gEngfuncs.pfnGetCvarPointer("cl_fx");
	float capped = (float)lvl;
	if (s_cl_fx != nullptr && s_cl_fx->value < capped)
		capped = s_cl_fx->value;
	switch ((int)capped)
	{
	case 1: e->frames = 10; break;
	case 2: e->frames = 15; break;
	default: e->frames = 5; break;
	}
	e->nextTime = gHUD.m_flTime;
	e->active = 1;
	e->count = 0;
	return 1;
}

void TS_MotionBlur_Init()
{
	memset(g_TSBlur, 0, sizeof(g_TSBlur));
	gEngfuncs.pfnHookUserMsg("TSMBlur", __MsgFunc_TSMBlur);
}

// Per-frame ghost spawner (FUN_10029780), called from HUD_CreateEntities.
void TS_UpdateMotionBlur()
{
	if (s_cl_motionblur == nullptr)
		s_cl_motionblur = gEngfuncs.pfnGetCvarPointer("cl_motionblur");
	if (s_cl_motionblur == nullptr || (int)s_cl_motionblur->value == 0)
		return;

	cl_entity_t* local = gEngfuncs.GetLocalPlayer();

	for (int i = 0; i < 32; i++) // 0..31 — entindex 32 never ghosts (original quirk)
	{
		blurentry_t* e = &g_TSBlur[i];
		if (e->active == 0 || e->nextTime > gHUD.m_flTime)
			continue;

		// NOT in slow-mo -> the trail dies instantly at normal speed.
		if (g_flClientSlow > 0.99f)
		{
			e->active = 0;
			continue;
		}

		float ts = g_flClientSlow;
		if (ts < 0.01f)
			ts = 0.01f;
		e->nextTime = gHUD.m_flTime + 0.03f / ts;

		// Own-ghost hiding — INTERIM divergence: the original's faithful mechanism is a
		// RENDER-time gate in its StudioDrawPlayer (FUN_10031ae0 @0x31c41, keyed on
		// curstate.rendercolor.b == 1 && rendercolor.r == localidx). We have no studio ghost
		// gate yet, so we don't SPAWN local ghosts unless thirdperson && cl_motionblur >= 2
		// (differs from the original only if the camera toggles mid-trail).
		if (local != nullptr && i == local->index &&
			!(0 != CL_IsThirdPerson() && (int)s_cl_motionblur->value >= 2))
			continue;

		cl_entity_t* ent = gEngfuncs.GetEntityByIndex(i);
		if (ent == nullptr || ent->model == nullptr)
			continue;

		TEMPENTITY* te = gEngfuncs.pEfxAPI->CL_TentEntAllocCustom(
			(float*)&ent->origin, ent->model, 0, TS_GhostFadeCallback);
		if (te != nullptr)
		{
			entity_state_t* ps = IEngineStudio.GetPlayerState(i - 1);
			if (ps != nullptr)
				te->entity.curstate = *ps; // whole entity_state_t snapshot
			te->entity.angles = ent->angles;

			entity_state_t& cs = te->entity.curstate;
			cs.number = i;
			cs.rendercolor.r = (unsigned char)i; // ghost OWNER entindex
			cs.rendercolor.b = 1;       // THE GHOST MARKER (the "entstate+0x302" StudioDrawPlayer gate)
			cs.rendermode = kRenderTransColor;
			cs.renderamt = 120;
			cs.renderfx = 17; // kRenderFxDeadPlayer — routes the tempent through StudioDrawPlayer
			                  // (our TS deadplayer contract reads the index from rendercolor.r)
			cs.iuser4 = 0;
			cs.fuser2 = 120.0f; // the float alpha accumulator the fade callback drains

			// Player remap block -> the ghost's colormap/gait snapshot (original reads
			// pi+0x178/0x17c/0x180/0x184). VERIFY fields — mapped by player_info_t layout:
			// 0x178 renderframe / 0x17c gaitsequence / 0x180 gaitframe / 0x184 gaityaw.
			player_info_t* pi = IEngineStudio.PlayerInfo(i - 1);
			if (pi != nullptr)
			{
				cs.iuser2 = pi->renderframe;
				cs.iuser1 = pi->gaitsequence;
				cs.fuser3 = pi->gaitframe;
				cs.fuser4 = pi->gaityaw;
			}

			cs.origin = ent->origin;
			te->entity.baseline.origin = Vector(0, 0, 0); // the 3 dwords @ te+0x5c the original zeroes
		}

		e->count++;
		if (e->count >= e->frames)
			e->active = 0;
	}
}
