// ===== FUN @ 000890b8 size=132 =====

void FUN_000890b8(void)

{
  int *piVar1;
  bool bVar2;
  CBaseEntity *a0;
  int unaff_retaddr;
  CBasePlayer *in_stack_00000028;
  
  a0 = (CBaseEntity *)0x0;
  piVar1 = &in_stack_00000028->pev->weapons;
  *piVar1 = *piVar1 | 0x80000000;
  bVar2 = true;
  while( true ) {
    a0 = (CBaseEntity *)
         UTIL_FindEntityByClassname__FP11CBaseEntityPCc(a0,(char *)(unaff_retaddr + 0x7f6bd));
    if (a0 == (CBaseEntity *)0x0) break;
    (*a0->vtbl->Touch)(a0,(CBaseEntity *)in_stack_00000028);
    bVar2 = false;
  }
  if (bVar2) {
    CBasePlayer::GiveNamedItem(in_stack_00000028,&UNK_0007efb2 + unaff_retaddr);
    CBasePlayer::BuyArmament(in_stack_00000028);
    *(int *)&in_stack_00000028->field_0x53c = in_stack_00000028->field_0x540;
  }
  return;
}


// ===== FUN @ 00075ff4 size=302 =====

/* WARNING: Unknown calling convention -- yet parameter storage is locked */
/* TS_gamerules.cpp */

void SetPlayerRespawnTime__FP11CBasePlayerib(CBasePlayer *a0,int a1,bool a2)

{
  float fVar1;
  float fVar2;
  
  if ((((*PTR_TSGameRules_00177300 == '\x03') || (*PTR_TSGameRules_00177300 == '\x02')) ||
      (*(int *)PTR_g_lastManStanding_0017733c == 0)) || (*PTR_TSGameRules_00177300 != '\0')) {
    a0->field_0x770 = (int)((float)a1 + (*PTR_gpGlobals_00177410)->time);
  }
  else {
    fVar1 = *(float *)(PTR_respawnnumber_001779d4 + 0xc);
    if (fVar1 < fc_1) {
      fVar1 = fc_1;
    }
    if (fVar1 <= (float)(short)(char)a0->field_0x7ba) {
      a0->field_0x770 = -0x40800000;
    }
    else {
      fVar1 = *(float *)(PTR_TSGameRules_00177300 + 8) - (*PTR_gpGlobals_00177410)->time;
      fVar2 = (float)a1;
      if (fVar1 < fVar2) {
        fVar2 = fVar1;
      }
      a0->field_0x770 = (int)(fVar2 + (*PTR_gpGlobals_00177410)->time);
      fVar1 = *(float *)(PTR_TSGameRules_00177300 + 8) - (*PTR_gpGlobals_00177410)->time;
      if (fc_10 < fVar1) {
        fVar1 = fc_10;
      }
      a0->field_0x7bc = (int)(fVar1 + (*PTR_gpGlobals_00177410)->time);
    }
  }
  if ((float)a0->field_0x770 < fc_0) {
    a0->field_0x74c = 1;
  }
  else {
    a0->field_0x74c = 0xb;
  }
  return;
}


// ===== FUN @ 00099ca4 size=142 =====

/* WARNING: Unknown calling convention -- yet parameter storage is locked */
/* TS_spectator.cpp */

void CBasePlayer::Observer_Idle(CBasePlayer *this)

{
  int iVar1;
  
  if ((((float)this->field_0x764 <= (*PTR_gpGlobals_00177410)->time) &&
      (this->field_0x764 = (int)((float)dc_0_5 + (*PTR_gpGlobals_00177410)->time),
      this->pev->iuser1 != 3)) && (this->field_0x75c != 0)) {
    iVar1 = (**(code **)(*(int *)(this->field_0x75c + 0x60) + 0xb4))(this->field_0x75c);
    if ((iVar1 == 0) || (*(int *)(*(int *)this->field_0x75c + 0x244) != 0)) {
      Observer_FindNextPlayer(this,false);
    }
  }
  return;
}


// ===== FUN @ 00079134 size=1256 =====

/* WARNING: Unknown calling convention -- yet parameter storage is locked */
/* TS_gamerulesclass.cpp */

void CTSGameRules::InitHUD(CTSGameRules *this,CBasePlayer *a0)

