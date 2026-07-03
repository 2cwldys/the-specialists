// ===== FUN @ 10020110 size=34 =====

undefined4 FUN_10020110(undefined4 param_1,undefined4 param_2,undefined4 param_3)

{
  undefined4 uVar1;
  
  if (DAT_100d5cb0 != 0) {
    uVar1 = FUN_10065250(param_1,param_2,param_3);
    return uVar1;
  }
  return 0;
}


// ===== FUN @ 100425b0 size=123 =====

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void __cdecl FUN_100425b0(int param_1)

{
  undefined4 uVar1;
  
  DAT_10189ca0 = *(int *)(param_1 + 0x38);
  DAT_10189e3c = FUN_100071c0(*(int *)(param_1 + 0x38));
  DAT_10189ff4 = *(uint *)(param_1 + 0x3c);
  _DAT_10189eb4 = *(int *)(param_1 + 4);
  uVar1 = FUN_1001c5c0(_DAT_10189eb4);
  if ((char)uVar1 != '\0') {
    DAT_10189c04 = FUN_1003df60((int)DAT_10189e3c);
    _DAT_10189f5c = FUN_10007110(DAT_10189ca0,DAT_10189ff4);
    (**(code **)(DAT_100d5c78 + 0x40))((int)*(short *)(DAT_10189c04 + 4),_DAT_10189f5c);
  }
  return;
}


// ===== FUN @ 1004c840 size=26 =====

void FUN_1004c840(undefined4 param_1,undefined4 param_2,undefined4 param_3)

{
  FUN_1004f990(&DAT_100d0ca0,param_1,param_2,param_3);
  return;
}


// ===== FUN @ 10065250 size=221 =====

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

undefined4 FUN_10065250(undefined4 param_1,undefined4 param_2,undefined4 param_3)

{
  undefined1 uVar1;
  bool bVar2;
  short sVar3;
  uint uVar4;
  int *piVar5;
  undefined3 extraout_var;
  undefined3 extraout_var_00;
  undefined3 extraout_var_01;
  int iVar6;
  
  FUN_100299b0(param_3,param_2);
  uVar4 = FUN_10029a10();
  sVar3 = (short)uVar4;
  if ((sVar3 < 1) || (0x40 < sVar3)) {
    return 1;
  }
  iVar6 = (int)sVar3;
  uVar4 = FUN_10029a10();
  (&DAT_101900ec)[iVar6] = uVar4;
  piVar5 = (int *)(*DAT_100d5bf4)();
  if (*piVar5 != iVar6) goto LAB_10065302;
  DAT_100d4401 = (&DAT_101900ec)[iVar6] == 1;
  if ((bool)DAT_100d4401) {
    bVar2 = CL_IsThirdPerson();
    if (CONCAT31(extraout_var,bVar2) != 0) {
      FUN_1004fe00();
    }
    FUN_10036670(&DAT_100d1460);
  }
  else if (*(int *)(DAT_100df544 + 0xc) == 0x3f800000) {
    bVar2 = CL_IsThirdPerson();
    if (CONCAT31(extraout_var_00,bVar2) != 0) goto LAB_100652da;
LAB_100652f8:
    FUN_1004fe00();
  }
  else {
LAB_100652da:
    if (*(float *)(DAT_100df544 + 0xc) == _DAT_100799f0) {
      bVar2 = CL_IsThirdPerson();
      if (CONCAT31(extraout_var_01,bVar2) != 0) goto LAB_100652f8;
    }
  }
  FUN_10004600();
LAB_10065302:
  uVar1 = *(undefined1 *)(&DAT_101900ec + iVar6);
  *(undefined1 *)((int)&DAT_1018d128 + iVar6 * 2) = 0;
  *(undefined1 *)((int)&DAT_101901f4 + iVar6 * 0x20 + 1) = uVar1;
  return 1;
}


// ===== FUN @ 10007110 size=170 =====

int __cdecl FUN_10007110(int param_1,uint param_2)

