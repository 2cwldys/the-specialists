// ===== FUN @ 100581c0 size=304 =====

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void __cdecl FUN_100581c0(int param_1,int param_2)

{
  undefined4 *puVar1;
  int iVar2;
  undefined4 *puVar3;
  
  puVar1 = FUN_100071c0(param_1);
  iVar2 = param_1 * 0x30 + param_2;
  _DAT_100d142c = *(undefined4 *)(iVar2 + 4);
  DAT_100d142a = *(undefined1 *)(iVar2 + 2);
  _DAT_100d1438 = *(undefined4 *)(iVar2 + 0x10);
  if ((*(byte *)((int)puVar1 + 0x85) < 0x11) && (*(byte *)(puVar1 + 0x21) < 0xb)) {
    *(int *)(&DAT_100d0e04 +
            ((uint)*(byte *)((int)puVar1 + 0x85) * 0xb + (uint)*(byte *)(puVar1 + 0x21)) * 8) =
         param_1;
    *(undefined4 *)
     (&DAT_100d0e08 +
     ((uint)*(byte *)((int)puVar1 + 0x85) * 0xb + (uint)*(byte *)(puVar1 + 0x21)) * 8) =
         *(undefined4 *)(iVar2 + 4);
  }
  iVar2 = puVar1[0x32];
  if (iVar2 != -1) {
    puVar3 = FUN_100071c0(iVar2);
    if ((*(byte *)((int)puVar3 + 0x85) < 0x11) && (*(byte *)(puVar3 + 0x21) < 0xb)) {
      *(undefined4 *)
       (&DAT_100d0e08 +
       ((uint)*(byte *)((int)puVar3 + 0x85) * 0xb + (uint)*(byte *)(puVar3 + 0x21)) * 8) =
           *(undefined4 *)(iVar2 * 0x30 + 4 + param_2);
    }
  }
  iVar2 = puVar1[0x33];
  if ((iVar2 != -1) && (iVar2 != param_1)) {
    puVar1 = FUN_100071c0(iVar2);
    if ((*(byte *)((int)puVar1 + 0x85) < 0x11) && (*(byte *)(puVar1 + 0x21) < 0xb)) {
      *(undefined4 *)
       (&DAT_100d0e08 +
       ((uint)*(byte *)((int)puVar1 + 0x85) * 0xb + (uint)*(byte *)(puVar1 + 0x21)) * 8) =
           *(undefined4 *)(iVar2 * 0x30 + 4 + param_2);
    }
  }
  return;
}


