// ===== FUN @ 1003de90 size=198 =====

void __cdecl FUN_1003de90(undefined4 param_1)

{
  int iVar1;
  int iVar2;
  float *pfVar3;
  undefined4 *puVar4;
  float10 fVar5;
  int in_stack_00000014;
  char in_stack_00000018;
  undefined4 in_stack_0000001c;
  int in_stack_00000020;
  undefined4 uVar6;
  
  iVar1 = DAT_100d5c78;
  if (in_stack_00000018 == '\0') {
    iVar2 = (*DAT_100d5c38)(0xfffffffb,5);
    iVar2 = iVar2 + in_stack_00000020;
    uVar6 = 0;
    puVar4 = FUN_100071f0((uint)*(byte *)(in_stack_00000014 + 8));
    fVar5 = (float10)(*DAT_100d5c34)(0x3f6b851f,0x3f800000,*puVar4,uVar6,iVar2);
    in_stack_00000014 = in_stack_00000014 + 0x2c6;
  }
  else {
    iVar2 = (*DAT_100d5c38)(0xfffffffb,5);
    iVar2 = iVar2 + in_stack_00000020;
    uVar6 = 0;
    pfVar3 = (float *)FUN_100071f0((uint)*(byte *)(in_stack_00000014 + 8));
    fVar5 = (float10)(*DAT_100d5c34)(0x3f570a3d,0x3f6b851f,*pfVar3 + *pfVar3,uVar6,iVar2);
    in_stack_00000014 = in_stack_00000014 + 0x306;
  }
  (**(code **)(iVar1 + 4))
            (param_1,&stack0x00000008,in_stack_0000001c,in_stack_00000014,(float)fVar5);
  return;
}


