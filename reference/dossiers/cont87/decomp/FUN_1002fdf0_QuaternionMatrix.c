// ===== FUN @ 1002fdf0 size=219 =====

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void __cdecl FUN_1002fdf0(float *param_1,float *param_2)

{
  float fVar1;
  float fVar2;
  
  fVar1 = param_1[1] * param_1[1];
  fVar2 = param_1[2] * param_1[2];
  *param_2 = ((float)_DAT_10079e80 - (fVar1 + fVar1)) - (fVar2 + fVar2);
  fVar1 = *param_1 * param_1[1] + param_1[2] * param_1[3];
  param_2[4] = fVar1 + fVar1;
  param_2[8] = (param_1[2] * *param_1 + param_1[2] * *param_1) -
               (param_1[3] * param_1[1] + param_1[3] * param_1[1]);
  param_2[1] = (*param_1 * param_1[1] + *param_1 * param_1[1]) -
               (param_1[2] * param_1[3] + param_1[2] * param_1[3]);
  fVar1 = *param_1 * *param_1;
  fVar2 = param_1[2] * param_1[2];
  param_2[5] = ((float)_DAT_10079e80 - (fVar1 + fVar1)) - (fVar2 + fVar2);
  fVar1 = *param_1 * param_1[3] + param_1[2] * param_1[1];
  param_2[9] = fVar1 + fVar1;
  fVar1 = param_1[3] * param_1[1] + param_1[2] * *param_1;
  param_2[2] = fVar1 + fVar1;
  param_2[6] = (param_1[2] * param_1[1] + param_1[2] * param_1[1]) -
               (*param_1 * param_1[3] + *param_1 * param_1[3]);
  fVar1 = *param_1 * *param_1;
  fVar2 = param_1[1] * param_1[1];
  param_2[10] = ((float)_DAT_10079e80 - (fVar1 + fVar1)) - (fVar2 + fVar2);
  return;
}


