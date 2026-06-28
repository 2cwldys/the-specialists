#!/usr/bin/env bash
# re.sh — one entry point for the full server RE pipeline. Runs every pass, then decompiles a target
# so it reads like source (members/vtables/engine API/FP constants/clean names — see ApplyInventory.java).
#
#   scripts/ghidra/re.sh setup            # (re)build the applied project: run.py + import + ApplyInventory.
#                                         #   Do this once, and after coverage/script changes.
#   scripts/ghidra/re.sh members          # DumpMembers on the applied project -> members_auto.tsv skeleton
#   scripts/ghidra/re.sh <Class>          # decompile every method of a class (resolves addrs via filemap.py)
#   scripts/ghidra/re.sh 0x7ea30 0x7decc  # decompile by Ghidra address
#   scripts/ghidra/re.sh refs <Class|addr># who CALLS/uses it (DIRECT xrefs) — decompile every referencing fn
#   scripts/ghidra/re.sh vrefs <Method>   # who calls it through a VTABLE (virtual dispatch — refs can't see these)
#   scripts/ghidra/re.sh export <Class>   # write the original class's decompile to recovered/decomp/<Class>.c
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

# Strip the analyzeHeadless log noise, keeping ONLY our script's println output — including
# MULTI-LINE messages (e.g. a whole decompiled function). Ghidra logs each println as one event:
#   "INFO  <tag>> <first line>\n<continuation>\n…\n<last line> (GhidraScript)"
# only the FIRST physical line carries the "INFO  <tag>> " prefix and only the LAST carries the
# " (GhidraScript)" suffix. A naive `sed -n 's/^INFO  <tag>> //p'` therefore drops every
# continuation line (the body of the C). Track the block open/close instead.
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

setup() {
  echo ">>> [1/3] refreshing the inventory (scripts/coverage/run.py)"
  python3 "$ROOT/scripts/coverage/run.py" >/dev/null
  echo ">>> [2/3] fresh import + analysis of ts_i386.so"
  rm -rf "$PROJ"; mkdir -p "$PROJ"
  echo ">>> [3/3] applying ApplyInventory (structs/vtables/engine API/FP constants/clean names)"
  "$GH" "$PROJ" re -import "$SO" -preScript DisableNoReturn.java -scriptPath "$SP" \
    -postScript ApplyInventory.java "$COV" 2>&1 | _filter ApplyInventory.java || true
  echo ">>> done. decompile a target:  scripts/ghidra/re.sh <Class>   (or: re.sh members  for the member skeleton)"
}

members() {
  if [ ! -d "$PROJ/re.rep" ]; then echo "no applied project — run: scripts/ghidra/re.sh setup" >&2; exit 1; fi
  echo ">>> DumpMembers — recovering transient member offset+type -> reference/coverage/members_auto.tsv"
  "$GH" "$PROJ" re -process ts_i386.so -noanalysis -scriptPath "$SP" \
    -postScript DumpMembers.java "$COV" 2>&1 | _filter DumpMembers.java || true
  echo ">>> curate_members — naming the skeleton from our source's RE'd offsets (-> members_re.tsv)"
  python3 "$ROOT/scripts/coverage/curate_members.py" || true
  echo ">>> now re-apply to pick up the names:  scripts/ghidra/re.sh setup"
}

need_project() { [ -d "$PROJ/re.rep" ] || { echo "no applied project — run: scripts/ghidra/re.sh setup" >&2; exit 1; }; }

# resolve args (a Class name and/or 0x<addr>) to Ghidra addresses (Class -> its methods via filemap.py)
resolve() {
  local out=()
  for q in "$@"; do
    if [[ "$q" == 0x* ]]; then out+=("$q")
    else while read -r hex; do out+=("$(printf '0x%x' $((0x$hex + 0x10000)))"); done < <(
      python3 "$ROOT/scripts/coverage/filemap.py" "$q" 2>/dev/null | awk '/^  [0-9a-f]+ /{print $1}'); fi
  done
  echo "${out[*]}"
}

decompile() {
  need_project; local addrs; addrs=$(resolve "$@")
  [ -z "$addrs" ] && { echo "nothing to decompile for: $*  (unknown class/addr?)" >&2; exit 1; }
  "$GH" "$PROJ" re -process ts_i386.so -noanalysis -scriptPath "$SP" \
    -postScript DecompAt.java "$addrs" 2>&1 | _filter DecompAt.java
}

refs() {
  need_project; local addrs; addrs=$(resolve "$@")
  [ -z "$addrs" ] && { echo "no target for: $*" >&2; exit 1; }
  "$GH" "$PROJ" re -process ts_i386.so -noanalysis -scriptPath "$SP" \
    -postScript DecompRefs.java "$addrs" 2>&1 | _filter DecompRefs.java
}

export_class() {
  need_project; local out="$ROOT/recovered/decomp"; mkdir -p "$out"
  "$GH" "$PROJ" re -process ts_i386.so -noanalysis -scriptPath "$SP" \
    -postScript DecompExport.java "$out" "$1" 2>&1 | _filter DecompExport.java
  echo ">>> .c files written under $out/  (one per class matching '$1')"
}

case "${1:-}" in
  setup|--setup|-s) setup ;;
  members|--members|-m) members ;;
  refs) shift; refs "$@" ;;
  vrefs) shift; python3 "$ROOT/scripts/coverage/vrefs.py" "$@" ;;  # virtual caller-finder (objdump, no project)
  export) shift; export_class "$@" ;;
  "" ) echo "usage: re.sh setup | members | <Class> | 0x<addr>... | refs <Class|addr> | vrefs <Method> | export <Class>" ;;
  * ) decompile "$@" ;;
esac
