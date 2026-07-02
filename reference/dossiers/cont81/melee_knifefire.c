// ===== FUN @ 00084044 size=398 =====

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */
/* WARNING: Unknown calling convention -- yet parameter storage is locked */
/* TS_gun.cpp
   assets: weapons/knife/knife_hitbody.wav, weapons/knife/knife_miss.wav */

void CTSGun::KnifeFire(CTSGun *this)

{
  undefined1 *puVar1;
  globalvars_t *pgVar2;
  edict_s *peVar3;
  entvars_t *peVar4;
  CTSGun_vtbl *pCVar5;
  CTSGun__SendWeaponAnim_372 *pCVar6;
  entvars_t *peVar7;
  engine_fn *peVar8;
  Vector a2;
  Vector a2_00;
  globalvars_t **ppgVar9;
  undefined uVar10;
  char cVar11;
  undefined3 extraout_var;
  undefined3 extraout_var_00;
  undefined3 extraout_var_01;
  int *piVar12;
  undefined3 extraout_var_02;
  int iVar13;
  int iVar14;
  undefined1 *puVar15;
  TraceResult *pTVar16;
  void **ppvVar17;
  Vector *pVVar18;
  float *pfVar19;
  CBaseEntity *this_00;
  entvars_t **ppeVar20;
  float *pfVar21;
  bool bVar22;
  longdouble lVar23;
  float in_stack_fffffeac;
  float fVar24;
  vec_t vVar25;
  undefined8 in_stack_fffffeb4;
  TraceResult *in_stack_fffffebc;
  undefined4 uStack_124;
  float local_110;
  float afStack_10c [4];
  float fStack_fc;
  float fStack_f8;
  float fStack_f4;
  float fStack_ec;
  float fStack_e8;
  float fStack_e4;
  float fStack_dc;
  float fStack_d8;
  float fStack_d4;
  float fStack_cc;
  float fStack_c8;
  float fStack_c4;
  float fStack_bc;
  float fStack_b8;
  float fStack_b4;
  float fStack_b0;
  float afStack_ac [2];
  vec_t vStack_a4;
  float fStack_a0;
  entvars_t *peStack_9c;
  CBaseEntity *pCStack_98;
  float fStack_94;
  float fStack_90;
  entvars_t *local_8c;
  CBaseEntity *pCStack_88;
  undefined1 auStack_80 [4];
  TraceResult local_7c [4];
  void *apvStack_78 [10];
  int iStack_50;
  Vector VStack_4c;
  float local_40;
  entvars_t *peStack_3c;
  CBaseEntity *pCStack_38;
  float fStack_34;
  float fStack_20;
  
  puVar15 = &stack0xfffffeb4;
  if (this->m_pActiveItem != (s_weapon_status *)0x0) {
    bVar22 = (this->m_pPlayer->pev->iuser4 & 0x70c30U) == 0;
    local_110 = this->m_pActiveInfo->flFireRate;
    if (!bVar22) {
      local_110 = local_110 * fc_1_5;
    }
    lVar23 = (longdouble)UTIL_WeaponTimeBase__Fv();
    this->field_0xa0 = (int)(float)(lVar23 + (longdouble)local_110);
    this->field_0x9c = (int)(float)(lVar23 + (longdouble)local_110);
    vVar25 = 7.57925e-40;
    lVar23 = (longdouble)UTIL_WeaponTimeBase__Fv();
    this->field_0xa8 = (int)(float)((longdouble)dc_3 + lVar23);
    *(float *)&this->m_pPlayer->field_0x4f8 = local_110;
    UTIL_MakeVectors__FRC6Vector(&this->m_pPlayer->pev->v_angle);
    ppgVar9 = PTR_gpGlobals_00177410;
    if (this->m_pActiveItem->pumpToggle == '\0') {
      uVar10 = (*PTR_g_engfuncs_001778b8->pfnRandomLong)(0,2);
      if ((bVar22) && ((this->m_pPlayer->pev->flags & 0x200) != 0)) {
        in_stack_fffffeac = (float)(CONCAT31(extraout_var,uVar10) + 0x1d);
        CTSAnimation::Animate
                  ((CTSAnimation *)&this->m_pPlayer->field_0x28c,(CBasePlayer *)this->m_pPlayer,
                   (int)in_stack_fffffeac);
      }
      CBasePlayer::GetMeleeMultiplier((CBasePlayer *)this->m_pPlayer);
      a2_00.y = (vec_t)(int)in_stack_fffffeb4;
      a2_00.z = (vec_t)(int)((ulonglong)in_stack_fffffeb4 >> 0x20);
      a2_00.x = vVar25;
      (*this->m_pPlayer->vtbl[1].TraceAttack)
                ((CBaseEntity *)&local_8c,(entvars_s *)this->m_pPlayer,in_stack_fffffeac,a2_00,
                 in_stack_fffffebc,(int)&local_8c);
      pTVar16 = in_stack_fffffebc;
      pfVar19 = &local_40;
      for (iVar13 = 3; iVar13 != 0; iVar13 = iVar13 + -1) {
        *pfVar19 = *(float *)pTVar16;
        pTVar16 = pTVar16 + 4;
        pfVar19 = pfVar19 + 1;
      }
      puVar15 = &stack0xfffffeb0;
      fStack_90 = local_40;
      local_8c = peStack_3c;
      pCStack_88 = pCStack_38;
      pgVar2 = *PTR_gpGlobals_00177410;
      fStack_b0 = (pgVar2->v_forward).x * 35.0;
      afStack_ac[0] = (pgVar2->v_forward).y * 35.0;
      afStack_ac[1] = (pgVar2->v_forward).z * 35.0;
      fStack_a0 = local_40 + fStack_b0;
      peStack_9c = (entvars_t *)((float)peStack_3c + afStack_ac[0]);
      pCStack_98 = (CBaseEntity *)((float)pCStack_38 + afStack_ac[1]);
      CBasePlayer::CheckHull(this->m_pPlayer,in_stack_fffffebc,&fStack_a0,uStack_124,3);
      this_00 = (CBaseEntity *)0x0;
      if ((float)apvStack_78[2] < (float)dc_1) {
        if (iStack_50 == 0) {
          uVar10 = (*PTR_g_engfuncs_001778b8->pfnPEntityOfEntOffset)(0);
          iStack_50 = CONCAT31(extraout_var_00,uVar10);
        }
        this_00 = (CBaseEntity *)0x0;
        if (iStack_50 != 0) {
          this_00 = *(CBaseEntity **)(iStack_50 + 0x7c);
        }
        iVar13 = (*this_00->vtbl->IsAlive)(this_00);
        if ((iVar13 == 0) || ((char)this_00->pev->effects < '\0')) {
          this_00 = (CBaseEntity *)0x0;
        }
        else {
          pTVar16 = local_7c;
          cVar11 = VictimKnifeHold__FP11CBasePlayerP11CBaseEntityf
                             ((CBasePlayer *)this->m_pPlayer,this_00,(float)local_7c);
          if (cVar11 == '\0') {
            iVar13 = 0x84326;
            ClearMultiDamage__Fv();
            pgVar2 = *PTR_gpGlobals_00177410;
            fStack_b0 = (pgVar2->v_forward).x;
            afStack_ac[0] = (pgVar2->v_forward).y;
            afStack_ac[1] = (pgVar2->v_forward).z;
            a2.y = (vec_t)auStack_80;
            a2.x = (vec_t)&fStack_b0;
            a2.z = 5.74532e-42;
            (*this_00->vtbl->TraceAttack)
                      (this_00,this->m_pPlayer->pev,(float)local_7c,a2,pTVar16,iVar13);
            ApplyMultiDamage__FP9entvars_sT0(this->m_pPlayer->pev,this->m_pPlayer->pev);
            iVar13 = (*this_00->vtbl->IsPlayer)(this_00);
            if (iVar13 != 0) {
              this->m_pActiveItem->burstOrPhase = 1;
              this->pev->body = 1;
              peVar3 = this_00->pev->pContainingEntity;
              fVar24 = 0.5;
              uVar10 = (*PTR_g_engfuncs_001778b8->pfnRandomLong)(0,0x1f);
              EMIT_SOUND_DYN__FP7edict_siPCcffiif
                        (peVar3,4,"weapons/knife/knife_hitbody.wav",1.0,0.8,0,
                         CONCAT31(extraout_var_01,uVar10) + 0x5f,fVar24);
            }
          }
        }
      }
      if (this_00 == (CBaseEntity *)0x0) {
        *(float *)&this->m_pPlayer->field_0x79c = fc_0_25 * this->m_pActiveInfo->flFireRate;
        *(TraceResult **)&this->m_pPlayer->field_0x7a0 = local_7c;
        this->m_pPlayer->field_0x7a4 = 1;
      }
      (*PTR_g_engfuncs_001778b8->pfnPlaybackEvent)
                (1,this->m_pPlayer->pev->pContainingEntity,(uint)*(ushort *)&this->field_0x972,0,
                 PTR_g_vecZero_00177698,PTR_g_vecZero_00177698,0,0,this->m_weaponType,
                 this->m_pActiveItem->burstOrPhase,0,0);
    }
    else {
      peVar7 = this->m_pPlayer->pev;
      fStack_bc = (peVar7->origin).x + (peVar7->view_ofs).x;
      fStack_b8 = (peVar7->origin).y + (peVar7->view_ofs).y;
      fStack_b4 = (peVar7->origin).z + (peVar7->view_ofs).z;
      pgVar2 = *PTR_gpGlobals_00177410;
      fStack_cc = fc_32 * (pgVar2->v_forward).x;
      fStack_c8 = fc_32 * (pgVar2->v_forward).y;
      fStack_c4 = fc_32 * (pgVar2->v_forward).z;
      peStack_3c = (entvars_t *)(fStack_bc + fStack_cc);
      pCStack_38 = (CBaseEntity *)(fStack_c8 + fStack_b8);
      fStack_34 = fStack_b4 + fStack_c4;
      fStack_dc = fc_1 * (pgVar2->v_right).x;
      fStack_d8 = fc_1 * (pgVar2->v_right).y;
      fStack_d4 = fc_1 * (pgVar2->v_right).z;
      VStack_4c.x = (float)peStack_3c + fStack_dc;
      VStack_4c.y = (float)pCStack_38 + fStack_d8;
      VStack_4c.z = fStack_34 + fStack_d4;
      peVar4 = this->m_pPlayer->pev;
      fStack_fc = (peVar4->origin).x + (peVar4->view_ofs).x;
      fStack_f8 = (peVar4->origin).y + (peVar4->view_ofs).y;
      fStack_f4 = (peVar4->origin).z + (peVar4->view_ofs).z;
      afStack_10c[0] = fc_1 * (pgVar2->v_right).x;
      afStack_10c[1] = fc_1 * (pgVar2->v_right).y;
      afStack_10c[2] = fc_1 * (pgVar2->v_right).z;
      fStack_ec = fStack_fc + afStack_10c[0];
      fStack_e8 = afStack_10c[1] + fStack_f8;
      fStack_e4 = fStack_f4 + afStack_10c[2];
      UTIL_TraceLine__FRC6VectorT015IGNORE_MONSTERSP7edict_sP11TraceResult
                (&fStack_ec,&VStack_4c,0,peVar7->pContainingEntity,&local_8c);
      ppvVar17 = apvStack_78;
      pfVar19 = afStack_ac;
      for (iVar13 = 3; iVar13 != 0; iVar13 = iVar13 + -1) {
        *pfVar19 = (float)*ppvVar17;
        ppvVar17 = ppvVar17 + 1;
        pfVar19 = pfVar19 + 1;
      }
      if (*(int *)&this->m_pActiveInfo->field_0x20 == 6) {
        pgVar2 = *ppgVar9;
        VStack_4c.x = _UNK_00107f68 * (pgVar2->v_forward).x;
        VStack_4c.y = _UNK_00107f68 * (pgVar2->v_forward).y;
        VStack_4c.z = _UNK_00107f68 * (pgVar2->v_forward).z;
        iVar13 = 3;
        pVVar18 = &VStack_4c;
        ppeVar20 = &peStack_9c;
      }
      else {
        pgVar2 = *PTR_gpGlobals_00177410;
        pVVar18 = &VStack_4c;
        VStack_4c.x = _UNK_00107f68 * (pgVar2->v_forward).x;
        VStack_4c.y = _UNK_00107f68 * (pgVar2->v_forward).y;
        VStack_4c.z = _UNK_00107f68 * (pgVar2->v_forward).z;
        ppeVar20 = &peStack_9c;
        iVar13 = 3;
      }
      for (; iVar13 != 0; iVar13 = iVar13 + -1) {
        *ppeVar20 = (entvars_t *)pVVar18->x;
        pVVar18 = (Vector *)&pVVar18->y;
        ppeVar20 = ppeVar20 + 1;
      }
      VStack_4c.x = afStack_ac[0];
      VStack_4c.y = afStack_ac[1];
      VStack_4c.z = vStack_a4;
      peStack_3c = peStack_9c;
      pCStack_38 = pCStack_98;
      fStack_34 = fStack_94;
      piVar12 = (int *)CTSKnife::CreateKnife(this->m_pPlayer,&VStack_4c,&peStack_3c);
      if (piVar12 != (int *)0x0) {
        iVar13 = *piVar12;
        fStack_20 = SQRT(*(float *)(iVar13 + 0x28) * *(float *)(iVar13 + 0x28) +
                         *(float *)(iVar13 + 0x24) * *(float *)(iVar13 + 0x24) +
                         *(float *)(iVar13 + 0x20) * *(float *)(iVar13 + 0x20));
        if (fStack_20 == fc_0) {
          fStack_34 = fc_1;
          peStack_3c = (entvars_t *)0x0;
          pCStack_38 = (CBaseEntity *)0x0;
        }
        else {
          fStack_34 = fc_1 / fStack_20;
          peStack_3c = (entvars_t *)(fStack_34 * *(float *)(iVar13 + 0x20));
          pCStack_38 = (CBaseEntity *)(fStack_34 * *(float *)(iVar13 + 0x24));
          fStack_34 = fStack_34 * *(float *)(iVar13 + 0x28);
        }
        UTIL_VecToAngles__FRC6Vector(&VStack_4c);
        iVar13 = *piVar12;
        pVVar18 = &VStack_4c;
        pfVar19 = afStack_10c;
        for (iVar14 = 3; iVar14 != 0; iVar14 = iVar14 + -1) {
          *pfVar19 = pVVar18->x;
          pVVar18 = (Vector *)&pVVar18->y;
          pfVar19 = pfVar19 + 1;
        }
        pfVar19 = afStack_10c;
        pfVar21 = (float *)(iVar13 + 0x50);
        for (iVar14 = 3; iVar14 != 0; iVar14 = iVar14 + -1) {
          *pfVar21 = *pfVar19;
          pfVar19 = pfVar19 + 1;
          pfVar21 = pfVar21 + 1;
        }
        *(undefined4 *)(*piVar12 + 0x58) = 0x42b40000;
        if (*(int *)&this->m_pActiveInfo->field_0x20 == 6) {
          iVar13 = *piVar12;
        }
        else {
          iVar13 = *piVar12;
        }
        *(undefined4 *)(iVar13 + 0x5c) = 0xc4a28000;
      }
      peVar7 = this->m_pPlayer->pev;
      if (((peVar7->iuser4 & 0x30c30U) == 0) && ((peVar7->flags & 0x200) != 0)) {
        CTSAnimation::Animate
                  ((CTSAnimation *)&this->m_pPlayer->field_0x28c,(CBasePlayer *)this->m_pPlayer,0x20
                  );
      }
      peVar3 = this->m_pPlayer->pev->pContainingEntity;
      fVar24 = 0.5;
      uVar10 = (*PTR_g_engfuncs_001778b8->pfnRandomLong)(0,0x1f);
      EMIT_SOUND_DYN__FP7edict_siPCcffiif
                (peVar3,4,"weapons/knife/knife_miss.wav",1.0,0.8,0,
                 CONCAT31(extraout_var_02,uVar10) + 0x5f,fVar24);
    }
    if (*(int *)(puVar15 + 0x38) != 0) {
      *(CBaseEntity **)(puVar15 + -0x10) = this->m_pPlayer;
      *(undefined4 *)(puVar15 + -0x14) = 0x8490d;
      iVar13 = InfiniteClips__FP11CBasePlayer(*(CBasePlayer **)(puVar15 + -0x10));
      if (iVar13 == 0) {
        puVar1 = &this->m_pPlayer->field_0x548;
        *(uint *)puVar1 = *(int *)puVar1 + (uint)(byte)this->m_pActiveInfo->slotCost;
        *(CBaseEntity **)(puVar15 + -0x10) = this->m_pPlayer;
        *(undefined4 *)(puVar15 + -0x14) = 0x8493b;
        lVar23 = (longdouble)GetSpeedBySlots__FP11CBasePlayer(*(CBasePlayer **)(puVar15 + -0x10));
        *(float *)(puVar15 + -0xc) = (float)lVar23;
        *(CBaseEntity **)(puVar15 + -0x10) = this->m_pPlayer;
        *(undefined4 *)(puVar15 + -0x14) = 0x8494c;
        CBasePlayer::SetPlayerSpeed(*(CBasePlayer **)(puVar15 + -0x10),*(float *)(puVar15 + -0xc));
        piVar12 = &this->m_pActiveItem->iClip;
        *piVar12 = *piVar12 + -1;
      }
      if (this->m_pActiveItem->iClip < 1) {
        *(undefined2 *)(puVar15 + -8) = 1;
        *(int *)(puVar15 + -0xc) = this->m_weaponType;
        *(CTSGun **)(puVar15 + -0x10) = this;
        *(undefined4 *)(puVar15 + -0x14) = 0x84a06;
        DropWeapon(*(CTSGun **)(puVar15 + -0x10),*(int *)(puVar15 + -0xc),(bool)puVar15[-8]);
      }
      else {
        pCVar5 = this->vtbl;
        *(undefined4 *)(puVar15 + -4) = 0;
        *(undefined4 *)(puVar15 + -8) = 1;
        if (*(int *)&this->m_pActiveItem->field_0x28 == 0) {
          iVar13 = 0;
        }
        else {
          iVar13 = (int)*(short *)(*(int *)&this->m_pActiveItem->field_0x28 + 0x28);
        }
        *(int *)(puVar15 + -0xc) = iVar13;
        *(CTSGun **)(puVar15 + -0x10) = this;
        pCVar6 = pCVar5->SendWeaponAnim;
        *(undefined4 *)(puVar15 + -0x14) = 0x84991;
        (*pCVar6)(*(CBasePlayerWeapon **)(puVar15 + -0x10),*(int *)(puVar15 + -0xc),
                  *(int *)(puVar15 + -8),*(int *)(puVar15 + -4));
        peVar7 = this->m_pPlayer->pev;
        *(undefined4 *)(puVar15 + -4) = 0;
        *(undefined4 *)(puVar15 + -8) = 0;
        *(int *)(puVar15 + -0xc) = this->m_pActiveItem->attachEquipped;
        *(int *)(puVar15 + -0x10) = this->m_weaponType;
        *(undefined4 *)(puVar15 + -0x14) = 0;
        *(undefined4 *)(puVar15 + -0x18) = 0;
        *(undefined **)(puVar15 + -0x1c) = PTR_g_vecZero_00177698;
        *(undefined **)(puVar15 + -0x20) = PTR_g_vecZero_00177698;
        *(undefined4 *)(puVar15 + -0x24) = 0;
        *(uint *)(puVar15 + -0x28) = (uint)*(ushort *)&this->field_0x976;
        *(edict_t **)(puVar15 + -0x2c) = peVar7->pContainingEntity;
        *(undefined4 *)(puVar15 + -0x30) = 1;
        peVar8 = PTR_g_engfuncs_001778b8->pfnPlaybackEvent;
        *(undefined4 *)(puVar15 + -0x34) = 0x849d9;
        (*peVar8)();
        *(undefined4 *)(puVar15 + -4) = 0x849e1;
        lVar23 = (longdouble)UTIL_WeaponTimeBase__Fv();
        this->field_0xa8 = (int)(float)((longdouble)dc_2 + lVar23);
      }
    }
  }
  return;
}


