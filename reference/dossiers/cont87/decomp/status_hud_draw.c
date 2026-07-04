// Raw Ghidra decompiles for the TS bottom-left STATUS HUD (client.dll).
//
// FUN_10049d10  (VA 0x10049d10) — CHudTSStatus::VidInit — loads the status sprites.
//     +0x38 stand  +0x3c crouched  +0x40 prone  +0x58 health(heart)  +0x5c kevlar
//     +0x64 kungfu +0x68 movement +0x6c flag  (icons, not bottom-left)
// FUN_1004a3e0  (VA 0x1004a3e0) — CHudTSStatus::Draw — the bottom-left status panel.
//     Reads: +0x20 health(byte) +0x21 armor(byte) +0x22 posture(byte)
//            +0x24 cash(int) +0x28 freeSlots(byte) +0x29 kungfu +0x2a movement flags
//     Engine fn ptrs: 5b28=SPR_Load 5b30=SPR_Height 5b34=SPR_Width 5b38=SPR_Set
//            5b44=SPR_DrawAdditive 5b54=FillRGBA 5b90=DrawCharacter 5bf4=GetPlayerInfo
//     FUN_1004c290 = DrawHudNumber(numspr,x,y,flags,val,r,g,b)->xEnd
//     FUN_10021ce0 = DrawConsoleString(x,y,clipX,str,r,g,b)->xEnd
//     FUN_100349a0 = LookupString/Localize("#token")
//     Globals: 100d5908=ScreenWidth 100d590c=ScreenHeight
//              1018a474/598/480 = HUD colour R/G/B (from 3 colour cvars .value)
//              1018a604 = running Y cursor, 1018a4c4 = running X cursor
// See reference/dossiers/cont87/hud_bottomleft.md for the full geometry table.
//
// ===== FUN @ 10049d10 size=352 =====

void __fastcall FUN_10049d10(int param_1)

{
  undefined4 uVar1;
  int iVar2;
  undefined4 *puVar3;
  
  puVar3 = (undefined4 *)(param_1 + 0x38);
  for (iVar2 = 8; iVar2 != 0; iVar2 = iVar2 + -1) {
    *puVar3 = 0;
    puVar3 = puVar3 + 1;
  }
  uVar1 = (*DAT_100d5b28)(s_sprites_player_stand_spr_10095a60);
  *(undefined4 *)(param_1 + 0x38) = uVar1;
  uVar1 = (*DAT_100d5b28)(s_sprites_player_crouched_spr_10095a44);
  *(undefined4 *)(param_1 + 0x3c) = uVar1;
  uVar1 = (*DAT_100d5b28)(s_sprites_player_prone_spr_10095a28);
  *(undefined4 *)(param_1 + 0x40) = uVar1;
  uVar1 = (*DAT_100d5b28)(s_sprites_player_health_spr_10095a0c);
  *(undefined4 *)(param_1 + 0x58) = uVar1;
  uVar1 = (*DAT_100d5b28)(s_sprites_player_kevlar_spr_100959f0);
  *(undefined4 *)(param_1 + 0x5c) = uVar1;
  uVar1 = (*DAT_100d5b28)(s_sprites_cursor_spr_100959dc);
  *(undefined4 *)(param_1 + 0x1434) = uVar1;
  uVar1 = (*DAT_100d5b28)(s_sprites_compass_spr_100959c8);
  *(undefined4 *)(param_1 + 0x1438) = uVar1;
  uVar1 = (*DAT_100d5b28)(s_sprites_uparrow_spr_100959b4);
  *(undefined4 *)(param_1 + 0x143c) = uVar1;
  uVar1 = (*DAT_100d5b28)(s_sprites_downarrow_spr_1009599c);
  *(undefined4 *)(param_1 + 0x1440) = uVar1;
  uVar1 = (*DAT_100d5b28)(s_sprites_player_kungfu_spr_10095980);
  *(undefined4 *)(param_1 + 100) = uVar1;
  uVar1 = (*DAT_100d5b28)(s_sprites_player_movement_spr_10095964);
  *(undefined4 *)(param_1 + 0x68) = uVar1;
  uVar1 = (*DAT_100d5b28)(s_sprites_player_flag_spr_1009594c);
  *(undefined4 *)(param_1 + 0x6c) = uVar1;
  *(undefined1 *)(param_1 + 0x22) = 0;
  *(undefined1 *)(param_1 + 0x20) = 0;
  *(undefined1 *)(param_1 + 0x35) = 0;
  *(undefined1 *)(param_1 + 0x21) = 0;
  *(undefined1 *)(param_1 + 0x29) = 0;
  *(undefined1 *)(param_1 + 0x2a) = 0;
  *(undefined1 *)(param_1 + 0x2b) = 0;
  *(undefined4 *)(param_1 + 0x14) = 0;
  *(undefined4 *)(param_1 + 0x1c) = 0;
  *(undefined4 *)(param_1 + 0x24) = 0;
  *(undefined1 *)(param_1 + 0x28) = 0;
  *(undefined4 *)(param_1 + 0x2c) = 0;
  *(undefined4 *)(param_1 + 0x30) = 0;
  *(undefined1 *)(param_1 + 0x34) = 0;
  puVar3 = (undefined4 *)(param_1 + 0x70);
  for (iVar2 = 0x270; iVar2 != 0; iVar2 = iVar2 + -1) {
    *puVar3 = 0;
    puVar3 = puVar3 + 1;
  }
  puVar3 = (undefined4 *)(param_1 + 0x1444);
  for (iVar2 = 8; iVar2 != 0; iVar2 = iVar2 + -1) {
    *puVar3 = 0;
    puVar3 = puVar3 + 1;
  }
  *(undefined1 *)puVar3 = 0;
  puVar3 = (undefined4 *)(param_1 + 0xa30);
  for (iVar2 = 0x280; iVar2 != 0; iVar2 = iVar2 + -1) {
    *puVar3 = 0;
    puVar3 = puVar3 + 1;
  }
  *(undefined4 *)(param_1 + 0x1468) = 0;
  puVar3 = (undefined4 *)(param_1 + 0x146c);
  for (iVar2 = 0x30; iVar2 != 0; iVar2 = iVar2 + -1) {
    *puVar3 = 0;
    puVar3 = puVar3 + 1;
  }
  *(undefined1 *)(param_1 + 0x152c) = 0;
  *(undefined1 *)(param_1 + 0x152d) = 0;
  *(undefined4 *)(param_1 + 0x1530) = 0;
  *(undefined4 *)(param_1 + 0x1534) = 0;
  *(undefined4 *)(param_1 + 0x1430) = 0;
  *(uint *)(param_1 + 0x10) = *(uint *)(param_1 + 0x10) | 1;
  return;
}


