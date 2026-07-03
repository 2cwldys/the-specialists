void CTSGun::CCombatAttack(CTSGun *this)

{
  int *piVar1;
  float fVar2;
  float fVar3;
  float fVar4;
  float fVar5;
  entvars_t *peVar6;
  globalvars_t *pgVar7;
  float fVar8;
  Vector a2;
  byte bVar9;
  undefined uVar10;
  undefined3 extraout_var;
  undefined3 extraout_var_00;
  undefined3 extraout_var_01;
  int iVar11;
  undefined1 *puVar12;
  Vector *pVVar13;
  CBaseEntity *pCVar14;
  Vector *pVVar15;
  float *pfVar16;
  char cVar17;
  bool bVar18;
  longdouble lVar19;
  undefined4 in_stack_fffffebc;
  undefined2 uVar20;
  uint uVar21;
  CBaseEntity *pCVar22;
  float in_stack_fffffecc;
  undefined4 uVar23;
  undefined4 in_stack_fffffed4;
  char cVar24;
  undefined2 in_stack_fffffed8;
  undefined1 uVar25;
  TraceResult *a3;
  undefined3 in_stack_fffffee0;
  uint a4;
  uint local_114;
  float local_110;
  float local_10c;
  float local_108;
  int local_104;
  vec_t local_100;
  vec_t vStack_fc;
  vec_t vStack_f8;
  float local_f0;
  float local_ec;
  float local_e8;
  float local_e0;
  float local_dc;
  float local_d8;
  float local_d0;
  float local_cc;
  float fStack_c8;
  float local_c0;
  entvars_t *local_bc;
  CBaseEntity *local_b8;
  float local_b4;
  Vector local_ac;
  Vector local_9c [2];
  float local_80;
  float local_30;
  entvars_t *local_2c;
  CBaseEntity *local_28;
  
  fVar8 = fc_0;
  uVar20 = (undefined2)((uint)in_stack_fffffebc >> 0x10);
  puVar12 = &stack0xfffffed4;
  local_108 = fc_0_6000000238418579;
  local_110 = fc_0;
  a4 = CONCAT13(1,in_stack_fffffee0);
  peVar6 = this->m_pPlayer->pev;
  fVar2 = (peVar6->velocity).x;
  fVar3 = (peVar6->velocity).y;
  fVar4 = (peVar6->velocity).z;
  fVar2 = SQRT(fVar4 * fVar4 + fVar3 * fVar3 + fVar2 * fVar2);
  local_114 = 0;
  local_100 = 0.0;
  local_104 = 0;
  a3 = (TraceResult *)0x0;
  uVar25 = 0;
  pCVar22 = this->m_pPlayer;
  cVar17 = (pCVar22->pev->iuser4 & 0x50c30U) != 0;
  lVar19 = (longdouble)UTIL_WeaponTimeBase__Fv();
  if (lVar19 < (longdouble)*(float *)&pCVar22->field_0x12c) {
    return;
  }
  lVar19 = (longdouble)UTIL_WeaponTimeBase__Fv();
  if (lVar19 < (longdouble)(float)this->field_0x9c) {
    return;
  }
  uVar23 = 0x86147;
  lVar19 = (longdouble)UTIL_WeaponTimeBase__Fv();
  if (lVar19 < (longdouble)(float)this->field_0xa0) {
    return;
  }
  peVar6 = this->m_pPlayer->pev;
  local_9c[0].y = (vec_t)(peVar6->v_angle).y;
  local_9c[0].z = (peVar6->v_angle).z;
  local_9c[0].x = 0.0;
  if (fVar2 <= fc_100) {
    bVar18 = (this->m_pPlayer->pev->button & 0x10) != 0;
  }
  else {
    UTIL_MakeVectors__FRC6Vector(local_9c);
    pgVar7 = *PTR_gpGlobals_00177410;
    peVar6 = this->m_pPlayer->pev;
    fVar3 = (peVar6->velocity).x;
    fVar4 = (peVar6->velocity).y;
    fVar5 = (peVar6->velocity).z;
    local_80 = SQRT(fVar5 * fVar5 + fVar4 * fVar4 + fVar3 * fVar3);
    if (local_80 == fVar8) {
      local_ac.z = fc_1;
      local_ac.x = 0.0;
      local_ac.y = 0.0;
    }
    else {
      local_ac.z = fc_1 / local_80;
      local_ac.x = local_ac.z * (peVar6->velocity).x;
      local_ac.y = local_ac.z * (peVar6->velocity).y;
      local_ac.z = local_ac.z * (peVar6->velocity).z;
    }
    fVar8 = (pgVar7->v_forward).z * local_ac.z +
            (pgVar7->v_forward).y * local_ac.y + (pgVar7->v_forward).x * local_ac.x;
    bVar18 = fVar8 <= (float)dc_neg0_5;
  }
  bVar9 = bVar18 + 2;
  if (cVar17 != '\0') {
    local_100 = -NAN;
    a3 = (TraceResult *)0x4;
    local_108 = fc_0_550000011920929;
    local_110 = fc_15;
    goto LAB_000862f9;
  }
  peVar6 = this->m_pPlayer->pev;
  if ((peVar6->button & 1) == 0) {
    peVar6 = this->m_pPlayer->pev;
    uVar21 = peVar6->button;
    local_10c = local_110;
    if ((uVar21 & 0x800) == 0) goto LAB_000869a3;
    if (((peVar6->flags & 0x200) != 0) &&
       (((uVar21 & 4) != 0 || ((this->pev->flags & 0x4000) != 0)))) {
      local_110 = fc_35;
      local_104 = 2;
      uVar25 = 1;
      local_100 = 2.8026e-44;
      a3 = (TraceResult *)0x6;
      local_114 = 0x2d;
      local_108 = fc_1;
      local_10c = fc_0_3499999940395355;
      piVar1 = &this->m_pPlayer->pev->iuser4;
      *piVar1 = *piVar1 | 4;
      piVar1 = &this->m_pPlayer->pev->iuser4;
      *piVar1 = *piVar1 | 1;
      local_ac.x = fc_0;
      local_ac.y = fc_0;
      local_ac.z = fc_16;
      pVVar13 = &local_ac;
      pVVar15 = &this->m_pPlayer->pev->view_ofs;
      for (iVar11 = 3; iVar11 != 0; iVar11 = iVar11 + -1) {
        pVVar15->x = pVVar13->x;
        pVVar13 = (Vector *)&pVVar13->y;
        pVVar15 = (Vector *)&pVVar15->y;
      }
      *(undefined4 *)&this->m_pPlayer->field_0x4f4 = 0x40000000;
      bVar9 = 2;
      goto LAB_000869a3;
    }
    if (bVar9 == 3) {
      local_104 = 3;
      local_100 = 2.38221e-44;
      a3 = (TraceResult *)0x7;
      local_110 = fVar2 * fc_0_3030303120613098;
      if (fc_80 < local_110) {
        local_110 = fc_80;
      }
      if (local_110 < fc_15) {
        local_110 = fc_15;
      }
      local_114 = 0x37;
      local_108 = fc_1_350000023841858;
      if ((this->m_pPlayer->pev->flags & 0x200) != 0) {
        uVar25 = 1;
        local_108 = fc_1;
      }
      local_10c = fc_0_30000001192092896;
      goto LAB_000869a3;
    }
    local_104 = 2;
    peVar6 = this->m_pPlayer->pev;
    if (((((peVar6->flags & 0x200) == 0) && (fc_270 < fVar2)) && ((peVar6->button & 4) != 0)) &&
       ((float)dc_0_5 < fVar8)) {
      uVar25 = 0;
      this->m_kickCombo = 0;
      local_110 = fVar2 * fc_0_1538461595773697;
      if (fc_45 < local_110) {
        local_110 = fc_45;
      }
      if (local_110 < fc_15) {
        local_110 = fc_15;
      }
      local_114 = 0x2d;
      local_108 = fc_1;
      local_10c = fc_0_25;
      local_100 = 2.24208e-44;
      a3 = (TraceResult *)0x8;
      a4 = a4 & 0xffffff;
      piVar1 = &this->m_pPlayer->pev->iuser4;
      *piVar1 = *piVar1 | 4;
      local_ac.x = fc_0;
      local_ac.y = fc_0;
      local_ac.z = fc_16;
      pVVar13 = &local_ac;
      pVVar15 = &this->m_pPlayer->pev->view_ofs;
      for (iVar11 = 3; iVar11 != 0; iVar11 = iVar11 + -1) {
        pVVar15->x = pVVar13->x;
        pVVar13 = (Vector *)&pVVar13->y;
        pVVar15 = (Vector *)&pVVar15->y;
      }
      *(undefined4 *)&this->m_pPlayer->field_0x4f4 = 0x40000000;
      goto LAB_000869a3;
    }
    if (((this->m_pPlayer->pev->flags & 0x200) != 0) || (fVar2 <= fc_200)) {
      if (((this->m_pPlayer->field_0x815 & 8) != 0) || (2 < (int)this->m_kickCombo)) {
        uVar25 = 1;
        local_108 = fc_0_5;
        local_10c = fc_0_30000001192092896;
        this->m_kickCombo = 0;
        if (this->m_lastMeleeAnim == 2.10195e-44) {
switchD_000868eb_caseD_2:
          local_100 = 1.4013e-44;
        }
        else if (this->m_lastMeleeAnim == 1.4013e-44) {
switchD_000868eb_caseD_0:
          local_100 = 1.96182e-44;
        }
        else if (this->m_lastMeleeAnim == 1.96182e-44) {
switchD_000868eb_caseD_1:
          local_100 = 1.82169e-44;
        }
        else {
          if (this->m_lastMeleeAnim != 1.82169e-44) {
            uVar10 = (*PTR_g_engfuncs_001778b8->pfnRandomLong)(0,3);
            switch(CONCAT31(extraout_var_00,uVar10)) {
            case 0:
              goto switchD_000868eb_caseD_0;
            case 1:
              goto switchD_000868eb_caseD_1;
            case 2:
              goto switchD_000868eb_caseD_2;
            case 3:
              break;
            default:
              goto switchD_000868eb_default;
            }
          }
          local_100 = 2.10195e-44;
        }
switchD_000868eb_default:
        local_110 = fc_25;
        goto LAB_00086912;
      }
      uVar25 = 1;
      local_114 = 0x2d;
      this->m_kickCombo = this->m_kickCombo + 1;
      local_108 = fc_0_30000001192092896;
      local_10c = fc_0_20000000298023224;
      local_110 = fc_15;
      if (this->m_lastMeleeAnim == 1.54143e-44) {
LAB_0008698f:
        local_100 = 1.68156e-44;
      }
      else {
        if (this->m_lastMeleeAnim != 1.68156e-44) {
          uVar10 = (*PTR_g_engfuncs_001778b8->pfnRandomLong)(0,1);
          if (CONCAT31(extraout_var_01,uVar10) != 0) {
            if (CONCAT31(extraout_var_01,uVar10) == 1) goto LAB_0008698f;
            goto LAB_00086997;
          }
        }
        local_100 = 1.54143e-44;
      }
LAB_00086997:
      a3 = (TraceResult *)0x9;
      goto LAB_000869a3;
    }
    uVar25 = 0;
    this->m_kickCombo = 0;
    lVar19 = (longdouble)dc_0_1 * (longdouble)fVar2;
    if ((longdouble)dc_30 < lVar19) {
      lVar19 = (longdouble)dc_30;
    }
    if (lVar19 < (longdouble)dc_15) {
      lVar19 = (longdouble)dc_15;
    }
    local_110 = (float)lVar19;
    local_108 = fc_0_75;
    local_10c = fc_0_25;
    if (this->m_lastMeleeAnim == 2.10195e-44) {
LAB_000867c4:
      local_100 = 2.66247e-44;
      goto LAB_00086912;
    }
    if (this->m_lastMeleeAnim == 2.66247e-44) {
LAB_000867dd:
      local_100 = 2.52234e-44;
    }
    else if (this->m_lastMeleeAnim == 2.52234e-44) {
LAB_00086829:
      local_100 = 2.10195e-44;
    }
    else {
      uVar10 = (*PTR_g_engfuncs_001778b8->pfnRandomLong)(0,2);
      iVar11 = CONCAT31(extraout_var,uVar10);
      if (iVar11 == 1) goto LAB_000867dd;
      if (iVar11 < 2) {
        if (iVar11 == 0) goto LAB_000867c4;
      }
      else if (iVar11 == 2) goto LAB_00086829;
    }
LAB_00086912:
    local_114 = 0x2d;
    a3 = (TraceResult *)0xa;
    goto LAB_000869a3;
  }
  if ((peVar6->flags & 0x200) == 0) {
    local_100 = 3.36312e-44;
    a3 = (TraceResult *)0x5;
    local_108 = fc_0_5;
    local_110 = fc_25;
LAB_000862f9:
    local_10c = fc_0_15000000596046448;
    this->m_comboStep = 0;
  }
  else {
    if ((2 < this->m_comboStep) ||
       ((peVar6 = this->m_pPlayer->pev, (peVar6->button & 4) != 0 && ((peVar6->flags & 0x4000) == 0)
        ))) {
      local_100 = 3.50325e-44;
      a3 = (TraceResult *)0xb;
      local_108 = fc_0_6499999761581421;
      local_110 = fc_30;
      goto LAB_000862f9;
    }
    if ((((this->m_pPlayer->field_0x814 ^ 1) & 1) == 0) || (2 < this->m_comboStep)) {
      local_108 = fc_0_3499999940395355;
      local_10c = fc_0_15000000596046448;
      this->m_comboStep = 0;
      local_100 = 3.22299e-44;
      a3 = (TraceResult *)0x4;
      local_110 = fc_20;
    }
    else {
      this->m_comboStep = this->m_comboStep + 1;
      local_100 = 3.08286e-44;
      a3 = (TraceResult *)0x3;
      local_108 = fc_0_25;
      local_10c = fc_0_10000000149011612;
      local_110 = fc_10;
    }
  }
  local_104 = 1;
  bVar9 = 2;
  local_114 = 0x23;
LAB_000869a3:
  if (local_104 != 0) {
    if ((char)(a4 >> 0x18) != '\0') {
      peVar6 = this->m_pPlayer->pev;
      uVar21 = peVar6->iuser4;
      if ((uVar21 & 0x20000) != 0) {
        peVar6->iuser4 = uVar21 & 0xfffdffff;
        piVar1 = &this->m_pPlayer->pev->iuser4;
        *piVar1 = *piVar1 & 0xfffffffb;
        peVar6 = this->m_pPlayer->pev;
        if ((peVar6->flags & 0x4000) == 0) {
          peVar6 = this->m_pPlayer->pev;
          local_ac.x = fc_0;
          local_ac.y = fc_0;
          local_ac.z = fc_28;
        }
        else {
          local_ac.x = fc_0;
          local_ac.y = fc_0;
          local_ac.z = fc_16;
        }
        local_ac.y = fc_0;
        pVVar13 = &local_ac;
        pVVar15 = &peVar6->view_ofs;
        for (iVar11 = 3; iVar11 != 0; iVar11 = iVar11 + -1) {
          pVVar15->x = pVVar13->x;
          pVVar13 = (Vector *)&pVVar13->y;
          pVVar15 = (Vector *)&pVVar15->y;
        }
        this->m_pPlayer->pev->friction = 1.0;
        this->m_pPlayer->pev->gravity = 1.0;
        local_ac.x = local_ac.y;
      }
    }
    this->m_lastMeleeAnim = local_100;
    lVar19 = (longdouble)CBasePlayer::GetMeleeMultiplier((CBasePlayer *)this->m_pPlayer);
    local_110 = (float)(lVar19 * (longdouble)local_110);
    iVar11 = KungFu__FP11CBasePlayer((CBasePlayer *)this->m_pPlayer);
    if (iVar11 != 0) {
      local_110 = local_110 + local_110;
    }
    if (bVar9 == 3) {
      pVVar13 = &this->m_pPlayer->pev->v_angle;
      pVVar15 = &local_ac;
      for (iVar11 = 3; iVar11 != 0; iVar11 = iVar11 + -1) {
        pVVar15->x = pVVar13->x;
        pVVar13 = (Vector *)&pVVar13->y;
        pVVar15 = (Vector *)&pVVar15->y;
      }
      local_ac.z = 0.0;
    }
    else {
      pVVar13 = &this->m_pPlayer->pev->v_angle;
      pVVar15 = &local_ac;
      for (iVar11 = 3; iVar11 != 0; iVar11 = iVar11 + -1) {
        pVVar15->x = pVVar13->x;
        pVVar13 = (Vector *)&pVVar13->y;
        pVVar15 = (Vector *)&pVVar15->y;
      }
    }
    local_80 = SQRT(local_ac.z * local_ac.z + local_ac.y * local_ac.y + local_ac.x * local_ac.x);
    if (local_80 == fc_0) {
      local_b4 = fc_1;
      local_bc = (entvars_t *)0x0;
      local_b8 = (CBaseEntity *)0x0;
    }
    else {
      local_b4 = fc_1 / local_80;
      local_bc = (entvars_t *)(local_b4 * local_ac.x);
      local_b8 = (CBaseEntity *)(local_b4 * local_ac.y);
      local_b4 = local_b4 * local_ac.z;
    }
    UTIL_MakeVectors__FRC6Vector(&local_ac);
    pCVar22 = this->m_pPlayer;
    uVar21 = 0x86bcb;
    a2.y = (vec_t)in_stack_fffffed4;
    a2.x = (vec_t)uVar23;
    a2.z._0_2_ = in_stack_fffffed8;
    a2.z._2_1_ = cVar17;
    a2.z._3_1_ = uVar25;
    (*pCVar22->vtbl[1].TraceAttack)
              ((CBaseEntity *)&local_bc,(entvars_s *)pCVar22,in_stack_fffffecc,a2,a3,a4);
    cVar24 = (char)((uint)in_stack_fffffed4 >> 0x18);
    pCVar14 = (CBaseEntity *)&local_bc;
    pfVar16 = &local_30;
    for (iVar11 = 3; iVar11 != 0; iVar11 = iVar11 + -1) {
      *pfVar16 = (float)pCVar14->pev;
      pCVar14 = (CBaseEntity *)&pCVar14->m_pGoalEnt;
      pfVar16 = pfVar16 + 1;
    }
    local_c0 = local_30;
    local_bc = local_2c;
    local_b8 = local_28;
    pgVar7 = *PTR_gpGlobals_00177410;
    local_e8 = (float)bVar9;
    local_f0 = local_e8 * (pgVar7->v_forward).x;
    local_ec = local_e8 * (pgVar7->v_forward).y;
    local_e8 = local_e8 * (pgVar7->v_forward).z;
    local_d8 = fc_1;
    if (fVar2 == 4.2039e-45) {
      local_d8 = fc_neg1;
    }
    local_e0 = local_d8 * local_f0;
    local_dc = local_d8 * local_ec;
    local_d8 = local_d8 * local_e8;
    local_d0 = local_30 + local_e0;
    local_cc = (float)local_2c + local_dc;
    fStack_c8 = (float)local_28 + local_d8;
    pgVar7 = *PTR_gpGlobals_00177410;
    local_100 = (pgVar7->v_forward).x;
    vStack_fc = (pgVar7->v_forward).y;
    vStack_f8 = (pgVar7->v_forward).z;
    iVar11 = CloseCombatAttack__11CBasePlayerG6VectorN21ifibN37
                       (this->m_pPlayer,&local_c0,&local_d0,&local_100,
                        (fVar2 != 4.2039e-45) * '\x02' + '\x01',local_114,local_104,
                        CONCAT22(uVar20,1),uVar21 & 0xffff0000,1,(uint)pCVar22 & 0xffff0000);
    if ((iVar11 == 0) && (fc_0_0010000000474974513 < local_110)) {
      *(float *)&this->m_pPlayer->field_0x79c = local_110;
      if (fVar2 == 4.2039e-45) {
        *(uint *)&this->m_pPlayer->field_0x7a0 = local_114 ^ 0x80000000;
      }
      else {
        *(uint *)&this->m_pPlayer->field_0x7a0 = local_114;
      }
      this->m_pPlayer->field_0x7a4 = 0;
    }
    if ((cVar24 != '\0') && ((((byte)this->m_pPlayer->pev->iuser4 ^ 1) & 1) != 0)) {
      CBasePlayer::BlockPlayer((CBasePlayer *)this->m_pPlayer,local_10c * fc_0_75);
    }
    (*PTR_g_engfuncs_001778b8->pfnPlaybackEvent)
              (1,this->m_pPlayer->pev->pContainingEntity,(uint)*(ushort *)&this->field_0x96c,0,
               PTR_g_vecZero_00177698,PTR_g_vecZero_00177698,0,0,local_108,
               CONCAT13(uVar25,CONCAT12(cVar17,in_stack_fffffed8)),0,0);
    puVar12 = &stack0xfffffed0;
  }
  *(undefined4 *)(puVar12 + -4) = 0x86dde;
  lVar19 = (longdouble)UTIL_WeaponTimeBase__Fv();
  fVar2 = *(float *)(puVar12 + 0x24);
  this->field_0xa0 = (int)(float)(lVar19 + (longdouble)fVar2);
  this->field_0x9c = (int)(float)(lVar19 + (longdouble)fVar2);
  *(undefined4 *)(puVar12 + -4) = 0x86df3;
  lVar19 = (longdouble)UTIL_WeaponTimeBase__Fv();
  this->field_0xa8 = (int)(float)(lVar19 + (longdouble)fc_2);
  *(undefined4 *)&this->m_pPlayer->field_0x7a8 = 0;
  *(undefined4 *)&this->m_pPlayer->field_0x4f8 = *(undefined4 *)(puVar12 + 0x24);
  return;
}


