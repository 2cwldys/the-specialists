set pagination off
set confirm off
handle SIGUSR1 nostop noprint pass
handle SIGUSR2 nostop noprint pass
handle SIG33 nostop noprint pass
handle SIG34 nostop noprint pass
set $hits = 0
attach 22494
set *(int*)0x0c1fd174 = 0x6f6f6f6f
watch *(int*)0x0c1fd174
commands
  set $hits = $hits + 1
  shell date +%T.%3N
  printf ">>> WRITE #%d: new = %d (0x%x)  caller=", $hits, *(int*)0x0c1fd174, *(int*)0x0c1fd174
  x/1i $eip
  continue
end
printf ">>> ARMED — poison 0x6f6f6f6f set; workaround-disabled server live. RECONNECT NOW.\n"
shell date +%T.%3N
continue
