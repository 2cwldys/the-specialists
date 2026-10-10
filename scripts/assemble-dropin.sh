#!/usr/bin/env bash
# assemble-dropin.sh — lay out the reTS patch for a stock The Specialists 3.0 install from binaries that
# are already built. TS 3.0 is the base; the patch replaces only what reTS changes, under TS's own file
# names, so TS's liblist.gam keeps working unchanged. Used by scripts/package.sh (local builds) and by
# CI's package job (its artifacts).
#
# Usage: scripts/assemble-dropin.sh <rev> <i386-server.so> <i386-client.so> <win32-server.dll> \
#                                   <win32-client.dll> <outdir>
# Output: <outdir>/reTS-<rev>/{ts/..., README.txt, MANIFEST.tsv}
#         and <outdir>/reTS-<rev>.zip, plus <outdir>/reTS-<rev>-symbols/ (the unstripped Linux
#         libraries, for decoding crash addresses; not for release)
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

if [ $# -ne 6 ]; then
  sed -n '2,20p' "$0" | sed 's/^# \{0,1\}//'
  exit 2
fi
REV="$1"; SO="$2"; CLSO="$3"; DLL="$4"; CLDLL="$5"; OUT="$6"
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
CONTENT=(delta.lst resource/trackerscheme.res)
SERVER_SO=(ts.so ts_i386.so ts_i686.so)

for f in "$SO" "$CLSO" "$DLL" "$CLDLL"; do
  [ -f "$f" ] || { echo "ERROR: missing $f" >&2; exit 1; }
done

PKG="$OUT/reTS-$REV"
if [ -e "$PKG" ] || [ -e "$PKG.zip" ] || [ -e "$OUT/reTS-$REV-symbols" ]; then
  echo "ERROR: $PKG exists; remove it or package another commit" >&2
  exit 1
fi
mkdir -p "$PKG/ts/dlls" "$PKG/ts/cl_dlls" "$PKG/ts/resource"

# The Linux libraries ship stripped: no static symbol table, only the dynamic exports the engine and
# plugins use (GiveFnptrsToDll, GetEntityAPI2, the entity factories, ReTS_GetInterface, the client's
# HUD_* and friends). The unstripped ones go to <outdir>/reTS-<rev>-symbols/ to decode crash addresses.
# The Windows builds export only their entry points already, as TS 3.0's do.
SYM="$OUT/reTS-$REV-symbols"
mkdir -p "$SYM/dlls" "$SYM/cl_dlls"
cp "$SO" "$SYM/dlls/ts.so"
cp "$CLSO" "$SYM/cl_dlls/client.so"
strip_so() { # <in> <out>: strip, and refuse if a dynamic export went missing
  strip --strip-unneeded -o "$2" "$1"
  if ! diff <(nm -D --defined-only "$1" | awk '{print $3}' | sort) \
            <(nm -D --defined-only "$2" | awk '{print $3}' | sort) >/dev/null; then
    echo "ERROR: stripping $1 changed its dynamic exports" >&2; exit 1
  fi
}
strip_so "$SO" "$PKG/ts/dlls/ts.so"
for name in "${SERVER_SO[@]}"; do [ "$name" = ts.so ] || cp "$PKG/ts/dlls/ts.so" "$PKG/ts/dlls/$name"; done
cp "$DLL" "$PKG/ts/dlls/mp.dll"
strip_so "$CLSO" "$PKG/ts/cl_dlls/client.so"
cp "$CLDLL" "$PKG/ts/cl_dlls/client.dll"
for f in "${CONTENT[@]}"; do cp "$ROOT/game/ts/$f" "$PKG/ts/$f"; done

FILES=()
for name in "${SERVER_SO[@]}"; do FILES+=("dlls/$name"); done
FILES+=(dlls/mp.dll cl_dlls/client.so cl_dlls/client.dll "${CONTENT[@]}")

sha() { sha256sum "$1" | cut -d' ' -f1; }
{
  printf 'path\tsource\tsha256\n'
  for name in "${SERVER_SO[@]}"; do printf 'ts/dlls/%s\t%s @%s\t%s\n' "$name" "$(basename "$SO")" "$REV" "$(sha "$PKG/ts/dlls/$name")"; done
  printf 'ts/dlls/mp.dll\t%s @%s\t%s\n' "$(basename "$DLL")" "$REV" "$(sha "$PKG/ts/dlls/mp.dll")"
  printf 'ts/cl_dlls/client.so\t%s @%s\t%s\n' "$(basename "$CLSO")" "$REV" "$(sha "$PKG/ts/cl_dlls/client.so")"
  printf 'ts/cl_dlls/client.dll\t%s @%s\t%s\n' "$(basename "$CLDLL")" "$REV" "$(sha "$PKG/ts/cl_dlls/client.dll")"
  for f in "${CONTENT[@]}"; do printf 'ts/%s\tgame/ts/%s @%s\t%s\n' "$f" "$f" "$REV" "$(sha "$PKG/ts/$f")"; done
} > "$PKG/MANIFEST.tsv"

cat > "$PKG/README.txt" <<EOF
reTS ${REV}: a patch for The Specialists 3.0

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

( cd "$PKG" && zip -qr "../reTS-$REV.zip" ts README.txt MANIFEST.tsv )
{
  printf 'path\tsha256\tstripped copy in reTS-%s\n' "$REV"
  printf 'dlls/ts.so\t%s\t%s\n' "$(sha "$SYM/dlls/ts.so")" "$(sha "$PKG/ts/dlls/ts.so")"
  printf 'cl_dlls/client.so\t%s\t%s\n' "$(sha "$SYM/cl_dlls/client.so")" "$(sha "$PKG/ts/cl_dlls/client.so")"
} > "$SYM/MANIFEST.tsv"
echo ">>> $SYM (unstripped, not for release)"
echo ">>> $PKG"
echo ">>> $OUT/reTS-$REV.zip"
column -t -s $'\t' "$PKG/MANIFEST.tsv"
