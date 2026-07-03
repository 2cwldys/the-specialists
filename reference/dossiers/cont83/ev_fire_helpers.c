// ===== FUN @ 100450c0 size=3277 =====

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void __cdecl
FUN_100450c0(int param_1,undefined4 param_2,float *param_3,float *param_4,byte param_5,
            float *param_6,float *param_7,float param_8,float param_9,float param_10,int param_11,
            int param_12,undefined4 param_13,int param_14)

{
  float *pfVar1;
  float *pfVar2;
  byte bVar3;
  float *pfVar4;
  int *piVar5;
  uint uVar6;
  int iVar7;
  float10 fVar8;
  float10 fVar9;
  float10 fVar10;
  longlong lVar11;
  float in_stack_ffffff7c;
  float fVar12;
  undefined *puVar13;
  float fVar14;
  float fVar15;
  char cVar16;
  float fStack_54;
  float local_50;
  float local_4c;
  float fStack_48;
  float local_44;
  float local_40;
  float local_3c;
  float local_38;
  float local_34;
  float local_30;
  float local_2c;
  float local_28;
  float local_24;
  float local_20;
  float local_1c;
  float local_18 [5];
  undefined1 uStack_1;
  
  pfVar2 = param_7;
  pfVar1 = param_6;
  if ((DAT_1018a1b4 & 1) == 0) {
    DAT_1018a1b4 = DAT_1018a1b4 | 1;
    FUN_1006af63(0x10045de0);
  }
  if ((DAT_1018a1b4 & 2) == 0) {
    DAT_1018a1b4 = DAT_1018a1b4 | 2;
    FUN_1006af63(0x10045dd0);
  }
  if ((DAT_1018a1b4 & 4) == 0) {
    DAT_1018a1b4 = DAT_1018a1b4 | 4;
    FUN_1006af63(0x10045dc0);
  }
  if ((DAT_1018a1b4 & 8) == 0) {
    DAT_1018a1b4 = DAT_1018a1b4 | 8;
    FUN_1006af63(0x10045db0);
  }
  if ((DAT_1018a1b4 & 0x10) == 0) {
    DAT_1018a1b4 = DAT_1018a1b4 | 0x10;
    FUN_1006af63(0x10045da0);
  }
  if ((DAT_1018a1b4 & 0x20) == 0) {
    DAT_1018a1b4 = DAT_1018a1b4 | 0x20;
    FUN_1006af63(0x10045d90);
  }
  if ((DAT_1018d16c != 0) || (DAT_1018a154 = '\0', DAT_100963e8 < _DAT_1007d4e8)) {
    DAT_1018a154 = '\x01';
  }
  DAT_1018a0e0 = FUN_100071f0(param_12);
  if (param_11 == 2) {
    _DAT_1018a1b8 = *param_7 * _DAT_1007d440 + *param_6;
    _DAT_1018a1bc = param_7[1] * _DAT_1007d440 + param_6[1];
    _DAT_1018a1c0 = param_7[2] * _DAT_1007d440 + param_6[2];
    (**(code **)(DAT_100d5c78 + 0x28))();
    (**(code **)(DAT_100d5c78 + 0x2c))();
    (**(code **)(DAT_100d5c78 + 0x34))();
    (**(code **)(DAT_100d5c78 + 0x38))();
    (**(code **)(DAT_100d5c78 + 0x3c))(param_6);
    if (_DAT_1018a120 != (float)_DAT_10079e80) {
      FUN_10044bf0(param_1,0x1018a110,param_6,&DAT_1018a1b8,2);
      FUN_10001180(0x1018a110);
    }
    (**(code **)(DAT_100d5c78 + 0x30))();
    return;
  }
  local_3c = *param_7;
  local_34 = param_7[2];
  local_38 = param_7[1];
  pfVar4 = (float *)FUN_1003d240(&local_3c,local_18,4096.0);
  local_50 = pfVar4[1];
  local_30 = *pfVar4 + *param_6;
  local_4c = pfVar4[2];
  local_44 = param_6[1];
  local_40 = param_6[2];
  local_2c = local_44 + local_50;
  local_28 = local_40 + local_4c;
  FUN_10002610(&local_24,&local_30);
  _DAT_1018a1b8 = local_24;
  _DAT_1018a1bc = local_20;
  _DAT_1018a1c0 = local_1c;
  (**(code **)(DAT_100d5c78 + 0x3c))();
  _DAT_1018a1b8 = DAT_1018a124;
  _DAT_1018a1bc = DAT_1018a128;
  _DAT_1018a1c0 = DAT_1018a12c;
  local_30 = *param_6 + DAT_1018a124;
  local_3c = DAT_1018a124;
  local_20 = param_6[1];
  local_38 = DAT_1018a128;
  local_2c = local_20 + DAT_1018a128;
  local_1c = param_6[2];
  local_34 = DAT_1018a12c;
  local_28 = local_1c + DAT_1018a12c;
  FUN_10002610(&local_24,&local_30);
  local_3c = local_24 * _DAT_1007a870;
  local_38 = local_20 * _DAT_1007a870;
  local_34 = local_1c * _DAT_1007a870;
  FUN_10002610(&fStack_54,&local_3c);
  local_24 = fStack_54 - _DAT_1018a1b8;
  local_20 = local_50 - _DAT_1018a1bc;
  local_1c = local_4c - _DAT_1018a1c0;
  FUN_10002610(&local_3c,&local_24);
  piVar5 = (int *)(*DAT_100d5bf4)();
  if (param_1 != *piVar5) {
    local_30 = *param_6 - _DAT_1018a1b8;
    local_20 = param_6[1];
    local_1c = param_6[2];
    local_2c = local_20 - _DAT_1018a1bc;
    local_28 = local_1c - _DAT_1018a1c0;
    FUN_10002610(&fStack_48,&local_30);
    if (_DAT_1007a938 < SQRT(fStack_48 * fStack_48 + local_44 * local_44 + local_40 * local_40)) {
      local_30 = fStack_54 - DAT_10190b28;
      local_20 = DAT_10190b2c;
      local_1c = DAT_10190b30;
      local_2c = local_50 - DAT_10190b2c;
      local_28 = local_4c - DAT_10190b30;
      FUN_10002610(&fStack_48,&local_30);
      if (SQRT(fStack_48 * fStack_48 + local_44 * local_44 + local_40 * local_40) <
          SQRT(local_3c * local_3c + local_38 * local_38 + local_34 * local_34) - _DAT_1007a910) {
        local_2c = param_6[1];
        fStack_48 = DAT_10190b28 - *param_6;
        local_20 = DAT_10190b2c;
        local_28 = param_6[2];
        local_1c = DAT_10190b30;
        local_44 = DAT_10190b2c - local_2c;
        local_40 = DAT_10190b30 - local_28;
        FUN_10002610(&local_3c,&fStack_48);
        local_20 = DAT_10190b2c;
        local_1c = DAT_10190b30;
        fVar14 = SQRT(local_3c * local_3c + local_38 * local_38 + local_34 * local_34);
        local_30 = DAT_10190b28 - _DAT_1018a1b8;
        local_2c = DAT_10190b2c - _DAT_1018a1bc;
        local_28 = DAT_10190b30 - _DAT_1018a1c0;
        FUN_10002610(&fStack_48,&local_30);
        local_30 = *param_6 - _DAT_1018a1b8;
        local_20 = param_6[1];
        local_1c = param_6[2];
        local_2c = local_20 - _DAT_1018a1bc;
        local_28 = local_1c - _DAT_1018a1c0;
        FUN_10002610(&local_3c,&local_30);
        fVar15 = SQRT(local_3c * local_3c + local_38 * local_38 + local_34 * local_34);
        fVar12 = (SQRT(fStack_48 * fStack_48 + local_44 * local_44 + local_40 * local_40) + fVar15 +
                 fVar14) * _DAT_1007a870;
        fVar14 = SQRT((fVar12 - fVar14) * (fVar12 - fVar15) * (fVar12 - fVar15) * fVar12);
        if ((fVar14 + fVar14) / fVar15 < _DAT_1007cccc) {
          (*DAT_100d5c38)();
          FUN_1006ac7a((undefined1 *)local_18,(byte *)s_weapons_whizz_d_wav_100956e8);
          uStack_1 = 0;
          (*DAT_100d5c38)();
          __ftol();
          uVar6 = DAT_1018a1c4 & 0x80000001;
          if ((int)uVar6 < 0) {
            uVar6 = (uVar6 - 1 | 0xfffffffe) + 1;
          }
          in_stack_ffffff7c = (float)(uVar6 + 0x1f7);
          DAT_1018a1c4 = DAT_1018a1c4 + 1;
          (**(code **)(DAT_100d5c78 + 4))(0xffffffff,&DAT_10190b28);
        }
      }
    }
  }
  param_7 = (float *)param_10;
  if (param_12 == 3) {
    param_7 = (float *)(param_10 * _DAT_1007ccf0);
  }
  local_24 = fStack_54 - _DAT_1018a1b8;
  local_20 = local_50 - _DAT_1018a1bc;
  local_1c = local_4c - _DAT_1018a1c0;
  FUN_10002610(local_18,&local_24);
  if (DAT_1018ca5a != '\0') {
    param_6 = (float *)param_10;
    if (param_12 == 3) {
      param_6 = (float *)(param_10 * _DAT_1007ccf0);
    }
    fVar15 = 0.0;
    fVar14 = 2.6100624e-29;
    pfVar4 = pfVar1;
    FUN_100524c0(&DAT_1018bde8,pfVar1,(float *)0x0,512.0,(float)param_7);
    if (((param_12 == 5) || (param_12 == 8)) || (param_12 == 0xd)) {
      cVar16 = '\x01';
    }
    else {
      cVar16 = '\0';
    }
    puVar13 = &DAT_1018a1b8;
    fVar12 = 2.610078e-29;
    FUN_10002610(&stack0xffffff88,(undefined4 *)&DAT_1018a1b8);
    FUN_10002610(&stack0xffffff7c,pfVar1);
    FUN_10052720(&DAT_1018bde8,in_stack_ffffff7c,fVar12,(float)puVar13,fVar14,(float)pfVar4,fVar15,
                 (float)param_6,cVar16);
  }
  DAT_1018a0dc = CONCAT31(DAT_1018a0dc._1_3_,1);
  if (param_5 != 0) {
    do {
      fVar8 = FUN_1001aea0((DAT_1018a0dc & 0xff) + param_14,-3.1415927,3.1415927);
      fVar9 = FUN_1001aea0((DAT_1018a0dc & 0xff) + 1 + param_14,0.0,(float)DAT_1018a0e0[7]);
      fVar9 = (float10)_DAT_10079e58 - fVar9 * fVar9 * fVar9;
      fVar10 = (float10)fcos((float10)(float)fVar8);
      fVar14 = (float)(fVar9 * fVar10);
      fVar8 = (float10)fsin((float10)(float)fVar8);
      fVar9 = fVar9 * fVar8;
      DAT_1018a1a8 = (float)(fVar9 * (float10)*param_4 * (float10)param_9 +
                             (float10)fVar14 * (float10)*param_3 * (float10)param_8 +
                            (float10)*pfVar2);
      DAT_1018a1ac = (float)((float10)fVar14 * (float10)param_3[1] * (float10)param_8 +
                             fVar9 * (float10)param_9 * (float10)param_4[1] + (float10)pfVar2[1]);
      DAT_1018a1b0 = (float)((float10)fVar14 * (float10)param_3[2] * (float10)param_8 +
                             fVar9 * (float10)param_4[2] * (float10)param_9 + (float10)pfVar2[2]);
      _DAT_1018a1b8 = (float)(int)DAT_1018a0e0[5] * DAT_1018a1a8 + *pfVar1;
      _DAT_1018a1bc = (float)(int)DAT_1018a0e0[5] * DAT_1018a1ac + pfVar1[1];
      _DAT_1018a1c0 = (float)(int)DAT_1018a0e0[5] * DAT_1018a1b0 + pfVar1[2];
      if (DAT_1018a154 == '\0') {
        (**(code **)(DAT_100d5c78 + 0x28))();
        (**(code **)(DAT_100d5c78 + 0x2c))();
        (**(code **)(DAT_100d5c78 + 0x34))();
        (**(code **)(DAT_100d5c78 + 0x38))();
        (**(code **)(DAT_100d5c78 + 0x3c))(pfVar1);
        if (_DAT_1018a120 != (float)_DAT_10079e80) {
          _DAT_1018a100 = *pfVar1 - DAT_1018a124;
          _DAT_1018a104 = pfVar1[1] - DAT_1018a128;
          _DAT_1018a108 = pfVar1[2] - DAT_1018a12c;
          fVar8 = FUN_10058390((float *)&DAT_1018a100);
          _DAT_1018a0f4 = (float)(fVar8 / ((float10)(int)DAT_1018a0e0[8] * (float10)_DAT_1007d6cc));
          lVar11 = __ftol();
          fVar8 = FUN_10007290((int)lVar11);
          DAT_1018a164 = (float)(fVar8 * (float10)param_10);
          iVar7 = param_11;
          if (param_11 == 1) {
            iVar7 = 1;
          }
          FUN_10044bf0(param_1,0x1018a110,pfVar1,&DAT_1018a1b8,iVar7);
          FUN_10001180(0x1018a110);
          DAT_1018a164 = DAT_1018a164 * (float)DAT_1018a0e0[6];
        }
        (**(code **)(DAT_100d5c78 + 0x30))();
      }
      else {
        local_3c = *pfVar1;
        local_38 = pfVar1[1];
        local_34 = pfVar1[2];
        local_24 = DAT_1018a1a8;
        local_20 = DAT_1018a1ac;
        local_1c = DAT_1018a1b0;
        local_30 = local_3c;
        local_2c = local_38;
        local_28 = local_34;
        FUN_10035c20(&DAT_100eb678,param_1,(int *)&local_30,(int *)&local_24,(int *)&param_10,
                     &param_12,param_11);
      }
      bVar3 = (char)DAT_1018a0dc + 1;
      DAT_1018a0dc = CONCAT31(DAT_1018a0dc._1_3_,bVar3);
    } while (bVar3 <= param_5);
  }
  if (((DAT_1018a154 == '\0') && (param_11 != 1)) && ((float)_DAT_1007a918 < DAT_1018a164)) {
    fVar14 = _DAT_1007d6c8;
    if (DAT_1018a164 < _DAT_1007d6c8) {
      fVar14 = DAT_1018a164;
    }
    local_24 = fVar14 * DAT_1018a1a8;
    _DAT_1018a0e8 = local_24 + DAT_1018a124;
    _DAT_1018a0ec = DAT_1018a1ac * fVar14 + DAT_1018a128;
    _DAT_1018a0f0 = fVar14 * DAT_1018a1b0 + DAT_1018a12c;
    local_3c = _DAT_1018a0e8;
    local_38 = _DAT_1018a0ec;
    local_34 = _DAT_1018a0f0;
    local_20 = _DAT_1018a0ec;
    local_1c = _DAT_1018a0f0;
    iVar7 = (*DAT_100d5c0c)();
    if (iVar7 != -2) {
      _DAT_1018a158 = DAT_1018a124;
      _DAT_1018a15c = DAT_1018a128;
      _DAT_1018a160 = DAT_1018a12c;
      (**(code **)(DAT_100d5c78 + 0x38))();
      (**(code **)(DAT_100d5c78 + 0x3c))();
      if (_DAT_1018a120 != (float)_DAT_10079e80) {
        FUN_10044bf0(param_1,0x1018a110,&DAT_1018a0e8,&DAT_1018a158,2);
        FUN_10001180(0x1018a110);
      }
      FUN_100450c0(param_1,param_2,param_3,param_4,1,(float *)&DAT_1018a0e8,&DAT_1018a1a8,0.0,0.0,
                   DAT_1018a164,2,param_12,param_13,param_14);
    }
  }
  return;
}


