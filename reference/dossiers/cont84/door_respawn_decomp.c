// ===== FUN @ 000b60cc size=816 =====

/* WARNING: Unknown calling convention -- yet parameter storage is locked */
/* doors.cpp */

void CBaseDoor::Spawn(CBaseDoor *this)

{
  entvars_t *peVar1;
  undefined *puVar2;
  undefined4 uVar3;
  int iVar4;
  int iVar5;
  float *pfVar6;
  int *piVar7;
  Vector *pVVar8;
  int *piVar9;
  edict_t *peVar10;
  float local_3c;
  float local_38;
  float local_34;
  float local_2c [7];
  
  pVVar8 = &this->pev->origin;
  piVar7 = &this->field_0x124;
  for (iVar5 = 3; iVar5 != 0; iVar5 = iVar5 + -1) {
    *piVar7 = (int)pVVar8->x;
    pVVar8 = (Vector *)&pVVar8->y;
    piVar7 = piVar7 + 1;
  }
  pVVar8 = &this->pev->angles;
  piVar7 = &this->field_0x130;
  for (iVar5 = 3; iVar5 != 0; iVar5 = iVar5 + -1) {
    *piVar7 = (int)pVVar8->x;
    pVVar8 = (Vector *)&pVVar8->y;
    piVar7 = piVar7 + 1;
  }
  this->field_0x13c = 0;
  this->field_0x110 = 0;
  this->field_0x114 = 0;
  if ((float)this->field_0x118 <= fc_0) {
    this->pev->takedamage = 0.0;
  }
  else {
    this->pev->takedamage = 1.0;
  }
  (*this->vtbl->Precache)(this);
  SetMovedir__FP9entvars_s(this->pev);
  peVar1 = this->pev;
  if (peVar1->skin == 0) {
    if ((peVar1->spawnflags & 8) == 0) {
      this->pev->solid = 4;
    }
    else {
      peVar1->solid = 0;
    }
  }
  else {
    this->pev->solid = 0;
    piVar7 = &this->pev->spawnflags;
    *piVar7 = *piVar7 | 0x80000000;
  }
  this->pev->movetype = 7;
  UTIL_SetOrigin__FP9entvars_sRC6Vector(this->pev,&this->pev->origin);
  (*PTR_g_engfuncs_001778b8->pfnSetModel)
            (this->pev->pContainingEntity,(*PTR_gpGlobals_00177410)->pStringBase + this->pev->model)
  ;
  if (fc_0 == this->pev->speed) {
    this->pev->speed = 100.0;
  }
  pVVar8 = &this->pev->origin;
  piVar7 = &this->field_0x9c;
  for (iVar5 = 3; iVar5 != 0; iVar5 = iVar5 + -1) {
    *piVar7 = (int)pVVar8->x;
    pVVar8 = (Vector *)&pVVar8->y;
    piVar7 = piVar7 + 1;
  }
  peVar1 = this->pev;
  local_34 = (ABS(((peVar1->size).z - fc_2) * (peVar1->movedir).z) +
             ABS(((peVar1->size).y - fc_2) * (peVar1->movedir).y) +
             ABS(((peVar1->size).x - fc_2) * (peVar1->movedir).x)) - (float)this->field_0x90;
  local_3c = local_34 * (peVar1->movedir).x;
  local_38 = local_34 * (peVar1->movedir).y;
  local_34 = local_34 * (peVar1->movedir).z;
  local_2c[0] = (float)this->field_0x9c + local_3c;
  local_2c[1] = (float)this->field_0xa0 + local_38;
  local_2c[2] = (float)this->field_0xa4 + local_34;
  pfVar6 = local_2c;
  piVar7 = &this->field_0xa8;
  for (iVar5 = 3; iVar5 != 0; iVar5 = iVar5 + -1) {
    *piVar7 = (int)*pfVar6;
    pfVar6 = pfVar6 + 1;
    piVar7 = piVar7 + 1;
  }
  if ((this->pev->spawnflags & 1) != 0) {
    UTIL_SetOrigin__FP9entvars_sRC6Vector(this->pev,(Vector *)&this->field_0xa8);
    piVar7 = &this->field_0x9c;
    piVar9 = &this->field_0xa8;
    for (iVar5 = 3; iVar5 != 0; iVar5 = iVar5 + -1) {
      *piVar9 = *piVar7;
      piVar7 = piVar7 + 1;
      piVar9 = piVar9 + 1;
    }
    pVVar8 = &this->pev->origin;
    piVar7 = &this->field_0x9c;
    for (iVar5 = 3; iVar5 != 0; iVar5 = iVar5 + -1) {
      *piVar7 = (int)pVVar8->x;
      pVVar8 = (Vector *)&pVVar8->y;
      piVar7 = piVar7 + 1;
    }
  }
  this->field_0x80 = 1;
  puVar2 = PTR_DoorTouch_0017760c;
  if ((this->pev->spawnflags & 0x100) == 0) {
    this->m_pfnTouch = (void *)0xffff0000;
    this->field_0x18 = (int)puVar2;
  }
  else {
    this->m_pfnTouch = (void *)0x0;
    this->field_0x18 = 0;
  }
  if (fc_0 <= (float)this->field_0x10c) {
    iVar5 = __opi__7EHANDLE(&this->field_0x104);
    if (iVar5 == 0) {
      peVar10 = this->pev->pContainingEntity;
      pVVar8 = &this->pev->angles;
      VecBModelOrigin__FP9entvars_s((entvars_s *)&local_3c);
      uVar3 = CBaseEntity::Create(PTR_g_szInfoBreakable_0017792c,(entvars_s *)&local_3c,pVVar8,
                                  peVar10);
      __as__7EHANDLEP11CBaseEntity(&this->field_0x104,uVar3);
    }
    iVar5 = __opi__7EHANDLE(&this->field_0x104);
    if (iVar5 != 0) {
      iVar4 = EHANDLE::Get((EHANDLE *)&this->field_0x104);
      iVar5 = 0;
      if (iVar4 != 0) {
        iVar5 = *(int *)(iVar4 + 0x7c);
      }
      __as__7EHANDLEP11CBaseEntity(iVar5 + 100,this);
    }
  }
  return;
}


