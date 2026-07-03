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


// ===== FUN @ 0009314c size=505 =====

/* TS_player.cpp
   assets: items/gunpickup2.wav */

undefined4
CBasePlayer::PickUpWeapon
          (CBasePlayer *this,int param_2,s_weapon_status *param_3,int param_4,char param_5,
          char param_6)

{
  int iVar1;
  CTSGun *this_00;
  int iVar2;
  int iVar3;
  longdouble lVar4;
  uint uVar5;
  
  this_00 = (CTSGun *)this->m_pActiveItem;
  if (((this_00 != (CTSGun *)0x0) && (this_00->m_iId == 0x10)) &&
     ((iVar2 = get_weapon_info__F10tsweapon_t(param_2), *(char *)(iVar2 + 0x348) != '\x04' ||
      (*(int *)(iVar2 + 0xcc) == -1)))) {
    lVar4 = (longdouble)*(byte *)(iVar2 + 0xd0);
    if ((*(char *)(iVar2 + 0xdc) != '\0') && (param_3 != (s_weapon_status *)0x0)) {
      lVar4 = lVar4 * (longdouble)param_3->iClip;
    }
    iVar1 = (int)ROUND((longdouble)this->field_0x548 - lVar4);
    if ((-1 < iVar1) && (iVar3 = CTSGun::TakeWeapon(this_00,param_2,param_3,param_4), iVar3 != 0)) {
      if ((param_6 != '\0') &&
         ((__as__7EHANDLEP11CBaseEntity(&this_00->field_0x144 + param_2 * 0x38,this),
          *(int *)(iVar2 + 200) != -1 &&
          ((&this_00->field_0x114)[*(int *)(iVar2 + 200) * 0x38] != '\0')))) {
        __as__7EHANDLEP11CBaseEntity(&this_00->field_0x144 + *(int *)(iVar2 + 200) * 0x38,this);
      }
      this->field_0x548 = iVar1;
      lVar4 = (longdouble)GetSpeedBySlots__FP11CBasePlayer(this);
      SetPlayerSpeed(this,(float)lVar4);
      uVar5 = 0;
      EMIT_SOUND_DYN__FP7edict_siPCcffiif
                (this->pev->pContainingEntity,3,"items/gunpickup2.wav",fc_1,fc_0_800000011920929,0,
                 100,fc_0_5);
      if ((param_5 != '\0') || (this_00->m_pActiveItem == (s_weapon_status *)0x0)) {
        if ((*(int *)(iVar2 + 200) != -1) &&
           ((&this_00->field_0x114)[*(int *)(iVar2 + 200) * 0x38] != '\0')) {
          param_2 = *(int *)(iVar2 + 200);
        }
        iVar2 = CTSGun::SelectWeapon(this_00,param_2,uVar5 & 0xffff0000);
        if (iVar2 != 0) {
          (*this_00->vtbl->Deploy)(this_00);
        }
      }
      return 1;
    }
  }
  return 0;
}


// ===== FUN @ 00077f60 size=380 =====

/* WARNING: Unknown calling convention -- yet parameter storage is locked */
/* TS_gamerulesclass.cpp */

void CTSGameRules::InitTeams(CTSGameRules *this)

{
  int iVar1;
  int iVar2;
  int *piVar3;
  int *piVar4;
  int local_1c;
  
  piVar3 = &this->field_0x1c;
  for (iVar1 = 0x8c; iVar1 != 0; iVar1 = iVar1 + -1) {
    *piVar3 = 0;
    piVar3 = piVar3 + 1;
  }
  local_1c = 0;
  do {
    (&this->field_0x20)[local_1c * 0x23] = 0;
    (&this->field_0x24)[local_1c * 0x23] = 0;
    (&this->field_0x1c)[local_1c * 0x23] = (int)ROUND(*(float *)(PTR_initial_cash_001779b8 + 0xc));
    iVar2 = local_1c + 1;
    sprintf(&this->field_0x28 + local_1c * 0x8c,"Team%i",iVar2);
    sprintf((char *)(&this->field_0x68 + local_1c * 0x23),"info_player_team%i",iVar2);
    piVar3 = (int *)&DAT_00107630;
    piVar4 = &this->field_0x88 + local_1c * 0x23;
    for (iVar1 = 5; iVar1 != 0; iVar1 = iVar1 + -1) {
      *piVar4 = *piVar3;
      piVar3 = piVar3 + 1;
      piVar4 = piVar4 + 1;
    }
    *(short *)piVar4 = (short)*piVar3;
    *(undefined1 *)((int)piVar4 + 2) = *(undefined1 *)((int)piVar3 + 2);
    SetTeamModelList(this,local_1c,defTeamModels + local_1c * 0x200);
    local_1c = iVar2;
  } while (iVar2 < 4);
  if ((byte)(*(char *)&this->pev - 2U) < 2) {
    SetTeamModelList(this,0,"seal");
    SetTeamModelList(this,1,"merc");
    SetTeamList(this,"Specialists;Mercenaries");
    piVar3 = (int *)"info_player_start";
    piVar4 = &this->field_0x88;
    for (iVar1 = 4; iVar1 != 0; iVar1 = iVar1 + -1) {
      *piVar4 = *piVar3;
      piVar3 = piVar3 + 1;
      piVar4 = piVar4 + 1;
    }
    *(short *)piVar4 = (short)*piVar3;
  }
  return;
}


