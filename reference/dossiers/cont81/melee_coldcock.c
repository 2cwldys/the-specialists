// ===== FUN @ 00096610 size=1158 =====

CBasePlayer *
ColdCockAttack__11CBasePlayerG6VectorN21ifibN37
          (CBasePlayer *param_1,float *param_2,float *param_3,undefined4 *param_4,undefined4 param_5
          ,float param_6,CBasePlayer *param_7,char param_8,char param_9,char param_10,char param_11)

{
  float fVar1;
  float fVar2;
  float fVar3;
  edict_s *a0;
  entvars_t *peVar4;
  entvars_t *peVar5;
  Vector a2;
  Vector a1;
  bool bVar6;
  undefined *puVar7;
  undefined uVar8;
  char cVar9;
  undefined3 extraout_var;
  int iVar10;
  undefined3 extraout_var_00;
  CBasePlayer *this;
  entvars_t **ppeVar11;
  Vector *pVVar12;
  TraceResult *a3;
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
  
  CTSAnimation::Animate((CTSAnimation *)&param_1->field_0x28c,param_1,(int)param_7);
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
    uVar8 = (*PTR_g_engfuncs_001778b8->pfnPEntityOfEntOffset)(0);
    local_2c = CONCAT31(extraout_var,uVar8);
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
LAB_0009678c:
    this = (CBasePlayer *)0x0;
  }
  else {
    iVar10 = (*this->vtbl->IsPlayer)(this);
    if (iVar10 == 0) {
      if (param_9 != '\0') {
        return (CBasePlayer *)0x0;
      }
    }
    else if (((param_11 != '\0') || (fc_0 == *(float *)(PTR_friendlyfire_00177164 + 0xc))) &&
            (param_7 = this,
            iVar10 = (**(code **)(**(int **)PTR_g_pGameRules_00177e70 + 0xcc))
                               (*(int **)PTR_g_pGameRules_00177e70,param_1), iVar10 == 1))
    goto LAB_0009678c;
    bVar6 = true;
    iVar10 = (*this->vtbl->IsPlayer)(this);
    if (iVar10 != 0) {
      a3 = local_5c;
      cVar9 = VictimKungfuHold__FP11CBasePlayerP11CBaseEntityfP11TraceResult
                        (param_1,(CBaseEntity *)this,param_6,a3);
      if (cVar9 == '\0') {
        if ((param_8 != '\0') && ((float)this->field_0x71c < (*PTR_gpGlobals_00177410)->time)) {
          CBasePlayer::DropWeapon(this,0xfffffffe,param_6);
        }
        if ((param_10 != '\0') && (fc_30 < param_6)) {
          peVar4 = param_1->pev;
          fVar1 = (peVar4->velocity).x;
          fVar2 = (peVar4->velocity).y;
          fVar3 = (peVar4->velocity).z;
          fVar1 = SQRT(fVar3 * fVar3 + fVar2 * fVar2 + fVar1 * fVar1);
          peVar4 = this->pev;
          peVar5 = param_1->pev;
          local_9c = (peVar4->origin).x - (peVar5->origin).x;
          local_98 = (peVar4->origin).y - (peVar5->origin).y;
          fVar2 = (peVar4->origin).z - (peVar5->origin).z;
          local_94 = SQRT(fVar2 * fVar2 + local_98 * local_98 + local_9c * local_9c);
          if (local_94 == fc_0) {
            local_94 = fc_1;
            local_9c = 0.0;
            local_98 = 0.0;
          }
          else {
            local_94 = fc_1 / local_94;
            local_9c = local_94 * local_9c;
            local_98 = local_94 * local_98;
            local_94 = local_94 * fVar2;
          }
          local_6c = fVar1 * local_9c;
          local_68 = fVar1 * local_98;
          local_64 = fVar1 * local_94;
          local_7c = (entvars_t *)((peVar4->velocity).x + fVar1 * local_9c);
          local_78 = (peVar4->velocity).y + fVar1 * local_98;
          local_74 = (peVar4->velocity).z + fVar1 * local_94;
          ppeVar11 = &local_7c;
          pVVar12 = &peVar4->velocity;
          for (iVar10 = 3; iVar10 != 0; iVar10 = iVar10 + -1) {
            pVVar12->x = (vec_t)*ppeVar11;
            ppeVar11 = ppeVar11 + 1;
            pVVar12 = (Vector *)&pVVar12->y;
          }
          (this->pev->velocity).z = 235.0;
        }
        a0 = this->pev->pContainingEntity;
        a3 = (TraceResult *)0x3f000000;
        uVar8 = (*PTR_g_engfuncs_001778b8->pfnRandomLong)(0,0x1f);
        EMIT_SOUND_DYN__FP7edict_siPCcffiif
                  (a0,4,"player/kungfuhit.wav",1.0,0.8,0,CONCAT31(extraout_var_00,uVar8) + 0x5f,
                   (float)a3);
      }
      else {
        bVar6 = false;
      }
      peVar4 = param_1->pev;
      peVar5 = this->pev;
      param_7 = (CBasePlayer *)&local_7c;
      local_7c = (entvars_t *)((peVar4->origin).x - (peVar5->origin).x);
      local_78 = (peVar4->origin).y - (peVar5->origin).y;
      local_74 = (peVar4->origin).z - (peVar5->origin).z;
      a1.y = (vec_t)a3;
      a1.x = (vec_t)param_7;
      a1.z = in_stack_ffffff44;
      CBasePlayer::PunchScreen(this,param_6 * fc_0_5,a1);
    }
    puVar7 = PTR_glast_coldcock_00177e64;
    if (bVar6) {
      *PTR_glast_coldcock_00177e64 = 1;
      iVar10 = 0x96a31;
      ClearMultiDamage__Fv();
      local_7c = (entvars_t *)*param_4;
      local_78 = (float)param_4[1];
      local_74 = (float)param_4[2];
      a2.y = (vec_t)local_5c;
      a2.x = (vec_t)&local_7c;
      a2.z = 1.5053904e-36;
      (*this->vtbl->TraceAttack)(this,param_1->pev,param_6,a2,(TraceResult *)param_7,iVar10);
      ApplyMultiDamage__FP9entvars_sT0(param_1->pev,param_1->pev);
      *puVar7 = 0;
    }
  }
  return this;
}


