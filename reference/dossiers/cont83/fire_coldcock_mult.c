// ===== FUN @ 00081c78 size=334 =====

/* WARNING: Unknown calling convention -- yet parameter storage is locked */
/* TS_gun.cpp */

void CTSGun::Fire(CTSGun *this)

{
  s_weapon_status *psVar1;
  char cVar2;
  int iVar3;
  
  if (this->m_pActiveItem != (s_weapon_status *)0x0) {
    if (this->m_pPlayer->pev->waterlevel == 3) {
      EmptySound(this);
    }
    else {
      iVar3 = InfiniteClips__FP11CBasePlayer((CBasePlayer *)this->m_pPlayer);
      if (((iVar3 == 0) && (psVar1 = this->m_pActiveItem, psVar1->iClip < 1)) &&
         ((psVar1->pumpToggle == '\0' ||
          ((psVar1->fireMode != '\x03' || (this->m_pActiveInfo->iFireType != '\x01')))))) {
        EmptySound(this);
      }
      else {
        shootanim_1297 = -1;
        UTIL_MakeVectors__FRC6Vector(&this->m_pPlayer->pev->v_angle);
        if ((byte)(this->m_pActiveInfo->iFireType - 4U) < 2) {
          cVar2 = AkimboFire(this);
          if (cVar2 != '\0') {
            shootanim_1297 = (this->m_pActiveItem->pumpToggle != '\0') + 8;
          }
        }
        else {
          if (this->m_pActiveInfo->iFireType == '\x01') {
            ShotgunFire(this);
          }
          else {
            StandardFire(this);
          }
          shootanim_1297 = 0;
        }
        if (-1 < shootanim_1297) {
          CTSAnimation::Animate
                    ((CTSAnimation *)&this->m_pPlayer->field_0x28c,(CBasePlayer *)this->m_pPlayer,
                     shootanim_1297);
        }
      }
    }
  }
  return;
}


// ===== FUN @ 00081dd0 size=821 =====

/* WARNING: Unknown calling convention -- yet parameter storage is locked */
/* TS_gun.cpp
   assets: player/kevlarhit.wav */

void CTSGun::ColdCock(CTSGun *this)