// ===== FUN @ 1003de90 size=198 =====

void __cdecl FUN_1003de90(undefined4 param_1)

{
  int iVar1;
  int iVar2;
  float *pfVar3;
  undefined4 *puVar4;
  float10 fVar5;
  int in_stack_00000014;
  char in_stack_00000018;
  undefined4 in_stack_0000001c;
  int in_stack_00000020;
  undefined4 uVar6;
  
  iVar1 = DAT_100d5c78;
  if (in_stack_00000018 == '\0') {
    iVar2 = (*DAT_100d5c38)(0xfffffffb,5);
    iVar2 = iVar2 + in_stack_00000020;
    uVar6 = 0;
    puVar4 = FUN_100071f0((uint)*(byte *)(in_stack_00000014 + 8));
    fVar5 = (float10)(*DAT_100d5c34)(0x3f6b851f,0x3f800000,*puVar4,uVar6,iVar2);
    in_stack_00000014 = in_stack_00000014 + 0x2c6;
  }
  else {
    iVar2 = (*DAT_100d5c38)(0xfffffffb,5);
    iVar2 = iVar2 + in_stack_00000020;
    uVar6 = 0;
    pfVar3 = (float *)FUN_100071f0((uint)*(byte *)(in_stack_00000014 + 8));
    fVar5 = (float10)(*DAT_100d5c34)(0x3f570a3d,0x3f6b851f,*pfVar3 + *pfVar3,uVar6,iVar2);
    in_stack_00000014 = in_stack_00000014 + 0x306;
  }
  (**(code **)(iVar1 + 4))
            (param_1,&stack0x00000008,in_stack_0000001c,in_stack_00000014,(float)fVar5);
  return;
}


