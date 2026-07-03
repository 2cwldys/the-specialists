// ===== FUN @ 00077d6c size=412 =====

/* WARNING: Unknown calling convention -- yet parameter storage is locked */
/* TS_gamerulesclass.cpp */

void CTSGameRules::Init(CTSGameRules *this)

{
  globalvars_t **ppgVar1;
  int iVar2;
  int *piVar3;
  
  *(undefined *)&this->pev = *PTR_g_teamplay_00177f1c;
  InitTeams(this);
  *(undefined1 *)((int)&this->pev + 1) = 0xff;
  this->m_flNextGlobalSlow = 0.0;
  this->m_flGlobalSlowActive = 0;
  this->m_flGlobalSlowScale = 0.0;
  this->m_flGlobalSlowEnd = 0.0;
  this->field_0x574 = -0x40800000;
  this->field_0x578 = -1;
  this->field_0x55c = -1;
  this->field_0x558 = -1;
  piVar3 = &this->field_0x57c;
  for (iVar2 = 0x21; ppgVar1 = PTR_gpGlobals_00177410, iVar2 != 0; iVar2 = iVar2 + -1) {
    *piVar3 = 0;
    piVar3 = piVar3 + 1;
  }
  this->field_0x570 = (int)((float)dc_60 + (*PTR_gpGlobals_00177410)->time);
  this->field_0x4 = 0;
  this->field_0x14 = 0;
  *(undefined1 *)&this->field_0x18 = 0;
  this->m_flRoundEnd = 0.0;
  this->m_flRoundRestart = (float)dc_5 + (*ppgVar1)->time;
  *(undefined1 *)&this->field_0x10 = 1;
  InitObjectives(this);
  SetObjective(this);
  piVar3 = &this->field_0x684;
  for (iVar2 = 0x240; iVar2 != 0; iVar2 = iVar2 + -1) {
    *piVar3 = 0;
    piVar3 = piVar3 + 1;
  }
  this->field_0xf84 = (int)((float)dc_10 + (*ppgVar1)->time);
  this->field_0x1824 = 0;
  CBulletManager::Reset((CBulletManager *)&this->field_0x1864);
  iVar2 = InitWeaponRestriction__Fv();
  this->field_0x586c = iVar2;
  this->field_0x5870 = iVar2;
  *(undefined1 *)&this->field_0x5878 = 0;
  this->field_0x587c = (int)((float)dc_10 + (*ppgVar1)->time);
  this->m_flCheckDeads = 0.0;
  this->field_0x5884 = 0;
  this->field_0x5885 = 0;
  this->field_0x5875 = 0;
  this->field_0x5874 = 0;
  this->field_0x5876 = 0;
  this->field_0x5888 = 0x3dcccccd;
  this->field_0x588c = 0x3e59999a;
  this->field_0x5890 = 0;
  *(undefined1 *)&this->field_0x5891 = 0;
  return;
}


