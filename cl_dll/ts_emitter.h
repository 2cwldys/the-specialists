/***
*  ts_emitter.h — The Specialists CLIENT map effects: ts_particle / ts_emitter / ts_fog.
*
*  client.dll's CTS_CL_Emitter (its own name, from its console messages). The server removes these
*  three entities at spawn (dlls/ts_emitter.cpp); the client reads them straight from the map's BSP
*  entity lump whenever the level name changes, and from then on spawns the emitters' particles into
*  the sprite-particle pool (cl_dll/ts_particles.cpp) and applies the map's fog. This is how TS maps
*  get their rain and snow (ts_neobaroque, ts_wyndham, ts_frigid). ts_wind is server-side: its
*  TSPSys message drives the particles' wind (g_tsWindState).
*    - level change + emit  : FUN_1003ab40 (from HUD_Frame)
*    - load                 : FUN_1003a960 / FUN_1003a860 (LoadEntityLump) / FUN_1003a5d0 (ProcessEnts)
*    - block parsers        : ts_particle FUN_1003a290 + FUN_10055f90, ts_emitter FUN_1003a160 +
*                             FUN_10055d20, ts_fog FUN_1003a3c0 + FUN_1003a7f0
*    - particle setup       : FUN_1003a9b0; collide callbacks FUN_10039d30/FUN_10039d50/FUN_10039f60
*    - fog pass             : FUN_10056c20 (the V_CalcRefdef tail); off again at the end of the
*                             transparent pass, as client.dll's FX root FUN_10056dd0 does
***/
#pragma once

// Per frame, from HUD_Frame (frametime = HUD_Frame's argument).
void TSEmitter_Frame(double frametime);

// The fog pass, at the end of V_CalcRefdef.
void TS_ApplyFog();
// Turn the fog back off once the transparent pass is drawn (end of HUD_DrawTransparentTriangles).
void TS_FogFrameEnd();
