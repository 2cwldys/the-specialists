// ===== FUN @ 000f8fc8 size=74 =====

void PseudoClimb(void)

{
  float fVar1;
  int iVar2;
  undefined *puVar3;
  undefined *puVar4;
  undefined4 *puVar5;
  longdouble lVar6;
  undefined1 auStack_24 [4];
  undefined4 uStack_20;
  undefined4 uStack_1c;
  undefined1 *puStack_18;
  
  puVar3 = PTR_pmove_001772c0;
  uStack_20 = 0xf8fd4;
  fVar1 = *(float *)(*(int *)PTR_pmove_001772c0 + 0x44);
  if ((fc_neg60 <= fVar1) && (fVar1 <= fc_60)) {
    climb_4 = fVar1 < fc_neg30;
    savehull_3 = *(int *)(*(int *)PTR_pmove_001772c0 + 0xbc);
    *(undefined4 *)(*(int *)PTR_pmove_001772c0 + 0xbc) = 2;
    puVar4 = PTR_pmove_001772c0;
    vec_start_0._0_4_ = *(float *)(*(int *)puVar3 + 0x38);
    vec_start_0._4_4_ = *(float *)(*(int *)puVar3 + 0x3c);
    vec_start_0._8_4_ = *(float *)(*(int *)puVar3 + 0x40) + *(float *)(*(int *)puVar3 + 0x88);
    if (climb_4 != '\0') {
      vec_start_0._8_4_ = (float)vec_start_0._8_4_ + fc_40;
    }
    puStack_18 = vec_start_0;
    vec_end_1._0_4_ =
         *(float *)(*(int *)PTR_pmove_001772c0 + 0x14) * fc_64 + (float)vec_start_0._0_4_;
    vec_end_1._4_4_ =
         *(float *)(*(int *)PTR_pmove_001772c0 + 0x18) * fc_64 + (float)vec_start_0._4_4_;
    vec_end_1._8_4_ = vec_start_0._8_4_;
    (**(code **)((int)&__DT_REL[0x1e0d].r_offset + *(int *)PTR_pmove_001772c0))
              (trace_2,vec_start_0,vec_end_1,0,0xffffffff);
    puVar5 = &uStack_20;
    if ((float)trace_2._16_4_ == 1.0) {
      vec_start_0._8_4_ =
           *(float *)(*(int *)puVar4 + 0x40) +
           *(float *)(*(int *)puVar4 + 0x4f4fc + savehull_3 * 0xc);
      vec_end_1._0_4_ = *(float *)(*(int *)puVar4 + 0x14) * fc_32 + (float)vec_start_0._0_4_;
      vec_end_1._4_4_ = *(float *)(*(int *)puVar4 + 0x18) * fc_32 + (float)vec_start_0._4_4_;
      vec_end_1._8_4_ = vec_start_0._8_4_;
      *(undefined4 *)(*(int *)puVar4 + 0xbc) = 2;
      (**(code **)((int)&__DT_REL[0x1e0d].r_offset + *(int *)puVar4))
                (trace_2,uStack_1c,vec_end_1,0,0xffffffff);
      puVar5 = (undefined4 *)auStack_24;
      if ((float)trace_2._16_4_ != 1.0) {
        lVar6 = (longdouble)Length(*(int *)puVar4 + 0x5c);
        puVar5 = (undefined4 *)auStack_24;
        if (lVar6 < (longdouble)dc_0_5 * (longdouble)*(float *)(*(int *)puVar4 + 500)) {
          *(undefined4 *)(*(int *)puVar4 + 0xe0) = 0xffffffff;
          iVar2 = *(int *)puVar4;
          *(float *)(iVar2 + 100) =
               (float)dc_100 * *(float *)(iVar2 + 0x10) * fc_16 + *(float *)(iVar2 + 100);
          PM_FixupGravityVelocity();
          puVar5 = (undefined4 *)auStack_24;
        }
      }
    }
    *(int *)(*(int *)PTR_pmove_001772c0 + 0xbc) = savehull_3;
    *(undefined4 *)((int)puVar5 + -0x10) = 1;
    *(undefined4 *)((int)puVar5 + -0x14) = 0xf91e8;
    PM_FixPlayerCrouchStuck();
    *(undefined4 *)((int)puVar5 + -0x14) = 0xf91ed;
    PM_CatagorizePosition();
  }
  return;
}
