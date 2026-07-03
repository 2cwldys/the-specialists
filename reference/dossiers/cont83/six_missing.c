// ===== FUN @ 00081be4 size=146 =====

/* WARNING: Unknown calling convention -- yet parameter storage is locked */
/* TS_gun.cpp */

void CTSGun::EmptySound(CTSGun *this)

{
  longdouble lVar1;
  longdouble lVar2;
  
  if (this->m_weaponType != 0x24) {
    (*PTR_g_engfuncs_001778b8->pfnPlaybackEvent)
              (1,this->m_pPlayer->pev->pContainingEntity,(uint)*(ushort *)&this->field_0x968,0,
               PTR_g_vecZero_00177698,PTR_g_vecZero_00177698,0,0,0,0,0,0);
    lVar1 = (longdouble)UTIL_WeaponTimeBase__Fv();
    lVar2 = (longdouble)dc_0_1;
    this->field_0xa0 = (int)(float)(lVar2 + lVar1);
    this->field_0x9c = (int)(float)(lVar2 + lVar1);
    lVar1 = (longdouble)UTIL_WeaponTimeBase__Fv();
    this->field_0xa8 = (int)(float)((longdouble)dc_0_5 + lVar1);
  }
  return;
}


// ===== FUN @ 00081288 size=28 =====

/* WARNING: Unknown calling convention -- yet parameter storage is locked */
/* TS_gun.cpp */

void CTSGun::StopReload(CTSGun *this)

{
  longdouble lVar1;
  
  if (((this->m_pActiveItem != (s_weapon_status *)0x0) &&
      (this->m_pActiveInfo != (s_weapon_info *)0x0)) &&
     (*(int *)&this->m_pActiveItem->field_0xc != 0)) {
    *(undefined4 *)&this->m_pActiveItem->field_0xc = 0;
    if (this->m_pActiveInfo->iFireType == '\x01') {
      this->m_pActiveItem->burstOrPhase = 0;
      this->m_pActiveItem->pumpToggle = '\0';
    }
    lVar1 = (longdouble)UTIL_WeaponTimeBase__Fv();
    this->field_0xa8 = (int)(float)lVar1;
    *(float *)&this->m_pPlayer->field_0x12c = (float)lVar1;
  }
  return;
}


// ===== FUN @ 00084a14 size=149 =====

/* WARNING: Unknown calling convention -- yet parameter storage is locked */
/* TS_gun.cpp */

undefined4 CTSGun::ReloadWeapon(CTSGun *this,int a0,int a1,float a2)

