// ===== FUN @ 0009f238 size=228 =====

/* WARNING: Unknown calling convention -- yet parameter storage is locked */
/* TS_util.cpp */

int TSCountPlayers__FPcb(char *a0,bool a1)

{
  int iVar1;
  int iVar2;
  char *__s2;
  int iVar3;
  int iVar4;
  byte *pbVar5;
  byte *pbVar6;
  bool bVar7;
  bool bVar8;
  
  bVar7 = false;
  bVar8 = false;
  iVar4 = 0;
  iVar3 = 1;
  pbVar5 = (byte *)a0;
  pbVar6 = &DAT_00106657;
  do {
    if (iVar3 == 0) break;
    iVar3 = iVar3 + -1;
    bVar7 = *pbVar5 < *pbVar6;
    bVar8 = *pbVar5 == *pbVar6;
    pbVar5 = pbVar5 + 1;
    pbVar6 = pbVar6 + 1;
  } while (bVar8);
  bVar7 = (!bVar7 && !bVar8) != bVar7;
  iVar3 = 1;
  do {
    if ((*PTR_gpGlobals_00177410)->maxClients < iVar3) {
      return iVar4;
    }
    iVar1 = UTIL_PlayerByIndex__Fi(iVar3);
    if (iVar1 != 0) {
      iVar2 = (**(code **)(*(int *)(iVar1 + 0x60) + 200))(iVar1);
      if (iVar2 != 0) {
        if (bVar7) {
          __s2 = (char *)(**(code **)(*(int *)(iVar1 + 0x60) + 0xd0))(iVar1);
          iVar2 = strcasecmp(a0,__s2);
          if (iVar2 != 0) goto LAB_0009f2ca;
        }
        else {
LAB_0009f2ca:
          if (bVar7) goto LAB_0009f300;
        }
        if (a1) {
          iVar2 = (**(code **)(*(int *)(iVar1 + 0x60) + 0xb4))(iVar1);
          if ((iVar2 == 0) && (*(float *)(iVar1 + 0x770) < fc_0)) goto LAB_0009f300;
        }
        iVar4 = iVar4 + 1;
      }
    }
LAB_0009f300:
    iVar3 = iVar3 + 1;
  } while( true );
}
