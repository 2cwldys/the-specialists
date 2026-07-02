// ===== FUN @ 0009f044 size=496 =====

/* WARNING: Unknown calling convention -- yet parameter storage is locked */
/* TS_util.cpp */

undefined4 save_weapon_value__FPc(char *a0)

{
  char cVar1;
  FILE *__stream;
  char *pcVar2;
  int iVar3;
  int iVar4;
  undefined4 uVar5;
  int iVar6;
  uint uVar7;
  char *pcVar8;
  double in_stack_fffffaa4;
  double dVar9;
  undefined4 uVar10;
  undefined8 uVar11;
  undefined4 uVar12;
  char local_51c [1024];
  char local_11c [268];
  
  (*PTR_g_engfuncs_001778b8->pfnGetGameDir)(local_51c);
  sprintf(local_11c,"%s%s%s",local_51c,&DAT_00109d6a,a0);
  __stream = fopen(local_11c,"w");
  if (__stream == (FILE *)0x0) {
    uVar5 = 0;
  }
  else {
    fseek(__stream,0,0);
    pcVar2 = 
    "//===========================================\n//values are : |weapon id|accuracy|view kick|accuracy kick|damage|cost|\n\n//IMPORTANT:\n//-dont change weapon id!\n//-accuracy cant go under 0.001\n//-if you put an accuracy kick of 0 accuracy will not be scaled\n\n//If you want use different weapon settings use the \'customweapons\' cvar.\n//Example: \'customweapons weapons_custom.txt\'\n//===========================================\n\n"
    ;
    pcVar8 = local_51c;
    for (iVar6 = 0x69; iVar6 != 0; iVar6 = iVar6 + -1) {
      *(undefined4 *)pcVar8 = *(undefined4 *)pcVar2;
      pcVar2 = pcVar2 + 4;
      pcVar8 = pcVar8 + 4;
    }
    *(undefined2 *)pcVar8 = *(undefined2 *)pcVar2;
    pcVar8[2] = pcVar2[2];
    uVar7 = 0xffffffff;
    pcVar2 = local_51c;
    do {
      if (uVar7 == 0) break;
      uVar7 = uVar7 - 1;
      cVar1 = *pcVar2;
      pcVar2 = pcVar2 + 1;
    } while (cVar1 != '\0');
    dVar9 = (double)CONCAT44(1,local_51c);
    fwrite(local_51c,1,~uVar7 - 1,__stream);
    iVar6 = 0;
    do {
      pcVar2 = (char *)get_weapon_info__F10tsweapon_t(iVar6);
      uVar5 = (undefined4)((ulonglong)in_stack_fffffaa4 >> 0x20);
      uVar10 = (undefined4)((ulonglong)dVar9 >> 0x20);
      if (*pcVar2 != '\0') {
        iVar3 = get_weapon_info__F10tsweapon_t(iVar6);
        uVar12 = *(undefined4 *)(iVar3 + 0xd8);
        iVar3 = get_weapon_info__F10tsweapon_t(iVar6);
        iVar3 = get_weapon_info__F10tsweapon_t(iVar6,uVar10,(double)*(float *)(iVar3 + 0x1c),uVar12)
        ;
        dVar9 = (double)*(float *)(iVar3 + 0x34);
        iVar3 = get_weapon_info__F10tsweapon_t(iVar6);
        iVar3 = get_weapon_info__F10tsweapon_t(iVar6,uVar5,(double)*(float *)(iVar3 + 0x30),dVar9);
        in_stack_fffffaa4 = (double)*(float *)(iVar3 + 0x28);
        iVar3 = iVar6;
        iVar4 = get_weapon_info__F10tsweapon_t(iVar6);
        sprintf(local_51c,"//%s\n%i %f %f %f %f %i\n\n",iVar4 + 0x44,iVar3,in_stack_fffffaa4);
        uVar7 = 0xffffffff;
        pcVar2 = local_51c;
        do {
          if (uVar7 == 0) break;
          uVar7 = uVar7 - 1;
          cVar1 = *pcVar2;
          pcVar2 = pcVar2 + 1;
        } while (cVar1 != '\0');
        fwrite(local_51c,1,~uVar7 - 1,__stream);
      }
      iVar6 = iVar6 + 1;
    } while (iVar6 < 0x26);
    sprintf(local_51c,
            "//===========================================\n//Misc game variables\narmor_absorb %f\n"
            ,(double)*(float *)(PTR_TSGameRules_00177300 + 0x588c));
    uVar7 = 0xffffffff;
    pcVar2 = local_51c;
    do {
      if (uVar7 == 0) break;
      uVar7 = uVar7 - 1;
      cVar1 = *pcVar2;
      pcVar2 = pcVar2 + 1;
    } while (cVar1 != '\0');
    fwrite(local_51c,1,~uVar7 - 1,__stream);
    fclose(__stream);
    iVar3 = 0;
    iVar6 = 0;
    uVar11 = 0;
    pcVar2 = (char *)UTIL_VarArgs__FPce("weapons behaviour saved to %s\n");
    UTIL_ClientPrintAll__FiPCcN41(3,pcVar2,(int)uVar11,(int)((ulonglong)uVar11 >> 0x20),iVar6,iVar3)
    ;
    uVar5 = 1;
  }
  return uVar5;
}
