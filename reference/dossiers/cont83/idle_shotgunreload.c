// ===== FUN @ 000854dc size=400 =====

/* WARNING: Unknown calling convention -- yet parameter storage is locked */
/* TS_gun.cpp */

void CTSGun::WeaponIdle(CTSGun *this)

{
  int *piVar1;
  undefined1 *puVar2;
  int iVar3;
  s_weapon_status *psVar4;
  longdouble lVar5;
  
  if (this->m_pActiveItem == (s_weapon_status *)0x0) {
    CCombatIdle(this);
  }
  else {
    psVar4 = this->m_pActiveItem;
    this->field_0xb4 = psVar4->iClip;
    if (psVar4->holdPosition == '\0') {
      puVar2 = &this->m_pActiveInfo->field_0x252;
    }
    else {
      psVar4 = this->m_pActiveItem;
      puVar2 = &this->m_pActiveInfo->field_0x28c;
    }
    *(undefined1 **)&psVar4->field_0x28 = puVar2;
    if ((this->m_pActiveInfo->iFireType == '\x01') && (this->m_pActiveItem->burstOrPhase != 0)) {
      (*this->vtbl->Reload)(this);
    }
    else {
      if ((this->m_pActiveItem->fireMode == '\x02') && ((float)this->field_0x9c <= (float)dc_0)) {
        if (this->m_pActiveItem->burstOrPhase < 1) {
          this->m_pActiveItem->burstOrPhase = 0;
        }
        else {
          Fire(this);
          piVar1 = &this->m_pActiveItem->burstOrPhase;
          *piVar1 = *piVar1 + -1;
        }
      }
      lVar5 = (longdouble)UTIL_WeaponTimeBase__Fv();
      if ((longdouble)(float)this->field_0xa8 <= lVar5) {
        if (this->m_pActiveInfo->iFireType == '\x02') {
          GrenadeIdle(this,0);
        }
        else {
          iVar3 = InfiniteClips__FP11CBasePlayer((CBasePlayer *)this->m_pPlayer);
          if ((iVar3 == 0) && (this->m_pActiveItem->iClip < 1)) {
            (*this->vtbl->Reload)(this);
          }
          else if (this->m_pActiveItem->iClip != 0) {
            lVar5 = (longdouble)UTIL_WeaponTimeBase__Fv();
            this->field_0xa8 = (int)(float)((longdouble)dc_3 + lVar5);
            if (*(int *)&this->m_pActiveItem->field_0x28 == 0) {
              iVar3 = 0;
            }
            else {
              iVar3 = (int)**(short **)&this->m_pActiveItem->field_0x28;
            }
            (*this->vtbl->SendWeaponAnim)((CBasePlayerWeapon *)this,iVar3,0,this->pev->body);
          }
        }
      }
    }
  }
  return;
}


// ===== FUN @ 00084aac size=682 =====

/* WARNING: Unknown calling convention -- yet parameter storage is locked */
/* TS_gun.cpp */

void CTSGun::ShotgunReload(CTSGun *this)

{
  int *piVar1;
  entvars_t *peVar2;
  longdouble lVar3;
  
  if (this->m_pActiveItem->burstOrPhase == 0) {
    if ((0 < (int)(&this->m_ammoReserve)[(byte)this->m_pActiveInfo->iCaliber]) &&
       (this->m_pActiveItem->iClip < this->m_pActiveInfo->iClip)) {
      Unzoom(this);
      peVar2 = this->m_pPlayer->pev;
      if (peVar2->viewmodel == 0) {
        peVar2->viewmodel =
             (int)this->m_pActiveInfo + (0x132 - (int)(*PTR_gpGlobals_00177410)->pStringBase);
      }
      this->m_pActiveItem->burstOrPhase = 1;
      (*PTR_g_engfuncs_001778b8->pfnPlaybackEvent)
                (1,this->m_pPlayer->pev->pContainingEntity,(uint)*(ushort *)&this->field_0x970,0,
                 PTR_g_vecZero_00177698,PTR_g_vecZero_00177698,0x41000000,0,this->m_weaponType,
                 this->m_pActiveItem->attachEquipped,0,0);
      lVar3 = (longdouble)UTIL_WeaponTimeBase__Fv();
      *(float *)&this->m_pPlayer->field_0x12c = (float)((longdouble)dc_0_3 + lVar3);
      lVar3 = (longdouble)UTIL_WeaponTimeBase__Fv();
      this->field_0xa8 = (int)(float)((longdouble)dc_0_3 + lVar3);
      lVar3 = (longdouble)UTIL_WeaponTimeBase__Fv();
      this->field_0x9c = (int)(float)((longdouble)dc_0_3 + lVar3);
      lVar3 = (longdouble)UTIL_WeaponTimeBase__Fv();
      this->field_0xa0 = (int)(float)((longdouble)dc_0_3 + lVar3);
    }
  }
  else if (this->m_pActiveItem->burstOrPhase == 1) {
    lVar3 = (longdouble)UTIL_WeaponTimeBase__Fv();
    if ((longdouble)(float)this->field_0xa8 <= lVar3) {
      if (((int)(&this->m_ammoReserve)[(byte)this->m_pActiveInfo->iCaliber] < 1) ||
         (this->m_pActiveInfo->iClip <= this->m_pActiveItem->iClip)) {
        this->m_pActiveItem->burstOrPhase = 2;
      }
      else {
        (&this->m_ammoReserve)[(byte)this->m_pActiveInfo->iCaliber] =
             (&this->m_ammoReserve)[(byte)this->m_pActiveInfo->iCaliber] + -1;
        piVar1 = &this->m_pActiveItem->iClip;
        *piVar1 = *piVar1 + 1;
        (*PTR_g_engfuncs_001778b8->pfnPlaybackEvent)
                  (1,this->m_pPlayer->pev->pContainingEntity,(uint)*(ushort *)&this->field_0x970,0,
                   PTR_g_vecZero_00177698,PTR_g_vecZero_00177698,0x41100000,0,this->m_weaponType,
                   this->m_pActiveItem->attachEquipped,0,0);
        lVar3 = (longdouble)UTIL_WeaponTimeBase__Fv();
        this->field_0xa8 =
             (int)(float)(lVar3 + (longdouble)*(float *)&this->m_pActiveInfo->field_0x14);
      }
    }
  }
  else if (this->m_pActiveItem->burstOrPhase == 2) {
    this->m_pActiveItem->burstOrPhase = 0;
    this->m_pActiveItem->pumpToggle = '\0';
    (*PTR_g_engfuncs_001778b8->pfnPlaybackEvent)
              (1,this->m_pPlayer->pev->pContainingEntity,(uint)*(ushort *)&this->field_0x970,0,
               PTR_g_vecZero_00177698,PTR_g_vecZero_00177698,0x41200000,0,this->m_weaponType,
               this->m_pActiveItem->attachEquipped,0,0);
    lVar3 = (longdouble)UTIL_WeaponTimeBase__Fv();
    *(float *)&this->m_pPlayer->field_0x12c = (float)((longdouble)dc_0_3 + lVar3);
    lVar3 = (longdouble)UTIL_WeaponTimeBase__Fv();
    this->field_0xa8 = (int)(float)((longdouble)dc_3 + lVar3);
  }
  return;
}


