// ===== FUN @ 000ceee8 size=140 =====

/* WARNING: Unknown calling convention -- yet parameter storage is locked */
/* multiplay_gamerules.cpp */

undefined4 CHalfLifeMultiplay::ClientCommand(CHalfLifeMultiplay *this,CBasePlayer *a0,char *a1)

{
  char cVar1;
  undefined uVar2;
  int iVar3;
  undefined3 extraout_var;
  
  cVar1 = CVoiceGameMgr::ClientCommand((CVoiceGameMgr *)PTR_g_VoiceGameMgr_00177330,a0,a1);
  if (cVar1 == '\0') {
    iVar3 = strcmp(a1,"menuselect");
    if (iVar3 != 0) {
      return 0;
    }
    uVar2 = (*PTR_g_engfuncs_001778b8->pfnCmd_Argc)();
    if (1 < CONCAT31(extraout_var,uVar2)) {
      uVar2 = (*PTR_g_engfuncs_001778b8->pfnCmd_Argv)(1);
      __strtol_internal(uVar2,0,10,0);
    }
  }
  return 1;
}


// ===== FUN @ 000e43c8 size=140 =====

/* WARNING: Unknown calling convention -- yet parameter storage is locked */
/* teamplay_gamerules.cpp */

undefined4 CHalfLifeTeamplay::ClientCommand(CHalfLifeTeamplay *this,CBasePlayer *a0,char *a1)

{
  char cVar1;
  undefined uVar2;
  int iVar3;
  undefined3 extraout_var;
  
  cVar1 = CVoiceGameMgr::ClientCommand((CVoiceGameMgr *)PTR_g_VoiceGameMgr_00177330,a0,a1);
  if (cVar1 == '\0') {
    iVar3 = strcmp(a1,"menuselect");
    if (iVar3 != 0) {
      return 0;
    }
    uVar2 = (*PTR_g_engfuncs_001778b8->pfnCmd_Argc)();
    if (1 < CONCAT31(extraout_var,uVar2)) {
      uVar2 = (*PTR_g_engfuncs_001778b8->pfnCmd_Argv)(1);
      __strtol_internal(uVar2,0,10,0);
    }
  }
  return 1;
}


// ===== FUN @ 000e44ac size=297 =====

/* WARNING: Unknown calling convention -- yet parameter storage is locked */
/* teamplay_gamerules.cpp */

int * CHalfLifeTeamplay::SetDefaultPlayerTeam(CHalfLifeTeamplay *this,CBasePlayer *a0)

{
  enginefuncs_t *peVar1;
  undefined uVar2;
  char *__src;
  undefined3 extraout_var;
  undefined3 extraout_var_00;
  int iVar3;
  undefined1 *__src_00;
  
  peVar1 = PTR_g_engfuncs_001778b8;
  if ((byte)*PTR_TSGameRules_00177300 < 2) {
    if (*(int *)PTR_g_theOneMode_001770dc == 0) {
      uVar2 = (*PTR_g_engfuncs_001778b8->pfnGetInfoKeyBuffer)(a0->pev->pContainingEntity);
      uVar2 = (*peVar1->pfnInfoKeyValue)(CONCAT31(extraout_var,uVar2),"team");
      __src = (char *)CONCAT31(extraout_var_00,uVar2);
    }
    else if (a0->field_0x7b9 == '\0') {
      __src = PTR_TSGameRules_00177300 + 0xb4;
    }
    else {
      __src = PTR_TSGameRules_00177300 + 0x28;
    }
    strncpy((char *)&a0->field_0xb48,__src,0x10);
  }
  RecountTeams(this,true);
  if ((char)a0->field_0xb48 != '\0') {
    iVar3 = (*(code *)(this->pev->absmax).z)(this,&a0->field_0xb48);
    if ((iVar3 != 0) && (fc_0 == *(float *)(PTR_defaultteam_00177828 + 0xc))) goto LAB_000e45c9;
  }
  __src_00 = team_names;
  if (fc_0 == *(float *)(PTR_defaultteam_00177828 + 0xc)) {
    __src_00 = (undefined1 *)TeamWithFewestPlayers(this);
  }
  strncpy((char *)&a0->field_0xb48,__src_00,0x10);
LAB_000e45c9:
  return &a0->field_0xb48;
}


// ===== FUN @ 000e488c size=1156 =====

/* WARNING: Unknown calling convention -- yet parameter storage is locked */
/* teamplay_gamerules.cpp */

void CHalfLifeTeamplay::ChangePlayerTeam
               (CHalfLifeTeamplay *this,CBasePlayer *a0,char *a1,int a2,int a3)

