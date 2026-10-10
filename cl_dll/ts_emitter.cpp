/***
*  ts_emitter.cpp — The Specialists CLIENT map effects (client.dll CTS_CL_Emitter). See ts_emitter.h.
*  Tables, offsets and constants are client.dll's; its addresses are kept as provenance.
***/
#include "hud.h"
#include "cl_util.h"
#include "const.h"
#include "entity_state.h"
#include "cl_entity.h"
#include "triangleapi.h"
#include "event_api.h"
#include "r_studioint.h"
#include "ts_particles.h"
#include "ts_emitter.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern Vector v_origin;
extern float g_flClientSlow; // client.dll DAT_100963e8 (ts_powerup_hud.cpp)
extern engine_studio_api_t IEngineStudio;

#define TSE_MAX_DEFS     512 // 0x10108bb8, 0x2ec bytes each
#define TSE_MAX_EMITTERS 512 // 0x101663b8, 0x118 bytes each
#define TSE_STRING       100

// A ts_particle (FUN_10055f90's keys). Each value pair is "low"/"high"; client.dll draws between them.
struct TSParticleDef
{
	int index;          // +0x000 its list slot
	bool used;          // +0x004 in the list
	int refs;           // +0x008 emitters using it
	float lifeHigh, lifeLow;               // +0x00c/+0x010 lifehigh/lifelow
	float gravityHigh, gravityLow;         // +0x014/+0x018
	float colorHigh, colorLow;             // +0x01c/+0x020 brightness
	float fadeHigh, fadeLow;               // +0x024/+0x028 fadespeed
	float radiusHigh, radiusLow;           // +0x02c/+0x030 p_radius
	float scaleHigh, scaleLow;             // +0x034/+0x038 scalespeed
	int frameHigh, frameLow;               // +0x03c/+0x040 current_frame
	float frameRateHigh, frameRateLow;     // +0x044/+0x048 frame_rate
	float deltaHigh, deltaLow;             // +0x04c/+0x050 randomdelta
	float reactHigh, reactLow;             // +0x054/+0x058 reactamount
	float windHigh, windLow;               // +0x05c/+0x060 windamount
	float termHigh, termLow;               // +0x064/+0x068 termvelocity (parsed, never used)
	float fadeInHigh, fadeInLow;           // +0x06c/+0x070 fadeintime
	Vector velHigh, velLow;                // +0x074/+0x080 velocity
	Vector avelHigh, avelLow;              // +0x08c/+0x098 a_velocity
	Vector randVelHigh, randVelLow;        // +0x0bc/+0x0c8 randomvel
	Vector followAxis;                     // +0x0d4 followaxis
	float stretch;                         // +0x0e0 stretchspeed
	char spriteName[TSE_STRING];           // +0x0e4 sprite_name
	int spriteTrans;                       // +0x148 sprite_trans (the rendermode)
	int followView;                        // +0x14c follow_view (the particle's orient)
	int collide;                           // +0x150 collide (its collideMode)
	int onCollide;                         // +0x154 oncollide: 1 die, 2 die + spawn target, 3 spawn target
	char soundOnEmit[TSE_STRING];          // +0x158 soundonemit
	char classname[TSE_STRING];            // +0x1bc
	char targetname[TSE_STRING];           // +0x220 what emitters and targets name it by
	char target[TSE_STRING];               // +0x284 the particle oncollide spawns
	int spawnflags;                        // +0x2e8 bit 0: a spawned target gets its own velocity;
	                                       //        bit 1: copied to the particle, never read
};

