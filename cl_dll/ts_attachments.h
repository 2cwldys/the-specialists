/***
*  ts_attachments.h — client render of the TS gun attachments that are VISIBLE in the world:
*  the lasersight (attach bit 2) and the flashlight (attach bit 4). The silencer (bit 1) and
*  scope (bit 8) are handled elsewhere (the fire event / the player FOV); these two are drawn
*  per-frame from the muzzle of every player who has them.
*
*  RE: client.dll FUN_1003d600 (laser: beam sprites/laserbeam.spr muzzle->aim-trace + the
*  sprites/laserdot.spr dot, colour by index 1=green/2=blue/else=red — FUN_100582f0) and
*  FUN_1003b750 (flashlight: a flash beam + a white dynamic light at the surface). Each iterates
*  all players, gated by the per-player attach byte the server broadcasts in the `ActItems`
*  user message (BYTE playerIndex, BYTE attachMask) — client handler FUN_1004fa70.
***/
#ifndef TS_ATTACHMENTS_H
#define TS_ATTACHMENTS_H

void TS_Attachments_Init();    // hook the ActItems message (call from HUD_Init)
void TS_Attachments_VidInit(); // drop cached sprite handles on a map change (call from HUD VidInit)
void TS_Attachments_Draw();    // per-frame render (call from HUD_DrawTransparentTriangles)
bool TS_LocalHasLaser();        // is the LOCAL player's lasersight ACTIVE? (dive-reticle distance/ticks)
int  TS_LocalActiveAttach();    // the LOCAL player's ACTIVE attach mask (laser 2 / flash 4), 0 on error

#endif // TS_ATTACHMENTS_H
