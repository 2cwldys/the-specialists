// ===== FUN @ 000890ac size=11 =====

/* WARNING: Unknown calling convention -- yet parameter storage is locked */
/* TS_multiplay_gamerules.cpp */

void CHalfLifeMultiplay::PlayerSpawn(CHalfLifeMultiplay *this,CBasePlayer *a0)

{
  FUN_000890b8();
  return;
}


// ===== FUN @ 00099634 size=987 =====

/* TS_spectator.cpp */

void CBasePlayer::StartObserver(CBasePlayer *this,Vector *a0,vec_t *param_3)

{
  undefined1 *puVar1;
  int *piVar2;
  CTSGun *this_00;
  entvars_t *peVar3;
  enginefuncs_t *peVar4;
  byte bVar5;
  undefined uVar6;
  int iVar7;
  undefined3 extraout_var;
  undefined3 extraout_var_00;
  undefined4 uVar8;
  undefined3 extraout_var_01;
  vec_t *pvVar9;
  Vector *pVVar10;
  Vector *pVVar11;
  
  this->field_0x544 = this->field_0x540;
  peVar4 = PTR_g_engfuncs_001778b8;
  (*PTR_g_engfuncs_001778b8->pfnMessageBegin)
            (1,*(undefined4 *)PTR_gmsgTSCash_001777e4,0,this->pev->pContainingEntity);
  (*peVar4->pfnWriteShort)(this->field_0x540);
  (*peVar4->pfnMessageEnd)();
  this->field_0x764 = 0;
  this->pev->iuser4 = 0;
  this->pev->fuser1 = 1.0;
  this->m_flSlowMotionTarget = 1.0;
  this->m_flSlowMotionGo = 1.0;
  (*peVar4->pfnMessageBegin)(5,0x17,&this->pev->origin,0);
  (*peVar4->pfnWriteByte)(0x7d);
  bVar5 = (*peVar4->pfnIndexOfEdict)(this->pev->pContainingEntity);
  (*peVar4->pfnWriteByte)((uint)bVar5);
  (*peVar4->pfnMessageEnd)();
  if (this->m_pActiveItem != (CBaseEntity *)0x0) {
    this_00 = (CTSGun *)this->m_pActiveItem;
    if (this_00->m_iId == 0x10) {
      CTSGun::Reset(this_00);
      CTSGun::HolsterWeapon(this_00);
    }
    (*this->m_pActiveItem->vtbl[1].IsObjective)(this->m_pActiveItem);
  }
  DropFlag(this,false);
  puVar1 = &this->field_0x9c4;
  iVar7 = __opi__7EHANDLE(puVar1);
  if (iVar7 != 0) {
    iVar7 = __rf__7EHANDLE(puVar1);
    (**(code **)(*(int *)(iVar7 + 0x60) + 0xe0))(iVar7,this,this,0,0);
    __as__7EHANDLEP11CBaseEntity(puVar1,0);
  }
  SetSuitUpdate(this,(char *)0x0,0,0);
  peVar4 = PTR_g_engfuncs_001778b8;
  (*PTR_g_engfuncs_001778b8->pfnMessageBegin)(2,*(undefined4 *)PTR_gmsgTSActiveItems_00177b2c,0,0);
  uVar6 = (*peVar4->pfnIndexOfEdict)(this->pev->pContainingEntity);
  (*peVar4->pfnWriteByte)(CONCAT31(extraout_var,uVar6));
  (*peVar4->pfnWriteByte)(0);
  (*peVar4->pfnMessageEnd)();
  this->field_0x9f4 = 0;
  this->m_iFOV = 0;
  this->pev->fov = 0.0;
  (*peVar4->pfnMessageBegin)
            (1,*(undefined4 *)PTR_gmsgSetFOV_00177718,0,this->pev->pContainingEntity);
  (*peVar4->pfnWriteByte)(0);
  (*peVar4->pfnMessageEnd)();
  this->pev->effects = 0x80;
  pvVar9 = (vec_t *)PTR_g_vecZero_00177698;
  pVVar10 = &this->pev->view_ofs;
  for (iVar7 = 3; iVar7 != 0; iVar7 = iVar7 + -1) {
    pVVar10->x = *pvVar9;
    pvVar9 = pvVar9 + 1;
    pVVar10 = (Vector *)&pVVar10->y;
  }
  peVar3 = this->pev;
  pVVar10 = &peVar3->v_angle;
  for (iVar7 = 3; iVar7 != 0; iVar7 = iVar7 + -1) {
    pVVar10->x = *param_3;
    param_3 = param_3 + 1;
    pVVar10 = (Vector *)&pVVar10->y;
  }
  pVVar10 = &this->pev->v_angle;
  pVVar11 = &peVar3->angles;
  for (iVar7 = 3; iVar7 != 0; iVar7 = iVar7 + -1) {
    pVVar11->x = pVVar10->x;
    pVVar10 = (Vector *)&pVVar10->y;
    pVVar11 = (Vector *)&pVVar11->y;
  }
  this->pev->fixangle = 1;
  this->pev->solid = 0;
  this->pev->takedamage = 0.0;
  this->pev->movetype = 0;
  this->m_afPhysicsFlags = this->m_afPhysicsFlags & 0xfffffff7;
  piVar2 = &this->pev->flags;
  *piVar2 = *piVar2 & 0xffffbfff;
  this->pev->deadflag = 3;
  this->pev->health = 1.0;
  *(undefined4 *)&this->m_fInitHUD = 1;
  (*peVar4->pfnMessageBegin)(2,*(undefined4 *)PTR_gmsgTeamInfo_00177c0c,0,0);
  uVar6 = (*peVar4->pfnIndexOfEdict)(this->pev->pContainingEntity);
  (*peVar4->pfnWriteByte)(CONCAT31(extraout_var_00,uVar6));
  uVar8 = (*this->vtbl->TeamID)(this);
  (*peVar4->pfnWriteString)(uVar8);
  (*peVar4->pfnMessageEnd)();
  RemoveAllItems(this,1);
  UTIL_SetOrigin__FP9entvars_sRC6Vector(this->pev,a0);
  if (this->field_0x76c == 0) {
    iVar7 = 2;
  }
  else {
    iVar7 = this->field_0x76c;
  }
  Observer_SetMode(this,iVar7);
  peVar4 = PTR_g_engfuncs_001778b8;
  (*PTR_g_engfuncs_001778b8->pfnMessageBegin)(2,*(undefined4 *)PTR_gmsgSpectator_00177b94,0,0);
  uVar6 = (*peVar4->pfnIndexOfEdict)(this->pev->pContainingEntity);
  (*peVar4->pfnWriteByte)(CONCAT31(extraout_var_01,uVar6));
  (*peVar4->pfnWriteByte)(1);
  (*peVar4->pfnMessageEnd)();
  return;
}


