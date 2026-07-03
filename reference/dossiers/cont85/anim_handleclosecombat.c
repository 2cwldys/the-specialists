    iVar2 = 2;
    break;
  case 0xc:
    pcVar3 = "circkicklow";
    goto LAB_0006db9e;
  case 0xd:
    pcVar3 = "circback";
    iVar2 = 2;
    pcVar4 = local_5c;
    break;
  case 0xe:
    local_5c._0_4_ = s_latback_0010690d._0_4_;
    local_5c._4_4_ = s_latback_0010690d._4_4_;
    goto switchD_0006d902_caseD_15;
  case 0xf:
    pcVar3 = "circspin";
    iVar2 = 2;
    pcVar4 = local_5c;
    break;
  case 0x10:
    pcVar3 = "sidekick";
    iVar2 = 2;
    pcVar4 = local_5c;
    break;
  case 0x11:
    pcVar3 = "backkick";
    iVar2 = 2;
    pcVar4 = local_5c;
    break;
  case 0x12:
    local_5c._0_4_ = s_airkick_00106930._0_4_;
    local_5c._4_4_ = s_airkick_00106930._4_4_;
    goto switchD_0006d902_caseD_15;
  case 0x13:
    pcVar3 = "airkick2";
    iVar2 = 2;
    pcVar4 = local_5c;
    break;
  case 0x14:
    pcVar3 = "duckkick";
    iVar2 = 2;
    pcVar4 = local_5c;
    break;
  default:
    goto switchD_0006d902_caseD_15;
  case 0x16:
    pcVar3 = "punchleft";
LAB_0006daa2:
    pcVar4 = local_5c;
    for (iVar2 = 2; iVar2 != 0; iVar2 = iVar2 + -1) {
      *(undefined4 *)pcVar4 = *(undefined4 *)pcVar3;
      pcVar3 = pcVar3 + 4;
      pcVar4 = (char *)((int)pcVar4 + 4);
    }
    *(undefined2 *)pcVar4 = *(undefined2 *)pcVar3;
    goto switchD_0006d902_caseD_15;
  case 0x17:
    pcVar4 = "punchright";
    iVar2 = 2;
    goto LAB_0006dc56;
  case 0x18:
    pcVar3 = "punchair";
    iVar2 = 2;
    pcVar4 = local_5c;
    break;
  case 0x19:
    pcVar3 = "uppercut";
    iVar2 = 2;
    pcVar4 = local_5c;
    break;
  case 0x1d:
    pcVar4 = "knife1";
    iVar2 = 1;
    pcVar3 = local_5c;
    goto LAB_0006dc56;
  case 0x1e:
    pcVar4 = "knife2";
    iVar2 = 1;
    pcVar3 = local_5c;
    goto LAB_0006dc56;
  case 0x1f:
    pcVar4 = "knife3";
    iVar2 = 1;
    pcVar3 = local_5c;
    goto LAB_0006dc56;
  case 0x20:
    iVar2 = 3;
    pcVar3 = "knife_launch";
    pcVar4 = local_5c;
    break;
  case 0x21:
    local_5c._0_4_ = DAT_00106993;
    local_5c._4_4_ = DAT_00106997;
    goto switchD_0006d902_caseD_15;
  case 0x22:
    local_5c._0_4_ = DAT_0010699b;
    local_5c._4_4_ = DAT_0010699f;
    goto switchD_0006d902_caseD_15;
  case 0x23:
    local_5c._0_4_ = DAT_001069a3;
    local_5c._4_4_ = DAT_001069a7;
    goto switchD_0006d902_caseD_15;
  case 0x24:
    pcVar4 = "katana_air";
    iVar2 = 2;
    pcVar3 = local_5c;
    goto LAB_0006dc56;
  case 0x25:
    pcVar3 = "katana_duck";
