#!/usr/bin/env bash
# check-live.sh — load maps on a LAN-only dedicated server under gdb and check the engine's precache
# tables for names whose memory changed after they were precached (a name precached from a stack or
# entity buffer). Clients download from that table on join, so a bad entry drops them with
# "server failed to transmit file 'sound/<garbage>'". Puppet tests never download, so they cannot see it.
#
# Needs a Half-Life install with hlds_linux and engine_i486.so (Steam's ships with debug info) and the
# server library under test already in <install>/ts/dlls (Steam's engine loads dlls/ts.so).
#
# Usage: scripts/precache/check-live.sh [map ...]   (default: Mesa_a2 ts_frigid estate10 ts_hammertime)
#   HL_DIR overrides the install (default ~/.local/share/Steam/steamapps/common/Half-Life); PORT the
#   server port (default 27099). Exit 1 if any map has a bad entry.
set -euo pipefail

HL_DIR="${HL_DIR:-$HOME/.local/share/Steam/steamapps/common/Half-Life}"
PORT="${PORT:-27099}"
HERE="$(cd "$(dirname "$0")" && pwd)"
OUT="$(mktemp -d)"
MAPS=("$@"); [ ${#MAPS[@]} -eq 0 ] && MAPS=(Mesa_a2 ts_frigid estate10 ts_hammertime)

fail=0
cd "$HL_DIR"
for m in "${MAPS[@]}"; do
  PRECACHE_OUT="$OUT/$m.txt" LD_LIBRARY_PATH=".:${LD_LIBRARY_PATH:-}" timeout 240 gdb -batch -nx \
    -ex 'set breakpoint pending on' -ex 'set pagination off' -ex 'handle SIGPIPE nostop noprint' \
    -ex 'break SV_ActivateServer' -ex run -ex "source $HERE/dump.py" -ex kill \
    --args ./hlds_linux -game ts +sv_lan 1 +maxplayers 2 -port "$PORT" +log off +map "$m" \
    > "$OUT/$m.gdb.log" 2>&1 || true
  if [ ! -s "$OUT/$m.txt" ]; then
    echo "$m: no dump (see $OUT/$m.gdb.log)"; fail=1; continue
  fi
  n=$(sed -n 's/^bad entries //p' "$OUT/$m.txt")
  echo "$m: $(grep entries "$OUT/$m.txt" | tr '\n' ' ')"
  grep '^BAD' "$OUT/$m.txt" || true
  [ "$n" = 0 ] || fail=1
done
echo "dumps in $OUT"
exit $fail