{
  int iVar1;
  int iVar2;
  longdouble lVar3;
  
  if ((this->m_pActiveItem != (s_weapon_status *)0x0) &&
     (0 < (int)(&this->m_ammoReserve)[(byte)this->m_pActiveInfo->iCaliber])) {
    iVar1 = a0 - this->m_pActiveItem->iClip;
    iVar2 = (&this->m_ammoReserve)[(byte)this->m_pActiveInfo->iCaliber];
    if (iVar1 < (int)(&this->m_ammoReserve)[(byte)this->m_pActiveInfo->iCaliber]) {
      iVar2 = iVar1;
    }
    if (0 < iVar2) {
      *(undefined4 *)&this->m_pActiveItem->field_0xc = 1;
      lVar3 = (longdouble)UTIL_WeaponTimeBase__Fv();
      *(float *)&this->m_pPlayer->field_0x12c = (float)(lVar3 + (longdouble)a2);
      return 1;
    }
  }
  return 0;
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


// ===== FUN @ 0006f69c size=2926 =====

/* WARNING: Unknown calling convention -- yet parameter storage is locked */
/* TS_combat.cpp
   cvars: dmgreport */

CBasePlayer *
CBasePlayer::ShootBulletsPlayer
          (CBasePlayer *this,byte a0,Vector a1,Vector a2,Vector a3,float a4,int a5,int a6,int a7)

{
  globalvars_t *pgVar1;
  float fVar2;
  Vector a1_00;
  Vector a2_00;
  Vector a0_00;
  Vector a0_01;
  Vector a3_00;
  Vector a1_01;
  Vector a1_02;
  undefined uVar3;
  undefined3 extraout_var;
  char *pcVar4;
  undefined3 extraout_var_00;
  int iVar5;
  int iVar6;
  Vector *pVVar7;
  float *pfVar8;
  uint a0_02;
  entvars_t **ppeVar9;
  CBasePlayer *pCVar10;
  float *pfVar11;
  entvars_t **ppeVar12;
  undefined4 *puVar13;
  undefined2 in_FPUControlWord;
  longdouble lVar14;
  longdouble extraout_ST0;
  longdouble lVar15;
  longdouble extraout_ST0_00;
  longdouble lVar16;
  undefined3 in_stack_00000009;
  int iVar17;
  int iVar18;
  undefined4 uVar19;
  vec_t in_stack_ffffff5c;
  float fVar20;
  int in_stack_ffffff64;
  vec_t in_stack_ffffff68;
  int in_stack_ffffff6c;
  byte local_75;
  entvars_t *local_6c;
  float local_68;
  float local_64;
  float local_5c;
  float local_58;
  float local_54;
  vec_t local_48;
  undefined2 local_44;
  undefined2 uStack_42;
  float local_40;
  entvars_t *local_3c;
  float local_38;
  float local_34;
  float local_2c [7];
  
  fVar20 = 6.39501e-40;
  if (__tmp_0_1136 == 0) {
    __tmp_0_1136 = 1;
  }
  if (__tmp_1_1141 == 0) {
    __tmp_1_1141 = 1;
  }
  if (__tmp_2_1143 == 0) {
    __tmp_2_1143 = 1;
  }
  if (__tmp_3_1145 == 0) {
    __tmp_3_1145 = 1;
  }
  if (__tmp_4_1147 == 0) {
    __tmp_4_1147 = 1;
  }
  if (__tmp_5_1149 == 0) {
    __tmp_5_1149 = 1;
  }
  iVar6 = 0;
  if ((*(int *)(PTR_TSGameRules_00177300 + 0x578) != 0) ||
     (lVar14 = (longdouble)(**(code **)(_a0[0x18] + 0x10))(_a0),
     lVar14 < (longdouble)fc_0_9900000095367432)) {
    iVar6 = 1;
  }
  pgVar1 = *PTR_gpGlobals_00177410;
  pVVar7 = &pgVar1->v_right;
  pfVar8 = &vecRight_1140;
  rbullet_1156 = iVar6;
  for (iVar5 = 3; iVar5 != 0; iVar5 = iVar5 + -1) {
    *pfVar8 = pVVar7->x;
    pVVar7 = (Vector *)&pVVar7->y;
    pfVar8 = pfVar8 + 1;
  }
  pVVar7 = &pgVar1->v_up;
  pfVar8 = &vecUp_1142;
  for (iVar6 = 3; iVar6 != 0; iVar6 = iVar6 + -1) {
    *pfVar8 = pVVar7->x;
    pVVar7 = (Vector *)&pVVar7->y;
    pfVar8 = pfVar8 + 1;
  }
  a_info_1134 = get_ammo_info__Fi((int)a3.x);
  if (rbullet_1156 == 0) {
    fVar20 = 6.39896e-40;
    ClearMultiDamage__Fv();
  }
  *(undefined4 *)(PTR_gMultiDamage_00177510 + 8) = 0x1002;
  damage_ammount_1152 = 0.0;
  damage_adjust_1154 = 1.0;
  if (a2.z == 2.8026e-45) {
    local_38 = fc_4096 * *(float *)((int)a1.z + 4);
    local_34 = fc_4096 * *(float *)((int)a1.z + 8);
    local_3c = (entvars_t *)(fc_4096 * *(float *)a1.z);
    local_2c[0] = *(float *)a1.y + fc_4096 * *(float *)a1.z;
    local_2c[1] = *(float *)((int)a1.y + 4) + local_38;
    local_2c[2] = *(float *)((int)a1.y + 8) + local_34;
    pfVar8 = local_2c;
    pfVar11 = (float *)&vecEnd_1148;
    for (iVar6 = 3; iVar6 != 0; iVar6 = iVar6 + -1) {
      *pfVar11 = *pfVar8;
      pfVar8 = pfVar8 + 1;
      pfVar11 = pfVar11 + 1;
    }
    UTIL_TraceLine_Expanded__FRC6VectorT015IGNORE_MONSTERSP7edict_sP11TraceResult
              (a1.y,&vecEnd_1148,0,*(undefined4 *)(*_a0 + 0x208),tr_1135);
    if ((float)tr_1135._16_4_ != (float)dc_1) {
      iVar6 = tr_1135._48_4_;
      if (tr_1135._48_4_ == 0) {
        uVar3 = (*PTR_g_engfuncs_001778b8->pfnPEntityOfEntOffset)(0);
        iVar6 = CONCAT31(extraout_var,uVar3);
      }
      pEntity_1155 = 0;
      if (iVar6 != 0) {
        pEntity_1155 = *(int *)(iVar6 + 0x7c);
      }
      local_3c = (entvars_t *)(*(float *)a1.y - (float)tr_1135._20_4_);
      local_38 = *(float *)((int)a1.y + 4) - (float)tr_1135._24_4_;
      local_34 = *(float *)((int)a1.y + 8) - (float)tr_1135._28_4_;
      local_40 = SQRT(local_34 * local_34 + local_38 * local_38 + (float)local_3c * (float)local_3c)
      ;
      distance_1153 = local_40 * fc_0_0033333334140479565;
      _local_44 = CONCAT22(uStack_42,in_FPUControlWord);
      local_48 = (vec_t)(int)ROUND(distance_1153 * fc_10);
      lVar14 = (longdouble)get_range_multipler__Fi((int)local_48);
      damage_ammount_1152 = (float)(lVar14 * (longdouble)a2.y);
      local_3c = *(entvars_t **)a1.z;
      local_38 = *(float *)((int)a1.z + 4);
      local_34 = *(float *)((int)a1.z + 8);
      (**(code **)(*(int *)(pEntity_1155 + 0x60) + 0x54))
                (pEntity_1155,*_a0,damage_ammount_1152,&local_3c,tr_1135,0x1002);
      if ((fc_0 != *(float *)PTR_g_flWeaponCheat_00177bcc) &&
         ((*PTR_g_engfuncs_001778b8->pfnCVarGetFloat)("dmgreport"), (longdouble)fc_1 < extraout_ST0)
         ) {
        iVar18 = 0;
        iVar17 = 0;
        iVar5 = 0;
        iVar6 = 0;
        pcVar4 = (char *)UTIL_VarArgs__FPce("damage(piercing):%f\n");
        UTIL_ClientPrintAll__FiPCcN41(3,pcVar4,iVar6,iVar5,iVar17,iVar18);
      }
    }
    local_3c = *(entvars_t **)a1.y;
    local_38 = *(float *)((int)a1.y + 4);
    local_34 = *(float *)((int)a1.y + 8);
    local_2c[0] = (float)tr_1135._20_4_;
    local_2c[1] = (float)tr_1135._24_4_;
    local_2c[2] = (float)tr_1135._28_4_;
    _local_44 = CONCAT22(uStack_42,in_FPUControlWord);
    local_48 = (vec_t)(int)ROUND((float)dc_0_015625 * (float)tr_1135._16_4_ * fc_4096);
    a0_00.y = (vec_t)local_2c;
    a0_00.x = (vec_t)&local_3c;
    a0_00.z = local_48;
    a1_01.y = (vec_t)in_stack_ffffff64;
    a1_01.x = fVar20;
    a1_01.z = in_stack_ffffff68;
    UTIL_BubbleTrail__FG6VectorT0i(a0_00,a1_01,in_stack_ffffff6c);
    ApplyMultiDamage__FP9entvars_sT0((entvars_s *)*_a0,(entvars_s *)*_a0);
    this->pev = *(entvars_t **)a2.x;
    this->field_0x4 = *(int *)((int)a2.x + 4);
    fVar20 = *(float *)((int)a2.x + 8);
  }
  else {
    local_75 = 1;
    if (a1.x._0_1_ != 0) {
      do {
        a0_02 = (uint)local_75 + (int)a3.y;
        lVar14 = (longdouble)UTIL_SharedRandomFloat__FUiff(a0_02,-3.1415927,3.1415927);
        lVar15 = (longdouble)
                 UTIL_SharedRandomFloat__FUiff(a0_02 + 1,0.0,*(float *)(a_info_1134 + 0x1c));
        lVar16 = (longdouble)fc_1 - lVar15 * lVar15 * lVar15;
        lVar15 = (longdouble)fcos((longdouble)(float)lVar14);
        x_1150 = (float)(lVar15 * lVar16);
        lVar14 = (longdouble)fsin((longdouble)(float)lVar14);
        y_1151 = (float)(lVar14 * lVar16);
        local_54 = x_1150 * *(float *)a2.x;
        local_5c = local_54 * vecRight_1140;
        local_58 = local_54 * DAT_0017813c;
        local_54 = local_54 * DAT_00178140;
        local_64 = y_1151 * *(float *)((int)a2.x + 4);
        local_6c = (entvars_t *)(local_64 * vecUp_1142);
        local_68 = local_64 * DAT_00178148;
        local_64 = local_64 * DAT_0017814c;
        ppeVar9 = &local_3c;
        ppeVar12 = &vecDir_1146;
        for (iVar6 = 3; iVar6 != 0; iVar6 = iVar6 + -1) {
          *ppeVar12 = *ppeVar9;
          ppeVar9 = ppeVar9 + 1;
          ppeVar12 = ppeVar12 + 1;
        }
        local_2c[2] = (float)*(int *)(a_info_1134 + 0x14);
        local_2c[0] = local_2c[2] * (float)vecDir_1146;
        local_2c[1] = local_2c[2] * DAT_00178160;
        local_2c[2] = local_2c[2] * DAT_00178164;
        local_3c = (entvars_t *)(*(float *)a1.y + local_2c[0]);
        local_38 = *(float *)((int)a1.y + 4) + local_2c[1];
        local_34 = *(float *)((int)a1.y + 8) + local_2c[2];
        ppeVar9 = &local_3c;
        puVar13 = &vecEnd_1148;
        for (iVar6 = 3; iVar6 != 0; iVar6 = iVar6 + -1) {
          *puVar13 = *ppeVar9;
          ppeVar9 = ppeVar9 + 1;
          puVar13 = puVar13 + 1;
        }
        if (rbullet_1156 == 0) {
          UTIL_TraceLine_Expanded__FRC6VectorT015IGNORE_MONSTERSP7edict_sP11TraceResult
                    (a1.y,&vecEnd_1148,0,*(undefined4 *)(*_a0 + 0x208),tr_1135);
          if ((float)tr_1135._16_4_ != (float)dc_1) {
            iVar6 = tr_1135._48_4_;
            if (tr_1135._48_4_ == 0) {
              uVar3 = (*PTR_g_engfuncs_001778b8->pfnPEntityOfEntOffset)(0);
              iVar6 = CONCAT31(extraout_var_00,uVar3);
            }
            pEntity_1155 = 0;
            if (iVar6 != 0) {
              pEntity_1155 = *(int *)(iVar6 + 0x7c);
            }
            local_3c = (entvars_t *)(*(float *)a1.y - (float)tr_1135._20_4_);
            local_38 = *(float *)((int)a1.y + 4) - (float)tr_1135._24_4_;
            local_34 = *(float *)((int)a1.y + 8) - (float)tr_1135._28_4_;
            local_40 = SQRT(local_34 * local_34 +
                            local_38 * local_38 + (float)local_3c * (float)local_3c);
            distance_1153 = local_40 / (fc_10 * (float)*(int *)(a_info_1134 + 0x20));
            _local_44 = CONCAT22(uStack_42,in_FPUControlWord);
            local_48 = (vec_t)(int)ROUND(distance_1153 * fc_10);
            lVar14 = (longdouble)get_range_multipler__Fi((int)local_48);
            damage_ammount_1152 = (float)(lVar14 * (longdouble)a2.y);
            if ((fc_0 < *(float *)(PTR_damagemult_00177294 + 0xc)) &&
               (fc_0 != *(float *)PTR_g_flWeaponCheat_00177bcc)) {
              damage_adjust_1154 = *(float *)(PTR_damagemult_00177294 + 0xc);
            }
            if ((a2.z == 0.0) || (a2.z != 1.4013e-45)) {
              iVar6 = *(int *)(pEntity_1155 + 0x60);
              uVar19 = 0x1002;
              iVar5 = *_a0;
            }
            else {
              iVar6 = *(int *)(pEntity_1155 + 0x60);
              uVar19 = 0x2001002;
              iVar5 = *_a0;
            }
            local_34 = DAT_00178164;
            local_38 = DAT_00178160;
            local_3c = vecDir_1146;
            (**(code **)(iVar6 + 0x54))
                      (pEntity_1155,iVar5,damage_ammount_1152 * damage_adjust_1154,&local_3c,tr_1135
                       ,uVar19);
            if ((fc_0 != *(float *)PTR_g_flWeaponCheat_00177bcc) &&
               ((*PTR_g_engfuncs_001778b8->pfnCVarGetFloat)("dmgreport"),
               (longdouble)fc_1 < extraout_ST0_00)) {
              iVar18 = 0;
              iVar17 = 0;
              iVar5 = 0;
              iVar6 = 0;
              pcVar4 = (char *)UTIL_VarArgs__FPce("damage:%f\n");
              UTIL_ClientPrintAll__FiPCcN41(3,pcVar4,iVar6,iVar5,iVar17,iVar18);
            }
            damage_ammount_1152 = damage_ammount_1152 * *(float *)(a_info_1134 + 0x18);
            iVar6 = (**(code **)(*(int *)(pEntity_1155 + 0x60) + 200))(pEntity_1155);
            if (iVar6 != 0) {
              damage_ammount_1152 = (float)dc_0_5 * damage_ammount_1152;
            }
          }
        }
        else {
          CBulletManager::FireBullet
                    (PTR_TSGameRules_00177300 + 0x1864,_a0,a1.y,&vecDir_1146,&a2.y,&a3,a2.z);
        }
        local_3c = *(entvars_t **)a1.y;
        local_38 = *(float *)((int)a1.y + 4);
        local_34 = *(float *)((int)a1.y + 8);
        local_2c[0] = (float)tr_1135._20_4_;
        local_2c[1] = (float)tr_1135._24_4_;
        local_2c[2] = (float)tr_1135._28_4_;
        _local_44 = CONCAT22(uStack_42,in_FPUControlWord);
        in_stack_ffffff5c =
             (vec_t)(int)ROUND((float)dc_0_015625 *
                               (float)*(int *)(a_info_1134 + 0x14) * (float)tr_1135._16_4_);
        a0_01.y = (vec_t)local_2c;
        a0_01.x = (vec_t)&local_3c;
        a0_01.z = in_stack_ffffff5c;
        a1_02.y = (vec_t)in_stack_ffffff64;
        a1_02.x = fVar20;
        a1_02.z = in_stack_ffffff68;
        local_48 = in_stack_ffffff5c;
        UTIL_BubbleTrail__FG6VectorT0i(a0_01,a1_02,in_stack_ffffff6c);
        local_75 = local_75 + 1;
      } while (local_75 <= a1.x._0_1_);
    }
    if (((rbullet_1156 == 0) &&
        (ApplyMultiDamage__FP9entvars_sT0((entvars_s *)*_a0,(entvars_s *)*_a0), 1 < (int)a2.z - 1U))
       && ((float)dc_5 < damage_ammount_1152)) {
      fVar2 = damage_ammount_1152 + damage_ammount_1152;
      if (fc_48 <= damage_ammount_1152 + damage_ammount_1152) {
        fVar2 = fc_48;
      }
      local_2c[0] = fVar2 * (float)vecDir_1146;
      local_2c[1] = fVar2 * DAT_00178160;
      local_2c[2] = fVar2 * DAT_00178164;
      local_3c = (entvars_t *)((float)tr_1135._20_4_ + fVar2 * (float)vecDir_1146);
      local_38 = (float)tr_1135._24_4_ + fVar2 * DAT_00178160;
      local_34 = (float)tr_1135._28_4_ + fVar2 * DAT_00178164;
      pCVar10 = (CBasePlayer *)&local_3c;
      pfVar8 = &vecSrcPierce_1144;
      for (iVar6 = 3; iVar6 != 0; iVar6 = iVar6 + -1) {
        *pfVar8 = (float)pCVar10->pev;
        pCVar10 = (CBasePlayer *)&pCVar10->field_0x4;
        pfVar8 = pfVar8 + 1;
      }
      iVar6 = UTIL_PointContents__FRC6Vector((Vector *)&vecSrcPierce_1144);
      if (iVar6 != -2) {
        local_2c[0] = vecSrcPierce_1144;
        local_2c[1] = (float)DAT_00178154;
        local_2c[2] = (float)DAT_00178158;
        local_6c = vecDir_1146;
        local_68 = DAT_00178160;
        local_64 = DAT_00178164;
        local_5c = x_1150 * *(float *)a2.x;
        local_58 = y_1151 * *(float *)((int)a2.x + 4);
        local_54 = fc_0;
        a1_00.y = (vec_t)local_2c;
        a1_00.x = 1.4013e-45;
        a1_00.z = (vec_t)&local_6c;
        a2_00.y = damage_ammount_1152 * damage_adjust_1154;
        a2_00.x = (vec_t)&local_5c;
        a2_00.z = 2.8026e-45;
        a3_00.y = a3.y;
        a3_00.x = a3.x;
        a3_00.z = in_stack_ffffff5c;
        ShootBulletsPlayer((CBasePlayer *)&local_3c,a0,a1_00,a2_00,a3_00,fVar20,in_stack_ffffff64,
                           (int)in_stack_ffffff68,in_stack_ffffff6c);
      }
    }
    fVar20 = fc_0;
    fVar2 = y_1151 * *(float *)((int)a2.x + 4);
    this->pev = (entvars_t *)(x_1150 * *(float *)a2.x);
    this->field_0x4 = (int)fVar2;
  }
  this->field_0x8 = (int)fVar20;
  return this;
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


