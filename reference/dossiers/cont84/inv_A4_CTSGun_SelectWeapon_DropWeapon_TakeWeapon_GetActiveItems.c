// ===== FUN @ 00080fb8 size=36 =====

/* TS_gun.cpp */

undefined4 CTSGun::SelectWeapon(CTSGun *this,uint param_2)

{
  undefined4 uVar1;
  
  uVar1 = 0;
  if (((param_2 != this->m_weaponType) && (param_2 < 0x26)) &&
     ((((&this->field_0x114)[param_2 * 0x38] != '\0' &&
       (*(char *)(&global_weapon_info + param_2 * 0xf7) != '\0')) || (uVar1 = 0, param_2 == 0)))) {
    HolsterWeapon(this);
    this->m_weaponType = param_2;
    uVar1 = 1;
  }
  return uVar1;
}


// ===== FUN @ 0008094c size=176 =====

/* WARNING: Unknown calling convention -- yet parameter storage is locked */
/* TS_gun.cpp */

undefined4 CTSGun::DropWeapon(CTSGun *this,int a0,bool a1)

{
  int iVar1;
  undefined4 *puVar2;
  
  if ((a0 - 1U < 0x25) && ((&this->field_0x114)[a0 * 0x38] != '\0')) {
    puVar2 = (undefined4 *)(&this->field_0x114 + a0 * 0x38);
    for (iVar1 = 0xe; iVar1 != 0; iVar1 = iVar1 + -1) {
      *puVar2 = 0;
      puVar2 = puVar2 + 1;
    }
    if ((this->m_pPlayer == (CBaseEntity *)0x0) ||
       (iVar1 = (*this->m_pPlayer->vtbl->IsAlive)(this->m_pPlayer), iVar1 != 0)) {
      if (a0 == this->m_weaponType) {
        if (a1) {
          GetBetterWeapon(this,a0);
        }
        else {
          HolsterWeapon(this);
        }
      }
      return 1;
    }
  }
  return 0;
}


// ===== FUN @ 00080a04 size=1327 =====

/* WARNING: Unknown calling convention -- yet parameter storage is locked */
/* TS_gun.cpp */

undefined4 CTSGun::TakeWeapon(CTSGun *this,int a0,s_weapon_status *a1,int a2)

