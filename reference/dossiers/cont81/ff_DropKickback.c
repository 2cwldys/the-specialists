// ===== FUN @ 00086ea0 size=176 =====

/* WARNING: Unknown calling convention -- yet parameter storage is locked */
/* TS_gun.cpp */

void DropKickback__FPfT0ff(float *a0,float *a1,float a2,float a3)

{
  float fVar1;
  float fVar2;
  float fVar3;
  float fVar4;
  float fVar5;
  longdouble lVar6;
  
  if ((fc_0 <= a2) && (fVar2 = SQRT(a0[1] * a0[1] + *a0 * *a0), fc_0 < fVar2)) {
    fVar3 = *a0 - *a1;
    fVar5 = a0[1] - a1[1];
    fVar4 = SQRT(fVar5 * fVar5 + fVar3 * fVar3);
    fVar1 = *a1;
    lVar6 = (longdouble)FixedSlowMul__Ff(a3);
    if ((longdouble)dc_15 * ((longdouble)a2 / lVar6) <= (longdouble)fVar4) {
      lVar6 = (longdouble)FixedSlowMul__Ff(a3);
      lVar6 = (longdouble)fVar3 * (longdouble)dc_15 * ((longdouble)a2 / lVar6);
    }
    else {
      lVar6 = (longdouble)fVar3 * (longdouble)fVar4;
    }
    *a1 = (float)(lVar6 + (longdouble)fVar1);
    fVar1 = a1[1];
    lVar6 = (longdouble)FixedSlowMul__Ff(a3);
    if ((longdouble)dc_15 * ((longdouble)a2 / lVar6) <= (longdouble)fVar4) {
      lVar6 = (longdouble)FixedSlowMul__Ff(a3);
      lVar6 = (longdouble)fVar5 * (longdouble)dc_15 * ((longdouble)a2 / lVar6);
    }
    else {
      lVar6 = (longdouble)fVar5 * (longdouble)fVar4;
    }
    a1[1] = (float)(lVar6 + (longdouble)fVar1);
    *a0 = *a0 / fVar2;
    a0[1] = a0[1] / fVar2;
    lVar6 = (longdouble)FixedSlowMul__Ff(a3);
    fVar2 = (float)((longdouble)fVar2 - (longdouble)dc_15 * ((longdouble)a2 / lVar6));
    if (fVar2 < (float)dc_0) {
      fVar2 = (float)dc_0;
    }
    *a0 = fVar2 * *a0;
    a0[1] = fVar2 * a0[1];
  }
  return;
}
