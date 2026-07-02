// ===== FUN @ 0009fb4c size=160 =====

/* WARNING: Unknown calling convention -- yet parameter storage is locked */
/* TS_util.cpp */

void RemoveEntityFromSlowedArray__FP11CBaseEntity(CBaseEntity *a0)

{
  int iVar1;
  int iVar2;
  int iVar3;
  int a0_00;
  
  for (a0_00 = 1; a0_00 <= (*PTR_gpGlobals_00177410)->maxClients; a0_00 = a0_00 + 1) {
    iVar1 = UTIL_PlayerByIndex__Fi(a0_00);
    if (iVar1 != 0) {
      iVar2 = (**(code **)(*(int *)(iVar1 + 0x60) + 200))(iVar1);
      if ((iVar2 != 0) && (iVar2 = 0, '\0' < *(char *)(iVar1 + 0x288))) {
        do {
          iVar3 = UTIL_EntInArray__FPP11CBaseEntityiP11CBaseEntity
                            ((CBaseEntity **)(iVar1 + 0x208),(int)*(char *)(iVar1 + 0x288),a0);
          if (-1 < iVar3) {
            *(undefined4 *)(iVar1 + 0x208 + iVar3 * 4) = 0;
          }
          iVar2 = iVar2 + 1;
        } while (iVar2 < *(char *)(iVar1 + 0x288));
      }
    }
  }
  return;
}
