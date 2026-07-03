/***
*  ts_gunanchor.h — the per-player GUN-ANCHOR cache (the original's table @0x1018caf8).
*  The client renderer copies each player's 4 computed attachment positions here every frame
*  (player.mdl: att0/1/3 = collinear points along the right-gun bone, att2 = the left/akimbo
*  gun; p_ models carry no attachments). Effect code (muzzle flare, brass, magazines, laser)
*  reads THIS — never cl_entity.attachment, which the engine can reset/own (the pelvis bug).
*  Direction of the gun = normalize(att[1] − att[0]) (and att[3]−att[2] for the left gun).
***/
#pragma once

extern Vector g_tsPlayerGunAtt[65][4];   // [entity index 1..64][attachment 0..3]
extern float g_tsPlayerGunAttTime[65];   // client time of the last refresh (staleness check)

// The anchor for player idx, attachment n — falls back to the entity origin when the player
// has not been drawn yet this session.
const float* TS_PlayerGunAtt(int idx, int n);
