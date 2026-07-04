// ===== FUN @ 000c9f44 size=54 =====

/* WARNING: Unknown calling convention -- yet parameter storage is locked */
/* hldmcompat.cpp */

void CTSSuperJump::Spawn(CTSSuperJump *this)

{
  this->field_0x84 = 0x100;
  CTSPowerUp::Spawn((CTSPowerUp *)this);
  PTR_TSGameRules_00177300[1] = 0xff;
  return;
}


