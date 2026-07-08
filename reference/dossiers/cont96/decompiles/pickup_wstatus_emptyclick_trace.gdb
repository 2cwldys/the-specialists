# cont.96 knife-pickup corruption trace v2 — pickup chain + WStatus bytes + empty-click state
set pagination off
set breakpoint pending on
set print inferior-events off
set logging file /home/stevenlafl/.claude/jobs/040f4004/tmp/knife2.log
set logging overwrite on
set logging enabled on

break CTSWorldGun::GetWeapon
commands
  silent
  printf "\n[GetWeapon] t=%.2f type=%d clipInStatus=%d\n", gpGlobals->time, this->m_weaponType, this->m_status.clip
  bt 4
  continue
end

break CBasePlayer::PickUpWeapon
commands
  silent
  printf "\n[PickUpWeapon] t=%.2f type=%d pStatus=%p clip=%d iAttach=%d bSelect=%d\n", gpGlobals->time, type, pStatus, pStatus ? pStatus->clip : -999, iAttach, (int)bSelect
  continue
end

break CTSGun::TakeWeapon
commands
  silent
  printf "\n[TakeWeapon] t=%.2f type=%d pStatus=%p statClip=%d | before: owned=%d itclip=%d active_wt=%d m_iClip=%d\n", gpGlobals->time, type, pStatus, pStatus ? pStatus->clip : -999, (int)this->m_items[type].owned, this->m_items[type].clip, this->m_weaponType, this->m_iClip
  continue
end

break CTSGun::SelectWeapon
commands
  silent
  printf "\n[SelectWeapon] t=%.2f sel=%d | cur_wt=%d m_iClip=%d items[sel].clip=%d items[sel].owned=%d\n", gpGlobals->time, type, this->m_weaponType, this->m_iClip, this->m_items[type].clip, (int)this->m_items[type].owned
  continue
end

# WStatus bar sends — kung-fu (0) and knife (25) cells only
break ts_gun.cpp:2496 if (t == 0 || t == 25 || t == 29)
commands
  silent
  printf "[WStatus] t=%.2f slot=%d state=%d\n", gpGlobals->time, t, this->m_iClientWStatus[t]
  continue
end
break ts_gun.cpp:2546
commands
  silent
  printf "[WStatus-pair] t=%.2f slot=%d c=%d\n", gpGlobals->time, t, this->m_iClientWStatus[t]
  continue
end

# the dry-fire click — what state produced the empty?
break ts_gun.cpp:2607
commands
  silent
  printf "\n[EMPTY-CLICK] t=%.2f wt=%d m_iClip=%d items[wt].clip=%d items[wt].owned=%d\n", gpGlobals->time, this->m_weaponType, this->m_iClip, this->m_items[this->m_weaponType].clip, (int)this->m_items[this->m_weaponType].owned
  bt 4
  continue
end

catch signal SIGSEGV
commands
  printf "\n!!! SIGSEGV !!!\n"
  bt 40
  detach
  quit
end

run
