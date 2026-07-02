// ===== FUN @ 000f9f7c size=742 =====

void PM_Duck(void)

{
  int iVar1;
  uint uVar2;
  uint uVar3;
  float fVar4;
  undefined *puVar5;
  uint uVar6;
  longdouble lVar7;
  
  iVar1 = *(int *)PTR_pmove_001772c0;
  uVar6 = (uint)*(ushort *)((int)&__DT_REL[0x9eb].r_info + iVar1 + 2);
  uVar2 = *(uint *)(iVar1 + 200);
  if ((*(byte *)((int)&__DT_REL[0x9eb].r_info + iVar1 + 2) & 4) == 0) {
    *(uint *)(*(int *)PTR_pmove_001772c0 + 200) =
         *(uint *)(*(int *)PTR_pmove_001772c0 + 200) & 0xfffffffb;
  }
  else {
    *(uint *)(iVar1 + 200) = uVar2 | 4;
  }
  puVar5 = PTR_pmove_001772c0;
  iVar1 = *(int *)PTR_pmove_001772c0;
  uVar3 = *(uint *)(iVar1 + 0x208);
  if ((((((uVar3 & 4) == 0) && ((uVar3 & 8) == 0)) && ((uVar3 & 0x20) == 0)) &&
      (((uVar3 & 0x10) == 0 && ((uVar3 & 0x10000) == 0)))) &&
     (((uVar3 & 0x20000) == 0 && ((uVar3 & 0xc00) == 0)))) {
    if ((*(int *)(iVar1 + 0x204) == 0) && (*(int *)(iVar1 + 0xd0) == 0)) {
      iVar1 = *(int *)PTR_pmove_001772c0;
      if ((*(byte *)(iVar1 + 0xb9) & 0x40) != 0) {
        fVar4 = (float)dc_0_333;
        *(float *)((int)&__DT_REL[0x9ea].r_offset + iVar1) =
             fVar4 * *(float *)((int)&__DT_REL[0x9ea].r_offset + iVar1);
        iVar1 = *(int *)puVar5;
        *(float *)((int)&__DT_REL[0x9ea].r_info + iVar1) =
             fVar4 * *(float *)((int)&__DT_REL[0x9ea].r_info + iVar1);
        iVar1 = *(int *)puVar5;
        *(float *)((int)&__DT_REL[0x9eb].r_offset + iVar1) =
             fVar4 * *(float *)((int)&__DT_REL[0x9eb].r_offset + iVar1);
      }
      puVar5 = PTR_pmove_001772c0;
      iVar1 = *(int *)PTR_pmove_001772c0;
      if ((((*(byte *)((int)&__DT_REL[0x9eb].r_info + iVar1 + 2) & 4) != 0) ||
          (*(int *)(iVar1 + 0x90) != 0)) || ((*(byte *)(iVar1 + 0xb9) & 0x40) != 0)) {
        iVar1 = *(int *)PTR_pmove_001772c0;
        if ((*(byte *)((int)&__DT_REL[0x9eb].r_info + iVar1 + 2) & 4) == 0) {
          PM_UnDuck();
        }
        else {
          if ((((uVar6 ^ uVar2) & uVar6 & 4) != 0) && ((*(byte *)(iVar1 + 0xb9) & 0x40) == 0)) {
            *(undefined4 *)(iVar1 + 0x8c) = 0x447a0000;
            *(undefined4 *)(*(int *)puVar5 + 0x90) = 1;
          }
          puVar5 = PTR_pmove_001772c0;
          fVar4 = (float)dc_1 - (float)dc_0_001 * *(float *)(*(int *)PTR_pmove_001772c0 + 0x8c);
          if (fVar4 < (float)dc_0) {
            fVar4 = (float)dc_0;
          }
          iVar1 = *(int *)PTR_pmove_001772c0;
          if (*(int *)(iVar1 + 0x90) != 0) {
            if ((dc_0_001 * (double)*(float *)(iVar1 + 0x8c) <= dc_0_6) ||
               (*(int *)(iVar1 + 0xe0) == -1)) {
              *(undefined4 *)(*(int *)PTR_pmove_001772c0 + 0xbc) = 1;
              *(undefined4 *)(*(int *)puVar5 + 0x88) = 0x41800000;
              *(uint *)(*(int *)puVar5 + 0xb8) = *(uint *)(*(int *)puVar5 + 0xb8) | 0x4000;
              *(undefined4 *)(*(int *)puVar5 + 0x90) = 0;
              iVar1 = *(int *)puVar5;
              if (*(int *)(iVar1 + 0xe0) != -1) {
                *(float *)(iVar1 + 0x38) =
                     *(float *)(iVar1 + 0x38) -
                     (*(float *)((int)&__DT_REL[0x1dfd].r_offset + iVar1) -
                     *(float *)((int)&__DT_REL[0x1dfb].r_info + iVar1));
                iVar1 = *(int *)puVar5;
                *(float *)(iVar1 + 0x3c) =
                     *(float *)(iVar1 + 0x3c) -
                     (*(float *)((int)&__DT_REL[0x1dfd].r_info + iVar1) -
                     *(float *)((int)&__DT_REL[0x1dfc].r_offset + iVar1));
                iVar1 = *(int *)puVar5;
                *(float *)(iVar1 + 0x40) =
                     *(float *)(iVar1 + 0x40) -
                     (*(float *)((int)&__DT_REL[0x1dfe].r_offset + iVar1) -
                     *(float *)((int)&__DT_REL[0x1dfc].r_info + iVar1));
                PM_FixPlayerCrouchStuck(1);
                PM_CatagorizePosition();
              }
            }
            else {
              lVar7 = (longdouble)PM_SplineFraction(fVar4,0x40200000);
              *(float *)(*(int *)PTR_pmove_001772c0 + 0x88) =
                   (float)(((longdouble)fc_1 - lVar7) * (longdouble)fc_28 +
                          ((longdouble)fc_16 - (longdouble)fc_18) * lVar7);
            }
          }
        }
      }
    }
    else if ((*(byte *)(*(int *)PTR_pmove_001772c0 + 0xb9) & 0x40) != 0) {
      PM_UnDuck();
    }
  }
  return;
}


