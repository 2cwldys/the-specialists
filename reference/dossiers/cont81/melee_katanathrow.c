// ===== FUN @ 00083b68 size=787 =====

/* WARNING: Unknown calling convention -- yet parameter storage is locked */
/* TS_gun.cpp
   assets: weapons/katana/miss.wav */

void CTSGun::KatanaThrow(CTSGun *this)

{
  undefined1 *puVar1;
  entvars_t *peVar2;
  globalvars_t *pgVar3;
  entvars_t *peVar4;
  edict_s *a0;
  undefined uVar5;
  int *piVar6;
  undefined3 extraout_var;
  int iVar7;
  int iVar8;
  vec_t *pvVar9;
  Vector *pVVar10;
  float *pfVar11;
  vec_t *pvVar12;
  float *pfVar13;
  longdouble lVar14;
  float fVar15;
  Vector local_10c;
  float local_fc;
  float local_f8;
  float local_f4;
  float local_ec [4];
  float local_dc;
  float local_d8;
  float local_d4;
  float local_cc;
  float local_c8;
  float local_c4;
  float local_bc;
  float local_b8;
  float local_b4;
  float local_ac;
  float local_a8;
  float local_a4;
  float local_9c;
  float local_98;
  float local_94;
  undefined1 local_8c [20];
  vec_t local_78 [11];
  float local_4c [4];
  vec_t local_3c [7];
  float fStack_20;
  
  if (this->m_pActiveItem != (s_weapon_status *)0x0) {
    fVar15 = this->m_pActiveInfo->flSpread;
    lVar14 = (longdouble)UTIL_WeaponTimeBase__Fv();
    lVar14 = lVar14 + (longdouble)fVar15;
    this->field_0xa0 = (int)(float)lVar14;
    this->field_0x9c = (int)(float)lVar14;
    lVar14 = (longdouble)UTIL_WeaponTimeBase__Fv();
    this->field_0xa8 = (int)(float)((longdouble)dc_3 + lVar14);
    UTIL_MakeVectors__FRC6Vector(&this->m_pPlayer->pev->v_angle);
    fVar15 = fc_1;
    peVar2 = this->m_pPlayer->pev;
    local_bc = (peVar2->origin).x + (peVar2->view_ofs).x;
    local_b8 = (peVar2->origin).y + (peVar2->view_ofs).y;
    local_b4 = (peVar2->origin).z + (peVar2->view_ofs).z;
    pgVar3 = *PTR_gpGlobals_00177410;
    local_cc = fc_32 * (pgVar3->v_forward).x;
    local_c8 = fc_32 * (pgVar3->v_forward).y;
    local_c4 = fc_32 * (pgVar3->v_forward).z;
    local_ac = local_bc + local_cc;
    local_a8 = local_b8 + local_c8;
    local_a4 = local_b4 + local_c4;
    local_dc = fc_1 * (pgVar3->v_right).x;
    local_d8 = fc_1 * (pgVar3->v_right).y;
    local_d4 = fc_1 * (pgVar3->v_right).z;
    local_9c = local_ac + local_dc;
    local_98 = local_a8 + local_d8;
    local_94 = local_a4 + local_d4;
    peVar4 = this->m_pPlayer->pev;
    local_fc = (peVar4->origin).x + (peVar4->view_ofs).x;
    local_f8 = (peVar4->origin).y + (peVar4->view_ofs).y;
    local_f4 = (peVar4->origin).z + (peVar4->view_ofs).z;
    local_10c.x = fc_1 * (pgVar3->v_right).x;
    local_10c.y = fc_1 * (pgVar3->v_right).y;
    local_10c.z = fc_1 * (pgVar3->v_right).z;
    local_ec[0] = local_fc + local_10c.x;
    local_ec[1] = local_f8 + local_10c.y;
    local_ec[2] = local_f4 + local_10c.z;
    UTIL_TraceLine__FRC6VectorT015IGNORE_MONSTERSP7edict_sP11TraceResult
              (local_ec,&local_9c,0,peVar2->pContainingEntity,local_8c);
    pvVar9 = local_78;
    pvVar12 = local_3c;
    for (iVar7 = 3; iVar7 != 0; iVar7 = iVar7 + -1) {
      *pvVar12 = *pvVar9;
      pvVar9 = pvVar9 + 1;
      pvVar12 = pvVar12 + 1;
    }
    pVVar10 = &local_10c;
    pvVar9 = local_4c;
    for (iVar7 = 3; iVar7 != 0; iVar7 = iVar7 + -1) {
      *pvVar9 = pVVar10->x;
      pVVar10 = (Vector *)&pVVar10->y;
      pvVar9 = pvVar9 + 1;
    }
    local_10c.x = local_3c[0];
    local_10c.y = local_3c[1];
    local_10c.z = local_3c[2];
    local_fc = local_4c[0];
    local_f8 = local_4c[1];
    local_f4 = local_4c[2];
    piVar6 = (int *)CTSKnife::CreateKnife
                              (this->m_pPlayer,&local_10c,&local_fc,
                               *(undefined4 *)&this->m_pActiveInfo->field_0x20);
    if (piVar6 != (int *)0x0) {
      iVar7 = *piVar6;
      fStack_20 = SQRT(*(float *)(iVar7 + 0x28) * *(float *)(iVar7 + 0x28) +
                       *(float *)(iVar7 + 0x24) * *(float *)(iVar7 + 0x24) +
                       *(float *)(iVar7 + 0x20) * *(float *)(iVar7 + 0x20));
      if (fStack_20 == fc_0) {
        local_fc = 0.0;
        local_f8 = 0.0;
      }
      else {
        fVar15 = fc_1 / fStack_20;
        local_fc = fVar15 * *(float *)(iVar7 + 0x20);
        local_f8 = fVar15 * *(float *)(iVar7 + 0x24);
        fVar15 = fVar15 * *(float *)(iVar7 + 0x28);
      }
      local_f4 = fVar15;
      UTIL_VecToAngles__FRC6Vector(&local_10c);
      iVar7 = *piVar6;
      pVVar10 = &local_10c;
      pfVar11 = local_ec;
      for (iVar8 = 3; iVar8 != 0; iVar8 = iVar8 + -1) {
        *pfVar11 = pVVar10->x;
        pVVar10 = (Vector *)&pVVar10->y;
        pfVar11 = pfVar11 + 1;
      }
      pfVar11 = local_ec;
      pfVar13 = (float *)(iVar7 + 0x50);
      for (iVar8 = 3; iVar8 != 0; iVar8 = iVar8 + -1) {
        *pfVar13 = *pfVar11;
        pfVar11 = pfVar11 + 1;
        pfVar13 = pfVar13 + 1;
      }
      *(undefined4 *)(*piVar6 + 0x58) = 0x42b40000;
      *(undefined4 *)(*piVar6 + 0x5c) = 0xc42f0000;
    }
    peVar2 = this->m_pPlayer->pev;
    if (((peVar2->iuser4 & 0x30c30U) == 0) && ((peVar2->flags & 0x200) != 0)) {
      CTSAnimation::Animate
                ((CTSAnimation *)&this->m_pPlayer->field_0x28c,(CBasePlayer *)this->m_pPlayer,0x20);
    }
    a0 = this->m_pPlayer->pev->pContainingEntity;
    fVar15 = 0.5;
    uVar5 = (*PTR_g_engfuncs_001778b8->pfnRandomLong)(0,0x1f);
    EMIT_SOUND_DYN__FP7edict_siPCcffiif
              (a0,4,"weapons/katana/miss.wav",1.0,0.8,0,CONCAT31(extraout_var,uVar5) + 0x5f,fVar15);
    iVar7 = InfiniteClips__FP11CBasePlayer((CBasePlayer *)this->m_pPlayer);
    if (iVar7 == 0) {
      puVar1 = &this->m_pPlayer->field_0x548;
      *(uint *)puVar1 = *(int *)puVar1 + (uint)(byte)this->m_pActiveInfo->slotCost;
      lVar14 = (longdouble)GetSpeedBySlots__FP11CBasePlayer((CBasePlayer *)this->m_pPlayer);
      CBasePlayer::SetPlayerSpeed((CBasePlayer *)this->m_pPlayer,(float)lVar14);
      DropWeapon(this,this->m_weaponType,true);
    }
  }
  return;
}


