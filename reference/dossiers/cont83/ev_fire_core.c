// ===== FUN @ 1003f090 size=6881 =====

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void __cdecl FUN_1003f090(int param_1,float param_2,char param_3)

{
  char cVar1;
  bool bVar2;
  int iVar3;
  float *pfVar4;
  void *pvVar5;
  undefined4 *puVar6;
  undefined3 extraout_var;
  float fVar7;
  uint uVar8;
  float *pfVar9;
  int iVar10;
  undefined3 extraout_var_00;
  float *pfVar11;
  short sVar12;
  float fVar13;
  float fVar14;
  undefined4 uVar15;
  float *pfVar16;
  undefined4 uVar17;
  float *in_stack_ffffff08;
  float *in_stack_ffffff0c;
  float fVar18;
  float *pfVar19;
  float fVar20;
  float fVar21;
  float fVar22;
  float fStack_d0;
  float fStack_cc;
  float fStack_c8;
  int iStack_c4;
  float fStack_c0;
  float fStack_bc;
  float fStack_b8;
  float fStack_b4;
  float fStack_b0;
  float fStack_ac;
  float fStack_a8;
  float fStack_a4;
  float fStack_a0;
  float fStack_9c;
  float fStack_98;
  float fStack_94;
  float afStack_90 [3];
  float fStack_84;
  float fStack_80;
  float fStack_7c;
  float fStack_78;
  float fStack_74;
  float fStack_70;
  float afStack_6c [3];
  float fStack_60;
  float fStack_5c;
  float fStack_58;
  float fStack_54;
  float fStack_50;
  float fStack_4c;
  float fStack_48;
  float fStack_44;
  float fStack_40;
  float fStack_3c;
  float fStack_38;
  float fStack_34;
  float fStack_30;
  float fStack_2c;
  float fStack_28;
  float fStack_24;
  float fStack_20;
  float fStack_1c;
  float fStack_18;
  float fStack_14;
  float fStack_10;
  undefined4 auStack_c [3];
  
  if ((DAT_10189edc & 1) == 0) {
    DAT_10189edc = DAT_10189edc | 1;
    FUN_1006af63(0x10040c10);
  }
  if ((DAT_10189edc & 2) == 0) {
    DAT_10189edc = DAT_10189edc | 2;
    FUN_1006af63(0x10040c00);
  }
  if ((DAT_10189edc & 4) == 0) {
    DAT_10189edc = DAT_10189edc | 4;
    FUN_1006af63(0x10040bf0);
  }
  if ((DAT_10189edc & 8) == 0) {
    DAT_10189edc = DAT_10189edc | 8;
    FUN_1006af63(0x10040be0);
  }
  if ((DAT_10189edc & 0x10) == 0) {
    DAT_10189edc = DAT_10189edc | 0x10;
    FUN_1006af63(0x10040bd0);
  }
  if ((DAT_10189edc & 0x20) == 0) {
    DAT_10189edc = DAT_10189edc | 0x20;
    FUN_1006af63(0x10040bc0);
  }
  if ((DAT_10189edc & 0x40) == 0) {
    DAT_10189edc = DAT_10189edc | 0x40;
    FUN_1006af63(0x10040bb0);
  }
  if ((DAT_10189edc & 0x80) == 0) {
    DAT_10189edc = DAT_10189edc | 0x80;
    FUN_1006af63(0x10040ba0);
  }
  if ((DAT_10189e7c & 1) == 0) {
    DAT_10189e7c = DAT_10189e7c | 1;
    FUN_1006af63(0x10040b90);
  }
  if ((DAT_10189e7c & 2) == 0) {
    DAT_10189e7c = DAT_10189e7c | 2;
    FUN_1006af63(0x10040b80);
  }
  DAT_10189c14 = *(float **)(param_1 + 4);
  _DAT_10189ed0 = *(undefined4 *)(param_1 + 8);
  _DAT_10189ed4 = *(undefined4 *)(param_1 + 0xc);
  _DAT_10189ed8 = *(undefined4 *)(param_1 + 0x10);
  _DAT_1018a000 = *(undefined4 *)(param_1 + 0x14);
  _DAT_1018a004 = *(undefined4 *)(param_1 + 0x18);
  _DAT_1018a008 = *(undefined4 *)(param_1 + 0x1c);
  _DAT_10189ea0 = *(float *)(param_1 + 0x20);
  _DAT_10189ea4 = *(float *)(param_1 + 0x24);
  _DAT_10189ea8 = *(float *)(param_1 + 0x28);
  _DAT_10189c9c = *(undefined4 *)(param_1 + 0x40);
  DAT_10189d3c = *(int *)(param_1 + 0x40);
  pfVar19 = (float *)0x1003f275;
  (*DAT_100d5b88)();
  DAT_10189a5c = *(int *)(param_1 + 0x38);
  _DAT_1018a0b4 = *(uint *)(param_1 + 0x3c);
  if (*(char *)((int)param_2 + 0xe8) == '\0') {
    DAT_10189d5c = 0;
  }
  else {
    DAT_10189d5c = (**(code **)(DAT_100d5c78 + 0xc))();
  }
  _DAT_10189b40 = (*DAT_100d5c38)();
  DAT_10189a3c = (*DAT_100d5c38)();
  if (((_DAT_1018a0b4 & 1) == 0) &&
     ((cVar1 = FUN_100579a0((int)DAT_10189c14), cVar1 != '\0' ||
      (iVar3 = FUN_1001c5e0((int)DAT_10189c14), iVar3 != 0)))) {
    pfVar19 = (float *)FUN_1003d240(&DAT_10189d18,afStack_90,32.0);
    in_stack_ffffff0c = afStack_6c;
    pfVar4 = (float *)FUN_1003d240(&DAT_10189cb8,&fStack_d0,16.0);
    pvVar5 = (void *)FUN_10035b10(&DAT_10189ed0,&fStack_c0,pfVar4);
    in_stack_ffffff08 = (float *)0x1003f377;
    puVar6 = (undefined4 *)FUN_10035b10(pvVar5,in_stack_ffffff0c,pfVar19);
    FUN_10002610(&stack0xffffff08,puVar6);
    FUN_10048e60(in_stack_ffffff08,in_stack_ffffff0c,pfVar19);
  }
  iVar3 = FUN_1001c5e0((int)DAT_10189c14);
  if (iVar3 != 0) {
    DAT_10189d54 = FUN_1003df60((int)param_2);
    DAT_101899c4 = FUN_10007110(DAT_10189a5c,_DAT_1018a0b4);
    if (param_3 == '\0') {
      if (DAT_10189d3c == 0) {
        iVar3 = (*DAT_100d5c38)();
        sVar12 = *(short *)(DAT_10189d54 + 0x30 + iVar3 * 2);
      }
      else {
        iVar3 = (*DAT_100d5c38)();
        sVar12 = *(short *)(DAT_10189d54 + 0x2c + iVar3 * 2);
      }
    }
    else {
      iVar3 = (*DAT_100d5c38)();
      sVar12 = *(short *)(DAT_10189d54 + 6 + iVar3 * 2);
    }
    _DAT_10189e6c = (int)sVar12;
    (**(code **)(DAT_100d5c78 + 0x40))();
    bVar2 = CL_IsThirdPerson();
    if (CONCAT31(extraout_var,bVar2) == 0) {
      if (DAT_101914a0 == 0) {
        if (param_3 == '\0') {
          if (DAT_10189d5c == 0) goto LAB_100403d3;
          iStack_c4 = DAT_10189a3c + 0x32;
          fVar21 = (float)iStack_c4;
          puVar6 = (undefined4 *)
                   FUN_10009190(afStack_6c,fVar21 * DAT_10189cb8,fVar21 * DAT_10189cbc,
                                fVar21 * DAT_10189cc0);
          FUN_10002610(&fStack_b4,puVar6);
          fVar21 = (float)DAT_10189a3c;
          puVar6 = (undefined4 *)
                   FUN_10009190(&fStack_78,fVar21 * _DAT_10189d18,fVar21 * _DAT_10189d1c,
                                fVar21 * _DAT_10189d20);
          FUN_10002610(&fStack_c0,puVar6);
          fVar21 = (float)_DAT_10189b40;
          puVar6 = (undefined4 *)
                   FUN_10009190(&fStack_84,fVar21 * _DAT_10189e50,fVar21 * _DAT_10189e54,
                                fVar21 * _DAT_10189e58);
          FUN_10002610(&fStack_d0,puVar6);
          puVar6 = (undefined4 *)
                   FUN_10009190(&fStack_48,_DAT_10189ea0 + fStack_d0,_DAT_10189ea4 + fStack_cc,
                                _DAT_10189ea8 + fStack_c8);
          FUN_10002610(&fStack_9c,puVar6);
          puVar6 = (undefined4 *)
                   FUN_10009190(&fStack_54,fStack_c0 + fStack_9c,fStack_bc + fStack_98,
                                fStack_b8 + fStack_94);
          FUN_10002610(&fStack_a8,puVar6);
          puVar6 = (undefined4 *)
                   FUN_10009190(&fStack_60,fStack_b4 + fStack_a8,fStack_b0 + fStack_a4,
                                fStack_ac + fStack_a0);
          FUN_10002610(afStack_90,puVar6);
          in_stack_ffffff0c = &DAT_10190b28;
        }
        else {
          if (DAT_10189d5c == 0) goto LAB_100403d3;
          iStack_c4 = DAT_10189a3c + 0x32;
          fStack_c8 = (float)iStack_c4;
          fStack_d0 = fStack_c8 * DAT_10189cb8;
          fStack_cc = fStack_c8 * DAT_10189cbc;
          fStack_c8 = fStack_c8 * DAT_10189cc0;
          FUN_10002610(&fStack_48,&fStack_d0);
          fStack_b8 = (float)DAT_10189a3c;
          fStack_c0 = fStack_b8 * _DAT_10189d18;
          fStack_bc = fStack_b8 * _DAT_10189d1c;
          fStack_b8 = fStack_b8 * _DAT_10189d20;
          FUN_10002610(&fStack_78,&fStack_c0);
          fStack_94 = (float)_DAT_10189b40;
          fStack_9c = fStack_94 * _DAT_10189e50;
          fStack_98 = fStack_94 * _DAT_10189e54;
          fStack_94 = fStack_94 * _DAT_10189e58;
          FUN_10002610(&fStack_b4,&fStack_9c);
          fStack_a8 = fStack_b4 + _DAT_10189ea0;
          fStack_a4 = _DAT_10189ea4 + fStack_b0;
          fStack_a0 = _DAT_10189ea8 + fStack_ac;
          FUN_10002610(&fStack_84,&fStack_a8);
          puVar6 = (undefined4 *)
                   FUN_10009190(&fStack_18,fStack_78 + fStack_84,fStack_74 + fStack_80,
                                fStack_70 + fStack_7c);
          FUN_10002610(&fStack_54,puVar6);
          puVar6 = (undefined4 *)
                   FUN_10009190(&fStack_24,fStack_48 + fStack_54,fStack_44 + fStack_50,
                                fStack_40 + fStack_4c);
          FUN_10002610(afStack_90,puVar6);
          fStack_60 = _DAT_10189e50 * _DAT_1007d454;
          fStack_5c = _DAT_10189e54 * _DAT_1007d454;
          fStack_58 = _DAT_10189e58 * _DAT_1007d454;
          FUN_10002610(&fStack_3c,&fStack_60);
          puVar6 = (undefined4 *)
                   FUN_10009190(&fStack_30,DAT_10190b28 - fStack_3c,DAT_10190b2c - fStack_38,
                                DAT_10190b30 - fStack_34);
          FUN_10002610(afStack_6c,puVar6);
          FUN_10056890(afStack_6c,(float *)&DAT_1018a000,afStack_90,&DAT_10189ea0,DAT_10189d5c,1);
          iStack_c4 = DAT_10189a3c + 0x32;
          fStack_c8 = (float)iStack_c4;
          fStack_d0 = fStack_c8 * DAT_10189cb8;
          fStack_cc = fStack_c8 * DAT_10189cbc;
          fStack_c8 = fStack_c8 * DAT_10189cc0;
          FUN_10002610(&fStack_a8,&fStack_d0);
          fVar21 = (float)DAT_10189a3c;
          puVar6 = (undefined4 *)
                   FUN_10009190(&fStack_48,fVar21 * _DAT_10189d18,fVar21 * _DAT_10189d1c,
                                fVar21 * _DAT_10189d20);
          FUN_10002610(&fStack_9c,puVar6);
          fVar21 = (float)_DAT_10189b40;
          puVar6 = (undefined4 *)
                   FUN_10009190(&fStack_54,fVar21 * _DAT_10189e50,fVar21 * _DAT_10189e54,
                                fVar21 * _DAT_10189e58);
          FUN_10002610(&fStack_c0,puVar6);
          puVar6 = (undefined4 *)
                   FUN_10009190(&fStack_60,fStack_c0 + _DAT_10189ea0,_DAT_10189ea4 + fStack_bc,
                                _DAT_10189ea8 + fStack_b8);
          FUN_10002610(&fStack_b4,puVar6);
          puVar6 = (undefined4 *)
                   FUN_10009190(&fStack_3c,fStack_9c + fStack_b4,fStack_98 + fStack_b0,
                                fStack_94 + fStack_ac);
          FUN_10002610(&fStack_78,puVar6);
          puVar6 = (undefined4 *)
                   FUN_10009190(&fStack_18,fStack_a8 + fStack_78,fStack_a4 + fStack_74,
                                fStack_a0 + fStack_70);
          FUN_10002610(afStack_90,puVar6);
          puVar6 = (undefined4 *)
                   FUN_10009190(&fStack_24,_DAT_10189e50 * _DAT_1007d454,
                                _DAT_10189e54 * _DAT_1007d454,_DAT_10189e58 * _DAT_1007d454);
          FUN_10002610(&fStack_84,puVar6);
          puVar6 = (undefined4 *)
                   FUN_10009190(&fStack_30,fStack_84 + DAT_10190b28,DAT_10190b2c + fStack_80,
                                DAT_10190b30 + fStack_7c);
          FUN_10002610(afStack_6c,puVar6);
          in_stack_ffffff0c = afStack_6c;
        }
      }
      else {
        fVar22 = 0.0;
        fVar21 = param_2;
        if (param_3 == '\0') {
          fVar7 = (float)(_DAT_1018a0b4 & 0xffffff01);
          iVar3 = (_DAT_1018a0b4 & 1) << 3;
          fVar20 = 1.26117e-44;
          FUN_10002610(&stack0xffffff08,&DAT_10189cb8);
          pfVar4 = afStack_90;
          uVar15 = 0x41200000;
          fVar13 = 2.602418e-29;
          puVar6 = (undefined4 *)FUN_1003efb0(&DAT_10189ea0,pfVar4,10.0);
          FUN_10002610(&stack0xfffffef8,puVar6);
          uVar8 = CONCAT31((int3)((uint)-DAT_10189d3c >> 8),-(DAT_10189d3c != 0)) & 0xfffffffe;
          fVar14 = (float)CONCAT31((int3)(uVar8 >> 8),(char)uVar8 + '\x02');
        }
        else {
          fVar14 = (float)(_DAT_1018a0b4 & 0xffffff01);
          iVar3 = (_DAT_1018a0b4 & 1) << 3;
          fVar20 = 1.26117e-44;
          FUN_10002610(&stack0xffffff08,&DAT_10189cb8);
          pfVar4 = afStack_90;
          uVar17 = 0x41200000;
          uVar15 = 0x1003f49d;
          fVar7 = param_2;
          puVar6 = (undefined4 *)FUN_1003efb0(&DAT_10189ea0,pfVar4,10.0);
          FUN_10002610(&stack0xfffffef8,puVar6);
          fVar13 = 0.0;
          pfVar9 = afStack_6c;
          pfVar16 = DAT_10189c14;
          puVar6 = (undefined4 *)FUN_10056be0(pfVar9,(int)DAT_10189c14,0);
          FUN_10002610(&stack0xfffffeec,puVar6);
          FUN_100474b0((float)pfVar9,(float)pfVar16,fVar13,uVar15,pfVar4,uVar17,fVar7,
                       (float)in_stack_ffffff08,(float)in_stack_ffffff0c,(float)pfVar19,fVar20,
                       fVar14,iVar3,fVar22);
          fVar7 = (float)(_DAT_1018a0b4 & 0xffffff01);
          fVar22 = 0.0;
          iVar3 = (_DAT_1018a0b4 & 1) << 3;
          fVar20 = 1.26117e-44;
          FUN_10002610(&stack0xffffff08,&DAT_10189cb8);
          pfVar4 = afStack_90;
          uVar15 = 0x41200000;
          fVar13 = 2.602395e-29;
          puVar6 = (undefined4 *)FUN_1003efb0(&DAT_10189ea0,pfVar4,10.0);
          FUN_10002610(&stack0xfffffef8,puVar6);
          fVar14 = 2.8026e-45;
        }
        pfVar16 = afStack_6c;
        pfVar9 = DAT_10189c14;
        puVar6 = (undefined4 *)FUN_10056be0(pfVar16,(int)DAT_10189c14,(uint)fVar14);
        FUN_10002610(&stack0xfffffeec,puVar6);
        FUN_100474b0((float)pfVar16,(float)pfVar9,fVar14,fVar13,pfVar4,uVar15,fVar21,
                     (float)in_stack_ffffff08,(float)in_stack_ffffff0c,(float)pfVar19,fVar20,fVar7,
                     iVar3,fVar22);
        iVar3 = 0;
        fVar21 = 24.0;
        if ((_DAT_1018a0b4 & 1) == 0) {
          FUN_10002610(&stack0xffffff10,&DAT_10189cb8);
          in_stack_ffffff08 = afStack_90;
          in_stack_ffffff0c = (float *)0x41200000;
          fVar22 = 2.6024517e-29;
          pfVar9 = (float *)FUN_1003efb0(&DAT_10189ea0,in_stack_ffffff08,10.0);
          pfVar4 = (float *)0x1003f5cc;
          FUN_10002610(&stack0xffffff04,pfVar9);
          FUN_10002610(&stack0xfffffef8,
                       (undefined4 *)
                       (DAT_101914a0 + 0xb60 + ((-(uint)(DAT_10189d3c != 0) & 0xfffffffe) + 2) * 0xc
                       ));
          iVar10 = (*DAT_100d5c38)();
          sVar12 = 0x10;
        }
        else {
          FUN_10002610(&stack0xffffff10,&DAT_10189cb8);
          pfVar4 = afStack_90;
          fVar18 = 10.0;
          fVar14 = 2.6024842e-29;
          puVar6 = (undefined4 *)FUN_1003efb0(&DAT_10189ea0,pfVar4,10.0);
          fVar22 = 2.6024875e-29;
          FUN_10002610(&stack0xffffff04,puVar6);
          FUN_10002610(&stack0xfffffef8,
                       (undefined4 *)
                       (DAT_101914a0 + 0xb60 + ((-(uint)(DAT_10189d3c != 0) & 0xfffffffe) + 2) * 0xc
                       ));
          iVar10 = (*DAT_100d5c38)();
          FUN_10047130(8,iVar10,fVar13,fVar22,(float)puVar6,fVar14,(float)pfVar4,fVar18,
                       (float)pfVar19,fVar20,fVar7,fVar21,iVar3);
          iVar3 = 1;
          fVar21 = 4.0;
          FUN_10002610(&stack0xffffff10,(undefined4 *)&DAT_10189e50);
          in_stack_ffffff08 = afStack_90;
          in_stack_ffffff0c = (float *)0x41200000;
          fVar22 = 2.602519e-29;
          puVar6 = (undefined4 *)FUN_1003efb0(&DAT_10189ea0,in_stack_ffffff08,10.0);
          FUN_10002610(&stack0xffffff04,puVar6);
          pfVar9 = (float *)FUN_1003d240(&DAT_10189cb8,afStack_6c,
                                         (float)*(int *)((int)param_2 + 0x34c));
          pfVar4 = &fStack_d0;
          fVar13 = 2.602543e-29;
          puVar6 = (undefined4 *)
                   FUN_1003d200((void *)(DAT_101914a0 + 0xb60 +
                                        ((-(uint)(DAT_10189d3c != 0) & 0xfffffffe) + 2) * 0xc),
                                pfVar4,pfVar9);
          FUN_10002610(&stack0xfffffef8,puVar6);
          iVar10 = (*DAT_100d5c38)();
          sVar12 = 1;
        }
        FUN_10047130(sVar12,iVar10,fVar13,(float)pfVar4,(float)pfVar9,fVar22,
                     (float)in_stack_ffffff08,(float)in_stack_ffffff0c,(float)pfVar19,fVar20,fVar7,
                     fVar21,iVar3);
        if (param_3 == '\0') {
          if (DAT_10189d5c == 0) goto LAB_100403d3;
          iStack_c4 = DAT_10189a3c + 0x32;
          fStack_c8 = (float)iStack_c4;
          fStack_d0 = fStack_c8 * DAT_10189cb8;
          fStack_cc = fStack_c8 * DAT_10189cbc;
          fStack_c8 = fStack_c8 * DAT_10189cc0;
          FUN_10002610(&fStack_54,&fStack_d0);
          fStack_b8 = (float)DAT_10189a3c;
          fStack_c0 = fStack_b8 * _DAT_10189d18;
          fStack_bc = fStack_b8 * _DAT_10189d1c;
          fStack_b8 = fStack_b8 * _DAT_10189d20;
          FUN_10002610(&fStack_78,&fStack_c0);
          fStack_94 = (float)_DAT_10189b40;
          fStack_9c = fStack_94 * _DAT_10189e50;
          fStack_98 = fStack_94 * _DAT_10189e54;
          fStack_94 = fStack_94 * _DAT_10189e58;
          FUN_10002610(&fStack_b4,&fStack_9c);
          fStack_a8 = fStack_b4 + _DAT_10189ea0;
          fStack_a4 = _DAT_10189ea4 + fStack_b0;
          fStack_a0 = _DAT_10189ea8 + fStack_ac;
          FUN_10002610(&fStack_84,&fStack_a8);
          fStack_48 = fStack_78 + fStack_84;
          fStack_44 = fStack_74 + fStack_80;
          fStack_40 = fStack_70 + fStack_7c;
          FUN_10002610(&fStack_60,&fStack_48);
          fStack_3c = fStack_54 + fStack_60;
          fStack_38 = fStack_50 + fStack_5c;
          fStack_34 = fStack_4c + fStack_58;
          FUN_10002610(afStack_90,&fStack_3c);
          fStack_10 = (float)*(int *)((int)param_2 + 0x34c);
          fStack_18 = fStack_10 * DAT_10189cb8;
          fStack_14 = fStack_10 * DAT_10189cbc;
          fStack_10 = fStack_10 * DAT_10189cc0;
          FUN_10002610(&fStack_24,&fStack_18);
          iVar3 = (-(uint)(DAT_10189d3c != 0) & 0xfffffffe) + 2;
          fStack_30 = *(float *)(DAT_101914a0 + 0xb60 + iVar3 * 0xc) - fStack_24;
          iVar3 = DAT_101914a0 + 0xb60 + iVar3 * 0xc;
          fStack_2c = *(float *)(iVar3 + 4) - fStack_20;
          fStack_28 = *(float *)(iVar3 + 8) - fStack_1c;
          FUN_10002610(afStack_6c,&fStack_30);
          in_stack_ffffff0c = afStack_6c;
        }
        else {
          if (DAT_10189d5c == 0) goto LAB_100403d3;
          iStack_c4 = DAT_10189a3c + 0x32;
          fStack_28 = (float)iStack_c4;
          fStack_30 = fStack_28 * DAT_10189cb8;
          fStack_2c = fStack_28 * DAT_10189cbc;
          fStack_28 = fStack_28 * DAT_10189cc0;
          FUN_10002610(&fStack_78,&fStack_30);
          fStack_1c = (float)DAT_10189a3c;
          fStack_24 = fStack_1c * _DAT_10189d18;
          fStack_20 = fStack_1c * _DAT_10189d1c;
          fStack_1c = fStack_1c * _DAT_10189d20;
          FUN_10002610(&fStack_54,&fStack_24);
          fStack_10 = (float)_DAT_10189b40;
          fStack_18 = fStack_10 * _DAT_10189e50;
          fStack_14 = fStack_10 * _DAT_10189e54;
          fStack_10 = fStack_10 * _DAT_10189e58;
          FUN_10002610(&fStack_3c,&fStack_18);
          fStack_60 = fStack_3c + _DAT_10189ea0;
          fStack_5c = _DAT_10189ea4 + fStack_38;
          fStack_58 = _DAT_10189ea8 + fStack_34;
          FUN_10002610(&fStack_48,&fStack_60);
          fStack_84 = fStack_54 + fStack_48;
          fStack_80 = fStack_50 + fStack_44;
          fStack_7c = fStack_4c + fStack_40;
          FUN_10002610(&fStack_a8,&fStack_84);
          fStack_b4 = fStack_78 + fStack_a8;
          fStack_b0 = fStack_74 + fStack_a4;
          fStack_ac = fStack_70 + fStack_a0;
          FUN_10002610(afStack_6c,&fStack_b4);
          fStack_94 = (float)*(int *)((int)param_2 + 0x34c);
          fStack_9c = fStack_94 * DAT_10189cb8;
          fStack_98 = fStack_94 * DAT_10189cbc;
          fStack_94 = fStack_94 * DAT_10189cc0;
          FUN_10002610(&fStack_c0,&fStack_9c);
          fStack_d0 = *(float *)(DAT_101914a0 + 0xb60) - fStack_c0;
          fStack_cc = *(float *)(DAT_101914a0 + 0xb64) - fStack_bc;
          fStack_c8 = *(float *)(DAT_101914a0 + 0xb68) - fStack_b8;
          FUN_10002610(afStack_90,&fStack_d0);
          FUN_10056890(afStack_90,(float *)&DAT_1018a000,afStack_6c,&DAT_10189ea0,DAT_10189d5c,1);
          iStack_c4 = DAT_10189a3c + 0x32;
          fStack_c8 = (float)iStack_c4;
          fStack_d0 = fStack_c8 * DAT_10189cb8;
          fStack_cc = fStack_c8 * DAT_10189cbc;
          fStack_c8 = fStack_c8 * DAT_10189cc0;
          FUN_10002610(&fStack_54,&fStack_d0);
          fStack_b8 = (float)DAT_10189a3c;
          fStack_c0 = fStack_b8 * _DAT_10189d18;
          fStack_bc = fStack_b8 * _DAT_10189d1c;
          fStack_b8 = fStack_b8 * _DAT_10189d20;
          FUN_10002610(&fStack_78,&fStack_c0);
          fStack_94 = (float)_DAT_10189b40;
          fStack_9c = fStack_94 * _DAT_10189e50;
          fStack_98 = fStack_94 * _DAT_10189e54;
          fStack_94 = fStack_94 * _DAT_10189e58;
          FUN_10002610(&fStack_b4,&fStack_9c);
          fStack_a8 = fStack_b4 + _DAT_10189ea0;
          fStack_a4 = _DAT_10189ea4 + fStack_b0;
          fStack_a0 = _DAT_10189ea8 + fStack_ac;
          FUN_10002610(&fStack_84,&fStack_a8);
          fStack_48 = fStack_78 + fStack_84;
          fStack_44 = fStack_74 + fStack_80;
          fStack_40 = fStack_70 + fStack_7c;
          FUN_10002610(&fStack_60,&fStack_48);
          fStack_3c = fStack_54 + fStack_60;
          fStack_38 = fStack_50 + fStack_5c;
          fStack_34 = fStack_4c + fStack_58;
          FUN_10002610(afStack_90,&fStack_3c);
          fStack_10 = (float)*(int *)((int)param_2 + 0x34c);
          fStack_18 = fStack_10 * DAT_10189cb8;
          fStack_14 = fStack_10 * DAT_10189cbc;
          fStack_10 = fStack_10 * DAT_10189cc0;
          FUN_10002610(&fStack_24,&fStack_18);
          fStack_30 = *(float *)(DAT_101914a0 + 0xb78) - fStack_24;
          fStack_2c = *(float *)(DAT_101914a0 + 0xb7c) - fStack_20;
          fStack_28 = *(float *)(DAT_101914a0 + 0xb80) - fStack_1c;
          FUN_10002610(afStack_6c,&fStack_30);
          in_stack_ffffff0c = afStack_6c;
        }
      }
      pfVar19 = (float *)&DAT_1018a000;
      in_stack_ffffff08 = (float *)0x100403d0;
      FUN_10056890(in_stack_ffffff0c,(float *)&DAT_1018a000,afStack_90,&DAT_10189ea0,DAT_10189d5c,1)
      ;
    }
  }
LAB_100403d3:
  bVar2 = CL_IsThirdPerson();
  if (((CONCAT31(extraout_var_00,bVar2) != 0) ||
      (iVar3 = FUN_1001c5e0((int)DAT_10189c14), iVar3 == 0)) &&
     (cVar1 = FUN_100579a0((int)DAT_10189c14), cVar1 != '\0')) {
    fVar22 = 1.4013e-45;
    fVar21 = param_2;
    if (param_3 == '\0') {
      fVar7 = (float)(_DAT_1018a0b4 & 0xffffff01);
      iVar3 = (_DAT_1018a0b4 & 1) << 3;
      fVar13 = 1.26117e-44;
      FUN_10002610(&stack0xffffff08,&DAT_10189cb8);
      pfVar4 = afStack_90;
      uVar17 = 0x41200000;
      uVar15 = 0x10040503;
      puVar6 = (undefined4 *)FUN_1003efb0(&DAT_10189ea0,pfVar4,10.0);
      FUN_10002610(&stack0xfffffef8,puVar6);
      uVar8 = CONCAT31((int3)((uint)-DAT_10189d3c >> 8),-(DAT_10189d3c != 0)) & 0xfffffffe;
      fVar14 = (float)CONCAT31((int3)(uVar8 >> 8),(char)uVar8 + '\x02');
    }
    else {
      fVar14 = (float)(_DAT_1018a0b4 & 0xffffff01);
      iVar3 = (_DAT_1018a0b4 & 1) << 3;
      fVar20 = 1.26117e-44;
      FUN_10002610(&stack0xffffff08,&DAT_10189cb8);
      pfVar4 = afStack_90;
      uVar17 = 0x41200000;
      uVar15 = 0x1004044f;
      fVar7 = param_2;
      puVar6 = (undefined4 *)FUN_1003efb0(&DAT_10189ea0,pfVar4,10.0);
      FUN_10002610(&stack0xfffffef8,puVar6);
      fVar13 = 0.0;
      pfVar9 = afStack_6c;
      pfVar16 = DAT_10189c14;
      puVar6 = (undefined4 *)FUN_10056be0(pfVar9,(int)DAT_10189c14,0);
      FUN_10002610(&stack0xfffffeec,puVar6);
      FUN_100474b0((float)pfVar9,(float)pfVar16,fVar13,uVar15,pfVar4,uVar17,fVar7,
                   (float)in_stack_ffffff08,(float)in_stack_ffffff0c,(float)pfVar19,fVar20,fVar14,
                   iVar3,fVar22);
      fVar7 = (float)(_DAT_1018a0b4 & 0xffffff01);
      fVar22 = 1.4013e-45;
      iVar3 = (_DAT_1018a0b4 & 1) << 3;
      fVar13 = 1.26117e-44;
      FUN_10002610(&stack0xffffff08,&DAT_10189cb8);
      pfVar4 = afStack_90;
      uVar17 = 0x41200000;
      uVar15 = 0x100404b7;
      puVar6 = (undefined4 *)FUN_1003efb0(&DAT_10189ea0,pfVar4,10.0);
      FUN_10002610(&stack0xfffffef8,puVar6);
      fVar14 = 2.8026e-45;
    }
    pfVar16 = afStack_6c;
    pfVar9 = DAT_10189c14;
    puVar6 = (undefined4 *)FUN_10056be0(pfVar16,(int)DAT_10189c14,(uint)fVar14);
    FUN_10002610(&stack0xfffffeec,puVar6);
    FUN_100474b0((float)pfVar16,(float)pfVar9,fVar14,uVar15,pfVar4,uVar17,fVar21,
                 (float)in_stack_ffffff08,(float)in_stack_ffffff0c,(float)pfVar19,fVar13,fVar7,iVar3
                 ,fVar22);
    iVar3 = 0;
    fVar21 = 24.0;
    if ((_DAT_1018a0b4 & 1) == 0) {
      FUN_10002610(&stack0xffffff10,&DAT_10189cb8);
      pfVar4 = afStack_90;
      fVar14 = 10.0;
      fVar22 = 2.6036617e-29;
      puVar6 = (undefined4 *)FUN_1003efb0(&DAT_10189ea0,pfVar4,10.0);
      FUN_10002610(&stack0xffffff04,puVar6);
      pfVar16 = afStack_6c;
      uVar8 = CONCAT31((int3)((uint)-DAT_10189d3c >> 8),-(DAT_10189d3c != 0)) & 0xfffffffe;
      pfVar11 = (float *)CONCAT31((int3)(uVar8 >> 8),(char)uVar8 + '\x02');
      pfVar9 = DAT_10189c14;
      puVar6 = (undefined4 *)FUN_10056be0(pfVar16,(int)DAT_10189c14,(uint)pfVar11);
      FUN_10002610(&stack0xfffffef8,puVar6);
      iVar10 = (*DAT_100d5c38)();
      sVar12 = 0x10;
    }
    else {
      FUN_10002610(&stack0xffffff10,&DAT_10189cb8);
      pfVar4 = afStack_90;
      fVar20 = 10.0;
      fVar14 = 2.603695e-29;
      puVar6 = (undefined4 *)FUN_1003efb0(&DAT_10189ea0,pfVar4,10.0);
      FUN_10002610(&stack0xffffff04,puVar6);
      pfVar9 = afStack_6c;
      uVar8 = CONCAT31((int3)((uint)-DAT_10189d3c >> 8),-(DAT_10189d3c != 0)) & 0xfffffffe;
      fVar22 = (float)CONCAT31((int3)(uVar8 >> 8),(char)uVar8 + '\x02');
      pfVar16 = DAT_10189c14;
      puVar6 = (undefined4 *)FUN_10056be0(pfVar9,(int)DAT_10189c14,(uint)fVar22);
      FUN_10002610(&stack0xfffffef8,puVar6);
      iVar10 = (*DAT_100d5c38)();
      FUN_10047130(8,iVar10,(float)pfVar9,(float)pfVar16,fVar22,fVar14,(float)pfVar4,fVar20,
                   (float)pfVar19,fVar13,fVar7,fVar21,iVar3);
      iVar3 = 1;
      fVar21 = 4.0;
      FUN_10002610(&stack0xffffff10,(undefined4 *)&DAT_10189e50);
      pfVar4 = afStack_90;
      fVar14 = 10.0;
      fVar22 = 2.6037306e-29;
      puVar6 = (undefined4 *)FUN_1003efb0(&DAT_10189ea0,pfVar4,10.0);
      FUN_10002610(&stack0xffffff04,puVar6);
      pfVar11 = (float *)FUN_1003d240(&DAT_10189cb8,afStack_6c,(float)*(int *)((int)param_2 + 0x34c)
                                     );
      pfVar9 = &fStack_d0;
      uVar8 = CONCAT31((int3)((uint)-DAT_10189d3c >> 8),-(DAT_10189d3c != 0)) & 0xfffffffe;
      pvVar5 = (void *)FUN_10056be0(&fStack_c0,(int)DAT_10189c14,
                                    CONCAT31((int3)(uVar8 >> 8),(char)uVar8 + '\x02'));
      pfVar16 = (float *)0x100406b1;
      puVar6 = (undefined4 *)FUN_1003d200(pvVar5,pfVar9,pfVar11);
      FUN_10002610(&stack0xfffffef8,puVar6);
      iVar10 = (*DAT_100d5c38)();
      sVar12 = 1;
    }
    FUN_10047130(sVar12,iVar10,(float)pfVar16,(float)pfVar9,(float)pfVar11,fVar22,(float)pfVar4,
                 fVar14,(float)pfVar19,fVar13,fVar7,fVar21,iVar3);
    if (DAT_10189d5c != 0) {
      iStack_c4 = DAT_10189a3c + 0x32;
      fStack_c8 = (float)iStack_c4;
      fStack_d0 = fStack_c8 * DAT_10189cb8;
      fStack_cc = fStack_c8 * DAT_10189cbc;
      fStack_c8 = fStack_c8 * DAT_10189cc0;
      FUN_10002610(&fStack_54,&fStack_d0);
      fStack_b8 = (float)DAT_10189a3c;
      fStack_c0 = fStack_b8 * _DAT_10189d18;
      fStack_bc = fStack_b8 * _DAT_10189d1c;
      fStack_b8 = fStack_b8 * _DAT_10189d20;
      FUN_10002610(&fStack_78,&fStack_c0);
      fStack_94 = (float)_DAT_10189b40;
      fStack_9c = fStack_94 * _DAT_10189e50;
      fStack_98 = fStack_94 * _DAT_10189e54;
      fStack_94 = fStack_94 * _DAT_10189e58;
      FUN_10002610(&fStack_b4,&fStack_9c);
      fStack_a8 = _DAT_10189ea0 + fStack_b4;
      fStack_a4 = _DAT_10189ea4 + fStack_b0;
      fStack_a0 = _DAT_10189ea8 + fStack_ac;
      FUN_10002610(&fStack_84,&fStack_a8);
      fStack_48 = fStack_78 + fStack_84;
      fStack_44 = fStack_74 + fStack_80;
      fStack_40 = fStack_70 + fStack_7c;
      FUN_10002610(&fStack_60,&fStack_48);
      fStack_3c = fStack_54 + fStack_60;
      fStack_38 = fStack_50 + fStack_5c;
      fStack_34 = fStack_4c + fStack_58;
      FUN_10002610(afStack_90,&fStack_3c);
      fStack_10 = (float)*(int *)((int)param_2 + 0x34c);
      fStack_18 = DAT_10189cb8 * fStack_10;
      fStack_14 = DAT_10189cbc * fStack_10;
      fStack_10 = DAT_10189cc0 * fStack_10;
      FUN_10002610(&fStack_24,&fStack_18);
      uVar8 = CONCAT31((int3)((uint)-DAT_10189d3c >> 8),-(DAT_10189d3c != 0)) & 0xfffffffe;
      pfVar19 = (float *)FUN_10056be0(auStack_c,(int)DAT_10189c14,
                                      CONCAT31((int3)(uVar8 >> 8),(char)uVar8 + '\x02'));
      fStack_30 = *pfVar19 - fStack_24;
      fStack_2c = pfVar19[1] - fStack_20;
      fStack_28 = pfVar19[2] - fStack_1c;
      FUN_10002610(afStack_6c,&fStack_30);
      FUN_10056890(afStack_6c,(float *)&DAT_1018a000,afStack_90,&DAT_10189ea0,DAT_10189d5c,1);
    }
  }
  __ftol();
  puVar6 = (undefined4 *)FUN_1001fb70(auStack_c,100,100,100);
  FUN_10002610(&stack0xffffff08,puVar6);
  FUN_1003de90(DAT_10189c14);
  FUN_1001c620(param_1,(float *)&DAT_10189d78,(float *)&DAT_10189ed0);
  _DAT_10189cd0 = DAT_10189cc0;
  _DAT_10189cc8 = DAT_10189cb8;
  _DAT_10189ccc = DAT_10189cbc;
  if (param_3 != '\0') {
    fStack_d0 = _DAT_10189e50 * _DAT_1007a8d0 + _DAT_10189d78;
    fStack_cc = _DAT_10189d7c + _DAT_10189e54 * _DAT_1007a8d0;
    fStack_c8 = _DAT_10189d80 + _DAT_10189e58 * _DAT_1007a8d0;
    fStack_bc = fStack_cc;
    fStack_b8 = fStack_c8;
    FUN_100450c0((int)DAT_10189c14,&DAT_10189cb8,(float *)&DAT_10189e50,(float *)&DAT_10189d18,
                 *(byte *)((int)param_2 + 0x24),&fStack_d0,(float *)&DAT_10189cc8,
                 *(float *)(param_1 + 0x30),*(float *)(param_1 + 0x34),
                 *(float *)((int)param_2 + 0x1c),*(int *)((int)param_2 + 0x20),
                 (uint)*(byte *)((int)param_2 + 8),
                 CONCAT31((int3)((uint)fStack_cc >> 8),*(undefined1 *)((int)param_2 + 0x2c)),
                 *(int *)(param_1 + 0x44));
    fStack_d0 = _DAT_10189e50 * _DAT_1007d6f8 + _DAT_10189d78;
    fStack_cc = _DAT_10189d7c + _DAT_10189e54 * _DAT_1007d6f8;
    fStack_c8 = _DAT_10189d80 + _DAT_10189e58 * _DAT_1007d6f8;
    fStack_bc = fStack_cc;
    fStack_b8 = fStack_c8;
    FUN_100450c0((int)DAT_10189c14,&DAT_10189cb8,(float *)&DAT_10189e50,(float *)&DAT_10189d18,
                 *(byte *)((int)param_2 + 0x24),&fStack_d0,(float *)&DAT_10189cc8,
                 *(float *)(param_1 + 0x30),*(float *)(param_1 + 0x34),
                 *(float *)((int)param_2 + 0x1c),*(int *)((int)param_2 + 0x20),
                 (uint)*(byte *)((int)param_2 + 8),
                 CONCAT31((int3)((uint)fStack_cc >> 8),*(undefined1 *)((int)param_2 + 0x2c)),
                 *(int *)(param_1 + 0x44) + 0x21);
    return;
  }
  FUN_100450c0((int)DAT_10189c14,&DAT_10189cb8,(float *)&DAT_10189e50,(float *)&DAT_10189d18,
               *(byte *)((int)param_2 + 0x24),(float *)&DAT_10189d78,(float *)&DAT_10189cc8,
               *(float *)(param_1 + 0x30),*(float *)(param_1 + 0x34),*(float *)((int)param_2 + 0x1c)
               ,*(int *)((int)param_2 + 0x20),(uint)*(byte *)((int)param_2 + 8),
               CONCAT31((int3)((uint)DAT_10189cbc >> 8),*(undefined1 *)((int)param_2 + 0x2c)),
               *(int *)(param_1 + 0x44));
  return;
}


