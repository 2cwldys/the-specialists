// ===== FUN @ 00085698 size=556 =====

/* WARNING: Unknown calling convention -- yet parameter storage is locked */
/* TS_gun.cpp */

void CTSGun::HolsterWeapon(CTSGun *this)

{
  CBaseEntity *pCVar1;
  CTSGun_vtbl *pCVar2;
  undefined1 uVar3;
  int iVar4;
  longdouble lVar5;
  int a1;
  int a2;
  
  *(CTSGun **)&this->m_pPlayer->field_0xa18 = this;
  this->m_pPlayer->pev->viewmodel = 0;
  this->m_pPlayer->pev->weaponmodel = 0;
  Unzoom(this);
  uVar3 = DAT_00106657;
  this->m_pPlayer->field_0xb58 = DAT_00106657;
  this->m_pPlayer->field_0x478 = uVar3;
  lVar5 = (longdouble)UTIL_WeaponTimeBase__Fv();
  *(float *)&this->m_pPlayer->field_0x12c = (float)((longdouble)dc_0_5 + lVar5);
  lVar5 = (longdouble)UTIL_WeaponTimeBase__Fv();
  this->field_0xa8 = (int)(float)((longdouble)dc_3 + lVar5);
  pCVar1 = this->m_pPlayer;
  if ((fc_0 != *(float *)&pCVar1->field_0x79c) && (pCVar1->field_0x7a4 != '\0')) {
    *(undefined4 *)&pCVar1->field_0x79c = 0;
    *(undefined4 *)&this->m_pPlayer->field_0x7a0 = 0;
    this->m_pPlayer->field_0x7a4 = 0;
  }
  if ((this->m_pPlayer != (CBaseEntity *)0x0) &&
     (iVar4 = (*this->m_pPlayer->vtbl->IsAlive)(this->m_pPlayer), iVar4 == 0)) {
    return;
  }
  if ((this->m_pActiveItem != (s_weapon_status *)0x0) &&
     (this->m_pActiveInfo != (s_weapon_info *)0x0)) {
    if (this->m_pActiveInfo->iFireType == '\x02') {
      GrenadeIdle(this,1);
    }
    else {
      *(undefined4 *)&this->m_pActiveItem->field_0xc = 0;
      this->m_pActiveItem->burstOrPhase = 0;
    }
  }
  this->m_weaponType = 0;
  this->m_pActiveItem = (s_weapon_status *)0x0;
  this->m_pActiveInfo = (s_weapon_info *)0x0;
  iVar4 = get_weapon_info__F10tsweapon_t(0);
  this->m_pPlayer->pev->viewmodel = (iVar4 - (int)(*PTR_gpGlobals_00177410)->pStringBase) + 0x132;
  pCVar2 = this->vtbl;
  a2 = 0;
  a1 = 0;
  iVar4 = get_weapon_info__F10tsweapon_t(0);
  (*pCVar2->SendWeaponAnim)((CBasePlayerWeapon *)this,(int)*(short *)(iVar4 + 0x256),a1,a2);
  (*PTR_g_engfuncs_001778b8->pfnPlaybackEvent)
            (1,this->m_pPlayer->pev->pContainingEntity,(uint)*(ushort *)&this->field_0x976,0,
             PTR_g_vecZero_00177698,PTR_g_vecZero_00177698,0,0,this->m_weaponType,0,0,0);
  this->m_kickCombo = 0;
  this->m_comboStep = 0;
  this->m_lastMeleeAnim = 0.0;
  return;
}


