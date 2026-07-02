// ===== FUN @ 1004f780 size=121 =====

void __thiscall FUN_1004f780(void *this,undefined4 param_1,undefined4 param_2,undefined4 param_3)

{
  byte bVar1;
  uint uVar2;
  undefined4 *puVar3;
  
  FUN_100299b0(param_3,param_2);
  uVar2 = FUN_10029a10();
  *(uint *)((int)this + 0x78c) = uVar2;
  if (*(int *)((int)this + 0x7b8) != 0) {
    puVar3 = FUN_100071c0(*(int *)((int)this + 0x7b8));
    bVar1 = *(byte *)((int)puVar3 + 0x85);
    puVar3 = FUN_100071c0(*(int *)((int)this + 0x7b8));
    *(undefined4 *)((int)this + ((uint)bVar1 * 0xb + (uint)*(byte *)(puVar3 + 0x21)) * 8 + 0x168) =
         *(undefined4 *)((int)this + 0x78c);
  }
  *(uint *)((int)this + 0x10) = *(uint *)((int)this + 0x10) | 1;
  return;
}