{
  float fVar1;
  enginefuncs_t *peVar2;
  undefined *puVar3;
  undefined uVar4;
  char *a5;
  int *piVar5;
  int iVar6;
  undefined3 extraout_var;
  uint uVar7;
  int iVar8;
  undefined2 in_FPUControlWord;
  entvars_t *peVar9;
  vec_t local_4c;
  vec_t local_48;
  vec_t local_44;
  vec_t local_3c;
  vec_t local_38;
  vec_t local_34;
  int local_24;
  undefined2 local_20;
  undefined2 uStack_1e;
  undefined3 extraout_var_00;
  
  if ((*(float *)PTR_g_flWeaponCheat_00177bcc != 0.0) &&
     (TSHudMessageOne__FUcUcUciiPcP9entvars_s
                (0x2d,0xc3,0x2d,8,100,"This server has cheats enabled.",a0->pev),
     0.0 < *(float *)(PTR_damagemult_00177294 + 0xc))) {
    peVar9 = a0->pev;
    a5 = (char *)UTIL_VarArgs__FPce("This server has custom weapon damages (%.2f)");
    TSHudMessageOne__FUcUcUciiPcP9entvars_s(0x2d,0xc3,0x2d,8,100,a5,peVar9);
  }
  if (this->field_0x5884 != '\0') {
    InformClientsOnWeaponsParams__FP11CBaseEntity((CBaseEntity *)a0);
  }
  puVar3 = PTR_gmsgTSServerSetting_00177d44;
  peVar2 = PTR_g_engfuncs_001778b8;
  (*PTR_g_engfuncs_001778b8->pfnMessageBegin)
            (1,*(undefined4 *)PTR_gmsgTSServerSetting_00177d44,0,a0->pev->pContainingEntity);
  (*peVar2->pfnWriteByte)(0);
  (*peVar2->pfnWriteShort)(this->field_0x578);
  (*peVar2->pfnMessageEnd)();
  (*peVar2->pfnMessageBegin)(1,*(undefined4 *)puVar3,0,a0->pev->pContainingEntity);
  (*peVar2->pfnWriteByte)(1);
  (*peVar2->pfnWriteShort)((uint)(*(float *)(PTR_dmcashbased_00177290 + 0xc) != 0.0));
  (*peVar2->pfnMessageEnd)();
  (*peVar2->pfnMessageBegin)(1,*(undefined4 *)puVar3,0,a0->pev->pContainingEntity);
  (*peVar2->pfnWriteByte)(2);
  local_24 = (int)ROUND(*(float *)(PTR_cv_ammocount_001776cc + 0xc));
  (*peVar2->pfnWriteShort)(local_24);
  (*peVar2->pfnMessageEnd)();
  (*peVar2->pfnMessageBegin)(1,*(undefined4 *)puVar3,0,a0->pev->pContainingEntity);
  (*peVar2->pfnWriteByte)(3);
  (*peVar2->pfnWriteShort)(this->field_0x5870);
  (*peVar2->pfnMessageEnd)();
  if ((this->m_flRoundEnd != 0.0) && ((char)this->field_0x10 != '\0')) {
    (*peVar2->pfnMessageBegin)
              (1,*(undefined4 *)PTR_gmsgTSRoundTime_001770d8,0,a0->pev->pContainingEntity);
    fVar1 = this->m_flRoundEnd - (*PTR_gpGlobals_00177410)->time;
    if (fVar1 < 0.0) {
      fVar1 = 0.0;
    }
    local_24 = (int)ROUND(fVar1);
    (*peVar2->pfnWriteByte)(local_24);
    peVar2 = PTR_g_engfuncs_001778b8;
    (*PTR_g_engfuncs_001778b8->pfnWriteByte)(0);
    (*peVar2->pfnMessageEnd)();
  }
  peVar2 = PTR_g_engfuncs_001778b8;
  iVar8 = 1;
  if (0 < (*PTR_gpGlobals_00177410)->maxClients) {
    do {
      piVar5 = (int *)UTIL_PlayerByIndex__Fi(iVar8);
      if ((piVar5 != (int *)0x0) && (iVar6 = (**(code **)(piVar5[0x18] + 200))(piVar5), iVar6 != 0))
      {
        (*peVar2->pfnMessageBegin)
                  (1,*(undefined4 *)PTR_gmsgSpectator_00177b94,0,a0->pev->pContainingEntity);
        uVar4 = (*peVar2->pfnIndexOfEdict)(*(undefined4 *)(*piVar5 + 0x208));
        (*peVar2->pfnWriteByte)(CONCAT31(extraout_var,uVar4));
        (*peVar2->pfnWriteByte)((uint)(*(int *)(*piVar5 + 0x244) != 0));
        (*peVar2->pfnMessageEnd)();
      }
      iVar8 = iVar8 + 1;
    } while (iVar8 <= (*PTR_gpGlobals_00177410)->maxClients);
  }
  peVar9 = a0->pev;
  local_3c = (peVar9->origin).x;
  local_38 = (peVar9->origin).y;
  local_34 = (peVar9->origin).z;
  peVar9 = a0->pev;
  local_4c = (peVar9->v_angle).x;
  local_48 = (peVar9->v_angle).y;
  local_44 = (peVar9->v_angle).z;
  CBasePlayer::StartObserver(a0,&local_3c,&local_4c);
  a0->field_0x7ba = 0;
  a0->m_iGrenadeBuys = 0;
  *(undefined4 *)&a0->field_0x53c = 0;
  *(undefined4 *)&a0->field_0x538 = 0;
  *(undefined4 *)&a0->field_0x7c4 = 0;
  a0->field_0x7c8 = 0;
  a0->field_0x7cc = 0;
  fVar1 = *(float *)(PTR_respawntime_00177d94 + 0xc);
  if (*(float *)(PTR_respawntime_00177d94 + 0xc) < fc_5) {
    fVar1 = fc_5;
  }
  if (fc_30 < fVar1) {
    fVar1 = fc_30;
  }
  _local_20 = CONCAT22(uStack_1e,in_FPUControlWord);
  local_24 = (int)ROUND(fVar1);
  SetPlayerRespawnTime__FP11CBasePlayerib(a0,local_24,false);
  memset((void *)((int)&a0->field_0x499 + 1),0,0x40);
  CBasePlayer::ResetArmament(a0);
  if (*(int *)PTR_g_teamplay_00177f1c - 2U < 2) {
    *(undefined4 *)&a0->field_0x758 = 0;
    SendObjectiveList(this,(CBaseEntity *)a0);
  }
  uVar4 = (*PTR_g_engfuncs_001778b8->pfnIndexOfEdict)(a0->pev->pContainingEntity);
  uVar7 = CONCAT31(extraout_var_00,uVar4);
  if (uVar7 < 0x20) {
    piVar5 = &this->field_0x684 + uVar7 * 0x12;
    for (iVar8 = 0x12; iVar8 != 0; iVar8 = iVar8 + -1) {
      *piVar5 = 0;
      piVar5 = piVar5 + 1;
    }
    (&this->field_0x6c8)[uVar7 * 0x12] = uVar7 + 1;
  }
  return;
}


