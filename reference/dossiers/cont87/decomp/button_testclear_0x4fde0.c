// ===== FUN @ 1004fde0 size=30 =====

uint __cdecl FUN_1004fde0(undefined4 param_1,uint param_2)

{
  if ((param_2 & DAT_100cf970) != 0) {
    DAT_100cf970 = DAT_100cf970 & ~param_2;
    return CONCAT31((int3)(~param_2 >> 8),1);
  }
  return param_2 & 0xffffff00;
}