// ===== FUN @ 1004a3e0 size=1885 =====

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void __fastcall FUN_1004a3e0(int param_1)

{
  char cVar1;
  byte bVar2;
  undefined4 uVar3;
  int iVar4;
  uint uVar5;
  int iVar6;
  int *piVar7;
  char *pcVar8;
  undefined4 uVar9;
  uint uVar10;
  int iVar11;
  char *pcVar12;
  longlong lVar13;
  longlong lVar14;
  longlong lVar15;
  undefined4 uStack_10;
  undefined4 uStack_c;
  int iStack_8;
  int iStack_4;
  
  if (*(float *)(DAT_100df494 + 0xc) == _DAT_100799f0) {
    lVar13 = __ftol();
    DAT_1018a474 = (int)lVar13;
    lVar14 = __ftol();
    DAT_1018a598 = (int)lVar14;
    lVar15 = __ftol();
    DAT_1018a480 = (int)lVar15;
    DAT_1018a604 = DAT_100d590c + -0x30;
    DAT_1018a4c4 = 8;
    DAT_1018a594 = 0x28;
    (*DAT_100d5b54)(8,DAT_1018a604,0x7f,0x27,(int)lVar13,(int)lVar14,DAT_1018a480,0xf);
    uVar3 = (*DAT_100d5b34)(*(undefined4 *)(param_1 + 0x38 + (uint)*(byte *)(param_1 + 0x22) * 4),0)
    ;
    iVar4 = (*DAT_100d5b30)(*(undefined4 *)(param_1 + 0x38 + (uint)*(byte *)(param_1 + 0x22) * 4),0)
    ;
    bVar2 = *(byte *)(param_1 + 0x20);
    if (bVar2 < 0x65) {
      uVar5 = (uint)bVar2;
    }
    else {
      uVar5 = 100;
    }
    iVar11 = iVar4 - (int)(uVar5 * iVar4) / 100;
    iStack_8 = 0;
    uStack_10 = 0;
    iStack_4 = iVar4;
    uStack_c = uVar3;
    if (bVar2 < 100) {
      iStack_4 = iVar11;
      (*DAT_100d5b38)(*(undefined4 *)(param_1 + 0x38 + (uint)*(byte *)(param_1 + 0x22) * 4),0xc3,
                      0x2d,0x2d);
      (*DAT_100d5b44)(0,DAT_1018a4c4,DAT_1018a604 + -4,&uStack_10);
    }
    iStack_8 = iStack_4;
    iStack_4 = iVar4;
    (*DAT_100d5b38)(*(undefined4 *)(param_1 + 0x38 + (uint)*(byte *)(param_1 + 0x22) * 4),
                    DAT_1018a474 / 2,DAT_1018a598 / 2,DAT_1018a480 / 2);
    (*DAT_100d5b44)(0,DAT_1018a4c4,DAT_1018a604 + -4 + iVar11,&uStack_10);
    DAT_1018a4c4 = 0x48;
    if (*(byte *)(param_1 + 0x20) < 0x32) {
      iVar11 = 0x2d;
      iVar4 = 0x2d;
      iVar6 = 0xc3;
    }
    else {
      iVar11 = DAT_1018a480 / 2;
      iVar4 = DAT_1018a598 / 2;
      iVar6 = DAT_1018a474 / 2;
    }
    FUN_1004c290(&DAT_100cf920,0x48,DAT_1018a604 + 4,5,(uint)*(byte *)(param_1 + 0x20),iVar6,iVar4,
                 iVar11);
    if (*(byte *)(param_1 + 0x20) < 0x32) {
      iVar11 = 0x2d;
      iVar4 = 0x2d;
      iVar6 = 0xc3;
    }
    else {
      iVar11 = DAT_1018a480 / 2;
      iVar4 = DAT_1018a598 / 2;
      iVar6 = DAT_1018a474 / 2;
    }
    (*DAT_100d5b38)(*(undefined4 *)(param_1 + 0x58),iVar6,iVar4,iVar11);
    (*DAT_100d5b44)(0,0x68,DAT_1018a604,0);
    if (*(byte *)(param_1 + 0x21) != 0) {
      if (*(byte *)(param_1 + 0x21) < 0x32) {
        iVar11 = 0x2d;
        iVar4 = 0x2d;
        iVar6 = 0xc3;
      }
      else {
        iVar11 = DAT_1018a480 / 2;
        iVar4 = DAT_1018a598 / 2;
        iVar6 = DAT_1018a474 / 2;
      }
      (*DAT_100d5b38)(*(undefined4 *)(param_1 + 0x5c),iVar6,iVar4,iVar11);
      (*DAT_100d5b44)(0,DAT_1018a4c4,DAT_1018a604 + 0x14,0);
    }
    DAT_1018a4c4 = 8;
    DAT_1018a604 = DAT_1018a604 + -0x14;
    DAT_1018a594 = 0x14;
    if (DAT_100cf97c != 0) {
      piVar7 = (int *)(*DAT_100d5bf4)();
      iVar4 = *piVar7;
      uVar5 = 0xffffffff;
      pcVar8 = &DAT_101901fe + iVar4 * 0x20;
      do {
        pcVar12 = pcVar8;
        if (uVar5 == 0) break;
        uVar5 = uVar5 - 1;
        pcVar12 = pcVar8 + 1;
        cVar1 = *pcVar8;
        pcVar8 = pcVar12;
      } while (cVar1 != '\0');
      uVar5 = ~uVar5;
      pcVar8 = pcVar12 + -uVar5;
      pcVar12 = (char *)&DAT_1018a5b4;
      for (uVar10 = uVar5 >> 2; uVar10 != 0; uVar10 = uVar10 - 1) {
        *(undefined4 *)pcVar12 = *(undefined4 *)pcVar8;
        pcVar8 = pcVar8 + 4;
        pcVar12 = pcVar12 + 4;
      }
      for (uVar5 = uVar5 & 3; uVar5 != 0; uVar5 = uVar5 - 1) {
        *pcVar12 = *pcVar8;
        pcVar8 = pcVar8 + 1;
        pcVar12 = pcVar12 + 1;
      }
      iVar4 = (int)(short)(&DAT_101901fc)[iVar4 * 0x10];
      (*DAT_100d5b54)(DAT_1018a4c4,DAT_1018a604,0x7f,DAT_1018a594 + -1,(&DAT_10096974)[iVar4 * 3],
                      (&DAT_10096978)[iVar4 * 3],(&DAT_1009697c)[iVar4 * 3],0x23);
      FUN_10021ce0(DAT_1018a4c4 + 2,DAT_1018a604,DAT_100d5908,(char *)&DAT_1018a5b4,
                   (&DAT_10096974)[iVar4 * 3],(&DAT_10096978)[iVar4 * 3],(&DAT_1009697c)[iVar4 * 3])
      ;
    }
    DAT_1018a604 = DAT_100d590c + -0x1c;
    DAT_1018a4c4 = 0x88;
    DAT_1018a594 = 0x14;
    (*DAT_100d5b54)(0x88,DAT_1018a604,0x7f,0x13,DAT_1018a474,DAT_1018a598,DAT_1018a480,0xf);
    if (*(byte *)(param_1 + 0x28) == 0) {
      iVar11 = 0x2d;
      iVar4 = 0x2d;
      iVar6 = 0xc3;
    }
    else {
      iVar11 = DAT_1018a480 / 2;
      iVar4 = DAT_1018a598 / 2;
      iVar6 = DAT_1018a474 / 2;
    }
    DAT_1018a58c = FUN_1004c290(&DAT_100cf920,DAT_1018a4c4 + 8,DAT_1018a604 + 2,3,
                                (uint)*(byte *)(param_1 + 0x28),iVar6,iVar4,iVar11);
    if (*(char *)(param_1 + 0x28) == '\0') {
      iVar11 = 0x2d;
      iVar4 = 0x2d;
      iVar6 = 0xc3;
    }
    else {
      iVar11 = DAT_1018a480 / 2;
      iVar4 = DAT_1018a598 / 2;
      iVar6 = DAT_1018a474 / 2;
    }
    pcVar8 = FUN_100349a0(s__HUD_FreeSlots_10095ad4);
    FUN_10021ce0(DAT_1018a58c + 8,DAT_1018a604,DAT_100d5908,pcVar8,iVar6,iVar4,iVar11);
    if ((1 < DAT_100cf97c) || (DAT_1018d170 != 0)) {
      DAT_1018a604 = DAT_1018a604 - DAT_1018a594;
      (*DAT_100d5b54)(DAT_1018a4c4,DAT_1018a604,0x7f,DAT_1018a594 + -1,DAT_1018a474,DAT_1018a598,
                      DAT_1018a480,0xf);
      if (*(int *)(param_1 + 0x24) < 1000) {
        iVar11 = 0x2d;
        iVar4 = 0x2d;
        iVar6 = 0xc3;
      }
      else {
        iVar11 = DAT_1018a480 / 2;
        iVar4 = DAT_1018a598 / 2;
        iVar6 = DAT_1018a474 / 2;
      }
      DAT_1018a58c = FUN_1004c290(&DAT_100cf920,DAT_1018a4c4,DAT_1018a604 + 2,0x11,
                                  *(int *)(param_1 + 0x24),iVar6,iVar4,iVar11);
      if (*(int *)(param_1 + 0x24) < 1000) {
        iVar11 = 0x2d;
        iVar4 = 0x2d;
        iVar6 = 0xc3;
      }
      else {
        iVar11 = DAT_1018a480 / 2;
        iVar4 = DAT_1018a598 / 2;
        iVar6 = DAT_1018a474 / 2;
      }
      pcVar8 = FUN_100349a0(s__HUD_Credits_10095ac4);
      FUN_10021ce0(DAT_1018a58c + 8,DAT_1018a604,DAT_100d5908,pcVar8,iVar6,iVar4,iVar11);
    }
    if (*(char *)(param_1 + 0x29) != '\0') {
      DAT_1018a4c4 = DAT_100d5908 / 2 + -0x30;
      DAT_1018a604 = DAT_100d590c + -0x38;
      lVar13 = __ftol();
      uVar3 = (undefined4)lVar13;
      lVar13 = __ftol();
      uVar9 = (undefined4)lVar13;
      lVar13 = __ftol();
      (*DAT_100d5b38)(*(undefined4 *)(param_1 + 100),(int)lVar13,uVar9,uVar3);
      (*DAT_100d5b44)(0,DAT_1018a4c4,DAT_1018a604,0);
    }
    if (*(char *)(param_1 + 0x2a) != '\0') {
      DAT_1018a604 = DAT_100d590c + -0x38;
      DAT_1018a4c4 = DAT_100d5908 / 2;
      lVar13 = __ftol();
      uVar3 = (undefined4)lVar13;
      lVar13 = __ftol();
      uVar9 = (undefined4)lVar13;
      lVar13 = __ftol();
      (*DAT_100d5b38)(*(undefined4 *)(param_1 + 0x68),(int)lVar13,uVar9,uVar3);
      (*DAT_100d5b44)(0,DAT_1018a4c4,DAT_1018a604,0);
    }
  }
  return;
}


