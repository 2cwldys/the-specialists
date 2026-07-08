set pagination off
set confirm off
set $hits = 0
attach 22494
set *(int*)0x0c1fd174 = 0x6f6f6f6f
watch *(int*)0x0c1fd174
commands
  set $hits = $hits + 1
  printf "\n>>> WRITE #%d: new = %d (0x%x)  caller=", $hits, *(int*)0x0c1fd174, *(int*)0x0c1fd174
  x/1i $eip
  continue
end
printf ">>> ARMED (server now all-weapons). Type reconnect in the client.\n"
continue