// A ts_emitter (FUN_10055d20's keys).
struct TSEmitter
{
	int index;                    // +0x000 its list slot
	Vector origin;                // +0x004
	bool used;                    // +0x010 in the list
	int particle;                 // +0x014 the def it spawns (0 = the first def when its name has no match)
	char particleName[TSE_STRING];// +0x018 particle
	float epsHigh, epsLow;        // +0x07c/+0x080 emissions per second
	int ppeHigh, ppeLow;          // +0x084/+0x088 particles per emission
	Vector varianceHigh;          // +0x08c the box around origin
	Vector varianceLow;           // +0x098
	float cameraDensity;          // +0x0a4 inside the box, spawn around the camera instead ...
	float cameraRadius;           // +0x0a8 ... within this radius
	char classname[TSE_STRING];   // +0x0ac
	float nextEmit;               // +0x110 on the emitter clock
	int spawnflags;               // +0x114 bit 0: emit once
};

// The map's ts_fog (0x101893b8, 31 dwords).
struct TSFog
{
	bool active;      // +0x00
	float nearDist;   // +0x04 near
	float farDist;    // +0x08 far
	float color[3];   // +0x0c color "r g b" as written in the map (0..1)
	char classname[TSE_STRING];
};

static TSParticleDef g_defs[TSE_MAX_DEFS];
static TSEmitter g_emitters[TSE_MAX_EMITTERS];
static TSFog g_fog;
static char g_level[64];      // 0x10108b38, the level the tables were built for
static double g_emitterClock; // 0x10189478, the sum of HUD_Frame's frame times since the level loaded
static bool g_fogOnThisFrame;

static void TSE_Copy(char* dst, const char* src)
{
	strncpy(dst, src, TSE_STRING - 1);
	dst[TSE_STRING - 1] = 0;
}

// "x y z" straight into the vector (client.dll's sscanf; a short value leaves the rest as it was).
static void TSE_Vec(const char* value, Vector& v)
{
	sscanf(value, "%f %f %f", &v.x, &v.y, &v.z);
}

// FUN_10055f90: one ts_particle key. Unknown keys are ignored.
static void TSE_ParticleKey(TSParticleDef* d, const char* key, const char* value)
{
	if (!stricmp(key, "sprite_name")) TSE_Copy(d->spriteName, value);
	else if (!stricmp(key, "sprite_trans")) d->spriteTrans = atoi(value);
	else if (!stricmp(key, "follow_view")) d->followView = atoi(value);
	else if (!stricmp(key, "collide")) d->collide = atoi(value);
	else if (!stricmp(key, "oncollide")) d->onCollide = atoi(value);
	else if (!stricmp(key, "lifelow")) d->lifeLow = atof(value);
	else if (!stricmp(key, "lifehigh")) d->lifeHigh = atof(value);
	else if (!stricmp(key, "gravitylow")) d->gravityLow = atof(value);
	else if (!stricmp(key, "gravityhigh")) d->gravityHigh = atof(value);
	else if (!stricmp(key, "colorlow")) d->colorLow = atof(value);
	else if (!stricmp(key, "colorhigh")) d->colorHigh = atof(value);
	else if (!stricmp(key, "fadespeedlow")) d->fadeLow = atof(value);
	else if (!stricmp(key, "fadespeedhigh")) d->fadeHigh = atof(value);
	else if (!stricmp(key, "p_radiuslow")) d->radiusLow = atof(value);
	else if (!stricmp(key, "p_radiushigh")) d->radiusHigh = atof(value);
	else if (!stricmp(key, "scalespeedlow")) d->scaleLow = atof(value);
	else if (!stricmp(key, "scalespeedhigh")) d->scaleHigh = atof(value);
	else if (!stricmp(key, "current_framelow")) d->frameLow = atoi(value);
	else if (!stricmp(key, "current_framehigh")) d->frameHigh = atoi(value);
	else if (!stricmp(key, "frame_ratelow")) d->frameRateLow = atof(value);
	else if (!stricmp(key, "frame_ratehigh")) d->frameRateHigh = atof(value);
	else if (!stricmp(key, "randomdeltalow")) d->deltaLow = atof(value);
	else if (!stricmp(key, "randomdeltahigh")) d->deltaHigh = atof(value);
	else if (!stricmp(key, "velocitylow")) TSE_Vec(value, d->velLow);
	else if (!stricmp(key, "velocityhigh")) TSE_Vec(value, d->velHigh);
	else if (!stricmp(key, "a_velocitylow")) TSE_Vec(value, d->avelLow);
	else if (!stricmp(key, "a_velocityhigh")) TSE_Vec(value, d->avelHigh);
	else if (!stricmp(key, "randomvellow")) TSE_Vec(value, d->randVelLow);
	else if (!stricmp(key, "randomvelhigh")) TSE_Vec(value, d->randVelHigh);
	else if (!stricmp(key, "followaxis")) TSE_Vec(value, d->followAxis);
	else if (!stricmp(key, "soundonemit")) TSE_Copy(d->soundOnEmit, value);
	else if (!stricmp(key, "termvelocitylow")) d->termLow = atof(value);
	else if (!stricmp(key, "termvelocityhigh")) d->termHigh = atof(value);
	else if (!stricmp(key, "reactamountlow")) d->reactLow = atof(value);
	else if (!stricmp(key, "reactamounthigh")) d->reactHigh = atof(value);
	else if (!stricmp(key, "windamountlow")) d->windLow = atof(value);
	else if (!stricmp(key, "windamounthigh")) d->windHigh = atof(value);
	else if (!stricmp(key, "fadeintimelow")) d->fadeInLow = atof(value);
	else if (!stricmp(key, "fadeintimehigh")) d->fadeInHigh = atof(value);
	else if (!stricmp(key, "stretchspeed")) d->stretch = atof(value);
	else if (!stricmp(key, "classname")) TSE_Copy(d->classname, value);
	else if (!stricmp(key, "targetname")) TSE_Copy(d->targetname, value);
	else if (!stricmp(key, "target")) TSE_Copy(d->target, value);
	else if (!stricmp(key, "spawnflags")) d->spawnflags = atoi(value);
}

