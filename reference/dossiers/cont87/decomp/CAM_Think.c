// ===== FUN @ 10024c80 size=1489 =====

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void CAM_Think(void)

{
  float fVar1;
  float fVar2;
  int iVar3;
  int iVar4;
  float10 fVar5;
  longlong lVar6;
  float local_28;
  float local_18;
  float local_14;
  float fStack_c;
  float fStack_8;
  
                    /* 0x24c80  464  CAM_Think */
  lVar6 = __ftol();
  if ((int)lVar6 == 1) {
    FUN_1004fe00();
  }
  else if ((int)lVar6 == 2) {
    FUN_10025320();
  }
  if (DAT_100df390 != 0) {
    local_18 = *(float *)(DAT_100df364 + 0xc);
    local_14 = *(float *)(DAT_100df2f4 + 0xc);
    local_28 = *(float *)(DAT_100df300 + 0xc);
    if ((DAT_100df2dc != 0) && (GetCursorPos((LPPOINT)&DAT_100df308), DAT_100df354 == 0)) {
      iVar3 = (*DAT_100d5ba8)();
      if (iVar3 < DAT_100df308) {
        if (local_14 < *(float *)(DAT_100df310 + 0xc)) {
          iVar3 = (*DAT_100d5ba8)();
          local_14 = (float)((DAT_100df308 - iVar3) / 2) * (float)_DAT_10079e90 + local_14;
        }
        if (*(float *)(DAT_100df310 + 0xc) < local_14) {
          local_14 = *(float *)(DAT_100df310 + 0xc);
        }
      }
      else {
        iVar3 = (*DAT_100d5ba8)();
        if (DAT_100df308 < iVar3) {
          if (*(float *)(DAT_100df2d0 + 0xc) < local_14) {
            iVar3 = (*DAT_100d5ba8)();
            local_14 = local_14 - (float)((iVar3 - DAT_100df308) / 2) * (float)_DAT_10079e90;
          }
          if (local_14 < *(float *)(DAT_100df2d0 + 0xc)) {
            local_14 = *(float *)(DAT_100df2d0 + 0xc);
          }
        }
      }
      iVar3 = (*DAT_100d5bac)();
      if (iVar3 < DAT_100df30c) {
        if (local_18 < *(float *)(DAT_100df324 + 0xc)) {
          iVar3 = (*DAT_100d5bac)();
          local_18 = (float)((DAT_100df30c - iVar3) / 2) * (float)_DAT_10079e90 + local_18;
        }
        if (*(float *)(DAT_100df324 + 0xc) < local_18) {
          local_18 = *(float *)(DAT_100df324 + 0xc);
        }
      }
      else {
        iVar3 = (*DAT_100d5bac)();
        if (DAT_100df30c < iVar3) {
          if (*(float *)(DAT_100df2fc + 0xc) < local_18) {
            iVar3 = (*DAT_100d5bac)();
            local_18 = local_18 - (float)((iVar3 - DAT_100df30c) / 2) * (float)_DAT_10079e90;
          }
          if (local_18 < *(float *)(DAT_100df2fc + 0xc)) {
            local_18 = *(float *)(DAT_100df2fc + 0xc);
          }
        }
      }
      fVar5 = FUN_10020a90(0x100cf920);
      if (fVar5 == (float10)_DAT_100799f0) {
        _DAT_100df2e4 = DAT_100df308;
        _DAT_100df2e0 = DAT_100df30c;
      }
      else {
        lVar6 = __ftol();
        _DAT_100df2e4 = (int)lVar6;
        lVar6 = __ftol();
        _DAT_100df2e0 = (int)lVar6;
      }
      iVar3 = (*DAT_100d5bac)();
      iVar4 = (*DAT_100d5ba8)();
      SetCursorPos(iVar4,iVar3);
    }
    fVar5 = FUN_100262b0(0x100df338);
    if (fVar5 == (float10)_DAT_100799f0) {
      fVar5 = FUN_100262b0(0x100df370);
      if (fVar5 != (float10)_DAT_100799f0) {
        local_18 = local_18 - (float)_DAT_10079e78;
      }
    }
    else {
      local_18 = local_18 + (float)_DAT_10079e78;
    }
    fVar5 = FUN_100262b0(0x100df2e8);
    if (fVar5 == (float10)_DAT_100799f0) {
      fVar5 = FUN_100262b0(0x100df348);
      if (fVar5 != (float10)_DAT_100799f0) {
        local_14 = local_14 + (float)_DAT_10079e78;
      }
    }
    else {
      local_14 = local_14 - (float)_DAT_10079e78;
    }
    fVar5 = FUN_100262b0(0x100df380);
    if (fVar5 == (float10)_DAT_100799f0) {
      fVar5 = FUN_100262b0(0x100df328);
      if (fVar5 != (float10)_DAT_100799f0) {
        local_28 = local_28 + (float)_DAT_10079e80;
      }
    }
    else {
      local_28 = local_28 - (float)_DAT_10079e80;
      if (local_28 < (float)_DAT_1007a930) {
        local_18 = 0.0;
        local_14 = 0.0;
        local_28 = 30.0;
      }
    }
    if (DAT_100df354 != 0) {
      iVar3 = (*DAT_100d5bac)();
      if (iVar3 < DAT_100df30c) {
        if (local_28 < *(float *)(DAT_100df2f8 + 0xc)) {
          iVar3 = (*DAT_100d5bac)();
          local_28 = (float)((DAT_100df30c - iVar3) / 2) + local_28;
        }
        if (*(float *)(DAT_100df2f8 + 0xc) < local_28) {
          local_28 = *(float *)(DAT_100df2f8 + 0xc);
        }
      }
      else {
        iVar3 = (*DAT_100d5bac)();
        if (DAT_100df30c < iVar3) {
          if (*(float *)(DAT_100df38c + 0xc) < local_28) {
            iVar3 = (*DAT_100d5bac)();
            local_28 = local_28 - (float)((iVar3 - DAT_100df30c) / 2);
          }
          if (local_28 < *(float *)(DAT_100df38c + 0xc)) {
            local_28 = *(float *)(DAT_100df38c + 0xc);
          }
        }
      }
      FUN_10020a90(0x100cf920);
      lVar6 = __ftol();
      _DAT_100df2e4 = (int)lVar6;
      FUN_10020a90(0x100cf920);
      lVar6 = __ftol();
      _DAT_100df2e0 = (int)lVar6;
      iVar3 = (*DAT_100d5bac)();
      iVar4 = (*DAT_100d5ba8)();
      SetCursorPos(iVar4,iVar3);
    }
    *(float *)(DAT_100df364 + 0xc) = local_18;
    *(float *)(DAT_100df2f4 + 0xc) = local_14;
    *(float *)(DAT_100df300 + 0xc) = local_28;
    fVar2 = DAT_100df31c;
    fVar1 = DAT_100df318;
    local_14 = DAT_100df31c;
    (*DAT_100d5bb0)(&fStack_c);
    if (*(float *)(DAT_100df2d8 + 0xc) == _DAT_100799f0) {
      if (fVar2 - fStack_8 != *(float *)(DAT_100df2f4 + 0xc)) {
        fVar5 = FUN_10024ba0(fVar2,fStack_8 + *(float *)(DAT_100df2f4 + 0xc));
        local_14 = (float)fVar5;
      }
      fVar5 = (float10)fVar1;
      if (fVar5 - (float10)fStack_c != (float10)*(float *)(DAT_100df364 + 0xc)) {
        fVar5 = FUN_10024ba0(fVar1,fStack_c + *(float *)(DAT_100df364 + 0xc));
      }
    }
    else {
      local_14 = fStack_8 + *(float *)(DAT_100df2f4 + 0xc);
      fVar5 = (float10)fStack_c + (float10)*(float *)(DAT_100df364 + 0xc);
    }
    DAT_100df318 = (float)fVar5;
    DAT_100df31c = local_14;
    DAT_100df320 = local_28;
  }
  return;
}


