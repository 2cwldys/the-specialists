// ===== FUN @ 00076bd4 size=117 =====

/* WARNING: Unknown calling convention -- yet parameter storage is locked */
/* TS_gamerules.cpp */

CBasePlayer * GetSpeedBySlots__FP11CBasePlayer(CBasePlayer *a0)

{
  if ((a0->field_0x7b9 == '\0') && (a0->field_0x548 < 0x51)) {
    a0 = (CBasePlayer *)
         (int)ROUND(fc_0_012345679104328156 * (float)a0->field_0x548 * fc_120 + 210.0);
  }
  return a0;
}


