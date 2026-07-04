// ===== FUN @ 000f9944 size=193 =====

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void PM_Jump(void)

{
  uint uVar1;
  float fVar2;
  undefined *puVar3;
  undefined *puVar4;
  undefined4 uVar5;
  char *pcVar6;
  int iVar7;
  
  puVar3 = PTR_pmove_001772c0;
  fVar2 = fc_0;
  iVar7 = *(int *)PTR_pmove_001772c0;
  if (*(int *)(iVar7 + 0xd0) == 0) {
    iVar7 = *(int *)PTR_pmove_001772c0;
    uVar1 = *(uint *)(iVar7 + 0x208);
    if ((uVar1 & 0x20) != 0) {
      return;
    }
    if ((uVar1 & 0x10) != 0) {
      return;
    }
    if ((uVar1 & 0x10000) != 0) {
      return;
    }
    if ((uVar1 & 0x20000) != 0) {
      return;
    }
    if ((uVar1 & 0xc00) != 0) {
      return;
    }
    if ((uVar1 & 4) != 0) {
      return;
    }
    if (fc_0 != *(float *)(iVar7 + 0xcc)) {
      *(float *)(iVar7 + 0xcc) =
           *(float *)(iVar7 + 0xcc) - (float)*(byte *)((int)&__DT_REL[0x9e8].r_offset + iVar7 + 2);
      if (fVar2 <= *(float *)(*(int *)puVar3 + 0xcc)) {
        return;
      }
      *(undefined4 *)(*(int *)puVar3 + 0xcc) = 0;
      return;
    }
    if (1 < *(int *)(*(int *)PTR_pmove_001772c0 + 0xe4)) {
      *(undefined4 *)(*(int *)PTR_pmove_001772c0 + 0xe0) = 0xffffffff;
      if (*(int *)(*(int *)puVar3 + 0xe8) == -3) {
        *(undefined4 *)(*(int *)puVar3 + 100) = 0x42c80000;
      }
      else if (*(int *)(*(int *)PTR_pmove_001772c0 + 0xe8) == -4) {
        *(undefined4 *)(*(int *)PTR_pmove_001772c0 + 100) = 0x42a00000;
      }
      else {
        *(undefined4 *)(*(int *)PTR_pmove_001772c0 + 100) = 0x42480000;
      }
      puVar3 = PTR_pmove_001772c0;
      if (fc_0 < *(float *)(*(int *)PTR_pmove_001772c0 + 0xac)) {
        return;
      }
      *(undefined4 *)(*(int *)PTR_pmove_001772c0 + 0xac) = 0x447a0000;
      uVar5 = (**(code **)((int)&__DT_REL[0x1e0e].r_offset + *(int *)puVar3))(0,3);
      switch(uVar5) {
      case 0:
        iVar7 = *(int *)PTR_pmove_001772c0;
        pcVar6 = "player/pl_wade1.wav";
        break;
      case 1:
        iVar7 = *(int *)PTR_pmove_001772c0;
        pcVar6 = "player/pl_wade2.wav";
        break;
      case 2:
        iVar7 = *(int *)PTR_pmove_001772c0;
        pcVar6 = "player/pl_wade3.wav";
        break;
      case 3:
        iVar7 = *(int *)PTR_pmove_001772c0;
        pcVar6 = "player/pl_wade4.wav";
        break;
      default:
        return;
      }
      (**(code **)((int)&__DT_REL[0x1e13].r_info + iVar7))(4,pcVar6,0x3f800000,0x3f4ccccd,0,100);
      return;
    }
    iVar7 = *(int *)PTR_pmove_001772c0;
    if (*(int *)(iVar7 + 0xe0) != -1) {
      if ((*(byte *)(*(int *)PTR_pmove_001772c0 + 200) & 2) != 0) {
        return;
      }
      *(undefined4 *)(*(int *)PTR_pmove_001772c0 + 0xe0) = 0xffffffff;
      PM_PreventMegaBunnyJumping();
      uVar5 = PM_MapTextureTypeStepType((int)*(char *)(*(int *)puVar3 + 0x1f0),0x3f800000);
      PM_PlayStepSound(uVar5);
      uVar5 = (**(code **)((int)&__DT_REL[0x1e07].r_info + *(int *)puVar3))
                        (*(int *)puVar3 + 0x4f3f0,&DAT_0010791d);
      iVar7 = __strtol_internal(uVar5,0,10,0);
      puVar4 = PTR_pmove_001772c0;
      canhighjump_24 = (uint)(iVar7 == 1);
      if ((((*(int *)(*(int *)puVar3 + 0x90) == 0) &&
           ((*(byte *)(*(int *)puVar3 + 0xb9) & 0x40) == 0)) || (canhighjump_24 == 0)) ||
         ((iVar7 = *(int *)PTR_pmove_001772c0,
          (*(byte *)((int)&__DT_REL[0x9eb].r_info + iVar7 + 2) & 4) == 0 ||
          (*(float *)(iVar7 + 0x8c) <= fc_0)))) {
        canhighjump_24 = 0;
      }
      else {
        *(float *)(iVar7 + 100) = SQRT((float)_UNK_00110bc8);
        *(uint *)(*(int *)puVar4 + 0x208) = *(uint *)(*(int *)puVar4 + 0x208) | 0x200000;
      }
      if (canhighjump_24 == 0) {
        *(float *)(*(int *)PTR_pmove_001772c0 + 100) = SQRT((float)_UNK_00110bb8);
      }
      PM_FixupGravityVelocity();
      iVar7 = *(int *)PTR_pmove_001772c0;
    }
  }
  *(uint *)(iVar7 + 200) = *(uint *)(iVar7 + 200) | 2;
  return;
}


