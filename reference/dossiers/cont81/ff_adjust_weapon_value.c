// ===== FUN @ 0009eca4 size=921 =====

/* WARNING: Unknown calling convention -- yet parameter storage is locked */
/* TS_util.cpp */

undefined4 adjust_weapon_value__FPc(char *a0)

{
  char cVar1;
  short sVar2;
  float fVar3;
  FILE *__stream;
  char *pcVar4;
  undefined4 uVar5;
  uint uVar6;
  int iVar7;
  int iVar8;
  int a4;
  int a5;
  uint local_63c;
  float local_638;
  undefined4 local_634;
  undefined4 local_630;
  undefined4 local_62c;
  undefined4 local_628;
  float local_624;
  undefined4 local_620;
  char local_61c [256];
  char local_51c [1024];
  char local_11c [268];
  
  (*PTR_g_engfuncs_001778b8->pfnGetGameDir)(local_61c);
  sprintf(local_11c,"%s%s%s",local_61c,&DAT_00109d6a,a0);
  __stream = fopen(local_11c,"rb");
  if (__stream == (FILE *)0x0) {
    uVar5 = 0;
  }
  else {
    while (pcVar4 = fgets(local_51c,0x400,__stream), pcVar4 != (char *)0x0) {
      local_61c[0] = '\0';
      strcpy(local_61c,local_51c);
      uVar6 = 0xffffffff;
      pcVar4 = local_61c;
      do {
        if (uVar6 == 0) break;
        uVar6 = uVar6 - 1;
        cVar1 = *pcVar4;
        pcVar4 = pcVar4 + 1;
      } while (cVar1 != '\0');
      iVar7 = ~uVar6 - 2;
      if (-1 < iVar7) {
        sVar2 = *(short *)(*(int *)PTR___ctype_b_00177cd8 + local_61c[iVar7] * 2);
        while ((-1 < sVar2 && (iVar7 = iVar7 + -1, -1 < iVar7))) {
          sVar2 = *(short *)(*(int *)PTR___ctype_b_00177cd8 + local_61c[iVar7] * 2);
        }
      }
      iVar8 = iVar7 + 1;
      if (iVar8 < 0x100) {
        uVar6 = -iVar8 & 3;
        if (iVar8 < 0x100) {
          if (uVar6 != 0) {
            if (1 < uVar6) {
              if (2 < uVar6) {
                local_61c[iVar7 + 1] = '\0';
                iVar8 = iVar7 + 2;
              }
              local_61c[iVar8] = '\0';
              iVar8 = iVar8 + 1;
            }
            goto LAB_0009eda8;
          }
        }
        else {
LAB_0009eda8:
          local_61c[iVar8] = '\0';
          iVar8 = iVar8 + 1;
          if (0xff < iVar8) goto LAB_0009edd6;
        }
        do {
          local_61c[iVar8] = '\0';
          local_61c[iVar8 + 1] = '\0';
          local_61c[iVar8 + 2] = '\0';
          local_61c[iVar8 + 3] = '\0';
          iVar8 = iVar8 + 4;
        } while (iVar8 < 0x100);
      }
LAB_0009edd6:
      if (local_61c[0] != '\0') {
        pcVar4 = strstr(local_61c,"//");
        if (pcVar4 == (char *)0x0) {
          pcVar4 = strstr(local_61c,"armor_absorb");
          if (pcVar4 == (char *)0x0) {
            pcVar4 = strstr(local_61c,"health_absorb");
            if (pcVar4 == (char *)0x0) {
              iVar7 = sscanf(local_61c," %i %f %f %f %f %i ",&local_63c,&local_638,&local_634,
                             &local_630,&local_62c,&local_628);
              if (iVar7 < 6) {
                (*PTR_g_engfuncs_001778b8->pfnAlertMessage)
                          (1,"You need to run saveweapons again.\n");
                return 1;
              }
              if ((local_63c < 0x26) &&
                 (pcVar4 = (char *)get_weapon_info__F10tsweapon_t(local_63c), *pcVar4 != '\0')) {
                iVar7 = get_weapon_info__F10tsweapon_t(local_63c);
                fVar3 = local_638;
                if (local_638 < fc_0_0010000000474974513) {
                  fVar3 = fc_0_0010000000474974513;
                }
                *(float *)(iVar7 + 0x28) = fVar3;
                iVar7 = get_weapon_info__F10tsweapon_t(local_63c);
                *(undefined4 *)(iVar7 + 0x30) = local_634;
                iVar7 = get_weapon_info__F10tsweapon_t(local_63c);
                *(undefined4 *)(iVar7 + 0x34) = local_630;
                iVar7 = get_weapon_info__F10tsweapon_t(local_63c);
                *(undefined4 *)(iVar7 + 0x1c) = local_62c;
                iVar7 = get_weapon_info__F10tsweapon_t(local_63c);
                *(undefined4 *)(iVar7 + 0xd8) = local_628;
              }
            }
            else {
              iVar7 = sscanf(pcVar4 + 0xd," %f ",&local_624);
              if (iVar7 != 0) {
                fVar3 = local_624;
                if (local_624 < fc_0) {
                  fVar3 = fc_0;
                }
                *(float *)(PTR_TSGameRules_00177300 + 0x5888) = fVar3;
              }
            }
          }
          else {
            iVar7 = sscanf(pcVar4 + 0xc," %f ",&local_620);
            if (iVar7 != 0) {
              fVar3 = local_620;
              if (local_620 < fc_0) {
                fVar3 = fc_0;
              }
              *(float *)(PTR_TSGameRules_00177300 + 0x588c) = fVar3;
            }
          }
        }
      }
    }
    fclose(__stream);
    a5 = 0;
    a4 = 0;
    iVar8 = 0;
    iVar7 = 0;
    pcVar4 = (char *)UTIL_VarArgs__FPce("weapons behaviour changed by %s\n");
    UTIL_ClientPrintAll__FiPCcN41(3,pcVar4,iVar7,iVar8,a4,a5);
    PTR_TSGameRules_00177300[0x5884] = 1;
    uVar5 = 1;
  }
  return uVar5;
}
