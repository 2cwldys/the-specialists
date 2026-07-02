// ===== FUN @ 000870c8 size=107 =====

/* TS_gun.cpp */

uint IsRestrictedWeapon__F10tsweapon_ti(int param_1,uint param_2)

{
  char cVar1;
  uint uVar2;
  uint uVar3;
  bool bVar4;
  
  uVar3 = param_1 - 1;
  if (uVar3 < 0x25) {
    if (param_2 == 0) {
      return CONCAT31((int3)(uVar3 >> 8),1);
    }
    cVar1 = GetWeaponType__F10tsweapon_t(param_1);
    uVar2 = (int)cVar1 - 1;
    uVar3 = uVar2;
    if (8 < uVar2) goto switchD_00087111_caseD_1;
    uVar3 = (int)&__DT_PLTGOT - (&switchD_00087111::switchdataD_00087114)[uVar2];
    switch(uVar2) {
    case 0:
      bVar4 = (param_2 & 0x10) == 0;
      break;
    default:
      goto switchD_00087111_caseD_1;
    case 2:
      uVar3 = param_2 ^ 1;
      if ((uVar3 & 1) == 0) goto switchD_00087111_caseD_1;
      goto LAB_00087144;
    case 5:
    case 6:
      bVar4 = (param_2 & 8) == 0;
      break;
    case 7:
      bVar4 = (param_2 & 2) == 0;
      break;
    case 8:
      bVar4 = (param_2 & 4) == 0;
    }
    if (bVar4) {
LAB_00087144:
      return CONCAT31((int3)(uVar3 >> 8),1);
    }
  }
switchD_00087111_caseD_1:
  return uVar3 & 0xffffff00;
}
