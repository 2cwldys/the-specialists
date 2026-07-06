set pagination off
set breakpoint pending on
break Host_Error
run
printf "\n==== HOST_ERROR HIT ====\n"
x/30a $rsp
