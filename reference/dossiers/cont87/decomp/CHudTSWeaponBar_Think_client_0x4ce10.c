// ===== FUN @ 1004ce10 size=1344 =====

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void __fastcall FUN_1004ce10(int param_1)

{
  undefined4 *puVar1;
  float fVar2;
  bool bVar3;
  float fVar4;
  char cVar5;
  uint uVar6;
  int iVar7;
  int iVar8;
  undefined4 *puVar9;
  float10 fVar10;
  longlong lVar11;
  undefined1 *puVar12;
  char *pcVar13;
  undefined1 auStack_50 [16];
  undefined1 auStack_40 [64];
  
  lVar11 = __ftol();
  iVar7 = (int)lVar11;
  if (*(int *)(param_1 + 0x3728) != iVar7) {
    puVar12 = auStack_50;
    pcVar13 = s_lasclr__i_10095e08;
    *(int *)(param_1 + 0x3728) = iVar7;
    FUN_1006ac7a(puVar12,(byte *)s_lasclr__i_10095e08);
    (*DAT_100d5b74)(auStack_50,puVar12,pcVar13,iVar7);
  }
  fVar10 = FUN_10055ac0(DAT_100963e8);
  fVar2 = (float)((float10)_DAT_100cf950 / fVar10);
  fVar4 = *(float *)(param_1 + 0x34) - fVar2;
  if (fVar4 <= _DAT_100799f0) {
    fVar4 = _DAT_100799f0;
  }
  *(float *)(param_1 + 0x34) = fVar4;
  if (((*(float *)(DAT_100df444 + 0xc) != _DAT_100799f0) && (DAT_100d43e0 != 0)) &&
     (((*(uint *)(param_1 + 0x3724) ^ DAT_100cf970) & DAT_100cf970 & 0x20) != 0)) {
    if (DAT_100cf944 <= *(float *)(param_1 + 0x3720)) {
      (*DAT_100d5b74)(s_usepowerup_10095dfc);
    }
    *(float *)(param_1 + 0x3720) = DAT_100cf944 + (float)_DAT_10079e50;
  }
  bVar3 = *(float *)(param_1 + 0x3700) < _DAT_1007a910;
  *(uint *)(param_1 + 0x3724) = DAT_100cf970;
  if (bVar3) {
    if (_DAT_1007d788 <= *(float *)(param_1 + 0x3700)) {
      fVar2 = fVar2 * _DAT_1007a890;
    }
    *(float *)(param_1 + 0x3700) = fVar2 + *(float *)(param_1 + 0x3700);
    if (_DAT_1007a910 < *(float *)(param_1 + 0x3700)) {
      *(undefined4 *)(param_1 + 0x3700) = 0x42c80000;
    }
  }
  _DAT_1018a718 = *(undefined4 *)(param_1 + 0x3700);
  uVar6 = FUN_1004c250();
  if ((char)uVar6 == '\0') {
    cVar5 = FUN_1004c280();
    bVar3 = false;
    if (cVar5 == '\0') goto LAB_1004cf8b;
  }
  bVar3 = true;
LAB_1004cf8b:
  iVar7 = *(int *)(param_1 + 0x3708);
  if (_DAT_10079e58 <= DAT_100963e8) {
    if (iVar7 == 2) {
      *(undefined4 *)(param_1 + 0x3708) = 1;
    }
  }
  else if ((iVar7 == 0) || (iVar7 == 1)) {
    *(undefined4 *)(param_1 + 0x370c) = 0x3f800000;
    iVar7 = (*DAT_100d5c38)(0xfffffffb,5);
    lVar11 = __ftol();
    iVar8 = (*DAT_100d5bf4)();
    puVar1 = (undefined4 *)(DAT_100d5c78 + 4);
    puVar9 = (undefined4 *)
             (*DAT_100d5bf4)(iVar8 + 0x2c0,0x1f5,s_sound_player_heartbeat1_wav_10095de0,
                             *(undefined4 *)(param_1 + 0x370c),0x3f4ccccd,
                             (-(uint)(*(int *)(param_1 + 0x3708) != 1) & 0xffffffc0) + 0x40,
                             iVar7 + (0x19 - (int)lVar11));
    (*(code *)*puVar1)(*puVar9);
    *(undefined4 *)(param_1 + 0x3708) = 2;
  }
  if (*(int *)(param_1 + 0x3708) == 1) {
    fVar2 = *(float *)(param_1 + 0x370c) - (float)_DAT_10079e60;
    *(float *)(param_1 + 0x370c) = fVar2;
    if (fVar2 <= _DAT_100799f0) {
      *(undefined4 *)(param_1 + 0x3708) = 0;
    }
    else {
      iVar7 = (*DAT_100d5c38)(0xfffffffb,5);
      lVar11 = __ftol();
      iVar8 = (*DAT_100d5bf4)();
      puVar1 = (undefined4 *)(DAT_100d5c78 + 4);
      puVar9 = (undefined4 *)
               (*DAT_100d5bf4)(iVar8 + 0x2c0,0x1f5,s_sound_player_heartbeat1_wav_10095de0,
                               *(undefined4 *)(param_1 + 0x370c),0x3f4ccccd,0x40,
                               iVar7 + (0x19 - (int)lVar11));
      (*(code *)*puVar1)(*puVar9);
    }
  }
  iVar7 = *(int *)(param_1 + 0x3710);
  if (_DAT_1007cd74 <= *(float *)(param_1 + 0x3700)) {
    if (iVar7 == 2) {
      *(undefined4 *)(param_1 + 0x3710) = 1;
    }
  }
  else if ((iVar7 == 0) || (iVar7 == 1)) {
    *(undefined4 *)(param_1 + 0x3714) = 0x3f333333;
    iVar7 = (*DAT_100d5c38)(0xfffffffb,5);
    lVar11 = __ftol();
    iVar8 = (*DAT_100d5bf4)();
    puVar1 = (undefined4 *)(DAT_100d5c78 + 4);
    puVar9 = (undefined4 *)
             (*DAT_100d5bf4)(iVar8 + 0x2c0,0x1f6,s_sound_player_breathe2_wav_10095dc4,
                             *(undefined4 *)(param_1 + 0x3714),0x3f4ccccd,
                             (-(uint)(*(int *)(param_1 + 0x3710) != 1) & 0xffffffc0) + 0x40,
                             iVar7 + (0x19 - (int)lVar11));
    (*(code *)*puVar1)(*puVar9);
    *(undefined4 *)(param_1 + 0x3710) = 2;
  }
  if (*(int *)(param_1 + 0x3710) == 1) {
    fVar2 = *(float *)(param_1 + 0x3714) - (float)_DAT_1007d780;
    *(float *)(param_1 + 0x3714) = fVar2;
    if (fVar2 <= _DAT_100799f0) {
      *(undefined4 *)(param_1 + 0x3710) = 0;
    }
    else {
      iVar7 = (*DAT_100d5c38)(0xfffffffb,5);
      lVar11 = __ftol();
      iVar8 = (*DAT_100d5bf4)();
      puVar1 = (undefined4 *)(DAT_100d5c78 + 4);
      puVar9 = (undefined4 *)
               (*DAT_100d5bf4)(iVar8 + 0x2c0,0x1f6,s_sound_player_breathe2_wav_10095dc4,
                               *(undefined4 *)(param_1 + 0x3714),0x3f4ccccd,0x40,
                               iVar7 + (0x19 - (int)lVar11));
      (*(code *)*puVar1)(*puVar9);
    }
  }
  FUN_100361f0((undefined1 *)(param_1 + 0x7c0));
  if (!bVar3) {
    if (*(char *)(param_1 + 0x73c) != '\0') {
      uVar6 = FUN_1004fde0(&DAT_100cf920,1);
      if ((char)uVar6 != '\0') {
        if ((*(int *)(param_1 + 0x740) != -1) &&
           (iVar7 = *(int *)(param_1 + 0x164 +
                            (*(int *)(param_1 + 0x740) * 0xb + *(int *)(param_1 + 0x744)) * 8),
           -1 < iVar7)) {
          puVar12 = auStack_40;
          pcVar13 = s_weapon__i_10095db8;
          FUN_1006ac7a(puVar12,(byte *)s_weapon__i_10095db8);
          (*DAT_100d5b74)(auStack_40,puVar12,pcVar13,iVar7);
        }
        *(undefined1 *)(param_1 + 0x73c) = 0;
      }
      uVar6 = FUN_1004fde0(&DAT_100cf920,0x800);
      if ((char)uVar6 != '\0') {
        iVar7 = *(int *)(param_1 + 0x164 +
                        (*(int *)(param_1 + 0x740) * 0xb + *(int *)(param_1 + 0x744)) * 8);
        if (-1 < iVar7) {
          puVar12 = auStack_40;
          pcVar13 = s_drop__i_10095db0;
          FUN_1006ac7a(puVar12,(byte *)s_drop__i_10095db0);
          (*DAT_100d5b74)(auStack_40,puVar12,pcVar13,iVar7);
        }
        *(undefined1 *)(param_1 + 0x73c) = 0;
      }
    }
    if ((0 < *(int *)(param_1 + 0x7b8)) &&
       ((int)*(char *)(param_1 + 0x7bc) != (uint)*(byte *)(param_1 + 0x78a))) {
      iVar7 = (*DAT_100d5c38)(0xfffffffb,5);
      lVar11 = __ftol();
      iVar8 = (*DAT_100d5bf4)();
      puVar1 = (undefined4 *)(DAT_100d5c78 + 4);
      fVar10 = (float10)(*DAT_100d5c34)(0x3f6b851f,0x3f800000,0x3fa00000,0,
                                        iVar7 + (0x19 - (int)lVar11));
      puVar9 = (undefined4 *)
               (*DAT_100d5bf4)(iVar8 + 0x2c0,3,s_weapons_switch_wav_10095d9c,(float)fVar10);
      (*(code *)*puVar1)(*puVar9);
      *(undefined1 *)(param_1 + 0x7bc) = *(undefined1 *)(param_1 + 0x78a);
    }
  }
  return;
}


