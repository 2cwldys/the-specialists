// ===== FUN @ 000f96a0 size=295 =====

void TSHandle(void)

{
  int iVar1;
  float fVar2;
  undefined *puVar3;
  
  if (((*(byte *)((int)&__DT_REL[0x9eb].r_info + *(int *)PTR_pmove_001772c0 + 2) & 2) != 0) &&
     (*(int *)(*(int *)PTR_pmove_001772c0 + 0xe0) == -1)) {
    PseudoClimb();
  }
  puVar3 = PTR_pmove_001772c0;
  iVar1 = *(int *)PTR_pmove_001772c0;
  if ((*(byte *)(iVar1 + 0x208) & 1) != 0) {
    fVar2 = (float)dc_0_5;
    *(float *)((int)&__DT_REL[0x9ea].r_offset + iVar1) =
         fVar2 * *(float *)((int)&__DT_REL[0x9ea].r_offset + iVar1);
    iVar1 = *(int *)puVar3;
    *(float *)((int)&__DT_REL[0x9ea].r_info + iVar1) =
         fVar2 * *(float *)((int)&__DT_REL[0x9ea].r_info + iVar1);
    iVar1 = *(int *)puVar3;
    *(float *)((int)&__DT_REL[0x9eb].r_offset + iVar1) =
         fVar2 * *(float *)((int)&__DT_REL[0x9eb].r_offset + iVar1);
  }
  puVar3 = PTR_pmove_001772c0;
  if ((*(byte *)(*(int *)PTR_pmove_001772c0 + 0x208) & 0x20) != 0) {
    *(undefined4 *)((int)&__DT_REL[0x9ea].r_offset + *(int *)PTR_pmove_001772c0) = 0;
    iVar1 = *(int *)puVar3;
    fVar2 = (float)dc_0_3;
    *(float *)((int)&__DT_REL[0x9ea].r_info + iVar1) =
         fVar2 * *(float *)((int)&__DT_REL[0x9ea].r_info + iVar1);
    iVar1 = *(int *)puVar3;
    *(float *)((int)&__DT_REL[0x9eb].r_offset + iVar1) =
         fVar2 * *(float *)((int)&__DT_REL[0x9eb].r_offset + iVar1);
  }
  if ((*(uint *)(*(int *)PTR_pmove_001772c0 + 0x208) & 0x1c0) != 0) {
    TSDive();
  }
  if ((*(uint *)(*(int *)PTR_pmove_001772c0 + 0x208) & 0x7000) != 0) {
    TSWallJump();
  }
  if (*(short *)(*(int *)PTR_pmove_001772c0 + 0x208) < 0) {
    TSWallJumpClimb();
  }
  if ((*(byte *)(*(int *)PTR_pmove_001772c0 + 0x208) & 4) != 0) {
    TSGoDuck();
  }
  if ((*(byte *)(*(int *)PTR_pmove_001772c0 + 0x208) & 8) != 0) {
    TSStandUp();
  }
  return;
}


