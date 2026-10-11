#!/usr/bin/env bash
# dc.sh — decompile a function by address in ANY recovered Ghidra project (the client/mp counterpart to
# re.sh, which is server-only/ts_i386.so). Output reads like source + is cleaned of the analyzeHeadless noise.
#   scripts/ghidra/dc.sh client 0x69b90   # client.dll  (give the RVA; Ghidra addr = PE imagebase 0x10000000 + RVA)
#   scripts/ghidra/dc.sh mp     0x4b9a0   # mp.dll       (same PE imagebase)
#   scripts/ghidra/dc.sh server 0x6ea30   # ts_i386.so   (give the nm/file addr; Ghidra addr = addr + 0x10000)
# Pass several addresses to batch them. For the SERVER prefer re.sh (it resolves Class->methods + applies the
# full inventory); dc.sh is the quick by-address path, and the only one for client.dll / mp.dll.
# `scripts/live.py resolve/bt` gives you the RVA from a live trace; feed it straight in here.
#
#   scripts/ghidra/dc.sh setup client     # build recovered/ghidra/client if missing, then apply ApplyWinTypes
#   scripts/ghidra/dc.sh setup mp         # the same for recovered/ghidra/mpdll
# A decompile of client/mp with no project yet runs the setup first. ApplyWinTypes (varargs signatures,
# the CRT names, __chkstk, the typed engine tables and a call-site prototype for every variadic call with a
# literal format) is idempotent, so `setup` also re-applies it to an existing project. To rebuild a project
# from scratch, move its directory to the attic first. DC_GHIDRA_DIR=<dir> puts the client/mp projects
# under <dir> instead of recovered/ghidra (a throwaway fresh import).
#
#   scripts/ghidra/dc.sh scan client out.tsv   # every variadic call: format, decompiled call, raw argument
#                                              #   pushes (ScanVarargs.java); varargs-ports.py joins it with
#                                              #   the addresses reTS cites
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
SP="$ROOT/scripts/ghidra"
GH="$(command -v analyzeHeadless || echo "$HOME/Tools/ghidra/support/analyzeHeadless")"
GDIR="${DC_GHIDRA_DIR:-$ROOT/recovered/ghidra}"

# Strip the analyzeHeadless log noise, keeping multi-line decompile bodies (same helper as re.sh): only the
# FIRST physical line of a println carries "INFO  <tag>> " and only the LAST carries " (GhidraScript)".
_filter() {
  awk -v tag="$1" '
    BEGIN { pfx = "INFO  " tag "> "; plen = length(pfx); inblk = 0 }
    {
      if (substr($0, 1, plen) == pfx) { line = substr($0, plen + 1); inblk = 1 }
      else if (inblk)                 { line = $0 }
      else                            { next }
      if (line ~ / \(GhidraScript\)[ \t\r]*$/) { sub(/ \(GhidraScript\)[ \t\r]*$/, "", line); inblk = 0 }
      print line
    }'
}

# setup <client|mp>: import the PE (noreturn discovery off) if the project is missing, then ApplyWinTypes.
_setup() {
  local proj="$1" name="$2" prog="$3" log
  log="$(mktemp -t "dc-setup-$name.XXXXXX.log")"
  if [ ! -d "$proj/$name.rep" ]; then
    mkdir -p "$proj"
    echo ">>> importing reference/$prog -> $proj/$name (noreturn discovery off)" >&2
    "$GH" "$proj" "$name" -import "$ROOT/reference/$prog" -scriptPath "$SP" -preScript DisableNoReturn.java \
      >"$log" 2>&1 || { echo "import failed, see $log" >&2; exit 1; }
  fi
  echo ">>> ApplyWinTypes on $name (log: $log)" >&2
  "$GH" "$proj" "$name" -process "$prog" -noanalysis -scriptPath "$SP" -postScript ApplyWinTypes.java "$ROOT" \
    >>"$log" 2>&1 || { echo "ApplyWinTypes failed, see $log" >&2; exit 1; }
  grep -h "ApplyWinTypes: " "$log" | sed 's/^INFO  ApplyWinTypes.java> //; s/ (GhidraScript) *$//' \
    | grep -vE '^ApplyWinTypes: (named|variadic|not variadic|override skipped)' >&2
}

mod="${1:?usage: dc.sh <client|mp|server> 0x<addr> [0x<addr>...] | dc.sh setup <client|mp>}"; shift
if [ "$mod" = setup ]; then
  case "${1:-}" in
    client) _setup "$GDIR/client" clientdll client.dll;;
    mp)     _setup "$GDIR/mpdll"  mpdll     mp.dll;;
    *) echo "usage: dc.sh setup <client|mp>" >&2; exit 1;;
  esac
  exit 0
fi
if [ "$mod" = scan ]; then
  case "${1:-}" in
    client) P="$GDIR/client"; N=clientdll; G=client.dll;;
    mp)     P="$GDIR/mpdll";  N=mpdll;     G=mp.dll;;
    *) echo "usage: dc.sh scan <client|mp> <out.tsv>" >&2; exit 1;;
  esac
  out="$(realpath -m "${2:?usage: dc.sh scan <client|mp> <out.tsv>}")"
  [ -d "$P/$N.rep" ] || _setup "$P" "$N" "$G"
  "$GH" "$P" "$N" -process "$G" -noanalysis -readOnly -scriptPath "$SP" -postScript ScanVarargs.java "$out" 2>&1 \
    | grep -h "ScanVarargs: " | sed 's/^INFO  ScanVarargs.java> //; s/ (GhidraScript) *$//'
  exit 0
fi
case "$mod" in
  client) PROJ="$GDIR/client"; NAME=clientdll; PROG=client.dll; BASE=$((0x10000000));;
  mp)     PROJ="$GDIR/mpdll";  NAME=mpdll;     PROG=mp.dll;     BASE=$((0x10000000));;
  server) PROJ="$ROOT/recovered/ghidra/re";     NAME=re;        PROG=ts_i386.so; BASE=$((0x10000));;
  *) echo "module must be client | mp | server" >&2; exit 1;;
esac
if [ ! -d "$PROJ/$NAME.rep" ]; then
  case "$mod" in
    client|mp) _setup "$PROJ" "$NAME" "$PROG";;
    *) echo "no Ghidra project at $PROJ — build it (re.sh setup)" >&2; exit 1;;
  esac
fi

addrs=()
for a in "$@"; do addrs+=("$(printf '0x%x' $(( BASE + a )))"); done
[ ${#addrs[@]} -gt 0 ] || { echo "give at least one 0x<addr>" >&2; exit 1; }

"$GH" "$PROJ" "$NAME" -process "$PROG" -noanalysis -scriptPath "$SP" \
  -postScript DecompAt.java "${addrs[*]}" 2>&1 | _filter DecompAt.java
