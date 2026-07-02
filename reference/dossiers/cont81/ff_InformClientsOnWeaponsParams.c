// ===== FUN @ 0009eb14 size=129 =====

/* WARNING: Unknown calling convention -- yet parameter storage is locked */
/* TS_util.cpp */

void InformClientsOnWeaponsParams__FP11CBaseEntity(CBaseEntity *a0)

{
  int iVar1;
  
  if (a0 == (CBaseEntity *)0x0) {
    TSHudMessageAll__FUcUcUciiPc(0x2d,0xc3,0x2d,8,100,"This server is using custom weapons.");
  }
  else {
    TSHudMessageOne__FUcUcUciiPcP9entvars_s
              (0x2d,0xc3,0x2d,8,100,"This server is using custom weapons.",a0->pev);
  }
  iVar1 = 0;
  do {
    SendWeaponParams__F10tsweapon_tP11CBaseEntity(iVar1,a0);
    SendWeaponParams__F10tsweapon_tP11CBaseEntity(iVar1 + 1,a0);
    iVar1 = iVar1 + 2;
  } while (iVar1 < 0x26);
  return;
}
