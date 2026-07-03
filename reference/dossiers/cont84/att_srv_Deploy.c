// ===== FUN @ 00081030 size=346 =====

/* WARNING: Unknown calling convention -- yet parameter storage is locked */
/* TS_gun.cpp */

undefined4 CTSGun::Deploy(CTSGun *this)

{
  undefined4 uVar1;
  int iVar2;
  char *pcVar3;
  
  if (this->m_weaponType == 0) {
    HolsterWeapon(this);
  }
  else {
    this->field_0x9e0 = 0;
    iVar2 = this->m_weaponType;
    this->m_pActiveItem = (s_weapon_status *)(&this->field_0x114 + iVar2 * 0x38);
    this->m_pActiveInfo = (s_weapon_info *)(&global_weapon_info + iVar2 * 0xf7);
    (&this->field_0x13c)[iVar2 * 0xe] = iVar2 * 0x3dc + 0x17dd92;
    uVar1 = GetBodyByItems__Fii(this->m_weaponType,this->m_pActiveItem->attachEquipped);
    *(undefined4 *)&this->m_pActiveItem->field_0x18 = uVar1;
    this->pev->body = *(int *)&this->m_pActiveItem->field_0x18;
    this->m_pActiveItem->burstOrPhase = 0;
    this->m_pActiveItem->holdPosition = '\0';
    if (*(int *)&this->m_pActiveItem->field_0x28 == 0) {
      iVar2 = 0;
    }
    else {
      iVar2 = (int)*(short *)(*(int *)&this->m_pActiveItem->field_0x28 + 4);
    }
    if ((this->m_pActiveItem->attachEquipped & 1) == 0) {
      pcVar3 = &this->m_pActiveInfo->szPlayerModel;
    }
    else {
      pcVar3 = &this->m_pActiveInfo->field_0x1b2;
    }
    CBasePlayerWeapon::DefaultDeploy
              (this,&this->m_pActiveInfo->szViewModel,pcVar3,iVar2,&this->m_pActiveInfo->szAnimExt,0
               ,this->pev->body);
    (*PTR_g_engfuncs_001778b8->pfnPlaybackEvent)
              (1,this->m_pPlayer->pev->pContainingEntity,(uint)*(ushort *)&this->field_0x976,0,
               PTR_g_vecZero_00177698,PTR_g_vecZero_00177698,0,0,this->m_weaponType,
               this->m_pActiveItem->attachEquipped,0,0);
    CTSAnimation::Animate
              ((CTSAnimation *)&this->m_pPlayer->field_0x28c,(CBasePlayer *)this->m_pPlayer,0x34);
    if ((char)(&this->m_pPlayer->field_0x49a)[this->m_weaponType] < '\x01') {
      (&this->m_pPlayer->field_0x49a)[this->m_weaponType] = this->m_pActiveItem->fireMode + '\x01';
    }
    else {
      this->m_pActiveItem->fireMode = (&this->m_pPlayer->field_0x49a)[this->m_weaponType] + -1;
    }
  }
  return 1;
}