// FUN_10055d20: one ts_emitter key. variancelow/variancehigh/origin are "x y z".
static void TSE_EmitterKey(TSEmitter* e, const char* key, const char* value)
{
	if (!stricmp(key, "particle")) TSE_Copy(e->particleName, value);
	else if (!stricmp(key, "epslow")) e->epsLow = atof(value);
	else if (!stricmp(key, "epshigh")) e->epsHigh = atof(value);
	else if (!stricmp(key, "ppelow")) e->ppeLow = atoi(value);
	else if (!stricmp(key, "ppehigh")) e->ppeHigh = atoi(value);
	else if (!stricmp(key, "variancelow")) TSE_Vec(value, e->varianceLow);
	else if (!stricmp(key, "variancehigh")) TSE_Vec(value, e->varianceHigh);
	else if (!stricmp(key, "cameradensity")) e->cameraDensity = atof(value);
	else if (!stricmp(key, "cameraradius")) e->cameraRadius = atof(value);
	else if (!stricmp(key, "origin")) TSE_Vec(value, e->origin);
	else if (!stricmp(key, "classname")) TSE_Copy(e->classname, value);
	else if (!stricmp(key, "spawnflags")) e->spawnflags = atoi(value);
}

// FUN_1003a3c0's keys: far, near, color ("r g b", else a console warning), classname.
static void TSE_FogKey(TSFog* f, const char* key, const char* value)
{
	if (!stricmp(key, "far"))
		f->farDist = atof(value);
	else if (!stricmp(key, "near"))
		f->nearDist = atof(value);
	else if (!stricmp(key, "color"))
	{
		if (sscanf(value, "%f %f %f", &f->color[0], &f->color[1], &f->color[2]) != 3)
			gEngfuncs.Con_Printf("color has bad value. Should be \"r g b\"\n");
	}
	else if (!stricmp(key, "classname"))
		TSE_Copy(f->classname, value);
}

