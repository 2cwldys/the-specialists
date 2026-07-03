// ===== FUN @ 100419a0 size=2713 =====

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void FUN_100419a0(int param_1)

{
  bool bVar1;
  char cVar2;
  int iVar3;
  undefined3 extraout_var;
  undefined4 *puVar4;
  float *pfVar5;
  undefined3 extraout_var_00;
  float *pfVar6;
  void *this;
  float fVar7;
  float *pfVar8;
  float fVar9;
  float *pfVar10;
  float fVar11;
  undefined *puVar12;
  undefined *puVar13;
  float fVar14;
  int iVar15;
  float fStack_cc;
  float fStack_c8;
  float fStack_c4;
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
  float fStack_90;
  float fStack_8c;
  float fStack_88;
  float fStack_84;
  float fStack_80;
  float fStack_7c;
  float fStack_78;
  float fStack_74;
  float fStack_70;
  float fStack_6c;
  float fStack_68;
  float fStack_64;
  float fStack_60;
  float fStack_5c;
  float fStack_58;
  float fStack_54;
  float fStack_50;
  float fStack_4c;
  float afStack_48 [3];
  float afStack_3c [3];
  float fStack_30;
  float fStack_2c;
  float fStack_28;
  float fStack_24;
  float fStack_20;
  float fStack_1c;
  float fStack_18;
  float fStack_14;
  float fStack_10;
  float afStack_c [3];
  
  if ((DAT_10189f8c & 1) == 0) {
    DAT_10189f8c = DAT_10189f8c | 1;
    FUN_1006af63(0x100424d0);
  }
  if ((DAT_10189f8c & 2) == 0) {
    DAT_10189f8c = DAT_10189f8c | 2;
    FUN_1006af63(0x100424c0);
  }
  if ((DAT_10189f8c & 4) == 0) {
    DAT_10189f8c = DAT_10189f8c | 4;
    FUN_1006af63(0x100424b0);
  }
  if ((DAT_10189f8c & 8) == 0) {
    DAT_10189f8c = DAT_10189f8c | 8;
    FUN_1006af63(0x100424a0);
  }
  if ((DAT_10189f8c & 0x10) == 0) {
    DAT_10189f8c = DAT_10189f8c | 0x10;
    FUN_1006af63(0x10042490);
  }
  if ((DAT_10189f8c & 0x20) == 0) {
    DAT_10189f8c = DAT_10189f8c | 0x20;
    FUN_1006af63(0x10042480);
  }
  if ((DAT_10189f8c & 0x40) == 0) {
    DAT_10189f8c = DAT_10189f8c | 0x40;
    FUN_1006af63(0x10042470);
  }
  if ((DAT_10189f8c & 0x80) == 0) {
    DAT_10189f8c = DAT_10189f8c | 0x80;
    FUN_1006af63(0x10042460);
  }
  if ((DAT_1018a09c & 1) == 0) {
    DAT_1018a09c = DAT_1018a09c | 1;
    FUN_1006af63(0x10042450);
  }
  if ((DAT_1018a09c & 2) == 0) {
    DAT_1018a09c = DAT_1018a09c | 2;
    FUN_1006af63(0x10042440);
  }
  DAT_10189d74 = *(int *)(param_1 + 4);
  _DAT_10189be8 = *(undefined4 *)(param_1 + 8);
  _DAT_10189bec = *(undefined4 *)(param_1 + 0xc);
  _DAT_10189bf0 = *(undefined4 *)(param_1 + 0x10);
  _DAT_10189bf8 = *(undefined4 *)(param_1 + 0x14);
  _DAT_10189bfc = *(undefined4 *)(param_1 + 0x18);
  _DAT_10189c00 = *(undefined4 *)(param_1 + 0x1c);
  _DAT_10189f90 = *(float *)(param_1 + 0x20);
  _DAT_10189f94 = *(float *)(param_1 + 0x24);
  _DAT_10189f98 = *(float *)(param_1 + 0x28);
  (*DAT_100d5b88)();
  DAT_10189ae0 = *(int *)(param_1 + 0x38);
  DAT_1018a054 = *(uint *)(param_1 + 0x3c);
  DAT_10189c54 = FUN_100071c0(*(int *)(param_1 + 0x38));
  if (*(char *)(DAT_10189c54 + 0x3a) == '\0') {
    DAT_10189d60 = 0;
  }
  else {
    DAT_10189d60 = (**(code **)(DAT_100d5c78 + 0xc))();
  }
  _DAT_10189d04 = (*DAT_100d5c38)();
  _DAT_10189d64 = (*DAT_100d5c38)();
  iVar3 = FUN_1001c5e0(DAT_10189d74);
  if (iVar3 != 0) {
    DAT_10189f4c = FUN_1003df60((int)DAT_10189c54);
    _DAT_1018a0a4 = FUN_10007110(DAT_10189ae0,DAT_1018a054);
    puVar4 = (undefined4 *)(DAT_100d5c78 + 0x40);
    (*DAT_100d5c38)();
    (*(code *)*puVar4)();
    bVar1 = CL_IsThirdPerson();
    if (CONCAT31(extraout_var,bVar1) == 0) {
      if (DAT_101914a0 == 0) {
        if (DAT_10189d60 != 0) {
          fStack_c4 = (float)_DAT_10189d04;
          fStack_cc = fStack_c4 * _DAT_10189fc8;
          fStack_c8 = fStack_c4 * _DAT_10189fcc;
          fStack_c4 = fStack_c4 * _DAT_10189fd0;
          FUN_10002610(&fStack_60,&fStack_cc);
          fStack_4c = (float)_DAT_10189d64;
          fStack_54 = fStack_4c * _DAT_10189d68;
          fStack_50 = _DAT_10189d6c * fStack_4c;
          fStack_4c = _DAT_10189d70 * fStack_4c;
          FUN_10002610(&fStack_c0,&fStack_54);
          fStack_64 = (float)_DAT_10189d04;
          fStack_6c = fStack_64 * _DAT_10189e10;
          fStack_68 = _DAT_10189e14 * fStack_64;
          fStack_64 = _DAT_10189e18 * fStack_64;
          FUN_10002610(&fStack_84,&fStack_6c);
          fStack_b4 = fStack_84 + _DAT_10189f90;
          fStack_b0 = _DAT_10189f94 + fStack_80;
          fStack_ac = _DAT_10189f98 + fStack_7c;
          FUN_10002610(&fStack_a8,&fStack_b4);
          fStack_78 = fStack_c0 + fStack_a8;
          fStack_74 = fStack_bc + fStack_a4;
          fStack_70 = fStack_b8 + fStack_a0;
          FUN_10002610(&fStack_90,&fStack_78);
          fStack_9c = fStack_60 + fStack_90;
          fStack_98 = fStack_5c + fStack_8c;
          fStack_94 = fStack_58 + fStack_88;
          FUN_10002610(afStack_48,&fStack_9c);
          FUN_10056890(&DAT_10190b28,(float *)&DAT_10189bf8,afStack_48,&DAT_10189f90,DAT_10189d60,2)
          ;
        }
      }
      else if (DAT_10189d60 != 0) {
        fStack_10 = (float)_DAT_10189d04;
        fStack_18 = fStack_10 * _DAT_10189fc8;
        fStack_14 = fStack_10 * _DAT_10189fcc;
        fStack_10 = fStack_10 * _DAT_10189fd0;
        FUN_10002610(&fStack_c0,&fStack_18);
        fStack_1c = (float)_DAT_10189d64;
        fStack_24 = fStack_1c * _DAT_10189d68;
        fStack_20 = _DAT_10189d6c * fStack_1c;
        fStack_1c = _DAT_10189d70 * fStack_1c;
        FUN_10002610(&fStack_60,&fStack_24);
        fStack_28 = (float)_DAT_10189d04;
        fStack_30 = fStack_28 * _DAT_10189e10;
        fStack_2c = _DAT_10189e14 * fStack_28;
        fStack_28 = _DAT_10189e18 * fStack_28;
        FUN_10002610(&fStack_9c,&fStack_30);
        fStack_90 = fStack_9c + _DAT_10189f90;
        fStack_8c = _DAT_10189f94 + fStack_98;
        fStack_88 = _DAT_10189f98 + fStack_94;
        FUN_10002610(&fStack_78,&fStack_90);
        fStack_a8 = fStack_60 + fStack_78;
        fStack_a4 = fStack_5c + fStack_74;
        fStack_a0 = fStack_58 + fStack_70;
        FUN_10002610(&fStack_b4,&fStack_a8);
        fStack_84 = fStack_c0 + fStack_b4;
        fStack_80 = fStack_bc + fStack_b0;
        fStack_7c = fStack_b8 + fStack_ac;
        FUN_10002610(afStack_3c,&fStack_84);
        fStack_64 = (float)(int)DAT_10189c54[0xd3];
        fStack_6c = fStack_64 * _DAT_10189fc8;
        fStack_68 = fStack_64 * _DAT_10189fcc;
        fStack_64 = fStack_64 * _DAT_10189fd0;
        FUN_10002610(&fStack_54,&fStack_6c);
        fStack_cc = *(float *)(DAT_101914a0 + 0xb60) - fStack_54;
        fStack_c8 = *(float *)(DAT_101914a0 + 0xb64) - fStack_50;
        fStack_c4 = *(float *)(DAT_101914a0 + 0xb68) - fStack_4c;
        FUN_10002610(afStack_48,&fStack_cc);
        pfVar10 = afStack_3c;
        puVar13 = &DAT_10189f90;
        puVar12 = &DAT_10189bf8;
        FUN_10056890(afStack_48,(float *)&DAT_10189bf8,pfVar10,&DAT_10189f90,DAT_10189d60,2);
        iVar15 = 1;
        fVar14 = 4.0;
        FUN_10002610(&stack0xffffff14,(undefined4 *)&DAT_10189e10);
        pfVar8 = afStack_48;
        fVar11 = 10.0;
        fVar9 = 2.605612e-29;
        puVar4 = (undefined4 *)FUN_1003efb0(&DAT_10189f90,pfVar8,10.0);
        FUN_10002610(&stack0xffffff08,puVar4);
        pfVar5 = (float *)FUN_1003d240(&DAT_10189fc8,afStack_3c,(float)(int)DAT_10189c54[0xd3]);
        pfVar6 = &fStack_cc;
        fVar7 = 2.6056325e-29;
        puVar4 = (undefined4 *)FUN_1003d200((void *)(DAT_101914a0 + 0xb60),pfVar6,pfVar5);
        FUN_10002610(&stack0xfffffefc,puVar4);
        iVar3 = (*DAT_100d5c38)(0x14,0x28);
        FUN_10047130(1,iVar3,fVar7,(float)pfVar6,(float)pfVar5,fVar9,(float)pfVar8,fVar11,
                     (float)puVar12,(float)pfVar10,(float)puVar13,fVar14,iVar15);
      }
    }
  }
  bVar1 = CL_IsThirdPerson();
  if (CONCAT31(extraout_var_00,bVar1) == 0) {
    iVar3 = FUN_1001c5e0(DAT_10189d74);
    if (iVar3 != 0) goto LAB_100423c6;
  }
  cVar2 = FUN_100579a0(DAT_10189d74);
  if ((cVar2 != '\0') && (DAT_10189d60 != 0)) {
    fStack_c4 = (float)_DAT_10189d04;
    fStack_cc = fStack_c4 * _DAT_10189fc8;
    fStack_c8 = fStack_c4 * _DAT_10189fcc;
    fStack_c4 = fStack_c4 * _DAT_10189fd0;
    FUN_10002610(&fStack_78,&fStack_cc);
    fStack_4c = (float)_DAT_10189d64;
    fStack_54 = fStack_4c * _DAT_10189d68;
    fStack_50 = _DAT_10189d6c * fStack_4c;
    fStack_4c = _DAT_10189d70 * fStack_4c;
    FUN_10002610(&fStack_c0,&fStack_54);
    fStack_64 = (float)_DAT_10189d04;
    fStack_6c = fStack_64 * _DAT_10189e10;
    fStack_68 = _DAT_10189e14 * fStack_64;
    fStack_64 = _DAT_10189e18 * fStack_64;
    FUN_10002610(&fStack_84,&fStack_6c);
    fStack_b4 = fStack_84 + _DAT_10189f90;
    fStack_b0 = _DAT_10189f94 + fStack_80;
    fStack_ac = _DAT_10189f98 + fStack_7c;
    FUN_10002610(&fStack_a8,&fStack_b4);
    puVar4 = (undefined4 *)
             FUN_10009190(&fStack_30,fStack_c0 + fStack_a8,fStack_bc + fStack_a4,
                          fStack_b8 + fStack_a0);
    FUN_10002610(&fStack_60,puVar4);
    puVar4 = (undefined4 *)
             FUN_10009190(&fStack_24,fStack_78 + fStack_60,fStack_74 + fStack_5c,
                          fStack_70 + fStack_58);
    FUN_10002610(afStack_48,puVar4);
    fStack_88 = (float)(int)DAT_10189c54[0xd3];
    fStack_90 = fStack_88 * _DAT_10189fc8;
    fStack_8c = fStack_88 * _DAT_10189fcc;
    fStack_88 = fStack_88 * _DAT_10189fd0;
    FUN_10002610(&fStack_9c,&fStack_90);
    pfVar6 = (float *)FUN_10056be0(&fStack_18,DAT_10189d74,3);
    puVar4 = (undefined4 *)
             FUN_10009190(afStack_c,*pfVar6 - fStack_9c,pfVar6[1] - fStack_98,pfVar6[2] - fStack_94)
    ;
    FUN_10002610(afStack_3c,puVar4);
    pfVar6 = afStack_48;
    puVar13 = &DAT_10189f90;
    puVar12 = &DAT_10189bf8;
    FUN_10056890(afStack_3c,(float *)&DAT_10189bf8,pfVar6,&DAT_10189f90,DAT_10189d60,2);
    iVar15 = 1;
    fVar14 = 4.0;
    FUN_10002610(&stack0xffffff14,(undefined4 *)&DAT_10189e10);
    pfVar10 = afStack_c;
    fVar11 = 10.0;
    fVar9 = 2.6059614e-29;
    puVar4 = (undefined4 *)FUN_1003efb0(&DAT_10189f90,pfVar10,10.0);
    FUN_10002610(&stack0xffffff08,puVar4);
    pfVar5 = (float *)FUN_1003d240(&DAT_10189fc8,afStack_48,(float)(int)DAT_10189c54[0xd3]);
    pfVar8 = afStack_3c;
    this = (void *)FUN_10056be0(&fStack_cc,DAT_10189d74,3);
    fVar7 = 2.6059863e-29;
    puVar4 = (undefined4 *)FUN_1003d200(this,pfVar8,pfVar5);
    FUN_10002610(&stack0xfffffefc,puVar4);
    iVar3 = (*DAT_100d5c38)(0x14,0x28);
    FUN_10047130(1,iVar3,fVar7,(float)pfVar8,(float)pfVar5,fVar9,(float)pfVar10,fVar11,
                 (float)puVar12,(float)pfVar6,(float)puVar13,fVar14,iVar15);
  }
LAB_100423c6:
  (*DAT_100d5c38)();
  __ftol();
  puVar4 = (undefined4 *)(DAT_100d5c78 + 4);
  (*DAT_100d5c34)();
  (*(code *)*puVar4)();
  return;
}


