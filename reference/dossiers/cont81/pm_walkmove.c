// ===== FUN @ 000fa26c size=1286 =====

void PM_WalkMove(void)

{
  float fVar1;
  int iVar2;
  float fVar3;
  undefined *puVar4;
  undefined *puVar5;
  int iVar6;
  longdouble lVar7;
  float local_e4;
  int local_e0;
  undefined1 local_dc [8];
  undefined4 local_d4;
  float local_d0;
  undefined4 local_cc;
  undefined4 local_c8;
  undefined4 local_c4;
  float local_c0;
  undefined4 local_98;
  undefined4 local_94;
  undefined4 local_90;
  undefined4 local_8c;
  float local_88;
  float local_84;
  undefined4 local_80;
  undefined4 local_7c;
  undefined4 local_78;
  undefined4 local_70;
  undefined4 local_6c;
  float local_68;
  float local_64;
  undefined4 local_60;
  undefined4 local_5c;
  undefined4 local_58;
  float local_4c;
  float local_48;
  undefined4 local_44;
  undefined4 local_40;
  float local_3c;
  float local_38;
  undefined4 local_34;
  float local_2c;
  float local_28;
  undefined4 local_24;
  float local_1c;
  float local_18;
  undefined4 local_14;
  
  puVar4 = PTR_pmove_001772c0;
  local_e4 = *(float *)((int)&__DT_REL[0x9ea].r_offset + *(int *)PTR_pmove_001772c0);
  if (local_e4 < fc_0) {
    local_e4 = (float)dc_0_8 * local_e4;
  }
  fVar1 = *(float *)((int)&__DT_REL[0x9ea].r_info + *(int *)PTR_pmove_001772c0);
  *(undefined4 *)(*(int *)PTR_pmove_001772c0 + 0x1c) = 0;
  *(undefined4 *)(*(int *)puVar4 + 0x28) = 0;
  VectorNormalize(*(int *)puVar4 + 0x14);
  VectorNormalize(*(int *)puVar4 + 0x20);
  iVar6 = *(int *)puVar4;
  local_2c = fVar1 * *(float *)(iVar6 + 0x20) + local_e4 * *(float *)(iVar6 + 0x14);
  local_28 = fVar1 * *(float *)(iVar6 + 0x24) + local_e4 * *(float *)(iVar6 + 0x18);
  local_14 = 0;
  local_24 = 0;
  local_1c = local_2c;
  local_18 = local_28;
  lVar7 = (longdouble)VectorNormalize(&local_2c);
  if ((longdouble)*(float *)(*(int *)puVar4 + 500) < lVar7) {
    VectorScale(&local_1c,(float)((longdouble)*(float *)(*(int *)puVar4 + 500) / lVar7),&local_1c);
    lVar7 = (longdouble)*(float *)(*(int *)puVar4 + 500);
  }
  puVar4 = PTR_pmove_001772c0;
  *(undefined4 *)(*(int *)PTR_pmove_001772c0 + 100) = 0;
  PM_Accelerate(&local_2c,(float)lVar7,
                *(undefined4 *)(*(int *)((int)&__DT_REL[0x1dfb].r_offset + *(int *)puVar4) + 0x10));
  *(undefined4 *)(*(int *)puVar4 + 100) = 0;
  iVar6 = *(int *)puVar4;
  *(float *)(iVar6 + 0x5c) = *(float *)(iVar6 + 0x5c) + *(float *)(iVar6 + 0x74);
  iVar6 = *(int *)puVar4;
  *(float *)(iVar6 + 0x60) = *(float *)(iVar6 + 0x60) + *(float *)(iVar6 + 0x78);
  iVar6 = *(int *)puVar4;
  *(float *)(iVar6 + 100) = *(float *)(iVar6 + 100) + *(float *)(iVar6 + 0x7c);
  lVar7 = (longdouble)Length(*(int *)puVar4 + 0x5c);
  puVar5 = PTR_pmove_001772c0;
  if (lVar7 < (longdouble)fc_1) {
    *(undefined4 *)(*(int *)puVar4 + 0x5c) = 0;
    *(undefined4 *)(*(int *)puVar4 + 0x60) = 0;
    *(undefined4 *)(*(int *)puVar4 + 100) = 0;
    return;
  }
  iVar6 = *(int *)PTR_pmove_001772c0;
  iVar2 = *(int *)(iVar6 + 0xe0);
  local_4c = *(float *)(iVar6 + 0x5c) * *(float *)(iVar6 + 0x10) + *(float *)(iVar6 + 0x38);
  local_48 = *(float *)(iVar6 + 0x60) * *(float *)(iVar6 + 0x10) + *(float *)(iVar6 + 0x3c);
  local_44 = *(undefined4 *)(iVar6 + 0x40);
  local_3c = local_4c;
  local_38 = local_48;
  local_34 = local_44;
  (**(code **)((int)&__DT_REL[0x1e0d].r_offset + iVar6))
            (local_dc,iVar6 + 0x38,&local_3c,0,0xffffffff);
  puVar4 = PTR_pmove_001772c0;
  if (fc_1 == local_d0) {
    *(undefined4 *)(*(int *)puVar5 + 0x38) = local_cc;
    *(undefined4 *)(*(int *)puVar5 + 0x3c) = local_c8;
    *(undefined4 *)(*(int *)puVar5 + 0x40) = local_c4;
    return;
  }
  if ((iVar2 == -1) && (*(int *)(*(int *)PTR_pmove_001772c0 + 0xe4) == 0)) {
    return;
  }
  iVar6 = *(int *)PTR_pmove_001772c0;
  if (fc_0 != *(float *)(iVar6 + 0xcc)) {
    return;
  }
  local_60 = *(undefined4 *)(iVar6 + 0x38);
  local_5c = *(undefined4 *)(iVar6 + 0x3c);
  local_58 = *(undefined4 *)(iVar6 + 0x40);
  local_70 = *(undefined4 *)(iVar6 + 0x5c);
  local_6c = *(undefined4 *)(iVar6 + 0x60);
  local_68 = *(float *)(iVar6 + 100);
  PM_FlyMove();
  iVar6 = *(int *)puVar4;
  local_80 = *(undefined4 *)(iVar6 + 0x38);
  local_7c = *(undefined4 *)(iVar6 + 0x3c);
  local_78 = *(undefined4 *)(iVar6 + 0x40);
  local_90 = *(undefined4 *)(iVar6 + 0x5c);
  local_8c = *(undefined4 *)(iVar6 + 0x60);
  local_88 = *(float *)(iVar6 + 100);
  *(undefined4 *)(iVar6 + 0x38) = local_60;
  *(undefined4 *)(*(int *)puVar4 + 0x3c) = local_5c;
  *(undefined4 *)(*(int *)puVar4 + 0x40) = local_58;
  *(undefined4 *)(*(int *)puVar4 + 0x5c) = local_70;
  *(undefined4 *)(*(int *)puVar4 + 0x60) = local_6c;
  *(float *)(*(int *)puVar4 + 100) = local_68;
  iVar6 = *(int *)puVar4;
  local_40 = *(undefined4 *)(iVar6 + 0x38);
  local_3c = *(float *)(iVar6 + 0x3c);
  local_38 = *(float *)(iVar6 + 0x40) +
             *(float *)(*(int *)((int)&__DT_REL[0x1dfb].r_offset + iVar6) + 0x30);
  (**(code **)((int)&__DT_REL[0x1e0d].r_offset + iVar6))
            (&local_e0,iVar6 + 0x38,&local_40,0,0xffffffff);
  if ((local_e0 == 0) && (local_e4 == 0.0)) {
    *(float *)(*(int *)puVar4 + 0x38) = local_d0;
    *(undefined4 *)(*(int *)puVar4 + 0x3c) = local_cc;
    *(undefined4 *)(*(int *)puVar4 + 0x40) = local_c8;
  }
  PM_FlyMove();
  puVar4 = PTR_pmove_001772c0;
  iVar6 = *(int *)PTR_pmove_001772c0;
  local_44 = *(undefined4 *)(iVar6 + 0x38);
  local_40 = *(undefined4 *)(iVar6 + 0x3c);
  local_3c = *(float *)(iVar6 + 0x40) -
             *(float *)(*(int *)((int)&__DT_REL[0x1dfb].r_offset + iVar6) + 0x30);
  (**(code **)((int)&__DT_REL[0x1e0d].r_offset + iVar6))
            (&local_e4,iVar6 + 0x38,&local_44,0,0xffffffff);
  if ((float)dc_0_7 <= local_c0) {
    if ((local_e4 == 0.0) && (fVar1 == 0.0)) {
      *(undefined4 *)(*(int *)puVar4 + 0x38) = local_d4;
      *(float *)(*(int *)puVar4 + 0x3c) = local_d0;
      *(undefined4 *)(*(int *)puVar4 + 0x40) = local_cc;
    }
    puVar4 = PTR_pmove_001772c0;
    *(undefined4 *)(*(int *)PTR_pmove_001772c0 + 0x2c) =
         *(undefined4 *)(*(int *)PTR_pmove_001772c0 + 0x38);
    *(undefined4 *)(*(int *)puVar4 + 0x30) = *(undefined4 *)(*(int *)puVar4 + 0x3c);
    *(undefined4 *)(*(int *)puVar4 + 0x34) = *(undefined4 *)(*(int *)puVar4 + 0x40);
    fVar1 = *(float *)(*(int *)puVar4 + 0x2c) - local_68;
    fVar3 = *(float *)(*(int *)puVar4 + 0x30) - local_64;
    if ((local_84 - local_64) * (local_84 - local_64) +
        (local_88 - local_68) * (local_88 - local_68) <= fVar3 * fVar3 + fVar1 * fVar1) {
      iVar6 = *(int *)PTR_pmove_001772c0;
      goto LAB_000fa768;
    }
  }
  puVar4 = PTR_pmove_001772c0;
  *(float *)(*(int *)PTR_pmove_001772c0 + 0x38) = local_88;
  *(float *)(*(int *)puVar4 + 0x3c) = local_84;
  *(undefined4 *)(*(int *)puVar4 + 0x40) = local_80;
  *(undefined4 *)(*(int *)puVar4 + 0x5c) = local_98;
  *(undefined4 *)(*(int *)puVar4 + 0x60) = local_94;
  iVar6 = *(int *)puVar4;
LAB_000fa768:
  *(undefined4 *)(iVar6 + 100) = local_90;
  return;
}


