// ===== FUN @ 10043200 size=1385 =====

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void FUN_10043200(int param_1)

{
  undefined4 *puVar1;
  char cVar2;
  undefined4 uVar3;
  int iVar4;
  longlong lVar5;
  float fVar6;
  float fVar7;
  undefined *puVar8;
  undefined *puVar9;
  float fVar10;
  float fVar11;
  char *pcVar12;
  short sVar13;
  float fStack_30;
  float fStack_2c;
  float fStack_28;
  float fStack_24;
  float fStack_20;
  float fStack_1c;
  float fStack_18;
  float fStack_14;
  float fStack_10;
  float fStack_c;
  float fStack_8;
  float fStack_4;
  
  if ((DAT_10189ec5 & 1) == 0) {
    DAT_10189ec5 = DAT_10189ec5 | 1;
    FUN_1006af63(0x100437f0);
  }
  if ((DAT_10189ec5 & 2) == 0) {
    DAT_10189ec5 = DAT_10189ec5 | 2;
    FUN_1006af63(0x100437e0);
  }
  if ((DAT_10189ec5 & 4) == 0) {
    DAT_10189ec5 = DAT_10189ec5 | 4;
    FUN_1006af63(0x100437d0);
  }
  if ((DAT_10189ec5 & 8) == 0) {
    DAT_10189ec5 = DAT_10189ec5 | 8;
    FUN_1006af63(0x100437c0);
  }
  if ((DAT_10189ec5 & 0x10) == 0) {
    DAT_10189ec5 = DAT_10189ec5 | 0x10;
    FUN_1006af63(0x100437b0);
  }
  if ((DAT_10189ec5 & 0x20) == 0) {
    DAT_10189ec5 = DAT_10189ec5 | 0x20;
    FUN_1006af63(0x100437a0);
  }
  if ((DAT_10189ec5 & 0x40) == 0) {
    DAT_10189ec5 = DAT_10189ec5 | 0x40;
    FUN_1006af63(0x10043790);
  }
  if ((DAT_10189ec5 & 0x80) == 0) {
    DAT_10189ec5 = DAT_10189ec5 | 0x80;
    FUN_1006af63(0x10043780);
  }
  if ((DAT_10189a4c & 1) == 0) {
    DAT_10189a4c = DAT_10189a4c | 1;
    FUN_1006af63(0x10043770);
  }
  DAT_10189b14 = *(int *)(param_1 + 4);
  _DAT_10189a20 = *(undefined4 *)(param_1 + 8);
  _DAT_10189a24 = *(undefined4 *)(param_1 + 0xc);
  _DAT_10189a28 = *(undefined4 *)(param_1 + 0x10);
  _DAT_10189ca8 = *(undefined4 *)(param_1 + 0x14);
  _DAT_10189cac = *(undefined4 *)(param_1 + 0x18);
  _DAT_10189cb0 = *(undefined4 *)(param_1 + 0x1c);
  _DAT_10189b58 = *(undefined4 *)(param_1 + 0x20);
  _DAT_10189b5c = *(undefined4 *)(param_1 + 0x24);
  _DAT_10189b60 = *(undefined4 *)(param_1 + 0x28);
  (*DAT_100d5b88)();
  DAT_10189fa0 = *(int *)(param_1 + 0x38);
  lVar5 = __ftol();
  FUN_100071c0((int)lVar5);
  uVar3 = FUN_1001c5c0(DAT_10189b14);
  if (((char)uVar3 != '\0') || (iVar4 = FUN_1001c5e0(DAT_10189b14), iVar4 != 0)) {
    (**(code **)(DAT_100d5c78 + 0x40))();
  }
  FUN_1001c620(param_1,(float *)&DAT_10189a50,(float *)&DAT_10189a20);
  if (DAT_10189fa0 == 0) {
    (*DAT_100d5c38)();
    __ftol();
    puVar1 = (undefined4 *)(DAT_100d5c78 + 4);
    (*DAT_100d5c34)();
    (*(code *)*puVar1)();
    return;
  }
  if (DAT_10189fa0 == 3) {
    _DAT_1018a0a8 = _DAT_10189a40 * _DAT_1007d6fc + _DAT_10189a50;
    _DAT_1018a0ac = _DAT_10189a44 * _DAT_1007d6fc + _DAT_10189a54;
    _DAT_1018a0b0 = _DAT_10189a48 * _DAT_1007d6fc + _DAT_10189a58;
    fStack_30 = _DAT_1018a0a8;
    fStack_2c = _DAT_1018a0ac;
    fStack_28 = _DAT_1018a0b0;
    fStack_20 = _DAT_1018a0ac;
    fStack_1c = _DAT_1018a0b0;
  }
  else if (DAT_10189fa0 == 2) {
    fStack_20 = _DAT_10189a44 * _DAT_1007a900;
    fStack_1c = _DAT_10189a48 * _DAT_1007a900;
    fStack_30 = _DAT_10189a40 * _DAT_1007a900 + _DAT_10189a50;
    fStack_2c = fStack_20 + _DAT_10189a54;
    fStack_28 = fStack_1c + _DAT_10189a58;
    FUN_10002610(&fStack_24,&fStack_30);
    _DAT_1018a0a8 = fStack_24;
    _DAT_1018a0ac = fStack_20;
    _DAT_1018a0b0 = fStack_1c;
  }
  else {
    fStack_24 = _DAT_10189a40 * _DAT_1007a8d8;
    fStack_20 = _DAT_10189a44 * _DAT_1007a8d8;
    fStack_1c = _DAT_10189a48 * _DAT_1007a8d8;
    FUN_10002610(&fStack_30,&fStack_24);
    fStack_18 = fStack_30 + _DAT_10189a50;
    fStack_14 = fStack_2c + _DAT_10189a54;
    fStack_10 = fStack_28 + _DAT_10189a58;
    FUN_10002610(&fStack_c,&fStack_18);
    _DAT_1018a0a8 = fStack_c;
    _DAT_1018a0ac = fStack_8;
    _DAT_1018a0b0 = fStack_4;
  }
  (*DAT_100d5c38)();
  __ftol();
  puVar1 = (undefined4 *)(DAT_100d5c78 + 4);
  (*DAT_100d5c34)();
  fVar10 = 1.4013e-45;
  puVar9 = &DAT_10189a20;
  (*(code *)*puVar1)();
  fVar6 = 0.0;
  (**(code **)(DAT_100d5c78 + 0x28))();
  (**(code **)(DAT_100d5c78 + 0x2c))();
  (**(code **)(DAT_100d5c78 + 0x34))(DAT_10189b14 + -1);
  (**(code **)(DAT_100d5c78 + 0x38))(2);
  (**(code **)(DAT_100d5c78 + 0x3c))(&DAT_10189a50,&DAT_1018a0a8,2,0xffffffff,&DAT_1018a058);
  if (_DAT_1018a068 == (float)_DAT_10079e80) goto LAB_100436eb;
  fVar11 = 2.6074467e-29;
  cVar2 = FUN_10045df0(DAT_10189b14,0x1018a058,&DAT_10189a50,&DAT_1018a0a8);
  if (cVar2 == 'D') {
LAB_100436ba:
    pcVar12 = s_sprites_wood_smoke_add_spr_100953d4;
  }
  else {
    if (cVar2 == 'F') goto LAB_100436eb;
    if (cVar2 == 'W') goto LAB_100436ba;
    pcVar12 = s_sprites_gun_smoke_add_spr_100953f0;
  }
  iVar4 = 0x1e;
  sVar13 = 0xc;
  uVar3 = 5;
  puVar8 = &DAT_1018a078;
  fVar7 = 2.6074648e-29;
  FUN_10002610(&stack0xffffffa8,(undefined4 *)&DAT_1018a078);
  FUN_10002610(&stack0xffffff9c,(undefined4 *)&DAT_1018a06c);
  FUN_100464f0(fVar6,fVar7,(float)puVar8,(float)puVar9,fVar10,fVar11,(byte *)pcVar12,uVar3,sVar13,
               iVar4);
LAB_100436eb:
  (**(code **)(DAT_100d5c78 + 0x30))();
  return;
}


