// ===== FUN @ 1004f800 size=392 =====

void __thiscall FUN_1004f800(void *this,undefined4 param_1,undefined4 param_2,undefined4 param_3)

{
  byte bVar1;
  bool bVar2;
  uint uVar3;
  int iVar4;
  undefined4 *puVar5;
  undefined3 extraout_var;
  int iVar6;
  undefined3 extraout_var_00;
  
  FUN_100299b0(param_3,param_2);
  iVar6 = *(int *)((int)this + 0x7b8);
  uVar3 = FUN_10029a10();
  *(uint *)((int)this + 0x7b8) = uVar3;
  if (uVar3 == 0xffffffff) {
    *(int *)((int)this + 0x7b8) = iVar6;
  }
  uVar3 = FUN_10029a10();
  *(uint *)((int)this + 0x78c) = uVar3;
  iVar4 = FUN_10029a50();
  puVar5 = FUN_100071c0(*(int *)((int)this + 0x7b8));
  *(int *)((int)this + (uint)*(byte *)(puVar5 + 2) * 4 + 0x748) = iVar4;
  uVar3 = FUN_10029a10();
  *(char *)((int)this + 0x78a) = (char)uVar3;
  uVar3 = FUN_10029a10();
  *(uint *)((int)this + 0x798) = uVar3;
  if (*(int *)((int)this + 0x7b8) < 1) {
    if (*(int *)(DAT_100df3a4 + 0xc) != 0x3f800000) goto LAB_1004f975;
    bVar2 = CL_IsThirdPerson();
    iVar6 = CONCAT31(extraout_var_00,bVar2);
  }
  else {
    puVar5 = FUN_100071c0(*(int *)((int)this + 0x7b8));
    if ((*(byte *)((int)puVar5 + 0x85) < 0x11) &&
       (puVar5 = FUN_100071c0(*(int *)((int)this + 0x7b8)), *(byte *)(puVar5 + 0x21) < 0xb)) {
      puVar5 = FUN_100071c0(*(int *)((int)this + 0x7b8));
      bVar1 = *(byte *)((int)puVar5 + 0x85);
      puVar5 = FUN_100071c0(*(int *)((int)this + 0x7b8));
      *(undefined4 *)((int)this + ((uint)bVar1 * 0xb + (uint)*(byte *)(puVar5 + 0x21)) * 8 + 0x164)
           = *(undefined4 *)((int)this + 0x7b8);
      puVar5 = FUN_100071c0(*(int *)((int)this + 0x7b8));
      bVar1 = *(byte *)((int)puVar5 + 0x85);
      puVar5 = FUN_100071c0(*(int *)((int)this + 0x7b8));
      *(undefined4 *)((int)this + ((uint)bVar1 * 0xb + (uint)*(byte *)(puVar5 + 0x21)) * 8 + 0x168)
           = *(undefined4 *)((int)this + 0x78c);
    }
    if ((*(int *)(DAT_100df3a4 + 0xc) != 0x3f800000) ||
       (bVar2 = CL_IsThirdPerson(), CONCAT31(extraout_var,bVar2) == 0)) goto LAB_1004f975;
  }
  if (iVar6 == 0) {
    FUN_1004fe00();
  }
LAB_1004f975:
  *(uint *)((int)this + 0x10) = *(uint *)((int)this + 0x10) | 1;
  return;
}


