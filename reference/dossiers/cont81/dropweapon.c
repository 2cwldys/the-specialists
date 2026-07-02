// ===== FUN @ 00094900 size=1099 =====

/* TS_player.cpp
   assets: weapons/weapondrop.wav */

void CBasePlayer::DropWeapon(CBasePlayer *this,uint param_2,float param_3)

{
  float fVar1;
  CTSGun *this_00;
  float fVar2;
  char cVar3;
  int iVar4;
  int *piVar5;
  int iVar6;
  byte bVar7;
  entvars_t *peVar8;
  uint a0;
  Vector *pVVar9;
  int iVar10;
  int iVar11;
  vec_t *pvVar12;
  undefined4 *puVar13;
  int *piVar14;
  longdouble lVar15;
  int in_stack_ffffff4c;
  undefined2 uVar17;
  uint uVar16;
  int local_78;
  bool local_71;
  undefined4 auStack_6c [4];
  int local_5c [19];
  
  iVar11 = 0;
  this_00 = (CTSGun *)this->m_pActiveItem;
  if (this_00 == (CTSGun *)0x0) {
    return;
  }
  if (this_00->m_iId != 0x10) {
    return;
  }
  if (((this_00->m_weaponType == 0x1d) || (this_00->m_weaponType < 1)) && (param_2 == 0xfffffffe)) {
    return;
  }
  local_71 = true;
  if (param_2 == 0xfffffffe) {
    this->field_0x7b4 = (int)(param_3 + (float)this->field_0x7b4);
    bVar7 = 0x1e;
    if (this_00->m_weaponType != 0x22) {
      cVar3 = GetWeaponType__F10tsweapon_t(this_00->m_weaponType);
      bVar7 = 0x19;
      if (cVar3 != '\x03') {
        if (((byte)this_00->m_pActiveInfo->slotCost < 0x32) &&
           ((byte)this_00->m_pActiveInfo->slotCost < 0x15)) {
          bVar7 = 0x14;
        }
        else {
          bVar7 = 0x32;
          if ((byte)this_00->m_pActiveInfo->slotCost < 0x32) {
            bVar7 = this_00->m_pActiveInfo->slotCost;
          }
        }
      }
    }
    if ((float)this->field_0x7b4 < (float)bVar7) {
      return;
    }
    local_71 = false;
    this->field_0x7b4 = 0;
  }
  a0 = param_2;
  if (0x7fffffff < param_2) {
    a0 = this_00->m_weaponType;
  }
  if (0x24 < a0 - 1) {
    if (param_2 == 0xfffffffe) {
      return;
    }
    cVar3 = HaveFlag__FP11CBasePlayer(this);
    if (cVar3 == '\0') {
      return;
    }
    DropFlag(this,false);
    return;
  }
  if ((&this_00->field_0x114)[a0 * 0x38] == '\0') {
    return;
  }
  if (((a0 == 0x18) && ((byte)*PTR_TSGameRules_00177300 < 2)) &&
     (iVar4 = (*this->vtbl->IsAlive)(this), iVar4 == 0)) {
    if (*(int *)&this_00->field_0x65c == 1) {
      CTSGun::GrenadeIdle(this_00,1);
    }
    local_78 = 0;
    if (*(int *)&this_00->field_0x658 < 1) {
      return;
    }
    iVar11 = 0;
    do {
      piVar5 = (int *)CTSPowerUp::CreatePowerUp((CTSPowerUp *)0x20,-1,in_stack_ffffff4c);
      pVVar9 = &this->pev->origin;
      pvVar12 = (vec_t *)(*piVar5 + 8);
      for (iVar4 = 3; iVar4 != 0; iVar4 = iVar4 + -1) {
        *pvVar12 = pVVar9->x;
        pVVar9 = (Vector *)&pVVar9->y;
        pvVar12 = pvVar12 + 1;
      }
      *(float *)(*piVar5 + 8) = (float)iVar11 + *(float *)(*piVar5 + 8);
      iVar11 = iVar11 + 5;
      local_78 = local_78 + 1;
    } while (local_78 < *(int *)&this_00->field_0x658);
    return;
  }
  uVar17 = (undefined2)((uint)in_stack_ffffff4c >> 0x10);
  if (a0 == 0x18) {
    if (*(int *)&this_00->field_0x65c == 1) {
      CTSGun::GrenadeIdle(this_00,1);
    }
    else if (0 < *(int *)&this_00->field_0x658) {
      piVar5 = (int *)&this_00->field_0x654;
      piVar14 = local_5c;
      for (iVar11 = 0xe; iVar11 != 0; iVar11 = iVar11 + -1) {
        *piVar14 = *piVar5;
        piVar5 = piVar5 + 1;
        piVar14 = piVar14 + 1;
      }
      local_5c[1] = 1;
      iVar11 = ThrowWeapon(this,0x18,local_5c,0,0);
      iVar4 = get_weapon_info__F10tsweapon_t(0x18);
      this->field_0x548 = this->field_0x548 + (uint)*(byte *)(iVar4 + 0xd0);
      if (*(int *)&this_00->field_0x658 < 2) {
LAB_000951af:
        CTSGun::DropWeapon(this_00,a0,local_71);
      }
      else {
        *(int *)&this_00->field_0x658 = *(int *)&this_00->field_0x658 + -1;
        if (local_71 == false) {
          CTSGun::HolsterWeapon(this_00);
        }
      }
      peVar8 = this->pev;
      goto LAB_000951c0;
    }
  }
  else {
    iVar11 = get_weapon_info__F10tsweapon_t(a0);
    if (*(char *)(iVar11 + 0xdc) == '\0') {
      iVar11 = get_weapon_info__F10tsweapon_t(a0);
      if ((*(char *)(iVar11 + 0x348) == '\x04') &&
         (iVar11 = get_weapon_info__F10tsweapon_t(a0), *(int *)(iVar11 + 0xcc) != -1)) {
        iVar11 = get_weapon_info__F10tsweapon_t(a0);
        iVar4 = *(int *)(iVar11 + 0xcc);
        piVar5 = (int *)(&this_00->field_0x114 + iVar4 * 0x38);
        piVar14 = local_5c;
        for (iVar11 = 0xe; iVar11 != 0; iVar11 = iVar11 + -1) {
          *piVar14 = *piVar5;
          piVar5 = piVar5 + 1;
          piVar14 = piVar14 + 1;
        }
        local_5c[1] = (&this_00->field_0x118)[a0 * 0xe] / 2;
        puVar13 = auStack_6c;
        for (iVar11 = 3; iVar11 != 0; iVar11 = iVar11 + -1) {
          *puVar13 = 0;
          puVar13 = puVar13 + 1;
        }
        iVar10 = 0;
        do {
          if (iVar10 == 0) {
            auStack_6c[1] = 0x40800000;
          }
          else {
            auStack_6c[1] = 0xc0800000;
          }
          iVar11 = ThrowWeapon(this,iVar4,local_5c,0,auStack_6c);
          if ((int)param_2 < 0) {
            fVar1 = (*PTR_gpGlobals_00177410)->time;
            fVar2 = *(float *)(PTR_weaponstay_time_001774b4 + 0xc) +
                    *(float *)(PTR_weaponstay_time_001774b4 + 0xc);
            if (fc_120 <= fVar2) {
LAB_00094eb2:
              fVar2 = fVar1 + fc_120;
            }
            else if (fVar2 <= fc_10) {
              fVar2 = fVar1 + fc_10;
            }
            else {
              if (fc_120 <= fVar2) goto LAB_00094eb2;
              fVar2 = fVar2 + fVar1;
            }
            *(float *)(iVar11 + 0xc4) = fVar2;
            *(edict_t **)(iVar11 + 0xd0) = this->pev->pContainingEntity;
          }
          iVar11 = get_weapon_info__F10tsweapon_t(iVar4);
          this->field_0x548 = this->field_0x548 + (uint)*(byte *)(iVar11 + 0xd0);
          if (iVar10 == -1) {
            auStack_6c[1] = 0x40800000;
          }
          else {
            auStack_6c[1] = 0xc0800000;
          }
          iVar11 = ThrowWeapon(this,iVar4,local_5c,0,auStack_6c);
          if ((int)param_2 < 0) {
            fVar1 = (*PTR_gpGlobals_00177410)->time;
            fVar2 = *(float *)(PTR_weaponstay_time_001774b4 + 0xc) +
                    *(float *)(PTR_weaponstay_time_001774b4 + 0xc);
            if (fc_120 <= fVar2) {
LAB_00094f86:
              fVar2 = fVar1 + fc_120;
            }
            else if (fVar2 <= fc_10) {
              fVar2 = fVar1 + fc_10;
            }
            else {
              if (fc_120 <= fVar2) goto LAB_00094f86;
              fVar2 = fVar2 + fVar1;
            }
            *(float *)(iVar11 + 0xc4) = fVar2;
            *(edict_t **)(iVar11 + 0xd0) = this->pev->pContainingEntity;
          }
          iVar6 = get_weapon_info__F10tsweapon_t(iVar4);
          this->field_0x548 = this->field_0x548 + (uint)*(byte *)(iVar6 + 0xd0);
          iVar10 = iVar10 + 2;
        } while (iVar10 < 2);
        CTSGun::DropWeapon(this_00,a0,local_71);
        CTSGun::DropWeapon(this_00,iVar4,local_71);
      }
      else {
        iVar11 = get_weapon_info__F10tsweapon_t(a0);
        if ((*(int *)(iVar11 + 200) == -1) ||
           (iVar11 = get_weapon_info__F10tsweapon_t(a0),
           (&this_00->field_0x114)[*(int *)(iVar11 + 200) * 0x38] == '\0')) {
          iVar11 = ThrowWeapon(this,a0,&this_00->field_0x114 + a0 * 0x38,0,0);
          iVar4 = get_weapon_info__F10tsweapon_t(a0);
          this->field_0x548 = this->field_0x548 + (uint)*(byte *)(iVar4 + 0xd0);
          goto LAB_000951af;
        }
        piVar5 = (int *)(&this_00->field_0x114 + a0 * 0x38);
        piVar14 = local_5c;
        for (iVar11 = 0xe; iVar11 != 0; iVar11 = iVar11 + -1) {
          *piVar14 = *piVar5;
          piVar5 = piVar5 + 1;
          piVar14 = piVar14 + 1;
        }
        local_5c[1] = (&this_00->field_0x118)[a0 * 0xe];
        iVar11 = get_weapon_info__F10tsweapon_t(a0);
        (&this_00->field_0x118)[a0 * 0xe] = (&this_00->field_0x134)[*(int *)(iVar11 + 200) * 0xe];
        iVar11 = ThrowWeapon(this,a0,local_5c,0,0);
        iVar4 = get_weapon_info__F10tsweapon_t(a0);
        this->field_0x548 = this->field_0x548 + (uint)*(byte *)(iVar4 + 0xd0);
        uVar16 = CONCAT22(uVar17,(ushort)local_71);
        iVar4 = get_weapon_info__F10tsweapon_t(a0);
        CTSGun::DropWeapon(this_00,*(int *)(iVar4 + 200),SUB41(uVar16,0));
        CTSGun::HolsterWeapon(this_00);
        iVar4 = CTSGun::SelectWeapon(this_00,a0,uVar16 & 0xffff0000);
        if (iVar4 != 0) {
          (*this_00->vtbl->Deploy)(this_00);
        }
      }
    }
    else {
      piVar5 = (int *)(&this_00->field_0x114 + a0 * 0x38);
      piVar14 = local_5c;
      for (iVar11 = 0xe; iVar11 != 0; iVar11 = iVar11 + -1) {
        *piVar14 = *piVar5;
        piVar5 = piVar5 + 1;
        piVar14 = piVar14 + 1;
      }
      if (param_2 == 0xfffffffe) {
        local_5c[1] = 1;
      }
      iVar11 = ThrowWeapon(this,a0,local_5c,0,0);
      iVar4 = get_weapon_info__F10tsweapon_t(a0);
      this->field_0x548 = this->field_0x548 + (uint)*(byte *)(iVar4 + 0xd0) * local_5c[1];
      if (local_5c[1] < (&this_00->field_0x118)[a0 * 0xe]) {
        (&this_00->field_0x118)[a0 * 0xe] = (&this_00->field_0x118)[a0 * 0xe] - local_5c[1];
        if (local_71 == false) {
          CTSGun::HolsterWeapon(this_00);
        }
      }
      else {
        CTSGun::DropWeapon(this_00,a0,local_71);
      }
    }
    peVar8 = this->pev;
LAB_000951c0:
    EMIT_SOUND_DYN__FP7edict_siPCcffiif
              (peVar8->pContainingEntity,3,"weapons/weapondrop.wav",fc_1,fc_0_800000011920929,0,100,
               fc_0_5);
  }
  if (((-1 < (int)param_2) || (a0 == 0x1d)) || (iVar11 == 0)) goto LAB_0009526b;
  fVar1 = (*PTR_gpGlobals_00177410)->time;
  fVar2 = *(float *)(PTR_weaponstay_time_001774b4 + 0xc) +
          *(float *)(PTR_weaponstay_time_001774b4 + 0xc);
  if (fc_120 <= fVar2) {
LAB_0009523c:
    fVar2 = fVar1 + fc_120;
  }
  else if (fVar2 <= fc_10) {
    fVar2 = fVar1 + fc_10;
  }
  else {
    if (fc_120 <= fVar2) goto LAB_0009523c;
    fVar2 = fVar2 + fVar1;
  }
  *(float *)(iVar11 + 0xc4) = fVar2;
  *(edict_t **)(iVar11 + 0xd0) = this->pev->pContainingEntity;
LAB_0009526b:
  lVar15 = (longdouble)GetSpeedBySlots__FP11CBasePlayer(this);
  SetPlayerSpeed(this,(float)lVar15);
  return;
}


