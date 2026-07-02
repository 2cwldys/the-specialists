// ===== FUN @ 0008705c size=99 =====

/* TS_gun.cpp */

undefined1 GetWeaponType__F10tsweapon_t(int param_1)

{
  undefined1 uVar1;
  int iVar2;
  
  if (param_1 == 0x22) {
    uVar1 = 3;
  }
  else if (param_1 == 0x21) {
    uVar1 = 1;
  }
  else if ((param_1 == 0x10) || (param_1 == 0x25)) {
    uVar1 = 9;
  }
  else if ((param_1 == 10) || (((param_1 == 0x1e || (param_1 == 8)) || (param_1 == 0x15)))) {
    uVar1 = 8;
  }
  else {
    iVar2 = get_weapon_info__F10tsweapon_t(param_1);
    uVar1 = *(undefined1 *)(iVar2 + 0x348);
  }
  return uVar1;
}
