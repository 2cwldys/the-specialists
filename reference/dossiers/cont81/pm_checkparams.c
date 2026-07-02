// ===== FUN @ 000f9c94 size=733 =====

void PM_CheckParamters(void)

{
  float fVar1;
  float fVar2;
  int iVar3;
  uint *puVar4;
  undefined *puVar5;
  float fVar6;
  longdouble lVar7;
  float local_1c;
  float local_18;
  float local_14;
  
  iVar3 = *(int *)PTR_pmove_001772c0;
  fVar6 = *(float *)((int)&__DT_REL[0x9ea].r_offset + iVar3);
  fVar1 = *(float *)((int)&__DT_REL[0x9ea].r_info + iVar3);
  fVar2 = *(float *)((int)&__DT_REL[0x9eb].r_offset + iVar3);
  fVar1 = SQRT(fVar2 * fVar2 + fVar1 * fVar1 + fVar6 * fVar6);
  fVar6 = *(float *)(iVar3 + 0x1f8);
  if (fVar6 != (float)dc_0) {
    fVar2 = *(float *)(iVar3 + 500);
    if (fVar6 < fVar2) {
      fVar2 = fVar6;
    }
    *(float *)(iVar3 + 500) = fVar2;
  }
  puVar5 = PTR_pmove_001772c0;
  if (((fVar1 != (float)dc_0) &&
      (iVar3 = *(int *)PTR_pmove_001772c0, *(float *)(iVar3 + 500) < fVar1)) &&
     (fVar1 = *(float *)(iVar3 + 500) / fVar1, 0 < *(int *)(iVar3 + 0x1fc))) {
    *(float *)((int)&__DT_REL[0x9ea].r_offset + iVar3) =
         fVar1 * *(float *)((int)&__DT_REL[0x9ea].r_offset + iVar3);
    iVar3 = *(int *)puVar5;
    *(float *)((int)&__DT_REL[0x9ea].r_info + iVar3) =
         fVar1 * *(float *)((int)&__DT_REL[0x9ea].r_info + iVar3);
    iVar3 = *(int *)puVar5;
    *(float *)((int)&__DT_REL[0x9eb].r_offset + iVar3) =
         fVar1 * *(float *)((int)&__DT_REL[0x9eb].r_offset + iVar3);
  }
  puVar5 = PTR_pmove_001772c0;
  iVar3 = *(int *)PTR_pmove_001772c0;
  if ((((*(uint *)(iVar3 + 0xb8) & 0x1001000) != 0) || (*(int *)(iVar3 + 0xd0) != 0)) ||
     ((*(uint *)(iVar3 + 0x208) & 0x10c10) != 0)) {
    *(undefined4 *)((int)&__DT_REL[0x9ea].r_offset + *(int *)PTR_pmove_001772c0) = 0;
    *(undefined4 *)((int)&__DT_REL[0x9ea].r_info + *(int *)puVar5) = 0;
    *(undefined4 *)((int)&__DT_REL[0x9eb].r_offset + *(int *)puVar5) = 0;
  }
  puVar5 = PTR_pmove_001772c0;
  iVar3 = *(int *)PTR_pmove_001772c0;
  if ((*(byte *)(iVar3 + 0x20a) & 0x10) != 0) {
    *(float *)((int)&__DT_REL[0x9ea].r_offset + iVar3) =
         (float)dc_0_5 * *(float *)((int)&__DT_REL[0x9ea].r_offset + iVar3);
    fVar6 = (float)dc_0_25;
    *(float *)((int)&__DT_REL[0x9ea].r_info + *(int *)puVar5) =
         fVar6 * *(float *)((int)&__DT_REL[0x9ea].r_info + *(int *)puVar5);
    *(float *)((int)&__DT_REL[0x9eb].r_offset + *(int *)puVar5) =
         fVar6 * *(float *)((int)&__DT_REL[0x9eb].r_offset + *(int *)puVar5);
    iVar3 = *(int *)puVar5;
    if ((*(byte *)((int)&__DT_REL[0x9eb].r_info + iVar3 + 2) & 2) == 0) {
      *(undefined4 *)(*(int *)PTR_pmove_001772c0 + 0xc0) = 0x3f800000;
    }
    else {
      fVar6 = fc_0_30000001192092896;
      if (*(float *)(iVar3 + 100) < fc_0) {
        fVar6 = fc_0_75;
      }
      *(float *)(iVar3 + 0xc0) = fVar6;
    }
  }
  puVar5 = PTR_pmove_001772c0;
  PM_DropPunchAngle(*(int *)PTR_pmove_001772c0 + 0xa0);
  iVar3 = *(int *)puVar5;
  if ((*(int *)(iVar3 + 0xd0) == 0) && ((*(byte *)(iVar3 + 0x208) & 2) == 0)) {
    local_1c = *(float *)((int)&__DT_REL[0x9e8].r_info + iVar3) + *(float *)(iVar3 + 0xa0);
    local_18 = *(float *)((int)&__DT_REL[0x9e9].r_offset + iVar3) + *(float *)(iVar3 + 0xa4);
    local_14 = *(float *)((int)&__DT_REL[0x9e9].r_info + iVar3) + *(float *)(iVar3 + 0xa8);
    lVar7 = (longdouble)
            PM_CalcRoll(&local_1c,iVar3 + 0x5c,
                        *(undefined4 *)(*(int *)((int)&__DT_REL[0x1dfb].r_offset + iVar3) + 100),
                        *(undefined4 *)(*(int *)((int)&__DT_REL[0x1dfb].r_offset + iVar3) + 0x68));
    *(float *)(*(int *)puVar5 + 0x4c) = (float)(lVar7 * (longdouble)fc_4);
    *(float *)(*(int *)puVar5 + 0x44) = local_1c;
    *(float *)(*(int *)puVar5 + 0x48) = local_18;
    puVar4 = *(uint **)puVar5;
    if (*puVar4 < 0x20) {
      *(uint *)(OldrotArray + *puVar4 * 4) = puVar4[0x12];
    }
  }
  else {
    puVar4 = *(uint **)PTR_pmove_001772c0;
    if (*puVar4 < 0x20) {
      puVar4[0x12] = *(uint *)(OldrotArray + *puVar4 * 4);
    }
  }
  if (*(int *)(*(int *)PTR_pmove_001772c0 + 0xd0) != 0) {
    *(undefined4 *)(*(int *)PTR_pmove_001772c0 + 0x88) = 0xc1000000;
  }
  fVar6 = *(float *)(*(int *)PTR_pmove_001772c0 + 0x48);
  if (fc_180 < fVar6) {
    *(float *)(*(int *)PTR_pmove_001772c0 + 0x48) = fVar6 - fc_360;
  }
  return;
}


