// ===== FUN @ 000829c0 size=299 =====

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */
/* WARNING: Unknown calling convention -- yet parameter storage is locked */
/* TS_gun.cpp */

undefined4 CTSGun::AkimboFire(CTSGun *this)

{
  int *piVar1;
  float fVar2;
  globalvars_t *pgVar3;
  edict_t *peVar4;
  engine_fn *peVar5;
  bool bVar6;
  longdouble lVar7;
  Vector a1;
  Vector a1_00;
  Vector a1_01;
  Vector a2;
  Vector a2_00;
  Vector a2_01;
  Vector a3;
  Vector a3_00;
  Vector a3_01;
  Vector a2_02;
  Vector a2_03;
  Vector a2_04;
  bool bVar8;
  globalvars_t **a4;
  uint uVar9;
  int iVar10;
  char cVar11;
  ushort uVar12;
  undefined4 extraout_EDX;
  undefined4 extraout_EDX_00;
  undefined4 uVar13;
  undefined4 *puVar14;
  TraceResult *pTVar15;
  CBasePlayer *pCVar16;
  Vector *pVVar17;
  CBaseEntity *pCVar18;
  undefined4 *puVar19;
  vec_t *pvVar20;
  undefined4 *puVar21;
  longdouble lVar22;
  longdouble extraout_ST0;
  CBaseEntity *pCVar23;
  float in_stack_ffffff1c;
  int iVar24;
  vec_t in_stack_ffffff24;
  CBasePlayer *in_stack_ffffff28;
  TraceResult *a3_02;
  TraceResult *in_stack_ffffff2c;
  int in_stack_ffffff30;
  undefined4 *puStack_bc;
  undefined4 *puStack_a0;
  float *pfStack_88;
  undefined4 *puStack_80;
  entvars_t *peStack_74;
  undefined4 uStack_70;
  CBaseEntity *apCStack_6c [2];
  int iStack_64;
  entvars_t *peStack_60;
  float fStack_5c;
  void *pvStack_58;
  int iStack_54;
  void *pvStack_50;
  entvars_t *peStack_4c;
  CBaseEntity *pCStack_48;
  void *pvStack_44;
  void *apvStack_40 [8];
  float fStack_20;
  
  iVar24 = 0x829cf;
  if (__tmp_9_1335 == 0) {
    __tmp_9_1335 = 1;
  }
  if (__tmp_10_1337 == 0) {
    __tmp_10_1337 = 1;
  }
  if (__tmp_11_1339 == 0) {
    __tmp_11_1339 = 1;
  }
  bVar6 = 1 < (byte)(this->m_pActiveItem->fireMode - 4U);
  bVar8 = false;
  if (!bVar6) {
    if (this->m_pActiveItem->fireMode == '\x05') {
      uVar9 = this->m_pPlayer->pev->button;
    }
    else {
      uVar9 = *(uint *)&this->m_pPlayer->field_0x814;
    }
    bVar8 = false;
    if (((uVar9 & 1) != 0) && ((uVar9 & 0x800) != 0)) {
      bVar8 = true;
    }
  }
  cVar11 = '\0';
  if ((bVar8) || (this->m_pActiveItem->fireMode == '\0')) {
    cVar11 = '\x01';
  }
  uStack_70 = (CBaseEntity *)CONCAT13(cVar11,(uint3)uStack_70);
  if ((bVar6) || (cVar11 != '\0')) {
    if (this->m_pActiveItem->pumpToggle == '\0') {
      this->m_pActiveItem->pumpToggle = '\x01';
    }
    else {
      this->m_pActiveItem->pumpToggle = '\0';
    }
    if (this->m_pActiveItem->rightClip < 1) {
      uStack_70 = (CBaseEntity *)(uint)(uint3)uStack_70;
      this->m_pActiveItem->pumpToggle = '\x01';
    }
    if (this->m_pActiveItem->leftClip < 1) {
      uStack_70 = (CBaseEntity *)((uint)uStack_70 & 0xffffff);
      this->m_pActiveItem->pumpToggle = '\0';
    }
  }
  else {
    if ((this->m_pPlayer->pev->button & 1) != 0) {
      if (this->m_pActiveItem->rightClip < 1) goto LAB_00082ad8;
      this->m_pActiveItem->pumpToggle = '\0';
    }
    if ((this->m_pPlayer->pev->button & 0x800) != 0) {
      if (this->m_pActiveItem->leftClip < 1) {
LAB_00082ad8:
        EmptySound(this);
        return 0;
      }
      this->m_pActiveItem->pumpToggle = '\x01';
    }
  }
  iVar10 = InfiniteClips__FP11CBasePlayer((CBasePlayer *)this->m_pPlayer);
  if (iVar10 != 0) goto LAB_00082bf8;
  if (uStack_70._3_1_ == '\0') {
    piVar1 = &this->m_pActiveItem->iClip;
    *piVar1 = *piVar1 + -1;
    if (this->m_pActiveItem->pumpToggle == '\0') goto LAB_00082b88;
    piVar1 = &this->m_pActiveItem->leftClip;
    *piVar1 = *piVar1 + -1;
  }
  else {
    piVar1 = &this->m_pActiveItem->iClip;
    *piVar1 = *piVar1 + -2;
    piVar1 = &this->m_pActiveItem->leftClip;
    *piVar1 = *piVar1 + -1;
LAB_00082b88:
    piVar1 = &this->m_pActiveItem->rightClip;
    *piVar1 = *piVar1 + -1;
  }
  if (this->m_pActiveItem->iClip < 0) {
    this->m_pActiveItem->iClip = 0;
  }
  if (this->m_pActiveItem->leftClip < 0) {
    this->m_pActiveItem->leftClip = 0;
  }
  if (this->m_pActiveItem->rightClip < 0) {
    this->m_pActiveItem->rightClip = 0;
  }
  if (this->m_pActiveInfo->iFireType == '\x04') {
    (&this->field_0x118)[this->m_pActiveInfo->iSingleType * 0xe] = this->m_pActiveItem->leftClip;
  }
LAB_00082bf8:
  spread_1341 = (void *)this->m_pActiveInfo->flSpread;
  if ((char)this->m_pActiveInfo->bUsesSpread != '\0') {
    iVar24 = 0;
    if (((this->m_pActiveItem->attachEquipped & 1) != 0) &&
       ((((byte)this->m_pActiveInfo->attachBuiltin ^ 1) & 1) != 0)) {
      iVar24 = 1;
    }
    in_stack_ffffff1c = this->m_pActiveInfo->flRecoil;
    if (uStack_70._3_1_ != '\0') {
      in_stack_ffffff1c = in_stack_ffffff1c + in_stack_ffffff1c;
    }
    fStack_20 = in_stack_ffffff1c;
    ScaleSpread(this,(float *)&spread_1341,in_stack_ffffff1c,iVar24);
  }
  a4 = PTR_gpGlobals_00177410;
  dmg_1343 = (float)this->m_pActiveInfo->flDamage;
  if (((this->m_pActiveItem->attachEquipped & 1) != 0) &&
     ((((byte)this->m_pActiveInfo->attachBuiltin ^ 1) & 1) != 0)) {
    dmg_1343 = dmg_1343 * fc_0_7749999761581421;
  }
  if (uStack_70._3_1_ == '\0') {
    pVVar17 = &(*PTR_gpGlobals_00177410)->v_forward;
    pvVar20 = (vec_t *)vecAiming_1336;
    for (iVar10 = 3; iVar10 != 0; iVar10 = iVar10 + -1) {
      *pvVar20 = pVVar17->x;
      pVVar17 = (Vector *)&pVVar17->y;
      pvVar20 = pvVar20 + 1;
    }
    pCVar23 = this->m_pPlayer;
    a2_04.y = in_stack_ffffff24;
    a2_04.x = (vec_t)iVar24;
    a2_04.z = (vec_t)in_stack_ffffff28;
    (*pCVar23->vtbl[1].TraceAttack)
              ((CBaseEntity *)apCStack_6c,(entvars_s *)pCVar23,in_stack_ffffff1c,a2_04,
               (TraceResult *)apCStack_6c,in_stack_ffffff30);
    pCVar18 = (CBaseEntity *)apCStack_6c;
    for (iVar10 = 3; iVar10 != 0; iVar10 = iVar10 + -1) {
      *puStack_bc = pCVar18->pev;
      pCVar18 = (CBaseEntity *)&pCVar18->m_pGoalEnt;
      puStack_bc = puStack_bc + 1;
    }
    peStack_60 = (entvars_t *)vecSrc_1334._0_4_;
    fStack_5c = (float)vecSrc_1334._4_4_;
    pvStack_58 = (void *)vecSrc_1334._8_4_;
    pvStack_50 = (void *)vecAiming_1336._0_4_;
    peStack_4c = (entvars_t *)vecAiming_1336._4_4_;
    pCStack_48 = (CBaseEntity *)vecAiming_1336._8_4_;
    apvStack_40[0] = spread_1341;
    a1_01.y = (vec_t)&peStack_60;
    a1_01.x = (vec_t)(uint)(byte)this->m_pActiveInfo->iPellets;
    a1_01.z = (vec_t)&pvStack_50;
    a2_01.y = dmg_1343;
    a2_01.x = (vec_t)apvStack_40;
    a2_01.z = *(vec_t *)&this->m_pActiveInfo->field_0x20;
    a3_01.y = (vec_t)*(undefined4 *)&this->m_pPlayer->field_0x7ec;
    a3_01.x = (vec_t)(uint)(byte)this->m_pActiveInfo->iCaliber;
    a3_01.z = (vec_t)pCVar23;
    apvStack_40[1] = apvStack_40[0];
    apvStack_40[2] = apvStack_40[0];
    CBasePlayer::ShootBulletsPlayer
              (in_stack_ffffff28,(byte)this->m_pPlayer,a1_01,a2_01,a3_01,in_stack_ffffff1c,iVar24,
               (int)in_stack_ffffff24,(int)in_stack_ffffff28);
    puVar14 = (undefined4 *)&stack0xfffffef4;
    uVar13 = extraout_EDX_00;
  }
  else {
    peStack_74 = (entvars_t *)vecAiming_1336;
    pVVar17 = &(*PTR_gpGlobals_00177410)->v_forward;
    pvVar20 = (vec_t *)peStack_74;
    for (iVar10 = 3; iVar10 != 0; iVar10 = iVar10 + -1) {
      *pvVar20 = pVVar17->x;
      pVVar17 = (Vector *)&pVVar17->y;
      pvVar20 = pvVar20 + 1;
    }
    pCVar23 = this->m_pPlayer;
    a2_03.y = in_stack_ffffff24;
    a2_03.x = (vec_t)iVar24;
    a2_03.z = (vec_t)in_stack_ffffff28;
    (*pCVar23->vtbl[1].TraceAttack)
              ((CBaseEntity *)&peStack_4c,(entvars_s *)pCVar23,in_stack_ffffff1c,a2_03,
               in_stack_ffffff2c,(int)a4);
    iVar10 = *(int *)in_stack_ffffff2c;
    fVar2 = fc_5;
    if (this->m_pActiveItem->pumpToggle != '\0') {
      fVar2 = _UNK_00107f38;
    }
    peStack_60 = (entvars_t *)(fVar2 * *(float *)(iVar10 + 0x40));
    fStack_5c = fVar2 * *(float *)(iVar10 + 0x44);
    pvStack_58 = (void *)(fVar2 * *(float *)(iVar10 + 0x48));
    a3_02 = (TraceResult *)apvStack_40;
    apvStack_40[0] = (void *)((float)peStack_4c + (float)peStack_60);
    apvStack_40[1] = (void *)((float)pCStack_48 + fStack_5c);
    apvStack_40[2] = (void *)((float)pvStack_44 + (float)pvStack_58);
    pTVar15 = a3_02;
    for (iVar10 = 3; iVar10 != 0; iVar10 = iVar10 + -1) {
      *puStack_80 = *(void **)pTVar15;
      pTVar15 = pTVar15 + 4;
      puStack_80 = puStack_80 + 1;
    }
    pvStack_50 = (void *)vecSrc_1334._0_4_;
    peStack_4c = (entvars_t *)vecSrc_1334._4_4_;
    pCStack_48 = (CBaseEntity *)vecSrc_1334._8_4_;
    apvStack_40[0] = (void *)vecAiming_1336._0_4_;
    apvStack_40[1] = (void *)vecAiming_1336._4_4_;
    apvStack_40[2] = (void *)vecAiming_1336._8_4_;
    uStack_70 = spread_1341;
    a1_00.y = (vec_t)&pvStack_50;
    a1_00.x = (vec_t)(uint)(byte)this->m_pActiveInfo->iPellets;
    a1_00.z = (vec_t)a3_02;
    a2_00.y = dmg_1343;
    a2_00.x = (vec_t)&uStack_70;
    a2_00.z = *(vec_t *)&this->m_pActiveInfo->field_0x20;
    a3_00.y = (vec_t)*(undefined4 *)&this->m_pPlayer->field_0x7ec;
    a3_00.x = (vec_t)(uint)(byte)this->m_pActiveInfo->iCaliber;
    a3_00.z = (vec_t)pCVar23;
    apCStack_6c[0] = uStack_70;
    apCStack_6c[1] = uStack_70;
    CBasePlayer::ShootBulletsPlayer
              ((CBasePlayer *)&peStack_60,(byte)this->m_pPlayer,a1_00,a2_00,a3_00,in_stack_ffffff1c,
               iVar24,(int)in_stack_ffffff24,(int)a3_02);
    pCVar16 = (CBasePlayer *)&peStack_60;
    puVar19 = (undefined4 *)vecDir_1338;
    for (iVar10 = 3; iVar10 != 0; iVar10 = iVar10 + -1) {
      *puVar19 = pCVar16->pev;
      pCVar16 = (CBasePlayer *)&pCVar16->field_0x4;
      puVar19 = puVar19 + 1;
    }
    pCVar18 = this->m_pPlayer;
    a2_02.y = (vec_t)iVar24;
    a2_02.x = in_stack_ffffff1c;
    a2_02.z = in_stack_ffffff24;
    (*pCVar18->vtbl[1].TraceAttack)
              ((CBaseEntity *)&peStack_60,(entvars_s *)pCVar18,(float)pCVar23,a2_02,a3_02,
               (int)in_stack_ffffff2c);
    pgVar3 = *PTR_gpGlobals_00177410;
    fVar2 = _UNK_00107f38;
    if (this->m_pActiveItem->pumpToggle != '\0') {
      fVar2 = fc_5;
    }
    peStack_74 = (entvars_t *)(*pfStack_88 + fVar2 * (pgVar3->v_right).x);
    uStack_70 = (CBaseEntity *)(pfStack_88[1] + fVar2 * (pgVar3->v_right).y);
    apCStack_6c[0] = (CBaseEntity *)(pfStack_88[2] + fVar2 * (pgVar3->v_right).z);
    pCVar16 = (CBasePlayer *)&peStack_74;
    for (iVar10 = 3; iVar10 != 0; iVar10 = iVar10 + -1) {
      *puStack_a0 = pCVar16->pev;
      pCVar16 = (CBasePlayer *)&pCVar16->field_0x4;
      puStack_a0 = puStack_a0 + 1;
    }
    iStack_64 = vecSrc_1334._0_4_;
    peStack_60 = (entvars_t *)vecSrc_1334._4_4_;
    fStack_5c = (float)vecSrc_1334._8_4_;
    iStack_54 = vecAiming_1336._0_4_;
    pvStack_50 = (void *)vecAiming_1336._4_4_;
    peStack_4c = (entvars_t *)vecAiming_1336._8_4_;
    pvStack_44 = spread_1341;
    a1.y = (vec_t)&iStack_64;
    a1.x = (vec_t)(uint)(byte)this->m_pActiveInfo->iPellets;
    a1.z = (vec_t)&iStack_54;
    a2.y = dmg_1343;
    a2.x = (vec_t)&pvStack_44;
    a2.z = *(vec_t *)&this->m_pActiveInfo->field_0x20;
    a3.y = (vec_t)(*(int *)&this->m_pPlayer->field_0x7ec + 0x21);
    a3.x = (vec_t)(uint)(byte)this->m_pActiveInfo->iCaliber;
    a3.z = (vec_t)pCVar18;
    apvStack_40[0] = pvStack_44;
    apvStack_40[1] = pvStack_44;
    CBasePlayer::ShootBulletsPlayer
              ((CBasePlayer *)&peStack_74,(byte)this->m_pPlayer,a1,a2,a3,(float)pCVar23,
               (int)in_stack_ffffff1c,iVar24,(int)&peStack_74);
    puVar14 = (undefined4 *)&stack0xfffffef0;
    uVar13 = extraout_EDX;
  }
  puVar19 = (undefined4 *)puVar14[0xd];
  puVar21 = (undefined4 *)vecDir_1338;
  for (iVar24 = 3; iVar24 != 0; iVar24 = iVar24 + -1) {
    *puVar21 = *puVar19;
    puVar19 = puVar19 + 1;
    puVar21 = puVar21 + 1;
  }
  if (this->m_pActiveInfo->field_0x38 != '\0') {
    *(undefined2 *)(puVar14 + 9) = 0;
    puVar14[8] = vecDir_1338._4_4_;
    puVar14[7] = vecDir_1338._0_4_;
    uVar12 = (ushort)uVar13 & 0xff00;
    if (((this->m_pActiveItem->attachEquipped & 1) != 0) &&
       ((((byte)this->m_pActiveInfo->attachBuiltin ^ 1) & 1) != 0)) {
      uVar12 = (ushort)CONCAT31((int3)((uint)uVar13 >> 8),1);
    }
    *(ushort *)(puVar14 + 6) = uVar12;
    puVar14[5] = (uint)(byte)this->m_pActiveInfo->iFireType;
    fVar2 = this->m_pActiveInfo->flViewKick;
    if (*(char *)((int)puVar14 + 0x9b) != '\0') {
      fVar2 = fVar2 + fVar2;
    }
    puVar14[0x3a] = fVar2;
    puVar14[4] = puVar14[0x3a];
    puVar14[3] = this;
    puVar14[2] = 0x831d2;
    VKickback((CTSGun *)puVar14[3],(float)puVar14[4],puVar14[5],*(bool *)(puVar14 + 6),
              (float)puVar14[7],(float)puVar14[8],(float)puVar14[9]);
  }
  bparam1_1340 = this->m_pActiveItem->pumpToggle != '\0';
  peVar4 = this->m_pPlayer->pev->pContainingEntity;
  puVar14[10] = *(undefined4 *)&this->m_pPlayer->field_0x7ec;
  puVar14[9] = (uint)(byte)bparam1_1340;
  puVar14[8] = this->m_pActiveItem->attachEquipped;
  puVar14[7] = this->m_weaponType;
  puVar14[6] = spread_1341;
  puVar14[5] = spread_1341;
  puVar14[4] = PTR_g_vecZero_00177698;
  puVar14[3] = PTR_g_vecZero_00177698;
  puVar14[2] = 0;
  if (*(char *)((int)puVar14 + 0x9b) == '\0') {
    uVar12 = *(ushort *)&this->field_0x964;
  }
  else {
    uVar12 = *(ushort *)&this->field_0x96e;
  }
  puVar14[1] = (uint)uVar12;
  *puVar14 = peVar4;
  puVar14[-1] = 1;
  peVar5 = PTR_g_engfuncs_001778b8->pfnPlaybackEvent;
  puVar14[-2] = 0x83259;
  (*peVar5)();
  if (this->m_pActiveItem->fireMode == '\x02') {
    rate_1342 = _UNK_00107f34;
    if (this->m_pActiveItem->burstOrPhase == 1) {
      rate_1342 = fc_0_20000000298023224;
    }
  }
  else if ((this->m_pActiveItem->fireMode == '\x01') || (this->m_pActiveItem->fireMode == '\x04')) {
    rate_1342 = 0.1;
  }
  else {
    rate_1342 = this->m_pActiveInfo->flFireRate;
  }
  puVar14[10] = 0x832bc;
  lVar22 = (longdouble)UTIL_WeaponTimeBase__Fv();
  lVar7 = (longdouble)rate_1342;
  this->field_0xa0 = (int)(float)(lVar22 + lVar7);
  this->field_0x9c = (int)(float)(lVar22 + lVar7);
  puVar14[10] = 0x832d3;
  UTIL_WeaponTimeBase__Fv();
  this->field_0xa8 = (int)(float)((longdouble)dc_2 + extraout_ST0);
  return 1;
}


