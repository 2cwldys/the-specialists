// ===== FUN @ 10068f70 size=3055 =====

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void __cdecl FUN_10068f70(float *param_1)

{
  float *pfVar1;
  float *pfVar2;
  float fVar3;
  int iVar4;
  int *piVar5;
  float *pfVar6;
  undefined4 *puVar7;
  int iVar8;
  uint uVar9;
  int iVar10;
  float *pfVar11;
  undefined4 *puVar12;
  uint uVar13;
  float10 fVar14;
  longlong lVar15;
  float fStack_24;
  float fStack_20;
  float fStack_1c;
  float fStack_18;
  float fStack_14;
  float fStack_10;
  float afStack_c [3];
  
  if ((DAT_101914dc & 1) == 0) {
    DAT_101914dc = DAT_101914dc | 1;
    _DAT_10191418 = 0.0;
    _DAT_1019141c = 0.0;
    _DAT_10191420 = 0.0;
    FUN_1006af63(0x10069b80);
  }
  iVar4 = (*DAT_100d5bfc)(DAT_10190b20);
  piVar5 = (int *)(*DAT_100d5bf8)();
  DAT_10191490 = param_1[0x19];
  *param_1 = DAT_10191490;
  param_1[1] = param_1[0x1a];
  param_1[2] = param_1[0x1b];
  DAT_10191494 = param_1[0x1a];
  DAT_10191498 = param_1[0x1b];
  DAT_101913f8 = param_1[0x20];
  DAT_101913fc = param_1[0x21];
  DAT_10191400 = param_1[0x22];
  DAT_101914d0 = param_1[3];
  DAT_101914d4 = param_1[4];
  DAT_101914d8 = param_1[5];
  DAT_10190b28 = *param_1;
  DAT_10190b2c = param_1[1];
  DAT_10190b30 = param_1[2];
  if (DAT_101914f0 == 0) {
    DAT_101913e4 = *param_1;
    DAT_101913e8 = param_1[1];
    DAT_101913ec = param_1[2];
  }
  else {
    *param_1 = DAT_101913e4;
    param_1[1] = DAT_101913e8;
    param_1[2] = DAT_101913ec;
  }
  param_1[3] = param_1[0x20];
  param_1[4] = param_1[0x21];
  param_1[5] = param_1[0x22];
  if (DAT_1019149c == 0) {
    DAT_10190b70 = param_1[4];
    DAT_10190b74 = param_1[5];
    DAT_10190b6c = param_1[0x20];
  }
  else {
    param_1[3] = DAT_10190b6c;
    param_1[4] = DAT_10190b70;
    param_1[5] = DAT_10190b74;
  }
  if (param_1[0x38] == 0.0) {
    fStack_18 = param_1[3] - _DAT_10190b44;
    fStack_14 = param_1[4] - _DAT_10190b48;
    fStack_10 = param_1[5] - _DAT_10190b4c;
    fVar14 = FUN_10058390(&fStack_18);
    if (fVar14 != (float10)_DAT_100799f0) {
      *(float *)(&DAT_10190f98 + (DAT_1019139c & 0x3f) * 0xc) = param_1[3];
      *(float *)(&DAT_10190f9c + (DAT_1019139c & 0x3f) * 0xc) = param_1[4];
      *(float *)(&DAT_10190fa0 + (DAT_1019139c & 0x3f) * 0xc) = param_1[5];
      *(float *)(&DAT_10191298 + (DAT_1019139c & 0x3f) * 4) = param_1[0x10];
      DAT_1019139c = DAT_1019139c + 1;
      _DAT_10190b44 = param_1[3];
      _DAT_10190b48 = param_1[4];
      _DAT_10190b4c = param_1[5];
    }
    if (((DAT_100df430 != 0) && (*(float *)(DAT_100df430 + 0xc) != _DAT_100799f0)) &&
       (((byte)DAT_101914ec & 2) != 0)) {
      fVar3 = param_1[0x10] - *(float *)(DAT_100df430 + 0xc);
      iVar8 = 1;
      uVar13 = DAT_1019139c - 2;
      do {
        if (*(float *)(&DAT_10191298 + (uVar13 & 0x3f) * 4) <= fVar3) {
          if ((iVar8 < 0x3f) &&
             (uVar9 = uVar13 & 0x3f, *(float *)(&DAT_10191298 + uVar9 * 4) != (float)_DAT_1007a878))
          {
            (*DAT_100d5b88)(&DAT_10190f98 + uVar9 * 0xc,&fStack_24,0,0);
            uVar13 = uVar13 + 1 & 0x3f;
            (*DAT_100d5b88)(&DAT_10190f98 + uVar13 * 0xc,afStack_c,0,0);
            fVar14 = FUN_10029d70(&fStack_24,afStack_c);
            if (((float)_DAT_1007a878 <
                 *(float *)(&DAT_10191298 + uVar13 * 4) - *(float *)(&DAT_10191298 + uVar9 * 4)) &&
               ((float)fVar14 < _DAT_1007d59c)) {
              fVar3 = (fVar3 - *(float *)(&DAT_10191298 + uVar9 * 4)) /
                      (*(float *)(&DAT_10191298 + uVar13 * 4) -
                      *(float *)(&DAT_10191298 + uVar9 * 4));
              if ((float)_DAT_10079e80 < fVar3) {
                fVar3 = (float)_DAT_10079e80;
              }
              FUN_10029cf0((float *)(&DAT_10190f98 + uVar9 * 0xc),
                           (float *)(&DAT_10190f98 + uVar13 * 0xc),&DAT_10190b6c,fVar3);
              param_1[3] = DAT_10190b6c;
              param_1[4] = DAT_10190b70;
              param_1[5] = DAT_10190b74;
            }
          }
          break;
        }
        iVar8 = iVar8 + 1;
        uVar13 = uVar13 - 1;
      } while (iVar8 < 0x3f);
    }
    fStack_18 = *param_1 - _DAT_10190b60;
    fStack_14 = param_1[1] - _DAT_10190b64;
    fStack_10 = param_1[2] - _DAT_10190b68;
    fVar14 = FUN_10058390(&fStack_18);
    if (fVar14 != (float10)_DAT_1007a878) {
      *(float *)(&DAT_10190b98 + (DAT_10191398 & 0x3f) * 0xc) = *param_1;
      *(float *)(&DAT_10190b9c + (DAT_10191398 & 0x3f) * 0xc) = param_1[1];
      *(float *)(&DAT_10190ba0 + (DAT_10191398 & 0x3f) * 0xc) = param_1[2];
      *(float *)(&DAT_10190e98 + (DAT_10191398 & 0x3f) * 4) = param_1[0x10];
      DAT_10191398 = DAT_10191398 + 1;
    }
    if (((DAT_100df430 != 0) && (*(float *)(DAT_100df430 + 0xc) != _DAT_100799f0)) &&
       (((byte)DAT_101914ec & 4) != 0)) {
      fVar3 = param_1[0x10] - *(float *)(DAT_100df430 + 0xc);
      iVar8 = 1;
      uVar13 = DAT_10191398 - 2;
      do {
        if (*(float *)(&DAT_10190e98 + (uVar13 & 0x3f) * 4) <= fVar3) {
          if ((iVar8 < 0x3f) &&
             (uVar9 = uVar13 & 0x3f, *(float *)(&DAT_10190e98 + uVar9 * 4) != (float)_DAT_1007a878))
          {
            uVar13 = uVar13 + 1 & 0x3f;
            if ((float)_DAT_1007a878 <
                *(float *)(&DAT_10190e98 + uVar13 * 4) - *(float *)(&DAT_10190e98 + uVar9 * 4)) {
              fVar3 = (fVar3 - *(float *)(&DAT_10190e98 + uVar9 * 4)) /
                      (*(float *)(&DAT_10190e98 + uVar13 * 4) -
                      *(float *)(&DAT_10190e98 + uVar9 * 4));
              if ((float)_DAT_10079e80 < fVar3) {
                fVar3 = (float)_DAT_10079e80;
              }
              iVar10 = uVar9 * 0xc;
              iVar8 = uVar13 * 0xc;
              fStack_24 = *(float *)(&DAT_10190b98 + iVar8) - *(float *)(&DAT_10190b98 + iVar10);
              fStack_20 = *(float *)(&DAT_10190b9c + iVar8) - *(float *)(&DAT_10190b9c + iVar10);
              fStack_1c = *(float *)(&DAT_10190ba0 + iVar8) - *(float *)(&DAT_10190ba0 + iVar10);
              FUN_10058410((float *)(&DAT_10190b98 + iVar10),fVar3,&fStack_24,afStack_c);
              fVar14 = FUN_10058390(&fStack_24);
              if (fVar14 < (float10)_DAT_1007a534) {
                fStack_20 = afStack_c[1] - param_1[0x1a];
                fStack_1c = afStack_c[2] - param_1[0x1b];
                DAT_101913e4 = (afStack_c[0] - param_1[0x19]) + *param_1;
                *param_1 = DAT_101913e4;
                param_1[1] = fStack_20 + param_1[1];
                param_1[2] = fStack_1c + param_1[2];
                DAT_101913e8 = param_1[1];
                DAT_101913ec = param_1[2];
              }
            }
          }
          break;
        }
        iVar8 = iVar8 + 1;
        uVar13 = uVar13 - 1;
      } while (iVar8 < 0x3f);
    }
  }
  if (_DAT_100799f0 < param_1[0xf]) {
    fStack_24 = *param_1 - _DAT_10190b60;
    fStack_20 = param_1[1] - _DAT_10190b64;
    fStack_1c = param_1[2] - _DAT_10190b68;
    FUN_100583e0(&fStack_24,_DAT_10079e58 / param_1[0xf],&fStack_24);
    _DAT_10191418 = _DAT_10191418 * _DAT_10081e88 + fStack_24 * _DAT_1007a8f0;
    _DAT_1019141c = _DAT_1019141c * _DAT_10081e88 + fStack_20 * _DAT_1007a8f0;
    _DAT_10191420 = _DAT_10191420 * _DAT_10081e88 + fStack_1c * _DAT_1007a8f0;
  }
  _DAT_10190b60 = *param_1;
  _DAT_10190b64 = param_1[1];
  _DAT_10190b68 = param_1[2];
  _DAT_101914e4 = param_1[0x10];
  if (param_1[0x38] == 0.0) {
    switch(DAT_10190b18) {
    case 1:
      FUN_10068d40(DAT_10190b20,(float *)0x0,&DAT_10190b28,&DAT_101914d0);
      break;
    case 2:
      FUN_10068d40(DAT_10190b20,&DAT_101913f8,&DAT_10190b28,&DAT_101914d0);
      break;
    case 3:
      DAT_101914d0 = DAT_101913f8;
      DAT_101914d4 = DAT_101913fc;
      DAT_101914d8 = DAT_10191400;
      DAT_10190b28 = DAT_10191490;
      DAT_10190b2c = DAT_10191494;
      DAT_10190b30 = DAT_10191498;
      break;
    default:
      param_1[0x39] = 0.0;
      break;
    case 5:
      FUN_10068e20(DAT_10190b20,param_1,param_1 + 3);
    }
    if (DAT_100d000c != 0) {
      param_1[0x38] = 1.4013e-45;
    }
    _DAT_100d0010 = 0;
  }
  else {
    lVar15 = __ftol();
    param_1[0x34] = (float)lVar15;
    lVar15 = __ftol();
    param_1[0x35] = (float)lVar15;
    lVar15 = __ftol();
    param_1[0x36] = (float)lVar15;
    lVar15 = __ftol();
    param_1[0x37] = (float)lVar15;
    param_1[0x38] = 0.0;
    param_1[0x39] = 0.0;
    if (DAT_100d000c == 2) {
      FUN_10068e20(DAT_100d0940,param_1,param_1 + 3);
    }
    else if (DAT_100d000c == 3) {
      param_1[0x39] = 1.4013e-45;
      afStack_c[0] = param_1[0x20];
      _DAT_100d0958 = param_1[0x21];
      _DAT_100d095c = param_1[0x22];
      _DAT_100d0954 = afStack_c[0] * _DAT_1007d44c * _DAT_10081e84 + _DAT_10081e80;
      fStack_24 = _DAT_100d0954;
      fStack_20 = _DAT_100d0958;
      fStack_1c = _DAT_100d095c;
      afStack_c[1] = _DAT_100d0958;
      afStack_c[2] = _DAT_100d095c;
      FUN_100240b0(&DAT_100cfff8,param_1);
      param_1[3] = fStack_24;
      param_1[4] = fStack_20;
      param_1[5] = fStack_1c;
    }
    _DAT_100d0010 = 1;
  }
  if (((DAT_10190b18 == 5) && (iVar4 != 0)) && (DAT_10190b20 != 0)) {
    pfVar1 = (float *)(piVar5 + 0x2d2);
    pfVar2 = (float *)(piVar5 + 0x2d5);
    *pfVar1 = DAT_10190b28;
    piVar5[0x2d3] = (int)DAT_10190b2c;
    piVar5[0x2d4] = (int)DAT_10190b30;
    *pfVar2 = DAT_101914d0;
    piVar5[0x2d6] = (int)DAT_101914d4;
    piVar5[0x2d7] = (int)DAT_101914d8;
    *pfVar2 = *pfVar2 * _DAT_1007d4d8;
    (*DAT_100d5b88)(&DAT_101914d0,afStack_c,0,0);
    pfVar11 = afStack_c;
    iVar8 = 3;
    pfVar6 = pfVar1;
    do {
      fVar3 = *pfVar11;
      pfVar11 = pfVar11 + 1;
      iVar8 = iVar8 + -1;
      *pfVar6 = fVar3 * _DAT_10191dac * (float)_DAT_1007cd80 + *pfVar6;
      pfVar6 = pfVar6 + 1;
    } while (iVar8 != 0);
    piVar5[0x2d6] = (int)((float)piVar5[0x2d6] - _DAT_10191dac * (float)_DAT_10079e90);
    piVar5[0x2d7] = (int)((float)piVar5[0x2d7] - _DAT_10191dac);
    *pfVar2 = *pfVar2 - _DAT_10191dac * (float)_DAT_10079e50;
    piVar5[0xb3] = (int)*pfVar2;
    piVar5[0xb4] = piVar5[0x2d6];
    piVar5[0xb5] = piVar5[0x2d7];
    piVar5[0x2ca] = (int)*pfVar2;
    piVar5[0x2cb] = piVar5[0x2d6];
    piVar5[0x2cc] = piVar5[0x2d7];
    iVar4 = *(int *)(iVar4 + 0x364);
    if (DAT_10191db0 != iVar4) {
      DAT_10191db0 = iVar4;
      DAT_10097218 = FUN_10068ef0(iVar4);
      if (DAT_10097218 == -1) {
        piVar5[0x2e5] = 0;
        DAT_10191db0 = 0;
        DAT_10097218 = -1;
        piVar5[0x2e5] = 0;
        DAT_101914a0 = (int *)0x0;
        goto LAB_10069b0c;
      }
      _DAT_10191d5c =
           FUN_10007110(DAT_10097218,(int)*(char *)((int)&DAT_1018d128 + DAT_10190b20 * 2));
    }
    if (DAT_10097218 == -1) {
      piVar5[0x2e5] = 0;
      DAT_101914a0 = (int *)0x0;
    }
    else {
      puVar12 = (undefined4 *)(DAT_100d5c78 + 0xc);
      puVar7 = FUN_100071c0(DAT_10097218);
      iVar4 = (*(code *)*puVar12)((int)puVar7 + 0x132);
      iVar8 = (*DAT_100ea7d4)(iVar4);
      piVar5[0x2e5] = iVar8;
      piVar5[0xb6] = iVar4;
      piVar5[0xb8] = 0;
      piVar5[0xb9] = 0;
      *piVar5 = DAT_10190b20;
      *pfVar2 = param_1[0x20];
      piVar5[0x2d6] = (int)param_1[0x21];
      piVar5[0x2d7] = (int)param_1[0x22];
      FUN_100674a0((int)param_1,(float *)&DAT_10191dac);
      *pfVar1 = *param_1;
      piVar5[0x2d3] = (int)param_1[1];
      piVar5[0x2d4] = (int)param_1[2];
      (*DAT_100d5c08)(pfVar1,pfVar2,0x3f666666);
      *pfVar1 = DAT_10191428 + *pfVar1;
      piVar5[0x2d3] = (int)(DAT_1019142c + (float)piVar5[0x2d3]);
      piVar5[0x2d4] = (int)(_DAT_10191430 + (float)piVar5[0x2d4]);
      piVar5[0x2d4] = (int)((float)piVar5[0x2d4] - _DAT_10079e58);
      DAT_101914a0 = piVar5;
    }
  }
  else {
    piVar5[0x2e5] = 0;
    DAT_10191db0 = 0;
    DAT_10097218 = -1;
    DAT_101914a0 = (int *)0x0;
  }
LAB_10069b0c:
  DAT_101914d0 = param_1[3];
  DAT_101914d4 = param_1[4];
  DAT_101914d8 = param_1[5];
  DAT_10190b28 = *param_1;
  DAT_10190b2c = param_1[1];
  DAT_10190b30 = param_1[2];
  return;
}


