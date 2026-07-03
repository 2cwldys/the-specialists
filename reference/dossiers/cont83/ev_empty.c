// ===== FUN @ 100424e0 size=182 =====

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void FUN_100424e0(int param_1)

{
  undefined4 *puVar1;
  int iVar2;
  float10 fVar3;
  longlong lVar4;
  
  if ((DAT_10189f9c & 1) == 0) {
    DAT_10189f9c = DAT_10189f9c | 1;
    FUN_1006af63(0x100425a0);
  }
  DAT_10189fbc = *(undefined4 *)(param_1 + 4);
  _DAT_10189c68 = *(undefined4 *)(param_1 + 8);
  _DAT_10189c6c = *(undefined4 *)(param_1 + 0xc);
  _DAT_10189c70 = *(undefined4 *)(param_1 + 0x10);
  iVar2 = (*DAT_100d5c38)(0xfffffffb,5);
  lVar4 = __ftol();
  puVar1 = (undefined4 *)(DAT_100d5c78 + 4);
  fVar3 = (float10)(*DAT_100d5c34)(0x3f6b851f,0x3f800000,0x3f4ccccd,0,iVar2 + (0x19 - (int)lVar4));
  (*(code *)*puVar1)(DAT_10189fbc,&DAT_10189c68,1,s_weapons_pistol_empty_wav_100911e4,(float)fVar3);
  return;
}


