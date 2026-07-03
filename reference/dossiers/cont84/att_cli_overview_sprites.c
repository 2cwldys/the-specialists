// ===== FUN @ 10022fe0 size=159 =====

undefined4 __fastcall FUN_10022fe0(int param_1)

{
  undefined4 uVar1;
  
  uVar1 = (*DAT_100d5b28)(s_sprites_iplayer_spr_10093c6c);
  *(undefined4 *)(param_1 + 0xc74) = uVar1;
  uVar1 = (*DAT_100d5b28)(s_sprites_iplayerblue_spr_10093c54);
  *(undefined4 *)(param_1 + 0xc6c) = uVar1;
  uVar1 = (*DAT_100d5b28)(s_sprites_iplayerred_spr_10093c3c);
  *(undefined4 *)(param_1 + 0xc70) = uVar1;
  uVar1 = (*DAT_100d5b28)(s_sprites_iplayerdead_spr_10093c24);
  *(undefined4 *)(param_1 + 0xc7c) = uVar1;
  uVar1 = (*DAT_100d5b28)(s_sprites_tile_spr_10093c10);
  *(undefined4 *)(param_1 + 0xc84) = uVar1;
  uVar1 = (*DAT_100d5b28)(s_sprites_laserbeam_spr_10093bf8);
  *(undefined4 *)(param_1 + 0xc88) = uVar1;
  uVar1 = (*DAT_100d5b28)(s_sprites_camera_spr_10093be4);
  *(undefined4 *)(param_1 + 0xc78) = uVar1;
  uVar1 = (*DAT_100d5b28)(s_sprites_crosshairs_spr_10093bcc);
  *(undefined4 *)(param_1 + 0xc8c) = uVar1;
  *(undefined4 *)(param_1 + 0xc94) = 0;
  return 1;
}


