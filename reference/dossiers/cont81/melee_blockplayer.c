// ===== FUN @ 00095b58 size=24 =====

/* WARNING: Unknown calling convention -- yet parameter storage is locked */
/* TS_player.cpp */

void CBasePlayer::BlockPlayer(CBasePlayer *this,float a0)

{
  int *piVar1;
  
  this->field_0x4dc = (int)a0;
  piVar1 = &this->pev->iuser4;
  *piVar1 = *piVar1 | 1;
  return;
}


