set pagination off
handle SIGSEGV stop print
handle SIGPIPE nostop noprint
run
echo \n=== CRASH BACKTRACE ===\n
bt full
echo \n=== REGISTERS ===\n
info registers
echo \n=== SEARCH STRING (rsi) ===\n
x/s $rsi
echo \n=== RAW STACK (96 qwords) ===\n
x/96xg $rsp
echo \n=== MAPPINGS ===\n
info proc mappings
echo \n=== SHARED LIBS ===\n
info sharedlibrary
detach
