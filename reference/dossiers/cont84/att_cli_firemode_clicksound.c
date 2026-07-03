// ===== FUN @ 1004d350 size=150 =====

void __fastcall FUN_1004d350(undefined4 param_1)

{
  undefined4 *puVar1;
  int in_EAX;
  int iVar2;
  int iVar3;
  undefined4 *puVar4;
  int unaff_ESI;
  float10 fVar5;
  longlong lVar6;
  
  if (in_EAX != CONCAT31((int3)((uint)param_1 >> 8),*(undefined1 *)(unaff_ESI + 0x78a))) {
    iVar2 = (*DAT_100d5c38)(0xfffffffb,5);
    lVar6 = __ftol();
    iVar3 = (*DAT_100d5bf4)();
    puVar1 = (undefined4 *)(DAT_100d5c78 + 4);
    fVar5 = (float10)(*DAT_100d5c34)(0x3f6b851f,0x3f800000,0x3fa00000,0,iVar2 + (0x19 - (int)lVar6))
    ;
    puVar4 = (undefined4 *)
             (*DAT_100d5bf4)(iVar3 + 0x2c0,3,s_weapons_switch_wav_10095d9c,(float)fVar5);
    (*(code *)*puVar1)(*puVar4);
    *(undefined1 *)(unaff_ESI + 0x7bc) = *(undefined1 *)(unaff_ESI + 0x78a);
  }
  return;
}


