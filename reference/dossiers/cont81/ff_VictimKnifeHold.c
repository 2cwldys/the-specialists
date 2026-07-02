// ===== FUN @ 00076de4 size=361 =====

/* WARNING: Unknown calling convention -- yet parameter storage is locked */
/* TS_gamerules.cpp
   assets: weapons/knife/knife_hit.wav */

undefined2 VictimKnifeHold__FP11CBasePlayerP11CBaseEntityf(CBasePlayer *a0,CBaseEntity *a1,float a2)

{
  entvars_t *peVar1;
  entvars_t *peVar2;
  int iVar3;
  float *pfVar4;
  Vector *pVVar5;
  float local_3c [4];
  Vector local_2c [2];
  
  iVar3 = (*a1->vtbl->IsPlayer)(a1);
  if (iVar3 == 0) {
    return 0;
  }
  if (*(float *)&a1->field_0x7ac <= fc_0) {
    if (*(float *)&a1->field_0x7b0 <= fc_0) {
      return 0;
    }
    if (*(float *)&a1->field_0x774 <= fc_0_10000000149011612) {
      return 0;
    }
  }
  peVar1 = a0->pev;
  peVar2 = a1->pev;
  local_3c[0] = fc_0_5 * ((peVar1->origin).x + (peVar2->origin).x);
  local_3c[1] = fc_0_5 * ((peVar1->origin).y + (peVar2->origin).y);
  local_3c[2] = fc_0_5 * ((peVar1->origin).z + (peVar2->origin).z);
  pfVar4 = local_3c;
  pVVar5 = local_2c;
  for (iVar3 = 3; iVar3 != 0; iVar3 = iVar3 + -1) {
    pVVar5->x = *pfVar4;
    pfVar4 = pfVar4 + 1;
    pVVar5 = (Vector *)&pVVar5->y;
  }
  local_2c[0].z = fc_20 + local_2c[0].z;
  UTIL_Sparks__FRC6Vector(local_2c);
  EMIT_SOUND_DYN__FP7edict_siPCcffiif
            (a0->pev->pContainingEntity,4,"weapons/knife/knife_hit.wav",fc_1,fc_0_800000011920929,0,
             100,fc_0_5);
  a0->field_0x7a0 = 0;
  a0->field_0x12c = 0x3f000000;
  *(undefined4 *)&a1->field_0x12c = 0x3e800000;
  CBasePlayer::UpdateMeleeBar(a0,33.0);
  return 1;
}
