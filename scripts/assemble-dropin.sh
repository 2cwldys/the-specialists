#!/usr/bin/env bash
# assemble-dropin.sh — lay out the reTS patch for a stock The Specialists 3.0 install from binaries that
# are already built. TS 3.0 is the base; the patch replaces only what reTS changes, under TS's own file
# names, so TS's liblist.gam keeps working unchanged. Used by scripts/package.sh (local builds) and by
# CI's package job (its artifacts).
#
# Usage: scripts/assemble-dropin.sh [--debug] <rev> <i386-server.so> <i386-client.so> \
#                                   <win32-server.dll> <win32-client.dll> <outdir>
# Output: <outdir>/reTS-<rev>/{ts/..., README.txt, MANIFEST.tsv} and <outdir>/reTS-<rev>.zip
#
#   Release (the default): give it Release builds. The Linux libraries are stripped to their dynamic
#   exports (the engine's, Metamod-R's, Ham Sandwich's and reTSX's entry points); the script refuses the
#   result if any export changed. The Windows dlls already export only their entry points.
#   --debug: give it Debug builds. Nothing is stripped, each dll's PDB (hl.pdb, client.pdb, beside the
#   dll it was built with) is included under the name the dll records, and the output is
#   reTS-<rev>-debug. Not for release.
#
#   ts/dlls/ts.so ts_i386.so ts_i686.so   reTS server, i386. Steam's current engine loads ts.so (it cuts
#                                         liblist's gamedll_linux at the first "_"), older engines and
#                                         liblist name ts_i386.so, Metamod-R prefers ts_i686.so.
#   ts/dlls/mp.dll                        reTS server, Win32 (liblist's gamedll)
#   ts/cl_dlls/client.so, client.dll      reTS client; every engine loads the client by this fixed name
#   ts/delta.lst                          the 25th-anniversary engine's field widths
#   ts/resource/trackerscheme.res         loads Valve's Linux fonts (Fira Sans for Trebuchet MS)
# Add a content file to CONTENT only once it is confirmed working in a real client.
set -euo pipefail