// ===== FUN @ 00099a14 size=246 =====

/* WARNING: Unknown calling convention -- yet parameter storage is locked */
/* TS_spectator.cpp */

void CBasePlayer::StopObserver(CBasePlayer *this)

{
  entvars_t *peVar1;
  enginefuncs_t *peVar2;
  undefined uVar3;
  undefined3 extraout_var;
  
  if (((float)this->field_0x770 <= fc_0) ||
     ((float)this->field_0x770 <= (*PTR_gpGlobals_00177410)->time)) {
    if ((this->pev->iuser1 != 0) || (this->pev->iuser2 != 0)) {
      this->field_0x76c = this->pev->iuser1;
      peVar2 = PTR_g_engfuncs_001778b8;
      (*PTR_g_engfuncs_001778b8->pfnMessageBegin)(2,*(undefined4 *)PTR_gmsgSpectator_00177b94,0,0);
      uVar3 = (*peVar2->pfnIndexOfEdict)(this->pev->pContainingEntity);
      (*peVar2->pfnWriteByte)(CONCAT31(extraout_var,uVar3));
      (*peVar2->pfnWriteByte)(0);
      (*peVar2->pfnMessageEnd)();
      peVar1 = this->pev;
      peVar1->iuser2 = 0;
      peVar1->iuser1 = 0;
    }
    this->pev->button = 0;
    this->field_0xb34 = 0;
    respawn__FP9entvars_si(this->pev,0);
    this->pev->nextthink = -1.0;
  }
  return;
}


// ===== FUN @ 00092594 size=263 =====

/* WARNING: Unknown calling convention -- yet parameter storage is locked */
/* TS_player.cpp */

void CBasePlayer::ForceClientDllUpdate(CBasePlayer *this)

