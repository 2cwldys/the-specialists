// ===== FUN @ 0006d7e4 size=225 =====

/* WARNING: Unknown calling convention -- yet parameter storage is locked */
/* TS_animation.cpp */

undefined4 CTSAnimation::SetAnim(CTSAnimation *this,CBasePlayer *a0,char *a1,int a2,int a3)

{
  int *piVar1;
  uint uVar2;
  undefined4 uVar3;
  
  uVar2 = CBaseAnimating::LookupSequence((CBaseAnimating *)a0,a1);
  uVar2 = uVar2 & (uVar2 == 0xffffffff) - 1;
  if ((a0->pev->sequence == uVar2) && (-1 < (char)a2)) {
    uVar3 = 0;
  }
  else {
    if ((a2 & 0x10U) == 0) {
      piVar1 = &a0->pev->effects;
      *piVar1 = *piVar1 | 0x20;
    }
    else {
      piVar1 = &a0->pev->effects;
      *piVar1 = *piVar1 & 0xffffffdf;
    }
    if ((a2 & 4U) == 0) {
      if ((a2 & 1U) != 0) {
        a0->pev->gaitsequence = uVar2;
        return 1;
      }
    }
    else {
      a0->pev->gaitsequence = 0;
    }
    a0->pev->sequence = uVar2;
    if ((a2 & 0x20U) == 0) {
      if ((a2 & 0x100U) == 0) {
        a0->pev->frame = 0.0;
      }
    }
    else {
      a0->pev->frame = (float)a3;
    }
    CBaseAnimating::ResetSequenceInfo((CBaseAnimating *)a0);
    uVar3 = 1;
  }
  return uVar3;
}


// ===== FUN @ 0006d630 size=414 =====

/* WARNING: Unknown calling convention -- yet parameter storage is locked */
/* TS_animation.cpp */

void CTSAnimation::BlendJumpAnimations(CTSAnimation *this,CBasePlayer *a0)

{
  float fVar1;
  float fVar2;
  float fVar3;
  entvars_t *peVar4;
  Vector a0_00;
  Vector a1;
  int iVar5;
  int iVar6;
  byte *pbVar7;
  char *pcVar8;
  byte *pbVar9;
  char *pcVar10;
  bool bVar11;
  bool bVar12;
  float in_stack_ffffff80;
  float local_6c;
  float local_68;
  float local_64;
  char local_5c [2];
  char acStack_5a [74];
  
  bVar11 = (CBasePlayer *)0xfffff4a7 < a0;
  bVar12 = &a0->field_0xb58 == (undefined1 *)0x0;
  iVar5 = 1;
  pbVar7 = &a0->field_0xb58;
  pbVar9 = &DAT_00106657;
  do {
    if (iVar5 == 0) break;
    iVar5 = iVar5 + -1;
    bVar11 = *pbVar7 < *pbVar9;
    bVar12 = *pbVar7 == *pbVar9;
    pbVar7 = pbVar7 + 1;
    pbVar9 = pbVar9 + 1;
  } while (bVar12);
  if ((!bVar11 && !bVar12) != bVar11) {
    peVar4 = a0->pev;
    fVar1 = (peVar4->velocity).x;
    fVar2 = (peVar4->velocity).y;
    fVar3 = (peVar4->velocity).z;
    local_64 = SQRT(fVar3 * fVar3 + fVar2 * fVar2 + fVar1 * fVar1);
    if (local_64 == fc_0) {
      local_64 = fc_1;
      local_6c = 0.0;
      local_68 = 0.0;
    }
    else {
      local_64 = fc_1 / local_64;
      local_6c = local_64 * (peVar4->velocity).x;
      local_68 = local_64 * (peVar4->velocity).y;
      local_64 = local_64 * (peVar4->velocity).z;
    }
    peVar4 = a0->pev;
    a0_00.y = (vec_t)&stack0xffffff74;
    a0_00.x = (vec_t)&local_6c;
    a0_00.z = 0.6;
    a1.y = (peVar4->v_angle).x;
    a1.x = 0.3;
    a1.z = (peVar4->v_angle).y;
    iVar5 = GetDirection__FG6VectorT0ff(a0_00,a1,(peVar4->v_angle).z,in_stack_ffffff80);
    if (iVar5 != this->field_0x14) {
      switch(iVar5) {
      case 0:
        pcVar8 = "jump_left_";
        pcVar10 = local_5c;
        for (iVar6 = 2; iVar6 != 0; iVar6 = iVar6 + -1) {
          *(undefined4 *)pcVar10 = *(undefined4 *)pcVar8;
          pcVar8 = pcVar8 + 4;
          pcVar10 = pcVar10 + 4;
        }
        *(undefined2 *)pcVar10 = *(undefined2 *)pcVar8;
        pcVar10[2] = pcVar8[2];
        break;
      case 1:
        pcVar8 = "jump_right_";
        pcVar10 = local_5c;
        for (iVar6 = 3; iVar6 != 0; iVar6 = iVar6 + -1) {
          *(undefined4 *)pcVar10 = *(undefined4 *)pcVar8;
          pcVar8 = pcVar8 + 4;
          pcVar10 = pcVar10 + 4;
        }
        break;
      case 2:
      case 3:
      case 4:
        pcVar8 = "jump_forward_";
        pcVar10 = local_5c;
        for (iVar6 = 3; iVar6 != 0; iVar6 = iVar6 + -1) {
          *(undefined4 *)pcVar10 = *(undefined4 *)pcVar8;
          pcVar8 = pcVar8 + 4;
          pcVar10 = pcVar10 + 4;
        }
        *(undefined2 *)pcVar10 = *(undefined2 *)pcVar8;
      }
      strcat(local_5c,&a0->field_0xb58);
      SetAnim(this,a0,local_5c,0x194,0);
      this->field_0x14 = iVar5;
    }
  }
  return;
}