// ===== FUN @ 000b57ac size=913 =====

/* WARNING: Unknown calling convention -- yet parameter storage is locked */
/* doors.cpp
   assets: debris/bustglass1.wav, debris/bustglass2.wav, debris/bustcrate1.wav,
   debris/bustcrate2.wav, debris/bustmetal1.wav, debris/bustmetal2.wav, debris/bustflesh1.wav,
   debris/bustflesh2.wav, debris/bustconcrete1.wav, debris/bustconcrete2.wav, debris/bustceiling.wav
    */

void CBaseDoor::TraceAttack(CBaseDoor *this,entvars_s *a0,float a1,Vector a2,TraceResult *a3,int a4)

{
  undefined *puVar1;
  enginefuncs_t *peVar2;
  undefined uVar3;
  int iVar4;
  undefined3 extraout_var;
  undefined3 extraout_var_00;
  undefined3 extraout_var_01;
  undefined3 extraout_var_02;
  undefined3 extraout_var_03;
  undefined3 extraout_var_04;
  undefined3 extraout_var_05;
  char *pcVar5;
  int *piVar6;
  entvars_t *peVar7;
  float *pfVar8;
  float fVar9;
  float *pfVar10;
  longdouble extraout_ST0;
  char local_9d;
  float fStack_8c;
  float fStack_88;
  float fStack_84;
  float afStack_7c [4];
  float fStack_6c;
  float fStack_68;
  float fStack_64;
  float afStack_5c [7];
  float local_40;
  float afStack_3c [4];
  float afStack_2c [7];
  
  local_9d = '\0';
  if ((*PTR_gpGlobals_00177410)->time < (float)this->field_0x114) {
    return;
  }
  if (fc_0 == this->pev->takedamage) {
    return;
  }
  if (((uint)a2.z & 0x6000040) == 0) {
    return;
  }
  iVar4 = strcmp((*PTR_gpGlobals_00177410)->pStringBase + this->pev->classname,"func_water");
  if (iVar4 == 0) {
    return;
  }
  local_40 = a1;
  if (((uint)a2.z & 0x40) != 0) {
    local_40 = a1 + a1;
  }
  local_40 = (float)this->field_0x110 + local_40;
  this->field_0x110 = (int)local_40;
  if (local_40 < (float)this->field_0x118) {
    return;
  }
  if (this->field_0x13c == '\0') {
    uVar3 = (*PTR_g_engfuncs_001778b8->pfnRandomLong)(0,100);
    if (((uint)a2.z & 0x2000000) == 0) {
      if (0x32 < CONCAT31(extraout_var,uVar3)) goto LAB_000b58b1;
    }
    else if (0x1e < CONCAT31(extraout_var,uVar3)) {
LAB_000b58b1:
      if (this->field_0x80 == 1) {
        this->field_0x13c = 1;
        this->field_0x114 = (int)((float)dc_0_1 + (*PTR_gpGlobals_00177410)->time);
        return;
      }
    }
  }
  uVar3 = (*PTR_g_engfuncs_001778b8->pfnRandomLong)(0,0x1d);
  iVar4 = CONCAT31(extraout_var_00,uVar3) + 0x5f;
  if (CONCAT31(extraout_var_00,uVar3) - 3U < 5) {
    iVar4 = 100;
  }
  (*PTR_g_engfuncs_001778b8->pfnRandomFloat)(0x3f59999a,0x3f800000);
  fVar9 = (float)extraout_ST0;
  if ((longdouble)dc_1 < extraout_ST0) {
    fVar9 = fc_1;
  }
  switch(this->field_0x11c) {
  case 0:
    uVar3 = (*PTR_g_engfuncs_001778b8->pfnRandomLong)(0,1);
    if (CONCAT31(extraout_var_01,uVar3) == 0) {
      peVar7 = this->pev;
      pcVar5 = "debris/bustglass1.wav";
LAB_000b59d3:
      EMIT_SOUND_DYN__FP7edict_siPCcffiif(peVar7->pContainingEntity,2,pcVar5,fVar9,0.8,0,iVar4,0.5);
    }
    else if (CONCAT31(extraout_var_01,uVar3) == 1) {
      peVar7 = this->pev;
      pcVar5 = "debris/bustglass2.wav";
      goto LAB_000b59d3;
    }
    local_9d = '\x01';
    break;
  case 1:
    uVar3 = (*PTR_g_engfuncs_001778b8->pfnRandomLong)(0,1);
    if (CONCAT31(extraout_var_02,uVar3) == 0) {
      peVar7 = this->pev;
      pcVar5 = "debris/bustcrate1.wav";
LAB_000b5a47:
      EMIT_SOUND_DYN__FP7edict_siPCcffiif(peVar7->pContainingEntity,2,pcVar5,fVar9,0.8,0,iVar4,0.5);
    }
    else if (CONCAT31(extraout_var_02,uVar3) == 1) {
      peVar7 = this->pev;
      pcVar5 = "debris/bustcrate2.wav";
      goto LAB_000b5a47;
    }
    local_9d = '\b';
    break;
  case 2:
  case 6:
    uVar3 = (*PTR_g_engfuncs_001778b8->pfnRandomLong)(0,1);
    if (CONCAT31(extraout_var_03,uVar3) == 0) {
      peVar7 = this->pev;
      pcVar5 = "debris/bustmetal1.wav";
LAB_000b5abb:
      EMIT_SOUND_DYN__FP7edict_siPCcffiif(peVar7->pContainingEntity,2,pcVar5,fVar9,0.8,0,iVar4,0.5);
    }
    else if (CONCAT31(extraout_var_03,uVar3) == 1) {
      peVar7 = this->pev;
      pcVar5 = "debris/bustmetal2.wav";
      goto LAB_000b5abb;
    }
    local_9d = '\x02';
    break;
  case 3:
    uVar3 = (*PTR_g_engfuncs_001778b8->pfnRandomLong)(0,1);
    if (CONCAT31(extraout_var_04,uVar3) == 0) {
      peVar7 = this->pev;
      pcVar5 = "debris/bustflesh1.wav";
LAB_000b5b2f:
      EMIT_SOUND_DYN__FP7edict_siPCcffiif(peVar7->pContainingEntity,2,pcVar5,fVar9,0.8,0,iVar4,0.5);
    }
    else if (CONCAT31(extraout_var_04,uVar3) == 1) {
      peVar7 = this->pev;
      pcVar5 = "debris/bustflesh2.wav";
      goto LAB_000b5b2f;
    }
    local_9d = '\x04';
    break;
  case 4:
  case 8:
    uVar3 = (*PTR_g_engfuncs_001778b8->pfnRandomLong)(0,1);
    if (CONCAT31(extraout_var_05,uVar3) == 0) {
      peVar7 = this->pev;
      pcVar5 = "debris/bustconcrete1.wav";
LAB_000b5ba3:
      EMIT_SOUND_DYN__FP7edict_siPCcffiif(peVar7->pContainingEntity,2,pcVar5,fVar9,0.8,0,iVar4,0.5);
    }
    else if (CONCAT31(extraout_var_05,uVar3) == 1) {
      peVar7 = this->pev;
      pcVar5 = "debris/bustconcrete2.wav";
      goto LAB_000b5ba3;
    }
    local_9d = '@';
    break;
  case 5:
    EMIT_SOUND_DYN__FP7edict_siPCcffiif
              (this->pev->pContainingEntity,2,"debris/bustceiling.wav",fVar9,0.8,0,iVar4,0.5);
  }
  peVar2 = PTR_g_engfuncs_001778b8;
  peVar7 = this->pev;
  afStack_7c[0] = (peVar7->mins).x + (peVar7->maxs).x;
  afStack_7c[1] = (peVar7->mins).y + (peVar7->maxs).y;
  afStack_7c[2] = (peVar7->mins).z + (peVar7->maxs).z;
  fStack_6c = fc_0_5 * afStack_7c[0];
  fStack_68 = fc_0_5 * afStack_7c[1];
  fStack_64 = fc_0_5 * afStack_7c[2];
  afStack_5c[0] = (peVar7->origin).x + fStack_6c;
  afStack_5c[1] = (peVar7->origin).y + fStack_68;
  afStack_5c[2] = (peVar7->origin).z + fStack_64;
  pfVar8 = afStack_5c;
  pfVar10 = afStack_2c;
  for (iVar4 = 3; iVar4 != 0; iVar4 = iVar4 + -1) {
    *pfVar10 = *pfVar8;
    pfVar8 = pfVar8 + 1;
    pfVar10 = pfVar10 + 1;
  }
  fStack_8c = afStack_2c[0] - (a0->origin).x;
  fStack_88 = afStack_2c[1] - (a0->origin).y;
  fVar9 = afStack_2c[2] - (a0->origin).z;
  local_40 = SQRT(fVar9 * fVar9 + fStack_88 * fStack_88 + fStack_8c * fStack_8c);
  if (local_40 == fc_0) {
    fStack_84 = fc_1;
    fStack_8c = 0.0;
    fStack_88 = 0.0;
  }
  else {
    fStack_84 = fc_1 / local_40;
    fStack_8c = fStack_84 * fStack_8c;
    fStack_88 = fStack_84 * fStack_88;
    fStack_84 = fStack_84 * fVar9;
  }
  afStack_5c[0] = a1 * fStack_8c;
  afStack_5c[1] = a1 * fStack_88;
  afStack_5c[2] = a1 * fStack_84;
  fStack_6c = fc_2 * a1 * fStack_8c;
  fStack_68 = fc_2 * a1 * fStack_88;
  fStack_64 = fc_2 * a1 * fStack_84;
  fVar9 = fc_1;
  if (((uint)a2.z & 0x2000000) != 0) {
    fVar9 = fc_8;
  }
  afStack_7c[0] = fVar9 * fStack_6c;
  afStack_7c[1] = fVar9 * fStack_68;
  afStack_7c[2] = fVar9 * fStack_64;
  pfVar8 = afStack_7c;
  pfVar10 = afStack_3c;
  for (iVar4 = 3; iVar4 != 0; iVar4 = iVar4 + -1) {
    *pfVar10 = *pfVar8;
    pfVar8 = pfVar8 + 1;
    pfVar10 = pfVar10 + 1;
  }
  if (this->field_0x11c == 0) {
    (*PTR_g_engfuncs_001778b8->pfnMessageBegin)
              (4,*(undefined4 *)PTR_gmsgTSCreateDebris_00177150,afStack_2c,0);
    (*peVar2->pfnWriteCoord)(afStack_2c[0]);
    (*peVar2->pfnWriteCoord)(afStack_2c[1]);
    (*peVar2->pfnWriteCoord)(afStack_2c[2]);
    (*peVar2->pfnWriteCoord)((this->pev->size).x);
    (*peVar2->pfnWriteCoord)((this->pev->size).y);
    (*peVar2->pfnWriteCoord)((this->pev->size).z);
    (*peVar2->pfnWriteByte)(0);
    (*peVar2->pfnWriteCoord)(0);
    (*peVar2->pfnWriteCoord)(0);
    (*peVar2->pfnWriteCoord)(0);
    (*peVar2->pfnWriteCoord)(0);
    (*peVar2->pfnWriteCoord)(0);
    (*peVar2->pfnWriteCoord)(0);
    (*peVar2->pfnWriteLong)(0);
    (*peVar2->pfnWriteCoord)(0);
  }
  else {
    (*PTR_g_engfuncs_001778b8->pfnMessageBegin)(4,0x17,afStack_2c,0);
    (*peVar2->pfnWriteByte)(0x6c);
    (*peVar2->pfnWriteCoord)(afStack_2c[0]);
    (*peVar2->pfnWriteCoord)(afStack_2c[1]);
    (*peVar2->pfnWriteCoord)(afStack_2c[2]);
    (*peVar2->pfnWriteCoord)((this->pev->size).x);
    (*peVar2->pfnWriteCoord)((this->pev->size).y);
    (*peVar2->pfnWriteCoord)((this->pev->size).z);
    (*peVar2->pfnWriteCoord)(afStack_3c[0]);
    (*peVar2->pfnWriteCoord)(afStack_3c[1]);
    (*peVar2->pfnWriteCoord)(afStack_3c[2]);
    (*peVar2->pfnWriteByte)(10);
    (*peVar2->pfnWriteShort)(this->field_0x120);
    (*peVar2->pfnWriteByte)(0);
    (*peVar2->pfnWriteByte)(0x19);
    (*peVar2->pfnWriteByte)((int)local_9d);
  }
  (*peVar2->pfnMessageEnd)();
  this->pev->solid = 0;
  this->pev->effects = 0x80;
  this->pev->takedamage = 0.0;
  this->pev->deadflag = 2;
  this->m_pfnTouch = (void *)0x0;
  this->field_0x18 = 0;
  this->m_pfnThink = (void *)0x0;
  this->field_0x10 = 0;
  if (0.0 <= (float)this->field_0x10c) {
    iVar4 = __opi__7EHANDLE(&this->field_0x104);
    if (iVar4 != 0) {
      piVar6 = (int *)__rf__7EHANDLE(&this->field_0x104);
      fVar9 = fc_30;
      if ((float)this->field_0x10c != 0.0) {
        fVar9 = (float)this->field_0x10c;
      }
      *(float *)(*piVar6 + 0x104) = (*PTR_gpGlobals_00177410)->time + fVar9;
      iVar4 = __rf__7EHANDLE(&this->field_0x104);
      puVar1 = PTR_RespawnThink_001772c4;
      *(undefined4 *)(iVar4 + 0xc) = 0xffff0000;
      *(undefined **)(iVar4 + 0x10) = puVar1;
    }
  }
  return;
}