// The shared block reader of FUN_1003a290/FUN_1003a160/FUN_1003a3c0: key/value pairs up to '}', each
// key's trailing spaces trimmed. Returns the text after the block, or NULL with *error set when the
// block is cut short.
template <typename T>
static char* TSE_ReadBlock(char* data, T* into, void (*keyFn)(T*, const char*, const char*), bool* error)
{
	char token[1024];
	char key[256];
	for (;;)
	{
		data = gEngfuncs.COM_ParseFile(data, token);
		if (token[0] == '}')
			return data;
		if (data == NULL)
		{
			*error = true;
			return NULL;
		}
		strncpy(key, token, sizeof(key) - 1);
		key[sizeof(key) - 1] = 0;
		for (size_t n = strlen(key); n > 0 && key[n - 1] == ' '; n--)
			key[n - 1] = 0;
		data = gEngfuncs.COM_ParseFile(data, token);
		if (data == NULL)
		{
			*error = true;
			return NULL;
		}
		if (token[0] == '}')
		{
			*error = true;
			return data;
		}
		keyFn(into, key, token);
	}
}

// FUN_1003a820: the def an emitter or a target names, by targetname; -1 when none.
static int TSE_FindParticle(const char* name)
{
	for (int i = 0; i < TSE_MAX_DEFS; i++)
	{
		if (g_defs[i].used && !stricmp(g_defs[i].targetname, name))
			return g_defs[i].index;
	}
	return -1;
}

// FUN_1003a5d0 (CTS_CL_Emitter::ProcessEnts): three passes over the whole entity text, one for each
// class: every ts_particle into the next free def slot, every ts_emitter into the next free emitter
// slot (linked to its particle by name), then the last ts_fog. A pass stops when its table is full.
static void TSE_ProcessEnts(char* text)
{
	char token[1024];
	bool error = false;

	char* data = text;
	for (int slot = 0; slot < TSE_MAX_DEFS;)
	{
		data = gEngfuncs.COM_ParseFile(data, token);
		if (data == NULL)
			break;
		if (token[0] != '{')
		{
			gEngfuncs.Con_Printf("CTS_CL_Emitter::ProcessEnts: found %s when expecting {\n", token);
			return;
		}
		TSParticleDef* d = &g_defs[slot];
		memset(d, 0, sizeof(*d));
		data = TSE_ReadBlock(data, d, TSE_ParticleKey, &error);
		if (error)
		{
			gEngfuncs.Con_Printf("CTS_CL_Emitter::ProcessEnts: error parsing entities\n");
			return;
		}
		if (!stricmp(d->classname, "ts_particle"))
		{
			d->index = slot; // FUN_10055c50
			d->used = true;
			d->refs = 0;
			slot++;
		}
	}

	data = text;
	for (int slot = 0; slot < TSE_MAX_EMITTERS;)
	{
		data = gEngfuncs.COM_ParseFile(data, token);
		if (data == NULL)
			break;
		if (token[0] != '{')
		{
			gEngfuncs.Con_Printf("CTS_CL_Emitter::ProcessEnts: found %s when expecting {\n", token);
			return;
		}
		TSEmitter* e = &g_emitters[slot];
		memset(e, 0, sizeof(*e));
		data = TSE_ReadBlock(data, e, TSE_EmitterKey, &error);
		if (error)
		{
			gEngfuncs.Con_Printf("CTS_CL_Emitter::ProcessEnts: error parsing entities\n");
			return;
		}
		if (!stricmp(e->classname, "ts_emitter"))
		{
			e->index = slot; // FUN_10055c10
			e->used = true;
			e->nextEmit = 0;
			const int particle = TSE_FindParticle(e->particleName);
			if (particle != -1) // FUN_10055c90: an unmatched name leaves particle 0
			{
				g_defs[particle].refs++;
				e->particle = particle;
			}
			slot++;
		}
	}

	data = text;
	for (;;)
	{
		data = gEngfuncs.COM_ParseFile(data, token);
		if (data == NULL)
			return;
		if (token[0] != '{')
		{
			gEngfuncs.Con_Printf("CTS_CL_Emitter::ProcessEnts: found %s when expecting {\n", token);
			return;
		}
		TSFog f;
		memset(&f, 0, sizeof(f));
		data = TSE_ReadBlock(data, &f, TSE_FogKey, &error);
		if (error)
		{
			gEngfuncs.Con_Printf("CTS_CL_Emitter::ProcessEnts: error parsing entities\n");
			return;
		}
		if (!stricmp(f.classname, "ts_fog"))
		{
			g_fog = f; // FUN_1003a7f0
			g_fog.active = true;
		}
	}
}

