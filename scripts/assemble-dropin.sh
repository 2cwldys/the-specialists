#!/usr/bin/env bash
# assemble-dropin.sh — lay out the reTS patch for a stock The Specialists 3.0 install from binaries that
# are already built. TS 3.0 is the base; the patch replaces only what reTS changes, under TS's own file
# names, so TS's liblist.gam keeps working unchanged. Used by scripts/package.sh (local builds) and by
# CI's package job (its artifacts).
#
# Usage: scripts/assemble-dropin.sh [--debug] [--amd64 <server.so> <client.so>] [--arm64 <server.so>
#            <client.so>] [--win64 <server.dll> <client.dll>] <rev> <i386-server.so> <i386-client.so> \
#            <win32-server.dll> <win32-client.dll> <outdir>
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
#   ts/dlls/ts_amd64.so ts_arm64.so       64-bit servers (--amd64/--arm64/--win64), under the names Xash
#   ts/dlls/mp_amd64.dll                  builds from TS's liblist (base name + arch); Steam's 32-bit
#   ts/cl_dlls/client_amd64.so            engine never asks for them, and none replaces a TS 3.0 file
#     client_arm64.so client_amd64.dll
#   ts/delta.lst                          the 25th-anniversary engine's field widths
#   ts/resource/trackerscheme.res         loads Valve's Linux fonts (Fira Sans for Trebuchet MS)
# Add a content file to CONTENT only once it is confirmed working in a real client.
set -euo pipefail

usage() { sed -n '2,32p' "$0" | sed 's/^# \{0,1\}//'; exit 2; }

DEBUG=0
X64_SO=""; X64_CL=""; A64_SO=""; A64_CL=""; W64_DLL=""; W64_CL=""
while [ $# -gt 0 ]; do
  case "$1" in
    --debug) DEBUG=1; shift ;;
    --amd64) X64_SO="$2"; X64_CL="$3"; shift 3 ;;
    --arm64) A64_SO="$2"; A64_CL="$3"; shift 3 ;;
    --win64) W64_DLL="$2"; W64_CL="$3"; shift 3 ;;
    --*) usage ;;
    *) break ;;
  esac
done
[ $# -eq 6 ] || usage
REV="$1"; SO="$2"; CLSO="$3"; DLL="$4"; CLDLL="$5"; OUT="$6"
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
CONTENT=(delta.lst resource/trackerscheme.res)
SERVER_SO=(ts.so ts_i386.so ts_i686.so)

for f in "$SO" "$CLSO" "$DLL" "$CLDLL" $X64_SO $X64_CL $A64_SO $A64_CL $W64_DLL $W64_CL; do
  [ -f "$f" ] || { echo "ERROR: missing $f" >&2; exit 1; }
done

NAME="reTS-$REV"; [ $DEBUG = 1 ] && NAME="reTS-$REV-debug"
PKG="$OUT/$NAME"
if [ -e "$PKG" ] || [ -e "$PKG.zip" ]; then
  echo "ERROR: $PKG exists; remove it or package another commit" >&2
  exit 1
fi
mkdir -p "$PKG/ts/dlls" "$PKG/ts/cl_dlls" "$PKG/ts/resource"

# GNU binutils for x86 (the i386 and amd64 builds), llvm's for anything GNU's cannot read (aarch64 on an
# x86 host). Checked per file so the x86 output stays what GNU strip makes.
tool() { # <gnu> <llvm> <file>: GNU's for an x86 ELF, else the first llvm-* found
  case "$(readelf -h "$3" | awk -F: '/Machine:/{print $2}')" in
    *80386*|*X86-64*) echo "$1"; return ;;
  esac
  for t in "$2" "$2"-21 "$2"-20 "$2"-19 "$2"-18 "$2"-17; do
    command -v "$t" >/dev/null 2>&1 && { echo "$t"; return; }
  done
  echo "ERROR: no $2 to read $3" >&2; exit 1
}
exports() { "$(tool nm llvm-nm "$1")" -D --defined-only "$1" | awk '{print $NF}' | sort; }
put_so() { # <in> <out>: Release strips (refusing a changed export list), Debug copies
  if [ $DEBUG = 1 ]; then cp "$1" "$2"; return; fi
  local st; st="$(tool strip llvm-strip "$1")"
  "$st" --strip-unneeded -o "$2" "$1"
  if ! diff <(exports "$1") <(exports "$2") >/dev/null; then
    echo "ERROR: stripping $1 changed its dynamic exports" >&2; exit 1
  fi
}

