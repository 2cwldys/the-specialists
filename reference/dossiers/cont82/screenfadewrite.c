// ===== FUN @ 000ecc54 size=293 =====

/* WARNING: Unknown calling convention -- yet parameter storage is locked */
/* util.cpp */

void UTIL_ScreenFadeWrite__FRC10ScreenFadeP11CBaseEntity(ScreenFade *a0,CBaseEntity *a1)

{
  enginefuncs_t *peVar1;
  int iVar2;
  longdouble extraout_ST0;
  longdouble extraout_ST0_00;
  
  if (a1 != (CBaseEntity *)0x0) {
    iVar2 = (*a1->vtbl->IsNetClient)(a1);
    peVar1 = PTR_g_engfuncs_001778b8;
    if (iVar2 != 0) {
      (*PTR_g_engfuncs_001778b8->pfnMessageBegin)
                (1,*(undefined4 *)PTR_gmsgTSFade_001775d0,0,a1->pev->pContainingEntity);
      (*a1->vtbl->GiveSlowMul)(a1);
      (*peVar1->pfnWriteShort)((int)ROUND((longdouble)*(ushort *)a0 / extraout_ST0));
      (*a1->vtbl->GiveSlowMul)(a1);
      (*peVar1->pfnWriteShort)((int)ROUND((longdouble)*(ushort *)(a0 + 2) / extraout_ST0_00));
      (*peVar1->pfnWriteShort)((int)*(short *)(a0 + 4));
      (*peVar1->pfnWriteByte)((uint)(byte)a0[6]);
      (*peVar1->pfnWriteByte)((uint)(byte)a0[7]);
      (*peVar1->pfnWriteByte)((uint)(byte)a0[8]);
      (*peVar1->pfnWriteByte)((uint)(byte)a0[9]);
      (*peVar1->pfnMessageEnd)();
    }
  }
  return;
}


