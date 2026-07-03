// ===== FUN @ 00081710 size=52 =====

/* WARNING: Unknown calling convention -- yet parameter storage is locked */
/* TS_gun.cpp */

void CTSGun::Zoom(CTSGun *this,int a0)

{
  this->m_pPlayer->pev->fov = (float)a0;
  if (this->m_pActiveInfo->zoomFov1 < 0x28) {
    this->m_pPlayer->pev->viewmodel = 0;
  }
  return;
}


