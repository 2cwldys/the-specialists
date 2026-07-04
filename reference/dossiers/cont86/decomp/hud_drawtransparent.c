// ===== FUN @ 10034e50 size=5 =====

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void HUD_DrawTransparentTriangles(void)

{
  int iVar1;
  
                    /* 0x34e50  475  HUD_DrawTransparentTriangles */
  if (DAT_1018d16a == '\0') {
    FUN_10057d50();
    FUN_10056bc0();
    FUN_10034eb0();
  }
  else if (*(float *)(DAT_100df464 + 0xc) != _DAT_1018cac4) {
    FUN_10057d50();
  }
  FUN_10056f60();
  FUN_10056e70();
  iVar1 = (*DAT_100ea860)();
  if (iVar1 != 0) {
    FUN_10055760();
  }
  FUN_1003de00();
  FUN_1003bad0();
  FUN_100520d0(&DAT_1018bde8);
  iVar1 = (*DAT_100ea860)();
  if (iVar1 != 0) {
    FUN_10054510();
  }
  FUN_10056f80();
  if (*(int *)(DAT_100df464 + 0xc) == 0x3f800000) {
    FUN_10057a70();
  }
  FUN_10034e60();
  glDisable(0xb60);
  return;
}


