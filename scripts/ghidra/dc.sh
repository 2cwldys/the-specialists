#!/usr/bin/env bash
# dc.sh — decompile a function by address in ANY recovered Ghidra project (the client/mp counterpart to
# re.sh, which is server-only/ts_i386.so). Output reads like source + is cleaned of the analyzeHeadless noise.
#   scripts/ghidra/dc.sh client 0x69b90   # client.dll  (give the RVA; Ghidra addr = PE imagebase 0x10000000 + RVA)
#   scripts/ghidra/dc.sh mp     0x4b9a0   # mp.dll       (same PE imagebase)
#   scripts/ghidra/dc.sh server 0x6ea30   # ts_i386.so   (give the nm/file addr; Ghidra addr = addr + 0x10000)
# Pass several addresses to batch them. For the SERVER prefer re.sh (it resolves Class->methods + applies the
# full inventory); dc.sh is the quick by-address path, and the only one for client.dll / mp.dll.
# `scripts/live.py resolve/bt` gives you the RVA from a live trace; feed it straight in here.
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
SP="$ROOT/scripts/ghidra"
GH="$(command -v analyzeHeadless || echo "$HOME/Tools/ghidra/support/analyzeHeadless")"

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

mod="${1:?usage: dc.sh <client|mp|server> 0x<addr> [0x<addr>...]}"; shift
case "$mod" in
  client) PROJ="$ROOT/recovered/ghidra/client"; NAME=clientdll; PROG=client.dll; BASE=$((0x10000000));;
  mp)     PROJ="$ROOT/recovered/ghidra/mpdll";  NAME=mpdll;     PROG=mp.dll;     BASE=$((0x10000000));;
  server) PROJ="$ROOT/recovered/ghidra/re";     NAME=re;        PROG=ts_i386.so; BASE=$((0x10000));;
  *) echo "module must be client | mp | server" >&2; exit 1;;
esac
[ -d "$PROJ" ] || { echo "no Ghidra project at $PROJ — build it (setup-ghidra.sh / re.sh setup)" >&2; exit 1; }

addrs=()
for a in "$@"; do addrs+=("$(printf '0x%x' $(( BASE + a )))"); done
[ ${#addrs[@]} -gt 0 ] || { echo "give at least one 0x<addr>" >&2; exit 1; }

"$GH" "$PROJ" "$NAME" -process "$PROG" -noanalysis -scriptPath "$SP" \
  -postScript DecompAt.java "${addrs[*]}" 2>&1 | _filter DecompAt.java
