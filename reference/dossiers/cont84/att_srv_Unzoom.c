// ===== FUN @ 00081748 size=96 =====

/* WARNING: Unknown calling convention -- yet parameter storage is locked */
/* TS_gun.cpp */

void CTSGun::Unzoom(CTSGun *this)

{
  entvars_t *peVar1;
  
  this->m_pPlayer->pev->fov = 0.0;
  peVar1 = this->m_pPlayer->pev;
  if ((peVar1->viewmodel == 0) && (this->m_pActiveInfo != (s_weapon_info *)0x0)) {
    peVar1->viewmodel =
         (int)this->m_pActiveInfo + (0x132 - (int)(*PTR_gpGlobals_00177410)->pStringBase);
  }
  return;
}


