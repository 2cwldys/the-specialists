// ===== FUN @ 000824a4 size=52 =====

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */
/* WARNING: Unknown calling convention -- yet parameter storage is locked */
/* TS_gun.cpp */

void CTSGun::StandardFire(CTSGun *this)

{
  int *piVar1;
  float fVar2;
  float fVar3;
  float fVar4;
  entvars_t *peVar5;
  undefined4 uVar6;
  Vector a1;
  Vector a2;
  Vector a3;
  Vector a2_00;
  int iVar7;
  bool a2_01;
  CBaseEntity *pCVar8;
  Vector *pVVar9;
  CBasePlayer *pCVar10;
  float fVar11;
  vec_t *pvVar12;
  undefined4 *puVar13;
  longdouble lVar14;
  undefined2 uVar15;
  undefined2 uVar16;
  float in_stack_ffffff6c;
  int iVar17;
  int in_stack_ffffff74;
  vec_t in_stack_ffffff78;
  TraceResult *in_stack_ffffff7c;
  int in_stack_ffffff80;
  float fStack_70;
  float fStack_6c;
  float fStack_68;
  undefined4 auStack_60 [4];
  undefined4 auStack_50 [7];
  float fStack_34;
  entvars_t *apeStack_30 [8];
  
  iVar17 = 0x824b0;
  if (__tmp_6_1323 == 0) {
    __tmp_6_1323 = 1;
  }
  if (__tmp_7_1325 == 0) {
    __tmp_7_1325 = 1;
  }
  if (__tmp_8_1327 == 0) {
    vecDir_1326._0_4_ = *(undefined4 *)PTR_g_vecZero_00177698;
    vecDir_1326._4_4_ = *(undefined4 *)(PTR_g_vecZero_00177698 + 4);
    vecDir_1326._8_4_ = *(undefined4 *)(PTR_g_vecZero_00177698 + 8);
    __tmp_8_1327 = 1;
  }
  pCVar8 = this->m_pPlayer;
  uVar15 = SUB42(pCVar8,0);
  uVar16 = (undefined2)((uint)pCVar8 >> 0x10);
  a2_00.y = (vec_t)in_stack_ffffff74;
  a2_00.x = 7.47845e-40;
  a2_00.z = in_stack_ffffff78;
  (*pCVar8->vtbl[1].TraceAttack)
            ((CBaseEntity *)(apeStack_30 + 1),(entvars_s *)pCVar8,in_stack_ffffff6c,a2_00,
             in_stack_ffffff7c,in_stack_ffffff80);
  fVar11 = (float)CONCAT22(uVar16,uVar15);
  pCVar8 = (CBaseEntity *)(apeStack_30 + 1);
  puVar13 = (undefined4 *)vecSrc_1322;
  for (iVar7 = 3; iVar7 != 0; iVar7 = iVar7 + -1) {
    *puVar13 = pCVar8->pev;
    pCVar8 = (CBaseEntity *)&pCVar8->m_pGoalEnt;
    puVar13 = puVar13 + 1;
  }
  pVVar9 = &(*PTR_gpGlobals_00177410)->v_forward;
  pvVar12 = (vec_t *)vecAiming_1324;
  for (iVar7 = 3; iVar7 != 0; iVar7 = iVar7 + -1) {
    *pvVar12 = pVVar9->x;
    pVVar9 = (Vector *)&pVVar9->y;
    pvVar12 = pvVar12 + 1;
  }
  spread_1329 = this->m_pActiveInfo->flSpread;
  if (this->m_weaponType == 0x12) {
    peVar5 = this->m_pPlayer->pev;
    fVar2 = (peVar5->velocity).x;
    fVar3 = (peVar5->velocity).y;
    fVar4 = (peVar5->velocity).z;
    fStack_34 = SQRT(fVar4 * fVar4 + fVar3 * fVar3 + fVar2 * fVar2);
    if (fc_35 < fStack_34) {
      *(undefined4 *)&this->m_pPlayer->field_0x778 = 0x3e99999a;
    }
  }
  if (this->m_weaponType == 0x20) {
    peVar5 = this->m_pPlayer->pev;
    fVar2 = (peVar5->velocity).x;
    fVar3 = (peVar5->velocity).y;
    fVar4 = (peVar5->velocity).z;
    fStack_34 = SQRT(fVar4 * fVar4 + fVar3 * fVar3 + fVar2 * fVar2);
    if (fStack_34 <= _UNK_00107f2c) goto LAB_00082638;
    in_stack_ffffff6c = 0.0;
    if (((this->m_pActiveItem->attachEquipped & 1) != 0) &&
       ((((byte)this->m_pActiveInfo->attachBuiltin ^ 1) & 1) != 0)) {
      in_stack_ffffff6c = 1.4013e-45;
    }
    fVar11 = fc_5 * this->m_pActiveInfo->flRecoil;
LAB_00082674:
    ScaleSpread(this,&spread_1329,fVar11,(int)in_stack_ffffff6c);
  }
  else {
LAB_00082638:
    if ((char)this->m_pActiveInfo->bUsesSpread != '\0') {
      in_stack_ffffff6c = 0.0;
      if (((this->m_pActiveItem->attachEquipped & 1) != 0) &&
         ((((byte)this->m_pActiveInfo->attachBuiltin ^ 1) & 1) != 0)) {
        in_stack_ffffff6c = 1.4013e-45;
      }
      fVar11 = this->m_pActiveInfo->flRecoil;
      goto LAB_00082674;
    }
  }
  fVar2 = (float)this->m_pActiveInfo->flDamage;
  if (((this->m_pActiveItem->attachEquipped & 1) != 0) &&
     ((((byte)this->m_pActiveInfo->attachBuiltin ^ 1) & 1) != 0)) {
    fVar2 = fVar2 * fc_0_7749999761581421;
  }
  auStack_50[0] = vecSrc_1322._0_4_;
  auStack_50[1] = vecSrc_1322._4_4_;
  auStack_50[2] = vecSrc_1322._8_4_;
  auStack_60[0] = vecAiming_1324._0_4_;
  auStack_60[1] = vecAiming_1324._4_4_;
  auStack_60[2] = vecAiming_1324._8_4_;
  fStack_70 = spread_1329;
  uVar6 = *(undefined4 *)&this->m_pActiveInfo->field_0x20;
  a1.y = (vec_t)auStack_50;
  a1.x = (vec_t)(uint)(byte)this->m_pActiveInfo->iPellets;
  a1.z = (vec_t)auStack_60;
  a2.y = fVar2;
  a2.x = (vec_t)&fStack_70;
  a2.z._0_1_ = (char)uVar6;
  a2.z._1_1_ = (char)((uint)uVar6 >> 8);
  a2.z._2_2_ = (short)((uint)uVar6 >> 0x10);
  a3.y = (vec_t)*(undefined4 *)&this->m_pPlayer->field_0x7ec;
  a3.x = (vec_t)(uint)(byte)this->m_pActiveInfo->iCaliber;
  a3.z._0_2_ = SUB42(fVar11,0);
  a3.z._2_2_ = (short)((uint)fVar11 >> 0x10);
  fStack_6c = fStack_70;
  fStack_68 = fStack_70;
  CBasePlayer::ShootBulletsPlayer
            ((CBasePlayer *)apeStack_30,(byte)this->m_pPlayer,a1,a2,a3,in_stack_ffffff6c,iVar17,
             in_stack_ffffff74,(int)in_stack_ffffff78);
  pCVar10 = (CBasePlayer *)apeStack_30;
  puVar13 = (undefined4 *)vecDir_1326;
  for (iVar17 = 3; iVar17 != 0; iVar17 = iVar17 + -1) {
    *puVar13 = pCVar10->pev;
    pCVar10 = (CBasePlayer *)&pCVar10->field_0x4;
    puVar13 = puVar13 + 1;
  }
  if (((this->m_pActiveInfo->iFireType == '\a') || (this->m_weaponType == 0x1f)) ||
     (this->m_weaponType == 0x21)) {
    uVar15 = 1;
  }
  else {
    if (this->m_pActiveInfo->field_0x38 == '\0') goto LAB_000827fd;
    uVar15 = 0;
  }
  a2_01 = false;
  if (((this->m_pActiveItem->attachEquipped & 1) != 0) &&
     ((((byte)this->m_pActiveInfo->attachBuiltin ^ 1) & 1) != 0)) {
    a2_01 = true;
  }
  VKickback(this,this->m_pActiveInfo->flViewKick,(uint)(byte)this->m_pActiveInfo->iFireType,a2_01,
            (float)vecDir_1326._0_4_,(float)vecDir_1326._4_4_,
            (float)CONCAT22((short)((uint)fVar11 >> 0x10),uVar15));
LAB_000827fd:
  iVar17 = InfiniteClips__FP11CBasePlayer((CBasePlayer *)this->m_pPlayer);
  if (iVar17 == 0) {
    piVar1 = &this->m_pActiveItem->iClip;
    *piVar1 = *piVar1 + -1;
    if (this->m_pActiveInfo->iAkimboType != -1) {
      (&this->field_0x118)[this->m_pActiveInfo->iAkimboType * 0xe] =
           (&this->field_0x118)[this->m_pActiveInfo->iAkimboType * 0xe] + -1;
      (&this->field_0x130)[this->m_pActiveInfo->iAkimboType * 0xe] = this->m_pActiveItem->iClip;
      iVar17 = this->m_pActiveInfo->iAkimboType;
      if ((&this->field_0x118)[iVar17 * 0xe] < 0) {
        (&this->field_0x118)[iVar17 * 0xe] = 0;
      }
    }
  }
  bparam1_1328 = this->m_pActiveItem->iClip == 0;
  (*PTR_g_engfuncs_001778b8->pfnPlaybackEvent)
            (1,this->m_pPlayer->pev->pContainingEntity,(uint)*(ushort *)&this->field_0x964,0,
             PTR_g_vecZero_00177698,PTR_g_vecZero_00177698,spread_1329,SUB41(spread_1329,0),
             this->m_weaponType,this->m_pActiveItem->attachEquipped,(ushort)(byte)bparam1_1328,
             *(undefined4 *)&this->m_pPlayer->field_0x7ec);
  if (this->m_pActiveItem->fireMode == '\x02') {
    rate_1330 = _UNK_00107f34;
    if (this->m_pActiveItem->burstOrPhase == 1) {
      rate_1330 = fc_0_20000000298023224;
    }
  }
  else if ((this->m_pActiveItem->fireMode == '\x01') && ((this->m_pActiveInfo->iFireModes & 1) != 0)
          ) {
    rate_1330 = 0.1;
  }
  else {
    rate_1330 = this->m_pActiveInfo->flFireRate;
  }
  lVar14 = (longdouble)UTIL_WeaponTimeBase__Fv();
  this->field_0x9c = (int)(float)(lVar14 + (longdouble)rate_1330);
  fVar11 = fc_2;
  if (((this->m_weaponType == 0x24) && (this->m_pActiveItem->iClip < 1)) &&
     (iVar17 = InfiniteClips__FP11CBasePlayer((CBasePlayer *)this->m_pPlayer), iVar17 == 0)) {
    fVar11 = fc_0_5;
  }
  lVar14 = (longdouble)UTIL_WeaponTimeBase__Fv();
  this->field_0xa8 = (int)(float)((longdouble)fVar11 + lVar14);
  return;
}


