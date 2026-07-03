// ===== FUN @ 1004fa70 size=68 =====

void __thiscall FUN_1004fa70(void *this,undefined4 param_1,undefined4 param_2,undefined4 param_3)

{
  uint uVar1;
  uint uVar2;
  
  FUN_100299b0(param_3,param_2);
  uVar1 = FUN_10029a10();
  uVar2 = FUN_10029a10();
  if ((-1 < (int)uVar1) && ((int)uVar1 < 0x21)) {
    *(char *)((int)&DAT_1018d128 + uVar1 * 2) = (char)uVar2;
  }
  *(uint *)((int)this + 0x10) = *(uint *)((int)this + 0x10) | 1;
  return;
}


