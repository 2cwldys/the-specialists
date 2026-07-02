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