// ===== FUN @ 00077920 size=1098 =====

/* WARNING: Unknown calling convention -- yet parameter storage is locked */
/* TS_gamerulesclass.cpp */

void CTSGameRules::RestartRound(CTSGameRules *this)

{
  CBaseEntity *pCVar1;
  entvars_t *peVar2;
  float fVar3;
  globalvars_t **ppgVar4;
  enginefuncs_t *peVar5;
  CBasePlayer *this_00;
  int iVar6;
  int iVar7;
  int iVar8;
  undefined2 in_FPUControlWord;
  undefined1 *local_6c;
  undefined1 *local_68;
  int local_60;
  int local_50;
  vec_t local_4c;
  vec_t local_48;
  vec_t local_44;
  vec_t local_3c;
  vec_t local_38;
  vec_t local_34;
  int local_24;
  undefined2 local_20;
  undefined2 uStack_1e;
  
  fVar3 = *(float *)(PTR_roundtime_00177580 + 0xc);
  if (*(float *)(PTR_roundtime_00177580 + 0xc) < fc_30) {
    fVar3 = fc_30;
  }
  if (fc_180 < fVar3) {
    fVar3 = fc_180;
  }
  this->m_flRoundEnd = fVar3 + (*PTR_gpGlobals_00177410)->time;
  this->m_flRoundRestart = 0.0;
  peVar5 = PTR_g_engfuncs_001778b8;
  if ((char)this->field_0x10 != '\0') {
    (*PTR_g_engfuncs_001778b8->pfnMessageBegin)(2,*(undefined4 *)PTR_gmsgTSRoundTime_001770d8,0,0);
    fVar3 = this->m_flRoundEnd - (*PTR_gpGlobals_00177410)->time;
    if (fVar3 < fc_0) {
      fVar3 = fc_0;
    }
    _local_20 = CONCAT22(uStack_1e,in_FPUControlWord);
    local_24 = (int)ROUND(fVar3);
    (*peVar5->pfnWriteByte)(local_24);
    peVar5 = PTR_g_engfuncs_001778b8;
    (*PTR_g_engfuncs_001778b8->pfnWriteByte)(0);
    (*peVar5->pfnMessageEnd)();
  }
  InitObjectives(this);
  SetObjective(this);
  if (*(char *)&this->pev == '\x02') {
    TSHudMessageAll__FUcUcUciiPc(0,0,0,8,0x1e,"Defeat the opposite team");
  }
  ppgVar4 = PTR_gpGlobals_00177410;
  local_50 = 1;
  if (0 < (*PTR_gpGlobals_00177410)->maxClients) {
    do {
      this_00 = (CBasePlayer *)UTIL_PlayerByIndex__Fi(local_50);
      if ((this_00 != (CBasePlayer *)0x0) &&
         (iVar6 = (*this_00->vtbl->IsPlayer)(this_00), iVar6 != 0)) {
        if ((byte)(*(char *)&this->pev - 2U) < 2) {
          if ((this->field_0x5874 != '\0') || (*(char *)&this->pev == '\x02')) {
            this_00->field_0x7bc = (int)(fc_10 + (*ppgVar4)->time);
          }
        }
        else {
          this_00->field_0x7bc = (int)(fc_10 + (*ppgVar4)->time);
        }
        this_00->field_0x770 = (int)((float)dc_3 + (*ppgVar4)->time);
        this_00->field_0x74c = 0xb;
        this_00->m_iGrenadeBuys = 0;
        this_00->field_0x7ba = 0;
        this_00->m_flPowerupExpireTime = 0.0;
        this_00->m_flPowerupTime = 0;
        this_00->m_iPowerups = 0;
        this_00->m_iHeldPowerup = 0;
        pCVar1 = this_00->m_pActiveItem;
        local_60 = 0;
        if ((pCVar1 != (CBaseEntity *)0x0) && (*(int *)&pCVar1->field_0x88 == 0x10)) {
          iVar6 = 0;
          iVar8 = 0x120;
          do {
            if (((*(char *)((int)pCVar1 + iVar8 + -0xc) != '\0') && (0 < iVar6)) && (iVar6 != 0x1d))
            {
              iVar7 = get_weapon_info__F10tsweapon_t(iVar6);
              local_60 = local_60 + *(int *)(iVar7 + 0xd8);
              if ((*(byte *)((int)&pCVar1->m_pGoalEnt + iVar8) & 1) != 0) {
                local_60 = local_60 + 900;
              }
              if ((*(byte *)((int)&pCVar1->m_pGoalEnt + iVar8) & 2) != 0) {
                local_60 = local_60 + 800;
              }
              if ((*(byte *)((int)&pCVar1->m_pGoalEnt + iVar8) & 4) != 0) {
                local_60 = local_60 + 500;
              }
              if ((*(byte *)((int)&pCVar1->m_pGoalEnt + iVar8) & 8) != 0) {
                local_60 = local_60 + 0x708;
              }
            }
            iVar8 = iVar8 + 0x38;
            iVar6 = iVar6 + 1;
          } while (iVar6 < 0x26);
          iVar6 = 0;
          local_6c = &pCVar1->field_0xd0;
          local_68 = local_6c;
          do {
            iVar8 = get_ammo_info__Fi(iVar6);
            if (*(int *)(iVar8 + 0x10) < *(int *)(local_6c + 4)) {
              iVar8 = get_ammo_info__Fi(iVar6);
              iVar7 = get_ammo_info__Fi(iVar6);
              local_60 = local_60 +
                         (*(int *)(iVar8 + 4) * *(int *)(local_68 + 4)) / *(int *)(iVar7 + 0x10);
            }
            iVar8 = iVar6 + 1;
            iVar7 = get_ammo_info__Fi(iVar8);
            if (*(int *)(iVar7 + 0x10) < *(int *)(local_6c + 8)) {
              iVar7 = get_ammo_info__Fi(iVar8);
              iVar8 = get_ammo_info__Fi(iVar8);
              local_60 = local_60 +
                         (*(int *)(iVar7 + 4) * *(int *)(local_68 + 8)) / *(int *)(iVar8 + 0x10);
            }
            local_68 = local_68 + 8;
            local_6c = local_6c + 8;
            iVar6 = iVar6 + 2;
          } while (iVar6 < 0x10);
        }
        CBasePlayer::GiveCash(this_00,local_60);
        if (this_00->pev->iuser1 == 0) {
          peVar2 = this_00->pev;
          local_3c = (peVar2->origin).x;
          local_38 = (peVar2->origin).y;
          local_34 = (peVar2->origin).z;
          peVar2 = this_00->pev;
          local_4c = (peVar2->v_angle).x;
          local_48 = (peVar2->v_angle).y;
          local_44 = (peVar2->v_angle).z;
          CBasePlayer::StartObserver(this_00,&local_3c,&local_4c);
        }
      }
      local_50 = local_50 + 1;
    } while (local_50 <= (*ppgVar4)->maxClients);
  }
  CHalfLifeTeamplay::RecountTeams(*(CHalfLifeTeamplay **)PTR_g_pGameRules_00177e70,false);
  RespawnEntities(this,true);
  return;
}


