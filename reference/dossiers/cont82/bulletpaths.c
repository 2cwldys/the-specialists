// ===== FUN @ 0006ef5c size=1419 =====

/* WARNING: Unknown calling convention -- yet parameter storage is locked */
/* TS_bullet.cpp
   cvars: dmgreport */

void CBulletManager::UpdateBullet(CBulletManager *this,int a0,TraceResult *a1)

{
  vec_t vVar1;
  float fVar2;
  undefined *puVar3;
  undefined uVar4;
  int iVar5;
  undefined3 extraout_var;
  char *a1_00;
  int iVar6;
  int iVar7;
  TraceResult *pTVar8;
  float *pfVar9;
  Vector *pVVar10;
  float *pfVar11;
  entvars_t **ppeVar12;
  vec_t *pvVar13;
  entvars_t **ppeVar14;
  longdouble lVar15;
  longdouble extraout_ST0;
  int a3;
  int a4;
  int a5;
  float local_78;
  float local_6c;
  Vector local_5c;
  float local_4c [4];
  float local_3c [4];
  float local_2c [7];
  
  local_78 = fc_1;
  ppeVar14 = &this->pev + a0 * 0x10;
  iVar5 = get_ammo_info__Fi((int)ppeVar14[0xb]);
  if (fc_0 == (float)(*ppeVar14)->button) {
    lVar15 = (longdouble)(**(code **)((int)((*ppeVar14)->avelocity).y + 0x10))(*ppeVar14);
    ppeVar14[0xd] = (entvars_t *)(float)lVar15;
  }
  else {
    ppeVar14[0xd] = (entvars_t *)(*ppeVar14)->button;
  }
  lVar15 = (longdouble)
           GetBulletVelocityMult__Ffc
                     ((float)ppeVar14[0xd],*(char *)((int)&((*ppeVar14)->view_ofs).x + 1));
  local_4c[2] = (float)ppeVar14[0xe] - (*PTR_gpGlobals_00177410)->time;
  if (local_4c[2] < fc_0_0010000000474974513) {
    local_4c[2] = (*PTR_gpGlobals_00177410)->frametime;
  }
  fVar2 = fc_40 * (float)*(int *)(iVar5 + 0x24);
  local_5c.x = fVar2 * (float)ppeVar14[7];
  local_5c.y = fVar2 * (float)ppeVar14[8];
  local_5c.z = fVar2 * (float)ppeVar14[9];
  local_4c[0] = local_4c[2] * fVar2 * (float)ppeVar14[7];
  local_4c[1] = local_4c[2] * fVar2 * (float)ppeVar14[8];
  local_4c[2] = local_4c[2] * fVar2 * (float)ppeVar14[9];
  local_3c[0] = (float)(lVar15 * (longdouble)local_4c[0]);
  local_3c[1] = (float)(lVar15 * (longdouble)local_4c[1]);
  local_3c[2] = (float)(lVar15 * (longdouble)local_4c[2]);
  pfVar9 = local_3c;
  pfVar11 = local_2c;
  for (iVar6 = 3; iVar6 != 0; iVar6 = iVar6 + -1) {
    *pfVar11 = *pfVar9;
    pfVar9 = pfVar9 + 1;
    pfVar11 = pfVar11 + 1;
  }
  local_5c.x = (float)ppeVar14[1] + local_2c[0];
  local_5c.y = (float)ppeVar14[2] + local_2c[1];
  local_5c.z = (float)ppeVar14[3] + local_2c[2];
  UTIL_TraceLine_Expanded__FRC6VectorT015IGNORE_MONSTERSP7edict_sP11TraceResult
            (ppeVar14 + 1,&local_5c,0,*(undefined4 *)((*ppeVar14)->classname + 0x208),a1);
  if (*(float *)(a1 + 0x10) == (float)dc_1) {
    local_5c.x = (float)ppeVar14[1] + local_2c[0];
    local_5c.y = (float)ppeVar14[2] + local_2c[1];
    local_5c.z = (float)ppeVar14[3] + local_2c[2];
    pVVar10 = &local_5c;
    for (iVar5 = 3; ppeVar14 = ppeVar14 + 1, iVar5 != 0; iVar5 = iVar5 + -1) {
      *ppeVar14 = (entvars_t *)pVVar10->x;
      pVVar10 = (Vector *)&pVVar10->y;
    }
  }
  else {
    pTVar8 = a1 + 0x14;
    pvVar13 = (vec_t *)(ppeVar14 + 1);
    for (iVar6 = 3; iVar6 != 0; iVar6 = iVar6 + -1) {
      *pvVar13 = *(vec_t *)pTVar8;
      pTVar8 = pTVar8 + 4;
      pvVar13 = pvVar13 + 1;
    }
    iVar6 = *(int *)(a1 + 0x30);
    if (iVar6 == 0) {
      uVar4 = (*PTR_g_engfuncs_001778b8->pfnPEntityOfEntOffset)(0);
      iVar6 = CONCAT31(extraout_var,uVar4);
    }
    iVar7 = 0;
    if (iVar6 != 0) {
      iVar7 = *(int *)(iVar6 + 0x7c);
    }
    if (ppeVar14[0xf] == (entvars_t *)0x2) {
      local_5c.x = (float)ppeVar14[4] - (float)ppeVar14[1];
      local_5c.y = (float)ppeVar14[5] - (float)ppeVar14[2];
      local_5c.z = (float)ppeVar14[6] - (float)ppeVar14[3];
      lVar15 = (longdouble)
               get_range_multipler__Fi
                         ((int)ROUND(SQRT(local_5c.z * local_5c.z +
                                          local_5c.y * local_5c.y + local_5c.x * local_5c.x) *
                                     fc_0_0033333334140479565 * fc_10));
      local_6c = (float)(lVar15 * (longdouble)(float)ppeVar14[10]);
    }
    else {
      local_5c.x = (float)ppeVar14[4] - (float)ppeVar14[1];
      local_5c.y = (float)ppeVar14[5] - (float)ppeVar14[2];
      local_5c.z = (float)ppeVar14[6] - (float)ppeVar14[3];
      lVar15 = (longdouble)
               get_range_multipler__Fi
                         ((int)ROUND(fc_10 * (SQRT(local_5c.z * local_5c.z +
                                                   local_5c.y * local_5c.y + local_5c.x * local_5c.x
                                                  ) / (fc_10 * (float)*(int *)(iVar5 + 0x20)))));
      local_6c = (float)(lVar15 * (longdouble)(float)ppeVar14[10]);
      if ((fc_0 < *(float *)(PTR_damagemult_00177294 + 0xc)) &&
         (fc_0 != *(float *)PTR_g_flWeaponCheat_00177bcc)) {
        local_78 = *(float *)(PTR_damagemult_00177294 + 0xc);
      }
    }
    puVar3 = PTR_glast_weaponid_00177d2c;
    vVar1 = (vec_t)ppeVar14[0xf];
    *(entvars_t **)PTR_glast_weaponid_00177d2c = ppeVar14[0xc];
    ClearMultiDamage__Fv();
    local_5c.x = (vec_t)ppeVar14[7];
    local_5c.y = (vec_t)ppeVar14[8];
    local_5c.z = (vec_t)ppeVar14[9];
    (**(code **)(*(int *)(iVar7 + 0x60) + 0x54))
              (iVar7,(*ppeVar14)->classname,local_6c * local_78,&local_5c,a1,
               ((vVar1 == 1.4013e-45) - 1 & 0xfe000000) + 0x2001002);
    ApplyMultiDamage__FP9entvars_sT0
              ((entvars_s *)(*ppeVar14)->classname,(entvars_s *)(*ppeVar14)->classname);
    *(undefined4 *)puVar3 = 0xffffffff;
    if ((fc_0 != *(float *)PTR_g_flWeaponCheat_00177bcc) &&
       ((*PTR_g_engfuncs_001778b8->pfnCVarGetFloat)("dmgreport"), (longdouble)fc_1 < extraout_ST0))
    {
      a5 = 0;
      a4 = 0;
      a3 = 0;
      iVar6 = 0;
      a1_00 = (char *)UTIL_VarArgs__FPce("damage:%f\n");
      UTIL_ClientPrintAll__FiPCcN41(3,a1_00,iVar6,a3,a4,a5);
    }
    local_6c = local_6c * *(float *)(iVar5 + 0x18);
    iVar5 = (**(code **)(*(int *)(iVar7 + 0x60) + 200))(iVar7);
    if (iVar5 != 0) {
      local_6c = (float)dc_0_5 * local_6c;
    }
    if ((1 < (int)ppeVar14[0xf] - 1U) && ((float)dc_5 < local_6c)) {
      fVar2 = local_6c;
      if (fc_48 < local_6c) {
        fVar2 = fc_48;
      }
      local_3c[0] = fVar2 * (float)ppeVar14[7];
      local_3c[1] = fVar2 * (float)ppeVar14[8];
      local_3c[2] = fVar2 * (float)ppeVar14[9];
      local_4c[0] = *(float *)(a1 + 0x14) + fVar2 * (float)ppeVar14[7];
      local_4c[1] = *(float *)(a1 + 0x18) + fVar2 * (float)ppeVar14[8];
      local_4c[2] = *(float *)(a1 + 0x1c) + fVar2 * (float)ppeVar14[9];
      pfVar9 = local_4c;
      pVVar10 = &local_5c;
      for (iVar5 = 3; iVar5 != 0; iVar5 = iVar5 + -1) {
        pVVar10->x = *pfVar9;
        pfVar9 = pfVar9 + 1;
        pVVar10 = (Vector *)&pVVar10->y;
      }
      iVar5 = UTIL_PointContents__FRC6Vector(&local_5c);
      if (iVar5 != -2) {
        pVVar10 = &local_5c;
        ppeVar12 = ppeVar14;
        for (iVar5 = 3; ppeVar12 = ppeVar12 + 1, iVar5 != 0; iVar5 = iVar5 + -1) {
          *ppeVar12 = (entvars_t *)pVVar10->x;
          pVVar10 = (Vector *)&pVVar10->y;
        }
        pVVar10 = &local_5c;
        pvVar13 = (vec_t *)(ppeVar14 + 4);
        for (iVar5 = 3; iVar5 != 0; iVar5 = iVar5 + -1) {
          *pvVar13 = pVVar10->x;
          pVVar10 = (Vector *)&pVVar10->y;
          pvVar13 = pvVar13 + 1;
        }
        ppeVar14[10] = (entvars_t *)(local_6c * local_78);
        ppeVar14[0xf] = (entvars_t *)0x2;
        return;
      }
    }
    DestroyBullet(this,a0);
  }
  return;
}


