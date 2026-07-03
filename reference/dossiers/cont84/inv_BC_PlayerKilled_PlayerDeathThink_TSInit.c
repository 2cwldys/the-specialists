// ===== FUN @ 000895e4 size=315 =====

/* TS_multiplay_gamerules.cpp */

void CHalfLifeMultiplay::PlayerKilled(int *this,CBasePlayer *a0,entvars_s *a1,entvars_s *param_4)

{
  entvars_t *peVar1;
  bool bVar2;
  float fVar3;
  bool bVar4;
  bool bVar5;
  bool bVar6;
  bool bVar7;
  bool bVar8;
  bool bVar9;
  undefined *puVar10;
  enginefuncs_t *peVar11;
  undefined uVar12;
  char cVar13;
  undefined1 uVar14;
  undefined3 extraout_var;
  int iVar16;
  CBasePlayer *pCVar17;
  undefined3 extraout_var_00;
  char *pcVar18;
  undefined3 extraout_var_01;
  undefined3 extraout_var_02;
  uint uVar19;
  CBasePlayer *this_00;
  CTSKnife *this_01;
  int iVar20;
  byte *pbVar21;
  uint uVar22;
  byte *pbVar23;
  undefined4 *puVar24;
  char *pcVar25;
  int *piVar26;
  bool bVar27;
  bool bVar28;
  bool bVar29;
  undefined2 in_FPUControlWord;
  CBasePlayer *pCVar30;
  CBasePlayer *a2;
  int iVar31;
  int iVar32;
  int iStack_e0;
  int local_c4;
  CBasePlayer *local_bc;
  CBasePlayer *local_b8;
  bool local_aa;
  bool local_a9;
  int iStack_a8;
  undefined2 uStack_a4;
  undefined2 uStack_a2;
  entvars_t *peStack_9c;
  vec_t vStack_98;
  vec_t vStack_94;
  undefined1 uStack_85;
  char acStack_7c [32];
  undefined4 uStack_5c;
  void *apvStack_58 [18];
  edict_t *peVar15;
  
  bVar9 = false;
  bVar2 = false;
  bVar8 = false;
  local_aa = false;
  local_a9 = false;
  bVar7 = false;
  bVar6 = false;
  bVar4 = false;
  bVar5 = false;
  local_b8 = (CBasePlayer *)0x0;
  local_bc = (CBasePlayer *)0x0;
  peVar15 = a1->pContainingEntity;
  if (peVar15 == (edict_t *)0x0) {
    uVar12 = (*PTR_g_engfuncs_001778b8->pfnPEntityOfEntOffset)();
    peVar15 = (edict_t *)CONCAT31(extraout_var,uVar12);
  }
  this_00 = (CBasePlayer *)0x0;
  if (peVar15 != (edict_t *)0x0) {
    this_00 = *(CBasePlayer **)(peVar15 + 0x7c);
  }
  if ((this_00 != (CBasePlayer *)0x0) && (iVar16 = (*this_00->vtbl->Classify)(this_00), iVar16 == 2)
     ) {
    local_bc = this_00;
  }
  local_c4 = (**(code **)(*this + 0x6c))();
  if (((a0->pev != a1) && (this_00 != (CBasePlayer *)0x0)) &&
     (iVar16 = (*this_00->vtbl->IsPlayer)(this_00), iVar16 != 0)) {
    local_c4 = TSGetPointsForFrag__FP11CBasePlayerP9entvars_sT1iPbT4
                         (a0,a1,param_4,local_c4,&local_aa,&local_a9);
    if (((this_00->m_pActiveItem != (CBaseEntity *)0x0) &&
        (*(int *)&this_00->m_pActiveItem->field_0xcc != 0)) &&
       (pCVar17 = (CBasePlayer *)__opP11CBaseEntity__7EHANDLE(), pCVar17 == a0)) {
      cVar13 = GetWeaponType__F10tsweapon_t();
      if (((1 < (int)cVar13 - 2U) && (cVar13 != 0xb)) &&
         ((*PTR_glast_closecombat_00177730 == '\0' && (param_4 == a1)))) {
        bVar9 = true;
      }
    }
    bVar2 = (*PTR_gpGlobals_00177410)->time - (float)local_bc->field_0x748 < (float)dc_2;
    if ((2 < (byte)a0->field_0x744) &&
       (local_b8 = (CBasePlayer *)((*PTR_gpGlobals_00177410)->pStringBase + a0->pev->netname),
       8 < (byte)a0->field_0x744)) {
      bVar8 = true;
    }
    if ((char)local_bc->field_0x744 != -1) {
      *(char *)&local_bc->field_0x744 = (char)local_bc->field_0x744 + '\x01';
    }
    if ((byte)local_bc->field_0x744 < 10) {
      if ((char)local_bc->field_0x744 == '\t') {
        bVar5 = true;
      }
      else if ((char)local_bc->field_0x744 == '\x06') {
        bVar6 = true;
      }
      else if ((char)local_bc->field_0x744 == '\x03') {
        bVar7 = true;
      }
    }
    else {
      bVar4 = true;
    }
  }
  peVar11 = PTR_g_engfuncs_001778b8;
  uStack_5c = (entvars_t *)s_world_001087da._0_4_;
  apvStack_58[0] = (void *)CONCAT22(apvStack_58[0]._2_2_,s_world_001087da._4_2_);
  if ((a1->flags & 8) == 0) {
    if (param_4->targetname == 0) {
      pcVar18 = (*PTR_gpGlobals_00177410)->pStringBase + param_4->classname;
    }
    else {
      pcVar18 = (*PTR_gpGlobals_00177410)->pStringBase + param_4->targetname;
    }
LAB_00089a86:
    strcpy((char *)&uStack_5c,pcVar18);
  }
  else {
    (*PTR_g_engfuncs_001778b8->pfnIndexOfEdict)();
    if (param_4 != (entvars_s *)0x0) {
      if (param_4 == a1) {
        peVar15 = a1->pContainingEntity;
        if (peVar15 == (edict_t *)0x0) {
          uVar12 = (*peVar11->pfnPEntityOfEntOffset)();
          peVar15 = (edict_t *)CONCAT31(extraout_var_00,uVar12);
        }
        iVar16 = 0;
        if (peVar15 != (edict_t *)0x0) {
          iVar16 = *(int *)(peVar15 + 0x7c);
        }
        if (*(int *)PTR_glast_weaponid_00177d2c == -1) {
          if (*PTR_glast_closecombat_00177730 == '\0') {
            if (((*(int *)(iVar16 + 0xa10) != 0) &&
                (*(int *)(*(int *)(iVar16 + 0xa10) + 0x88) == 0x10)) &&
               (*(int *)(*(int *)(iVar16 + 0xa10) + 0xcc) != 0)) {
              if (*PTR_glast_coldcock_00177e64 == '\0') goto LAB_0008999b;
              iVar16 = get_weapon_info__F10tsweapon_t();
              strcpy((char *)&uStack_5c,(char *)(iVar16 + 0x44));
              strcat((char *)&uStack_5c," Stock");
            }
          }
          else {
            uStack_5c = (entvars_t *)s_Kung_Fu_001087f5._0_4_;
            apvStack_58[0] = (void *)s_Kung_Fu_001087f5._4_4_;
          }
          goto LAB_00089a97;
        }
LAB_0008999b:
        iVar16 = get_weapon_info__F10tsweapon_t();
        pcVar18 = (char *)(iVar16 + 0x44);
      }
      else {
        bVar27 = CARRY4((uint)(*PTR_gpGlobals_00177410)->pStringBase,param_4->classname);
        pbVar21 = (byte *)((*PTR_gpGlobals_00177410)->pStringBase + param_4->classname);
        bVar29 = pbVar21 == (byte *)0x0;
        iVar16 = 6;
        pbVar23 = &DAT_001077e5;
        do {
          if (iVar16 == 0) break;
          iVar16 = iVar16 + -1;
          bVar27 = *pbVar21 < *pbVar23;
          bVar29 = *pbVar21 == *pbVar23;
          pbVar21 = pbVar21 + 1;
          pbVar23 = pbVar23 + 1;
        } while (bVar29);
        if ((!bVar27 && !bVar29) == bVar27) {
          peVar15 = param_4->pContainingEntity;
          if (peVar15 == (edict_t *)0x0) {
            uVar12 = (*PTR_g_engfuncs_001778b8->pfnPEntityOfEntOffset)();
            peVar15 = (edict_t *)CONCAT31(extraout_var_01,uVar12);
          }
          this_01 = (CTSKnife *)0x0;
          if (peVar15 != (edict_t *)0x0) {
            this_01 = *(CTSKnife **)(peVar15 + 0x7c);
          }
          if (this_01 == (CTSKnife *)0x0) goto LAB_00089a97;
          pcVar18 = (char *)CTSKnife::GetDescription(this_01);
        }
        else {
          pcVar18 = (*PTR_gpGlobals_00177410)->pStringBase + param_4->classname;
        }
      }
      goto LAB_00089a86;
    }
  }
LAB_00089a97:
  iVar20 = 0;
  pCVar17 = (CBasePlayer *)0x7;
  iVar16 = strncmp((char *)&uStack_5c,"weapon_",7);
  if (iVar16 == 0) {
    iVar20 = 7;
  }
  else {
    pCVar17 = (CBasePlayer *)0x8;
    iVar16 = strncmp((char *)&uStack_5c,"monster_",8);
    if (iVar16 == 0) {
      iVar20 = 8;
    }
    else {
      pCVar17 = (CBasePlayer *)0x5;
      iVar16 = strncmp((char *)&uStack_5c,"func_",5);
      if (iVar16 == 0) {
        iVar20 = 5;
      }
    }
  }
  bVar27 = iVar20 != 0;
  if (bVar27) {
    strcpy((char *)&peStack_9c,(char *)&uStack_5c);
    strcpy((char *)&uStack_5c,(char *)((int)&peStack_9c + iVar20));
  }
  bVar27 = bVar27 && (undefined1 *)0xffffffef < &stack0xffffff04;
  bVar29 = iVar20 == 0 || &stack0x00000000 == (undefined1 *)0xec;
  iVar16 = 6;
  pbVar21 = (byte *)&uStack_5c;
  pbVar23 = &DAT_001077e5;
  do {
    if (iVar16 == 0) break;
    iVar16 = iVar16 + -1;
    bVar27 = *pbVar21 < *pbVar23;
    bVar29 = *pbVar21 == *pbVar23;
    pbVar21 = pbVar21 + 1;
    pbVar23 = pbVar23 + 1;
  } while (bVar29);
  bVar28 = (!bVar27 && !bVar29) < bVar27;
  bVar27 = (!bVar27 && !bVar29) == bVar27;
  if (bVar27) {
    pcVar18 = "Throwing Knife";
    puVar24 = &uStack_5c;
    for (iVar16 = 3; iVar16 != 0; iVar16 = iVar16 + -1) {
      *puVar24 = *(undefined4 *)pcVar18;
      pcVar18 = pcVar18 + 4;
      puVar24 = puVar24 + 1;
    }
    *(undefined2 *)puVar24 = *(undefined2 *)pcVar18;
    *(char *)((int)puVar24 + 2) = pcVar18[2];
  }
  else {
    iVar16 = 9;
    pbVar21 = (byte *)&uStack_5c;
    pbVar23 = (byte *)"shrapnel";
    do {
      if (iVar16 == 0) break;
      iVar16 = iVar16 + -1;
      bVar28 = *pbVar21 < *pbVar23;
      bVar27 = *pbVar21 == *pbVar23;
      pbVar21 = pbVar21 + 1;
      pbVar23 = pbVar23 + 1;
    } while (bVar27);
    bVar29 = (!bVar28 && !bVar27) < bVar28;
    bVar27 = (!bVar28 && !bVar27) == bVar28;
    if (bVar27) {
      pcVar18 = "Shrapnel";
      pcVar25 = (char *)&uStack_5c;
      for (iVar16 = 2; iVar16 != 0; iVar16 = iVar16 + -1) {
        *(undefined4 *)pcVar25 = *(undefined4 *)pcVar18;
        pcVar18 = pcVar18 + 4;
        pcVar25 = pcVar25 + 4;
      }
      *pcVar25 = *pcVar18;
    }
    else {
      iVar16 = 8;
      pbVar21 = (byte *)&uStack_5c;
      pbVar23 = (byte *)0x10c734;
      do {
        if (iVar16 == 0) break;
        iVar16 = iVar16 + -1;
        bVar29 = *pbVar21 < *pbVar23;
        bVar27 = *pbVar21 == *pbVar23;
        pbVar21 = pbVar21 + 1;
        pbVar23 = pbVar23 + 1;
      } while (bVar27);
      if ((!bVar29 && !bVar27) == bVar29) {
        pcVar18 = "M61 Grenade";
        puVar24 = &uStack_5c;
        for (iVar16 = 3; iVar16 != 0; iVar16 = iVar16 + -1) {
          *puVar24 = *(undefined4 *)pcVar18;
          pcVar18 = pcVar18 + 4;
          puVar24 = puVar24 + 1;
        }
      }
    }
  }
  (**(code **)(*this + 0x74))(this,a0,a1,param_4);
  peVar11 = PTR_g_engfuncs_001778b8;
  iVar16 = -1;
  pcVar18 = (char *)&uStack_5c;
  do {
    if (iVar16 == 0) break;
    iVar16 = iVar16 + -1;
    cVar13 = *pcVar18;
    pcVar18 = pcVar18 + 1;
  } while (cVar13 != '\0');
  uVar22 = 0;
  if (iVar16 != -2) {
    do {
      cVar13 = *(char *)((int)&uStack_5c + uVar22);
      if ((byte)(cVar13 + 0xbfU) < 0x1a) {
        *(char *)((int)&uStack_5c + uVar22) = cVar13 + ' ';
      }
      else if (*(char *)((int)apvStack_58 + (uVar22 - 4)) == ' ') {
        *(undefined1 *)((int)apvStack_58 + (uVar22 - 4)) = 0x5f;
      }
      uVar22 = uVar22 + 1;
      uVar19 = 0xffffffff;
      pcVar18 = (char *)&uStack_5c;
      do {
        if (uVar19 == 0) break;
        uVar19 = uVar19 - 1;
        cVar13 = *pcVar18;
        pcVar18 = pcVar18 + 1;
      } while (cVar13 != '\0');
    } while (uVar22 < ~uVar19 - 1);
  }
  if (a0->pev == a1) {
    if (*(int *)PTR_g_teamplay_00177f1c == 0) {
      (*PTR_g_engfuncs_001778b8->pfnGetPlayerUserId)();
      (*peVar11->pfnGetPlayerAuthId)();
      (*peVar11->pfnGetPlayerUserId)();
      pcVar18 = "\"%s<%i><%s><%i>\" committed suicide with \"%s\"\n";
    }
    else {
      (*PTR_g_engfuncs_001778b8->pfnGetInfoKeyBuffer)();
      (*peVar11->pfnInfoKeyValue)();
      (*peVar11->pfnGetPlayerAuthId)();
      (*peVar11->pfnGetPlayerUserId)();
      pcVar18 = "\"%s<%i><%s><%s>\" committed suicide with \"%s\"\n";
    }
LAB_0008a2b5:
    a2 = (CBasePlayer *)&uStack_5c;
    UTIL_LogPrintf__FPce(pcVar18);
  }
  else {
    if ((a1->flags & 8) == 0) {
      if (*(int *)PTR_g_teamplay_00177f1c == 0) {
        (*PTR_g_engfuncs_001778b8->pfnGetPlayerUserId)();
        (*peVar11->pfnGetPlayerAuthId)();
        (*peVar11->pfnGetPlayerUserId)();
        pcVar18 = "\"%s<%i><%s><%i>\" committed suicide with \"%s\" (world)\n";
      }
      else {
        (*PTR_g_engfuncs_001778b8->pfnGetInfoKeyBuffer)();
        (*peVar11->pfnInfoKeyValue)();
        (*peVar11->pfnGetPlayerAuthId)();
        (*peVar11->pfnGetPlayerUserId)();
        pcVar18 = "\"%s<%i><%s><%s>\" committed suicide with \"%s\" (world)\n";
      }
      goto LAB_0008a2b5;
    }
    if (local_b8 != (CBasePlayer *)0x0) {
      pCVar17 = local_b8;
      sprintf((char *)&peStack_9c," (stopped_fury \"%s\")");
    }
    peVar11 = PTR_g_engfuncs_001778b8;
    if (bVar4) {
      pcVar18 = " (is_unstoppable)";
      pcVar25 = acStack_7c;
      for (iVar16 = 4; iVar16 != 0; iVar16 = iVar16 + -1) {
        *(undefined4 *)pcVar25 = *(undefined4 *)pcVar18;
        pcVar18 = pcVar18 + 4;
        pcVar25 = pcVar25 + 4;
      }
      *(undefined2 *)pcVar25 = *(undefined2 *)pcVar18;
    }
    else {
      if (bVar5) {
        pcVar18 = " (is_specialist)";
LAB_00089e0d:
        iVar16 = 4;
        pcVar25 = acStack_7c;
      }
      else {
        if (bVar6) {
          pcVar18 = " (is_demolition)";
          goto LAB_00089e0d;
        }
        if (!bVar7) {
          acStack_7c[0] = '\0';
          goto LAB_00089e3d;
        }
        pcVar18 = " (is_killer)";
        pcVar25 = acStack_7c;
        iVar16 = 3;
      }
      for (; iVar16 != 0; iVar16 = iVar16 + -1) {
        *(undefined4 *)pcVar25 = *(undefined4 *)pcVar18;
        pcVar18 = pcVar18 + 4;
        pcVar25 = pcVar25 + 4;
      }
      *pcVar25 = *pcVar18;
    }
LAB_00089e3d:
    if (*(int *)PTR_g_teamplay_00177f1c == 0) {
      (*PTR_g_engfuncs_001778b8->pfnGetPlayerUserId)();
      (*peVar11->pfnGetPlayerAuthId)();
      (*peVar11->pfnGetPlayerUserId)();
      (*peVar11->pfnGetPlayerUserId)();
      (*peVar11->pfnGetPlayerAuthId)();
      (*peVar11->pfnGetPlayerUserId)();
      pcVar18 = "\"%s<%i><%s><%i>\" killed \"%s<%i><%s><%i>\" with \"%s\"%s%s%s%s%s%s%s%s\n";
    }
    else {
      (*PTR_g_engfuncs_001778b8->pfnGetInfoKeyBuffer)();
      (*peVar11->pfnInfoKeyValue)();
      (*peVar11->pfnGetPlayerAuthId)();
      (*peVar11->pfnGetPlayerUserId)();
      (*peVar11->pfnGetInfoKeyBuffer)();
      (*peVar11->pfnInfoKeyValue)();
      (*peVar11->pfnGetPlayerAuthId)();
      (*peVar11->pfnGetPlayerUserId)();
      pcVar18 = "\"%s<%i><%s><%s>\" killed \"%s<%i><%s><%s>\" with \"%s\"%s%s%s%s%s%s%s%s\n";
    }
    a2 = (CBasePlayer *)acStack_7c;
    UTIL_LogPrintf__FPce(pcVar18);
  }
  a0->field_0xb30 = a0->field_0xb30 + 1;
  FireTargets__FPCcP11CBaseEntityT18USE_TYPEf("game_playerdie",a0,a0,3);
  if (a0->field_0x7b9 != '\0') {
    a2 = a0;
    pCVar17 = local_bc;
    CTSGameRules::TheOneIsDead();
    *(undefined1 *)&a0->field_0x744 = 0;
  }
  if (a0->pev == a1) {
    if (8 < (byte)a0->field_0x744) {
      a1->frags = a1->frags - fc_5;
      pcVar18 = (char *)UTIL_VarArgs__FPce("%s, #TS_SpecialistSuicide");
      TSHudMessageAll__FUcUcUciiPc(0,0xff,0,1,0x1e,pcVar18);
    }
    a1->frags = a1->frags - fc_1;
    goto LAB_0008aef5;
  }
  if ((this_00 == (CBasePlayer *)0x0) ||
     (pCVar30 = this_00, iVar16 = (*this_00->vtbl->IsPlayer)(this_00), iVar16 == 0)) {
    a1->frags = a1->frags - fc_1;
    goto LAB_0008aef5;
  }
  if ((((a1->iuser4 & 0x10c10U) != 0) || (*PTR_glast_closecombat_00177730 != '\0')) ||
     (2 < local_c4)) {
    uVar14 = 0xe5;
    iVar16 = DoSpectacularNess__Fv();
    if (iVar16 != 0) {
      peVar1 = a0->pev;
      peStack_9c = (entvars_t *)(peVar1->origin).x;
      vStack_98 = (peVar1->origin).y;
      vStack_94 = (peVar1->origin).z;
      CTSSlowMotionPoint::CreateSlowMotionPoint
                ((CTSSlowMotionPoint *)&peStack_9c,(Vector)ZEXT812(0x3f8000003e800000),
                 (float)((uint)pCVar30 & 0xffff0000),(float)a2,(float)pCVar17,(bool)uVar14);
    }
  }
  if (0 < local_c4) {
    if (*PTR_glast_headshot_00177468 != '\0') {
      pcVar18 = (char *)UTIL_VarArgs__FPce("#TS_Headshot #TS_FrgFr %s!");
      TSHudMessageAll__FUcUcUciiPc(0,0xff,0,1,0x14,pcVar18);
    }
    if (((((param_4 == a1) && (local_bc != (CBasePlayer *)0x0)) &&
         (local_bc->m_pActiveItem != (CBaseEntity *)0x0)) &&
        ((*(int *)&local_bc->m_pActiveItem->field_0x88 == 0x10 &&
         (*(int *)&local_bc->m_pActiveItem->field_0xcc != 0)))) &&
       (*PTR_glast_closecombat_00177730 == '\0')) {
      iVar16 = get_weapon_info__F10tsweapon_t();
      if (*(char *)(iVar16 + 0x348) == '\x03') {
        pcVar18 = "#TS_KnifeKill #TS_FrgFr %s!";
      }
      else {
        iVar16 = get_weapon_info__F10tsweapon_t();
        if (*(char *)(iVar16 + 0x348) != '\n') goto LAB_0008a57e;
        pcVar18 = "#TS_KatanaKill #TS_FrgFr %s!";
      }
      pcVar18 = (char *)UTIL_VarArgs__FPce(pcVar18);
      TSHudMessageAll__FUcUcUciiPc(0,0xff,0,1,0x14,pcVar18);
    }
LAB_0008a57e:
    if (*PTR_glast_coldcock_00177e64 != '\0') {
      pcVar18 = (char *)UTIL_VarArgs__FPce("#TS_Coldcock #TS_FrgFr %s!");
      TSHudMessageAll__FUcUcUciiPc(0,0xff,0,1,0x14,pcVar18);
    }
    if (bVar2) {
      local_c4 = local_c4 << 1;
      pcVar18 = (char *)UTIL_VarArgs__FPce("#TS_Doublefrag #TS_FrgFr %s!");
      TSHudMessageAll__FUcUcUciiPc(0,0xff,0,1,0x14,pcVar18);
    }
    if (local_b8 != (CBasePlayer *)0x0) {
      if (bVar8) {
        local_c4 = local_c4 + 5;
        pcVar18 = (char *)UTIL_VarArgs__FPce("%i #TS_Frg #TS_FrgFr %s, #TS_KilledTheSpecialist");
        TSHudMessageAll__FUcUcUciiPc(0,0xff,0,1,0x1e,pcVar18);
        if ((local_bc->m_iPowerups == 0) && (local_bc->m_iHeldPowerup == 0)) {
          local_bc->m_iPowerups = 0;
          local_bc->m_iHeldPowerup = 8;
          local_bc->m_flPowerupExpireTime = 0.0;
          local_bc->m_flPowerupTime = 2;
        }
      }
      (*PTR_g_engfuncs_001778b8->pfnRandomLong)();
      sprintf((char *)&peStack_9c,"#TS_SpreeKilled%i");
      uStack_85 = 0;
      pcVar18 = (char *)UTIL_VarArgs__FPce("%s %s");
      TSHudMessageAll__FUcUcUciiPc(0,0xff,0,1,0x1e,pcVar18);
    }
    iVar16 = 0;
    iStack_e0 = 0;
    if (bVar4) {
      pcVar18 = (char *)UTIL_VarArgs__FPce("#TS_Unstoppable %s! %i #TS_ConsKills");
      TSHudMessageAll__FUcUcUciiPc(0,0xff,0,1,0x1e,pcVar18);
      local_c4 = local_c4 << 1;
    }
    else if (bVar5) {
      pcVar18 = (char *)UTIL_VarArgs__FPce("%s #TS_Specialist");
      TSHudMessageAll__FUcUcUciiPc(0,0xff,0,1,0x37,pcVar18);
      if (local_bc->pev->health < fc_150) {
        local_bc->pev->health = 150.0;
      }
      local_bc->field_0x74e = 1;
      *(undefined1 *)&local_bc->m_bLowGravity = 1;
      peVar11 = PTR_g_engfuncs_001778b8;
      (*PTR_g_engfuncs_001778b8->pfnSetPhysicsKeyValue)();
      (*peVar11->pfnMessageBegin)();
      (*peVar11->pfnWriteShort)();
      (*peVar11->pfnWriteByte)();
      (*peVar11->pfnMessageEnd)();
      (*peVar11->pfnMessageBegin)();
      (*peVar11->pfnWriteShort)();
      (*peVar11->pfnWriteByte)();
      (*peVar11->pfnMessageEnd)();
      if ((local_bc->m_iPowerups == 0) && (local_bc->m_iHeldPowerup == 0)) {
        local_bc->m_iPowerups = 0;
        local_bc->m_iHeldPowerup = 8;
        local_bc->m_flPowerupExpireTime = 0.0;
        local_bc->m_flPowerupTime = 2;
      }
      uVar14 = 10;
      if (local_bc->field_0x74c != '\n') {
        uVar14 = 4;
      }
      local_bc->field_0x74c = uVar14;
    }
    else if (bVar6) {
      fVar3 = fc_60 + local_bc->pev->health;
      if (fc_150 < fVar3) {
        fVar3 = fc_150;
      }
      local_bc->pev->health = fVar3;
      uVar14 = 10;
      if (local_bc->field_0x74c != '\n') {
        uVar14 = 3;
      }
      local_bc->field_0x74c = uVar14;
      iVar16 = 6;
      iStack_e0 = 0x2d;
    }
    else if (bVar7) {
      fVar3 = fc_30 + local_bc->pev->health;
      if (fc_150 < fVar3) {
        fVar3 = fc_150;
      }
      local_bc->pev->health = fVar3;
      uVar14 = 10;
      if (local_bc->field_0x74c != '\n') {
        uVar14 = 2;
      }
      local_bc->field_0x74c = uVar14;
      iVar16 = 3;
      iStack_e0 = 0x23;
    }
    if (iVar16 != 0) {
      (*PTR_g_engfuncs_001778b8->pfnRandomLong)();
      sprintf((char *)&peStack_9c,"#TS_KillSpree%i");
      uStack_85 = 0;
      pcVar18 = (char *)UTIL_VarArgs__FPce("%s %s %i #TS_ConsKills");
      TSHudMessageAll__FUcUcUciiPc(0,0xff,0,1,iStack_e0,pcVar18);
    }
    if (fc_100 < local_bc->pev->health) {
      local_bc->m_flHealthRegen = (float)dc_1 + (*PTR_gpGlobals_00177410)->time;
    }
    if (bVar2) {
      local_bc->field_0x748 = 0;
    }
    else {
      local_bc->field_0x748 = (int)(*PTR_gpGlobals_00177410)->time;
    }
    if (local_c4 == -1) {
      iVar16 = -1000;
    }
    else {
      iVar16 = local_c4 * 500 + 400;
    }
    CBasePlayer::GiveCash(local_bc,iVar16);
    CBasePlayer::GiveCash(a0,900);
  }
  if (1 < (byte)*PTR_TSGameRules_00177300) {
    CBasePlayer::GiveCash(local_bc,((local_c4 != -1) - 1 & 0xfffff830) + 1000);
    CBasePlayer::GiveCash(a0,1000);
  }
  if (bVar9) {
    pcVar18 = (char *)UTIL_VarArgs__FPce("%s #TS_Killed %s #TS_OwnWeapon");
    TSHudMessageAll__FUcUcUciiPc(0,0xff,0,1,0x1e,pcVar18);
  }
  a1->frags = (float)local_c4 + a1->frags;
  iVar16 = local_c4;
  pcVar18 = (char *)(*local_bc->vtbl->TeamID)(local_bc);
  puVar10 = PTR_TSGameRules_00177300;
  iVar20 = CTSGameRules::GetTeamIndex((CTSGameRules *)PTR_TSGameRules_00177300,pcVar18);
  CTSGameRules::UpdateTeamScores((CTSGameRules *)puVar10,iVar20,iVar16);
  FireTargets__FPCcP11CBaseEntityT18USE_TYPEf("game_playerkill",this_00,this_00,3);
  if ((((fc_1 == *(float *)(PTR_slowmatch_001772c8 + 0xc)) && (local_bc != (CBasePlayer *)0x0)) &&
      (0 < local_c4)) &&
     (((local_bc->m_iHeldPowerup == 0 || ((*(byte *)&local_bc->m_iHeldPowerup & 1) != 0)) &&
      (local_bc->m_iPowerups == 0)))) {
    local_bc->m_iPowerups = 0;
    local_bc->m_iHeldPowerup = 1;
    uVar22 = local_bc->m_flPowerupTime + 1;
    if (3 < (int)uVar22) {
      uVar22 = 3;
    }
    local_bc->m_flPowerupTime = uVar22;
  }
  if (local_c4 == -1) {
    if ((fc_0 == *(float *)&local_bc->field_0x7c4) ||
       (*(float *)&local_bc->field_0x7c4 < (*PTR_gpGlobals_00177410)->time)) {
      *(float *)&local_bc->field_0x7c4 = (float)dc_30 + (*PTR_gpGlobals_00177410)->time;
      local_bc->field_0x7c8 = 1;
    }
    else if ((*PTR_gpGlobals_00177410)->time < *(float *)&local_bc->field_0x7c4) {
      *(float *)&local_bc->field_0x7c4 = (float)dc_30 + (*PTR_gpGlobals_00177410)->time;
      local_bc->field_0x7c8 = local_bc->field_0x7c8 + '\x01';
    }
    puVar10 = PTR_banfortk_00177bb4;
    if ('\x02' < (char)local_bc->field_0x7c8) {
      if (*(float *)(PTR_banfortk_00177bb4 + 0xc) <= fc_0) {
        ClientPrint__FP9entvars_siPCcN42(a1,2,"You have been kicked for teamkilling...\n",0,0,0,0);
        peVar11 = PTR_g_engfuncs_001778b8;
        (*PTR_g_engfuncs_001778b8->pfnGetPlayerUserId)();
        UTIL_VarArgs__FPce("kick #%i\n");
        (*peVar11->pfnServerCommand)();
        iVar32 = 0;
        iVar31 = 0;
        iVar20 = 0;
        iVar16 = 0;
        pcVar18 = (char *)UTIL_VarArgs__FPce("%s kicked for teamkilling\n");
      }
      else {
        iVar32 = 0;
        iVar31 = 0;
        iVar20 = 0;
        iVar16 = 0;
        iStack_a8 = (int)ROUND(*(float *)(PTR_banfortk_00177bb4 + 0xc));
        pcVar18 = (char *)UTIL_VarArgs__FPce(
                                            "You have been banned for teamkilling (%i minutes)...\n"
                                            );
        ClientPrint__FP9entvars_siPCcN42(a1,2,pcVar18,iVar16,iVar20,iVar31,iVar32);
        peVar11 = PTR_g_engfuncs_001778b8;
        (*PTR_g_engfuncs_001778b8->pfnGetPlayerUserId)();
        iStack_a8 = (int)ROUND(*(float *)(puVar10 + 0xc));
        UTIL_VarArgs__FPce("banid %i #%i kick\n");
        (*peVar11->pfnServerCommand)();
        iVar32 = 0;
        iVar31 = 0;
        iVar20 = 0;
        iVar16 = 0;
        _uStack_a4 = CONCAT22(uStack_a2,in_FPUControlWord);
        iStack_a8 = (int)ROUND(*(float *)(puVar10 + 0xc));
        pcVar18 = (char *)UTIL_VarArgs__FPce("%s banned for teamkilling (%i minutes)\n");
      }
      UTIL_ClientPrintAll__FiPCcN41(3,pcVar18,iVar16,iVar20,iVar31,iVar32);
    }
  }
LAB_0008aef5:
  peVar11 = PTR_g_engfuncs_001778b8;
  (*PTR_g_engfuncs_001778b8->pfnMessageBegin)();
  (*peVar11->pfnIndexOfEdict)();
  (*peVar11->pfnWriteByte)();
  _uStack_a4 = CONCAT22(uStack_a2,in_FPUControlWord);
  iStack_a8 = (int)ROUND(a0->pev->frags);
  (*peVar11->pfnWriteShort)();
  (*peVar11->pfnWriteShort)();
  (*peVar11->pfnWriteShort)();
  (**(code **)(*this + 0xd0))(this,&a0->field_0xb48);
  (*peVar11->pfnWriteShort)();
  (*peVar11->pfnMessageEnd)();
  peVar15 = a1->pContainingEntity;
  if (peVar15 == (edict_t *)0x0) {
    uVar12 = (*peVar11->pfnPEntityOfEntOffset)();
    peVar15 = (edict_t *)CONCAT31(extraout_var_02,uVar12);
  }
  piVar26 = (int *)0x0;
  if (peVar15 != (edict_t *)0x0) {
    piVar26 = *(int **)(peVar15 + 0x7c);
  }
  if ((piVar26 != (int *)0x0) &&
     (iVar16 = (**(code **)(piVar26[0x18] + 0x4c))(), peVar11 = PTR_g_engfuncs_001778b8, iVar16 == 2
     )) {
    (*PTR_g_engfuncs_001778b8->pfnMessageBegin)();
    (*peVar11->pfnIndexOfEdict)();
    (*peVar11->pfnWriteByte)();
    _uStack_a4 = CONCAT22(uStack_a2,in_FPUControlWord);
    iStack_a8 = (int)ROUND(*(float *)(*piVar26 + 0x164));
    (*peVar11->pfnWriteShort)();
    (*peVar11->pfnWriteShort)();
    (*peVar11->pfnWriteShort)();
    (**(code **)(*this + 0xd0))(this,piVar26 + 0x2d2);
    (*peVar11->pfnWriteShort)();
    (*peVar11->pfnMessageEnd)();
    piVar26[0x2d1] = (int)(*PTR_gpGlobals_00177410)->time;
  }
  CTSGameRules::CallCheckDeads((CTSGameRules *)PTR_TSGameRules_00177300);
  return;
}