// ===== FUN @ 00078750 size=695 =====

/* WARNING: Unknown calling convention -- yet parameter storage is locked */
/* TS_gamerulesclass.cpp */

void CTSGameRules::ResetRestartRound(CTSGameRules *this,int a0)

{
  float fVar1;
  globalvars_t **ppgVar2;
  enginefuncs_t *peVar3;
  int *piVar4;
  int *piVar5;
  int *local_30;
  int *local_2c;
  int *local_28;
  int local_24;
  int local_20;
  
  ppgVar2 = PTR_gpGlobals_00177410;
  if (((byte)(*(char *)&this->pev - 2U) < 2) || (*(int *)PTR_g_lastManStanding_0017733c != 0)) {
    this->m_flRoundRestart = (float)dc_1 + (*PTR_gpGlobals_00177410)->time;
    this->m_flRoundEnd = 0.0;
    peVar3 = PTR_g_engfuncs_001778b8;
    if ((char)this->field_0x10 != '\0') {
      (*PTR_g_engfuncs_001778b8->pfnMessageBegin)(2,*(undefined4 *)PTR_gmsgTSRoundTime_001770d8,0,0)
      ;
      fVar1 = this->m_flRoundRestart - (*ppgVar2)->time;
      if (fVar1 < fc_0) {
        fVar1 = fc_0;
      }
      (*peVar3->pfnWriteByte)((int)ROUND(fVar1));
      peVar3 = PTR_g_engfuncs_001778b8;
      (*PTR_g_engfuncs_001778b8->pfnWriteByte)(0);
      (*peVar3->pfnMessageEnd)();
    }
    peVar3 = PTR_g_engfuncs_001778b8;
    if (1 < a0) {
      local_20 = 0;
      piVar5 = (int *)&this->field_0x28;
      local_24 = 0x10;
      piVar4 = &this->field_0x10;
      local_30 = piVar4;
      local_2c = piVar4;
      local_28 = piVar4;
      do {
        *(undefined4 *)((int)&this->field_0x10 + local_24) = 0;
        local_30[5] = 0;
        local_2c[3] = (int)ROUND(*(float *)(PTR_initial_cash_001779b8 + 0xc));
        if (*(char *)(piVar4 + 6) != '\0') {
          (*peVar3->pfnMessageBegin)(2,*(undefined4 *)PTR_gmsgTeamScore_001779f8,0,0);
          (*peVar3->pfnWriteString)(piVar5);
          (*peVar3->pfnWriteShort)(local_28[5]);
          (*peVar3->pfnWriteShort)(0);
          (*peVar3->pfnMessageEnd)();
        }
        *(undefined4 *)((int)&this->field_0x9c + local_24) = 0;
        local_30[0x28] = 0;
        local_2c[0x26] = (int)ROUND(*(float *)(PTR_initial_cash_001779b8 + 0xc));
        if ((char)piVar4[0x29] != '\0') {
          (*peVar3->pfnMessageBegin)(2,*(undefined4 *)PTR_gmsgTeamScore_001779f8,0,0);
          (*peVar3->pfnWriteString)(piVar5 + 0x23);
          (*peVar3->pfnWriteShort)(local_28[0x28]);
          (*peVar3->pfnWriteShort)(0);
          (*peVar3->pfnMessageEnd)();
        }
        piVar5 = piVar5 + 0x46;
        local_28 = local_28 + 0x46;
        piVar4 = piVar4 + 0x46;
        local_2c = local_2c + 0x46;
        local_30 = local_30 + 0x46;
        local_24 = local_24 + 0x118;
        local_20 = local_20 + 2;
      } while (local_20 < 4);
    }
    TSHudMessageAll__FUcUcUciiPc(0,0,0,8,0x1e,"Restarting Round");
  }
  return;
}


