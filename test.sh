#!/bin/bash
# Build → stage → launch the mod in xash for a graphical test, in one shot.
# setup-xash-mod.sh must re-run after every rebuild (CLAUDE.md) to copy the fresh
# hl_amd64.so / client_amd64.so into the xash mod root — so we always do both here.
#   ./test.sh            # build + stage + launch on the default map
#   ./test.sh tm_avana   # ...on a specific map
#   NOBUILD=1 ./test.sh  # skip the build (just re-stage + relaunch)
set -e
MOD=/home/stevenlafl/Projects/goldsrc.net/mod-ts
XASH=/home/stevenlafl/Projects/goldsrc.net/xash3d-fwgs/dist/client-x64
MAP="${1:-ts_kissmeyerbasic}"

[ -n "$NOBUILD" ] || "$MOD/scripts/build.sh" amd64 all
"$MOD/scripts/setup-xash-mod.sh"

# A missing .bsp makes xash silently drop to the main menu (this bit us: tm_ vs ts_ prefix).
# Fail loudly instead, listing what's actually available.
if [ ! -f "$XASH/ts/maps/$MAP.bsp" ] && [ ! -f "$XASH/valve/maps/$MAP.bsp" ]; then
	echo "!! map '$MAP' not found in ts/maps or valve/maps — xash would drop to the menu." >&2
	echo "   available ts maps:" >&2
	ls "$XASH"/ts/maps/*.bsp 2>/dev/null | xargs -n1 basename | sed 's/\.bsp$//' | column >&2
	exit 1
fi

# +deathmatch 1 is REQUIRED — without it the listenserver runs single-player rules and
# fire-to-play bounces you back to spectate in an endless map-reload loop (CLAUDE.md §3).
cd "$XASH"
# GDB=<script.gdb> launches xash UNDER gdb in batch mode (gdb becomes the parent, which
# satisfies ptrace_scope=1 with no sudo) so a watch/trace script can drive the live build
# and log to a file. GDB=1 (or GDB=attach) launches under PLAIN gdb — no script: it runs
# the game and CATCHES a segfault/hang at the gdb prompt (`bt` for the backtrace; `c` to
# continue). NB with the mouse grabbed, reach the prompt from ANOTHER terminal or a TTY —
# or use `kill -TRAP <xash pid>` to break in. Default (GDB unset) launches normally.
if [ -n "$GDB" ]; then
	export LD_LIBRARY_PATH=.   # gdb passes its env to the inferior
	if [ "$GDB" = "1" ] || [ "$GDB" = "attach" ]; then
		# plain interactive gdb: auto-run, stay attached; on crash you get the prompt.
		exec gdb -nx -ex run --args ./xash3d -dev 2 -game ts +deathmatch 1 +maxplayers 8 +map "$MAP"
	fi
	exec gdb -batch -nx -x "$GDB" --args ./xash3d -dev 2 -game ts +deathmatch 1 +maxplayers 8 +map "$MAP"
fi
LD_LIBRARY_PATH=. ./xash3d -dev 2 -game ts +deathmatch 1 +maxplayers 8 +map "$MAP"
