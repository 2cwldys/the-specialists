// ===== FUN @ 0009e9c0 size=118 =====

/* WARNING: Unknown calling convention -- yet parameter storage is locked */
/* TS_util.cpp */

int WeaponIdFromString__FPc(char *a0)

{
  int iVar1;
  int iVar2;
  
  iVar2 = 0;
  while( true ) {
    iVar1 = get_weapon_info__F10tsweapon_t(iVar2);
    iVar1 = strcasecmp(a0,(char *)(iVar1 + 0x44));
    if (iVar1 == 0) {
      return iVar2;
    }
    iVar1 = get_weapon_info__F10tsweapon_t(iVar2 + 1);
    iVar1 = strcasecmp(a0,(char *)(iVar1 + 0x44));
    if (iVar1 == 0) break;
    iVar2 = iVar2 + 2;
    if (0x25 < iVar2) {
      return -1;
    }
  }
  return iVar2 + 1;
}