{
  entvars_t *peVar1;
  globalvars_t *pgVar2;
  edict_s *a0;
  Vector a2;
  undefined uVar3;
  int iVar4;
  undefined3 extraout_var;
  CBaseEntity *pCVar5;
  Vector *pVVar6;
  float *pfVar7;
  vec_t *pvVar8;
  longdouble lVar9;
  undefined4 in_stack_ffffff6c;
  ushort uVar10;
  uint uVar11;
  CBaseEntity *pCVar12;
  CBaseEntity *a0_00;
  float in_stack_ffffff7c;
  float fVar13;
  undefined4 uVar14;
  undefined4 in_stack_ffffff84;
  vec_t in_stack_ffffff88;
  TraceResult *in_stack_ffffff8c;
  char cVar15;
  undefined4 in_stack_ffffff90;
  float local_50;
  float local_4c;
  float local_48;
  float local_40;
  entvars_t *local_3c;
  CBaseEntity *local_38;
  
  uVar10 = (ushort)((uint)in_stack_ffffff6c >> 0x10);
  uVar14 = 0x81ddc;
  if (__tmp_0_1302 == 0) {
    __tmp_0_1302 = 1;
  }
  if (__tmp_1_1304 == 0) {
    __tmp_1_1304 = 1;
  }
  if (__tmp_2_1306 == 0) {
    __tmp_2_1306 = 1;
  }
  if (this->m_pActiveItem != (s_weapon_status *)0x0) {
    iVar4 = get_weapon_info__F10tsweapon_t(this->m_weaponType);
    if (*(int *)(iVar4 + 0x10) != 0) {
      peVar1 = this->m_pPlayer->pev;
      iVar4 = CONCAT13((peVar1->iuser4 & 0x70c30U) != 0,(int3)in_stack_ffffff90);
      UTIL_MakeVectors__FRC6Vector(&peVar1->v_angle);
      if (this->m_pActiveInfo != (s_weapon_info *)0x0) {
        if (this->m_pActiveInfo->iFireType == '\x02') {
          GrenadeIdle(this,1);
        }
        StopReload(this);
      }
      rate_1307 = 0.725;
      if ((char)((uint)iVar4 >> 0x18) != '\0') {
        rate_1307 = fc_1_5 * 0.725;
      }
      Unzoom(this);
      lVar9 = (longdouble)UTIL_WeaponTimeBase__Fv();
      lVar9 = lVar9 + (longdouble)rate_1307;
      this->field_0xa4 = (int)(float)lVar9;
      this->field_0xa0 = (int)(float)lVar9;
      this->field_0x9c = (int)(float)lVar9;
      *(float *)&this->m_pPlayer->field_0x12c = (float)lVar9;
      lVar9 = (longdouble)UTIL_WeaponTimeBase__Fv();
      this->field_0xa8 = (int)(float)((longdouble)dc_2 + lVar9);
      a0_00 = this->m_pPlayer;
      uVar11 = 0x81f29;
      a2.y = (vec_t)in_stack_ffffff84;
      a2.x = (vec_t)uVar14;
      a2.z = in_stack_ffffff88;
      pCVar12 = (CBaseEntity *)&local_3c;
      (*a0_00->vtbl[1].TraceAttack)
                ((CBaseEntity *)&local_3c,(entvars_s *)a0_00,in_stack_ffffff7c,a2,in_stack_ffffff8c,
                 iVar4);
      pCVar5 = (CBaseEntity *)&local_3c;
      pfVar7 = &vecSrc_1301;
      for (iVar4 = 3; iVar4 != 0; iVar4 = iVar4 + -1) {
        *pfVar7 = (float)pCVar5->pev;
        pCVar5 = (CBaseEntity *)&pCVar5->m_pGoalEnt;
        pfVar7 = pfVar7 + 1;
      }
      pVVar6 = &(*PTR_gpGlobals_00177410)->v_forward;
      pvVar8 = (vec_t *)&vecAiming_1303;
      for (iVar4 = 3; iVar4 != 0; iVar4 = iVar4 + -1) {
        *pvVar8 = pVVar6->x;
        pVVar6 = (Vector *)&pVVar6->y;
        pvVar8 = pvVar8 + 1;
      }
      fVar13 = (float)*(int *)&this->m_pActiveInfo->field_0x10;
      cVar15 = (char)((uint)in_stack_ffffff8c >> 0x18);
      if (cVar15 != '\0') {
        fVar13 = fVar13 * fc_0_6666666865348816;
      }
      local_40 = vecSrc_1301;
      local_3c = (entvars_t *)DAT_0017da58;
      local_38 = (CBaseEntity *)DAT_0017da5c;
      pgVar2 = *PTR_gpGlobals_00177410;
      local_50 = vecSrc_1301 + (pgVar2->v_forward).x * 35.0;
      local_4c = DAT_0017da58 + (pgVar2->v_forward).y * 35.0;
      local_48 = DAT_0017da5c + (pgVar2->v_forward).z * 35.0;
      iVar4 = ColdCockAttack__11CBasePlayerG6VectorN21ifibN37
                        (this->m_pPlayer,&local_40,&local_50,&stack0xffffff90,3,fVar13,
                         (cVar15 == '\0') - 1 | 0x27,(uint)uVar10 << 0x10,uVar11 & 0xffff0000,
                         (uint)pCVar12 & 0xffff0000,(uint)a0_00 & 0xffff0000);
      if (iVar4 != 0) {
        a0 = this->m_pPlayer->pev->pContainingEntity;
        fVar13 = 0.5;
        uVar3 = (*PTR_g_engfuncs_001778b8->pfnRandomLong)(0,0x1f);
        EMIT_SOUND_DYN__FP7edict_siPCcffiif
                  (a0,4,"player/kevlarhit.wav",1.0,0.8,0,CONCAT31(extraout_var,uVar3) + 0x3c,fVar13)
        ;
      }
      if ((this->m_pActiveItem != (s_weapon_status *)0x0) && (this->m_pActiveItem->bOwned != '\0'))
      {
        (*PTR_g_engfuncs_001778b8->pfnPlaybackEvent)
                  (1,this->m_pPlayer->pev->pContainingEntity,(uint)*(ushort *)&this->field_0x966,0,
                   PTR_g_vecZero_00177698,PTR_g_vecZero_00177698,vecDir_1305,DAT_0017da70,
                   this->m_weaponType,this->m_pActiveItem->attachEquipped,0,0);
      }
    }
  }
  return;
}