// ===== FUN @ 000b5604 size=127 =====

/* WARNING: Unknown calling convention -- yet parameter storage is locked */
/* doors.cpp */

void CBaseDoor::BreakableRespawn(CBaseDoor *this,bool a0)

{
  entvars_t *peVar1;
  undefined *puVar2;
  int iVar3;
  int *piVar4;
  int *piVar5;
  vec_t *pvVar6;
  vec_t *pvVar7;
  Vector *pVVar8;
  CBaseEntity *pCStack_40;
  Vector local_3c;
  Vector local_2c [2];
  
  if (((this->pev->deadflag == 2) || (this->pev->effects == 0x80)) || (a0)) {
    peVar1 = this->pev;
    local_2c[0].x = (peVar1->absmin).x;
    local_2c[0].y = (peVar1->absmin).y;
    local_2c[0].z = (peVar1->absmin).z;
    peVar1 = this->pev;
    local_3c.x = (peVar1->absmax).x;
    local_3c.y = (peVar1->absmax).y;
    local_3c.z = (peVar1->absmax).z;
    iVar3 = UTIL_EntitiesInBox__FPP11CBaseEntityiRC6VectorT2i
                      (&pCStack_40,1,local_2c,&local_3c,0x2008);
    if (iVar3 == 0) {
      this->pev->deadflag = 0;
      this->pev->effects = 0;
      piVar5 = &this->field_0x124;
      pVVar8 = &this->pev->origin;
      for (iVar3 = 3; iVar3 != 0; iVar3 = iVar3 + -1) {
        pVVar8->x = (vec_t)*piVar5;
        piVar5 = piVar5 + 1;
        pVVar8 = (Vector *)&pVVar8->y;
      }
      UTIL_SetOrigin__FP9entvars_sRC6Vector(this->pev,&this->pev->origin);
      piVar5 = &this->field_0x130;
      pVVar8 = &this->pev->angles;
      for (iVar3 = 3; pvVar7 = (vec_t *)PTR_g_vecZero_00177698, iVar3 != 0; iVar3 = iVar3 + -1) {
        pVVar8->x = (vec_t)*piVar5;
        piVar5 = piVar5 + 1;
        pVVar8 = (Vector *)&pVVar8->y;
      }
      pvVar6 = (vec_t *)PTR_g_vecZero_00177698;
      pVVar8 = &this->pev->avelocity;
      for (iVar3 = 3; iVar3 != 0; iVar3 = iVar3 + -1) {
        pVVar8->x = *pvVar6;
        pvVar6 = pvVar6 + 1;
        pVVar8 = (Vector *)&pVVar8->y;
      }
      pVVar8 = &this->pev->velocity;
      for (iVar3 = 3; iVar3 != 0; iVar3 = iVar3 + -1) {
        pVVar8->x = *pvVar7;
        pvVar7 = pvVar7 + 1;
        pVVar8 = (Vector *)&pVVar8->y;
      }
      this->m_pfnTouch = (void *)0x0;
      this->field_0x18 = 0;
      this->m_pfnThink = (void *)0x0;
      this->field_0x10 = 0;
      (*this->vtbl->Spawn)(this);
    }
    else {
      piVar5 = &this->field_0x104;
      iVar3 = __opi__7EHANDLE(piVar5);
      if (iVar3 != 0) {
        piVar4 = (int *)__rf__7EHANDLE(piVar5);
        *(float *)(*piVar4 + 0x104) = fc_5 + (*PTR_gpGlobals_00177410)->time;
        iVar3 = __rf__7EHANDLE(piVar5);
        puVar2 = PTR_RespawnThink_001772c4;
        *(undefined4 *)(iVar3 + 0xc) = 0xffff0000;
        *(undefined **)(iVar3 + 0x10) = puVar2;
      }
    }
  }
  return;
}