{
  short sVar1;
  int iVar2;
  
  iVar2 = 0;
  if ((param_2 & 1) != 0) {
    iVar2 = (int)*(short *)(&DAT_100aa178 + param_1 * 0x3dc);
  }
  if ((((param_2 & 2) == 0) || ((param_2 & 4) == 0)) ||
     (sVar1 = *(short *)(&DAT_100aa180 + param_1 * 0x3dc), sVar1 == -1)) {
    if ((param_2 & 2) != 0) {
      iVar2 = iVar2 + *(short *)(&DAT_100aa17a + param_1 * 0x3dc);
    }
    if ((param_2 & 4) == 0) goto LAB_10007195;
    sVar1 = *(short *)(&DAT_100aa17c + param_1 * 0x3dc);
  }
  iVar2 = iVar2 + sVar1;
LAB_10007195:
  if ((param_2 & 8) == 0) {
    if (param_1 == 0x24) {
      iVar2 = iVar2 + 1;
    }
    return iVar2;
  }
  return iVar2 + *(short *)(&DAT_100aa17e + param_1 * 0x3dc);
}


// ===== FUN @ 1004f990 size=210 =====

void __thiscall FUN_1004f990(void *this,undefined4 param_1,undefined4 param_2,undefined4 param_3)

{
  byte bVar1;
  uint uVar2;
  uint uVar3;
  undefined4 *puVar4;
  uint uVar5;
  
  FUN_100299b0(param_3,param_2);
  uVar2 = FUN_10029a10();
  uVar3 = FUN_10029a10();
  puVar4 = FUN_100071c0(uVar2);
  if ((*(byte *)((int)puVar4 + 0x85) < 0x11) &&
     (puVar4 = FUN_100071c0(uVar2), *(byte *)(puVar4 + 0x21) < 0xb)) {
    uVar5 = uVar2;
    if (uVar3 == 0) {
      uVar5 = 0xffffffff;
    }
    puVar4 = FUN_100071c0(uVar2);
    bVar1 = *(byte *)((int)puVar4 + 0x85);
    puVar4 = FUN_100071c0(uVar2);
    *(uint *)((int)this + ((uint)bVar1 * 0xb + (uint)*(byte *)(puVar4 + 0x21)) * 8 + 0x164) = uVar5;
    uVar3 = FUN_10029a10();
    puVar4 = FUN_100071c0(uVar2);
    bVar1 = *(byte *)((int)puVar4 + 0x85);
    puVar4 = FUN_100071c0(uVar2);
    *(uint *)((int)this + ((uint)bVar1 * 0xb + (uint)*(byte *)(puVar4 + 0x21)) * 8 + 0x168) = uVar3;
  }
  *(uint *)((int)this + 0x10) = *(uint *)((int)this + 0x10) | 1;
  return;
}


// ===== FUN @ 10023380 size=204 =====

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void __thiscall
FUN_10023380(void *this,char param_1,undefined4 param_2,undefined4 param_3,undefined4 param_4)

{
  int iVar1;
  undefined4 *puVar2;
  
  if (param_1 == '\0') {
    DAT_10190a10 = 0;
    DAT_10190b24 = 0;
    DAT_10192154 = 0;
    *(undefined4 *)((int)this + 0x14) = 0;
    DAT_10190b18 = 0;
    *(undefined4 *)((int)this + 0xca4) = 0;
    *(undefined4 *)((int)this + 0xca0) = 0;
    *(undefined4 *)((int)this + 0x948) = 0;
    *(undefined4 *)((int)this + 0xc94) = 0;
    puVar2 = (undefined4 *)((int)this + 0x148);
    for (iVar1 = 0x200; iVar1 != 0; iVar1 = iVar1 + -1) {
      *puVar2 = 0;
      puVar2 = puVar2 + 1;
    }
    FUN_100234d0(this,2,0);
    CHudSpectator::ParseOverviewFile(this);
    FUN_10023c60((int)this);
    FUN_10022da0();
    return;
  }
  if (param_1 != '\x02') {
    (*DAT_100d5bcc)(s_CHudSpectator__DirectorEvent__un_10093c80);
    return;
  }
  if (DAT_10190b18 == 4) {
    DAT_10190b20 = param_2;
    *(undefined4 *)((int)this + 0x948) = param_2;
    DAT_10190b1c = param_3;
  }
  *(undefined4 *)((int)this + 0xca0) = param_2;
  *(undefined4 *)((int)this + 0xca4) = param_3;
  _DAT_10192158 = param_4;
  return;
}


// ===== FUN @ 100234d0 size=510 =====

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void __thiscall FUN_100234d0(void *this,int param_1,int param_2)