// FUN_1003a960 + FUN_1003a860 (CTS_CL_Emitter::LoadEntityLump): read the level's BSP header, check
// the version (29 or 30) and hand the entity lump's text to ProcessEnts. client.dll fopens
// "<gamedir>/<level>"; reTS reads it through the engine filesystem.
static void TSE_LoadLevel(const char* level)
{
	int length = 0;
	byte* file = gEngfuncs.COM_LoadFile(level, 5, &length);
	if (file == NULL)
		return;
	if (length < 0x7c)
	{
		gEngfuncs.Con_Printf("CTS_CL_Emitter::LoadEntityLump:  Could not read BSP header for map [%s].\n", level);
		gEngfuncs.COM_FreeFile(file);
		return;
	}
	int header[3];
	memcpy(header, file, sizeof(header)); // version, entity lump offset, entity lump length
	if (header[0] != 29 && header[0] != 30)
	{
		gEngfuncs.Con_Printf("CTS_CL_Emitter::LoadEntityLump:  Map [%s] has incorrect BSP version (%i should be %i).\n",
			level, header[0], 30);
		gEngfuncs.COM_FreeFile(file);
		return;
	}
	if (header[1] < 0 || header[2] < 0 || header[1] + header[2] > length)
	{
		gEngfuncs.COM_FreeFile(file);
		return;
	}
	char* text = (char*)malloc(header[2] + 1);
	if (text == NULL)
	{
		gEngfuncs.Con_Printf("CTS_CL_Emitter::LoadEntityLump:  Couldn't allocate %i bytes\n", header[2] + 1);
		gEngfuncs.COM_FreeFile(file);
		return;
	}
	memcpy(text, file + header[1], header[2]);
	text[header[2]] = 0;
	gEngfuncs.COM_FreeFile(file);
	TSE_ProcessEnts(text);
	free(text);
}

// FUN_1003a9b0: draw a particle's values from its def.
static void TSE_InitFromDef(TSParticle* p, const TSParticleDef* d)
{
	p->life = gEngfuncs.pfnRandomFloat(d->lifeLow, d->lifeHigh);
	p->gravity = gEngfuncs.pfnRandomFloat(d->gravityLow, d->gravityHigh);
	p->bright = gEngfuncs.pfnRandomFloat(d->colorLow, d->colorHigh);
	p->fadeRate = gEngfuncs.pfnRandomFloat(d->fadeLow, d->fadeHigh);
	p->size = gEngfuncs.pfnRandomFloat(d->radiusLow, d->radiusHigh);
	p->accelRate = gEngfuncs.pfnRandomFloat(d->scaleLow, d->scaleHigh);
	p->frame = (float)gEngfuncs.pfnRandomLong(d->frameLow, d->frameHigh);
	p->frameRate = gEngfuncs.pfnRandomFloat(d->frameRateLow, d->frameRateHigh);
	p->randomDelta = gEngfuncs.pfnRandomFloat(d->deltaLow, d->deltaHigh);
	p->react = gEngfuncs.pfnRandomFloat(d->reactLow, d->reactHigh);
	p->wind = gEngfuncs.pfnRandomFloat(d->windLow, d->windHigh);
	p->fadeIn = gEngfuncs.pfnRandomFloat(d->fadeInLow, d->fadeInHigh);
	p->vel.x = gEngfuncs.pfnRandomFloat(d->velLow.x, d->velHigh.x);
	p->vel.y = gEngfuncs.pfnRandomFloat(d->velLow.y, d->velHigh.y);
	p->vel.z = gEngfuncs.pfnRandomFloat(d->velLow.z, d->velHigh.z);
	p->avel.x = gEngfuncs.pfnRandomFloat(d->avelLow.x, d->avelHigh.x);
	p->avel.y = gEngfuncs.pfnRandomFloat(d->avelLow.y, d->avelHigh.y);
	p->avel.z = gEngfuncs.pfnRandomFloat(d->avelLow.z, d->avelHigh.z);
}

