// ===== FUN @ 100071c0 size=39 =====

undefined4 * __cdecl FUN_100071c0(int param_1)

{
  if ((-1 < param_1) && (param_1 < 0x26)) {
    return &DAT_100aa050 + param_1 * 0xf7;
  }
  return &DAT_100aa050;
}


// ===== FUN @ 10007110 size=170 =====

int __cdecl FUN_10007110(int param_1,uint param_2)

{
  short sVar1;
  int iVar2;
  
  iVar2 = 0;
  if ((param_2 & 1) != 0) {
    iVar2 = (int)*(short *)(&DAT_100aa178 + param_1 * 0x3dc);
  }
  if ((((param_2 & 2) == 0) || ((param_2 & 4) == 0)) ||
     (sVar1 = *(short *)(&DAT_100aa180 + param_1 * 0x3dc), sVar1 == -1)) {
    if ((param_2 & 2) != 0) {
      iVar2 = iVar2 + *(short *)(&DAT_100aa17a + param_1 * 0x3dc);
    }
    if ((param_2 & 4) == 0) goto LAB_10007195;
    sVar1 = *(short *)(&DAT_100aa17c + param_1 * 0x3dc);
  }
  iVar2 = iVar2 + sVar1;
LAB_10007195:
  if ((param_2 & 8) == 0) {
    if (param_1 == 0x24) {
      iVar2 = iVar2 + 1;
    }
    return iVar2;
  }
  return iVar2 + *(short *)(&DAT_100aa17e + param_1 * 0x3dc);
}


// ===== FUN @ 1003df60 size=63 =====

int __cdecl FUN_1003df60(int param_1)

{
  int iVar1;
  
  iVar1 = (**(code **)(DAT_100d5c7c + 4))();
  if (iVar1 != 0) {
    DAT_10189f9d = DAT_100d6078;
  }
  if ((DAT_10189f9d != '\0') && (DAT_10190b18 != 5)) {
    return param_1 + 0x28c;
  }
  return param_1 + 0x252;
}


// ===== FUN @ 1001c5c0 size=31 =====

undefined4 __cdecl FUN_1001c5c0(int param_1)

{
  if ((DAT_10190b18 == 5) && (DAT_10190b20 == param_1)) {
    return 1;
  }
  return 0;
}


// ===== FUN @ 1001c5e0 size=49 =====

undefined4 __cdecl FUN_1001c5e0(int param_1)

{
  int iVar1;
  undefined4 uVar2;
  
  iVar1 = (**(code **)(DAT_100d5c78 + 0x10))(param_1 + -1);
  if (iVar1 == 0) {
    uVar2 = FUN_1001c5c0(param_1);
    if ((char)uVar2 == '\0') {
      return 0;
    }
  }
  return 1;
}


// ===== FUN @ 1001c620 size=258 =====

void __cdecl FUN_1001c620(int param_1,float *param_2,float *param_3)

{
  int iVar1;
  undefined4 uVar2;
  int iVar3;
  float local_30;
  float local_2c;
  float local_28;
  float local_24;
  float local_20;
  float local_1c;
  float local_c [3];
  
  local_30 = 0.0;
  iVar3 = *(int *)(param_1 + 4);
  local_2c = 0.0;
  local_28 = 28.0;
  iVar1 = FUN_1001c5a0(iVar3);
  if (iVar1 != 0) {
    iVar1 = FUN_1001c5e0(iVar3);
    if (iVar1 != 0) {
      uVar2 = FUN_1001c5c0(iVar3);
      if ((char)uVar2 == '\0') {
        (**(code **)(DAT_100d5c78 + 0x18))(&local_30);
        goto LAB_1001c6fc;
      }
    }
    FUN_10068e20(iVar3,param_2,local_c);
    local_24 = *param_2;
    local_20 = param_2[1];
    local_1c = param_2[2];
    iVar3 = (*DAT_100d5bfc)(iVar3);
    local_30 = local_24 - *(float *)(iVar3 + 0xb48);
    local_2c = local_20 - *(float *)(iVar3 + 0xb4c);
    local_28 = local_1c - *(float *)(iVar3 + 0xb50);
  }
LAB_1001c6fc:
  *param_2 = local_30 + *param_3;
  param_2[1] = local_2c + param_3[1];
  param_2[2] = local_28 + param_3[2];
  return;
}