// ===== FUN @ 10047130 size=896 =====

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void __cdecl
FUN_10047130(short param_1,int param_2,float param_3,float param_4,float param_5,float param_6,
            float param_7,float param_8,float param_9,float param_10,float param_11,float param_12,
            int param_13)

{
  float *pfVar1;
  float fVar2;
  float fVar3;
  undefined4 uVar4;
  int iVar5;
  undefined4 *puVar6;
  float10 fVar7;
  longlong lVar8;
  longlong lVar9;
  float fStack_30;
  float fStack_2c;
  float fStack_28;
  float fStack_24;
  undefined4 uStack_20;
  undefined4 uStack_1c;
  float fStack_18;
  float fStack_14;
  undefined4 local_c;
  float fStack_8;
  float fStack_4;
  
  if ((char)param_13 == '\0') {
    uVar4 = FUN_10050ad0(&DAT_1018bde8,1);
    if ((char)uVar4 == '\0') {
      return;
    }
    fVar7 = FUN_10056fa0(0.1);
    FUN_10050b00(&DAT_1018bde8,1,(float)fVar7);
  }
  if ((*(float *)(DAT_100df464 + 0xc) != _DAT_100799f0) &&
     (uVar4 = FUN_100570b0(&param_3), (char)uVar4 != '\0')) {
    uVar4 = FUN_10057270(&local_c);
    iVar5 = FUN_100572a0(&param_3,uVar4);
    if (((char)iVar5 != '\0') &&
       (iVar5 = FUN_100509d0(&DAT_1018bde8,(byte *)s_sprites_gun_smoke_add_spr_100953f0,5),
       iVar5 != -1)) {
      puVar6 = FUN_1004ffe0();
      puVar6[0x3f] = (float)param_2;
      puVar6[0x34] = 0x3c23d70a;
      *(undefined1 *)((int)puVar6 + 7) = 0;
      *(undefined1 *)((int)puVar6 + 6) = 0;
      *(undefined1 *)((int)puVar6 + 5) = 0;
      puVar6[0x42] = (float)(int)param_1;
      *(undefined1 *)puVar6 = 1;
      puVar6[0x38] = 0;
      puVar6[0x37] = 0x3e99999a;
      *(short *)(puVar6 + 0x45) = (short)iVar5;
      fVar2 = (float)(int)param_1 * (float)_DAT_10079e90;
      puVar6[0x36] = 0;
      puVar6[0x3e] = 0;
      param_13 = 0;
      *(undefined1 *)(puVar6 + 1) = 0;
      puVar6[0x3d] = 0x40533333;
      puVar6[5] = param_3 + param_9 * fVar2;
      puVar6[6] = param_4 + param_10 * fVar2;
      puVar6[7] = param_5 + fVar2 * param_11;
      lVar8 = __ftol();
      lVar9 = __ftol();
      pfVar1 = (float *)(puVar6 + 0xe);
      _param_1 = 5;
      do {
        iVar5 = (*DAT_100d5c38)((int)lVar9,(int)lVar8);
        puVar6[0x3f] = (float)(iVar5 + param_2);
        fVar2 = param_12 / (float)_param_1;
        fStack_30 = param_6 + fVar2 * param_9;
        fStack_2c = param_7 + param_10 * fVar2;
        fStack_28 = param_8 + fVar2 * param_11;
        FUN_10002610(&fStack_24,&fStack_30);
        fVar2 = (float)puVar6[0x42] * _DAT_1007d720;
        *pfVar1 = fStack_24;
        puVar6[0xf] = uStack_20;
        puVar6[0x10] = uStack_1c;
        fVar7 = (float10)(*DAT_100d5c34)(-fVar2,fVar2);
        *pfVar1 = (float)(fVar7 + (float10)*pfVar1);
        fVar7 = (float10)(*DAT_100d5c34)(-fVar2,fVar2);
        puVar6[0xf] = (float)(fVar7 + (float10)(float)puVar6[0xf]);
        puVar6[0x39] = 0x3dcccccd;
        iVar5 = FUN_10050b60(&DAT_1018bde8,puVar6,0,0);
        if (iVar5 == -1) {
          return;
        }
        fVar2 = (float)puVar6[0x42];
        fVar3 = (float)_DAT_1007d570;
        puVar6[0x42] = fVar2 * fVar3;
        fVar2 = fVar2 * fVar3 * (float)param_13;
        fStack_18 = fVar2 * param_9;
        fStack_14 = param_10 * fVar2;
        fStack_8 = param_4 + fStack_14;
        fStack_4 = param_5 + fVar2 * param_11;
        puVar6[5] = fStack_18 + param_3;
        puVar6[6] = fStack_8;
        puVar6[7] = fStack_4;
        _param_1 = _param_1 + -1;
        param_13 = param_13 + 1;
      } while (0 < _param_1);
    }
  }
  return;
}


