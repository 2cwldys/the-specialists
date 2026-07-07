// ===== FUN @ 000901cc size=1415 =====

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */
/* WARNING: Unknown calling convention -- yet parameter storage is locked */
/* TS_player.cpp
   assets: player/headshot.wav, player/headshot2.wav, player/kevlarhit.wav, player/bodyhit1.wav,
   player/bodyhit2.wav, player/bodyhit3.wav, player/bodyhit4.wav, player/pain2.wav,
   player/pain3.wav, player/pain4.wav
   cvars: dmgreport */

void CBasePlayer::TraceAttack
               (CBasePlayer *this,entvars_s *a0,float a1,Vector a2,TraceResult *a3,int a4)

{
  edict_s *a0_00;
  float fVar1;
  bool bVar2;
  bool bVar3;
  enginefuncs_t *peVar4;
  undefined uVar5;
  int iVar6;
  int iVar7;
  undefined3 extraout_var;
  undefined3 extraout_var_00;
  undefined3 extraout_var_01;
  undefined3 extraout_var_02;
  undefined3 extraout_var_03;
  undefined3 extraout_var_04;
  undefined3 extraout_var_05;
  undefined3 extraout_var_06;
  undefined3 extraout_var_07;
  undefined3 extraout_var_08;
  char *a2_00;
  entvars_t *peVar8;
  float *pfVar9;
  Vector *pVVar10;
  longdouble extraout_ST0;
  float fVar11;
  float local_3c [11];
  
  if ((*PTR_gpGlobals_00177410)->time < (float)this->field_0x71c) {
    return;
  }
  bVar3 = false;
  if ((this->pev->iuser4 & 0x10c10U) != 0) {
    a1 = (float)dc_0_7 * a1;
  }
  if (((*PTR_gpGlobals_00177410)->time < (float)this->field_0x524) && (((uint)a2.z & 2) != 0)) {
    a1 = (float)dc_0_7 * a1;
  }
  iVar6 = GetClassPtr__H1Z11CBaseEntity_PX01_PX01(a0);
  if ((((iVar6 != 0) && (iVar7 = (**(code **)(*(int *)(iVar6 + 0x60) + 200))(iVar6), iVar7 != 0)) &&
      (iVar6 = (**(code **)(**(int **)PTR_g_pGameRules_00177e70 + 0xcc))
                         (*(int **)PTR_g_pGameRules_00177e70,iVar6,this), iVar6 == 1)) &&
     (fc_0 == *(float *)(PTR_friendlyfire_00177164 + 0xc))) {
    return;
  }
  if (fc_0 == this->pev->takedamage) {
    return;
  }
  this->field_0x10c = *(int *)((int)a2.y + 0x34);
  switch(*(undefined4 *)((int)a2.y + 0x34)) {
  case 1:
    if (((uint)a2.z & 0x8000002) != 0) {
      if (((uint)a2.z & 0x2000000) == 0) {
        a1 = a1 + a1;
      }
      else {
        a1 = a1 * fc_1_5;
      }
      bVar3 = true;
      *PTR_glast_headshot_00177468 = 1;
    }
    break;
  case 6:
  case 7:
    a1 = (float)dc_0_5 * a1;
  }
  bVar2 = false;
  if (((fc_0 != this->pev->armorvalue) && (((uint)a2.z & 2) != 0)) &&
     ((iVar6 = (*this->vtbl->IsAlive)(this), iVar6 != 0 && (this->field_0x10c - 2U < 4)))) {
    bVar2 = true;
  }
  iVar6 = (*this->vtbl->IsAlive)(this);
  if (iVar6 == 0) {
    CTSAnimation::BulletDance((CTSAnimation *)&this->field_0x28c,this);
    peVar8 = this->pev;
    if ((peVar8->flags & 0x200) == 0) {
      local_3c[0] = (peVar8->velocity).x + fc_50 * *(float *)a2.x;
      local_3c[1] = (peVar8->velocity).y + fc_50 * *(float *)((int)a2.x + 4);
      local_3c[2] = (peVar8->velocity).z + fc_50 * *(float *)((int)a2.x + 8);
      pfVar9 = local_3c;
      pVVar10 = &peVar8->velocity;
      for (iVar6 = 3; iVar6 != 0; iVar6 = iVar6 + -1) {
        pVVar10->x = *pfVar9;
        pfVar9 = pfVar9 + 1;
        pVVar10 = (Vector *)&pVVar10->y;
      }
    }
  }
  if (*(float *)((int)&this->m_bLowGravity + 1) < (*PTR_gpGlobals_00177410)->time) {
    fVar11 = a1;
    if (((uint)a2.z & 0x2000000) != 0) {
      fVar11 = a1 * fc_8;
    }
    *(float *)((int)&this->m_bLowGravity + 1) = (float)dc_0_3 + (*PTR_gpGlobals_00177410)->time;
    peVar4 = PTR_g_engfuncs_001778b8;
    if (bVar2) {
      fVar1 = fVar11 * fc_0_5;
      if (fc_127 < fVar11 * fc_0_5) {
        fVar1 = fc_127;
      }
      fVar1 = fVar1 + fc_128;
    }
    else {
      fVar1 = fVar11 * fc_0_5;
      if (fc_127 < fVar11 * fc_0_5) {
        fVar1 = fc_127;
      }
    }
    pVVar10 = &this->pev->origin;
    (*PTR_g_engfuncs_001778b8->pfnMessageBegin)
              (4,*(undefined4 *)PTR_gmsgTSTakeDamage_001775b4,pVVar10,0);
    (*peVar4->pfnWriteCoord)(*(undefined4 *)((int)a2.y + 0x14));
    (*peVar4->pfnWriteCoord)(*(undefined4 *)((int)a2.y + 0x18));
    (*peVar4->pfnWriteCoord)(*(undefined4 *)((int)a2.y + 0x1c));
    (*peVar4->pfnWriteCoord)(*(undefined4 *)a2.x);
    (*peVar4->pfnWriteCoord)(*(undefined4 *)((int)a2.x + 4));
    (*peVar4->pfnWriteCoord)(*(undefined4 *)((int)a2.x + 8));
    (*peVar4->pfnWriteByte)((int)ROUND(fVar1));
    (*peVar4->pfnMessageEnd)();
    if (((uint)a2.z & 0x42) != 0) {
      if (bVar3) {
        uVar5 = (*peVar4->pfnRandomLong)(0);
        if (CONCAT31(extraout_var,uVar5) == 0) {
          uVar5 = (*PTR_g_engfuncs_001778b8->pfnRandomLong)(0,0x1f,pVVar10,0x3f000000);
          FUN_00090796(0x3fa00000,0,CONCAT31(extraout_var_00,uVar5) + 0x5f);
          return;
        }
        if (CONCAT31(extraout_var,uVar5) == 1) {
          uVar5 = (*PTR_g_engfuncs_001778b8->pfnRandomLong)(0,0x1f,pVVar10,0x3f000000);
          FUN_00090796(0x3fa00000,0,CONCAT31(extraout_var_01,uVar5) + 0x5f);
          return;
        }
      }
      else {
        if (bVar2) {
          uVar5 = (*PTR_g_engfuncs_001778b8->pfnRandomLong)(0,0x1f,pVVar10,0x3f000000);
          FUN_00090796(0x3fa00000,0,CONCAT31(extraout_var_02,uVar5) + 0x5f);
          return;
        }
        uVar5 = (*PTR_g_engfuncs_001778b8->pfnRandomLong)(0);
        switch(CONCAT31(extraout_var_03,uVar5)) {
        case 0:
          uVar5 = (*PTR_g_engfuncs_001778b8->pfnRandomLong)(0,0x1f,pVVar10,0x3f000000);
          FUN_00090796(0x3fa00000,0,CONCAT31(extraout_var_04,uVar5) + 0x5f);
          return;
        case 1:
          uVar5 = (*PTR_g_engfuncs_001778b8->pfnRandomLong)(0,0x1f,pVVar10,0x3f000000);
          FUN_00090796(0x3fa00000,0,CONCAT31(extraout_var_05,uVar5) + 0x5f);
          return;
        case 2:
          uVar5 = (*PTR_g_engfuncs_001778b8->pfnRandomLong)(0,0x1f,pVVar10,0x3f000000);
          FUN_00090796(0x3fa00000,0,CONCAT31(extraout_var_06,uVar5) + 0x5f);
          return;
        case 3:
          a0_00 = this->pev->pContainingEntity;
          fVar11 = 0.5;
          uVar5 = (*PTR_g_engfuncs_001778b8->pfnRandomLong)(0);
          EMIT_SOUND_DYN__FP7edict_siPCcffiif
                    (a0_00,4,"player/bodyhit4.wav",1.0,1.25,0,CONCAT31(extraout_var_07,uVar5) + 0x5f
                     ,fVar11);
        }
      }
    }
    iVar6 = (*this->vtbl->IsAlive)(this);
    if (iVar6 != 0) {
      uVar5 = (*PTR_g_engfuncs_001778b8->pfnRandomLong)(1);
      iVar6 = CONCAT31(extraout_var_08,uVar5);
      if (iVar6 == 2) {
        peVar8 = this->pev;
        a2_00 = "player/pain3.wav";
      }
      else if (iVar6 < 3) {
        if (iVar6 != 1) goto LAB_00090830;
        peVar8 = this->pev;
        a2_00 = "player/pain2.wav";
      }
      else {
        if (iVar6 != 3) goto LAB_00090830;
        peVar8 = this->pev;
        a2_00 = "player/pain4.wav";
      }
      EMIT_SOUND_DYN__FP7edict_siPCcffiif
                (peVar8->pContainingEntity,2,a2_00,fc_1,_UNK_001092cc,0,100,fc_0_15000000596046448);
    }
  }
LAB_00090830:
  AddMultiDamage__FP9entvars_sP11CBaseEntityfi(a0,(CBaseEntity *)this,a1,(int)a2.z);
  peVar4 = PTR_g_engfuncs_001778b8;
  if ((*(float *)PTR_g_flWeaponCheat_00177bcc != 0.0) &&
     ((*PTR_g_engfuncs_001778b8->pfnCVarGetFloat)("dmgreport"), extraout_ST0 != (longdouble)0)) {
    (*peVar4->pfnAlertMessage)
              (1,"%s damaged %s (%f hp)\n",(*PTR_gpGlobals_00177410)->pStringBase + a0->netname,
               (*PTR_gpGlobals_00177410)->pStringBase + this->pev->netname,(double)a1);
  }
  return;
}


