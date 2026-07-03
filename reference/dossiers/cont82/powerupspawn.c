// ===== FUN @ 00098258 size=369 =====

/* WARNING: Unknown calling convention -- yet parameter storage is locked */
/* TS_powerup.cpp
   assets: models/powerup.mdl */

void CTSPowerUp::Spawn(CTSPowerUp *this)

{
  globalvars_t **ppgVar1;
  undefined *puVar2;
  Vector local_2c;
  Vector local_1c;
  
  (*this->vtbl->Precache)(this);
  if (PTR_TSGameRules_00177300[1] == -1) {
    PTR_TSGameRules_00177300[1] = 0;
  }
  ppgVar1 = PTR_gpGlobals_00177410;
  this->pev->classname = (int)"ts_powerup" - (int)(*PTR_gpGlobals_00177410)->pStringBase;
  (*PTR_g_engfuncs_001778b8->pfnSetModel)(this->pev->pContainingEntity,"models/powerup.mdl");
  this->pev->movetype = 6;
  this->pev->solid = 2;
  this->pev->renderamt = 255.0;
  this->pev->rendermode = 1;
  (this->pev->rendercolor).x = 0.0;
  (this->pev->rendercolor).y = 0.0;
  (this->pev->rendercolor).z = 0.0;
  this->pev->renderfx = 0xe;
  UTIL_SetOrigin__FP9entvars_sRC6Vector(this->pev,&this->pev->origin);
  local_2c.x = fc_0;
  local_2c.y = local_2c.x;
  local_2c.z = local_2c.x;
  local_1c.x = local_2c.x;
  local_1c.y = local_2c.x;
  local_1c.z = local_2c.x;
  UTIL_SetSize__FP9entvars_sRC6VectorT1(this->pev,&local_2c,&local_1c);
  this->m_pfnTouch = (void *)0x0;
  this->field_0x18 = 0;
  puVar2 = PTR_Fall_00177c28;
  this->m_pfnThink = (void *)0xffff0000;
  this->field_0x10 = (int)puVar2;
  this->pev->nextthink = (float)dc_0_1 + (*ppgVar1)->time;
  CBaseAnimating::ResetSequenceInfo((CBaseAnimating *)this);
  this->pev->framerate = 1.0;
  SetupPowerUp(this,this->poweruptype,-1,0.0,true);
  return;
}


