// ===== FUN @ 000b00ac size=36 =====

/* WARNING: Unknown calling convention -- yet parameter storage is locked */
/* client.cpp */

undefined4 GetWeaponData__FP7edict_sP13weapon_data_s(edict_s *a0,weapon_data_s *a1)

{
  float fVar1;
  undefined uVar2;
  undefined3 extraout_var;
  int *piVar3;
  int iVar4;
  int iVar5;
  int iVar6;
  weapon_data_s *pwVar7;
  undefined4 *puVar8;
  int iStack_5c;
  int iStack_54;
  undefined4 auStack_4c [8];
  uint uStack_2c;
  
  iVar5 = *(int *)(a0 + 0x288);
  if (iVar5 == 0) {
    uVar2 = (*PTR_g_engfuncs_001778b8->pfnPEntityOfEntOffset)(0);
    iVar5 = CONCAT31(extraout_var,uVar2);
  }
  iVar6 = 0;
  if (iVar5 != 0) {
    iVar6 = *(int *)(iVar5 + 0x7c);
  }
  pwVar7 = a1;
  for (iVar5 = 0x2c0; iVar5 != 0; iVar5 = iVar5 + -1) {
    *(undefined4 *)pwVar7 = 0;
    pwVar7 = pwVar7 + 4;
  }
  iStack_54 = 0;
  iStack_5c = 0x9f0;
  if (iVar6 != 0) {
    do {
      if (*(int *)(iStack_5c + iVar6 + 8) != 0) {
        iVar5 = *(int *)(iStack_5c + iVar6 + 8);
        do {
          piVar3 = (int *)(**(code **)(*(int *)(iVar5 + 0x60) + 0x158))(iVar5);
          if ((piVar3 != (int *)0x0) &&
             (iVar4 = (**(code **)(piVar3[0x18] + 0x198))(piVar3), iVar4 != 0)) {
            puVar8 = auStack_4c;
            for (iVar4 = 0xb; iVar4 != 0; iVar4 = iVar4 + -1) {
              *puVar8 = 0;
              puVar8 = puVar8 + 1;
            }
            (**(code **)(piVar3[0x18] + 0x120))(piVar3,auStack_4c);
            if (uStack_2c < 0x20) {
              pwVar7 = a1 + uStack_2c * 0x58;
              *(uint *)pwVar7 = uStack_2c;
              *(int *)(pwVar7 + 4) = piVar3[0x2d];
              fVar1 = (float)piVar3[0x2a];
              if (fVar1 < (float)dc_neg0_001) {
                fVar1 = (float)dc_neg0_001;
              }
              *(float *)(pwVar7 + 0x10) = fVar1;
              fVar1 = (float)piVar3[0x27];
              if (fVar1 < (float)dc_neg0_001) {
                fVar1 = (float)dc_neg0_001;
              }
              *(float *)(pwVar7 + 8) = fVar1;
              fVar1 = (float)piVar3[0x28];
              if (fVar1 < (float)dc_neg0_001) {
                fVar1 = (float)dc_neg0_001;
              }
              *(float *)(pwVar7 + 0xc) = fVar1;
              *(int *)(pwVar7 + 0x14) = piVar3[0x30];
              *(int *)(pwVar7 + 0x18) = piVar3[0x26];
              fVar1 = *(float *)(*piVar3 + 0x254);
              if (fVar1 < (float)dc_neg0_001) {
                fVar1 = (float)dc_neg0_001;
              }
              *(float *)(pwVar7 + 0x48) = fVar1;
              *(int *)(pwVar7 + 0x4c) = piVar3[0x13];
              *(int *)(pwVar7 + 0x50) = piVar3[0x14];
              *(int *)(pwVar7 + 0x38) = piVar3[0x15];
              *(int *)(pwVar7 + 0x3c) = piVar3[0x16];
              *(int *)(pwVar7 + 0x40) = piVar3[0x17];
            }
          }
          iVar5 = *(int *)(iVar5 + 0x84);
        } while (iVar5 != 0);
      }
      iStack_5c = iStack_5c + 4;
      iStack_54 = iStack_54 + 1;
    } while (iStack_54 < 6);
  }
  return 1;
}


// ===== FUN @ 000858c8 size=1201 =====

