// ===== FUN @ 1002fbd0 size=150 =====

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void __cdecl FUN_1002fbd0(float *param_1,float *param_2)

{
  float fVar1;
  float10 fVar2;
  float10 fVar3;
  float10 fVar4;
  float10 fVar5;
  float10 fVar6;
  float10 fVar7;
  float10 fVar8;
  
  fVar2 = (float10)fsin((float10)param_1[2] * (float10)_DAT_10079e90);
  fVar1 = (float)fVar2;
  fVar2 = (float10)fcos((float10)param_1[2] * (float10)_DAT_10079e90);
  fVar3 = (float10)fsin((float10)param_1[1] * (float10)_DAT_10079e90);
  fVar4 = (float10)fcos((float10)param_1[1] * (float10)_DAT_10079e90);
  fVar5 = (float10)fsin((float10)*param_1 * (float10)_DAT_10079e90);
  fVar6 = (float10)fcos((float10)*param_1 * (float10)_DAT_10079e90);
  fVar7 = (float10)(float)fVar5 * fVar4;
  fVar8 = fVar6 * (float10)(float)fVar3;
  *param_2 = (float)(fVar7 * fVar2 - fVar8 * (float10)fVar1);
  param_2[1] = (float)(fVar7 * (float10)fVar1 + fVar8 * fVar2);
  fVar3 = (float10)(float)fVar5 * (float10)(float)fVar3;
  param_2[2] = (float)(fVar6 * fVar4 * (float10)fVar1 - fVar3 * fVar2);
  param_2[3] = (float)(fVar6 * fVar4 * fVar2 + fVar3 * (float10)fVar1);
  return;
}


