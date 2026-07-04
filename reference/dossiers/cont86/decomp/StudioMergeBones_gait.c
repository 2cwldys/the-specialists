// ===== FUN @ 10031080 size=1829 =====

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void __fastcall FUN_10031080(int *param_1)

{
  byte bVar1;
  int iVar2;
  undefined4 uVar3;
  byte *pbVar4;
  int iVar5;
  float *pfVar6;
  void *extraout_ECX;
  void *this;
  byte *pbVar7;
  undefined4 *puVar8;
  float unaff_ESI;
  float fVar9;
  int iVar10;
  char *pcVar11;
  int iVar12;
  bool bVar13;
  float10 fVar14;
  float fVar15;
  float fVar16;
  float *pfVar17;
  byte *pbVar18;
  float fStack_78;
  float fStack_64;
  float fStack_60;
  double dStack_5c;
  float afStack_50 [3];
  undefined4 uStack_44;
  undefined4 uStack_34;
  undefined4 uStack_24;
  
  if (*(int *)(param_1[0x11] + 0xa4) <= *(int *)(param_1[0xc] + 0x2dc)) {
    *(undefined4 *)(param_1[0xc] + 0x2dc) = 0;
  }
  fVar9 = (float)(*(int *)(param_1[0xc] + 0x2dc) * 0xb0 + *(int *)(param_1[0x11] + 0xa8) +
                 param_1[0x11]);
  fStack_78 = fVar9;
  fVar14 = (float10)(**(code **)(*param_1 + 0x34))();
  iVar12 = param_1[0xd];
  dStack_5c = (double)fVar14;
  iVar2 = (**(code **)(*param_1 + 0x18))(iVar12,fVar9);
  fVar15 = (float)(double)CONCAT44(fStack_60,fStack_64);
  (**(code **)(*param_1 + 0x4c))(&DAT_100e6790,&DAT_100e89c0,fVar9,iVar2,fVar15);
  if (1 < *(int *)((int)fVar9 + 0x78)) {
    iVar2 = iVar2 + *(int *)(param_1[0x11] + 0x8c) * 0xc;
    (**(code **)(*param_1 + 0x4c))(&DAT_100ea86c,&DAT_100e91c0,fVar9,iVar2,fVar15);
    fVar14 = (float10)(**(code **)(*param_1 + 0x30))();
    (**(code **)(*param_1 + 0x3c))
              (&DAT_100e89c0,&DAT_100e6790,&DAT_100e91c0,&DAT_100ea86c,
               ((float)*(byte *)(param_1[0xc] + 0xb40) * ((float)_DAT_10079e80 - (float)fVar14) +
               (float)*(byte *)(param_1[0xc] + 0x31c) * (float)fVar14) * (float)_DAT_10079e68);
    if (*(int *)((int)fVar9 + 0x78) == 4) {
      iVar2 = iVar2 + *(int *)(param_1[0x11] + 0x8c) * 0xc;
      fVar16 = fVar9;
      iVar10 = iVar2;
      (**(code **)(*param_1 + 0x4c))(&DAT_100e83c0,&DAT_100e7bc0,fVar9,iVar2,fVar15);
      iVar2 = iVar2 + *(int *)(param_1[0x11] + 0x8c) * 0xc;
      (**(code **)(*param_1 + 0x4c))(&DAT_100e7590,&DAT_100e9fc0,fVar9,iVar2,fVar15);
      fVar15 = ((float)*(byte *)(param_1[0xc] + 0xb40) * (float)(double)CONCAT44(iVar10,fVar16) +
               (float)*(byte *)(param_1[0xc] + 0x31c) * fVar15) * (float)_DAT_10079e68;
      (**(code **)(*param_1 + 0x3c))(&DAT_100e7bc0,&DAT_100e83c0,&DAT_100e9fc0,&DAT_100e7590,fVar15)
      ;
      (**(code **)(*param_1 + 0x3c))
                (&DAT_100e89c0,&DAT_100e6790,&DAT_100e7bc0,&DAT_100e83c0,
                 ((float)*(byte *)(param_1[0xc] + 0xb41) * (float)(double)CONCAT44(iVar2,fVar9) +
                 (float)*(byte *)(param_1[0xc] + 0x31d) * fVar15) * (float)_DAT_10079e68);
    }
  }
  if (((param_1[6] != 0) && (iVar2 = param_1[0xc], *(float *)(iVar2 + 0xb14) != _DAT_100799f0)) &&
     ((float)*(double *)(param_1 + 2) < *(float *)(iVar2 + 0xb14) + (float)_DAT_1007a8c8)) {
    iVar10 = param_1[0x11];
    if (*(int *)(iVar2 + 0xb34) < *(int *)(iVar10 + 0xa4)) {
      iVar10 = *(int *)(iVar2 + 0xb34) * 0xb0 + *(int *)(iVar10 + 0xa8) + iVar10;
      iVar2 = (**(code **)(*param_1 + 0x18))(param_1[0xd],iVar10);
      (**(code **)(*param_1 + 0x4c))
                (&DAT_100e5990,&DAT_100e6d90,iVar10,iVar2,*(undefined4 *)(param_1[0xc] + 0xb38));
      if (1 < *(int *)(iVar10 + 0x78)) {
        iVar2 = iVar2 + *(int *)(param_1[0x11] + 0x8c) * 0xc;
        (**(code **)(*param_1 + 0x4c))
                  (&DAT_100ea86c,&DAT_100e91c0,iVar10,iVar2,*(undefined4 *)(param_1[0xc] + 0xb38));
        (**(code **)(*param_1 + 0x3c))
                  (&DAT_100e6d90,&DAT_100e5990,&DAT_100e91c0,&DAT_100ea86c,
                   (float)*(byte *)(param_1[0xc] + 0xb18) * (float)_DAT_10079e68);
        if (*(int *)(iVar10 + 0x78) == 4) {
          iVar2 = iVar2 + *(int *)(param_1[0x11] + 0x8c) * 0xc;
          (**(code **)(*param_1 + 0x4c))
                    (&DAT_100e83c0,&DAT_100e7bc0,iVar10,iVar2,*(undefined4 *)(param_1[0xc] + 0xb38))
          ;
          (**(code **)(*param_1 + 0x4c))
                    (&DAT_100e7590,&DAT_100e9fc0,iVar10,iVar2 + *(int *)(param_1[0x11] + 0x8c) * 0xc
                     ,*(undefined4 *)(param_1[0xc] + 0xb38));
          (**(code **)(*param_1 + 0x3c))
                    (&DAT_100e7bc0,&DAT_100e83c0,&DAT_100e9fc0,&DAT_100e7590,
                     (float)*(byte *)(param_1[0xc] + 0xb18) * (float)_DAT_10079e68);
          (**(code **)(*param_1 + 0x3c))
                    (&DAT_100e6d90,&DAT_100e5990,&DAT_100e7bc0,&DAT_100e83c0,
                     (float)*(byte *)(param_1[0xc] + 0xb19) * (float)_DAT_10079e68);
        }
      }
      (**(code **)(*param_1 + 0x3c))
                (&DAT_100e89c0,&DAT_100e6790,&DAT_100e6d90,&DAT_100e5990,
                 (float)_DAT_10079e80 -
                 ((float)*(double *)(param_1 + 2) - *(float *)(param_1[0xc] + 0xb14)) *
                 (float)_DAT_1007a918);
      this = extraout_ECX;
      goto LAB_100314ab;
    }
  }
  this = (void *)param_1[0xc];
  *(int *)((int)this + 0xb38) = iVar12;
LAB_100314ab:
  pbVar7 = (byte *)(*(int *)(param_1[0x11] + 0x90) + param_1[0x11]);
  pbVar18 = pbVar7;
  CStudioModelRenderer::Init(this);
  pbVar4 = pbVar7;
  if ((param_1[0xe] != 0) && (iVar2 = *(int *)(param_1[0xe] + 0x17c), iVar2 != 0)) {
    iVar2 = iVar2 * 0xb0 + *(int *)(param_1[0x11] + 0xa8) + param_1[0x11];
    uVar3 = (**(code **)(*param_1 + 0x18))(param_1[0xd],iVar2);
    (**(code **)(*param_1 + 0x4c))
              (&DAT_100ea86c,&DAT_100e91c0,iVar2,uVar3,*(undefined4 *)(param_1[0xe] + 0x180));
    iVar2 = 0;
    if (0 < *(int *)(param_1[0x11] + 0x8c)) {
      iVar12 = 0;
      iVar10 = 0;
      do {
        pcVar11 = s_Bip01_Dorso_10094d34;
        pbVar4 = pbVar7;
        do {
          bVar1 = *pbVar4;
          bVar13 = bVar1 < (byte)*pcVar11;
          if (bVar1 != *pcVar11) {
LAB_10031568:
            iVar5 = (1 - (uint)bVar13) - (uint)(bVar13 != 0);
            goto LAB_1003156d;
          }
          if (bVar1 == 0) break;
          bVar1 = pbVar4[1];
          bVar13 = bVar1 < (byte)pcVar11[1];
          if (bVar1 != pcVar11[1]) goto LAB_10031568;
          pbVar4 = pbVar4 + 2;
          pcVar11 = pcVar11 + 2;
        } while (bVar1 != 0);
        iVar5 = 0;
LAB_1003156d:
        if (iVar5 == 0) {
LAB_10031616:
          FUN_1002ffe0(&fStack_60,(float *)&stack0xffffff94);
          FUN_1002ffe0((float *)&DAT_100e89c0,&fStack_78);
          fVar15 = fStack_64 - unaff_ESI;
          FUN_1002ffe0((float *)(&DAT_100e89c0 + iVar2 * 4),&fStack_78);
          fStack_78 = fStack_78 + fVar15;
          FUN_1002fbd0(&fStack_78,(float *)(&DAT_100e89c0 + iVar2 * 4));
          pbVar4 = pbVar18;
          break;
        }
        pcVar11 = s_Bip01_Spine_10094d28;
        pbVar4 = pbVar7;
        do {
          bVar1 = *pbVar4;
          bVar13 = bVar1 < (byte)*pcVar11;
          if (bVar1 != *pcVar11) {
LAB_100315a2:
            iVar5 = (1 - (uint)bVar13) - (uint)(bVar13 != 0);
            goto LAB_100315a7;
          }
          if (bVar1 == 0) break;
          bVar1 = pbVar4[1];
          bVar13 = bVar1 < (byte)pcVar11[1];
          if (bVar1 != pcVar11[1]) goto LAB_100315a2;
          pbVar4 = pbVar4 + 2;
          pcVar11 = pcVar11 + 2;
        } while (bVar1 != 0);
        iVar5 = 0;
LAB_100315a7:
        if (iVar5 == 0) goto LAB_10031616;
        pbVar7 = pbVar7 + 0x70;
        *(undefined4 *)((int)&DAT_100e6790 + iVar10) = *(undefined4 *)((int)&DAT_100ea86c + iVar10);
        *(undefined4 *)((int)&DAT_100e6794 + iVar10) = *(undefined4 *)((int)&DAT_100ea870 + iVar10);
        *(undefined4 *)((int)&DAT_100e6798 + iVar10) = *(undefined4 *)((int)&DAT_100ea874 + iVar10);
        *(undefined4 *)((int)&DAT_100e89c0 + iVar12) = *(undefined4 *)((int)&DAT_100e91c0 + iVar12);
        *(undefined4 *)((int)&DAT_100e89c4 + iVar12) = *(undefined4 *)((int)&DAT_100e91c4 + iVar12);
        *(undefined4 *)((int)&DAT_100e89c8 + iVar12) = *(undefined4 *)((int)&DAT_100e91c8 + iVar12);
        *(undefined4 *)((int)&DAT_100e89cc + iVar12) = *(undefined4 *)((int)&DAT_100e91cc + iVar12);
        iVar2 = iVar2 + 1;
        iVar12 = iVar12 + 0x10;
        pbVar4 = pbVar18;
        iVar10 = iVar10 + 0xc;
      } while (iVar2 < *(int *)(param_1[0x11] + 0x8c));
    }
  }
  iVar12 = 0;
  iVar2 = 0;
  if (0 < *(int *)(param_1[0x11] + 0x8c)) {
    puVar8 = &DAT_100e6794;
    pfVar17 = (float *)&DAT_100e89c0;
    pbVar4 = pbVar4 + 0x20;
    do {
      FUN_1002fdf0(pfVar17,afStack_50);
      uStack_44 = puVar8[-1];
      uStack_34 = *puVar8;
      uStack_24 = puVar8[1];
      if (*(int *)pbVar4 == -1) {
        iVar10 = (*DAT_100ea860)();
        if (iVar10 == 0) {
          pfVar6 = (float *)param_1[0x1029];
        }
        else {
          pfVar6 = (float *)param_1[0x1028];
        }
        FUN_1002fa90(pfVar6,afStack_50,(float *)(param_1[0x102a] + iVar12));
        FUN_1002fa90((float *)param_1[0x1028],afStack_50,(float *)(param_1[0x102b] + iVar12));
        (**(code **)(*param_1 + 0x38))(param_1[0xc],param_1[0x102a] + iVar12);
      }
      else {
        FUN_1002fa90((float *)(*(int *)pbVar4 * 0x30 + param_1[0x102a]),afStack_50,
                     (float *)(param_1[0x102a] + iVar12));
        FUN_1002fa90((float *)(*(int *)pbVar4 * 0x30 + param_1[0x102b]),afStack_50,
                     (float *)(param_1[0x102b] + iVar12));
      }
      pfVar17 = pfVar17 + 4;
      iVar2 = iVar2 + 1;
      puVar8 = puVar8 + 3;
      iVar12 = iVar12 + 0x30;
      pbVar4 = pbVar4 + 0x70;
    } while (iVar2 < *(int *)(param_1[0x11] + 0x8c));
  }
  return;
}


