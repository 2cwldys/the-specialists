// ===== FUN @ 10069b90 size=676 =====

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void __cdecl V_CalcRefdef(float *param_1)

{
  longlong lVar1;
  undefined1 local_18 [16];
  undefined1 uStack_8;
  undefined1 uStack_7;
  undefined1 uStack_6;
  undefined1 uStack_5;
  undefined4 uStack_4;
  
                    /* 0x69b90  504  V_CalcRefdef */
  DAT_10191478 = param_1[0xf];
  if (param_1[0x11] == 0.0) {
    if ((param_1[0x13] == 0.0) && (DAT_10190b18 == 0)) {
      if (param_1[0x12] == 0.0) {
        FUN_10067890(param_1);
      }
    }
    else {
      FUN_10068f70(param_1);
    }
  }
  else {
    FUN_100676e0(param_1);
  }
  (*DAT_100d5c54)(local_18);
  if ((*(float *)(DAT_101914e0 + 0xc) == _DAT_100799f0) || ((float)_DAT_10079e80 <= DAT_100963e8)) {
    DAT_101914c8 = 0;
    DAT_101914c9 = 0;
    DAT_101914ca = 0;
    DAT_101914cb = 0;
  }
  else {
    DAT_101914c8 = 0;
    DAT_101914c9 = 100;
    DAT_101914ca = 0x7d;
    lVar1 = __ftol();
    DAT_101914cb = (undefined1)lVar1;
  }
  _DAT_101914cc = 5;
  if ((param_1[0x10] <= _DAT_10190b5c) ||
     (_DAT_10190b5c + (float)(DAT_101914a8 & 0xffff) * _DAT_10081e8c <= param_1[0x10])) {
    uStack_8 = 0;
    uStack_7 = DAT_101914c9;
    uStack_6 = DAT_101914ca;
    uStack_5 = DAT_101914cb;
  }
  else {
    lVar1 = __ftol();
    uStack_8 = (undefined1)lVar1;
    lVar1 = __ftol();
    uStack_7 = (undefined1)lVar1;
    lVar1 = __ftol();
    uStack_6 = (undefined1)lVar1;
    lVar1 = __ftol();
    uStack_5 = (undefined1)lVar1;
  }
  uStack_4 = 5;
  (*DAT_100d5c58)(local_18);
  FUN_10056c20();
  return;
}


