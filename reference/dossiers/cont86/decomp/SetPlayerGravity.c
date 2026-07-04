// ===== FUN @ 00095b70 size=60 =====

/* WARNING: Unknown calling convention -- yet parameter storage is locked */
/* TS_player.cpp */

void CBasePlayer::SetPlayerGravity(CBasePlayer *this,float a0,float a1)

{
  if ((float)dc_1 <= this->pev->gravity) {
    this->pev->gravity = a1;
    this->field_0x4ec = (int)a0;
  }
  return;
}