static void TSE_Die(TSParticle* p, struct pmtrace_s* tr);
static void TSE_DieAndSpawn(TSParticle* p, struct pmtrace_s* tr);
static void TSE_Spawn(TSParticle* p, struct pmtrace_s* tr);

static TSParticleCollide TSE_CollideFn(int onCollide)
{
	switch (onCollide)
	{
	case 1: return TSE_Die;
	case 2: return TSE_DieAndSpawn;
	case 3: return TSE_Spawn;
	default: return NULL;
	}
}

// The fields every map particle gets from its def before TSE_InitFromDef (the emit loop and the
// collide callbacks write the same ones).
static void TSE_Prepare(TSParticle* p, const TSParticleDef* d, int sprite)
{
	p->attachment = 0;
	p->active = true;
	p->imode = 0;
	p->spriteIdx = (short)sprite;
	p->collide = 3;
	p->randVelLow = d->randVelLow;
	p->randVelHigh = d->randVelHigh;
	p->orient = (unsigned char)d->followView;
	p->collideMode = d->collide;
	p->target = TSE_FindParticle(d->target);
	p->stretch = d->stretch;
	p->onCollide = TSE_CollideFn(d->onCollide);
}

// FUN_10039d50 / FUN_10039f60's spawn: the target particle at the hit particle's position, moving on
// with its velocity unless the target's spawnflags bit 0 gives it its own. client.dll indexes the
// def table with the target unchecked; a missing target (-1) spawns nothing here.
static void TSE_SpawnTarget(TSParticle* from)
{
	if (from->target < 0 || from->target >= TSE_MAX_DEFS)
		return;
	const TSParticleDef* d = &g_defs[from->target];
	const int sprite = TSP_RegisterSprite(d->spriteName, (short)d->spriteTrans);
	if (sprite == -1)
	{
		gEngfuncs.Con_Printf("Sprite '%s' is not a valid sprite.\n", d->spriteName);
		return;
	}
	TSParticle* p = TSP_Scratch();
	TSE_Prepare(p, d, sprite);
	TSE_InitFromDef(p, d);
	p->pos = from->pos;
	if ((d->spawnflags & 1) == 0)
		p->vel = from->vel;
	else
	{
		p->vel.x = gEngfuncs.pfnRandomFloat(d->velLow.x, d->velHigh.x);
		p->vel.y = gEngfuncs.pfnRandomFloat(d->velLow.y, d->velHigh.y);
		p->vel.z = gEngfuncs.pfnRandomFloat(d->velLow.z, d->velHigh.z);
	}
	TSP_Commit(p, NULL, p->onCollide);
}

// oncollide 1 (FUN_10039d30): the particle dies on the hit.
static void TSE_Die(TSParticle* p, struct pmtrace_s* tr)
{
	TSP_Free(p->slot);
}

// oncollide 2 (FUN_10039d50): dies and spawns its target (a raindrop turning into a splash).
static void TSE_DieAndSpawn(TSParticle* p, struct pmtrace_s* tr)
{
	TSP_Free(p->slot);
	TSE_SpawnTarget(p);
}

// oncollide 3 (FUN_10039f60): spawns its target and goes on.
static void TSE_Spawn(TSParticle* p, struct pmtrace_s* tr)
{
	TSE_SpawnTarget(p);
}

