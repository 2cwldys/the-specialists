// ===== FUN @ 000974f4 size=919 =====

/* WARNING: Restarted to delay deadcode elimination for space: stack */
/* WARNING: Unknown calling convention -- yet parameter storage is locked */
/* TS_powerup.cpp */

Vector * GetRandomPosition__FR6Vector(Vector *a0)

{
  double dVar1;
  float fVar2;
  undefined uVar3;
  undefined3 extraout_var;
  undefined3 extraout_var_00;
  int iVar4;
  float *pfVar5;
  float *pfVar6;
  undefined2 in_FPUControlWord;
  longdouble lVar7;
  float *in_stack_00000008;
  int local_b8;
  int local_b4;
  Vector local_ac;
  Vector local_9c;
  float local_8c [5];
  float local_78;
  undefined2 local_70;
  undefined2 uStack_6e;
  float local_6c [4];
  undefined1 local_5c [16];
  float local_4c;
  float local_48;
  float local_44;
  float local_40;
  
  local_6c[0] = fc_0;
  dVar1 = dc_360 / (double)DAT_00109760;
  _local_70 = CONCAT22(uStack_6e,in_FPUControlWord);
  local_6c[1] = local_6c[0];
  local_6c[2] = local_6c[0];
  uVar3 = (*PTR_g_engfuncs_001778b8->pfnRandomLong)(0,0x10);
  local_b4 = CONCAT31(extraout_var,uVar3) * (int)ROUND(dVar1);
  local_b8 = 1;
  do {
    uVar3 = (*PTR_g_engfuncs_001778b8->pfnRandomLong)(0x80,0x400);
    lVar7 = (longdouble)fcos((longdouble)dc_0_017453292519943295 * (longdouble)local_b4);
    local_6c[0] = (float)lVar7;
    lVar7 = (longdouble)fsin((longdouble)dc_0_017453292519943295 * (longdouble)local_b4);
    local_6c[1] = (float)lVar7;
    local_78 = SQRT(local_6c[2] * local_6c[2] +
                    local_6c[1] * local_6c[1] + local_6c[0] * local_6c[0]);
    if (local_78 == fc_0) {
      local_8c[0] = 0.0;
      local_8c[1] = 0.0;
      local_8c[2] = fc_1;
    }
    else {
      fVar2 = fc_1 / local_78;
      local_8c[0] = fVar2 * local_6c[0];
      local_8c[1] = fVar2 * local_6c[1];
      local_8c[2] = fVar2 * local_6c[2];
    }
    pfVar5 = local_8c;
    pfVar6 = local_6c;
    for (iVar4 = 3; iVar4 != 0; iVar4 = iVar4 + -1) {
      *pfVar6 = *pfVar5;
      pfVar5 = pfVar5 + 1;
      pfVar6 = pfVar6 + 1;
    }
    local_9c.z = (vec_t)CONCAT31(extraout_var_00,uVar3);
    local_9c.x = local_9c.z * local_6c[0];
    local_9c.y = local_9c.z * local_6c[1];
    local_9c.z = local_9c.z * local_6c[2];
    local_8c[0] = *in_stack_00000008 + local_9c.x;
    local_8c[1] = in_stack_00000008[1] + local_9c.y;
    local_8c[2] = in_stack_00000008[2] + local_9c.z;
    UTIL_TraceLine__FRC6VectorT015IGNORE_MONSTERSP7edict_sP11TraceResult
              (in_stack_00000008,local_8c,1,0,local_5c);
    fVar2 = fc_64;
    if ((float)dc_1 <= local_4c) {
      local_9c.x = fc_64 * local_6c[0];
      local_9c.y = fc_64 * local_6c[1];
      local_9c.z = fc_64 * local_6c[2];
      local_ac.x = local_48 - local_9c.x;
      local_ac.y = local_44 - local_9c.y;
      local_ac.z = local_40 - local_9c.z;
      iVar4 = UTIL_PointContents__FRC6Vector(&local_ac);
      if (iVar4 != -2) {
        local_48 = local_48 - fVar2 * local_6c[0];
        local_44 = local_44 - fVar2 * local_6c[1];
        local_40 = local_40 - fVar2 * local_6c[2];
        goto LAB_00097845;
      }
    }
    else {
      local_8c[0] = fc_64 * local_6c[0];
      local_8c[1] = fc_64 * local_6c[1];
      local_8c[2] = fc_64 * local_6c[2];
      local_9c.x = local_48 - local_8c[0];
      local_9c.y = local_44 - local_8c[1];
      local_9c.z = local_40 - local_8c[2];
      iVar4 = UTIL_PointContents__FRC6Vector(&local_9c);
      if (iVar4 != -2) {
        local_48 = local_48 - fVar2 * local_6c[0];
        local_44 = local_44 - fVar2 * local_6c[1];
        local_40 = local_40 - fVar2 * local_6c[2];
LAB_00097845:
        a0->x = local_48;
        a0->y = local_44;
        a0->z = local_40;
        return a0;
      }
    }
    local_b4 = local_b4 + (int)ROUND(dVar1);
    local_b8 = local_b8 + 1;
    if (0x10 < local_b8) {
      a0->x = *in_stack_00000008;
      a0->y = in_stack_00000008[1];
      a0->z = in_stack_00000008[2];
      return a0;
    }
  } while( true );
}
