// ===== FUN @ 000fe434 size=310 =====

void PM_AirMove(void)

{
  float fVar1;
  float fVar2;
  int iVar3;
  undefined *puVar4;
  longdouble lVar5;
  float local_2c;
  float local_28;
  undefined4 local_24;
  float local_1c;
  float local_18;
  undefined4 local_14;
  
  puVar4 = PTR_pmove_001772c0;
  iVar3 = *(int *)PTR_pmove_001772c0;
  fVar1 = *(float *)((int)&__DT_REL[0x9ea].r_offset + iVar3);
  fVar2 = *(float *)((int)&__DT_REL[0x9ea].r_info + iVar3);
  *(undefined4 *)(iVar3 + 0x1c) = 0;
  *(undefined4 *)(*(int *)puVar4 + 0x28) = 0;
  VectorNormalize(*(int *)puVar4 + 0x14);
  VectorNormalize(*(int *)puVar4 + 0x20);
  iVar3 = *(int *)puVar4;
  local_2c = fVar2 * *(float *)(iVar3 + 0x20) + fVar1 * *(float *)(iVar3 + 0x14);
  local_28 = fVar2 * *(float *)(iVar3 + 0x24) + fVar1 * *(float *)(iVar3 + 0x18);
  local_14 = 0;
  local_24 = 0;
  local_1c = local_2c;
  local_18 = local_28;
  lVar5 = (longdouble)VectorNormalize(&local_2c);
  if ((longdouble)*(float *)(*(int *)puVar4 + 500) < lVar5) {
    VectorScale(&local_1c,(float)((longdouble)*(float *)(*(int *)puVar4 + 500) / lVar5),&local_1c);
    lVar5 = (longdouble)*(float *)(*(int *)puVar4 + 500);
  }
  puVar4 = PTR_pmove_001772c0;
  PM_AirAccelerate(&local_2c,(float)lVar5,
                   *(undefined4 *)
                    (*(int *)((int)&__DT_REL[0x1dfb].r_offset + *(int *)PTR_pmove_001772c0) + 0x14))
  ;
  iVar3 = *(int *)puVar4;
  *(float *)(iVar3 + 0x5c) = *(float *)(iVar3 + 0x5c) + *(float *)(iVar3 + 0x74);
  iVar3 = *(int *)puVar4;
  *(float *)(iVar3 + 0x60) = *(float *)(iVar3 + 0x60) + *(float *)(iVar3 + 0x78);
  iVar3 = *(int *)puVar4;
  *(float *)(iVar3 + 100) = *(float *)(iVar3 + 100) + *(float *)(iVar3 + 0x7c);
  PM_FlyMove();
  return;
}


