#!/usr/bin/env bash
# decompile.sh — run Ghidra headless to decompile a reference binary to C.
#
# Imports the binary into a throwaway project, auto-analyzes (applies the
# symtab), and runs DecompExport.java to write <Class>.c files.
#
# Usage:
#   scripts/decompile.sh <binary> <outDir> [classFilter]
# e.g.
#   scripts/decompile.sh reference/ts_i386.so recovered/server CTSSlowMotion
#   scripts/decompile.sh reference/ts_i386.so recovered/server      # everything
set -euo pipefail

BIN="${1:?usage: decompile.sh <binary> <outDir> [classFilter]}"
OUT="${2:?usage: decompile.sh <binary> <outDir> [classFilter]}"
FILTER="${3:-}"

GHIDRA="${GHIDRA_HOME:-$HOME/Tools/ghidra}/support/analyzeHeadless"
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
SCRIPTDIR="$ROOT/scripts/ghidra"
BIN_ABS="$(cd "$(dirname "$BIN")" && pwd)/$(basename "$BIN")"
mkdir -p "$OUT"; OUT_ABS="$(cd "$OUT" && pwd)"
PROJ="$(mktemp -d /tmp/ghproj.XXXXXX)"

trap 'rm -rf "$PROJ"' EXIT

post=(-postScript DecompExport.java "$OUT_ABS")
[ -n "$FILTER" ] && post+=("$FILTER")

"$GHIDRA" "$PROJ" tsdecomp \
  -import "$BIN_ABS" \
  -scriptPath "$SCRIPTDIR" \
  "${post[@]}" \
  -deleteProject

echo ">>> decompiled C in: $OUT_ABS"
ls -la "$OUT_ABS" | tail -n +2 | sed 's/^/    /'
