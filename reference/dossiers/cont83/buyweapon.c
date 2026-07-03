// ===== FUN @ 000945bc size=305 =====

/* TS_player.cpp */

void CBasePlayer::BuyWeapon(int this,int param_2,uint param_3)

{
  undefined *puVar1;
  char cVar2;
  int iVar3;
  int iVar4;
  undefined2 uVar5;
  undefined4 uVar6;
  undefined2 uVar7;
  
  puVar1 = PTR_TSGameRules_00177300;
  iVar4 = 0;
  if ((*(int *)(this + 0xa10) != 0) && (*(int *)(*(int *)(this + 0xa10) + 0x88) == 0x10)) {
    uVar6 = *(undefined4 *)(PTR_TSGameRules_00177300 + 0x5870);
    iVar3 = param_2;
    cVar2 = IsRestrictedWeapon__F10tsweapon_ti(param_2,uVar6);
    uVar7 = (undefined2)((uint)uVar6 >> 0x10);
    uVar5 = (undefined2)((uint)iVar3 >> 0x10);
    if ((cVar2 == '\0') && ((1 < (byte)*puVar1 || (param_2 != 0x18)))) {
      if (param_2 == 0x18) {
        if (1 < *(byte *)(this + 0x714)) {
          return;
        }
        *(char *)(this + 0x714) = *(char *)(this + 0x714) + '\x01';
      }
      iVar3 = CashBased__Fv();
      if (iVar3 != 0) {
        iVar4 = param_2;
        iVar3 = get_weapon_info__F10tsweapon_t(param_2);
        uVar5 = (undefined2)((uint)iVar4 >> 0x10);
        iVar4 = *(int *)(this + 0x540) - *(int *)(iVar3 + 0xd8);
      }
      iVar3 = CashBased__Fv();
      if (iVar3 != 0) {
        if ((param_3 & 1) != 0) {
          iVar4 = iVar4 + -900;
        }
        if ((param_3 & 2) != 0) {
          iVar4 = iVar4 + -800;
        }
        if ((param_3 & 4) != 0) {
          iVar4 = iVar4 + -500;
        }
        if ((param_3 & 8) != 0) {
          iVar4 = iVar4 + -0x708;
        }
      }
      if (((-1 < iVar4) &&
          (iVar3 = PickUpWeapon(this,param_2,0,param_3,CONCAT22(uVar5,1),CONCAT22(uVar7,1)),
          iVar3 != 0)) && (iVar3 = CashBased__Fv(), iVar3 != 0)) {
        *(int *)(this + 0x540) = iVar4;
      }
    }
  }
  return;
}


