// ===== FUN @ 0006b470 size=7218 =====

/* WARNING: Unknown calling convention -- yet parameter storage is locked */
/* TS_animation.cpp */

void CTSAnimation::Animate(CTSAnimation *this,CBasePlayer *a0,int a1)

{
  undefined1 *__src;
  float fVar1;
  float fVar2;
  float fVar3;
  char cVar4;
  char cVar5;
  entvars_t *peVar6;
  globalvars_t *pgVar7;
  uint uVar8;
  Vector a0_00;
  Vector a0_01;
  Vector a0_02;
  Vector a0_03;
  Vector a0_04;
  Vector a0_05;
  Vector a0_06;
  Vector a0_07;
  Vector a0_08;
  Vector a0_09;
  Vector a1_00;
  Vector a1_01;
  Vector a1_02;
  Vector a1_03;
  Vector a1_04;
  Vector a1_05;
  Vector a1_06;
  Vector a1_07;
  Vector a1_08;
  Vector a1_09;
  undefined uVar9;
  int iVar10;
  undefined4 uVar11;
  undefined3 extraout_var;
  undefined3 extraout_var_00;
  undefined3 extraout_var_01;
  byte *pbVar12;
  undefined4 *puVar13;
  char *pcVar14;
  byte *pbVar15;
  bool bVar16;
  bool bVar17;
  bool bVar18;
  char *pcVar19;
  undefined4 in_stack_ffffff64;
  vec_t in_stack_ffffff68;
  uint in_stack_ffffff6c;
  float fVar20;
  float fVar21;
  float local_8c;
  float local_88;
  float local_84;
  float local_7c;
  float local_78;
  float local_74;
  float local_60;
  undefined4 local_5c;
  char local_58 [72];
  
  if (a1 < 0) {
    return;
  }
  fVar21 = (a0->pev->velocity).x;
  fVar20 = (a0->pev->velocity).y;
  fVar21 = SQRT(fVar20 * fVar20 + fVar21 * fVar21);
  if (((a0->pev->flags & 0x200) == 0) && (iVar10 = (*a0->vtbl->IsAlive)(a0), iVar10 != 0)) {
    if ((this->pev == (entvars_t *)0x4) &&
       ((fc_96 < fVar21 && (iVar10 = IsInCloseCombat(this,a1), iVar10 == 0)))) {
      peVar6 = a0->pev;
      fVar20 = (peVar6->velocity).x;
      fVar1 = (peVar6->velocity).y;
      fVar2 = (peVar6->velocity).z;
      local_60 = SQRT(fVar2 * fVar2 + fVar1 * fVar1 + fVar20 * fVar20);
      if (local_60 == fc_0) {
        local_74 = fc_1;
        local_7c = 0.0;
        local_78 = 0.0;
      }
      else {
        local_74 = fc_1 / local_60;
        local_7c = local_74 * (peVar6->velocity).x;
        local_78 = local_74 * (peVar6->velocity).y;
        local_74 = local_74 * (peVar6->velocity).z;
      }
      peVar6 = a0->pev;
      local_8c = (peVar6->v_angle).x;
      local_88 = (peVar6->v_angle).y;
      local_84 = (peVar6->v_angle).z;
      BlendAnimationsByDir__12CTSAnimationP11CBasePlayerG6VectorT2PA15_cffPCc
                (this,a0,&local_7c,&local_8c);
    }
    if (a1 == 2) {
      local_8c = *(float *)&a0->field_0x510;
      local_88 = *(float *)&a0->field_0x514;
      local_84 = *(float *)&a0->field_0x518;
      peVar6 = a0->pev;
      local_7c = (peVar6->v_angle).x;
      local_78 = (peVar6->v_angle).y;
      local_74 = (peVar6->v_angle).z;
      iVar10 = 1;
      pcVar19 = &a0->field_0xb58;
      pcVar14 = "";
      do {
        if (iVar10 == 0) break;
        iVar10 = iVar10 + -1;
        cVar5 = *pcVar14;
        cVar4 = *pcVar19;
        pcVar19 = pcVar19 + 1;
        pcVar14 = pcVar14 + 1;
      } while (cVar4 == cVar5);
      BlendAnimationsByDir__12CTSAnimationP11CBasePlayerG6VectorT2PA15_cffPCc
                (this,a0,&local_8c,&local_7c);
    }
  }
  fVar20 = (float)(in_stack_ffffff6c & 0xffffff);
  iVar10 = IsInCloseCombat(this,(int)this->pev);
  if (iVar10 == 0) {
    switch(this->pev) {
    case (entvars_t *)0x3:
      iVar10 = (*a0->vtbl->IsAlive)(a0);
      if (((iVar10 != 0) && (a1 != 4)) &&
         (((a1 != 0x39 && (((a1 != 0x2a && (a1 != 0x2b)) && (a1 != 0x29)))) &&
          (((a1 != 0 && (a1 != 8)) &&
           ((a1 != 9 && (iVar10 = CBasePlayer::IsOnLadder(a0), iVar10 == 0)))))))) {
        BlendAnimationsForSlide(this,a0);
        return;
      }
      break;
    case (entvars_t *)0x4:
      if (((((*(int *)&a0->m_fSequenceFinished == 0) && (a1 != 4)) &&
           (((a1 != 0x28 && ((a1 != 0x2f && (a1 != 0x30)))) && (a1 != 0x31)))) &&
          (((a1 != 0x37 && (a1 != 0x38)) && (a1 != 0x36)))) &&
         (((a1 != 0x32 && (iVar10 = (*a0->vtbl->IsAlive)(a0), iVar10 != 0)) &&
          ((a0->pev->flags & 0x200) == 0)))) {
        iVar10 = IsInCloseCombat(this,a1);
joined_r0x0006bf4f:
        if (iVar10 == 0) {
          DAT_00106782._0_1_ = (char)DAT_00106782;
          DAT_00106782._1_1_ = SUB21(DAT_00106782,1);
          DAT_00106788._0_1_ = (char)DAT_00106788;
          DAT_00106788._1_1_ = SUB21(DAT_00106788,1);
          s_duck_idle__00106895[5] = (char)s_duck_idle__00106895._5_4_;
          s_duck_idle__00106895[6] = SUB41(s_duck_idle__00106895._5_4_,1);
          s_duck_idle__00106895[7] = SUB41(s_duck_idle__00106895._5_4_,2);
          s_duck_idle__00106895[8] = SUB41(s_duck_idle__00106895._5_4_,3);
          s_duck_idle__00106895[9] = (char)s_duck_idle__00106895._9_2_;
          s_duck_idle__00106895[10] = SUB21(s_duck_idle__00106895._9_2_,1);
          return;
        }
      }
      break;
    case (entvars_t *)0x7:
      goto switchD_0006b7ce_caseD_7;
    case (entvars_t *)0x28:
      if ((((*(int *)&a0->m_fSequenceFinished == 0) &&
           (iVar10 = (*a0->vtbl->IsAlive)(a0), iVar10 != 0)) && (a1 != 0x29)) && (a1 != 0x2e)) {
        bVar16 = a1 == 3;
LAB_0006bdba:
        if (!bVar16) {
          iVar10 = CBasePlayer::IsOnLadder(a0);
          goto joined_r0x0006bf4f;
        }
      }
      break;
    case (entvars_t *)0x29:
    case (entvars_t *)0x2e:
    case (entvars_t *)0x2f:
      if (((*(int *)&a0->m_fSequenceFinished == 0) &&
          (iVar10 = (*a0->vtbl->IsAlive)(a0), iVar10 != 0)) && ((a0->pev->iuser4 & 4) != 0)) {
        return;
      }
      break;
    case (entvars_t *)0x2a:
    case (entvars_t *)0x2b:
      UTIL_MakeVectors__FRC6Vector(&a0->pev->v_angle);
      bVar16 = false;
      peVar6 = a0->pev;
      fVar1 = (peVar6->velocity).x;
      fVar2 = (peVar6->velocity).y;
      fVar3 = (peVar6->velocity).z;
      local_60 = SQRT(fVar3 * fVar3 + fVar2 * fVar2 + fVar1 * fVar1);
      if (local_60 == fc_0) {
LAB_0006bc5d:
        iVar10 = (*a0->vtbl->IsAlive)(a0);
        if ((((iVar10 != 0) && (a1 != 0x2a)) && (a1 != 0x2b)) &&
           (((a1 != 3 && (a1 != 0x29)) && (iVar10 = CBasePlayer::IsOnLadder(a0), iVar10 == 0)))) {
          bVar16 = true;
        }
      }
      else {
        peVar6 = a0->pev;
        fVar1 = (peVar6->velocity).x;
        fVar2 = (peVar6->velocity).y;
        fVar3 = (peVar6->velocity).z;
        local_84 = SQRT(fVar3 * fVar3 + fVar2 * fVar2 + fVar1 * fVar1);
        if (local_84 == fc_0) {
          local_84 = fc_1;
          local_8c = 0.0;
          local_88 = 0.0;
        }
        else {
          local_84 = fc_1 / local_84;
          local_8c = local_84 * (peVar6->velocity).x;
          local_88 = local_84 * (peVar6->velocity).y;
          local_84 = local_84 * (peVar6->velocity).z;
        }
        pgVar7 = *PTR_gpGlobals_00177410;
        fVar1 = (pgVar7->v_right).x;
        fVar2 = (pgVar7->v_right).y;
        fVar3 = (pgVar7->v_right).z;
        local_60 = SQRT(fVar3 * fVar3 + fVar2 * fVar2 + fVar1 * fVar1);
        if (local_60 == fc_0) {
          local_74 = fc_1;
          local_7c = 0.0;
          local_78 = 0.0;
        }
        else {
          local_74 = fc_1 / local_60;
          local_7c = local_74 * (pgVar7->v_right).x;
          local_78 = local_74 * (pgVar7->v_right).y;
          local_74 = local_74 * (pgVar7->v_right).z;
        }
        if (local_84 * local_74 + local_88 * local_78 + local_8c * local_7c != fc_0)
        goto LAB_0006bc5d;
      }
      if (bVar16) {
        return;
      }
      break;
    case (entvars_t *)0x30:
      if ((*(int *)&a0->m_fSequenceFinished == 0) &&
         (iVar10 = (*a0->vtbl->IsAlive)(a0), iVar10 != 0)) {
        return;
      }
      break;
    case (entvars_t *)0x31:
      if ((*(int *)&a0->m_fSequenceFinished == 0) &&
         (iVar10 = (*a0->vtbl->IsAlive)(a0), iVar10 != 0)) {
        bVar16 = (a0->pev->iuser4 & 4) == 0;
        goto LAB_0006bdba;
      }
      break;
    case (entvars_t *)0x32:
      if ((((*(int *)&a0->m_fSequenceFinished == 0) &&
           (iVar10 = (*a0->vtbl->IsAlive)(a0), iVar10 != 0)) &&
          (iVar10 = CBasePlayer::IsOnLadder(a0), iVar10 == 0)) && ((a1 != 0x29 && (a1 != 0x2e)))) {
        return;
      }
      break;
    case (entvars_t *)0x36:
      if ((*(int *)&a0->m_fSequenceFinished == 0) &&
         (iVar10 = (*a0->vtbl->IsAlive)(a0), iVar10 != 0)) {
        return;
      }
      *(undefined4 *)&a0->field_0x520 = 0;
      break;
    case (entvars_t *)0x37:
      if ((*(int *)&a0->m_fSequenceFinished == 0) &&
         (((((iVar10 = (*a0->vtbl->IsAlive)(a0), iVar10 != 0 && ((a0->pev->iuser4 & 4) != 0)) &&
            (iVar10 = CBasePlayer::IsOnLadder(a0), iVar10 == 0)) && ((a1 != 0x37 && (a1 != 0x38))))
          && (a1 != 0x28)))) {
LAB_0006ba51:
        if ((a0->pev->flags & 0x200) == 0) {
          return;
        }
      }
      break;
    case (entvars_t *)0x38:
      if ((((*(int *)&a0->m_fSequenceFinished == 0) &&
           (iVar10 = (*a0->vtbl->IsAlive)(a0), iVar10 != 0)) && ((a0->pev->iuser4 & 4) != 0)) &&
         (((iVar10 = CBasePlayer::IsOnLadder(a0), iVar10 == 0 && (a1 != 0x37)) &&
          ((a1 != 0x38 && ((a1 != 0x28 && (iVar10 = IsInCloseCombat(this,a1), iVar10 == 0))))))))
      goto LAB_0006ba51;
      break;
    case (entvars_t *)0x39:
      if (((*(int *)&a0->m_fSequenceFinished == 0) && (a1 != 0x39)) &&
         (((((a1 != 0x28 && (((a1 != 0x2f && (a1 != 0x30)) && (a1 != 0x31)))) &&
            (((a1 != 0x37 && (a1 != 0x38)) && (a1 != 0x36)))) &&
           ((a1 != 0x32 && (iVar10 = (*a0->vtbl->IsAlive)(a0), iVar10 != 0)))) &&
          ((a0->pev->flags & 0x200) == 0)))) {
        iVar10 = IsInCloseCombat(this,a1);
        goto joined_r0x0006bf4f;
      }
    }
  }
  else if ((((((*(int *)&a0->m_fSequenceFinished == 0) &&
              (iVar10 = IsInCloseCombat(this,a1), iVar10 == 0)) &&
             (iVar10 = (*a0->vtbl->IsAlive)(a0), iVar10 != 0)) &&
            (((a1 != 0x28 && (a1 != 0x2f)) && ((a1 != 0x30 && ((a1 != 0x36 && (a1 != 0x32)))))))) &&
           (a1 != 0x31)) &&
          ((((a1 != 0x37 && (a1 != 0x38)) && (a1 != 0x39)) &&
           (((char)this->field_0x1e8 == '\0' || ((a0->pev->flags & 0x200) == 0)))))) {
    iVar10 = GaitCloseCombat(this,(int)this->pev,a0);
    if (iVar10 == 0) {
      return;
    }
    fVar20 = (float)CONCAT13(1,SUB43(fVar20,0));
  }
  if ((char)((uint)fVar20 >> 0x18) != '\0') goto switchD_0006bfba_caseD_2;
  iVar10 = IsInCloseCombat(this,a1);
  if (iVar10 != 0) {
    HandleCloseCombat(this,a0,a1);
    goto switchD_0006bfba_caseD_2;
  }
  switch(a1) {
  case 0:
  case 8:
  case 9:
    if ((a0->pev->flags & 0x200) == 0) {
      return;
    }
    if ((a0->pev->iuser4 & 0x20) == 0) {
      this->field_0xc = 0x40000000;
      if ((a0->pev->flags & 0x4000) == 0) {
        pcVar19 = "shoot_";
        puVar13 = &local_5c;
        for (iVar10 = 1; iVar10 != 0; iVar10 = iVar10 + -1) {
          *puVar13 = *(undefined4 *)pcVar19;
          pcVar19 = pcVar19 + 4;
          puVar13 = puVar13 + 1;
        }
        *(undefined2 *)puVar13 = *(undefined2 *)pcVar19;
        *(char *)((int)puVar13 + 2) = pcVar19[2];
      }
      else {
        pcVar19 = "crouch_shoot_";
        puVar13 = &local_5c;
        for (iVar10 = 3; iVar10 != 0; iVar10 = iVar10 + -1) {
          *puVar13 = *(undefined4 *)pcVar19;
          pcVar19 = pcVar19 + 4;
          puVar13 = puVar13 + 1;
        }
        *(undefined2 *)puVar13 = *(undefined2 *)pcVar19;
      }
      __src = &a0->field_0xb58;
      strcat((char *)&local_5c,__src);
      if ((((fc_0_0010000000474974513 < (float)this->field_0x8) && ((a0->pev->flags & 0x4000) == 0))
          && (fc_0 == a0->pev->fov)) &&
         ((iVar10 = strncmp(__src,"one",3), iVar10 == 0 ||
          (iVar10 = strncmp(__src,"two",3), iVar10 == 0)))) {
        strcat((char *)&local_5c,"2");
        this->field_0xc = 0;
      }
      if (a1 != 0) {
        pcVar19 = "slide_b_right";
        if (a1 != 9) {
          pcVar19 = "slide_b_left";
        }
        strcat((char *)&local_5c,pcVar19 + 7);
      }
      iVar10 = 0x92;
    }
    else {
      if (a0->field_0x51c == '\0') {
        pcVar19 = "slide_f_";
        pcVar14 = (char *)&local_5c;
        for (iVar10 = 2; iVar10 != 0; iVar10 = iVar10 + -1) {
          *(undefined4 *)pcVar14 = *(undefined4 *)pcVar19;
          pcVar19 = pcVar19 + 4;
          pcVar14 = pcVar14 + 4;
        }
        *pcVar14 = *pcVar19;
      }
      else {
        local_8c = *(float *)&a0->field_0x510;
        local_88 = *(float *)&a0->field_0x514;
        local_84 = *(float *)&a0->field_0x518;
        peVar6 = a0->pev;
        local_7c = (peVar6->v_angle).x;
        local_78 = (peVar6->v_angle).y;
        local_74 = (peVar6->v_angle).z;
        a0_09.y = (vec_t)&local_7c;
        a0_09.x = (vec_t)&local_8c;
        a0_09.z = 0.6;
        a1_09.y = (vec_t)in_stack_ffffff64;
        a1_09.x = 0.3;
        a1_09.z = in_stack_ffffff68;
        iVar10 = GetDirection__FG6VectorT0ff(a0_09,a1_09,fVar20,fVar21);
        if ((iVar10 != this->field_0x14) && (iVar10 != 2)) {
          this->field_0x14 = iVar10;
        }
        strcpy((char *)&local_5c,(char *)(&this->field_0x9c + this->field_0x14 * 4));
      }
      strcat((char *)&local_5c,&a0->field_0xb58);
      strcat((char *)&local_5c,"_shoot");
      if (a1 != 0) {
        pcVar19 = "slide_b_right";
        if (a1 != 9) {
          pcVar19 = "slide_b_left";
        }
        strcat((char *)&local_5c,pcVar19 + 7);
      }
      iVar10 = 0x94;
    }
    goto LAB_0006ce80;
  case 1:
  case 6:
    if (((a0->pev->flags & 0x200) == 0) && (iVar10 = CBasePlayer::IsOnLadder(a0), iVar10 == 0)) {
      return;
    }
    bVar18 = false;
    bVar16 = false;
    iVar10 = 1;
    pbVar12 = &a0->field_0x478;
    pbVar15 = &DAT_00106657;
    do {
      if (iVar10 == 0) break;
      iVar10 = iVar10 + -1;
      bVar16 = *pbVar12 < *pbVar15;
      bVar18 = *pbVar12 == *pbVar15;
      pbVar12 = pbVar12 + 1;
      pbVar15 = pbVar15 + 1;
    } while (bVar18);
    bVar17 = (!bVar16 && !bVar18) < bVar16;
    if ((!bVar16 && !bVar18) == bVar16) {
      iVar10 = 1;
      bVar16 = true;
      pbVar12 = &a0->field_0xb58;
      pbVar15 = &DAT_00106657;
      do {
        if (iVar10 == 0) break;
        iVar10 = iVar10 + -1;
        bVar17 = *pbVar12 < *pbVar15;
        bVar16 = *pbVar12 == *pbVar15;
        pbVar12 = pbVar12 + 1;
        pbVar15 = pbVar15 + 1;
      } while (bVar16);
      if ((!bVar17 && !bVar16) == bVar17) break;
      if (((((this->pev == (entvars_t *)0x0) || (this->pev == (entvars_t *)0x8)) ||
           (this->pev == (entvars_t *)0x9)) ||
          ((this->pev == (entvars_t *)0x34 || (this->pev == (entvars_t *)0x33)))) &&
         (*(int *)&a0->m_fSequenceFinished == 0)) {
        a1 = (int)this->pev;
        break;
      }
      if ((a0->pev->flags & 0x4000) == 0) {
        if ((fc_0_009999999776482582 <= (float)this->field_0xc) ||
           (pcVar19 = "idle_", fc_0 != a0->pev->fov)) {
          pcVar19 = "aim_";
        }
      }
      else if ((fc_0_009999999776482582 <= (float)this->field_0xc) ||
              (pcVar19 = "duck_idle_", fc_0 != a0->pev->fov)) {
        pcVar19 = "crouch_aim_";
      }
      strcpy((char *)&local_5c,pcVar19);
      strcat((char *)&local_5c,&a0->field_0xb58);
      iVar10 = 0x12;
      goto LAB_0006cfe1;
    }
    iVar10 = 0x12;
    pcVar19 = &a0->field_0x478;
    goto LAB_0006cfea;
  case 3:
    if ((((this->pev != (entvars_t *)0x0) && (this->pev != (entvars_t *)0x8)) &&
        (this->pev != (entvars_t *)0x9)) || (*(int *)&a0->m_fSequenceFinished != 0)) {
      if (a0->field_0x51c == '\0') {
        pcVar19 = "slide_f_";
        pcVar14 = (char *)&local_5c;
        for (iVar10 = 2; iVar10 != 0; iVar10 = iVar10 + -1) {
          *(undefined4 *)pcVar14 = *(undefined4 *)pcVar19;
          pcVar19 = pcVar19 + 4;
          pcVar14 = pcVar14 + 4;
        }
        *pcVar14 = *pcVar19;
      }
      else {
        local_8c = *(float *)&a0->field_0x510;
        local_88 = *(float *)&a0->field_0x514;
        local_84 = *(float *)&a0->field_0x518;
        peVar6 = a0->pev;
        local_7c = (peVar6->v_angle).x;
        local_78 = (peVar6->v_angle).y;
        local_74 = (peVar6->v_angle).z;
        a0_05.y = (vec_t)&local_7c;
        a0_05.x = (vec_t)&local_8c;
        a0_05.z = 0.6;
        a1_05.y = (vec_t)in_stack_ffffff64;
        a1_05.x = 0.3;
        a1_05.z = in_stack_ffffff68;
        iVar10 = GetDirection__FG6VectorT0ff(a0_05,a1_05,fVar20,fVar21);
        if ((iVar10 != this->field_0x14) && (iVar10 != 2)) {
          this->field_0x14 = iVar10;
        }
        strcpy((char *)&local_5c,(char *)(&this->field_0x9c + this->field_0x14 * 4));
      }
      bVar16 = &stack0xffffff64 < (undefined1 *)0x8;
      goto LAB_0006c776;
    }
    break;
  case 4:
    uVar8 = a0->pev->iuser4;
    if ((uVar8 & 0x20) != 0) {
      return;
    }
    bVar16 = false;
    bVar18 = (uVar8 & 4) == 0;
    if (!bVar18) {
      return;
    }
    iVar10 = 1;
    pbVar12 = &a0->field_0xb58;
    pbVar15 = &DAT_00106657;
    do {
      if (iVar10 == 0) break;
      iVar10 = iVar10 + -1;
      bVar16 = *pbVar12 < *pbVar15;
      bVar18 = *pbVar12 == *pbVar15;
      pbVar12 = pbVar12 + 1;
      pbVar15 = pbVar15 + 1;
    } while (bVar18);
    if ((!bVar16 && !bVar18) == bVar16) {
      pcVar19 = "standard_jump";
      puVar13 = &local_5c;
      for (iVar10 = 3; iVar10 != 0; iVar10 = iVar10 + -1) {
        *puVar13 = *(undefined4 *)pcVar19;
        pcVar19 = pcVar19 + 4;
        puVar13 = puVar13 + 1;
      }
      *(undefined2 *)puVar13 = *(undefined2 *)pcVar19;
    }
    else {
      peVar6 = a0->pev;
      fVar1 = (peVar6->velocity).x;
      fVar2 = (peVar6->velocity).y;
      fVar3 = (peVar6->velocity).z;
      local_60 = SQRT(fVar3 * fVar3 + fVar2 * fVar2 + fVar1 * fVar1);
      if (local_60 == fc_0) {
        local_84 = fc_1;
        local_8c = 0.0;
        local_88 = 0.0;
      }
      else {
        local_84 = fc_1 / local_60;
        local_8c = local_84 * (peVar6->velocity).x;
        local_88 = local_84 * (peVar6->velocity).y;
        local_84 = local_84 * (peVar6->velocity).z;
      }
      peVar6 = a0->pev;
      local_7c = (peVar6->v_angle).x;
      local_78 = (peVar6->v_angle).y;
      local_74 = (peVar6->v_angle).z;
      a0_08.y = (vec_t)&local_7c;
      a0_08.x = (vec_t)&local_8c;
      a0_08.z = 0.6;
      a1_08.y = (vec_t)in_stack_ffffff64;
      a1_08.x = 0.3;
      a1_08.z = in_stack_ffffff68;
      iVar10 = GetDirection__FG6VectorT0ff(a0_08,a1_08,fVar20,fVar21);
      this->field_0x14 = iVar10;
      strcpy((char *)&local_5c,(char *)(&this->field_0x1c + iVar10 * 4));
      strcat((char *)&local_5c,&a0->field_0xb58);
    }
    iVar10 = 0x94;
    goto LAB_0006ce80;
  case 5:
    if (this->field_0x4 == 0) {
      SetAnim(this,a0,"goduck",0x94,0);
      this->field_0x4 = 1;
    }
    break;
  case 7:
    if (this->field_0x1e4 == 0) {
      if ((fc_80 <= fVar21) || (0x32 < a0->m_lastDamageAmount)) {
        if (a0->m_lastDamageAmount < 0x33) {
          peVar6 = a0->pev;
          fVar1 = (peVar6->velocity).x;
          fVar2 = (peVar6->velocity).y;
          fVar3 = (peVar6->velocity).z;
          local_60 = SQRT(fVar3 * fVar3 + fVar2 * fVar2 + fVar1 * fVar1);
          if (local_60 == fc_0) {
            local_84 = fc_1;
            local_8c = 0.0;
            local_88 = 0.0;
          }
          else {
            local_84 = fc_1 / local_60;
            local_8c = local_84 * (peVar6->velocity).x;
            local_88 = local_84 * (peVar6->velocity).y;
            local_84 = local_84 * (peVar6->velocity).z;
          }
          peVar6 = a0->pev;
          local_7c = (peVar6->v_angle).x;
          local_78 = (peVar6->v_angle).y;
          local_74 = (peVar6->v_angle).z;
        }
        else {
          local_8c = -*(float *)PTR_g_vecAttackDir_0017799c;
          local_88 = -*(float *)(PTR_g_vecAttackDir_0017799c + 4);
          local_84 = -*(float *)(PTR_g_vecAttackDir_0017799c + 8);
          peVar6 = a0->pev;
          local_7c = (peVar6->v_angle).x;
          local_78 = (peVar6->v_angle).y;
          local_74 = (peVar6->v_angle).z;
        }
        a0_07.y = (vec_t)&local_7c;
        a0_07.x = (vec_t)&local_8c;
        a0_07.z = 0.6;
        a1_07.y = (vec_t)in_stack_ffffff64;
        a1_07.x = 0.3;
        a1_07.z = in_stack_ffffff68;
        iVar10 = GetDirection__FG6VectorT0ff(a0_07,a1_07,fVar20,fVar21);
        if (iVar10 == 2) {
          iVar10 = 0x14;
          pcVar19 = "forward_die";
        }
        else {
          if (iVar10 != 3) goto LAB_0006ca14;
          if ((a0->field_0x10c == 1) &&
             (uVar9 = (*PTR_g_engfuncs_001778b8->pfnRandomLong)(),
             0x1e < CONCAT31(extraout_var_00,uVar9))) {
            iVar10 = 0x14;
            pcVar19 = "die_spinback";
          }
          else {
            uVar9 = (*PTR_g_engfuncs_001778b8->pfnRandomLong)();
            if (CONCAT31(extraout_var_01,uVar9) == 0) {
              iVar10 = 0x14;
              pcVar19 = "leg_die";
            }
            else {
              if (CONCAT31(extraout_var_01,uVar9) != 1) break;
              iVar10 = 0x14;
              pcVar19 = "die_back";
            }
          }
        }
      }
      else {
LAB_0006ca14:
        iVar10 = 0x14;
        pcVar19 = "die_spin";
      }
    }
    else {
      uVar9 = (*PTR_g_engfuncs_001778b8->pfnRandomLong)();
      iVar10 = CONCAT31(extraout_var,uVar9);
      if (iVar10 != 1) {
        if (iVar10 < 2) {
          if (iVar10 == 0) {
            iVar10 = 0x14;
            pcVar19 = "normal_die1";
            goto LAB_0006cfea;
          }
        }
        else if (iVar10 == 2) {
          iVar10 = 0x14;
          pcVar19 = "normal_die3";
          goto LAB_0006cfea;
        }
        break;
      }
      iVar10 = 0x14;
      pcVar19 = "normal_die2";
    }
    goto LAB_0006cfea;
  case 0x28:
    peVar6 = a0->pev;
    fVar1 = (peVar6->velocity).x;
    fVar2 = (peVar6->velocity).y;
    fVar3 = (peVar6->velocity).z;
    local_60 = SQRT(fVar3 * fVar3 + fVar2 * fVar2 + fVar1 * fVar1);
    if (local_60 == fc_0) {
      local_84 = fc_1;
      local_8c = 0.0;
      local_88 = 0.0;
    }
    else {
      local_84 = fc_1 / local_60;
      local_8c = local_84 * (peVar6->velocity).x;
      local_88 = local_84 * (peVar6->velocity).y;
      local_84 = local_84 * (peVar6->velocity).z;
    }
    peVar6 = a0->pev;
    local_7c = (peVar6->v_angle).x;
    local_78 = (peVar6->v_angle).y;
    local_74 = (peVar6->v_angle).z;
    a0_06.y = (vec_t)&local_7c;
    a0_06.x = (vec_t)&local_8c;
    a0_06.z = 0.6;
    a1_06.y = (vec_t)in_stack_ffffff64;
    a1_06.x = 0.3;
    a1_06.z = in_stack_ffffff68;
    iVar10 = GetDirection__FG6VectorT0ff(a0_06,a1_06,fVar20,fVar21);
    this->field_0x14 = iVar10;
    strcpy((char *)&local_5c,(char *)(&this->field_0x5c + iVar10 * 4));
    bVar16 = (undefined1 *)0xfffffff7 < &stack0xffffff54;
LAB_0006c776:
    bVar18 = &stack0x00000000 == (undefined1 *)0xa4;
    iVar10 = 1;
    pbVar12 = &a0->field_0xb58;
    pbVar15 = &DAT_00106657;
    do {
      if (iVar10 == 0) break;
      iVar10 = iVar10 + -1;
      bVar16 = *pbVar12 < *pbVar15;
      bVar18 = *pbVar12 == *pbVar15;
      pbVar12 = pbVar12 + 1;
      pbVar15 = pbVar15 + 1;
    } while (bVar18);
    pcVar19 = "akimbo";
    if ((!bVar16 && !bVar18) != bVar16) {
      pcVar19 = &a0->field_0xb58;
    }
    strcat((char *)&local_5c,pcVar19);
    iVar10 = 0x94;
LAB_0006cfe1:
    pcVar19 = (char *)&local_5c;
    goto LAB_0006cfea;
  case 0x29:
    iVar10 = this->field_0x14 * 0x10 + 0xdc;
    goto LAB_0006c502;
  case 0x2a:
  case 0x2b:
    if (a0->field_0x51c == '\0') {
      pcVar19 = "slide_f_";
      pcVar14 = (char *)&local_5c;
      for (iVar10 = 2; iVar10 != 0; iVar10 = iVar10 + -1) {
        *(undefined4 *)pcVar14 = *(undefined4 *)pcVar19;
        pcVar19 = pcVar19 + 4;
        pcVar14 = pcVar14 + 4;
      }
      *pcVar14 = *pcVar19;
      bVar16 = &stack0xffffff64 < (undefined1 *)0x8;
      bVar18 = &stack0x00000000 == (undefined1 *)0xa4;
      iVar10 = 1;
      pbVar12 = &a0->field_0xb58;
      pbVar15 = &DAT_00106657;
      do {
        if (iVar10 == 0) break;
        iVar10 = iVar10 + -1;
        bVar16 = *pbVar12 < *pbVar15;
        bVar18 = *pbVar12 == *pbVar15;
        pbVar12 = pbVar12 + 1;
        pbVar15 = pbVar15 + 1;
      } while (bVar18);
      pcVar19 = "akimbo";
      if ((!bVar16 && !bVar18) != bVar16) {
        pcVar19 = &a0->field_0xb58;
      }
      strcat((char *)&local_5c,pcVar19);
      pcVar19 = "slide_b_left";
      if (a1 != 0x2a) {
        pcVar19 = "slide_b_right";
      }
      strcat((char *)&local_5c,pcVar19 + 7);
    }
    else {
      pcVar19 = "slide_b_left";
      if (a1 != 0x2a) {
        pcVar19 = "slide_b_right";
      }
      strcpy((char *)&local_5c,pcVar19);
    }
    iVar10 = 0x14;
    goto LAB_0006ce80;
  case 0x2e:
    local_8c = *(float *)&a0->field_0x510;
    local_88 = *(float *)&a0->field_0x514;
    local_84 = *(float *)&a0->field_0x518;
    peVar6 = a0->pev;
    local_7c = (peVar6->v_angle).x;
    local_78 = (peVar6->v_angle).y;
    local_74 = (peVar6->v_angle).z;
    a0_04.y = (vec_t)&local_7c;
    a0_04.x = (vec_t)&local_8c;
    a0_04.z = 0.6;
    a1_04.y = (vec_t)in_stack_ffffff64;
    a1_04.x = 0.3;
    a1_04.z = in_stack_ffffff68;
    iVar10 = GetDirection__FG6VectorT0ff(a0_04,a1_04,fVar20,fVar21);
    iVar10 = iVar10 * 0x10 + 0x11c;
    goto LAB_0006c502;
  case 0x2f:
    local_8c = *(float *)&a0->field_0x510;
    local_88 = *(float *)&a0->field_0x514;
    local_84 = *(float *)&a0->field_0x518;
    peVar6 = a0->pev;
    local_7c = (peVar6->v_angle).x;
    local_78 = (peVar6->v_angle).y;
    local_74 = (peVar6->v_angle).z;
    a0_02.y = (vec_t)&local_7c;
    a0_02.x = (vec_t)&local_8c;
    a0_02.z = 0.6;
    a1_02.y = (vec_t)in_stack_ffffff64;
    a1_02.x = 0.3;
    a1_02.z = in_stack_ffffff68;
    iVar10 = GetDirection__FG6VectorT0ff(a0_02,a1_02,fVar20,fVar21);
    iVar10 = iVar10 * 0x10 + 0x15c;
LAB_0006c502:
    pcVar19 = (char *)((int)&this->pev + iVar10);
LAB_0006c505:
    SetAnim(this,a0,pcVar19,0x94,0);
    this->field_0x8 = 0x40400000;
    break;
  case 0x30:
    local_8c = *(float *)&a0->field_0x510;
    local_88 = *(float *)&a0->field_0x514;
    local_84 = *(float *)&a0->field_0x518;
    peVar6 = a0->pev;
    local_7c = (peVar6->v_angle).x;
    local_78 = (peVar6->v_angle).y;
    local_74 = (peVar6->v_angle).z;
    a0_03.y = (vec_t)&local_7c;
    a0_03.x = (vec_t)&local_8c;
    a0_03.z = 0.6;
    a1_03.y = (vec_t)in_stack_ffffff64;
    a1_03.x = 0.3;
    a1_03.z = in_stack_ffffff68;
    iVar10 = GetDirection__FG6VectorT0ff(a0_03,a1_03,fVar20,fVar21);
    pcVar19 = (char *)(&this->field_0x19c + iVar10 * 4);
    iVar10 = 0x94;
    goto LAB_0006cfea;
  case 0x31:
    local_8c = *(float *)&a0->field_0x510;
    local_88 = *(float *)&a0->field_0x514;
    local_84 = *(float *)&a0->field_0x518;
    peVar6 = a0->pev;
    local_7c = (peVar6->v_angle).x;
    local_78 = (peVar6->v_angle).y;
    local_74 = (peVar6->v_angle).z;
    a0_01.y = (vec_t)&local_7c;
    a0_01.x = (vec_t)&local_8c;
    a0_01.z = 0.6;
    a1_01.y = (vec_t)in_stack_ffffff64;
    a1_01.x = 0.3;
    a1_01.z = in_stack_ffffff68;
    uVar11 = GetDirection__FG6VectorT0ff(a0_01,a1_01,fVar20,fVar21);
    switch(uVar11) {
    case 0:
      pcVar19 = "wheel_l";
      break;
    case 1:
      pcVar19 = "wheel_r";
      break;
    default:
      pcVar19 = "wheel_f";
      break;
    case 3:
      pcVar19 = "wheel_b";
    }
    SetAnim(this,a0,pcVar19,0x94,0);
    this->field_0x8 = 0x40400000;
    break;
  case 0x32:
    local_8c = *(float *)&a0->field_0x510;
    local_88 = *(float *)&a0->field_0x514;
    local_84 = *(float *)&a0->field_0x518;
    peVar6 = a0->pev;
    local_7c = (peVar6->v_angle).x;
    local_78 = (peVar6->v_angle).y;
    local_74 = (peVar6->v_angle).z;
    a0_00.y = (vec_t)&local_7c;
    a0_00.x = (vec_t)&local_8c;
    a0_00.z = 0.6;
    a1_00.y = (vec_t)in_stack_ffffff64;
    a1_00.x = 0.3;
    a1_00.z = in_stack_ffffff68;
    iVar10 = GetDirection__FG6VectorT0ff(a0_00,a1_00,fVar20,fVar21);
    pcVar19 = "lowwheel_f";
    if ((iVar10 != 2) && (pcVar19 = "lowwheel_l", iVar10 != 0)) {
      pcVar19 = "lowwheel_r";
    }
    goto LAB_0006c505;
  case 0x33:
  case 0x34:
    if ((a0->pev->flags & 0x200) == 0) {
      return;
    }
    bVar16 = false;
    bVar18 = (a0->pev->iuser4 & 0x30U) == 0;
    if (!bVar18) {
      return;
    }
    iVar10 = 1;
    pbVar12 = &a0->field_0xb58;
    pbVar15 = &DAT_00106657;
    do {
      if (iVar10 == 0) break;
      iVar10 = iVar10 + -1;
      bVar16 = *pbVar12 < *pbVar15;
      bVar18 = *pbVar12 == *pbVar15;
      pbVar12 = pbVar12 + 1;
      pbVar15 = pbVar15 + 1;
    } while (bVar18);
    if ((!bVar16 && !bVar18) == bVar16) {
      return;
    }
    if (a1 == 0x33) {
      local_5c = DAT_0010677e;
      local_58._0_2_ = DAT_00106782;
    }
    else {
      local_5c = DAT_00106784;
      local_58._0_2_ = DAT_00106788;
    }
    strcat((char *)&local_5c,&a0->field_0xb58);
    SetAnim(this,a0,(char *)&local_5c,0x92,0);
    this->field_0xc = 0x40000000;
    break;
  case 0x35:
    if ((a0->pev->flags & 0x200) == 0) {
      return;
    }
    if ((a0->pev->iuser4 & 0x30U) != 0) {
      return;
    }
    SetAnim(this,a0,"grenade",0x92,0);
    this->field_0xc = 0x40000000;
    this->field_0x10 = 0x3f800000;
    break;
  case 0x36:
    iVar10 = 0x94;
    pcVar19 = "bullettime";
    goto LAB_0006cfea;
  case 0x37:
    SetAnim(this,a0,"walljump_end",0x94,0);
    this->field_0x8 = 0x40400000;
    break;
  case 0x38:
    pcVar19 = "walljump_climb";
    goto LAB_0006c505;
  case 0x39:
    uVar8 = a0->pev->iuser4;
    if ((uVar8 & 0x20) != 0) {
      return;
    }
    bVar16 = false;
    bVar18 = (uVar8 & 4) == 0;
    if (!bVar18) {
      return;
    }
    iVar10 = 1;
    pbVar12 = &a0->field_0xb58;
    pbVar15 = &DAT_00106657;
    do {
      if (iVar10 == 0) break;
      iVar10 = iVar10 + -1;
      bVar16 = *pbVar12 < *pbVar15;
      bVar18 = *pbVar12 == *pbVar15;
      pbVar12 = pbVar12 + 1;
      pbVar15 = pbVar15 + 1;
    } while (bVar18);
    if ((!bVar16 && !bVar18) == bVar16) {
      pcVar19 = "morfy_jump";
      puVar13 = &local_5c;
      for (iVar10 = 2; iVar10 != 0; iVar10 = iVar10 + -1) {
        *puVar13 = *(undefined4 *)pcVar19;
        pcVar19 = pcVar19 + 4;
        puVar13 = puVar13 + 1;
      }
      *(undefined2 *)puVar13 = *(undefined2 *)pcVar19;
      *(char *)((int)puVar13 + 2) = pcVar19[2];
    }
    else {
      pcVar19 = "morfy_jump_guns";
      puVar13 = &local_5c;
      for (iVar10 = 4; iVar10 != 0; iVar10 = iVar10 + -1) {
        *puVar13 = *(undefined4 *)pcVar19;
        pcVar19 = pcVar19 + 4;
        puVar13 = puVar13 + 1;
      }
    }
    iVar10 = 0x94;
LAB_0006ce80:
    pcVar19 = (char *)&local_5c;
LAB_0006cfea:
    SetAnim(this,a0,pcVar19,iVar10,0);
  }
switchD_0006bfba_caseD_2:
  if ((char)((uint)fVar20 >> 0x18) == '\0') {
    this->pev = (entvars_t *)a1;
  }
  if ((((((a1 != 4) && (a1 != 0x39)) && (a1 != 5)) && ((a1 != 7 && (a1 != 0x29)))) && (a1 != 0x2e))
     && (((a1 != 0x2f && (a1 != 0x30)) &&
         ((a1 != 0x36 &&
          (((iVar10 = IsInCloseCombat(this,a1), iVar10 == 0 ||
            (iVar10 = GaitCloseCombat(this,a1,a0), iVar10 != 0)) &&
           ((a0->pev->iuser4 & 0x30c30U) == 0)))))))) {
    if ((a0->pev->flags & 0x4000) == 0) {
      if (fVar21 <= fc_180) {
        bVar16 = false;
        bVar18 = fc_0 < fVar21;
        if (fVar21 <= fc_0) {
          iVar10 = 1;
          pbVar12 = &a0->field_0x478;
          pbVar15 = &DAT_00106657;
          do {
            if (iVar10 == 0) break;
            iVar10 = iVar10 + -1;
            bVar16 = *pbVar12 < *pbVar15;
            bVar18 = *pbVar12 == *pbVar15;
            pbVar12 = pbVar12 + 1;
            pbVar15 = pbVar15 + 1;
          } while (bVar18);
          bVar17 = (!bVar16 && !bVar18) < bVar16;
          if ((!bVar16 && !bVar18) == bVar16) {
            iVar10 = 1;
            bVar16 = true;
            pbVar12 = &a0->field_0xb58;
            pbVar15 = &DAT_00106657;
            do {
              if (iVar10 == 0) break;
              iVar10 = iVar10 + -1;
              bVar17 = *pbVar12 < *pbVar15;
              bVar16 = *pbVar12 == *pbVar15;
              pbVar12 = pbVar12 + 1;
              pbVar15 = pbVar15 + 1;
            } while (bVar16);
            if ((!bVar17 && !bVar16) == bVar17) {
              pcVar19 = "combat_idle_low";
              if (fc_0_009999999776482582 <= (float)this->field_0xc) {
                pcVar19 = "combat_idle";
              }
            }
            else {
              local_5c._0_1_ = s_duck_idle__00106895[5];
              local_5c._1_1_ = s_duck_idle__00106895[6];
              local_5c._2_1_ = s_duck_idle__00106895[7];
              local_5c._3_1_ = s_duck_idle__00106895[8];
              local_58[0] = s_duck_idle__00106895[9];
              local_58[1] = s_duck_idle__00106895[10];
              pcVar19 = (char *)&local_5c;
              strcat(pcVar19,&a0->field_0xb58);
            }
          }
          else {
            pcVar19 = &a0->field_0x478;
          }
        }
        else {
          pcVar19 = "walk2";
        }
      }
      else {
        pcVar19 = "run2";
      }
    }
    else {
      this->field_0x4 = 0;
      if (fVar21 == fc_0) {
        pcVar19 = "crouch_idle";
      }
      else {
        pcVar19 = "crawl";
      }
    }
    iVar10 = CBaseAnimating::LookupSequence((CBaseAnimating *)a0,pcVar19);
    a0->pev->gaitsequence = iVar10;
    bVar16 = (undefined1 *)0xffffffef < &stack0xffffff54;
    bVar18 = &stack0x00000000 == (undefined1 *)0x9c;
    iVar10 = 1;
    pbVar12 = &a0->field_0x478;
    pbVar15 = &DAT_00106657;
    do {
      if (iVar10 == 0) break;
      iVar10 = iVar10 + -1;
      bVar16 = *pbVar12 < *pbVar15;
      bVar18 = *pbVar12 == *pbVar15;
      pbVar12 = pbVar12 + 1;
      pbVar15 = pbVar15 + 1;
    } while (bVar18);
    bVar17 = (!bVar16 && !bVar18) < bVar16;
    if ((!bVar16 && !bVar18) == bVar16) {
      iVar10 = 1;
      bVar16 = true;
      pbVar12 = &a0->field_0xb58;
      pbVar15 = &DAT_00106657;
      do {
        if (iVar10 == 0) break;
        iVar10 = iVar10 + -1;
        bVar17 = *pbVar12 < *pbVar15;
        bVar16 = *pbVar12 == *pbVar15;
        pbVar12 = pbVar12 + 1;
        pbVar15 = pbVar15 + 1;
      } while (bVar16);
      if (((((!bVar17 && !bVar16) == bVar17) && (iVar10 = GaitCloseCombat(this,a1,a0), iVar10 == 0))
          && (((float)this->field_0xc < fc_0_009999999776482582 ||
              (*(int *)&a0->m_fSequenceFinished != 0)))) &&
         (peVar6 = a0->pev, peVar6->sequence != peVar6->gaitsequence)) {
        peVar6->sequence = peVar6->gaitsequence;
        a0->pev->frame = 0.0;
        CBaseAnimating::ResetSequenceInfo((CBaseAnimating *)a0);
      }
    }
  }
switchD_0006b7ce_caseD_7:
  return;
}


