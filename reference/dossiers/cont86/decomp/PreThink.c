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


