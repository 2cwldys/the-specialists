// ===== FUN @ 10068e20 size=201 =====

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void __cdecl FUN_10068e20(int param_1,undefined4 *param_2,float *param_3)

{
  float fVar1;
  int iVar2;
  
  iVar2 = (*DAT_100d5bfc)(param_1);
  if (((iVar2 != 0) && (*(int *)(iVar2 + 4) != 0)) &&
     (*(int *)(&DAT_1018fbd8 + param_1 * 0x14) != 0)) {
    *param_2 = *(undefined4 *)(iVar2 + 0xb48);
    param_2[1] = *(undefined4 *)(iVar2 + 0xb4c);
    param_2[2] = *(undefined4 *)(iVar2 + 0xb50);
    *param_3 = *(float *)(iVar2 + 0xb54);
    fVar1 = (float)_DAT_10081e78;
    param_3[1] = *(float *)(iVar2 + 0xb58);
    param_3[2] = *(float *)(iVar2 + 0xb5c);
    *param_3 = *param_3 * fVar1;
    if (*(short *)(iVar2 + 0x2ea) == 0) {
      param_3[2] = 80.0;
      param_2[2] = (float)param_2[2] - _DAT_1007ccf0;
      return;
    }
    if ((*(uint *)(iVar2 + 0x3c0) & 0x400000) == 0) {
      if (*(int *)(iVar2 + 0x378) == 1) {
        param_2[2] = (float)param_2[2] + _DAT_100799f4;
        return;
      }
      param_2[2] = (float)param_2[2] + _DAT_1007d810;
    }
  }
  return;
}


