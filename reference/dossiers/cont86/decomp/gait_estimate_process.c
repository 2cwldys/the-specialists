// ===== FUN @ 10031fa0 size=676 =====

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void __thiscall FUN_10031fa0(int param_1,int param_2)

{
  int iVar1;
  int iVar2;
  float10 fVar3;
  float10 extraout_ST0;
  float10 extraout_ST0_00;
  float10 fVar4;
  longlong lVar5;
  float fStack_10;
  float fStack_c;
  float fStack_8;
  
  fVar3 = (float10)*(double *)(param_1 + 8) - (float10)*(double *)(param_1 + 0x10);
  fStack_10 = (float)fVar3;
  if (fVar3 < (float10)_DAT_100799f0) {
LAB_10032236:
    *(undefined4 *)(param_1 + 0x40) = 0;
    return;
  }
  if (fStack_10 <= (float)_DAT_10079e80) {
    if (fStack_10 == _DAT_100799f0) goto LAB_10032236;
  }
  else {
    fStack_10 = 1.0;
  }
  iVar1 = *(int *)(param_1 + 0x38);
  if (*(int *)(iVar1 + 0x178) == *(int *)(param_1 + 0x20)) goto LAB_10032236;
  if (*(int *)(param_1 + 0x1c) == 0) {
    fStack_8 = *(float *)(param_2 + 0x74);
    fStack_c = *(float *)(param_2 + 0x70);
    fVar3 = SQRT((float10)fStack_c * (float10)fStack_c + (float10)fStack_8 * (float10)fStack_8) *
            (float10)fStack_10;
    *(float *)(param_1 + 0x40) = (float)fVar3;
LAB_100320da:
    if ((fStack_8 != _DAT_100799f0) || (fStack_c != _DAT_100799f0)) {
      fVar4 = (float10)fpatan((float10)fStack_8,(float10)fStack_c);
      *(float *)(*(int *)(param_1 + 0x38) + 0x184) = (float)(fVar4 * (float10)_DAT_1007d538);
      if (_DAT_1007d52c < *(float *)(*(int *)(param_1 + 0x38) + 0x184)) {
        *(undefined4 *)(*(int *)(param_1 + 0x38) + 0x184) = 0x43340000;
      }
      if (*(float *)(*(int *)(param_1 + 0x38) + 0x184) < _DAT_1007d528) {
        *(undefined4 *)(*(int *)(param_1 + 0x38) + 0x184) = 0xc3340000;
      }
      goto LAB_10032216;
    }
  }
  else {
    iVar2 = *(int *)(param_1 + 0x30);
    fStack_c = *(float *)(iVar2 + 0xb48) - *(float *)(iVar1 + 0x188);
    fStack_8 = *(float *)(iVar2 + 0xb4c) - *(float *)(iVar1 + 0x18c);
    *(undefined4 *)(iVar1 + 0x188) = *(undefined4 *)(iVar2 + 0xb48);
    *(undefined4 *)(*(int *)(param_1 + 0x38) + 0x18c) =
         *(undefined4 *)(*(int *)(param_1 + 0x30) + 0xb4c);
    *(undefined4 *)(*(int *)(param_1 + 0x38) + 400) =
         *(undefined4 *)(*(int *)(param_1 + 0x30) + 0xb50);
    fVar3 = SQRT((float10)fStack_c * (float10)fStack_c + (float10)fStack_8 * (float10)fStack_8);
    *(float *)(param_1 + 0x40) = (float)fVar3;
    fVar3 = fVar3 / (float10)fStack_10;
    if ((_DAT_100799f0 < fStack_10) && (_DAT_1007a8d0 <= (float)fVar3)) goto LAB_100320da;
    *(undefined4 *)(param_1 + 0x40) = 0;
  }
  iVar1 = *(int *)(param_1 + 0x38);
  lVar5 = __ftol();
  fVar3 = extraout_ST0 - (float10)((int)lVar5 * 0x168);
  if ((float10)_DAT_1007d52c < fVar3) {
    fVar3 = fVar3 - (float10)_DAT_1007d458;
  }
  if (fVar3 < (float10)_DAT_1007d528) {
    fVar3 = fVar3 + (float10)_DAT_1007d458;
  }
  fVar3 = fVar3 * (float10)fStack_10;
  if (fStack_10 < (float)_DAT_1007a528) {
    fVar3 = fVar3 * (float10)_DAT_1007d454;
  }
  *(float *)(iVar1 + 0x184) = (float)(fVar3 + (float10)*(float *)(iVar1 + 0x184));
  iVar1 = *(int *)(param_1 + 0x38);
  lVar5 = __ftol();
  *(float *)(iVar1 + 0x184) = *(float *)(iVar1 + 0x184) - (float)((int)lVar5 * 0x168);
  *(undefined4 *)(param_1 + 0x40) = 0;
  fVar3 = extraout_ST0_00;
LAB_10032216:
  if ((float10)_DAT_1007d52c <= fVar3) {
    return;
  }
  *(float *)(param_1 + 0x40) = *(float *)(param_1 + 0x40) * (float)_DAT_1007d6a0;
  return;
}


