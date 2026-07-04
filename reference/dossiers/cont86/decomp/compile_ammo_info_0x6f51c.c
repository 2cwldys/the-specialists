// ===== FUN @ 0007f51c size=1301 =====

/* WARNING: Unknown calling convention -- yet parameter storage is locked */
/* TS_gun.cpp */

void compile_ammo_info__Fv(void)

{
  undefined4 *puVar1;
  int iVar2;
  char *pcVar3;
  undefined4 *puVar4;
  
  puVar1 = &global_ammo_info;
  for (iVar2 = 0xe0; iVar2 != 0; iVar2 = iVar2 + -1) {
    *puVar1 = 0;
    puVar1 = puVar1 + 1;
  }
  iVar2 = 0xf;
  puVar1 = &DAT_00187164;
  do {
    *puVar1 = 0x3f800000;
    puVar1[-0xe] = 0x3f800000;
    puVar1[-0x1c] = 0x3f800000;
    puVar1[-0x2a] = 0x3f800000;
    puVar1[-0x38] = 0x3f800000;
    puVar1[-0x46] = 0x3f800000;
    puVar1[-0x54] = 0x3f800000;
    puVar1[-0x62] = 0x3f800000;
    puVar1 = puVar1 + -0x70;
    iVar2 = iVar2 + -8;
  } while (-1 < iVar2);
  DAT_00186e38 = 0x3ecccccd;
  DAT_00186e3c = 0x32;
  DAT_00186e40 = 1;
  DAT_00186e48 = 0x1e;
  DAT_00186e4c = 0x2000;
  DAT_00186e50 = 0x3e4ccccd;
  DAT_00186e58 = 0x50;
  DAT_00186e5c = 0x14a;
  puVar1 = &DAT_00107ff8;
  puVar4 = &DAT_00186e60;
  for (iVar2 = 2; iVar2 != 0; iVar2 = iVar2 + -1) {
    *puVar4 = *puVar1;
    puVar1 = puVar1 + 1;
    puVar4 = puVar4 + 1;
  }
  *(undefined1 *)puVar4 = *(undefined1 *)puVar1;
  DAT_00186f18 = 0x3e99999a;
  DAT_00186f1c = 0x46;
  DAT_00186f20 = 1;
  DAT_00186f28 = 10;
  DAT_00186f2c = 0x2000;
  DAT_00186f30 = 0x3f000000;
  DAT_00186f38 = 0x5a;
  DAT_00186f3c = 0x1e0;
  DAT_00186f40 = DAT_00108001;
  DAT_00186f44 = DAT_00108005;
  DAT_00186f88 = 0x3f000000;
  DAT_00186f8c = 0x78;
  DAT_00186f90 = 1;
  DAT_00186f98 = 0x14;
  DAT_00186f9c = 0x2000;
  DAT_00186fa0 = 0x3f800000;
  DAT_00186fa8 = 100;
  DAT_00186fac = 0x2da;
  puVar1 = &DAT_00108007;
  puVar4 = &DAT_00186fb0;
  for (iVar2 = 2; iVar2 != 0; iVar2 = iVar2 + -1) {
    *puVar4 = *puVar1;
    puVar1 = puVar1 + 1;
    puVar4 = puVar4 + 1;
  }
  *(undefined1 *)puVar4 = *(undefined1 *)puVar1;
  DAT_00186e70 = 0x3ecccccd;
  DAT_00186e74 = 0x3c;
  DAT_00186e78 = 1;
  DAT_00186e80 = 0xc;
  DAT_00186e84 = 0x2000;
  DAT_00186e88 = 0x3e4ccccd;
  DAT_00186e90 = 0x5a;
  DAT_00186e94 = 0x118;
  puVar1 = &DAT_00108010;
  puVar4 = &DAT_00186e98;
  for (iVar2 = 1; iVar2 != 0; iVar2 = iVar2 + -1) {
    *puVar4 = *puVar1;
    puVar1 = puVar1 + 1;
    puVar4 = puVar4 + 1;
  }
  *(undefined2 *)puVar4 = *(undefined2 *)puVar1;
  *(undefined1 *)((int)puVar4 + 2) = *(undefined1 *)((int)puVar1 + 2);
  DAT_00186ff8 = 0x3e99999a;
  DAT_00186ffc = 0x46;
  DAT_00187000 = 1;
  DAT_00187008 = 0xf;
  DAT_0018700c = 0x2000;
  DAT_00187010 = 0x3f000000;
  DAT_00187018 = 0x5a;
  DAT_0018701c = 0x1c2;
  pcVar3 = "10mm AUTO";
  puVar1 = &DAT_00187020;
  for (iVar2 = 2; iVar2 != 0; iVar2 = iVar2 + -1) {
    *puVar1 = *(undefined4 *)pcVar3;
    pcVar3 = pcVar3 + 4;
    puVar1 = puVar1 + 1;
  }
  *(undefined2 *)puVar1 = *(undefined2 *)pcVar3;
  DAT_00186ee0 = 0x3e99999a;
  DAT_00186ee4 = 0x50;
  DAT_00186ee8 = 2;
  DAT_00186ef0 = 0x1e;
  DAT_00186ef4 = 0x2000;
  DAT_00186ef8 = 0x3f666666;
  DAT_00186f00 = 0x96;
  DAT_00186f04 = 900;
  pcVar3 = "5.56 Nato";
  puVar1 = &DAT_00186f08;
  for (iVar2 = 2; iVar2 != 0; iVar2 = iVar2 + -1) {
    *puVar1 = *(undefined4 *)pcVar3;
    pcVar3 = pcVar3 + 4;
    puVar1 = puVar1 + 1;
  }
  *(undefined2 *)puVar1 = *(undefined2 *)pcVar3;
  DAT_00186f50 = 0x3e4ccccd;
  DAT_00186f54 = 100;
  DAT_00186f58 = 2;
  DAT_00186f60 = 0x1e;
  DAT_00186f64 = 0x2000;
  DAT_00186f68 = 0x3f333333;
  DAT_00186f70 = 0xaa;
  DAT_00186f74 = 0x316;
  puVar1 = &DAT_0010802b;
  puVar4 = &DAT_00186f78;
  for (iVar2 = 3; iVar2 != 0; iVar2 = iVar2 + -1) {
    *puVar4 = *puVar1;
    puVar1 = puVar1 + 1;
    puVar4 = puVar4 + 1;
  }
  DAT_00186fc0 = 0x3dcccccd;
  DAT_00186fc4 = 500;
  DAT_00186fc8 = 2;
  DAT_00186fd0 = 5;
  DAT_00186fd4 = 0x2000;
  DAT_00186fd8 = 0x3f800000;
  DAT_00186fe0 = 200;
  DAT_00186fe4 = 1000;
  DAT_00186fe8 = DAT_00108037;
  DAT_00186fec = DAT_0010803b;
  DAT_00186ea8 = 0x3e99999a;
  DAT_00186eac = 0x78;
  DAT_00186eb0 = 2;
  DAT_00186eb8 = 8;
  DAT_00186ebc = 0x2000;
  DAT_00186ec0 = 0;
  DAT_00186ec8 = 0x5a;
  DAT_00186ecc = 0x15e;
  pcVar3 = "SHELLS";
  puVar1 = &DAT_00186ed0;
  for (iVar2 = 1; iVar2 != 0; iVar2 = iVar2 + -1) {
    *puVar1 = *(undefined4 *)pcVar3;
    pcVar3 = pcVar3 + 4;
    puVar1 = puVar1 + 1;
  }
  *(undefined2 *)puVar1 = *(undefined2 *)pcVar3;
  *(char *)((int)puVar1 + 2) = pcVar3[2];
  DAT_00187030 = 0x3f000000;
  DAT_00187034 = 100;
  DAT_00187038 = 1;
  DAT_00187040 = 0x11;
  DAT_00187044 = 0x2000;
  DAT_00187048 = 0x3f666666;
  DAT_00187050 = 0x46;
  DAT_00187054 = 0x15e;
  puVar1 = &DAT_00108044;
  puVar4 = &DAT_00187058;
  for (iVar2 = 1; iVar2 != 0; iVar2 = iVar2 + -1) {
    *puVar4 = *puVar1;
    puVar1 = puVar1 + 1;
    puVar4 = puVar4 + 1;
  }
  *(undefined2 *)puVar4 = *(undefined2 *)puVar1;
  *(undefined1 *)((int)puVar4 + 2) = *(undefined1 *)((int)puVar1 + 2);
  DAT_00187068 = 0x3e99999a;
  DAT_0018706c = 0x96;
  DAT_00187070 = 1;
  DAT_00187078 = 5;
  DAT_0018707c = 0x2000;
  DAT_00187080 = 0x3f400000;
  DAT_00187088 = 0x14;
  DAT_0018708c = 400;
  pcVar3 = ".454 Casull";
  puVar1 = &DAT_00187090;
  for (iVar2 = 3; iVar2 != 0; iVar2 = iVar2 + -1) {
    *puVar1 = *(undefined4 *)pcVar3;
    pcVar3 = pcVar3 + 4;
    puVar1 = puVar1 + 1;
  }
  DAT_001870a0 = 0x3f000000;
  DAT_001870a4 = 0x1e;
  DAT_001870a8 = 1;
  DAT_001870b0 = 0x14;
  DAT_001870b4 = 0x2000;
  DAT_001870b8 = 0;
  DAT_001870c0 = 0x28;
  DAT_001870c4 = 300;
  puVar1 = &DAT_00108057;
  puVar4 = &DAT_001870c8;
  for (iVar2 = 1; iVar2 != 0; iVar2 = iVar2 + -1) {
    *puVar4 = *puVar1;
    puVar1 = puVar1 + 1;
    puVar4 = puVar4 + 1;
  }
  *(undefined2 *)puVar4 = *(undefined2 *)puVar1;
  *(undefined1 *)((int)puVar4 + 2) = *(undefined1 *)((int)puVar1 + 2);
  DAT_001870d8 = 0x3e4ccccd;
  DAT_001870dc = 0x96;
  DAT_001870e0 = 2;
  DAT_001870e8 = 5;
  DAT_001870ec = 0x2000;
  DAT_001870f0 = 0x3f333333;
  DAT_001870f8 = 0xaa;
  DAT_001870fc = 0x352;
  puVar1 = &DAT_0010805e;
  puVar4 = &DAT_00187100;
  for (iVar2 = 3; iVar2 != 0; iVar2 = iVar2 + -1) {
    *puVar4 = *puVar1;
    puVar1 = puVar1 + 1;
    puVar4 = puVar4 + 1;
  }
  Set_ammo_count__Fi(0);
  return;
}