// ===== FUN @ 100579a0 size=23 =====

undefined1 __cdecl FUN_100579a0(int param_1)

{
  if ((-1 < param_1) && (param_1 < 0x21)) {
    return *(undefined1 *)((int)&DAT_1018bd44 + param_1);
  }
  return 0;
}


// ===== FUN @ 10056890 size=264 =====

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void __cdecl
FUN_10056890(undefined4 param_1,float *param_2,undefined4 param_3,undefined4 param_4,
            undefined4 param_5,uint param_6)

{
  float fVar1;
  uint *puVar2;
  float10 fVar3;
  float local_c;
  float local_8;
  float local_4;
  
  local_c = *param_2;
  local_4 = param_2[2];
  local_8 = param_2[1] - _DAT_1007d450;
  if (*(int *)(&DAT_1018bbc8 + DAT_1018bcc8 * 4) != 0) {
    *(undefined4 *)(*(int *)(&DAT_1018bbc8 + DAT_1018bcc8 * 4) + 4) = 0;
  }
  puVar2 = (uint *)(**(code **)(DAT_100d5c74 + 0xc0))
                             (param_1,param_3,&local_c,DAT_1018ca54,param_5,0);
  if (puVar2 != (uint *)0x0) {
    puVar2[0xfe] = param_6;
    *puVar2 = *puVar2 | 4;
    puVar2[0xd6] = 0;
    puVar2[0x101] = 0xc;
    fVar3 = (float10)(*DAT_100d5c34)(0xc2700000,0x42700000);
    local_c = (float)(fVar3 * fVar3);
    fVar3 = (float10)(*DAT_100d5c34)(0xc2700000,0x42700000);
    fVar1 = (float)_DAT_10079e90;
    puVar2[0x1a] = (uint)local_c;
    puVar2[0x1b] = (uint)(float)(fVar3 * fVar3);
    puVar2[0x1c] = (uint)((float)(fVar3 * fVar3) * fVar1);
    DAT_1018bcc8 = DAT_1018bcc8 + 1;
    if ((0x3f < DAT_1018bcc8) || (DAT_1018bcc8 < 0)) {
      DAT_1018bcc8 = 0;
    }
  }
  return;
}


// ===== FUN @ 10056be0 size=52 =====

void __cdecl FUN_10056be0(undefined4 *param_1,int param_2,uint param_3)

{
  int iVar1;
  
  iVar1 = (param_3 & 0xff) + param_2 * 4;
  *param_1 = (&DAT_1018caf8)[iVar1 * 3];
  param_1[1] = (&DAT_1018cafc)[iVar1 * 3];
  param_1[2] = (&DAT_1018cb00)[iVar1 * 3];
  return;
}


