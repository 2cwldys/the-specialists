// ===== FUN @ 10044230 size=1228 =====

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void FUN_10044230(int param_1)

{
  undefined4 *puVar1;
  undefined1 uVar2;
  bool bVar3;
  undefined3 extraout_var;
  undefined *puVar4;
  float fVar5;
  undefined *puVar6;
  float fVar7;
  float fVar8;
  float fVar9;
  char *pcVar10;
  int iVar11;
  short sVar12;
  float fVar13;
  short sVar14;
  float fVar15;
  
  if ((DAT_10189fa8 & 1) == 0) {
    DAT_10189fa8 = DAT_10189fa8 | 1;
    FUN_1006af63(0x100447e0);
  }
  if ((DAT_10189fa8 & 2) == 0) {
    DAT_10189fa8 = DAT_10189fa8 | 2;
    FUN_1006af63(0x100447d0);
  }
  if ((DAT_10189fa8 & 4) == 0) {
    DAT_10189fa8 = DAT_10189fa8 | 4;
    FUN_1006af63(0x100447c0);
  }
  if ((DAT_10189fa8 & 8) == 0) {
    DAT_10189fa8 = DAT_10189fa8 | 8;
    FUN_1006af63(0x100447b0);
  }
  if ((DAT_10189fa8 & 0x10) == 0) {
    DAT_10189fa8 = DAT_10189fa8 | 0x10;
    FUN_1006af63(0x100447a0);
  }
  if ((DAT_10189fa8 & 0x20) == 0) {
    DAT_10189fa8 = DAT_10189fa8 | 0x20;
    FUN_1006af63(0x10044790);
  }
  if ((DAT_10189fa8 & 0x40) == 0) {
    DAT_10189fa8 = DAT_10189fa8 | 0x40;
    FUN_1006af63(0x10044780);
  }
  if ((DAT_10189fa8 & 0x80) == 0) {
    DAT_10189fa8 = DAT_10189fa8 | 0x80;
    FUN_1006af63(0x10044770);
  }
  if ((DAT_10189e7d & 1) == 0) {
    DAT_10189e7d = DAT_10189e7d | 1;
    FUN_1006af63(0x10044760);
  }
  DAT_10189b3c = *(float *)(param_1 + 4);
  _DAT_10189e30 = *(undefined4 *)(param_1 + 8);
  _DAT_10189e34 = *(undefined4 *)(param_1 + 0xc);
  _DAT_10189e38 = *(undefined4 *)(param_1 + 0x10);
  _DAT_10189b68 = *(undefined4 *)(param_1 + 0x14);
  _DAT_10189b6c = *(undefined4 *)(param_1 + 0x18);
  _DAT_10189b70 = *(undefined4 *)(param_1 + 0x1c);
  _DAT_10189ce8 = *(undefined4 *)(param_1 + 0x20);
  _DAT_10189cec = *(undefined4 *)(param_1 + 0x24);
  _DAT_10189cf0 = *(undefined4 *)(param_1 + 0x28);
  (*DAT_100d5b88)();
  _DAT_10189eb0 = FUN_100071c0(0x22);
  DAT_10189ff8 = (int)_DAT_10189eb0 + 0x252;
  FUN_1001c620(param_1,(float *)&DAT_10189dd8,(float *)&DAT_10189e30);
  DAT_101899d4 = *(int *)(param_1 + 0x38);
  if (DAT_101899d4 == 0xf) {
    _DAT_10189ab0 = _DAT_10189dd8 - _DAT_10189fb0 * _DAT_1007a8f4;
    _DAT_10189ab4 = _DAT_10189ddc - _DAT_10189fb4 * _DAT_1007a8f4;
    _DAT_10189ab8 = _DAT_10189de0 - _DAT_10189fb8 * _DAT_1007a8f4;
  }
  else {
    _DAT_10189ab0 = _DAT_10189fb0 * _DAT_1007cd74 + _DAT_10189dd8;
    _DAT_10189ab4 = _DAT_10189fb4 * _DAT_1007cd74 + _DAT_10189ddc;
    _DAT_10189ab8 = _DAT_10189fb8 * _DAT_1007cd74 + _DAT_10189de0;
  }
  (**(code **)(DAT_100d5c78 + 0x28))();
  (**(code **)(DAT_100d5c78 + 0x2c))();
  (**(code **)(DAT_100d5c78 + 0x34))();
  (**(code **)(DAT_100d5c78 + 0x38))();
  fVar7 = -NAN;
  puVar4 = &DAT_10189dd8;
  (**(code **)(DAT_100d5c78 + 0x3c))();
  DAT_10189e08 = *(undefined4 *)(param_1 + 0x3c);
  if (_DAT_10189a70 == (float)_DAT_10079e80) {
    (*DAT_100d5c38)();
LAB_10044649:
    (*DAT_100d5c38)();
    __ftol();
    puVar1 = (undefined4 *)(DAT_100d5c78 + 4);
    (*DAT_100d5c34)();
    (*(code *)*puVar1)();
  }
  else {
    fVar8 = 2.608598e-29;
    fVar9 = DAT_10189b3c;
    uVar2 = FUN_10045df0(DAT_10189b3c,0x10189a60,&DAT_10189dd8,&DAT_10189ab0);
    switch(uVar2) {
    case 0:
    case 0x44:
    case 0x57:
      break;
    default:
      sVar14 = 0xc;
      sVar12 = 5;
      pcVar10 = s_sprites_scintille_metal_spr_10095460;
      puVar6 = &DAT_10189a80;
      fVar5 = 2.6086176e-29;
      FUN_10002610(&stack0xffffffb8,(undefined4 *)&DAT_10189a80);
      FUN_10002610(&stack0xffffffac,(undefined4 *)&DAT_10189a74);
      FUN_10046f50((float)puVar4,fVar5,(float)puVar6,fVar7,fVar8,fVar9,(byte *)pcVar10,sVar12,sVar14
                  );
      fVar15 = 25.0;
      fVar13 = 1.15;
      iVar11 = 7;
      puVar6 = &DAT_10189a80;
      fVar5 = 2.6086327e-29;
      FUN_10002610(&stack0xffffffb8,(undefined4 *)&DAT_10189a80);
      FUN_10002610(&stack0xffffffac,(undefined4 *)&DAT_10189a74);
      FUN_100492b0((float)puVar4,fVar5,(float)puVar6,fVar7,fVar8,fVar9,iVar11,fVar13,fVar15);
      (*DAT_100d5c38)(0,1);
      goto LAB_10044649;
    case 0x46:
      DAT_10189e08 = 1;
    }
  }
  iVar11 = FUN_1001c5e0((int)DAT_10189b3c);
  if ((iVar11 != 0) && (bVar3 = CL_IsThirdPerson(), CONCAT31(extraout_var,bVar3) == 0)) {
    (**(code **)(DAT_100d5c78 + 0x40))();
  }
  (**(code **)(DAT_100d5c78 + 0x30))();
  return;
}


