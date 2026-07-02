// ===== FUN @ 00085e54 size=104 =====

/* WARNING: Unknown calling convention -- yet parameter storage is locked */
/* TS_gun.cpp */

void CTSGun::CCombatIdle(CTSGun *this)

{
  CTSGun_vtbl *pCVar1;
  int iVar2;
  longdouble lVar3;
  int a1;
  int a2;
  
  lVar3 = (longdouble)UTIL_WeaponTimeBase__Fv();
  if ((longdouble)(float)this->field_0xa8 <= lVar3) {
    lVar3 = (longdouble)UTIL_WeaponTimeBase__Fv();
    this->field_0xa8 = (int)(float)((longdouble)dc_3 + lVar3);
    pCVar1 = this->vtbl;
    a2 = 0;
    a1 = 0;
    iVar2 = get_weapon_info__F10tsweapon_t(0);
    (*pCVar1->SendWeaponAnim)((CBasePlayerWeapon *)this,(int)*(short *)(iVar2 + 0x252),a1,a2);
  }
  return;
}


