// ===== FUN @ 10056e70 size=240 =====

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void FUN_10056e70(void)

{
  int iVar1;
  
  DAT_1018bdd0 = (*DAT_100d5bb8)();
  DAT_1018bdd4 = 0;
  if (-1 < DAT_1018bdd0) {
    do {
      iVar1 = (*DAT_100d5bfc)(DAT_1018bdd4);
      _DAT_1018bd8c = iVar1;
      if ((iVar1 != 0) && (*(int *)(iVar1 + 4) != 0)) {
        (&DAT_1018caf8)[DAT_1018bdd4 * 0xc] = *(undefined4 *)(iVar1 + 0xb60);
        (&DAT_1018cafc)[DAT_1018bdd4 * 0xc] = *(undefined4 *)(iVar1 + 0xb64);
        (&DAT_1018cb00)[DAT_1018bdd4 * 0xc] = *(undefined4 *)(iVar1 + 0xb68);
        (&DAT_1018cb04)[DAT_1018bdd4 * 0xc] = *(undefined4 *)(iVar1 + 0xb6c);
        (&DAT_1018cb08)[DAT_1018bdd4 * 0xc] = *(undefined4 *)(iVar1 + 0xb70);
        (&DAT_1018cb0c)[DAT_1018bdd4 * 0xc] = *(undefined4 *)(iVar1 + 0xb74);
        (&DAT_1018cb10)[DAT_1018bdd4 * 0xc] = *(undefined4 *)(iVar1 + 0xb78);
        (&DAT_1018cb14)[DAT_1018bdd4 * 0xc] = *(undefined4 *)(iVar1 + 0xb7c);
        (&DAT_1018cb18)[DAT_1018bdd4 * 0xc] = *(undefined4 *)(iVar1 + 0xb80);
        (&DAT_1018cb1c)[DAT_1018bdd4 * 0xc] = *(undefined4 *)(iVar1 + 0xb84);
        (&DAT_1018cb20)[DAT_1018bdd4 * 0xc] = *(undefined4 *)(iVar1 + 0xb88);
        (&DAT_1018cb24)[DAT_1018bdd4 * 0xc] = *(undefined4 *)(iVar1 + 0xb8c);
      }
      DAT_1018bdd4 = DAT_1018bdd4 + 1;
    } while (DAT_1018bdd4 <= DAT_1018bdd0);
  }
  return;
}


