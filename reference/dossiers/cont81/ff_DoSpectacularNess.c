// ===== FUN @ 00076b48 size=139 =====

/* WARNING: Unknown calling convention -- yet parameter storage is locked */
/* TS_gamerules.cpp */

bool DoSpectacularNess__Fv(void)

{
  float fVar1;
  undefined uVar2;
  undefined3 extraout_var;
  
  uVar2 = (*PTR_g_engfuncs_001778b8->pfnRandomLong)(0,100);
  fVar1 = *(float *)(PTR_spectacularness_001771f0 + 0xc);
  if (fVar1 < fc_0) {
    fVar1 = fc_0;
  }
  if (fc_100 < fVar1) {
    fVar1 = fc_100;
  }
  return (int)ROUND(fVar1) != 0 && CONCAT31(extraout_var,uVar2) <= (int)ROUND(fVar1);
}
