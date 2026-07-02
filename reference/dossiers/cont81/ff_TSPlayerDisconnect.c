// ===== FUN @ 0009f94c size=280 =====

/* WARNING: Unknown calling convention -- yet parameter storage is locked */
/* TS_util.cpp */

void TSPlayerDisconnect__FP11CBasePlayer(CBasePlayer *a0)

{
  int *piVar1;
  undefined uVar2;
  undefined3 extraout_var;
  int iVar3;
  undefined4 *puVar4;
  
  CBasePlayer::DropFlag(a0,false);
  uVar2 = (*PTR_g_engfuncs_001778b8->pfnIndexOfEdict)(a0->pev->pContainingEntity);
  if (CONCAT31(extraout_var,uVar2) < 0x20) {
    puVar4 = (undefined4 *)(PTR_TSGameRules_00177300 + CONCAT31(extraout_var,uVar2) * 0x48 + 0x684);
    for (iVar3 = 0x12; iVar3 != 0; iVar3 = iVar3 + -1) {
      *puVar4 = 0;
      puVar4 = puVar4 + 1;
    }
  }
  a0->pev->solid = 0;
  a0->pev->deadflag = 2;
  a0->pev->takedamage = 0.0;
  piVar1 = &a0->pev->flags;
  *piVar1 = *piVar1 & 0xffffdfff;
  piVar1 = &a0->pev->flags;
  *piVar1 = *piVar1 | 0x80000000;
  a0->field_0x160 = 0;
  a0->pev->frags = 0.0;
  a0->field_0xb30 = 0;
  a0->field_0x770 = -0x40800000;
  *(undefined1 *)&a0->field_0xb48 = DAT_00106657;
  a0->pev->effects = 0x80;
  a0->pev->health = 0.0;
  a0->field_0x9b8 = 0;
  a0->field_0x534 = -0x40800000;
  a0->field_0x7b9 = 0;
  CTSGameRules::CallCheckDeads((CTSGameRules *)PTR_TSGameRules_00177300);
  return;
}
