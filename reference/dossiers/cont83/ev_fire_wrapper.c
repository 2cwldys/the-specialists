// ===== FUN @ 1003de40 size=79 =====

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void __cdecl FUN_1003de40(int param_1)

{
  char cVar1;
  
  _DAT_10189ec8 = FUN_100071c0(*(int *)(param_1 + 0x38));
  cVar1 = *(char *)(_DAT_10189ec8 + 0xd2);
  if ((cVar1 != '\x04') && (cVar1 != '\x05')) {
    if (cVar1 == '\x01') {
      FUN_10040c20(param_1,(float)_DAT_10189ec8);
      return;
    }
    FUN_1003dfa0(param_1,(float)_DAT_10189ec8);
    return;
  }
  FUN_1003f090(param_1,(float)_DAT_10189ec8,'\0');
  return;
}