{
  int *piVar1;
  entvars_t *peVar2;
  bool bVar3;
  undefined *this_00;
  enginefuncs_t *peVar4;
  undefined uVar5;
  undefined uVar6;
  undefined3 extraout_var;
  undefined3 extraout_var_00;
  undefined3 extraout_var_01;
  undefined3 extraout_var_02;
  undefined4 uVar8;
  undefined3 extraout_var_03;
  undefined3 extraout_var_04;
  undefined3 extraout_var_05;
  undefined3 extraout_var_06;
  entvars_s *a0_00;
  byte *pbVar9;
  int iVar10;
  byte *pbVar11;
  int iVar12;
  bool bVar13;
  bool bVar14;
  undefined2 in_FPUControlWord;
  size_t __n;
  vec_t local_4c;
  vec_t local_48;
  vec_t local_44;
  vec_t local_3c;
  vec_t local_38;
  vec_t local_34;
  int local_24;
  undefined2 local_20;
  undefined2 uStack_1e;
  undefined4 uVar7;
  
  uVar5 = (*PTR_g_engfuncs_001778b8->pfnIndexOfEdict)(a0->pev->pContainingEntity);
  uVar7 = CONCAT31(extraout_var,uVar5);
  if (a3 == 0) {
    iVar10 = 0x1000;
  }
  else {
    iVar10 = 0x2000;
  }
  if (a2 != 0) {
    this->field_0x20c = 1;
    this->field_0x210 = 1;
    uVar5 = (*PTR_g_engfuncs_001778b8->pfnPEntityOfEntIndex)(0);
    a0_00 = (entvars_s *)
            (CONCAT31(extraout_var_00,uVar5) + 0x80U & (CONCAT31(extraout_var_00,uVar5) == 0) - 1);
    (*a0->vtbl->TakeDamage)(a0,a0_00,SUB41(a0_00,0),900.0,iVar10);
    this->field_0x20c = 0;
    this->field_0x210 = 0;
  }
  if (*(int *)PTR_g_theOneMode_001770dc != 0) {
    if (a0->field_0x7b9 == '\0') {
      __n = 0x10;
      a1 = PTR_TSGameRules_00177300 + 0xb4;
    }
    else {
      __n = 0x10;
      a1 = PTR_TSGameRules_00177300 + 0x28;
    }
    goto LAB_000e49ff;
  }
  bVar3 = false;
  bVar13 = false;
  if (*a1 == '\0') {
LAB_000e49d6:
    bVar3 = true;
  }
  else {
    iVar10 = 1;
    bVar14 = false;
    pbVar9 = (byte *)a1;
    pbVar11 = &DAT_00106657;
    do {
      if (iVar10 == 0) break;
      iVar10 = iVar10 + -1;
      bVar13 = *pbVar9 < *pbVar11;
      bVar14 = *pbVar9 == *pbVar11;
      pbVar9 = pbVar9 + 1;
      pbVar11 = pbVar11 + 1;
    } while (bVar14);
    if (((!bVar13 && !bVar14) == bVar13) ||
       (iVar10 = (*(code *)(this->pev->absmax).z)(this,a1), iVar10 == 0)) goto LAB_000e49d6;
  }
  __n = 0x10;
  if (bVar3) {
    a1 = (char *)TeamWithFewestPlayers(this);
  }
LAB_000e49ff:
  strncpy((char *)&a0->field_0xb48,a1,__n);
  peVar4 = PTR_g_engfuncs_001778b8;
  if (*(int *)PTR_g_teamplay_00177f1c - 2U < 2) {
    uVar5 = (*PTR_g_engfuncs_001778b8->pfnGetInfoKeyBuffer)(a0->pev->pContainingEntity);
    uVar6 = (*peVar4->pfnIndexOfEdict)(a0->pev->pContainingEntity);
    this_00 = PTR_TSGameRules_00177300;
    uVar8 = CTSGameRules::GetPlayerModel((CTSGameRules *)PTR_TSGameRules_00177300,a0);
    (*peVar4->pfnSetClientKeyValue)
              (CONCAT31(extraout_var_02,uVar6),CONCAT31(extraout_var_01,uVar5),"model",uVar8);
    CTSGameRules::SendObjectiveList((CTSGameRules *)this_00,(CBaseEntity *)a0);
  }
  else if (this->field_0x214 == 0) {
    uVar5 = (*PTR_g_engfuncs_001778b8->pfnGetInfoKeyBuffer)(a0->pev->pContainingEntity);
    (*peVar4->pfnSetClientKeyValue)(uVar7,CONCAT31(extraout_var_05,uVar5),"model",&a0->field_0xb48);
  }
  else {
    uVar5 = (*PTR_g_engfuncs_001778b8->pfnGetInfoKeyBuffer)(a0->pev->pContainingEntity);
    uVar6 = (*peVar4->pfnIndexOfEdict)(a0->pev->pContainingEntity);
    uVar8 = CTSGameRules::GetPlayerModel((CTSGameRules *)PTR_TSGameRules_00177300,a0);
    (*peVar4->pfnSetClientKeyValue)
              (CONCAT31(extraout_var_04,uVar6),CONCAT31(extraout_var_03,uVar5),"model",uVar8);
  }
  peVar4 = PTR_g_engfuncs_001778b8;
  uVar5 = (*PTR_g_engfuncs_001778b8->pfnGetInfoKeyBuffer)(a0->pev->pContainingEntity);
  piVar1 = &a0->field_0xb48;
  (*peVar4->pfnSetClientKeyValue)(uVar7,CONCAT31(extraout_var_06,uVar5),"team",piVar1);
  RecountTeams(this,false);
  (*peVar4->pfnMessageBegin)(2,*(undefined4 *)PTR_gmsgTeamInfo_00177c0c,0,0);
  (*peVar4->pfnWriteByte)(uVar7);
  (*peVar4->pfnWriteString)(piVar1);
  (*peVar4->pfnMessageEnd)();
  (*peVar4->pfnMessageBegin)(2,*(undefined4 *)PTR_gmsgScoreInfo_00177108,0,0);
  (*peVar4->pfnWriteByte)(uVar7);
  _local_20 = CONCAT22(uStack_1e,in_FPUControlWord);
  local_24 = (int)ROUND(a0->pev->frags);
  (*peVar4->pfnWriteShort)(local_24);
  (*peVar4->pfnWriteShort)(a0->field_0xb30);
  (*peVar4->pfnWriteShort)(0);
  iVar10 = (**(code **)(**(int **)PTR_g_pGameRules_00177e70 + 0xd0))
                     (*(int **)PTR_g_pGameRules_00177e70,piVar1);
  (*peVar4->pfnWriteShort)(iVar10 + 1);
  (*peVar4->pfnMessageEnd)();
  if ((a2 == 0) && (*(int *)PTR_g_theOneMode_001770dc == 0)) {
    peVar2 = a0->pev;
    local_3c = (peVar2->origin).x;
    local_38 = (peVar2->origin).y;
    local_34 = (peVar2->origin).z;
    peVar2 = a0->pev;
    local_4c = (peVar2->v_angle).x;
    local_48 = (peVar2->v_angle).y;
    local_44 = (peVar2->v_angle).z;
    CBasePlayer::StartObserver(a0,&local_3c,&local_4c);
    SetPlayerRespawnTime__FP11CBasePlayerib(a0,5,true);
  }
  peVar4 = PTR_g_engfuncs_001778b8;
  (*PTR_g_engfuncs_001778b8->pfnMessageBegin)(2,*(undefined4 *)PTR_gmsgTeamNames_0017794c,0,0);
  (*peVar4->pfnWriteByte)(num_teams);
  iVar10 = 0;
  if (0 < num_teams) {
    iVar12 = 0;
    do {
      (*peVar4->pfnWriteString)(team_names + iVar12);
      iVar12 = iVar12 + 0x10;
      iVar10 = iVar10 + 1;
    } while (iVar10 < num_teams);
  }
  (*PTR_g_engfuncs_001778b8->pfnMessageEnd)();
  return;
}