// ===== FUN @ 10045df0 size=735 =====

undefined1 __cdecl
FUN_10045df0(undefined4 param_1,int param_2,undefined4 param_3,undefined4 param_4)

{
  undefined4 *puVar1;
  char cVar2;
  undefined1 uVar3;
  int iVar4;
  int iVar5;
  char *pcVar6;
  uint uVar7;
  uint uVar8;
  byte *pbVar9;
  char *pcVar10;
  byte *pbVar11;
  longlong lVar12;
  undefined4 uStack_98;
  char *apcStack_94 [4];
  int local_84;
  char acStack_80 [2];
  char acStack_7e [62];
  byte abStack_40 [15];
  undefined1 uStack_31;
  
  local_84 = param_2 + 0x14;
  iVar4 = (*DAT_100d5c0c)(local_84,0);
  if (iVar4 == -6) {
    return 0;
  }
  iVar4 = (**(code **)(DAT_100d5c78 + 0x20))(param_2);
  uVar3 = 0;
  if ((iVar4 < 1) || (iVar5 = (*DAT_100d5bb8)(), iVar5 < iVar4)) {
    pcVar6 = (char *)(**(code **)(DAT_100d5c78 + 0x4c))
                               (*(undefined4 *)(param_2 + 0x30),param_3,param_4);
    if (pcVar6 != (char *)0x0) {
      uVar7 = 0xffffffff;
      do {
        pcVar10 = pcVar6;
        if (uVar7 == 0) break;
        uVar7 = uVar7 - 1;
        pcVar10 = pcVar6 + 1;
        cVar2 = *pcVar6;
        pcVar6 = pcVar10;
      } while (cVar2 != '\0');
      uVar7 = ~uVar7;
      pcVar6 = pcVar10 + -uVar7;
      pcVar10 = acStack_80;
      for (uVar8 = uVar7 >> 2; uVar8 != 0; uVar8 = uVar8 - 1) {
        *(undefined4 *)pcVar10 = *(undefined4 *)pcVar6;
        pcVar6 = pcVar6 + 4;
        pcVar10 = pcVar10 + 4;
      }
      for (uVar7 = uVar7 & 3; uVar7 != 0; uVar7 = uVar7 - 1) {
        *pcVar10 = *pcVar6;
        pcVar6 = pcVar6 + 1;
        pcVar10 = pcVar10 + 1;
      }
      pcVar6 = acStack_80;
      if ((acStack_80[0] == '-') || (acStack_80[0] == '+')) {
        pcVar6 = acStack_80 + 2;
      }
      cVar2 = *pcVar6;
      if ((((cVar2 == '{') || (cVar2 == '!')) || (cVar2 == '~')) || (cVar2 == ' ')) {
        pcVar6 = pcVar6 + 1;
      }
      uVar7 = 0xffffffff;
      do {
        pcVar10 = pcVar6;
        if (uVar7 == 0) break;
        uVar7 = uVar7 - 1;
        pcVar10 = pcVar6 + 1;
        cVar2 = *pcVar6;
        pcVar6 = pcVar10;
      } while (cVar2 != '\0');
      uVar7 = ~uVar7;
      pbVar9 = (byte *)(pcVar10 + -uVar7);
      pbVar11 = abStack_40;
      for (uVar8 = uVar7 >> 2; uVar8 != 0; uVar8 = uVar8 - 1) {
        *(undefined4 *)pbVar11 = *(undefined4 *)pbVar9;
        pbVar9 = pbVar9 + 4;
        pbVar11 = pbVar11 + 4;
      }
      for (uVar7 = uVar7 & 3; uVar7 != 0; uVar7 = uVar7 - 1) {
        *pbVar11 = *pbVar9;
        pbVar9 = pbVar9 + 1;
        pbVar11 = pbVar11 + 1;
      }
      uStack_31 = 0;
      uVar3 = FUN_1002a340(abStack_40);
    }
  }
  else {
    uVar3 = 0x46;
  }
  switch(uVar3) {
  case 0x44:
    uStack_98 = 0x3f666666;
    apcStack_94[0] = s_player_pl_dirt1_wav_10094afc;
    apcStack_94[1] = s_player_pl_dirt2_wav_10094ad4;
    apcStack_94[2] = s_player_pl_dirt3_wav_10094ae8;
    iVar4 = 3;
    break;
  default:
    uStack_98 = 0x3f666666;
    apcStack_94[0] = s_player_pl_step1_wav_10094888;
    apcStack_94[1] = s_player_pl_step2_wav_10094860;
    goto LAB_1004605c;
  case 0x46:
    return uVar3;
  case 0x47:
    uStack_98 = 0x3f666666;
    apcStack_94[0] = s_player_pl_grate1_wav_10094a58;
    apcStack_94[1] = s_player_pl_grate4_wav_10094a10;
    goto LAB_1004605c;
  case 0x4d:
    uStack_98 = 0x3f666666;
    apcStack_94[0] = s_player_pl_metal1_wav_10094b58;
    apcStack_94[1] = s_player_pl_metal2_wav_10094b28;
    goto LAB_1004605c;
  case 0x50:
  case 0x59:
    uStack_98 = 0x3f4ccccd;
    apcStack_94[0] = s_debris_glass1_wav_10095594;
    apcStack_94[1] = s_debris_glass2_wav_10095580;
    apcStack_94[2] = s_debris_glass3_wav_1009556c;
    iVar4 = 3;
    break;
  case 0x53:
    uStack_98 = 0x3f666666;
    apcStack_94[0] = s_player_pl_slosh1_wav_10094994;
    apcStack_94[1] = s_player_pl_slosh3_wav_1009497c;
    apcStack_94[2] = s_player_pl_slosh2_wav_10094964;
    apcStack_94[3] = s_player_pl_slosh4_wav_1009494c;
    iVar4 = 4;
    break;
  case 0x54:
    uStack_98 = 0x3f4ccccd;
    apcStack_94[0] = s_player_pl_tile1_wav_100949fc;
    apcStack_94[1] = s_player_pl_tile3_wav_100949e8;
    apcStack_94[2] = s_player_pl_tile2_wav_100949d4;
    apcStack_94[3] = s_player_pl_tile4_wav_100949c0;
    iVar4 = 4;
    break;
  case 0x56:
    uStack_98 = 0x3f000000;
    apcStack_94[0] = s_player_pl_duct1_wav_10094aac;
    apcStack_94[1] = s_player_pl_duct1_wav_10094aac;
LAB_1004605c:
    iVar4 = 2;
    break;
  case 0x57:
    uStack_98 = 0x3f666666;
    apcStack_94[0] = s_debris_wood1_wav_100955d0;
    apcStack_94[1] = s_debris_wood2_wav_100955bc;
    apcStack_94[2] = s_debris_wood3_wav_100955a8;
    iVar4 = 3;
  }
  iVar5 = (*DAT_100d5c38)(0xfffffffb,5);
  lVar12 = __ftol();
  puVar1 = (undefined4 *)(DAT_100d5c78 + 4);
  iVar4 = (*DAT_100d5c38)(0,iVar4 + -1,uStack_98,0x3f4ccccd,0,iVar5 + (0x19 - (int)lVar12));
  (*(code *)*puVar1)(0,local_84,6,apcStack_94[iVar4]);
  return uVar3;
}


