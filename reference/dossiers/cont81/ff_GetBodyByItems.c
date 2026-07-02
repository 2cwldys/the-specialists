// ===== FUN @ 0007f0a4 size=234 =====

/* WARNING: Unknown calling convention -- yet parameter storage is locked */
/* TS_gun.cpp */

int GetBodyByItems__Fii(int a0,int a1)

{
  short sVar1;
  int iVar2;
  
  iVar2 = 0;
  if ((a1 & 1U) != 0) {
    iVar2 = (int)*(short *)(&DAT_0017dc68 + a0 * 0x3dc);
  }
  if ((a1 & 6U) == 6) {
    if (*(short *)(&DAT_0017dc70 + a0 * 0x3dc) == -1) goto LAB_0007f114;
    sVar1 = *(short *)(&DAT_0017dc70 + a0 * 0x3dc);
  }
  else {
LAB_0007f114:
    if ((a1 & 2U) != 0) {
      iVar2 = iVar2 + *(short *)(&DAT_0017dc6a + a0 * 0x3dc);
    }
    if ((a1 & 4U) == 0) goto LAB_0007f15c;
    sVar1 = *(short *)(&DAT_0017dc6c + a0 * 0x3dc);
  }
  iVar2 = iVar2 + sVar1;
LAB_0007f15c:
  if ((a1 & 8U) == 0) {
    if (a0 == 0x24) {
      iVar2 = iVar2 + 1;
    }
  }
  else {
    iVar2 = iVar2 + *(short *)(&DAT_0017dc6e + a0 * 0x3dc);
  }
  return iVar2;
}
