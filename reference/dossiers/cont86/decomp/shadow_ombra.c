// ===== FUN @ 10055760 size=764 =====

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void FUN_10055760(void)

{
  float fVar1;
  bool bVar2;
  char cVar3;
  undefined4 uVar4;
  int iVar5;
  undefined4 *puVar6;
  undefined3 extraout_var;
  
  if ((DAT_1018aaa0 & 1) == 0) {
    DAT_1018aaa0 = DAT_1018aaa0 | 1;
    FUN_1006af63(0x10055a90);
  }
  if ((DAT_1018aaa0 & 2) == 0) {
    DAT_1018aaa0 = DAT_1018aaa0 | 2;
    FUN_1006af63(0x10055a80);
  }
  if ((DAT_1018aaa0 & 4) == 0) {
    DAT_1018aaa0 = DAT_1018aaa0 | 4;
    FUN_1006af63(0x10055a70);
  }
  if ((DAT_1018aaa0 & 8) == 0) {
    DAT_1018aaa0 = DAT_1018aaa0 | 8;
    FUN_1006af63(0x10055a60);
  }
  if ((*(float *)(DAT_100df4cc + 0xc) != _DAT_100799f0) &&
     (_DAT_1018abc0 = FUN_100509d0(&DAT_1018bde8,(byte *)s_sprites_ombra_spr_10095f84,2),
     _DAT_1018abc0 != -1)) {
    puVar6 = (undefined4 *)(DAT_100d5c70 + 0x2c);
    uVar4 = (*DAT_100d5c20)(*(undefined4 *)(&DAT_1018be14 + _DAT_1018abc0 * 0x28),0);
    iVar5 = (*(code *)*puVar6)(uVar4);
    if (iVar5 != 0) {
      (**(code **)(DAT_100d5c70 + 4))(4);
      (**(code **)(DAT_100d5c70 + 0x28))(1);
      (**(code **)(DAT_100d5c70 + 8))(2);
      puVar6 = (undefined4 *)(*DAT_100d5bf4)();
      DAT_1018abb8 = *puVar6;
      bVar2 = CL_IsThirdPerson();
      _DAT_1018aa9c = CONCAT31(extraout_var,bVar2);
      if (((_DAT_1018aa9c == 0) && (cVar3 = FUN_1004c280(), cVar3 == '\0')) &&
         (*(int *)(DAT_100df4cc + 0xc) == 0x3f800000)) {
        _DAT_1018aa40 = DAT_10190b30 - _DAT_1007d448;
        _DAT_1018aa38 = DAT_10190b28;
        _DAT_1018aa3c = DAT_10190b2c;
        (**(code **)(DAT_100d5c78 + 0x38))(2);
        (**(code **)(DAT_100d5c78 + 0x3c))(&DAT_10190b28,&DAT_1018aa38,0,DAT_1018abb8,&DAT_1018aa58)
        ;
        FUN_10046110((float *)&DAT_1018ab00,(float *)&DAT_1018aa78);
        _DAT_1018aa40 = _DAT_1018aa80 + _DAT_1018aa74;
        _DAT_1018aa38 = _DAT_1018aa78 + _DAT_1018aa6c;
        _DAT_1018aa3c = _DAT_1018aa7c + _DAT_1018aa70;
        fVar1 = SQRT((_DAT_1018aa70 - DAT_10190b2c) * (_DAT_1018aa70 - DAT_10190b2c) +
                     (_DAT_1018aa74 - DAT_10190b30) * (_DAT_1018aa74 - DAT_10190b30) +
                     (_DAT_1018aa6c - DAT_10190b28) * (_DAT_1018aa6c - DAT_10190b28)) -
                _DAT_1007d824;
        if (fVar1 < _DAT_1007a938) {
          (**(code **)(DAT_100d5c70 + 0x10))
                    (0x3f800000,0x3f800000,0x3f800000,_DAT_10079e58 - fVar1 * _DAT_1007d820);
          FUN_10050080((float *)&DAT_1018ab10,(float *)&DAT_1018aa38,&DAT_1018ab00,24.0,'\0',
                       (float *)0x0,0.0,(float *)0x0);
          FUN_10050000(0x1018ab10);
        }
      }
      DAT_1018ab60 = (*DAT_100d5bb8)();
      DAT_1018abbc = 0;
      if (-1 < DAT_1018ab60) {
        do {
          cVar3 = FUN_100579a0(DAT_1018abbc);
          if (cVar3 != '\0') {
            FUN_10055500((undefined1)DAT_1018abbc);
          }
          DAT_1018abbc = DAT_1018abbc + 1;
        } while (DAT_1018abbc <= DAT_1018ab60);
      }
      (**(code **)(DAT_100d5c70 + 0xc))();
      (**(code **)(DAT_100d5c70 + 4))(0);
    }
  }
  return;
}