// ===== FUN @ 00078a08 size=1558 =====

/* TS_gamerulesclass.cpp */

void CTSGameRules::TeamWin(CTSGameRules *this,int a0,int a1,char a2,char param_5)

{
  enginefuncs_t *peVar1;
  undefined *puVar2;
  int iVar3;
  CBasePlayer *this_00;
  int iVar4;
  char *pcVar5;
  int iVar6;
  undefined4 uVar7;
  int *piVar8;
  int *piVar9;
  undefined2 in_FPUControlWord;
  undefined1 *puVar10;
  int local_6c;
  int local_68;
  int local_54;
  int local_50;
  char local_4c [40];
  int local_24;
  undefined2 local_20;
  undefined2 uStack_1e;
  
  peVar1 = PTR_g_engfuncs_001778b8;
  if (((char)this->field_0x10 != '\0') && (param_5 == '\0')) {
    (*PTR_g_engfuncs_001778b8->pfnMessageBegin)(2,*(undefined4 *)PTR_gmsgTSRoundTime_001770d8,0,0);
    _local_20 = CONCAT22(uStack_1e,in_FPUControlWord);
    local_24 = (int)ROUND(this->m_flRoundEnd - (*PTR_gpGlobals_00177410)->time);
    (*peVar1->pfnWriteByte)(local_24);
    (*peVar1->pfnWriteByte)(2);
    (*peVar1->pfnMessageEnd)();
  }
  if (a2 != '\0') {
    this->m_flRoundRestart = (float)dc_3 + (*PTR_gpGlobals_00177410)->time;
    this->m_flRoundEnd = 0.0;
  }
  iVar3 = CountPlayers(this);
  puVar2 = PTR_initial_cash_001779b8;
  peVar1 = PTR_g_engfuncs_001778b8;
  if (iVar3 < 2) {
    this->field_0x20 = 0;
    this->field_0x24 = 0;
    this->field_0x1c = (int)ROUND(*(float *)(puVar2 + 0xc));
    this->field_0xac = 0;
    this->field_0xb0 = 0;
    this->field_0xa8 = (int)ROUND(*(float *)(puVar2 + 0xc));
    this->field_0x138 = 0;
    this->field_0x13c = 0;
    this->field_0x134 = (int)ROUND(*(float *)(puVar2 + 0xc));
    this->field_0x1c4 = 0;
    this->field_0x1c8 = 0;
    _local_20 = CONCAT22(uStack_1e,in_FPUControlWord);
    local_24 = (int)ROUND(*(float *)(puVar2 + 0xc));
    this->field_0x1c0 = local_24;
    TSHudMessageAll__FUcUcUciiPc(0,0,0,8,0x1e,"Waiting for other players.");
  }
  else {
    iVar3 = 1;
    if (0 < (*PTR_gpGlobals_00177410)->maxClients) {
      do {
        this_00 = (CBasePlayer *)UTIL_PlayerByIndex__Fi(iVar3);
        if ((this_00 != (CBasePlayer *)0x0) &&
           (iVar4 = (*this_00->vtbl->IsPlayer)(this_00), iVar4 != 0)) {
          if (a1 == 0) {
            sscanf(*(char **)(PTR_cashforwin_0017744c + 4),"%i",&local_50);
          }
          else {
            local_50 = a1;
          }
          sscanf(*(char **)(PTR_cashforlose_00177900 + 4),"%i",&local_54);
          pcVar5 = (char *)(*this_00->vtbl->TeamID)(this_00);
          iVar4 = GetTeamIndex(this,pcVar5);
          if (a0 != iVar4) {
            CBasePlayer::GiveCash(this_00,local_54);
          }
          else {
            CBasePlayer::GiveCash(this_00,local_50);
          }
          (*peVar1->pfnMessageBegin)
                    (1,*(undefined4 *)PTR_gmsgTSVoiceMessage_00177944,0,
                     this_00->pev->pContainingEntity);
          (*peVar1->pfnWriteByte)((uint)(a0 != iVar4));
          (*peVar1->pfnMessageEnd)();
        }
        iVar3 = iVar3 + 1;
      } while (iVar3 <= (*PTR_gpGlobals_00177410)->maxClients);
    }
    if (a0 == 2) {
      TSHudMessageAll__FUcUcUciiPc(0,0,0,8,0x1e,"Game Draw");
      pcVar5 = "Round draw\n";
    }
    else {
      (&this->field_0x24)[a0 * 0x23] = (&this->field_0x24)[a0 * 0x23] + 1;
      if (param_5 == '\0') {
        puVar10 = &this->field_0x28 + a0 * 0x8c;
        pcVar5 = "%s Win!";
      }
      else {
        puVar10 = &this->field_0x28 + a0 * 0x8c;
        pcVar5 = "%s Score!";
      }
      sprintf(local_4c,pcVar5,puVar10);
      TSHudMessageAll__FUcUcUciiPc(0,0,0,8,0x1e,local_4c);
      puVar2 = PTR_g_pGameRules_00177e70;
      (**(code **)(**(int **)PTR_g_pGameRules_00177e70 + 0xd0))
                (*(int **)PTR_g_pGameRules_00177e70,&this->field_0x28 + a0 * 0x8c);
      iVar3 = 1;
      if (0 < (*PTR_gpGlobals_00177410)->maxClients) {
        do {
          iVar4 = UTIL_PlayerByIndex__Fi(iVar3);
          if ((iVar4 != 0) &&
             (iVar6 = (**(code **)(*(int *)(iVar4 + 0x60) + 200))(iVar4), iVar6 != 0)) {
            iVar6 = **(int **)puVar2;
            uVar7 = (**(code **)(*(int *)(iVar4 + 0x60) + 0xd0))(iVar4);
            (**(code **)(iVar6 + 0xd0))(*(undefined4 *)puVar2,uVar7);
          }
          iVar3 = iVar3 + 1;
        } while (iVar3 <= (*PTR_gpGlobals_00177410)->maxClients);
      }
      pcVar5 = "Team \"%s\" scored \"%d\" with \"%d\" players\n";
    }
    UTIL_LogPrintf__FPce(pcVar5);
  }
  peVar1 = PTR_g_engfuncs_001778b8;
  if ((byte)(*(char *)&this->pev - 2U) < 2) {
    local_68 = 0;
    piVar9 = (int *)&this->field_0x28;
    local_6c = 0x10;
    piVar8 = &this->field_0x10;
    do {
      if (*(char *)(piVar8 + 6) != '\0') {
        (*peVar1->pfnMessageBegin)(2,*(undefined4 *)PTR_gmsgTeamScore_001779f8,0,0);
        (*peVar1->pfnWriteString)(piVar9);
        (*peVar1->pfnWriteShort)(*(undefined4 *)((int)&this->field_0x14 + local_6c));
        (*peVar1->pfnWriteShort)(0);
        (*peVar1->pfnMessageEnd)();
      }
      if ((char)piVar8[0x29] != '\0') {
        (*peVar1->pfnMessageBegin)(2,*(undefined4 *)PTR_gmsgTeamScore_001779f8,0,0);
        (*peVar1->pfnWriteString)(piVar9 + 0x23);
        (*peVar1->pfnWriteShort)(*(undefined4 *)((int)&this->field_0xa0 + local_6c));
        (*peVar1->pfnWriteShort)(0);
        (*peVar1->pfnMessageEnd)();
      }
      piVar9 = piVar9 + 0x46;
      piVar8 = piVar8 + 0x46;
      local_6c = local_6c + 0x118;
      local_68 = local_68 + 2;
    } while (local_68 < 4);
  }
  return;
}