{
  enginefuncs_t *peVar1;
  undefined uVar2;
  undefined3 extraout_var;
  
  this->field_0x9e0 = -1;
  this->field_0x9e4 = -1;
  this->m_iTrain = this->m_iTrain | 0xc0;
  this->field_0x9c0 = 0;
  this->field_0x840 = 0;
  *(undefined1 *)&this->field_0x499 = 0xff;
  this->field_0x54c = -1;
  this->field_0x544 = -1;
  *(undefined1 *)&this->field_0x720 = 0;
  this->m_iClientHeldType = 0xff;
  this->m_iClientHeldDur = 0;
  this->m_flClientSlowAmt = -1.0;
  this->field_0x174 = 0xff;
  this->field_0x534 = 0;
  (*this->vtbl->UpdateClientData)(this);
  peVar1 = PTR_g_engfuncs_001778b8;
  (*PTR_g_engfuncs_001778b8->pfnMessageBegin)
            (1,*(undefined4 *)PTR_gmsgSpectator_00177b94,0,this->pev->pContainingEntity);
  uVar2 = (*peVar1->pfnIndexOfEdict)(this->pev->pContainingEntity);
  (*peVar1->pfnWriteByte)(CONCAT31(extraout_var,uVar2));
  (*peVar1->pfnWriteByte)((uint)(this->pev->deadflag == 3));
  (*peVar1->pfnMessageEnd)();
  return;
}


// ===== FUN @ 00099d34 size=267 =====

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */
/* WARNING: Unknown calling convention -- yet parameter storage is locked */
/* TS_spectator.cpp */

void CBasePlayer::Observer_HandleButtons(CBasePlayer *this)

{
  int a0;
  
  if ((float)this->field_0x760 <= (*PTR_gpGlobals_00177410)->time) {
    if ((this->pev->button & 2) != 0) {
      if (this->pev->iuser1 == 3) {
        a0 = 2;
      }
      else if (this->pev->iuser1 == 2) {
        a0 = 1;
      }
      else if (this->pev->iuser1 == 1) {
        a0 = 5;
      }
      else {
        a0 = 3;
      }
      Observer_SetMode(this,a0);
      this->field_0x760 = (int)((float)_UNK_00109a90 + (*PTR_gpGlobals_00177410)->time);
    }
    if (((this->pev->button & 8) != 0) && (this->pev->iuser1 != 3)) {
      Observer_FindNextPlayer(this,false);
      this->field_0x760 = (int)((float)_UNK_00109a90 + (*PTR_gpGlobals_00177410)->time);
    }
    if (((this->pev->button & 0x10) != 0) && (this->pev->iuser1 != 3)) {
      Observer_FindNextPlayer(this,true);
      this->field_0x760 = (int)((float)_UNK_00109a90 + (*PTR_gpGlobals_00177410)->time);
    }
  }
  return;
}


// ===== FUN @ 000908cc size=1644 =====

/* WARNING: Removing unreachable block (ram,0x0009120d) */
/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */
/* WARNING: Unknown calling convention -- yet parameter storage is locked */
/* TS_player.cpp */

void CBasePlayer::PreThink(CBasePlayer *this)