// ===== FUN @ 100474b0 size=2045 =====

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void __cdecl
FUN_100474b0(float param_1,float param_2,float param_3,undefined4 param_4,undefined4 param_5,
            undefined4 param_6,float param_7,float param_8,float param_9,float param_10,
            float param_11,float param_12,int param_13,float param_14)

{
  char cVar1;
  float10 fVar2;
  undefined4 uVar3;
  undefined4 uVar4;
  undefined4 *puVar5;
  int iVar6;
  int iVar7;
  float *pfVar8;
  short sVar9;
  float10 fVar10;
  float10 fVar11;
  longlong lVar12;
  float in_stack_ffffff90;
  float fVar13;
  float in_stack_ffffff9c;
  float in_stack_ffffffa0;
  float fVar14;
  float fVar15;
  float fVar16;
  float fStack_40;
  float fStack_3c;
  int iStack_34;
  int local_30;
  float fStack_18;
  float fStack_14;
  float fStack_10;
  undefined4 auStack_c [3];
  
  fVar15 = *(float *)((int)param_7 + 0x1c);
  cVar1 = *(char *)((int)param_7 + 0x350);
  pfVar8 = (float *)((int)param_7 + 0x358);
  iVar7 = *(int *)((int)param_7 + 0x354);
  param_7 = *pfVar8;
  if (param_14._0_1_ != '\0') {
    lVar12 = __ftol();
    param_7 = (float)lVar12;
  }
  if (param_12._0_1_ == '\0') {
    fVar14 = 2.6122616e-29;
    fVar10 = (float10)(*DAT_100d5c34)();
    pfVar8 = &param_8;
    fVar16 = (float)(fVar10 * (float10)(int)param_7);
    fVar13 = 2.6122706e-29;
    FUN_10002610(&stack0xffffff9c,pfVar8);
    FUN_10002610(&stack0xffffff90,&param_1);
    FUN_10052c80(&DAT_1018bde8,in_stack_ffffff90,fVar13,(float)pfVar8,in_stack_ffffff9c,
                 in_stack_ffffffa0,fVar14,fVar15,fVar16);
  }
  param_14 = (float)(int)param_11._0_2_;
  lVar12 = __ftol();
  param_11 = (float)lVar12;
  if ((short)lVar12 < 3) {
    param_11 = 4.2039e-45;
  }
  fVar16 = param_11;
  puVar5 = FUN_1004ffe0();
  puVar5[0x40] = 0;
  puVar5[0x3d] = 0x40533333;
  puVar5[0x4a] = 0;
  iVar6 = FUN_100509d0(&DAT_1018bde8,(byte *)s_sprites_ts_muzzleflash1h_spr_10095734,5);
  fVar15 = param_7;
  if (iVar6 != -1) {
    sVar9 = (short)iVar6;
    if (param_12._0_1_ == '\0') {
      fStack_40 = (float)(int)param_7;
      iVar6 = (int)param_11._0_2_;
      puVar5[0x34] = 0;
      *(undefined1 *)((int)puVar5 + 7) = 0;
      *(undefined1 *)((int)puVar5 + 6) = 0;
      *(undefined1 *)puVar5 = 1;
      puVar5[0x36] = 0;
      puVar5[0x3f] = 0;
      *(undefined1 *)((int)puVar5 + 5) = 1;
      *(undefined1 *)(puVar5 + 1) = 1;
      param_12 = 0.9;
      fStack_3c = 0.17999999;
      iStack_34 = 0;
      if (0 < iVar6) {
        param_14 = (float)iVar7;
        param_11 = (float)iVar6;
        param_7 = (float)param_13;
        do {
          fVar10 = (float10)(*DAT_100d5c34)();
          puVar5[0x42] = (float)(fVar10 * (float10)fStack_40);
          if (iStack_34 == 0) {
            puVar5[0x42] = (float)(fVar10 * (float10)fStack_40 * (float10)_DAT_1007a8e0);
          }
          fVar16 = (float)iStack_34;
          puVar5[5] = param_7 * param_8 + (param_14 * fVar16 * param_8) / param_11 + param_1;
          puVar5[6] = param_7 * param_9 + (param_14 * fVar16 * param_9) / param_11 + param_2;
          puVar5[7] = param_7 * param_10 + (param_14 * fVar16 * param_10) / param_11 + param_3;
          puVar5[0xe] = param_4;
          puVar5[0xf] = param_5;
          puVar5[0x10] = param_6;
          fVar10 = (float10)(*DAT_100d5c34)();
          puVar5[0x10] = (float)(fVar10 + (float10)(float)puVar5[0x10]);
          *(short *)(puVar5 + 0x45) = sVar9;
          iVar7 = FUN_100509a0(&DAT_1018bde8,(int)sVar9);
          puVar5[0x37] = param_12;
          puVar5[0x38] = fStack_3c;
          puVar5[0x3e] = (float)iVar7;
          uVar3 = _DAT_100799f0;
          if (iStack_34 == 1) {
            uVar3 = _DAT_1007a8f0;
          }
          puVar5[0x39] = uVar3;
          iVar7 = (*DAT_100d5c38)();
          puVar5[0x13] = (float)iVar7;
          iVar7 = FUN_10050b60(&DAT_1018bde8,puVar5,0,0);
          if (iVar7 == -1) {
            return;
          }
          param_12 = param_12 * (float)_DAT_1007d738;
          iStack_34 = iStack_34 + 1;
          fStack_3c = param_12 * (float)_DAT_1007a8c8;
          fStack_40 = fStack_40 * (float)_DAT_1007d738;
        } while (iStack_34 < iVar6);
      }
      if (cVar1 == '\x01') {
        iVar7 = FUN_100509d0(&DAT_1018bde8,(byte *)s_sprites_ts_muzzleflash6_spr_10095718,5);
        puVar5[0x34] = 0;
        param_12 = (float)(param_13 + -2);
        *(undefined1 *)((int)puVar5 + 7) = 0;
        *(undefined1 *)((int)puVar5 + 6) = 0;
        fVar16 = (float)(int)param_12;
        puVar5[5] = fVar16 * param_8 + param_1;
        puVar5[6] = fVar16 * param_9 + param_2;
        puVar5[7] = fVar16 * param_10 + param_3;
        puVar5[0xe] = param_4;
        puVar5[0xf] = param_5;
        puVar5[0x10] = param_6;
        param_12 = (float)(*DAT_100d5c38)();
        puVar5[0x13] = (float)(int)param_12;
        *(undefined1 *)puVar5 = 1;
        puVar5[0x36] = 0;
        puVar5[0x38] = 0x3e051eb8;
        *(undefined1 *)((int)puVar5 + 5) = 1;
        puVar5[0x3f] = 0;
        fVar10 = (float10)(*DAT_100d5c34)();
        puVar5[0x37] = 0x3f19999a;
        *(short *)(puVar5 + 0x45) = (short)iVar7;
        *(undefined1 *)(puVar5 + 1) = 0;
        puVar5[0x42] = (float)(fVar10 * (float10)(int)fVar15);
        param_12 = (float)(*DAT_100d5c38)();
        puVar5[0x13] = (float)(int)param_12;
        param_12 = (float)FUN_100509a0(&DAT_1018bde8,(int)*(short *)(puVar5 + 0x45));
      }
      else {
        puVar5[0x34] = 0;
        param_12 = (float)(param_13 + -2);
        *(undefined1 *)((int)puVar5 + 7) = 0;
        *(undefined1 *)((int)puVar5 + 6) = 0;
        fVar16 = (float)(int)param_12;
        puVar5[5] = fVar16 * param_8 + param_1;
        puVar5[6] = fVar16 * param_9 + param_2;
        puVar5[7] = fVar16 * param_10 + param_3;
        puVar5[0xe] = param_4;
        puVar5[0xf] = param_5;
        puVar5[0x10] = param_6;
        param_12 = (float)(*DAT_100d5c38)();
        puVar5[0x13] = (float)(int)param_12;
        *(undefined1 *)puVar5 = 1;
        puVar5[0x36] = 0;
        puVar5[0x38] = 0x3e051eb8;
        *(undefined1 *)((int)puVar5 + 5) = 1;
        puVar5[0x3f] = 0;
        fVar10 = (float10)(*DAT_100d5c34)();
        puVar5[0x37] = 0x3f333333;
        *(short *)(puVar5 + 0x45) = sVar9;
        *(undefined1 *)(puVar5 + 1) = 0;
        puVar5[0x42] = (float)(fVar10 * (float10)(int)fVar15);
        param_12 = (float)(*DAT_100d5c38)();
        puVar5[0x13] = (float)(int)param_12;
        param_12 = (float)FUN_100509a0(&DAT_1018bde8,(int)*(short *)(puVar5 + 0x45));
      }
      puVar5[0x3e] = (float)(int)param_12;
      FUN_10050b60(&DAT_1018bde8,puVar5,0,0);
      return;
    }
    pfVar8 = (float *)FUN_10057270(auStack_c);
    fStack_18 = param_1 - *pfVar8;
    fStack_14 = param_2 - pfVar8[1];
    fStack_10 = param_3 - pfVar8[2];
    fVar10 = FUN_10058390(&fStack_18);
    if (fVar10 <= (float10)_DAT_1007d730) {
      fVar11 = (float10)_DAT_1007d730;
      param_14 = (float)(int)SUB42(fVar16,0);
      fVar2 = (float10)_DAT_1007d728;
      puVar5[0x34] = 0;
      *(undefined1 *)((int)puVar5 + 7) = 0;
      *(undefined1 *)((int)puVar5 + 6) = 0;
      *(undefined1 *)puVar5 = 1;
      puVar5[0x36] = 0;
      param_12 = (float)((fVar11 - fVar10) * fVar2);
      *(undefined1 *)((int)puVar5 + 5) = 1;
      puVar5[0x3f] = 0;
      fStack_3c = param_12 * (float)_DAT_1007a8c8;
      *(short *)(puVar5 + 0x45) = sVar9;
      *(undefined1 *)(puVar5 + 1) = 1;
      fStack_40 = (float)(int)param_7;
      lVar12 = __ftol();
      uVar3 = param_6;
      iVar6 = (int)(short)lVar12;
      local_30 = 0;
      if (0 < iVar6) {
        param_14 = (float)iVar7;
        param_11 = (float)iVar6;
        param_7 = (float)param_13;
        while( true ) {
          fVar10 = (float10)(*DAT_100d5c34)();
          puVar5[0x42] = (float)(fVar10 * (float10)fStack_40);
          fVar15 = (float)local_30;
          puVar5[5] = param_7 * param_8 + (fVar15 * param_14 * param_8) / param_11 + param_1;
          puVar5[6] = param_7 * param_9 + (fVar15 * param_14 * param_9) / param_11 + param_2;
          puVar5[7] = param_7 * param_10 + (fVar15 * param_14 * param_10) / param_11 + param_3;
          puVar5[0xe] = param_4;
          puVar5[0xf] = param_5;
          puVar5[0x10] = uVar3;
          fVar10 = (float10)(*DAT_100d5c34)();
          puVar5[0x10] = (float)(fVar10 + (float10)(float)puVar5[0x10]);
          param_13 = FUN_100509a0(&DAT_1018bde8,(int)*(short *)(puVar5 + 0x45));
          puVar5[0x37] = param_12;
          puVar5[0x38] = fStack_3c;
          puVar5[0x3e] = (float)param_13;
          uVar4 = _DAT_100799f0;
          if (local_30 == 1) {
            uVar4 = _DAT_1007a8b0;
          }
          puVar5[0x39] = uVar4;
          iVar7 = FUN_10050b60(&DAT_1018bde8,puVar5,0,0);
          if (iVar7 == -1) break;
          param_12 = param_12 * (float)_DAT_1007d738;
          local_30 = local_30 + 1;
          fStack_3c = param_12 * (float)_DAT_1007a8c8;
          fStack_40 = fStack_40 * (float)_DAT_1007d738;
          if (iVar6 <= local_30) {
            return;
          }
        }
      }
    }
  }
  return;
}


