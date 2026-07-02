// ===== FUN @ 0009f320 size=584 =====

/* WARNING: Unknown calling convention -- yet parameter storage is locked */
/* TS_util.cpp
   assets: player/superjump-land.wav, player/land.wav, player/dive.wav, player/airwheel.wav,
   player/headshot.wav, player/headshot2.wav, player/kevlarhit.wav, player/block.wav,
   player/splat.wav, player/bodyhit1.wav, player/bodyhit2.wav, player/bodyhit3.wav,
   player/bodyhit4.wav, player/die.wav, player/die1.wav, player/die2.wav, player/die3.wav,
   player/pain1.wav, player/pain2.wav, player/pain3.wav, player/pain4.wav, misc/hacking.wav,
   misc/hacked.wav, misc/failed.wav, misc/successful.wav, misc/gogogo.wav, debris/ric1.wav,
   debris/ric2.wav, debris/ric3.wav, debris/ric4.wav, debris/ric5.wav, debris/cement.wav,
   debris/cement1.wav, debris/cement2.wav, debris/cement3.wav, debris/cement4.wav,
   debris/cement5.wav, debris/cement6.wav, debris/gravel1.wav, debris/gravel2.wav,
   debris/gravel3.wav, debris/gravel4.wav, debris/metal1.wav, debris/metal2.wav, debris/metal3.wav,
   debris/metal4.wav, debris/metal5.wav, weapons/shell.wav, weapons/sshell.wav,
   weapons/pistol-empty.wav, weapons/insert-shell.wav, goslow.wav, weapons/whizz1.wav,
   weapons/whizz2.wav, weapons/whizz3.wav, weapons/whizz4.wav, weapons/whizz5.wav,
   player/closecombat.wav, player/kungfuhit.wav, weapons/weapondrop.wav, weapons/items.wav,
   weapons/draw.wav, weapons/switch.wav, items/9mmclip1.wav, weapons/generic_reload.wav,
   explo/explode.wav, explo/explode1.wav, explo/explode2.wav, explo/explode3.wav,
   explo/explode4.wav, models/generic_clip.mdl, models/ammopack.mdl, models/bullet.mdl,
   models/long_bullet.mdl, models/pellet.mdl, weapons/knife/knife_miss.wav,
   weapons/knife/knife_hit.wav, weapons/knife/knife_hitbody.wav, weapons/katana/hit-body.wav,
   weapons/katana/miss1.wav, weapons/katana/miss.wav, weapons/katana/hit.wav,
   weapons/katana/hit1.wav */

void TSPrecache__Fv(void)