// ===== FUN @ 0007f69c size=1 =====

void __regparm3 FUN_0007f69c(undefined4 param_1,undefined4 *param_2)

{
  int iVar1;
  int unaff_EBX;
  undefined4 *unaff_ESI;
  undefined4 *puVar2;
  undefined4 *puVar3;
  undefined4 *puStack00000004;
  undefined4 *puStack00000008;
  undefined4 *puStack0000000c;
  undefined4 *puStack00000010;
  undefined4 *puStack00000014;
  undefined4 *puStack00000018;
  undefined4 *puStack0000001c;
  undefined4 *puStack00000020;
  
  *(int *)(unaff_EBX + 0x548928c2) = *(int *)(unaff_EBX + 0x548928c2) + 1;
  for (iVar1 = 2; iVar1 != 0; iVar1 = iVar1 + -1) {
    *param_2 = *unaff_ESI;
    unaff_ESI = unaff_ESI + 1;
    param_2 = param_2 + 1;
  }
  *(undefined1 *)param_2 = *(undefined1 *)unaff_ESI;
  *(undefined4 *)((int)DWORD_ARRAY_0001009c + unaff_EBX + 0xfc4) = 0x3ecccccd;
  *(undefined4 *)((int)DWORD_ARRAY_0001009c + unaff_EBX + 0xfc8) = 0x3c;
  *(undefined4 *)((int)DWORD_ARRAY_0001009c + unaff_EBX + 0xfcc) = 1;
  *(undefined4 *)((int)DWORD_ARRAY_0001009c + unaff_EBX + 0xfd4) = 0xc;
  *(undefined4 *)((int)DWORD_ARRAY_0001009c + unaff_EBX + 0xfd8) = 0x2000;
  *(undefined4 *)((int)DWORD_ARRAY_0001009c + unaff_EBX + 0xfdc) = 0x3e4ccccd;
  *(undefined4 *)((int)DWORD_ARRAY_0001009c + unaff_EBX + 0xfe4) = 0x5a;
  *(undefined4 *)((int)DWORD_ARRAY_0001009c + unaff_EBX + 0xfe8) = 0x118;
  puStack00000020 = (undefined4 *)((int)DWORD_ARRAY_0001009c + unaff_EBX + 0xfec);
  puVar2 = (undefined4 *)(unaff_EBX + -0x6de00);
  puVar3 = puStack00000020;
  for (iVar1 = 1; iVar1 != 0; iVar1 = iVar1 + -1) {
    *puVar3 = *puVar2;
    puVar2 = puVar2 + 1;
    puVar3 = puVar3 + 1;
  }
  *(undefined2 *)puVar3 = *(undefined2 *)puVar2;
  *(undefined1 *)((int)puVar3 + 2) = *(undefined1 *)((int)puVar2 + 2);
  *(undefined4 *)((int)DWORD_ARRAY_0001009c + unaff_EBX + 0x114c) = 0x3e99999a;
  *(undefined4 *)((int)DWORD_ARRAY_0001009c + unaff_EBX + 0x1150) = 0x46;
  *(undefined4 *)((int)DWORD_ARRAY_0001009c + unaff_EBX + 0x1154) = 1;
  *(undefined4 *)((int)DWORD_ARRAY_0001009c + unaff_EBX + 0x115c) = 0xf;
  *(undefined4 *)((int)DWORD_ARRAY_0001009c + unaff_EBX + 0x1160) = 0x2000;
  *(undefined4 *)((int)DWORD_ARRAY_0001009c + unaff_EBX + 0x1164) = 0x3f000000;
  *(undefined4 *)((int)DWORD_ARRAY_0001009c + unaff_EBX + 0x116c) = 0x5a;
  *(undefined4 *)((int)DWORD_ARRAY_0001009c + unaff_EBX + 0x1170) = 0x1c2;
  puStack0000001c = (undefined4 *)((int)DWORD_ARRAY_0001009c + unaff_EBX + 0x1174);
  puVar2 = (undefined4 *)(unaff_EBX + -0x6ddf9);
  puVar3 = puStack0000001c;
  for (iVar1 = 2; iVar1 != 0; iVar1 = iVar1 + -1) {
    *puVar3 = *puVar2;
    puVar2 = puVar2 + 1;
    puVar3 = puVar3 + 1;
  }
  *(undefined2 *)puVar3 = *(undefined2 *)puVar2;
  *(undefined4 *)((int)DWORD_ARRAY_0001009c + unaff_EBX + 0x1034) = 0x3e99999a;
  *(undefined4 *)((int)DWORD_ARRAY_0001009c + unaff_EBX + 0x1038) = 0x50;
  *(undefined4 *)((int)DWORD_ARRAY_0001009c + unaff_EBX + 0x103c) = 2;
  *(undefined4 *)((int)DWORD_ARRAY_0001009c + unaff_EBX + 0x1044) = 0x1e;
  *(undefined4 *)((int)DWORD_ARRAY_0001009c + unaff_EBX + 0x1048) = 0x2000;
  *(undefined4 *)((int)DWORD_ARRAY_0001009c + unaff_EBX + 0x104c) = 0x3f666666;
  *(undefined4 *)((int)DWORD_ARRAY_0001009c + unaff_EBX + 0x1054) = 0x96;
  *(undefined4 *)((int)DWORD_ARRAY_0001009c + unaff_EBX + 0x1058) = 900;
  puStack00000018 = (undefined4 *)((int)DWORD_ARRAY_0001009c + unaff_EBX + 0x105c);
  puVar2 = (undefined4 *)(unaff_EBX + -0x6ddef);
  puVar3 = puStack00000018;
  for (iVar1 = 2; iVar1 != 0; iVar1 = iVar1 + -1) {
    *puVar3 = *puVar2;
    puVar2 = puVar2 + 1;
    puVar3 = puVar3 + 1;
  }
  *(undefined2 *)puVar3 = *(undefined2 *)puVar2;
  *(undefined4 *)((int)DWORD_ARRAY_0001009c + unaff_EBX + 0x10a4) = 0x3e4ccccd;
  *(undefined4 *)((int)DWORD_ARRAY_0001009c + unaff_EBX + 0x10a8) = 100;
  *(undefined4 *)((int)DWORD_ARRAY_0001009c + unaff_EBX + 0x10ac) = 2;
  *(undefined4 *)((int)DWORD_ARRAY_0001009c + unaff_EBX + 0x10b4) = 0x1e;
  *(undefined4 *)((int)DWORD_ARRAY_0001009c + unaff_EBX + 0x10b8) = 0x2000;
  *(undefined4 *)((int)DWORD_ARRAY_0001009c + unaff_EBX + 0x10bc) = 0x3f333333;
  *(undefined4 *)((int)DWORD_ARRAY_0001009c + unaff_EBX + 0x10c4) = 0xaa;
  *(undefined4 *)((int)DWORD_ARRAY_0001009c + unaff_EBX + 0x10c8) = 0x316;
  puStack00000014 = (undefined4 *)((int)DWORD_ARRAY_0001009c + unaff_EBX + 0x10cc);
  puVar2 = (undefined4 *)(unaff_EBX + -0x6dde5);
  puVar3 = puStack00000014;
  for (iVar1 = 3; iVar1 != 0; iVar1 = iVar1 + -1) {
    *puVar3 = *puVar2;
    puVar2 = puVar2 + 1;
    puVar3 = puVar3 + 1;
  }
  *(undefined4 *)((int)DWORD_ARRAY_0001009c + unaff_EBX + 0x1114) = 0x3dcccccd;
  *(undefined4 *)((int)DWORD_ARRAY_0001009c + unaff_EBX + 0x1118) = 500;
  *(undefined4 *)((int)DWORD_ARRAY_0001009c + unaff_EBX + 0x111c) = 2;
  *(undefined4 *)((int)DWORD_ARRAY_0001009c + unaff_EBX + 0x1124) = 5;
  *(undefined4 *)((int)DWORD_ARRAY_0001009c + unaff_EBX + 0x1128) = 0x2000;
  *(undefined4 *)((int)DWORD_ARRAY_0001009c + unaff_EBX + 0x112c) = 0x3f800000;
  *(undefined4 *)((int)DWORD_ARRAY_0001009c + unaff_EBX + 0x1134) = 200;
  *(undefined4 *)((int)DWORD_ARRAY_0001009c + unaff_EBX + 0x1138) = 1000;
  *(undefined4 *)((int)DWORD_ARRAY_0001009c + unaff_EBX + 0x113c) =
       *(undefined4 *)(unaff_EBX + -0x6ddd9);
  *(undefined2 *)((int)DWORD_ARRAY_0001009c + unaff_EBX + 0x1140) =
       *(undefined2 *)(unaff_EBX + -0x6ddd5);
  *(undefined4 *)((int)DWORD_ARRAY_0001009c + unaff_EBX + 0xffc) = 0x3e99999a;
  *(undefined4 *)((int)DWORD_ARRAY_0001009c + unaff_EBX + 0x1000) = 0x78;
  *(undefined4 *)((int)DWORD_ARRAY_0001009c + unaff_EBX + 0x1004) = 2;
  *(undefined4 *)((int)DWORD_ARRAY_0001009c + unaff_EBX + 0x100c) = 8;
  *(undefined4 *)((int)DWORD_ARRAY_0001009c + unaff_EBX + 0x1010) = 0x2000;
  *(undefined4 *)((int)DWORD_ARRAY_0001009c + unaff_EBX + 0x1014) = 0;
  *(undefined4 *)((int)DWORD_ARRAY_0001009c + unaff_EBX + 0x101c) = 0x5a;
  *(undefined4 *)((int)DWORD_ARRAY_0001009c + unaff_EBX + 0x1020) = 0x15e;
  puStack00000010 = (undefined4 *)((int)DWORD_ARRAY_0001009c + unaff_EBX + 0x1024);
  puVar2 = (undefined4 *)(unaff_EBX + -0x6ddd3);
  puVar3 = puStack00000010;
  for (iVar1 = 1; iVar1 != 0; iVar1 = iVar1 + -1) {
    *puVar3 = *puVar2;
    puVar2 = puVar2 + 1;
    puVar3 = puVar3 + 1;
  }
  *(undefined2 *)puVar3 = *(undefined2 *)puVar2;
  *(undefined1 *)((int)puVar3 + 2) = *(undefined1 *)((int)puVar2 + 2);
  *(undefined4 *)((int)DWORD_ARRAY_0001009c + unaff_EBX + 0x1184) = 0x3f000000;
  *(undefined4 *)((int)DWORD_ARRAY_0001009c + unaff_EBX + 0x1188) = 100;
  *(undefined4 *)((int)DWORD_ARRAY_0001009c + unaff_EBX + 0x118c) = 1;
  *(undefined4 *)((int)DWORD_ARRAY_0001009c + unaff_EBX + 0x1194) = 0x11;
  *(undefined4 *)((int)DWORD_ARRAY_0001009c + unaff_EBX + 0x1198) = 0x2000;
  *(undefined4 *)((int)DWORD_ARRAY_0001009c + unaff_EBX + 0x119c) = 0x3f666666;
  *(undefined4 *)((int)DWORD_ARRAY_0001009c + unaff_EBX + 0x11a4) = 0x46;
  *(undefined4 *)((int)DWORD_ARRAY_0001009c + unaff_EBX + 0x11a8) = 0x15e;
  puStack0000000c = (undefined4 *)((int)DWORD_ARRAY_0001009c + unaff_EBX + 0x11ac);
  puVar2 = (undefined4 *)(unaff_EBX + -0x6ddcc);
  puVar3 = puStack0000000c;
  for (iVar1 = 1; iVar1 != 0; iVar1 = iVar1 + -1) {
    *puVar3 = *puVar2;
    puVar2 = puVar2 + 1;
    puVar3 = puVar3 + 1;
  }
  *(undefined2 *)puVar3 = *(undefined2 *)puVar2;
  *(undefined1 *)((int)puVar3 + 2) = *(undefined1 *)((int)puVar2 + 2);
  *(undefined4 *)((int)DWORD_ARRAY_0001009c + unaff_EBX + 0x11bc) = 0x3e99999a;
  *(undefined4 *)((int)DWORD_ARRAY_0001009c + unaff_EBX + 0x11c0) = 0x96;
  *(undefined4 *)((int)DWORD_ARRAY_0001009c + unaff_EBX + 0x11c4) = 1;
  *(undefined4 *)((int)DWORD_ARRAY_0001009c + unaff_EBX + 0x11cc) = 5;
  *(undefined4 *)((int)DWORD_ARRAY_0001009c + unaff_EBX + 0x11d0) = 0x2000;
  *(undefined4 *)((int)DWORD_ARRAY_0001009c + unaff_EBX + 0x11d4) = 0x3f400000;
  *(undefined4 *)((int)DWORD_ARRAY_0001009c + unaff_EBX + 0x11dc) = 0x14;
  *(undefined4 *)((int)DWORD_ARRAY_0001009c + unaff_EBX + 0x11e0) = 400;
  puStack00000008 = (undefined4 *)((int)DWORD_ARRAY_0001009c + unaff_EBX + 0x11e4);
  puVar2 = (undefined4 *)(unaff_EBX + -0x6ddc5);
  puVar3 = puStack00000008;
  for (iVar1 = 3; iVar1 != 0; iVar1 = iVar1 + -1) {
    *puVar3 = *puVar2;
    puVar2 = puVar2 + 1;
    puVar3 = puVar3 + 1;
  }
  *(undefined4 *)((int)DWORD_ARRAY_0001009c + unaff_EBX + 0x11f4) = 0x3f000000;
  *(undefined4 *)((int)DWORD_ARRAY_0001009c + unaff_EBX + 0x11f8) = 0x1e;
  *(undefined4 *)((int)DWORD_ARRAY_0001009c + unaff_EBX + 0x11fc) = 1;
  *(undefined4 *)((int)DWORD_ARRAY_0001009c + unaff_EBX + 0x1204) = 0x14;
  *(undefined4 *)((int)DWORD_ARRAY_0001009c + unaff_EBX + 0x1208) = 0x2000;
  *(undefined4 *)((int)DWORD_ARRAY_0001009c + unaff_EBX + 0x120c) = 0;
  *(undefined4 *)((int)DWORD_ARRAY_0001009c + unaff_EBX + 0x1214) = 0x28;
  *(undefined4 *)((int)DWORD_ARRAY_0001009c + unaff_EBX + 0x1218) = 300;
  puStack00000004 = (undefined4 *)((int)DWORD_ARRAY_0001009c + unaff_EBX + 0x121c);
  puVar2 = (undefined4 *)(unaff_EBX + -0x6ddb9);
  puVar3 = puStack00000004;
  for (iVar1 = 1; iVar1 != 0; iVar1 = iVar1 + -1) {
    *puVar3 = *puVar2;
    puVar2 = puVar2 + 1;
    puVar3 = puVar3 + 1;
  }
  *(undefined2 *)puVar3 = *(undefined2 *)puVar2;
  *(undefined1 *)((int)puVar3 + 2) = *(undefined1 *)((int)puVar2 + 2);
  *(undefined4 *)((int)DWORD_ARRAY_0001009c + unaff_EBX + 0x122c) = 0x3e4ccccd;
  *(undefined4 *)((int)DWORD_ARRAY_0001009c + unaff_EBX + 0x1230) = 0x96;
  *(undefined4 *)((int)DWORD_ARRAY_0001009c + unaff_EBX + 0x1234) = 2;
  *(undefined4 *)((int)DWORD_ARRAY_0001009c + unaff_EBX + 0x123c) = 5;
  *(undefined4 *)((int)DWORD_ARRAY_0001009c + unaff_EBX + 0x1240) = 0x2000;
  *(undefined4 *)((int)DWORD_ARRAY_0001009c + unaff_EBX + 0x1244) = 0x3f333333;
  *(undefined4 *)((int)DWORD_ARRAY_0001009c + unaff_EBX + 0x124c) = 0xaa;
  *(undefined4 *)((int)DWORD_ARRAY_0001009c + unaff_EBX + 0x1250) = 0x352;
  puVar2 = (undefined4 *)(unaff_EBX + -0x6ddb2);
  puVar3 = (undefined4 *)((int)DWORD_ARRAY_0001009c + unaff_EBX + 0x1254);
  for (iVar1 = 3; iVar1 != 0; iVar1 = iVar1 + -1) {
    *puVar3 = *puVar2;
    puVar2 = puVar2 + 1;
    puVar3 = puVar3 + 1;
  }
  Set_ammo_count__Fi(0);
  return;
}


