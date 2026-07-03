// ===== FUN @ 000c60cc size=228 =====

CHalfLifeTeamplay * FUN_000c60cc(void)

{
  int iVar1;
  CHalfLifeTeamplay *this;
  int unaff_EBX;
  longdouble in_ST0;
  int iStack00000014;
  
  iStack00000014 = (int)ROUND(in_ST0);
  iVar1 = DecideGamePlay__FPci
                    ((char *)(*(int *)(**(int **)(unaff_EBX + 0x1600) + 0x98) +
                             *(int *)(**(int **)(unaff_EBX + 0x1600) + 0xc)),iStack00000014);
  **(int **)(unaff_EBX + 0x210c) = iVar1;
  if (iVar1 < 1) {
    iStack00000014 = (int)ROUND(*(float *)(**(int **)(unaff_EBX + 0x1600) + 0x14));
    if (iStack00000014 == 1) {
      **(undefined4 **)(unaff_EBX + 0x210c) = 0;
      this = (CHalfLifeTeamplay *)__builtin_new(0xc);
      CHalfLifeMultiplay::CHalfLifeMultiplay((CHalfLifeMultiplay *)this);
    }
    else {
      **(undefined4 **)(unaff_EBX + 0x210c) = 0;
      this = (CHalfLifeTeamplay *)__builtin_new(0xc);
      CHalfLifeMultiplay::CHalfLifeMultiplay((CHalfLifeMultiplay *)this);
    }
  }
  else {
    this = (CHalfLifeTeamplay *)__builtin_new(0x218);
    CHalfLifeTeamplay::CHalfLifeTeamplay(this);
  }
  return this;
}


