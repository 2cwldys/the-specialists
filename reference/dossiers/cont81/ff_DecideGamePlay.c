// ===== FUN @ 000767ac size=267 =====

/* WARNING: Unknown calling convention -- yet parameter storage is locked */
/* TS_gamerules.cpp */

int DecideGamePlay__FPci(char *a0,int a1)

{
  int iVar1;
  char local_4c [68];
  
  *(undefined4 *)PTR_g_theOneMode_001770dc = 0;
  *(undefined4 *)PTR_g_lastManStanding_0017733c = 0;
  if (*(int *)PTR_g_gamemode_0017787c < 1) {
    local_4c[0] = '\0';
    strncpy(local_4c,(*PTR_gpGlobals_00177410)->pStringBase + (*PTR_gpGlobals_00177410)->mapname,3);
    if (fc_0 == *(float *)(PTR_mp_theonemode_00177a48 + 0xc)) {
      if (fc_0 != *(float *)(PTR_lastmanstanding_001772dc + 0xc)) {
        *(undefined4 *)PTR_g_lastManStanding_0017733c = 1;
        goto switchD_000767fb_caseD_1;
      }
      if (fc_0 == *(float *)(PTR_teamplay_00177d08 + 0xc)) goto LAB_000768bf;
    }
    else {
      *(undefined4 *)PTR_g_theOneMode_001770dc = 1;
    }
    a1 = 1;
  }
  else {
    if (*(int *)PTR_g_gamemode_0017787c - 1U < 4) {
                    /* WARNING: Could not recover jumptable at 0x000767fb. Too many branches */
                    /* WARNING: Treating indirect jump as call */
      iVar1 = (*(code *)((int)&__DT_PLTGOT -
                        (&switchD_000767fb::switchdataD_00076800)
                        [*(int *)PTR_g_gamemode_0017787c - 1U]))();
      return iVar1;
    }
switchD_000767fb_caseD_1:
    a1 = 0;
  }
LAB_000768bf:
  *(undefined4 *)PTR_g_gamemode_0017787c = 0;
  return a1;
}
