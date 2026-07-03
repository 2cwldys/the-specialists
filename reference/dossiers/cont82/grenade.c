// ===== FUN @ 000832f0 size=235 =====

/* WARNING: Unknown calling convention -- yet parameter storage is locked */
/* TS_gun.cpp */

void CTSGun::GrenadeFire(CTSGun *this)

{
  int a0;
  longdouble lVar1;
  longdouble lVar2;
  
  if (((this->m_pActiveItem != (s_weapon_status *)0x0) && (this->m_pActiveItem->burstOrPhase == 0))
     && (0 < this->m_pActiveItem->iClip)) {
    if (*(int *)&this->m_pActiveItem->field_0x28 == 0) {
      a0 = 0;
    }
    else {
      a0 = (int)*(short *)(*(int *)&this->m_pActiveItem->field_0x28 + 0x26);
    }
    (*this->vtbl->SendWeaponAnim)((CBasePlayerWeapon *)this,a0,1,0);
    (*PTR_g_engfuncs_001778b8->pfnPlaybackEvent)
              (1,this->m_pPlayer->pev->pContainingEntity,(uint)*(ushort *)&this->field_0x978,0,
               PTR_g_vecZero_00177698,PTR_g_vecZero_00177698,0,0,0x13,0,0,0);
    this->m_pActiveItem->burstOrPhase = 1;
    lVar1 = (longdouble)UTIL_WeaponTimeBase__Fv();
    lVar2 = (longdouble)dc_0_7;
    this->field_0xa0 = (int)(float)(lVar2 + lVar1);
    this->field_0x9c = (int)(float)(lVar2 + lVar1);
    lVar1 = (longdouble)UTIL_WeaponTimeBase__Fv();
    this->field_0xa8 = (int)(float)((longdouble)dc_0_7 + lVar1);
    this->field_0xa0c = (int)(fc_5 + (*PTR_gpGlobals_00177410)->time);
  }
  return;
}


// ===== FUN @ 00084f18 size=1456 =====

/* WARNING: Unknown calling convention -- yet parameter storage is locked */
/* TS_gun.cpp */

void CTSGun::GrenadeIdle(CTSGun *this,int a0)

