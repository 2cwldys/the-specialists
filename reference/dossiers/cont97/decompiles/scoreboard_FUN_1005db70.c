// ===== FUN @ 1005db70 size=2559 =====
void __fastcall FUN_1005db70(int *param_1)
{
  char cVar1;
  int iVar2;
  int iVar3;
  int *piVar4;
  undefined *puVar5;
  undefined4 uVar6;
  undefined4 uVar7;
  uint uVar8;
  uint uVar9;
  int iVar10;
  int iVar11;
  undefined4 *puVar12;
  int *piVar13;
  char *pcVar14;
  char *pcVar15;
  longlong lVar16;
  undefined2 uVar17;
  undefined2 uVar18;
  int *piVar19;
  byte *pbVar20;
  undefined2 uVar21;
  undefined2 uVar22;
  int *piStack_190;
  undefined4 uStack_18c;
  int iStack_188;
  void *pvVar23;
  undefined4 uVar24;
  int *piVar25;
  int *piVar26;
  int *this;
  int iStack_134;
  undefined1 auStack_12c [4];
  int *local_128;
  undefined1 auStack_124 [4];
  undefined4 local_120;
  undefined1 auStack_11c [8];
  int *local_114;
  undefined4 local_110;
  undefined4 local_108;
  char acStack_dc [220];
  pvVar23 = (void *)(DAT_100d5cb0 + 0x22c);
  local_114 = param_1;
  iVar2 = FUN_1005cc60(pvVar23,(byte *)s_Scoreboard_Text_10096858);
  iVar3 = FUN_1005cc60(pvVar23,(byte *)s_Scoreboard_Title_Text_100967cc);
  local_128 = (int *)FUN_1005cc60(pvVar23,(byte *)s_Scoreboard_Small_Text_100967b4);
  local_108 = FUN_1005cce0(pvVar23,iVar2);
  local_120 = FUN_1005cce0(pvVar23,iVar3);
  local_110 = FUN_1005cce0(pvVar23,(int)local_128);
  piVar4 = (int *)(**(code **)(*param_1 + 0x14c))();
  (**(code **)(*piVar4 + 0x34))();
  (**(code **)param_1[0x2f])();
  puVar5 = FUN_10017570();
  cVar1 = FUN_100188b0((int)puVar5);
  if (cVar1 == '\0') {
    param_1[0x26e] = -1;
  }
  piVar4 = param_1 + 0x9a16;
  piVar26 = param_1 + 0x13c3;
  this = param_1 + 0x349;
  iVar2 = 0;
  do {
    FUN_1005b1b0(this,0,'\0',0,0,0,0,0);
    if (iVar2 < param_1[0x9a15]) {
      iVar3 = 0;
      do {
        iVar11 = iVar2 + iVar3 * 0x4a;
        piVar13 = param_1 + iVar11 * 0x3a + 0x13c3;
        (**(code **)(param_1[iVar11 * 0x3a + 0x13c3] + 0x24))();
        (**(code **)(**(int **)(piVar13[0x34] + 0x28) + 0x70))(s_);
        (**(code **)(*piVar13 + 0x1fc))(0);
        piVar19 = local_128;
        (**(code **)(*piVar13 + 0x208))();
        piVar13[0x37] = 0;
        piVar13[0x38] = 0;
        if (DAT_100d590c < 0x1e1) {
          iVar11 = 0xf;
        }
        else {
          lVar16 = __ftol();
          iVar11 = (int)lVar16;
        }
        iVar10 = *piVar13;
        uVar6 = (**(code **)(iVar10 + 0x18))();
        (**(code **)(iVar10 + 8))(uVar6);
        puVar12 = (undefined4 *)0x0;
        uVar24 = 0xff;
        pvVar23 = (void *)0x0;
        (**(code **)(*piVar13 + 300))(0,0,0,0xff);
        piVar25 = (int *)0x0;
        if (piVar4[0x4a] == 3) {
          (**(code **)(*piVar13 + 0x22c))(&DAT_10094c94);
          goto LAB_1005e491;
        }
        if (piVar4[0x4a] == 1) {
          iVar11 = *piVar4;
          puVar12 = &DAT_1018f1b0 + iVar11 * 10;
          iVar10 = *(int *)(&DAT_1018f1d4 + iVar11 * 0x28) % DAT_10096970;
          iStack_188 = (&DAT_1009697c)[iVar10 * 3];
          uStack_18c = (&DAT_10096978)[iVar10 * 3];
          piStack_190 = (int *)(&DAT_10096974)[iVar10 * 3];
          (**(code **)(*piVar13 + 0x120))();
          uVar7 = 0x14;
          if (0x1df < DAT_100d590c) {
            lVar16 = __ftol();
            uVar7 = (undefined4)lVar16;
          }
          iVar10 = *piVar13;
          uVar7 = (**(code **)(iVar10 + 0x18))(uVar7);
          (**(code **)(iVar10 + 8))(uVar7);
          (**(code **)(*piVar13 + 0x208))(uVar24);
          iVar11 = *(int *)(&DAT_1018f1d4 + iVar11 * 0x28) % DAT_10096970;
          uVar21 = (undefined2)(&DAT_1009697c)[iVar11 * 3];
          uVar18 = (undefined2)(&DAT_10096978)[iVar11 * 3];
          uVar17 = (undefined2)(&DAT_10096974)[iVar11 * 3];
LAB_1005de43:
          uVar22 = 0;
          lVar16 = __ftol();
          FUN_1005b1b0(pvVar23,0,'\x01',(int)lVar16,uVar17,uVar18,uVar21,uVar22);
        }
        else {
          if (piVar4[0x4a] == 2) {
            iStack_188 = 100;
            uStack_18c = 100;
            piStack_190 = (int *)0x64;
            (**(code **)(*piVar13 + 0x120))();
            uVar7 = 0x14;
            if (0x1df < DAT_100d590c) {
              lVar16 = __ftol();
              uVar7 = (undefined4)lVar16;
            }
            iVar11 = *piVar13;
            uVar7 = (**(code **)(iVar11 + 0x18))(uVar7);
            (**(code **)(iVar11 + 8))(uVar7);
            (**(code **)(*piVar13 + 0x208))(uVar24);
            uVar21 = 100;
            uVar18 = 100;
            uVar17 = 100;
            goto LAB_1005de43;
          }
          iVar10 = (int)(short)(&DAT_101901fc)[*piVar4 * 0x10] % DAT_10096970;
          iStack_188 = (&DAT_1009697c)[iVar10 * 3];
          uStack_18c = (&DAT_10096978)[iVar10 * 3];
          piStack_190 = (int *)(&DAT_10096974)[iVar10 * 3];
          (**(code **)(*piVar13 + 0x120))();
          if ((&DAT_1018fbde)[*piVar4 * 0x14] == '\0') {
            if (((*piVar4 == *(int *)(iVar11 + 0x26ae8)) && (*(int *)(iVar11 + 0x26aec) != 0)) &&
               (DAT_100cf944 < (float)*(int *)(iVar11 + 0x26aec))) {
              iVar11 = *piVar13;
              lVar16 = __ftol();
              (**(code **)(iVar11 + 300))(0xff,0,0,(int)lVar16);
            }
          }
          else {
            (**(code **)(*piVar13 + 0x124))(2);
            iVar11 = (int)(short)(&DAT_101901fc)[*piVar4 * 0x10] % DAT_10096970;
            (**(code **)(*piVar13 + 300))
                      ((&DAT_10096974)[iVar11 * 3],(&DAT_10096978)[iVar11 * 3],
                       (&DAT_1009697c)[iVar11 * 3],0xc4);
          }
        }
        if ((iStack_188 == 1) || (iStack_188 == 2)) {
          iVar11 = *piVar13;
          uVar24 = 3;
LAB_1005e006:
          (**(code **)(iVar11 + 0x21c))(uVar24);
          piVar4 = piVar25;
        }
        else {
          if (iStack_188 == 0) {
            iVar11 = *piVar13;
            uVar24 = 4;
            goto LAB_1005e006;
          }
          (**(code **)(*piVar13 + 0x21c))(5);
          piVar4 = piVar25;
        }
        uVar8 = 0xffffffff;
        pcVar14 = s_;
        do {
          pcVar15 = pcVar14;
          if (uVar8 == 0) break;
          uVar8 = uVar8 - 1;
          pcVar15 = pcVar14 + 1;
          cVar1 = *pcVar14;
          pcVar14 = pcVar15;
        } while (cVar1 != '\0');
        uVar8 = ~uVar8;
        pcVar14 = pcVar15 + -uVar8;
        pcVar15 = &stack0xfffffea4;
        for (uVar9 = uVar8 >> 2; uVar9 != 0; uVar9 = uVar9 - 1) {
          *(undefined4 *)pcVar15 = *(undefined4 *)pcVar14;
          pcVar14 = pcVar14 + 4;
          pcVar15 = pcVar15 + 4;
        }
        for (uVar8 = uVar8 & 3; uVar8 != 0; uVar8 = uVar8 - 1) {
          *pcVar15 = *pcVar14;
          pcVar14 = pcVar14 + 1;
          pcVar15 = pcVar15 + 1;
        }
        iVar11 = piStack_190[0x4a];
        if (iVar11 != 0) {
          switch(uStack_18c) {
          case 1:
            if (iVar11 == 2) {
              pbVar20 = FUN_100349a0(s__Spectators_1009684c);
              FUN_1006ac7a(acStack_dc,pbVar20);
            }
            else {
              FUN_1006ac7a(acStack_dc,(byte *)(puVar12[9] * 0x40 + 0x8bc + DAT_100d5cb0));
            }
            uVar8 = 0xffffffff;
            pcVar14 = acStack_dc;
            do {
              pcVar15 = pcVar14;
              if (uVar8 == 0) break;
              uVar8 = uVar8 - 1;
              pcVar15 = pcVar14 + 1;
              cVar1 = *pcVar14;
              pcVar14 = pcVar15;
            } while (cVar1 != '\0');
            uVar8 = ~uVar8;
            pcVar14 = pcVar15 + -uVar8;
            pcVar15 = &stack0xfffffea4;
            for (uVar9 = uVar8 >> 2; uVar9 != 0; uVar9 = uVar9 - 1) {
              *(undefined4 *)pcVar15 = *(undefined4 *)pcVar14;
              pcVar14 = pcVar14 + 4;
              pcVar15 = pcVar15 + 4;
            }
            for (uVar8 = uVar8 & 3; uVar8 != 0; uVar8 = uVar8 - 1) {
              *pcVar15 = *pcVar14;
              pcVar14 = pcVar14 + 1;
              pcVar15 = pcVar15 + 1;
            }
            if (piStack_190[0x4a] == 1) {
              if (*(short *)((int)puVar12 + 0x1a) == 1) {
                FUN_100349a0(s__Player_10096844);
              }
              else {
                FUN_100349a0(s__Player_plural_1009682c);
              }
              FUN_1006ac7a(acStack_dc,(byte *)s___d__s__1009683c);
              (**(code **)(**(int **)(piVar13[0x34] + 0x28) + 0x70))(acStack_dc);
              (**(code **)(**(int **)(piVar13[0x34] + 0x28) + 0x78))(uVar6);
            }
            break;
          case 3:
            if (iVar11 == 1) {
              pbVar20 = &DAT_10093a90;
              goto LAB_1005e475;
            }
            break;
          case 4:
            if (iVar11 == 1) {
              pbVar20 = &DAT_10093a90;
              goto LAB_1005e475;
            }
            break;
          case 5:
            if (iVar11 == 1) goto LAB_1005e46a;
          }
          goto switchD_1005e05c_caseD_2;
        }
        switch(uStack_18c) {
        case 1:
          pbVar20 = &DAT_10096824;
          break;
        case 2:
          switch(*(undefined1 *)(&DAT_101901f4 + *piStack_190 * 8)) {
          case 1:
            uVar8 = 0xffffffff;
            pcVar14 = &DAT_10096814;
            do {
              pcVar15 = pcVar14;
              if (uVar8 == 0) break;
              uVar8 = uVar8 - 1;
              pcVar15 = pcVar14 + 1;
              cVar1 = *pcVar14;
              pcVar14 = pcVar15;
            } while (cVar1 != '\0');
            uVar8 = ~uVar8;
            pcVar14 = pcVar15 + -uVar8;
            pcVar15 = &stack0xfffffea4;
            for (uVar9 = uVar8 >> 2; uVar9 != 0; uVar9 = uVar9 - 1) {
              *(undefined4 *)pcVar15 = *(undefined4 *)pcVar14;
              pcVar14 = pcVar14 + 4;
              pcVar15 = pcVar15 + 4;
            }
            for (uVar8 = uVar8 & 3; uVar8 != 0; uVar8 = uVar8 - 1) {
              *pcVar15 = *pcVar14;
              pcVar14 = pcVar14 + 1;
              pcVar15 = pcVar15 + 1;
            }
            break;
          case 2:
            uVar8 = 0xffffffff;
            pcVar14 = &DAT_1009680c;
            do {
              pcVar15 = pcVar14;
              if (uVar8 == 0) break;
              uVar8 = uVar8 - 1;
              pcVar15 = pcVar14 + 1;
              cVar1 = *pcVar14;
              pcVar14 = pcVar15;
            } while (cVar1 != '\0');
            uVar8 = ~uVar8;
            iVar11 = *piVar13;
            pcVar14 = pcVar15 + -uVar8;
            pcVar15 = &stack0xfffffea4;
            for (uVar9 = uVar8 >> 2; uVar9 != 0; uVar9 = uVar9 - 1) {
              *(undefined4 *)pcVar15 = *(undefined4 *)pcVar14;
              pcVar14 = pcVar14 + 4;
              pcVar15 = pcVar15 + 4;
            }
            for (uVar8 = uVar8 & 3; uVar8 != 0; uVar8 = uVar8 - 1) {
              *pcVar15 = *pcVar14;
              pcVar14 = pcVar14 + 1;
              pcVar15 = pcVar15 + 1;
            }
            (**(code **)(iVar11 + 0x120))(0x7d,0xff,0x7d,5);
            break;
          case 3:
            uVar8 = 0xffffffff;
            pcVar14 = &DAT_10096804;
            do {
              pcVar15 = pcVar14;
              if (uVar8 == 0) break;
              uVar8 = uVar8 - 1;
              pcVar15 = pcVar14 + 1;
              cVar1 = *pcVar14;
              pcVar14 = pcVar15;
            } while (cVar1 != '\0');
            uVar8 = ~uVar8;
            iVar11 = *piVar13;
            pcVar14 = pcVar15 + -uVar8;
            pcVar15 = &stack0xfffffea4;
            for (uVar9 = uVar8 >> 2; uVar9 != 0; uVar9 = uVar9 - 1) {
              *(undefined4 *)pcVar15 = *(undefined4 *)pcVar14;
              pcVar14 = pcVar14 + 4;
              pcVar15 = pcVar15 + 4;
            }
            for (uVar8 = uVar8 & 3; uVar8 != 0; uVar8 = uVar8 - 1) {
              *pcVar15 = *pcVar14;
              pcVar14 = pcVar14 + 1;
              pcVar15 = pcVar15 + 1;
            }
            (**(code **)(iVar11 + 0x120))(0x87,0xff,0x87,5);
            break;
          case 4:
            uVar8 = 0xffffffff;
            pcVar14 = s_SPECIALIST_100967f8;
            do {
              pcVar15 = pcVar14;
              if (uVar8 == 0) break;
              uVar8 = uVar8 - 1;
              pcVar15 = pcVar14 + 1;
              cVar1 = *pcVar14;
              pcVar14 = pcVar15;
            } while (cVar1 != '\0');
            uVar8 = ~uVar8;
            iVar11 = *piVar13;
            pcVar14 = pcVar15 + -uVar8;
            pcVar15 = &stack0xfffffea4;
            for (uVar9 = uVar8 >> 2; uVar9 != 0; uVar9 = uVar9 - 1) {
              *(undefined4 *)pcVar15 = *(undefined4 *)pcVar14;
              pcVar14 = pcVar14 + 4;
              pcVar15 = pcVar15 + 4;
            }
            for (uVar8 = uVar8 & 3; uVar8 != 0; uVar8 = uVar8 - 1) {
              *pcVar15 = *pcVar14;
              pcVar14 = pcVar14 + 1;
              pcVar15 = pcVar15 + 1;
            }
            (**(code **)(iVar11 + 0x120))(0x91,0xff,0x91,5);
            break;
          case 5:
            uVar8 = 0xffffffff;
            pcVar14 = s_UNSTOPPABLE_100967ec;
            do {
              pcVar15 = pcVar14;
              if (uVar8 == 0) break;
              uVar8 = uVar8 - 1;
              pcVar15 = pcVar14 + 1;
              cVar1 = *pcVar14;
              pcVar14 = pcVar15;
            } while (cVar1 != '\0');
            uVar8 = ~uVar8;
            iVar11 = *piVar13;
            pcVar14 = pcVar15 + -uVar8;
            pcVar15 = &stack0xfffffea4;
            for (uVar9 = uVar8 >> 2; uVar9 != 0; uVar9 = uVar9 - 1) {
              *(undefined4 *)pcVar15 = *(undefined4 *)pcVar14;
              pcVar14 = pcVar14 + 4;
              pcVar15 = pcVar15 + 4;
            }
            for (uVar8 = uVar8 & 3; uVar8 != 0; uVar8 = uVar8 - 1) {
              *pcVar15 = *pcVar14;
              pcVar14 = pcVar14 + 1;
              pcVar15 = pcVar15 + 1;
            }
            (**(code **)(iVar11 + 0x120))(0x9b,0xff,0x9b,5);
            break;
          default:
            pcVar14 = s_WAITING_1009681c;
            if (*(char *)((int)&DAT_101901f4 + *piStack_190 * 0x20 + 1) == '\0') {
              pcVar14 = s_;
            }
            uVar8 = 0xffffffff;
            do {
              pcVar15 = pcVar14;
              if (uVar8 == 0) break;
              uVar8 = uVar8 - 1;
              pcVar15 = pcVar14 + 1;
              cVar1 = *pcVar14;
              pcVar14 = pcVar15;
            } while (cVar1 != '\0');
            uVar8 = ~uVar8;
            pcVar14 = pcVar15 + -uVar8;
            pcVar15 = &stack0xfffffea4;
            for (uVar9 = uVar8 >> 2; uVar9 != 0; uVar9 = uVar9 - 1) {
              *(undefined4 *)pcVar15 = *(undefined4 *)pcVar14;
              pcVar14 = pcVar14 + 4;
              pcVar15 = pcVar15 + 4;
            }
            for (uVar8 = uVar8 & 3; uVar8 != 0; uVar8 = uVar8 - 1) {
              *pcVar15 = *pcVar14;
              pcVar14 = pcVar14 + 1;
              pcVar15 = pcVar15 + 1;
            }
            break;
          case 10:
            uVar8 = 0xffffffff;
            pcVar14 = s_THE_ONE_100967e4;
            do {
              pcVar15 = pcVar14;
              if (uVar8 == 0) break;
              uVar8 = uVar8 - 1;
              pcVar15 = pcVar14 + 1;
              cVar1 = *pcVar14;
              pcVar14 = pcVar15;
            } while (cVar1 != '\0');
            uVar8 = ~uVar8;
            iVar11 = *piVar13;
            pcVar14 = pcVar15 + -uVar8;
            pcVar15 = &stack0xfffffea4;
            for (uVar9 = uVar8 >> 2; uVar9 != 0; uVar9 = uVar9 - 1) {
              *(undefined4 *)pcVar15 = *(undefined4 *)pcVar14;
              pcVar14 = pcVar14 + 4;
              pcVar15 = pcVar15 + 4;
            }
            for (uVar8 = uVar8 & 3; uVar8 != 0; uVar8 = uVar8 - 1) {
              *pcVar15 = *pcVar14;
              pcVar14 = pcVar14 + 1;
              pcVar15 = pcVar15 + 1;
            }
            (**(code **)(iVar11 + 0x120))(0xaf,0xff,0xaf,5);
            break;
          case 0xb:
            uVar8 = 0xffffffff;
            pcVar14 = s_WAITING_1009681c;
            do {
              pcVar15 = pcVar14;
              if (uVar8 == 0) break;
              uVar8 = uVar8 - 1;
              pcVar15 = pcVar14 + 1;
              cVar1 = *pcVar14;
              pcVar14 = pcVar15;
            } while (cVar1 != '\0');
            uVar8 = ~uVar8;
            pcVar14 = pcVar15 + -uVar8;
            pcVar15 = &stack0xfffffea4;
            for (uVar9 = uVar8 >> 2; uVar9 != 0; uVar9 = uVar9 - 1) {
              *(undefined4 *)pcVar15 = *(undefined4 *)pcVar14;
              pcVar14 = pcVar14 + 4;
              pcVar15 = pcVar15 + 4;
            }
            for (uVar8 = uVar8 & 3; uVar8 != 0; uVar8 = uVar8 - 1) {
              *pcVar15 = *pcVar14;
              pcVar14 = pcVar14 + 1;
              pcVar15 = pcVar15 + 1;
            }
          }
          goto switchD_1005e05c_caseD_2;
        case 3:
          pbVar20 = &DAT_10093a90;
          break;
        case 4:
          pbVar20 = &DAT_10093a90;
          break;
        case 5:
LAB_1005e46a:
          pbVar20 = &DAT_10093a90;
          break;
        case 6:
          if ((*(char *)((int)piVar19 + 6) == '\0') || (iVar11 = (*DAT_100d5c88)(), iVar11 != 0)) {
            iVar11 = *piStack_190;
            piVar19 = piVar13;
            puVar5 = FUN_10017570();
            FUN_100185d0(puVar5,piVar19,iVar11);
          }
        default:
          goto switchD_1005e05c_caseD_2;
        }
LAB_1005e475:
        FUN_1006ac7a(&stack0xfffffea4,pbVar20);
switchD_1005e05c_caseD_2:
        (**(code **)(*piVar13 + 0x22c))(&stack0xfffffea4);
LAB_1005e491:
        iVar3 = iVar3 + 1;
        param_1 = local_128;
      } while (iVar3 < 8);
    }
    else {
      iVar3 = 8;
      piVar13 = piVar26;
      do {
        (**(code **)(*piVar13 + 0x24))();
        piVar13 = piVar13 + 0x10c4;
        iVar3 = iVar3 + -1;
      } while (iVar3 != 0);
    }
    iVar2 = iVar2 + 1;
    this = this + 0x39;
    piVar26 = piVar26 + 0x3a;
    piVar4 = piVar4 + 1;
    if (0x49 < iVar2) {
      piVar4 = param_1 + 0x349;
      iStack_134 = 0x4a;
      do {
        FUN_1005af30(piVar4);
        (**(code **)(*piVar4 + 0x14))(&stack0xfffffec4,auStack_11c,auStack_124);
        iVar2 = *piVar4;
        iVar3 = FUN_1005b0c0((int)piVar4);
        (**(code **)(iVar2 + 8))(iStack_134,iVar3);
        FUN_1005b030(piVar4);
        piVar4 = piVar4 + 0x39;
        iStack_134 = iStack_134 + -1;
      } while (iStack_134 != 0);
      (**(code **)(param_1[0x26f] + 0xc))(auStack_12c);
      (**(code **)(param_1[0x26f] + 8))(0,this);
      return;
    }
  } while( true );
}
