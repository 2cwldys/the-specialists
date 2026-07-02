// ===== FUN @ 10044b50 size=6490 =====

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void __thiscall FUN_10044b50(int *param_1,int *param_2,int param_3)

{
  float fVar1;
  char cVar2;
  byte bVar3;
  float fVar4;
  float fVar5;
  float fVar6;
  float fVar7;
  float fVar8;
  float fVar9;
  float fVar10;
  int iVar11;
  undefined4 uVar12;
  byte *pbVar13;
  int iVar14;
  uint uVar15;
  uint uVar16;
  byte *pbVar17;
  byte *pbVar18;
  int *piVar19;
  char *pcVar20;
  char *pcVar21;
  int *piVar22;
  int *piVar23;
  bool bVar24;
  undefined4 in_stack_ffffff4c;
  undefined4 uVar25;
  undefined1 *puVar26;
  undefined4 in_stack_ffffff58;
  undefined4 in_stack_ffffff5c;
  undefined4 in_stack_ffffff60;
  undefined4 uVar27;
  undefined4 uVar28;
  char *pcVar29;
  undefined1 auStack_70 [4];
  float fStack_6c;
  float fStack_68;
  undefined1 auStack_64 [12];
  undefined1 auStack_58 [12];
  undefined1 auStack_4c [12];
  char acStack_40 [64];
  
  if (param_3 < 0) {
    return;
  }
  iVar11 = param_2[1];
  fVar4 = SQRT(*(float *)(iVar11 + 0x24) * *(float *)(iVar11 + 0x24) +
               *(float *)(iVar11 + 0x20) * *(float *)(iVar11 + 0x20));
  if (((*(uint *)(iVar11 + 0x1a4) & 0x200) == 0) &&
     (iVar11 = (**(code **)(*param_2 + 0xac))(), iVar11 != 0)) {
    if ((*param_1 == 4) &&
       ((_DAT_10092a1c < fVar4 && (iVar11 = FUN_10046f90(param_3), iVar11 == 0)))) {
      piVar23 = param_2 + 0x2d1;
      uVar28 = 0x3e99999a;
      piVar19 = param_1 + 7;
      uVar27 = 0x3f19999a;
      FUN_1000c120(param_2[1] + 0x74);
      puVar26 = auStack_70;
      uVar25 = 0x10044c0c;
      uVar12 = FUN_100148f0(puVar26);
      FUN_1000c120(uVar12);
      FUN_10046820(param_2,in_stack_ffffff4c,uVar25,puVar26,in_stack_ffffff58,in_stack_ffffff5c,
                   in_stack_ffffff60,piVar19,uVar27,uVar28,piVar23);
    }
    if (param_3 == 2) {
      pbVar17 = &DAT_100a9398;
      pbVar13 = (byte *)(param_2 + 0x2d1);
      do {
        bVar3 = *pbVar13;
        bVar24 = bVar3 < *pbVar17;
        if (bVar3 != *pbVar17) {
LAB_10044c59:
          iVar11 = (1 - (uint)bVar24) - (uint)(bVar24 != 0);
          goto LAB_10044c5e;
        }
        if (bVar3 == 0) break;
        bVar3 = pbVar13[1];
        bVar24 = bVar3 < pbVar17[1];
        if (bVar3 != pbVar17[1]) goto LAB_10044c59;
        pbVar13 = pbVar13 + 2;
        pbVar17 = pbVar17 + 2;
      } while (bVar3 != 0);
      iVar11 = 0;
LAB_10044c5e:
      pbVar13 = &DAT_1009a3e0;
      if (iVar11 != 0) {
        pbVar13 = (byte *)(param_2 + 0x2d1);
      }
      uVar27 = 0x3e99999a;
      piVar19 = param_1 + 0x17;
      uVar25 = 0x3f19999a;
      iVar11 = param_2[1] + 0x74;
      uVar12 = 0x10044c89;
      FUN_1000c120(iVar11);
      FUN_1000c120(param_2 + 0x13f);
      FUN_10046820(param_2,in_stack_ffffff4c,uVar12,iVar11,in_stack_ffffff58,in_stack_ffffff5c,
                   in_stack_ffffff60,piVar19,uVar25,uVar27,pbVar13);
    }
  }
  iVar11 = FUN_10046f90(*param_1);
  if (iVar11 != 0) {
    if (((((((param_2[0x1a] != 0) || (iVar11 = FUN_10046f90(param_3), iVar11 != 0)) ||
           (iVar11 = (**(code **)(*param_2 + 0xac))(), iVar11 == 0)) ||
          ((param_3 == 0x28 || (param_3 == 0x2f)))) ||
         ((param_3 == 0x30 || ((param_3 == 0x36 || (param_3 == 0x32)))))) || (param_3 == 0x31)) ||
       ((((param_3 == 0x37 || (param_3 == 0x38)) || (param_3 == 0x39)) ||
        (((char)param_1[0x7a] != '\0' && ((*(uint *)(param_2[1] + 0x1a4) & 0x200) != 0))))))
    goto switchD_10044d8b_caseD_6;
    iVar11 = FUN_10046fc0(*param_1,param_2);
    if (iVar11 == 0) {
      return;
    }
    goto LAB_100461c4;
  }
  switch(*param_1) {
  case 3:
    iVar11 = (**(code **)(*param_2 + 0xac))();
    if ((((iVar11 != 0) && (param_3 != 4)) && (param_3 != 0x39)) &&
       (((param_3 != 0x2a && (param_3 != 0x2b)) &&
        ((((param_3 != 0x29 && ((param_3 != 0 && (param_3 != 8)))) && (param_3 != 9)) &&
         (iVar11 = FUN_10033670(), iVar11 == 0)))))) {
      FUN_100466f0(param_2);
      return;
    }
    break;
  case 4:
    if (((((((param_2[0x1a] == 0) && (param_3 != 4)) && (param_3 != 0x28)) &&
          ((param_3 != 0x2f && (param_3 != 0x30)))) && (param_3 != 0x31)) &&
        ((param_3 != 0x37 && (param_3 != 0x38)))) &&
       ((param_3 != 0x36 &&
        ((param_3 != 0x32 && (iVar11 = (**(code **)(*param_2 + 0xac))(), iVar11 != 0)))))) {
      uVar15 = *(uint *)(param_2[1] + 0x1a4);
LAB_100452d4:
      if ((uVar15 & 0x200) == 0) {
        iVar11 = FUN_10046f90(param_3);
        goto LAB_100452de;
      }
    }
    break;
  case 5:
    if ((((((param_2[0x1a] == 0) && ((*(byte *)(param_2[1] + 0x180) & 4) != 0)) &&
          (((*(uint *)(param_2[1] + 0x1a4) & 0x4000) == 0 &&
           (((param_3 != 0x2f && (param_3 != 0x30)) && (param_3 != 0x28)))))) &&
         ((param_3 != 4 && (param_3 != 0x39)))) && (param_3 != 0x14)) &&
       (((param_3 < 0x16 && (0x19 < param_3)) && (param_3 != 0x25)))) {
      iVar11 = (**(code **)(*param_2 + 0xac))();
joined_r0x10044f27:
      if (iVar11 != 0) {
        return;
      }
    }
    break;
  case 7:
    return;
  case 0x28:
    if ((((param_2[0x1a] == 0) && (iVar11 = (**(code **)(*param_2 + 0xac))(), iVar11 != 0)) &&
        (param_3 != 0x29)) && ((param_3 != 0x2e && (param_3 != 3)))) {
      iVar11 = FUN_10033670();
LAB_100452de:
      bVar24 = iVar11 == 0;
LAB_100452e0:
      if (bVar24) {
        return;
      }
    }
    break;
  case 0x29:
    if (((param_2[0x1a] == 0) && (iVar11 = (**(code **)(*param_2 + 0xac))(), iVar11 != 0)) &&
       ((*(byte *)(param_2[1] + 0x250) & 4) != 0)) {
      return;
    }
    break;
  case 0x2a:
  case 0x2b:
    FUN_10076530(param_2[1] + 0x74);
    iVar11 = param_2[1];
    if (SQRT(*(float *)(iVar11 + 0x28) * *(float *)(iVar11 + 0x28) +
             *(float *)(iVar11 + 0x24) * *(float *)(iVar11 + 0x24) +
             *(float *)(iVar11 + 0x20) * *(float *)(iVar11 + 0x20)) != _DAT_10089110) {
      fVar1 = SQRT(*(float *)(DAT_100ae734 + 0x48) * *(float *)(DAT_100ae734 + 0x48) +
                   *(float *)(DAT_100ae734 + 0x44) * *(float *)(DAT_100ae734 + 0x44) +
                   *(float *)(DAT_100ae734 + 0x40) * *(float *)(DAT_100ae734 + 0x40));
      fVar5 = _DAT_10089110;
      fVar6 = _DAT_10089110;
      fVar8 = _DAT_10089138;
      if (fVar1 != _DAT_10089110) {
        fVar1 = _DAT_10089138 / fVar1;
        fStack_6c = fVar1 * *(float *)(DAT_100ae734 + 0x44);
        fStack_68 = fVar1 * *(float *)(DAT_100ae734 + 0x48);
        fVar5 = fVar1 * *(float *)(DAT_100ae734 + 0x40);
        fVar6 = fStack_6c;
        fVar8 = fStack_68;
      }
      fVar1 = SQRT(*(float *)(iVar11 + 0x28) * *(float *)(iVar11 + 0x28) +
                   *(float *)(iVar11 + 0x24) * *(float *)(iVar11 + 0x24) +
                   *(float *)(iVar11 + 0x20) * *(float *)(iVar11 + 0x20));
      fVar7 = _DAT_10089110;
      fVar9 = _DAT_10089138;
      fVar10 = _DAT_10089110;
      if (fVar1 != _DAT_10089110) {
        fVar1 = _DAT_10089138 / fVar1;
        fStack_6c = fVar1 * *(float *)(iVar11 + 0x24);
        fStack_68 = fVar1 * *(float *)(iVar11 + 0x28);
        fVar7 = fStack_6c;
        fVar9 = fStack_68;
        fVar10 = fVar1 * *(float *)(iVar11 + 0x20);
      }
      if (fVar7 * fVar6 + fVar9 * fVar8 + fVar10 * fVar5 == _DAT_10089110) break;
    }
    iVar11 = (**(code **)(*param_2 + 0xac))();
    if ((((iVar11 != 0) && (param_3 != 0x2a)) && (param_3 != 0x2b)) &&
       ((param_3 != 3 && (param_3 != 0x29)))) {
      iVar11 = FUN_10033670();
      goto LAB_100452de;
    }
    break;
  case 0x2e:
  case 0x2f:
    if ((param_2[0x1a] == 0) && (iVar11 = (**(code **)(*param_2 + 0xac))(), iVar11 != 0)) {
      bVar24 = (*(byte *)(param_2[1] + 0x250) & 4) == 0;
LAB_10044f5a:
      if (!bVar24) {
        return;
      }
    }
    break;
  case 0x30:
    if (param_2[0x1a] == 0) {
      iVar11 = (**(code **)(*param_2 + 0xac))();
      goto joined_r0x10044f27;
    }
    break;
  case 0x31:
    if (((param_2[0x1a] == 0) && (iVar11 = (**(code **)(*param_2 + 0xac))(), iVar11 != 0)) &&
       ((*(byte *)(param_2[1] + 0x250) & 4) != 0)) {
      iVar11 = FUN_10033670();
      goto LAB_100452de;
    }
    break;
  case 0x32:
    if ((((param_2[0x1a] == 0) && (iVar11 = (**(code **)(*param_2 + 0xac))(), iVar11 != 0)) &&
        (iVar11 = FUN_10033670(), iVar11 == 0)) && (param_3 != 0x29)) {
      bVar24 = param_3 == 0x2e;
      goto LAB_10044f5a;
    }
    break;
  case 0x36:
    if ((param_2[0x1a] == 0) && (iVar11 = (**(code **)(*param_2 + 0xac))(), iVar11 != 0)) {
      return;
    }
    param_2[0x143] = 0;
    break;
  case 0x37:
    if ((((param_2[0x1a] == 0) && (iVar11 = (**(code **)(*param_2 + 0xac))(), iVar11 != 0)) &&
        ((*(byte *)(param_2[1] + 0x250) & 4) != 0)) &&
       (((iVar11 = FUN_10033670(), iVar11 == 0 && (param_3 != 0x37)) &&
        ((param_3 != 0x38 && (param_3 != 0x28)))))) {
      bVar24 = (*(uint *)(param_2[1] + 0x1a4) & 0x200) == 0;
      goto LAB_100452e0;
    }
    break;
  case 0x38:
    if (((((param_2[0x1a] == 0) && (iVar11 = (**(code **)(*param_2 + 0xac))(), iVar11 != 0)) &&
         ((*(byte *)(param_2[1] + 0x250) & 4) != 0)) &&
        ((iVar11 = FUN_10033670(), iVar11 == 0 && (param_3 != 0x37)))) &&
       ((param_3 != 0x38 && ((param_3 != 0x28 && (iVar11 = FUN_10046f90(param_3), iVar11 == 0))))))
    {
      bVar24 = (*(uint *)(param_2[1] + 0x1a4) & 0x200) == 0;
      goto LAB_100452e0;
    }
    break;
  case 0x39:
    if ((((param_2[0x1a] == 0) && (param_3 != 0x39)) &&
        ((param_3 != 0x28 && (((param_3 != 0x2f && (param_3 != 0x30)) && (param_3 != 0x31)))))) &&
       (((param_3 != 0x37 && (param_3 != 0x38)) &&
        ((param_3 != 0x36 &&
         ((param_3 != 0x32 && (iVar11 = (**(code **)(*param_2 + 0xac))(), iVar11 != 0)))))))) {
      uVar15 = *(uint *)(param_2[1] + 0x1a4);
      goto LAB_100452d4;
    }
  }
switchD_10044d8b_caseD_6:
  iVar14 = FUN_10046f90(param_3);
  iVar11 = param_3;
  if (iVar14 != 0) {
    FUN_100469f0(param_2,param_3);
    goto switchD_10045371_caseD_2;
  }
  switch(param_3) {
  case 0:
  case 8:
  case 9:
    iVar14 = param_2[1];
    if ((*(uint *)(iVar14 + 0x1a4) & 0x200) == 0) {
      return;
    }
    if ((*(byte *)(iVar14 + 0x250) & 0x20) == 0) {
      param_1[3] = 0x40000000;
      pcVar29 = s_crouch_shoot__100a3000;
      if ((*(uint *)(param_2[1] + 0x1a4) & 0x4000) == 0) {
        pcVar29 = s_shoot__100a2ff8;
      }
      uVar15 = 0xffffffff;
      do {
        pcVar21 = pcVar29;
        if (uVar15 == 0) break;
        uVar15 = uVar15 - 1;
        pcVar21 = pcVar29 + 1;
        cVar2 = *pcVar29;
        pcVar29 = pcVar21;
      } while (cVar2 != '\0');
      uVar15 = ~uVar15;
      piVar19 = param_2 + 0x2d1;
      pcVar29 = pcVar21 + -uVar15;
      pcVar21 = acStack_40;
      for (uVar16 = uVar15 >> 2; uVar16 != 0; uVar16 = uVar16 - 1) {
        *(undefined4 *)pcVar21 = *(undefined4 *)pcVar29;
        pcVar29 = pcVar29 + 4;
        pcVar21 = pcVar21 + 4;
      }
      for (uVar15 = uVar15 & 3; uVar15 != 0; uVar15 = uVar15 - 1) {
        *pcVar21 = *pcVar29;
        pcVar29 = pcVar29 + 1;
        pcVar21 = pcVar21 + 1;
      }
      uVar15 = 0xffffffff;
      piVar23 = piVar19;
      do {
        piVar22 = piVar23;
        if (uVar15 == 0) break;
        uVar15 = uVar15 - 1;
        piVar22 = (int *)((int)piVar23 + 1);
        iVar14 = *piVar23;
        piVar23 = piVar22;
      } while ((char)iVar14 != '\0');
      uVar15 = ~uVar15;
      iVar14 = -1;
      pcVar29 = acStack_40;
      do {
        pcVar21 = pcVar29;
        if (iVar14 == 0) break;
        iVar14 = iVar14 + -1;
        pcVar21 = pcVar29 + 1;
        cVar2 = *pcVar29;
        pcVar29 = pcVar21;
      } while (cVar2 != '\0');
      fVar1 = (float)param_1[2];
      pcVar29 = (char *)((int)piVar22 - uVar15);
      pcVar21 = pcVar21 + -1;
      for (uVar16 = uVar15 >> 2; uVar16 != 0; uVar16 = uVar16 - 1) {
        *(undefined4 *)pcVar21 = *(undefined4 *)pcVar29;
        pcVar29 = pcVar29 + 4;
        pcVar21 = pcVar21 + 4;
      }
      for (uVar15 = uVar15 & 3; uVar15 != 0; uVar15 = uVar15 - 1) {
        *pcVar21 = *pcVar29;
        pcVar29 = pcVar29 + 1;
        pcVar21 = pcVar21 + 1;
      }
      if (((_DAT_1008a0c8 < fVar1) && ((*(uint *)(param_2[1] + 0x1a4) & 0x4000) == 0)) &&
         ((*(float *)(param_2[1] + 0x214) == _DAT_10089110 &&
          ((iVar14 = _strncmp((char *)piVar19,&DAT_100a2ff4,3), iVar14 == 0 ||
           (iVar14 = _strncmp((char *)piVar19,&DAT_100a2ff0,3), iVar14 == 0)))))) {
        uVar15 = 0xffffffff;
        pcVar29 = &DAT_100a2fec;
        do {
          pcVar21 = pcVar29;
          if (uVar15 == 0) break;
          uVar15 = uVar15 - 1;
          pcVar21 = pcVar29 + 1;
          cVar2 = *pcVar29;
          pcVar29 = pcVar21;
        } while (cVar2 != '\0');
        uVar15 = ~uVar15;
        iVar14 = -1;
        pcVar29 = acStack_40;
        do {
          pcVar20 = pcVar29;
          if (iVar14 == 0) break;
          iVar14 = iVar14 + -1;
          pcVar20 = pcVar29 + 1;
          cVar2 = *pcVar29;
          pcVar29 = pcVar20;
        } while (cVar2 != '\0');
        pcVar29 = pcVar21 + -uVar15;
        pcVar21 = pcVar20 + -1;
        for (uVar16 = uVar15 >> 2; uVar16 != 0; uVar16 = uVar16 - 1) {
          *(undefined4 *)pcVar21 = *(undefined4 *)pcVar29;
          pcVar29 = pcVar29 + 4;
          pcVar21 = pcVar21 + 4;
        }
        for (uVar15 = uVar15 & 3; uVar15 != 0; uVar15 = uVar15 - 1) {
          *pcVar21 = *pcVar29;
          pcVar29 = pcVar29 + 1;
          pcVar21 = pcVar21 + 1;
        }
        param_1[3] = 0;
      }
      if (param_3 != 0) {
        pcVar29 = s__right_100a30ac;
        if (param_3 != 9) {
          pcVar29 = &DAT_100a30b4;
        }
        uVar15 = 0xffffffff;
        do {
          pcVar21 = pcVar29;
          if (uVar15 == 0) break;
          uVar15 = uVar15 - 1;
          pcVar21 = pcVar29 + 1;
          cVar2 = *pcVar29;
          pcVar29 = pcVar21;
        } while (cVar2 != '\0');
        uVar15 = ~uVar15;
        iVar14 = -1;
        pcVar29 = acStack_40;
        do {
          pcVar20 = pcVar29;
          if (iVar14 == 0) break;
          iVar14 = iVar14 + -1;
          pcVar20 = pcVar29 + 1;
          cVar2 = *pcVar29;
          pcVar29 = pcVar20;
        } while (cVar2 != '\0');
        pcVar29 = pcVar21 + -uVar15;
        pcVar21 = pcVar20 + -1;
        for (uVar16 = uVar15 >> 2; uVar16 != 0; uVar16 = uVar16 - 1) {
          *(undefined4 *)pcVar21 = *(undefined4 *)pcVar29;
          pcVar29 = pcVar29 + 4;
          pcVar21 = pcVar21 + 4;
        }
        for (uVar15 = uVar15 & 3; uVar15 != 0; uVar15 = uVar15 - 1) {
          *pcVar21 = *pcVar29;
          pcVar29 = pcVar29 + 1;
          pcVar21 = pcVar21 + 1;
        }
      }
      uVar12 = 0x92;
      goto LAB_10046014;
    }
    if ((char)param_2[0x142] == '\0') {
      pcVar29 = s_slide_f__100a2f18;
    }
    else {
      FUN_1000c120(iVar14 + 0x74);
      FUN_1000c120(param_2 + 0x13f);
      iVar11 = FUN_10044510();
      if ((iVar11 != param_1[5]) && (iVar11 != 2)) {
        param_1[5] = iVar11;
      }
      pcVar29 = (char *)(param_1 + param_1[5] * 4 + 0x27);
    }
    uVar15 = 0xffffffff;
    do {
      piVar19 = (int *)pcVar29;
      if (uVar15 == 0) break;
      uVar15 = uVar15 - 1;
      piVar19 = (int *)((int)pcVar29 + 1);
      iVar11 = *(int *)pcVar29;
      pcVar29 = (char *)piVar19;
    } while ((char)iVar11 != '\0');
    uVar15 = ~uVar15;
    pcVar29 = (char *)((int)piVar19 - uVar15);
    pcVar21 = acStack_40;
    for (uVar16 = uVar15 >> 2; uVar16 != 0; uVar16 = uVar16 - 1) {
      *(undefined4 *)pcVar21 = *(undefined4 *)pcVar29;
      pcVar29 = pcVar29 + 4;
      pcVar21 = pcVar21 + 4;
    }
    for (uVar15 = uVar15 & 3; uVar15 != 0; uVar15 = uVar15 - 1) {
      *pcVar21 = *pcVar29;
      pcVar29 = pcVar29 + 1;
      pcVar21 = pcVar21 + 1;
    }
    uVar15 = 0xffffffff;
    piVar19 = param_2 + 0x2d1;
    do {
      piVar23 = piVar19;
      if (uVar15 == 0) break;
      uVar15 = uVar15 - 1;
      piVar23 = (int *)((int)piVar19 + 1);
      iVar11 = *piVar19;
      piVar19 = piVar23;
    } while ((char)iVar11 != '\0');
    uVar15 = ~uVar15;
    iVar11 = -1;
    pcVar29 = acStack_40;
    do {
      pcVar21 = pcVar29;
      if (iVar11 == 0) break;
      iVar11 = iVar11 + -1;
      pcVar21 = pcVar29 + 1;
      cVar2 = *pcVar29;
      pcVar29 = pcVar21;
    } while (cVar2 != '\0');
    pcVar29 = (char *)((int)piVar23 - uVar15);
    pcVar21 = pcVar21 + -1;
    for (uVar16 = uVar15 >> 2; uVar16 != 0; uVar16 = uVar16 - 1) {
      *(undefined4 *)pcVar21 = *(undefined4 *)pcVar29;
      pcVar29 = pcVar29 + 4;
      pcVar21 = pcVar21 + 4;
    }
    for (uVar15 = uVar15 & 3; uVar15 != 0; uVar15 = uVar15 - 1) {
      *pcVar21 = *pcVar29;
      pcVar29 = pcVar29 + 1;
      pcVar21 = pcVar21 + 1;
    }
    uVar15 = 0xffffffff;
    pcVar29 = s__shoot_100a3010;
    do {
      pcVar21 = pcVar29;
      if (uVar15 == 0) break;
      uVar15 = uVar15 - 1;
      pcVar21 = pcVar29 + 1;
      cVar2 = *pcVar29;
      pcVar29 = pcVar21;
    } while (cVar2 != '\0');
    uVar15 = ~uVar15;
    iVar11 = -1;
    pcVar29 = acStack_40;
    do {
      pcVar20 = pcVar29;
      if (iVar11 == 0) break;
      iVar11 = iVar11 + -1;
      pcVar20 = pcVar29 + 1;
      cVar2 = *pcVar29;
      pcVar29 = pcVar20;
    } while (cVar2 != '\0');
    pcVar29 = pcVar21 + -uVar15;
    pcVar21 = pcVar20 + -1;
    for (uVar16 = uVar15 >> 2; uVar16 != 0; uVar16 = uVar16 - 1) {
      *(undefined4 *)pcVar21 = *(undefined4 *)pcVar29;
      pcVar29 = pcVar29 + 4;
      pcVar21 = pcVar21 + 4;
    }
    for (uVar15 = uVar15 & 3; uVar15 != 0; uVar15 = uVar15 - 1) {
      *pcVar21 = *pcVar29;
      pcVar29 = pcVar29 + 1;
      pcVar21 = pcVar21 + 1;
    }
    if (param_3 != 0) {
      pcVar29 = s__right_100a30ac;
      if (param_3 != 9) {
        pcVar29 = &DAT_100a30b4;
      }
      uVar15 = 0xffffffff;
      do {
        pcVar21 = pcVar29;
        if (uVar15 == 0) break;
        uVar15 = uVar15 - 1;
        pcVar21 = pcVar29 + 1;
        cVar2 = *pcVar29;
        pcVar29 = pcVar21;
      } while (cVar2 != '\0');
      uVar15 = ~uVar15;
      iVar11 = -1;
      pcVar29 = acStack_40;
      do {
        pcVar20 = pcVar29;
        if (iVar11 == 0) break;
        iVar11 = iVar11 + -1;
        pcVar20 = pcVar29 + 1;
        cVar2 = *pcVar29;
        pcVar29 = pcVar20;
      } while (cVar2 != '\0');
      pcVar29 = pcVar21 + -uVar15;
      pcVar21 = pcVar20 + -1;
      for (uVar16 = uVar15 >> 2; uVar16 != 0; uVar16 = uVar16 - 1) {
        *(undefined4 *)pcVar21 = *(undefined4 *)pcVar29;
        pcVar29 = pcVar29 + 4;
        pcVar21 = pcVar21 + 4;
      }
      for (uVar15 = uVar15 & 3; uVar15 != 0; uVar15 = uVar15 - 1) {
        *pcVar21 = *pcVar29;
        pcVar29 = pcVar29 + 1;
        pcVar21 = pcVar21 + 1;
      }
    }
    uVar12 = 0x94;
    goto LAB_100461a5;
  case 1:
  case 6:
    if (((*(uint *)(param_2[1] + 0x1a4) & 0x200) == 0) && (iVar14 = FUN_10033670(), iVar14 == 0)) {
      return;
    }
    pbVar17 = &DAT_100a9398;
    pbVar13 = (byte *)(param_2 + 0x119);
    do {
      bVar3 = *pbVar13;
      bVar24 = bVar3 < *pbVar17;
      if (bVar3 != *pbVar17) {
LAB_10046077:
        iVar14 = (1 - (uint)bVar24) - (uint)(bVar24 != 0);
        goto LAB_1004607c;
      }
      if (bVar3 == 0) break;
      bVar3 = pbVar13[1];
      bVar24 = bVar3 < pbVar17[1];
      if (bVar3 != pbVar17[1]) goto LAB_10046077;
      pbVar13 = pbVar13 + 2;
      pbVar17 = pbVar17 + 2;
    } while (bVar3 != 0);
    iVar14 = 0;
LAB_1004607c:
    if (iVar14 == 0) {
      pbVar13 = (byte *)(param_2 + 0x2d1);
      pbVar17 = &DAT_100a9398;
      do {
        bVar3 = *pbVar13;
        bVar24 = bVar3 < *pbVar17;
        if (bVar3 != *pbVar17) {
LAB_100460c1:
          iVar14 = (1 - (uint)bVar24) - (uint)(bVar24 != 0);
          goto LAB_100460c6;
        }
        if (bVar3 == 0) break;
        bVar3 = pbVar13[1];
        bVar24 = bVar3 < pbVar17[1];
        if (bVar3 != pbVar17[1]) goto LAB_100460c1;
        pbVar13 = pbVar13 + 2;
        pbVar17 = pbVar17 + 2;
      } while (bVar3 != 0);
      iVar14 = 0;
LAB_100460c6:
      if ((iVar14 != 0) &&
         (((((iVar11 = *param_1, iVar11 != 0 && (iVar11 != 8)) && (iVar11 != 9)) &&
           ((iVar11 != 0x34 && (iVar11 != 0x33)))) || (param_2[0x1a] != 0)))) {
        if ((*(uint *)(param_2[1] + 0x1a4) & 0x4000) == 0) {
          pcVar29 = &DAT_100a2fc4;
        }
        else {
          pcVar29 = s_crouch_aim__100a2fd4;
        }
        uVar15 = 0xffffffff;
        do {
          pcVar21 = pcVar29;
          if (uVar15 == 0) break;
          uVar15 = uVar15 - 1;
          pcVar21 = pcVar29 + 1;
          cVar2 = *pcVar29;
          pcVar29 = pcVar21;
        } while (cVar2 != '\0');
        uVar15 = ~uVar15;
        pcVar29 = pcVar21 + -uVar15;
        pcVar21 = acStack_40;
        for (uVar16 = uVar15 >> 2; uVar16 != 0; uVar16 = uVar16 - 1) {
          *(undefined4 *)pcVar21 = *(undefined4 *)pcVar29;
          pcVar29 = pcVar29 + 4;
          pcVar21 = pcVar21 + 4;
        }
        for (uVar15 = uVar15 & 3; uVar15 != 0; uVar15 = uVar15 - 1) {
          *pcVar21 = *pcVar29;
          pcVar29 = pcVar29 + 1;
          pcVar21 = pcVar21 + 1;
        }
        uVar15 = 0xffffffff;
        piVar19 = param_2 + 0x2d1;
        do {
          piVar23 = piVar19;
          if (uVar15 == 0) break;
          uVar15 = uVar15 - 1;
          piVar23 = (int *)((int)piVar19 + 1);
          iVar11 = *piVar19;
          piVar19 = piVar23;
        } while ((char)iVar11 != '\0');
        uVar15 = ~uVar15;
        uVar12 = 0x12;
        iVar11 = -1;
        pcVar29 = acStack_40;
        do {
          pcVar21 = pcVar29;
          if (iVar11 == 0) break;
          iVar11 = iVar11 + -1;
          pcVar21 = pcVar29 + 1;
          cVar2 = *pcVar29;
          pcVar29 = pcVar21;
        } while (cVar2 != '\0');
        pcVar29 = (char *)((int)piVar23 - uVar15);
        pcVar21 = pcVar21 + -1;
        for (uVar16 = uVar15 >> 2; uVar16 != 0; uVar16 = uVar16 - 1) {
          *(undefined4 *)pcVar21 = *(undefined4 *)pcVar29;
          pcVar29 = pcVar29 + 4;
          pcVar21 = pcVar21 + 4;
        }
        for (uVar15 = uVar15 & 3; uVar15 != 0; uVar15 = uVar15 - 1) {
          *pcVar21 = *pcVar29;
          pcVar29 = pcVar29 + 1;
          pcVar21 = pcVar21 + 1;
        }
        goto LAB_100461a5;
      }
    }
    else {
      FUN_10046920(param_2,param_2 + 0x119,0x12,0);
    }
    break;
  case 3:
    iVar14 = *param_1;
    if ((((iVar14 != 0) && (iVar14 != 8)) && (iVar14 != 9)) || (param_2[0x1a] != 0)) {
      if ((char)param_2[0x142] == '\0') {
        pcVar29 = s_slide_f__100a2f18;
      }
      else {
        FUN_1000c120(param_2[1] + 0x74);
        FUN_1000c120(param_2 + 0x13f);
        iVar11 = FUN_10044510();
        if ((iVar11 != param_1[5]) && (iVar11 != 2)) {
          param_1[5] = iVar11;
        }
        pcVar29 = (char *)(param_1 + param_1[5] * 4 + 0x27);
      }
      uVar15 = 0xffffffff;
      do {
        piVar19 = (int *)pcVar29;
        if (uVar15 == 0) break;
        uVar15 = uVar15 - 1;
        piVar19 = (int *)((int)pcVar29 + 1);
        iVar11 = *(int *)pcVar29;
        pcVar29 = (char *)piVar19;
      } while ((char)iVar11 != '\0');
      uVar15 = ~uVar15;
      pcVar29 = (char *)((int)piVar19 - uVar15);
      pcVar21 = acStack_40;
      for (uVar16 = uVar15 >> 2; uVar16 != 0; uVar16 = uVar16 - 1) {
        *(undefined4 *)pcVar21 = *(undefined4 *)pcVar29;
        pcVar29 = pcVar29 + 4;
        pcVar21 = pcVar21 + 4;
      }
      for (uVar15 = uVar15 & 3; uVar15 != 0; uVar15 = uVar15 - 1) {
        *pcVar21 = *pcVar29;
        pcVar29 = pcVar29 + 1;
        pcVar21 = pcVar21 + 1;
      }
      pbVar13 = (byte *)(param_2 + 0x2d1);
      pbVar17 = &DAT_100a9398;
      do {
        bVar3 = *pbVar13;
        bVar24 = bVar3 < *pbVar17;
        if (bVar3 != *pbVar17) goto LAB_1004591b;
        if (bVar3 == 0) break;
        bVar3 = pbVar13[1];
        bVar24 = bVar3 < pbVar17[1];
        if (bVar3 != pbVar17[1]) goto LAB_1004591b;
        pbVar13 = pbVar13 + 2;
        pbVar17 = pbVar17 + 2;
      } while (bVar3 != 0);
      iVar11 = 0;
      goto LAB_10045920;
    }
    break;
  case 4:
    if ((*(byte *)(param_2[1] + 0x250) & 0x24) != 0) {
      return;
    }
    pbVar17 = &DAT_100a9398;
    pbVar13 = (byte *)(param_2 + 0x2d1);
    do {
      bVar3 = *pbVar13;
      bVar24 = bVar3 < *pbVar17;
      if (bVar3 != *pbVar17) {
LAB_10045c11:
        iVar14 = (1 - (uint)bVar24) - (uint)(bVar24 != 0);
        goto LAB_10045c16;
      }
      if (bVar3 == 0) break;
      bVar3 = pbVar13[1];
      bVar24 = bVar3 < pbVar17[1];
      if (bVar3 != pbVar17[1]) goto LAB_10045c11;
      pbVar13 = pbVar13 + 2;
      pbVar17 = pbVar17 + 2;
    } while (bVar3 != 0);
    iVar14 = 0;
LAB_10045c16:
    if (iVar14 == 0) {
      uVar15 = 0xffffffff;
      pcVar29 = s_standard_jump_100a3034;
      do {
        pcVar21 = pcVar29;
        if (uVar15 == 0) break;
        uVar15 = uVar15 - 1;
        pcVar21 = pcVar29 + 1;
        cVar2 = *pcVar29;
        pcVar29 = pcVar21;
      } while (cVar2 != '\0');
      uVar15 = ~uVar15;
      pcVar29 = pcVar21 + -uVar15;
      pcVar21 = acStack_40;
      for (uVar16 = uVar15 >> 2; uVar16 != 0; uVar16 = uVar16 - 1) {
        *(undefined4 *)pcVar21 = *(undefined4 *)pcVar29;
        pcVar29 = pcVar29 + 4;
        pcVar21 = pcVar21 + 4;
      }
      uVar12 = 0x94;
      for (uVar15 = uVar15 & 3; uVar15 != 0; uVar15 = uVar15 - 1) {
        *pcVar21 = *pcVar29;
        pcVar29 = pcVar29 + 1;
        pcVar21 = pcVar21 + 1;
      }
    }
    else {
      FUN_1000c120(param_2[1] + 0x74);
      uVar12 = FUN_100148f0(auStack_4c);
      FUN_1000c120(uVar12);
      iVar14 = FUN_10044510();
      param_1[5] = iVar14;
      uVar15 = 0xffffffff;
      piVar19 = param_1 + iVar14 * 4 + 7;
      do {
        piVar23 = piVar19;
        if (uVar15 == 0) break;
        uVar15 = uVar15 - 1;
        piVar23 = (int *)((int)piVar19 + 1);
        iVar14 = *piVar19;
        piVar19 = piVar23;
      } while ((char)iVar14 != '\0');
      uVar15 = ~uVar15;
      pcVar29 = (char *)((int)piVar23 - uVar15);
      pcVar21 = acStack_40;
      for (uVar16 = uVar15 >> 2; uVar16 != 0; uVar16 = uVar16 - 1) {
        *(undefined4 *)pcVar21 = *(undefined4 *)pcVar29;
        pcVar29 = pcVar29 + 4;
        pcVar21 = pcVar21 + 4;
      }
      for (uVar15 = uVar15 & 3; uVar15 != 0; uVar15 = uVar15 - 1) {
        *pcVar21 = *pcVar29;
        pcVar29 = pcVar29 + 1;
        pcVar21 = pcVar21 + 1;
      }
      uVar15 = 0xffffffff;
      pbVar13 = (byte *)(param_2 + 0x2d1);
      do {
        pbVar17 = pbVar13;
        if (uVar15 == 0) break;
        uVar15 = uVar15 - 1;
        pbVar17 = pbVar13 + 1;
        bVar3 = *pbVar13;
        pbVar13 = pbVar17;
      } while (bVar3 != 0);
      uVar15 = ~uVar15;
      uVar12 = 0x94;
      iVar14 = -1;
      pcVar29 = acStack_40;
      do {
        pcVar21 = pcVar29;
        if (iVar14 == 0) break;
        iVar14 = iVar14 + -1;
        pcVar21 = pcVar29 + 1;
        cVar2 = *pcVar29;
        pcVar29 = pcVar21;
      } while (cVar2 != '\0');
      pbVar13 = pbVar17 + -uVar15;
      pbVar17 = (byte *)(pcVar21 + -1);
      for (uVar16 = uVar15 >> 2; uVar16 != 0; uVar16 = uVar16 - 1) {
        *(undefined4 *)pbVar17 = *(undefined4 *)pbVar13;
        pbVar13 = pbVar13 + 4;
        pbVar17 = pbVar17 + 4;
      }
      for (uVar15 = uVar15 & 3; uVar15 != 0; uVar15 = uVar15 - 1) {
        *pbVar17 = *pbVar13;
        pbVar13 = pbVar13 + 1;
        pbVar17 = pbVar17 + 1;
      }
    }
LAB_10046014:
    FUN_10046920(param_2,acStack_40,uVar12,0);
    break;
  case 5:
    if (param_1[1] == 0) {
      FUN_10046920(param_2,s_goduck_100a3044,0x94,0);
      param_1[1] = 1;
    }
    break;
  case 7:
    if (param_1[0x79] == 0) {
      if (_DAT_10092a18 <= fVar4) {
        if (0x32 < param_2[0x25a]) goto LAB_10045aa1;
        FUN_1000c120(param_2[1] + 0x74);
        uVar12 = FUN_100148f0(auStack_58);
LAB_10045af8:
        FUN_1000c120(uVar12);
        iVar14 = FUN_10044510();
        if (iVar14 == 2) {
          FUN_10046920(param_2,s_forward_die_100a3070,0x14,0);
        }
        else {
          if (iVar14 != 3) goto LAB_10045a86;
          if ((param_2[0x3e] == 1) && (iVar14 = (*DAT_100ae630)(0,100), 0x1e < iVar14)) {
            FUN_10046920(param_2,s_die_spinback_100a3060,0x14,0);
          }
          else {
            iVar14 = (*DAT_100ae630)(0,1);
            if (iVar14 == 0) {
              FUN_10046920(param_2,s_leg_die_100a304c,0x14,0);
            }
            else if (iVar14 == 1) {
              FUN_10046920(param_2,s_die_back_100a3054,0x14,0);
            }
          }
        }
      }
      else {
        if (0x32 < param_2[0x25a]) {
LAB_10045aa1:
          FUN_1000c120(param_2[1] + 0x74);
          uVar12 = FUN_100465c0(auStack_64);
          goto LAB_10045af8;
        }
LAB_10045a86:
        FUN_10046920(param_2,s_die_spin_100a307c,0x14,0);
      }
    }
    else {
      iVar14 = (*DAT_100ae630)(0,2);
      if (iVar14 == 0) {
        FUN_10046920(param_2,s_normal_die1_100a3088,0x14,0);
      }
      else if (iVar14 == 1) {
        FUN_10046920(param_2,s_normal_die2_100a3094,0x14,0);
      }
      else if (iVar14 == 2) {
        FUN_10046920(param_2,s_normal_die3_100a30a0,0x14,0);
      }
    }
    break;
  case 0x28:
    FUN_1000c120(param_2[1] + 0x74);
    uVar12 = FUN_100148f0(auStack_70);
    FUN_1000c120(uVar12);
    iVar11 = FUN_10044510();
    param_1[5] = iVar11;
    uVar15 = 0xffffffff;
    piVar19 = param_1 + iVar11 * 4 + 0x17;
    do {
      piVar23 = piVar19;
      if (uVar15 == 0) break;
      uVar15 = uVar15 - 1;
      piVar23 = (int *)((int)piVar19 + 1);
      iVar11 = *piVar19;
      piVar19 = piVar23;
    } while ((char)iVar11 != '\0');
    uVar15 = ~uVar15;
    pcVar29 = (char *)((int)piVar23 - uVar15);
    pcVar21 = acStack_40;
    for (uVar16 = uVar15 >> 2; uVar16 != 0; uVar16 = uVar16 - 1) {
      *(undefined4 *)pcVar21 = *(undefined4 *)pcVar29;
      pcVar29 = pcVar29 + 4;
      pcVar21 = pcVar21 + 4;
    }
    for (uVar15 = uVar15 & 3; uVar15 != 0; uVar15 = uVar15 - 1) {
      *pcVar21 = *pcVar29;
      pcVar29 = pcVar29 + 1;
      pcVar21 = pcVar21 + 1;
    }
    pbVar13 = (byte *)(param_2 + 0x2d1);
    pbVar17 = &DAT_100a9398;
    do {
      bVar3 = *pbVar13;
      bVar24 = bVar3 < *pbVar17;
      if (bVar3 != *pbVar17) goto LAB_1004591b;
      if (bVar3 == 0) break;
      bVar3 = pbVar13[1];
      bVar24 = bVar3 < pbVar17[1];
      if (bVar3 != pbVar17[1]) goto LAB_1004591b;
      pbVar13 = pbVar13 + 2;
      pbVar17 = pbVar17 + 2;
    } while (bVar3 != 0);
    iVar11 = 0;
LAB_10045920:
    piVar19 = param_2 + 0x2d1;
    if (iVar11 == 0) {
      piVar19 = (int *)&DAT_1009a3e0;
    }
    uVar15 = 0xffffffff;
    do {
      piVar23 = piVar19;
      if (uVar15 == 0) break;
      uVar15 = uVar15 - 1;
      piVar23 = (int *)((int)piVar19 + 1);
      iVar11 = *piVar19;
      piVar19 = piVar23;
    } while ((char)iVar11 != '\0');
    uVar15 = ~uVar15;
    iVar11 = -1;
    pcVar29 = acStack_40;
    do {
      pcVar21 = pcVar29;
      if (iVar11 == 0) break;
      iVar11 = iVar11 + -1;
      pcVar21 = pcVar29 + 1;
      cVar2 = *pcVar29;
      pcVar29 = pcVar21;
    } while (cVar2 != '\0');
    pcVar29 = (char *)((int)piVar23 - uVar15);
    pcVar21 = pcVar21 + -1;
    for (uVar16 = uVar15 >> 2; uVar16 != 0; uVar16 = uVar16 - 1) {
      *(undefined4 *)pcVar21 = *(undefined4 *)pcVar29;
      pcVar29 = pcVar29 + 4;
      pcVar21 = pcVar21 + 4;
    }
    uVar12 = 0x94;
    for (uVar15 = uVar15 & 3; uVar15 != 0; uVar15 = uVar15 - 1) {
      *pcVar21 = *pcVar29;
      pcVar29 = pcVar29 + 1;
      pcVar21 = pcVar21 + 1;
    }
    goto LAB_100461a5;
  case 0x29:
    pcVar29 = (char *)(param_1 + param_1[5] * 4 + 0x37);
    goto LAB_1004571c;
  case 0x2a:
  case 0x2b:
    if ((char)param_2[0x142] == '\0') {
      uVar15 = 0xffffffff;
      pcVar29 = s_slide_f__100a2f18;
      do {
        pcVar21 = pcVar29;
        if (uVar15 == 0) break;
        uVar15 = uVar15 - 1;
        pcVar21 = pcVar29 + 1;
        cVar2 = *pcVar29;
        pcVar29 = pcVar21;
      } while (cVar2 != '\0');
      uVar15 = ~uVar15;
      pcVar29 = pcVar21 + -uVar15;
      pcVar21 = acStack_40;
      for (uVar16 = uVar15 >> 2; uVar16 != 0; uVar16 = uVar16 - 1) {
        *(undefined4 *)pcVar21 = *(undefined4 *)pcVar29;
        pcVar29 = pcVar29 + 4;
        pcVar21 = pcVar21 + 4;
      }
      for (uVar15 = uVar15 & 3; uVar15 != 0; uVar15 = uVar15 - 1) {
        *pcVar21 = *pcVar29;
        pcVar29 = pcVar29 + 1;
        pcVar21 = pcVar21 + 1;
      }
      pbVar13 = (byte *)(param_2 + 0x2d1);
      pbVar18 = &DAT_100a9398;
      pbVar17 = pbVar13;
      do {
        bVar3 = *pbVar17;
        bVar24 = bVar3 < *pbVar18;
        if (bVar3 != *pbVar18) {
LAB_100457cb:
          iVar11 = (1 - (uint)bVar24) - (uint)(bVar24 != 0);
          goto LAB_100457d0;
        }
        if (bVar3 == 0) break;
        bVar3 = pbVar17[1];
        bVar24 = bVar3 < pbVar18[1];
        if (bVar3 != pbVar18[1]) goto LAB_100457cb;
        pbVar17 = pbVar17 + 2;
        pbVar18 = pbVar18 + 2;
      } while (bVar3 != 0);
      iVar11 = 0;
LAB_100457d0:
      if (iVar11 == 0) {
        pbVar13 = &DAT_1009a3e0;
      }
      uVar15 = 0xffffffff;
      do {
        pbVar17 = pbVar13;
        if (uVar15 == 0) break;
        uVar15 = uVar15 - 1;
        pbVar17 = pbVar13 + 1;
        bVar3 = *pbVar13;
        pbVar13 = pbVar17;
      } while (bVar3 != 0);
      uVar15 = ~uVar15;
      iVar11 = -1;
      pcVar29 = acStack_40;
      do {
        pcVar21 = pcVar29;
        if (iVar11 == 0) break;
        iVar11 = iVar11 + -1;
        pcVar21 = pcVar29 + 1;
        cVar2 = *pcVar29;
        pcVar29 = pcVar21;
      } while (cVar2 != '\0');
      pbVar13 = pbVar17 + -uVar15;
      pbVar17 = (byte *)(pcVar21 + -1);
      for (uVar16 = uVar15 >> 2; uVar16 != 0; uVar16 = uVar16 - 1) {
        *(undefined4 *)pbVar17 = *(undefined4 *)pbVar13;
        pbVar13 = pbVar13 + 4;
        pbVar17 = pbVar17 + 4;
      }
      for (uVar15 = uVar15 & 3; uVar15 != 0; uVar15 = uVar15 - 1) {
        *pbVar17 = *pbVar13;
        pbVar13 = pbVar13 + 1;
        pbVar17 = pbVar17 + 1;
      }
      pcVar29 = &DAT_100a30b4;
      if (param_3 != 0x2a) {
        pcVar29 = s__right_100a30ac;
      }
      uVar15 = 0xffffffff;
      do {
        pcVar21 = pcVar29;
        if (uVar15 == 0) break;
        uVar15 = uVar15 - 1;
        pcVar21 = pcVar29 + 1;
        cVar2 = *pcVar29;
        pcVar29 = pcVar21;
      } while (cVar2 != '\0');
      uVar15 = ~uVar15;
      iVar11 = -1;
      pcVar29 = acStack_40;
      do {
        pcVar20 = pcVar29;
        if (iVar11 == 0) break;
        iVar11 = iVar11 + -1;
        pcVar20 = pcVar29 + 1;
        cVar2 = *pcVar29;
        pcVar29 = pcVar20;
      } while (cVar2 != '\0');
      pcVar29 = pcVar21 + -uVar15;
      pcVar21 = pcVar20 + -1;
      for (uVar16 = uVar15 >> 2; uVar16 != 0; uVar16 = uVar16 - 1) {
        *(undefined4 *)pcVar21 = *(undefined4 *)pcVar29;
        pcVar29 = pcVar29 + 4;
        pcVar21 = pcVar21 + 4;
      }
      uVar12 = 0x14;
      for (uVar15 = uVar15 & 3; uVar15 != 0; uVar15 = uVar15 - 1) {
        *pcVar21 = *pcVar29;
        pcVar29 = pcVar29 + 1;
        pcVar21 = pcVar21 + 1;
      }
    }
    else {
      pcVar29 = s_slide_b_left_100a30cc;
      if (param_3 != 0x2a) {
        pcVar29 = s_slide_b_right_100a30bc;
      }
      uVar15 = 0xffffffff;
      do {
        pcVar21 = pcVar29;
        if (uVar15 == 0) break;
        uVar15 = uVar15 - 1;
        pcVar21 = pcVar29 + 1;
        cVar2 = *pcVar29;
        pcVar29 = pcVar21;
      } while (cVar2 != '\0');
      uVar15 = ~uVar15;
      pcVar29 = pcVar21 + -uVar15;
      pcVar21 = acStack_40;
      for (uVar16 = uVar15 >> 2; uVar16 != 0; uVar16 = uVar16 - 1) {
        *(undefined4 *)pcVar21 = *(undefined4 *)pcVar29;
        pcVar29 = pcVar29 + 4;
        pcVar21 = pcVar21 + 4;
      }
      uVar12 = 0x14;
      for (uVar15 = uVar15 & 3; uVar15 != 0; uVar15 = uVar15 - 1) {
        *pcVar21 = *pcVar29;
        pcVar29 = pcVar29 + 1;
        pcVar21 = pcVar21 + 1;
      }
    }
LAB_100461a5:
    FUN_10046920(param_2,acStack_40,uVar12,0);
    iVar11 = param_3;
    break;
  case 0x2e:
    uVar25 = 0;
    uVar12 = 0x94;
    FUN_1000c120(param_2[1] + 0x74);
    FUN_1000c120(param_2 + 0x13f);
    iVar14 = FUN_10044510();
    iVar14 = iVar14 * 0x10 + 0x11c;
    goto LAB_100456f3;
  case 0x2f:
    uVar25 = 0;
    uVar12 = 0x94;
    FUN_1000c120(param_2[1] + 0x74);
    FUN_1000c120(param_2 + 0x13f);
    iVar14 = FUN_10044510();
    iVar14 = iVar14 * 0x10 + 0x15c;
LAB_100456f3:
    FUN_10046920(param_2,iVar14 + (int)param_1,uVar12,uVar25);
    param_1[2] = 0x40400000;
    break;
  case 0x30:
    uVar25 = 0;
    uVar12 = 0x94;
    FUN_1000c120(param_2[1] + 0x74);
    FUN_1000c120(param_2 + 0x13f);
    iVar14 = FUN_10044510();
    FUN_10046920(param_2,param_1 + iVar14 * 4 + 0x67,uVar12,uVar25);
    break;
  case 0x31:
    FUN_1000c120(param_2[1] + 0x74);
    FUN_1000c120(param_2 + 0x13f);
    iVar14 = FUN_10044510();
    if (iVar14 == 0) {
      FUN_10046920(param_2,s_wheel_l_100a3108,0x94,0);
      param_1[2] = 0x40400000;
    }
    else if (iVar14 == 1) {
      FUN_10046920(param_2,s_wheel_r_100a3110,0x94,0);
      param_1[2] = 0x40400000;
    }
    else if (iVar14 == 3) {
      FUN_10046920(param_2,s_wheel_b_100a3118,0x94,0);
      param_1[2] = 0x40400000;
    }
    else {
      FUN_10046920(param_2,s_wheel_f_100a3120,0x94,0);
      param_1[2] = 0x40400000;
    }
    break;
  case 0x32:
    FUN_1000c120(param_2[1] + 0x74);
    FUN_1000c120(param_2 + 0x13f);
    iVar14 = FUN_10044510();
    if (iVar14 == 2) {
      pcVar29 = s_lowwheel_f_100a3140;
    }
    else {
      pcVar29 = s_lowwheel_l_100a3134;
      if (iVar14 != 0) {
        pcVar29 = s_lowwheel_r_100a3128;
      }
    }
LAB_1004571c:
    FUN_10046920(param_2,pcVar29,0x94,0);
    param_1[2] = 0x40400000;
    break;
  case 0x33:
  case 0x34:
    if ((*(uint *)(param_2[1] + 0x1a4) & 0x200) == 0) {
      return;
    }
    if ((*(byte *)(param_2[1] + 0x250) & 0x30) != 0) {
      return;
    }
    pbVar17 = &DAT_100a9398;
    pbVar13 = (byte *)(param_2 + 0x2d1);
    do {
      bVar3 = *pbVar13;
      bVar24 = bVar3 < *pbVar17;
      if (bVar3 != *pbVar17) {
LAB_10045408:
        iVar14 = (1 - (uint)bVar24) - (uint)(bVar24 != 0);
        goto LAB_1004540d;
      }
      if (bVar3 == 0) break;
      bVar3 = pbVar13[1];
      bVar24 = bVar3 < pbVar17[1];
      if (bVar3 != pbVar17[1]) goto LAB_10045408;
      pbVar13 = pbVar13 + 2;
      pbVar17 = pbVar17 + 2;
    } while (bVar3 != 0);
    iVar14 = 0;
LAB_1004540d:
    if (iVar14 == 0) {
      return;
    }
    pcVar29 = s_relo__100a3154;
    if (param_3 != 0x33) {
      pcVar29 = &DAT_100a314c;
    }
    uVar15 = 0xffffffff;
    do {
      pcVar21 = pcVar29;
      if (uVar15 == 0) break;
      uVar15 = uVar15 - 1;
      pcVar21 = pcVar29 + 1;
      cVar2 = *pcVar29;
      pcVar29 = pcVar21;
    } while (cVar2 != '\0');
    uVar15 = ~uVar15;
    pcVar29 = pcVar21 + -uVar15;
    pcVar21 = acStack_40;
    for (uVar16 = uVar15 >> 2; uVar16 != 0; uVar16 = uVar16 - 1) {
      *(undefined4 *)pcVar21 = *(undefined4 *)pcVar29;
      pcVar29 = pcVar29 + 4;
      pcVar21 = pcVar21 + 4;
    }
    for (uVar15 = uVar15 & 3; uVar15 != 0; uVar15 = uVar15 - 1) {
      *pcVar21 = *pcVar29;
      pcVar29 = pcVar29 + 1;
      pcVar21 = pcVar21 + 1;
    }
    uVar15 = 0xffffffff;
    pbVar13 = (byte *)(param_2 + 0x2d1);
    do {
      pbVar17 = pbVar13;
      if (uVar15 == 0) break;
      uVar15 = uVar15 - 1;
      pbVar17 = pbVar13 + 1;
      bVar3 = *pbVar13;
      pbVar13 = pbVar17;
    } while (bVar3 != 0);
    uVar15 = ~uVar15;
    iVar14 = -1;
    pcVar29 = acStack_40;
    do {
      pcVar21 = pcVar29;
      if (iVar14 == 0) break;
      iVar14 = iVar14 + -1;
      pcVar21 = pcVar29 + 1;
      cVar2 = *pcVar29;
      pcVar29 = pcVar21;
    } while (cVar2 != '\0');
    pbVar13 = pbVar17 + -uVar15;
    pbVar17 = (byte *)(pcVar21 + -1);
    for (uVar16 = uVar15 >> 2; uVar16 != 0; uVar16 = uVar16 - 1) {
      *(undefined4 *)pbVar17 = *(undefined4 *)pbVar13;
      pbVar13 = pbVar13 + 4;
      pbVar17 = pbVar17 + 4;
    }
    for (uVar15 = uVar15 & 3; uVar15 != 0; uVar15 = uVar15 - 1) {
      *pbVar17 = *pbVar13;
      pbVar13 = pbVar13 + 1;
      pbVar17 = pbVar17 + 1;
    }
    FUN_10046920(param_2,acStack_40,0x92,0);
    param_1[3] = 0x40000000;
    break;
  case 0x35:
    if ((*(uint *)(param_2[1] + 0x1a4) & 0x200) == 0) {
      return;
    }
    if ((*(byte *)(param_2[1] + 0x250) & 0x30) != 0) {
      return;
    }
    FUN_10046920(param_2,s_grenade_100a315c,0x92,0);
    param_1[3] = 0x40000000;
    param_1[4] = 0x3f800000;
    break;
  case 0x36:
    FUN_10046920(param_2,s_bullettime_100a30dc,0x94,0);
    break;
  case 0x37:
    FUN_10046920(param_2,s_walljump_end_100a30f8,0x94,0);
    param_1[2] = 0x40400000;
    break;
  case 0x38:
    FUN_10046920(param_2,s_walljump_climb_100a30e8,0x94,0);
    param_1[2] = 0x40400000;
    break;
  case 0x39:
    if ((*(byte *)(param_2[1] + 0x250) & 0x24) != 0) {
      return;
    }
    pbVar17 = &DAT_100a9398;
    pbVar13 = (byte *)(param_2 + 0x2d1);
    do {
      bVar3 = *pbVar13;
      bVar24 = bVar3 < *pbVar17;
      if (bVar3 != *pbVar17) {
LAB_10045d29:
        iVar14 = (1 - (uint)bVar24) - (uint)(bVar24 != 0);
        goto LAB_10045d2e;
      }
      if (bVar3 == 0) break;
      bVar3 = pbVar13[1];
      bVar24 = bVar3 < pbVar17[1];
      if (bVar3 != pbVar17[1]) goto LAB_10045d29;
      pbVar13 = pbVar13 + 2;
      pbVar17 = pbVar17 + 2;
    } while (bVar3 != 0);
    iVar14 = 0;
LAB_10045d2e:
    pcVar29 = s_morfy_jump_100a3028;
    if (iVar14 != 0) {
      pcVar29 = s_morfy_jump_guns_100a3018;
    }
    uVar15 = 0xffffffff;
    do {
      pcVar21 = pcVar29;
      if (uVar15 == 0) break;
      uVar15 = uVar15 - 1;
      pcVar21 = pcVar29 + 1;
      cVar2 = *pcVar29;
      pcVar29 = pcVar21;
    } while (cVar2 != '\0');
    uVar15 = ~uVar15;
    pcVar29 = pcVar21 + -uVar15;
    pcVar21 = acStack_40;
    for (uVar16 = uVar15 >> 2; uVar16 != 0; uVar16 = uVar16 - 1) {
      *(undefined4 *)pcVar21 = *(undefined4 *)pcVar29;
      pcVar29 = pcVar29 + 4;
      pcVar21 = pcVar21 + 4;
    }
    for (uVar15 = uVar15 & 3; uVar15 != 0; uVar15 = uVar15 - 1) {
      *pcVar21 = *pcVar29;
      pcVar29 = pcVar29 + 1;
      pcVar21 = pcVar21 + 1;
    }
    FUN_10046920(param_2,acStack_40,0x94,0);
  }
switchD_10045371_caseD_2:
  param_3 = iVar11;
  *param_1 = param_3;
LAB_100461c4:
  if (param_3 == 4) {
    return;
  }
  if (param_3 == 0x39) {
    return;
  }
  if (param_3 == 5) {
    return;
  }
  if (param_3 == 7) {
    return;
  }
  if (param_3 == 0x29) {
    return;
  }
  if (param_3 == 0x2e) {
    return;
  }
  if (param_3 == 0x2f) {
    return;
  }
  if (param_3 == 0x30) {
    return;
  }
  if (param_3 == 0x36) {
    return;
  }
  iVar11 = FUN_10046f90(param_3);
  if ((iVar11 != 0) && (iVar11 = FUN_10046fc0(param_3,param_2), iVar11 == 0)) {
    return;
  }
  if ((*(uint *)(param_2[1] + 0x250) & 0x30c30) != 0) {
    return;
  }
  if ((*(uint *)(param_2[1] + 0x1a4) & 0x4000) == 0) {
    if (fVar4 <= _DAT_1008f4e4) {
      if (fVar4 <= _DAT_10089110) {
        pcVar29 = (char *)(param_2 + 0x119);
        pbVar17 = &DAT_100a9398;
        pbVar13 = (byte *)pcVar29;
        do {
          bVar3 = *pbVar13;
          bVar24 = bVar3 < *pbVar17;
          if (bVar3 != *pbVar17) {
LAB_100462fd:
            iVar11 = (1 - (uint)bVar24) - (uint)(bVar24 != 0);
            goto LAB_10046302;
          }
          if (bVar3 == 0) break;
          bVar3 = pbVar13[1];
          bVar24 = bVar3 < pbVar17[1];
          if (bVar3 != pbVar17[1]) goto LAB_100462fd;
          pbVar13 = pbVar13 + 2;
          pbVar17 = pbVar17 + 2;
        } while (bVar3 != 0);
        iVar11 = 0;
LAB_10046302:
        if (iVar11 != 0) goto LAB_1004626d;
        pbVar17 = &DAT_100a9398;
        pbVar13 = (byte *)(param_2 + 0x2d1);
        do {
          bVar3 = *pbVar13;
          bVar24 = bVar3 < *pbVar17;
          if (bVar3 != *pbVar17) {
LAB_1004633d:
            iVar11 = (1 - (uint)bVar24) - (uint)(bVar24 != 0);
            goto LAB_10046342;
          }
          if (bVar3 == 0) break;
          bVar3 = pbVar13[1];
          bVar24 = bVar3 < pbVar17[1];
          if (bVar3 != pbVar17[1]) goto LAB_1004633d;
          pbVar13 = pbVar13 + 2;
          pbVar17 = pbVar17 + 2;
        } while (bVar3 != 0);
        iVar11 = 0;
LAB_10046342:
        if (iVar11 != 0) {
          uVar15 = 0xffffffff;
          pcVar29 = &DAT_100a2fcc;
          do {
            pcVar21 = pcVar29;
            if (uVar15 == 0) break;
            uVar15 = uVar15 - 1;
            pcVar21 = pcVar29 + 1;
            cVar2 = *pcVar29;
            pcVar29 = pcVar21;
          } while (cVar2 != '\0');
          uVar15 = ~uVar15;
          pcVar29 = pcVar21 + -uVar15;
          pcVar21 = acStack_40;
          for (uVar16 = uVar15 >> 2; uVar16 != 0; uVar16 = uVar16 - 1) {
            *(undefined4 *)pcVar21 = *(undefined4 *)pcVar29;
            pcVar29 = pcVar29 + 4;
            pcVar21 = pcVar21 + 4;
          }
          for (uVar15 = uVar15 & 3; uVar15 != 0; uVar15 = uVar15 - 1) {
            *pcVar21 = *pcVar29;
            pcVar29 = pcVar29 + 1;
            pcVar21 = pcVar21 + 1;
          }
          uVar15 = 0xffffffff;
          pbVar13 = (byte *)(param_2 + 0x2d1);
          do {
            pbVar17 = pbVar13;
            if (uVar15 == 0) break;
            uVar15 = uVar15 - 1;
            pbVar17 = pbVar13 + 1;
            bVar3 = *pbVar13;
            pbVar13 = pbVar17;
          } while (bVar3 != 0);
          uVar15 = ~uVar15;
          iVar11 = -1;
          pcVar29 = acStack_40;
          do {
            pcVar21 = pcVar29;
            if (iVar11 == 0) break;
            iVar11 = iVar11 + -1;
            pcVar21 = pcVar29 + 1;
            cVar2 = *pcVar29;
            pcVar29 = pcVar21;
          } while (cVar2 != '\0');
          pbVar13 = pbVar17 + -uVar15;
          pbVar17 = (byte *)(pcVar21 + -1);
          for (uVar16 = uVar15 >> 2; uVar16 != 0; uVar16 = uVar16 - 1) {
            *(undefined4 *)pbVar17 = *(undefined4 *)pbVar13;
            pbVar13 = pbVar13 + 4;
            pbVar17 = pbVar17 + 4;
          }
          for (uVar15 = uVar15 & 3; uVar15 != 0; uVar15 = uVar15 - 1) {
            *pbVar17 = *pbVar13;
            pbVar13 = pbVar13 + 1;
            pbVar17 = pbVar17 + 1;
          }
          uVar12 = FUN_100083f0(acStack_40);
          *(undefined4 *)(param_2[1] + 300) = uVar12;
          goto LAB_100463d4;
        }
        pcVar29 = s_combat_idle_low_100a2f90;
        if (_DAT_1008c494 <= (float)param_1[3]) {
          pcVar29 = s_combat_idle_100a2f84;
        }
      }
      else {
        pcVar29 = s_walk2_100a2fa0;
      }
LAB_10046287:
      uVar12 = FUN_100083f0(pcVar29);
      *(undefined4 *)(param_2[1] + 300) = uVar12;
      goto LAB_100463d4;
    }
    pcVar29 = &DAT_100a2fa8;
  }
  else {
    bVar24 = fVar4 != _DAT_10089110;
    param_1[1] = 0;
    if (bVar24) {
      pcVar29 = s_crawl_100a2fb0;
      goto LAB_10046287;
    }
    pcVar29 = s_crouch_idle_100a2fb8;
  }
LAB_1004626d:
  uVar12 = FUN_100083f0(pcVar29);
  *(undefined4 *)(param_2[1] + 300) = uVar12;
LAB_100463d4:
  pbVar17 = &DAT_100a9398;
  pbVar13 = (byte *)(param_2 + 0x119);
  do {
    bVar3 = *pbVar13;
    bVar24 = bVar3 < *pbVar17;
    if (bVar3 != *pbVar17) {
LAB_10046403:
      iVar11 = (1 - (uint)bVar24) - (uint)(bVar24 != 0);
      goto LAB_10046408;
    }
    if (bVar3 == 0) break;
    bVar3 = pbVar13[1];
    bVar24 = bVar3 < pbVar17[1];
    if (bVar3 != pbVar17[1]) goto LAB_10046403;
    pbVar13 = pbVar13 + 2;
    pbVar17 = pbVar17 + 2;
  } while (bVar3 != 0);
  iVar11 = 0;
LAB_10046408:
  if (iVar11 == 0) {
    pbVar17 = &DAT_100a9398;
    pbVar13 = (byte *)(param_2 + 0x2d1);
    do {
      bVar3 = *pbVar13;
      bVar24 = bVar3 < *pbVar17;
      if (bVar3 != *pbVar17) {
LAB_1004643f:
        iVar11 = (1 - (uint)bVar24) - (uint)(bVar24 != 0);
        goto LAB_10046444;
      }
      if (bVar3 == 0) break;
      bVar3 = pbVar13[1];
      bVar24 = bVar3 < pbVar17[1];
      if (bVar3 != pbVar17[1]) goto LAB_1004643f;
      pbVar13 = pbVar13 + 2;
      pbVar17 = pbVar17 + 2;
    } while (bVar3 != 0);
    iVar11 = 0;
LAB_10046444:
    if (((iVar11 == 0) && (iVar11 = FUN_10046fc0(param_3,param_2), iVar11 == 0)) &&
       (((float)param_1[3] < _DAT_1008c494 || (param_2[0x1a] != 0)))) {
      iVar11 = param_2[1];
      if (*(int *)(iVar11 + 0x128) != *(int *)(iVar11 + 300)) {
        *(int *)(iVar11 + 0x128) = *(int *)(iVar11 + 300);
        *(undefined4 *)(param_2[1] + 0x130) = 0;
        FUN_10008420();
      }
    }
  }
  return;
LAB_1004591b:
  iVar11 = (1 - (uint)bVar24) - (uint)(bVar24 != 0);
  goto LAB_10045920;
}


