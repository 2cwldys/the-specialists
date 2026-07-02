// ===== FUN @ 0006f4f4 size=354 =====

void UTIL_TraceLine_Expanded__FRC6VectorT015IGNORE_MONSTERSP7edict_sP11TraceResult
               (undefined4 param_1,undefined4 param_2,undefined4 param_3,undefined4 param_4,
               undefined4 param_5)

{
  float fVar1;
  int *piVar2;
  int iVar3;
  int iVar4;
  undefined4 *puVar5;
  undefined4 *puVar6;
  int local_18;
  int local_14;
  
  iVar4 = 1;
  if (0 < (*PTR_gpGlobals_00177410)->maxClients) {
    local_18 = 0xc;
    do {
      piVar2 = (int *)UTIL_PlayerByIndex__Fi(iVar4);
      if (piVar2 != (int *)0x0) {
        puVar5 = (undefined4 *)(*piVar2 + 0xd0);
        puVar6 = (undefined4 *)(PTR_vecBoxes_00177bdc + local_18);
        for (iVar3 = 3; fVar1 = fc_32, iVar3 != 0; iVar3 = iVar3 + -1) {
          *puVar6 = *puVar5;
          puVar5 = puVar5 + 1;
          puVar6 = puVar6 + 1;
        }
        *(float *)(*piVar2 + 0xd0) = *(float *)(*piVar2 + 0xd0) + fc_32;
        *(float *)(*piVar2 + 0xd4) = *(float *)(*piVar2 + 0xd4) + fVar1;
        *(float *)(*piVar2 + 0xd8) = *(float *)(*piVar2 + 0xd8) + fVar1;
        *(float *)(*piVar2 + 0xc4) = *(float *)(*piVar2 + 0xc4) - fVar1;
        *(float *)(*piVar2 + 200) = *(float *)(*piVar2 + 200) - fVar1;
        *(float *)(*piVar2 + 0xcc) = *(float *)(*piVar2 + 0xcc) - fVar1;
      }
      local_18 = local_18 + 0xc;
      iVar4 = iVar4 + 1;
    } while (iVar4 <= (*PTR_gpGlobals_00177410)->maxClients);
  }
  UTIL_TraceLine__FRC6VectorT015IGNORE_MONSTERSP7edict_sP11TraceResult
            (param_1,param_2,param_3,param_4,param_5);
  iVar4 = 1;
  if (0 < (*PTR_gpGlobals_00177410)->maxClients) {
    local_14 = 0xc;
    do {
      piVar2 = (int *)UTIL_PlayerByIndex__Fi(iVar4);
      if (piVar2 != (int *)0x0) {
        puVar5 = (undefined4 *)(PTR_vecBoxes_00177bdc + local_14);
        puVar6 = (undefined4 *)(*piVar2 + 0xd0);
        for (iVar3 = 3; iVar3 != 0; iVar3 = iVar3 + -1) {
          *puVar6 = *puVar5;
          puVar5 = puVar5 + 1;
          puVar6 = puVar6 + 1;
        }
      }
      local_14 = local_14 + 0xc;
      iVar4 = iVar4 + 1;
    } while (iVar4 <= (*PTR_gpGlobals_00177410)->maxClients);
  }
  return;
}
