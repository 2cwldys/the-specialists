// ===== FUN @ 0009c630 size=220 =====

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */
/* WARNING: Unknown calling convention -- yet parameter storage is locked */
/* TS_stuntmanager.cpp
   assets: player/closecombat.wav */

undefined4 CTSStunt::CloseAttack(CTSStunt *this)

{
  globalvars_t *pgVar1;
  edict_s *a0;
  undefined uVar2;
  float fVar3;
  int iVar4;
  undefined3 extraout_var;
  undefined3 extraout_var_00;
  int a1;
  longdouble lVar5;
  undefined2 uVar6;
  CBasePlayer *a0_00;
  float *pfVar7;
  undefined2 uVar9;
  float fVar10;
  vec_t vStack_80;
  vec_t vStack_7c;
  vec_t vStack_78;
  float fStack_70;
  float fStack_6c;
  float fStack_68;
  float fStack_60;
  float fStack_5c;
  float fStack_58;
  float fStack_50;
  float fStack_4c;
  float fStack_48;
  float local_34;
  float fStack_30;
  float local_2c;
  float fStack_28;
  undefined2 uVar8;
  
  if (fc_0 < this->m_flRollTime) {
    return 0;
  }
  if ((*(uint *)(*(int *)this->field_0x40 + 0x250) & 0x30c30) != 0) {
    return 0;
  }
  if ((*(byte *)(*(int *)this->field_0x40 + 0x1a5) & 0x40) != 0) {
    return 0;
  }
  if (fc_0 < this->m_flStunCooldown) {
    return 0;
  }
  iVar4 = *(int *)(this->field_0x40 + 0xa10);
  if (((iVar4 != 0) && (*(int *)(iVar4 + 0x88) == 0x10)) && (*(int *)(iVar4 + 0xcc) == 0)) {
    return 0;
  }
  UTIL_MakeVectors__FRC6Vector((Vector *)(*(int *)this->field_0x40 + 0x74));
  pfVar7 = &local_2c;
  iVar4 = this->field_0x40;
  (**(code **)(*(int *)(iVar4 + 0x60) + 0x168))(pfVar7,iVar4);
  uVar8 = (undefined2)((uint)pfVar7 >> 0x10);
  uVar9 = (undefined2)((uint)iVar4 >> 0x10);
  iVar4 = *(int *)this->field_0x40;
  if (SQRT(*(float *)(iVar4 + 0x28) * *(float *)(iVar4 + 0x28) +
           *(float *)(iVar4 + 0x24) * *(float *)(iVar4 + 0x24) +
           *(float *)(iVar4 + 0x20) * *(float *)(iVar4 + 0x20)) * (float)_UNK_00109b10 <=
      (float)UNK_00107698) {
    iVar4 = *(int *)this->field_0x40;
    local_34 = SQRT(*(float *)(iVar4 + 0x28) * *(float *)(iVar4 + 0x28) +
                    *(float *)(iVar4 + 0x24) * *(float *)(iVar4 + 0x24) +
                    *(float *)(iVar4 + 0x20) * *(float *)(iVar4 + 0x20));
    fVar3 = fc_10;
    if (local_34 * (float)_UNK_00109b10 < (float)dc_10) goto LAB_0009c7e6;
  }
  iVar4 = *(int *)this->field_0x40;
  local_34 = SQRT(*(float *)(iVar4 + 0x28) * *(float *)(iVar4 + 0x28) +
                  *(float *)(iVar4 + 0x24) * *(float *)(iVar4 + 0x24) +
                  *(float *)(iVar4 + 0x20) * *(float *)(iVar4 + 0x20));
  fVar3 = fc_20;
  if (local_34 * (float)_UNK_00109b10 <= (float)UNK_00107698) {
    iVar4 = *(int *)this->field_0x40;
    local_34 = SQRT(*(float *)(iVar4 + 0x28) * *(float *)(iVar4 + 0x28) +
                    *(float *)(iVar4 + 0x24) * *(float *)(iVar4 + 0x24) +
                    *(float *)(iVar4 + 0x20) * *(float *)(iVar4 + 0x20)) * (float)_UNK_00109b10;
    fVar3 = local_34;
  }
LAB_0009c7e6:
  a0_00 = (CBasePlayer *)this->field_0x40;
  uVar6 = 9;
  iVar4 = KungFu__FP11CBasePlayer(a0_00);
  fVar10 = fVar3;
  if (iVar4 != 0) {
    fVar10 = fVar3 + fVar3;
  }
  *(undefined4 *)(this->field_0x40 + 0x7a8) = 0;
  fStack_50 = fStack_30;
  fStack_4c = local_2c;
  fStack_48 = fStack_28;
  pgVar1 = *PTR_gpGlobals_00177410;
  fStack_70 = fc_48 * (pgVar1->v_forward).x;
  fStack_6c = fc_48 * (pgVar1->v_forward).y;
  fStack_68 = fc_48 * (pgVar1->v_forward).z;
  fStack_60 = fStack_30 + fStack_70;
  fStack_5c = fStack_6c + local_2c;
  fStack_58 = fStack_28 + fStack_68;
  vStack_80 = (pgVar1->v_forward).x;
  vStack_7c = (pgVar1->v_forward).y;
  vStack_78 = (pgVar1->v_forward).z;
  iVar4 = CloseCombatAttack__11CBasePlayerG6VectorN21ifibN37
                    (this->field_0x40,&fStack_50,&fStack_60,&vStack_80,3,fVar10,0xffffffff,
                     CONCAT22(uVar6,1),(uint)a0_00 & 0xffff0000,CONCAT22(uVar8,1),CONCAT22(uVar9,1))
  ;
  if (iVar4 == 0) {
    return 0;
  }
  uVar2 = (*PTR_g_engfuncs_001778b8->pfnRandomLong)(0,2);
  iVar4 = CONCAT31(extraout_var,uVar2);
  a1 = 0xe;
  if ((iVar4 != 1) && ((iVar4 < 2 || (a1 = 0xd, iVar4 != 2)))) {
    a1 = 10;
  }
  this->m_flStunCooldown = 0.7;
  CTSAnimation::Animate
            ((CTSAnimation *)(this->field_0x40 + 0x28c),(CBasePlayer *)this->field_0x40,a1);
  iVar4 = this->field_0x40;
  lVar5 = (longdouble)UTIL_WeaponTimeBase__Fv();
  if ((longdouble)*(float *)(iVar4 + 300) < lVar5) {
    lVar5 = (longdouble)UTIL_WeaponTimeBase__Fv();
    *(float *)(this->field_0x40 + 300) = (float)(lVar5 + (longdouble)fc_0_3499999940395355);
  }
  a0 = *(edict_s **)(*(int *)this->field_0x40 + 0x208);
  fVar10 = 0.5;
  uVar2 = (*PTR_g_engfuncs_001778b8->pfnRandomLong)(0,0x1f);
  EMIT_SOUND_DYN__FP7edict_siPCcffiif
            (a0,4,"player/closecombat.wav",1.0,0.8,0,CONCAT31(extraout_var_00,uVar2) + 0x5f,fVar10);
  CBasePlayer::UpdateMeleeBar((CBasePlayer *)this->field_0x40,fVar3 * fc_1_5);
  return 1;
}