// ===== FUN @ 00079c00 size=1342 =====

/* WARNING: Unknown calling convention -- yet parameter storage is locked */
/* TS_gamerulesclass.cpp */

void CTSGameRules::CheckDeads(CTSGameRules *this)

{
  globalvars_t **ppgVar1;
  enginefuncs_t *peVar2;
  undefined uVar3;
  int *piVar4;
  int iVar5;
  char *pcVar6;
  undefined3 extraout_var;
  CTSGameRules *pCVar7;
  int iVar8;
  int iVar9;
  CTSGameRules *a0;
  undefined2 in_FPUControlWord;
  undefined4 uVar10;
  CTSGameRules *in_stack_ffffffa4;
  ushort uVar11;
  int local_44;
  int *local_40;
  int local_3c [6];
  int local_24;
  undefined2 local_20;
  undefined2 uStack_1e;
  
  this->m_flCheckDeads = 0.0;
  if ((fc_0 == this->m_flRoundRestart) && (*(char *)&this->pev != '\x01')) {
    if ((*(char *)&this->pev == '\0') && (*(int *)PTR_g_lastManStanding_0017733c != 0)) {
      iVar8 = 0;
      local_44 = 0;
      local_40 = (int *)0x0;
      for (iVar9 = 1; iVar9 <= (*PTR_gpGlobals_00177410)->maxClients; iVar9 = iVar9 + 1) {
        piVar4 = (int *)UTIL_PlayerByIndex__Fi(iVar9);
        if ((piVar4 != (int *)0x0) &&
           (iVar5 = (**(code **)(piVar4[0x18] + 200))(piVar4), iVar5 != 0)) {
          local_44 = local_44 + 1;
          iVar5 = (**(code **)(piVar4[0x18] + 0xb4))(piVar4);
          if (iVar5 != 0) {
            iVar8 = iVar8 + 1;
            local_40 = piVar4;
          }
        }
      }
      if (iVar8 == 1) {
        if (local_44 < 2) {
          TSHudMessageAll__FUcUcUciiPc(0,0xff,0,8,0x1e,"Waiting for other players.");
        }
        else {
          pcVar6 = (char *)UTIL_VarArgs__FPce("%s is the Last Man Standing! %i frags");
          TSHudMessageAll__FUcUcUciiPc(0,0xff,0,1,0x1e,pcVar6);
          *(float *)(*local_40 + 0x164) = *(float *)(*local_40 + 0x164) + 10.0;
          peVar2 = PTR_g_engfuncs_001778b8;
          (*PTR_g_engfuncs_001778b8->pfnMessageBegin)
                    (2,*(undefined4 *)PTR_gmsgScoreInfo_00177108,0,0);
          uVar3 = (*peVar2->pfnIndexOfEdict)(*(undefined4 *)(*local_40 + 0x208));
          (*peVar2->pfnWriteByte)(CONCAT31(extraout_var,uVar3));
          _local_20 = CONCAT22(uStack_1e,in_FPUControlWord);
          local_24 = (int)ROUND(*(float *)(*local_40 + 0x164));
          (*peVar2->pfnWriteShort)(local_24);
          (*peVar2->pfnWriteShort)(local_40[0x2cc]);
          (*peVar2->pfnWriteShort)(0);
          iVar8 = (**(code **)(**(int **)PTR_g_pGameRules_00177e70 + 0xd0))
                            (*(int **)PTR_g_pGameRules_00177e70,local_40 + 0x2d2);
          (*peVar2->pfnWriteShort)(iVar8 + 1);
          (*peVar2->pfnMessageEnd)();
          UTIL_LogPrintf__FPce("Player \"%s\" triggered \"last_man_standing\"\n");
        }
        ppgVar1 = PTR_gpGlobals_00177410;
        this->m_flRoundRestart = (float)dc_3 + (*PTR_gpGlobals_00177410)->time;
        peVar2 = PTR_g_engfuncs_001778b8;
        if ((char)this->field_0x10 != '\0') {
          (*PTR_g_engfuncs_001778b8->pfnMessageBegin)
                    (2,*(undefined4 *)PTR_gmsgTSRoundTime_001770d8,0,0);
          _local_20 = CONCAT22(uStack_1e,in_FPUControlWord);
          local_24 = (int)ROUND(this->m_flRoundEnd - (*ppgVar1)->time);
          (*peVar2->pfnWriteByte)(local_24);
          (*peVar2->pfnWriteByte)(2);
          (*peVar2->pfnMessageEnd)();
        }
      }
      else if ((((0 < iVar8) && ((*PTR_gpGlobals_00177410)->time <= this->m_flRoundEnd)) &&
               (1 < local_44)) || ((char)this->field_0x10 == '\0')) {
        pcVar6 = (char *)UTIL_VarArgs__FPce("%d men standing!");
        TSHudMessageAll__FUcUcUciiPc(0,0xff,0,1,0x1e,pcVar6);
      }
      else {
        TSHudMessageAll__FUcUcUciiPc(0,0xff,0,8,0x1e,"Round over");
        this->m_flRoundRestart = (float)dc_3 + (*PTR_gpGlobals_00177410)->time;
      }
    }
    else if ((byte)(*(char *)&this->pev - 2U) < 2) {
      iVar9 = 0;
      piVar4 = local_3c;
      for (iVar8 = 4; iVar8 != 0; iVar8 = iVar8 + -1) {
        *piVar4 = 0;
        piVar4 = piVar4 + 1;
      }
      for (a0 = (CTSGameRules *)0x1; (int)a0 <= (*PTR_gpGlobals_00177410)->maxClients;
          a0 = (CTSGameRules *)((int)&a0->pev + 1)) {
        in_stack_ffffffa4 = a0;
        pCVar7 = (CTSGameRules *)UTIL_PlayerByIndex__Fi((int)a0);
        if (((pCVar7 != (CTSGameRules *)0x0) &&
            (in_stack_ffffffa4 = pCVar7,
            iVar8 = (**(code **)(*(int *)&pCVar7->field_0x60 + 200))(pCVar7), iVar8 != 0)) &&
           ((in_stack_ffffffa4 = pCVar7,
            iVar8 = (**(code **)(*(int *)&pCVar7->field_0x60 + 0xb4))(pCVar7), iVar8 != 0 ||
            (fc_0 <= *(float *)&pCVar7->field_0x770)))) {
          pcVar6 = (char *)(**(code **)(*(int *)&pCVar7->field_0x60 + 0xd0))(pCVar7);
          in_stack_ffffffa4 = this;
          iVar8 = GetTeamIndex(this,pcVar6);
          if (-1 < iVar8) {
            local_3c[iVar8] = local_3c[iVar8] + 1;
            iVar9 = iVar9 + 1;
          }
        }
      }
      uVar11 = (ushort)((uint)in_stack_ffffffa4 >> 0x10);
      if (*(char *)&this->pev == '\x03') {
        if (0 < iVar9) {
          if (local_3c[1] < 1) {
            if (this->field_0x554 == 0) {
              return;
            }
            FireTargets__FPCcP11CBaseEntityT18USE_TYPEf
                      ((*PTR_gpGlobals_00177410)->pStringBase + *(int *)(this->field_0x554 + 0x9c),
                       this->field_0x554,this->field_0x554,3,0);
            return;
          }
          if (0 < local_3c[0]) {
            return;
          }
          if (this->field_0x554 == 0) {
            return;
          }
          FireTargets__FPCcP11CBaseEntityT18USE_TYPEf
                    ((*PTR_gpGlobals_00177410)->pStringBase + *(int *)(this->field_0x554 + 0x98),
                     this->field_0x554,this->field_0x554,3,0);
          return;
        }
        iVar8 = (uint)uVar11 << 0x10;
        uVar10 = 2;
      }
      else if (iVar9 < 1) {
        iVar8 = (uint)uVar11 << 0x10;
        uVar10 = 2;
      }
      else if (local_3c[1] < 1) {
        iVar8 = (uint)uVar11 << 0x10;
        uVar10 = 0;
      }
      else {
        if (0 < local_3c[0]) {
          return;
        }
        iVar8 = (uint)uVar11 << 0x10;
        uVar10 = 1;
      }
      TeamWin(this,uVar10,0,1,iVar8);
    }
  }
  return;
}


