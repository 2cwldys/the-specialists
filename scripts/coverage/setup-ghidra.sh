#!/usr/bin/env bash
# setup-ghidra.sh — (re)build the persistent Ghidra projects used for ad-hoc RE.
#
# /tmp is tmpfs here (wiped on reboot), so the projects live under recovered/ghidra/
# (gitignored, on-disk, re-derivable). Run once per machine/session; idempotent
# (skips a project that already exists unless -f).
#
# Projects:
#   recovered/ghidra/server  <- reference/ts_i386.so   (project name: serverso)
#   recovered/ghidra/client  <- reference/client.dll   (project name: clientdll)
#
# Both import with the "Non-Returning Functions - Discovered" analyzer OFF
# (scripts/ghidra/DisableNoReturn.java) so decompiles don't truncate.
#
# Usage:  scripts/coverage/setup-ghidra.sh [-f]      (-f = force rebuild)
# Then:   analyzeHeadless recovered/ghidra/server serverso -process ts_i386.so \
#           -noanalysis -scriptPath scripts/ghidra -postScript DecompAt.java 0x<addr>
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
GH="${GHIDRA_HOME:-$HOME/Tools/ghidra}/support/analyzeHeadless"
SP="$ROOT/scripts/ghidra"
FORCE=0; [ "${1:-}" = "-f" ] && FORCE=1

build() {  # <projdir> <projname> <binary>
  local dir="$1" name="$2" bin="$3"
  if [ "$FORCE" = 1 ]; then rm -rf "$dir"; fi
  if [ -d "$dir/$name.rep" ]; then echo ">>> $name: exists (use -f to rebuild)"; return; fi
  mkdir -p "$dir"
  echo ">>> importing $bin -> $dir/$name (noreturn-discovery OFF)"
  "$GH" "$dir" "$name" -import "$ROOT/$bin" \
    -scriptPath "$SP" -preScript DisableNoReturn.java \
    >/tmp/ghidra-setup-$name.log 2>&1 \
    && echo "    ok ($dir/$name)" \
    || { echo "    FAILED — see /tmp/ghidra-setup-$name.log"; tail -3 /tmp/ghidra-setup-$name.log; }
}

build "$ROOT/recovered/ghidra/server" serverso reference/ts_i386.so
build "$ROOT/recovered/ghidra/client" clientdll reference/client.dll

# Apply the recovered inventory to the server program: real per-class member STRUCTS (from the save
# tables) + typed prototypes + entvars_t, so decompiles read `this->poweruptype` / `this->pev->origin`
# instead of `*(int *)(this + 0x84)`. (ApplyInventory is a superset of ApplyTypes; needs the coverage
# tsvs — run scripts/coverage/run.py first.) Idempotent — safe to re-run.
if [ -f "$ROOT/reference/coverage/members.tsv" ] && [ -d "$ROOT/recovered/ghidra/server/serverso.rep" ]; then
  echo ">>> applying recovered inventory to the server program (ApplyInventory: member structs + prototypes)"
  "$GH" "$ROOT/recovered/ghidra/server" serverso -process ts_i386.so -noanalysis \
    -scriptPath "$SP" -postScript ApplyInventory.java "$ROOT/reference/coverage" \
    >/tmp/ghidra-applyinv.log 2>&1 && grep -h "ApplyInventory:" /tmp/ghidra-applyinv.log || echo "    (ApplyInventory failed — see /tmp/ghidra-applyinv.log)"
fi
echo ">>> done. (projects under recovered/ghidra/ — gitignored, survive reboot)"
echo "    decompile by addr:  analyzeHeadless recovered/ghidra/server serverso -process ts_i386.so -noanalysis -scriptPath scripts/ghidra -postScript DecompAt.java 0x<addr>"
echo "    resolve a vtable slot:  python3 scripts/coverage/vtcall.py <Class> 0x<off>"
