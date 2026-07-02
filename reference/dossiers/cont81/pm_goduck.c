// ===== FUN @ 000f93a8 size=150 =====

void TSGoDuck(void)

{
  int iVar1;
  undefined *puVar2;
  int iVar3;
  
  puVar2 = PTR_pmove_001772c0;
  if (*(int *)(*(int *)PTR_pmove_001772c0 + 0xbc) != 1) {
    *(undefined4 *)(*(int *)PTR_pmove_001772c0 + 0xbc) = 1;
    *(uint *)(*(int *)puVar2 + 0xb8) = *(uint *)(*(int *)puVar2 + 0xb8) | 0x4000;
    if (*(int *)(*(int *)puVar2 + 0xe0) != -1) {
      i_8 = 0;
      do {
        iVar1 = *(int *)puVar2;
        iVar3 = i_8 * 4;
        *(float *)(iVar3 + 0x30 + iVar1 + 8) =
             *(float *)(iVar3 + 0x30 + iVar1 + 8) -
             (*(float *)(iVar3 + 0x4f500 + iVar1) - *(float *)(iVar3 + 0x4f4f4 + iVar1));
        i_8 = i_8 + 1;
      } while (i_8 < 3);
      PM_FixPlayerCrouchStuck(1);
      PM_CatagorizePosition();
    }
  }
  return;
}


