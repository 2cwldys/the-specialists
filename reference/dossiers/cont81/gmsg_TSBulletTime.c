// ===== FUN @ 1004b8e0 size=54 =====

void __thiscall FUN_1004b8e0(void *this,undefined4 param_1,undefined4 param_2,undefined4 param_3)

{
  uint uVar1;
  
  FUN_100299b0(param_3,param_2);
  uVar1 = FUN_10029a10();
  DAT_1018d178 = uVar1 != 0;
  *(uint *)((int)this + 0x10) = *(uint *)((int)this + 0x10) | 1;
  return;
}
