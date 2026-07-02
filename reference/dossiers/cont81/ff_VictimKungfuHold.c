// ===== FUN @ 00076f50 size=498 =====

/* WARNING: Unknown calling convention -- yet parameter storage is locked */
/* TS_gamerules.cpp
   assets: player/kungfuhit.wav */

undefined4
VictimKungfuHold__FP11CBasePlayerP11CBaseEntityfP11TraceResult
          (CBasePlayer *a0,CBaseEntity *a1,float a2,TraceResult *a3)

{
  entvars_t *peVar1;
  entvars_t *peVar2;
  bool bVar3;
  float fVar4;
  int iVar5;
  undefined4 uVar6;
  float *pfVar7;
  float *pfVar8;
  CBasePlayer *local_48;
  float local_3c [4];
  float local_2c [7];
  
  iVar5 = (*a1->vtbl->IsPlayer)(a1);
  uVar6 = 0;
  if (iVar5 != 0) {
    bVar3 = false;
    local_48 = (CBasePlayer *)0x0;
    if (*(float *)&a1->field_0x7a8 <= fc_0) {
      if ((fc_0 < (float)a0->field_0x7a8) && (a1->field_0x7a4 == '\0')) {
        if (fc_0 < *(float *)&a1->field_0x79c) {
          bVar3 = true;
          local_48 = a0;
          a0 = (CBasePlayer *)a1;
        }
      }
    }
    else {
      bVar3 = true;
      local_48 = (CBasePlayer *)a1;
    }
    uVar6 = 0;
    if ((bVar3) && (local_48 != (CBasePlayer *)0x0)) {
      peVar1 = a0->pev;
      peVar2 = local_48->pev;
      local_2c[0] = (peVar1->origin).x - (peVar2->origin).x;
      local_2c[1] = (peVar1->origin).y - (peVar2->origin).y;
      local_2c[2] = (peVar1->origin).z - (peVar2->origin).z;
      fVar4 = SQRT(local_2c[2] * local_2c[2] + local_2c[1] * local_2c[1] + local_2c[0] * local_2c[0]
                  );
      if (fVar4 == fc_0) {
        local_3c[0] = 0.0;
        local_3c[1] = 0.0;
        local_3c[2] = fc_1;
      }
      else {
        fVar4 = fc_1 / fVar4;
        local_3c[0] = fVar4 * local_2c[0];
        local_3c[1] = fVar4 * local_2c[1];
        local_3c[2] = fVar4 * local_2c[2];
      }
      pfVar7 = local_3c;
      pfVar8 = local_2c;
      for (iVar5 = 3; iVar5 != 0; iVar5 = iVar5 + -1) {
        *pfVar8 = *pfVar7;
        pfVar7 = pfVar7 + 1;
        pfVar8 = pfVar8 + 1;
      }
      UTIL_MakeVectors__FRC6Vector(&local_48->pev->v_angle);
      uVar6 = 0;
      if (fc_0_10000000149011612 <= *(float *)&a1->field_0x774) {
        EMIT_SOUND_DYN__FP7edict_siPCcffiif
                  (a0->pev->pContainingEntity,4,"player/kungfuhit.wav",fc_1,fc_0_800000011920929,0,
                   100,fc_0_5);
        local_48->field_0x7a0 = 0;
        a0->field_0x7a0 = 0;
        a0->field_0x12c = 0x3f000000;
        local_48->field_0x12c = 0x3e800000;
        CBasePlayer::UpdateMeleeBar(a0,33.0);
        uVar6 = 1;
      }
    }
  }
  return uVar6;
}
