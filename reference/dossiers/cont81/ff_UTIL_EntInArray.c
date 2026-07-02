// ===== FUN @ 0009e944 size=122 =====

/* WARNING: Unknown calling convention -- yet parameter storage is locked */
/* TS_util.cpp */

int UTIL_EntInArray__FPP11CBaseEntityiP11CBaseEntity(CBaseEntity **a0,int a1,CBaseEntity *a2)

{
  uint uVar1;
  int iVar2;
  
  iVar2 = 0;
  if (a1 < 1) {
    return -1;
  }
  uVar1 = a1 & 3;
  if (0 < a1) {
    if (uVar1 == 0) goto LAB_0009e990;
    if (1 < uVar1) {
      if (2 < uVar1) {
        if (*a0 == a2) {
          return 0;
        }
        iVar2 = 1;
      }
      if (a0[iVar2] == a2) {
        return iVar2;
      }
      iVar2 = iVar2 + 1;
    }
  }
  if (a0[iVar2] == a2) {
    return iVar2;
  }
  iVar2 = iVar2 + 1;
  if (a1 <= iVar2) {
    return -1;
  }
LAB_0009e990:
  while( true ) {
    if (a0[iVar2] == a2) {
      return iVar2;
    }
    if (a0[iVar2 + 1] == a2) {
      return iVar2 + 1;
    }
    if (a0[iVar2 + 2] == a2) {
      return iVar2 + 2;
    }
    if (a0[iVar2 + 3] == a2) break;
    iVar2 = iVar2 + 4;
    if (a1 <= iVar2) {
      return -1;
    }
  }
  return iVar2 + 3;
}
