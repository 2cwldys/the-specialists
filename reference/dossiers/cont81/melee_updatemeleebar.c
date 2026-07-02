// ===== FUN @ 0009650c size=192 =====

/* WARNING: Unknown calling convention -- yet parameter storage is locked */
/* TS_player.cpp */

void CBasePlayer::UpdateMeleeBar(CBasePlayer *this,float a0)

{
  float fVar1;
  enginefuncs_t *peVar2;
  
  fVar1 = (float)this->field_0x774 - a0;
  if (fc_0_009999999776482582 <= fVar1) {
    if (fc_neg0_0010000000474974513 <= fVar1) {
      this->field_0x774 = (int)fVar1;
    }
    else {
      this->field_0x774 = 0;
    }
  }
  else {
    this->field_0x774 = 0;
    this->field_0x12c = 0x3fc00000;
  }
  peVar2 = PTR_g_engfuncs_001778b8;
  (*PTR_g_engfuncs_001778b8->pfnMessageBegin)
            (1,*(undefined4 *)PTR_gmsgKFuPower_00177488,0,this->pev->pContainingEntity);
  (*peVar2->pfnWriteChar)((int)ROUND((float)this->field_0x774));
  (*peVar2->pfnMessageEnd)();
  return;
}