{
  int *piVar1;
  globalvars_t *pgVar2;
  bool bVar3;
  float fVar4;
  globalvars_t **ppgVar5;
  enginefuncs_t *peVar6;
  undefined uVar7;
  int iVar8;
  char *pcVar9;
  undefined3 extraout_var;
  uint uVar11;
  undefined4 uVar12;
  undefined4 uVar13;
  uint uVar14;
  float fVar15;
  vec_t *pvVar16;
  Vector *pVVar17;
  undefined2 in_FPUControlWord;
  longdouble extraout_ST0;
  longdouble extraout_ST0_00;
  longdouble lVar18;
  int a3;
  int a4;
  int a5;
  entvars_t *peVar19;
  int *piStack_94;
  float local_90;
  float fStack_7c;
  float fStack_78;
  float fStack_74;
  undefined1 auStack_6c [16];
  float fStack_5c;
  int iStack_3c;
  int local_28;
  undefined2 local_24;
  undefined2 uStack_22;
  edict_t *peVar10;
  undefined3 extraout_var_00;
  
  uVar11 = this->pev->button;
  uVar14 = this->m_afButtonLast ^ uVar11;
  this->m_afButtonPressed = uVar11 & uVar14;
  this->m_afButtonReleased = uVar14 & ~this->pev->button;
  (**(code **)(**(int **)PTR_g_pGameRules_00177e70 + 0x50))(*(int **)PTR_g_pGameRules_00177e70,this)
  ;
  if (*(int *)PTR_g_fGameOver_00177600 != 0) {
    return;
  }
  UTIL_MakeVectors__FRC6Vector(&this->pev->v_angle);
  ItemPreFrame(this);
  WaterMove(this);
  (*this->vtbl->UpdateClientData)(this);
  CheckTimeBasedDamage(this);
  CheckSuitUpdate(this);
  ppgVar5 = PTR_gpGlobals_00177410;
  InterpolateSlowMotion__FfPff
            ((*PTR_gpGlobals_00177410)->frametime,&this->pev->fuser1,this->m_flSlowMotionGo);
  (*this->vtbl->GiveSlowMul)(this);
  this->pev->framerate = (float)extraout_ST0;
  if ((this->pev->flags & 0x200) != 0) {
    this->field_0xb88 = (int)(*ppgVar5)->time;
  }
  (*this->vtbl->GiveSlowMul)(this);
  lVar18 = extraout_ST0_00 * (longdouble)(*PTR_gpGlobals_00177410)->frametime;
  if ((this->m_iPowerups & 0x10) != 0) {
    lVar18 = lVar18 * (longdouble)fc_1_5;
  }
  local_90 = (float)lVar18;
  if (fc_0 != (float)this->field_0x79c) {
    fVar15 = (float)this->field_0x79c - local_90;
    if (fVar15 < fc_0_0010000000474974513) {
      fVar15 = fc_0_0010000000474974513;
    }
    this->field_0x79c = (int)fVar15;
  }
  if (fc_0 != (float)this->field_0x4dc) {
    fVar15 = (float)this->field_0x4dc - local_90;
    if (fVar15 < fc_0_0010000000474974513) {
      fVar15 = fc_0_0010000000474974513;
    }
    this->field_0x4dc = (int)fVar15;
  }
  if (fc_0 != (float)this->field_0x4ec) {
    fVar15 = (float)this->field_0x4ec - local_90;
    if (fVar15 < fc_0_0010000000474974513) {
      fVar15 = fc_0_0010000000474974513;
    }
    this->field_0x4ec = (int)fVar15;
  }
  fVar15 = (float)this->field_0x7a8 - local_90;
  if (fVar15 < fc_0) {
    fVar15 = fc_0;
  }
  this->field_0x7a8 = (int)fVar15;
  fVar15 = (float)this->field_0x7b0 - local_90;
  if (fVar15 < fc_0) {
    fVar15 = fc_0;
  }
  this->field_0x7b0 = (int)fVar15;
  fVar15 = (float)this->field_0x7ac - local_90;
  if (fVar15 < fc_0) {
    fVar15 = fc_0;
  }
  this->field_0x7ac = (int)fVar15;
  fVar15 = (float)this->field_0x7b4 - local_90 * fc_5;
  if (fVar15 < fc_0) {
    fVar15 = fc_0;
  }
  this->field_0x7b4 = (int)fVar15;
  CTSStunt::Play((CTSStunt *)&this->field_0x4f0);
  CTSAnimation::UpdateTimers((CTSAnimation *)&this->field_0x28c,this);
  peVar19 = this->pev;
  bVar3 = false;
  if ((((peVar19->view_ofs).x == *(float *)PTR_g_vecZero_00177698) &&
      ((peVar19->view_ofs).y == *(float *)(PTR_g_vecZero_00177698 + 4))) &&
     ((peVar19->view_ofs).z == *(float *)(PTR_g_vecZero_00177698 + 8))) {
    bVar3 = true;
  }
  if (bVar3) {
    piVar1 = &this->pev->iuser4;
    *piVar1 = *piVar1 | 0x400000;
  }
  else {
    piVar1 = &this->pev->iuser4;
    *piVar1 = *piVar1 & 0xffbfffff;
  }
  UpdatePowerUpState(this);
  SlowMotionFlow(this);
  if (fc_0 != this->m_flHealthRegen) {
    if (this->field_0x7b9 == '\0') {
      if (this->pev->health <= this->pev->max_health) {
        this->m_flHealthRegen = 0.0;
      }
      else if (this->m_flHealthRegen < (*PTR_gpGlobals_00177410)->time) {
        this->m_flHealthRegen = (float)dc_1 + (*PTR_gpGlobals_00177410)->time;
        this->pev->health = this->pev->health - fc_1;
      }
    }
    else if (this->m_flHealthRegen < (*PTR_gpGlobals_00177410)->time) {
      fVar15 = (float)(short)(char)PTR_TSGameRules_00177300[0x5876] * fc_0_3199999928474426;
      if (fVar15 < fc_1) {
        fVar15 = fc_1;
      }
      this->m_flHealthRegen = fc_1 / fVar15 + (*PTR_gpGlobals_00177410)->time;
      fVar15 = (float)(short)(char)PTR_TSGameRules_00177300[0x5876] * fc_0_1599999964237213 +
               this->pev->health;
      if (fc_150 < fVar15) {
        fVar15 = fc_150;
      }
      this->pev->health = fVar15;
    }
  }
  if (fc_0 != (float)this->field_0x4dc) {
    uVar11 = this->pev->iuser4;
    if (((uVar11 & 1) != 0) && ((float)this->field_0x4dc < fc_0_009999999776482582)) {
      this->pev->iuser4 = uVar11 & 0xfffffffe;
      this->field_0x4dc = 0;
    }
  }
  if ((fc_0 != (float)this->field_0x4ec) && ((float)this->field_0x4ec < fc_0_009999999776482582)) {
    this->pev->gravity = 1.0;
    this->field_0x4ec = 0;
  }
  iVar8 = (*this->vtbl->IsAlive)(this);
  fVar15 = fc_0;
  if (iVar8 != 0) {
    if ((fc_0 < (float)this->m_flSpreadAccum) &&
       (fVar4 = (float)this->m_flSpreadAccum - local_90 * fc_0_10000000149011612,
       this->m_flSpreadAccum = fVar4, fVar4 < fVar15)) {
      this->m_flSpreadAccum = 0;
    }
    if ((float)this->field_0x774 < fc_100) {
      if (fc_neg0_0010000000474974513 <= (float)this->field_0x774) {
        local_90 = local_90 * fc_15;
      }
      this->field_0x774 = (int)(local_90 + (float)this->field_0x774);
      if (fc_100 < (float)this->field_0x774) {
        this->field_0x774 = 0x42c80000;
      }
    }
  }
  if (fc_0 != (float)this->field_0x79c) {
    if (((float)this->field_0x79c < fc_0_009999999776482582) || (fc_0 == (float)this->field_0x7a0))
    {
      this->field_0x79c = 0;
      this->field_0x7a0 = 0;
      *(undefined1 *)&this->field_0x7a4 = 0;
    }
    this->field_0x7a8 = 0;
  }
  ppgVar5 = PTR_gpGlobals_00177410;
  if (this->pev->iuser1 != 0) {
    if (((fc_0 < (float)this->field_0x770) &&
        (pgVar2 = *PTR_gpGlobals_00177410, pgVar2->time < (float)this->field_0x770)) &&
       ((float)this->field_0x754 < pgVar2->time)) {
      this->field_0x754 = (int)((float)dc_1 + pgVar2->time);
      _local_24 = CONCAT22(uStack_22,in_FPUControlWord);
      local_28 = (int)ROUND((float)this->field_0x770 - (*ppgVar5)->time);
      if (local_28 < 1) {
        peVar19 = this->pev;
        pcVar9 = "Press Fire To Play!";
        iVar8 = 0x1e;
      }
      else {
        peVar19 = this->pev;
        fVar15 = (float)this->field_0x770 - (*PTR_gpGlobals_00177410)->time;
        if (fc_30 < fVar15) {
          fVar15 = fc_30;
        }
        _local_24 = CONCAT22(uStack_22,in_FPUControlWord);
        local_28 = (int)ROUND(fVar15);
        pcVar9 = (char *)UTIL_VarArgs__FPce("%i");
        iVar8 = 0x14;
      }
      TSHudMessageOne__FUcUcUciiPcP9entvars_s(0,0,0,0x10,iVar8,pcVar9,peVar19);
    }
    ppgVar5 = PTR_gpGlobals_00177410;
    if (fc_0 != (float)this->field_0x7bc) {
      if ((*PTR_gpGlobals_00177410)->time <= (float)this->field_0x7bc) {
        if ((float)this->field_0x754 < (*PTR_gpGlobals_00177410)->time) {
          this->field_0x754 = (int)((float)dc_1 + (*PTR_gpGlobals_00177410)->time);
          peVar19 = this->pev;
          _local_24 = CONCAT22(uStack_22,in_FPUControlWord);
          local_28 = (int)ROUND((float)this->field_0x7bc - (*ppgVar5)->time);
          pcVar9 = (char *)UTIL_VarArgs__FPce("%i seconds left for respawn, press Fire to Play!");
          TSHudMessageOne__FUcUcUciiPcP9entvars_s(0,0,0,0x10,0x14,pcVar9,peVar19);
        }
      }
      else {
        this->field_0x7bc = 0;
        this->field_0x770 = -0x40800000;
        this->field_0x74c = 1;
        TSHudMessageOne__FUcUcUciiPcP9entvars_s
                  (0xc3,0x2d,0x2d,0x10,0x14,"You have to wait until the next round.",this->pev);
        CTSGameRules::CallCheckDeads((CTSGameRules *)PTR_TSGameRules_00177300);
      }
    }
    Observer_HandleButtons(this);
    Observer_Idle(this);
    this->pev->impulse = 0;
    return;
  }
  CTSStunt::CheckForBreakables((CTSStunt *)&this->field_0x4f0);
  if (0 < this->pev->deadflag) {
    PlayerDeathThink(this);
    return;
  }
  if ((this->m_afPhysicsFlags & 2U) == 0) {
    piVar1 = &this->pev->flags;
    *piVar1 = *piVar1 & 0xfeffffff;
  }
  else {
    piVar1 = &this->pev->flags;
    *piVar1 = *piVar1 | 0x1000000;
  }
  if ((this->m_afPhysicsFlags & 2U) == 0) {
    if ((char)this->m_iTrain < '\0') {
      this->m_iTrain = 0xc0;
    }
LAB_000913ab:
    if ((this->pev->button & 2) != 0) {
      (*this->vtbl->Jump)(this);
    }
    if ((((this->pev->button & 4) != 0) || ((this->pev->flags & 0x4000) != 0)) ||
       ((this->m_afPhysicsFlags & 8U) != 0)) {
      (*this->vtbl->Duck)(this);
    }
    if ((this->pev->flags & 0x200) == 0) {
      this->m_flFallVelocity = -(this->pev->velocity).z;
    }
    UpdatePlayerState(this);
    if ((*PTR_TSGameRules_00177300 == '\x03') &&
       ((float)this->field_0x794 < (*PTR_gpGlobals_00177410)->time)) {
      this->field_0x794 = (int)((float)dc_0_5 + (*PTR_gpGlobals_00177410)->time);
      CheckForWires(this);
    }
    if (((float)this->field_0x71c < (*PTR_gpGlobals_00177410)->time) && (this->pev->rendermode != 0)
       ) {
      this->pev->rendermode = 0;
      (this->pev->rendercolor).x = 0.0;
      (this->pev->rendercolor).y = 0.0;
      (this->pev->rendercolor).z = 0.0;
      this->pev->renderamt = 0.0;
      this->pev->renderfx = 0;
    }
    if ((fc_0 != (float)this->field_0x7cc) &&
       ((float)this->field_0x7cc < (*PTR_gpGlobals_00177410)->time)) {
      ClientPrint__FP9entvars_siPCcN42
                (this->pev,2,"You have been kicked for some reason...\n",0,0,0,0);
      a5 = 0;
      a4 = 0;
      a3 = 0;
      iVar8 = 0;
      pcVar9 = (char *)UTIL_VarArgs__FPce("%s kicked.\n");
      UTIL_ClientPrintAll__FiPCcN41(1,pcVar9,iVar8,a3,a4,a5);
      peVar6 = PTR_g_engfuncs_001778b8;
      uVar7 = (*PTR_g_engfuncs_001778b8->pfnGetPlayerUserId)(this->pev->pContainingEntity);
      uVar12 = CONCAT31(extraout_var_00,uVar7);
      uVar13 = UTIL_VarArgs__FPce("kick #%i\n");
      (*peVar6->pfnServerCommand)(uVar13,uVar12);
      this->field_0x7cc = 0;
    }
    __as__7EHANDLEP11CBaseEntity(&this->field_0x130,0);
    if ((this->m_afPhysicsFlags & 4U) != 0) {
      pvVar16 = (vec_t *)PTR_g_vecZero_00177698;
      pVVar17 = &this->pev->velocity;
      for (iVar8 = 3; iVar8 != 0; iVar8 = iVar8 + -1) {
        pVVar17->x = *pvVar16;
        pvVar16 = pvVar16 + 1;
        pVVar17 = (Vector *)&pVVar17->y;
      }
    }
    if ((fc_0_0010000000474974513 < (float)this->field_0x84c) &&
       ((float)this->field_0x84c < (*PTR_gpGlobals_00177410)->time)) {
      this->field_0x84c = 0;
      this->pev->health = 0.0;
      (*this->vtbl->Killed)(this,this->pev,1);
    }
  }
  else {
    peVar10 = this->pev->groundentity;
    if (peVar10 == (edict_t *)0x0) {
      uVar7 = (*PTR_g_engfuncs_001778b8->pfnPEntityOfEntOffset)(0);
      peVar10 = (edict_t *)CONCAT31(extraout_var,uVar7);
    }
    piStack_94 = (int *)0x0;
    if (peVar10 != (edict_t *)0x0) {
      piStack_94 = *(int **)(peVar10 + 0x7c);
    }
    if (piStack_94 == (int *)0x0) {
      peVar19 = this->pev;
      fStack_7c = (peVar19->origin).x + fc_0;
      fStack_78 = (peVar19->origin).y + fc_0;
      fStack_74 = (peVar19->origin).z + _UNK_001092d8;
      UTIL_TraceLine__FRC6VectorT015IGNORE_MONSTERSP7edict_sP11TraceResult
                (&this->pev->origin,&fStack_7c,1,peVar19->pContainingEntity,auStack_6c);
      if (((fStack_5c != (float)dc_1) && (iStack_3c != 0)) &&
         (piStack_94 = (int *)0x0, iStack_3c != 0)) {
        piStack_94 = *(int **)(iStack_3c + 0x7c);
      }
      if (((piStack_94 != (int *)0x0) &&
          (uVar11 = (**(code **)(piStack_94[0x18] + 0x38))(piStack_94), (uVar11 & 0x40) != 0)) &&
         (iVar8 = (**(code **)(piStack_94[0x18] + 0xac))(piStack_94,this->pev), iVar8 != 0)) {
LAB_000912b0:
        pvVar16 = (vec_t *)PTR_g_vecZero_00177698;
        pVVar17 = &this->pev->velocity;
        for (iVar8 = 3; iVar8 != 0; iVar8 = iVar8 + -1) {
          pVVar17->x = *pvVar16;
          pvVar16 = pvVar16 + 1;
          pVVar17 = (Vector *)&pVVar17->y;
        }
        if ((this->m_afButtonPressed & 8U) == 0) {
          fVar15 = fc_0;
          if ((this->m_afButtonPressed & 0x10U) != 0) {
            iVar8 = piStack_94[0x18];
            fVar15 = fc_neg1;
            goto LAB_00091312;
          }
        }
        else {
          iVar8 = piStack_94[0x18];
          fVar15 = fc_1;
LAB_00091312:
          (**(code **)(iVar8 + 0xe0))(piStack_94,this,this,2,fVar15);
        }
        if (fVar15 != fc_0) {
          _local_24 = CONCAT22(uStack_22,in_FPUControlWord);
          local_28 = (int)ROUND(*(float *)(*piStack_94 + 0x1f8));
          uVar11 = TrainSpeed__Fii(local_28,*(int *)(*piStack_94 + 0x184));
          this->m_iTrain = uVar11 | 0xc0;
        }
        goto LAB_000913ab;
      }
    }
    else if ((((this->pev->flags & 0x200) != 0) && ((*(byte *)(*piStack_94 + 0x1a0) & 2) == 0)) &&
            ((this->pev->button & 0x600U) == 0)) goto LAB_000912b0;
    this->m_afPhysicsFlags = this->m_afPhysicsFlags & 0xfffffffd;
    this->m_iTrain = 0xc0;
  }
  return;
}


