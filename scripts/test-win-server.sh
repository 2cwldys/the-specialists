#!/usr/bin/env bash
#
# test-win-server.sh <mp.dll> [seconds] — load a Windows server library in the Windows HLDS under Wine,
# on ts_hammertime, and keep the console log. Answers "does this mp.dll load and run on Windows" without a
# Windows machine. Run it on reference/mp.dll first: TS 3.0's own library is the control.
#
#   HLDS_WIN   the Windows HLDS install (default /media/Data3/Projects/goldsrc.net/hlds-win). Fetched with
#              steamcmd (app 90, anonymous) when hlds.exe is missing.
#   TS_CONTENT TS 3.0's content (default recovered/ts3-linux/ts in this repo). Linked, not copied.
#   UNTIL      stop as soon as the console log matches this grep -E pattern; SECS stays the upper limit
#   MAP        the map (default ts_hammertime); MAXPLAYERS (default 8); EXTRA_ARGS more command line
#              (e.g. "+rcbot config min_bots 6"; the engine splits commas, so no commas)
#
# Output: $HLDS_WIN/ts-test/<dll name>-<time>.log (the engine's console, from -condebug), and an exit code:
# 0 when the running server answered an A2S_INFO query (it is running frames on the map), else 1.
set -euo pipefail
MODTS="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
DLL="$(realpath "${1:?usage: test-win-server.sh <mp.dll> [seconds]}")"
SECS="${2:-40}"
HLDS_WIN="${HLDS_WIN:-/media/Data3/Projects/goldsrc.net/hlds-win}"
TS_CONTENT="$(realpath "${TS_CONTENT:-$MODTS/recovered/ts3-linux/ts}")"
MAP="${MAP:-ts_hammertime}"
export WINEPREFIX="${WINEPREFIX:-$HLDS_WIN/wineprefix}" WINEDEBUG="${WINEDEBUG:--all}"
# Wine's Linux-side helpers can trip a libdbus assertion ("pending != NULL") that aborts the whole process
# a few seconds into a map, TS 3.0's own mp.dll included. With no bus to reach they skip those calls.
export DBUS_SYSTEM_BUS_ADDRESS=unix:path=/nonexistent DBUS_SESSION_BUS_ADDRESS=unix:path=/nonexistent
# A crash prints Wine's backtrace into the run's stdout log instead of opening the debugger on the desktop
# (which also keeps the server's port until someone closes it).

if [ ! -f "$HLDS_WIN/hlds.exe" ]; then
  mkdir -p "$HLDS_WIN"
  steamcmd +@sSteamCmdForcePlatformType windows +force_install_dir "$HLDS_WIN" +login anonymous \
    +app_update 90 validate +quit
fi

# ts/ = links to every top-level entry of the original content, except dlls/, which holds the library
# under test only. Rebuilt each run so a previous test's library never lingers.
GAME="$HLDS_WIN/ts"
if [ -e "$GAME" ] && [ ! -f "$GAME/.test-win-server" ]; then
  echo "$GAME exists and was not made by this script; refusing to touch it" >&2; exit 2
