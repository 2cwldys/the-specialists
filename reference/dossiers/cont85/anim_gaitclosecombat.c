// ===== FUN @ 0006dd14 size=178 =====

/* WARNING: Unknown calling convention -- yet parameter storage is locked */
/* TS_animation.cpp */

undefined4 CTSAnimation::GaitCloseCombat(CTSAnimation *this,int a0,CBasePlayer *a1)

{
  float fVar1;
  float fVar2;
  entvars_t *peVar3;
  undefined4 uVar4;
  
  if ((a0 == 0x25) && (((a1->pev->flags & 0x4000) != 0 || ((a1->pev->button & 4) != 0)))) {
    uVar4 = 0;
  }
  else {
    uVar4 = 0;
    if ((((a0 != 0x24) && ((a0 != 0x23 && (a0 != 0x3a)))) &&
        (((uVar4 = 0, a0 - 0x16U < 2 || (((a0 - 0x21U < 5 || (a0 == 0x19)) || (a0 == 0x26)))) ||
         ((a0 == 0x27 || (a0 - 0x1dU < 4)))))) &&
       ((peVar3 = a1->pev, (peVar3->flags & 0x4000) != 0 ||
        (fVar1 = (peVar3->velocity).x, fVar2 = (peVar3->velocity).y,
        fc_120 < SQRT(fVar2 * fVar2 + fVar1 * fVar1))))) {
      uVar4 = 1;
    }
  }
  return uVar4;
}


