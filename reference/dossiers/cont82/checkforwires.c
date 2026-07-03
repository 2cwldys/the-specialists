// ===== FUN @ 00092f08 size=171 =====

/* WARNING: Unknown calling convention -- yet parameter storage is locked */
/* TS_player.cpp */

void CBasePlayer::CheckForWires(CBasePlayer *this)

{
  char cVar1;
  int iVar2;
  CTSObjective *this_00;
  
  this->field_0x78c = 0;
  this->field_0x78d = 0;
  this->field_0x78e = 0;
  this_00 = (CTSObjective *)0x0;
  while( true ) {
    this_00 = (CTSObjective *)
              UTIL_FindEntityInSphere__FP11CBaseEntityRC6Vectorf
                        ((CBaseEntity *)this_00,&this->pev->origin,64.0);
    if (this_00 == (CTSObjective *)0x0) break;
    iVar2 = strcmp((*PTR_gpGlobals_00177410)->pStringBase + this_00->pev->classname,"ts_bomb");
    if ((iVar2 == 0) && ((char)this_00->field_0x64 != '\0')) {
      cVar1 = CTSObjective::CanInteract(this_00,this);
      if (cVar1 != '\0') {
        this->field_0x78c = this_00->field_0x7c;
        this->field_0x78d = this_00->field_0x7d;
        this->field_0x78e = this_00->field_0x7e;
      }
    }
  }
  return;
}


