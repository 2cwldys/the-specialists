// ===== FUN @ 00075ff4 size=302 =====

/* WARNING: Unknown calling convention -- yet parameter storage is locked */
/* TS_gamerules.cpp */

void SetPlayerRespawnTime__FP11CBasePlayerib(CBasePlayer *a0,int a1,bool a2)

{
  float fVar1;
  float fVar2;
  
  if ((((*PTR_TSGameRules_00177300 == '\x03') || (*PTR_TSGameRules_00177300 == '\x02')) ||
      (*(int *)PTR_g_lastManStanding_0017733c == 0)) || (*PTR_TSGameRules_00177300 != '\0')) {
    a0->field_0x770 = (int)((float)a1 + (*PTR_gpGlobals_00177410)->time);
  }
  else {
    fVar1 = *(float *)(PTR_respawnnumber_001779d4 + 0xc);
    if (fVar1 < fc_1) {
      fVar1 = fc_1;
    }
    if (fVar1 <= (float)(short)(char)a0->field_0x7ba) {
      a0->field_0x770 = -0x40800000;
    }
    else {
      fVar1 = *(float *)(PTR_TSGameRules_00177300 + 8) - (*PTR_gpGlobals_00177410)->time;
      fVar2 = (float)a1;
      if (fVar1 < fVar2) {
        fVar2 = fVar1;
      }
      a0->field_0x770 = (int)(fVar2 + (*PTR_gpGlobals_00177410)->time);
      fVar1 = *(float *)(PTR_TSGameRules_00177300 + 8) - (*PTR_gpGlobals_00177410)->time;
      if (fc_10 < fVar1) {
        fVar1 = fc_10;
      }
      a0->field_0x7bc = (int)(fVar1 + (*PTR_gpGlobals_00177410)->time);
    }
  }
  if ((float)a0->field_0x770 < fc_0) {
    a0->field_0x74c = 1;
  }
  else {
    a0->field_0x74c = 0xb;
  }
  return;
}