// ===== FUN @ 00081308 size=424 =====

/* WARNING: Unknown calling convention -- yet parameter storage is locked */
/* TS_gun.cpp */

int CTSGun::MultiplyClips(CTSGun *this)

{
  undefined1 *puVar1;
  int *piVar2;
  byte bVar3;
  float fVar4;
  undefined4 in_EAX;
  uint3 uVar7;
  int iVar5;
  s_weapon_info *psVar6;
  int iVar8;
  longdouble lVar9;
  
  uVar7 = (uint3)((uint)in_EAX >> 8);
  if ((this->m_pActiveItem == (s_weapon_status *)0x0) ||
     (this->m_pActiveInfo == (s_weapon_info *)0x0)) {
    iVar5 = (uint)uVar7 << 8;
  }
  else {
    iVar5 = (uint)uVar7 << 8;
    if (this->m_weaponType != 0x18) {
      StopReload(this);
      psVar6 = this->m_pActiveInfo;
      if (psVar6->field_0xdc == '\0') {
        this->m_pActiveItem->iClip = this->m_pActiveInfo->iClip << 1;
        fVar4 = fc_0_5;
        if (this->m_pActiveInfo->iAkimboType == -1) {
          psVar6 = (s_weapon_info *)
                   (CONCAT31((int3)((uint)this->m_pActiveInfo >> 8),this->m_pActiveInfo->iFireType)
                   + -4);
          if ((byte)psVar6 < 2) {
            this->m_pActiveItem->leftClip = (int)ROUND(fc_0_5 * (float)this->m_pActiveItem->iClip);
            this->m_pActiveItem->rightClip = (int)ROUND(fVar4 * (float)this->m_pActiveItem->iClip);
            psVar6 = this->m_pActiveInfo;
            if (psVar6->iSingleType != -1) {
              piVar2 = &psVar6->iSingleType;
              psVar6 = (s_weapon_info *)(int)ROUND(fVar4 * (float)this->m_pActiveItem->iClip);
              (&this->field_0x118)[*piVar2 * 0xe] = (int)psVar6;
            }
          }
        }
        else {
          iVar5 = this->m_pActiveInfo->iAkimboType;
          (&this->field_0x118)[iVar5 * 0xe] =
               (int)ROUND(fc_0_5 * (float)this->m_pActiveItem->iClip +
                          (float)(&this->field_0x118)[iVar5 * 0xe]);
          iVar5 = this->m_pActiveInfo->iAkimboType;
          psVar6 = (s_weapon_info *)(iVar5 * 7);
          (&this->field_0x130)[iVar5 * 0xe] = this->m_pActiveItem->iClip;
        }
      }
      else {
        iVar8 = (psVar6->iPellets * 2 - this->m_pActiveItem->iClip) * (uint)(byte)psVar6->slotCost;
        iVar5 = *(int *)&this->m_pPlayer->field_0x548;
        if (iVar8 < iVar5) {
          iVar5 = iVar8;
        }
        bVar3 = this->m_pActiveInfo->slotCost;
        puVar1 = &this->m_pPlayer->field_0x548;
        *(int *)puVar1 = *(int *)puVar1 - iVar5;
        lVar9 = (longdouble)GetSpeedBySlots__FP11CBasePlayer((CBasePlayer *)this->m_pPlayer);
        CBasePlayer::SetPlayerSpeed((CBasePlayer *)this->m_pPlayer,(float)lVar9);
        psVar6 = (s_weapon_info *)this->m_pActiveItem;
        psVar6->iClip = psVar6->iClip + iVar5 / (int)(uint)bVar3;
      }
      iVar5 = CONCAT31((int3)((uint)psVar6 >> 8),1);
    }
  }
  return iVar5;
}