// The emit pass of FUN_1003ab40 for one emitter whose time has come.
static void TSE_Emit(TSEmitter* e)
{
	const int divisor = g_tsFx.divisor;
	const TSParticleDef* d = &g_defs[e->particle];
	int count = gEngfuncs.pfnRandomLong(e->ppeLow, e->ppeHigh);
	if (count == 0)
		return;

	// Is the camera inside the emitter's box (x and y only)?
	const bool inside = (e->varianceLow.x + e->origin.x < v_origin.x) && (e->varianceLow.y + e->origin.y < v_origin.y) &&
		(v_origin.x < e->varianceHigh.x + e->origin.x) && (v_origin.y < e->varianceHigh.y + e->origin.y);
	float thin = 1.0f;
	if (!inside)
	{
		// Outside: skip it while its centre is behind the view (and try again next frame), and thin it
		// out with distance, 1 to 10 times, as box sizes away.
		const float dx = ((e->varianceLow.x + e->varianceHigh.x) * 0.5f + e->origin.x) - v_origin.x;
		const float dy = ((e->varianceLow.y + e->varianceHigh.y) * 0.5f + e->origin.y) - v_origin.y;
		float dist = sqrt(dx * dx + dy * dy);
		if (!(0.0001f < dist))
			dist = 0.0001f;
		if (g_tspViewForward.x * (dx / dist) + g_tspViewForward.z * (0.0f / dist) + g_tspViewForward.y * (dy / dist) < 0.1)
			return;
		const Vector box = e->varianceHigh - e->varianceLow;
		thin = dist / box.Length();
		if (thin >= 1.0f && thin > 10.0f)
			thin = 10.0f;
		else if (thin < 1.0f)
			thin = 1.0f;
	}
	count = (count % divisor == 0) ? count / divisor : count / divisor + 1;

	TSParticle* p = TSP_Scratch();
	const int sprite = TSP_RegisterSprite(d->spriteName, (short)d->spriteTrans);
	if (sprite == -1)
	{
		gEngfuncs.Con_Printf("Sprite '%s' is not a valid sprite.\n", d->spriteName);
		return;
	}
	TSE_Prepare(p, d, sprite);
	p->flags = (d->spawnflags >> 1) & 1;
	p->followAxis = d->followAxis;

	int n = 0;
	do
	{
		TSE_InitFromDef(p, d);
		if (e->cameraDensity <= 0.01f || e->cameraRadius <= 0.01f || !inside)
		{
			p->pos.x = gEngfuncs.pfnRandomFloat(e->varianceLow.x, e->varianceHigh.x) + e->origin.x;
			p->pos.y = gEngfuncs.pfnRandomFloat(e->varianceLow.y, e->varianceHigh.y) + e->origin.y;
		}
		else
		{
			// Inside a camera-density box: around the camera, nearer the denser, a little ahead of
			// the local player's motion.
			const float outer = e->cameraRadius + e->cameraRadius;
			const float density = (e->cameraDensity <= 1.0f) ? e->cameraDensity : 1.0f;
			const float r = outer - (outer - e->cameraRadius) * density;
			p->pos.x = gEngfuncs.pfnRandomFloat(-r, r) + v_origin.x;
			p->pos.y = gEngfuncs.pfnRandomFloat(-r, r) + v_origin.y;
			cl_entity_t* local = gEngfuncs.GetLocalPlayer();
			if (local != NULL)
			{
				p->pos.x = local->curstate.velocity.x * 1.5f + p->pos.x;
				p->pos.y = local->curstate.velocity.y * 1.5f + p->pos.y;
			}
		}
		p->pos.z = gEngfuncs.pfnRandomFloat(e->varianceLow.z, e->varianceHigh.z) + e->origin.z;
		TSP_Commit(p, NULL, p->onCollide);
		n++;
	} while ((float)n < (float)count / thin);

	if (d->soundOnEmit[0] != 0)
	{
		const int pitch = gEngfuncs.pfnRandomLong(-5, 5) + (25 - (int)(g_flClientSlow * -75.0f));
		gEngfuncs.pEventAPI->EV_PlaySound(0, p->pos, 6, d->soundOnEmit, 1.0f, 1.0f, 0, pitch);
	}

	if (e->spawnflags & 1)
	{
		memset(e, 0, sizeof(*e)); // emit once: out of the list, slot cleared
		return;
	}
	const float eps = gEngfuncs.pfnRandomFloat(e->epsLow, e->epsHigh);
	if (eps <= 0.0f)
		e->nextEmit = (float)((float)divisor * thin + g_emitterClock);
	else
	{
		const float slow = (g_flClientSlow < 0.01f) ? 0.01f : g_flClientSlow; // FUN_10056fa0
		e->nextEmit = (float)(g_emitterClock + (1.0f / eps) * (1.0f / slow) * (float)divisor * thin);
	}
}