{
  undefined1 *puVar1;
  double dVar2;
  entvars_t *peVar3;
  float fVar4;
  globalvars_t *pgVar5;
  s_weapon_status *psVar6;
  bool bVar7;
  int *piVar8;
  short *psVar9;
  int iVar10;
  float *pfVar11;
  float *pfVar12;
  float *pfVar13;
  longdouble lVar14;
  float local_94;
  float local_8c [4];
  float local_7c;
  float local_78;
  float local_74;
  float local_6c [4];
  float local_5c [4];
  float local_4c [7];
  float local_30;
  Vector local_2c [2];
  
  bVar7 = false;
  if (this->m_pActiveItem != (s_weapon_status *)0x0) {
    if (this->m_pActiveItem->burstOrPhase == 1) {
      if (((this->m_pPlayer->pev->button & 0x801U) != 0) && (a0 == 0)) {
        return;
      }
      peVar3 = this->m_pPlayer->pev;
      fVar4 = (peVar3->v_angle).x;
      local_2c[0].y = (peVar3->v_angle).y;
      local_2c[0].z = (peVar3->v_angle).z;
      dVar2 = dc_1_1111111111111112;
      if (fVar4 < fc_0) {
        dVar2 = dc_0_888888888888889;
      }
      local_2c[0].x = (float)dVar2 * fVar4 + (float)dc_neg10;
      local_94 = (fc_90 - local_2c[0].x) * fc_7;
      if (fc_750 < local_94) {
        local_94 = fc_750;
      }
      UTIL_MakeVectors__FRC6Vector(local_2c);
      if (this->m_pActiveItem->pumpToggle == '\0') {
        peVar3 = this->m_pPlayer->pev;
        local_7c = (peVar3->origin).x + (peVar3->view_ofs).x;
        local_78 = (peVar3->origin).y + (peVar3->view_ofs).y;
        local_74 = (peVar3->origin).z + (peVar3->view_ofs).z;
        pgVar5 = *PTR_gpGlobals_00177410;
        local_8c[0] = fc_16 * (pgVar5->v_forward).x;
        local_8c[1] = fc_16 * (pgVar5->v_forward).y;
        local_8c[2] = fc_16 * (pgVar5->v_forward).z;
        local_6c[0] = local_7c + local_8c[0];
        local_6c[1] = local_78 + local_8c[1];
        local_6c[2] = local_74 + local_8c[2];
        pfVar12 = local_6c;
        pfVar11 = local_4c;
        for (iVar10 = 3; iVar10 != 0; iVar10 = iVar10 + -1) {
          *pfVar11 = *pfVar12;
          pfVar12 = pfVar12 + 1;
          pfVar11 = pfVar11 + 1;
        }
        local_7c = local_94 * (pgVar5->v_forward).x;
        local_78 = local_94 * (pgVar5->v_forward).y;
        local_74 = local_94 * (pgVar5->v_forward).z;
        peVar3 = this->m_pPlayer->pev;
        local_8c[0] = local_7c + (peVar3->velocity).x;
        local_8c[1] = local_78 + (peVar3->velocity).y;
        local_8c[2] = local_74 + (peVar3->velocity).z;
        iVar10 = 3;
        pfVar12 = local_8c;
        pfVar11 = local_5c;
      }
      else {
        peVar3 = this->m_pPlayer->pev;
        pgVar5 = *PTR_gpGlobals_00177410;
        local_7c = fc_16 * (pgVar5->v_forward).x;
        local_78 = fc_16 * (pgVar5->v_forward).y;
        local_74 = fc_16 * (pgVar5->v_forward).z;
        pfVar12 = local_8c;
        local_8c[0] = (peVar3->origin).x + local_7c;
        local_8c[1] = (peVar3->origin).y + local_78;
        local_8c[2] = (peVar3->origin).z + local_74;
        pfVar11 = pfVar12;
        pfVar13 = local_4c;
        for (iVar10 = 3; iVar10 != 0; iVar10 = iVar10 + -1) {
          *pfVar13 = *pfVar11;
          pfVar11 = pfVar11 + 1;
          pfVar13 = pfVar13 + 1;
        }
        local_30 = (float)dc_0_6666666666666666 * local_94;
        local_7c = local_30 * (pgVar5->v_forward).x;
        local_78 = local_30 * (pgVar5->v_forward).y;
        local_74 = local_30 * (pgVar5->v_forward).z;
        peVar3 = this->m_pPlayer->pev;
        local_8c[0] = local_7c + (peVar3->velocity).x;
        local_8c[1] = local_78 + (peVar3->velocity).y;
        local_8c[2] = local_74 + (peVar3->velocity).z;
        pfVar11 = local_5c;
        iVar10 = 3;
      }
      for (; iVar10 != 0; iVar10 = iVar10 + -1) {
        *pfVar11 = *pfVar12;
        pfVar12 = pfVar12 + 1;
        pfVar11 = pfVar11 + 1;
      }
      if (*(int *)&this->m_pActiveInfo->field_0x20 == 4) {
        local_8c[0] = local_4c[0];
        local_8c[1] = local_4c[1];
        local_8c[2] = local_4c[2];
        local_7c = local_5c[0];
        local_78 = local_5c[1];
        local_74 = local_5c[2];
        piVar8 = (int *)CM61Grenade::CreateGrenade(this->m_pPlayer,fc_3,local_8c,&local_7c);
        if (piVar8 != (int *)0x0) {
          if (this->m_pActiveItem->pumpToggle == '\0') {
            *(undefined4 *)(*piVar8 + 0x5c) = 0x43480000;
          }
          else {
            *(undefined4 *)(*piVar8 + 0x120) = 0x3ecccccd;
            *(undefined4 *)(*piVar8 + 0x60) = 0x44160000;
            *(undefined4 *)(*piVar8 + 0x5c) = 0xc2c80000;
          }
        }
      }
      psVar6 = this->m_pActiveItem;
      if (*(int *)&psVar6->field_0x28 == 0) {
        iVar10 = 0;
      }
      else {
        if (psVar6->pumpToggle == '\0') {
          psVar9 = (short *)(*(int *)&psVar6->field_0x28 + 0x28);
        }
        else {
          psVar9 = (short *)(*(int *)&psVar6->field_0x28 + 0x2a);
        }
        iVar10 = (int)*psVar9;
      }
      (*this->vtbl->SendWeaponAnim)((CBasePlayerWeapon *)this,iVar10,1,0);
      (*PTR_g_engfuncs_001778b8->pfnPlaybackEvent)
                (1,this->m_pPlayer->pev->pContainingEntity,(uint)*(ushort *)&this->field_0x978,0,
                 PTR_g_vecZero_00177698,PTR_g_vecZero_00177698,0,0,
                 (this->m_pActiveItem->pumpToggle != '\0') + 0x14,0,0,0);
      CTSAnimation::Animate
                ((CTSAnimation *)&this->m_pPlayer->field_0x28c,(CBasePlayer *)this->m_pPlayer,0x35);
      lVar14 = (longdouble)UTIL_WeaponTimeBase__Fv();
      local_30 = (float)((longdouble)dc_0_5 + lVar14);
      this->field_0xa0 = (int)local_30;
      this->field_0x9c = (int)local_30;
      lVar14 = (longdouble)UTIL_WeaponTimeBase__Fv();
      this->field_0xa8 = (int)(float)((longdouble)dc_0_5 + lVar14);
      if (this->m_pActiveItem->iClip == 1) {
        this->m_pPlayer->pev->weaponmodel = 0;
      }
      if (a0 == 0) {
        this->m_pActiveItem->burstOrPhase = 2;
      }
      else {
        this->m_pActiveItem->burstOrPhase = 0;
      }
      bVar7 = a0 != 0;
      piVar8 = &this->m_pActiveItem->iClip;
      *piVar8 = *piVar8 + -1;
    }
    else if (this->m_pActiveItem->burstOrPhase == 2) {
      this->m_pActiveItem->burstOrPhase = 0;
      bVar7 = true;
    }
    else {
      lVar14 = (longdouble)UTIL_WeaponTimeBase__Fv();
      this->field_0xa8 = (int)(float)((longdouble)dc_3 + lVar14);
    }
    if (bVar7) {
      puVar1 = &this->m_pPlayer->field_0x548;
      *(uint *)puVar1 = *(int *)puVar1 + (uint)(byte)this->m_pActiveInfo->slotCost;
      lVar14 = (longdouble)GetSpeedBySlots__FP11CBasePlayer((CBasePlayer *)this->m_pPlayer);
      CBasePlayer::SetPlayerSpeed((CBasePlayer *)this->m_pPlayer,(float)lVar14);
      if (this->m_pActiveItem->iClip < 1) {
        DropWeapon(this,this->m_weaponType,true);
      }
      else {
        if (*(int *)&this->m_pActiveItem->field_0x28 == 0) {
          iVar10 = 0;
        }
        else {
          iVar10 = (int)*(short *)(*(int *)&this->m_pActiveItem->field_0x28 + 4);
        }
        (*this->vtbl->SendWeaponAnim)((CBasePlayerWeapon *)this,iVar10,1,0);
        lVar14 = (longdouble)UTIL_WeaponTimeBase__Fv();
        this->field_0xa8 = (int)(float)((longdouble)dc_2 + lVar14);
      }
    }
  }
  return;
}


