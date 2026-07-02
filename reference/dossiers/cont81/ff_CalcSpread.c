// ===== FUN @ 0008020c size=4 =====

/* WARNING: Unknown calling convention -- yet parameter storage is locked */
/* TS_gun.cpp */

void CalcSpread__FRfP11CBasePlayerf(float *a0,CBasePlayer *a1,float a2)

{
  float fVar1;
  float fVar2;
  float fVar3;
  entvars_t *peVar4;
  
  peVar4 = a1->pev;
  fVar1 = (peVar4->velocity).x;
  fVar2 = (peVar4->velocity).y;
  fVar3 = (peVar4->velocity).z;
  spread_mul_1221 = SQRT(fVar3 * fVar3 + fVar2 * fVar2 + fVar1 * fVar1) / ((float)dc_100 / a2);
  if (spread_mul_1221 < (float)dc_1) {
    spread_mul_1221 = 1.0;
  }
  if ((float)dc_3 * a2 < spread_mul_1221) {
    spread_mul_1221 = (float)dc_3 * a2;
  }
  *a0 = *a0 * spread_mul_1221;
  return;
}