// ===== FUN @ 10048e60 size=244 =====

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void __cdecl FUN_10048e60(undefined4 param_1,undefined4 param_2,undefined4 param_3)

{
  float10 fVar1;
  float10 fVar2;
  
  if (DAT_1018ca58 != '\0') {
    fVar1 = (float10)(*DAT_100d5c00)();
    _DAT_1018a460 = (float)(fVar1 - (float10)_DAT_1018a45c);
    if (fVar1 - (float10)_DAT_1018a45c < (float10)_DAT_1007a878) {
      fVar1 = (float10)(*DAT_100d5c00)();
      _DAT_1018a45c = (float)fVar1;
      return;
    }
    fVar1 = FUN_10056fa0(0.1);
    if (fVar1 <= (float10)_DAT_1018a460) {
      fVar1 = (float10)(*DAT_100d5c00)();
      _DAT_1018a45c = (float)fVar1;
      DAT_1018a444 = (undefined4 *)0x0;
      DAT_1018a444 = (undefined4 *)(**(code **)(DAT_100d5c74 + 0x100))(0);
      if (DAT_1018a444 != (undefined4 *)0x0) {
        *DAT_1018a444 = param_1;
        DAT_1018a444[1] = param_2;
        DAT_1018a444[2] = param_3;
        DAT_1018a444[3] = 0x43400000;
        *(undefined1 *)(DAT_1018a444 + 4) = 0xf0;
        *(undefined1 *)((int)DAT_1018a444 + 0x11) = 0xdd;
        *(undefined1 *)((int)DAT_1018a444 + 0x12) = 0xac;
        fVar1 = FUN_10056fa0(0.06);
        fVar2 = (float10)(*DAT_100d5c00)();
        DAT_1018a444[5] = (float)(fVar2 + (float10)(float)fVar1);
      }
    }
  }
  return;
}


