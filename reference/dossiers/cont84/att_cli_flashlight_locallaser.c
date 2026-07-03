// ===== FUN @ 1003bb00 size=5758 =====

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void FUN_1003bb00(void)

{
  int iVar1;
  float fVar2;
  int *piVar3;
  undefined4 *puVar4;
  float *pfVar5;
  int iVar6;
  int iVar7;
  undefined4 uVar8;
  float10 fVar9;
  longlong lVar10;
  undefined1 *puVar11;
  undefined4 uVar12;
  undefined *puVar13;
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
  undefined4 auStack_84 [3];
  float afStack_78 [3];
  float afStack_6c [3];
  undefined1 auStack_60 [12];
  undefined1 auStack_54 [12];
  int iStack_48;
  int iStack_44;
  int iStack_40;
  undefined4 auStack_3c [3];
  undefined1 auStack_30 [12];
  undefined1 auStack_24 [12];
  undefined4 auStack_18 [3];
  undefined4 auStack_c [3];
  
  if ((DAT_101897d8 & 1) == 0) {
    DAT_101897d8 = DAT_101897d8 | 1;
    FUN_1006af63(0x1003d1f0);
  }
  if ((DAT_101897d8 & 2) == 0) {
    DAT_101897d8 = DAT_101897d8 | 2;
    FUN_1006af63(0x1003d1e0);
  }
  if ((DAT_101897d8 & 4) == 0) {
    DAT_101897d8 = DAT_101897d8 | 4;
    FUN_1006af63(0x1003d1d0);
  }
  if ((DAT_101897d8 & 8) == 0) {
    DAT_101897d8 = DAT_101897d8 | 8;
    FUN_1006af63(0x1003d1c0);
  }
  if ((DAT_101897d8 & 0x10) == 0) {
    DAT_101897d8 = DAT_101897d8 | 0x10;
    FUN_1006af63(0x1003d1b0);
  }
  if ((DAT_101897d8 & 0x20) == 0) {
    DAT_101897d8 = DAT_101897d8 | 0x20;
    FUN_100025e0(&DAT_10189800,0xc,4,&LAB_10017e90);
    FUN_1006af63(0x1003d1a0);
  }
  if ((DAT_101897d8 & 0x40) == 0) {
    DAT_101897d8 = DAT_101897d8 | 0x40;
    FUN_1006af63(0x1003d190);
  }
  if ((DAT_101897d8 & 0x80) == 0) {
    DAT_101897d8 = DAT_101897d8 | 0x80;
    FUN_1006af63(0x1003d180);
  }
  if ((DAT_100d0cb4 & 2) == 0) {
    return;
  }
  piVar3 = (int *)(*DAT_100d5bf4)();
  iVar1 = *piVar3;
  (**(code **)(DAT_100d5c78 + 0x2c))();
  (**(code **)(DAT_100d5c78 + 0x28))(0,1);
  (**(code **)(DAT_100d5c78 + 0x34))(iVar1 + -1);
  fVar9 = (float10)(*DAT_100d5c34)(0xbf800000,0x3f800000);
  _DAT_101897e8 = (float)fVar9;
  fVar9 = (float10)(*DAT_100d5c34)(0xbf800000,0x3f800000);
  _DAT_1018997c = (float)fVar9;
  if ((DAT_101914a0 == 0) || (*(int *)(DAT_100df5b0 + 0xc) == 0x3f800000)) {
    _DAT_101897c8 = DAT_1018bd90;
    _DAT_101897cc = DAT_1018bd94;
    _DAT_101897d0 = DAT_1018bd98;
  }
  else {
    puVar4 = (undefined4 *)
             FUN_10009190(auStack_60,
                          *(float *)(DAT_101914a0 + 0xb6c) - *(float *)(DAT_101914a0 + 0xb60),
                          *(float *)(DAT_101914a0 + 0xb70) - *(float *)(DAT_101914a0 + 0xb64),
                          *(float *)(DAT_101914a0 + 0xb74) - *(float *)(DAT_101914a0 + 0xb68));
    FUN_10002610(&fStack_9c,puVar4);
    fVar9 = FUN_1003d5c0(&fStack_9c);
    if (fVar9 == (float10)_DAT_100799f0) {
      puVar4 = (undefined4 *)FUN_1001fb70(auStack_54,0,0,1);
    }
    else {
      fVar9 = (float10)_DAT_10079e58 / fVar9;
      puVar4 = (undefined4 *)
               FUN_10009190(&fStack_b4,(float)((float10)fStack_9c * fVar9),
                            (float)((float10)fStack_98 * fVar9),(float)((float10)fStack_94 * fVar9))
      ;
    }
    FUN_10002610(&fStack_90,puVar4);
    _DAT_101897c8 = _DAT_101897e8 * (float)_DAT_1007d6e8 + fStack_90;
    _DAT_101897d0 = fStack_88;
    _DAT_101897cc = _DAT_1018997c * (float)_DAT_1007d6e8 + fStack_8c;
  }
  if ((DAT_100d0cb4 & 0x40) == 0) {
    (**(code **)(DAT_100d5c78 + 0x38))(2);
    puVar4 = (undefined4 *)
             FUN_10009190(auStack_84,_DAT_101897c8 * _DAT_1007d448,_DAT_101897cc * _DAT_1007d448,
                          _DAT_101897d0 * _DAT_1007d448);
    FUN_10002610(&fStack_b4,puVar4);
    pfVar5 = (float *)FUN_10057270(afStack_78);
    puVar4 = (undefined4 *)
             FUN_10009190(afStack_6c,fStack_b4 + *pfVar5,fStack_b0 + pfVar5[1],fStack_ac + pfVar5[2]
                         );
    FUN_10002610(auStack_60,puVar4);
    puVar13 = &DAT_101898c8;
    puVar11 = auStack_60;
    uVar12 = 2;
    puVar4 = (undefined4 *)(DAT_100d5c78 + 0x3c);
    iVar6 = iVar1;
    uVar8 = FUN_10057270(auStack_3c);
    (*(code *)*puVar4)(uVar8,puVar11,uVar12,iVar6,puVar13);
    pfVar5 = (float *)FUN_10057270(auStack_84);
    puVar4 = (undefined4 *)
             FUN_10009190(afStack_78,_DAT_101898dc - *pfVar5,_DAT_101898e0 - pfVar5[1],
                          _DAT_101898e4 - pfVar5[2]);
    FUN_10002610(&fStack_90,puVar4);
    DAT_101897dc = SQRT(fStack_8c * fStack_8c + fStack_88 * fStack_88 + fStack_90 * fStack_90);
    DAT_1018987c = 0.0;
  }
  else {
    if ((DAT_101914a0 == 0) || (*(int *)(DAT_100df5b0 + 0xc) == 0x3f800000)) {
      _DAT_10189970 = DAT_1018bd90;
      _DAT_10189974 = DAT_1018bd94;
      _DAT_10189978 = DAT_1018bd98;
    }
    else {
      puVar4 = (undefined4 *)
               FUN_10009190(auStack_60,
                            *(float *)(DAT_101914a0 + 0xb84) - *(float *)(DAT_101914a0 + 0xb78),
                            *(float *)(DAT_101914a0 + 0xb88) - *(float *)(DAT_101914a0 + 0xb7c),
                            *(float *)(DAT_101914a0 + 0xb8c) - *(float *)(DAT_101914a0 + 0xb80));
      FUN_10002610(&fStack_9c,puVar4);
      fVar9 = FUN_1003d5c0(&fStack_9c);
      if (fVar9 == (float10)_DAT_100799f0) {
        puVar4 = (undefined4 *)FUN_1001fb70(auStack_54,0,0,1);
      }
      else {
        fVar9 = (float10)_DAT_10079e58 / fVar9;
        puVar4 = (undefined4 *)
                 FUN_10009190(&fStack_b4,(float)((float10)fStack_9c * fVar9),
                              (float)((float10)fStack_98 * fVar9),
                              (float)((float10)fStack_94 * fVar9));
      }
      FUN_10002610(&fStack_90,puVar4);
      _DAT_10189970 = _DAT_1018997c * (float)_DAT_1007d6e8 + fStack_90;
      _DAT_10189978 = fStack_88;
      _DAT_10189974 = _DAT_101897e8 * (float)_DAT_1007d6e8 + fStack_8c;
    }
    (**(code **)(DAT_100d5c78 + 0x38))(2);
    puVar4 = (undefined4 *)
             FUN_10009190(auStack_24,_DAT_1018bd9c * _DAT_1007d454,_DAT_1018bda0 * _DAT_1007d454,
                          _DAT_1018bda4 * _DAT_1007d454);
    FUN_10002610(&fStack_90,puVar4);
    puVar4 = (undefined4 *)
             FUN_10009190(auStack_c,_DAT_101897c8 * _DAT_1007d448,_DAT_101897cc * _DAT_1007d448,
                          _DAT_101897d0 * _DAT_1007d448);
    FUN_10002610(&fStack_9c,puVar4);
    pfVar5 = (float *)FUN_10057270(auStack_18);
    puVar4 = (undefined4 *)
             FUN_10009190(auStack_30,fStack_9c + *pfVar5,fStack_98 + pfVar5[1],fStack_94 + pfVar5[2]
                         );
    FUN_10002610(&fStack_a8,puVar4);
    puVar4 = (undefined4 *)
             FUN_10009190(auStack_3c,fStack_90 + fStack_a8,fStack_8c + fStack_a4,
                          fStack_88 + fStack_a0);
    FUN_10002610(auStack_54,puVar4);
    puVar4 = (undefined4 *)
             FUN_10009190(afStack_6c,_DAT_1018bd9c * _DAT_1007d454,_DAT_1018bda0 * _DAT_1007d454,
                          _DAT_1018bda4 * _DAT_1007d454);
    FUN_10002610(&fStack_b4,puVar4);
    pfVar5 = (float *)FUN_10057270(afStack_78);
    puVar4 = (undefined4 *)
             FUN_10009190(auStack_84,fStack_b4 + *pfVar5,fStack_b0 + pfVar5[1],fStack_ac + pfVar5[2]
                         );
    FUN_10002610(auStack_60,puVar4);
    (**(code **)(DAT_100d5c78 + 0x3c))(auStack_60,auStack_54,2,iVar1,&DAT_101898c8);
    puVar4 = (undefined4 *)
             FUN_10009190(auStack_84,_DAT_1018bd9c * _DAT_1007d6e4,_DAT_1018bda0 * _DAT_1007d6e4,
                          _DAT_1018bda4 * _DAT_1007d6e4);
    FUN_10002610(&fStack_a8,puVar4);
    puVar4 = (undefined4 *)
             FUN_10009190(afStack_78,_DAT_10189970 * _DAT_1007d448,_DAT_10189974 * _DAT_1007d448,
                          _DAT_10189978 * _DAT_1007d448);
    FUN_10002610(&fStack_b4,puVar4);
    pfVar5 = (float *)FUN_10057270(afStack_6c);
    puVar4 = (undefined4 *)
             FUN_10009190(auStack_3c,fStack_b4 + *pfVar5,fStack_b0 + pfVar5[1],fStack_ac + pfVar5[2]
                         );
    FUN_10002610(&fStack_9c,puVar4);
    puVar4 = (undefined4 *)
             FUN_10009190(auStack_30,fStack_a8 + fStack_9c,fStack_a4 + fStack_98,
                          fStack_a0 + fStack_94);
    FUN_10002610(auStack_60,puVar4);
    puVar4 = (undefined4 *)
             FUN_10009190(auStack_18,_DAT_1018bd9c * _DAT_1007d6e4,_DAT_1018bda0 * _DAT_1007d6e4,
                          _DAT_1018bda4 * _DAT_1007d6e4);
    FUN_10002610(&fStack_90,puVar4);
    pfVar5 = (float *)FUN_10057270(auStack_c);
    puVar4 = (undefined4 *)
             FUN_10009190(auStack_24,fStack_90 + *pfVar5,fStack_8c + pfVar5[1],fStack_88 + pfVar5[2]
                         );
    FUN_10002610(auStack_54,puVar4);
    (**(code **)(DAT_100d5c78 + 0x3c))(auStack_54,auStack_60,2,iVar1,&DAT_10189880);
    pfVar5 = (float *)FUN_10057270(auStack_84);
    puVar4 = (undefined4 *)
             FUN_10009190(afStack_78,_DAT_101898dc - *pfVar5,_DAT_101898e0 - pfVar5[1],
                          _DAT_101898e4 - pfVar5[2]);
    FUN_10002610(&fStack_90,puVar4);
    DAT_101897dc = SQRT(fStack_90 * fStack_90 + fStack_8c * fStack_8c + fStack_88 * fStack_88);
    pfVar5 = (float *)FUN_10057270(auStack_84);
    puVar4 = (undefined4 *)
             FUN_10009190(afStack_78,_DAT_10189894 - *pfVar5,_DAT_10189898 - pfVar5[1],
                          _DAT_1018989c - pfVar5[2]);
    FUN_10002610(&fStack_90,puVar4);
    DAT_1018987c = SQRT(fStack_8c * fStack_8c + fStack_88 * fStack_88 + fStack_90 * fStack_90);
  }
  (**(code **)(DAT_100d5c78 + 0x30))();
  puVar4 = (undefined4 *)
           FUN_10009190(auStack_84,DAT_1018bd90 + DAT_1018bd90,DAT_1018bd94 + DAT_1018bd94,
                        DAT_1018bd98 + DAT_1018bd98);
  FUN_10002610(&fStack_b4,puVar4);
  puVar4 = (undefined4 *)
           FUN_10009190(afStack_78,_DAT_101898dc - fStack_b4,_DAT_101898e0 - fStack_b0,
                        _DAT_101898e4 - fStack_ac);
  FUN_10002610(&fStack_a8,puVar4);
  _DAT_101899a0 = fStack_a8;
  _DAT_101899a4 = fStack_a4;
  _DAT_101899a8 = fStack_a0;
  FUN_100579c0(&DAT_101899a0,(undefined4 *)&DAT_101895d4,(undefined4 *)&DAT_10189878);
  _DAT_100d0cbc = (float)_DAT_101895d4;
  _DAT_100d0cc4 = DAT_101897dc;
  _DAT_100d0cc0 = (float)_DAT_10189878;
  if ((DAT_100d0cb4 & 0x40) != 0) {
    puVar4 = (undefined4 *)
             FUN_10009190(auStack_84,DAT_1018bd90 + DAT_1018bd90,DAT_1018bd94 + DAT_1018bd94,
                          DAT_1018bd98 + DAT_1018bd98);
    FUN_10002610(&fStack_b4,puVar4);
    puVar4 = (undefined4 *)
             FUN_10009190(afStack_78,_DAT_10189894 - fStack_b4,_DAT_10189898 - fStack_b0,
                          _DAT_1018989c - fStack_ac);
    FUN_10002610(&fStack_a8,puVar4);
    _DAT_101899a0 = fStack_a8;
    _DAT_101899a4 = fStack_a4;
    _DAT_101899a8 = fStack_a0;
    FUN_100579c0(&DAT_101899a0,(undefined4 *)&DAT_101895d4,(undefined4 *)&DAT_10189878);
    _DAT_100d0cc8 = (float)_DAT_101895d4;
    _DAT_100d0cd0 = DAT_1018987c;
    _DAT_100d0ccc = (float)_DAT_10189878;
  }
  if (DAT_101914a0 == 0) goto LAB_1003d15f;
  iVar6 = FUN_100509d0(&DAT_1018bde8,(byte *)s_sprites_laserbeamanim_spr_10095354,5);
  if (iVar6 == -1) {
    DAT_101899b0 = iVar6;
    return;
  }
  _DAT_101899b4 = DAT_100963e8 * DAT_10191478 * _DAT_1007a530 + _DAT_101899b4;
  DAT_101899b0 = iVar6;
  lVar10 = __ftol();
  if ((int)lVar10 < 0) {
LAB_1003c672:
    _DAT_101899b4 = 0.0;
  }
  else {
    iVar7 = FUN_10050980(&DAT_1018bde8,iVar6);
    lVar10 = __ftol();
    iVar6 = DAT_101899b0;
    if (iVar7 + -1 < (int)lVar10) goto LAB_1003c672;
  }
  puVar4 = (undefined4 *)(DAT_100d5c70 + 0x2c);
  lVar10 = __ftol();
  uVar8 = (*DAT_100d5c20)(*(undefined4 *)(&DAT_1018be14 + iVar6 * 0x28),(int)lVar10);
  iVar6 = (*(code *)*puVar4)(uVar8);
  if (iVar6 == 0) {
    return;
  }
  fStack_b4 = _DAT_1018bda8;
  _DAT_10189860 = *(float *)(DAT_101914a0 + 0xb60);
  _DAT_10189864 = *(float *)(DAT_101914a0 + 0xb64);
  fStack_b0 = _DAT_1018bdac;
  _DAT_10189868 = *(float *)(DAT_101914a0 + 0xb68);
  fStack_ac = _DAT_1018bdb0;
  FUN_10002610(&fStack_a8,&fStack_b4);
  fStack_9c = _DAT_10189860 - fStack_a8;
  fStack_98 = _DAT_10189864 - fStack_a4;
  fStack_94 = _DAT_10189868 - fStack_a0;
  FUN_10002610(&fStack_90,&fStack_9c);
  _DAT_10189860 = fStack_90;
  _DAT_10189864 = fStack_8c;
  _DAT_10189868 = fStack_88;
  fVar2 = _DAT_1007a524 - DAT_101897dc;
  if (_DAT_1007a524 - DAT_101897dc < _DAT_100799f0) {
    fVar2 = _DAT_100799f0;
  }
  fVar2 = _DAT_10079e58 - fVar2 * _DAT_1007d6e0;
  (**(code **)(DAT_100d5c70 + 4))(5);
  (**(code **)(DAT_100d5c70 + 0x28))(1);
  (**(code **)(DAT_100d5c70 + 8))(2);
  FUN_100582f0((int)*(char *)((int)&DAT_1018d128 + iVar1 * 2 + 1),&iStack_48,&iStack_44,&iStack_40);
  (**(code **)(DAT_100d5c70 + 0x10))
            ((float)iStack_48 * fVar2 * _DAT_1007cd70,(float)iStack_44 * fVar2 * _DAT_1007cd70,
             (float)iStack_40 * fVar2 * _DAT_1007cd70,0x3f800000);
  puVar4 = (undefined4 *)
           FUN_10009190(auStack_84,_DAT_101897c8 + _DAT_101897c8,_DAT_101897cc + _DAT_101897cc,
                        _DAT_101897d0 + _DAT_101897d0);
  FUN_10002610(&fStack_b4,puVar4);
  puVar4 = (undefined4 *)
           FUN_10009190(afStack_78,_DAT_101898dc - fStack_b4,_DAT_101898e0 - fStack_b0,
                        _DAT_101898e4 - fStack_ac);
  FUN_10002610(&fStack_a8,puVar4);
  _DAT_101899a0 = fStack_a8;
  _DAT_101899a4 = fStack_a4;
  _DAT_101899a8 = fStack_a0;
  puVar4 = (undefined4 *)
           FUN_10009190(auStack_84,_DAT_1018bda8 * _DAT_1007a870,_DAT_1018bdac * _DAT_1007a870,
                        _DAT_1018bdb0 * _DAT_1007a870);
  FUN_10002610(&fStack_b4,puVar4);
  puVar4 = (undefined4 *)
           FUN_10009190(afStack_78,fStack_b4 * fVar2,fStack_b0 * fVar2,fStack_ac * fVar2);
  FUN_10002610(&fStack_a8,puVar4);
  puVar4 = (undefined4 *)
           FUN_10009190(afStack_6c,_DAT_101899a0 - fStack_a8,_DAT_101899a4 - fStack_a4,
                        _DAT_101899a8 - fStack_a0);
  FUN_10002610(&fStack_9c,puVar4);
  _DAT_10189990 = fStack_9c;
  _DAT_10189994 = fStack_98;
  _DAT_10189998 = fStack_94;
  (**(code **)(DAT_100d5c70 + 0x18))(0,0);
  (**(code **)(DAT_100d5c70 + 0x1c))(&DAT_10189990);
  puVar4 = (undefined4 *)
           FUN_10009190(auStack_84,_DAT_1018bda8 * _DAT_1007a870,_DAT_1018bdac * _DAT_1007a870,
                        _DAT_1018bdb0 * _DAT_1007a870);
  FUN_10002610(&fStack_b4,puVar4);
  puVar4 = (undefined4 *)
           FUN_10009190(afStack_78,fStack_b4 * fVar2,fStack_b0 * fVar2,fStack_ac * fVar2);
  FUN_10002610(&fStack_a8,puVar4);
  puVar4 = (undefined4 *)
           FUN_10009190(afStack_6c,fStack_a8 + _DAT_101899a0,_DAT_101899a4 + fStack_a4,
                        _DAT_101899a8 + fStack_a0);
  FUN_10002610(&fStack_9c,puVar4);
  _DAT_10189990 = fStack_9c;
  _DAT_10189994 = fStack_98;
  _DAT_10189998 = fStack_94;
  (**(code **)(DAT_100d5c70 + 0x18))(0x3f800000,0);
  (**(code **)(DAT_100d5c70 + 0x1c))(&DAT_10189990);
  puVar4 = (undefined4 *)
           FUN_10009190(auStack_84,_DAT_1018bda8 * _DAT_1007a870,_DAT_1018bdac * _DAT_1007a870,
                        _DAT_1018bdb0 * _DAT_1007a870);
  FUN_10002610(&fStack_b4,puVar4);
  puVar4 = (undefined4 *)
           FUN_10009190(afStack_78,fStack_b4 * fVar2,fStack_b0 * fVar2,fStack_ac * fVar2);
  FUN_10002610(&fStack_a8,puVar4);
  puVar4 = (undefined4 *)
           FUN_10009190(afStack_6c,fStack_a8 + _DAT_10189860,_DAT_10189864 + fStack_a4,
                        _DAT_10189868 + fStack_a0);
  FUN_10002610(&fStack_9c,puVar4);
  _DAT_10189990 = fStack_9c;
  _DAT_10189994 = fStack_98;
  _DAT_10189998 = fStack_94;
  (**(code **)(DAT_100d5c70 + 0x18))(0x3f800000,0x3f800000);
  (**(code **)(DAT_100d5c70 + 0x1c))(&DAT_10189990);
  puVar4 = (undefined4 *)
           FUN_10009190(auStack_84,_DAT_1018bda8 * _DAT_1007a870,_DAT_1018bdac * _DAT_1007a870,
                        _DAT_1018bdb0 * _DAT_1007a870);
  FUN_10002610(&fStack_b4,puVar4);
  puVar4 = (undefined4 *)
           FUN_10009190(afStack_78,fStack_b4 * fVar2,fStack_b0 * fVar2,fStack_ac * fVar2);
  FUN_10002610(&fStack_a8,puVar4);
  puVar4 = (undefined4 *)
           FUN_10009190(afStack_6c,_DAT_10189860 - fStack_a8,_DAT_10189864 - fStack_a4,
                        _DAT_10189868 - fStack_a0);
  FUN_10002610(&fStack_9c,puVar4);
  _DAT_10189990 = fStack_9c;
  _DAT_10189994 = fStack_98;
  _DAT_10189998 = fStack_94;
  (**(code **)(DAT_100d5c70 + 0x18))(0,0x3f800000);
  (**(code **)(DAT_100d5c70 + 0x1c))(&DAT_10189990);
  if ((DAT_100d0cb4 & 0x40) != 0) {
    fStack_b4 = _DAT_1018bda8;
    _DAT_10189860 = *(float *)(DAT_101914a0 + 0xb78);
    _DAT_10189864 = *(float *)(DAT_101914a0 + 0xb7c);
    fStack_b0 = _DAT_1018bdac;
    _DAT_10189868 = *(float *)(DAT_101914a0 + 0xb80);
    fStack_ac = _DAT_1018bdb0;
    FUN_10002610(&fStack_a8,&fStack_b4);
    fStack_9c = _DAT_10189860 - fStack_a8;
    fStack_98 = _DAT_10189864 - fStack_a4;
    fStack_94 = _DAT_10189868 - fStack_a0;
    FUN_10002610(&fStack_90,&fStack_9c);
    _DAT_10189864 = fStack_8c;
    _DAT_10189860 = fStack_90;
    _DAT_10189868 = fStack_88;
    puVar4 = (undefined4 *)
             FUN_10009190(auStack_84,_DAT_10189970 + _DAT_10189970,_DAT_10189974 + _DAT_10189974,
                          _DAT_10189978 + _DAT_10189978);
    FUN_10002610(&fStack_b4,puVar4);
    puVar4 = (undefined4 *)
             FUN_10009190(afStack_78,_DAT_10189894 - fStack_b4,_DAT_10189898 - fStack_b0,
                          _DAT_1018989c - fStack_ac);
    FUN_10002610(&fStack_a8,puVar4);
    _DAT_101899a0 = fStack_a8;
    _DAT_101899a4 = fStack_a4;
    _DAT_101899a8 = fStack_a0;
    puVar4 = (undefined4 *)
             FUN_10009190(auStack_84,_DAT_1018bda8 * _DAT_1007a870,_DAT_1018bdac * _DAT_1007a870,
                          _DAT_1018bdb0 * _DAT_1007a870);
    FUN_10002610(&fStack_b4,puVar4);
    puVar4 = (undefined4 *)
             FUN_10009190(afStack_78,fStack_b4 * fVar2,fStack_b0 * fVar2,fStack_ac * fVar2);
    FUN_10002610(&fStack_a8,puVar4);
    puVar4 = (undefined4 *)
             FUN_10009190(afStack_6c,_DAT_101899a0 - fStack_a8,_DAT_101899a4 - fStack_a4,
                          _DAT_101899a8 - fStack_a0);
    FUN_10002610(&fStack_9c,puVar4);
    _DAT_10189990 = fStack_9c;
    _DAT_10189994 = fStack_98;
    _DAT_10189998 = fStack_94;
    (**(code **)(DAT_100d5c70 + 0x18))(0,0);
    (**(code **)(DAT_100d5c70 + 0x1c))(&DAT_10189990);
    puVar4 = (undefined4 *)
             FUN_10009190(auStack_84,_DAT_1018bda8 * _DAT_1007a870,_DAT_1018bdac * _DAT_1007a870,
                          _DAT_1018bdb0 * _DAT_1007a870);
    FUN_10002610(&fStack_b4,puVar4);
    puVar4 = (undefined4 *)
             FUN_10009190(afStack_78,fStack_b4 * fVar2,fStack_b0 * fVar2,fStack_ac * fVar2);
    FUN_10002610(&fStack_a8,puVar4);
    puVar4 = (undefined4 *)
             FUN_10009190(afStack_6c,fStack_a8 + _DAT_101899a0,_DAT_101899a4 + fStack_a4,
                          _DAT_101899a8 + fStack_a0);
    FUN_10002610(&fStack_9c,puVar4);
    _DAT_10189990 = fStack_9c;
    _DAT_10189994 = fStack_98;
    _DAT_10189998 = fStack_94;
    (**(code **)(DAT_100d5c70 + 0x18))(0x3f800000,0);
    (**(code **)(DAT_100d5c70 + 0x1c))(&DAT_10189990);
    puVar4 = (undefined4 *)
             FUN_10009190(auStack_84,_DAT_1018bda8 * _DAT_1007a870,_DAT_1018bdac * _DAT_1007a870,
                          _DAT_1018bdb0 * _DAT_1007a870);
    FUN_10002610(&fStack_b4,puVar4);
    puVar4 = (undefined4 *)
             FUN_10009190(afStack_78,fStack_b4 * fVar2,fStack_b0 * fVar2,fStack_ac * fVar2);
    FUN_10002610(&fStack_a8,puVar4);
    puVar4 = (undefined4 *)
             FUN_10009190(afStack_6c,fStack_a8 + _DAT_10189860,_DAT_10189864 + fStack_a4,
                          _DAT_10189868 + fStack_a0);
    FUN_10002610(&fStack_9c,puVar4);
    _DAT_10189990 = fStack_9c;
    _DAT_10189994 = fStack_98;
    _DAT_10189998 = fStack_94;
    (**(code **)(DAT_100d5c70 + 0x18))(0x3f800000,0x3f800000);
    (**(code **)(DAT_100d5c70 + 0x1c))(&DAT_10189990);
    puVar4 = (undefined4 *)
             FUN_10009190(auStack_84,_DAT_1018bda8 * _DAT_1007a870,_DAT_1018bdac * _DAT_1007a870,
                          _DAT_1018bdb0 * _DAT_1007a870);
    FUN_10002610(auStack_60,puVar4);
    pfVar5 = (float *)FUN_1003d240(auStack_60,afStack_78,fVar2);
    pfVar5 = (float *)FUN_1003d200(&DAT_10189860,afStack_6c,pfVar5);
    _DAT_10189990 = *pfVar5;
    _DAT_10189994 = pfVar5[1];
    _DAT_10189998 = pfVar5[2];
    (**(code **)(DAT_100d5c70 + 0x18))(0,0x3f800000);
    (**(code **)(DAT_100d5c70 + 0x1c))(&DAT_10189990);
  }
  (**(code **)(DAT_100d5c70 + 0xc))();
  (**(code **)(DAT_100d5c70 + 4))(0);
LAB_1003d15f:
  lVar10 = __ftol();
  _DAT_100d0cb8 = (int)lVar10;
  return;
}