fi
rm -rf "${GAME:?}"
mkdir -p "$GAME/dlls"
touch "$GAME/.test-win-server"
for e in "$TS_CONTENT"/*; do
  [ "$(basename "$e")" = dlls ] || ln -s "$e" "$GAME/$(basename "$e")"
done
cp "$DLL" "$GAME/dlls/mp.dll"
# GAMECFG: lines for game.cfg, which the game runs before it picks its rules (e.g. "mp_teamplay 1").
if [ -n "${GAMECFG:-}" ]; then rm -f "$GAME/game.cfg"; printf '%s\n' "$GAMECFG" > "$GAME/game.cfg"; fi

# ADDONS=1: load the library through Metamod 1.21.1-am and stock AMX Mod X 1.8.2 with its TS modules (tsx,
# tsfun), running scripts/winrig/rets_probe.sma, which drives TSX's setters on a fake client every second.
# The releases are fetched once into $HLDS_WIN/addons-src.
#
# ADDONS=<suite zip> (scripts/winrig/package-win-suite.sh): unpack the suite over ts/ (its mp.dll replaces
# the one given), wire liblist.gam to its Metamod-R and add PROBE, compiled with the suite's own compiler,
# to the shipped plugins.ini.
#
# UPGRADE=1 with ADDONS=<suite zip>: install stock Metamod 1.21.1-am and AMX Mod X 1.8.2 first, with their
# own configs and plugins, then extract the suite over them, as an existing server would. The 1.8.2
# configs stay; only modules.ini and plugins.ini are set for the probe.
if [ "${ADDONS:-0}" != 0 ] && [ "${ADDONS:-0}" != 1 ]; then
  if [ "${UPGRADE:-0}" = 1 ]; then
    SRC="$HLDS_WIN/addons-src"
    ( cd "$GAME" && unzip -qo "$SRC/zips/amxmodx-1.8.2-base-windows.zip" \
        && unzip -qo "$SRC/zips/amxmodx-1.8.2-ts-windows.zip" && unzip -qo "$SRC/zips/metamod-1.21.1-am.zip" )
  fi
  unzip -qo "$(realpath "$ADDONS")" 'ts/*' -d "$(dirname "$GAME")"
  rm "$GAME/liblist.gam"
  sed 's|^gamedll .*|gamedll "addons\\metamod\\dlls\\metamod.dll"|' "$TS_CONTENT/liblist.gam" > "$GAME/liblist.gam"
  printf 'win32 addons\\amxmodx\\dlls\\amxmodx_mm.dll\n' > "$GAME/addons/metamod/plugins.ini"
  # the shipped modules.ini and plugins.ini stay (AMX Mod X loads the modules a plugin needs); the probe
  # is added after the stock plugins
  PROBE="${PROBE:-rets_probe}"   # PROBE=none adds no probe
  SCR="$GAME/addons/amxmodx/scripting"
  printf '\n' >> "$GAME/addons/amxmodx/configs/plugins.ini"
  if [ "$PROBE" != none ]; then
    printf '%s.amxx\n' "$PROBE" >> "$GAME/addons/amxmodx/configs/plugins.ini"
    cp "$MODTS/scripts/winrig/$PROBE.sma" "$SCR/"
    ( cd "$SCR" && wine amxxpc.exe "$PROBE.sma" "-o../plugins/$PROBE.amxx" ) > /dev/null 2>&1
    [ -f "$GAME/addons/amxmodx/plugins/$PROBE.amxx" ] || { echo "$PROBE did not compile" >&2; exit 3; }
  fi
  # EXTRA_SMA: plugin sources from outside this repo (a server's own plugins), compiled with the suite's
  # compiler and added to plugins.ini; EXTRA_CFG: files copied into addons/amxmodx/configs.
  for sma in ${EXTRA_SMA:-}; do
    cp "$sma" "$SCR/"; n="$(basename "$sma" .sma)"
    ( cd "$SCR" && wine amxxpc.exe "$n.sma" "-o../plugins/$n.amxx" ) > "$GAME/addons/amxmodx/$n.compile.txt" 2>&1
    [ -f "$GAME/addons/amxmodx/plugins/$n.amxx" ] || { echo "$n did not compile" >&2; tail -20 "$GAME/addons/amxmodx/$n.compile.txt" >&2; exit 3; }
    printf '%s.amxx\n' "$n" >> "$GAME/addons/amxmodx/configs/plugins.ini"
  done
  for c in ${EXTRA_CFG:-}; do cp "$c" "$GAME/addons/amxmodx/configs/"; done
  # RCBOT=1: the drop-in's RCBot (rcbot_mm_i386.dll under Metamod, its data tree at the install root)
  if [ "${RCBOT:-0}" = 1 ]; then
    DZ="$HLDS_WIN/addons-src/zips/hlds-dropin-windows-i386-v1.0.9.zip"
    unzip -qo "$DZ" 'rcbot/*' -d "$HLDS_WIN"
    unzip -qjo "$DZ" 'valve/addons/metamod/dlls/rcbot_mm_i386.dll' -d "$GAME/addons/metamod/dlls"
    printf 'win32 addons\\metamod\\dlls\\rcbot_mm_i386.dll\n' >> "$GAME/addons/metamod/plugins.ini"
  fi
  echo ">>> suite $(basename "$ADDONS"); mp.dll $(sha256sum "$GAME/dlls/mp.dll" | cut -c1-12)"
elif [ "${ADDONS:-0}" = 1 ]; then
  SRC="$HLDS_WIN/addons-src"; mkdir -p "$SRC/zips"
  for z in amxmodx-1.8.2-base-windows.zip amxmodx-1.8.2-ts-windows.zip metamod-1.21.1-am.zip; do
    [ -f "$SRC/zips/$z" ] || curl -sfL -o "$SRC/zips/$z" "https://www.amxmodx.org/release/$z"
  done
  ( cd "$GAME" && unzip -qo "$SRC/zips/amxmodx-1.8.2-base-windows.zip" \
      && unzip -qo "$SRC/zips/amxmodx-1.8.2-ts-windows.zip" && unzip -qo "$SRC/zips/metamod-1.21.1-am.zip" )
  rm "$GAME/liblist.gam"
  sed 's|^gamedll .*|gamedll "addons\\metamod\\dlls\\metamod.dll"|' "$TS_CONTENT/liblist.gam" > "$GAME/liblist.gam"
  printf 'win32 addons\\amxmodx\\dlls\\amxmodx_mm.dll\n' > "$GAME/addons/metamod/plugins.ini"
  printf 'fakemeta\nhamsandwich\ntsx\ntsfun\n' > "$GAME/addons/amxmodx/configs/modules.ini"
  PROBE="${PROBE:-rets_probe}"   # a plugin in scripts/winrig/
  printf '%s.amxx\n' "$PROBE" > "$GAME/addons/amxmodx/configs/plugins.ini"
  SCR="$GAME/addons/amxmodx/scripting"
  cp "$MODTS/scripts/winrig/$PROBE.sma" "$SCR/"
  ( cd "$SCR" && wine amxxpc.exe "$PROBE.sma" "-o../plugins/$PROBE.amxx" ) > /dev/null 2>&1
  [ -f "$GAME/addons/amxmodx/plugins/$PROBE.amxx" ] || { echo "$PROBE did not compile" >&2; exit 3; }
fi

wineserver -k 2>/dev/null || true   # this prefix only: leftovers of an earlier run
wine reg add 'HKCU\Software\Wine\WineDbg' /v ShowCrashDialog /t REG_DWORD /d 0 /f > /dev/null 2>&1 || true
LOGDIR="$HLDS_WIN/ts-test"; mkdir -p "$LOGDIR"
LOG="$LOGDIR/$(basename "$(dirname "$DLL")")-$(basename "$DLL" .dll)-$(date +%H%M%S).log"
rm -f "$GAME/qconsole.log" "$HLDS_WIN/qconsole.log"
echo ">>> $DLL ($(sha256sum "$DLL" | cut -c1-12)) for ${SECS}s"
set +e
( cd "$HLDS_WIN" && timeout "$SECS" wine hlds.exe -console -condebug -game ts -port 27099 \
    +sv_lan 1 +maxplayers "${MAXPLAYERS:-8}" +deathmatch 1 +localinfo hamhooks "${HAMHOOKS:-}" ${EXTRA_ARGS:-} \
    +map "$MAP" ) > "$LOG.stdout" 2>&1 &
pid=$!
# A server that is running frames answers A2S_INFO with its map; a hung or dead one does not.
answered=""
for _ in $(seq 1 $((SECS / 5))); do
  answered="$(python3 -I - <<'EOF' 2>/dev/null
import socket
s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM); s.settimeout(2)
q = b'\xff\xff\xff\xffTSource Engine Query\x00'
s.sendto(q, ('127.0.0.1', 27099)); d = s.recv(1400)
if d[4:5] == b'A':  # challenge: repeat with it
    s.sendto(q + d[5:9], ('127.0.0.1', 27099)); d = s.recv(1400)
f = d[6:].split(b'\x00')
print(f'name={f[0].decode(errors="replace")} map={f[1].decode()} game={f[2].decode()}')
EOF
)"
  [ -n "$answered" ] && break
  kill -0 "$pid" 2>/dev/null || break
  sleep 5
done
# UNTIL=<grep -E pattern>: stop the server as soon as its console log matches (or the time runs out),
# so a run takes as long as what it is checking, not a fixed SECS.
if [ -n "${UNTIL:-}" ]; then
  while kill -0 "$pid" 2>/dev/null; do
    # grep the files directly: "cat | grep -q" fails under pipefail once grep stops reading
    grep -qsE "$UNTIL" "$HLDS_WIN/qconsole.log" "$GAME/qconsole.log" && { sleep 1; break; }
    sleep 1
  done
  # end the run's own timeout (the subshell's child) so it stops waiting out SECS, then Wine
  pkill -TERM -P "$pid" 2>/dev/null || true
  wineserver -k 2>/dev/null || true
fi
wait "$pid"; rc=$?
set -e
echo ">>> A2S_INFO: ${answered:-no answer}"
cat "$HLDS_WIN/qconsole.log" "$GAME/qconsole.log" 2>/dev/null > "$LOG" || true
echo ">>> exit $rc (124 = ran out the time, 143 = stopped by UNTIL); console: $LOG"
grep -E "\[probe\]|\[model\]|\[ham\]|\[cl\]|\[ib\]|\[ms\]|Connection to Steam servers|Could not load|LoadLibrary|failed|crash|exception|Error" \
  "$LOG" "$LOG.stdout" | tail -25 || true
[ -n "$answered" ]