// ===== FUN @ 0008eaec size=611 =====

/* WARNING: Unknown calling convention -- yet parameter storage is locked */
/* TS_player.cpp */

void CBasePlayer::PlayerDeathThink(CBasePlayer *this)

{
  int *piVar1;
  float fVar2;
  float fVar3;
  float fVar4;
  float fVar5;
  entvars_t *peVar6;
  int iVar7;
  float *pfVar8;
  Vector *pVVar9;
  float local_4c;
  float local_48;
  float local_44;
  float local_3c;
  float local_38;
  float local_34;
  float local_20;
  
  peVar6 = this->pev;
  if ((peVar6->flags & 0x200) != 0) {
    fVar2 = (peVar6->velocity).x;
    fVar3 = (peVar6->velocity).y;
    fVar4 = (peVar6->velocity).z;
    local_20 = SQRT(fVar4 * fVar4 + fVar3 * fVar3 + fVar2 * fVar2);
    fVar2 = local_20 - fc_20;
    if (fc_0 < fVar2) {
      peVar6 = this->pev;
      fVar3 = (peVar6->velocity).x;
      fVar4 = (peVar6->velocity).y;
      fVar5 = (peVar6->velocity).z;
      local_20 = SQRT(fVar5 * fVar5 + fVar4 * fVar4 + fVar3 * fVar3);
      if (local_20 == fc_0) {
        local_44 = fc_1;
        local_4c = 0.0;
        local_48 = 0.0;
      }
      else {
        fVar3 = fc_1 / local_20;
        local_4c = fVar3 * (peVar6->velocity).x;
        local_48 = fVar3 * (peVar6->velocity).y;
        local_44 = fVar3 * (peVar6->velocity).z;
      }
      pfVar8 = &local_3c;
      local_3c = fVar2 * local_4c;
      local_38 = fVar2 * local_48;
      local_34 = fVar2 * local_44;
    }
    else {
      peVar6 = this->pev;
      pfVar8 = (float *)PTR_g_vecZero_00177698;
    }
    pVVar9 = &peVar6->velocity;
    for (iVar7 = 3; iVar7 != 0; iVar7 = iVar7 + -1) {
      pVVar9->x = *pfVar8;
      pfVar8 = pfVar8 + 1;
      pVVar9 = (Vector *)&pVVar9->y;
    }
  }
  iVar7 = HasWeapons(this);
  if (iVar7 != 0) {
    RemoveAllItems(this,1);
  }
  if (((this->pev->modelindex == 0) || (*(int *)&this->m_fSequenceFinished != 0)) ||
     (this->pev->deadflag != 1)) {
    peVar6 = this->pev;
    if ((peVar6->movetype != 0) && ((peVar6->flags & 0x200) != 0)) {
      peVar6->movetype = 0;
    }
    if (this->pev->deadflag == 1) {
      this->pev->deadflag = 2;
    }
    this->pev->framerate = fc_0;
    piVar1 = &this->pev->effects;
    *piVar1 = *piVar1 | 0x20;
    this->pev->framerate = 0.0;
    peVar6 = this->pev;
    if (peVar6->deadflag == 2) {
      peVar6->deadflag = 3;
    }
    else if ((((float)this->field_0x768 < (*PTR_gpGlobals_00177410)->time) ||
             ((peVar6->button & 0xffff7fffU) != 0)) && (this->pev->iuser1 == 0)) {
      TSCreateDeadCorpse__FP11CBasePlayerf(this,10.0);
      peVar6 = this->pev;
      local_4c = (peVar6->origin).x;
      local_48 = (peVar6->origin).y;
      local_44 = (peVar6->origin).z;
      peVar6 = this->pev;
      local_3c = (peVar6->v_angle).x;
      local_38 = (peVar6->v_angle).y;
      local_34 = (peVar6->v_angle).z;
      StartObserver(this,&local_4c,&local_3c);
    }
  }
  else {
    CBaseAnimating::StudioFrameAdvance((CBaseAnimating *)this,0.0);
  }
  return;
}