{
  enginefuncs_t *peVar1;
  enginefuncs_t *peVar2;
  uint uVar3;
  int iVar4;
  byte *pbVar5;
  byte *pbVar6;
  bool bVar7;
  bool bVar8;
  int iStack_14;
  
  peVar1 = PTR_g_engfuncs_001778b8;
  (*PTR_g_engfuncs_001778b8->pfnPrecacheSound)("player/superjump-land.wav");
  (*peVar1->pfnPrecacheSound)("player/land.wav");
  (*peVar1->pfnPrecacheSound)("player/dive.wav");
  (*peVar1->pfnPrecacheSound)("player/airwheel.wav");
  (*peVar1->pfnPrecacheSound)("player/headshot.wav");
  (*peVar1->pfnPrecacheSound)("player/headshot2.wav");
  (*peVar1->pfnPrecacheSound)("player/kevlarhit.wav");
  (*peVar1->pfnPrecacheSound)("player/block.wav");
  (*peVar1->pfnPrecacheSound)("player/splat.wav");
  (*peVar1->pfnPrecacheSound)("player/bodyhit1.wav");
  (*peVar1->pfnPrecacheSound)("player/bodyhit2.wav");
  (*peVar1->pfnPrecacheSound)("player/bodyhit3.wav");
  (*peVar1->pfnPrecacheSound)("player/bodyhit4.wav");
  (*peVar1->pfnPrecacheSound)("player/die.wav");
  (*peVar1->pfnPrecacheSound)("player/die1.wav");
  (*peVar1->pfnPrecacheSound)("player/die2.wav");
  (*peVar1->pfnPrecacheSound)("player/die3.wav");
  (*peVar1->pfnPrecacheSound)("player/pain1.wav");
  (*peVar1->pfnPrecacheSound)("player/pain2.wav");
  (*peVar1->pfnPrecacheSound)("player/pain3.wav");
  (*peVar1->pfnPrecacheSound)("player/pain4.wav");
  (*peVar1->pfnPrecacheSound)("misc/hacking.wav");
  (*peVar1->pfnPrecacheSound)("misc/hacked.wav");
  (*peVar1->pfnPrecacheSound)("misc/failed.wav");
  (*peVar1->pfnPrecacheSound)("misc/successful.wav");
  (*peVar1->pfnPrecacheSound)("misc/gogogo.wav");
  (*peVar1->pfnPrecacheSound)("debris/ric1.wav");
  (*peVar1->pfnPrecacheSound)("debris/ric2.wav");
  (*peVar1->pfnPrecacheSound)("debris/ric3.wav");
  (*peVar1->pfnPrecacheSound)("debris/ric4.wav");
  (*peVar1->pfnPrecacheSound)("debris/ric5.wav");
  (*peVar1->pfnPrecacheSound)("debris/cement.wav");
  (*peVar1->pfnPrecacheSound)("debris/cement1.wav");
  (*peVar1->pfnPrecacheSound)("debris/cement2.wav");
  (*peVar1->pfnPrecacheSound)("debris/cement3.wav");
  (*peVar1->pfnPrecacheSound)("debris/cement4.wav");
  (*peVar1->pfnPrecacheSound)("debris/cement5.wav");
  (*peVar1->pfnPrecacheSound)("debris/cement6.wav");
  (*peVar1->pfnPrecacheSound)("debris/gravel1.wav");
  (*peVar1->pfnPrecacheSound)("debris/gravel2.wav");
  (*peVar1->pfnPrecacheSound)("debris/gravel3.wav");
  (*peVar1->pfnPrecacheSound)("debris/gravel4.wav");
  (*peVar1->pfnPrecacheSound)("debris/metal1.wav");
  (*peVar1->pfnPrecacheSound)("debris/metal2.wav");
  (*peVar1->pfnPrecacheSound)("debris/metal3.wav");
  (*peVar1->pfnPrecacheSound)("debris/metal4.wav");
  (*peVar1->pfnPrecacheSound)("debris/metal5.wav");
  (*peVar1->pfnPrecacheSound)("weapons/shell.wav");
  (*peVar1->pfnPrecacheSound)("weapons/sshell.wav");
  iStack_14 = 0;
  do {
    uVar3 = get_weapon_info__F10tsweapon_t(iStack_14);
    if (uVar3 != 0xffffff18) {
      bVar7 = 0xffffff17 < uVar3;
      bVar8 = (byte *)(uVar3 + 0xe8) == (byte *)0x0;
      iVar4 = 1;
      pbVar5 = (byte *)(uVar3 + 0xe8);
      pbVar6 = &DAT_00106657;
      do {
        if (iVar4 == 0) break;
        iVar4 = iVar4 + -1;
        bVar7 = *pbVar5 < *pbVar6;
        bVar8 = *pbVar5 == *pbVar6;
        pbVar5 = pbVar5 + 1;
        pbVar6 = pbVar6 + 1;
      } while (bVar8);
      if ((!bVar7 && !bVar8) != bVar7) {
        (*peVar1->pfnPrecacheModel)(uVar3 + 0xe8);
      }
    }
    if (uVar3 != 0xfffffece) {
      bVar7 = 0xfffffecd < uVar3;
      bVar8 = (byte *)(uVar3 + 0x132) == (byte *)0x0;
      iVar4 = 1;
      pbVar5 = (byte *)(uVar3 + 0x132);
      pbVar6 = &DAT_00106657;
      do {
        if (iVar4 == 0) break;
        iVar4 = iVar4 + -1;
        bVar7 = *pbVar5 < *pbVar6;
        bVar8 = *pbVar5 == *pbVar6;
        pbVar5 = pbVar5 + 1;
        pbVar6 = pbVar6 + 1;
      } while (bVar8);
      if ((!bVar7 && !bVar8) != bVar7) {
        (*peVar1->pfnPrecacheModel)(uVar3 + 0x132);
      }
    }
    if (uVar3 != 0xfffffe8e) {
      bVar7 = 0xfffffe8d < uVar3;
      bVar8 = (byte *)(uVar3 + 0x172) == (byte *)0x0;
      iVar4 = 1;
      pbVar5 = (byte *)(uVar3 + 0x172);
      pbVar6 = &DAT_00106657;
      do {
        if (iVar4 == 0) break;
        iVar4 = iVar4 + -1;
        bVar7 = *pbVar5 < *pbVar6;
        bVar8 = *pbVar5 == *pbVar6;
        pbVar5 = pbVar5 + 1;
        pbVar6 = pbVar6 + 1;
      } while (bVar8);
      if ((!bVar7 && !bVar8) != bVar7) {
        (*peVar1->pfnPrecacheModel)(uVar3 + 0x172);
      }
    }
    if (uVar3 != 0xfffffe4e) {
      bVar7 = 0xfffffe4d < uVar3;
      bVar8 = (byte *)(uVar3 + 0x1b2) == (byte *)0x0;
      iVar4 = 1;
      pbVar5 = (byte *)(uVar3 + 0x1b2);
      pbVar6 = &DAT_00106657;
      do {
        if (iVar4 == 0) break;
        iVar4 = iVar4 + -1;
        bVar7 = *pbVar5 < *pbVar6;
        bVar8 = *pbVar5 == *pbVar6;
        pbVar5 = pbVar5 + 1;
        pbVar6 = pbVar6 + 1;
      } while (bVar8);
      if ((!bVar7 && !bVar8) != bVar7) {
        (*peVar1->pfnPrecacheModel)(uVar3 + 0x1b2);
      }
    }
    if (uVar3 != 0xfffffe0e) {
      bVar7 = 0xfffffe0d < uVar3;
      bVar8 = (byte *)(uVar3 + 0x1f2) == (byte *)0x0;
      iVar4 = 1;
      pbVar5 = (byte *)(uVar3 + 0x1f2);
      pbVar6 = &DAT_00106657;
      do {
        if (iVar4 == 0) break;
        iVar4 = iVar4 + -1;
        bVar7 = *pbVar5 < *pbVar6;
        bVar8 = *pbVar5 == *pbVar6;
        pbVar5 = pbVar5 + 1;
        pbVar6 = pbVar6 + 1;
      } while (bVar8);
      if ((!bVar7 && !bVar8) != bVar7) {
        (*peVar1->pfnPrecacheModel)(uVar3 + 0x1f2);
      }
    }
    if (uVar3 != 0xfffffd3a) {
      bVar7 = 0xfffffd39 < uVar3;
      bVar8 = (byte *)(uVar3 + 0x2c6) == (byte *)0x0;
      iVar4 = 1;
      pbVar5 = (byte *)(uVar3 + 0x2c6);
      pbVar6 = &DAT_00106657;
      do {
        if (iVar4 == 0) break;
        iVar4 = iVar4 + -1;
        bVar7 = *pbVar5 < *pbVar6;
        bVar8 = *pbVar5 == *pbVar6;
        pbVar5 = pbVar5 + 1;
        pbVar6 = pbVar6 + 1;
      } while (bVar8);
      if ((!bVar7 && !bVar8) != bVar7) {
        (*peVar1->pfnPrecacheSound)(uVar3 + 0x2c6);
      }
    }
    if (uVar3 != 0xfffffcfa) {
      bVar7 = 0xfffffcf9 < uVar3;
      bVar8 = (byte *)(uVar3 + 0x306) == (byte *)0x0;
      iVar4 = 1;
      pbVar5 = (byte *)(uVar3 + 0x306);
      pbVar6 = &DAT_00106657;
      do {
        if (iVar4 == 0) break;
        iVar4 = iVar4 + -1;
        bVar7 = *pbVar5 < *pbVar6;
        bVar8 = *pbVar5 == *pbVar6;
        pbVar5 = pbVar5 + 1;
        pbVar6 = pbVar6 + 1;
      } while (bVar8);
      if ((!bVar7 && !bVar8) != bVar7) {
        (*peVar1->pfnPrecacheSound)(uVar3 + 0x306);
      }
    }
    peVar2 = PTR_g_engfuncs_001778b8;
    iStack_14 = iStack_14 + 1;
    if (0x25 < iStack_14) {
      (*PTR_g_engfuncs_001778b8->pfnPrecacheSound)("weapons/pistol-empty.wav");
      (*peVar2->pfnPrecacheSound)("weapons/insert-shell.wav");
      (*peVar2->pfnPrecacheSound)("goslow.wav");
      (*peVar2->pfnPrecacheSound)("weapons/whizz1.wav");
      (*peVar2->pfnPrecacheSound)("weapons/whizz2.wav");
      (*peVar2->pfnPrecacheSound)("weapons/whizz3.wav");
      (*peVar2->pfnPrecacheSound)("weapons/whizz4.wav");
      (*peVar2->pfnPrecacheSound)("weapons/whizz5.wav");
      (*peVar2->pfnPrecacheSound)("player/closecombat.wav");
      (*peVar2->pfnPrecacheSound)("player/kungfuhit.wav");
      (*peVar2->pfnPrecacheSound)("weapons/weapondrop.wav");
      (*peVar2->pfnPrecacheSound)("weapons/items.wav");
      (*peVar2->pfnPrecacheSound)("weapons/draw.wav");
      (*peVar2->pfnPrecacheSound)("weapons/switch.wav");
      (*peVar2->pfnPrecacheSound)("items/9mmclip1.wav");
      (*peVar2->pfnPrecacheSound)("weapons/generic_reload.wav");
      (*peVar2->pfnPrecacheSound)("explo/explode.wav");
      (*peVar2->pfnPrecacheSound)("explo/explode1.wav");
      (*peVar2->pfnPrecacheSound)("explo/explode2.wav");
      (*peVar2->pfnPrecacheSound)("explo/explode3.wav");
      (*peVar2->pfnPrecacheSound)("explo/explode4.wav");
      (*peVar2->pfnPrecacheModel)("models/generic_clip.mdl");
      (*peVar2->pfnPrecacheModel)("models/ammopack.mdl");
      (*peVar2->pfnPrecacheModel)("models/bullet.mdl");
      (*peVar2->pfnPrecacheModel)("models/long_bullet.mdl");
      (*peVar2->pfnPrecacheModel)("models/pellet.mdl");
      UTIL_PrecacheOther__FPCc("m61_grenade");
      UTIL_PrecacheOther__FPCc("shrapnel");
      UTIL_PrecacheOther__FPCc("ts_flag");
      (*peVar2->pfnPrecacheSound)("weapons/knife/knife_miss.wav");
      (*peVar2->pfnPrecacheSound)("weapons/knife/knife_hit.wav");
      (*peVar2->pfnPrecacheSound)(&DAT_00107ee0);
      (*peVar2->pfnPrecacheSound)("weapons/katana/hit-body.wav");
      (*peVar2->pfnPrecacheSound)("weapons/katana/miss1.wav");
      (*peVar2->pfnPrecacheSound)("weapons/katana/miss.wav");
      (*peVar2->pfnPrecacheSound)("weapons/katana/hit.wav");
      (*peVar2->pfnPrecacheSound)("weapons/katana/hit1.wav");
      return;
    }
  } while( true );
}
