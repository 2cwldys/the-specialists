// ===== FUN @ 1002fef0 size=231 =====

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void __cdecl FUN_1002fef0(float *param_1,float *param_2)

{
  float10 fVar1;
  float10 fVar2;
  
  fVar1 = (float10)FUN_1006c9b0();
  param_1[1] = (float)fVar1;
  fVar2 = fVar1;
  if (fVar1 <= (float10)_DAT_100799f0) {
    fVar2 = -fVar1;
  }
  if (fVar2 - (float10)_DAT_1007d5e0 <= (float10)_DAT_1007a878) {
    if (fVar1 <= (float10)_DAT_100799f0) {
      fVar1 = -fVar1;
    }
    fVar1 = -(fVar1 - (float10)_DAT_1007d5e0);
  }
  else {
    if (fVar1 <= (float10)_DAT_100799f0) {
      fVar1 = -fVar1;
    }
    fVar1 = fVar1 - (float10)_DAT_1007d5e0;
  }
  if (fVar1 < (float10)_DAT_1007d5f0) {
    *param_1 = 0.0;
    fVar2 = (float10)fpatan(-(float10)param_2[1],(float10)param_2[5]);
    param_1[2] = (float)fVar2;
    return;
  }
  fVar2 = (float10)fpatan((float10)param_2[9],(float10)param_2[10]);
  *param_1 = (float)fVar2;
  fVar2 = (float10)fpatan((float10)param_2[4],(float10)*param_2);
  param_1[2] = (float)fVar2;
  return;
}


