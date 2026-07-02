// ===== FUN @ 0007f290 size=53 =====

/* WARNING: Unknown calling convention -- yet parameter storage is locked */
/* TS_gun.cpp */

longdouble get_range_multipler__Fi(int a0)

{
  longdouble lVar1;
  
  if (a0 < 0) {
    lVar1 = (longdouble)range_damage_multipler;
  }
  else if (a0 < 10000) {
    lVar1 = (longdouble)(&range_damage_multipler)[a0];
  }
  else {
    lVar1 = (longdouble)DAT_00190dbc;
  }
  return lVar1;
}
