#!/usr/bin/env bash
#
# test-win-server.sh <mp.dll> [seconds] — load a Windows server library in the Windows HLDS under Wine,
# on ts_hammertime, and keep the console log. Answers "does this mp.dll load and run on Windows" without a
# Windows machine. Run it on reference/mp.dll first: TS 3.0's own library is the control.
#
#   HLDS_WIN   the Windows HLDS install (default /media/Data3/Projects/goldsrc.net/hlds-win). Fetched with
#              steamcmd (app 90, anonymous) when hlds.exe is missing.
#   TS_CONTENT TS 3.0's content (default recovered/ts3-linux/ts in this repo). Linked, not copied.
#   MAP        the map (default ts_hammertime)
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

LOGDIR="$HLDS_WIN/ts-test"; mkdir -p "$LOGDIR"
LOG="$LOGDIR/$(basename "$(dirname "$DLL")")-$(basename "$DLL" .dll)-$(date +%H%M%S).log"
rm -f "$GAME/qconsole.log" "$HLDS_WIN/qconsole.log"
echo ">>> $DLL ($(sha256sum "$DLL" | cut -c1-12)) for ${SECS}s"
set +e
( cd "$HLDS_WIN" && timeout "$SECS" wine hlds.exe -console -condebug -game ts -port 27099 \
    +sv_lan 1 +maxplayers 8 +deathmatch 1 +map "$MAP" ) > "$LOG.stdout" 2>&1 &
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
wait "$pid"; rc=$?
set -e
echo ">>> A2S_INFO: ${answered:-no answer}"
cat "$HLDS_WIN/qconsole.log" "$GAME/qconsole.log" 2>/dev/null > "$LOG" || true
echo ">>> exit $rc (124 = still running when the time ran out); console: $LOG"
grep -E "Connection to Steam servers|VAC secure|Could not load|LoadLibrary|failed|crash|exception|Error" \
  "$LOG" "$LOG.stdout" | head -20 || true
[ -n "$answered" ]