/* WARNING: Unknown calling convention -- yet parameter storage is locked */
/* TS_gun.cpp */

undefined4 CTSGun::UpdateClientData(CTSGun *this,CBasePlayer *a0)

{
  bool bVar1;
  enginefuncs_t *peVar2;
  int iVar3;
  s_weapon_status *psVar4;
  int *piVar5;
  undefined4 *puVar6;
  int *local_34;
  int *local_30;
  int *local_2c;
  int *local_28;
  int *local_24;
  int local_20;
  
  if ((float)this->field_0x9e0 <= (*PTR_gpGlobals_00177410)->time) {
    bVar1 = false;
    this->field_0x9e0 = (int)(fc_0_10000000149011612 + (*PTR_gpGlobals_00177410)->time);
    peVar2 = PTR_g_engfuncs_001778b8;
    local_20 = 0x25;
    piVar5 = &this->field_0x928;
    local_34 = &this->field_0x9d5;
    local_30 = piVar5;
    local_2c = piVar5;
    local_28 = local_34;
    local_24 = piVar5;
    do {
      if (*(char *)(local_34 + 2) != *(char *)(local_30 + 1)) {
        *(undefined1 *)(local_28 + 2) = *(undefined1 *)(local_2c + 1);
        (*peVar2->pfnMessageBegin)
                  (1,*(undefined4 *)PTR_gmsgTSWeaponStatus_00177160,0,a0->pev->pContainingEntity);
        (*peVar2->pfnWriteByte)(local_20);
        (*peVar2->pfnWriteByte)((uint)*(byte *)(piVar5 + 1));
        (*peVar2->pfnWriteByte)(local_24[2]);
        (*peVar2->pfnMessageEnd)();
      }
      if (*(char *)((int)local_34 + 7) != *(char *)(local_30 + -0xd)) {
        *(undefined1 *)((int)local_28 + 7) = *(undefined1 *)(local_2c + -0xd);
        (*peVar2->pfnMessageBegin)
                  (1,*(undefined4 *)PTR_gmsgTSWeaponStatus_00177160,0,a0->pev->pContainingEntity);
        (*peVar2->pfnWriteByte)(local_20 + -1);
        (*peVar2->pfnWriteByte)((uint)*(byte *)(piVar5 + -0xd));
        (*peVar2->pfnWriteByte)(local_24[-0xc]);
        (*peVar2->pfnMessageEnd)();
      }
      local_24 = local_24 + -0x1c;
      piVar5 = piVar5 + -0x1c;
      local_2c = local_2c + -0x1c;
      local_30 = local_30 + -0x1c;
      local_28 = (int *)((int)local_28 + -2);
      local_34 = (int *)((int)local_34 + -2);
      local_20 = local_20 + -2;
    } while (-1 < local_20);
    if (this->m_pActiveItem != (s_weapon_status *)0x0) {
      if (this->m_weaponType != (uint)(byte)this->field_0x97a) {
        bVar1 = true;
        this->field_0x97a = (char)this->m_weaponType;
        psVar4 = this->m_pActiveItem;
        puVar6 = (undefined4 *)&this->field_0x97c;
        for (iVar3 = 0xe; iVar3 != 0; iVar3 = iVar3 + -1) {
          *puVar6 = *(undefined4 *)psVar4;
          psVar4 = (s_weapon_status *)&psVar4->iClip;
          puVar6 = puVar6 + 1;
        }
        this->field_0x9b4 = (&this->m_ammoReserve)[(byte)this->m_pActiveInfo->iCaliber];
      }
      if (this->field_0x9b4 != (&this->m_ammoReserve)[(byte)this->m_pActiveInfo->iCaliber]) {
        bVar1 = true;
        psVar4 = this->m_pActiveItem;
        puVar6 = (undefined4 *)&this->field_0x97c;
        for (iVar3 = 0xe; iVar3 != 0; iVar3 = iVar3 + -1) {
          *puVar6 = *(undefined4 *)psVar4;
          psVar4 = (s_weapon_status *)&psVar4->iClip;
          puVar6 = puVar6 + 1;
        }
        this->field_0x9b4 = (&this->m_ammoReserve)[(byte)this->m_pActiveInfo->iCaliber];
      }
      peVar2 = PTR_g_engfuncs_001778b8;
      if (bVar1) {
        (*PTR_g_engfuncs_001778b8->pfnMessageBegin)
                  (1,*(undefined4 *)PTR_gmsgWeaponInfo_001771f8,0,a0->pev->pContainingEntity);
        (*peVar2->pfnWriteByte)(this->m_weaponType);
        (*peVar2->pfnWriteByte)(this->field_0x980);
        (*peVar2->pfnWriteShort)(this->field_0x9b4);
        (*peVar2->pfnWriteByte)((uint)(byte)this->field_0x97e);
        (*peVar2->pfnWriteByte)(this->field_0x98c);
        (*peVar2->pfnMessageEnd)();
        iVar3 = get_weapon_info__F10tsweapon_t(this->m_weaponType);
        peVar2 = PTR_g_engfuncs_001778b8;
        iVar3 = *(int *)(iVar3 + 200);
        if ((iVar3 != -1) && ((&this->field_0x114)[iVar3 * 0x38] != '\0')) {
          (*PTR_g_engfuncs_001778b8->pfnMessageBegin)
                    (1,*(undefined4 *)PTR_gmsgTSWeaponStatus_00177160,0,a0->pev->pContainingEntity);
          (*peVar2->pfnWriteByte)(iVar3);
          (*peVar2->pfnWriteByte)((uint)(byte)(&this->field_0x114)[iVar3 * 0x38]);
          (*peVar2->pfnWriteByte)((&this->field_0x118)[iVar3 * 0xe]);
          (*peVar2->pfnMessageEnd)();
        }
        iVar3 = get_weapon_info__F10tsweapon_t(this->m_weaponType);
        peVar2 = PTR_g_engfuncs_001778b8;
        iVar3 = *(int *)(iVar3 + 0xcc);
        if (((iVar3 != -1) && (iVar3 != this->m_weaponType)) &&
           ((&this->field_0x114)[iVar3 * 0x38] != '\0')) {
          (*PTR_g_engfuncs_001778b8->pfnMessageBegin)
                    (1,*(undefined4 *)PTR_gmsgTSWeaponStatus_00177160,0,a0->pev->pContainingEntity);
          (*peVar2->pfnWriteByte)(iVar3);
          (*peVar2->pfnWriteByte)((uint)(byte)(&this->field_0x114)[iVar3 * 0x38]);
          (*peVar2->pfnWriteByte)((&this->field_0x118)[iVar3 * 0xe]);
          (*peVar2->pfnMessageEnd)();
        }
        this->field_0x980 = this->m_pActiveItem->iClip;
      }
      return 1;
    }
    if (this->m_weaponType != (uint)(byte)this->field_0x97a) {
      this->field_0x97a = (char)this->m_weaponType;
      peVar2 = PTR_g_engfuncs_001778b8;
      (*PTR_g_engfuncs_001778b8->pfnMessageBegin)
                (1,*(undefined4 *)PTR_gmsgWeaponInfo_001771f8,0,a0->pev->pContainingEntity);
      (*peVar2->pfnWriteByte)(this->m_weaponType);
      (*peVar2->pfnWriteByte)((int)ROUND(*(float *)&this->m_pPlayer->field_0x774));
      (*peVar2->pfnWriteShort)(0);
      (*peVar2->pfnWriteByte)(0);
      (*peVar2->pfnWriteByte)(0);
      (*peVar2->pfnMessageEnd)();
    }
  }
  return 0;
}