// ===== FUN @ 0008b244 size=352 =====

/* WARNING: Unknown calling convention -- yet parameter storage is locked */
/* TS_multiplay_gamerules.cpp */

void CHalfLifeMultiplay::ClientUserInfoChanged(CHalfLifeMultiplay *this,CBasePlayer *a0,char *a1)

{
  enginefuncs_t *peVar1;
  undefined uVar2;
  char cVar3;
  undefined uVar4;
  undefined3 extraout_var;
  undefined3 extraout_var_00;
  char *__s1;
  int iVar5;
  undefined3 extraout_var_02;
  char local_41c [1036];
  undefined3 extraout_var_01;
  
  peVar1 = PTR_g_engfuncs_001778b8;
  uVar2 = (*PTR_g_engfuncs_001778b8->pfnIndexOfEdict)(a0->pev->pContainingEntity);
  cVar3 = PlayerIsCheating__FP11CBasePlayerPc(a0,a1);
  if (cVar3 == '\0') {
    uVar4 = (*PTR_g_engfuncs_001778b8->pfnInfoKeyValue)(a1,"model");
    __s1 = (char *)CONCAT31(extraout_var_01,uVar4);
    iVar5 = strcasecmp(__s1,(char *)&a0->field_0xb48);
    if (iVar5 != 0) {
      if ((fc_0 == *(float *)(PTR_defaultteam_00177828 + 0xc)) &&
         (iVar5 = (*(code *)(this->pev->size).x)(this,__s1), iVar5 != 0)) {
        return;
      }
      peVar1 = PTR_g_engfuncs_001778b8;
      uVar4 = (*PTR_g_engfuncs_001778b8->pfnGetInfoKeyBuffer)(a0->pev->pContainingEntity);
      (*peVar1->pfnSetClientKeyValue)
                (CONCAT31(extraout_var,uVar2),CONCAT31(extraout_var_02,uVar4),"model",
                 &a0->field_0xb48);
      sprintf(local_41c,"* Can\'t change team to \'%s\'\n",__s1);
      UTIL_SayText__FPCcP11CBaseEntity(local_41c,(CBaseEntity *)a0);
      sprintf(local_41c,"* HLDM models not allowed.\n");
      UTIL_SayText__FPCcP11CBaseEntity(local_41c,(CBaseEntity *)a0);
    }
  }
  else {
    uVar4 = (*peVar1->pfnGetInfoKeyBuffer)(a0->pev->pContainingEntity);
    (*peVar1->pfnSetClientKeyValue)
              (CONCAT31(extraout_var,uVar2),CONCAT31(extraout_var_00,uVar4),"cl_lw","1");
  }
  return;
}


