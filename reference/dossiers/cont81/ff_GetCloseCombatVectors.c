// ===== FUN @ 00086e2c size=116 =====

/* WARNING: Unknown calling convention -- yet parameter storage is locked */
/* TS_gun.cpp */

void GetCloseCombatVectors__FiR6VectorN31(int a0,Vector *a1,int a2,int a3,int a4)

{
  int iVar1;
  float *pfVar2;
  float local_1c [4];
  
  local_1c[0] = a1->x + fc_32 * *(float *)a3;
  local_1c[1] = a1->y + fc_32 * *(float *)(a3 + 4);
  local_1c[2] = a1->z + fc_32 * *(float *)(a3 + 8);
  pfVar2 = local_1c;
  for (iVar1 = 3; iVar1 != 0; iVar1 = iVar1 + -1) {
    *(float *)a2 = *pfVar2;
    pfVar2 = pfVar2 + 1;
    a2 = (int)(a2 + 4);
  }
  return;
}
