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


