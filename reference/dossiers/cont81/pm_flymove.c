// ===== FUN @ 000fd408 size=378 =====

undefined4 PM_FlyMove(void)

{
  float fVar1;
  float fVar2;
  float fVar3;
  code *pcVar4;
  int iVar5;
  undefined *puVar6;
  undefined *puVar7;
  undefined4 uVar8;
  uint uVar9;
  int iVar10;
  int iVar11;
  int iVar12;
  int iVar13;
  undefined *puVar14;
  undefined1 *puVar15;
  undefined4 *puVar16;
  undefined1 *puVar17;
  undefined1 *puVar18;
  undefined4 *puVar19;
  undefined1 *puVar20;
  undefined1 *puVar21;
  undefined4 auStack_194 [13];
  int aiStack_160 [5];
  float fStack_14c;
  int aiStack_148 [3];
  undefined1 auStack_13c [24];
  int aiStack_124 [4];
  float local_114 [3];
  int local_108;
  float local_104 [5];
  int aiStack_f0 [4];
  float afStack_e0 [16];
  float afStack_a0 [28];
  float afStack_30 [8];
  
  local_114[0] = 0.0;
  local_108 = 0;
  iVar10 = *(int *)PTR_pmove_001772c0;
  afStack_a0[5] = (float)*(undefined4 *)(iVar10 + 0x5c);
  afStack_a0[6] = (float)*(undefined4 *)(iVar10 + 0x60);
  afStack_a0[7] = (float)*(undefined4 *)(iVar10 + 100);
  afStack_a0[9] = (float)*(undefined4 *)(iVar10 + 0x5c);
  afStack_a0[10] = (float)*(undefined4 *)(iVar10 + 0x60);
  afStack_a0[0xb] = (float)*(undefined4 *)(iVar10 + 100);
  local_114[1] = fc_0;
  local_114[2] = (float)*(undefined4 *)(iVar10 + 0x10);
  local_104[0] = 0.0;
  puVar15 = auStack_13c;
  while( true ) {
    puVar6 = PTR_pmove_001772c0;
    if (((fc_0 == *(float *)(iVar10 + 0x5c)) && (fc_0 == *(float *)(iVar10 + 0x60))) &&
       (puVar17 = puVar15, fc_0 == *(float *)(iVar10 + 100))) goto LAB_000fdc7f;
    iVar10 = *(int *)PTR_pmove_001772c0;
    *(float *)(puVar15 + 0x40) =
         *(float *)(puVar15 + 0x30) * *(float *)(iVar10 + 0x5c) + *(float *)(iVar10 + 0x38);
    *(float *)(puVar15 + 0x44) =
         *(float *)(puVar15 + 0x30) * *(float *)(iVar10 + 0x60) + *(float *)(iVar10 + 0x3c);
    *(float *)(puVar15 + 0x48) =
         *(float *)(puVar15 + 0x30) * *(float *)(iVar10 + 100) + *(float *)(iVar10 + 0x40);
    *(undefined4 *)(puVar15 + -0x10) = 0xffffffff;
    *(undefined4 *)(puVar15 + -0x14) = 0;
    *(undefined1 **)(puVar15 + -0x18) = puVar15 + 0x40;
    *(int *)(puVar15 + -0x1c) = iVar10 + 0x38;
    *(undefined1 **)(puVar15 + -0x20) = puVar15 + 0x50;
    pcVar4 = *(code **)((int)&__DT_REL[0x1e0d].r_offset + iVar10);
    *(undefined4 *)(puVar15 + -0x24) = 0xfd4e5;
    (*pcVar4)();
    puVar7 = PTR_vec3_origin_00177cf4;
    puVar14 = PTR_pmove_001772c0;
    *(float *)(puVar15 + 0x28) = *(float *)(puVar15 + 0x28) + *(float *)(puVar15 + 0x5c);
    if (*(int *)(puVar15 + 0x4c) != 0) {
      *(undefined4 *)(*(int *)puVar6 + 0x5c) = *(undefined4 *)PTR_vec3_origin_00177cf4;
      *(undefined4 *)(*(int *)puVar6 + 0x60) = *(undefined4 *)(puVar7 + 4);
      *(undefined4 *)(*(int *)puVar6 + 100) = *(undefined4 *)(puVar7 + 8);
      return 4;
    }
    if (fc_0 < *(float *)(puVar15 + 0x5c)) {
      *(undefined4 *)(*(int *)PTR_pmove_001772c0 + 0x38) = *(undefined4 *)(puVar15 + 0x60);
      *(undefined4 *)(*(int *)puVar14 + 0x3c) = *(undefined4 *)(puVar15 + 100);
      *(undefined4 *)(*(int *)puVar14 + 0x40) = *(undefined4 *)(puVar15 + 0x68);
      iVar10 = *(int *)puVar14;
      *(undefined4 *)(puVar15 + 0xac) = *(undefined4 *)(iVar10 + 0x5c);
      *(undefined4 *)(puVar15 + 0xb0) = *(undefined4 *)(iVar10 + 0x60);
      *(undefined4 *)(puVar15 + 0xb4) = *(undefined4 *)(iVar10 + 100);
      *(undefined4 *)(puVar15 + 0x30) = 0;
    }
    puVar17 = puVar15 + -4;
    if (fc_1 == *(float *)(puVar15 + 0x5c)) goto LAB_000fdc7f;
    *(int *)(puVar15 + -0x10) = *(int *)PTR_pmove_001772c0 + 0x5c;
    puVar16 = (undefined4 *)(puVar15 + 0x4c);
    puVar19 = (undefined4 *)(puVar15 + -0x54);
    for (iVar10 = 0x11; iVar10 != 0; iVar10 = iVar10 + -1) {
      *puVar19 = *puVar16;
      puVar16 = puVar16 + 1;
      puVar19 = puVar19 + 1;
    }
    *(undefined4 *)(puVar15 + -0x58) = 0xfd5b9;
    PM_AddToTouched();
    puVar17 = puVar15 + -4;
    if ((float)dc_0_7 < *(float *)(puVar15 + 0x74)) {
      *(uint *)(puVar15 + 0x24) = *(uint *)(puVar15 + 0x24) | 1;
    }
    if (fc_0 == *(float *)(puVar15 + 0x74)) {
      *(uint *)(puVar15 + 0x24) = *(uint *)(puVar15 + 0x24) | 2;
    }
    *(float *)(puVar15 + 0x2c) =
         *(float *)(puVar15 + 0x2c) - *(float *)(puVar15 + 0x2c) * *(float *)(puVar15 + 0x5c);
    if (4 < *(int *)(puVar15 + 0x30)) break;
    iVar10 = *(int *)(puVar15 + 0x30);
    iVar12 = iVar10 * 0xc;
    *(undefined4 *)(puVar15 + iVar12 + 0xcc) = *(undefined4 *)(puVar15 + 0x6c);
    *(undefined4 *)(puVar15 + iVar12 + 0xd0) = *(undefined4 *)(puVar15 + 0x70);
    *(undefined4 *)(puVar15 + iVar12 + 0xd4) = *(undefined4 *)(puVar15 + 0x74);
    *(int *)(puVar15 + 0x30) = iVar10 + 1;
    iVar10 = *(int *)PTR_pmove_001772c0;
    if ((*(int *)(iVar10 + 0xdc) == 3) &&
       ((*(int *)(iVar10 + 0xe0) == -1 || (fc_1 != *(float *)(iVar10 + 0xc4))))) {
      iVar10 = 0;
      if (*(int *)(puVar15 + 0x30) < 1) goto LAB_000fd8ce;
      puVar17 = puVar15 + 0xcc;
      *(undefined1 **)(puVar15 + 0x18) = puVar17;
      iVar12 = 0;
      if ((*(int *)(puVar15 + 0x30) < 1) || ((*(uint *)(puVar15 + 0x30) & 1) != 0)) {
        if (*(float *)(puVar15 + 0xd4) <= (float)dc_0_7) {
          *(float *)(puVar15 + 0x38) =
               (float)dc_0_01 * (fc_1 - *(float *)(*(int *)PTR_pmove_001772c0 + 0xc4)) + (float)dc_1
          ;
          *(undefined4 *)(puVar15 + -8) = *(undefined4 *)(puVar15 + 0x38);
          *(undefined1 **)(puVar15 + -0xc) = puVar15 + 0x9c;
          *(undefined1 **)(puVar15 + -0x10) = puVar17;
          *(undefined1 **)(puVar15 + -0x14) = puVar15 + 0xac;
          *(undefined4 *)(puVar15 + -0x18) = 0xfd743;
          PM_ClipVelocity();
        }
        else {
          *(undefined4 *)(puVar15 + -8) = 0x3f800000;
          *(undefined1 **)(puVar15 + -0xc) = puVar15 + 0x9c;
          *(undefined4 *)(puVar15 + -0x10) = *(undefined4 *)(puVar15 + 0x18);
          *(undefined1 **)(puVar15 + -0x14) = puVar15 + 0xac;
          *(undefined4 *)(puVar15 + -0x18) = 0xfd6cf;
          PM_ClipVelocity();
          *(undefined4 *)(puVar15 + 0xac) = *(undefined4 *)(puVar15 + 0x9c);
          *(undefined4 *)(puVar15 + 0xb0) = *(undefined4 *)(puVar15 + 0xa0);
          *(undefined4 *)(puVar15 + 0xb4) = *(undefined4 *)(puVar15 + 0xa4);
        }
        puVar17 = puVar15 + 0xd8;
        *(int *)(puVar15 + 0x18) = *(int *)(puVar15 + 0x18) + 0xc;
        iVar12 = 0xc;
        iVar10 = 1;
        if (*(int *)(puVar15 + 0x30) < 2) goto LAB_000fd8ce;
      }
      puVar20 = puVar15 + iVar12 + 0x11c;
      do {
        if (*(float *)(puVar20 + -0x48) <= (float)dc_0_7) {
          *(float *)(puVar15 + 0x38) =
               (float)dc_0_01 * (fc_1 - *(float *)(*(int *)PTR_pmove_001772c0 + 0xc4)) + (float)dc_1
          ;
          *(undefined4 *)(puVar15 + -8) = *(undefined4 *)(puVar15 + 0x38);
          *(undefined1 **)(puVar15 + -0xc) = puVar15 + 0x9c;
          *(undefined1 **)(puVar15 + -0x10) = puVar17;
          *(undefined1 **)(puVar15 + -0x14) = puVar15 + 0xac;
          *(undefined4 *)(puVar15 + -0x18) = 0xfd807;
          PM_ClipVelocity();
        }
        else {
          *(undefined4 *)(puVar15 + -8) = 0x3f800000;
          *(undefined1 **)(puVar15 + -0xc) = puVar15 + 0x9c;
          *(undefined4 *)(puVar15 + -0x10) = *(undefined4 *)(puVar15 + 0x18);
          *(undefined1 **)(puVar15 + -0x14) = puVar15 + 0xac;
          *(undefined4 *)(puVar15 + -0x18) = 0xfd792;
          PM_ClipVelocity();
          *(undefined4 *)(puVar15 + 0xac) = *(undefined4 *)(puVar15 + 0x9c);
          *(undefined4 *)(puVar15 + 0xb0) = *(undefined4 *)(puVar15 + 0xa0);
          *(undefined4 *)(puVar15 + 0xb4) = *(undefined4 *)(puVar15 + 0xa4);
        }
        *(int *)(puVar15 + 0x18) = *(int *)(puVar15 + 0x18) + 0xc;
        if (*(float *)(puVar20 + -0x3c) <= (float)dc_0_7) {
          *(float *)(puVar15 + 0x38) =
               (float)dc_0_01 * (fc_1 - *(float *)(*(int *)PTR_pmove_001772c0 + 0xc4)) + (float)dc_1
          ;
          *(undefined4 *)(puVar15 + -8) = *(undefined4 *)(puVar15 + 0x38);
          *(undefined1 **)(puVar15 + -0xc) = puVar15 + 0x9c;
          *(undefined1 **)(puVar15 + -0x10) = puVar17 + 0xc;
          *(undefined1 **)(puVar15 + -0x14) = puVar15 + 0xac;
          *(undefined4 *)(puVar15 + -0x18) = 0xfd8b3;
          PM_ClipVelocity();
        }
        else {
          *(undefined4 *)(puVar15 + -8) = 0x3f800000;
          *(undefined1 **)(puVar15 + -0xc) = puVar15 + 0x9c;
          *(undefined4 *)(puVar15 + -0x10) = *(undefined4 *)(puVar15 + 0x18);
          *(undefined1 **)(puVar15 + -0x14) = puVar15 + 0xac;
          *(undefined4 *)(puVar15 + -0x18) = 0xfd840;
          PM_ClipVelocity();
          *(undefined4 *)(puVar15 + 0xac) = *(undefined4 *)(puVar15 + 0x9c);
          *(undefined4 *)(puVar15 + 0xb0) = *(undefined4 *)(puVar15 + 0xa0);
          *(undefined4 *)(puVar15 + 0xb4) = *(undefined4 *)(puVar15 + 0xa4);
        }
        puVar17 = puVar17 + 0x18;
        *(int *)(puVar15 + 0x18) = *(int *)(puVar15 + 0x18) + 0xc;
        puVar20 = puVar20 + 0x18;
        iVar10 = iVar10 + 2;
      } while (iVar10 < *(int *)(puVar15 + 0x30));
LAB_000fd8ce:
      puVar6 = PTR_pmove_001772c0;
      *(undefined4 *)(*(int *)PTR_pmove_001772c0 + 0x5c) = *(undefined4 *)(puVar15 + 0x9c);
      *(undefined4 *)(*(int *)puVar6 + 0x60) = *(undefined4 *)(puVar15 + 0xa0);
      *(undefined4 *)(*(int *)puVar6 + 100) = *(undefined4 *)(puVar15 + 0xa4);
      *(undefined4 *)(puVar15 + 0xac) = *(undefined4 *)(puVar15 + 0x9c);
      *(undefined4 *)(puVar15 + 0xb0) = *(undefined4 *)(puVar15 + 0xa0);
      *(undefined4 *)(puVar15 + 0xb4) = *(undefined4 *)(puVar15 + 0xa4);
      uVar8 = FUN_000fdc3c();
      return uVar8;
    }
    iVar10 = 0;
    if (0 < *(int *)(puVar15 + 0x30)) {
      *(undefined4 *)(puVar15 + 0x1c) = 0;
      do {
        puVar6 = PTR_pmove_001772c0;
        *(undefined4 *)(puVar15 + -8) = 0x3f800000;
        *(int *)(puVar15 + -0xc) = *(int *)PTR_pmove_001772c0 + 0x5c;
        *(undefined1 **)(puVar15 + -0x10) = puVar15 + *(int *)(puVar15 + 0x1c) + 0xcc;
        *(undefined1 **)(puVar15 + -0x14) = puVar15 + 0xac;
        *(undefined4 *)(puVar15 + -0x18) = 0xfd96a;
        PM_ClipVelocity();
        iVar11 = 0;
        iVar12 = iVar11;
        if (0 < *(int *)(puVar15 + 0x30)) {
          *(undefined **)(puVar15 + 0x20) = puVar6;
          iVar13 = 0;
          uVar9 = *(uint *)(puVar15 + 0x30) & 3;
          if (*(int *)(puVar15 + 0x30) < 1) {
LAB_000fda25:
            if ((iVar11 == iVar10) ||
               (iVar5 = **(int **)(puVar15 + 0x20), iVar12 = iVar11,
               fc_0 <= *(float *)(iVar5 + 100) * *(float *)(puVar15 + iVar13 + 0xd4) +
                       *(float *)(iVar5 + 0x60) * *(float *)(puVar15 + iVar13 + 0xd0) +
                       *(float *)(iVar5 + 0x5c) * *(float *)(puVar15 + iVar13 + 0xcc))) {
              iVar13 = iVar13 + 0xc;
              iVar11 = iVar11 + 1;
              iVar12 = iVar11;
              if (iVar11 < *(int *)(puVar15 + 0x30)) goto LAB_000fda6c;
            }
          }
          else {
            if (uVar9 != 0) {
              if (1 < uVar9) {
                if (2 < uVar9) {
                  if ((iVar10 != 0) &&
                     (iVar11 = *(int *)puVar6,
                     *(float *)(iVar11 + 100) * *(float *)(puVar15 + 0xd4) +
                     *(float *)(iVar11 + 0x60) * *(float *)(puVar15 + 0xd0) +
                     *(float *)(iVar11 + 0x5c) * *(float *)(puVar15 + 0xcc) < fc_0))
                  goto LAB_000fdb39;
                  iVar13 = 0xc;
                  iVar11 = 1;
                }
                if ((iVar11 != iVar10) &&
                   (iVar5 = **(int **)(puVar15 + 0x20), iVar12 = iVar11,
                   *(float *)(iVar5 + 100) * *(float *)(puVar15 + iVar13 + 0xd4) +
                   *(float *)(iVar5 + 0x60) * *(float *)(puVar15 + iVar13 + 0xd0) +
                   *(float *)(iVar5 + 0x5c) * *(float *)(puVar15 + iVar13 + 0xcc) < fc_0))
                goto LAB_000fdb39;
                iVar13 = iVar13 + 0xc;
                iVar11 = iVar11 + 1;
              }
              goto LAB_000fda25;
            }
LAB_000fda6c:
            puVar20 = puVar15 + iVar13 + 0x11c;
            puVar18 = puVar20;
            puVar21 = puVar20;
            do {
              if (((((iVar11 != iVar10) &&
                    (iVar13 = **(int **)(puVar15 + 0x20), iVar12 = iVar11,
                    *(float *)(iVar13 + 100) * *(float *)(puVar21 + -0x48) +
                    *(float *)(iVar13 + 0x60) * *(float *)(puVar20 + -0x4c) +
                    *(float *)(iVar13 + 0x5c) * *(float *)(puVar18 + -0x50) < fc_0)) ||
                   ((iVar11 + 1 != iVar10 &&
                    (iVar13 = **(int **)(puVar15 + 0x20), iVar12 = iVar11 + 1,
                    *(float *)(iVar13 + 100) * *(float *)(puVar21 + -0x3c) +
                    *(float *)(iVar13 + 0x60) * *(float *)(puVar20 + -0x40) +
                    *(float *)(iVar13 + 0x5c) * *(float *)(puVar18 + -0x44) < fc_0)))) ||
                  ((iVar11 + 2 != iVar10 &&
                   (iVar13 = **(int **)(puVar15 + 0x20), iVar12 = iVar11 + 2,
                   *(float *)(iVar13 + 100) * *(float *)(puVar21 + -0x30) +
                   *(float *)(iVar13 + 0x60) * *(float *)(puVar20 + -0x34) +
                   *(float *)(iVar13 + 0x5c) * *(float *)(puVar18 + -0x38) < fc_0)))) ||
                 ((iVar11 + 3 != iVar10 &&
                  (iVar13 = **(int **)(puVar15 + 0x20), iVar12 = iVar11 + 3,
                  *(float *)(iVar13 + 100) * *(float *)(puVar21 + -0x24) +
                  *(float *)(iVar13 + 0x60) * *(float *)(puVar20 + -0x28) +
                  *(float *)(iVar13 + 0x5c) * *(float *)(puVar18 + -0x2c) < fc_0)))) break;
              puVar21 = puVar21 + 0x30;
              puVar20 = puVar20 + 0x30;
              puVar18 = puVar18 + 0x30;
              iVar11 = iVar11 + 4;
              iVar12 = iVar11;
            } while (iVar11 < *(int *)(puVar15 + 0x30));
          }
        }
LAB_000fdb39:
        if (iVar12 == *(int *)(puVar15 + 0x30)) break;
        *(int *)(puVar15 + 0x1c) = *(int *)(puVar15 + 0x1c) + 0xc;
        iVar10 = iVar10 + 1;
      } while (iVar10 < *(int *)(puVar15 + 0x30));
    }
    if (iVar10 == *(int *)(puVar15 + 0x30)) {
      if (*(int *)(puVar15 + 0x30) != 2) break;
      *(undefined1 **)(puVar15 + -0xc) = puVar15 + 0x10c;
      *(undefined1 **)(puVar15 + -0x10) = puVar15 + 0xd8;
      *(undefined1 **)(puVar15 + -0x14) = puVar15 + 0xcc;
      *(undefined4 *)(puVar15 + -0x18) = 0xfdba4;
      CrossProduct();
      iVar10 = *(int *)PTR_pmove_001772c0;
      fVar1 = *(float *)(iVar10 + 0x5c);
      fVar2 = *(float *)(iVar10 + 0x60);
      fVar3 = *(float *)(iVar10 + 100);
      *(int *)(puVar15 + -0xc) = iVar10 + 0x5c;
      *(float *)(puVar15 + -0x10) =
           *(float *)(puVar15 + 0x114) * fVar3 +
           *(float *)(puVar15 + 0x110) * fVar2 + *(float *)(puVar15 + 0x10c) * fVar1;
      *(undefined1 **)(puVar15 + -0x14) = puVar15 + 0x10c;
      *(undefined4 *)(puVar15 + -0x18) = 0xfdbe2;
      VectorScale();
    }
    puVar14 = PTR_vec3_origin_00177cf4;
    puVar6 = PTR_pmove_001772c0;
    iVar10 = *(int *)PTR_pmove_001772c0;
    if (*(float *)(iVar10 + 100) * *(float *)(puVar15 + 0xc4) +
        *(float *)(iVar10 + 0x60) * *(float *)(puVar15 + 0xc0) +
        *(float *)(iVar10 + 0x5c) * *(float *)(puVar15 + 0xbc) <= fc_0) {
      *(undefined4 *)(iVar10 + 0x5c) = *(undefined4 *)PTR_vec3_origin_00177cf4;
      *(undefined4 *)(*(int *)puVar6 + 0x60) = *(undefined4 *)(puVar14 + 4);
      iVar10 = *(int *)puVar6;
      goto LAB_000fdc31;
    }
    *(int *)(puVar15 + 0x34) = *(int *)(puVar15 + 0x34) + 1;
    if (3 < *(int *)(puVar15 + 0x34)) goto LAB_000fdc7f;
    iVar10 = *(int *)PTR_pmove_001772c0;
    puVar15 = puVar15 + -4;
  }
  puVar14 = PTR_vec3_origin_00177cf4;
  puVar6 = PTR_pmove_001772c0;
  *(undefined4 *)(*(int *)PTR_pmove_001772c0 + 0x5c) = *(undefined4 *)PTR_vec3_origin_00177cf4;
  *(undefined4 *)(*(int *)puVar6 + 0x60) = *(undefined4 *)(puVar14 + 4);
  iVar10 = *(int *)puVar6;
LAB_000fdc31:
  *(undefined4 *)(iVar10 + 100) = *(undefined4 *)(puVar14 + 8);
  puVar17 = puVar15 + -4;
LAB_000fdc7f:
  puVar14 = PTR_vec3_origin_00177cf4;
  puVar6 = PTR_pmove_001772c0;
  if (*(float *)(puVar17 + 0x2c) == fc_0) {
    *(undefined4 *)(*(int *)PTR_pmove_001772c0 + 0x5c) = *(undefined4 *)PTR_vec3_origin_00177cf4;
    *(undefined4 *)(*(int *)puVar6 + 0x60) = *(undefined4 *)(puVar14 + 4);
    *(undefined4 *)(*(int *)puVar6 + 100) = *(undefined4 *)(puVar14 + 8);
  }
  return *(undefined4 *)(puVar17 + 0x28);
}


