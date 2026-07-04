// ===== FUN @ 1004fe00 size=185 =====

void FUN_1004fe00(void)

{
  char cVar1;
  int iVar2;
  undefined4 uStack_70;
  undefined4 uStack_6c;
  uint uStack_64;
  
  iVar2 = (*(code *)DAT_100d5c7c[1])();
  if (iVar2 == 0) {
    cVar1 = FUN_1004c280();
    if ((cVar1 != '\0') && (DAT_100df390 == 0)) {
      return;
    }
  }
  else if (DAT_100df390 == DAT_100d607c) {
    return;
  }
  (*DAT_100d5bb0)(&uStack_70);
  DAT_100df390 = (uint)(DAT_100df390 == 0);
  if (DAT_100df390 != 0) {
    DAT_100df31c = uStack_6c;
    DAT_100df318 = uStack_70;
    DAT_100df320 = 0x41f00000;
  }
  (*DAT_100d5bbc)(s_cam_command_10093e24,0);
  iVar2 = (*(code *)*DAT_100d5c7c)();
  if (iVar2 != 0) {
    uStack_64 = DAT_100df390;
    FUN_1001b860();
  }
  return;
}


