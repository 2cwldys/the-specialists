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


// ===== FUN @ 00093068 size=164 =====

/* WARNING: Unknown calling convention -- yet parameter storage is locked */
/* TS_player.cpp */

void CBasePlayer::SetPlayerSpeed(CBasePlayer *this,float a0)

{
  this->field_0x550 = (int)a0;
  if (fc_330 < a0) {
    this->field_0x550 = 0x43a50000;
  }
  if ((float)this->field_0x550 < fc_0) {
    this->field_0x550 = 0;
  }
  (*PTR_g_engfuncs_001778b8->pfnSetClientMaxspeed)
            (this->pev->pContainingEntity,(float)(int)ROUND((float)this->field_0x550));
  return;
}


