// ===== FUN @ 000ffbb0 size=115 =====

void PM_AddGravity(void)

{
  int iVar1;
  float fVar2;
  undefined *puVar3;
  
  puVar3 = PTR_pmove_001772c0;
  fVar2 = fc_1;
  if (fc_0 != *(float *)(*(int *)PTR_pmove_001772c0 + 0xc0)) {
    fVar2 = *(float *)(*(int *)PTR_pmove_001772c0 + 0xc0);
  }
  iVar1 = *(int *)PTR_pmove_001772c0;
  *(float *)(iVar1 + 100) =
       *(float *)(iVar1 + 100) -
       fVar2 * **(float **)((int)&__DT_REL[0x1dfb].r_offset + iVar1) * *(float *)(iVar1 + 0x10);
  iVar1 = *(int *)puVar3;
  *(float *)(iVar1 + 100) =
       *(float *)(iVar1 + 0x7c) * *(float *)(iVar1 + 0x10) + *(float *)(iVar1 + 100);
  *(undefined4 *)(*(int *)puVar3 + 0x7c) = 0;
  PM_CheckVelocity();
  return;
}


// ===== FUN @ 000fd304 size=140 =====

void PM_AddCorrectGravity(void)

{
  int iVar1;
  float fVar2;
  undefined *puVar3;
  
  puVar3 = PTR_pmove_001772c0;
  iVar1 = *(int *)PTR_pmove_001772c0;
  if (fc_0 == *(float *)(iVar1 + 0xcc)) {
    fVar2 = fc_1;
    if (fc_0 != *(float *)(iVar1 + 0xc0)) {
      fVar2 = *(float *)(iVar1 + 0xc0);
    }
    iVar1 = *(int *)PTR_pmove_001772c0;
    *(float *)(iVar1 + 100) =
         *(float *)(iVar1 + 100) -
         (float)dc_0_5 * fVar2 * **(float **)((int)&__DT_REL[0x1dfb].r_offset + iVar1) *
         *(float *)(iVar1 + 0x10);
    iVar1 = *(int *)puVar3;
    *(float *)(iVar1 + 100) =
         *(float *)(iVar1 + 0x7c) * *(float *)(iVar1 + 0x10) + *(float *)(iVar1 + 100);
    *(undefined4 *)(*(int *)puVar3 + 0x7c) = 0;
    PM_CheckVelocity();
  }
  return;
}


// ===== FUN @ 000fd390 size=100 =====

void PM_FixupGravityVelocity(void)

{
  int iVar1;
  float fVar2;
  
  iVar1 = *(int *)PTR_pmove_001772c0;
  if (fc_0 == *(float *)(iVar1 + 0xcc)) {
    fVar2 = fc_1;
    if (fc_0 != *(float *)(iVar1 + 0xc0)) {
      fVar2 = *(float *)(iVar1 + 0xc0);
    }
    iVar1 = *(int *)PTR_pmove_001772c0;
    *(float *)(iVar1 + 100) =
         *(float *)(iVar1 + 100) -
         (float)dc_0_5 *
         fVar2 * **(float **)((int)&__DT_REL[0x1dfb].r_offset + iVar1) * *(float *)(iVar1 + 0x10);
    PM_CheckVelocity();
  }
  return;
}