// ===== FUN @ 10046f50 size=470 =====

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void __cdecl
FUN_10046f50(float param_1,float param_2,float param_3,float param_4,float param_5,float param_6,
            byte *param_7,short param_8,short param_9)

{
  float fVar1;
  double dVar2;
  int iVar3;
  undefined4 *puVar4;
  int iVar5;
  
  iVar3 = FUN_100509d0(&DAT_1018bde8,param_7,param_8);
  if (iVar3 != -1) {
    puVar4 = FUN_1004ffe0();
    *(short *)(puVar4 + 0x45) = (short)iVar3;
    puVar4[0x34] = 0;
    *(undefined1 *)((int)puVar4 + 7) = 0;
    puVar4[0xe] = param_4 + param_4;
    *(undefined1 *)((int)puVar4 + 6) = 0;
    puVar4[0xf] = param_5 + param_5;
    *(undefined1 *)((int)puVar4 + 5) = 0;
    *(undefined1 *)puVar4 = 1;
    puVar4[0x10] = param_6 + param_6;
    puVar4[0x38] = 0x3d4ccccd;
    puVar4[0x42] = (float)(int)param_9;
    puVar4[0x37] = 0x3f333333;
    puVar4[0x3f] = 0;
    puVar4[0x36] = 0;
    *(undefined1 *)(puVar4 + 1) = 0;
    _param_8 = 0;
    if (param_9 < 0x19 != 0xffffffff) {
      dVar2 = (double)(int)param_9 * _DAT_10079e90;
      do {
        iVar3 = _param_8 + 1;
        puVar4[0x42] = (float)puVar4[0x42] - (float)_param_8;
        fVar1 = (float)iVar3 * (float)dVar2;
        puVar4[5] = param_1 + param_4 * fVar1;
        puVar4[6] = param_2 + param_5 * fVar1;
        puVar4[7] = param_3 + fVar1 * param_6;
        iVar5 = FUN_100509a0(&DAT_1018bde8,(int)*(short *)(puVar4 + 0x45));
        puVar4[0x3e] = (float)iVar5;
        if (_param_8 < 1) {
          puVar4[0x39] = 0;
        }
        else {
          puVar4[0x39] = 0x3f800000;
        }
        iVar5 = FUN_10050b60(&DAT_1018bde8,puVar4,0,0);
      } while ((iVar5 != -1) && (_param_8 = iVar3, iVar3 < (int)((param_9 < 0x19) + 1)));
    }
  }
  return;
}


