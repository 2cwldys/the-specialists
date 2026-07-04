// ===== FUN @ 00092864 size=249 =====

/* WARNING: Unknown calling convention -- yet parameter storage is locked */
/* TS_player.cpp */

void CBasePlayer::Jump(CBasePlayer *this)

{
  entvars_t *peVar1;
  int iVar2;
  uint uVar3;
  float *pfVar4;
  Vector *pVVar5;
  float local_2c [7];
  
  if (((this->pev->flags & 0x800) == 0) && (this->pev->waterlevel < 2)) {
    iVar2 = CTSStunt::CloseAttack((CTSStunt *)&this->field_0x4f0);
    if (iVar2 == 0) {
      if (((this->m_afButtonPressed & 2) != 0) &&
         (iVar2 = CTSStunt::GoWheel((CTSStunt *)&this->field_0x4f0), iVar2 != 0)) {
        return;
      }
      if ((((this->m_afButtonPressed & 2) != 0) && ((this->pev->flags & 0x200) != 0)) &&
         (this->pev->groundentity != (edict_t *)0x0)) {
        CTSAnimation::Animate((CTSAnimation *)&this->field_0x28c,this,4);
        peVar1 = this->pev;
        uVar3 = (peVar1->groundentity == (edict_t *)0x0) - 1 & (uint)(peVar1->groundentity + 0x80);
        if ((uVar3 != 0) && ((*(byte *)(uVar3 + 0x1a4) & 4) != 0)) {
          local_2c[0] = (peVar1->velocity).x + (peVar1->basevelocity).x;
          local_2c[1] = (peVar1->velocity).y + (peVar1->basevelocity).y;
          local_2c[2] = (peVar1->velocity).z + (peVar1->basevelocity).z;
          pfVar4 = local_2c;
          pVVar5 = &peVar1->velocity;
          for (iVar2 = 3; iVar2 != 0; iVar2 = iVar2 + -1) {
            pVVar5->x = *pfVar4;
            pfVar4 = pfVar4 + 1;
            pVVar5 = (Vector *)&pVVar5->y;
          }
        }
      }
    }
  }
  return;
}


