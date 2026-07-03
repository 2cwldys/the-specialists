// ===== FUN @ 1003d600 size=1939 =====

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void FUN_1003d600(void)

{
  char cVar1;
  int *piVar2;
  int iVar3;
  float *pfVar4;
  float *pfVar5;
  int iVar6;
  float *pfVar7;
  void *this;
  undefined4 *puVar8;
  float10 fVar9;
  float fVar10;
  float fVar11;
  int *piVar12;
  int *piVar13;
  float fStack_120;
  float fStack_11c;
  float fStack_118;
  int iStack_114;
  int iStack_110;
  int iStack_10c;
  float fStack_108;
  float fStack_104;
  float fStack_100;
  float fStack_fc;
  float fStack_f8;
  float fStack_f4;
  float fStack_ec;
  float fStack_e8;
  float fStack_e4;
  float fStack_e0;
  float fStack_dc;
  float fStack_d8;
  float fStack_d4;
  float fStack_c8;
  float fStack_c4;
  float fStack_bc;
  float fStack_b8;
  float fStack_b0;
  float fStack_ac;
  float fStack_a4;
  float fStack_a0;
  float fStack_98;
  float fStack_94;
  float fStack_8c;
  float fStack_88;
  float fStack_84;
  float fStack_80;
  float fStack_7c;
  float fStack_74;
  float fStack_70;
  undefined4 auStack_6c [3];
  undefined4 auStack_60 [3];
  undefined4 auStack_54 [3];
  undefined4 auStack_48 [3];
  float afStack_3c [3];
  undefined4 auStack_30 [3];
  float afStack_24 [3];
  undefined4 auStack_18 [3];
  undefined4 auStack_c [3];
  
  if ((DAT_10189874 & 1) == 0) {
    DAT_10189874 = DAT_10189874 | 1;
    FUN_1006af63(0x1003ddf0);
  }
  if ((DAT_10189874 & 2) == 0) {
    DAT_10189874 = DAT_10189874 | 2;
    FUN_1006af63(0x1003dde0);
  }
  if ((DAT_10189874 & 4) == 0) {
    DAT_10189874 = DAT_10189874 | 4;
    FUN_1006af63(0x1003ddd0);
  }
  if ((DAT_10189874 & 8) == 0) {
    DAT_10189874 = DAT_10189874 | 8;
    FUN_1006af63(0x1003ddc0);
  }
  if ((DAT_10189874 & 0x10) == 0) {
    DAT_10189874 = DAT_10189874 | 0x10;
    FUN_1006af63(0x1003ddb0);
  }
  if ((DAT_10189874 & 0x20) == 0) {
    DAT_10189874 = DAT_10189874 | 0x20;
    FUN_1006af63(0x1003dda0);
  }
  DAT_101895d0 = FUN_100509d0(&DAT_1018bde8,(byte *)s_sprites_laserdot_spr_100936b0,5);
  if ((DAT_101895d0 != -1) &&
     (DAT_10189630 = FUN_100509d0(&DAT_1018bde8,(byte *)s_sprites_laserbeam_spr_10093bf8,5),
     DAT_10189630 != -1)) {
    fVar9 = (float10)(*DAT_100d5c34)();
    _DAT_1018962c = (float)fVar9;
    fVar9 = (float10)(*DAT_100d5c34)();
    _DAT_101897d4 = (float)fVar9;
    DAT_101897e4 = (*DAT_100d5bb8)();
    piVar2 = (int *)(*DAT_100d5bf4)();
    DAT_10189870 = *piVar2;
    DAT_1018986c = 0;
    if (-1 < DAT_101897e4) {
      do {
        if ((((DAT_1018986c != DAT_10189870) &&
             ((*(byte *)((int)&DAT_1018d128 + DAT_1018986c * 2) & 2) != 0)) &&
            (DAT_1018996c = (*DAT_100d5bfc)(), DAT_1018996c != 0)) &&
           ((*(int *)(DAT_1018996c + 4) != 0 && (cVar1 = FUN_100579a0(DAT_1018986c), cVar1 != '\0'))
           )) {
          puVar8 = (undefined4 *)(DAT_100d5c70 + 0x2c);
          (*DAT_100d5c20)();
          iVar3 = (*(code *)*puVar8)();
          if (iVar3 != 0) {
            FUN_10068e20(DAT_1018986c,(undefined4 *)&DAT_10189960,(float *)&DAT_10189980);
            _DAT_10189980 = _DAT_1018962c * (float)_DAT_10079e70 + _DAT_10189980;
            _DAT_10189984 = _DAT_101897d4 * (float)_DAT_10079e70 + _DAT_10189984;
            (*DAT_100d5b88)();
            pfVar4 = (float *)FUN_10056be0(auStack_6c,DAT_1018986c,1);
            pfVar5 = (float *)FUN_10056be0(auStack_30,DAT_1018986c,0);
            fStack_120 = *pfVar4 - *pfVar5;
            iVar3 = FUN_10056be0(auStack_48,DAT_1018986c,1);
            iVar6 = FUN_10056be0(auStack_18,DAT_1018986c,0);
            fStack_11c = *(float *)(iVar3 + 4) - *(float *)(iVar6 + 4);
            iVar3 = FUN_10056be0(auStack_60,DAT_1018986c,1);
            iVar6 = FUN_10056be0(auStack_54,DAT_1018986c,0);
            fStack_118 = *(float *)(iVar3 + 8) - *(float *)(iVar6 + 8);
            FUN_10029e40(&fStack_120);
            iVar3 = DAT_1018986c;
            if (_DAT_10189624 * fStack_11c + _DAT_10189628 * fStack_118 + _DAT_10189620 * fStack_120
                < _DAT_1007d6f0) {
              _DAT_10189620 = fStack_120;
              _DAT_10189624 = fStack_11c;
              _DAT_10189628 = fStack_118;
            }
            fVar10 = DAT_10191478 * _DAT_1007a890;
            if (_DAT_10079e58 < DAT_10191478 * _DAT_1007a890) {
              fVar10 = _DAT_10079e58;
            }
            fStack_c8 = _DAT_10189624 - (float)(&DAT_1018963c)[DAT_1018986c * 3];
            pfVar4 = (float *)(&DAT_10189638 + DAT_1018986c * 3);
            fStack_c4 = _DAT_10189628 - (float)(&DAT_10189640)[DAT_1018986c * 3];
            fStack_d8 = (_DAT_10189620 - (float)(&DAT_10189638)[DAT_1018986c * 3]) * fVar10;
            fStack_d4 = fStack_c8 * fVar10;
            fStack_108 = fStack_d8 + *pfVar4;
            fStack_104 = fStack_d4 + (float)(&DAT_1018963c)[DAT_1018986c * 3];
            fStack_100 = fVar10 * fStack_c4 + (float)(&DAT_10189640)[DAT_1018986c * 3];
            *pfVar4 = fStack_108;
            (&DAT_1018963c)[iVar3 * 3] = fStack_104;
            (&DAT_10189640)[iVar3 * 3] = fStack_100;
            fStack_74 = fStack_104;
            fStack_70 = fStack_100;
            FUN_10029e40(pfVar4);
            (**(code **)(DAT_100d5c78 + 0x2c))();
            (**(code **)(DAT_100d5c78 + 0x28))();
            (**(code **)(DAT_100d5c78 + 0x34))();
            (**(code **)(DAT_100d5c78 + 0x38))();
            fStack_98 = (float)(&DAT_1018963c)[DAT_1018986c * 3] * _DAT_1007d448;
            fStack_94 = (float)(&DAT_10189640)[DAT_1018986c * 3] * _DAT_1007d448;
            fStack_84 = (float)(&DAT_10189638)[DAT_1018986c * 3] * _DAT_1007d448 + _DAT_10189960;
            fStack_b0 = _DAT_10189964 + fStack_98;
            fStack_ac = _DAT_10189968 + fStack_94;
            fStack_80 = fStack_b0;
            fStack_7c = fStack_ac;
            (**(code **)(DAT_100d5c78 + 0x3c))(&DAT_10189960,&fStack_84);
            (**(code **)(DAT_100d5c78 + 0x30))();
            fStack_ec = (float)(&DAT_1018963c)[DAT_1018986c * 3] +
                        (float)(&DAT_1018963c)[DAT_1018986c * 3];
            fStack_e8 = (float)(&DAT_10189640)[DAT_1018986c * 3] +
                        (float)(&DAT_10189640)[DAT_1018986c * 3];
            _DAT_10189850 =
                 _DAT_101895ec -
                 ((float)(&DAT_10189638)[DAT_1018986c * 3] +
                 (float)(&DAT_10189638)[DAT_1018986c * 3]);
            _DAT_10189854 = _DAT_101895f0 - fStack_ec;
            _DAT_10189858 = _DAT_101895f4 - fStack_e8;
            fStack_fc = _DAT_10189850;
            fStack_f8 = _DAT_10189854;
            fStack_f4 = _DAT_10189858;
            fStack_bc = _DAT_10189854;
            fStack_b8 = _DAT_10189858;
            FUN_100582f0((int)*(char *)((int)&DAT_1018d128 + DAT_1018986c * 2 + 1),&iStack_114,
                         &iStack_110,&iStack_10c);
            (**(code **)(DAT_100d5c70 + 4))(5);
            (**(code **)(DAT_100d5c70 + 0x28))(1);
            (**(code **)(DAT_100d5c70 + 8))();
            (**(code **)(DAT_100d5c70 + 0x10))();
            FUN_10050080((float *)&DAT_10189910,(float *)&DAT_10189850,&DAT_10189960,4.0,'\x01',
                         (float *)0x0,0.0,(float *)0x0);
            FUN_10050000(0x10189910);
            (**(code **)(DAT_100d5c70 + 0xc))();
            (**(code **)(DAT_100d5c70 + 4))(0);
            puVar8 = (undefined4 *)(DAT_100d5c70 + 0x2c);
            (*DAT_100d5c20)();
            iVar3 = (*(code *)*puVar8)();
            if (iVar3 != 0) {
              fStack_a4 = (float)(&DAT_1018963c)[DAT_1018986c * 3] * _DAT_100799f4;
              fStack_a0 = (float)(&DAT_10189640)[DAT_1018986c * 3] * _DAT_100799f4;
              fStack_e4 = _DAT_10189850 - (float)(&DAT_10189638)[DAT_1018986c * 3] * _DAT_100799f4;
              fStack_e0 = _DAT_10189854 - fStack_a4;
              fStack_dc = _DAT_10189858 - fStack_a0;
              piVar2 = &iStack_10c;
              piVar13 = &iStack_110;
              piVar12 = &iStack_114;
              pfVar4 = &fStack_e4;
              fVar11 = 0.5;
              fStack_8c = fStack_e0;
              fStack_88 = fStack_dc;
              pfVar7 = (float *)FUN_1003d240(&DAT_10189638 + DAT_1018986c * 3,afStack_3c,16.0);
              pfVar5 = afStack_24;
              this = (void *)FUN_10056be0(auStack_c,DAT_1018986c,0);
              fVar10 = 2.600573e-29;
              puVar8 = (undefined4 *)FUN_10035b10(this,pfVar5,pfVar7);
              FUN_10002610(&stack0xfffffeb8,puVar8);
              FUN_1003d280(fVar10,(float)pfVar5,(float)pfVar7,pfVar4,fVar11,piVar12,piVar13,piVar2);
            }
          }
        }
        DAT_1018986c = DAT_1018986c + 1;
      } while (DAT_1018986c <= DAT_101897e4);
    }
  }
  return;
}


