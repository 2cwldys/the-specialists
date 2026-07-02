// ===== FUN @ 00095f88 size=1406 =====

CBasePlayer *
CloseCombatAttack__11CBasePlayerG6VectorN21ifibN37
          (CBasePlayer *param_1,float *param_2,float *param_3,undefined4 *param_4,undefined4 param_5
          ,float param_6,CBasePlayer *param_7,char param_8,char param_9,char param_10,char param_11)

{
  float fVar1;
  float fVar2;
  entvars_t *peVar3;
  entvars_t *peVar4;
  edict_s *a0;
  Vector a2;
  Vector a1;
  undefined *puVar5;
  enginefuncs_t *peVar6;
  undefined uVar7;
  char cVar8;
  undefined3 extraout_var;
  undefined3 extraout_var_00;
  undefined3 extraout_var_01;
  int iVar9;
  undefined3 extraout_var_02;
  CBasePlayer *this;
  entvars_t **ppeVar10;
  Vector *pVVar11;
  CBasePlayer *in_stack_ffffff3c;
  float fVar12;
  vec_t in_stack_ffffff44;
  float local_9c;
  float local_98;
  float local_94;
  entvars_t *local_7c;
  float local_78;
  float local_74;
  float local_6c;
  float local_68;
  float local_64;
  TraceResult local_5c [16];
  float local_4c;
  int local_2c;
  
  if (param_7 != (CBasePlayer *)0xffffffff) {
    CTSAnimation::Animate((CTSAnimation *)&param_1->field_0x28c,param_1,(int)param_7);
    in_stack_ffffff3c = param_7;
  }
  peVar6 = PTR_g_engfuncs_001778b8;
  uVar7 = (*PTR_g_engfuncs_001778b8->pfnRandomLong)(10,0x50);
  if ((float)CONCAT31(extraout_var,uVar7) < param_6) {
    in_stack_ffffff3c = (CBasePlayer *)&param_1->pev->origin;
    (*peVar6->pfnMessageBegin)(4,*(undefined4 *)PTR_gmsgTSMotionBlur_00177b48,in_stack_ffffff3c,0);
    uVar7 = (*peVar6->pfnIndexOfEdict)(param_1->pev->pContainingEntity);
    (*peVar6->pfnWriteByte)(CONCAT31(extraout_var_00,uVar7));
    (*peVar6->pfnWriteByte)((fc_50 < param_6) + 1);
    (*peVar6->pfnMessageEnd)();
  }
  local_6c = *param_2;
  local_68 = param_2[1];
  local_64 = param_2[2];
  local_7c = (entvars_t *)*param_3;
  local_78 = param_3[1];
  local_74 = param_3[2];
  CBasePlayer::CheckHull(param_1,&local_6c,&local_7c,local_5c,param_5);
  if ((float)dc_1 <= local_4c) {
    return (CBasePlayer *)0x0;
  }
  if (local_2c == 0) {
    uVar7 = (*PTR_g_engfuncs_001778b8->pfnPEntityOfEntOffset)(0);
    local_2c = CONCAT31(extraout_var_01,uVar7);
  }
  this = (CBasePlayer *)0x0;
  if (local_2c != 0) {
    this = *(CBasePlayer **)(local_2c + 0x7c);
  }
  if (this == (CBasePlayer *)0x0) {
    return (CBasePlayer *)0x0;
  }
  if (((fc_0 == this->pev->takedamage) || ((this->pev->flags & 0x2000000) != 0)) ||
     ((char)this->pev->effects < '\0')) {
LAB_000961af:
    this = (CBasePlayer *)0x0;
  }
  else {
    iVar9 = (*this->vtbl->IsPlayer)(this);
    if (iVar9 == 0) {
      if (param_9 != '\0') {
        return (CBasePlayer *)0x0;
      }
    }
    else if (((param_11 != '\0') || (fc_0 == *(float *)(PTR_friendlyfire_00177164 + 0xc))) &&
            (in_stack_ffffff3c = this,
            iVar9 = (**(code **)(**(int **)PTR_g_pGameRules_00177e70 + 0xcc))
                              (*(int **)PTR_g_pGameRules_00177e70,param_1), iVar9 == 1))
    goto LAB_000961af;
    iVar9 = (*this->vtbl->IsPlayer)(this);
    if (iVar9 != 0) {
      cVar8 = VictimKungfuHold__FP11CBasePlayerP11CBaseEntityfP11TraceResult
                        (param_1,(CBaseEntity *)this,param_6,local_5c);
      if (cVar8 != '\0') goto LAB_000961af;
      if ((param_8 != '\0') && ((float)this->field_0x71c < (*PTR_gpGlobals_00177410)->time)) {
        CBasePlayer::DropWeapon(this,0xfffffffe,param_6);
      }
      if ((param_10 != '\0') && (fc_30 < param_6)) {
        peVar3 = param_1->pev;
        fVar12 = (peVar3->velocity).x;
        fVar1 = (peVar3->velocity).y;
        fVar2 = (peVar3->velocity).z;
        fVar12 = SQRT(fVar2 * fVar2 + fVar1 * fVar1 + fVar12 * fVar12);
        peVar3 = this->pev;
        peVar4 = param_1->pev;
        local_9c = (peVar3->origin).x - (peVar4->origin).x;
        local_98 = (peVar3->origin).y - (peVar4->origin).y;
        fVar1 = (peVar3->origin).z - (peVar4->origin).z;
        local_94 = SQRT(fVar1 * fVar1 + local_98 * local_98 + local_9c * local_9c);
        if (local_94 == fc_0) {
          local_94 = fc_1;
          local_9c = 0.0;
          local_98 = 0.0;
        }
        else {
          local_94 = fc_1 / local_94;
          local_9c = local_94 * local_9c;
          local_98 = local_94 * local_98;
          local_94 = local_94 * fVar1;
        }
        local_6c = fVar12 * local_9c;
        local_68 = fVar12 * local_98;
        local_64 = fVar12 * local_94;
        local_7c = (entvars_t *)((peVar3->velocity).x + fVar12 * local_9c);
        local_78 = (peVar3->velocity).y + fVar12 * local_98;
        local_74 = (peVar3->velocity).z + fVar12 * local_94;
        ppeVar10 = &local_7c;
        pVVar11 = &peVar3->velocity;
        for (iVar9 = 3; iVar9 != 0; iVar9 = iVar9 + -1) {
          pVVar11->x = (vec_t)*ppeVar10;
          ppeVar10 = ppeVar10 + 1;
          pVVar11 = (Vector *)&pVVar11->y;
        }
        (this->pev->velocity).z = 235.0;
      }
      a0 = this->pev->pContainingEntity;
      fVar12 = 0.5;
      uVar7 = (*PTR_g_engfuncs_001778b8->pfnRandomLong)(0,0x1f);
      in_stack_ffffff3c = (CBasePlayer *)(CONCAT31(extraout_var_02,uVar7) + 0x5f);
      EMIT_SOUND_DYN__FP7edict_siPCcffiif
                (a0,4,"player/kungfuhit.wav",1.0,0.8,0,(int)in_stack_ffffff3c,fVar12);
      if (((param_1->m_pActiveItem != (CBaseEntity *)0x0) && (fc_20 < param_6)) &&
         (*(int *)&param_1->m_pActiveItem->field_0xc8 == 0)) {
        peVar3 = param_1->pev;
        peVar4 = this->pev;
        in_stack_ffffff3c = (CBasePlayer *)&local_7c;
        local_7c = (entvars_t *)((peVar3->origin).x - (peVar4->origin).x);
        local_78 = (peVar3->origin).y - (peVar4->origin).y;
        local_74 = (peVar3->origin).z - (peVar4->origin).z;
        a1.y = fVar12;
        a1.x = (vec_t)in_stack_ffffff3c;
        a1.z = in_stack_ffffff44;
        CBasePlayer::PunchScreen(this,param_6 * fc_0_5,a1);
      }
    }
    puVar5 = PTR_glast_closecombat_00177730;
    *PTR_glast_closecombat_00177730 = 1;
    iVar9 = 0x96498;
    ClearMultiDamage__Fv();
    local_7c = (entvars_t *)*param_4;
    local_78 = (float)param_4[1];
    local_74 = (float)param_4[2];
    a2.y = (vec_t)local_5c;
    a2.x = (vec_t)&local_7c;
    a2.z = 1.5053904e-36;
    (*this->vtbl->TraceAttack)(this,param_1->pev,param_6,a2,(TraceResult *)in_stack_ffffff3c,iVar9);
    ApplyMultiDamage__FP9entvars_sT0(param_1->pev,param_1->pev);
    *puVar5 = 0;
  }
  return this;
}