DEBUG=0
if [ "${1:-}" = "--debug" ]; then DEBUG=1; shift; fi
if [ $# -ne 6 ]; then
  sed -n '2,25p' "$0" | sed 's/^# \{0,1\}//'
  exit 2
fi
REV="$1"; SO="$2"; CLSO="$3"; DLL="$4"; CLDLL="$5"; OUT="$6"
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
CONTENT=(delta.lst resource/trackerscheme.res)
SERVER_SO=(ts.so ts_i386.so ts_i686.so)

for f in "$SO" "$CLSO" "$DLL" "$CLDLL"; do
  [ -f "$f" ] || { echo "ERROR: missing $f" >&2; exit 1; }
done

NAME="reTS-$REV"; [ $DEBUG = 1 ] && NAME="reTS-$REV-debug"
PKG="$OUT/$NAME"
if [ -e "$PKG" ] || [ -e "$PKG.zip" ]; then
  echo "ERROR: $PKG exists; remove it or package another commit" >&2
  exit 1
fi
mkdir -p "$PKG/ts/dlls" "$PKG/ts/cl_dlls" "$PKG/ts/resource"

put_so() { # <in> <out>: Release strips (refusing a changed export list), Debug copies
  if [ $DEBUG = 1 ]; then cp "$1" "$2"; return; fi
  strip --strip-unneeded -o "$2" "$1"
  if ! diff <(nm -D --defined-only "$1" | awk '{print $3}' | sort) \
            <(nm -D --defined-only "$2" | awk '{print $3}' | sort) >/dev/null; then
    echo "ERROR: stripping $1 changed its dynamic exports" >&2; exit 1
  fi
}
put_so "$SO" "$PKG/ts/dlls/ts.so"
for name in "${SERVER_SO[@]}"; do [ "$name" = ts.so ] || cp "$PKG/ts/dlls/ts.so" "$PKG/ts/dlls/$name"; done
cp "$DLL" "$PKG/ts/dlls/mp.dll"
put_so "$CLSO" "$PKG/ts/cl_dlls/client.so"
cp "$CLDLL" "$PKG/ts/cl_dlls/client.dll"
for f in "${CONTENT[@]}"; do cp "$ROOT/game/ts/$f" "$PKG/ts/$f"; done

FILES=()
for name in "${SERVER_SO[@]}"; do FILES+=("dlls/$name"); done
FILES+=(dlls/mp.dll cl_dlls/client.so cl_dlls/client.dll "${CONTENT[@]}")

PDBS=()
if [ $DEBUG = 1 ]; then
  for pair in "$DLL:dlls" "$CLDLL:cl_dlls"; do
    dll="${pair%%:*}"; dir="${pair##*:}"; pdb="${dll%.dll}.pdb"
    [ -f "$pdb" ] || { echo "ERROR: --debug needs $pdb beside $dll" >&2; exit 1; }
    cp "$pdb" "$PKG/ts/$dir/$(basename "$pdb")"
    PDBS+=("$dir/$(basename "$pdb")")
  done
fi

sha() { sha256sum "$1" | cut -d' ' -f1; }
{
  printf 'path\tsource\tsha256\n'
  for name in "${SERVER_SO[@]}"; do printf 'ts/dlls/%s\t%s @%s\t%s\n' "$name" "$(basename "$SO")" "$REV" "$(sha "$PKG/ts/dlls/$name")"; done
  printf 'ts/dlls/mp.dll\t%s @%s\t%s\n' "$(basename "$DLL")" "$REV" "$(sha "$PKG/ts/dlls/mp.dll")"
  printf 'ts/cl_dlls/client.so\t%s @%s\t%s\n' "$(basename "$CLSO")" "$REV" "$(sha "$PKG/ts/cl_dlls/client.so")"
  printf 'ts/cl_dlls/client.dll\t%s @%s\t%s\n' "$(basename "$CLDLL")" "$REV" "$(sha "$PKG/ts/cl_dlls/client.dll")"
  for f in "${CONTENT[@]}"; do printf 'ts/%s\tgame/ts/%s @%s\t%s\n' "$f" "$f" "$REV" "$(sha "$PKG/ts/$f")"; done
  for f in "${PDBS[@]}"; do printf 'ts/%s\t%s @%s\t%s\n' "$f" "$(basename "$f")" "$REV" "$(sha "$PKG/ts/$f")"; done
} > "$PKG/MANIFEST.tsv"

if [ $DEBUG = 1 ]; then
  KIND="Debug build (not stripped, with PDBs; for debugging, not for release)"
else
  KIND="Release build"
fi
cat > "$PKG/README.txt" <<EOF
reTS ${REV}: a patch for The Specialists 3.0
${KIND}

TS 3.0 is the base. This replaces only the files reTS changes, under TS's own names, so TS's
liblist.gam is left as it is:

$(for f in "${FILES[@]}"; do printf '  ts/%s\n' "$f"; done)

To install, copy the ts folder over your Half-Life/ts folder and restart Half-Life. Copy the files
listed above somewhere first if you want to go back to TS 3.0; putting them back undoes the patch.

The server is installed as ts.so, ts_i386.so and ts_i686.so because the engines pick different names:
Steam's current Linux engine loads ts.so, older engines load ts_i386.so (liblist's gamedll_linux), and
Metamod-R prefers ts_i686.so. Windows loads mp.dll. The client is always cl_dlls/client.dll or
client.so; no engine lets a mod rename it.

MANIFEST.tsv lists every file with its sha256.
EOF

( cd "$PKG" && zip -qr "../$NAME.zip" ts README.txt MANIFEST.tsv )
echo ">>> $PKG"
echo ">>> $OUT/$NAME.zip"
column -t -s $'\t' "$PKG/MANIFEST.tsv"