// ===== FUN @ 100492b0 size=689 =====

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void __cdecl
FUN_100492b0(float param_1,float param_2,float param_3,float param_4,float param_5,float param_6,
            int param_7,float param_8,float param_9)

{
  float *pfVar1;
  float fVar2;
  float fVar3;
  float fVar4;
  float fVar5;
  float fVar6;
  undefined4 uVar7;
  int iVar8;
  undefined4 *puVar9;
  float10 fVar10;
  longlong lVar11;
  int iStack_1c;
  
  uVar7 = FUN_10057910(&param_1,0x200);
  if ((((char)uVar7 == '\0') && (uVar7 = FUN_100570b0(&param_1), (char)uVar7 != '\0')) &&
     (iVar8 = FUN_100509d0(&DAT_1018bde8,(byte *)s_sprites_sparks_spr_1009585c,5), iVar8 != -1)) {
    puVar9 = FUN_1004ffe0();
    fVar2 = (float)puVar9[0x42];
    *(short *)(puVar9 + 0x45) = (short)iVar8;
    puVar9[0x34] = 0x3f800000;
    *(undefined1 *)((int)puVar9 + 7) = 0;
    *(undefined1 *)((int)puVar9 + 6) = 0;
    *(undefined1 *)((int)puVar9 + 5) = 0;
    *(undefined1 *)puVar9 = 1;
    puVar9[0x38] = 0x3d4ccccd;
    puVar9[0x37] = 0x3f666666;
    puVar9[0x3f] = 0;
    puVar9[0x4a] = 0x3eb33333;
    puVar9[0x3d] = 0x3dcccccd;
    puVar9[0x48] = 2;
    puVar9[5] = param_1 + param_4 * fVar2;
    puVar9[6] = param_2 + param_5 * fVar2;
    puVar9[7] = param_3 + fVar2 * param_6;
    param_7 = (*DAT_100d5c38)((int)(short)param_7 / 2,(int)(short)param_7);
    lVar11 = __ftol();
    fVar2 = param_9;
    param_7 = (int)lVar11;
    iStack_1c = 0;
    if (0 < param_7) {
      fVar4 = param_4 * _DAT_1007d75c;
      fVar5 = param_5 * _DAT_1007d75c;
      pfVar1 = (float *)(puVar9 + 0xe);
      fVar6 = param_6 * _DAT_1007d75c;
      fVar3 = -param_9;
      param_4 = fVar3;
      do {
        fVar10 = (float10)(*DAT_100d5c34)(0x3f000000,0x3f933333);
        *pfVar1 = fVar4;
        puVar9[0x42] = (float)(fVar10 * (float10)param_8);
        puVar9[0xf] = fVar5;
        puVar9[0x10] = fVar6;
        fVar10 = (float10)(*DAT_100d5c34)(fVar3,fVar2);
        *pfVar1 = (float)(fVar10 + (float10)*pfVar1);
        fVar10 = (float10)(*DAT_100d5c34)(fVar3,fVar2);
        puVar9[0xf] = (float)(fVar10 + (float10)(float)puVar9[0xf]);
        fVar10 = (float10)(*DAT_100d5c34)(0,fVar2);
        puVar9[0x10] = (float)(fVar10 + (float10)(float)puVar9[0x10]);
        fVar10 = (float10)(*DAT_100d5c34)(0x3dcccccd,0x3f800000);
        puVar9[0x36] = (float)fVar10;
        if ((fVar10 <= (float10)_DAT_1007d6f0) || (_DAT_1018ca50 <= _DAT_1007d758)) {
          *(undefined1 *)(puVar9 + 1) = 0;
        }
        else {
          *(undefined1 *)(puVar9 + 1) = 3;
        }
        param_9 = (float)FUN_100509a0(&DAT_1018bde8,(int)*(short *)(puVar9 + 0x45));
        puVar9[0x3e] = (float)(int)param_9;
        iVar8 = FUN_10050b60(&DAT_1018bde8,puVar9,0,0);
      } while ((iVar8 != -1) && (iStack_1c = iStack_1c + 1, iStack_1c < param_7));
    }
  }
  return;
}


