// ===== FUN @ 00097bc8 size=548 =====

/* WARNING: Unknown calling convention -- yet parameter storage is locked */
/* TS_powerup.cpp
   assets: items/gunpickup2.wav */

undefined4 PlayerAddPowerUp__FP11CBasePlayeriP10CTSPowerUp(CBasePlayer *a0,int a1,CTSPowerUp *a2)

{
  char cVar1;
  int iVar2;
  enginefuncs_t *peVar3;
  bool bVar4;
  undefined4 uVar5;
  uint in_stack_ffffffd8;
  
  if ((char)a1 < '\0') {
    CBasePlayer::TakeArmor(a0,(int)ROUND((float)a2->field_0x8c));
    goto LAB_00097dba;
  }
  if ((a1 & 4U) == 0) {
    if ((a1 & 0x100U) == 0) {
      if ((a1 & 0x40U) == 0) {
        if ((a1 & 0x20U) == 0) {
          if ((a1 & 2U) == 0) {
            a0->m_iPowerups = 0;
            a0->m_iHeldPowerup = a1;
            a0->m_flPowerupExpireTime = 0.0;
            a0->m_flPowerupTime = a2->field_0x90;
            goto LAB_00097dba;
          }
          if (a0->m_pActiveItem == (CBaseEntity *)0x0) {
            return 0;
          }
          if (((CTSGun *)a0->m_pActiveItem)->m_iId != 0x10) {
            return 0;
          }
          cVar1 = CTSGun::MultiplyClips((CTSGun *)a0->m_pActiveItem);
          bVar4 = cVar1 == '\0';
        }
        else {
          iVar2 = CBasePlayer::PickUpWeapon(a0,0x18,0,0,1,in_stack_ffffffd8 & 0xffff0000);
          bVar4 = iVar2 == 0;
        }
        if (bVar4) {
          return 0;
        }
      }
      else {
        if (a0->pev->health < fc_150) {
          a0->pev->health = 150.0;
        }
        a0->m_flHealthRegen = (float)dc_1 + (*PTR_gpGlobals_00177410)->time;
      }
      goto LAB_00097dba;
    }
    if (*(char *)&a0->m_bLowGravity != '\0') goto LAB_00097dba;
    *(undefined1 *)&a0->m_bLowGravity = 1;
    peVar3 = PTR_g_engfuncs_001778b8;
    (*PTR_g_engfuncs_001778b8->pfnSetPhysicsKeyValue)(a0->pev->pContainingEntity,&DAT_0010791d,"1");
    (*peVar3->pfnMessageBegin)
              (1,*(undefined4 *)PTR_gmsgTSPowerUp_00177548,0,a0->pev->pContainingEntity);
    uVar5 = 0x100;
  }
  else {
    if (a0->field_0x74e != '\0') goto LAB_00097dba;
    a0->field_0x74e = 1;
    peVar3 = PTR_g_engfuncs_001778b8;
    (*PTR_g_engfuncs_001778b8->pfnMessageBegin)
              (1,*(undefined4 *)PTR_gmsgTSPowerUp_00177548,0,a0->pev->pContainingEntity);
    uVar5 = 4;
  }
  (*peVar3->pfnWriteShort)(uVar5);
  (*peVar3->pfnWriteByte)(0);
  (*peVar3->pfnMessageEnd)();
LAB_00097dba:
  EMIT_SOUND_DYN__FP7edict_siPCcffiif
            (a0->pev->pContainingEntity,3,"items/gunpickup2.wav",fc_1,fc_0_800000011920929,0,100,
             fc_0_5);
  return 1;
}


