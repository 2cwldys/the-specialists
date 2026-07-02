// ===== FUN @ 00089140 size=1172 =====

/* WARNING: Unknown calling convention -- yet parameter storage is locked */
/* TS_multiplay_gamerules.cpp */

uint TSGetPointsForFrag__FP11CBasePlayerP9entvars_sT1iPbT4
               (CBasePlayer *a0,entvars_s *a1,entvars_s *a2,int a3,bool *a4,bool *a5)

{
  float fVar1;
  float fVar2;
  float fVar3;
  undefined uVar4;
  undefined3 extraout_var;
  int iVar6;
  undefined3 extraout_var_00;
  undefined3 extraout_var_01;
  char *pcVar7;
  int iVar8;
  CTSKnife *this;
  byte *pbVar9;
  undefined4 *puVar10;
  char *pcVar11;
  byte *pbVar12;
  char *pcVar13;
  bool bVar14;
  bool bVar15;
  char local_9c [64];
  char local_5c [76];
  edict_t *peVar5;
  
  if (a3 < 0) {
    return a3;
  }
  peVar5 = a1->pContainingEntity;
  if (peVar5 == (edict_t *)0x0) {
    uVar4 = (*PTR_g_engfuncs_001778b8->pfnPEntityOfEntOffset)(0);
    peVar5 = (edict_t *)CONCAT31(extraout_var,uVar4);
  }
  iVar8 = 0;
  if (peVar5 != (edict_t *)0x0) {
    iVar8 = *(int *)(peVar5 + 0x7c);
  }
  local_5c[0] = '\0';
  local_9c[0] = '\0';
  if (((*(int *)PTR_g_theOneMode_001770dc != 0) &&
      (iVar6 = (**(code **)(*(int *)(iVar8 + 0x60) + 200))(iVar8), iVar6 != 0)) &&
     (*(char *)(iVar8 + 0x7b9) == '\0')) {
    return 0;
  }
  if ((a1->flags & 8) == 0) goto LAB_00089542;
  strcpy(local_9c,(*PTR_gpGlobals_00177410)->pStringBase + a1->netname);
  if (a2 != (entvars_s *)0x0) {
    if (a2 == a1) {
      peVar5 = a1->pContainingEntity;
      if (peVar5 == (edict_t *)0x0) {
        uVar4 = (*PTR_g_engfuncs_001778b8->pfnPEntityOfEntOffset)(0);
        peVar5 = (edict_t *)CONCAT31(extraout_var_00,uVar4);
      }
      iVar6 = 0;
      if (peVar5 != (edict_t *)0x0) {
        iVar6 = *(int *)(peVar5 + 0x7c);
      }
      if (*(int *)PTR_glast_weaponid_00177d2c == -1) {
        if (*(int *)(iVar6 + 0xa10) == 0) {
          if (*PTR_glast_closecombat_00177730 != '\0') {
            puVar10 = &DAT_00108787;
            pcVar11 = local_5c;
            for (iVar6 = 2; iVar6 != 0; iVar6 = iVar6 + -1) {
              *(undefined4 *)pcVar11 = *puVar10;
              puVar10 = puVar10 + 1;
              pcVar11 = pcVar11 + 4;
            }
            pcVar13 = pcVar11 + 2;
            pcVar7 = (char *)((int)puVar10 + 2);
            *(undefined2 *)pcVar11 = *(undefined2 *)puVar10;
            goto LAB_00089382;
          }
        }
        else if ((*(int *)(*(int *)(iVar6 + 0xa10) + 0x88) == 0x10) &&
                ((*(int *)(*(int *)(iVar6 + 0xa10) + 0xcc) == 0 ||
                 (*PTR_glast_closecombat_00177730 != '\0')))) {
          puVar10 = &DAT_00108787;
          pcVar11 = local_5c;
          for (iVar6 = 2; iVar6 != 0; iVar6 = iVar6 + -1) {
            *(undefined4 *)pcVar11 = *puVar10;
            puVar10 = puVar10 + 1;
            pcVar11 = pcVar11 + 4;
          }
          pcVar13 = pcVar11 + 2;
          pcVar7 = (char *)((int)puVar10 + 2);
          *(undefined2 *)pcVar11 = *(undefined2 *)puVar10;
LAB_00089382:
          *pcVar13 = *pcVar7;
          goto LAB_00089383;
        }
      }
    }
    else {
      bVar14 = CARRY4((uint)(*PTR_gpGlobals_00177410)->pStringBase,a2->classname);
      pbVar9 = (byte *)((*PTR_gpGlobals_00177410)->pStringBase + a2->classname);
      bVar15 = pbVar9 == (byte *)0x0;
      iVar6 = 6;
      pbVar12 = &DAT_001077e5;
      do {
        if (iVar6 == 0) break;
        iVar6 = iVar6 + -1;
        bVar14 = *pbVar9 < *pbVar12;
        bVar15 = *pbVar9 == *pbVar12;
        pbVar9 = pbVar9 + 1;
        pbVar12 = pbVar12 + 1;
      } while (bVar15);
      if ((!bVar14 && !bVar15) != bVar14) {
        return a3 & 0xff;
      }
      peVar5 = a2->pContainingEntity;
      if (peVar5 == (edict_t *)0x0) {
        uVar4 = (*PTR_g_engfuncs_001778b8->pfnPEntityOfEntOffset)(0);
        peVar5 = (edict_t *)CONCAT31(extraout_var_01,uVar4);
      }
      this = (CTSKnife *)0x0;
      if (peVar5 != (edict_t *)0x0) {
        this = *(CTSKnife **)(peVar5 + 0x7c);
      }
      if (this == (CTSKnife *)0x0) {
        pcVar7 = "Throwing";
        pcVar13 = local_5c;
        for (iVar6 = 2; iVar6 != 0; iVar6 = iVar6 + -1) {
          *(undefined4 *)pcVar13 = *(undefined4 *)pcVar7;
          pcVar7 = pcVar7 + 4;
          pcVar13 = pcVar13 + 4;
        }
        goto LAB_00089382;
      }
      pcVar7 = (char *)CTSKnife::GetDescription(this);
      strcpy(local_5c,pcVar7);
LAB_00089383:
      a3 = 2;
    }
  }
  if (((a1->iuser4 & 0x10c10U) == 0) ||
     (fVar1 = (a1->velocity).x, fVar2 = (a1->velocity).y, fVar3 = (a1->velocity).z,
     SQRT(fVar3 * fVar3 + fVar2 * fVar2 + fVar1 * fVar1) <= fc_80)) {
    if (((a1->iuser4 & 0x20) == 0) ||
       (((float)dc_1 <= a1->friction ||
        (fVar1 = (a1->velocity).x, fVar2 = (a1->velocity).y, fVar3 = (a1->velocity).z,
        SQRT(fVar3 * fVar3 + fVar2 * fVar2 + fVar1 * fVar1) <= fc_80)))) {
      if (local_5c[0] != '\0') {
        strcat(local_5c," #TS_Stl");
      }
    }
    else {
      if ((*PTR_glast_closecombat_00177730 == '\0') &&
         (iVar6 = (**(code **)(*(int *)(iVar8 + 0x60) + 200))(iVar8), iVar6 != 0)) {
        a3 = a3 + 1;
      }
      if (1 < (byte)a3) {
        if (local_5c[0] == '\0') {
          puVar10 = &DAT_001087ab;
          pcVar7 = local_5c;
          for (iVar6 = 2; iVar6 != 0; iVar6 = iVar6 + -1) {
            *(undefined4 *)pcVar7 = *puVar10;
            puVar10 = puVar10 + 1;
            pcVar7 = pcVar7 + 4;
          }
          *(undefined2 *)pcVar7 = *(undefined2 *)puVar10;
        }
        else {
          strcat(local_5c," & #TS_SlStl");
        }
      }
      *a4 = true;
    }
  }
  else {
    if ((*PTR_glast_closecombat_00177730 == '\0') &&
       (iVar6 = (**(code **)(*(int *)(iVar8 + 0x60) + 200))(iVar8), iVar6 != 0)) {
      a3 = a3 + 1;
    }
    if (1 < (byte)a3) {
      if (local_5c[0] == '\0') {
        pcVar7 = "#TS_StStl";
        pcVar13 = local_5c;
        for (iVar6 = 2; iVar6 != 0; iVar6 = iVar6 + -1) {
          *(undefined4 *)pcVar13 = *(undefined4 *)pcVar7;
          pcVar7 = pcVar7 + 4;
          pcVar13 = pcVar13 + 4;
        }
        *(undefined2 *)pcVar13 = *(undefined2 *)pcVar7;
      }
      else {
        strcat(local_5c," & #TS_StStl");
      }
    }
    *a5 = true;
  }
LAB_00089542:
  if (1 < (byte)a3) {
    if (iVar8 != 0) {
      (**(code **)(*(int *)(iVar8 + 0x60) + 200))(iVar8);
    }
    pcVar7 = (char *)UTIL_VarArgs__FPce("%i #TS_Frg #TS_FrgFr %s, %s");
    TSHudMessageAll__FUcUcUciiPc(0,0xff,0,1,0x1e,pcVar7);
  }
  return a3 & 0xff;
}