// ===== FUN @ 100464f0 size=393 =====

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void __cdecl
FUN_100464f0(float param_1,float param_2,float param_3,float param_4,float param_5,float param_6,
            byte *param_7,undefined4 param_8,short param_9,int param_10)

{
  float fVar1;
  undefined4 uVar2;
  int iVar3;
  undefined4 *puVar4;
  int iVar5;
  float10 fVar6;
  float local_c;
  float local_8;
  float local_4;
  
  uVar2 = FUN_10050ad0(&DAT_1018bde8,0);
  if ((char)uVar2 != '\0') {
    fVar6 = FUN_10056fa0(DAT_1018ca48);
    FUN_10050b00(&DAT_1018bde8,0,(float)fVar6);
    uVar2 = FUN_100570b0(&param_1);
    if ((char)uVar2 != '\0') {
      uVar2 = FUN_10057270(&local_c);
      iVar3 = FUN_100572a0(&param_1,uVar2);
      if ((char)iVar3 != '\0') {
        iVar3 = FUN_100509d0(&DAT_1018bde8,param_7,(short)param_8);
        if (iVar3 != -1) {
          puVar4 = FUN_1004ffe0();
          iVar5 = (int)(short)param_10;
          param_10 = (int)param_9;
          *(undefined1 *)((int)puVar4 + 5) = 0;
          *(undefined1 *)puVar4 = 1;
          puVar4[0x38] = 0;
          puVar4[0x37] = 0x3f800000;
          puVar4[0x3f] = (float)iVar5;
          *(short *)(puVar4 + 0x45) = (short)iVar3;
          puVar4[0x36] = 0;
          puVar4[0x3e] = 0;
          puVar4[0x42] = (float)param_10;
          fVar1 = (float)param_10 * (float)_DAT_10079e90;
          local_c = param_4 * fVar1;
          param_4 = param_1 + local_c;
          param_5 = param_2 + param_5 * fVar1;
          *(undefined1 *)(puVar4 + 1) = 0;
          param_6 = param_3 + fVar1 * param_6;
          puVar4[0xe] = 0;
          puVar4[5] = param_4;
          puVar4[6] = param_5;
          puVar4[7] = param_6;
          puVar4[0xf] = 0;
          puVar4[0x10] = 0xc0000000;
          puVar4[0x39] = 0x3f000000;
          puVar4[0x3d] = 0x40533333;
          puVar4[0x4a] = 0x40e00000;
          local_8 = param_5;
          local_4 = param_6;
          FUN_10050b60(&DAT_1018bde8,puVar4,0,0);
        }
      }
    }
  }
  return;
}


// ===== FUN @ 10002610 size=25 =====

void __thiscall FUN_10002610(void *this,undefined4 *param_1)

{
  *(undefined4 *)this = *param_1;
  *(undefined4 *)((int)this + 4) = param_1[1];
  *(undefined4 *)((int)this + 8) = param_1[2];
  return;
}


