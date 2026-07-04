// ===== FUN @ 00095748 size=482 =====

/* WARNING: Unknown calling convention -- yet parameter storage is locked */
/* TS_player.cpp */

void CBasePlayer::UpdatePowerUpState(CBasePlayer *this)

{
  int iVar1;
  int *piVar2;
  Vector *pVVar3;
  vec_t *pvVar4;
  int in_stack_ffffffdc;
  
  if (this->m_iHeldPowerup != 0) {
    if (this->m_iPowerups == 0) {
      iVar1 = (*this->vtbl->IsAlive)(this);
      if (iVar1 != 0) {
        return;
      }
      if (((byte)*PTR_TSGameRules_00177300 < 2) &&
         ((((*(byte *)&this->m_iHeldPowerup ^ 1) & 1) != 0 ||
          (((this->m_iHeldPowerup & 1) != 0 && (2 < (int)this->m_flPowerupTime)))))) {
        piVar2 = (int *)CTSPowerUp::CreatePowerUp
                                  ((CTSPowerUp *)this->m_iHeldPowerup,-1,in_stack_ffffffdc);
        pVVar3 = &this->pev->origin;
        pvVar4 = (vec_t *)(*piVar2 + 8);
        for (iVar1 = 3; iVar1 != 0; iVar1 = iVar1 + -1) {
          *pvVar4 = pVVar3->x;
          pVVar3 = (Vector *)&pVVar3->y;
          pvVar4 = pvVar4 + 1;
        }
      }
    }
    else {
      if ((this->m_iPowerups & 9) == 0) {
        if ((this->m_iPowerups & 0x100) != 0) {
          this->pev->gravity = 0.15;
        }
      }
      else if ((this->m_flPowerupExpireTime < (*PTR_gpGlobals_00177410)->time) ||
              (iVar1 = (*this->vtbl->IsAlive)(this), iVar1 == 0)) {
        DeactivateSlowMotionFlow(this);
      }
      else if ((this->m_iPowerups & 8) == 0) {
        if ((this->m_iPowerups & 1) != 0) {
          this->field_0x175 = 0;
          this->m_flSlowMotionRamp = 0.35;
          this->m_flSlowMotionScale = 0.6;
        }
      }
      else {
        this->field_0x175 = 1;
        this->m_flSlowMotionRamp = 0.225;
        this->m_flSlowMotionScale = 0.8;
      }
      if (((*PTR_gpGlobals_00177410)->time <= this->m_flPowerupExpireTime) &&
         (iVar1 = (*this->vtbl->IsAlive)(this), iVar1 != 0)) {
        return;
      }
      iVar1 = (*this->vtbl->IsAlive)(this);
      if (((iVar1 == 0) && ((byte)*PTR_TSGameRules_00177300 < 2)) &&
         (((((byte)this->m_iPowerups ^ 1) & 1) != 0 ||
          (((this->m_iPowerups & 1) != 0 && (2 < (int)this->m_flPowerupTime)))))) {
        piVar2 = (int *)CTSPowerUp::CreatePowerUp
                                  ((CTSPowerUp *)this->m_iPowerups,-1,in_stack_ffffffdc);
        pVVar3 = &this->pev->origin;
        pvVar4 = (vec_t *)(*piVar2 + 8);
        for (iVar1 = 3; iVar1 != 0; iVar1 = iVar1 + -1) {
          *pvVar4 = pVVar3->x;
          pVVar3 = (Vector *)&pVVar3->y;
          pvVar4 = pvVar4 + 1;
        }
      }
      if ((this->m_iPowerups & 0x100) != 0) {
        this->pev->gravity = 1.0;
      }
    }
    this->m_flPowerupExpireTime = 0.0;
    this->m_flPowerupTime = 0;
    this->m_iPowerups = 0;
    this->m_iHeldPowerup = 0;
  }
  return;
}


