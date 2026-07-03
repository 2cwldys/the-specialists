// ===== FUN @ 10042690 size=2813 =====

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void FUN_10042690(int param_1)

{
  undefined4 *puVar1;
  short sVar2;
  float fVar3;
  bool bVar4;
  char cVar5;
  undefined4 uVar6;
  int iVar7;
  undefined3 extraout_var;
  undefined3 extraout_var_00;
  int iVar8;
  int iVar9;
  undefined4 uVar10;
  float *pfVar11;
  float10 fVar12;
  longlong lVar13;
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
  undefined1 auStack_18 [12];
  undefined4 auStack_c [3];
  
  if ((DAT_10189acc & 1) == 0) {
    DAT_10189acc = DAT_10189acc | 1;
    FUN_1006af63(0x100431f0);
  }
  if ((DAT_10189acc & 2) == 0) {
    DAT_10189acc = DAT_10189acc | 2;
    FUN_1006af63(0x100431e0);
  }
  if ((DAT_10189acc & 4) == 0) {
    DAT_10189acc = DAT_10189acc | 4;
    FUN_1006af63(0x100431d0);
  }
  if ((DAT_10189acc & 8) == 0) {
    DAT_10189acc = DAT_10189acc | 8;
    FUN_1006af63(0x100431c0);
  }
  if ((DAT_10189acc & 0x10) == 0) {
    DAT_10189acc = DAT_10189acc | 0x10;
    FUN_1006af63(0x100431b0);
  }
  if ((DAT_10189acc & 0x20) == 0) {
    DAT_10189acc = DAT_10189acc | 0x20;
    FUN_1006af63(0x100431a0);
  }
  if ((DAT_10189acc & 0x40) == 0) {
    DAT_10189acc = DAT_10189acc | 0x40;
    FUN_1006af63(0x10043190);
  }
  DAT_10189d0c = *(int *)(param_1 + 0x38);
  DAT_10189ae4 = FUN_100071c0(*(int *)(param_1 + 0x38));
  DAT_10189c98 = *(uint *)(param_1 + 0x3c);
  lVar13 = __ftol();
  DAT_101899d0 = (int)lVar13;
  DAT_10189f40 = *(int *)(param_1 + 4);
  _DAT_10189f80 = *(undefined4 *)(param_1 + 8);
  _DAT_10189f84 = *(undefined4 *)(param_1 + 0xc);
  _DAT_10189f88 = *(undefined4 *)(param_1 + 0x10);
  _DAT_10189c88 = *(undefined4 *)(param_1 + 0x14);
  _DAT_10189c8c = *(undefined4 *)(param_1 + 0x18);
  _DAT_10189c90 = *(undefined4 *)(param_1 + 0x1c);
  _DAT_10189f10 = *(float *)(param_1 + 0x20);
  _DAT_10189f14 = *(float *)(param_1 + 0x24);
  _DAT_10189f18 = *(float *)(param_1 + 0x28);
  (*DAT_100d5b88)(&DAT_10189c88,&DAT_10189de8,&DAT_10189d98,&DAT_10189cd8);
  uVar6 = FUN_1001c5c0(DAT_10189f40);
  if (((char)uVar6 != '\0') || (iVar7 = FUN_1001c5e0(DAT_10189f40), iVar7 != 0)) {
    DAT_10189d08 = FUN_1003df60((int)DAT_10189ae4);
    _DAT_10189c58 = FUN_10007110(DAT_10189d0c,DAT_10189c98);
    if (DAT_101899d0 == 0) {
      sVar2 = *(short *)(DAT_10189d08 + 2);
    }
    else {
      sVar2 = *(short *)(DAT_10189d08 + DAT_101899d0 * 2);
    }
    (**(code **)(DAT_100d5c78 + 0x40))((int)sVar2,_DAT_10189c58);
  }
  if (DAT_101899d0 == 9) {
    iVar7 = (*DAT_100d5c38)(0xfffffffb,5);
    lVar13 = __ftol();
    puVar1 = (undefined4 *)(DAT_100d5c78 + 4);
    fVar12 = (float10)(*DAT_100d5c34)(0x3f6b851f,0x3f800000,0x3f4ccccd,0,
                                      iVar7 + (0x19 - (int)lVar13));
    (*(code *)*puVar1)(DAT_10189f40,&DAT_10189f80,1,s_weapons_insert_shell_wav_100953a4,
                       (float)fVar12);
  }
  if ((DAT_101899d0 == 10) &&
     ((bVar4 = CL_IsThirdPerson(), CONCAT31(extraout_var,bVar4) != 0 ||
      (iVar7 = FUN_1001c5e0(DAT_10189f40), iVar7 == 0)))) {
    iVar7 = (*DAT_100d5c38)(0xfffffffb,5);
    lVar13 = __ftol();
    puVar1 = (undefined4 *)(DAT_100d5c78 + 4);
    fVar12 = (float10)(*DAT_100d5c34)(0x3f6b851f,0x3f800000,0x3f4ccccd,0,
                                      iVar7 + (0x19 - (int)lVar13));
    (*(code *)*puVar1)(DAT_10189f40,&DAT_10189f80,1,(int)DAT_10189ae4 + 0x306,(float)fVar12);
  }
  if (DAT_101899d0 == 0) {
    bVar4 = CL_IsThirdPerson();
    if ((CONCAT31(extraout_var_00,bVar4) == 0) && (iVar7 = FUN_1001c5e0(DAT_10189f40), iVar7 != 0))
    {
      return;
    }
    iVar7 = (*DAT_100d5c38)(0xfffffffb,5);
    lVar13 = __ftol();
    puVar1 = (undefined4 *)(DAT_100d5c78 + 4);
    fVar12 = (float10)(*DAT_100d5c34)(0x3f6b851f,0x3f800000,0x3f4ccccd,0,
                                      iVar7 + (0x19 - (int)lVar13));
    (*(code *)*puVar1)(DAT_10189f40,&DAT_10189f80,1,s_weapons_generic_reload_wav_10095388,
                       (float)fVar12);
    cVar5 = FUN_100579a0(DAT_10189f40);
    if (cVar5 != '\0') {
      if (DAT_10189d0c == 0x21) {
        uVar6 = (**(code **)(DAT_100d5c78 + 0xc))(s_models_shotgun_shell_mdl_10092510);
        iVar7 = 2;
        do {
          iVar8 = (*DAT_100d5c38)(0xffffffe2,0x1e);
          iVar9 = (*DAT_100d5c38)(0,0x1e);
          fStack_94 = (float)iVar9;
          fStack_9c = fStack_94 * _DAT_10189cd8;
          fStack_98 = _DAT_10189cdc * fStack_94;
          fStack_94 = _DAT_10189ce0 * fStack_94;
          FUN_10002610(&fStack_78,&fStack_9c);
          fStack_48 = _DAT_10189de8 * _DAT_10079e88;
          fStack_44 = _DAT_10189dec * _DAT_10079e88;
          fStack_40 = _DAT_10189df0 * _DAT_10079e88;
          FUN_10002610(&fStack_54,&fStack_48);
          fStack_34 = (float)iVar8;
          fStack_3c = fStack_34 * _DAT_10189d98;
          fStack_38 = _DAT_10189d9c * fStack_34;
          fStack_34 = _DAT_10189da0 * fStack_34;
          FUN_10002610(&fStack_24,&fStack_3c);
          fStack_30 = fStack_24 + _DAT_10189f10;
          fStack_2c = _DAT_10189f14 + fStack_20;
          fStack_28 = _DAT_10189f18 + fStack_1c;
          FUN_10002610(&fStack_90,&fStack_30);
          fStack_84 = fStack_54 + fStack_90;
          fStack_80 = fStack_50 + fStack_8c;
          fStack_7c = fStack_4c + fStack_88;
          FUN_10002610(&fStack_60,&fStack_84);
          fStack_6c = fStack_78 + fStack_60;
          fStack_68 = fStack_74 + fStack_5c;
          fStack_64 = fStack_70 + fStack_58;
          FUN_10002610(auStack_18,&fStack_6c);
          uVar10 = FUN_10056be0(auStack_c,DAT_10189f40,0);
          FUN_10056890(uVar10,(float *)&DAT_10189c88,auStack_18,&DAT_10189f10,uVar6,2);
          iVar7 = iVar7 + -1;
        } while (iVar7 != 0);
      }
      else {
        if (DAT_10189d0c == 0x1f) {
          uVar6 = (**(code **)(DAT_100d5c78 + 0xc))(s_models_9mm_shell_mdl_10091524);
          iVar7 = 5;
          do {
            iVar8 = (*DAT_100d5c38)(0xffffffe2,0x1e);
            iVar9 = (*DAT_100d5c38)(0,0x1e);
            fStack_64 = (float)iVar9;
            fStack_6c = fStack_64 * _DAT_10189cd8;
            fStack_68 = _DAT_10189cdc * fStack_64;
            fStack_64 = _DAT_10189ce0 * fStack_64;
            FUN_10002610(&fStack_3c,&fStack_6c);
            fStack_60 = _DAT_10189de8 * _DAT_10079e88;
            fStack_5c = _DAT_10189dec * _DAT_10079e88;
            fStack_58 = _DAT_10189df0 * _DAT_10079e88;
            FUN_10002610(&fStack_54,&fStack_60);
            fStack_70 = (float)iVar8;
            fStack_78 = fStack_70 * _DAT_10189d98;
            fStack_74 = _DAT_10189d9c * fStack_70;
            fStack_70 = _DAT_10189da0 * fStack_70;
            FUN_10002610(&fStack_84,&fStack_78);
            fStack_90 = fStack_84 + _DAT_10189f10;
            fStack_8c = _DAT_10189f14 + fStack_80;
            fStack_88 = _DAT_10189f18 + fStack_7c;
            FUN_10002610(&fStack_30,&fStack_90);
            fStack_24 = fStack_54 + fStack_30;
            fStack_20 = fStack_50 + fStack_2c;
            fStack_1c = fStack_4c + fStack_28;
            FUN_10002610(&fStack_48,&fStack_24);
            fStack_9c = fStack_3c + fStack_48;
            fStack_98 = fStack_38 + fStack_44;
            fStack_94 = fStack_34 + fStack_40;
            FUN_10002610(auStack_18,&fStack_9c);
            uVar10 = FUN_10056be0(auStack_c,DAT_10189f40,0);
            FUN_10056890(uVar10,(float *)&DAT_10189c88,auStack_18,&DAT_10189f10,uVar6,1);
            iVar7 = iVar7 + -1;
          } while (iVar7 != 0);
          return;
        }
        if (DAT_10189d0c == 0x24) {
          uVar6 = (**(code **)(DAT_100d5c78 + 0xc))(s_models_556_shell_mdl_10091264);
          iVar7 = (*DAT_100d5c38)(0xffffffe2,0x1e);
          iVar8 = (*DAT_100d5c38)(0,0x1e);
          fStack_64 = (float)iVar8;
          fStack_6c = fStack_64 * _DAT_10189cd8;
          fStack_68 = _DAT_10189cdc * fStack_64;
          fStack_64 = _DAT_10189ce0 * fStack_64;
          FUN_10002610(&fStack_3c,&fStack_6c);
          fStack_60 = _DAT_10189de8 * _DAT_10079e88;
          fStack_5c = _DAT_10189dec * _DAT_10079e88;
          fStack_58 = _DAT_10189df0 * _DAT_10079e88;
          FUN_10002610(&fStack_54,&fStack_60);
          fStack_70 = (float)iVar7;
          fStack_78 = fStack_70 * _DAT_10189d98;
          fStack_74 = _DAT_10189d9c * fStack_70;
          fStack_70 = _DAT_10189da0 * fStack_70;
          FUN_10002610(&fStack_84,&fStack_78);
          fStack_90 = fStack_84 + _DAT_10189f10;
          fStack_8c = _DAT_10189f14 + fStack_80;
          fStack_88 = _DAT_10189f18 + fStack_7c;
          FUN_10002610(&fStack_30,&fStack_90);
          fStack_24 = fStack_54 + fStack_30;
          fStack_20 = fStack_50 + fStack_2c;
          fStack_1c = fStack_4c + fStack_28;
          FUN_10002610(&fStack_48,&fStack_24);
          fStack_9c = fStack_3c + fStack_48;
          fStack_98 = fStack_38 + fStack_44;
          fStack_94 = fStack_34 + fStack_40;
          FUN_10002610(auStack_18,&fStack_9c);
          uVar10 = FUN_10056be0(auStack_c,DAT_10189f40,0);
          FUN_10056890(uVar10,(float *)&DAT_10189c88,auStack_18,&DAT_10189f10,uVar6,1);
          return;
        }
        iVar7 = 0;
        while( true ) {
          if ((*(char *)(DAT_10189ae4 + 0xd2) == '\x04') ||
             (iVar8 = 1, *(char *)(DAT_10189ae4 + 0xd2) == '\x05')) {
            iVar8 = 2;
          }
          if (iVar8 <= iVar7) break;
          fVar12 = (float10)(*DAT_100d5c34)(0xc2700000,0x42700000);
          _DAT_10189dcc = (float)fVar12;
          fVar12 = (float10)(*DAT_100d5c34)(0xc2700000,0x42700000);
          _DAT_10189dc8 = (float)fVar12;
          fVar12 = (float10)(*DAT_100d5c34)(0xc2700000,0x42700000);
          _DAT_10189dd0 = (float)fVar12;
          DAT_1018a03c = (**(code **)(DAT_100d5c78 + 0xc))(s_models_generic_clip_mdl_10095370);
          fVar3 = (float)(int)((-(uint)(iVar7 != 0) & 0xffffff88) + 0x3c);
          fStack_6c = fVar3 * _DAT_10189d98;
          fStack_68 = _DAT_10189d9c * fVar3;
          fStack_84 = fStack_6c + _DAT_10189f10;
          fStack_80 = _DAT_10189f14 + fStack_68;
          fStack_7c = _DAT_10189f18 + fVar3 * _DAT_10189da0;
          fStack_9c = _DAT_10189de8 * _DAT_1007ccf0;
          fStack_98 = _DAT_10189dec * _DAT_1007ccf0;
          fStack_94 = _DAT_10189df0 * _DAT_1007ccf0;
          fStack_5c = fStack_80;
          fStack_58 = fStack_7c;
          pfVar11 = (float *)FUN_10056be0(auStack_c,DAT_10189f40,(byte)-(iVar7 != 0) & 2);
          fStack_90 = *pfVar11 - fStack_9c;
          fStack_8c = pfVar11[1] - fStack_98;
          fStack_88 = pfVar11[2] - fStack_94;
          fStack_74 = fStack_8c;
          fStack_70 = fStack_88;
          (**(code **)(DAT_100d5c74 + 0xc0))
                    (&fStack_90,&fStack_84,&DAT_10189dc8,0x40000000,DAT_1018a03c,0);
          iVar7 = iVar7 + 1;
        }
      }
    }
  }
  return;
}