// ===== FUN @ 00092a4c size=1208 =====

/* WARNING: Unknown calling convention -- yet parameter storage is locked */
/* TS_player.cpp */

void CBasePlayer::TSInit(CBasePlayer *this)

{
  globalvars_t **ppgVar1;
  enginefuncs_t *peVar2;
  undefined uVar3;
  char *pcVar4;
  undefined3 extraout_var;
  undefined3 extraout_var_00;
  entvars_t *peVar5;
  int iVar6;
  int *piVar7;
  longdouble lVar8;
  Vector local_3c [3];
  
  CTSAnimation::Init((CTSAnimation *)&this->field_0x28c);
  CTSStunt::Init((CTSStunt *)&this->field_0x4f0,this);
  this->pev->fuser1 = 1.0;
  this->m_flSlowMotionGo = 1.0;
  this->m_flSlowMotionTarget = 1.0;
  this->m_flClientSlowAmt = -1.0;
  this->field_0x175 = 0;
  this->field_0x174 = 0xff;
  this->field_0x498 = 0;
  *(undefined1 *)&this->field_0x499 = 0xff;
  this->field_0x9e0 = -1;
  this->pev->iuser4 = 0;
  SetPlayerSpeed(this,330.0);
  this->field_0x548 = 0x51;
  this->field_0x54c = -1;
  this->field_0x544 = -1;
  this->field_0x534 = 0;
  ppgVar1 = PTR_gpGlobals_00177410;
  this->field_0x71c = (int)((float)dc_1 + (*PTR_gpGlobals_00177410)->time);
  lVar8 = (longdouble)UTIL_WeaponTimeBase__Fv();
  this->field_0x12c = (int)(float)((longdouble)dc_1 + lVar8);
  this->pev->renderamt = 125.0;
  this->pev->rendermode = 1;
  (this->pev->rendercolor).x = 0.0;
  (this->pev->rendercolor).y = 0.0;
  (this->pev->rendercolor).z = 0.0;
  this->pev->renderfx = 0xe;
  local_3c[0].x = fc_200;
  local_3c[0].z = DAT_001092ec;
  local_3c[0].y = local_3c[0].x;
  UTIL_ScreenFade__FP11CBaseEntityRC6Vectorffii
            ((CBaseEntity *)this,local_3c,0.2,(float)this->field_0x71c - (*ppgVar1)->time,0x19,0);
  this->field_0x164 = 0;
  *(undefined1 *)&this->field_0x720 = 0;
  this->m_iPowerups = 0;
  this->m_iHeldPowerup = 0;
  this->m_flPowerupExpireTime = 0.0;
  this->m_flPowerupTime = 0;
  this->m_iClientHeldDur = 0;
  this->m_iClientHeldType = 0xff;
  this->field_0x740 = 0;
  *(undefined1 *)&this->field_0x744 = 0;
  this->field_0x748 = 0;
  this->field_0x74c = 0;
  this->field_0x74d = 0xff;
  this->field_0x74e = 0;
  if (PTR_TSGameRules_00177300[0x5885] == '\0') {
    *(undefined1 *)&this->m_bLowGravity = 0;
    peVar5 = this->pev;
    pcVar4 = "0";
  }
  else {
    *(undefined1 *)&this->m_bLowGravity = 1;
    peVar5 = this->pev;
    pcVar4 = "1";
  }
  (*PTR_g_engfuncs_001778b8->pfnSetPhysicsKeyValue)(peVar5->pContainingEntity,&DAT_0010791d,pcVar4);
  *(undefined4 *)((int)&this->m_bLowGravity + 1) = 0;
  this->field_0x754 = 0;
  peVar2 = PTR_g_engfuncs_001778b8;
  if ((this->pev->iuser1 != 0) || (this->pev->iuser2 != 0)) {
    (*PTR_g_engfuncs_001778b8->pfnMessageBegin)(2,*(undefined4 *)PTR_gmsgSpectator_00177b94,0,0);
    uVar3 = (*peVar2->pfnIndexOfEdict)(this->pev->pContainingEntity);
    (*peVar2->pfnWriteByte)(CONCAT31(extraout_var,uVar3));
    (*peVar2->pfnWriteByte)(0);
    (*peVar2->pfnMessageEnd)();
    peVar5 = this->pev;
    peVar5->iuser2 = 0;
    peVar5->iuser1 = 0;
  }
  this->field_0x4dc = 0;
  this->field_0x4ec = 0;
  this->field_0x718 = 0;
  this->field_0x770 = 0;
  this->field_0x7bc = 0;
  this->field_0x788 = 0;
  this->field_0x78c = 0;
  this->field_0x78f = 0;
  this->field_0x78d = 0;
  this->field_0x790 = 0;
  this->field_0x78e = 0;
  this->field_0x791 = 0;
  this->field_0x794 = 0;
  this->field_0x774 = 0x42c80000;
  this->field_0x798 = 0;
  this->field_0x79c = 0;
  this->field_0x7a0 = 0;
  *(undefined1 *)&this->field_0x7a4 = 0;
  this->field_0x7a8 = 0;
  this->field_0x7ac = 0;
  this->field_0x7b0 = 0;
  this->field_0x7b4 = 0;
  if (this->field_0x7b9 != '\0') {
    PTR_TSGameRules_00177300[0x5878] = 0;
  }
  this->field_0x7b9 = 0;
  this->m_flHealthRegen = 0.0;
  piVar7 = &this->field_0x184;
  for (iVar6 = 0x21; iVar6 != 0; iVar6 = iVar6 + -1) {
    *piVar7 = 0;
    piVar7 = piVar7 + 1;
  }
  this->m_flSlowMotionScale = 0.0;
  this->m_flSlowMotionRamp = 0.0;
  this->m_flNextSlowFlow = 0.0;
  piVar7 = &this->field_0x208;
  for (iVar6 = 0x20; iVar6 != 0; iVar6 = iVar6 + -1) {
    *piVar7 = 0;
    piVar7 = piVar7 + 1;
  }
  *(undefined1 *)&this->field_0x288 = 0;
  this->field_0x77c = 0;
  this->field_0x780 = 0;
  this->field_0x784 = 0;
  this->m_flSpreadAccum = 0;
  this->field_0x7c0 = 0;
  piVar7 = &this->field_0x7d0;
  for (iVar6 = 7; iVar6 != 0; iVar6 = iVar6 + -1) {
    *piVar7 = 0;
    piVar7 = piVar7 + 1;
  }
  uVar3 = (*PTR_g_engfuncs_001778b8->pfnGetInfoKeyBuffer)(this->pev->pContainingEntity);
  PlayerIsCheating__FP11CBasePlayerPc(this,(char *)CONCAT31(extraout_var_00,uVar3));
  peVar5 = this->pev;
  peVar5->gaitsequence = 0;
  peVar5->sequence = 0;
  this->pev->frame = 0.0;
  CBaseAnimating::ResetSequenceInfo((CBaseAnimating *)this);
  this->field_0x84c = 0;
  return;
}