FILES=()
SRC=()   # "<path in ts/>\t<source>" for the manifest
add() { FILES+=("$1"); SRC+=("$1	$(basename "$2") @$REV"); }

put_so "$SO" "$PKG/ts/dlls/ts.so"
for name in "${SERVER_SO[@]}"; do
  [ "$name" = ts.so ] || cp "$PKG/ts/dlls/ts.so" "$PKG/ts/dlls/$name"
  add "dlls/$name" "$SO"
done
cp "$DLL" "$PKG/ts/dlls/mp.dll"; add dlls/mp.dll "$DLL"
put_so "$CLSO" "$PKG/ts/cl_dlls/client.so"; add cl_dlls/client.so "$CLSO"
cp "$CLDLL" "$PKG/ts/cl_dlls/client.dll"; add cl_dlls/client.dll "$CLDLL"
if [ -n "$X64_SO" ]; then
  put_so "$X64_SO" "$PKG/ts/dlls/ts_amd64.so"; add dlls/ts_amd64.so "$X64_SO"
  put_so "$X64_CL" "$PKG/ts/cl_dlls/client_amd64.so"; add cl_dlls/client_amd64.so "$X64_CL"
fi
if [ -n "$A64_SO" ]; then
  put_so "$A64_SO" "$PKG/ts/dlls/ts_arm64.so"; add dlls/ts_arm64.so "$A64_SO"
  put_so "$A64_CL" "$PKG/ts/cl_dlls/client_arm64.so"; add cl_dlls/client_arm64.so "$A64_CL"
fi
if [ -n "$W64_DLL" ]; then
  cp "$W64_DLL" "$PKG/ts/dlls/mp_amd64.dll"; add dlls/mp_amd64.dll "$W64_DLL"
  cp "$W64_CL" "$PKG/ts/cl_dlls/client_amd64.dll"; add cl_dlls/client_amd64.dll "$W64_CL"
fi
for f in "${CONTENT[@]}"; do
  cp "$ROOT/game/ts/$f" "$PKG/ts/$f"
  FILES+=("$f"); SRC+=("$f	game/ts/$f @$REV")
done

if [ $DEBUG = 1 ]; then
  # Each PDB beside the dll it belongs to, under the file name that dll records; the 64-bit dll's PDB
  # has the same name as the 32-bit one's, so it gets its own folder.
  for spec in "$DLL:dlls" "$CLDLL:cl_dlls" ${W64_DLL:+"$W64_DLL:dlls/amd64-pdb"} ${W64_CL:+"$W64_CL:cl_dlls/amd64-pdb"}; do
    dll="${spec%%:*}"; dir="${spec##*:}"; pdb="${dll%.dll}.pdb"
    [ -f "$pdb" ] || { echo "ERROR: --debug needs $pdb beside $dll" >&2; exit 1; }
    mkdir -p "$PKG/ts/$dir"
    cp "$pdb" "$PKG/ts/$dir/$(basename "$pdb")"
    SRC+=("$dir/$(basename "$pdb")	$(basename "$pdb") @$REV")
  done
fi

sha() { sha256sum "$1" | cut -d' ' -f1; }
{
  printf 'path\tsource\tsha256\n'
  for row in "${SRC[@]}"; do
    p="${row%%	*}"
    printf 'ts/%s\t%s\n' "$row" "$(sha "$PKG/ts/$p")"
  done
} > "$PKG/MANIFEST.tsv"

if [ $DEBUG = 1 ]; then
  KIND="Debug build (not stripped, with PDBs; for debugging, not for release)"
else
  KIND="Release build"
fi
X64_NOTE=""
if [ -n "$X64_SO$A64_SO$W64_DLL" ]; then
  X64_NOTE="
The *_amd64 and *_arm64 files are the 64-bit builds, for engines that run 64-bit game libraries (Xash3D
on 64-bit systems, 64-bit ReHLDS). They are named the way those engines build the name from TS's
liblist.gam. Steam's Half-Life is 32-bit and never loads them, and none of them replaces a TS 3.0 file."
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
client.so; no engine lets a mod rename it. ts.so and client.so are new: TS 3.0 shipped neither.
${X64_NOTE}
MANIFEST.tsv lists every file with its sha256.
EOF

( cd "$PKG" && zip -qr "../$NAME.zip" ts README.txt MANIFEST.tsv )
echo ">>> $PKG"
echo ">>> $OUT/$NAME.zip"
column -t -s $'\t' "$PKG/MANIFEST.tsv"
