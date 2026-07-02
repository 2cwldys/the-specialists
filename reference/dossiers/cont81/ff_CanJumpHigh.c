// ===== FUN @ 000f97cc size=339 =====

undefined4 CanJumpHigh(void)

{
  undefined *puVar1;
  
  puVar1 = PTR_pmove_001772c0;
  savehull_20 = *(undefined4 *)(*(int *)PTR_pmove_001772c0 + 0xbc);
  dir_18 = *(float *)(*(int *)PTR_pmove_001772c0 + 0x5c);
  DAT_001a0594 = *(float *)(*(int *)PTR_pmove_001772c0 + 0x60);
  DAT_001a0598 = *(undefined4 *)(*(int *)PTR_pmove_001772c0 + 100);
  VectorNormalize(&dir_18);
  vec_start_16 = *(float *)(*(int *)puVar1 + 0x38);
  DAT_001a057c = *(float *)(*(int *)puVar1 + 0x3c);
  DAT_001a0580 = *(undefined4 *)(*(int *)puVar1 + 0x40);
  vec_end_17 = dir_18 * fc_128 + vec_start_16;
  DAT_001a0588 = DAT_001a0594 * fc_128 + DAT_001a057c;
  DAT_001a058c = DAT_001a0580;
  *(undefined4 *)(*(int *)puVar1 + 0xbc) = 1;
  (**(code **)((int)&__DT_REL[0x1e0d].r_offset + *(int *)puVar1))
            (trace_19,&vec_start_16,&vec_end_17,0,0xffffffff);
  if ((float)trace_19._16_4_ != (float)dc_1) {
    vec_end_17 = dir_18 * fc_164 + *(float *)(*(int *)puVar1 + 0x38);
    DAT_001a0588 = DAT_001a0594 * fc_164 + *(float *)(*(int *)puVar1 + 0x3c);
    DAT_001a058c = *(undefined4 *)(*(int *)puVar1 + 0x40);
    content_21 = (**(code **)((int)&__DT_REL[0x1e0b].r_info + *(int *)puVar1))(&vec_end_17,0);
    if ((content_21 != -2) && (content_21 != -6)) {
      return 1;
    }
  }
  *(undefined4 *)(*(int *)PTR_pmove_001772c0 + 0xbc) = savehull_20;
  return 0;
}