// ===== FUN @ 10032250 size=739 =====

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void __fastcall FUN_10032250(int *param_1)

{
  int iVar1;
  int iVar2;
  float fVar3;
  undefined1 uVar4;
  undefined1 unaff_BP;
  float unaff_EDI;
  int iVar5;
  float10 extraout_ST0;
  float10 fVar6;
  longlong lVar7;
  int iStack_8;
  undefined1 auStack_4 [4];
  
  (**(code **)(*param_1 + 0x60))
            (*(int *)(param_1[0xc] + 0x2dc) * 0xb0 + *(int *)(param_1[0x11] + 0xa8) + param_1[0x11],
             auStack_4,param_1[0xc] + 0xb54);
  *(undefined4 *)(param_1[0xc] + 0xb28) = *(undefined4 *)(param_1[0xc] + 0xb54);
  *(undefined1 *)(param_1[0xc] + 0x31c) = unaff_BP;
  *(undefined1 *)(param_1[0xc] + 0xb40) = *(undefined1 *)(param_1[0xc] + 0x31c);
  *(undefined1 *)(param_1[0xc] + 0xb18) = *(undefined1 *)(param_1[0xc] + 0x31c);
  (**(code **)(*param_1 + 100))(iStack_8);
  iVar5 = param_1[0xe];
  lVar7 = __ftol();
  fVar6 = extraout_ST0 - (float10)((int)lVar7 * 0x168);
  if (fVar6 < (float10)_DAT_1007d528) {
    fVar6 = fVar6 + (float10)_DAT_1007d458;
  }
  if ((float10)_DAT_1007d52c < fVar6) {
    fVar6 = fVar6 - (float10)_DAT_1007d458;
  }
  if (fVar6 <= (float10)_DAT_1007d6bc) {
    if (fVar6 < (float10)_DAT_1007d6b8) {
      *(float *)(iVar5 + 0x184) = *(float *)(iVar5 + 0x184) + _DAT_1007d52c;
      param_1[0x10] = (int)-(float)param_1[0x10];
    }
  }
  else {
    *(float *)(iVar5 + 0x184) = *(float *)(iVar5 + 0x184) - _DAT_1007d52c;
    param_1[0x10] = (int)-(float)param_1[0x10];
  }
  lVar7 = __ftol();
  uVar4 = (undefined1)lVar7;
  *(undefined1 *)(param_1[0xc] + 0x318) = uVar4;
  *(undefined1 *)(param_1[0xc] + 0x319) = uVar4;
  *(undefined1 *)(param_1[0xc] + 0x31a) = uVar4;
  *(undefined1 *)(param_1[0xc] + 0x31b) = uVar4;
  *(undefined1 *)(param_1[0xc] + 0xb3c) = *(undefined1 *)(param_1[0xc] + 0x318);
  *(undefined1 *)(param_1[0xc] + 0xb3d) = *(undefined1 *)(param_1[0xc] + 0x319);
  *(undefined1 *)(param_1[0xc] + 0xb3e) = *(undefined1 *)(param_1[0xc] + 0x31a);
  *(undefined1 *)(param_1[0xc] + 0xb3f) = *(undefined1 *)(param_1[0xc] + 0x31b);
  *(undefined4 *)(param_1[0xc] + 0xb58) = *(undefined4 *)(param_1[0xe] + 0x184);
  iVar5 = param_1[0xc];
  if (*(float *)(iVar5 + 0xb58) < _DAT_100799f0) {
    *(float *)(iVar5 + 0xb58) = *(float *)(iVar5 + 0xb58) + _DAT_1007d458;
  }
  *(undefined4 *)(param_1[0xc] + 0xb2c) = *(undefined4 *)(param_1[0xc] + 0xb58);
  iVar5 = *(int *)(iStack_8 + 0xb8) * 0xb0 + *(int *)(param_1[0x11] + 0xa8) + param_1[0x11];
  if (*(float *)(iVar5 + 0x4c) <= _DAT_100799f0) {
    fVar3 = *(float *)(param_1[0xc] + 0x310) * *(float *)(iVar5 + 0x20) * unaff_EDI;
  }
  else {
    fVar3 = ((float)param_1[0x10] / *(float *)(iVar5 + 0x4c)) * (float)*(int *)(iVar5 + 0x38);
  }
  *(float *)(param_1[0xe] + 0x180) = fVar3 + *(float *)(param_1[0xe] + 0x180);
  iVar1 = *(int *)(iVar5 + 0x38);
  iVar2 = param_1[0xe];
  lVar7 = __ftol();
  *(float *)(iVar2 + 0x180) = *(float *)(iVar2 + 0x180) - (float)((int)lVar7 * iVar1);
  iVar1 = param_1[0xe];
  if (*(float *)(iVar1 + 0x180) < _DAT_100799f0) {
    *(float *)(iVar1 + 0x180) = (float)*(int *)(iVar5 + 0x38) + *(float *)(iVar1 + 0x180);
  }
  return;
}