LAB_0006db9e:
    pcVar4 = local_5c;
    for (iVar2 = 3; iVar2 != 0; iVar2 = iVar2 + -1) {
      *(undefined4 *)pcVar4 = *(undefined4 *)pcVar3;
      pcVar3 = pcVar3 + 4;
      pcVar4 = (char *)((int)pcVar4 + 4);
    }
    goto switchD_0006d902_caseD_15;
  case 0x26:
    iVar2 = strncmp(&a0->field_0x478,"kat",3);
    if (iVar2 == 0) {
      pcVar3 = "katana_block";
LAB_0006dc12:
      pcVar4 = local_5c;
      for (iVar2 = 3; iVar2 != 0; iVar2 = iVar2 + -1) {
        *(undefined4 *)pcVar4 = *(undefined4 *)pcVar3;
        pcVar3 = pcVar3 + 4;
        pcVar4 = pcVar4 + 4;
      }
      *pcVar4 = *pcVar3;
    }
    else {
      iVar2 = strncmp(&a0->field_0x478,"kni",3);
      if (iVar2 != 0) {
        pcVar3 = "kungfu_block";
        goto LAB_0006dc12;
      }
      pcVar3 = "knife_block";
      pcVar4 = local_5c;
      for (iVar2 = 3; iVar2 != 0; iVar2 = iVar2 + -1) {
        *(undefined4 *)pcVar4 = *(undefined4 *)pcVar3;
        pcVar3 = pcVar3 + 4;
        pcVar4 = (char *)((int)pcVar4 + 4);
      }
    }
    uVar1 = (*PTR_g_engfuncs_001778b8->pfnRandomLong)(0,1);
    pcVar3 = "1";
    if (CONCAT31(extraout_var,uVar1) == 0) {
      pcVar3 = "2";
    }
    goto LAB_0006dc7d;
  case 0x27:
    pcVar3 = "stock_";
    pcVar4 = local_5c;
    for (iVar2 = 1; iVar2 != 0; iVar2 = iVar2 + -1) {
      *(undefined4 *)pcVar4 = *(undefined4 *)pcVar3;
      pcVar3 = pcVar3 + 4;
      pcVar4 = (char *)((int)pcVar4 + 4);
    }
    *(undefined2 *)pcVar4 = *(undefined2 *)pcVar3;
    *(char *)((int)pcVar4 + 2) = pcVar3[2];
    pcVar3 = &a0->field_0xb58;
LAB_0006dc7d:
    strcat(local_5c,pcVar3);
    goto switchD_0006d902_caseD_15;
  case 0x3a:
    iVar2 = 3;
    pcVar4 = "superjump_land";
    pcVar3 = local_5c;
LAB_0006dc56:
    for (; iVar2 != 0; iVar2 = iVar2 + -1) {
      *(undefined4 *)pcVar3 = *(undefined4 *)pcVar4;
      pcVar4 = pcVar4 + 4;
      pcVar3 = (char *)((int)pcVar3 + 4);
    }
    *(undefined2 *)pcVar3 = *(undefined2 *)pcVar4;
    *(char *)((int)pcVar3 + 2) = pcVar4[2];
    goto switchD_0006d902_caseD_15;
  }
  for (; iVar2 != 0; iVar2 = iVar2 + -1) {
    *(undefined4 *)pcVar4 = *(undefined4 *)pcVar3;
    pcVar3 = pcVar3 + 4;
    pcVar4 = pcVar4 + 4;
  }
  *pcVar4 = *pcVar3;
switchD_0006d902_caseD_15:
  iVar2 = GaitCloseCombat(this,a1,a0);
  if (iVar2 == 0) {
    uVar5 = 0x14;
  }
  else {
    uVar5 = 0x12;
  }
  SetAnim(this,a0,local_5c,uVar5 | 0x80,0);
  this->field_0xc = 0x40000000;
  this->field_0x8 = 0x40400000;
  *(undefined1 *)&this->field_0x1e8 = 0;
  return;
}


