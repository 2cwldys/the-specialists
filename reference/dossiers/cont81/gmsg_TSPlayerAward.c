// ===== FUN @ 1004b790 size=70 =====

void __thiscall FUN_1004b790(void *this,undefined4 param_1,undefined4 param_2,undefined4 param_3)

{
  uint uVar1;
  uint uVar2;
  
  FUN_100299b0(param_3,param_2);
  uVar1 = FUN_10029a10();
  uVar2 = FUN_10029a10();
  if ((0 < (int)uVar1) && ((int)uVar1 < 0x41)) {
    *(char *)(&DAT_101901f4 + uVar1 * 8) = (char)uVar2;
  }
  *(uint *)((int)this + 0x10) = *(uint *)((int)this + 0x10) | 1;
  return;
}
