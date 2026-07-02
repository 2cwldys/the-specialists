// ===== FUN @ 1004b7e0 size=81 =====

void __thiscall FUN_1004b7e0(void *this,undefined4 param_1,undefined4 param_2,undefined4 param_3)

{
  uint uVar1;
  uint uVar2;
  uint uVar3;
  
  FUN_100299b0(param_3,param_2);
  uVar1 = FUN_10029a10();
  uVar2 = FUN_10029a10();
  uVar3 = FUN_10029a10();
  if (((0 < (int)uVar1) && ((int)uVar1 < 0x41)) && (uVar2 == 0)) {
    *(char *)((int)&DAT_1018d128 + uVar1 * 2 + 1) = (char)uVar3;
  }
  *(uint *)((int)this + 0x10) = *(uint *)((int)this + 0x10) | 1;
  return;
}
