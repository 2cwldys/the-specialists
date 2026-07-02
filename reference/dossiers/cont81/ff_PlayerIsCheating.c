// ===== FUN @ 0009fae0 size=104 =====

/* WARNING: Unknown calling convention -- yet parameter storage is locked */
/* TS_util.cpp */

uint PlayerIsCheating__FP11CBasePlayerPc(CBasePlayer *a0,char *a1)

{
  undefined uVar1;
  uint uVar2;
  undefined3 extraout_var;
  char *__format;
  int *piVar3;
  int local_8;
  
  if (a0->field_0x160 == 0) {
    piVar3 = &local_8;
    __format = "%i";
    uVar1 = (*PTR_g_engfuncs_001778b8->pfnInfoKeyValue)(a1,"cl_lw");
    sscanf((char *)CONCAT31(extraout_var,uVar1),__format,piVar3);
    uVar2 = (uint)(local_8 != 1);
  }
  else {
    uVar2 = (uint)a0 & 0xffffff00;
  }
  return uVar2;
}
