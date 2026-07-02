// ===== FUN @ 0007dce8 size=185 =====

/* WARNING: Unknown calling convention -- yet parameter storage is locked */
/* TS_grenades.cpp */

void CTSKnife::WaitTouch(CTSKnife *this,CBaseEntity *a0)

{
  float fVar1;
  float fVar2;
  float fVar3;
  entvars_t *peVar4;
  int iVar5;
  float *pfVar6;
  vec_t *pvVar7;
  Vector *pVVar8;
  float local_2c [7];
  
  peVar4 = this->pev;
  if ((peVar4->flags & 0x200) != 0) {
    local_2c[0] = fc_0_5 * (peVar4->velocity).x;
    local_2c[1] = fc_0_5 * (peVar4->velocity).y;
    local_2c[2] = fc_0_5 * (peVar4->velocity).z;
    pfVar6 = local_2c;
    pVVar8 = &peVar4->velocity;
    for (iVar5 = 3; iVar5 != 0; iVar5 = iVar5 + -1) {
      pVVar8->x = *pfVar6;
      pfVar6 = pfVar6 + 1;
      pVVar8 = (Vector *)&pVVar8->y;
    }
    peVar4 = this->pev;
    fVar1 = (peVar4->velocity).x;
    fVar2 = (peVar4->velocity).y;
    fVar3 = (peVar4->velocity).z;
    if (SQRT(fVar3 * fVar3 + fVar2 * fVar2 + fVar1 * fVar1) < fc_32) {
      (this->pev->angles).x = 0.0;
      (this->pev->angles).z = 0.0;
      pvVar7 = (vec_t *)PTR_g_vecZero_00177698;
      pVVar8 = &this->pev->velocity;
      for (iVar5 = 3; iVar5 != 0; iVar5 = iVar5 + -1) {
        pVVar8->x = *pvVar7;
        pvVar7 = pvVar7 + 1;
        pVVar8 = (Vector *)&pVVar8->y;
      }
    }
  }
  return;
}


