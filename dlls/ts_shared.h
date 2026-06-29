/***
*  ts_shared.h — the TS slow-motion math helpers (original TS_shared.cpp). Pure float math, no deps,
*  shared server+client. The slow factor `s` is pev->fuser1 (1 = normal, <1 = slowed).
***/
#ifndef TS_SHARED_H
#define TS_SHARED_H

float FixedSlowAmt(float s);                              // @0x890bc — clamp the slow factor: max(s, 0.01)
float FixedSlowMul(float s);                              // @0x890e4 — its inverse: 1 / FixedSlowAmt(s)
void  InterpolateSlowMotion(float scale, float *cur, float target); // @0x8910c — ease *cur toward target
float GetBulletVelocityMult(float slow, int pause);      // @0x89160 — a slow-mo bullet's speed fraction

#endif // TS_SHARED_H
