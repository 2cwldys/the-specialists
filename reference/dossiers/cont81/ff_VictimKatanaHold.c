// ===== FUN @ 00076c8c size=342 =====

/* WARNING: Unknown calling convention -- yet parameter storage is locked */
/* TS_gamerules.cpp
   assets: weapons/knife/knife_hit.wav */

undefined2
VictimKatanaHold__FP11CBasePlayerP11CBaseEntityf(CBasePlayer *a0,CBaseEntity *a1,float a2)

{
  entvars_t *peVar1;
  entvars_t *peVar2;
  undefined2 uVar3;
  int iVar4;
  float *pfVar5;
  Vector *pVVar6;
  float local_3c [4];
  Vector local_2c [2];
  
  iVar4 = (*a1->vtbl->IsPlayer)(a1);
  uVar3 = 0;
  if (iVar4 != 0) {
    uVar3 = 0;
    if (fc_0 < *(float *)&a1->field_0x7b0) {
      uVar3 = 0;
      if (fc_0_10000000149011612 < *(float *)&a1->field_0x774) {
        peVar1 = a0->pev;
        peVar2 = a1->pev;
        local_3c[0] = fc_0_5 * ((peVar1->origin).x + (peVar2->origin).x);
        local_3c[1] = fc_0_5 * ((peVar1->origin).y + (peVar2->origin).y);
        local_3c[2] = fc_0_5 * ((peVar1->origin).z + (peVar2->origin).z);
        pfVar5 = local_3c;
        pVVar6 = local_2c;
        for (iVar4 = 3; iVar4 != 0; iVar4 = iVar4 + -1) {
          pVVar6->x = *pfVar5;
          pfVar5 = pfVar5 + 1;
          pVVar6 = (Vector *)&pVVar6->y;
        }
        local_2c[0].z = fc_20 + local_2c[0].z;
        UTIL_Sparks__FRC6Vector(local_2c);
        EMIT_SOUND_DYN__FP7edict_siPCcffiif
                  (a0->pev->pContainingEntity,4,"weapons/knife/knife_hit.wav",fc_1,
                   fc_0_800000011920929,0,100,fc_0_5);
        a0->field_0x7a0 = 0;
        a0->field_0x12c = 0x3f400000;
        *(undefined4 *)&a1->field_0x12c = 0x3eb33333;
        CBasePlayer::UpdateMeleeBar(a0,33.0);
        uVar3 = 1;
      }
    }
  }
  return uVar3;
}