// ===== FUN @ 000780dc size=380 =====

/* WARNING: Unknown calling convention -- yet parameter storage is locked */
/* TS_gamerulesclass.cpp */

void CTSGameRules::SetTeamList(CTSGameRules *this,char *a0)

{
  int iVar1;
  char cVar2;
  char cVar3;
  char cVar4;
  enginefuncs_t *peVar5;
  char *__src;
  undefined1 *puVar6;
  int *__dest;
  int *piVar7;
  int local_220;
  char local_21c [524];
  
  *(undefined1 *)&this->field_0x1cc = 0;
  *(undefined1 *)&this->field_0x140 = 0;
  *(undefined1 *)&this->field_0xb4 = 0;
  this->field_0x28 = 0;
  puVar6 = &this[-1].field_0x7f9c;
  strcpy(local_21c,a0);
  __src = strtok(local_21c,";");
  peVar5 = PTR_g_engfuncs_001778b8;
  local_220 = 0;
  iVar1 = *(int *)PTR_g_theOneMode_001770dc;
  if (((__src != (char *)0x0) && (*__src != '\0')) && ((iVar1 == 0) != 0x7fffffff)) {
    __dest = (int *)&this->field_0x28;
    do {
      if ((local_220 == 0) && (*(int *)PTR_g_theOneMode_001770dc != 0)) {
        cVar2 = s_The_ONE_00107747[1];
        cVar3 = s_The_ONE_00107747[2];
        cVar4 = s_The_ONE_00107747[3];
        this->field_0x28 = s_The_ONE_00107747[0];
        this->field_0x29 = cVar2;
        this->field_0x2a = cVar3;
        this->field_0x2b = cVar4;
        this->field_0x2c = s_The_ONE_00107747._4_4_;
        puVar6 = &this->field_0x28;
        piVar7 = &this->field_0xb4;
        local_220 = 1;
      }
      else {
        puVar6 = puVar6 + 0x8c;
        piVar7 = __dest + 0x23;
        local_220 = local_220 + 1;
        strcpy((char *)__dest,__src);
      }
      __src = strtok((char *)0x0,";");
      (*peVar5->pfnAlertMessage)(1,"created team %s\n",puVar6);
    } while (((__src != (char *)0x0) && (*__src != '\0')) &&
            (__dest = piVar7, local_220 < (int)((uint)(iVar1 == 0) * 2 + 2)));
  }
  return;
}


// ===== FUN @ 00079b04 size=182 =====

/* WARNING: Unknown calling convention -- yet parameter storage is locked */
/* TS_gamerulesclass.cpp */

undefined4 CTSGameRules::GetTeamIndex(CTSGameRules *this,char *a0)

{
  int iVar1;
  
  if ((this->field_0x28 != '\0') && (iVar1 = strcasecmp(a0,&this->field_0x28), iVar1 == 0)) {
    return 0;
  }
  if (((char)this->field_0xb4 != '\0') &&
     (iVar1 = strcasecmp(a0,(char *)&this->field_0xb4), iVar1 == 0)) {
    return 1;
  }
  if (((char)this->field_0x140 != '\0') &&
     (iVar1 = strcasecmp(a0,(char *)&this->field_0x140), iVar1 == 0)) {
    return 2;
  }
  if (((char)this->field_0x1cc != '\0') &&
     (iVar1 = strcasecmp(a0,(char *)&this->field_0x1cc), iVar1 == 0)) {
    return 3;
  }
  return 0xffffffff;
}


// ===== FUN @ 000acf44 size=112 =====

/* WARNING: Unknown calling convention -- yet parameter storage is locked */
/* client.cpp */

void ClientPutInServer__FP7edict_s(edict_s *a0)

{
  int *piVar1;
  CBasePlayer *this;
  int local_c;
  
  this = (CBasePlayer *)GetClassPtr__H1Z11CBasePlayer_PX01_PX01(a0 + 0x80);
  CBasePlayer::SetCustomDecalFrames(this,-1);
  sscanf(*(char **)(PTR_initial_cash_001779b8 + 4),"%i",&local_c);
  CBasePlayer::GiveCash(this,local_c);
  (*this->vtbl->Spawn)(this);
  piVar1 = &this->pev->effects;
  *piVar1 = *piVar1 | 0x20;
  return;
}