// ===== FUN @ 00091d00 size=2190 =====

/* WARNING: Unknown calling convention -- yet parameter storage is locked */
/* TS_player.cpp */

void CBasePlayer::UpdateClientData(CBasePlayer *this)

{
  float fVar1;
  undefined *puVar2;
  enginefuncs_t *peVar3;
  undefined1 uVar4;
  undefined uVar5;
  int iVar6;
  uint uVar7;
  undefined3 extraout_var;
  undefined3 extraout_var_00;
  undefined2 in_FPUControlWord;
  undefined4 in_stack_ffffffe8;
  undefined2 uVar9;
  undefined4 uVar8;
  
  uVar9 = (undefined2)((uint)in_stack_ffffffe8 >> 0x10);
  if (((float)this->field_0x534 <= (*PTR_gpGlobals_00177410)->time) &&
     (fc_0 <= (float)this->field_0x534)) {
    this->field_0x534 = (int)((float)dc_0_1 + (*PTR_gpGlobals_00177410)->time);
    if (*(int *)&this->m_fInitHUD != 0) {
      *(undefined4 *)&this->m_fInitHUD = 0;
      *(undefined4 *)PTR_gInitHUD_00177b8c = 0;
      peVar3 = PTR_g_engfuncs_001778b8;
      (*PTR_g_engfuncs_001778b8->pfnMessageBegin)
                (1,*(undefined4 *)PTR_gmsgResetHUD_001772e4,0,this->pev->pContainingEntity);
      (*peVar3->pfnWriteByte)(0);
      (*peVar3->pfnMessageEnd)();
      if (this->field_0x9b8 == 0) {
        (*peVar3->pfnMessageBegin)
                  (1,*(undefined4 *)PTR_gmsgInitHUD_00177238,0,this->pev->pContainingEntity);
        (*peVar3->pfnMessageEnd)();
        puVar2 = PTR_g_pGameRules_00177e70;
        (**(code **)(**(int **)PTR_g_pGameRules_00177e70 + 0x34))
                  (*(int **)PTR_g_pGameRules_00177e70,this);
        CTSGameRules::InitHUD((CTSGameRules *)PTR_TSGameRules_00177300,this);
        this->field_0x9b8 = 1;
        iVar6 = (**(code **)(**(int **)puVar2 + 0x1c))(*(int **)puVar2);
        if (iVar6 != 0) {
          FireTargets__FPCcP11CBaseEntityT18USE_TYPEf("game_playerjoin",this,this,3,0);
        }
      }
      FireTargets__FPCcP11CBaseEntityT18USE_TYPEf("game_playerspawn",this,this,3,0);
      peVar3 = PTR_g_engfuncs_001778b8;
      puVar2 = PTR_TSGameRules_00177300;
      if ((*(float *)(PTR_TSGameRules_00177300 + 8) != 0.0) &&
         (PTR_TSGameRules_00177300[0x10] != '\0')) {
        (*PTR_g_engfuncs_001778b8->pfnMessageBegin)
                  (1,*(undefined4 *)PTR_gmsgTSRoundTime_001770d8,0,this->pev->pContainingEntity);
        fVar1 = *(float *)(puVar2 + 8) - (*PTR_gpGlobals_00177410)->time;
        if (fVar1 < 0.0) {
          fVar1 = 0.0;
        }
        (*peVar3->pfnWriteByte)((int)ROUND(fVar1));
        peVar3 = PTR_g_engfuncs_001778b8;
        (*PTR_g_engfuncs_001778b8->pfnWriteByte)(0);
        (*peVar3->pfnMessageEnd)();
      }
      peVar3 = PTR_g_engfuncs_001778b8;
      if ((PTR_TSGameRules_00177300[0x5885] != '\0') && (*(char *)&this->m_bLowGravity != '\0')) {
        (*PTR_g_engfuncs_001778b8->pfnMessageBegin)
                  (1,*(undefined4 *)PTR_gmsgTSPowerUp_00177548,0,this->pev->pContainingEntity);
        (*peVar3->pfnWriteShort)(0x100);
        (*peVar3->pfnWriteByte)(0);
        (*peVar3->pfnMessageEnd)();
      }
    }
    peVar3 = PTR_g_engfuncs_001778b8;
    if (this->m_iHideHUD != this->field_0x9ec) {
      (*PTR_g_engfuncs_001778b8->pfnMessageBegin)
                (1,*(undefined4 *)PTR_gmsgHideWeapon_001777b0,0,this->pev->pContainingEntity);
      (*peVar3->pfnWriteByte)(this->m_iHideHUD);
      (*peVar3->pfnMessageEnd)();
      this->field_0x9ec = this->m_iHideHUD;
    }
    peVar3 = PTR_g_engfuncs_001778b8;
    if ((float)this->field_0x9e0 != this->pev->health) {
      fVar1 = this->pev->health;
      if (fVar1 < fc_0) {
        fVar1 = fc_0;
      }
      uVar8 = CONCAT22(uVar9,in_FPUControlWord);
      (*PTR_g_engfuncs_001778b8->pfnMessageBegin)
                (1,*(undefined4 *)PTR_gmsgTSHealth_00177c4c,0,this->pev->pContainingEntity,
                 (int)ROUND(fVar1),uVar8);
      uVar9 = (undefined2)((uint)uVar8 >> 0x10);
      (*peVar3->pfnWriteByte)((int)ROUND(fVar1));
      (*peVar3->pfnMessageEnd)();
      this->field_0x9e0 = (int)ROUND(this->pev->health);
    }
    if ((float)this->field_0x9e4 != this->pev->armorvalue) {
      iVar6 = (int)ROUND(this->pev->armorvalue);
      this->field_0x9e4 = iVar6;
      peVar3 = PTR_g_engfuncs_001778b8;
      (*PTR_g_engfuncs_001778b8->pfnMessageBegin)
                (1,*(undefined4 *)PTR_gmsgTSArmor_00177428,0,this->pev->pContainingEntity,iVar6,
                 CONCAT22(uVar9,in_FPUControlWord));
      (*peVar3->pfnWriteByte)((int)ROUND(this->pev->armorvalue));
      (*peVar3->pfnMessageEnd)();
    }
    peVar3 = PTR_g_engfuncs_001778b8;
    if ((this->m_iTrain & 0xc0U) != 0) {
      (*PTR_g_engfuncs_001778b8->pfnMessageBegin)
                (1,*(undefined4 *)PTR_gmsgTrain_001774e8,0,this->pev->pContainingEntity);
      (*peVar3->pfnWriteByte)(this->m_iTrain & 0xf);
      (*peVar3->pfnMessageEnd)();
      this->m_iTrain = this->m_iTrain & 0xffffff3f;
    }
    if (this->field_0x498 != (char)this->field_0x499) {
      *(undefined1 *)&this->field_0x499 = this->field_0x498;
      peVar3 = PTR_g_engfuncs_001778b8;
      (*PTR_g_engfuncs_001778b8->pfnMessageBegin)
                (1,*(undefined4 *)PTR_gmsgTSState_00177ac0,0,this->pev->pContainingEntity);
      (*peVar3->pfnWriteByte)((uint)(byte)this->field_0x498);
      (*peVar3->pfnMessageEnd)();
    }
    if (this->field_0x540 != this->field_0x544) {
      this->field_0x544 = this->field_0x540;
      peVar3 = PTR_g_engfuncs_001778b8;
      (*PTR_g_engfuncs_001778b8->pfnMessageBegin)
                (1,*(undefined4 *)PTR_gmsgTSCash_001777e4,0,this->pev->pContainingEntity);
      (*peVar3->pfnWriteShort)(this->field_0x540);
      (*peVar3->pfnMessageEnd)();
    }
    if (this->field_0x548 != this->field_0x54c) {
      this->field_0x54c = this->field_0x548;
      peVar3 = PTR_g_engfuncs_001778b8;
      (*PTR_g_engfuncs_001778b8->pfnMessageBegin)
                (1,*(undefined4 *)PTR_gmsgTSSpace_00177388,0,this->pev->pContainingEntity);
      (*peVar3->pfnWriteByte)(this->field_0x548);
      (*peVar3->pfnMessageEnd)();
    }
    if ((this->m_iClientHeldType != this->m_iHeldPowerup) ||
       (this->m_iClientHeldDur != this->m_flPowerupTime)) {
      this->m_iClientHeldType = this->m_iHeldPowerup;
      this->m_iClientHeldDur = this->m_flPowerupTime;
      peVar3 = PTR_g_engfuncs_001778b8;
      (*PTR_g_engfuncs_001778b8->pfnMessageBegin)
                (1,*(undefined4 *)PTR_gmsgTSPowerUp_00177548,0,this->pev->pContainingEntity);
      (*peVar3->pfnWriteShort)(this->m_iHeldPowerup);
      (*peVar3->pfnWriteByte)
                (this->m_flPowerupTime & 0xffffffffU - ((int)this->m_flPowerupTime >> 0x1f));
      (*peVar3->pfnMessageEnd)();
    }
    fVar1 = fc_0_22499999403953552;
    if (((this->m_iPowerups & 8) == 0) &&
       (fVar1 = fc_0_3499999940395355, (this->m_iPowerups & 1) == 0)) {
      fVar1 = this->m_flSlowMotionTarget;
    }
    if (fVar1 != this->m_flClientSlowAmt) {
      this->m_flClientSlowAmt = fVar1;
      peVar3 = PTR_g_engfuncs_001778b8;
      (*PTR_g_engfuncs_001778b8->pfnMessageBegin)
                (1,*(undefined4 *)PTR_gmsgTSSlowAmt_00177838,0,this->pev->pContainingEntity);
      (*peVar3->pfnWriteCoord)(fc_100 * this->m_flClientSlowAmt);
      (*peVar3->pfnMessageEnd)();
    }
    if (this->field_0x174 != this->field_0x175) {
      this->field_0x174 = this->field_0x175;
      peVar3 = PTR_g_engfuncs_001778b8;
      (*PTR_g_engfuncs_001778b8->pfnMessageBegin)
                (1,*(undefined4 *)PTR_gmsgTSBulletTime_0017735c,0,this->pev->pContainingEntity);
      (*peVar3->pfnWriteByte)((int)(char)this->field_0x174);
      (*peVar3->pfnMessageEnd)();
    }
    if ((((this->m_pActiveItem != (CBaseEntity *)0x0) &&
         (iVar6 = (*this->vtbl->IsAlive)(this), iVar6 != 0)) &&
        (((CTSGun *)this->m_pActiveItem)->m_iId == 0x10)) &&
       (iVar6 = this->field_0x720, uVar7 = CTSGun::GetActiveItems((CTSGun *)this->m_pActiveItem),
       (byte)iVar6 != uVar7)) {
      uVar4 = CTSGun::GetActiveItems((CTSGun *)this->m_pActiveItem);
      *(undefined1 *)&this->field_0x720 = uVar4;
      peVar3 = PTR_g_engfuncs_001778b8;
      (*PTR_g_engfuncs_001778b8->pfnMessageBegin)
                (2,*(undefined4 *)PTR_gmsgTSActiveItems_00177b2c,0,0);
      uVar5 = (*peVar3->pfnIndexOfEdict)(this->pev->pContainingEntity);
      (*peVar3->pfnWriteByte)(CONCAT31(extraout_var,uVar5));
      (*peVar3->pfnWriteByte)((uint)(byte)this->field_0x720);
      (*peVar3->pfnMessageEnd)();
    }
    if (this->field_0x74d != this->field_0x74c) {
      this->field_0x74d = this->field_0x74c;
      peVar3 = PTR_g_engfuncs_001778b8;
      (*PTR_g_engfuncs_001778b8->pfnMessageBegin)
                (2,*(undefined4 *)PTR_gmsgTSPlayerAward_001776dc,0,0);
      uVar5 = (*peVar3->pfnIndexOfEdict)(this->pev->pContainingEntity);
      (*peVar3->pfnWriteByte)(CONCAT31(extraout_var_00,uVar5));
      (*peVar3->pfnWriteByte)((uint)(byte)this->field_0x74c);
      (*peVar3->pfnMessageEnd)();
    }
    if ((this->field_0x78c != this->field_0x78f) ||
       (*(short *)&this->field_0x78d != (short)*(undefined4 *)&this->field_0x790)) {
      this->field_0x78f = this->field_0x78c;
      this->field_0x790 = this->field_0x78d;
      this->field_0x791 = this->field_0x78e;
      peVar3 = PTR_g_engfuncs_001778b8;
      (*PTR_g_engfuncs_001778b8->pfnMessageBegin)
                (1,*(undefined4 *)PTR_gmsgTSWires_00177638,0,this->pev->pContainingEntity);
      (*peVar3->pfnWriteByte)((uint)(byte)this->field_0x78c);
      (*peVar3->pfnWriteByte)((uint)(byte)this->field_0x78d);
      (*peVar3->pfnWriteByte)((uint)(byte)this->field_0x78e);
      (*peVar3->pfnMessageEnd)();
    }
    if ((this->m_pActiveItem != (CBaseEntity *)0x0) &&
       (*(int *)&this->m_pActiveItem->field_0x88 == 0x10)) {
      (*this->m_pActiveItem->vtbl[1].OnFreeEntPrivateData)(this->m_pActiveItem);
    }
    this->field_0xa14 = (int)this->m_pActiveItem;
    this->field_0x9f4 = this->m_iFOV;
  }
  return;
}


