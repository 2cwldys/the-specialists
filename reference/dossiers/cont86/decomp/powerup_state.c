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


// ===== FUN @ 00095980 size=470 =====

/* WARNING: Unknown calling convention -- yet parameter storage is locked */
/* TS_player.cpp */

undefined4 CBasePlayer::UsePowerUp(CBasePlayer *this)

{
  globalvars_t **ppgVar1;
  enginefuncs_t *peVar2;
  enginefuncs_t *peVar3;
  undefined uVar4;
  int iVar5;
  undefined4 uVar6;
  undefined3 extraout_var;
  undefined3 extraout_var_00;
  
  if ((this->pev->iuser1 == 0) && (iVar5 = (*this->vtbl->IsAlive)(this), iVar5 != 0)) {
    uVar6 = 0;
    if ((this->m_iHeldPowerup != 0) && (this->m_iPowerups == 0)) {
      if ((PTR_TSGameRules_00177300[0x18] == '\0') || ((this->m_iHeldPowerup & 9) == 0)) {
        this->m_iPowerups = this->m_iHeldPowerup;
        ppgVar1 = PTR_gpGlobals_00177410;
        this->m_flPowerupExpireTime =
             (float)(int)this->m_flPowerupTime + (*PTR_gpGlobals_00177410)->time;
        peVar2 = PTR_g_engfuncs_001778b8;
        (*PTR_g_engfuncs_001778b8->pfnMessageBegin)
                  (1,*(undefined4 *)PTR_gmsgTSPowerUpTime_00177c48,0,this->pev->pContainingEntity);
        (*peVar2->pfnWriteByte)((int)ROUND(this->m_flPowerupExpireTime - (*ppgVar1)->time));
        (*peVar2->pfnMessageEnd)();
        peVar3 = PTR_g_engfuncs_001778b8;
        if ((*(byte *)&this->m_iHeldPowerup & 1) == 0) {
          if ((*(byte *)&this->m_iHeldPowerup & 8) != 0) {
            (*PTR_g_engfuncs_001778b8->pfnMessageBegin)
                      (4,*(undefined4 *)PTR_gmsgTSMotionBlur_00177b48,&this->pev->origin,0);
            uVar4 = (*peVar3->pfnIndexOfEdict)(this->pev->pContainingEntity);
            (*peVar3->pfnWriteByte)(CONCAT31(extraout_var_00,uVar4));
            (*peVar3->pfnWriteByte)(2);
            (*peVar3->pfnMessageEnd)();
          }
        }
        else {
          (*peVar2->pfnMessageBegin)
                    (4,*(undefined4 *)PTR_gmsgTSMotionBlur_00177b48,&this->pev->origin,0);
          uVar4 = (*peVar2->pfnIndexOfEdict)(this->pev->pContainingEntity);
          (*peVar2->pfnWriteByte)(CONCAT31(extraout_var,uVar4));
          (*peVar2->pfnWriteByte)(1);
          (*peVar2->pfnMessageEnd)();
        }
        uVar6 = 1;
      }
      else {
        TSHudMessageOne__FUcUcUciiPcP9entvars_s
                  (0xc3,0x2d,0x2d,0x10,0x14,"You can\'t use Slow-Motion now!",this->pev);
        uVar6 = 0;
      }
    }
  }
  else {
    uVar6 = 0;
  }
  return uVar6;
}


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


