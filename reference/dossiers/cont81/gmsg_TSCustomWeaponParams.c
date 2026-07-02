// ===== FUN @ 1004fb50 size=239 =====

void __thiscall FUN_1004fb50(void *this,undefined4 param_1,int param_2,undefined4 param_3)

{
  char cVar1;
  char *pcVar2;
  undefined4 *puVar3;
  uint uVar4;
  uint uVar5;
  char *pcVar6;
  undefined4 local_50;
  undefined4 local_4c;
  undefined4 local_48;
  undefined4 local_44;
  char local_40 [64];
  
  FUN_100299b0(param_3,param_2);
  pcVar2 = FUN_10029b00();
  uVar4 = 0xffffffff;
  do {
    pcVar6 = pcVar2;
    if (uVar4 == 0) break;
    uVar4 = uVar4 - 1;
    pcVar6 = pcVar2 + 1;
    cVar1 = *pcVar2;
    pcVar2 = pcVar6;
  } while (cVar1 != '\0');
  uVar4 = ~uVar4;
  pcVar2 = pcVar6 + -uVar4;
  pcVar6 = local_40;
  for (uVar5 = uVar4 >> 2; uVar5 != 0; uVar5 = uVar5 - 1) {
    *(undefined4 *)pcVar6 = *(undefined4 *)pcVar2;
    pcVar2 = pcVar2 + 4;
    pcVar6 = pcVar6 + 4;
  }
  for (uVar4 = uVar4 & 3; uVar4 != 0; uVar4 = uVar4 - 1) {
    *pcVar6 = *pcVar2;
    pcVar2 = pcVar2 + 1;
    pcVar6 = pcVar6 + 1;
  }
  FUN_1006bf83(local_40,(byte *)s__i__f__f__f__f__i_10095edc);
  puVar3 = FUN_100071c0(param_2);
  puVar3[10] = param_3;
  puVar3 = FUN_100071c0(param_2);
  puVar3[0xc] = local_50;
  puVar3 = FUN_100071c0(param_2);
  puVar3[0xd] = local_4c;
  puVar3 = FUN_100071c0(param_2);
  puVar3[7] = local_48;
  puVar3 = FUN_100071c0(param_2);
  puVar3[0x36] = local_44;
  *(uint *)((int)this + 0x10) = *(uint *)((int)this + 0x10) | 1;
  return;
}
