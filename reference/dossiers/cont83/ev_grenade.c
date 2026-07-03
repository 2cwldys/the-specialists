// ===== FUN @ 10042630 size=91 =====

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void __cdecl FUN_10042630(int param_1)

{
  undefined4 uVar1;
  
  DAT_10189abc = *(int *)(param_1 + 0x38);
  DAT_1018a0c4 = FUN_100071c0(0x18);
  _DAT_10189ee8 = *(int *)(param_1 + 4);
  uVar1 = FUN_1001c5c0(_DAT_10189ee8);
  if ((char)uVar1 != '\0') {
    _DAT_10189ffc = FUN_1003df60((int)DAT_1018a0c4);
    (**(code **)(DAT_100d5c78 + 0x40))((int)*(short *)(_DAT_10189ffc + DAT_10189abc * 2),1);
  }
  return;
}


