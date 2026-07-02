// ===== FUN @ 0009eb98 size=264 =====

/* TS_util.cpp */

void SendWeaponParams__F10tsweapon_tP11CBaseEntity(uint param_1,int param_2)

{
  enginefuncs_t *peVar1;
  char *pcVar2;
  int iVar3;
  double dVar4;
  double dVar5;
  double dVar6;
  undefined4 uVar7;
  char local_5c [63];
  undefined1 local_1d;
  
  if (param_1 < 0x26) {
    pcVar2 = (char *)get_weapon_info__F10tsweapon_t();
    if (*pcVar2 != '\0') {
      iVar3 = get_weapon_info__F10tsweapon_t();
      uVar7 = *(undefined4 *)(iVar3 + 0xd8);
      iVar3 = get_weapon_info__F10tsweapon_t();
      dVar6 = (double)*(float *)(iVar3 + 0x1c);
      iVar3 = get_weapon_info__F10tsweapon_t();
      dVar5 = (double)*(float *)(iVar3 + 0x34);
      iVar3 = get_weapon_info__F10tsweapon_t();
      dVar4 = (double)*(float *)(iVar3 + 0x30);
      iVar3 = get_weapon_info__F10tsweapon_t();
      sprintf(local_5c,"%i %f %f %f %f %i",param_1,(double)*(float *)(iVar3 + 0x28),dVar4,dVar5,
              dVar6,uVar7);
      peVar1 = PTR_g_engfuncs_001778b8;
      local_1d = 0;
      if (param_2 == 0) {
        (*PTR_g_engfuncs_001778b8->pfnMessageBegin)();
      }
      else {
        (*PTR_g_engfuncs_001778b8->pfnMessageBegin)();
      }
      (*peVar1->pfnWriteString)();
      (*peVar1->pfnMessageEnd)();
    }
  }
  return;
}