// ===== FUN @ 0007d01c size=417 =====

/* WARNING: Type propagation algorithm not settling */
/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */
/* WARNING: Unknown calling convention -- yet parameter storage is locked */
/* TS_grenades.cpp */

void TSExplosion__FG6VectorP9entvars_sT1fiff
               (Vector a0,entvars_s *a1,entvars_s *a2,float a3,int a4,float a5,float a6)

{
  float fVar1;
  float fVar2;
  code *pcVar3;
  char *pcVar4;
  int *piVar5;
  uint uVar6;
  engine_fn *peVar7;
  float fVar8;
  float fVar9;
  globalvars_t **ppgVar10;
  enginefuncs_t *peVar11;
  undefined *puVar12;
  undefined uVar13;
  int iVar14;
  undefined3 extraout_var;
  undefined3 extraout_var_00;
  int iVar15;
  undefined1 *puVar16;
  undefined1 *puVar17;
  int *piVar18;
  float *pfVar19;
  undefined4 *puVar20;
  float *pfVar21;
  undefined4 *puVar22;
  undefined2 in_FPUControlWord;
  int aiStackY_144 [2];
  undefined1 auStack_11c [24];
  float *local_104;
  float afStack_100 [2];
  float *local_f8;
  float local_f4 [9];
  float afStack_d0 [11];
  float afStack_a4 [8];
  int local_84;
  undefined2 local_80;
  undefined2 uStack_7e;
  float local_7c [8];
  int aiStack_5c [3];
  float afStack_50 [8];
  int aiStack_30 [8];
  
  piVar18 = (int *)0x0;
  *(float *)((int)a0.x + 8) = fc_1 + *(float *)((int)a0.x + 8);
  pfVar19 = (float *)a0.x;
  pfVar21 = local_7c;
  for (iVar15 = 3; iVar15 != 0; iVar15 = iVar15 + -1) {
    *pfVar21 = *pfVar19;
    pfVar19 = pfVar19 + 1;
    pfVar21 = pfVar21 + 1;
  }
  local_7c[2] = fc_50 + local_7c[2];
  UTIL_ScreenShake__FRC6Vectorffff((Vector *)a0.x,(float)a1,80.0,1.0,(float)a4 + (float)a4);
  peVar11 = PTR_g_engfuncs_001778b8;
  (*PTR_g_engfuncs_001778b8->pfnMessageBegin)(2,*(undefined4 *)PTR_gmsgTSCreateDebris_00177150,0,0);
  (*peVar11->pfnWriteCoord)(*(undefined4 *)a0.x);
  (*peVar11->pfnWriteCoord)(*(undefined4 *)((int)a0.x + 4));
  (*peVar11->pfnWriteCoord)(*(undefined4 *)((int)a0.x + 8));
  _local_80 = CONCAT22(uStack_7e,in_FPUControlWord);
  local_84 = (int)ROUND((float)a1);
  (*peVar11->pfnWriteCoord)((float)local_84);
  (*peVar11->pfnWriteCoord)(0);
  (*peVar11->pfnWriteCoord)(0);
  (*peVar11->pfnWriteByte)(1);
  (*peVar11->pfnMessageEnd)();
  local_f4[0] = (float)a1 / ((float)a4 - a3);
  local_f8 = afStack_a4 + 2;
  local_104 = afStack_d0 + 1;
  puVar17 = auStack_11c;
  do {
    do {
      *(undefined4 *)(puVar17 + -8) = *(undefined4 *)(puVar17 + 0x138);
      *(undefined4 *)(puVar17 + -0xc) = *(undefined4 *)(puVar17 + 0x120);
      *(int **)(puVar17 + -0x10) = piVar18;
      *(undefined4 *)(puVar17 + -0x14) = 0x7d957;
      piVar18 = (int *)UTIL_FindEntityInSphere__FP11CBaseEntityRC6Vectorf
                                 (*(CBaseEntity **)(puVar17 + -0x10),*(Vector **)(puVar17 + -0xc),
                                  *(float *)(puVar17 + -8));
      if (piVar18 == (int *)0x0) {
        *(undefined4 *)(puVar17 + -4) = 0x7d969;
        iVar15 = DoSpectacularNess__Fv();
        if (iVar15 != 0) {
          puVar20 = *(undefined4 **)(puVar17 + 0x120);
          *(undefined4 *)(puVar17 + 0x80) = *puVar20;
          *(undefined4 *)(puVar17 + 0x84) = puVar20[1];
          *(undefined4 *)(puVar17 + 0x88) = puVar20[2];
          *(undefined2 *)(puVar17 + -0x10) = 0;
          *(undefined4 *)(puVar17 + -0x14) = 0;
          *(undefined4 *)(puVar17 + -0x18) = 0x3f800000;
          *(undefined4 *)(puVar17 + -0x1c) = 0x3e800000;
          *(undefined1 **)(puVar17 + -0x20) = puVar17 + 0x80;
          *(undefined4 *)(puVar17 + -0x24) = 0x7d9b0;
          CTSSlowMotionPoint::CreateSlowMotionPoint
                    (*(CTSSlowMotionPoint **)(puVar17 + -0x20),*(Vector *)(puVar17 + -0x1c),
                     *(float *)(puVar17 + -0x10),*(float *)(puVar17 + -0xc),*(float *)(puVar17 + -8)
                     ,(bool)puVar17[-4]);
        }
        return;
      }
    } while (*(int *)(puVar17 + 0x124) == *piVar18);
    puVar16 = puVar17;
    if ((fc_0 != *(float *)(*piVar18 + 0x16c)) && (*(int *)(*piVar18 + 0x170) != 2)) {
      iVar15 = piVar18[0x18];
      *(int **)(puVar17 + -0x10) = piVar18;
      pcVar3 = *(code **)(iVar15 + 0xb4);
      *(undefined4 *)(puVar17 + -0x14) = 0x7d204;
      iVar15 = (*pcVar3)();
      if (iVar15 != 0) {
        iVar15 = piVar18[0x18];
        *(int **)(puVar17 + -0x10) = piVar18;
        pcVar3 = *(code **)(iVar15 + 200);
        *(undefined4 *)(puVar17 + -0x14) = 0x7d218;
        iVar15 = (*pcVar3)();
        if (iVar15 != 0) {
          *(undefined4 *)(puVar17 + 0x80) = _UNK_00107eb0;
          *(undefined4 *)(puVar17 + 0x84) = _UNK_00107eb4;
          *(undefined4 *)(puVar17 + 0x88) = _UNK_00107eb8;
          *(undefined4 *)(puVar17 + -0xc) = 0;
          *(undefined4 *)(puVar17 + -0x10) = 0x4b;
          *(undefined4 *)(puVar17 + -0x14) = 0x3d4ccccd;
          *(undefined4 *)(puVar17 + -0x18) = 0x3dcccccd;
          *(undefined4 *)(puVar17 + -0x1c) = *(undefined4 *)(puVar17 + 0x24);
          *(int **)(puVar17 + -0x20) = piVar18;
          *(undefined4 *)(puVar17 + -0x24) = 0x7d261;
          UTIL_ScreenFade__FP11CBaseEntityRC6Vectorffii
                    (*(CBaseEntity **)(puVar17 + -0x20),*(Vector **)(puVar17 + -0x1c),
                     *(float *)(puVar17 + -0x18),*(float *)(puVar17 + -0x14),
                     *(int *)(puVar17 + -0x10),*(int *)(puVar17 + -0xc));
        }
      }
      iVar15 = piVar18[0x18];
      *(int **)(puVar17 + -0xc) = piVar18;
      *(undefined4 *)(puVar17 + -0x10) = *(undefined4 *)(puVar17 + 0x24);
      pcVar3 = *(code **)(iVar15 + 0xf8);
      *(undefined4 *)(puVar17 + -0x14) = 0x7d275;
      (*pcVar3)();
      puVar20 = *(undefined4 **)(puVar17 + 0x20);
      puVar22 = (undefined4 *)(puVar17 + 0xac);
      for (iVar15 = 3; iVar15 != 0; iVar15 = iVar15 + -1) {
        *puVar22 = *puVar20;
        puVar20 = puVar20 + 1;
        puVar22 = puVar22 + 1;
      }
      *(float *)(puVar17 + 0xb4) = fc_1 + *(float *)(puVar17 + 0xb4);
      *(undefined1 **)(puVar17 + -0x14) = puVar17 + 0xbc;
      *(undefined4 *)(puVar17 + -0x18) = *(undefined4 *)(*(int *)(puVar17 + 0x120) + 0x208);
      *(undefined4 *)(puVar17 + -0x1c) = 0;
      *(undefined1 **)(puVar17 + -0x20) = puVar17 + 0xac;
      *(undefined4 *)(puVar17 + -0x24) = *(undefined4 *)(puVar17 + 0x11c);
      *(undefined4 *)(puVar17 + -0x28) = 0x7d2ca;
      UTIL_TraceLine__FRC6VectorT015IGNORE_MONSTERSP7edict_sP11TraceResult();
      pfVar19 = *(float **)(puVar17 + 0x11c);
      fVar1 = *pfVar19;
      fVar2 = pfVar19[1];
      fVar8 = pfVar19[2] - *(float *)(puVar17 + 0xb4);
      *(float *)(puVar17 + 0x7c) = fVar1 - *(float *)(puVar17 + 0xac);
      *(float *)(puVar17 + 0x80) = fVar2 - *(float *)(puVar17 + 0xb0);
      *(float *)(puVar17 + 0x84) = fVar8;
      *(float *)(puVar17 + 0x78) =
           SQRT(fVar8 * fVar8 +
                (fVar2 - *(float *)(puVar17 + 0xb0)) * *(float *)(puVar17 + 0x80) +
                (fVar1 - *(float *)(puVar17 + 0xac)) * *(float *)(puVar17 + 0x7c));
      *(undefined4 *)(puVar17 + 0x28) = *(undefined4 *)(puVar17 + 0x128);
      if ((*(float *)(puVar17 + 0x130) < *(float *)(puVar17 + 0x78)) &&
         (fVar1 = *(float *)(puVar17 + 0x128) -
                  (*(float *)(puVar17 + 0x78) - *(float *)(puVar17 + 0x130)) *
                  *(float *)(puVar17 + 0x24), *(float *)(puVar17 + 0x28) = fVar1, fVar1 < fc_0)) {
        *(float *)(puVar17 + 0x28) = fc_0;
      }
      if ((*(float *)(puVar17 + 0xcc) == (float)dc_1) ||
         (*(int *)(puVar17 + 0xec) == *(int *)(*piVar18 + 0x208))) {
        *(undefined4 *)(puVar17 + -8) = 0x7d3a5;
        ClearMultiDamage__Fv();
        pfVar19 = *(float **)(puVar17 + 0x11c);
        fVar1 = *(float *)(puVar17 + 0xac) - *pfVar19;
        fVar8 = *(float *)(puVar17 + 0xb0) - pfVar19[1];
        fVar2 = *(float *)(puVar17 + 0xb4) - pfVar19[2];
        *(float *)(puVar17 + 0x5c) = fVar1;
        *(float *)(puVar17 + 0x60) = fVar8;
        *(float *)(puVar17 + 100) = fVar2;
        *(float *)(puVar17 + 0x78) = SQRT(fVar2 * fVar2 + fVar8 * fVar8 + fVar1 * fVar1);
        if (*(float *)(puVar17 + 0x78) == fc_0) {
          *(undefined4 *)(puVar17 + 0x7c) = 0;
          *(undefined4 *)(puVar17 + 0x80) = 0;
          *(float *)(puVar17 + 0x84) = fc_1;
        }
        else {
          fVar1 = fc_1 / *(float *)(puVar17 + 0x78);
          *(float *)(puVar17 + 0x7c) = fVar1 * *(float *)(puVar17 + 0x5c);
          *(float *)(puVar17 + 0x80) = fVar1 * *(float *)(puVar17 + 0x60);
          *(float *)(puVar17 + 0x84) = fVar1 * *(float *)(puVar17 + 100);
        }
        iVar15 = piVar18[0x18];
        *(undefined4 *)(puVar17 + -0x10) = *(undefined4 *)(puVar17 + 300);
        *(undefined1 **)(puVar17 + -0x14) = puVar17 + 0xbc;
        *(undefined4 *)(puVar17 + -0x18) = *(undefined4 *)(puVar17 + 0x20);
        *(undefined4 *)(puVar17 + -0x1c) = *(undefined4 *)(puVar17 + 0x28);
LAB_0007d62d:
        *(undefined4 *)(puVar17 + -0x20) = *(undefined4 *)(puVar17 + 0x124);
        *(int **)(puVar17 + -0x24) = piVar18;
        pcVar3 = *(code **)(iVar15 + 0x54);
        *(undefined4 *)(puVar17 + -0x28) = 0x7d638;
        (*pcVar3)();
        *(undefined4 *)(puVar17 + -0x10) = *(undefined4 *)(puVar17 + 0x124);
        *(undefined4 *)(puVar17 + -0x14) = *(undefined4 *)(puVar17 + 0x120);
        *(undefined4 *)(puVar17 + -0x18) = 0x7d64e;
        ApplyMultiDamage__FP9entvars_sT0
                  (*(entvars_s **)(puVar17 + -0x14),*(entvars_s **)(puVar17 + -0x10));
        puVar16 = puVar17 + -4;
      }
      else {
        puVar20 = (undefined4 *)(puVar17 + 0xd0);
        puVar22 = (undefined4 *)(puVar17 + 0x7c);
        for (iVar15 = 3; iVar15 != 0; iVar15 = iVar15 + -1) {
          *puVar22 = *puVar20;
          puVar20 = puVar20 + 1;
          puVar22 = puVar22 + 1;
        }
        if (*(int *)(puVar17 + 0xc0) != 0) {
          puVar20 = *(undefined4 **)(puVar17 + 0x11c);
          puVar22 = (undefined4 *)(puVar17 + 0x7c);
          for (iVar15 = 3; iVar15 != 0; iVar15 = iVar15 + -1) {
            *puVar22 = *puVar20;
            puVar20 = puVar20 + 1;
            puVar22 = puVar22 + 1;
          }
        }
        iVar15 = *piVar18;
        *(undefined1 **)(puVar17 + -0x14) = puVar17 + 0xbc;
        *(undefined4 *)(puVar17 + -0x18) = *(undefined4 *)(iVar15 + 0x208);
        *(undefined4 *)(puVar17 + -0x1c) = 0;
        *(undefined4 *)(puVar17 + -0x20) = *(undefined4 *)(puVar17 + 0x11c);
        *(undefined1 **)(puVar17 + -0x24) = puVar17 + 0xac;
        *(undefined4 *)(puVar17 + -0x28) = 0x7d4db;
        UTIL_TraceLine__FRC6VectorT015IGNORE_MONSTERSP7edict_sP11TraceResult();
        puVar16 = puVar17 + -4;
        if (*(float *)(puVar17 + 0xcc) != (float)dc_1) {
          fVar1 = *(float *)(puVar17 + 0xd8) - *(float *)(puVar17 + 0x84);
          *(float *)(puVar17 + 0x5c) = *(float *)(puVar17 + 0xd0) - *(float *)(puVar17 + 0x7c);
          *(float *)(puVar17 + 0x60) = *(float *)(puVar17 + 0xd4) - *(float *)(puVar17 + 0x80);
          *(float *)(puVar17 + 100) = fVar1;
          *(float *)(puVar17 + 0x1c) =
               SQRT(fVar1 * fVar1 +
                    (*(float *)(puVar17 + 0xd4) - *(float *)(puVar17 + 0x80)) *
                    *(float *)(puVar17 + 0x60) +
                    (*(float *)(puVar17 + 0xd0) - *(float *)(puVar17 + 0x7c)) *
                    *(float *)(puVar17 + 0x5c));
          puVar16 = puVar17 + -4;
          if (*(float *)(puVar17 + 0x1c) < *(float *)(puVar17 + 0x28)) {
            *(undefined4 *)(puVar17 + -8) = 0x7d560;
            ClearMultiDamage__Fv();
            pfVar19 = *(float **)(puVar17 + 0x11c);
            fVar1 = *(float *)(puVar17 + 0xac) - *pfVar19;
            fVar8 = *(float *)(puVar17 + 0xb0) - pfVar19[1];
            fVar2 = *(float *)(puVar17 + 0xb4) - pfVar19[2];
            *(float *)(puVar17 + 0x4c) = fVar1;
            *(float *)(puVar17 + 0x50) = fVar8;
            *(float *)(puVar17 + 0x54) = fVar2;
            *(float *)(puVar17 + 0x78) = SQRT(fVar2 * fVar2 + fVar8 * fVar8 + fVar1 * fVar1);
            if (*(float *)(puVar17 + 0x78) == fc_0) {
              *(undefined4 *)(puVar17 + 0x5c) = 0;
              *(undefined4 *)(puVar17 + 0x60) = 0;
              *(float *)(puVar17 + 100) = fc_1;
            }
            else {
              fVar1 = fc_1 / *(float *)(puVar17 + 0x78);
              *(float *)(puVar17 + 0x5c) = fVar1 * *(float *)(puVar17 + 0x4c);
              *(float *)(puVar17 + 0x60) = fVar1 * *(float *)(puVar17 + 0x50);
              *(float *)(puVar17 + 100) = fVar1 * *(float *)(puVar17 + 0x54);
            }
            iVar15 = piVar18[0x18];
            *(undefined4 *)(puVar17 + -0x10) = *(undefined4 *)(puVar17 + 300);
            *(undefined1 **)(puVar17 + -0x14) = puVar17 + 0xbc;
            *(undefined1 **)(puVar17 + -0x18) = puVar17 + 0x5c;
            *(float *)(puVar17 + -0x1c) = *(float *)(puVar17 + 0x28) - *(float *)(puVar17 + 0x1c);
            goto LAB_0007d62d;
          }
        }
      }
    }
    ppgVar10 = PTR_gpGlobals_00177410;
    *(int **)(puVar16 + 0x1c) = piVar18;
    pcVar4 = (*PTR_gpGlobals_00177410)->pStringBase;
    iVar15 = *(int *)*piVar18;
    *(char **)(puVar16 + -0xc) = "WorldGun";
    *(char **)(puVar16 + -0x10) = pcVar4 + iVar15;
    *(undefined4 *)(puVar16 + -0x14) = 0x7d678;
    iVar15 = strcmp(*(char **)(puVar16 + -0x10),*(char **)(puVar16 + -0xc));
    puVar17 = puVar16;
    if (iVar15 == 0) {
      *(int *)(puVar16 + -0xc) = *piVar18 + 8;
      *(int *)(puVar16 + -0x10) = *piVar18;
      *(undefined4 *)(puVar16 + -0x14) = 0x7d695;
      UTIL_SetOrigin__FP9entvars_sRC6Vector
                (*(entvars_s **)(puVar16 + -0x10),*(Vector **)(puVar16 + -0xc));
      fVar9 = fc_0;
      *(float *)(puVar16 + 0x80) = fc_0;
      *(float *)(puVar16 + 0x84) = fc_0;
      *(float *)(puVar16 + 0x88) = fc_0;
      *(float *)(puVar16 + 0x50) = fc_0;
      *(float *)(puVar16 + 0x54) = fc_0;
      *(float *)(puVar16 + 0x58) = fc_0;
      *(undefined4 *)(puVar16 + -8) = *(undefined4 *)(puVar16 + 0x24);
      *(undefined4 *)(puVar16 + -0xc) = *(undefined4 *)(puVar16 + 0x18);
      *(int *)(puVar16 + -0x10) = *piVar18;
      *(undefined4 *)(puVar16 + -0x14) = 0x7d6cf;
      UTIL_SetSize__FP9entvars_sRC6VectorT1
                (*(entvars_s **)(puVar16 + -0x10),*(Vector **)(puVar16 + -0xc),
                 *(Vector **)(puVar16 + -8));
      puVar12 = PTR_BounceTouch_00177b4c;
      piVar18[5] = -0x10000;
      piVar18[6] = (int)puVar12;
      puVar12 = PTR_Fall_00177b3c;
      piVar18[3] = -0x10000;
      piVar18[4] = (int)puVar12;
      *(float *)(*piVar18 + 0x104) = (float)dc_0_1 + (*ppgVar10)->time;
      *(undefined4 *)(*piVar18 + 0x108) = 10;
      *(undefined4 *)(*piVar18 + 0x10c) = 2;
      *(undefined4 *)(*piVar18 + 0x11c) = 0x3f333333;
      *(undefined4 *)(*piVar18 + 0x120) = 0x3f4ccccd;
      iVar15 = *piVar18;
      fVar1 = *(float *)(iVar15 + 8) - *(float *)(puVar16 + 0xa0);
      fVar8 = *(float *)(iVar15 + 0xc) - *(float *)(puVar16 + 0xa4);
      fVar2 = *(float *)(iVar15 + 0x10) - *(float *)(puVar16 + 0xa8);
      *(float *)(puVar16 + 0x80) = fVar1;
      *(float *)(puVar16 + 0x84) = fVar8;
      *(float *)(puVar16 + 0x88) = fVar2;
      iVar15 = *piVar18;
      pfVar19 = *(float **)(puVar16 + 0x24);
      *(float *)(puVar16 + 0x7c) = SQRT(fVar2 * fVar2 + fVar8 * fVar8 + fVar1 * fVar1);
      *(float *)(puVar16 + 0xc) = fVar9;
      if (*(float *)(puVar16 + 0x7c) == *(float *)(puVar16 + 0xc)) {
        *(undefined4 *)(puVar16 + 0x30) = 0;
        *(undefined4 *)(puVar16 + 0x34) = 0;
        *(float *)(puVar16 + 0x38) = fc_1;
      }
      else {
        fVar8 = fc_1 / *(float *)(puVar16 + 0x7c);
        fVar1 = pfVar19[1];
        fVar2 = pfVar19[2];
        *(float *)(puVar16 + 0x30) = fVar8 * *pfVar19;
        *(float *)(puVar16 + 0x34) = fVar8 * fVar1;
        *(float *)(puVar16 + 0x38) = fVar8 * fVar2;
      }
      *(float *)(puVar16 + 0x7c) =
           SQRT(*(float *)(puVar16 + 0x88) * *(float *)(puVar16 + 0x88) +
                *(float *)(puVar16 + 0x84) * *(float *)(puVar16 + 0x84) +
                *(float *)(puVar16 + 0x80) * *(float *)(puVar16 + 0x80));
      fVar1 = *(float *)(puVar16 + 0x138) - *(float *)(puVar16 + 0x7c);
      *(float *)(puVar16 + 0x40) = fVar1 * *(float *)(puVar16 + 0x30);
      *(float *)(puVar16 + 0x44) = fVar1 * *(float *)(puVar16 + 0x34);
      *(float *)(puVar16 + 0x48) = fVar1 * *(float *)(puVar16 + 0x38);
      *(undefined4 *)(puVar16 + -0x10) = *(undefined4 *)(*(int *)(puVar16 + 0x1c) + 0x88);
      *(undefined4 *)(puVar16 + -0x14) = 0x7d85c;
      iVar14 = get_weapon_info__F10tsweapon_t();
      *(ushort *)(puVar16 + -2) = (ushort)*(byte *)(iVar14 + 0xd0);
      fVar1 = (float)*(short *)(puVar16 + -2);
      *(float *)(puVar16 + 0x60) = *(float *)(puVar16 + 0x40) / fVar1;
      *(float *)(puVar16 + 100) = *(float *)(puVar16 + 0x44) / fVar1;
      *(float *)(puVar16 + 0x68) = *(float *)(puVar16 + 0x48) / fVar1;
      *(float *)(puVar16 + 0x50) = fc_10 * *(float *)(puVar16 + 0x60);
      *(float *)(puVar16 + 0x54) = fc_10 * *(float *)(puVar16 + 100);
      *(float *)(puVar16 + 0x58) = fc_10 * *(float *)(puVar16 + 0x68);
      puVar20 = *(undefined4 **)(puVar16 + 0x18);
      puVar22 = (undefined4 *)(iVar15 + 0x20);
      for (iVar14 = 3; iVar14 != 0; iVar14 = iVar14 + -1) {
        *puVar22 = *puVar20;
        puVar20 = puVar20 + 1;
        puVar22 = puVar22 + 1;
      }
      piVar5 = *(int **)(puVar16 + 0x1c);
      iVar15 = *piVar5;
      uVar6 = *(uint *)(iVar15 + 0x1a4);
      if ((uVar6 & 0x200) != 0) {
        *(uint *)(iVar15 + 0x1a4) = uVar6 & 0xfffffdff;
        iVar15 = *piVar5;
        *(float *)(iVar15 + 0x28) = ABS(*(float *)(iVar15 + 0x28)) + (float)dc_50;
      }
      peVar11 = PTR_g_engfuncs_001778b8;
      *(undefined4 *)(puVar16 + -0xc) = 300;
      *(undefined4 *)(puVar16 + -0x10) = 0xfffffed4;
      peVar7 = PTR_g_engfuncs_001778b8->pfnRandomLong;
      *(undefined4 *)(puVar16 + -0x14) = 0x7d910;
      uVar13 = (*peVar7)();
      iVar15 = **(int **)(puVar16 + 0x1c);
      *(uint *)(puVar16 + -0x14) = CONCAT31(extraout_var,uVar13);
      *(float *)(iVar15 + 0x60) = (float)*(int *)(puVar16 + -0x14);
      *(undefined4 *)(puVar16 + -0xc) = 300;
      *(undefined4 *)(puVar16 + -0x10) = 0xfffffed4;
      peVar7 = peVar11->pfnRandomLong;
      *(undefined4 *)(puVar16 + -0x14) = 0x7d930;
      uVar13 = (*peVar7)();
      iVar15 = **(int **)(puVar16 + 0x1c);
      *(uint *)(puVar16 + -0x14) = CONCAT31(extraout_var_00,uVar13);
      *(float *)(iVar15 + 100) = (float)*(int *)(puVar16 + -0x14);
    }
  } while( true );
}


