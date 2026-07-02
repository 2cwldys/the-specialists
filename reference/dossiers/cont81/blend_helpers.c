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


// ===== FUN @ 0006d538 size=247 =====

void BlendAnimationsByDir__12CTSAnimationP11CBasePlayerG6VectorT2PA15_cffPCc
               (CTSAnimation *param_1,CBasePlayer *param_2,undefined4 *param_3,undefined4 *param_4,
               int param_5,vec_t param_6,undefined4 param_7,byte *param_8)

{
  Vector a0;
  Vector a1;
  int iVar1;
  byte *pbVar2;
  byte *pbVar3;
  bool bVar4;
  bool bVar5;
  float in_stack_ffffff90;
  undefined4 local_6c;
  undefined4 local_68;
  undefined4 local_64;
  char local_5c [76];
  
  bVar4 = false;
  bVar5 = false;
  iVar1 = 1;
  pbVar2 = param_8;
  pbVar3 = &DAT_00106657;
  do {
    if (iVar1 == 0) break;
    iVar1 = iVar1 + -1;
    bVar4 = *pbVar2 < *pbVar3;
    bVar5 = *pbVar2 == *pbVar3;
    pbVar2 = pbVar2 + 1;
    pbVar3 = pbVar3 + 1;
  } while (bVar5);
  if ((!bVar4 && !bVar5) != bVar4) {
    local_6c = *param_3;
    local_68 = param_3[1];
    local_64 = param_3[2];
    a0.y = (vec_t)&stack0xffffff84;
    a0.x = (vec_t)&local_6c;
    a0.z = param_6;
    a1.y = (vec_t)*param_4;
    a1.x = (vec_t)param_7;
    a1.z = (vec_t)param_4[1];
    iVar1 = GetDirection__FG6VectorT0ff(a0,a1,(float)param_4[2],in_stack_ffffff90);
    if (iVar1 != param_1->field_0x14) {
      strcpy(local_5c,(char *)(iVar1 * 0x10 + param_5));
      strcat(local_5c,(char *)param_8);
      CTSAnimation::SetAnim(param_1,param_2,local_5c,0x194,0);
      param_1->field_0x14 = iVar1;
    }
  }
  return;
}


