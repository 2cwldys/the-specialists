// ===== FUN @ 1004b840 size=142 =====

void __thiscall FUN_1004b840(void *this,undefined4 param_1,undefined4 param_2,undefined4 param_3)

{
  uint uVar1;
  int iVar2;
  
  FUN_100299b0(param_3,param_2);
  uVar1 = FUN_10029a10();
  iVar2 = FUN_10029a50();
  switch(uVar1) {
  case 0:
    DAT_1018d16c = iVar2;
    *(uint *)((int)this + 0x10) = *(uint *)((int)this + 0x10) | 1;
    return;
  case 1:
    DAT_1018d170 = iVar2;
    *(uint *)((int)this + 0x10) = *(uint *)((int)this + 0x10) | 1;
    return;
  case 2:
    FUN_100072c0(iVar2);
    *(uint *)((int)this + 0x10) = *(uint *)((int)this + 0x10) | 1;
    return;
  case 3:
    DAT_1018d174 = iVar2;
  }
  *(uint *)((int)this + 0x10) = *(uint *)((int)this + 0x10) | 1;
  return;
}
