// ===== FUN @ 100447f0 size=854 =====

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

undefined4 __cdecl HUD_AddEntity(int param_1,int *param_2)

{
  float *pfVar1;
  bool bVar2;
  uint uVar3;
  undefined4 uVar4;
  float *pfVar5;
  int iVar6;
  float10 fVar7;
  longlong lVar8;
  float local_24;
  float local_20;
  float local_1c;
  float local_18;
  float local_14;
  float local_10;
  undefined4 local_c [3];
  
                    /* 0x447f0  470  HUD_AddEntity */
  if (param_1 == 1) {
    uVar3 = FUN_10057790((int)param_2);
    if ((char)uVar3 == '\0') {
      return 0;
    }
    bVar2 = FUN_10057330((int)param_2);
    if (!bVar2) {
      return 0;
    }
    iVar6 = *param_2;
    if ((-1 < iVar6) && (iVar6 < 0x22)) {
      if ((&DAT_101900ec)[iVar6] != 0) {
        return 0;
      }
      *(undefined1 *)((int)&DAT_1018bd44 + iVar6) = 1;
      (&DAT_100d5810)[*param_2] = 1;
    }
    goto LAB_10044af9;
  }
  uVar3 = param_2[0xf0];
  if ((uVar3 & 1) == 0) {
    if ((uVar3 & 4) == 0) {
      if ((uVar3 & 2) != 0) {
        pfVar1 = (float *)(param_2 + 0x2d2);
        uVar4 = FUN_10057910(pfVar1,0x400);
        if ((char)uVar4 != '\0') {
          return 0;
        }
        uVar4 = FUN_100570b0(pfVar1);
        if ((char)uVar4 == '\0') {
          return 0;
        }
        pfVar5 = (float *)FUN_10057270(local_c);
        local_24 = *pfVar1 - *pfVar5;
        local_20 = (float)param_2[0x2d3] - pfVar5[1];
        local_1c = (float)param_2[0x2d4] - pfVar5[2];
        local_14 = local_20;
        local_10 = local_1c;
        fVar7 = FUN_10058390(&local_24);
        if (((float10)_DAT_1007d704 < fVar7) && (0x4f < DAT_100cf978)) {
          param_2[0xc1] = 0;
          param_2[0xbe] = 1;
          goto LAB_10044ac3;
        }
      }
    }
    else {
      pfVar1 = (float *)(param_2 + 0x2d2);
      uVar4 = FUN_10057910(pfVar1,0x100);
      if ((char)uVar4 != '\0') {
        return 0;
      }
      uVar4 = FUN_100570b0(pfVar1);
      if ((char)uVar4 == '\0') {
        return 0;
      }
      pfVar5 = (float *)FUN_10057270(local_c);
      local_24 = *pfVar1 - *pfVar5;
      local_20 = (float)param_2[0x2d3] - pfVar5[1];
      local_1c = (float)param_2[0x2d4] - pfVar5[2];
      local_14 = local_20;
      local_10 = local_1c;
      fVar7 = FUN_10058390(&local_24);
      if (((float10)_DAT_1007d70c < fVar7) && (0x4f < DAT_100cf978)) {
        param_2[0xc1] = 0;
        param_2[0xbe] = 1;
        goto LAB_10044ac3;
      }
    }
  }
  else {
    pfVar1 = (float *)(param_2 + 0x2d2);
    uVar4 = FUN_10057910(pfVar1,0x250);
    if ((char)uVar4 != '\0') {
      return 0;
    }
    uVar4 = FUN_100570b0(pfVar1);
    if ((char)uVar4 == '\0') {
      return 0;
    }
    pfVar5 = (float *)FUN_10057270(local_c);
    local_18 = *pfVar1 - *pfVar5;
    local_20 = (float)param_2[0x2d3] - pfVar5[1];
    local_1c = (float)param_2[0x2d4] - pfVar5[2];
    local_14 = local_20;
    local_10 = local_1c;
    fVar7 = FUN_10058390(&local_18);
    if (((float10)_DAT_1007d718 < fVar7) && (0x4f < DAT_100cf978)) {
      param_2[0xc1] = 0;
      param_2[0xbe] = 1;
LAB_10044ac3:
      lVar8 = __ftol();
      param_2[0xbf] = (int)lVar8;
    }
  }
  if ((*(byte *)(param_2 + 0xf0) & 8) != 0) {
    if (DAT_1018ca59 == '\0') {
      iVar6 = FUN_10056fc0((float *)(param_2 + 0x2d2));
    }
    else {
      iVar6 = 0;
    }
    param_2[0xc5] = iVar6;
  }
LAB_10044af9:
  if (DAT_10190b18 != 0) {
    FUN_100249f0(&DAT_100cfff8,param_1,param_2);
    if (((DAT_10190b18 == 5) || (DAT_100d000c == 2)) && (*param_2 == DAT_10190b20)) {
      return 0;
    }
  }
  return 1;
}