// ===== FUN @ 10055500 size=530 =====

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void __cdecl FUN_10055500(undefined1 param_1)

{
  float fVar1;
  float fVar2;
  float fVar3;
  
  if ((DAT_1018ab0c & 1) == 0) {
    DAT_1018ab0c = DAT_1018ab0c | 1;
    FUN_1006af63(0x10055750);
  }
  if ((DAT_1018ab0c & 2) == 0) {
    DAT_1018ab0c = DAT_1018ab0c | 2;
    FUN_1006af63(0x10055740);
  }
  if ((DAT_1018ab0c & 4) == 0) {
    DAT_1018ab0c = DAT_1018ab0c | 4;
    FUN_1006af63(0x10055730);
  }
  if ((DAT_1018ab0c & 8) == 0) {
    DAT_1018ab0c = DAT_1018ab0c | 8;
    FUN_100025e0(&DAT_1018ab68,0xc,4,&LAB_10017e90);
    FUN_1006af63(0x10055720);
  }
  DAT_1018aa54 = (*DAT_100d5bfc)(param_1);
  if ((DAT_1018aa54 != 0) && (*(int *)(DAT_1018aa54 + 4) != 0)) {
    _DAT_1018aaf0 = *(float *)(DAT_1018aa54 + 0xb48);
    _DAT_1018aaf4 = *(float *)(DAT_1018aa54 + 0xb4c);
    _DAT_1018aaf8 = *(float *)(DAT_1018aa54 + 0xb50) - _DAT_1007d448;
    (**(code **)(DAT_100d5c78 + 0x38))(2);
    (**(code **)(DAT_100d5c78 + 0x3c))(DAT_1018aa54 + 0xb48,&DAT_1018aaf0,0,param_1,&DAT_1018aaa8);
    FUN_10046110((float *)&DAT_1018aa48,(float *)&DAT_1018aac8);
    _DAT_1018aaf8 = _DAT_1018aad0 + _DAT_1018aac4;
    _DAT_1018aaf0 = _DAT_1018aac8 + _DAT_1018aabc;
    _DAT_1018aaf4 = _DAT_1018aacc + _DAT_1018aac0;
    fVar1 = _DAT_1018aabc - *(float *)(DAT_1018aa54 + 0xb48);
    fVar2 = _DAT_1018aac0 - *(float *)(DAT_1018aa54 + 0xb4c);
    fVar3 = _DAT_1018aac4 - *(float *)(DAT_1018aa54 + 0xb50);
    fVar1 = SQRT(fVar2 * fVar2 + fVar3 * fVar3 + fVar1 * fVar1);
    if (fVar1 < _DAT_1007a938) {
      (**(code **)(DAT_100d5c70 + 0x10))
                (0x3f800000,0x3f800000,0x3f800000,_DAT_10079e58 - fVar1 * _DAT_1007d820);
      FUN_10050080((float *)&DAT_1018ab68,(float *)&DAT_1018aaf0,&DAT_1018aa48,24.0,'\0',
                   (float *)0x0,0.0,(float *)0x0);
      FUN_10050000(0x1018ab68);
      return;
    }
  }
  return;
}


// ===== FUN @ 100579a0 size=23 =====

undefined1 __cdecl FUN_100579a0(int param_1)

{
  if ((-1 < param_1) && (param_1 < 0x21)) {
    return *(undefined1 *)((int)&DAT_1018bd44 + param_1);
  }
  return 0;
}


