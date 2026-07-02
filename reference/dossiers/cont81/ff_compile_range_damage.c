// ===== FUN @ 0007f1f4 size=153 =====

/* WARNING: Unknown calling convention -- yet parameter storage is locked */
/* TS_gun.cpp */

void compile_range_damage__Fv(void)

{
  int iVar1;
  int iVar2;
  int iVar3;
  
  iVar2 = 0;
  iVar3 = 0;
  do {
    iVar1 = iVar2;
    if (iVar2 < 1) {
      iVar1 = 1;
    }
    *(float *)((int)&range_damage_multipler + iVar3) =
         fc_1 / SQRT(fc_0_10000000149011612 * (float)iVar1);
    if ((float)dc_1 < *(float *)((int)&range_damage_multipler + iVar3)) {
      *(undefined4 *)((int)&range_damage_multipler + iVar3) = 0x3f800000;
    }
    if (*(float *)((int)&range_damage_multipler + iVar3) < (float)dc_0) {
      *(undefined4 *)((int)&range_damage_multipler + iVar3) = 0;
    }
    iVar3 = iVar3 + 4;
    iVar2 = iVar2 + 1;
  } while (iVar2 < 10000);
  return;
}
