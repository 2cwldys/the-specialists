// ===== FUN @ 000833f0 size=251 =====

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */
/* WARNING: Unknown calling convention -- yet parameter storage is locked */
/* TS_gun.cpp
   assets: weapons/katana/hit-body.wav */

void CTSGun::KatanaFire(CTSGun *this)

{
  int *piVar1;
  float fVar2;
  float fVar3;
  entvars_t *peVar4;
  globalvars_t *pgVar5;
  edict_s *a0;
  Vector a2;
  Vector a2_00;
  uint3 uVar6;
  undefined uVar7;
  char cVar8;
  undefined3 extraout_var;
  undefined3 extraout_var_00;
  undefined3 extraout_var_01;
  int iVar9;
  entvars_t **ppeVar10;
  Vector *pVVar11;
  float *pfVar12;
  CBaseEntity *pCVar13;
  bool bVar14;
  bool bVar15;
  longdouble lVar16;
  TraceResult *a3;
  float fVar17;
  undefined4 uVar18;
  undefined7 in_stack_ffffff24;
  char in_stack_ffffff2b;
  undefined2 in_stack_ffffff2c;
  TraceResult *a3_00;
  int in_stack_ffffff30;
  TraceResult *pTStack_cc;
  float fStack_c4;
  float fStack_c0;
  float fStack_bc;
  float fStack_b8;
  float fStack_b0;
  float fStack_ac;
  float fStack_a8;
  float fStack_a0;
  float fStack_9c;
  float fStack_98;
  float fStack_90;
  entvars_t *peStack_8c;
  CBaseEntity *pCStack_88;
  undefined1 auStack_80 [16];
  void *pvStack_70;
  int iStack_50;
  float fStack_40;
  entvars_t *local_3c;
  CBaseEntity *local_38;
  float fStack_34;
  float local_20;
  
  uVar18 = 0x833ff;
  if (this->m_pActiveItem != (s_weapon_status *)0x0) {
    UTIL_MakeVectors__FRC6Vector(&this->m_pPlayer->pev->v_angle);
    peVar4 = this->m_pPlayer->pev;
    bVar14 = (peVar4->iuser4 & 0x70c30U) != 0;
    uVar6 = CONCAT12(bVar14,in_stack_ffffff2c);
    a3_00 = (TraceResult *)(uint)uVar6;
    fVar17 = (peVar4->velocity).x;
    fVar2 = (peVar4->velocity).y;
    fVar3 = (peVar4->velocity).z;
    local_20 = SQRT(fVar3 * fVar3 + fVar2 * fVar2 + fVar17 * fVar17);
    if (local_20 <= fc_100) {
      bVar15 = (this->m_pPlayer->pev->button & 0x10) != 0;
    }
    else {
      pgVar5 = *PTR_gpGlobals_00177410;
      peVar4 = this->m_pPlayer->pev;
      fVar17 = (peVar4->velocity).x;
      fVar2 = (peVar4->velocity).y;
      fVar3 = (peVar4->velocity).z;
      local_20 = SQRT(fVar3 * fVar3 + fVar2 * fVar2 + fVar17 * fVar17);
      if (local_20 == fc_0) {
        fStack_34 = fc_1;
        local_3c = (entvars_t *)0x0;
        local_38 = (CBaseEntity *)0x0;
      }
      else {
        fStack_34 = fc_1 / local_20;
        local_3c = (entvars_t *)(fStack_34 * (peVar4->velocity).x);
        local_38 = (CBaseEntity *)(fStack_34 * (peVar4->velocity).y);
        fStack_34 = fStack_34 * (peVar4->velocity).z;
      }
      bVar15 = (pgVar5->v_forward).z * fStack_34 +
               (pgVar5->v_forward).y * (float)local_38 + (pgVar5->v_forward).x * (float)local_3c <=
               (float)dc_neg0_5;
    }
    pTStack_cc = (TraceResult *)0x32;
    if (bVar14) {
      in_stack_ffffff30 = 0x11;
      pTStack_cc = (TraceResult *)0x23;
      fStack_c0 = fc_1;
      fStack_c4 = -NAN;
    }
    else if (bVar15) {
      a3_00 = (TraceResult *)CONCAT13(1,uVar6);
      in_stack_ffffff30 = 0xf;
      pTStack_cc = (TraceResult *)0x37;
      fStack_c0 = fc_1;
      fStack_c4 = 4.90454e-44;
    }
    else {
      peVar4 = this->m_pPlayer->pev;
      if ((peVar4->flags & 0x200) == 0) {
        in_stack_ffffff30 = 0x11;
        fStack_c0 = fc_1_100000023841858;
        fStack_c4 = 5.04467e-44;
      }
      else if (((peVar4->button & 4) == 0) && ((this->pev->flags & 0x4000) == 0)) {
        fStack_c0 = fc_0_699999988079071;
        if (1 < (int)this->m_lastMeleeAnim - 1U) {
          uVar7 = (*PTR_g_engfuncs_001778b8->pfnRandomLong)(1,2);
          this->m_lastMeleeAnim = (vec_t)CONCAT31(extraout_var,uVar7);
        }
        if (this->m_lastMeleeAnim == 1.4013e-45) {
          in_stack_ffffff30 = 0x11;
          this->m_lastMeleeAnim = 2.8026e-45;
          fStack_c4 = 4.62428e-44;
        }
        else if (this->m_lastMeleeAnim == 2.8026e-45) {
          in_stack_ffffff30 = 0x10;
          this->m_lastMeleeAnim = 1.4013e-45;
          fStack_c4 = 4.76441e-44;
        }
      }
      else {
        fStack_c0 = fc_0_8999999761581421;
        a3_00 = (TraceResult *)CONCAT13(1,uVar6);
        in_stack_ffffff30 = 0x11;
        fStack_c4 = 5.1848e-44;
        piVar1 = &this->m_pPlayer->pev->iuser4;
        *piVar1 = *piVar1 | 4;
        piVar1 = &this->m_pPlayer->pev->iuser4;
        *piVar1 = *piVar1 | 1;
        local_3c = (entvars_t *)fc_0;
        local_38 = (CBaseEntity *)fc_0;
        fStack_34 = fc_16;
        ppeVar10 = &local_3c;
        pVVar11 = &this->m_pPlayer->pev->view_ofs;
        for (iVar9 = 3; iVar9 != 0; iVar9 = iVar9 + -1) {
          pVVar11->x = (vec_t)*ppeVar10;
          ppeVar10 = ppeVar10 + 1;
          pVVar11 = (Vector *)&pVVar11->y;
        }
        *(undefined4 *)&this->m_pPlayer->field_0x4f4 = 0x40000000;
      }
    }
    fVar17 = fStack_c4;
    CTSAnimation::Animate
              ((CTSAnimation *)&this->m_pPlayer->field_0x28c,(CBasePlayer *)this->m_pPlayer,
               (int)fStack_c4);
    CBasePlayer::GetMeleeMultiplier((CBasePlayer *)this->m_pPlayer);
    a2_00._4_7_ = in_stack_ffffff24;
    a2_00.x = (vec_t)uVar18;
    a2_00.z._3_1_ = in_stack_ffffff2b;
    (*this->m_pPlayer->vtbl[1].TraceAttack)
              ((CBaseEntity *)&peStack_8c,(entvars_s *)this->m_pPlayer,fVar17,a2_00,a3_00,
               in_stack_ffffff30);
    pCVar13 = (CBaseEntity *)&peStack_8c;
    pfVar12 = &fStack_40;
    for (iVar9 = 3; iVar9 != 0; iVar9 = iVar9 + -1) {
      *pfVar12 = (float)pCVar13->pev;
      pCVar13 = (CBaseEntity *)&pCVar13->m_pGoalEnt;
      pfVar12 = pfVar12 + 1;
    }
    fStack_90 = fStack_40;
    peStack_8c = local_3c;
    pCStack_88 = local_38;
    pgVar5 = *PTR_gpGlobals_00177410;
    fStack_b8 = (float)in_stack_ffffff30;
    fStack_c0 = fStack_b8 * (pgVar5->v_forward).x;
    fStack_bc = fStack_b8 * (pgVar5->v_forward).y;
    fStack_b8 = fStack_b8 * (pgVar5->v_forward).z;
    fStack_a8 = fc_1;
    if ((float)pTStack_cc < fc_0) {
      fStack_a8 = fc_neg1;
    }
    fStack_b0 = fStack_a8 * fStack_c0;
    fStack_ac = fStack_a8 * fStack_bc;
    fStack_a8 = fStack_a8 * fStack_b8;
    fStack_a0 = fStack_40 + fStack_b0;
    fStack_9c = (float)local_3c + fStack_ac;
    fStack_98 = (float)local_38 + fStack_a8;
    CBasePlayer::CheckHull
              (this->m_pPlayer,&fStack_90,&fStack_a0,auStack_80,
               (-(uint)((float)pTStack_cc < fc_0) & 0xfffffffe) + 3);
    pCVar13 = (CBaseEntity *)0x0;
    if ((float)pvStack_70 < (float)dc_1) {
      if (iStack_50 == 0) {
        uVar7 = (*PTR_g_engfuncs_001778b8->pfnPEntityOfEntOffset)(0);
        iStack_50 = CONCAT31(extraout_var_00,uVar7);
      }
      pCVar13 = (CBaseEntity *)0x0;
      if (iStack_50 != 0) {
        pCVar13 = *(CBaseEntity **)(iStack_50 + 0x7c);
      }
      iVar9 = (*pCVar13->vtbl->IsAlive)(pCVar13);
      if ((iVar9 == 0) || ((char)pCVar13->pev->effects < '\0')) {
        pCVar13 = (CBaseEntity *)0x0;
      }
      else {
        a3 = pTStack_cc;
        cVar8 = VictimKatanaHold__FP11CBasePlayerP11CBaseEntityf
                          ((CBasePlayer *)this->m_pPlayer,pCVar13,(float)pTStack_cc);
        if (cVar8 == '\0') {
          iVar9 = 0x83990;
          ClearMultiDamage__Fv();
          pgVar5 = *PTR_gpGlobals_00177410;
          fStack_c0 = (pgVar5->v_forward).x;
          fStack_bc = (pgVar5->v_forward).y;
          fStack_b8 = (pgVar5->v_forward).z;
          a2.y = (vec_t)auStack_80;
          a2.x = (vec_t)&fStack_c0;
          a2.z = 5.91908e-42;
          (*pCVar13->vtbl->TraceAttack)(pCVar13,this->m_pPlayer->pev,(float)pTStack_cc,a2,a3,iVar9);
          ApplyMultiDamage__FP9entvars_sT0(this->m_pPlayer->pev,this->m_pPlayer->pev);
          iVar9 = (*pCVar13->vtbl->IsPlayer)(pCVar13);
          if (iVar9 != 0) {
            this->m_pActiveItem->burstOrPhase = 1;
            this->pev->body = 1;
            a0 = pCVar13->pev->pContainingEntity;
            fVar17 = 0.5;
            uVar7 = (*PTR_g_engfuncs_001778b8->pfnRandomLong)(0,0x1f);
            EMIT_SOUND_DYN__FP7edict_siPCcffiif
                      (a0,4,"weapons/katana/hit-body.wav",1.0,0.8,0,
                       CONCAT31(extraout_var_01,uVar7) + 0x5f,fVar17);
          }
        }
      }
    }
    if (pCVar13 == (CBaseEntity *)0x0) {
      *(undefined4 *)&this->m_pPlayer->field_0x79c = 0x3e800000;
      *(TraceResult **)&this->m_pPlayer->field_0x7a0 = pTStack_cc;
      this->m_pPlayer->field_0x7a4 = 2;
    }
    if ((in_stack_ffffff2b != '\0') && ((((byte)this->m_pPlayer->pev->iuser4 ^ 1) & 1) != 0)) {
      *(float *)&this->m_pPlayer->field_0x4dc = fStack_c4 * fc_0_8999999761581421;
      piVar1 = &this->m_pPlayer->pev->iuser4;
      *piVar1 = *piVar1 | 1;
    }
    (*PTR_g_engfuncs_001778b8->pfnPlaybackEvent)
              (1,this->m_pPlayer->pev->pContainingEntity,(uint)*(ushort *)&this->field_0x974,0,
               PTR_g_vecZero_00177698,PTR_g_vecZero_00177698,0,0,a3_00,
               this->m_pActiveItem->burstOrPhase,0,0);
    lVar16 = (longdouble)UTIL_WeaponTimeBase__Fv();
    this->field_0xa0 = (int)(float)(lVar16 + (longdouble)fStack_c4);
    this->field_0x9c = (int)(float)(lVar16 + (longdouble)fStack_c4);
    lVar16 = (longdouble)UTIL_WeaponTimeBase__Fv();
    this->field_0xa8 = (int)(float)((longdouble)dc_3 + lVar16);
    *(float *)&this->m_pPlayer->field_0x4f8 = fStack_c4;
  }
  return;
}


