// ===== FUN @ 000d9e18 size=296 =====

/* WARNING: Unknown calling convention -- yet parameter storage is locked */
/* player.cpp */

void CBasePlayer::AddPoints(CBasePlayer *this,int a0,int a1)

{
  entvars_t *peVar1;
  enginefuncs_t *peVar2;
  undefined uVar3;
  undefined3 extraout_var;
  int iVar4;
  
  if ((a0 < 0) && (a1 == 0)) {
    peVar1 = this->pev;
    if (peVar1->frags < fc_0) {
      return;
    }
    if (peVar1->frags < (float)-a0) {
      a0 = (int)ROUND(-peVar1->frags);
    }
  }
  this->pev->frags = (float)a0 + this->pev->frags;
  peVar2 = PTR_g_engfuncs_001778b8;
  (*PTR_g_engfuncs_001778b8->pfnMessageBegin)(2,*(undefined4 *)PTR_gmsgScoreInfo_00177108,0,0);
  uVar3 = (*peVar2->pfnIndexOfEdict)(this->pev->pContainingEntity);
  (*peVar2->pfnWriteByte)(CONCAT31(extraout_var,uVar3));
  (*peVar2->pfnWriteShort)((int)ROUND(this->pev->frags));
  (*peVar2->pfnWriteShort)(this->field_0xb30);
  (*peVar2->pfnWriteShort)(0);
  iVar4 = (**(code **)(**(int **)PTR_g_pGameRules_00177e70 + 0xd0))
                    (*(int **)PTR_g_pGameRules_00177e70,&this->field_0xb48);
  (*peVar2->pfnWriteShort)(iVar4 + 1);
  (*peVar2->pfnMessageEnd)();
  return;
}