// ===== FUN @ 0006d400 size=310 =====

/* WARNING: Unknown calling convention -- yet parameter storage is locked */
/* TS_animation.cpp */

void CTSAnimation::BlendAnimationsForSlide(CTSAnimation *this,CBasePlayer *a0)

{
  entvars_t *peVar1;
  Vector a0_00;
  Vector a1;
  int iVar2;
  int iVar3;
  char *__src;
  byte *pbVar4;
  byte *pbVar5;
  bool bVar6;
  bool bVar7;
  float in_stack_ffffff90;
  undefined4 local_6c;
  undefined4 local_68;
  undefined4 local_64;
  char local_5c [76];
  
  if (a0->field_0x51c != '\0') {
    local_6c = *(undefined4 *)&a0->field_0x510;
    local_68 = *(undefined4 *)&a0->field_0x514;
    local_64 = *(undefined4 *)&a0->field_0x518;
    peVar1 = a0->pev;
    a0_00.y = (vec_t)&stack0xffffff84;
    a0_00.x = (vec_t)&local_6c;
    a0_00.z = 0.6;
    a1.y = (peVar1->v_angle).x;
    a1.x = 0.3;
    a1.z = (peVar1->v_angle).y;
    iVar2 = GetDirection__FG6VectorT0ff(a0_00,a1,(peVar1->v_angle).z,in_stack_ffffff90);
    if ((iVar2 != this->field_0x14) && (iVar2 != 2)) {
      strcpy(local_5c,(char *)(&this->field_0x9c + iVar2 * 4));
      bVar6 = (CBasePlayer *)0xfffff4a7 < a0;
      bVar7 = &a0->field_0xb58 == (undefined1 *)0x0;
      iVar3 = 1;
      pbVar4 = &a0->field_0xb58;
      pbVar5 = &DAT_00106657;
      do {
        if (iVar3 == 0) break;
        iVar3 = iVar3 + -1;
        bVar6 = *pbVar4 < *pbVar5;
        bVar7 = *pbVar4 == *pbVar5;
        pbVar4 = pbVar4 + 1;
        pbVar5 = pbVar5 + 1;
      } while (bVar7);
      __src = "akimbo";
      if ((!bVar6 && !bVar7) != bVar6) {
        __src = &a0->field_0xb58;
      }
      strcat(local_5c,__src);
      SetAnim(this,a0,local_5c,0x194,0);
      this->field_0x14 = iVar2;
    }
  }
  return;
}


