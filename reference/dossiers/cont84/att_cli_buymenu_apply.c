// ===== FUN @ 10039400 size=1039 =====

uint __fastcall FUN_10039400(int param_1)

{
  char cVar1;
  bool bVar2;
  int iVar3;
  uint uVar4;
  undefined4 *puVar5;
  undefined4 uVar6;
  uint uVar7;
  int iVar8;
  int iVar9;
  int *piVar10;
  int *piVar11;
  int *piVar12;
  char *pcVar13;
  char *pcVar14;
  undefined4 *puVar15;
  char *pcVar16;
  int local_1fc;
  char acStack_1f8 [64];
  char acStack_1b8 [128];
  int local_138 [14];
  char acStack_100 [128];
  char acStack_80 [128];
  
  if (*(int *)(param_1 + 0x414) < 1) {
    FUN_1004b520(&DAT_100d43cc,s__Buy_ConfigEmpty_10095154,0xc3,0x2d,0x2d,0x14,0);
  }
  iVar3 = FUN_10036640();
  if (iVar3 < *(int *)(param_1 + 0x414)) {
    uVar4 = FUN_1004b520(&DAT_100d43cc,s__Buy_NoCash_10095148,0xc3,0x2d,0x2d,0x14,0);
    return uVar4 & 0xffffff00;
  }
  uVar4 = FUN_10036660();
  if ((int)uVar4 < *(int *)(param_1 + 0x418)) {
    return uVar4 & 0xffffff00;
  }
  do {
    bVar2 = false;
    piVar10 = (int *)(param_1 + 0x98);
    local_1fc = 0xf;
    do {
      if ((0 < *piVar10) && (0 < piVar10[0xe])) {
        if (((char)piVar10[-1] == '\0') && ((char)piVar10[0xd] == '\0')) {
          puVar5 = FUN_100071c0(*piVar10);
          if ((*(char *)(puVar5 + 2) == '\0') ||
             (puVar5 = FUN_100071c0(piVar10[0xe]), *(char *)(puVar5 + 2) == '\0')) {
            puVar5 = FUN_100071c0(*piVar10);
            if ((*(char *)(puVar5 + 2) == '\0') &&
               (puVar5 = FUN_100071c0(piVar10[0xe]), *(char *)(puVar5 + 2) == '\0')) {
              if (piVar10[0x10] < piVar10[2]) goto LAB_10039546;
            }
            else {
              puVar5 = FUN_100071c0(*piVar10);
              if (*(char *)(puVar5 + 2) != '\0') {
LAB_10039546:
                piVar11 = piVar10 + -1;
                piVar12 = local_138;
                for (iVar3 = 0xe; iVar3 != 0; iVar3 = iVar3 + -1) {
                  *piVar12 = *piVar11;
                  piVar11 = piVar11 + 1;
                  piVar12 = piVar12 + 1;
                }
                bVar2 = true;
                piVar11 = piVar10 + 0xd;
                piVar12 = piVar10 + -1;
                for (iVar3 = 0xe; iVar3 != 0; iVar3 = iVar3 + -1) {
                  *piVar12 = *piVar11;
                  piVar11 = piVar11 + 1;
                  piVar12 = piVar12 + 1;
                }
                piVar11 = local_138;
                piVar12 = piVar10 + 0xd;
                for (iVar3 = 0xe; iVar3 != 0; iVar3 = iVar3 + -1) {
                  *piVar12 = *piVar11;
                  piVar11 = piVar11 + 1;
                  piVar12 = piVar12 + 1;
                }
              }
            }
          }
          else if (piVar10[0x10] < piVar10[2]) goto LAB_10039546;
        }
        else if (piVar10[0x10] < piVar10[2]) goto LAB_10039546;
      }
      piVar10 = piVar10 + 0xe;
      local_1fc = local_1fc + -1;
    } while (local_1fc != 0);
  } while (bVar2);
  (*DAT_100d5b74)(s_rst_cfg_10092a7c);
  uVar4 = 0xffffffff;
  pcVar16 = &DAT_10092a74;
  do {
    pcVar14 = pcVar16;
    if (uVar4 == 0) break;
    uVar4 = uVar4 - 1;
    pcVar14 = pcVar16 + 1;
    cVar1 = *pcVar16;
    pcVar16 = pcVar14;
  } while (cVar1 != '\0');
  uVar4 = ~uVar4;
  pcVar16 = pcVar14 + -uVar4;
  pcVar14 = acStack_1b8;
  for (uVar7 = uVar4 >> 2; uVar7 != 0; uVar7 = uVar7 - 1) {
    *(undefined4 *)pcVar14 = *(undefined4 *)pcVar16;
    pcVar16 = pcVar16 + 4;
    pcVar14 = pcVar14 + 4;
  }
  iVar3 = 0x10;
  for (uVar4 = uVar4 & 3; uVar4 != 0; uVar4 = uVar4 - 1) {
    *pcVar14 = *pcVar16;
    pcVar16 = pcVar16 + 1;
    pcVar14 = pcVar14 + 1;
  }
  uVar4 = 0xffffffff;
  pcVar16 = &DAT_10092a6c;
  do {
    pcVar14 = pcVar16;
    if (uVar4 == 0) break;
    uVar4 = uVar4 - 1;
    pcVar14 = pcVar16 + 1;
    cVar1 = *pcVar16;
    pcVar16 = pcVar14;
  } while (cVar1 != '\0');
  uVar4 = ~uVar4;
  pcVar16 = pcVar14 + -uVar4;
  pcVar14 = acStack_80;
  for (uVar7 = uVar4 >> 2; uVar7 != 0; uVar7 = uVar7 - 1) {
    *(undefined4 *)pcVar14 = *(undefined4 *)pcVar16;
    pcVar16 = pcVar16 + 4;
    pcVar14 = pcVar14 + 4;
  }
  for (uVar4 = uVar4 & 3; uVar4 != 0; uVar4 = uVar4 - 1) {
    *pcVar14 = *pcVar16;
    pcVar16 = pcVar16 + 1;
    pcVar14 = pcVar14 + 1;
  }
  uVar4 = 0xffffffff;
  pcVar16 = &DAT_10092a64;
  do {
    pcVar14 = pcVar16;
    if (uVar4 == 0) break;
    uVar4 = uVar4 - 1;
    pcVar14 = pcVar16 + 1;
    cVar1 = *pcVar16;
    pcVar16 = pcVar14;
  } while (cVar1 != '\0');
  uVar4 = ~uVar4;
  pcVar16 = pcVar14 + -uVar4;
  pcVar14 = acStack_100;
  for (uVar7 = uVar4 >> 2; uVar7 != 0; uVar7 = uVar7 - 1) {
    *(undefined4 *)pcVar14 = *(undefined4 *)pcVar16;
    pcVar16 = pcVar16 + 4;
    pcVar14 = pcVar14 + 4;
  }
  for (uVar4 = uVar4 & 3; uVar4 != 0; uVar4 = uVar4 - 1) {
    *pcVar14 = *pcVar16;
    pcVar16 = pcVar16 + 1;
    pcVar14 = pcVar14 + 1;
  }
  piVar10 = (int *)(param_1 + 0x98);
  do {
    if (*piVar10 != 0) {
      if ((char)piVar10[-1] == '\0') {
        puVar5 = FUN_100071c0(*piVar10);
        if ((*(char *)(puVar5 + 0x37) == '\0') && ((char)piVar10[0xb] != '\0')) {
          FUN_1006ac7a(acStack_1f8,(byte *)s__i__i_10092a5c);
          uVar4 = 0xffffffff;
          pcVar16 = acStack_1f8;
          do {
            pcVar14 = pcVar16;
            if (uVar4 == 0) break;
            uVar4 = uVar4 - 1;
            pcVar14 = pcVar16 + 1;
            cVar1 = *pcVar16;
            pcVar16 = pcVar14;
          } while (cVar1 != '\0');
          uVar4 = ~uVar4;
          iVar8 = -1;
          pcVar16 = acStack_1b8;
          do {
            pcVar13 = pcVar16;
            if (iVar8 == 0) break;
            iVar8 = iVar8 + -1;
            pcVar13 = pcVar16 + 1;
            cVar1 = *pcVar16;
            pcVar16 = pcVar13;
          } while (cVar1 != '\0');
          pcVar16 = pcVar14 + -uVar4;
          pcVar14 = pcVar13 + -1;
          for (uVar7 = uVar4 >> 2; uVar7 != 0; uVar7 = uVar7 - 1) {
            *(undefined4 *)pcVar14 = *(undefined4 *)pcVar16;
            pcVar16 = pcVar16 + 4;
            pcVar14 = pcVar14 + 4;
          }
          for (uVar4 = uVar4 & 3; uVar4 != 0; uVar4 = uVar4 - 1) {
            *pcVar14 = *pcVar16;
            pcVar16 = pcVar16 + 1;
            pcVar14 = pcVar14 + 1;
          }
          FUN_1006ac7a(acStack_1f8,(byte *)s__i__i_10092a5c);
        }
        else {
          FUN_1006ac7a(acStack_1f8,(byte *)s__i__i_10092a5c);
        }
        pcVar16 = acStack_1b8;
      }
      else {
        FUN_1006ac7a(acStack_1f8,&DAT_10091200);
        pcVar16 = acStack_100;
      }
      uVar4 = 0xffffffff;
      pcVar14 = acStack_1f8;
      do {
        pcVar13 = pcVar14;
        if (uVar4 == 0) break;
        uVar4 = uVar4 - 1;
        pcVar13 = pcVar14 + 1;
        cVar1 = *pcVar14;
        pcVar14 = pcVar13;
      } while (cVar1 != '\0');
      uVar4 = ~uVar4;
      iVar8 = -1;
      do {
        pcVar14 = pcVar16;
        if (iVar8 == 0) break;
        iVar8 = iVar8 + -1;
        pcVar14 = pcVar16 + 1;
        cVar1 = *pcVar16;
        pcVar16 = pcVar14;
      } while (cVar1 != '\0');
      pcVar16 = pcVar13 + -uVar4;
      pcVar14 = pcVar14 + -1;
      for (uVar7 = uVar4 >> 2; uVar7 != 0; uVar7 = uVar7 - 1) {
        *(undefined4 *)pcVar14 = *(undefined4 *)pcVar16;
        pcVar16 = pcVar16 + 4;
        pcVar14 = pcVar14 + 4;
      }
      for (uVar4 = uVar4 & 3; uVar4 != 0; uVar4 = uVar4 - 1) {
        *pcVar14 = *pcVar16;
        pcVar16 = pcVar16 + 1;
        pcVar14 = pcVar14 + 1;
      }
    }
    piVar10 = piVar10 + 0xe;
    iVar3 = iVar3 + -1;
  } while (iVar3 != 0);
  iVar8 = 0;
  iVar3 = param_1 + 0x18;
  do {
    if (*(int *)(iVar3 + -4) != 0) {
      FUN_1006ac7a(acStack_1f8,(byte *)s__i__i_10092a5c);
      uVar4 = 0xffffffff;
      pcVar16 = acStack_1f8;
      do {
        pcVar14 = pcVar16;
        if (uVar4 == 0) break;
        uVar4 = uVar4 - 1;
        pcVar14 = pcVar16 + 1;
        cVar1 = *pcVar16;
        pcVar16 = pcVar14;
      } while (cVar1 != '\0');
      uVar4 = ~uVar4;
      iVar9 = -1;
      pcVar16 = acStack_80;
      do {
        pcVar13 = pcVar16;
        if (iVar9 == 0) break;
        iVar9 = iVar9 + -1;
        pcVar13 = pcVar16 + 1;
        cVar1 = *pcVar16;
        pcVar16 = pcVar13;
      } while (cVar1 != '\0');
      pcVar16 = pcVar14 + -uVar4;
      pcVar14 = pcVar13 + -1;
      for (uVar7 = uVar4 >> 2; uVar7 != 0; uVar7 = uVar7 - 1) {
        *(undefined4 *)pcVar14 = *(undefined4 *)pcVar16;
        pcVar16 = pcVar16 + 4;
        pcVar14 = pcVar14 + 4;
      }
      for (uVar4 = uVar4 & 3; uVar4 != 0; uVar4 = uVar4 - 1) {
        *pcVar14 = *pcVar16;
        pcVar16 = pcVar16 + 1;
        pcVar14 = pcVar14 + 1;
      }
    }
    iVar8 = iVar8 + 1;
    iVar3 = iVar3 + 8;
    if (0xf < iVar8) {
      (*DAT_100d5b74)(acStack_1b8);
      (*DAT_100d5b74)(acStack_100);
      (*DAT_100d5b74)(acStack_80);
      if (*(char *)(param_1 + 0x2f08) == '\0') {
        pcVar16 = s__Buy_UsingLastConfig_10095130;
      }
      else {
        pcVar16 = s__Buy_UsingNewConfig_1009511c;
      }
      uVar6 = FUN_1004b520(&DAT_100d43cc,pcVar16,0,0,0,0x14,0);
      *(undefined1 *)(param_1 + 0x2f08) = 0;
      puVar5 = (undefined4 *)(param_1 + 0x14);
      puVar15 = (undefined4 *)(param_1 + 0x41c);
      for (iVar3 = 0x102; iVar3 != 0; iVar3 = iVar3 + -1) {
        *puVar15 = *puVar5;
        puVar5 = puVar5 + 1;
        puVar15 = puVar15 + 1;
      }
      return CONCAT31((int3)((uint)uVar6 >> 8),1);
    }
  } while( true );
}


