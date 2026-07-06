#!/usr/bin/env bash
#
# setup-rehlds-mod.sh — deploy OUR reconstructed server gamedll (hl.so) into the
# local ReHLDS dedicated-server rig so we can run a real client/server test: a
# ReHLDS dedicated server running OUR hl.so, joined by the xash graphical client
# running OUR client.so (see setup-xash-mod.sh). This is the true drop-in swap
# test — our server + our client speaking the reconstructed protocol.
#
# The rig loads the gamedll THROUGH metamod (so rcbot keeps working):
#   ts/liblist.gam         → gamedll_linux "addons/metamod/dlls/metamod_amd64.so"
#   ts/addons/metamod/config.ini → gamedll dlls/hl.so   ← metamod loads OUR hl.so
# so we only need to drop hl.so at ts/dlls/hl.so (+ delta.lst/sprites for wire
# parity). Pass NOMETAMOD=1 to instead point liblist directly at our hl.so
# (bypasses metamod/rcbot — a cleaner isolation of our gamedll).
#
# Re-run after rebuilding (scripts/build.sh amd64) to refresh the server lib.
#
set -euo pipefail

MODTS="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
RIG="${RIG:-$HOME/Containers/hlds/hlds-linux64-test}"   # ReHLDS dedicated rig
TS="$RIG/ts"                                            # the mod game dir on the rig

HL_SO="$MODTS/build-amd64/dlls/hl.so"

echo ">>> mod-ts     : $MODTS"
echo ">>> rehlds rig : $RIG"
echo ">>> ts gamedir : $TS"

[ -f "$HL_SO" ] || { echo "!! missing $HL_SO — run scripts/build.sh amd64 first"; exit 1; }
[ -x "$RIG/hlds_run" ] || { echo "!! no ReHLDS launcher at $RIG/hlds_run"; exit 1; }
[ -d "$TS" ] || { echo "!! no ts game dir at $TS"; exit 1; }

# --- server gamedll -----------------------------------------------------------
# metamod's config.ini resolves the real gamedll to ts/dlls/hl.so; drop ours there.
mkdir -p "$TS/dlls"
[ -f "$TS/dlls/hl.so" ] && [ ! -f "$TS/dlls/hl.so.orig" ] && cp -f "$TS/dlls/hl.so" "$TS/dlls/hl.so.orig"
cp -f "$HL_SO" "$TS/dlls/hl.so"
echo ">>> deployed hl.so ($(du -h "$HL_SO" | cut -f1))"

# --- wire table: deploy OUR merged delta.lst to the rig ----------------------
# network/delta.lst is the MERGED table: the original TS wire fields (iuser3/iuser4 etc.)
# 3-way-merged from the 1998 SDK seed with the anniversary/HL25 fixes (eflags flesh-sound,
# wider skin/m_iId/iuser2). It's the correct table for our server — a minimal WIDENING of the
# original TS table, so it stays drop-in with the original client (GoldSrc transmits the
# server's table at connect, and widening is decode-safe) while keeping the modern engine fixes.
# Use --remove-destination: the rig's ts/delta.lst starts as a SYMLINK into the pristine
# ~/Containers/hlds/mods/ts; a plain `cp -f` would follow it and write THROUGH, clobbering the
# shared original. --remove-destination REPLACES the symlink with a real file in the rig instead.
cp -f --remove-destination "$MODTS/network/delta.lst" "$TS/delta.lst"
# (No generated sprites are deployed — the scope now paints the ORIGINAL quarterscope.spr at
#  render time; ts_scopefull.spr was removed. quarterscope resolves via the ts/sprites symlink.)

# --- gamedll wiring: metamod (default, keeps rcbot) vs direct -----------------
LIB="$TS/liblist.gam"
if [ "${NOMETAMOD:-0}" = "1" ]; then
	sed -i.bak 's|^gamedll_linux .*|gamedll_linux "dlls/hl.so"|' "$LIB"
	echo ">>> liblist.gam → dlls/hl.so (metamod/rcbot BYPASSED)"
else
	sed -i.bak 's|^gamedll_linux .*|gamedll_linux "addons/metamod/dlls/metamod_amd64.so"|' "$LIB"
	# make sure metamod points at our gamedll
	CFG="$TS/addons/metamod/config.ini"
	[ -f "$CFG" ] && ! grep -q '^gamedll dlls/hl.so' "$CFG" && \
		sed -i.bak 's|^gamedll .*|gamedll dlls/hl.so|' "$CFG"
	echo ">>> liblist.gam → metamod (rcbot active); metamod → dlls/hl.so"
fi

echo ">>> done."
echo
echo "Run the DEDICATED SERVER (kill the hlds_run wrapper, not just hlds_linux, to stop it):"
echo "    cd $RIG && ./hlds_run -game ts +sv_lan 1 +maxplayers 8 +map tm_avana"
echo
echo "Then JOIN with our xash client (server side of the drop-in test):"
echo "    scripts/setup-xash-mod.sh          # refresh the client with our client.so"
echo "    cd \$HOME/Projects/goldsrc.net/xash3d-fwgs/dist/client-x64"
echo "    LD_LIBRARY_PATH=. ./xash3d -dev 2 -game ts +connect 127.0.0.1:27015"
echo
echo "(sv_lan 1 lets the LAN client join without a Steam ticket.)"
