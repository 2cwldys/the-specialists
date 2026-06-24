#!/usr/bin/env bash
# re.sh — one entry point for the full server RE pipeline. Runs every pass, then decompiles a target
# so it reads like source (members/vtables/engine API/FP constants/clean names — see ApplyInventory.java).
#
#   scripts/ghidra/re.sh setup            # (re)build the applied project: run.py + import + ApplyInventory.
#                                         #   Do this once, and after coverage/script changes.
#   scripts/ghidra/re.sh members          # DumpMembers on the applied project -> members_auto.tsv skeleton
#   scripts/ghidra/re.sh <Class>          # decompile every method of a class (resolves addrs via filemap.py)
#   scripts/ghidra/re.sh 0x7ea30 0x7decc  # decompile by Ghidra address
#
# Why a separate "setup": ApplyInventory renames functions, so it is NOT idempotent — re-applying on an
# already-applied project breaks the mangled-name lookup. So setup always rebuilds the project FRESH; the
# decompile path only reads it (fast, no re-apply). The project lives under recovered/ghidra/re (gitignored).
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
SP="$ROOT/scripts/ghidra"
COV="$ROOT/reference/coverage"
PROJ="$ROOT/recovered/ghidra/re"
GH="$(command -v analyzeHeadless || echo "$HOME/Tools/ghidra/support/analyzeHeadless")"
SO="$ROOT/reference/ts_i386.so"

setup() {
  echo ">>> [1/3] refreshing the inventory (scripts/coverage/run.py)"
  python3 "$ROOT/scripts/coverage/run.py" >/dev/null
  echo ">>> [2/3] fresh import + analysis of ts_i386.so"
  rm -rf "$PROJ"; mkdir -p "$PROJ"
  echo ">>> [3/3] applying ApplyInventory (structs/vtables/engine API/FP constants/clean names)"
  "$GH" "$PROJ" re -import "$SO" -preScript DisableNoReturn.java -scriptPath "$SP" \
    -postScript ApplyInventory.java "$COV" 2>&1 | sed -nE "s/^INFO  ApplyInventory.java> //p" || true
  echo ">>> done. decompile a target:  scripts/ghidra/re.sh <Class>   (or: re.sh members  for the member skeleton)"
}

members() {
  if [ ! -d "$PROJ/re.rep" ]; then echo "no applied project — run: scripts/ghidra/re.sh setup" >&2; exit 1; fi
  echo ">>> DumpMembers — recovering transient member offset+type -> reference/coverage/members_auto.tsv"
  "$GH" "$PROJ" re -process ts_i386.so -noanalysis -scriptPath "$SP" \
    -postScript DumpMembers.java "$COV" 2>&1 | sed -nE "s/^INFO  DumpMembers.java> //p" || true
}

decompile() {
  if [ ! -d "$PROJ/re.rep" ]; then echo "no applied project — run: scripts/ghidra/re.sh setup" >&2; exit 1; fi
  local addrs=()
  for q in "$@"; do
    if [[ "$q" == 0x* ]]; then addrs+=("$q")
    else
      # resolve Class -> Ghidra method addresses (filemap.py prints '  <nm-hex> <symbol>'; Ghidra = nm+0x10000)
      while read -r hex; do addrs+=("$(printf '0x%x' $((0x$hex + 0x10000)))"); done < <(
        python3 "$ROOT/scripts/coverage/filemap.py" "$q" 2>/dev/null | awk '/^  [0-9a-f]+ /{print $1}')
    fi
  done
  [ ${#addrs[@]} -eq 0 ] && { echo "nothing to decompile for: $*  (unknown class/addr?)" >&2; exit 1; }
  # DecompAt emits each line prefixed 'INFO  DecompAt.java> ' — print only those, stripped.
  "$GH" "$PROJ" re -process ts_i386.so -noanalysis -scriptPath "$SP" \
    -postScript DecompAt.java "${addrs[*]}" 2>&1 | sed -nE 's/^INFO  DecompAt\.java> //p'
}

case "${1:-}" in
  setup|--setup|-s) setup ;;
  members|--members|-m) members ;;
  "" ) echo "usage: re.sh setup | re.sh members | re.sh <Class> | re.sh 0x<addr>..." ;;
  * ) decompile "$@" ;;
esac
