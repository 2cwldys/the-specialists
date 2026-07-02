// ===== FUN @ 0007f1c4 size=48 =====

/* WARNING: Unknown calling convention -- yet parameter storage is locked */
/* TS_gun.cpp */

undefined4 * get_ammo_info__Fi(int a0)

{
  undefined4 *puVar1;
  
  puVar1 = &global_ammo_info;
  if ((uint)a0 < 0x10) {
    puVar1 = &global_ammo_info + a0 * 0xe;
  }
  return puVar1;
}