{
  char cVar1;
  int *piVar2;
  undefined4 uVar3;
  int iVar4;
  char *pcVar5;
  undefined4 uVar6;
  undefined4 uVar7;
  undefined4 uVar8;
  undefined4 uVar9;
  undefined4 uVar10;
  undefined4 uVar11;
  char local_20 [32];
  
  if ((param_1 < 1) || (5 < param_1)) {
    (*DAT_100d5bc8)(s_Invalid_spectator_mode__10093cc4);
    return;
  }
  if (param_2 != *(int *)((int)this + 0x14)) {
    *(int *)((int)this + 0x14) = param_2;
    FUN_1006ac7a(local_20,(byte *)s__c_Spec_Mode_Inset_d_10093cf0);
    iVar4 = -1;
    pcVar5 = local_20;
    do {
      if (iVar4 == 0) break;
      iVar4 = iVar4 + -1;
      cVar1 = *pcVar5;
      pcVar5 = pcVar5 + 1;
    } while (cVar1 != '\0');
    CHudTrain::Init(&DAT_100cff0c);
  }
  if (param_1 == DAT_10190b18) goto LAB_100236a3;
  if (param_1 == 2) {
    DAT_10190b18 = 4;
  }
  else if (param_1 == 3) {
    DAT_10190b18 = 3;
    *(undefined4 *)((int)this + 0x948) = 0;
    DAT_10190b20 = 0;
    DAT_10190b1c = 0;
  }
  else if (param_1 == 5) {
    DAT_10190b18 = 5;
  }
  FUN_1006ac7a(local_20,(byte *)s__c_Spec_Mode_d_10093ce0);
  iVar4 = -1;
  pcVar5 = local_20;
  do {
    if (iVar4 == 0) break;
    iVar4 = iVar4 + -1;
    cVar1 = *pcVar5;
    pcVar5 = pcVar5 + 1;
  } while (cVar1 != '\0');
  CHudTrain::Init(&DAT_100cff0c);
  if (DAT_10190b18 == 4) {
    if (*(int *)((int)this + 0x948) == 0) {
      piVar2 = (int *)(*DAT_100d5bfc)(*(undefined4 *)((int)this + 0xca0));
      uVar3 = FUN_100236d0(piVar2);
      if ((char)uVar3 != '\0') {
        DAT_10190b20 = *(undefined4 *)((int)this + 0xca0);
        *(undefined4 *)((int)this + 0x948) = DAT_10190b20;
        DAT_10190b1c = *(undefined4 *)((int)this + 0xca4);
      }
      goto LAB_1002360e;
    }
LAB_1002361d:
    uVar11 = 0;
    uVar10 = 0;
    uVar9 = 0;
    _DAT_100df2c4 = 0;
    _DAT_100df2b8 = 0;
    _DAT_100df2bc = 0;
    uVar6 = 0;
    uVar3 = 0;
    uVar7 = 0;
    uVar8 = 0;
  }
  else {
LAB_1002360e:
    if ((DAT_10190b18 != 3) && (DAT_10190b18 != 5)) goto LAB_1002361d;
    uVar11 = 0xff;
    uVar10 = 0xff;
    uVar9 = 0xff;
    _DAT_100df2b8 = 0x18;
    uVar6 = 0x18;
    _DAT_100df2bc = 0x30;
    _DAT_100df2c4 = 0x18;
    uVar7 = 0x30;
    uVar3 = *(undefined4 *)((int)this + 0xc8c);
    uVar8 = 0x18;
  }
  _DAT_100df2c0 = 0;
  (*DAT_100d5b5c)(uVar3,uVar6,uVar7,0,uVar8,uVar9,uVar10,uVar11);
LAB_100236a3:
  FUN_10063870(DAT_100d5cb0);
  return;
}


// ===== FUN @ 10023450 size=67 =====

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void __thiscall FUN_10023450(void *this,byte param_1)

{
  float10 fVar1;
  
  fVar1 = (float10)(*DAT_100d5c00)();
  if (((float10)*(float *)((int)this + 0xc94) <= fVar1) &&
     (*(float *)((int)this + 0xc94) = (float)(fVar1 + (float10)_DAT_1007a8c8), (param_1 & 1) != 0))
  {
    (*DAT_100d5b74)(s_respawn_10093cbc);
    return;
  }
  return;
}


