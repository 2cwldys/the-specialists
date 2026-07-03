// ===== FUN @ 000817a8 size=534 =====

/* WARNING: Unknown calling convention -- yet parameter storage is locked */
/* TS_gun.cpp */

void CTSGun::SecondaryAttack(CTSGun *this)

{
  s_weapon_status *psVar1;
  entvars_t *peVar2;
  s_weapon_info *psVar3;
  int iVar4;
  longdouble lVar5;
  if (this->m_pActiveItem == (s_weapon_status *)0x0) {
    CCombatAttack(this);
    return;
  }
  if (this->m_pActiveInfo->iFireType == '\x02') {
    this->m_pActiveItem->pumpToggle = '\x01';
    GrenadeFire(this);
    return;
  }
  if (this->m_pActiveInfo->iFireType == '\x03') {
    this->m_pActiveItem->pumpToggle = '\x01';
    KnifeFire(this);
    return;
  }
  if (this->m_pActiveInfo->iFireType == '\n') {
    KatanaThrow(this);
    return;
  }
  if (this->m_pActiveInfo->iFireType == '\v') {
    return;
  }
  if (this->m_pActiveItem->fireMode == '\x04') {
    if (((((CBasePlayer *)this->m_pPlayer)->m_afButtonPressed & 0x800) == 0) &&
       (iVar4 = DoubleFireRate__FP11CBasePlayer((CBasePlayer *)this->m_pPlayer), iVar4 == 0)) {
      return;
    }
  }
  else if (this->m_pActiveItem->fireMode != '\x05') {
    psVar1 = this->m_pActiveItem;
    if ((psVar1->attachEquipped & 8) == 0) {
      if ((psVar1->holdPosition == '\0') && (*(short *)&this->m_pActiveInfo->field_0x28c != -1)) {
        psVar1->holdPosition = '\x01';
        if (*(int *)&this->m_pActiveItem->field_0x28 == 0) {
          iVar4 = 0;
        }
        else {
          iVar4 = (int)*(short *)(*(int *)&this->m_pActiveItem->field_0x28 + 0xe);
        }
        (*this->vtbl->SendWeaponAnim)((CBasePlayerWeapon *)this,iVar4,1,this->pev->body);
        *(undefined1 **)&this->m_pActiveItem->field_0x28 = &this->m_pActiveInfo->field_0x28c;
      }
      else if (*(short *)&this->m_pActiveInfo->field_0x28c != -1) {
        this->m_pActiveItem->holdPosition = '\0';
        if (*(int *)&this->m_pActiveItem->field_0x28 == 0) {
          iVar4 = 0;
        }
        else {
          iVar4 = (int)*(short *)(*(int *)&this->m_pActiveItem->field_0x28 + 0xe);
        }
        (*this->vtbl->SendWeaponAnim)((CBasePlayerWeapon *)this,iVar4,1,this->pev->body);
        *(undefined1 **)&this->m_pActiveItem->field_0x28 = &this->m_pActiveInfo->field_0x252;
      }
    }
    else {
      peVar2 = this->m_pPlayer->pev;
      if (fc_0 == peVar2->fov) {
        if (this->m_pActiveInfo->zoomFov1 == 0) goto LAB_000819f3;
        iVar4 = this->m_pActiveInfo->zoomFov1;
      }
      else {
        psVar3 = this->m_pActiveInfo;
        if ((psVar3->zoomFov2 == 0) || ((float)psVar3->zoomFov2 == peVar2->fov)) {
          Unzoom(this);
          goto LAB_000819f3;
        }
        iVar4 = psVar3->zoomFov2;
      }
      Zoom(this,iVar4);
    }
LAB_000819f3:
    lVar5 = (longdouble)UTIL_WeaponTimeBase__Fv();
    this->field_0xa0 = (int)(float)((longdouble)dc_0_3 + lVar5);
    lVar5 = (longdouble)UTIL_WeaponTimeBase__Fv();
    this->field_0xa8 = (int)(float)((longdouble)dc_1 + lVar5);
    return;
  }
  Fire(this);
  return;
}


