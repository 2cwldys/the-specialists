// ===== FUN @ 10043800 size=1023 =====

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void FUN_10043800(int param_1)

{
  undefined4 *puVar1;
  char cVar2;
  undefined4 uVar3;
  int iVar4;
  float fVar5;
  float fVar6;
  undefined *puVar7;
  undefined *puVar8;
  float fVar9;
  float fVar10;
  char *pcVar11;
  short sVar12;
  
  if ((DAT_10189f3c & 1) == 0) {
    DAT_10189f3c = DAT_10189f3c | 1;
    FUN_1006af63(0x10043c80);
  }
  if ((DAT_10189f3c & 2) == 0) {
    DAT_10189f3c = DAT_10189f3c | 2;
    FUN_1006af63(0x10043c70);
  }
  if ((DAT_10189f3c & 4) == 0) {
    DAT_10189f3c = DAT_10189f3c | 4;
    FUN_1006af63(0x10043c60);
  }
  if ((DAT_10189f3c & 8) == 0) {
    DAT_10189f3c = DAT_10189f3c | 8;
    FUN_1006af63(0x10043c50);
  }
  if ((DAT_10189f3c & 0x10) == 0) {
    DAT_10189f3c = DAT_10189f3c | 0x10;
    FUN_1006af63(0x10043c40);
  }
  if ((DAT_10189f3c & 0x20) == 0) {
    DAT_10189f3c = DAT_10189f3c | 0x20;
    FUN_1006af63(0x10043c30);
  }
  if ((DAT_10189f3c & 0x40) == 0) {
    DAT_10189f3c = DAT_10189f3c | 0x40;
    FUN_1006af63(0x10043c20);
  }
  if ((DAT_10189f3c & 0x80) == 0) {
    DAT_10189f3c = DAT_10189f3c | 0x80;
    FUN_1006af63(0x10043c10);
  }
  if ((DAT_10189dc4 & 1) == 0) {
    DAT_10189dc4 = DAT_10189dc4 | 1;
    FUN_1006af63(0x10043c00);
  }
  DAT_10189c74 = FUN_100071c0(*(int *)(param_1 + 0x38));
  DAT_10189fa4 = *(int *)(param_1 + 0x38);
  DAT_10189d34 = *(uint *)(param_1 + 0x3c);
  DAT_10189f48 = *(int *)(param_1 + 4);
  _DAT_10189fd8 = *(undefined4 *)(param_1 + 8);
  _DAT_10189fdc = *(undefined4 *)(param_1 + 0xc);
  _DAT_10189fe0 = *(undefined4 *)(param_1 + 0x10);
  _DAT_10189fe8 = *(undefined4 *)(param_1 + 0x14);
  _DAT_10189fec = *(undefined4 *)(param_1 + 0x18);
  _DAT_10189ff0 = *(undefined4 *)(param_1 + 0x1c);
  _DAT_10189df8 = *(undefined4 *)(param_1 + 0x20);
  _DAT_10189dfc = *(undefined4 *)(param_1 + 0x24);
  _DAT_10189e00 = *(undefined4 *)(param_1 + 0x28);
  (*DAT_100d5b88)();
  uVar3 = FUN_1001c5c0(DAT_10189f48);
  if ((char)uVar3 == '\0') {
    iVar4 = FUN_1001c5e0(DAT_10189f48);
    if (iVar4 != 0) goto LAB_100439f8;
  }
  else {
LAB_100439f8:
    DAT_10189dd4 = (int)DAT_10189c74 + 0x252;
    _DAT_10189c5c = FUN_10007110(DAT_10189fa4,DAT_10189d34);
    (**(code **)(DAT_100d5c78 + 0x40))();
  }
  FUN_1001c620(param_1,(float *)&DAT_10189c78,(float *)&DAT_10189fd8);
  _DAT_10189f30 = _DAT_10189af0 * _DAT_1007a874 + _DAT_10189c78;
  _DAT_10189f34 = _DAT_10189c7c + _DAT_10189af4 * _DAT_1007a874;
  _DAT_10189f38 = _DAT_10189c80 + _DAT_10189af8 * _DAT_1007a874;
  (*DAT_100d5c38)();
  __ftol();
  puVar1 = (undefined4 *)(DAT_100d5c78 + 4);
  (*DAT_100d5c34)();
  fVar9 = 1.4013e-45;
  puVar8 = &DAT_10189fd8;
  (*(code *)*puVar1)();
  fVar5 = 0.0;
  (**(code **)(DAT_100d5c78 + 0x28))();
  (**(code **)(DAT_100d5c78 + 0x2c))();
  (**(code **)(DAT_100d5c78 + 0x34))(DAT_10189f48 + -1);
  (**(code **)(DAT_100d5c78 + 0x38))(2);
  (**(code **)(DAT_100d5c78 + 0x3c))(&DAT_10189c78,&DAT_10189f30,2,0xffffffff,&DAT_10189b78);
  if (_DAT_10189b88 == (float)_DAT_10079e80) goto LAB_10043bf0;
  fVar10 = 2.6078334e-29;
  cVar2 = FUN_10045df0(DAT_10189f48,0x10189b78,&DAT_10189c78,&DAT_10189f30);
  if (cVar2 == 'D') {
LAB_10043bbf:
    pcVar11 = s_sprites_wood_smoke_add_spr_100953d4;
  }
  else {
    if (cVar2 == 'F') goto LAB_10043bf0;
    if (cVar2 == 'W') goto LAB_10043bbf;
    pcVar11 = s_sprites_gun_smoke_add_spr_100953f0;
  }
  iVar4 = 0x1e;
  sVar12 = 0xc;
  uVar3 = 5;
  puVar7 = &DAT_10189b98;
  fVar6 = 2.6078515e-29;
  FUN_10002610(&stack0xffffffc0,(undefined4 *)&DAT_10189b98);
  FUN_10002610(&stack0xffffffb4,(undefined4 *)&DAT_10189b8c);
  FUN_100464f0(fVar5,fVar6,(float)puVar7,(float)puVar8,fVar9,fVar10,(byte *)pcVar11,uVar3,sVar12,
               iVar4);
LAB_10043bf0:
  (**(code **)(DAT_100d5c78 + 0x30))();
  return;
}


