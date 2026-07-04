// ===== FUN @ 10067890 size=4924 =====

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void __cdecl FUN_10067890(float *param_1)

{
  float *pfVar1;
  float fVar2;
  float fVar3;
  double dVar4;
  bool bVar5;
  float fVar6;
  uint uVar7;
  int iVar8;
  undefined3 extraout_var;
  undefined4 *puVar9;
  undefined3 extraout_var_00;
  undefined3 extraout_var_01;
  int iVar10;
  uint uVar11;
  uint uVar12;
  undefined4 *puVar13;
  float10 fVar14;
  longlong lVar15;
  uint uStack_6c;
  float fStack_68;
  float fStack_64;
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
  float afStack_30 [4];
  float fStack_20;
  float fStack_1c;
  float afStack_18 [6];
  
  DAT_10191d20 = param_1[0x16];
  DAT_10191d24 = param_1[0x17];
  DAT_10191d28 = param_1[0x18];
  FUN_10067320((int)param_1);
  fVar6 = (float)(*DAT_100d5bf4)();
  uVar7 = FUN_1004c250();
  if ((char)uVar7 == '\0') {
    fVar2 = _DAT_1007cae0;
    if ((DAT_1018a60c != '\0') && (lVar15 = __ftol(), fVar2 = _DAT_1007cae0, (int)lVar15 != 0)) {
      fVar2 = _DAT_1007a910;
    }
    DAT_10097210 = fVar2;
    uStack_6c = (uint)(DAT_10097214 < DAT_10097210);
    dVar4 = (double)uStack_6c;
    uStack_6c = 5;
    if (dVar4 * _DAT_100cf950 == _DAT_1007a878) {
      uStack_6c = 0x32;
    }
    fVar14 = FUN_10057e90(DAT_10097210,DAT_10097214,(float)uStack_6c);
    DAT_10097214 = (float)fVar14;
    *(undefined4 *)((int)fVar6 + 0x2f8) = 1;
    lVar15 = __ftol();
    *(int *)((int)fVar6 + 0x2fc) = (int)lVar15;
  }
  else {
    *(undefined4 *)((int)fVar6 + 0x2f8) = 0;
    *(undefined4 *)((int)fVar6 + 0x2fc) = 0xff;
  }
  uVar7 = (*DAT_100d5bf8)();
  fVar14 = FUN_10066e70((float)param_1);
  fStack_68 = (float)fVar14;
  if (DAT_101914f0 != 0) {
    *param_1 = DAT_101913e4;
    param_1[1] = DAT_101913e8;
    param_1[2] = DAT_101913ec;
    param_1[0x19] = DAT_101913e4;
    param_1[0x1a] = DAT_101913e8;
    param_1[0x1b] = DAT_101913ec;
  }
  fVar3 = _DAT_10079e58;
  fVar2 = param_1[0xf] * _DAT_1007a890;
  *param_1 = param_1[0x19];
  param_1[1] = param_1[0x1a];
  param_1[2] = param_1[0x1b];
  if (fVar3 < fVar2) {
    fVar2 = _DAT_10079e58;
  }
  _DAT_10191d90 = (param_1[0x1e] - _DAT_10191d90) * fVar2 + _DAT_10191d90;
  param_1[2] = _DAT_10191d90 + param_1[2];
  FUN_100677d0((int)param_1);
  if (DAT_1019149c != 0) {
    param_1[0x20] = DAT_10190b6c;
    param_1[0x21] = DAT_10190b70;
    param_1[0x22] = DAT_10190b74;
  }
  fStack_48 = param_1[0x20] - _DAT_10190b84;
  fStack_44 = param_1[0x21] - _DAT_10190b88;
  fStack_40 = param_1[0x22] - _DAT_10190b8c;
  fVar14 = FUN_10058390(&fStack_48);
  if (fVar14 != (float10)_DAT_1007a878) {
    *(float *)(&DAT_10191908 + (DAT_10191d0c & 0x3f) * 0xc) = param_1[0x20];
    *(float *)(&DAT_1019190c + (DAT_10191d0c & 0x3f) * 0xc) = param_1[0x21];
    *(float *)(&DAT_10191910 + (DAT_10191d0c & 0x3f) * 0xc) = param_1[0x22];
    *(float *)(&DAT_10191c08 + (DAT_10191d0c & 0x3f) * 4) = param_1[0x10];
    DAT_10191d0c = DAT_10191d0c + 1;
    _DAT_10190b84 = param_1[0x20];
    _DAT_10190b88 = param_1[0x21];
    _DAT_10190b8c = param_1[0x22];
  }
  if (((DAT_100df430 != 0) && (*(float *)(DAT_100df430 + 0xc) != _DAT_100799f0)) &&
     (((byte)DAT_101914ec & 2) != 0)) {
    if (*(float *)(DAT_100df430 + 0xc) < (float)_DAT_1007a878) {
      (*DAT_100d5bbc)(s_cl_vsmoothing_1009423c,0);
    }
    fVar2 = param_1[0x10] - *(float *)(DAT_100df430 + 0xc);
    iVar10 = 1;
    uVar12 = DAT_10191d0c - 2;
    do {
      if (*(float *)(&DAT_10191c08 + (uVar12 & 0x3f) * 4) <= fVar2) {
        if ((iVar10 < 0x3f) &&
           (uVar11 = uVar12 & 0x3f, *(float *)(&DAT_10191c08 + uVar11 * 4) != (float)_DAT_1007a878))
        {
          uVar12 = uVar12 + 1 & 0x3f;
          if ((float)_DAT_1007a878 <
              *(float *)(&DAT_10191c08 + uVar12 * 4) - *(float *)(&DAT_10191c08 + uVar11 * 4)) {
            fVar2 = (fVar2 - *(float *)(&DAT_10191c08 + uVar11 * 4)) /
                    (*(float *)(&DAT_10191c08 + uVar12 * 4) - *(float *)(&DAT_10191c08 + uVar11 * 4)
                    );
            if ((float)_DAT_10079e80 < fVar2) {
              fVar2 = (float)_DAT_10079e80;
            }
            FUN_10029cf0((float *)(&DAT_10191908 + uVar11 * 0xc),
                         (float *)(&DAT_10191908 + uVar12 * 0xc),param_1 + 0x20,fVar2);
            DAT_10190b6c = param_1[0x20];
            DAT_10190b70 = param_1[0x21];
            DAT_10190b74 = param_1[0x22];
          }
        }
        break;
      }
      iVar10 = iVar10 + 1;
      uVar12 = uVar12 - 1;
    } while (iVar10 < 0x3f);
  }
  FUN_10057ed0((float *)&DAT_10191d10,(float *)&DAT_101914f8,(float *)&DAT_10190b38,param_1[0xf]);
  pfVar1 = param_1 + 0x20;
  *pfVar1 = _DAT_10190b38 + param_1[0x20];
  param_1[0x21] = _DAT_10190b3c + param_1[0x21];
  param_1[0x22] = _DAT_10190b40 + param_1[0x22];
  _DAT_10190b38 = 0.0;
  _DAT_10190b3c = 0.0;
  _DAT_10190b40 = 0.0;
  FUN_10058000(&DAT_1018cac8,pfVar1,param_1[0xf]);
  *pfVar1 = _DAT_10190b78 * _DAT_1007a84c + *pfVar1;
  _DAT_10190b78 = 0.0;
  param_1[3] = *pfVar1;
  param_1[4] = param_1[0x21];
  param_1[5] = param_1[0x22];
  (*DAT_100d5c04)();
  (*DAT_100d5c08)(param_1,param_1 + 3,0x3f800000);
  *param_1 = *param_1 + (float)_DAT_10081e70;
  param_1[1] = param_1[1] + (float)_DAT_10081e70;
  param_1[2] = param_1[2] + (float)_DAT_10081e70;
  fVar2 = _DAT_100799f0;
  if (1 < (int)param_1[0x15]) {
    lVar15 = __ftol();
    uStack_6c = (uint)lVar15;
    if (((param_1[0x30] != 0.0) && (iVar10 = (*DAT_100d5c10)(param_1 + 0x19), -1 < iVar10)) &&
       ((iVar10 < (int)param_1[0x2e] &&
        ((iVar10 = (*DAT_100d5bfc)(iVar10), iVar10 != 0 && (*(int *)(iVar10 + 0xb94) != 0)))))) {
      lVar15 = __ftol();
      uStack_6c = (uint)lVar15;
    }
    fStack_60 = *param_1;
    fStack_5c = param_1[1];
    fVar2 = (float)(int)uStack_6c;
    if (param_1[0x15] == 2.8026e-45) {
      fStack_58 = param_1[2] - fVar2;
      iVar10 = 0;
      if (0 < (int)uStack_6c) {
        do {
          iVar8 = (*DAT_100d5c0c)(&fStack_60,0);
          if (-3 < iVar8) break;
          fStack_58 = fStack_58 + _DAT_10079e58;
          iVar10 = iVar10 + 1;
        } while (iVar10 < (int)uStack_6c);
      }
      fVar2 = fVar2 + fStack_58;
    }
    else {
      fStack_58 = fVar2 + param_1[2];
      iVar10 = 0;
      if (0 < (int)uStack_6c) {
        do {
          iVar8 = (*DAT_100d5c0c)(&fStack_60,0);
          if (iVar8 < -2) break;
          fStack_58 = fStack_58 - _DAT_10079e58;
          iVar10 = iVar10 + 1;
        } while (iVar10 < (int)uStack_6c);
      }
      fVar2 = fStack_58 - fVar2;
    }
    fVar2 = fVar2 - param_1[2];
  }
  param_1[2] = fVar2 + param_1[2];
  FUN_100675b0(fVar6,(int)param_1);
  FUN_10067550((int)param_1);
  fStack_3c = param_1[0x20];
  fStack_34 = param_1[0x22];
  fStack_38 = param_1[0x21];
  (*DAT_100d5b88)(&fStack_3c,param_1 + 6,param_1 + 9,param_1 + 0xc);
  bVar5 = CL_IsThirdPerson();
  if (CONCAT31(extraout_var,bVar5) != 0) {
    if ((DAT_101913f4 & 1) == 0) {
      DAT_101913f4 = DAT_101913f4 | 1;
      FUN_1006af63(0x10068bf0);
    }
    if ((_DAT_1007d810 <= param_1[0x1e]) || (DAT_10191da0 = '\x01', param_1[0x1e] == _DAT_100799f0))
    {
      DAT_10191da0 = '\0';
    }
    _DAT_10191458 = 0.0;
    DAT_10191454 = 0.0;
    DAT_10191450 = 0.0;
    CL_CameraOffset(&DAT_10191450);
    fStack_54 = DAT_10191450;
    fStack_50 = DAT_10191454;
    fStack_4c = 0.0;
    (*DAT_100d5b88)(&fStack_54,afStack_30,afStack_18 + 3,afStack_18);
    lVar15 = __ftol();
    iVar10 = (int)lVar15;
    fVar2 = _DAT_1007d824;
    if (iVar10 == 1) {
      DAT_10191450 = -48.0;
      DAT_10191454 = -16.0;
    }
    else if (iVar10 == 2) {
      DAT_10191450 = -90.0;
      DAT_10191454 = 0.0;
    }
    else if (iVar10 == 3) {
      DAT_10191450 = -*(float *)(DAT_100df54c + 0xc);
      DAT_10191454 = *(float *)(DAT_100df484 + 0xc);
      fVar2 = *(float *)(DAT_100df5a8 + 0xc);
    }
    else {
      DAT_10191450 = -48.0;
      DAT_10191454 = 16.0;
    }
    if ((DAT_10191450 <= _DAT_10081e68) || (DAT_10191450 < _DAT_1007d748)) {
      if (DAT_10191450 <= _DAT_10081e68) {
        DAT_10191450 = -128.0;
      }
    }
    else {
      DAT_10191450 = 24.0;
    }
    if (((DAT_10191454 <= _DAT_10081e64) || (fVar3 = _DAT_1007d748, DAT_10191454 < _DAT_1007d748))
       && (fVar3 = _DAT_10081e64, _DAT_10081e64 < DAT_10191454)) {
      fVar3 = DAT_10191454;
    }
    DAT_10191454 = fVar3;
    if (((fVar2 <= _DAT_10081e64) || (_DAT_10191458 = _DAT_1007d748, fVar2 < _DAT_1007d748)) &&
       (_DAT_10191458 = _DAT_10081e64, _DAT_10081e64 < fVar2)) {
      _DAT_10191458 = fVar2;
    }
    if ((DAT_10191da0 == '\0') || (_DAT_1007a94c <= _DAT_10191458)) {
      fVar2 = -_DAT_10191da4;
    }
    else {
      fVar2 = (_DAT_1007a94c - _DAT_10191458) - _DAT_10191da4;
    }
    _DAT_10191da4 = fVar2 * param_1[0xf] * _DAT_1007a8d0 + _DAT_10191da4;
    if ((int)param_1[0x23] < 1) {
      fVar2 = ((_DAT_1007d528 - DAT_10191450) - _DAT_10191da8) * param_1[0xf];
    }
    else {
      if (((*(uint *)((int)fVar6 + 0x26c) & 0x130c30) == 0) || (DAT_10191450 <= _DAT_10081e68)) {
        fVar2 = -_DAT_10191da8;
      }
      else {
        fVar2 = (_DAT_10081e60 - DAT_10191450) - _DAT_10191da8;
      }
      fVar2 = fVar2 * param_1[0xf];
    }
    _DAT_10191da8 = fVar2 * _DAT_1007a8d0 + _DAT_10191da8;
    DAT_10191450 = DAT_10191450 + _DAT_10191da8;
    if (0 < (int)param_1[0x23]) {
      _DAT_10191458 = _DAT_10191458 + _DAT_10191da4;
    }
    if (DAT_10191450 == _DAT_100799f0) {
      fVar14 = (float10)fsin((float10)fStack_54 * (float10)_DAT_1007d518);
      _DAT_10191d38 = (float)fVar14;
      if (fVar14 < (float10)_DAT_100799f0) {
        _DAT_10191d38 = 0.0;
      }
      *param_1 = *(float *)((int)fVar6 + 0xb48) + param_1[0x1c];
      param_1[1] = *(float *)((int)fVar6 + 0xb4c) + param_1[0x1d];
      param_1[2] = (float)(byte)((-(DAT_10191da0 != '\0') & 7U) + 1) +
                   *(float *)((int)fVar6 + 0xb50) + param_1[0x1e];
      iVar10 = 0;
      do {
        if (DAT_10191da0 != '\0') {
          *(float *)(iVar10 + (int)param_1) =
               ((float)_DAT_10079e80 - _DAT_10191d38) * *(float *)((int)afStack_30 + iVar10) *
               (float)_DAT_1007cce0 + *(float *)(iVar10 + (int)param_1);
        }
        iVar8 = iVar10 + 4;
        *(float *)(iVar10 + (int)param_1) =
             _DAT_10191d38 * *(float *)((int)afStack_30 + iVar10) * _DAT_1007d454 +
             _DAT_10191d38 * *(float *)((int)afStack_18 + iVar10) * _DAT_1007d58c +
             *(float *)(iVar10 + (int)param_1);
        iVar10 = iVar8;
      } while (iVar8 < 0xc);
      FUN_100677d0((int)param_1);
    }
    else {
      iVar10 = 0;
      do {
        iVar8 = iVar10 + 4;
        *(float *)(iVar10 + (int)param_1) =
             DAT_10191450 * *(float *)((int)afStack_30 + iVar10) +
             _DAT_10191458 * *(float *)((int)afStack_18 + iVar10) +
             DAT_10191454 * *(float *)((int)afStack_18 + iVar10 + 0xc) +
             *(float *)(iVar10 + (int)param_1);
        iVar10 = iVar8;
      } while (iVar8 < 0xc);
      if ((DAT_101913f4 & 2) == 0) {
        DAT_101913f4 = DAT_101913f4 | 2;
        FUN_1006af63(0x10068be0);
      }
      if ((DAT_101913f4 & 4) == 0) {
        DAT_101913f4 = DAT_101913f4 | 4;
        FUN_1006af63(0x10068bd0);
      }
      _DAT_10191408 = *(undefined4 *)((int)fVar6 + 0xb48);
      _DAT_1019140c = *(undefined4 *)((int)fVar6 + 0xb4c);
      _DAT_10191410 = *(float *)((int)fVar6 + 0xb50) + _DAT_10191458 + _DAT_1007d810;
      iVar10 = (*DAT_100d5c0c)(&DAT_10191408,0);
      if (iVar10 == -2) {
        _DAT_10191408 = *(undefined4 *)((int)fVar6 + 0xb48);
        _DAT_1019140c = *(undefined4 *)((int)fVar6 + 0xb4c);
        _DAT_10191410 = *(float *)((int)fVar6 + 0xb50);
      }
      puVar9 = (undefined4 *)(*DAT_100d5c14)(&DAT_10191408,param_1,0,2,0xffffffff);
      puVar13 = &DAT_101913a0;
      for (iVar10 = 0x11; iVar10 != 0; iVar10 = iVar10 + -1) {
        *puVar13 = *puVar9;
        puVar9 = puVar9 + 1;
        puVar13 = puVar13 + 1;
      }
      if (_DAT_101913b0 != (float)_DAT_10079e80) {
        iVar10 = 0;
        do {
          iVar8 = iVar10 + 4;
          *(float *)((int)param_1 + iVar10) =
               *(float *)((int)&DAT_101913c0 + iVar10) * _DAT_1007d454 +
               *(float *)((int)&DAT_101913b4 + iVar10);
          iVar10 = iVar8;
        } while (iVar8 < 0xc);
        param_1[2] = param_1[2] + _DAT_10079e58;
      }
    }
    if (fVar6 != 0.0) {
      DAT_10191d9c = _DAT_10191d94 - *(float *)((int)fVar6 + 0xb50);
      _DAT_10191d98 =
           (_DAT_10191d94 - *(float *)((int)fVar6 + 0xb50)) * param_1[0xf] * (float)_DAT_10081e58;
      fVar2 = _DAT_10191d98;
      if (_DAT_10191d98 <= _DAT_100799f0) {
        fVar2 = -_DAT_10191d98;
      }
      fVar3 = DAT_10191d9c;
      if (DAT_10191d9c <= _DAT_100799f0) {
        fVar3 = -DAT_10191d9c;
      }
      if (fVar3 < fVar2) {
        _DAT_10191d98 = DAT_10191d9c;
      }
      _DAT_10191d94 = _DAT_10191d94 - _DAT_10191d98;
      *(float *)((int)fVar6 + 0xb50) = _DAT_10191d94;
    }
  }
  if (uVar7 != 0) {
    *(float *)(uVar7 + 0xb54) = param_1[0x20];
    *(float *)(uVar7 + 0xb58) = param_1[0x21];
    *(float *)(uVar7 + 0xb5c) = param_1[0x22];
    FUN_100674a0((int)param_1,&fStack_68);
    pfVar1 = (float *)(uVar7 + 0xb48);
    *pfVar1 = *param_1;
    *(float *)(uVar7 + 0xb4c) = param_1[1];
    *(float *)(uVar7 + 0xb50) = param_1[2];
    (*DAT_100d5c08)(pfVar1,(float *)(uVar7 + 0xb54),0x3f666666);
    *pfVar1 = DAT_10191428 + *pfVar1;
    *(float *)(uVar7 + 0xb4c) = DAT_1019142c + *(float *)(uVar7 + 0xb4c);
    *(float *)(uVar7 + 0xb50) = _DAT_10191430 + *(float *)(uVar7 + 0xb50);
    fVar2 = (float)(0x50 - DAT_100cf978) * _DAT_1007a898;
    *pfVar1 = fVar2 * param_1[6] + *pfVar1;
    *(float *)(uVar7 + 0xb4c) = fVar2 * param_1[7] + *(float *)(uVar7 + 0xb4c);
    fVar2 = (fVar2 * param_1[8] + *(float *)(uVar7 + 0xb50)) - _DAT_10079e58;
    *(float *)(uVar7 + 0xb50) = fVar2;
    if (param_1[0x27] == 110.0) {
      *(float *)(uVar7 + 0xb50) = fVar2 + _DAT_10079e58;
    }
    else if (param_1[0x27] == 100.0) {
      *(float *)(uVar7 + 0xb50) = fVar2 + _DAT_1007a924;
    }
    else if (param_1[0x27] == 90.0) {
      *(float *)(uVar7 + 0xb50) = fVar2 + _DAT_10079e58;
    }
    else if (param_1[0x27] == 80.0) {
      *(float *)(uVar7 + 0xb50) = fVar2 + (float)_DAT_10079e90;
    }
  }
  fVar2 = param_1[3];
  param_1[3] = param_1[0x28] + fVar2;
  fVar3 = param_1[4];
  param_1[4] = param_1[0x29] + fVar3;
  fStack_64 = param_1[0x2a] + param_1[5];
  param_1[5] = fStack_64;
  param_1[3] = _DAT_10191480 + param_1[0x28] + fVar2;
  param_1[4] = _DAT_10191484 + param_1[0x29] + fVar3;
  param_1[5] = _DAT_10191488 + fStack_64;
  FUN_10069e40(param_1[0xf],(float *)&DAT_10191480);
  fStack_60 = param_1[0x19] - _DAT_10191434;
  fStack_5c = param_1[0x1a] - _DAT_10191438;
  fStack_58 = param_1[0x1b] - _DAT_1019143c;
  fVar14 = FUN_10058390(&fStack_60);
  if (fVar14 != (float10)_DAT_1007a878) {
    *(float *)(&DAT_10191508 + (DAT_10191d08 & 0x3f) * 0xc) = param_1[0x19];
    *(float *)(&DAT_1019150c + (DAT_10191d08 & 0x3f) * 0xc) = param_1[0x1a];
    *(float *)(&DAT_10191510 + (DAT_10191d08 & 0x3f) * 0xc) = param_1[0x1b];
    *(float *)(&DAT_10191808 + (DAT_10191d08 & 0x3f) * 4) = param_1[0x10];
    DAT_10191d08 = DAT_10191d08 + 1;
    _DAT_10191434 = param_1[0x19];
    _DAT_10191438 = param_1[0x1a];
    _DAT_1019143c = param_1[0x1b];
  }
  if (((DAT_100df430 != 0) && (*(float *)(DAT_100df430 + 0xc) != _DAT_100799f0)) &&
     ((((byte)DAT_101914ec & 4) != 0 || ((param_1[0x31] != 0.0 && (1 < (int)param_1[0x2b])))))) {
    if (*(float *)(DAT_100df430 + 0xc) < (float)_DAT_1007a878) {
      (*DAT_100d5bbc)(s_cl_vsmoothing_1009423c,0);
    }
    fVar2 = param_1[0x10] - *(float *)(DAT_100df430 + 0xc);
    iVar10 = 1;
    uVar12 = DAT_10191d08 - 2;
    do {
      if (*(float *)(&DAT_10191808 + (uVar12 & 0x3f) * 4) <= fVar2) {
        if ((iVar10 < 0x3f) &&
           (uVar11 = uVar12 & 0x3f, *(float *)(&DAT_10191808 + uVar11 * 4) != (float)_DAT_1007a878))
        {
          uVar12 = uVar12 + 1 & 0x3f;
          if ((float)_DAT_1007a878 <
              *(float *)(&DAT_10191808 + uVar12 * 4) - *(float *)(&DAT_10191808 + uVar11 * 4)) {
            fVar2 = (fVar2 - *(float *)(&DAT_10191808 + uVar11 * 4)) /
                    (*(float *)(&DAT_10191808 + uVar12 * 4) - *(float *)(&DAT_10191808 + uVar11 * 4)
                    );
            if ((float)_DAT_10079e80 < fVar2) {
              fVar2 = (float)_DAT_10079e80;
            }
            iVar8 = uVar11 * 0xc;
            iVar10 = uVar12 * 0xc;
            fStack_60 = *(float *)(&DAT_10191508 + iVar10) - *(float *)(&DAT_10191508 + iVar8);
            fStack_5c = *(float *)(&DAT_1019150c + iVar10) - *(float *)(&DAT_1019150c + iVar8);
            fStack_58 = *(float *)(&DAT_10191510 + iVar10) - *(float *)(&DAT_10191510 + iVar8);
            FUN_10058410((float *)(&DAT_10191508 + iVar8),fVar2,&fStack_60,afStack_30 + 3);
            fVar14 = FUN_10058390(&fStack_60);
            if (fVar14 < (float10)_DAT_1007a534) {
              afStack_30[3] = afStack_30[3] - param_1[0x19];
              fStack_20 = fStack_20 - param_1[0x1a];
              fStack_1c = fStack_1c - param_1[0x1b];
              param_1[0x19] = afStack_30[3] + param_1[0x19];
              param_1[0x1a] = fStack_20 + param_1[0x1a];
              param_1[0x1b] = fStack_1c + param_1[0x1b];
              *param_1 = afStack_30[3] + *param_1;
              param_1[1] = fStack_20 + param_1[1];
              param_1[2] = fStack_1c + param_1[2];
              *(float *)(uVar7 + 0xb48) = afStack_30[3] + *(float *)(uVar7 + 0xb48);
              *(float *)(uVar7 + 0xb4c) = fStack_20 + *(float *)(uVar7 + 0xb4c);
              *(float *)(uVar7 + 0xb50) = fStack_1c + *(float *)(uVar7 + 0xb50);
              DAT_101913e4 = param_1[0x19];
              DAT_101913e8 = param_1[0x1a];
              DAT_101913ec = param_1[0x1b];
            }
          }
        }
        break;
      }
      iVar10 = iVar10 + 1;
      uVar12 = uVar12 - 1;
    } while (iVar10 < 0x3f);
  }
  DAT_101913f8 = param_1[0x20];
  DAT_101913fc = param_1[0x21];
  DAT_10191400 = param_1[0x22];
  pfVar1 = param_1 + 3;
  DAT_101914d0 = *pfVar1;
  DAT_101914d4 = param_1[4];
  DAT_101914d8 = param_1[5];
  fStack_60 = DAT_101914d0;
  fStack_5c = DAT_101914d4;
  fStack_58 = DAT_101914d8;
  bVar5 = CL_IsThirdPerson();
  fVar2 = _DAT_1007d52c;
  if (CONCAT31(extraout_var_00,bVar5) != 0) {
    param_1[4] = fStack_50;
    *pfVar1 = fStack_54;
    param_1[5] = fStack_4c;
    if (fStack_54 <= fVar2) {
      fVar2 = fStack_54;
      if (fStack_54 < _DAT_1007d528) {
        fVar2 = fStack_54 + _DAT_1007d458;
      }
    }
    else {
      fVar2 = fStack_54 - (float)_DAT_1007d460;
    }
    fVar2 = fVar2 * (float)_DAT_10081e50;
    *(float *)((int)fVar6 + 0xb54) = fVar2;
    *(float *)((int)fVar6 + 0x2cc) = fVar2;
    *(float *)((int)fVar6 + 0x178) = fVar2;
    *(float *)((int)fVar6 + 0xb28) = fVar2;
  }
  if (((int)param_1[0x2b] < (int)param_1[0x2c]) &&
     (iVar10 = (*DAT_100d5bfc)(param_1[0x2c]), iVar10 != 0)) {
    *param_1 = *(float *)(iVar10 + 0xb48);
    param_1[1] = *(float *)(iVar10 + 0xb4c);
    param_1[2] = *(float *)(iVar10 + 0xb50);
    DAT_101914d0 = *(float *)(iVar10 + 0xb54);
    *pfVar1 = DAT_101914d0;
    param_1[4] = *(float *)(iVar10 + 0xb58);
    param_1[5] = *(float *)(iVar10 + 0xb5c);
    DAT_101914d4 = param_1[4];
    DAT_101914d8 = param_1[5];
  }
  _DAT_10191404 = param_1[0x10];
  DAT_10190b28 = *param_1;
  DAT_10190b2c = param_1[1];
  DAT_10190b30 = param_1[2];
  fStack_60 = DAT_10190b28;
  fStack_5c = DAT_10190b2c;
  fStack_58 = DAT_10190b30;
  bVar5 = CL_IsThirdPerson();
  if (CONCAT31(extraout_var_01,bVar5) == 0) {
    DAT_101914a0 = (DAT_100cf978 < 0x28) - 1 & uVar7;
  }
  else {
    DAT_101914a0 = 0;
    DAT_10191440 = param_1[0x1c] + *(float *)((int)fVar6 + 0xb48);
    DAT_10191444 = param_1[0x1d] + *(float *)((int)fVar6 + 0xb4c);
    DAT_10191448 = param_1[0x1e] + *(float *)((int)fVar6 + 0xb50);
  }
  DAT_10190b50 = *pfVar1;
  DAT_10190b54 = param_1[4];
  DAT_10190b58 = param_1[5];
  return;
}


