// ===== FUN @ 0009ea3c size=212 =====

/* TS_util.cpp */

void AdjustSpecificWeapon__F10tsweapon_tffffi
               (uint param_1,undefined4 param_2,undefined4 param_3,undefined4 param_4,
               undefined4 param_5,undefined4 param_6)

{
  char *pcVar1;
  int iVar2;
  int a3;
  int a4;
  int a5;
  
  if (param_1 < 0x26) {
    pcVar1 = (char *)get_weapon_info__F10tsweapon_t(param_1);
    if (*pcVar1 != '\0') {
      iVar2 = get_weapon_info__F10tsweapon_t(param_1);
      *(undefined4 *)(iVar2 + 0x28) = param_2;
      iVar2 = get_weapon_info__F10tsweapon_t(param_1);
      *(undefined4 *)(iVar2 + 0x30) = param_3;
      iVar2 = get_weapon_info__F10tsweapon_t(param_1);
      *(undefined4 *)(iVar2 + 0x34) = param_4;
      iVar2 = get_weapon_info__F10tsweapon_t(param_1);
      *(undefined4 *)(iVar2 + 0x1c) = param_5;
      iVar2 = get_weapon_info__F10tsweapon_t(param_1);
      *(undefined4 *)(iVar2 + 0xd8) = param_6;
      SendWeaponParams__F10tsweapon_tP11CBaseEntity(param_1,0);
      a5 = 0;
      a4 = 0;
      a3 = 0;
      iVar2 = 0;
      get_weapon_info__F10tsweapon_t(param_1);
      pcVar1 = (char *)UTIL_VarArgs__FPce("%s behaviour changed\n");
      UTIL_ClientPrintAll__FiPCcN41(3,pcVar1,iVar2,a3,a4,a5);
      PTR_TSGameRules_00177300[0x5884] = 1;
    }
  }
  return;
}
