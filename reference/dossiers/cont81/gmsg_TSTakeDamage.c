// ===== FUN @ 1004bcd0 size=119 =====

void __thiscall FUN_1004bcd0(void *this,undefined4 param_1,undefined4 param_2,undefined4 param_3)

{
  uint uVar1;
  float10 fVar2;
  float local_18;
  float local_14;
  float local_10;
  float local_c;
  float local_8;
  float local_4;
  
  FUN_100299b0(param_3,param_2);
  fVar2 = FUN_10029b60();
  local_c = (float)fVar2;
  fVar2 = FUN_10029b60();
  local_8 = (float)fVar2;
  fVar2 = FUN_10029b60();
  local_4 = (float)fVar2;
  fVar2 = FUN_10029b60();
  local_18 = (float)fVar2;
  fVar2 = FUN_10029b60();
  local_14 = (float)fVar2;
  fVar2 = FUN_10029b60();
  local_10 = (float)fVar2;
  uVar1 = FUN_10029a10();
  FUN_10048840(&local_c,&local_18,uVar1);
  *(uint *)((int)this + 0x10) = *(uint *)((int)this + 0x10) | 1;
  return;
}
