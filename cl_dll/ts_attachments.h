/***
*  ts_attachments.h — the ActItems / PlInfo attachment state, the lasersights and flashlights drawn in
*  the world (client.dll 0x1003bb00, 0x1003d600, 0x1003b250, 0x1003b750) and the player blob shadows.
***/
#ifndef TS_ATTACHMENTS_H
#define TS_ATTACHMENTS_H

void TS_Attachments_Init();    // hook the ActItems message (call from HUD_Init)
void TS_Attachments_VidInit(); // drop cached sprite handles on a map change (call from HUD VidInit)
void TS_Attachments_Draw();    // per-frame render (call from HUD_DrawTransparentTriangles)
void TS_Shadows_Draw();        // per-frame player blob shadows (ombra.spr; call from HUD_DrawTransparentTriangles)
bool TS_LocalHasLaser();        // the local player's ActItems laser bit is set
int  TS_LocalActiveAttach();    // the local player's ActItems byte (laser 2, flashlight 4, akimbo 0x40)

// HUD_AddEntity lets player `index` through this frame (client.dll 0x1018bd44); the passes draw only
// those, and the flags are cleared after them.
void TS_AttachPlayerAdded(int index);
bool TS_AttachPlayerWasAdded(int index); // read by the fire events' third-person gun smoke

// The local laser as the weapon bar keeps it after the pass: beam end `which` (0, or 1 for the akimbo
// second beam) projected to the screen, -1 when off screen, and the laser colour; false without the
// laser on. TS_LocalLaserRange is the range number (+0x18).
bool TS_LocalLaserDot(int which, float& x, float& y, int& r, int& g, int& b);
int TS_LocalLaserRange();

#endif // TS_ATTACHMENTS_H
