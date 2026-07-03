// ===== FUN @ 1004d7e0 size=2092 =====

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void __fastcall FUN_1004d7e0(void *param_1)

{
  char cVar1;
  uint uVar2;
  int iVar3;
  undefined4 *puVar4;
  undefined4 uVar5;
  undefined4 uVar6;
  undefined4 uVar7;
  uint uVar8;
  int iVar9;
  int iVar10;
  char *pcVar11;
  longlong lVar12;
  byte bVar13;
  char *pcVar14;
  
  if (*(float *)(DAT_100df494 + 0xc) != _DAT_100799f0) {
    return;
  }
  lVar12 = __ftol();
  DAT_1018a648 = (int)lVar12;
  lVar12 = __ftol();
  DAT_1018a70c = (int)lVar12;
  lVar12 = __ftol();
  DAT_1018a65c = (int)lVar12;
  DAT_1018a6c0 = FUN_100071c0(*(int *)((int)param_1 + 0x7b8));
  iVar10 = DAT_100d5908 + -0x88;
  iVar9 = 0x14;
  DAT_1018a700 = DAT_100d590c + -8;
  DAT_1018a6f8 = 0x14;
  DAT_1018a674 = 0;
  DAT_1018a67c = iVar10;
  do {
    uVar2 = 1 << ((byte)DAT_1018a674 & 0x1f);
    if ((*(uint *)((int)param_1 + 0x798) & uVar2) == 0) goto LAB_1004da28;
    DAT_1018a700 = DAT_1018a700 - iVar9;
    (*DAT_100d5b54)(iVar10,DAT_1018a700,0x80,iVar9 + -1,DAT_1018a648,DAT_1018a70c,DAT_1018a65c,
                    (-((uVar2 & *(uint *)((int)param_1 + 0x14)) != 0) & 0x1eU) + 0xf);
    switch(DAT_1018a674) {
    case 0:
      pcVar14 = s__HUD_Silencer_10095eac;
      break;
    case 1:
      pcVar14 = s__HUD_Lasersight_10095e9c;
      goto LAB_1004d924;
    case 2:
      pcVar14 = s__HUD_Flashlight_10095e8c;
      break;
    case 3:
      pcVar14 = s__HUD_Scope_10095e80;
LAB_1004d924:
      pcVar14 = FUN_100349a0(pcVar14);
      uVar2 = 0xffffffff;
      do {
        pcVar11 = pcVar14;
        if (uVar2 == 0) break;
        uVar2 = uVar2 - 1;
        pcVar11 = pcVar14 + 1;
        cVar1 = *pcVar14;
        pcVar14 = pcVar11;
      } while (cVar1 != '\0');
      uVar2 = ~uVar2;
      pcVar14 = pcVar11 + -uVar2;
      pcVar11 = (char *)&DAT_1018a688;
      for (uVar8 = uVar2 >> 2; uVar8 != 0; uVar8 = uVar8 - 1) {
        *(undefined4 *)pcVar11 = *(undefined4 *)pcVar14;
        pcVar14 = pcVar14 + 4;
        pcVar11 = pcVar11 + 4;
      }
      goto LAB_1004d949;
    default:
      goto switchD_1004d8de_default;
    }
    pcVar14 = FUN_100349a0(pcVar14);
    uVar2 = 0xffffffff;
    do {
      pcVar11 = pcVar14;
      if (uVar2 == 0) break;
      uVar2 = uVar2 - 1;
      pcVar11 = pcVar14 + 1;
      cVar1 = *pcVar14;
      pcVar14 = pcVar11;
    } while (cVar1 != '\0');
    uVar2 = ~uVar2;
    pcVar14 = pcVar11 + -uVar2;
    pcVar11 = (char *)&DAT_1018a688;
    for (uVar8 = uVar2 >> 2; uVar8 != 0; uVar8 = uVar8 - 1) {
      *(undefined4 *)pcVar11 = *(undefined4 *)pcVar14;
      pcVar14 = pcVar14 + 4;
      pcVar11 = pcVar11 + 4;
    }
LAB_1004d949:
    for (uVar2 = uVar2 & 3; uVar2 != 0; uVar2 = uVar2 - 1) {
      *pcVar11 = *pcVar14;
      pcVar14 = pcVar14 + 1;
      pcVar11 = pcVar11 + 1;
    }
switchD_1004d8de_default:
    FUN_10021ce0(DAT_1018a67c + 2,DAT_1018a700,DAT_100d5908,(char *)&DAT_1018a688,DAT_1018a648 / 2,
                 DAT_1018a70c / 2,DAT_1018a65c / 2);
    iVar3 = DAT_1018a648;
    iVar10 = DAT_1018a70c;
    iVar9 = DAT_1018a65c;
    if ((1 << ((byte)DAT_1018a674 & 0x1f) & *(uint *)((int)param_1 + 0x14)) == 0) {
      iVar3 = DAT_1018a648 / 2;
      iVar10 = DAT_1018a70c / 2;
      iVar9 = DAT_1018a65c / 2;
    }
    (*DAT_100d5b38)(*(undefined4 *)((int)param_1 + 0x144),iVar3,iVar10,iVar9);
    (*DAT_100d5b44)(0,DAT_1018a67c + 0x70,DAT_1018a700,0);
    iVar9 = DAT_1018a6f8;
    iVar10 = DAT_1018a67c;
LAB_1004da28:
    DAT_1018a674 = DAT_1018a674 + 1;
  } while (DAT_1018a674 < 4);
  DAT_1018a700 = DAT_1018a700 - iVar9;
  (*DAT_100d5b54)(iVar10,DAT_1018a700,0x80,iVar9 + -1,DAT_1018a648,DAT_1018a70c,DAT_1018a65c,0xf);
  puVar4 = FUN_100071c0(*(int *)((int)param_1 + 0x7b8));
  if (*(char *)(puVar4 + 2) == '\0') {
    FUN_1004d5f0(param_1,DAT_1018a67c,DAT_1018a700);
    DAT_1018a700 = DAT_1018a700 - DAT_1018a6f8;
    puVar4 = FUN_100071c0(*(int *)((int)param_1 + 0x7b8));
    if (*(char *)(puVar4 + 0x37) != '\0') {
      iVar10 = *(int *)((int)param_1 + 0x78c);
      lVar12 = __ftol();
      uVar5 = (undefined4)lVar12;
      lVar12 = __ftol();
      uVar6 = (undefined4)lVar12;
      lVar12 = __ftol();
      uVar7 = (undefined4)lVar12;
      bVar13 = 5;
      iVar9 = DAT_1018a700;
      lVar12 = __ftol();
      FUN_1004c290(&DAT_100cf920,(int)lVar12,iVar9,bVar13,iVar10,uVar7,uVar6,uVar5);
    }
    goto LAB_1004df0f;
  }
  iVar10 = *(int *)((int)param_1 + 0x78c);
  lVar12 = __ftol();
  uVar5 = (undefined4)lVar12;
  lVar12 = __ftol();
  uVar6 = (undefined4)lVar12;
  lVar12 = __ftol();
  FUN_1004c290(&DAT_100cf920,DAT_1018a67c + 2,DAT_1018a700,5,iVar10,(int)lVar12,uVar6,uVar5);
  FUN_10021ce0(DAT_1018a67c + 0x40,DAT_1018a700,DAT_100d5908,&DAT_10095e7c,
               (int)(DAT_1018a648 + (DAT_1018a648 >> 0x1f & 3U)) >> 2,
               (int)(DAT_1018a70c + (DAT_1018a70c >> 0x1f & 3U)) >> 2,
               (int)(DAT_1018a65c + (DAT_1018a65c >> 0x1f & 3U)) >> 2);
  FUN_100071c0(*(int *)((int)param_1 + 0x7b8));
  FUN_100071c0(*(int *)((int)param_1 + 0x7b8));
  FUN_100071c0(*(int *)((int)param_1 + 0x7b8));
  lVar12 = __ftol();
  uVar5 = (undefined4)lVar12;
  lVar12 = __ftol();
  uVar6 = (undefined4)lVar12;
  lVar12 = __ftol();
  uVar7 = (undefined4)lVar12;
  puVar4 = FUN_100071c0(*(int *)((int)param_1 + 0x7b8));
  iVar10 = *(int *)((int)param_1 + (uint)*(byte *)(puVar4 + 2) * 4 + 0x748);
  bVar13 = 5;
  iVar9 = DAT_1018a700;
  lVar12 = __ftol();
  FUN_1004c290(&DAT_100cf920,(int)lVar12,iVar9,bVar13,iVar10,uVar7,uVar6,uVar5);
  DAT_1018a700 = DAT_1018a700 - DAT_1018a6f8;
  (*DAT_100d5b54)(DAT_1018a67c,DAT_1018a700,0x80,DAT_1018a6f8 + -1,DAT_1018a648,DAT_1018a70c,
                  DAT_1018a65c,0xf);
  switch(*(undefined1 *)((int)param_1 + 0x78a)) {
  case 1:
    pcVar14 = s__HUD_SemiAuto_10095e6c;
    break;
  case 2:
    pcVar14 = s__HUD_Burst_10095e60;
    goto LAB_1004ddd4;
  case 3:
    pcVar14 = s__HUD_PumpAction_10095e50;
    break;
  case 4:
    pcVar14 = s__HUD_FreeSemi_10095e30;
    break;
  case 5:
    pcVar14 = s__HUD_FreeFull_10095e40;
    goto LAB_1004ddd4;
  default:
    pcVar14 = s__HUD_FullAuto_10095e20;
LAB_1004ddd4:
    pcVar14 = FUN_100349a0(pcVar14);
    uVar2 = 0xffffffff;
    do {
      pcVar11 = pcVar14;
      if (uVar2 == 0) break;
      uVar2 = uVar2 - 1;
      pcVar11 = pcVar14 + 1;
      cVar1 = *pcVar14;
      pcVar14 = pcVar11;
    } while (cVar1 != '\0');
    uVar2 = ~uVar2;
    pcVar14 = pcVar11 + -uVar2;
    pcVar11 = (char *)&DAT_1018a6c8;
    for (uVar8 = uVar2 >> 2; uVar8 != 0; uVar8 = uVar8 - 1) {
      *(undefined4 *)pcVar11 = *(undefined4 *)pcVar14;
      pcVar14 = pcVar14 + 4;
      pcVar11 = pcVar11 + 4;
    }
    goto LAB_1004ddf9;
  }
  pcVar14 = FUN_100349a0(pcVar14);
  uVar2 = 0xffffffff;
  do {
    pcVar11 = pcVar14;
    if (uVar2 == 0) break;
    uVar2 = uVar2 - 1;
    pcVar11 = pcVar14 + 1;
    cVar1 = *pcVar14;
    pcVar14 = pcVar11;
  } while (cVar1 != '\0');
  uVar2 = ~uVar2;
  pcVar14 = pcVar11 + -uVar2;
  pcVar11 = (char *)&DAT_1018a6c8;
  for (uVar8 = uVar2 >> 2; uVar8 != 0; uVar8 = uVar8 - 1) {
    *(undefined4 *)pcVar11 = *(undefined4 *)pcVar14;
    pcVar14 = pcVar14 + 4;
    pcVar11 = pcVar11 + 4;
  }
LAB_1004ddf9:
  for (uVar2 = uVar2 & 3; uVar2 != 0; uVar2 = uVar2 - 1) {
    *pcVar11 = *pcVar14;
    pcVar14 = pcVar14 + 1;
    pcVar11 = pcVar11 + 1;
  }
  lVar12 = __ftol();
  uVar5 = (undefined4)lVar12;
  lVar12 = __ftol();
  uVar6 = (undefined4)lVar12;
  lVar12 = __ftol();
  FUN_10021ce0(DAT_1018a67c + 2,DAT_1018a700,DAT_100d5908,(char *)&DAT_1018a6c8,(int)lVar12,uVar6,
               uVar5);
  DAT_1018a700 = DAT_1018a700 - DAT_1018a6f8;
  (*DAT_100d5b54)(DAT_1018a67c,DAT_1018a700,0x80,DAT_1018a6f8 + -1,DAT_1018a648,DAT_1018a70c,
                  DAT_1018a65c,0xf);
  lVar12 = __ftol();
  uVar5 = (undefined4)lVar12;
  lVar12 = __ftol();
  uVar6 = (undefined4)lVar12;
  lVar12 = __ftol();
  uVar7 = (undefined4)lVar12;
  puVar4 = FUN_100071c0(*(int *)((int)param_1 + 0x7b8));
  puVar4 = FUN_100071f0((uint)*(byte *)(puVar4 + 2));
  FUN_10021ce0(DAT_1018a67c + 2,DAT_1018a700,DAT_100d5908,(char *)(puVar4 + 10),uVar7,uVar6,uVar5);
LAB_1004df0f:
  DAT_1018a700 = DAT_1018a700 - DAT_1018a6f8;
  (*DAT_100d5b54)(DAT_1018a67c,DAT_1018a700,0x80,DAT_1018a6f8 + -1,DAT_1018a648,DAT_1018a70c,
                  DAT_1018a65c,0x23);
  lVar12 = __ftol();
  uVar5 = (undefined4)lVar12;
  lVar12 = __ftol();
  uVar6 = (undefined4)lVar12;
  lVar12 = __ftol();
  FUN_10021ce0(DAT_1018a67c + 2,DAT_1018a700,DAT_100d5908,(char *)(DAT_1018a6c0 + 0x11),(int)lVar12,
               uVar6,uVar5);
  if (DAT_100cf978 < 0x28) {
    FUN_1006ac7a((undefined1 *)&DAT_1018a6c8,&DAT_10095e1c);
    FUN_10021ce0(0x18,0x18,DAT_100d5908,(char *)&DAT_1018a6c8,DAT_1018a648,DAT_1018a70c,DAT_1018a65c
                );
  }
  return;
}


