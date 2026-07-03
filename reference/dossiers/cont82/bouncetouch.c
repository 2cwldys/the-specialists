// ===== FUN @ 0007decc size=2896 =====

/* WARNING: Unknown calling convention -- yet parameter storage is locked */
/* TS_grenades.cpp
   assets: weapons/knife/knife_hitbody.wav, weapons/knife/knife_hit.wav
   cvars: dmgreport */

void CTSKnife::BounceTouch(CTSKnife *this,CBaseEntity *a0)

{
  float fVar1;
  float fVar2;
  float fVar3;
  edict_s *peVar4;
  Vector a2;
  globalvars_t **ppgVar5;
  undefined *puVar6;
  char *pcVar7;
  entvars_s *a0_00;
  entvars_t *peVar8;
  entvars_s *a1;
  vec_t *pvVar9;
  float *pfVar10;
  Vector *pVVar11;
  Vector *pVVar12;
  undefined2 in_FPUControlWord;
  longdouble extraout_ST0;
  longdouble extraout_ST0_00;
  longdouble extraout_ST0_01;
  int iVar13;
  int a3;
  int a4;
  int a5;
  TraceResult *in_stack_ffffff0c;
  float fVar14;
  Vector local_bc [2];
  int local_a4;
  undefined2 local_a0;
  undefined2 uStack_9e;
  Vector local_9c;
  float local_8c;
  float local_88;
  float fStack_84;
  undefined1 local_7c [20];
  float local_68;
  float local_64;
  float fStack_60;
  float local_4c [4];
  Vector local_3c [2];
  float local_20;
  
  this->pev->nextthink = (float)dc_0_1 + (*PTR_gpGlobals_00177410)->time;
  if (a0->pev->pContainingEntity == this->m_pOwner) {
    return;
  }
  if ((char)a0->pev->effects < '\0') {
    return;
  }
  peVar8 = this->pev;
  fVar14 = (peVar8->velocity).x;
  fVar1 = (peVar8->velocity).y;
  fVar2 = (peVar8->velocity).z;
  local_20 = SQRT(fVar2 * fVar2 + fVar1 * fVar1 + fVar14 * fVar14);
  if (local_20 < fc_700) {
    this->field_0x90 = 0;
  }
  peVar8 = this->pev;
  local_3c[0].x = (float)this->field_0x94 - (peVar8->origin).x;
  local_3c[0].y = (float)this->field_0x98 - (peVar8->origin).y;
  local_3c[0].z = (float)this->field_0x9c - (peVar8->origin).z;
  fVar14 = fc_150;
  if ((this->m_iType != 8) && (fVar14 = fc_50, this->m_iType == 6)) {
    fVar14 = fc_100;
  }
  if ((fc_0 != *(float *)PTR_g_flWeaponCheat_00177bcc) &&
     ((*PTR_g_engfuncs_001778b8->pfnCVarGetFloat)("dmgreport"), (longdouble)fc_1 < extraout_ST0)) {
    a5 = 0;
    a4 = 0;
    a3 = 0;
    iVar13 = 0;
    pcVar7 = (char *)UTIL_VarArgs__FPce("(debug)%f\n");
    UTIL_ClientPrintAll__FiPCcN41(3,pcVar7,iVar13,a3,a4,a5);
  }
  this->field_0x91 = 0;
  if ((fc_0 != a0->pev->takedamage) && (iVar13 = (*a0->vtbl->IsPlayer)(a0), iVar13 != 0)) {
    if (this->m_pOwner == (edict_t *)0x0) {
      a1 = (entvars_s *)0x0;
    }
    else {
      a1 = (entvars_s *)((uint)(this->m_pOwner + 0x80) & (this->m_pOwner == (edict_t *)0x0) - 1);
    }
    UTIL_GetGlobalTrace__Fv();
    iVar13 = 0x7e0f6;
    ClearMultiDamage__Fv();
    peVar8 = this->pev;
    fVar1 = (peVar8->velocity).x;
    fVar2 = (peVar8->velocity).y;
    fVar3 = (peVar8->velocity).z;
    local_20 = SQRT(fVar3 * fVar3 + fVar2 * fVar2 + fVar1 * fVar1);
    if (local_20 == fc_0) {
      local_3c[0].z = fc_1;
      local_3c[0].x = 0.0;
      local_3c[0].y = 0.0;
    }
    else {
      local_3c[0].z = fc_1 / local_20;
      local_3c[0].x = local_3c[0].z * (peVar8->velocity).x;
      local_3c[0].y = local_3c[0].z * (peVar8->velocity).y;
      local_3c[0].z = local_3c[0].z * (peVar8->velocity).z;
    }
    a0_00 = a1;
    if (a1 == (entvars_s *)0x0) {
      a0_00 = this->pev;
    }
    a2.y = (vec_t)local_7c;
    a2.x = (vec_t)local_3c;
    a2.z = 3.8537407e-34;
    (*a0->vtbl->TraceAttack)(a0,a0_00,fVar14,a2,in_stack_ffffff0c,iVar13);
    if (a1 == (entvars_s *)0x0) {
      a1 = this->pev;
    }
    ApplyMultiDamage__FP9entvars_sT0(this->pev,a1);
    peVar8 = this->pev;
    local_8c = -(peVar8->velocity).x;
    local_88 = -(peVar8->velocity).y;
    fStack_84 = -(peVar8->velocity).z;
    local_3c[0].x = fc_0_25 * local_8c;
    local_3c[0].y = fc_0_25 * local_88;
    local_3c[0].z = fc_0_25 * fStack_84;
    pVVar11 = local_3c;
    pVVar12 = &peVar8->velocity;
    for (iVar13 = 3; iVar13 != 0; iVar13 = iVar13 + -1) {
      pVVar12->x = pVVar11->x;
      pVVar11 = (Vector *)&pVVar11->y;
      pVVar12 = (Vector *)&pVVar12->y;
    }
    peVar8 = this->pev;
    pcVar7 = "weapons/knife/knife_hitbody.wav";
    goto LAB_0007e985;
  }
  if (this->m_iType == 8) {
    this->field_0x91 = 1;
  }
  iVar13 = strcmp((*PTR_gpGlobals_00177410)->pStringBase + a0->pev->classname,"worldspawn");
  if (iVar13 == 0) {
    UTIL_GetGlobalTrace__Fv();
    peVar8 = this->pev;
    fVar14 = (peVar8->velocity).x;
    fVar1 = (peVar8->velocity).y;
    fVar2 = (peVar8->velocity).z;
    if (fc_100 <= SQRT(fVar2 * fVar2 + fVar1 * fVar1 + fVar14 * fVar14)) {
      local_9c.x = 0.0;
      local_9c.y = 0.0;
      local_9c.z = 1.0;
      peVar8 = this->pev;
      fVar14 = (peVar8->velocity).x;
      fVar1 = (peVar8->velocity).y;
      fVar2 = (peVar8->velocity).z;
      local_20 = SQRT(fVar2 * fVar2 + fVar1 * fVar1 + fVar14 * fVar14);
      if (local_20 == 0.0) {
        local_4c[0] = fc_0;
        local_4c[2] = fc_1;
        local_4c[1] = local_4c[0];
      }
      else {
        local_4c[2] = fc_1 / local_20;
        local_4c[0] = local_4c[2] * (peVar8->velocity).x;
        local_4c[1] = local_4c[2] * (peVar8->velocity).y;
        local_4c[2] = local_4c[2] * (peVar8->velocity).z;
      }
      if ((-(fStack_60 * local_4c[2] + local_64 * local_4c[1] + local_68 * local_4c[0]) <
           (float)dc_0_4) && (fStack_60 <= (float)dc_0_98)) {
        iVar13 = UTIL_PointContents__FRC6Vector(&this->pev->origin);
        if (iVar13 != -3) {
          UTIL_Sparks__FRC6Vector(&this->pev->origin);
        }
        peVar4 = this->pev->pContainingEntity;
        fVar14 = 0.5;
        (*PTR_g_engfuncs_001778b8->pfnRandomFloat)(0,0x41200000);
        _local_a0 = CONCAT22(uStack_9e,in_FPUControlWord);
        local_a4 = (int)ROUND(extraout_ST0_01 + (longdouble)fc_100);
        EMIT_SOUND_DYN__FP7edict_siPCcffiif
                  (peVar4,4,"weapons/knife/knife_hit.wav",1.0,0.8,0,local_a4,fVar14);
        peVar8 = this->pev;
        local_4c[0] = (peVar8->velocity).x / fc_2;
        local_4c[1] = (peVar8->velocity).y / fc_2;
        local_4c[2] = (peVar8->velocity).z / fc_2;
        pfVar10 = local_4c;
        pVVar11 = &peVar8->velocity;
        for (iVar13 = 3; iVar13 != 0; iVar13 = iVar13 + -1) {
          pVVar11->x = *pfVar10;
          pfVar10 = pfVar10 + 1;
          pVVar11 = (Vector *)&pVVar11->y;
        }
        return;
      }
      peVar8 = this->pev;
      fVar14 = (peVar8->velocity).x;
      fVar1 = (peVar8->velocity).y;
      fVar2 = (peVar8->velocity).z;
      local_20 = SQRT(fVar2 * fVar2 + fVar1 * fVar1 + fVar14 * fVar14);
      if (local_20 == fc_0) {
        local_4c[2] = fc_1;
        local_4c[0] = 0.0;
        local_4c[1] = 0.0;
      }
      else {
        local_4c[2] = fc_1 / local_20;
        local_4c[0] = local_4c[2] * (peVar8->velocity).x;
        local_4c[1] = local_4c[2] * (peVar8->velocity).y;
        local_4c[2] = local_4c[2] * (peVar8->velocity).z;
      }
      peVar8 = this->pev;
      local_bc[0].x = fc_6 * local_4c[0];
      local_bc[0].y = fc_6 * local_4c[1];
      local_bc[0].z = fc_6 * local_4c[2];
      local_3c[0].x = (peVar8->origin).x - local_bc[0].x;
      local_3c[0].y = (peVar8->origin).y - local_bc[0].y;
      local_3c[0].z = (peVar8->origin).z - local_bc[0].z;
      UTIL_SetOrigin__FP9entvars_sRC6Vector(this->pev,local_3c);
      UTIL_VecToAngles__FRC6Vector(local_bc);
      peVar8 = this->pev;
      pVVar11 = local_bc;
      pVVar12 = local_3c;
      for (iVar13 = 3; iVar13 != 0; iVar13 = iVar13 + -1) {
        pVVar12->x = pVVar11->x;
        pVVar11 = (Vector *)&pVVar11->y;
        pVVar12 = (Vector *)&pVVar12->y;
      }
      pVVar11 = local_3c;
      pVVar12 = &peVar8->angles;
      for (iVar13 = 3; iVar13 != 0; iVar13 = iVar13 + -1) {
        pVVar12->x = pVVar11->x;
        pVVar11 = (Vector *)&pVVar11->y;
        pVVar12 = (Vector *)&pVVar12->y;
      }
      (this->pev->angles).z = 90.0;
      this->pev->solid = 0;
      this->pev->movetype = 5;
      local_bc[0].x = fc_0;
      local_bc[0].y = fc_0;
      local_bc[0].z = fc_0;
      pVVar11 = local_bc;
      pVVar12 = &this->pev->velocity;
      for (iVar13 = 3; iVar13 != 0; iVar13 = iVar13 + -1) {
        pVVar12->x = pVVar11->x;
        pVVar11 = (Vector *)&pVVar11->y;
        pVVar12 = (Vector *)&pVVar12->y;
      }
      pVVar11 = &this->pev->avelocity;
      pVVar12 = (Vector *)PTR_g_vecZero_00177698;
      goto LAB_0007e941;
    }
    peVar8 = this->pev;
    fVar14 = (peVar8->velocity).x;
    fVar1 = (peVar8->velocity).y;
    fVar2 = (peVar8->velocity).z;
    local_20 = SQRT(fVar2 * fVar2 + fVar1 * fVar1 + fVar14 * fVar14);
    if (local_20 == fc_0) {
      local_4c[2] = fc_1;
      local_4c[0] = 0.0;
      local_4c[1] = 0.0;
    }
    else {
      local_4c[2] = fc_1 / local_20;
      local_4c[0] = local_4c[2] * (peVar8->velocity).x;
      local_4c[1] = local_4c[2] * (peVar8->velocity).y;
      local_4c[2] = local_4c[2] * (peVar8->velocity).z;
    }
    peVar8 = this->pev;
    local_9c.x = fc_6 * local_4c[0];
    local_9c.y = fc_6 * local_4c[1];
    local_9c.z = fc_6 * local_4c[2];
    local_3c[0].x = (peVar8->origin).x - local_9c.x;
    local_3c[0].y = (peVar8->origin).y - local_9c.y;
    local_3c[0].z = (peVar8->origin).z - local_9c.z;
    UTIL_SetOrigin__FP9entvars_sRC6Vector(this->pev,local_3c);
    UTIL_VecToAngles__FRC6Vector(&local_9c);
    peVar8 = this->pev;
    pVVar11 = &local_9c;
    pVVar12 = local_3c;
    for (iVar13 = 3; iVar13 != 0; iVar13 = iVar13 + -1) {
      pVVar12->x = pVVar11->x;
      pVVar11 = (Vector *)&pVVar11->y;
      pVVar12 = (Vector *)&pVVar12->y;
    }
    pVVar11 = local_3c;
    pVVar12 = &peVar8->angles;
    for (iVar13 = 3; iVar13 != 0; iVar13 = iVar13 + -1) {
      pVVar12->x = pVVar11->x;
      pVVar11 = (Vector *)&pVVar11->y;
      pVVar12 = (Vector *)&pVVar12->y;
    }
    (this->pev->angles).z = 90.0;
    this->pev->solid = 0;
    this->pev->movetype = 5;
    local_9c.x = fc_0;
    local_9c.y = fc_0;
    local_9c.z = fc_0;
    pVVar11 = &local_9c;
    pVVar12 = &this->pev->velocity;
    for (iVar13 = 3; iVar13 != 0; iVar13 = iVar13 + -1) {
      pVVar12->x = pVVar11->x;
      pVVar11 = (Vector *)&pVVar11->y;
      pVVar12 = (Vector *)&pVVar12->y;
    }
    pvVar9 = (vec_t *)PTR_g_vecZero_00177698;
    pVVar11 = &this->pev->avelocity;
    for (iVar13 = 3; iVar13 != 0; iVar13 = iVar13 + -1) {
      pVVar11->x = *pvVar9;
      pvVar9 = pvVar9 + 1;
      pVVar11 = (Vector *)&pVVar11->y;
    }
    peVar4 = this->pev->pContainingEntity;
    fVar14 = 0.5;
    (*PTR_g_engfuncs_001778b8->pfnRandomFloat)(0xc1200000,0);
    _local_a0 = CONCAT22(uStack_9e,in_FPUControlWord);
    local_a4 = (int)ROUND(extraout_ST0_00 + (longdouble)fc_100);
    EMIT_SOUND_DYN__FP7edict_siPCcffiif
              (peVar4,4,"weapons/knife/knife_hit.wav",1.0,0.8,0,local_a4,fVar14);
  }
  else {
    iVar13 = strcmp((*PTR_gpGlobals_00177410)->pStringBase + a0->pev->classname,"knife");
    if (iVar13 == 0) {
      return;
    }
    peVar8 = this->pev;
    pVVar11 = &peVar8->velocity;
    local_9c.x = -(peVar8->velocity).x;
    local_9c.y = -(peVar8->velocity).y;
    local_9c.z = -(peVar8->velocity).z;
    pVVar12 = local_bc;
    local_bc[0].x = fc_0_25 * local_9c.x;
    local_bc[0].y = fc_0_25 * local_9c.y;
    local_bc[0].z = fc_0_25 * local_9c.z;
LAB_0007e941:
    for (iVar13 = 3; iVar13 != 0; iVar13 = iVar13 + -1) {
      pVVar11->x = pVVar12->x;
      pVVar12 = (Vector *)&pVVar12->y;
      pVVar11 = (Vector *)&pVVar11->y;
    }
  }
  iVar13 = UTIL_PointContents__FRC6Vector(&this->pev->origin);
  if (iVar13 != -3) {
    UTIL_Sparks__FRC6Vector(&this->pev->origin);
  }
  peVar8 = this->pev;
  pcVar7 = "weapons/knife/knife_hit.wav";
LAB_0007e985:
  EMIT_SOUND_DYN__FP7edict_siPCcffiif
            (peVar8->pContainingEntity,4,pcVar7,fc_1,fc_0_800000011920929,0,100,fc_0_5);
  puVar6 = PTR_WaitTouch_00177de0;
  this->m_pfnTouch = (void *)0xffff0000;
  this->field_0x18 = (int)puVar6;
  this->m_pfnThink = (void *)0x460000;
  this->field_0x10 = CONCAT22((short)((uint)this >> 0x10),0x60);
  this->field_0x90 = 0;
  ppgVar5 = PTR_gpGlobals_00177410;
  this->pev->nextthink = (float)dc_0_1 + (*PTR_gpGlobals_00177410)->time;
  this->pev->gravity = 0.75;
  this->m_flExpire = (float)dc_10 + (*ppgVar5)->time;
  return;
}


