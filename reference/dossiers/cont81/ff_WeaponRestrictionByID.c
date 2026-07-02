// ===== FUN @ 00076ad8 size=79 =====

/* WARNING: Unknown calling convention -- yet parameter storage is locked */
/* TS_gamerules.cpp */

undefined4 WeaponRestrictionByID__Fi(int a0)

{
  undefined4 uVar1;
  
  switch(a0) {
  default:
    uVar1 = InitWeaponRestriction__Fv();
    break;
  case 1:
    uVar1 = 0;
    break;
  case 2:
    uVar1 = 1;
    break;
  case 3:
    uVar1 = 3;
    break;
  case 4:
    uVar1 = 0x13;
  }
  return uVar1;
}
