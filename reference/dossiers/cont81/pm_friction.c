// ===== FUN @ 000fde08 size=163 =====

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void PM_Friction(void)

{
  float fVar1;
  int iVar2;
  int iVar3;
  float fVar4;
  undefined *puVar5;
  undefined4 *puVar6;
  undefined4 uStack_b0;
  undefined1 auStack_ac [4];
  float fStack_a8;
  float local_a4;
  float local_a0;
  undefined1 auStack_9c [12];
  float fStack_90;
  float local_4c;
  float local_48;
  float fStack_44;
  float local_3c;
  float local_38;
  float fStack_34;
  
  puVar5 = PTR_pmove_001772c0;
  puVar6 = (undefined4 *)auStack_ac;
  uStack_b0 = 0xfde16;
  iVar2 = *(int *)PTR_pmove_001772c0;
  if (fc_0 == *(float *)(iVar2 + 0xcc)) {
    local_a0 = SQRT(*(float *)(iVar2 + 100) * *(float *)(iVar2 + 100) +
                    *(float *)(iVar2 + 0x60) * *(float *)(iVar2 + 0x60) +
                    *(float *)(iVar2 + 0x5c) * *(float *)(iVar2 + 0x5c));
    if (fc_0_10000000149011612 <= local_a0) {
      local_a4 = fc_0;
      if (*(int *)(iVar2 + 0xe0) != -1) {
        local_4c = (*(float *)(iVar2 + 0x5c) / local_a0) * fc_16 + *(float *)(iVar2 + 0x38);
        local_48 = (*(float *)(iVar2 + 0x60) / local_a0) * fc_16 + *(float *)(iVar2 + 0x3c);
        fStack_34 = *(float *)(iVar2 + 0x40) +
                    *(float *)(iVar2 + 0x4f4fc + *(int *)(iVar2 + 0xbc) * 0xc);
        fStack_44 = fStack_34 - _UNK_00110c90;
        local_3c = local_4c;
        local_38 = local_48;
        (**(code **)((int)&__DT_REL[0x1e0d].r_offset + iVar2))
                  (auStack_9c,&local_3c,&local_4c,0,0xffffffff);
        puVar6 = &uStack_b0;
        if (fStack_90 == (float)dc_1) {
          iVar3 = *(int *)((int)&__DT_REL[0x1dfb].r_offset + *(int *)puVar5);
          fVar1 = *(float *)(iVar3 + 0x1c) * *(float *)(iVar3 + 0x20);
        }
        else {
          fVar1 = *(float *)(*(int *)((int)&__DT_REL[0x1dfb].r_offset + *(int *)PTR_pmove_001772c0)
                            + 0x1c);
        }
        iVar3 = *(int *)((int)&__DT_REL[0x1dfb].r_offset + *(int *)PTR_pmove_001772c0);
        if (local_a4 < *(float *)(iVar3 + 4)) {
          local_a4 = *(float *)(iVar3 + 4);
        }
        fStack_a8 = local_a4 * fVar1 * *(float *)(*(int *)PTR_pmove_001772c0 + 0xc4) * fc_0_75 *
                    *(float *)(*(int *)PTR_pmove_001772c0 + 0x10) + fStack_a8;
      }
      puVar5 = PTR_pmove_001772c0;
      fVar1 = *(float *)((int)puVar6 + 0xc) - *(float *)((int)puVar6 + 8);
      if (fVar1 < fc_0) {
        fVar1 = fc_0;
      }
      fVar1 = fVar1 / *(float *)((int)puVar6 + 0xc);
      fVar4 = fVar1 * *(float *)(iVar2 + 0x5c);
      *(float *)((int)puVar6 + 0x90) = fVar4;
      *(float *)((int)puVar6 + 0x94) = fVar1 * *(float *)(iVar2 + 0x60);
      *(float *)((int)puVar6 + 0x98) = fVar1 * *(float *)(iVar2 + 100);
      *(float *)(*(int *)PTR_pmove_001772c0 + 0x5c) = fVar4;
      *(undefined4 *)(*(int *)puVar5 + 0x60) = *(undefined4 *)((int)puVar6 + 0x94);
      *(undefined4 *)(*(int *)puVar5 + 100) = *(undefined4 *)((int)puVar6 + 0x98);
    }
  }
  return;
}


