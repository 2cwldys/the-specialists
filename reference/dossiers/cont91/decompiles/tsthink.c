// ===== FUN @ 00077204 size=1365 =====

/* WARNING: Unknown calling convention -- yet parameter storage is locked */
/* TS_gamerulesclass.cpp */

void CTSGameRules::Think(CTSGameRules *this)

{
  double dVar1;
  bool bVar2;
  globalvars_t **ppgVar3;
  enginefuncs_t *peVar4;
  globalvars_t *pgVar5;
  CBaseEntity *this_00;
  int iVar6;
  int iVar7;
  longdouble extraout_ST0;
  CTSGameRules *in_stack_ffffffc4;
  
  if ((fc_0 != this->m_flGlobalSlowEnd) &&
     (this->m_flGlobalSlowEnd < (*PTR_gpGlobals_00177410)->time)) {
    this->m_flGlobalSlowActive = 0;
  }
  if (fc_0 == (float)this->m_flGlobalSlowActive) {
    if (fc_0 != this->m_flGlobalSlowScale) {
      in_stack_ffffffc4 = this;
      GlobalSlowMotion2(this,0.0,false);
      this->m_flGlobalSlowScale = 0.0;
      this->m_flGlobalSlowEnd = 0.0;
    }
  }
  else {
    in_stack_ffffffc4 = this;
    GlobalSlowMotion2(this,this->m_flGlobalSlowScale,false);
  }
  if ((fc_0 != this->m_flCheckDeads) && (this->m_flCheckDeads < (*PTR_gpGlobals_00177410)->time)) {
    in_stack_ffffffc4 = this;
    CheckDeads(this);
  }
  if (*(byte *)&this->pev < 2) {
    if (*(char *)((int)&this->pev + 1) == -1) {
      *(undefined1 *)((int)&this->pev + 1) = 1;
    }
    if ((*(float *)(PTR_nopowerup_00177c34 + 0xc) != 1.0) &&
       (*(char *)((int)&this->pev + 1) == '\x01')) {
      if ((*PTR_gpGlobals_00177410)->time <= (float)this->field_0x4) {
        if ((float)dc_21 < (float)this->field_0x4 - (*PTR_gpGlobals_00177410)->time) {
          this->field_0x4 = 0;
        }
      }
      else {
        (*PTR_g_engfuncs_001778b8->pfnRandomFloat)(0,0x3f800000);
        if (((longdouble)dc_0_25 <= extraout_ST0) ||
           (iVar7 = 1, *(float *)(PTR_slowmatch_001772c8 + 0xc) == 1.0)) {
          iVar7 = GetRandomPowerup((CTSGameRules *)PTR_TSGameRules_00177300,0x1ff);
        }
        iVar7 = PutPowerUp(this,iVar7);
        if (iVar7 == 0) {
          pgVar5 = *PTR_gpGlobals_00177410;
          dVar1 = dc_1;
        }
        else {
          pgVar5 = *PTR_gpGlobals_00177410;
          dVar1 = UNK_00107698;
        }
        this->field_0x4 = (int)((float)dVar1 + pgVar5->time);
      }
    }
    ppgVar3 = PTR_gpGlobals_00177410;
    if ((*(int *)PTR_g_theOneMode_001770dc != 0) &&
       ((float)this->field_0x587c < (*PTR_gpGlobals_00177410)->time)) {
      bVar2 = false;
      this->field_0x5876 = 0;
      iVar7 = 0;
      if (0 < (*ppgVar3)->maxClients) {
        do {
          this_00 = (CBaseEntity *)UTIL_PlayerByIndex__Fi(iVar7);
          if (((this_00 != (CBaseEntity *)0x0) &&
              (iVar6 = (*this_00->vtbl->IsPlayer)(this_00), iVar6 != 0)) &&
             (iVar6 = CBaseEntity::IsDormant(this_00), iVar6 == 0)) {
            if (this_00->field_0x7b9 == '\0') {
              this->field_0x5876 = this->field_0x5876 + '\x01';
            }
            else {
              bVar2 = true;
            }
          }
          iVar7 = iVar7 + 1;
        } while (iVar7 < (*PTR_gpGlobals_00177410)->maxClients);
      }
      if (!bVar2) {
        SelectRandomTheOne(this);
      }
      this->field_0x587c = (int)(fc_10 + (*PTR_gpGlobals_00177410)->time);
    }
    if ((*(int *)PTR_g_lastManStanding_0017733c != 0) && (*(char *)&this->pev == '\0')) {
      if ((this->m_flRoundRestart < (*PTR_gpGlobals_00177410)->time) &&
         (fc_0 != this->m_flRoundRestart)) {
        RestartRound(this);
      }
      if ((this->m_flRoundEnd < (*PTR_gpGlobals_00177410)->time) && (fc_0 == this->m_flRoundRestart)
         ) {
        CheckDeads(this);
      }
    }
  }
  else {
    if (((this->m_flRoundEnd < (*PTR_gpGlobals_00177410)->time) && (fc_0 == this->m_flRoundRestart))
       && ((char)this->field_0x10 != '\0')) {
      if (*(char *)&this->pev == '\x02') {
        TeamWin(this,2,0,1,(uint)in_stack_ffffffc4 & 0xffff0000);
      }
      else {
        RoundTimeLeft(this);
      }
    }
    if ((this->m_flRoundRestart < (*PTR_gpGlobals_00177410)->time) &&
       (fc_0 != this->m_flRoundRestart)) {
      RestartRound(this);
    }
  }
  CheckVotes(this);
  if ((float)this->field_0x578 != *(float *)(PTR_realbullet_00177544 + 0xc)) {
    this->field_0x578 = (int)ROUND(*(float *)(PTR_realbullet_00177544 + 0xc));
    peVar4 = PTR_g_engfuncs_001778b8;
    (*PTR_g_engfuncs_001778b8->pfnMessageBegin)
              (2,*(undefined4 *)PTR_gmsgTSServerSetting_00177d44,0,0);
    (*peVar4->pfnWriteByte)(0);
    (*peVar4->pfnWriteShort)(this->field_0x578);
    (*peVar4->pfnMessageEnd)();
  }
  if (this->field_0x5864 != 0) {
    CBulletManager::Think((CBulletManager *)&this->field_0x1864);
  }
  if ((float)this->field_0x558 != *(float *)(PTR_dmcashbased_00177290 + 0xc)) {
    this->field_0x558 = (int)ROUND(*(float *)(PTR_dmcashbased_00177290 + 0xc));
    peVar4 = PTR_g_engfuncs_001778b8;
    (*PTR_g_engfuncs_001778b8->pfnMessageBegin)
              (2,*(undefined4 *)PTR_gmsgTSServerSetting_00177d44,0,0);
    (*peVar4->pfnWriteByte)(1);
    (*peVar4->pfnWriteShort)((uint)(this->field_0x558 != 0));
    (*peVar4->pfnMessageEnd)();
  }
  iVar7 = (int)ROUND(*(float *)(PTR_cv_ammocount_001776cc + 0xc));
  if (iVar7 != this->field_0x55c) {
    this->field_0x55c = iVar7;
    Set_ammo_count__Fi(iVar7);
    peVar4 = PTR_g_engfuncs_001778b8;
    (*PTR_g_engfuncs_001778b8->pfnMessageBegin)
              (2,*(undefined4 *)PTR_gmsgTSServerSetting_00177d44,0,0);
    (*peVar4->pfnWriteByte)(2);
    (*peVar4->pfnWriteShort)(this->field_0x55c);
    (*peVar4->pfnMessageEnd)();
  }
  return;
}


