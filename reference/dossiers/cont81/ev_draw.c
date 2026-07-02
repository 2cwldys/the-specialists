// ===== FUN @ 100425b0 size=123 =====

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void __cdecl FUN_100425b0(int param_1)

{
  undefined4 uVar1;
  
  DAT_10189ca0 = *(int *)(param_1 + 0x38);
  DAT_10189e3c = FUN_100071c0(*(int *)(param_1 + 0x38));
  DAT_10189ff4 = *(uint *)(param_1 + 0x3c);
  _DAT_10189eb4 = *(int *)(param_1 + 4);
  uVar1 = FUN_1001c5c0(_DAT_10189eb4);
  if ((char)uVar1 != '\0') {
    DAT_10189c04 = FUN_1003df60((int)DAT_10189e3c);
    _DAT_10189f5c = FUN_10007110(DAT_10189ca0,DAT_10189ff4);
    (**(code **)(DAT_100d5c78 + 0x40))((int)*(short *)(DAT_10189c04 + 4),_DAT_10189f5c);
  }
  return;
}