// FUN_1003ab40: rebuild the tables when the level name changes, then run every emitter whose time has
// come on the emitter clock. Nothing runs before the particle pool exists (its first render frame).
void TSEmitter_Frame(double frametime)
{
	const char* level = gEngfuncs.pfnGetLevelName();
	if (level == NULL)
		level = "";
	if (stricmp(g_level, level) != 0)
	{
		strncpy(g_level, level, sizeof(g_level) - 1);
		g_level[sizeof(g_level) - 1] = 0;
		g_emitterClock = 0.0;
		memset(g_emitters, 0, sizeof(g_emitters));
		memset(g_defs, 0, sizeof(g_defs));
		memset(&g_fog, 0, sizeof(g_fog));
		if (g_level[0] == 0)
			return;
		TSE_LoadLevel(g_level);
	}
	if (g_level[0] == 0 || !TSP_PoolReady())
		return;

	g_emitterClock += frametime;
	for (int i = 0; i < TSE_MAX_EMITTERS; i++)
	{
		TSEmitter* e = &g_emitters[i];
		// A cleared slot draws 0 particles (ppelow/ppehigh 0) and spawns nothing.
		if (!e->used || !(e->nextEmit < (float)g_emitterClock))
			continue;
		TSE_Emit(e);
	}
}

//-----------------------------------------------------------------------------
// Fog (FUN_10056c20). client.dll calls OpenGL itself (linear fog, density 1, GL_NICEST) and turns it
// off at the end of its FX root; reTS goes through the engine's triangle API, whose Fog takes 0..255
// colours.
//-----------------------------------------------------------------------------
void TS_ApplyFog()
{
	g_fogOnThisFrame = false;
	triangleapi_t* tri = gEngfuncs.pTriAPI;
	if (tri == NULL || tri->Fog == NULL)
		return;

	float color[3];
	float start, end;
	// gl_fog 2, single player only: the user's fog from the gl_fog_* cvars.
	if (gEngfuncs.GetMaxClients() == 1 && gEngfuncs.pfnGetCvarFloat("gl_fog") == 2.0f)
	{
		color[0] = gEngfuncs.pfnGetCvarFloat("gl_fog_red") * 255.0f;
		color[1] = gEngfuncs.pfnGetCvarFloat("gl_fog_green") * 255.0f;
		color[2] = gEngfuncs.pfnGetCvarFloat("gl_fog_blue") * 255.0f;
		start = gEngfuncs.pfnGetCvarFloat("gl_fog_start");
		end = gEngfuncs.pfnGetCvarFloat("gl_fog_end");
	}
	// Else the map's ts_fog, unless gl_fog is 0, and only in the hardware renderer.
	else if (g_fog.active && gEngfuncs.pfnGetCvarFloat("gl_fog") != 0.0f && IEngineStudio.IsHardware() == 1)
	{
		color[0] = g_fog.color[0] * 255.0f;
		color[1] = g_fog.color[1] * 255.0f;
		color[2] = g_fog.color[2] * 255.0f;
		start = g_fog.nearDist;
		end = g_fog.farDist;
	}
	else
		return;

	tri->Fog(color, start, end, 1);
	g_fogOnThisFrame = true;
}

void TS_FogFrameEnd()
{
	if (!g_fogOnThisFrame)
		return;
	g_fogOnThisFrame = false;
	float color[3] = {0, 0, 0};
	gEngfuncs.pTriAPI->Fog(color, 0, 0, 0);
}