{
  undefined4 *puVar1;
  int iVar2;
  char cVar3;
  int iVar4;
  int iVar5;
  int iVar6;
  uint uVar7;
  undefined4 *puVar8;
  short local_12;
  
  if (0x25 < (uint)a0) {
    return 0;
  }
  if (*(char *)(&global_weapon_info + a0 * 0xf7) != '\0') {
    local_12 = (short)a0;
    if ((&this->field_0x114)[a0 * 0x38] == '\0') {
      if ((*(int *)(&DAT_0017dc08 + a0 * 0x3dc) == -1) ||
         ((*(char *)(&global_weapon_info + *(int *)(&DAT_0017dc08 + a0 * 0x3dc) * 0xf7) != '\0' &&
          ((&this->field_0x114)[*(int *)(&DAT_0017dc08 + a0 * 0x3dc) * 0x38] == '\0'))))
      goto LAB_00080b9b;
    }
    else if ((&DAT_0017dc1c)[a0 * 0x3dc] == '\0') {
      if (((*(int *)(&DAT_0017dc08 + a0 * 0x3dc) != -1) &&
          (*(char *)(&global_weapon_info + *(int *)(&DAT_0017dc08 + a0 * 0x3dc) * 0xf7) != '\0')) &&
         ((&this->field_0x114)[*(int *)(&DAT_0017dc08 + a0 * 0x3dc) * 0x38] == '\0')) {
        local_12 = *(short *)(&DAT_0017dc08 + a0 * 0x3dc);
        goto LAB_00080b9b;
      }
    }
    else if ((&this->field_0x118)[a0 * 0xe] < *(int *)(&DAT_0017db64 + a0 * 0x3dc)) {
LAB_00080b9b:
      iVar5 = (int)local_12;
      puVar1 = (undefined4 *)(&this->field_0x114 + iVar5 * 0x38);
      iVar2 = iVar5 * 0x3dc;
      if (((&DAT_0017dc1c)[iVar2] == '\0') || ((&this->field_0x114)[iVar5 * 0x38] == '\0')) {
        if (a1 == (s_weapon_status *)0x0) {
          *(undefined1 *)puVar1 = 1;
          (&this->field_0x115)[iVar5 * 0x38] = (&DAT_0017db48)[iVar2];
          (&this->field_0x120)[iVar5 * 0xe] = 0;
          if ((*(uint *)(&DAT_0017db4c + iVar2) & 1) == 0) {
            if ((*(uint *)(&DAT_0017db4c + iVar2) & 8) == 0) {
              (&this->field_0x116)[iVar5 * 0x38] =
                   '\x02' - ((*(uint *)(&DAT_0017db4c + iVar2) & 4) == 0);
            }
            else {
              (&this->field_0x116)[iVar5 * 0x38] = 3;
            }
          }
          else {
            (&this->field_0x116)[iVar5 * 0x38] = 0;
          }
          cVar3 = IsWeaponAkimbo__Fi((int)local_12);
          if (cVar3 == '\0') {
            cVar3 = IsOnlyAkimbo__Fi((int)local_12);
            if (cVar3 == '\0') {
              (&this->field_0x118)[iVar5 * 0xe] = (&DAT_0017db44)[iVar5 * 0xf7];
              (&this->field_0x124)[iVar5 * 0xe] = a2 | *(uint *)(&DAT_0017dc24 + iVar2);
              (&this->field_0x128)[iVar5 * 0xe] = 0;
            }
            else {
              (&this->field_0x130)[iVar5 * 0xe] = (int)(&DAT_0017db44)[iVar5 * 0xf7] / 2;
              (&this->field_0x134)[iVar5 * 0xe] = (int)(&DAT_0017db44)[iVar5 * 0xf7] / 2;
              (&this->field_0x118)[iVar5 * 0xe] = (&DAT_0017db44)[iVar5 * 0xf7];
              (&this->field_0x124)[iVar5 * 0xe] = a2 | *(uint *)(&DAT_0017dc24 + iVar2);
              (&this->field_0x128)[iVar5 * 0xe] = 0;
            }
          }
          else {
            iVar4 = (&this->field_0x118)[*(int *)(&DAT_0017dc0c + iVar2) * 0xe];
            (&this->field_0x130)[iVar5 * 0xe] = iVar4;
            iVar6 = (&DAT_0017db44)[*(int *)(&DAT_0017dc0c + iVar2) * 0xf7];
            (&this->field_0x134)[iVar5 * 0xe] = iVar6;
            (&this->field_0x118)[iVar5 * 0xe] = iVar4 + iVar6;
            uVar7 = a2 | (&this->field_0x124)[*(int *)(&DAT_0017dc0c + iVar2) * 0xe] |
                    (&this->field_0x124)[iVar5 * 0xe];
            (&this->field_0x124)[iVar5 * 0xe] = uVar7;
            (&this->field_0x124)[*(int *)(&DAT_0017dc0c + iVar2) * 0xe] = uVar7;
            uVar7 = (&this->field_0x128)[iVar5 * 0xe] |
                    (&this->field_0x128)[*(int *)(&DAT_0017dc0c + iVar2) * 0xe];
            (&this->field_0x128)[iVar5 * 0xe] = uVar7;
            (&this->field_0x128)[*(int *)(&DAT_0017dc0c + iVar2) * 0xe] = uVar7;
          }
          (&this->field_0x12c)[iVar5 * 0xe] = 0;
          *(undefined1 *)(&this->field_0x138 + iVar5 * 0xe) = 0;
          (&this->field_0x11c)[iVar5 * 0xe] = 0;
          (&this->field_0x13c)[iVar5 * 0xe] = iVar2 + 0x17dd92;
          uVar7 = (&this->field_0x124)[iVar5 * 0xe] & *(uint *)(&DAT_0017dc20 + iVar2);
          (&this->field_0x124)[iVar5 * 0xe] = uVar7;
          if ((uVar7 & 2) != 0) {
            (&this->field_0x128)[iVar5 * 0xe] = (&this->field_0x128)[iVar5 * 0xe] | 2;
          }
          if (((&this->field_0x124)[iVar5 * 0xe] & 4) != 0) {
            (&this->field_0x128)[iVar5 * 0xe] = (&this->field_0x128)[iVar5 * 0xe] | 4;
          }
        }
        else {
          puVar8 = puVar1;
          for (iVar4 = 0xe; iVar4 != 0; iVar4 = iVar4 + -1) {
            *puVar8 = *(undefined4 *)a1;
            a1 = (s_weapon_status *)&a1->iClip;
            puVar8 = puVar8 + 1;
          }
          *(undefined1 *)puVar1 = 1;
          (&this->field_0x11c)[iVar5 * 0xe] = 0;
          (&this->field_0x120)[iVar5 * 0xe] = 0;
          cVar3 = IsWeaponAkimbo__Fi((int)local_12);
          if (cVar3 != '\0') {
            iVar6 = (&this->field_0x118)[iVar5 * 0xe] +
                    (&this->field_0x118)[*(int *)(&DAT_0017dc0c + iVar2) * 0xe];
            (&this->field_0x118)[iVar5 * 0xe] = iVar6;
            iVar4 = (&this->field_0x118)[*(int *)(&DAT_0017dc0c + iVar2) * 0xe];
            (&this->field_0x130)[iVar5 * 0xe] = iVar4;
            (&this->field_0x134)[iVar5 * 0xe] = iVar6 - iVar4;
            uVar7 = a2 | (&this->field_0x124)[*(int *)(&DAT_0017dc0c + iVar2) * 0xe] |
                    (&this->field_0x124)[iVar5 * 0xe];
            (&this->field_0x124)[iVar5 * 0xe] = uVar7;
            (&this->field_0x124)[*(int *)(&DAT_0017dc0c + iVar2) * 0xe] = uVar7;
            uVar7 = (&this->field_0x128)[iVar5 * 0xe] |
                    (&this->field_0x128)[*(int *)(&DAT_0017dc0c + iVar2) * 0xe];
            (&this->field_0x128)[iVar5 * 0xe] = uVar7;
            (&this->field_0x128)[*(int *)(&DAT_0017dc0c + iVar2) * 0xe] = uVar7;
            if ((*(uint *)(&DAT_0017db4c + iVar2) & 1) == 0) {
              if ((*(uint *)(&DAT_0017db4c + iVar2) & 8) == 0) {
                (&this->field_0x116)[iVar5 * 0x38] =
                     '\x02' - ((*(uint *)(&DAT_0017db4c + iVar2) & 4) == 0);
              }
              else {
                (&this->field_0x116)[iVar5 * 0x38] = 3;
              }
            }
            else {
              (&this->field_0x116)[iVar5 * 0x38] = 0;
            }
          }
          (&this->field_0x124)[iVar5 * 0xe] =
               (&this->field_0x124)[iVar5 * 0xe] & *(uint *)(&DAT_0017dc20 + iVar2);
        }
      }
      else {
        (&this->field_0x118)[iVar5 * 0xe] = (&this->field_0x118)[iVar5 * 0xe] + 1;
      }
      return 1;
    }
  }
  return 0;
}


// ===== FUN @ 00081228 size=93 =====

/* WARNING: Unknown calling convention -- yet parameter storage is locked */
/* TS_gun.cpp */

uint CTSGun::GetActiveItems(CTSGun *this)

{
  char cVar1;
  uint uVar2;
  
  uVar2 = 0;
  if (this->m_pActiveItem != (s_weapon_status *)0x0) {
    uVar2 = this->m_pActiveItem->attachActive;
    cVar1 = IsWeaponAkimbo__Fi(this->m_weaponType);
    if ((cVar1 != '\0') || (cVar1 = IsOnlyAkimbo__Fi(this->m_weaponType), cVar1 != '\0')) {
      uVar2 = uVar2 | 0x40;
    }
  }
  return uVar2;
}


