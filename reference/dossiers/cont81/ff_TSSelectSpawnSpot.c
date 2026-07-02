// ===== FUN @ 00076244 size=1371 =====

/* WARNING: Unknown calling convention -- yet parameter storage is locked */
/* TS_gamerules.cpp */

undefined4 TSSelectSpawnSpot__FPCcP11CBaseEntityT0(char *a0,CBaseEntity *a1,char *a2)

{
  entvars_t *peVar1;
  entvars_t *peVar2;
  float fVar3;
  float fVar4;
  float fVar5;
  bool bVar6;
  bool bVar7;
  enginefuncs_t *peVar8;
  char cVar9;
  undefined uVar10;
  int iVar11;
  undefined3 extraout_var;
  int *piVar12;
  int iVar13;
  undefined3 extraout_var_00;
  undefined3 extraout_var_01;
  int iVar14;
  undefined1 *puVar15;
  uint uVar16;
  CBaseEntity *pCVar17;
  undefined1 *puVar18;
  undefined4 *puVar19;
  CBaseEntity *a0_00;
  int iVar20;
  undefined4 uVar21;
  undefined4 uVar22;
  int local_1a0;
  uint local_19c;
  float local_18c;
  float local_188;
  float local_184;
  undefined1 local_17c [16];
  float local_16c;
  int aiStack_140 [33];
  undefined4 local_bc [32];
  char local_3c [32];
  undefined1 auStack_1c [12];
  
  strcpy(local_3c,a0);
  if ((a2 != (char *)0x0) &&
     (iVar11 = UTIL_FindEntityByClassname__FP11CBaseEntityPCc((CBaseEntity *)0x0,a0), iVar11 == 0))
  {
    strcpy(local_3c,a2);
  }
  pCVar17 = (CBaseEntity *)0x0;
  puVar19 = local_bc;
  for (iVar11 = 0x20; iVar11 != 0; iVar11 = iVar11 + -1) {
    *puVar19 = 0;
    puVar19 = puVar19 + 1;
  }
  piVar12 = aiStack_140;
  for (iVar11 = 0x20; piVar12 = piVar12 + 1, iVar11 != 0; iVar11 = iVar11 + -1) {
    *piVar12 = 0;
  }
  local_19c = 0;
  do {
    do {
      pCVar17 = (CBaseEntity *)UTIL_FindEntityByClassname__FP11CBaseEntityPCc(pCVar17,local_3c);
      if (pCVar17 == (CBaseEntity *)0x0) goto LAB_00076585;
      iVar11 = (*pCVar17->vtbl->IsTriggered)(pCVar17,a1);
    } while (iVar11 == 0);
    a0_00 = (CBaseEntity *)0x0;
    bVar6 = true;
    bVar7 = false;
    local_1a0 = 0x1000;
    while (a0_00 = (CBaseEntity *)UTIL_FindEntityByClassname__FP11CBaseEntityPCc(a0_00,"player"),
          a0_00 != (CBaseEntity *)0x0) {
      if ((((a0_00 != a1) && (iVar11 = (*a0_00->vtbl->IsPlayer)(a0_00), iVar11 != 0)) &&
          (iVar11 = (*a0_00->vtbl->IsAlive)(a0_00), iVar11 != 0)) &&
         (peVar1 = a0_00->pev, -1 < (char)peVar1->effects)) {
        if (bVar6) {
          local_18c = (peVar1->origin).x + (peVar1->view_ofs).x;
          local_188 = (peVar1->origin).y + (peVar1->view_ofs).y;
          local_184 = (peVar1->origin).z + (peVar1->view_ofs).z;
          UTIL_TraceLine__FRC6VectorT015IGNORE_MONSTERS12IGNORE_GLASSP7edict_sP11TraceResult
                    (&pCVar17->pev->origin,&local_18c,1,1,pCVar17->pev->pContainingEntity,local_17c)
          ;
          if (local_16c == (float)dc_1) {
            iVar11 = (**(code **)(**(int **)PTR_g_pGameRules_00177e70 + 0xcc))
                               (*(int **)PTR_g_pGameRules_00177e70,a1,a0_00);
            if (iVar11 == 1) {
              peVar1 = pCVar17->pev;
              peVar2 = a0_00->pev;
              fVar3 = (peVar1->origin).x - (peVar2->origin).x;
              fVar5 = (peVar1->origin).y - (peVar2->origin).y;
              fVar4 = (peVar1->origin).z - (peVar2->origin).z;
              if (fc_128 <= SQRT(fVar4 * fVar4 + fVar5 * fVar5 + fVar3 * fVar3)) {
                bVar7 = true;
                goto LAB_0007645c;
              }
            }
            bVar6 = false;
          }
        }
LAB_0007645c:
        peVar1 = pCVar17->pev;
        peVar2 = a0_00->pev;
        local_18c = (peVar1->origin).x - (peVar2->origin).x;
        local_188 = (peVar1->origin).y - (peVar2->origin).y;
        local_184 = (peVar1->origin).z - (peVar2->origin).z;
        iVar11 = (int)ROUND(SQRT(local_184 * local_184 +
                                 local_188 * local_188 + local_18c * local_18c));
        if (iVar11 < local_1a0) {
          local_1a0 = iVar11;
        }
      }
    }
    local_bc[local_19c] = pCVar17;
    local_19c = local_19c + 1;
    if ((bVar7) && (bVar6)) {
      uVar22 = 0x2000;
      uVar21 = 0x1801;
LAB_0007653e:
      uVar10 = (*PTR_g_engfuncs_001778b8->pfnRandomLong)(uVar21,uVar22);
      aiStack_140[local_19c] = CONCAT31(extraout_var,uVar10);
    }
    else {
      if (bVar6) {
        uVar22 = 0x1800;
        uVar21 = 0x1001;
        goto LAB_0007653e;
      }
      aiStack_140[local_19c] = local_1a0;
    }
  } while ((int)local_19c < 0x20);
LAB_00076585:
  peVar8 = PTR_g_engfuncs_001778b8;
  if ((int)local_19c < 1) {
    piVar12 = (int *)UTIL_FindEntityByClassname__FP11CBaseEntityPCc
                               ((CBaseEntity *)0x0,"info_player_deathmatch");
    if (piVar12 == (int *)0x0) {
      piVar12 = (int *)UTIL_FindEntityByClassname__FP11CBaseEntityPCc
                                 ((CBaseEntity *)0x0,"info_player_start");
    }
    if (piVar12 == (int *)0x0) {
      return 0;
    }
    return *(undefined4 *)(*piVar12 + 0x208);
  }
  iVar11 = 0;
  iVar20 = 0;
  iVar13 = 0;
  if ((int)local_19c < 1) goto LAB_000766a8;
  iVar14 = 0;
  uVar16 = local_19c & 3;
  if ((int)local_19c < 1) {
LAB_00076631:
    if (iVar11 < *(int *)((int)aiStack_140 + iVar14 + 4)) {
      iVar11 = *(int *)((int)aiStack_140 + iVar14 + 4);
      iVar13 = iVar20;
    }
    iVar14 = iVar14 + 4;
    iVar20 = iVar20 + 1;
    if ((int)local_19c <= iVar20) goto LAB_000766a8;
  }
  else if (uVar16 != 0) {
    if (1 < uVar16) {
      if (2 < uVar16) {
        if (0 < aiStack_140[1]) {
          iVar11 = aiStack_140[1];
        }
        iVar14 = 4;
        iVar20 = 1;
      }
      if (iVar11 < *(int *)((int)aiStack_140 + iVar14 + 4)) {
        iVar11 = *(int *)((int)aiStack_140 + iVar14 + 4);
        iVar13 = iVar20;
      }
      iVar14 = iVar14 + 4;
      iVar20 = iVar20 + 1;
    }
    goto LAB_00076631;
  }
  puVar15 = auStack_1c + iVar14;
  puVar18 = puVar15;
  do {
    if (iVar11 < *(int *)(puVar15 + -0x120)) {
      iVar11 = *(int *)(puVar18 + -0x120);
      iVar13 = iVar20;
    }
    if (iVar11 < *(int *)(puVar15 + -0x11c)) {
      iVar11 = *(int *)(puVar18 + -0x11c);
      iVar13 = iVar20 + 1;
    }
    if (iVar11 < *(int *)(puVar15 + -0x118)) {
      iVar11 = *(int *)(puVar18 + -0x118);
      iVar13 = iVar20 + 2;
    }
    if (iVar11 < *(int *)(puVar15 + -0x114)) {
      iVar11 = *(int *)(puVar18 + -0x114);
      iVar13 = iVar20 + 3;
    }
    puVar18 = puVar18 + 0x10;
    puVar15 = puVar15 + 0x10;
    iVar20 = iVar20 + 4;
  } while (iVar20 < (int)local_19c);
LAB_000766a8:
  piVar12 = (int *)local_bc[iVar13];
  if (aiStack_140[iVar13 + 1] < 0x80) {
    pCVar17 = (CBaseEntity *)0x0;
    while (pCVar17 = (CBaseEntity *)
                     UTIL_FindEntityInSphere__FP11CBaseEntityRC6Vectorf
                               (pCVar17,(Vector *)(*piVar12 + 8),128.0),
          pCVar17 != (CBaseEntity *)0x0) {
      iVar11 = (*pCVar17->vtbl->IsPlayer)(pCVar17);
      if (((iVar11 != 0) && (pCVar17 != a1)) &&
         ((iVar11 = (*pCVar17->vtbl->IsAlive)(pCVar17), iVar11 != 0 &&
          (-1 < (char)pCVar17->pev->effects)))) {
        cVar9 = (*peVar8->pfnPEntityOfEntIndex)(0);
        uVar10 = (*peVar8->pfnPEntityOfEntIndex)(0);
        (*pCVar17->vtbl->TakeDamage)
                  (pCVar17,(entvars_s *)
                           (CONCAT31(extraout_var_01,uVar10) + 0x80U &
                           (CONCAT31(extraout_var_01,uVar10) == 0) - 1),
                   (T1_)(cVar9 + 0x80U & (CONCAT31(extraout_var_00,cVar9) == 0) - 1U),300.0,0x1000);
      }
    }
  }
  *(int **)PTR_g_pLastSpawn_00177f10 = piVar12;
  return *(undefined4 *)(*piVar12 + 0x208);
}
