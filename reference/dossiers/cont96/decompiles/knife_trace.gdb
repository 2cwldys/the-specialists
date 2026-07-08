# cont.96 knife auto-pickup live trace — non-stop logging on the weapon-acquisition chain.
# Run: cd <rig> && LD_LIBRARY_PATH=.:$LD_LIBRARY_PATH gdb -batch -nx -x knife_trace.gdb \
#        --args ./hlds_linux -game ts +sv_lan 1 +maxplayers 8 +rcon_password lol +map ts_hammertime
set pagination off
set breakpoint pending on
set print inferior-events off
set logging file /tmp/claude-1000/-home-stevenlafl-Projects-goldsrc-net-mod-ts/45debfdd-2db3-4a63-86cc-21d3728ab175/scratchpad/knife_trace.log
set logging overwrite on
set logging enabled on

# every dropped-weapon spawn: what type does a death-dropped knife REALLY carry?
break CTSWorldGun::CreateWeapon
commands
  silent
  printf "\n[CreateWeapon] t=%.2f type=%d status.clip=%d owner=%p\n", gpGlobals->time, type, status.clip, owner
  bt 8
  continue
end

# knife-typed touches only (keep spam down): does the gate pass without IN_USE?
break CTSWorldGun::WaitTouch if this->m_weaponType == 25
commands
  silent
  printf "\n[WaitTouch KNIFE] t=%.2f type=%d button=0x%x taken=%d\n", gpGlobals->time, this->m_weaponType, pOther ? pOther->pev->button : -1, (int)this->m_bTaken
  continue
end

# the FCAP_CONTINUOUS_USE path
break CTSWorldGun::Use
commands
  silent
  printf "\n[Use] t=%.2f type=%d cooldown=%.2f activator=%p button=0x%x\n", gpGlobals->time, this->m_weaponType, this->m_flUseCooldown, pActivator, pActivator ? pActivator->pev->button : -1
  bt 6
  continue
end

# the actual hand-over (caller visible in bt: WaitTouch vs Use vs other)
break CTSWorldGun::GetWeapon
commands
  silent
  printf "\n[GetWeapon] t=%.2f type=%d taken=%d nodraw=%d other=%p button=0x%x\n", gpGlobals->time, this->m_weaponType, (int)this->m_bTaken, (int)(pev->effects & 128), pOther, pOther ? pOther->pev->button : -1
  bt 8
  continue
end

# the ONE acquisition gate — all five callers funnel here
break CBasePlayer::PickUpWeapon
commands
  silent
  printf "\n[PickUpWeapon] t=%.2f type=%d pStatus=%p clip=%d iAttach=%d bSelect=%d bRecordOwner=%d\n", gpGlobals->time, type, pStatus, pStatus ? pStatus->clip : -999, iAttach, (int)bSelect, (int)bRecordOwner
  bt 8
  continue
end

# arsenal install (stack merge / fresh install) — the half-pickup suspect
break CTSGun::TakeWeapon
commands
  silent
  printf "\n[TakeWeapon] t=%.2f type=%d pStatus=%p iAttach=%d owned_before=%d\n", gpGlobals->time, type, pStatus, iAttach, (int)this->m_items[type].owned
  continue
end

# crash safety net (from cont.95 server_crash_capture.gdb)
catch signal SIGSEGV
commands
  printf "\n!!! SIGSEGV !!!\n"
  bt 40
  info registers
  detach
  quit
end

run
