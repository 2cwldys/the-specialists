// ===== FUN @ 10043c90 size=1185 =====

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void FUN_10043c90(int param_1)

{
  undefined1 uVar1;
  bool bVar2;
  undefined3 extraout_var;
  float fVar3;
  float fVar4;
  undefined *puVar5;
  undefined *puVar6;
  float fVar7;
  float fVar8;
  char *pcVar9;
  int iVar10;
  short sVar11;
  float fVar12;
  short sVar13;
  float fVar14;
  
  if ((DAT_10189e8c & 1) == 0) {
    DAT_10189e8c = DAT_10189e8c | 1;
    FUN_1006af63(0x10044220);
  }
  if ((DAT_10189e8c & 2) == 0) {
    DAT_10189e8c = DAT_10189e8c | 2;
    FUN_1006af63(0x10044210);
  }
  if ((DAT_10189e8c & 4) == 0) {
    DAT_10189e8c = DAT_10189e8c | 4;
    FUN_1006af63(0x10044200);
  }
  if ((DAT_10189e8c & 8) == 0) {
    DAT_10189e8c = DAT_10189e8c | 8;
    FUN_1006af63(0x100441f0);
  }
  if ((DAT_10189e8c & 0x10) == 0) {
    DAT_10189e8c = DAT_10189e8c | 0x10;
    FUN_1006af63(0x100441e0);
  }
  if ((DAT_10189e8c & 0x20) == 0) {
    DAT_10189e8c = DAT_10189e8c | 0x20;
    FUN_1006af63(0x100441d0);
  }
  if ((DAT_10189e8c & 0x40) == 0) {
    DAT_10189e8c = DAT_10189e8c | 0x40;
    FUN_1006af63(0x100441c0);
  }
  if ((DAT_10189e8c & 0x80) == 0) {
    DAT_10189e8c = DAT_10189e8c | 0x80;
    FUN_1006af63(0x100441b0);
  }
  if ((DAT_10189c60 & 1) == 0) {
    DAT_10189c60 = DAT_10189c60 | 1;
    FUN_1006af63(0x100441a0);
  }
  DAT_1018a0a0 = *(float *)(param_1 + 4);
  _DAT_1018a020 = *(undefined4 *)(param_1 + 8);
  _DAT_1018a024 = *(undefined4 *)(param_1 + 0xc);
  _DAT_1018a028 = *(undefined4 *)(param_1 + 0x10);
  _DAT_10189e70 = *(undefined4 *)(param_1 + 0x14);
  _DAT_10189e74 = *(undefined4 *)(param_1 + 0x18);
  _DAT_10189e78 = *(undefined4 *)(param_1 + 0x1c);
  _DAT_10189e90 = *(undefined4 *)(param_1 + 0x20);
  _DAT_10189e94 = *(undefined4 *)(param_1 + 0x24);
  _DAT_10189e98 = *(undefined4 *)(param_1 + 0x28);
  (*DAT_100d5b88)();
  _DAT_10189d40 = FUN_100071c0(*(int *)(param_1 + 0x38));
  puVar6 = &DAT_1018a020;
  DAT_1018a044 = (int)_DAT_10189d40 + 0x252;
  FUN_1001c620(param_1,(float *)&DAT_10189bd0,(float *)&DAT_1018a020);
  fVar3 = 1.4013e-45;
  _DAT_101899b8 = _DAT_10189e40 * _DAT_1007a8d8 + _DAT_10189bd0;
  _DAT_101899bc = _DAT_10189bd4 + _DAT_10189e44 * _DAT_1007a8d8;
  _DAT_101899c0 = _DAT_10189bd8 + _DAT_10189e48 * _DAT_1007a8d8;
  (**(code **)(DAT_100d5c78 + 0x28))(0);
  (**(code **)(DAT_100d5c78 + 0x2c))();
  (**(code **)(DAT_100d5c78 + 0x34))((int)DAT_1018a0a0 + -1);
  (**(code **)(DAT_100d5c78 + 0x38))(2);
  (**(code **)(DAT_100d5c78 + 0x3c))(&DAT_10189bd0,&DAT_101899b8,2,0xffffffff,&DAT_101899d8);
  DAT_10189cc4 = *(undefined4 *)(param_1 + 0x3c);
  if (_DAT_101899e8 == (float)_DAT_10079e80) {
    (*DAT_100d5c38)();
    __ftol();
    iVar10 = DAT_100d5c78;
    (*DAT_100d5c34)();
LAB_10044097:
    (**(code **)(iVar10 + 4))();
  }
  else {
    fVar7 = 2.6081256e-29;
    fVar8 = DAT_1018a0a0;
    uVar1 = FUN_10045df0(DAT_1018a0a0,0x101899d8,&DAT_10189bd0,&DAT_101899b8);
    switch(uVar1) {
    case 0:
    case 0x44:
    case 0x57:
      break;
    default:
      sVar13 = 6;
      sVar11 = 5;
      pcVar9 = s_sprites_scintille_metal_spr_10095460;
      puVar5 = &DAT_101899f8;
      fVar4 = 2.608144e-29;
      FUN_10002610(&stack0xffffffc4,(undefined4 *)&DAT_101899f8);
      FUN_10002610(&stack0xffffffb8,(undefined4 *)&DAT_101899ec);
      FUN_10046f50(fVar3,fVar4,(float)puVar5,(float)puVar6,fVar7,fVar8,(byte *)pcVar9,sVar11,sVar13)
      ;
      fVar14 = 25.0;
      fVar12 = 1.15;
      iVar10 = 7;
      puVar5 = &DAT_101899f8;
      fVar4 = 2.608159e-29;
      FUN_10002610(&stack0xffffffc4,(undefined4 *)&DAT_101899f8);
      FUN_10002610(&stack0xffffffb8,(undefined4 *)&DAT_101899ec);
      FUN_100492b0(fVar3,fVar4,(float)puVar5,(float)puVar6,fVar7,fVar8,iVar10,fVar12,fVar14);
      (*DAT_100d5c38)(0xfffffffb,5);
      __ftol();
      iVar10 = DAT_100d5c78;
      (*DAT_100d5c34)();
      goto LAB_10044097;
    case 0x46:
      DAT_10189cc4 = 1;
    }
  }
  iVar10 = FUN_1001c5e0((int)DAT_1018a0a0);
  if ((iVar10 != 0) && (bVar2 = CL_IsThirdPerson(), CONCAT31(extraout_var,bVar2) == 0)) {
    iVar10 = (*DAT_100d5c38)();
    if (iVar10 == 0) {
      DAT_10189adc = 0xf;
    }
    else if (iVar10 == 1) {
      DAT_10189adc = 0x10;
    }
    else if (iVar10 == 2) {
      DAT_10189adc = 0x11;
    }
    (**(code **)(DAT_100d5c78 + 0x40))();
  }
  (**(code **)(DAT_100d5c78 + 0x30))();
  return;
}


