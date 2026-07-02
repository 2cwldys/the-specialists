// ===== FUN @ 00095bac size=633 =====

/* WARNING: Unknown calling convention -- yet parameter storage is locked */
/* TS_player.cpp */

void FindHullIntersectionplayer__FRC6VectorR11TraceResultPfT2P7edict_s
               (Vector *a0,TraceResult *a1,float *a2,float *a3,edict_s *a4)

{
  int iVar1;
  int iVar2;
  float *pfVar3;
  undefined4 *puVar4;
  float *pfVar5;
  TraceResult *pTVar6;
  float local_c8;
  int local_c4;
  int local_c0;
  float *local_bc [3];
  float local_b0;
  float local_ac;
  float local_a8;
  float local_a4;
  float local_9c;
  float local_98;
  float local_94;
  float local_8c [4];
  float local_7c;
  float local_78;
  float local_74;
  float local_6c [4];
  undefined4 local_5c [4];
  float local_4c;
  float local_48;
  float local_44;
  float local_40;
  
  local_bc[0] = a2;
  local_bc[1] = a3;
  local_6c[0] = *(float *)(a1 + 0x14);
  local_6c[1] = *(float *)(a1 + 0x18);
  local_6c[2] = *(float *)(a1 + 0x1c);
  local_c8 = fc_1000000;
  local_ac = *(float *)(a1 + 0x14) - a0->x;
  local_a8 = *(float *)(a1 + 0x18) - a0->y;
  local_a4 = *(float *)(a1 + 0x1c) - a0->z;
  local_9c = fc_2 * local_ac;
  local_98 = fc_2 * local_a8;
  local_94 = fc_2 * local_a4;
  local_8c[0] = a0->x + local_9c;
  local_8c[1] = a0->y + local_98;
  local_8c[2] = a0->z + local_94;
  pfVar3 = local_8c;
  pfVar5 = local_6c;
  for (iVar1 = 3; iVar1 != 0; iVar1 = iVar1 + -1) {
    *pfVar5 = *pfVar3;
    pfVar3 = pfVar3 + 1;
    pfVar5 = pfVar5 + 1;
  }
  UTIL_TraceLine__FRC6VectorT015IGNORE_MONSTERSP7edict_sP11TraceResult(a0,local_6c,0,a4,local_5c);
  if ((float)dc_1 <= local_4c) {
    local_c0 = 0;
    do {
      local_c4 = 0;
      do {
        iVar1 = 0;
        do {
          local_7c = local_6c[0] + *local_bc[local_c0];
          local_78 = local_6c[1] + local_bc[local_c4][1];
          local_74 = local_6c[2] + local_bc[iVar1][2];
          UTIL_TraceLine__FRC6VectorT015IGNORE_MONSTERSP7edict_sP11TraceResult
                    (a0,&local_7c,0,a4,local_5c);
          if (local_4c < (float)dc_1) {
            local_ac = local_48 - a0->x;
            local_a8 = local_44 - a0->y;
            local_a4 = local_40 - a0->z;
            local_b0 = SQRT(local_a4 * local_a4 + local_a8 * local_a8 + local_ac * local_ac);
            if (local_b0 < local_c8) {
              puVar4 = local_5c;
              pTVar6 = a1;
              for (iVar2 = 0xe; local_c8 = local_b0, iVar2 != 0; iVar2 = iVar2 + -1) {
                *(undefined4 *)pTVar6 = *puVar4;
                puVar4 = puVar4 + 1;
                pTVar6 = pTVar6 + 4;
              }
            }
          }
          iVar1 = iVar1 + 1;
        } while (iVar1 < 2);
        local_c4 = local_c4 + 1;
      } while (local_c4 < 2);
      local_c0 = local_c0 + 1;
    } while (local_c0 < 2);
  }
  else {
    puVar4 = local_5c;
    for (iVar1 = 0xe; iVar1 != 0; iVar1 = iVar1 + -1) {
      *(undefined4 *)a1 = *puVar4;
      puVar4 = puVar4 + 1;
      a1 = a1 + 4;
    }
  }
  return;
}
